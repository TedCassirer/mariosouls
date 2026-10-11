#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <vector>
#include "../../core/validation.h"
static unsigned sounds = 0;

static void sound(uint32_t, float *) {
    ++sounds;
}

int main(int argc, char **argv) {
    assert(argc == 4);
    HMODULE dll = LoadLibraryA(argv[1]), runtime = LoadLibraryA(argv[3]);
    assert(dll && runtime);
    auto entry = (SC_GetAPI)GetProcAddress(dll, "SoulsCharacterGetAPI");
    assert(entry);
    auto api = entry(SC_ABI_VERSION, sizeof(SC_CharacterAPI));
    assert(SC::api(api));
    auto d = api->describe();
    assert(d->capabilities & SC_CAP_DEATH_POSE);
    std::vector<uint8_t> pixels(d->textureWidth * d->textureHeight * 4);
    SC_Texture texture = {pixels.data(), uint32_t(pixels.size())};
    SC_Init init = {sizeof(init), argv[2], argv[3], nullptr};
    assert(api->initialize(&init, &texture));
    auto registerSound = (void (*)(void (*)(uint32_t, float *)))GetProcAddress(
        runtime, "sm64_register_play_sound_function");
    assert(registerSound);
    registerSound(sound);
    SC_Surface floor[2] = {{{{-1000, 0, -1000}, {-1000, 0, 1000}, {1000, 0, 1000}}, 0},
                           {{{-1000, 0, -1000}, {1000, 0, 1000}, {1000, 0, -1000}}, 0}};
    api->surfaces(floor, 2, 0);
    std::vector<float> pos(d->maxTriangles * 9), normal(pos.size()), color(pos.size()),
        uv(d->maxTriangles * 6);
    SC_Mesh mesh = {pos.data(), normal.data(), color.data(), uv.data(), d->maxTriangles, 0};
    SC_State state{};
    SC_Input input{};
    input.camLookZ = 1;
    for (int cycle = 0; cycle < 2; cycle++) {
        int id = api->create(0, 0, 0, 0);
        assert(id >= 0);
        for (int i = 0; i < 10; i++)
            assert(api->step(id, &input, &state, &mesh));
        SC_EquipmentPose equipment = {
            sizeof(equipment), SC_EQUIPMENT_SWORD, SC_EQUIPMENT_SHIELD, 1, .5f, 1};
        assert(api->equipment(id, &equipment));
        assert(api->step(id, &input, &state, &mesh));
        auto idle = pos;
        unsigned before = sounds;
        int first = -1, previous = -1, last = -1;
        std::vector<float> held;
        SC_Pose death = {SC_POSE_DEATH, {0, 0, 0}, 0, -1, 0};
        for (int tick = 0; tick < 240; tick++) {
            assert(api->pose(id, &death, &state, &mesh) && SC::frame(state, mesh, *d));
            assert(mesh.numTrianglesUsed);
            assert(state.rawAnimation == 0x32 && state.flags == 0 && state.attackId == -1 &&
                   state.interruptedAttackId == -1);
            assert(state.rawAnimationFrame >= previous);
            previous = state.rawAnimationFrame;
            if (!tick)
                first = previous;
            assert(state.position[0] == 0 && state.position[1] == 0 && state.position[2] == 0);
            SC_Attachments hands = {sizeof(hands)};
            assert(api->attachments(id, &hands) && !hands.valid);
            if (tick == 150) {
                held = pos;
                last = previous;
            }
            if (tick > 150) {
                assert(previous == last);
                assert(held == pos);
            }
        }
        assert(first == 0 && last >= 77 && idle != pos && sounds - before == 1);
        printf("death cycle %d: animation=0x32 frames=%d..%d, final mesh held, one voice\n", cycle,
               first, last);
        // A host resume/new life must clear the death expression/pose and permit movement.
        api->resume(id, 0, 0, 0, 0);
        assert(api->step(id, &input, &state, &mesh));
        input.jump = 1;
        assert(api->step(id, &input, &state, &mesh));
        assert(state.position[1] > 0);
        input.jump = 0;
        api->destroy(id);
    }
    registerSound(nullptr);
    api->shutdown();
    FreeLibrary(runtime);
    FreeLibrary(dll);
    puts(
        "PASS Mario death: ROM collapse animation, monotonic frames, nonlooping final pose, one-shot voice, no combat/socket flags, recreate and jump recovery");
}
