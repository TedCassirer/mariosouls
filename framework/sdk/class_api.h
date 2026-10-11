#pragma once
#include <stdint.h>
// Optional native starting-class replacement. Hosts map archetypes to their own
// class records/items; adapters provide presentation, never addresses or writes.
#define SC_CLASS_PROFILE_VERSION 1u

enum SC_ClassArchetype { SC_CLASS_FIRECASTER = 1 };

struct SC_ClassProfile {
    uint32_t size, archetype;
    const char *name;        // UTF-8, at most 31 bytes
    const char *description; // UTF-8, at most 191 bytes; explicit newlines
    float previewHeightCm;   // framing only, never gameplay body size
    float previewYaw;
};

typedef const SC_ClassProfile *(*SC_GetClassProfile)(uint32_t version, uint32_t size);
