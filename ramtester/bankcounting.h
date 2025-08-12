/**************************************************************************
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   P2000T-RAMTESTER is free software:                                   *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   P2000T-RAMTESTER software is distributed in the hope that it will    *
 *   be useful, but WITHOUT ANY WARRANTY; without even the implied        *
 *   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#ifndef _BANK_COUNTING_H
#define _BANK_COUNTING_H

#include <stdint.h>
#include <z80.h>

#include "config.h"
#include "terminal.h"
#include "stack.h"

#define NR_SENTINELS    2
#define MAX_SELECTORS 256

void set_bank(uint8_t bank);

// Simple per-(selector,idx) tag: varied across locations, XOR with selector.
uint8_t tag_byte(uint8_t selector, uint8_t idx);
void write_signature(uint8_t selector);
uint8_t verify_signature(uint8_t selector);
uint16_t count_banks(void);

#endif