// Compilation only: C ABI sizes and callback declarations.
#include "../sdk/character_api.h"
#include "../sdk/class_api.h"
#include "../sdk/launcher_api.h"
#include "../sdk/environment_api.h"
#include <stddef.h>
_Static_assert(sizeof(SC_State) == 76, "C layout");
_Static_assert(sizeof(SC_Environment) == 16, "Environment C layout");
_Static_assert(sizeof(SC_UnarmedEquipmentProfile) == 576, "Unarmed equipment C layout");
_Static_assert(offsetof(SC_CharacterAPI, describe) == 8, "C callback table");

_Static_assert(sizeof(void *) == 4 || sizeof(void *) == 8, "Supported pointer sizes");
_Static_assert(sizeof(SC_CharacterAPI) == (sizeof(void *) == 8 ? 160 : 84), "C API table size");
_Static_assert(sizeof(SC_Mesh) == (sizeof(void *) == 8 ? 40 : 24), "C mesh pointer layout");
_Static_assert(sizeof(SC_Init) == (sizeof(void *) == 8 ? 32 : 16), "C init pointer layout");
_Static_assert(offsetof(SC_Mesh, capacityTriangles) == 4 * sizeof(void *), "C mesh field offset");

static void SC_CALL message(const char *text) {
    (void)text;
}

SC_Log const sdk_callback_convention = message;
