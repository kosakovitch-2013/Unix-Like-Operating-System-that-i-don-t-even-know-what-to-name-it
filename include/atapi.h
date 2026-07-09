#ifndef ATAPI_H
#define ATAPI_H

#include <stdint.h>
#include <stddef.h>

void atapi_init(void);
int atapi_read_sector(uint32_t lba, uint8_t* buffer);
int ata_read_sector(uint32_t lba, uint8_t* buffer);
int ata_write_sector(uint32_t lba, uint8_t* buffer);

#endif
