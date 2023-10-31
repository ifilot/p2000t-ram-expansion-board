#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "util.h"
#include "z80.h"

// forward declarations
void init(void);

int main(void) {
    init();

    z80_outp(0x94, 0);  // set bank 0

    // write value to bank
    for(uint16_t j=0; j<0x4000; j++) {
        highmem[j] = (uint8_t)(j & 0xFF);
    }

    z80_outp(0x94, 1);  // set to bank 1

    // write value to bank
    uint8_t testflag_highmem = 0x00;
    for(uint16_t j=0; j<0x4000; j++) {
        if(highmem[j] != (uint8_t)(j & 0xFF)) {
            testflag_highmem = 0xFF;
        }
    }

    sprintf(&vidmem[0x00], "High memory: %s", testflag_highmem == 0x00 ? "PASS" : "FAIL");

    // loop over the banks
    for(uint8_t i=0; i<2; i++) {

        z80_outp(0x94, i);  // set bank

        // write value to bank
        for(uint16_t j=0; j<0x2000; j++) {
            bankmem[j] = ((uint8_t)(j & 0xFF) - i);
        }

        sprintf(&vidmem[0x50*(i+2)], "Writing values to bank: %02i", i);
    }

    // loop over the banks
    for(uint8_t i=0; i<2; i++) {

        z80_outp(0x94, i);  // set bank

        uint16_t testflag = 0;

        // assert value for bank
        for(uint16_t j=0; j<0x2000; j++) {
            if(bankmem[j] != ((uint8_t)(j & 0xFF) - i)) {
                testflag++;
            }
        }

        sprintf(&vidmem[0x50*(i+9)], "Bank test: %02i - %s (%04X)", i, testflag == 0x00 ? "PASS" : "FAIL", testflag);
    }

    return 0;
}

void init(void) {
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
