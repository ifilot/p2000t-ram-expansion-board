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

#define MEMEXPNONE  0
#define MEMEXP16    1
#define MEMEXP24    2
#define MEMEXP64    3
#define MEMEXP1056  4
#define MEMEXP2080  5

// forward declarations
void init(void);

void set_bank(uint8_t bank);

void set_bank_highmem(uint8_t bank);

uint8_t read_bank(void);

void write_stack_pointer(void);

void ram_test_01(void);
void ram_test_02(void);
void ram_test_03(void);
void ram_test_04(void);
void ram_test_05(void);

// global variables
uint8_t expansion_type = 0;
uint8_t highmemsectors = 0;       // number of high memory sectors
uint8_t highmembanks = 0;         // number of high memory banks
uint16_t uppermembanks = 0;       // number of upper memory banks

int main(void) {
    init();

    // perform test on high memory
    ram_test_01();

    // if there are no high memory banks, stop here
    if(highmemsectors != 0) {
        ram_test_02();
        ram_test_03();
        ram_test_04();
        ram_test_05();
    }

    print_info("",0);   // print empty line
    print_inline_color("ALL DONE PERFORMING RAM TESTS", COL_CYAN);

    // put in infinite loop
    for(;;){}
}

/*
 * Test 1: Test high memory
 * ========================
 *
 * Perform a quick test where 0x55 is written to 0xA000, 0xB000, 0xC000,
 * and 0xD000 and where it is checked that this byte can be read back.
 */
void ram_test_01(void) {
    print_info("Test 1: High memory", 0);
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
            highmemsectors++;
        } else {
            sprintf(&termbuffer[(i - 0xA)*6], "%c%04X%c", COL_RED, i * 0x1000, COL_WHITE);
        }
    }
    terminal_printtermbuffer();

    if(highmemsectors == 0) {
        expansion_type = 0;
        print_info("No memory expansion card present.", 0);
    }
}

/*
 * Test 2: Bank switching
 * ======================
 *
 * Perform another quick test where a byte is written to 0xE000 and 0xF000
 * where the value written depends on the currently active bank switch. Next,
 * each bank is tested for being able to read this value back.
 * 
 * This test is also used to determine which type of expansion board the user has.
 */
void ram_test_02(void) {
    
    // no bank switching
    set_bank(0);
    memory[0xE000] = (1 << 4);
    memory[0xF000] = (1 << 5);
    if(memory[0xE000] == (1 << 4) && memory[0xF000] == (1 << 5)) {
        uppermembanks++;
    }

    if(uppermembanks == 0) {
        expansion_type = MEMEXP16;
        print_inline_color("16 kb memory expansion detected.", COL_GREEN);
        return;
    }

    // test for initial 6 bank switching
    print_info("Test 2: Bank switching", 0);
    for(uint8_t i=1; i<6; i++) {
        set_bank(i);
        memory[0xE000] = i | (1 << 4);
        memory[0xF000] = i | (1 << 5);
    }

    for(uint8_t i=1; i<6; i++) {
        set_bank(i);
        if(memory[0xE000] == (i | (1 << 4)) && memory[0xF000] == (i | (1 << 5))) {
            uppermembanks++;
        }        
    }
    
    if(uppermembanks == 1) {
        expansion_type = MEMEXP24;
        print_inline_color("24 kb memory expansion detected.", COL_GREEN);
        return;
    }

    // check whether this assessment can be continued to 128 banks
    for(uint8_t i=6; i<128; i++) {
        set_bank(i);
        memory[0xE000] = i;
        memory[0xF000] = i | 0x80;
    }

    // read back and test
    for(uint8_t i=6; i<128; i++) {
        set_bank(i);
        if(memory[0xE000] == i && memory[0xF000] == (i | 0x80)) {
            uppermembanks++;
        } else { // early exit when not possible to avoid generating false positives
            break;
        }
    }

    if(uppermembanks == 6) {
        expansion_type = MEMEXP64;
        print_inline_color("64 kb expansion card detected", COL_GREEN);
        return;
    }

    if(uppermembanks == 128) { // 1Mb+ or 2Mb+ ram expansion
        // there might be bank switching on for upper memory via the MSB on the
        // bank register, test this before proceeding to testing another 128 block
        // part

        // write 0x55 to 0xA000-0xDFFF bank 0
        set_bank(0x00);
        memory[0xA000] = 0x55;
        // write 0xAA to 0xA000-0xDFFF bank 1
        set_bank((uint8_t)(1 << 7));
        memory[0xA000] = 0xAA;

        set_bank(0x00);
        uint8_t exp128_test1 = memory[0xA000] == 0x55 ? 0 : 1;
        set_bank((uint8_t)(1 << 7));
        uint8_t exp128_test2 = memory[0xA000] == 0xAA ? 0 : 1;

        if(exp128_test1 == 0 && exp128_test2 == 0) {
            highmembanks = 1;
            expansion_type = MEMEXP1056;
            print_inline_color("1056 kb expansion card detected", COL_GREEN);
        } else {    // test for 2Mb+ expansion
            for(uint8_t i=255; i>=128; i--) {
                set_bank(i);
                memory[0xE000] = i;
                memory[0xF000] = i | 0x80;
            }

            for(uint8_t i=255; i>=128; i--) {
                set_bank(i);
                if(memory[0xE000] == i && memory[0xF000] == (i | 0x80)) {
                    uppermembanks++;
                } else {  // early exit when not possible to avoid generating false positives
                    break;
                }
            }

            // write 0x55 to 0xA000-0xDFFF bank 0
            set_bank_highmem(0x00);
            memory[0xA000] = 0x55;
            // write 0xAA to 0xA000-0xDFFF bank 1
            set_bank_highmem(0x01);
            memory[0xA000] = 0xAA;

            set_bank_highmem(0x00);
            uint8_t exp256_test1 = memory[0xA000] == 0x55 ? 0 : 1;
            set_bank_highmem(0x01);
            uint8_t exp256_test2 = memory[0xA000] == 0xAA ? 0 : 1;

            if(exp256_test1 == 0 && exp256_test2 == 0) {
                highmembanks = 1;
                expansion_type = MEMEXP2080;
                print_inline_color("2080 kb expansion card detected", COL_GREEN);
            }
        }
    }
}

/*
* Test 3: Reading bank register
* =============================
*
* Final quick test where a value is written to the bank register to swap
* the bank and check whether that value can be read back.
*/
void ram_test_03(void) {
    print_info("Test 3: Reading bank register", 0);
    uint16_t bankschecked = 0;
    for(uint16_t i=0; i<uppermembanks; i++) {
        set_bank(i);
        
        if(read_bank() == i) {
            bankschecked++;
        }
    }
    if(bankschecked == uppermembanks) {
        sprintf(termbuffer, "  %cOK%c Reading bank register", COL_GREEN, COL_WHITE);
    } else {
        sprintf(termbuffer, "  %cFAIL%c Reading bank register", COL_RED, COL_WHITE);
    }
    terminal_printtermbuffer();
}

/*
* Test 4: Test lower and higher memory
* ====================================
*
* Check that values can be written to and read back from lower and upper
* memory
*/
void ram_test_04(void) {
    set_bank(0);
    print_info("Test 4: Lower and higher memory", 0);
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
}

/*
 * Test 5: Bank switchable memory
 * ===================================
 *
 * Check that memory is conserved upon bank switching
 */
void ram_test_05(void) {   
    print_info("Test 5: Bank switching memory", 0);
    print_info("  Writing data to banks", 0);

    for(uint16_t i=0; i<uppermembanks; i++) {
        set_bank(i);
        memset(&memory[BANKMEM_START], 0x55 + i, BANKMEM_STOP - BANKMEM_START + 1);
        sprintf(&termbuffer[(i % 8) * 4], "%c%02X", COL_CYAN, i, COL_WHITE);

        if((i+1) % 8 == 0) {
            terminal_printtermbuffer();
        }
    }

    // also print result when total is not divisible by 8
    if(uppermembanks % 8 != 0) {
        terminal_printtermbuffer();
    }

    print_info("  Testing data on banks", 0);

    for(uint16_t i=0; i<uppermembanks; i++) {
        set_bank(i);
        uint16_t miscounts = count_ram_bytes(&memory[BANKMEM_START], 0x55 + i, BANKMEM_STOP - BANKMEM_START + 1);
        if(miscounts == 0) {
            sprintf(&termbuffer[(i % 8) * 4], "%c%02X", COL_GREEN, i, COL_WHITE);
        } else {
            sprintf(&termbuffer[(i % 8) * 4], "%c%02X", COL_RED, i, COL_WHITE);
        }

        if((i+1) % 8 == 0) {
            terminal_printtermbuffer();
        }
    }

    // also print result when total is not divisible by 8
    if(uppermembanks % 8 != 0) {
        terminal_printtermbuffer();
    }
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

void set_bank_highmem(uint8_t bank) {
    z80_outp(0x95, bank);
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
