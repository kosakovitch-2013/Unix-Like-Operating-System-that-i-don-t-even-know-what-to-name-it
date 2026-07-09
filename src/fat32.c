#include <stddef.h>
#include <string.h>
#include "fat32.h"
#include "atapi.h"
#include "printk.h"

typedef struct {
    uint8_t  jmp[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} __attribute__((packed)) fat32_bpb_t;

typedef struct {
    char     name[11];
    uint8_t  attr;
    uint8_t  reserved;
    uint8_t  creation_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t mod_time;
    uint16_t mod_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed)) fat32_dir_entry_t;

typedef struct {
    uint8_t  order;
    uint16_t name1[5];
    uint8_t  attr;
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];
    uint16_t zero;
    uint16_t name3[2];
} __attribute__((packed)) fat32_lfn_entry_t;

uint32_t data_start;
uint32_t sectors_per_cluster;
static uint32_t fat_start;
static uint32_t root_cluster;

static int fat32_mount(void* fs_data) {
    (void)fs_data;
    
    uint8_t sector[512];
    
    if (ata_read_sector(0, sector) != 0) {
        printk("FAT32: Failed to read boot sector");
        return -1;
    }
    
    fat32_bpb_t* bpb = (fat32_bpb_t*)sector;
    
    fat_start = bpb->reserved_sectors;
    data_start = fat_start + (bpb->num_fats * bpb->fat_size_32);
    root_cluster = bpb->root_cluster;
    sectors_per_cluster = bpb->sectors_per_cluster;
    
    char label[12];
    memcpy(label, bpb->volume_label, 11);
    label[11] = '\0';
    
    printk("FAT32: Mounted '%s'", label);
    return 0;
}

static uint32_t find_dir_cluster(const char* path) {
    if (!path || strcmp(path, "/") == 0 || strcmp(path, "") == 0)
        return root_cluster;
    
    while (*path == '/') path++;
    if (*path == '\0') return root_cluster;
    
    uint32_t current = root_cluster;
    char token[256];
    
    while (*path) {
        int i = 0;
        while (path[i] && path[i] != '/' && i < 255) {
            token[i] = path[i];
            i++;
        }
        token[i] = '\0';

	for (int t = 0; token[t]; t++)
            if (token[t] >= 'a' && token[t] <= 'z')
                token[t] -= 32;
        
        path += i;
        while (*path == '/') path++;
        
        uint8_t sector[512];
        uint32_t lba = data_start + (current - 2) * sectors_per_cluster;
        if (ata_read_sector(lba, sector) != 0) return 0;
        
        fat32_dir_entry_t* entry = (fat32_dir_entry_t*)sector;
        int found = 0;
        
        for (int j = 0; j < 16; j++) {
            if (entry[j].name[0] == 0x00) break;
            if (entry[j].name[0] == 0xE5) continue;
            if (entry[j].attr == 0x0F) continue;
            if (!(entry[j].attr & 0x10)) continue;
            
            char name[13];
            int p = 0;
            for (int k = 0; k < 8 && entry[j].name[k] != ' '; k++)
                name[p++] = entry[j].name[k];
            name[p] = '\0';
            
            if (strcmp(name, token) == 0) {
                current = entry[j].cluster_low | (entry[j].cluster_high << 16);
                found = 1;
                break;
            }
        }
        
        if (!found) return 0;
    }
    
    return current;
}

static int fat32_read_dir(void* fs_data, const char* path, char* buf, size_t size) {
    (void)fs_data;
    
    uint32_t cluster = find_dir_cluster(path);
    if (cluster == 0) {
        strcpy(buf, "(not found)\n");
        return -1;
    }
    
    uint8_t sector[512];
    uint32_t lba = data_start + (cluster - 2) * sectors_per_cluster;
    
    buf[0] = '\0';
    int offset = 0;
    
    if (ata_read_sector(lba, sector) != 0) return -1;
    
    fat32_dir_entry_t* entry = (fat32_dir_entry_t*)sector;
    
    char lfn_name[256] = {0};
    int lfn_len = 0;
    
    for (int i = 0; i < 16 && offset < (int)size - 32; i++) {
        if (entry[i].name[0] == 0x00) break;
        
        if (entry[i].attr == 0x0F) {
            fat32_lfn_entry_t* lfn = (fat32_lfn_entry_t*)&entry[i];
            int idx = ((lfn->order & 0x3F) - 1) * 13;
            if (idx < 0 || idx > 242) continue;
            for (int j = 0; j < 5; j++)
                if (lfn->name1[j] && idx+j < 256) lfn_name[idx+j] = (char)lfn->name1[j];
            for (int j = 0; j < 6; j++)
                if (lfn->name2[j] && idx+5+j < 256) lfn_name[idx+5+j] = (char)lfn->name2[j];
            for (int j = 0; j < 2; j++)
                if (lfn->name3[j] && idx+11+j < 256) lfn_name[idx+11+j] = (char)lfn->name3[j];
            if (lfn->order & 0x40) lfn_len = idx + 13;
            continue;
        }
        
        if (entry[i].name[0] == 0xE5) { lfn_len = 0; continue; }
        
        char name[256];
        if (lfn_len > 0) {
            int j;
            for (j = 0; j < lfn_len && j < 255; j++) name[j] = lfn_name[j];
            name[j] = '\0';
        } else {
            int p = 0;
            for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) name[p++] = entry[i].name[j];
            if (entry[i].name[8] != ' ') {
                name[p++] = '.';
                for (int j = 8; j < 11 && entry[i].name[j] != ' '; j++) name[p++] = entry[i].name[j];
            }
            name[p] = '\0';
        }
        
        if (entry[i].attr & 0x10)
            offset += string_format(buf + offset, size - offset, "  %s/\n", name);
        else
            offset += string_format(buf + offset, size - offset, "  %s (%d bytes)\n", name, entry[i].size);
        
        lfn_len = 0;
    }
    
    if (offset == 0) strcpy(buf, "(empty)\n");
    return 0;
}

static void* fat32_open(void* fs_data, const char* path, int flags) {
    (void)fs_data;
    (void)flags;
    
    char dir_path[256];
    const char* fname = path;
    
    int last_slash = -1;
    for (int i = 0; path[i]; i++)
        if (path[i] == '/') last_slash = i;
    
    if (last_slash >= 0) {
        for (int i = 0; i < last_slash; i++) dir_path[i] = path[i];
        dir_path[last_slash] = '\0';
        if (last_slash == 0) { dir_path[0] = '/'; dir_path[1] = '\0'; }
        fname = path + last_slash + 1;
    } else {
        dir_path[0] = '/'; dir_path[1] = '\0';
    }
    
    while (*fname == '/') fname++;
    
    uint32_t cluster = find_dir_cluster(dir_path);
	//printk("FAT32: Open dir_path='%s', cluster=%d", dir_path, cluster);
    if (cluster == 0) return NULL;
    
    uint8_t sector[512];
    uint32_t lba = data_start + (cluster - 2) * sectors_per_cluster;
    
    if (ata_read_sector(lba, sector) != 0) return NULL;
    
    fat32_dir_entry_t* entry = (fat32_dir_entry_t*)sector;
    char lfn_name[256] = {0};
    int lfn_len = 0;

    for (int i = 0; i < 16; i++) {
        if (entry[i].name[0] == 0x00) break;
        
        if (entry[i].attr == 0x0F) {
            fat32_lfn_entry_t* lfn = (fat32_lfn_entry_t*)&entry[i];
            int idx = ((lfn->order & 0x3F) - 1) * 13;
            if (idx < 0 || idx > 242) continue;
            for (int j = 0; j < 5; j++)
                if (lfn->name1[j] && idx+j < 256) lfn_name[idx+j] = (char)lfn->name1[j];
            for (int j = 0; j < 6; j++)
                if (lfn->name2[j] && idx+5+j < 256) lfn_name[idx+5+j] = (char)lfn->name2[j];
            for (int j = 0; j < 2; j++)
                if (lfn->name3[j] && idx+11+j < 256) lfn_name[idx+11+j] = (char)lfn->name3[j];
            if (lfn->order & 0x40) lfn_len = idx + 13;
            continue;
        }
        
        if (entry[i].name[0] == 0xE5) { lfn_len = 0; continue; }
        if (entry[i].attr & 0x10) { lfn_len = 0; continue; }
        
        char name[256];
        if (lfn_len > 0) {
            int j;
            for (j = 0; j < lfn_len && j < 255; j++) name[j] = lfn_name[j];
            name[j] = '\0';
        } else {
            int p = 0;
            for (int j = 0; j < 8 && entry[i].name[j] != ' '; j++) name[p++] = entry[i].name[j];
            if (entry[i].name[8] != ' ') {
                name[p++] = '.';
                for (int j = 8; j < 11 && entry[i].name[j] != ' '; j++) name[p++] = entry[i].name[j];
            }
            name[p] = '\0';
        }
        
        if (strcmp(name, fname) == 0) {
            uint32_t* file_data = (uint32_t*)0x90000;
            file_data[0] = entry[i].cluster_low | (entry[i].cluster_high << 16);
            file_data[1] = entry[i].size;
            file_data[2] = 0;
            return file_data;
        }
        lfn_len = 0;
    }
    return NULL;
}

static super_ops_t fat32_ops = {
    .mount = fat32_mount,
    .unmount = NULL,
    .open_file = fat32_open,
    .read_dir = fat32_read_dir,
    .mkdir = fat32_mkdir
};

fs_type_t fat32_fs_type = {
    .name = "fat32",
    .ops = &fat32_ops,
    .next = NULL
};

void fat32_init(void) {
    printk("FAT32: Driver initialized");
    vfs_register_filesystem(&fat32_fs_type);
}
