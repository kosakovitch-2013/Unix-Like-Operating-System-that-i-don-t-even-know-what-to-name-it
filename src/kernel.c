#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <shell.h>
#include <printk.h>
#include <vfs.h>
#include <fat32.h>
#include <iso9660.h>
#include <atapi.h>
#include "syscall.h"
#include "fs_detect.h"


typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#define VGA_BUFFER 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

static int cursor_pos = 0;
static volatile uint64_t system_ticks = 0;

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

extern struct idt_entry idt[256];

void set_idt_gate(int n, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[n].base_low = handler & 0xFFFF;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
    idt[n].sel = sel;
    idt[n].zero = 0;
    idt[n].flags = flags;
}

void timer_handler(void);
void timer_stub(void);
void syscall_stub(void);
void keyboard_stub(void);
void putchar_at_cursor(char c);
void format_uptime(char* buffer, size_t size);
void kernel_panic(const char* msg);

void isr0() { kernel_panic("Division by zero"); }
void isr3() { kernel_panic("Breakpoint"); }
void isr6() { kernel_panic("Invalid opcode"); }
void isr8() { kernel_panic("Double fault"); }
void isr13() { kernel_panic("General protection fault"); }
void isr14() { kernel_panic("Page fault"); }

__asm__(
    ".global timer_stub\n"
    "timer_stub:\n"
    "    pusha\n"
    "    push %ds\n"
    "    push %es\n"
    "    push %fs\n"
    "    push %gs\n"
    "    mov $0x10, %ax\n"
    "    mov %ax, %ds\n"
    "    mov %ax, %es\n"
    "    call timer_handler\n"
    "    pop %gs\n"
    "    pop %fs\n"
    "    pop %es\n"
    "    pop %ds\n"
    "    popa\n"
    "    iret\n"
);

__asm__(
    ".global syscall_stub\n"
    "syscall_stub:\n"
    "    movb $'!', 0xB800A\n"
    "    movb $0x0F, 0xB800B\n"
    "    push %ebp\n"
    "    mov %esp, %ebp\n"
    "    push %edx\n"
    "    push %ecx\n"
    "    push %ebx\n"
    "    push %eax\n"
    "    call do_syscall\n"
    "    mov %ebp, %esp\n"
    "    pop %ebp\n"
    "    iret\n"
);

__asm__ volatile (
    "inb $0x60, %al\n"
    "mov $0x20, %al\n"
    "out %al, $0x20\n"
    : // no output
    : // no input
    : "eax" // we change the value of eax only
);

int do_syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a" (data) : "dN" (port));
    return data;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a" (data), "dN" (port));
}

int do_syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    uint32_t* params = (uint32_t*)0x1FF00;
    uint32_t num = params[0];
    
    if (num == 6) {
        // Hardcode path - skip init's pointer
        int fd = vfs_open("/bin/sh", 0);
        if (fd >= 0) {
            void* addr = (void*)0x400000;
            vfs_read(fd, addr, 65536);
            vfs_close(fd);
            void (*prog)() = addr;
            prog();
        }
    }
    return 0;
}

void idt_init() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xF8);
    outb(0xA1, 0xFF);
    
    set_idt_gate(0, (uint32_t)isr0, 0x08, 0x8E);
    set_idt_gate(3, (uint32_t)isr3, 0x08, 0x8E);
    set_idt_gate(6, (uint32_t)isr6, 0x08, 0x8E);
    set_idt_gate(8, (uint32_t)isr8, 0x08, 0x8E);
    set_idt_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    set_idt_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    set_idt_gate(0x81, (uint32_t)syscall_stub, 0x08, 0x8E | 0x60);
    set_idt_gate(33, (uint32_t)keyboard_stub, 0x08, 0x8E);
    printk("IDT initialized");
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    set_idt_gate(32, (uint32_t)timer_stub, 0x08, 0x8E);
    printk("PIT initialized at %d Hz", frequency);
}

static uint32_t tick_count = 0;

void timer_handler() {
    system_ticks++;
    tick_count++;
    outb(0x20, 0x20);
}

void set_cursor_pos(int row, int col) {
    unsigned short pos = row*80+col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void scroll_screen(void) {
    uint16_t* vga = (uint16_t*)VGA_BUFFER;
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++)
        vga[i] = vga[i + VGA_WIDTH];
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = ' ' | 0x0F00;
    cursor_pos -= VGA_WIDTH;
}

void puts(const char* str) {
    uint16_t* vga = (uint16_t*)VGA_BUFFER;
    while (*str) {
        if (*str == '\n') {
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
        } else {
            vga[cursor_pos] = (uint16_t)*str | 0x0F00;
            cursor_pos++;
        }
        if (cursor_pos >= VGA_WIDTH * VGA_HEIGHT) scroll_screen();
        str++;
    }
    set_cursor_pos(cursor_pos / 80, cursor_pos % 80);
}

int get_cursor_pos(void) { return cursor_pos; }
uint64_t get_uptime_ms(void) { return system_ticks; }

void putchar_at_cursor(char c) {
    uint16_t* vga = (uint16_t*)VGA_BUFFER;
    vga[cursor_pos] = (uint16_t)c | 0x0F00;
    cursor_pos++;
    if (cursor_pos >= VGA_WIDTH * VGA_HEIGHT) scroll_screen();
    set_cursor_pos(cursor_pos / 80, cursor_pos % 80);
}

void move_cursor_back(void) {
    if (cursor_pos > 0) {
        cursor_pos--;
        set_cursor_pos(cursor_pos / 80, cursor_pos % 80);
    }
}

void puts_critical(const char* str) {
    uint16_t* vga = (uint16_t*)VGA_BUFFER;
    while (*str) {
        if (*str == '\n') {
            cursor_pos += VGA_WIDTH - (cursor_pos % VGA_WIDTH);
        } else {
            vga[cursor_pos] = (uint16_t)*str | 0x4F00;
            cursor_pos++;
        }
        if (cursor_pos >= VGA_WIDTH * VGA_HEIGHT) scroll_screen();
        str++;
    }
    set_cursor_pos(cursor_pos / 80, cursor_pos % 80);
}

void dump_reg(const char* name, uint32_t val) {
    printk("%s%x", name, val);
}

void kernel_panic(const char* msg) {

	uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eip, cs, eflags;
	__asm__ volatile ("mov %%eax, %0" : "=r"(eax));
	__asm__ volatile ("mov %%ebx, %0" : "=r"(ebx));
	__asm__ volatile ("mov %%ecx, %0" : "=r"(ecx));
	__asm__ volatile ("mov %%edx, %0" : "=r"(edx));
	__asm__ volatile ("mov %%esi, %0" : "=r"(esi));
	__asm__ volatile ("mov %%edi, %0" : "=r"(edi));
	__asm__ volatile ("mov %%ebp, %0" : "=r"(ebp));
	__asm__ volatile ("mov %%esp, %0" : "=r"(esp));

	uint32_t* frame = (uint32_t*)ebp;
	eip = frame[1];
	cs = 0x08;
	__asm__ volatile ("pushf; pop %0" : "=r"(eflags));
	
	char hex[9];

	puts_critical("Kernel panic - not syncing: ");
	puts_critical(msg);
	puts_critical("\n");

	printk("EAX=%x, EBX=%x, ECX=%x", eax, ebx, ecx);
	printk("EDX=%x, ESI=%x, EDI=%x", edx, esi, edi);
	printk("EBP=%x, ESP=%x, EIP=%x", ebp, esp, eip);
	printk("CS=%x, EFLAGS=%x", cs, eflags);

	puts_critical("end Kernel panic - not syncing: ");
	puts_critical(msg);
	
	while(1) { __asm__ volatile ("hlt"); }
}

void format_uptime(char* buffer, size_t size) {
    uint32_t ticks = (uint32_t)system_ticks;
    uint32_t sec = ticks / 1000;
    uint32_t ms = ticks % 1000;
    
    if (ms < 10)
        string_format(buffer, size, "[   %d.00%d] ", sec, ms);
    else if (ms < 100)
        string_format(buffer, size, "[   %d.0%d] ", sec, ms);
    else
        string_format(buffer, size, "[   %d.%d] ", sec, ms);
}

void sleep_ms(uint32_t ms) {
    uint64_t target = system_ticks + ms;
    while (system_ticks < target) {
        __asm__ volatile ("hlt");
    }
}

void kernel_main() {
    idt_init();
    pit_init(1000);
    __asm__ volatile ("sti");
    
    printk("Kernel starting...");
    atapi_init();
    vfs_init();
    fs_detect_and_mount();

	shell_run();

    //printk("Run /sbin/init");

    //int fd = vfs_open("/sbin/init", 0);
    //if (fd >= 0) {
    //    void* init_addr = (void*)0x200000;
    //    vfs_read(fd, init_addr, 65536);
    //    vfs_close(fd);
        
    //    fd = vfs_open("/bin/sh", 0);
    //    if (fd >= 0) {
    //        void* sh_addr = (void*)0x400000;
    //        vfs_read(fd, sh_addr, 65536);
    //        vfs_close(fd);
    //    }
        
    //    void (*init)() = init_addr;
    //    init();
    //}

    //printk("Init failed, panic!");
    //kernel_panic("Attempted to execute PID 1");
    while(1);
}
