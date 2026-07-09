#ifndef CPIO_H
#define CPIO_H

#include <stdint.h>
#include <stddef.h>

// New ASCII CPIO format header
typedef struct {
    char magic[6];       // "070701"
    char ino[8];
    char mode[8];
    char uid[8];
    char gid[8];
    char nlink[8];
    char mtime[8];
    char filesize[8];
    char devmajor[8];
    char devminor[8];
    char rdevmajor[8];
    char rdevminor[8];
    char namesize[8];
    char check[8];
} cpio_header_t;

void cpio_init(void* archive);
int cpio_open(const char* path, void** data, size_t* size);
void cpio_list(void);

#endif
