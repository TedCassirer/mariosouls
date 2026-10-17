// Private visual QA from real SDK output; owned-ROM geometry is never committed.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../../sdk/character_api.h"

static void check(bool ok) {
    if (!ok) {
        fprintf(stderr, "Casting preview failed\n");
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc != 5)
        return 2;
    auto module = LoadLibraryA(argv[1]);
    check(module != nullptr);
    auto get = (SC_GetAPI)GetProcAddress(module, "SoulsCharacterGetAPI");
    check(get != nullptr);
    auto api = get(SC_ABI_VERSION, sizeof(SC_CharacterAPI));
    check(api && api->equipment && api->attachments);
    auto *d = api->describe();
    std::vector<unsigned char> texture(d->textureWidth * d->textureHeight * 4);
    SC_Texture tex = {texture.data(), (uint32_t)texture.size()};
    SC_Init init = {sizeof(init), argv[2], argv[3], nullptr};
    check(api->initialize(&init, &tex));
    SC_Surface floor[2] = {{{{-16000, 0, -16000}, {-16000, 0, 16000}, {16000, 0, 16000}}, 0},
                           {{{-16000, 0, -16000}, {16000, 0, 16000}, {16000, 0, -16000}}, 0}};
    api->surfaces(floor, 2, 0);
    int id = api->create(0, 0, 0, 0);
    check(id >= 0);
    unsigned n = d->maxTriangles;
    std::vector<float> p(n * 9), normal(n * 9), color(n * 9), uv(n * 6);
    SC_Mesh mesh = {p.data(), normal.data(), color.data(), uv.data(), n, 0};
    SC_State state{};
    FILE *f = fopen(argv[4], "wb");
    check(f != nullptr);
    unsigned count = 62;
    fwrite("SCPOSE1", 1, 8, f);
    fwrite(&d->textureWidth, 4, 1, f);
    fwrite(&d->textureHeight, 4, 1, f);
    fwrite(&count, 4, 1, f);
    fwrite(texture.data(), 1, texture.size(), f);
    for (unsigned side : {1u, 2u})
        for (unsigned frame = 0; frame <= 30; frame++) {
            float phase = frame / 30.f;
            SC_EquipmentPose e = {sizeof(e),
                                  side == 1 ? SC_EQUIPMENT_PYROMANCY : SC_EQUIPMENT_NONE,
                                  side == 2 ? SC_EQUIPMENT_PYROMANCY : SC_EQUIPMENT_NONE,
                                  side == 1 ? SC_EQUIPMENT_CAST_RIGHT : SC_EQUIPMENT_CAST_LEFT,
                                  phase,
                                  0};
            check(api->equipment(id, &e));
            SC_Pose pose = {};
            pose.kind = SC_POSE_NATIVE_COMBAT;
            pose.phase = -1;
            check(api->pose(id, &pose, &state, &mesh));
            SC_Attachments hands = {sizeof(hands)};
            check(api->attachments(id, &hands) && hands.valid == side);
            unsigned header[] = {side == 1 ? 8u : 9u, 1, frame, mesh.numTrianglesUsed};
            fwrite(header, 4, 4, f);
            fwrite(&phase, 4, 1, f);
            fwrite(hands.right, 4, 16, f);
            fwrite(hands.left, 4, 16, f);
            fwrite(mesh.position, 4, mesh.numTrianglesUsed * 9, f);
            fwrite(mesh.normal, 4, mesh.numTrianglesUsed * 9, f);
            fwrite(mesh.color, 4, mesh.numTrianglesUsed * 9, f);
            fwrite(mesh.uv, 4, mesh.numTrianglesUsed * 6, f);
        }
    fclose(f);
    api->destroy(id);
    api->shutdown();
    FreeLibrary(module);
    puts("Exported 62 private casting frames");
}
