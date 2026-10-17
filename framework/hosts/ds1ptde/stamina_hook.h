#pragma once

// E67960 is an integer setter whose callers keep floating-point state live.
// The C++ callback may clobber floating-point state: preserve x87, SSE and MXCSR.
// EDI anchors the saved frame while ESP aligns the 512-byte FXSAVE buffer.
__attribute__((naked, used)) static void characterStaminaHook() {
    __asm__ volatile(
        "pushfl\n pushal\n movl %esp,%edi\n subl $528,%esp\n andl $-16,%esp\n fxsave (%esp)\n"
        "movl 36(%edi),%eax\n movl 40(%edi),%ecx\n pushl %ebx\n pushl %eax\n pushl %ecx\n pushl %edx\n"
        "call _characterFilterStamina\n addl $16,%esp\n movl %eax,40(%edi)\n"
        "fxrstor (%esp)\n movl %edi,%esp\n popal\n popfl\n jmp *_characterOriginalStamina\n");
}
