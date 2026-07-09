#include <stdint.h>
#include "atapi.h"
#include "printk.h"

#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTORS     0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

static inline uint8_t inb(uint16_t port) {
    uint8_t data;
    __asm__ volatile ("inb %1, %0" : "=a"(data) : "dN"(port));
    return data;
}

static inline void outb(uint16_t port, uint8_t data) {
    __asm__ volatile ("outb %0, %1" : : "a"(data), "dN"(port));
}

static inline void insw(uint16_t port, void* buf, int count) {
    __asm__ volatile ("rep insw" : "+D"(buf), "+c"(count) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, const void* buf, int count) {
    __asm__ volatile ("rep outsw" : "+S"(buf), "+c"(count) : "d"(port) : "memory");
}

static int wait_ready(void) {
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS);
        if (!(status & 0x80)) {
            //printk("wait_ready: OK status=%x", status);
            return 0;
        }
        if (status & 0x01) {
            printk("wait_ready: ERR status=%x", status);
            return -1;
        }
    }
    printk("wait_ready: TIMEOUT");
    return -1;
}

static int wait_data(void) {
    int timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(ATA_STATUS);
        if (status & 0x08) {
            //printk("wait_data: OK status=%x", status);
            return 0;
        }
        if (status & 0x01) {
            printk("wait_data: ERR status=%x", status);
            return -1;
        }
    }
    printk("wait_data: TIMEOUT");
    return -1;
}

void atapi_init(void) {
    printk("ATAPI: Initializing...");
    outb(ATA_DRIVE, 0xA0);
    for (int i = 0; i < 4; i++) inb(ATA_STATUS);
    if (wait_ready() == 0) {
        printk("ATAPI: Device found on primary master");
        return;
    }
    outb(ATA_DRIVE, 0xB0);
    for (int i = 0; i < 4; i++) inb(ATA_STATUS);
    if (wait_ready() == 0) {
        printk("ATAPI: Device found on primary slave");
        return;
    }
    printk("ATAPI: No device found");
}

int atapi_read_sector(uint32_t lba, uint8_t* buffer) {
    if (wait_ready() != 0) return -1;
    outb(ATA_DRIVE, 0xA0);
    for (int i = 0; i < 4; i++) inb(ATA_STATUS);
    if (wait_ready() != 0) return -1;
    outb(ATA_SECTORS, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, 0xA0);
    if (wait_ready() != 0) return -1;
    uint8_t packet[12] = { 0xA8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
    packet[2] = (lba >> 24) & 0xFF;
    packet[3] = (lba >> 16) & 0xFF;
    packet[4] = (lba >> 8) & 0xFF;
    packet[5] = lba & 0xFF;
    outsw(ATA_DATA, packet, 6);
    if (wait_data() != 0) return -1;
    insw(ATA_DATA, buffer, 1024);
    return 0;
}

int ata_read_sector(uint32_t lba, uint8_t* buffer) {
    //printk("ata_read: lba=%d", lba);
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    for (int i = 0; i < 4; i++) inb(ATA_STATUS);
    
    if (wait_ready() != 0) {
        printk("ata_read: not ready");
        return -1;
    }
    
    outb(ATA_SECTORS, 1);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, 0x20);
    
    if (wait_data() != 0) {
        printk("ata_read: no data");
        return -1;
    }
    
    insw(ATA_DATA, buffer, 256);
    //printk("ata_read: done");
    return 0;
}

int ata_write_sector(uint32_t lba, uint8_t* buffer) {
    if (wait_ready() != 0) return -1;
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTORS, 1);
    outb(ATA_LBA_LOW, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, 0x30);
    if (wait_data() != 0) return -1;
    outsw(ATA_DATA, buffer, 256);
    return 0;
}
