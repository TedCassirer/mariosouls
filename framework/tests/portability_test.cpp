#undef NDEBUG
#include "../sdk/character_api.h"
#include "../sdk/class_api.h"
#include "../sdk/launcher_api.h"
#include "../sdk/render_api.h"
#include "../core/mesh_pack.h"
#include "../core/session.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <type_traits>
#define FIXED(T, N)                                                                                \
    static_assert(sizeof(T) == N && alignof(T) == 4, #T " fixed layout");                          \
    static_assert(std::is_standard_layout<T>::value, #T " standard layout")
FIXED(SC_Input, 20);
FIXED(SC_Surface, 40);
FIXED(SC_State, 76);
FIXED(SC_Attack, 80);
FIXED(SC_Body, 32);
FIXED(SC_Descriptor, 2752);
FIXED(SC_Pose, 28);
FIXED(SC_MotionResolution, 76);
FIXED(SC_EquipmentPose, 24);
FIXED(SC_Attachments, 136);
FIXED(SC_LauncherState, 36);
FIXED(SC_ProjectileProfile, 36);
FIXED(SC_ImpactProfile, 148);
FIXED(SC_CameraProfile, 12);
constexpr bool wide = sizeof(void *) == 8;
static_assert(sizeof(void *) == 4 || wide, "Supported validation architectures");
static_assert(sizeof(uintptr_t) == sizeof(void *), "Opaque identity width");
static_assert(sizeof(SC_Mesh) == (wide ? 40 : 24), "Mesh pointer layout");
static_assert(sizeof(SC_Texture) == (wide ? 16 : 8), "Texture pointer layout");
static_assert(sizeof(SC_Init) == (wide ? 32 : 16), "Init pointer layout");
static_assert(sizeof(SC_CharacterAPI) == (wide ? 160 : 84), "API function table layout");
static_assert(sizeof(SC_ClassProfile) == (wide ? 32 : 24), "Class pointer layout");
static_assert(sizeof(SC_LauncherAPI) == (wide ? 80 : 48), "Launcher pointer layout");
static_assert(offsetof(SC_Mesh, capacityTriangles) == 4 * sizeof(void *), "Mesh field offset");
static_assert(offsetof(SC_CharacterAPI, describe) == 8, "Callback table offset");
static_assert(offsetof(SC_Init, assetPath) == (wide ? 8 : 4), "Init padding");
static_assert(offsetof(SC_State, rawAction) == 52, "State diagnostic offset");
static_assert(offsetof(SC_Descriptor, attacks) == 160, "Descriptor attacks offset");
static_assert(offsetof(SC_Descriptor, body) == 2720, "Descriptor body offset");
static_assert(std::is_same<SC_Log, void(__cdecl *)(const char *)>::value, "cdecl callback");
#if defined(__i386__)
static_assert(!std::is_same<SC_Log, void(__stdcall *)(const char *)>::value,
              "x86 conventions differ");
#endif
int main() {
    SCSession::Session session;
    uintptr_t high = wide ? uintptr_t(0x100000000ull) : uintptr_t(0x10000);
    SCSession::Identity first{high + 1, high + 2, high + 3, 0};
    session.activate(first);
    assert(session.matches(first));
    auto other = first;
    other.actor = 1;
    assert(!session.sameActor(other));
    printf("PASS %zu-bit SDK layouts, offsets, C calling convention and pointer identity\n",
           sizeof(void *) * 8);
}
