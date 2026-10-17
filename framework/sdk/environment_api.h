#ifndef SC_ENVIRONMENT_API_H
#define SC_ENVIRONMENT_API_H
#include "character_api.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Optional, independently versioned extension. Host observes native contact and
 * protection; guest supplies reaction/movement only. No HP, stamina, inventory,
 * native invulnerability or damage multiplier may be changed by this callback.
 * Called before simulation/control reconciliation, on the simulation thread.
 * Observations do not advance guest simulation time. */
#define SC_ENVIRONMENT_VERSION 1u
#define SC_ENVIRONMENT_BURNING_FLOOR 1u
#define SC_ENVIRONMENT_ENEMY_FIRE 2u
#define SC_ENV_CONTACT 1u
#define SC_ENV_PROTECTED 2u
#define SC_ENV_GROUNDED 4u
#define SC_ENV_CONTROL_ALLOWED 8u
#define SC_ENV_REACTION_OWNED 1u
#define SC_ENV_REACTION_STARTED 2u
/* Precedence over ordinary damage only. Hosts must verify safe native recovery;
 * death, throws and scripted ownership still win. */
#define SC_ENV_REACTION_PRIORITY 4u

typedef struct SC_Environment {
    uint32_t size, kind, flags, tickMs;
} SC_Environment;

typedef struct SC_EnvironmentAPI {
    uint32_t size, version;
    uint32_t(SC_CALL *observe)(int32_t character, const SC_Environment *environment);
} SC_EnvironmentAPI;

typedef const SC_EnvironmentAPI *(SC_CALL *SC_GetEnvironmentAPI)(uint32_t version, uint32_t size);

#ifdef __cplusplus
}
#endif
#endif
