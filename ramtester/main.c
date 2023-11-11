#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "constants.h"
#include "memory.h"
#include "util.h"
#include "stack.h"
#include "z80.h"

#define LOWMEM 0x6200
#define BANKMEM 0xE000
#define BANK_BYTES 0x2000
#define STACK 0x9F00

#define TITLELINE 2
#define STARTLINE 5
#define STATUSLINE 16
#define PROGLINE1 17
#define PROGLINE2 18
#define PROGLINE3 19

// forward declarations
void init(void);
uint8_t test_memory(uint16_t start, uint16_t nrbytes);
uint8_t test_lowmem(void);
uint8_t test_highmem(void);
uint8_t test_bankable_memory(void);
void set_bank(uint8_t bank);
void write_stack_pointer(void);

int main(void) {
    init();

    vidmem[0x50*TITLELINE] = TEXT_DOUBLE;
    vidmem[0x50*TITLELINE+1] = COL_CYAN;
    uint8_t nrchars = sprintf(&vidmem[0x50 * TITLELINE + 2], "RAM TESTER");
    vidmem[0x50*TITLELINE + 2 + nrchars] = COL_NONE;

    // test 1: check lowmem and verify that it is unaffected by bank value
    sprintf(&vidmem[0x50*STATUSLINE], "Testing lower memory...");
    set_bank(0);
    uint8_t pass_lowmem_0 = test_lowmem();
    set_bank(1);
    uint8_t pass_lowmem_1 = test_lowmem();

    sprintf(&vidmem[0x50*STARTLINE], "Low memory (0x%04X-0x%04X):%c%s",
        LOWMEM, STACK,
        (pass_lowmem_0 == 0 && pass_lowmem_1 == 0) ? COL_GREEN : COL_RED,
        (pass_lowmem_0 == 0 && pass_lowmem_1 == 0) ? "PASS" : "FAIL");
    clearline(STATUSLINE);

    // test 2: check highmem and verify that it is unaffected by bank value
    sprintf(&vidmem[0x50*STATUSLINE], "Testing upper memory...");
    set_bank(0);
    uint8_t pass_highmem_0 = test_highmem();
    set_bank(1);
    uint8_t pass_highmem_1 = test_highmem();

    sprintf(&vidmem[0x50*(STARTLINE+1)], "Upper memory (0xA000-0xDFFF):%c%s",
        (pass_highmem_0 == 0 && pass_highmem_1 == 0) ? COL_GREEN : COL_RED,
        (pass_highmem_0 == 0 && pass_highmem_1 == 0) ? "PASS" : "FAIL");
    clearline(STATUSLINE);

    // test 3: test bankable memory
    sprintf(&vidmem[0x50*STATUSLINE], "Testing bankable memory...");
    for(uint8_t i=0; i<6; i++) { // banks 0 - 5
        set_bank(i);
        uint8_t pass_bankable_memory = test_bankable_memory();
        sprintf(&vidmem[0x50*(STARTLINE+2+i)], "Bankable memory (%i: 0xE000-0xFFFF):%c%s",
            i,
            (pass_bankable_memory == 0) ? COL_GREEN : COL_RED,
            (pass_bankable_memory == 0) ? "PASS" : "FAIL");
    }
    clearline(STATUSLINE);

    // test 4: bank switching while data is stored
    sprintf(&vidmem[0x50*STATUSLINE], "Testing bank switching...");
    for(uint8_t i=0; i<6; i++) { // banks 0 - 5
        // first write the bank number to all the banked memory
        set_bank(i);
        for(uint16_t j=0xE000; j<0xFFFF; j++) {
            memory[j] = i;
        }
    }

    uint8_t pass_bank_switching = 0;
    for(uint8_t i=0; i<6; i++) { // banks 0 - 5
        // then check that the memory has been retained over the banks
        set_bank(i);
        for(uint16_t j=0xE000; j<0xFFFF; j++) {
            if(memory[j] != i) {
                pass_bank_switching |= (1 << i);
                break; // break the loop on a fail
            }
        }
    }
    sprintf(&vidmem[0x50*(STARTLINE+8)], "Bank switching test:%c%s",
            (pass_bank_switching == 0) ? COL_GREEN : COL_RED,
            (pass_bank_switching == 0) ? "PASS" : "FAIL");
    clearline(STATUSLINE);

    vidmem[0x50 * STATUSLINE] = TEXT_DOUBLE;
    vidmem[0x50 * STATUSLINE+1] = COL_GREEN;
    sprintf(&vidmem[0x50 * STATUSLINE+2], "TESTS COMPLETED");

    return 0;
}

/**
 * Test the lower memory
 */
uint8_t test_lowmem(void) {
    // low memory starts at 0x6000 but we cannot check below 0x6200 because this
    // is where variables are stored and not above 0x9F00 because this is
    // where the stack resides
    return test_memory(LOWMEM, STACK - LOWMEM);
}

/**
 * Test the upper memory
 */
uint8_t test_highmem(void) {
    // high memory starts at 0xA000 and the last page is reserved
    // for the stack
    return test_memory(0xA000, 0x4000);
}

/**
 * Test the bankable memory
 */
uint8_t test_bankable_memory(void) {
    return test_memory(BANKMEM, BANK_BYTES);
}

/**
 * @brief      Test the memory
 *
 * @param[in]  start    The start
 * @param[in]  nrbytes  The nrbytes
 *
 * @return     checksum parameter; 0 means everything passed
 */
uint8_t test_memory(uint16_t start, uint16_t nrbytes) {
    uint8_t pass = 0;

    for(uint8_t i=0; i<8; i++) {
        sprintf(&vidmem[0x50*PROGLINE3+11], "Testing bit %i", i);
        const uint8_t checkbyte = (1 << i);

        // first write the values; show some increment information
        const uint16_t increment = nrbytes / 32;
        for(uint8_t k=0; k<32; k++) {
            const uint16_t offset = start+k*increment;
            for(uint16_t j=0; j<increment; j++) {
                memory[offset+j] = checkbyte;
            }

            // progress bar for writing
            vidmem[0x50 * PROGLINE1 + k] = GRAPH_BLOCK;
            sprintf(&vidmem[0x50*PROGLINE1+33], "%04X", offset+increment-1);
        }

        for(uint8_t k=0; k<32; k++) {
            const uint16_t offset = start+k*increment;
            for(uint16_t j=0; j<increment; j++) {
                if(memory[offset+j] != checkbyte) {
                    pass |= checkbyte;
                    break;  // early exit in for loop
                }
            }

            // progress bar for checking
            vidmem[0x50 * PROGLINE2 + k] = GRAPH_BLOCK;
            sprintf(&vidmem[0x50*PROGLINE2+33], "%04X", offset+increment-1);
        }
        // write which bit is tested
        vidmem[0x50 * PROGLINE3 + i] = GRAPH_BLOCK;

        // clear the progress bar looping over the address space
        clearline(PROGLINE1);
        clearline(PROGLINE2);
    }
    // clear progress bar looping over bits
    clearline(PROGLINE3);

    return pass;
}

/**
 * @brief      Sets the bank.
 *
 * @param[in]  bank  The bank id
 */
void set_bank(uint8_t bank) {
    z80_outp(0x94, bank);

    // placeholder for bit pattern (terminating char = 0)
    uint8_t char_bits[4] = {' ',' ',' ',0};

    // build bit pattern
    for(uint8_t i=0; i<3; i++) {
        if((bank & (1 << i)) != 0) {
            char_bits[2-i] = GRAPH_BLOCK;
        } else {
            char_bits[2-i] = ' '; // write space
        }
    }

    vidmem[16] = 137; // non-blinking
    sprintf(&vidmem[16], "Bank register: |%s| (%i)", char_bits, bank);
    write_stack_pointer();
}

void write_stack_pointer(void) {
    uint16_t stackptr = get_stack_pointer();
    sprintf(&vidmem[0x50 + 16], "Stack pointer: %04X", stackptr);
}

void init(void) {
    clearline(0);
    set_bank(0);    // always set bank 0 upon initialization
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
