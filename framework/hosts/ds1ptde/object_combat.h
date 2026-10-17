#pragma once

// Shared native builder accepts both live actors and native map objects.
static int worldDamageObject(int behavior, uintptr_t owner, uintptr_t target, const float *point) {
    return nativeDamageTarget(behavior, owner, target, point);
}

static void combatObjects() {
    if (!combatUnarmedReady())
        return;
    if (!characterOwned() || !worldObjectDamageReady || !combatEnabled ||
        !hostPermits(SCHost::Operation::Combat) || !characterWindow.active(state, *profile))
        return;
    int move = characterWindow.move;
    if (move < 0 || (profile->attacks[move].flags & SC_ATTACK_NO_OBJECTS))
        return;
    uintptr_t manager = read<uintptr_t>(0x137d5f8), maps = read<uintptr_t>(manager + 0x20);
    int count = read<int>(manager + 0x1c);
    if (count < 1 || count > 64 || !maps)
        return;
    int serial = characterWindow.serial;
    Vec p = read<Vec>(playerPhysics + 0x10),
        forward = {sinf(state.faceAngle), 0, cosf(state.faceAngle)};
    for (int m = 0; m < count; m++) {
        uintptr_t map = maps + m * 0xec, objects = read<uintptr_t>(map + 0x40);
        int n = read<int>(map + 0x3c);
        if (!objects || n < 1 || n > 10000)
            continue;
        for (int j = 0; j < n; j++) {
            uintptr_t obj = objects + j * 0xc0;
            int handle = int(read<uint32_t>(obj + 4));
            if (!characterWindow.canHit(handle, *profile) || read<int16_t>(obj + 0x2a) <= 0 ||
                !(read<uint8_t>(obj + 0x2d) & 0x40) || !read<uintptr_t>(obj + 0x90))
                continue;
            Vec at = {read<float>(obj + 0x6c), read<float>(obj + 0x7c) + .7f,
                      read<float>(obj + 0x8c)};
            for (const auto &c : worldContacts)
                if (c.kind == 1 && c.owner == obj &&
                    dot(c.point - p, c.point - p) < dot(at - p, at - p))
                    at = c.point;
            Vec delta = at - (p + Vec{0, .7f, 0});
            float radius = profile->attacks[move].rangeCm * .01f;
            if (fabsf(delta.y) > 1.5f || delta.x * delta.x + delta.z * delta.z > radius * radius ||
                (!(profile->attacks[move].flags & SC_ATTACK_RADIAL) && dot(delta, forward) < 0))
                continue;
            if (!lineClear(p + Vec{0, .7f, 0}, at) || !worldLineClear(p + Vec{0, .7f, 0}, at, obj))
                continue;
            characterWindow.hit.insert(handle);
            characterPacketMove = move;
            characterPacketPower = characterWindow.attackPower;
            characterPacketPoise = 0;
            characterPacketBuilt = false;
            debugContact(at, false);
            uint16_t objectPower = uint16_t(std::clamp(characterWindow.attackPower, 1.f, 1200.f));
            memcpy(characterAttacks[move] + 0x62, &objectPower, 2);
            alignas(16) float point[4] = {at.x, at.y, at.z, 1};
            int before = read<int16_t>(obj + 0x2a);
            worldDamageObject(990050 + move, player, obj, point);
            characterPacketMove = -1;
            if (!characterPacketBuilt) {
                characterDamageReady = false;
                logMessage("Native object damage pipeline unavailable");
                return;
            }
            feedbackTrack(obj, handle, before, serial, move, at, true);
        }
    }
}
