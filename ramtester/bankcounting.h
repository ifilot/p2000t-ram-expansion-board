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

/**
 * @brief Set the bank in memory, informs the user in a status bar and writes
 *        the current position of the stack pointer to the screen
 * 
 * @param bank id
 */
void set_bank(uint8_t bank);

/**
 * Construct unique identifier byte
 */
uint8_t tag_byte(uint8_t selector, uint8_t idx);

/**
 * Write signature to sentinel addresses on bank identified by selector
 */
void write_signature(uint8_t selector);

/**
 * Checks for all sentinel addresses whether the value is correctly returned
 */
uint8_t verify_signature(uint8_t selector);

/**
 * Count the number of banks by writing a tag value to sentinel addresses. Next,
 * all other banks are screened for aliasing. If an alias is encountered, this means
 * that we have 'wrapped around' in the bank bits and thus no new banks are found.
 * This function performs early exit.
 */
uint16_t count_banks(void);

#endif