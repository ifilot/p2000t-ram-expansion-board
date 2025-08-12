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

#include "stack.h"

/**
 * @brief Writes the current stack position to the screen
 */
void write_stack_pointer(void) {
    uint16_t stackptr = get_stack_pointer();
    vidmem[0x50] = COL_MAGENTA;
    sprintf(&vidmem[0x50+1], "Stack pointer: %04X", stackptr);
}