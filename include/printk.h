#ifndef PRINTK_H
#define PRINTK_H

#include <stddef.h>

void printk_init(void);
void printk(const char* format, ...);
void printk_raw(const char* message);

#endif
