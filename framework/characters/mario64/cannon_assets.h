#pragma once
// Bounded decoder for the US ROM's common0 MIO0/Fast3D object bank. No models
// or textures are compiled into the adapter. Display-list addresses are format
// metadata checked against n64decomp/sm64's pinned actor declarations.
#include <stdexcept>

namespace MarioCannonAssets {
struct V {
    float p[3], n[3], c[3], uv[2];
};

static std::vector<uint8_t> bank, atlas;
static std::vector<V> parts[9];

static uint32_t u32(const std::vector<uint8_t> &b, size_t a) {
    if (a + 4 > b.size())
        throw std::runtime_error("ROM bounds");
    return uint32_t(b[a]) << 24 | uint32_t(b[a + 1]) << 16 | uint32_t(b[a + 2]) << 8 | b[a + 3];
}

static int s16(size_t a) {
    if (a + 2 > bank.size())
        throw std::runtime_error("vertex bounds");
    return int16_t(uint16_t(bank[a]) << 8 | bank[a + 1]);
}

static size_t address(uint32_t a) {
    if ((a >> 24) != 8 || (a & 0xffffff) >= bank.size())
        throw std::runtime_error("object segment");
    return a & 0xffffff;
}

struct Texture {
    uint32_t offset;
    int width, height;
};

static const Texture textures[] = {{0x49b8, 32, 32},  {0x58a8, 32, 32},  {0x4058, 32, 32},
                                   {0x1fa60, 32, 64}, {0x20a60, 32, 64}, {0x21a60, 32, 32},
                                   {0x22260, 32, 32}};
static constexpr int width = 512, height = 64;

static void decode(const std::vector<uint8_t> &rom) {
    size_t base = 2040320;
    if (u32(rom, base) != 0x4d494f30)
        throw std::runtime_error("MIO0 header");
    size_t size = u32(rom, base + 4), compressed = base + u32(rom, base + 8),
           raw = base + u32(rom, base + 12), bits = base + 16;
    if (!size || size > 1024 * 1024)
        throw std::runtime_error("MIO0 size");
    bank.clear();
    bank.reserve(size);
    uint8_t flags = 0, mask = 0;
    while (bank.size() < size) {
        if (!mask) {
            if (bits >= rom.size())
                throw std::runtime_error("MIO0 flags");
            flags = rom[bits++];
            mask = 128;
        }
        if (flags & mask) {
            if (raw >= rom.size())
                throw std::runtime_error("MIO0 literal");
            bank.push_back(rom[raw++]);
        } else {
            if (compressed + 2 > rom.size())
                throw std::runtime_error("MIO0 copy");
            unsigned word = (rom[compressed] << 8) | rom[compressed + 1];
            compressed += 2;
            size_t count = (word >> 12) + 3, back = (word & 4095) + 1;
            if (back > bank.size() || count > size - bank.size())
                throw std::runtime_error("MIO0 reference");
            while (count--) {
                auto b = bank[bank.size() - back];
                bank.push_back(b);
            }
        }
        mask >>= 1;
    }
    atlas.assign(width * height * 4, 255);
    for (unsigned t = 0; t < sizeof(textures) / sizeof(*textures); t++) {
        auto a = textures[t];
        if (a.offset + a.width * a.height * 2 > bank.size())
            throw std::runtime_error("texture bounds");
        for (int y = 0; y < a.height; y++)
            for (int x = 0; x < a.width; x++) {
                size_t p = a.offset + 2 * (y * a.width + x);
                unsigned v = (bank[p] << 8) | bank[p + 1];
                size_t dest = 4 * (y * width + t * 64 + x);
                atlas[dest] = ((v >> 11) & 31) * 255 / 31;
                atlas[dest + 1] = ((v >> 6) & 31) * 255 / 31;
                atlas[dest + 2] = ((v >> 1) & 31) * 255 / 31;
                atlas[dest + 3] = (v & 1) ? 255 : 0;
            }
    }
}

struct Reader {
    V cache[16]{};
    bool valid[16] = {}, textured = false, lighting = true;
    int tile = -1;
    float light[3] = {1, 1, 1};
    unsigned budget = 4096;
    std::vector<V> vertices;

    void triangle(uint32_t word) {
        if (vertices.size() >= 3000)
            throw std::runtime_error("triangle limit");
        for (int shift : {16, 8, 0}) {
            unsigned index = (word >> shift) & 255;
            if (index % 10 || index / 10 >= 16 || !valid[index / 10])
                throw std::runtime_error("triangle index");
            V v = cache[index / 10];
            for (int j = 0; j < 3; j++)
                v.c[j] = lighting ? light[j] : 1;
            if (textured) {
                if (tile < 0)
                    throw std::runtime_error("unknown texture");
                auto t = textures[tile];
                v.uv[0] =
                    (tile * 64 + std::clamp(v.uv[0] / 32.f, 0.f, float(t.width - 1)) + .5f) / width;
                v.uv[1] = (std::clamp(v.uv[1] / 32.f, 0.f, float(t.height - 1)) + .5f) / height;
            } else {
                v.uv[0] = 511.5f / width;
                v.uv[1] = .5f / height;
            }
            vertices.push_back(v);
        }
    }

    void list(uint32_t pointer, unsigned depth = 0) {
        if (depth > 8)
            throw std::runtime_error("display recursion");
        size_t p = address(pointer);
        while (budget--) {
            uint32_t a = u32(bank, p), b = u32(bank, p + 4);
            p += 8;
            unsigned op = a >> 24;
            switch (op) {
            case 0xb8:
                return;
            case 0x06:
                list(b, depth + 1);
                if ((a >> 16) & 255)
                    return;
                break;
            case 0x04: {
                unsigned count = ((a >> 20) & 15) + 1, start = (a >> 16) & 15;
                size_t at = address(b);
                if (start + count > 16 || at + count * 16 > bank.size())
                    throw std::runtime_error("vertices");
                for (unsigned k = 0; k < count; k++) {
                    V &v = cache[start + k];
                    size_t q = at + k * 16;
                    for (int j = 0; j < 3; j++) {
                        v.p[j] = float(s16(q + j * 2));
                        v.n[j] = int8_t(bank[q + 12 + j]) / 127.f;
                    }
                    v.uv[0] = float(s16(q + 8));
                    v.uv[1] = float(s16(q + 10));
                    valid[start + k] = true;
                }
                break;
            }
            case 0xbf:
                triangle(b);
                break;
            case 0xbb:
                textured = (a & 255) != 0;
                break;
            case 0xfd: {
                size_t at = address(b);
                tile = -1;
                for (int t = 0; t < 7; t++)
                    if (textures[t].offset == at)
                        tile = t;
                break;
            }
            case 0x03:
                if (((a >> 16) & 255) == 0x86) {
                    size_t at = address(b);
                    if (at + 3 > bank.size())
                        throw std::runtime_error("light");
                    for (int j = 0; j < 3; j++)
                        light[j] = bank[at + j] / 255.f;
                }
                break;
            case 0xb6:
                if (b & 0x20000)
                    lighting = false;
                break;
            case 0xb7:
                if (b & 0x20000)
                    lighting = true;
                break;
            case 0xe7:
            case 0xe6:
            case 0xe8:
            case 0xf3:
            case 0xf5:
            case 0xf2:
            case 0xfc:
                break;
            default:
                throw std::runtime_error("unsupported object display command");
            }
        }
        throw std::runtime_error("display budget");
    }
};

static bool initialize(const std::vector<uint8_t> &rom) {
    try {
        decode(rom);
        const uint32_t addresses[] = {0x080057f8, 0x08006660, 0x08022d78, 0x080048e0, 0x08023270,
                                      0x08023378, 0x08022b58, 0x08022b88, 0x08023480};
        for (int p = 0; p < 9; p++) {
            Reader r;
            r.list(addresses[p]);
            parts[p] = std::move(r.vertices);
        }
        return true;
    } catch (const std::exception &e) {
        if (report)
            report(e.what());
        bank.clear();
        atlas.clear();
        for (auto &p : parts)
            p.clear();
        return false;
    }
}

static bool append(SC_Mesh *m, int part, float yaw, float scale, float x, float y, float z) {
    if (m->numTrianglesUsed + parts[part].size() / 3 > m->capacityTriangles)
        return false;
    float cs = cosf(yaw), sn = sinf(yaw);
    size_t index = m->numTrianglesUsed * 3;
    for (const V &v : parts[part]) {
        float px = v.p[0] * scale, pz = v.p[2] * scale;
        m->position[index * 3] = x + px * cs + pz * sn;
        m->position[index * 3 + 1] = y + v.p[1] * scale;
        m->position[index * 3 + 2] = z - px * sn + pz * cs;
        m->normal[index * 3] = v.n[0] * cs + v.n[2] * sn;
        m->normal[index * 3 + 1] = v.n[1];
        m->normal[index * 3 + 2] = -v.n[0] * sn + v.n[2] * cs;
        memcpy(m->color + index * 3, v.c, 12);
        memcpy(m->uv + index * 2, v.uv, 8);
        index++;
    }
    m->numTrianglesUsed = uint32_t(index / 3);
    return true;
}

struct Matrix {
    float a[4][3]{};
};

static Matrix compose(const Matrix &p, const Matrix &c) {
    Matrix r{};
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) {
            r.a[i][j] = i == 3 ? p.a[3][j] : 0;
            for (int k = 0; k < 3; k++)
                r.a[i][j] += c.a[i][k] * p.a[k][j];
        }
    return r;
}

static Matrix rotation(float x, float y, float z) {
    float sx = sinf(x), cx = cosf(x), sy = sinf(y), cy = cosf(y), sz = sinf(z), cz = cosf(z);
    return {{{cy * cz, cy * sz, -sy},
             {sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy},
             {cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy},
             {0, 0, 0}}};
}

static bool appendPose(SC_Mesh *m, int part, const Matrix &t) {
    unsigned start = m->numTrianglesUsed * 3;
    if (!append(m, part, 0, 1, 0, 0, 0))
        return false;
    for (unsigned i = start; i < m->numTrianglesUsed * 3; i++) {
        float p[3], n[3];
        memcpy(p, m->position + i * 3, 12);
        memcpy(n, m->normal + i * 3, 12);
        for (int j = 0; j < 3; j++) {
            float v = t.a[3][j], w = 0;
            for (int k = 0; k < 3; k++) {
                v += p[k] * t.a[k][j];
                w += n[k] * t.a[k][j];
            }
            m->position[i * 3 + j] = v;
            m->normal[i * 3 + j] = w;
        }
    }
    return true;
}

static float animationValue(int channel, int frame) {
    size_t p = 0x23754 + channel * 4;
    int count = s16(p), offset = s16(p + 2);
    if (count < 1 || count > 25 || offset < 0 || offset > 300)
        throw std::runtime_error("Buddy animation bounds");
    return float(s16(0x23528 + 2 * (offset + std::min(frame, count - 1))));
}

static uint32_t mesh(uint32_t part, float frame, float viewYaw, SC_Mesh *m) {
    if (!m || !m->position || !m->normal || !m->color || !m->uv || atlas.empty() || part < 1 ||
        part > 4 || !std::isfinite(frame) || !std::isfinite(viewYaw))
        return 0;
    m->numTrianglesUsed = 0;
    if (part != SC_LAUNCHER_OPERATOR)
        return append(m, part - 1, 0, 1, 0, 0, 0);
    // Traverse the original Buddy joint hierarchy using ROM animation channels.
    // Billboard size is object scale, independent of the skeleton's .375 scale.
    try {
        int tick = int(fmodf(fabsf(frame), 25));
        const int parent[13] = {-1, 0, 1, 1, 3, 4, 5, 1, 7, 8, 9, 1, 1};
        Matrix object = rotation(0, viewYaw, 0);
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                object.a[j][k] *= .375f;
        Matrix joints[13];
        for (int i = 0; i < 13; i++) {
            float r[3];
            for (int j = 0; j < 3; j++)
                r[j] = animationValue((i + 1) * 3 + j, tick) * 3.14159265359f / 32768;
            Matrix t = rotation(r[0], r[1], r[2]);
            if (!i)
                for (int j = 0; j < 3; j++)
                    t.a[3][j] = animationValue(j, tick);
            if (i == 3) {
                t.a[3][1] = 57;
                t.a[3][2] = -60;
            }
            if (i == 7) {
                t.a[3][1] = 55;
                t.a[3][2] = 62;
            }
            if (i == 5 || i == 9)
                t.a[3][0] = 91;
            joints[i] = compose(parent[i] < 0 ? object : joints[parent[i]], t);
        }
        bool ok = append(m, 2, viewYaw, 1, joints[2].a[3][0], joints[2].a[3][1], joints[2].a[3][2]);
        return ok && appendPose(m, 4, joints[6]) && appendPose(m, 5, joints[10]) &&
               appendPose(m, 8, joints[11]) &&
               appendPose(m, int(fmodf(fabsf(frame), 150)) < 6 ? 7 : 6, joints[12]);
    } catch (const std::exception &) {
        m->numTrianglesUsed = 0;
        return 0;
    }
}
} // namespace MarioCannonAssets
