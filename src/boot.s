.section .multiboot
.align 4
.long 0x1BADB002
.long 0x00000000
.long -(0x1BADB002)

.extern kernel_main

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.align 4096
.globl idt
idt:
.skip 2048
idt_end:

.section .data
.align 16
.globl idt_ptr
idt_ptr:
.word idt_end - idt - 1
.long idt

.align 16
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF
    .quad 0x00CF92000000FFFF
gdt_end:

.globl gdt_ptr
gdt_ptr:
    .word gdt_end - gdt - 1
    .long gdt

.section .text
.globl _start
_start:
    lgdt gdt_ptr
    
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    
    ljmp $0x08, $reload_cs
reload_cs:
    lidt idt_ptr
    
    mov %cr4, %eax
    orl $0x600, %eax
    mov %eax, %cr4
    
    mov $stack_top, %esp
    
    cli
    cld
    
    push %ebx
    push %eax
    call kernel_main
    
    hlt
    jmp .
