VSYNC = 0x00
VBLANK = 0x01
WSYNC = 0x02
COLUP0 = 0x06
COLUP1 = 0x07
COLUPF = 0x08
COLUBK = 0x09
CTRLPF = 0x0A
PF0 = 0x0D
PF1 = 0x0E
PF2 = 0x0F
GRP0 = 0x1B
GRP1 = 0x1C
RESM0 = 0x12
RESP0 = 0x10
RESP1 = 0x11
HMP0 = 0x20
HMP1 = 0x21
HMM0 = 0x22
HMOVE = 0x2A
NUSIZ0 = 0x04
NUSIZ1 = 0x05
ENAM0 = 0x1D

ROM_START=0xF000
RESET_VECTOR=0xFFFC

*=ROM_START
lda #0
sta 0xF0
FRAME:
lda #0x00
sta VBLANK
VSYNC_LOOP:
lda #0x02
sta VSYNC
lda #0x01
sta CTRLPF
lda #0x37
sta COLUPF
lda #0xF0
sta COLUBK
sta WSYNC
sta WSYNC
lda 0xF0
BNE SKIP_SPRITE_INIT

nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
sta RESM0

nop
nop
nop
nop
nop
sta RESP0

nop
nop
nop
nop
nop
sta RESP1
lda #1
sta 0xF0
SKIP_SPRITE_INIT:
sta WSYNC
lda #0x00
sta VSYNC
ldy #37

VBLANK_LOOP:
sta WSYNC
dey
bne VBLANK_LOOP

; Visible
; Draw border
sta WSYNC
sta WSYNC
lda #0x80
sta PF0
lda #0xFF
sta PF1
sta PF2
sta WSYNC
sta WSYNC
lda #0x00
sta PF1
sta PF2
; 4 / 192 scanlines

ldx #95
L1:
sta WSYNC
dex
bne L1
; 99 / 192 scanlines

lda #0x56
sta COLUP0
lda #0xC4
sta COLUP1
lda #0x00
sta 0xFE
lda #0xF8
sta 0xFF
lda #0x00
sta 0xFC
lda #0xF9
sta 0xFD
lda #0x33
sta NUSIZ0
lda #0x05
sta NUSIZ1
lda #0x10
sta HMP0
sta HMP1
sta HMM0
sta WSYNC
; 100 / 192 scanlines


ldy #0
lda (0xFE),y
sta GRP0
lda (0xFC),y
sta GRP1
lda #0x02
sta ENAM0
iny
sta WSYNC
; 101 / 192 scanlines

lda (0xFE),y
sta GRP0
lda (0xFC),y
sta GRP1
iny
sta WSYNC
; 102 / 192 scanlines

lda (0xFE),y
sta GRP0
lda (0xFC),y
sta GRP1
lda #0x00
sta ENAM0
iny
sta WSYNC
; 103 / 192 scanlines

lda (0xFE),y
sta GRP0
lda (0xFC),y
sta GRP1
iny
sta WSYNC
; 104 / 192 scanlines

lda #0x00
sta GRP0
sta GRP1

ldy #84
L2:
sta WSYNC
dey
bne L2
; 188 / 192 scanlines

lda #0xFF
sta PF1
sta PF2
sta WSYNC
sta WSYNC
; 190 / 192 scanlines

lda #0x00
sta PF0
sta PF1
sta PF2
sta WSYNC
sta WSYNC
; 192 / 192 scanlines

lda #0x42
sta VBLANK
sta HMOVE
ldy #30
OVERSCAN_LOOP:
sta WSYNC
dey
bne OVERSCAN_LOOP
jmp FRAME

*=0xF800

; Happy face sprite
!byte 0b00100100
!byte 0b00000000
!byte 0b01000010
!byte 0b00111100

*=0xF900
; Heart face sprite
!byte 0b01100110
!byte 0b01111110
!byte 0b00111100
!byte 0b00011000

*=RESET_VECTOR
!word ROM_START
