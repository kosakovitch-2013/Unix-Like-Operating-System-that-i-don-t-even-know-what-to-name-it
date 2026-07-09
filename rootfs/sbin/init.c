__asm__(
    ".global _start\n"
    "_start:\n"
    "    movb $'I', 0xB8000\n"
    "    movb $0x0F, 0xB8001\n"
    "    movl $6, 0x1FF00\n"
    "    movl $sh, 0x1FF04\n"
    "    int $0x81\n"
    "1:  hlt\n"
    "    jmp 1b\n"
    ".section .rodata\n"
    "sh: .asciz \"/bin/sh\"\n"
    ".section .text\n"
);
