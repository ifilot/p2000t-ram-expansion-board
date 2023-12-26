;-------------------------------------------------------------------------------
;
;   Author: Ivo Filot <ivo@ivofilot.nl>
;
;   P2000T-RAMTESTER is free software:
;   you can redistribute it and/or modify it under the terms of the
;   GNU General Public License as published by the Free Software
;   Foundation, either version 3 of the License, or (at your option)
;   any later version.
;
;   P2000T-RAMTESTER software is distributed in the hope that it will
;   be useful, but WITHOUT ANY WARRANTY; without even the implied
;   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;   See the GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see http://www.gnu.org/licenses/.
;
;-------------------------------------------------------------------------------

SECTION code_user

PUBLIC _clearscreen

_clearscreen:
    ld a,0
    ld ($5000),a
    ld de,$5001
    ld bc,$1000
    dec bc
    ld hl,$5000
    ldir
    ret
