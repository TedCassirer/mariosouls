#pragma once
/* Shared DLL callbacks use the C calling convention on both architectures.
 * This documents ABI4; it does not make x86 and x64 DLLs interoperable. */
#if defined(_WIN32)
#define SC_CALL __cdecl
#else
#define SC_CALL
#endif
