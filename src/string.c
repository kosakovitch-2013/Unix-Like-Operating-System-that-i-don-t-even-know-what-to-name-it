#include <stddef.h>
#include <string.h>
#include <stdint.h>

int strcmp(const char *a, const char *b) {
	if (!a || !b) {
		if (a == b) return 0;
		return a ? 1 : -1;
	}
	size_t bytes = 0;
	while (*a != '\0' && *a == *b && bytes < 16777216/32) { a++; b++; bytes++; }
	return (*(unsigned char *)a) - (*(unsigned char *)b);
}

int strncmp(const char *a, const char *b, unsigned int count) {
	if (!a || !b) {
		if (a == b) return 0;
		return a ? 1 : -1;
	}
	size_t bytes = 0;
	while (*a != '\0' && *a == *b && bytes < 16777216/32 && bytes+1 < count) { a++; b++; bytes++; }
	return (*(unsigned char *)a) - (*(unsigned char *)b);
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++));
    return dest;
}

size_t strlen(const char* str) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    if (!dest || !src) return dest;
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

void* memcpy(void* restrict dest, const void* restrict src, size_t size) {
    if (!dest || !src) return dest;
    unsigned char* d = dest;
    const unsigned char* s = src;
    while (size--) *d++ = *s++;
    return dest;
}

int string_format(char* buffer, size_t buffer_size, const char* format, ...) {
    if (!buffer || buffer_size == 0) return -1;
    
    typedef __builtin_va_list va_list;
    va_list args;
    __builtin_va_start(args, format);
    
    char* buf = buffer;
    const char* fmt = format;
    size_t remaining = buffer_size - 1;
    
    while (*fmt && remaining > 0) {
        if (*fmt != '%') {
            *buf++ = *fmt++;
            remaining--;
            continue;
        }
        fmt++;
        
        if (*fmt == 'd') {
            int d = __builtin_va_arg(args, int);
            char num[32];
            int i = 0, neg = 0;
            if (d < 0) { neg = 1; d = -d; }
            if (d == 0) num[i++] = '0';
            while (d > 0) { num[i++] = '0' + (d % 10); d /= 10; }
            if (neg && remaining > 0) { *buf++ = '-'; remaining--; }
            while (i > 0 && remaining > 0) { *buf++ = num[--i]; remaining--; }
        } else if (*fmt == 's') {
            char* s = __builtin_va_arg(args, char*);
            while (*s && remaining > 0) { *buf++ = *s++; remaining--; }
        }
        fmt++;
    }
    
    *buf = '\0';
    __builtin_va_end(args);
    return buffer_size - remaining - 1;
}
