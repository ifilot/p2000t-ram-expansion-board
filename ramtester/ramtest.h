#ifndef _RAMTEST_H
#define _RAMTEST_H

#include <stdint.h>

uint16_t count_ram_bytes(char *memory, uint8_t val, uint16_t nrbytes) __z88dk_callee;

#endif // _RAMTEST_H