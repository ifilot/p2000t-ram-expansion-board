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

#include "bankcounting.h"

// Spread sentinels across the 8KB banked window (E000–FFFF).
// Each sentinel uses two bytes: addr holds TAG, addr+1 holds ~TAG.
static const uint16_t sentinels[NR_SENTINELS] = {
    0xE000, 0xF000
};

// Simple per-(selector,idx) tag: varied across locations, XOR with selector.
uint8_t tag_byte(uint8_t selector, uint8_t idx) {
    return (uint8_t)((0x5Au + 0x13u * idx) ^ selector);
}

void write_signature(uint8_t selector) {
    for (uint8_t i = 0; i < NR_SENTINELS; ++i) {
        volatile uint8_t *p = (volatile uint8_t *)(sentinels[i]);
        uint8_t t = tag_byte(selector, i);
        p[0] = t;
        p[1] = (uint8_t)~t;
    }
}

// Returns true if signature for `selector` matches at all sentinels.
uint8_t verify_signature(uint8_t selector) {
    for (uint8_t i = 0; i < NR_SENTINELS; ++i) {
        volatile uint8_t *p = (volatile uint8_t *)(sentinels[i]);
        uint8_t t  = tag_byte(selector, i);
        uint8_t v0 = p[0];
        uint8_t v1 = p[1];
        if (v0 != t) return FALSE;
        if (v1 != (uint8_t)~t) return FALSE;
    }
    return TRUE;
}

uint16_t count_banks(void) {
    static uint8_t reps[MAX_SELECTORS];   // avoid stack use
    uint16_t repcnt = 0;

    set_bank(0); // start from a known bank

    // loop over potential banks
    for (uint16_t s = 0; s < MAX_SELECTORS; ++s) {
        uint8_t alias  = FALSE;
        uint8_t shadow = FALSE;

        // start by pre-scrubbing
        for (uint8_t i = 0; i < NR_SENTINELS; ++i) {
            uint16_t eaddr = sentinels[i];                // E000 or F000
            uint16_t off   = (uint16_t)(eaddr - 0xE000);  // 0 or 0x1000

            volatile uint8_t *e = (volatile uint8_t *)eaddr;
            volatile uint8_t *a = (volatile uint8_t *)(0xA000 + off);
            volatile uint8_t *c = (volatile uint8_t *)(0xC000 + off);

            uint8_t t  = tag_byte((uint8_t)s, i);
            *a = (uint8_t)~t;
            *c = (uint8_t)~t;
        }

        // Select and tag this candidate
        z80_outp(0x94, (uint8_t)s);
        write_signature((uint8_t)s);

        #ifdef DEBUG
        sprintf(termbuffer, "Testing selector %3u...", (unsigned)s);
        terminal_printtermbuffer();
        #endif

        // Shadow check: E000 <-> A000/C000 and F000 <-> B000/D000
        for (uint8_t i = 0; i < NR_SENTINELS; ++i) {
            uint16_t eaddr = sentinels[i];                // E000 or F000
            uint16_t off   = (uint16_t)(eaddr - 0xE000);  // 0 or 0x1000

            volatile uint8_t *e = (volatile uint8_t *)eaddr;
            volatile uint8_t *a = (volatile uint8_t *)(0xA000 + off);
            volatile uint8_t *c = (volatile uint8_t *)(0xC000 + off);

            uint8_t t  = tag_byte((uint8_t)s, i);
            uint8_t v0 = e[0];
            uint8_t v1 = e[1];

            // First, ensure our tag stuck in E-window
            if (v0 != t || v1 != (uint8_t)~t) { shadow = TRUE; break; }

            // Read originals from lower RAM so we can restore if mirrored
            uint8_t oa0 = a[0], oa1 = a[1];
            uint8_t oc0 = c[0], oc1 = c[1];

            // If either lower block shows (tag, ~tag), this selection shadows base RAM
            if ((oa0 == v0 && oa1 == v1) || (oc0 == v0 && oc1 == v1)) {
                // restore whichever mirrored (mirroring made them equal to v0/v1)
                if (oa0 == v0 && oa1 == v1) { a[0] = oa0; a[1] = oa1; }
                if (oc0 == v0 && oc1 == v1) { c[0] = oc0; c[1] = oc1; }
                shadow = TRUE;
                break;
            }
        }

        if (shadow) {
            #ifdef DEBUG
            sprintf(termbuffer, " -> SHADOW of lower RAM; early exit");
            terminal_printtermbuffer();
            #endif
            break; // banks are sequential; first shadow means no more real banks
        }

        // Alias detection against previously found banks
        for (uint16_t j = 0; j < repcnt; ++j) {
            uint8_t r = reps[j];

            z80_outp(0x94, r);
            if (!verify_signature(r)) {
                alias = TRUE;

                #ifdef DEBUG
                sprintf(termbuffer, " -> ALIAS of selector %3u; early exit", (unsigned)r);
                terminal_printtermbuffer();
                #endif

                // restore r's signature so any later code sees it correct
                z80_outp(0x94, r);
                write_signature(r);
                break;
            }
        }

        if (alias) {
            break; // early exit on first alias (sequential guarantee)
        }

        // New, distinct bank
        reps[repcnt++] = (uint8_t)s;

        #ifdef DEBUG
        sprintf(termbuffer, " -> NEW BANK, total so far: %u", (unsigned)repcnt);
        terminal_printtermbuffer();
        #endif
    }

    set_bank(0); // leave system in a known state

    #ifdef DEBUG
    sprintf(termbuffer, "NR BANKS: %u", (unsigned)repcnt);
    terminal_printtermbuffer();
    #endif

    return repcnt;
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
    sprintf(&vidmem[1], "Bank register: |%s| (%u)", char_bits, bank);
    write_stack_pointer();
}