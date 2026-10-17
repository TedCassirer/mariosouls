// Compilation only: no entry point or executable behavior checks.
#include "../sdk/character_api.h"
#include "../sdk/class_api.h"
#include "../sdk/launcher_api.h"
#include "../sdk/render_api.h"
#include "../sdk/environment_api.h"
#include "../core/body.h"
#include "../core/character_storage.h"
#include "../core/character_presentation.h"
#include "../core/camera.h"
#include "../core/combat.h"
#include "../core/contact.h"
#include "../core/context_action.h"
#include "../core/debug_geometry.h"
#include "../core/debug_presentation.h"
#include "../core/render_geometry.h"
#include "../core/environment.h"
#include "../core/equipment.h"
#include "../core/feedback.h"
#include "../core/flag_lease.h"
#include "../core/hazard.h"
#include "../core/host_capabilities.h"
#include "../core/impact.h"
#include "../core/launcher.h"
#include "../core/mesh_interpolation.h"
#include "../core/motion.h"
#include "../core/native_action.h"
#include "../core/native_action_preparation.h"
#include "../core/presentation_transform.h"
#include "../core/projectile.h"
#include "../core/reaction.h"
#include "../core/session.h"
#include "../core/state.h"
#include "../core/starting_class.h"
#include "../core/terrain_policy.h"
#include "../core/timing.h"
#include "../core/unarmed_equipment.h"
#include "../core/validation.h"
#include <cstddef>
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
FIXED(SC_UnarmedEquipmentProfile, 576);
FIXED(SC_ReactionProfile, 20);
FIXED(SC_RecoveryProfile, 20);
FIXED(SC_Environment, 16);
constexpr bool wide = sizeof(void *) == 8;
static_assert(sizeof(SC_EnvironmentAPI) == (wide ? 16 : 12), "Environment callback layout");
static_assert(offsetof(SC_EnvironmentAPI, observe) == 8, "Environment callback offset");
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
static_assert(offsetof(SCRenderAPI::Vertex, color) == 12 &&
                  offsetof(SCRenderAPI::Vertex, shade) == 16 &&
                  offsetof(SCRenderAPI::Vertex, u) == 20,
              "Mesh color/shade/UV offsets");
static_assert(offsetof(SCRenderAPI::ScreenVertex, rhw) == 12 &&
                  offsetof(SCRenderAPI::ScreenVertex, color) == 16 &&
                  offsetof(SCRenderAPI::ScreenVertex, u) == 20,
              "Screen vertex offsets");
// Instantiate CPU prop assembly for both targets without an executable/check runner.
using PropSample = float (*)(const float *, int, bool);
template void SCGeometry::propMesh<PropSample>(const std::vector<SCGeometry::PropVertex> &,
                                               const std::vector<uint32_t> &, PropSample,
                                               const float *, const float *, float,
                                               std::vector<SCRenderAPI::Vertex> &);
#if defined(__i386__)
static_assert(!std::is_same<SC_Log, void(__stdcall *)(const char *)>::value,
              "x86 conventions differ");
#endif
