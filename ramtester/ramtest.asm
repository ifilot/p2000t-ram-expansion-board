SECTION code_user

PUBLIC _count_ram_bytes

;-------------------------------------------------------------------------------
; uint16_t check_ram(char *memory, uint8_t val, uint16_t nrbytes) __z88dk_callee;
;-------------------------------------------------------------------------------
_count_ram_bytes:
    di
    pop hl                      ; return address
    pop de                      ; ramptr
    dec sp                      ; decrement sp for 1-byte argument
    pop af                      ; byte to write (stored in a)
    pop bc                      ; number of bytes
    push hl                     ; push return address back onto stack
    ld hl,0                     ; set miscounter to 0
    ld iyl,a                    ; store checkbyte in iyl
nextbyte:
    ld a,(de)                   ; load value into a
    cp iyl                      ; compare with checkbyte
    jr z,skip                   ; if zero?
    inc hl                      ; increment hl counter
skip:
    dec bc                      ; decrement counter
    ld a,b
    or c
    jr nz,nextbyte              ; if counter is zero, fall through
    ei
    ret                         ; result is stored in hl
