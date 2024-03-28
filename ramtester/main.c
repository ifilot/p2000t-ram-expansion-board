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

#define TITLELINE 2
#define STARTLINE 5
#define STATUSLINE 19

// forward declarations
void init(void);

void set_bank(uint8_t bank);

uint8_t read_bank(void);

void write_stack_pointer(void);

uint8_t test_memory(const uint16_t start, const uint16_t stop, 
                    uint8_t testbyte, const uint16_t progressbyte);

void write_memory(const uint16_t start, const uint16_t stop, 
                  uint8_t testbyte, const uint16_t progressbyte);

uint8_t read_memory(const uint16_t start, const uint16_t stop, 
                    uint8_t testbyte, const uint16_t progressbyte);

int main(void) {
    init();

    vidmem[0x50*TITLELINE] = TEXT_DOUBLE;
    vidmem[0x50*TITLELINE+1] = COL_CYAN;
    uint8_t nrchars = sprintf(&vidmem[0x50 * TITLELINE + 2], "RAM TESTER");
    vidmem[0x50*TITLELINE + 2 + nrchars] = COL_NONE;
    uint8_t mem1056 = 0;

    // keep track of line number
    uint8_t linenr = STARTLINE;

    /*
     * Test 1: Test upper memory
     * =========================
     *
     * Perform a quick test where 0x55 is written to 0xA000, 0xB000, 0xC000,
     * and 0xD000 and where it is checked that this byte can be read back.
     */
    sprintf(&vidmem[0x50 * linenr], "Test 1: Upper memory");
    linenr++;
    for(uint8_t i=0xA; i<=0xD; i++) {

        memory[i * 0x1000] = 0x55;

        for(uint8_t j=0xA; j<=0xD; j++) {
            if(i == j) {
                continue;
            }
            memory[j * 0x1000] = 0x00;
        }

        if(memory[i * 0x1000] == 0x55) {
            vidmem[0x50 * linenr + (i - 0xA)*5] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr + (i - 0xA)*5] = COL_RED;
        }
        sprintf(&vidmem[0x50 * linenr + (i - 0xA) * 5 + 1], "%04X", i * 0x1000);
    }

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
    linenr++;
    sprintf(&vidmem[0x50 * (linenr)], "Test 2: Bank switching");
    for(uint8_t i=0; i<6; i++) {
        set_bank(i);
        memory[0xE000] = i | (1 << 4);
        memory[0xF000] = i | (1 << 5);
    }

    linenr++;
    for(uint8_t i=0; i<NUMBANKS; i++) {
        set_bank(i);
        if(memory[0xE000] == (i | (1 << 4))) {
            vidmem[0x50 * linenr+(i*6)] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr+(i*6)] = COL_RED;
        }
        sprintf(&vidmem[0x50 * linenr+(i*6+1)], "%1iE", i);

        if(memory[0xF000] == (i | (1 << 5))) {
            vidmem[0x50 * linenr+(i*6+3)] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr+(i*6+3)] = COL_RED;
        }
        sprintf(&vidmem[0x50 * linenr+(i*6+4)], "%1iF", i);
    }

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
        uint8_t mem1056 = 1;
        memset(&vidmem[0x50 * linenr], 0, 0x50);
        vidmem[0x50 * linenr] = COL_GREEN;
        sprintf(&vidmem[0x50 * linenr +1 ], "-- 1056kb expansion detected --");
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
    linenr++;
    sprintf(&vidmem[0x50 * linenr], "Test 3: Reading bank register");
    linenr++;
    uint8_t bankschecked = 0;
    for(uint8_t i=0; i<validbanks; i++) {
        set_bank(i);
        
        if(read_bank() == i) {
            bankschecked++;
        }

        if(bankschecked == (i+1)) {
            vidmem[0x50 * linenr] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr+(i*3)] = COL_RED;
        }

        memset(&vidmem[0x50 * linenr+1], 0x00, 40);
        sprintf(&vidmem[0x50 * linenr+1], "Valid bank responses: %i / %i", bankschecked, validbanks);
    }

    /*
     * Test 4: Test lower and upper memory
     * ===================================
     *
     * Check that values can be written to and read back from lower and upper
     * memory
     */

    linenr++;
    sprintf(&vidmem[0x50 * linenr], "Test 4: Lower and upper memory");
    linenr++;

    set_bank(0x00);
    uint8_t lowmem_flag_ok = test_memory(LOWMEM, STACK, 0x55, 0x50 * linenr + 25) |
                             test_memory(LOWMEM, STACK, 0xAA, 0x50 * linenr + 25);

    if(lowmem_flag_ok == 0) {
        vidmem[0x50 * linenr] = COL_GREEN;
    } else {
        vidmem[0x50 * linenr] = COL_RED;
    }
    sprintf(&vidmem[0x50 * linenr+1], "0x%04X-0x%04X", LOWMEM, STACK);

    linenr++;

    uint8_t highmem_flag = test_memory(HIGHMEM_START, HIGHMEM_STOP+1, 0x55, 0x50 * linenr + 25) |
                           test_memory(HIGHMEM_START, HIGHMEM_STOP+1, 0xAA, 0x50 * linenr + 25);

    if(highmem_flag == 0) {
        vidmem[0x50 * linenr] = COL_GREEN;
    } else {
        vidmem[0x50 * linenr] = COL_RED;
    }
    sprintf(&vidmem[0x50 * linenr + 1], "0x%04X-0x%04X (BIT 0)", HIGHMEM_START, HIGHMEM_STOP);
    memset(&vidmem[0x50 * linenr + 25], 0x00, 15);

    linenr++;

    if(validbanks == 128) {
        set_bank(0x80);
        uint8_t highmem_flag = test_memory(HIGHMEM_START, HIGHMEM_STOP+1, 0x55, 0x50 * linenr + 25) |
                               test_memory(HIGHMEM_START, HIGHMEM_STOP+1, 0xAA, 0x50 * linenr + 25);
        if(highmem_flag == 0) {
            vidmem[0x50 * linenr] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr] = COL_RED;
        }
        sprintf(&vidmem[0x50 * linenr + 1], "0x%04X-0x%04X (BIT 1)", HIGHMEM_START, HIGHMEM_STOP);
        memset(&vidmem[0x50 * linenr + 25], 0x00, 15);

        set_bank(0x00);
    }

    /*
     * Test 5: Bank invariant memory check
     * ===================================
     *
     * Check that memory can be written to and read back from lower and upper
     * memory that is invariant under a bank switch.
     */

    linenr++;
    sprintf(&vidmem[0x50 * linenr], "Test 5: Memory invariance");
    linenr++;

    set_bank(0);
    write_memory(HIGHMEM_START, HIGHMEM_START+0x1000, 0x55, 0x50 * linenr + 25);
    write_memory(0x7000, 0x8000, 0x55, 0x50 * linenr + 25);
    uint8_t test5_flag = 0;
    for(uint8_t i=1; i<NUMBANKS; i++) {
        set_bank(i);
        test5_flag |= read_memory(HIGHMEM_START, HIGHMEM_START+0x1000, 0x55, 0x50 * linenr + 25);
        test5_flag |= read_memory(0x7000, 0x8000, 0x55, 0x50 * linenr + 25);
    }

    if(test5_flag == 0) {
        vidmem[0x50 * linenr] = COL_GREEN;
        sprintf(&vidmem[0x50 * linenr+1], "PASS", LOWMEM, STACK);
    } else {
        vidmem[0x50 * linenr] = COL_RED;
        sprintf(&vidmem[0x50 * linenr+1], "FAIL", LOWMEM, STACK);
    }

    memset(&vidmem[0x50 * linenr + 25], 0x00, 15);

    /*
     * Test 6: Full banked memory test
     * ===============================
     *
     * Check that memory can be written to and read back from the bankable
     * memory
     */

    linenr++;
    sprintf(&vidmem[0x50 * linenr], "Test 6: Bankable memory");
    linenr++;

    // test immediate writing and reading back
    bankschecked = 0;
    for(uint8_t i=0; i<validbanks; i++) {
        set_bank(i);
        uint8_t testflag_bank = test_memory(BANKMEM_START, BANKMEM_STOP, 0x80 | i, 0x50 * linenr + 25);
        
        if(testflag_bank == 0) {
            bankschecked++;
        }

        if(bankschecked == (i+1)) {
            vidmem[0x50 * linenr] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr] = COL_RED;
        }

        sprintf(&vidmem[0x50 * linenr+1], "(T1) Bank: %i / %i", i+1, validbanks);
    }

    linenr++;

    // test preservation in second loop
    bankschecked = 0;
    for(uint8_t i=0; i<validbanks; i++) {
        set_bank(i);
        uint8_t testflag_bank = read_memory(BANKMEM_START, BANKMEM_STOP, 0x80 | i, 0x50 * linenr + 25);
        
        if(testflag_bank == 0) {
            bankschecked++;
        }

        if(bankschecked == (i+1)) {
            vidmem[0x50 * linenr] = COL_GREEN;
        } else {
            vidmem[0x50 * linenr] = COL_RED;
        }

        sprintf(&vidmem[0x50 * linenr+1], "(T2) Bank: %i / %i", i+1, validbanks);
    }

    /*
     * Done testing
     * ============
     */

    linenr++;
    vidmem[0x50 * linenr] = COL_GREEN;
    sprintf(&vidmem[0x50 * linenr+1], "TESTS COMPLETED");

    return 0;
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
 * @brief Writes byte to memory and check that it can be read back
 * 
 * @param start start position in memory
 * @param stop stop position in memory
 * @param testbyte byte to be written
 * @param progressbyte position in video memory to write progress to
 * @return uint8_t whether testbyte is correctly written to memory locations
 */
uint8_t test_memory(const uint16_t start, const uint16_t stop, 
                    const uint8_t testbyte, const uint16_t progressbyte) {
    
    // update stack pointer
    write_stack_pointer();

    // write a byte to the memory and read it back
    write_memory(start, stop, testbyte, progressbyte);
    return read_memory(start, stop, testbyte, progressbyte);
}

/**
 * @brief Write a byte to memory locations and output progress
 * 
 * @param start start position in memory
 * @param stop stop position in memory
 * @param testbyte byte to be written
 * @param progressbyte position in video memory to write progress to
 */
void write_memory(const uint16_t start, const uint16_t stop, 
                  const uint8_t testbyte, const uint16_t progressbyte) {

    // update stack pointer
    write_stack_pointer();

    // write value to memory
    for(uint16_t i=start; i<stop; i+=0x100) {
        for(uint16_t j=0; j<0x100; j++) {
            memory[i+j] = testbyte;
        }
        vidmem[progressbyte] = COL_WHITE;
        sprintf(&vidmem[progressbyte+1], "(W:%02Xxx:%02X)", (i >> 8 & 0xFF), testbyte);

        // overflow guard
        if(i >= 0xFF00) {
            break;
        }
    }
}

/**
 * @brief Test that a byte has been succesfully written to memory locations
 * 
 * @param start start position in memory
 * @param stop stop position in memory
 * @param testbyte byte to be written
 * @param progressbyte position in video memory to write progress to
 * @return uint8_t whether testbyte is correctly written to memory locations
 */
uint8_t read_memory(const uint16_t start, const uint16_t stop, 
                  const uint8_t testbyte, const uint16_t progressbyte) {

    // update stack pointer
    write_stack_pointer();

    // flag that all bytes are written and read back correctly
    uint8_t flag_ok = 0;    

    // read value back
    for(uint16_t i=start; i<stop; i+=0x100) {
        for(uint16_t j=0; j<0x100; j++) {
            if(memory[i+j] != testbyte) {
                flag_ok = 1;
                sprintf(&vidmem[0x50*21], "ERROR: 0x%04X: %02X != %02X", i+j, memory[i+j], testbyte);
            }
        }
        vidmem[progressbyte] = COL_WHITE;
        sprintf(&vidmem[progressbyte+1], "(R:%02Xxx:%02X)", (i >> 8 & 0xFF), testbyte);

        // overflow guard
        if(i >= 0xFF00) {
            break;
        }
    }

    return flag_ok;
}

/**
 * @brief Initialize the environment
 */
void init(void) {
    clearline(0);
    set_bank(0);    // always set bank 0 upon initialization
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
