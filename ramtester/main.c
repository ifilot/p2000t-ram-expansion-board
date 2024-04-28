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

/*
 * This short application will perform a set of tests on the RAM expansion
 * board of a P2000T. Note that this application assumes that the stack starts
 * at location 0x9FFF and grows downwards (i.e. with decreasing memory address).
 *
 * This program works for both the 64kb expansion board as well as the
 * 1056kb expansion board.
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "config.h"
#include "constants.h"
#include "memory.h"
#include "util.h"
#include "stack.h"
#include "z80.h"
#include "ramtest.h"
#include "terminal.h"

#define TITLELINE 2
#define STARTLINE 5
#define STATUSLINE 19

// forward declarations
void init(void);

void set_bank(uint8_t bank);

uint8_t read_bank(void);

void write_stack_pointer(void);

int main(void) {
    init();

    uint8_t flag_mem1056 = 0; // whether system contains 1056kb of memory

    /*
     * Test 1: Test upper memory
     * =========================
     *
     * Perform a quick test where 0x55 is written to 0xA000, 0xB000, 0xC000,
     * and 0xD000 and where it is checked that this byte can be read back.
     */
    print_info("Test 1: Upper memory", 0);
    for(uint8_t i=0xA; i<=0xD; i++) {

        memory[i * 0x1000] = 0x55;

        for(uint8_t j=0xA; j<=0xD; j++) {
            if(i == j) {
                continue;
            }
            memory[j * 0x1000] = 0x00;
        }

        if(memory[i * 0x1000] == 0x55) {
            sprintf(&termbuffer[(i - 0xA)*6], "%c%04X%c", COL_GREEN, i * 0x1000, COL_WHITE);
        } else {
            sprintf(&termbuffer[(i - 0xA)*6], "%c%04X%c", COL_RED, i * 0x1000, COL_WHITE);
        }
    }
    terminal_printtermbuffer();

    /*
     * Test 2: Bank switching
     * ======================
     *
     * Perform another quick test where a byte is written to 0xE000 and 0xF000
     * where the value written depends on the currently active bank switch. Next,
     * each bank is tested for being able to read this value back.
     *
     * This test is also used to determine whether the user has a 64kb
     * or a 1056 kb expansion board.
     */
    print_info("Test 2: Bank switching", 0);
    for(uint8_t i=0; i<6; i++) {
        set_bank(i);
        memory[0xE000] = i | (1 << 4);
        memory[0xF000] = i | (1 << 5);
    }

    for(uint8_t i=0; i<NUMBANKS; i++) {
        set_bank(i);
        if(memory[0xE000] == (i | (1 << 4)) && memory[0xF000] == (i | (1 << 5))) {
            sprintf(&termbuffer[i*4], "%c%02i%c", COL_GREEN, i, COL_WHITE);
        } else {
            sprintf(&termbuffer[i*4], "%c%02i%c", COL_RED, i, COL_WHITE);
        }        
    }
    terminal_printtermbuffer();

    // check whether this assessment can be continued to 128 banks
    for(uint8_t i=0; i<128; i++) {
        set_bank(i);
        memory[0xE000] = i;
        memory[0xF000] = i | 0x80;
    }

    uint8_t validbanks = 0;
    for(uint8_t i=0; i<128; i++) {
        set_bank(i);
        if(memory[0xE000] == i && memory[0xF000] == (i | 0x80)) {
            validbanks++;
        }
    }

    if(validbanks == 128) {
        flag_mem1056 = 1;
        print_info("Note: 1056kb expansion detected", 0);
    } else {
        validbanks = NUMBANKS;
    }

    /*
     * Test 3: Reading bank register
     * =============================
     *
     * Final quick test where a value is written to the bank register to swap
     * the bank and check whether that value can be read back.
     */
    print_info("Test 3: Reading bank register", 0);
    uint8_t bankschecked = 0;
    for(uint8_t i=0; i<validbanks; i++) {
        set_bank(i);
        
        if(read_bank() == i) {
            bankschecked++;
        }

        if(bankschecked == (i+1)) {
            sprintf(&termbuffer[i*4], "%c%02i%c", COL_GREEN, i, COL_WHITE);
        } else {
            sprintf(&termbuffer[i*4], "%c%02i%c", COL_RED, i, COL_WHITE);
        }
    }
    terminal_printtermbuffer();

    /*
     * Test 4: Test lower and upper memory
     * ===================================
     *
     * Check that values can be written to and read back from lower and upper
     * memory
     */
    set_bank(0);
    print_info("Test 4: Lower and upper memory", 0);
    memset(&memory[LOWMEM], 0x55, STACK - LOWMEM);
    uint16_t lowmem_count = count_ram_bytes(&memory[LOWMEM], 0x55, STACK - LOWMEM);
    memset(&memory[LOWMEM], 0xAA, STACK - LOWMEM);
    lowmem_count += count_ram_bytes(&memory[LOWMEM], 0xAA, STACK - LOWMEM);

    if(lowmem_count == 0) {
        sprintf(termbuffer, "  %04X - %04X: %cOK", LOWMEM, STACK-1, COL_GREEN);
    } else {
        sprintf(termbuffer, "  %04X - %04X: %c%u miscounts", LOWMEM, STACK-1, COL_RED, lowmem_count);
    }
    terminal_printtermbuffer();

    memset(&memory[HIGHMEM_START], 0x55, HIGHMEM_STOP - HIGHMEM_START - 1);
    uint16_t uppermem_count = count_ram_bytes(&memory[HIGHMEM_START], 0x55, HIGHMEM_STOP - HIGHMEM_START);
    memset(&memory[HIGHMEM_START], 0xAA, HIGHMEM_STOP - HIGHMEM_START - 1);
    uppermem_count += count_ram_bytes(&memory[HIGHMEM_START], 0xAA, HIGHMEM_STOP - HIGHMEM_START);

    if(uppermem_count == 0) {
        sprintf(termbuffer, "  %04X - %04X: %cOK", HIGHMEM_START, HIGHMEM_STOP, COL_GREEN);
    } else {
        sprintf(termbuffer, "  %04X - %04X: %c%u miscounts", HIGHMEM_START, HIGHMEM_STOP, COL_RED, uppermem_count);
    }
    terminal_printtermbuffer();

    /*
     * Test 5: Bank switchable memory
     * ===================================
     *
     * Check that memory is conserved upon bank switching
     */
    print_info("Test 5: Bank switching memory", 0);

    for(uint8_t i=0; i<validbanks; i++) {
        set_bank(i);
        memset(&memory[BANKMEM_START], 0x55 + i, BANKMEM_STOP - BANKMEM_START + 1);
    }

    for(uint8_t i=0; i<validbanks; i++) {
        set_bank(i);
        uint16_t miscounts = count_ram_bytes(&memory[BANKMEM_START], 0x55 + i, BANKMEM_STOP - BANKMEM_START + 1);
        if(miscounts == 0) {
            sprintf(&termbuffer[(i % 8) * 4], "%c%02X%c", COL_GREEN, i, COL_WHITE);
        } else {
            sprintf(&termbuffer[(i % 8) * 4], "%c%02X%c", COL_RED, i, COL_WHITE);
        }

        if((i+1) % 8 == 0) {
            terminal_printtermbuffer();
        }
    }

    // also print result when total is not divisable by 8
    if(validbanks % 8 != 0) {
        terminal_printtermbuffer();
    }

    // put in infinite loop
    for(;;){}
}

/**
 * @brief Set the bank in memory, informs the user in a status bar and writes
 *        the current position of the stack pointer to the screen
 * 
 * @param bank id
 */
void set_bank(uint8_t bank) {
    z80_outp(0x94, bank);

    // placeholder for bit pattern (terminating char = 0)
    uint8_t char_bits[9] = {' ',' ',' ',' ',' ',' ',' ',' ',0};

    // build bit pattern
    for(uint8_t i=0; i<8; i++) {
        if((bank & (1 << i)) != 0) {
            char_bits[7-i] = GRAPH_BLOCK;
        } else {
            char_bits[7-i] = ' '; // write space
        }
    }

    memset(vidmem, 0, 40);
    vidmem[0x00] = COL_MAGENTA;
    sprintf(&vidmem[1], "Bank register: |%s| (%i)", char_bits, bank);
    write_stack_pointer();
}

/**
 * @brief Read the current bank from the bank register
 * 
 * @return uint8_t bank id
 */
uint8_t read_bank(void) {
    return z80_inp(0x94);
}

/**
 * @brief Writes the current stack position to the screen
 */
void write_stack_pointer(void) {
    uint16_t stackptr = get_stack_pointer();
    vidmem[0x50] = COL_MAGENTA;
    sprintf(&vidmem[0x50+1], "Stack pointer: %04X", stackptr);
}

/**
 * @brief Initialize the environment
 */
void init(void) {
    clear_screen();
    terminal_init(3, 20);
    vidmem[0x50] = TEXT_DOUBLE;
    vidmem[0x50+1] = COL_CYAN;
    sprintf(&vidmem[0x50+2], "RAM TESTER");

    // insert cursor
    sprintf(termbuffer, "%c>%c", COL_CYAN, COL_WHITE);
    terminal_redoline();
    
    set_bank(0);    // always set bank 0 upon initialization
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
