#include <string.h>
#include "vfs.h"
#include "fat32.h"
#include "atapi.h"
#include "printk.h"

#define MAX_MOUNTS 8
#define MAX_FILES  16

static fs_type_t* fs_types = NULL;
static mount_t mounts[MAX_MOUNTS];
static int mount_count = 0;
static vfs_file_t open_files[MAX_FILES];

void vfs_init(void) {
    printk("VFS: Initializing virtual filesystem");
    for (int i = 0; i < MAX_FILES; i++) open_files[i].type = -1;
    mount_count = 0;
    printk("VFS: Ready");
}

int vfs_register_filesystem(fs_type_t* fs) {
    if (!fs || !fs->name || !fs->ops) return -1;
    fs->next = fs_types;
    fs_types = fs;
    printk("VFS: Registered filesystem: %s", fs->name);
    return 0;
}

int vfs_mount(const char* path, const char* fs_name) {
    if (mount_count >= MAX_MOUNTS) return -1;
    fs_type_t* fs = fs_types;
    while (fs) {
        if (strcmp(fs->name, fs_name) == 0) break;
        fs = fs->next;
    }
    if (!fs) return -1;
    mounts[mount_count].path = path;
    mounts[mount_count].fs_type = fs;
    mounts[mount_count].fs_data = NULL;
    if (fs->ops->mount) {
        int ret = fs->ops->mount(&mounts[mount_count]);
        if (ret < 0) return ret;
    }
    printk("VFS: Mounted %s at %s", fs_name, path);
    mount_count++;
    return 0;
}

int vfs_open(const char* path, int flags) {
    int fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (open_files[i].type == -1) { fd = i; break; }
    }
    if (fd < 0) return -1;
    for (int i = 0; i < mount_count; i++) {
        size_t path_len = strlen(mounts[i].path);
        if (strncmp(path, mounts[i].path, path_len) == 0) {
            fs_type_t* fs = mounts[i].fs_type;
            if (fs->ops->open_file) {
                void* file_data = fs->ops->open_file(mounts[i].fs_data, path, flags);
                if (file_data) {
                    open_files[fd].type = VFS_FILE;
                    open_files[fd].private_data = file_data;
                    return fd;
                }
            }
        }
    }
    return -1;
}

size_t vfs_read(int fd, void* buf, size_t count) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd].type == -1) {
        printk("vfs_read: bad fd=%d", fd);
        return 0;
    }
    uint32_t* info = (uint32_t*)open_files[fd].private_data;
    uint32_t cluster = info[0];
    uint32_t size = info[1];
    uint32_t offset = info[2];
    
    //printk("vfs_read: cluster=%d size=%d offset=%d", cluster, size, offset);
    
    if (offset >= size) return 0;
    if (count > size - offset) count = size - offset;
    
    uint8_t sector[512];
    uint32_t lba = data_start + (cluster - 2) * sectors_per_cluster;
    
    //printk("vfs_read: lba=%d data_start=%d spc=%d", lba, data_start, sectors_per_cluster);
    
    if (ata_read_sector(lba, sector) != 0) {
        printk("vfs_read: ata_read_sector failed");
        return 0;
    }
    
    for (size_t i = 0; i < count; i++)
        ((uint8_t*)buf)[i] = sector[offset + i];
    info[2] = offset + count;
    //printk("vfs_read: returning %d", count);
    return count;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd].type == -1) return -1;
    open_files[fd].type = -1;
    return 0;
}

int vfs_list_dir(const char* path, char* buf, size_t size) {
    if (!path || !buf || size == 0) return -1;
    for (int i = 0; i < mount_count; i++) {
        size_t path_len = strlen(mounts[i].path);
        if (strncmp(path, mounts[i].path, path_len) == 0) {
            fs_type_t* fs = mounts[i].fs_type;
            if (fs->ops->read_dir)
                return fs->ops->read_dir(mounts[i].fs_data, path, buf, size);
        }
    }
    strcpy(buf, "(empty)\n");
    return 0;
}

int vfs_mkdir(const char* path) {
    for (int i = 0; i < mount_count; i++) {
        size_t path_len = strlen(mounts[i].path);
        if (strncmp(path, mounts[i].path, path_len) == 0) {
            fs_type_t* fs = mounts[i].fs_type;
            if (fs->ops->mkdir)
                return fs->ops->mkdir(mounts[i].fs_data, path);
        }
    }
    return -1;
}

// i added mkdir and list directories because it was before i was doing userspace, like while i was using shell.c to run so that's why they exist, but they'll be used later once the userspace shell will work but i use shell.c for testing aswell
