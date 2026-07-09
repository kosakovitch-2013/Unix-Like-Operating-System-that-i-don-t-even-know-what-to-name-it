#include <stdint.h>
#include <string.h>
#include "cpio.h"
#include "printk.h"

static void* cpio_archive = NULL;

static uint32_t hex8_to_uint32(const char* hex) {
    uint32_t val = 0;
    for (int i = 0; i < 8; i++) {
        char c = hex[i];
        val <<= 4;
        if (c >= '0' && c <= '9') val |= c - '0';
        else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
    }
    return val;
}

void cpio_init(void* archive) {
    cpio_archive = archive;
    printk("CPIO: Initialized at 0x%x", archive);
}

int cpio_open(const char* path, void** data, size_t* size) {
    if (!cpio_archive || !path) return -1;
    
    cpio_header_t* header = (cpio_header_t*)cpio_archive;
    
    while (1) {
        uint32_t namesize = hex8_to_uint32(header->namesize);
        uint32_t filesize = hex8_to_uint32(header->filesize);
        
        char* name = (char*)(header + 1);
        
        // Check for end marker
        if (strcmp(name, "TRAILER!!!") == 0) break;
        
        if (strcmp(name, path) == 0) {
            *data = (void*)((uint8_t*)(header + 1) + namesize);
            *size = filesize;
            printk("CPIO: Opened '%s', size=%d", name, filesize);
            return 0;
        }
        
        // Next entry
        uint32_t total = sizeof(cpio_header_t) + namesize + filesize;
        total = (total + 3) & ~3;  // Align to 4 bytes
        header = (cpio_header_t*)((uint8_t*)header + total);
    }
    
    return -1;
}

void cpio_list(void) {
    if (!cpio_archive) return;
    
    cpio_header_t* header = (cpio_header_t*)cpio_archive;
    
    while (1) {
        uint32_t namesize = hex8_to_uint32(header->namesize);
        uint32_t filesize = hex8_to_uint32(header->filesize);
        
        char* name = (char*)(header + 1);
        
        if (strcmp(name, "TRAILER!!!") == 0) break;
        
        printk("CPIO: %s (%d bytes)", name, filesize);
        
        uint32_t total = sizeof(cpio_header_t) + namesize + filesize;
        total = (total + 3) & ~3;
        header = (cpio_header_t*)((uint8_t*)header + total);
    }
}
