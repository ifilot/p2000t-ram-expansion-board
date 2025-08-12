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

#include "terminal.h"

uint8_t _terminal_curline = 0;
uint8_t _terminal_maxlines = 0;
uint8_t _terminal_startline = 0;
uint8_t _terminal_endline = 0;
uint16_t _prevcounter = 0;
char termbuffer[LINELENGTH + TERMBUFFER_PADDING];

void terminal_init(uint8_t start, uint8_t stop) {
    _terminal_startline = start;
    _terminal_curline = _terminal_startline;
    _terminal_maxlines = stop - start + 1;
    _terminal_endline = stop;
    memset(termbuffer, 0x00, LINELENGTH);
}

void terminal_printtermbuffer(void) {
    // scroll everything up when we are at the last line
    if(_terminal_curline > _terminal_endline) {
        terminal_scrollup();
        _terminal_curline--;
    }

    // copy buffer to screen
    memcpy(&vidmem[_terminal_curline * 0x50], termbuffer, LINELENGTH);
    memset(termbuffer, 0x00, LINELENGTH);

    // go to next line
    _terminal_curline++;
}

void terminal_redoline(void) {
    if(_terminal_curline > _terminal_endline) {
        terminal_scrollup();
        _terminal_curline--;
    }

    memcpy(&vidmem[_terminal_curline * 0x50], termbuffer, LINELENGTH);
    memset(termbuffer, 0x00, LINELENGTH);
}

void terminal_scrollup(void) {
    for(uint8_t i=_terminal_startline; i<_terminal_endline; i++) {
        memcpy(&vidmem[0x50*i], &vidmem[0x50*(i+1)], LINELENGTH);
    }
    memset(&vidmem[0x50*_terminal_endline], 0x00, LINELENGTH);
}

void terminal_backup_line(void) {
    _terminal_curline--;
}

void print_error(char* str) {
    sprintf(termbuffer, "%cERROR%c%s", COL_RED, COL_WHITE, str);
    terminal_printtermbuffer();
}

void print_info(char* str, uint8_t backup_line) {
    strncpy(termbuffer, str, LINELENGTH);
    terminal_printtermbuffer();
    if(backup_line == 1) {
        terminal_backup_line();
    }
}

void print_inline_color(char* str, uint8_t color) {
    sprintf(termbuffer, "  %c%s", color, str);
    terminal_printtermbuffer();
}