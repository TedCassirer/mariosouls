#include "../sdk/character_api.h"
#include "../sdk/class_api.h"
#include "../sdk/launcher_api.h"
#include <stddef.h>
_Static_assert(sizeof(SC_State) == 76, "C layout");
_Static_assert(offsetof(SC_CharacterAPI, describe) == 8, "C callback table");

static void SC_CALL log_message(const char *message) {
    (void)message;
}

int main(void) {
    SC_Log callback = log_message;
    callback("C ABI");
    return 0;
}
