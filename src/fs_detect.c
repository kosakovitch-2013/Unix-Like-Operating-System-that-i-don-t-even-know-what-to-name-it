#include <stdint.h>
#include <string.h>
#include "fs_detect.h"
#include "vfs.h"
#include "cpio.h"
#include "fat32.h"
#include "atapi.h"
#include "printk.h"

void fs_detect_and_mount(void) {
    uint8_t sector[512];
    if (ata_read_sector(0, sector) == 0) {
        // Check FAT32 signature (0x55AA at end of boot sector)
        if (sector[510] == 0x55 && sector[511] == 0xAA) {
            printk("FS: Detected FAT32 on disk");
            fat32_init();
            vfs_mount("/", "fat32");
            return;
        }
    }
    
    // Fallback
    printk("FS: No filesystem found!");
}
