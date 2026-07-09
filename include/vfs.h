#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_FILE      0
#define VFS_DIRECTORY 1
#define VFS_DEVICE    2

#define O_RDONLY  0x01
#define O_WRONLY  0x02
#define O_RDWR    0x04
#define O_CREAT   0x08

typedef struct file_operations {
    int (*open)(void* file, int flags);
    int (*close)(void* file);
    size_t (*read)(void* file, void* buf, size_t count);
    size_t (*write)(void* file, const void* buf, size_t count);
} file_ops_t;

typedef struct super_operations {
    int (*mount)(void* fs_data);
    int (*unmount)(void* fs_data);
    void* (*open_file)(void* fs_data, const char* path, int flags);
    int (*read_dir)(void* fs_data, const char* path, char* buf, size_t size);
    int (*mkdir)(void* fs_data, const char* path);
} super_ops_t;

typedef struct filesystem_type {
    const char* name;
    super_ops_t* ops;
    struct filesystem_type* next;
} fs_type_t;

typedef struct mount_point {
    const char* path;
    fs_type_t* fs_type;
    void* fs_data;
    struct mount_point* next;
} mount_t;

typedef struct vfs_file {
    int type;
    char name[256];
    void* private_data;
    file_ops_t* f_ops;
} vfs_file_t;

void vfs_init(void);
int vfs_register_filesystem(fs_type_t* fs);
int vfs_mount(const char* path, const char* fs_name);
int vfs_open(const char* path, int flags);
size_t vfs_read(int fd, void* buf, size_t count);
size_t vfs_write(int fd, const void* buf, size_t count);
int vfs_close(int fd);
int vfs_list_dir(const char* path, char* buf, size_t size);
int vfs_mkdir(const char* path);

#endif
