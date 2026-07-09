#ifndef FAT32_H
#define FAT32_H

#include "vfs.h"

// FAT32 filesystem type
extern fs_type_t fat32_fs_type;
extern uint32_t data_start;
extern uint32_t sectors_per_cluster;


// Initialize FAT32 driver
void fat32_init(void);

#endif
