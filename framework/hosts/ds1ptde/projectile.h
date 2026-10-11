#pragma once
#include "../../core/projectile.h"
// Owned Steam PTDE executable: BulletFlyState::update(this, BulletIns*, dt),
// native map-hit dispatch (EAX=BulletIns, stack[1]=hit, stack[2]=record).
// Bounces bypass the entire map-hit dispatch, including its hit retirement.
// Never patch shared spell rows.
static SC_ProjectileProfile projectileProfile{};
static bool projectileSupported = false, projectileReady = false;
static uintptr_t projectileRateActor = 0, projectileRateAddress = 0;
static SCProjectile::Rate projectileRate;

static void projectileRateRelease() {
    float current = 0, value = 0;
    if (projectileRateAddress && chain(gameSlots.player, {0, 4, 0}) == projectileRateActor &&
        chain(projectileRateActor + 0x28, {0, 0x14}) + 0x64 == projectileRateAddress &&
        get(projectileRateAddress, current) && projectileRate.restore(current, value))
        put(projectileRateAddress, value);
    projectileRate = {};
    projectileRateActor = projectileRateAddress = 0;
}

static void projectileConfigure() {
    auto entry = (SC_GetProjectileProfile)GetProcAddress(adapterModule,
                                                         "SoulsCharacterGetProjectileProfile");
    auto p = entry ? entry(SC_PROJECTILE_PROFILE_VERSION, sizeof(SC_ProjectileProfile)) : nullptr;
    projectileSupported = SCProjectile::valid(p) && (profile->capabilities & SC_CAP_PYROMANCY);
    if (projectileSupported)
        projectileProfile = *p;
}

static bool projectileEnabled() {
    return projectileSupported && projectileReady &&
           GetPrivateProfileIntA("Projectiles", "Enabled", 1, ini) != 0;
}

static void projectileCastRate(bool cast) {
    if (!cast || !projectileEnabled()) {
        projectileRateRelease();
        return;
    }
    uintptr_t at = chain(player + 0x28, {0, 0x14});
    if (!at) {
        projectileRateRelease();
        return;
    }
    at += 0x64;
    if (projectileRateAddress && projectileRateAddress != at)
        projectileRateRelease();
    float current = 0, next = 0;
    if (!get(at, current))
        return;
    if (projectileRate.acquire(current, projectileProfile.castSpeed, next) && put(at, next)) {
        projectileRate.last = next;
        projectileRateActor = player;
        projectileRateAddress = at;
    }
}

static uintptr_t projectileOwner(uint32_t handle) {
    return nativeActor(handle);
}

struct ProjectileFlight {
    uintptr_t bullet = 0, actor = 0;
    uint32_t serial = 0, owner = 0;
    float age = 0;
    Vec velocity{};
    unsigned bounces = 0;
    float bouncedAt = -1, floorY = 0;
};

static ProjectileFlight projectileFlights[128]; // Native pool also has 128 slots.

static ProjectileFlight *projectileFind(uintptr_t bullet) {
    if (read<uint32_t>(bullet) != 0x11948b4 || read<int>(bullet + 0x70) != 2 ||
        read<int>(bullet + 0x290) != 6000)
        return nullptr;
    for (auto &f : projectileFlights)
        if (f.bullet == bullet && f.serial == read<uint32_t>(bullet + 4) &&
            f.owner == read<uint32_t>(bullet + 0x8c) && projectileOwner(f.owner) == f.actor)
            return &f;
    return nullptr;
}

using ProjectileFly = bool(__thiscall *)(uintptr_t, uintptr_t, float);
static ProjectileFly projectileOriginalFly = nullptr;

static bool __fastcall projectileFly(uintptr_t self, uintptr_t, uintptr_t bullet, float dt) {
    // FlyState::enter resets elapsed at +10 for every use of a pooled bullet.
    // Its +15 initialized flag remains set across reuse and is not a spawn ID.
    bool fresh = read<float>(self + 0x10) == 0;
    bool result = projectileOriginalFly(self, bullet, dt);
    // A native pool address is reusable, including by unrelated projectile kinds.
    if (fresh)
        for (auto &slot : projectileFlights)
            if (slot.bullet == bullet)
                slot = {};
    if (read<uint32_t>(bullet) != 0x11948b4 || self != bullet + 0x28c ||
        read<int>(self + 4) != 6000 || !SCProjectile::range(dt, 0, .1f))
        return result;
    auto *f = projectileFind(bullet);
    if (fresh) {
        f = nullptr;
        uintptr_t actor = projectileOwner(read<uint32_t>(bullet + 0x8c));
        logf("PROJECTILE spawn id=6000 owner=%08x actor=%08x player=%08x position=%.3f %.3f %.3f",
             read<uint32_t>(bullet + 0x8c), unsigned(actor), unsigned(player),
             read<float>(bullet + 0x10), read<float>(bullet + 0x14), read<float>(bullet + 0x18));
        if (projectileEnabled() && characterOwned() && equipmentActionOwned && actor == player &&
            player == lastPlayer && read<int>(player + 0x2d4) > 0) {
            for (auto &slot : projectileFlights)
                if (!slot.bullet || read<uint32_t>(slot.bullet + 4) != slot.serial ||
                    read<int>(slot.bullet + 0x70) != 2) {
                    f = &slot;
                    break;
                }
            if (f) {
                *f = {bullet, actor, read<uint32_t>(bullet + 4), read<uint32_t>(bullet + 0x8c)};
                Vec direction = read<Vec>(bullet + 0x30);
                direction.y = 0;
                if (dot(direction, direction) < .01f) {
                    float yaw = read<float>(posData + 4);
                    direction = {-sinf(yaw), 0, -cosf(yaw)};
                }
                f->velocity = unit(direction) * (projectileProfile.speed * .01f);
                f->velocity.y = -1.5f;
                Vec p = read<Vec>(bullet + 0x10);
                p.y = read<float>(posData + 0x14) + projectileProfile.spawnHeight * .01f;
                put(bullet + 0x10, p);
                put(bullet + 0x1d4, projectileProfile.lifetime);
            }
        }
    }
    if (f) {
        float before = f->age;
        f->age += dt;
        f->velocity.y -= projectileProfile.gravity * .01f * dt;
        put(bullet + 0x30, f->velocity);
        if (int(before * 4) != int(f->age * 4)) {
            Vec p = read<Vec>(bullet + 0x10);
            logf("PROJECTILE flight age=%.3f xyz=%.3f %.3f %.3f velocity_y=%.3f", f->age, p.x, p.y,
                 p.z, f->velocity.y);
        }
    }
    return result;
}

extern "C" {
void *projectileOriginalMap = nullptr;
}

extern "C" __attribute__((used, noinline)) int projectileMapContact(uintptr_t bullet,
                                                                    uintptr_t hit) {
    auto *f = projectileFind(bullet);
    if (!f || read<int>(bullet + 0x70) != 2)
        return 0;
    Vec n = read<Vec>(hit + 0xe0), p = read<Vec>(hit + 0xd0);
    logf("PROJECTILE contact age=%.3f normal=%.3f %.3f %.3f point=%.3f %.3f %.3f velocity_y=%.3f",
         f->age, n.x, n.y, n.z, p.x, p.y, p.z, f->velocity.y);
    // Havok can report several adjacent floor triangles in the same step.
    // They are one contact, not successive bounces or an explosion.
    if (SCProjectile::sameFloorContact(n.y, dot(n, n), p.y - f->floorY, f->age - f->bouncedAt,
                                       f->velocity.y))
        return 1;
    if (!SCProjectile::floorBounce(n.y, dot(n, n), f->velocity.y, f->bounces))
        return 0;
    ++f->bounces;
    f->bouncedAt = f->age;
    f->floorY = p.y;
    f->velocity.y = projectileProfile.bounceSpeed * .01f;
    Vec current = read<Vec>(bullet + 0x10);
    float radius = read<float>(read<uint32_t>(bullet + 0x294) + 0x44);
    if (SCProjectile::range(radius, .01f, .5f) && dot(current - p, current - p) < 1)
        put(bullet + 0x10, p + n * (radius + .025f));
    put(bullet + 0x30, f->velocity); // Native movement and hit processing continue.
    put(bullet + 0x40, f->velocity);
    logf("PROJECTILE bounce=%u age=%.3f", f->bounces, f->age);
    return 1;
}

#define SC_PROJECTILE_RESTORE_XMM                                                                                                                                                                     \
    "movups 0(%esp),%xmm0\n movups 16(%esp),%xmm1\n movups 32(%esp),%xmm2\n movups 48(%esp),%xmm3\n movups 64(%esp),%xmm4\n movups 80(%esp),%xmm5\n movups 96(%esp),%xmm6\n movups 112(%esp),%xmm7\n" \
    "addl $128,%esp\n popal\n popfl\n"

__attribute__((naked)) static void projectileMapHook() {
    __asm__ volatile(
        "pushfl\n pushal\n subl $128,%esp\n"
        "movups %xmm0,0(%esp)\n movups %xmm1,16(%esp)\n movups %xmm2,32(%esp)\n movups %xmm3,48(%esp)\n movups %xmm4,64(%esp)\n movups %xmm5,80(%esp)\n movups %xmm6,96(%esp)\n movups %xmm7,112(%esp)\n"
        "pushl 168(%esp)\n pushl %eax\n call _projectileMapContact\n addl $8,%esp\n testl %eax,%eax\n jz 1f\n" SC_PROJECTILE_RESTORE_XMM
        "xorl %eax,%eax\n ret\n 1:\n" SC_PROJECTILE_RESTORE_XMM "jmp *_projectileOriginalMap\n");
}

#undef SC_PROJECTILE_RESTORE_XMM

static void projectileInstall() {
    const unsigned char fly[] = {0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf8, 0x83, 0xec, 0x24};
    const unsigned char map[] = {0x55, 0x8b, 0xec, 0x83, 0xe4, 0xf0, 0x83, 0xec, 0x14, 0x53};
    unsigned char a[sizeof(fly)], b[sizeof(map)];
    if (!get(uintptr_t(0x1018dc0), a) || memcmp(a, fly, sizeof(a)) ||
        !get(uintptr_t(0x10258b0), b) || memcmp(b, map, sizeof(b)))
        return;
    if (MH_CreateHook((void *)0x1018dc0, (void *)projectileFly, (void **)&projectileOriginalFly) !=
        MH_OK)
        return;
    if (MH_CreateHook((void *)0x10258b0, (void *)projectileMapHook, &projectileOriginalMap) !=
        MH_OK) {
        MH_RemoveHook((void *)0x1018dc0);
        return;
    }
    if (MH_EnableHook((void *)0x1018dc0) != MH_OK || MH_EnableHook((void *)0x10258b0) != MH_OK) {
        for (auto p : {(void *)0x1018dc0, (void *)0x10258b0}) {
            MH_DisableHook(p);
            MH_RemoveHook(p);
        }
        return;
    }
    projectileReady = true;
    logf("PROJECTILE native flight/contact hooks ready");
}
