#pragma once
// Preserve flags, GPRs and SSE registers around the two native cap adjustments.
// At entry x87 is empty: each site immediately follows FSTP.
#define DEFENSE_SAVE                                                                               \
    "pushfl\n pushal\n subl $128,%esp\n"                                                           \
    "movups %xmm0,0(%esp)\n movups %xmm1,16(%esp)\n movups %xmm2,32(%esp)\n movups %xmm3,48(%esp)\n movups %xmm4,64(%esp)\n movups %xmm5,80(%esp)\n movups %xmm6,96(%esp)\n movups %xmm7,112(%esp)\n"
#define DEFENSE_RESTORE                                                                                                                                                                               \
    "addl $8,%esp\n"                                                                                                                                                                                  \
    "movups 0(%esp),%xmm0\n movups 16(%esp),%xmm1\n movups 32(%esp),%xmm2\n movups 48(%esp),%xmm3\n movups 64(%esp),%xmm4\n movups 80(%esp),%xmm5\n movups 96(%esp),%xmm6\n movups 112(%esp),%xmm7\n" \
    "addl $128,%esp\n popal\n popfl\n"

__attribute__((naked, used)) static void defenseRegenHook() {
    __asm__ volatile(
        DEFENSE_SAVE
        "leal 164(%esp),%eax\n pushl %esi\n pushl %eax\n call _characterPoiseAdjust\n" DEFENSE_RESTORE
        "jmp *_defenseOriginalRegen\n");
}

__attribute__((naked, used)) static void defenseHitHook() {
    __asm__ volatile(
        DEFENSE_SAVE
        "leal 192(%esp),%eax\n pushl %esi\n pushl %eax\n call _characterPoiseAdjust\n" DEFENSE_RESTORE
        "jmp *_defenseOriginalHit\n");
}

#undef DEFENSE_SAVE
#undef DEFENSE_RESTORE
