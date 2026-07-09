#include <stdint.h>
#include <string.h>
#include "printk.h"

extern void puts(const char* str);
extern uint64_t get_uptime_ms(void);
extern void format_uptime(char* buffer, size_t size);

typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v,l) __builtin_va_arg(v,l)

static char print_buffer[1024];

static void ksprintf(char* buf, const char* fmt, va_list args) {
    char* b = buf;
    const char* p = fmt;
    
    while (*p) {
        if (*p != '%') {
            *b++ = *p++;
            continue;
        }
        p++;
        
        switch (*p) {
            case 'd': {
                int d = va_arg(args, int);
                char num[32];
                int i = 0, neg = 0;
                if (d < 0) { neg = 1; d = -d; }
                if (d == 0) num[i++] = '0';
                while (d > 0) { num[i++] = '0' + (d % 10); d /= 10; }
                if (neg) *b++ = '-';
                while (i > 0) *b++ = num[--i];
                break;
            }
            case 's': {
                char* s = va_arg(args, char*);
                while (*s) *b++ = *s++;
                break;
            }
            case 'x': {
                unsigned int x = va_arg(args, unsigned int);
                char hex[32];
                int i = 0;
                if (x == 0) hex[i++] = '0';
                while (x > 0) { hex[i++] = "0123456789abcdef"[x % 16]; x /= 16; }
                while (i > 0) *b++ = hex[--i];
                break;
            }
            default:
                *b++ = *p;
                break;
        }
        p++;
    }
    *b = '\0';
}

void printk_init(void) {
    // reversed for future intilialization
}

void printk_raw(const char* message) {
    puts(message);
}

void printk(const char* format, ...) {
    char uptime_str[32];
    format_uptime(uptime_str, sizeof(uptime_str));
    puts(uptime_str);
    
    va_list args;
    va_start(args, format);
    ksprintf(print_buffer, format, args);
    va_end(args);
    
    puts(print_buffer);
    puts("\n");
}
