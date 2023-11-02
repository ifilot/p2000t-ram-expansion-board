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
            testflag_highmem++;
        }
    }

    sprintf(&vidmem[0x00], "High memory: %s (%04X)", testflag_highmem == 0 ? "PASS" : "FAIL", testflag_highmem);

    // loop over the banks
    for(uint8_t i=0; i<6; i++) {

        z80_outp(0x94, i);  // set bank

        // write value to bank
        for(uint16_t j=0; j<0x2000; j++) {
            bankmem[j] = i;
        }

        sprintf(&vidmem[0x50*(i+2)], "Writing values to bank: %02i", i);
    }

    // loop over the banks
    for(uint8_t i=0; i<6; i++) {

        uint16_t firsterror = 0;
        uint16_t lasterror = 0;
        z80_outp(0x94, i);  // set bank

        uint16_t testflag = 0;

        // assert value for bank
        for(uint16_t j=0; j<0x2000; j++) {
            if(bankmem[j] != i) {
                testflag++;
                if(testflag == 1) {
                    firsterror = j;
                }
                lasterror = j;
            }
        }

        sprintf(&vidmem[0x50*(i+9)], "Bank test: %02i - %s (%04X / %04X / %04X)",
            i, testflag == 0 ? "PASS" : "FAIL", testflag, firsterror, lasterror);
    }

    return 0;
}

void init(void) {
    sprintf(&vidmem[0x50*22], "Version: %s", __VERSION__);
    sprintf(&vidmem[0x50*23], "Compiled at: %s / %s", __DATE__, __TIME__);
}
