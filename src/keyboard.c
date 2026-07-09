#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a" (data) : "dN" (port));
    return data;
}

char keyboard_read_char(void) {
    static const char lowercase[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
        '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
        'm', ',', '.', '/', 0, '*', 0, ' '
    };
    
    static const char uppercase[] = {
        0, 0, '!', '@', '#', '$', '%', '^', '&', '*',
        '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R',
        'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
        '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', '<', '>', '?', 0, '*', 0, ' '
    };
    
    static int shift = 0;
    static int caps = 0;
    
    while (1) {
        while ((inb(0x64) & 1) == 0);
        
        uint8_t scancode = inb(0x60);
        
        // Shift pressed
        if (scancode == 0x2A || scancode == 0x36) {
            shift = 1;
            continue;
        }
        // Shift released
        if (scancode == 0xAA || scancode == 0xB6) {
            shift = 0;
            continue;
        }
        // Caps lock
        if (scancode == 0x3A) {
            caps = !caps;
            continue;
        }
        
        if (scancode < 58) {
            if (shift ^ caps) {
                if (uppercase[scancode]) return uppercase[scancode];
            } else {
                if (lowercase[scancode]) return lowercase[scancode];
            }
        }
    }
}
