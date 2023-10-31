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
