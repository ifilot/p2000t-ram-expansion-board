#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "constants.h"
#include "memory.h"
#include "util.h"
#include "z80.h"

#define LOWMEM 0x6200
#define BANKMEM 0xE000
#define BANK_BYTES 0x2000
#define STACK 0x9F00

// forward declarations
void init(void);
uint8_t test_memory(uint16_t start, uint16_t nrbytes);
uint8_t test_lowmem(void);
uint8_t test_highmem(void);
uint8_t test_bankable_memory(void);
void set_bank(uint8_t bank);

int main(void) {
    init();

    vidmem[0x50] = TEXT_DOUBLE;
    vidmem[0x50+1] = COL_CYAN;
    uint8_t nrchars = sprintf(&vidmem[0x50+2], "RAM TESTER");
    vidmem[0x50 + 2 + nrchars] = COL_NONE;

    // test 1: check lowmem and verify that it is unaffected by bank value
    set_bank(0);
    uint8_t pass_lowmem_0 = test_lowmem();
    set_bank(1);
    uint8_t pass_lowmem_1 = test_lowmem();

    sprintf(&vidmem[0x50*3], "Low memory (0x%04X-0x%04X): %s",
        LOWMEM, STACK,
        (pass_lowmem_0 == 0 && pass_lowmem_1 == 0) ? "PASS" : "FAIL");

    // test 2: check highmem and verify that it is unaffected by bank value
    set_bank(0);
    uint8_t pass_highmem_0 = test_highmem();
    set_bank(1);
    uint8_t pass_highmem_1 = test_highmem();

    sprintf(&vidmem[0x50*4], "Upper memory (0xA000-0xDFFF): %s",
        (pass_highmem_0 == 0 && pass_highmem_1 == 0) ? "PASS" : "FAIL");

    // test 3: test bankable memory
    for(uint8_t i=0; i<6; i++) { // banks 0 - 5
        set_bank(i);
        uint8_t pass_bankable_memory = test_bankable_memory();
        sprintf(&vidmem[0x50*(5+i)], "Bankable memory (%i: 0xE000-0xFFFF): %s",
            i, (pass_bankable_memory == 0) ? "PASS" : "FAIL");
    }

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
        const uint8_t checkbyte = (1 << i);
        sprintf(&vidmem[38], "%02X", checkbyte);

        for(uint16_t j=0; j<nrbytes; j++) {
            memory[start+j] = checkbyte;
        }

        for(uint16_t j=0; j<nrbytes; j++) {
            if(memory[start+j] != checkbyte) {
                pass |= checkbyte;
                break;  // early exit in for loop
            }
        }
    }

    return pass;
}

void set_bank(uint8_t bank) {
    z80_outp(0x94, bank);  // set bank 0
}

void init(void) {
    set_bank(0);    // always set bank 0 upon initialization
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
