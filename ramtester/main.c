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
 * This program works for both the 64KiB expansion board as well as the
 * 1056KiB expansion board.
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
#include "bankcounting.h"

#define MEMEXPNONE  0       // no expansion
#define MEMEXP16    1       // A000-DFFF, no banking
#define MEMEXP24    2       // A000-FFFF, no banking
#define MEMEXP64    3       // A000-FFFF, 6 banks
#define MEMEXP128   4       // A000-FFFF, 14 banks
#define MEMEXP256   5       // A000-FFFF, 30 banks
#define MEMEXP384   6       // A000-FFFF, 46 banks
#define MEMEXP512   7       // A000-FFFF, 62 banks
#define MEMEXP1056  8
#define MEMEXP2080  9

uint8_t test_passed[5];

// forward declarations
void init(void);

void set_bank_highmem(uint8_t bank);

uint8_t read_bank(void);

void ram_test_01(void);
void ram_test_02(void);
void ram_test_03(void);
void ram_test_04(void);
void ram_test_05(void);
void ram_test_06(void);
void ram_test_07(void);

uint8_t test_bank_helper(uint8_t startbank, uint8_t stopbank, uint8_t *uppermembanks,
                         uint8_t *expansion_type, uint8_t banktypefail, uint16_t szdetect);
void test_fixed_pattern(uint8_t pattern, uint8_t check_id);
void write_termbuffer_value(uint8_t i, uint8_t color);
static uint8_t fingerprint(uint8_t i) { return (uint8_t)(0xA5u ^ i); }

// global variables
uint8_t expansion_type = 0;
uint8_t highmemsectors = 0;       // number of high memory sectors
uint8_t highmembanks = 0;         // number of high memory banks
uint16_t uppermembanks = 0;       // number of upper memory banks

int main(void) {
    init();

    // reset passed tests array
    memset(test_passed, 0x00, 5);

    // perform test on high memory
    ram_test_01();

    // if there are no high memory banks, stop here
    if(highmemsectors != 0) {
        ram_test_02();        
        ram_test_03();
        ram_test_04();
        ram_test_05();
        ram_test_06();
        ram_test_07();
    }

    print_info("",0);   // print empty line
    print_inline_color("-= ALL DONE PERFORMING RAM TESTS =-", COL_CYAN);

    // show summary
    print_info("",0);   // print empty line
    print_inline_color("-= SUMMARY =-", COL_CYAN);
    char buf[50];
    for(uint8_t i=0; i<5; i++) {
        if(test_passed[i] == 0) {
            sprintf(buf, "  * TEST %u: %cPASSED%c", i+1, COL_GREEN, COL_WHITE);
            print_info(buf, 0);
        } else {
            sprintf(buf, "  * TEST %u: %cFAILED%c; %u ERROR(S) ENCOUNTERED", i+1, COL_RED, COL_WHITE, test_passed[i]);
            print_info(buf, 0);
        }
        
    }
    write_stack_pointer();

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
    print_info("Test 2: Determine number of RAM banks", 0);
    uppermembanks = count_banks();
    sprintf(termbuffer, "%c%u%c RAM banks found", COL_CYAN, uppermembanks, COL_WHITE);
    terminal_printtermbuffer();

    switch(uppermembanks) {
        case 0:
            print_inline_color("16 KiB memory expansion detected", COL_CYAN);
        break;
        case 1:
            print_inline_color("32 KiB memory expansion detected", COL_CYAN);
        break;
        case 6:
            print_inline_color("64 KiB memory expansion detected", COL_CYAN);
        break;
        case 14:
            print_inline_color("128 KiB memory expansion detected", COL_CYAN);
        break;
        case 30:
            print_inline_color("256 KiB memory expansion detected", COL_CYAN);
        break;
        case 46:
            print_inline_color("384 KiB memory expansion detected", COL_CYAN);
        break;
        case 62:
            print_inline_color("512 KiB memory expansion detected", COL_CYAN);
        break;
        case 128:
            print_inline_color("1056 KiB memory expansion detected", COL_CYAN);
        break;
        case 256:
            print_inline_color("2080 KiB memory expansion detected", COL_CYAN);
        break;
        default:
            print_inline_color("Unknown memory expansion, please inform developer", COL_RED);
        break;
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
    memset(&memory[LOWMEM], 0x00, STACK - LOWMEM);
    lowmem_count += count_ram_bytes(&memory[LOWMEM], 0x00, STACK - LOWMEM);
    memset(&memory[LOWMEM], 0xFF, STACK - LOWMEM);
    lowmem_count += count_ram_bytes(&memory[LOWMEM], 0xFF, STACK - LOWMEM);

    if(lowmem_count == 0) {
        sprintf(termbuffer, "  0x%04X - 0x%04X: %cOK", LOWMEM, STACK-1, COL_GREEN);
    } else {
        sprintf(termbuffer, "  0x%04X - 0x%04X: %c%u miscounts", LOWMEM, STACK-1, COL_RED, lowmem_count);
    }
    terminal_printtermbuffer();

    memset(&memory[HIGHMEM_START], 0x55, HIGHMEM_STOP - HIGHMEM_START - 1);
    uint16_t uppermem_count = count_ram_bytes(&memory[HIGHMEM_START], 0x55, HIGHMEM_STOP - HIGHMEM_START);
    memset(&memory[HIGHMEM_START], 0xAA, HIGHMEM_STOP - HIGHMEM_START - 1);
    uppermem_count += count_ram_bytes(&memory[HIGHMEM_START], 0xAA, HIGHMEM_STOP - HIGHMEM_START);
    memset(&memory[HIGHMEM_START], 0x00, HIGHMEM_STOP - HIGHMEM_START - 1);
    uppermem_count += count_ram_bytes(&memory[HIGHMEM_START], 0x00, HIGHMEM_STOP - HIGHMEM_START);
    memset(&memory[HIGHMEM_START], 0xFF, HIGHMEM_STOP - HIGHMEM_START - 1);
    uppermem_count += count_ram_bytes(&memory[HIGHMEM_START], 0xFF, HIGHMEM_STOP - HIGHMEM_START);

    if(uppermem_count == 0) {
        sprintf(termbuffer, "  0x%04X - 0x%04X: %cOK", HIGHMEM_START, HIGHMEM_STOP, COL_GREEN);
    } else {
        sprintf(termbuffer, "  0x%04X - 0x%04X: %c%u miscounts", HIGHMEM_START, HIGHMEM_STOP, COL_RED, uppermem_count);
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
        uint8_t t = tag_byte(0x00, (uint8_t)i);
        memset(&memory[BANKMEM_START], t, BANKMEM_STOP - BANKMEM_START + 1);
        write_termbuffer_value((uint8_t)i, COL_CYAN);

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
        uint8_t t = tag_byte(0x00, (uint8_t)i);
        uint16_t miscounts = count_ram_bytes(&memory[BANKMEM_START], t, BANKMEM_STOP - BANKMEM_START + 1);
        if(miscounts == 0) {
            write_termbuffer_value(i, COL_GREEN);
        } else {
            write_termbuffer_value((uint8_t)i, COL_RED);
            test_passed[0]++;
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

/*
 * Test 6: Checkerboard test
 * ===================================
 *
 * Check that memory is conserved upon bank switching
 */
void ram_test_06(void) {   
    print_info("Test 6: Checkerboard test", 0);
    test_fixed_pattern(0x55, 1);
    test_fixed_pattern(0xAA, 2);
}

/*
 * Test 7: Stuck-at-transition
 * ===================================
 *
 * Check that memory is conserved upon bank switching
 */
void ram_test_07(void) {   
    print_info("Test 7: Stuck at transition", 0);
    test_fixed_pattern(0x00, 3);
    test_fixed_pattern(0xFF, 4);
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
 * Perform a series of simple read/write tests on banks to determine the total
 * number of banks.
 */
uint8_t test_bank_helper(uint8_t startbank, uint8_t stopbank, uint8_t *uppermembanks,
                        uint8_t *expansion_type, uint8_t banktypefail, uint16_t szdetect) {
    
    char buf[50];

    // write test bit to new banks to be probed
    for(uint8_t i=startbank; i<stopbank; i++) {
        set_bank(i);
        memory[0xE000] = i;
        memory[0xF000] = i | 0x80;
    }

    // read back and test
    for(uint8_t i=startbank; i<stopbank; i++) {
        set_bank(i);
        if(memory[0xE000] == i && 
           memory[0xF000] == (i | 0x80)) {
            (*uppermembanks)++;
        } else { // early exit when not possible to avoid generating false positives
            break;
        }
    }

    // analyse results
    if((*uppermembanks) != stopbank) {
        (*expansion_type) = banktypefail;
        sprintf(buf, "%u KiB expansion card detected", szdetect);
        print_inline_color(buf, COL_GREEN);
        return 1;
    } else {
        sprintf(termbuffer, "  Banks %u - %u probed", startbank, stopbank-1);
        terminal_printtermbuffer();
        return 0;
    }
}

/**
 * Apply a fixed byte pattern to a set of RAM banks and verify whether these can be
 * read back.
 */
void test_fixed_pattern(uint8_t pattern, uint8_t check_id) {
    sprintf(termbuffer, "  Writing 0x%02X to banks", pattern);
    terminal_printtermbuffer();
    uint8_t miscounts = 0;

    for(uint16_t i=0; i<uppermembanks; i++) {
        set_bank(i);
        memset(&memory[BANKMEM_START], pattern, BANKMEM_STOP - BANKMEM_START + 1);
        write_termbuffer_value((uint8_t)i, COL_CYAN);

        if((i+1) % 8 == 0) {
            terminal_printtermbuffer();
        }
    }

    // also print result when total is not divisible by 8
    if(uppermembanks % 8 != 0) {
        terminal_printtermbuffer();
    }

    sprintf(termbuffer, "  Testing 0x%02X on banks", pattern);
    terminal_printtermbuffer();

    for(uint16_t i=0; i<uppermembanks; i++) {
        set_bank(i);
        uint16_t miscounts_bank = count_ram_bytes(&memory[BANKMEM_START], pattern, BANKMEM_STOP - BANKMEM_START + 1);
        if(miscounts_bank == 0) {
            write_termbuffer_value((uint8_t)i, COL_GREEN);
        } else {
            write_termbuffer_value((uint8_t)i, COL_RED);
            test_passed[check_id]++;
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

static inline char hex1(uint8_t v) {
    static const char hexd[] = "0123456789ABCDEF";  // size = 17 (includes '\0')
    return hexd[v & 0xF];
}

/**
 * Helper function to write 4-byte colored hex value to string buffer;
 * used to indicate RAM banks.
 */
void write_termbuffer_value(uint8_t i, uint8_t color) {
    // 4 visible chars + NUL; we intentionally copy 5 bytes into a 4-byte slot
    // so the NUL becomes the first byte of the next cell.
    char tmp[5];
    tmp[0] = (char)color;
    tmp[1] = hex1((uint8_t)(i >> 4));
    tmp[2] = hex1((uint8_t)i);
    tmp[3] = (char)COL_WHITE;
    tmp[4] = '\0';
    memcpy(&termbuffer[(i % 8) * 4], tmp, 5);
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
    
    set_bank(0);    // always set bank 0 upon initializati`on
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
