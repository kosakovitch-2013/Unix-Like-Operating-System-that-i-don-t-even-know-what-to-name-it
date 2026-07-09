#include <stddef.h>
#include <string.h>
#include "iso9660.h"
#include "atapi.h"
#include "printk.h"

// ISO9660 Primary Volume Descriptor
typedef struct {
    uint8_t  type;
    char     id[5];
    uint8_t  version;
    uint8_t  unused1;
    char     system_id[32];
    char     volume_id[32];
    uint8_t  unused2[8];
    uint32_t volume_space_size_le;
    uint32_t volume_space_size_be;
    uint8_t  unused3[32];
    uint16_t volume_set_size_le;
    uint16_t volume_set_size_be;
    uint16_t volume_seq_num_le;
    uint16_t volume_seq_num_be;
    uint16_t sector_size_le;
    uint16_t sector_size_be;
    uint32_t path_table_size_le;
    uint32_t path_table_size_be;
    uint32_t path_table_le;
    uint32_t path_table_opt_le;
    uint32_t path_table_be;
    uint32_t path_table_opt_be;
    uint8_t  root_dir_record[34];
    uint8_t  volume_set_id[128];
    uint8_t  publisher_id[128];
    uint8_t  data_preparer_id[128];
    uint8_t  app_id[128];
    uint8_t  copyright_file[37];
    uint8_t  abstract_file[37];
    uint8_t  biblio_file[37];
    uint8_t  creation_date[17];
    uint8_t  mod_date[17];
    uint8_t  expire_date[17];
    uint8_t  effective_date[17];
    uint8_t  file_struct_version;
} __attribute__((packed)) iso_primary_vd_t;

// Directory record
typedef struct {
    uint8_t  length;
    uint8_t  ext_attr_length;
    uint32_t extent_le;
    uint32_t extent_be;
    uint32_t size_le;
    uint32_t size_be;
    uint8_t  date[7];
    uint8_t  flags;
    uint8_t  file_unit_size;
    uint8_t  interleave_gap;
    uint16_t vol_seq_le;
    uint16_t vol_seq_be;
    uint8_t  name_length;
    char     name[];
} __attribute__((packed)) iso_dir_entry_t;

static uint32_t root_extent;
static uint32_t root_size;

static int iso9660_mount(void* fs_data) {
    (void)fs_data;
	printk("attempting to mount");    
    uint8_t sector[2048];
    
    // Read Primary Volume Descriptor (sector 16)
    if (atapi_read_sector(16, sector) != 0) {
        printk("ISO9660: Failed to read PVD");
        return -1;
    }
printk("attempting to mount2");    
    iso_primary_vd_t* pvd = (iso_primary_vd_t*)sector;
    
    if (strncmp(pvd->id, "CD001", 5) != 0) {
        printk("ISO9660: Not a valid ISO9660 filesystem");
        return -1;
    }

printk("attempting to mount3");

    
    // Get root directory info
    iso_dir_entry_t* root = (iso_dir_entry_t*)pvd->root_dir_record;
    root_extent = root->extent_le;
    root_size = root->size_le;
    
printk("attempting to mount4");


    char vol_id[33];
    memcpy(vol_id, pvd->volume_id, 32);
    vol_id[32] = '\0';
    
printk("attempting to mount5");


    printk("ISO9660: Mounted volume '%s'", vol_id);
    return 0;
}

static int iso9660_unmount(void* fs_data) {
    (void)fs_data;
    return 0;
}

static void* iso9660_open(void* fs_data, const char* path, int flags) {
    (void)fs_data;
    (void)path;
    (void)flags;
    
    uint8_t sector[2048];
    
    // Read root directory
    atapi_read_sector(root_extent, sector);
    
    iso_dir_entry_t* entry = (iso_dir_entry_t*)sector;
    uint8_t* end = sector + root_size;
    
    // Parse directory entries
    while ((uint8_t*)entry < end && entry->length > 0) {
        if (entry->name_length > 0) {
            char name[256];
            int name_len = entry->name_length;
            memcpy(name, entry->name, name_len);
            
            // Remove file version number (;1)
            for (int i = 0; i < name_len; i++) {
                if (name[i] == ';') {
                    name[i] = '\0';
                    break;
                }
            }
            name[name_len] = '\0';
            
            printk("ISO9660: Found file '%s' (size: %d bytes)", name, entry->size_le);
        }
        
        entry = (iso_dir_entry_t*)((uint8_t*)entry + entry->length);
    }
    
    return NULL;
}

static super_ops_t iso9660_ops = {
    .mount = iso9660_mount,
    .unmount = iso9660_unmount,
    .open_file = iso9660_open,
    .read_dir = NULL
};

fs_type_t iso9660_fs_type = {
    .name = "iso9660",
    .ops = &iso9660_ops,
    .next = NULL
};

void iso9660_init(void) {
    printk("ISO9660: Driver initialized");
    vfs_register_filesystem(&iso9660_fs_type);
}
