VSYNC = 0x00
VBLANK = 0x01
WSYNC = 0x02
COLUP0 = 0x06
COLUPF = 0x08
COLUBK = 0x09
CTRLPF = 0x0A
PF0 = 0x0D
PF1 = 0x0E
PF2 = 0x0F
GRP0 = 0x1B
RESP0 = 0x10

*=0x1000
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
sta WSYNC
sta WSYNC
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
ldy #0
lda #0x00
sta 0xFE
lda #0x18
sta 0xFF
lda (0xFE),y
sta WSYNC
; 100 / 192 scanlines

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
nop
nop
nop
nop
nop
nop

sta GRP0
sta RESP0
iny
lda (0xFE),y
sta WSYNC
; 101 / 192 scanlines

sta GRP0
iny
lda (0xFE),y
sta WSYNC
; 102 / 192 scanlines

sta GRP0
iny
lda (0xFE),y
sta WSYNC
; 103 / 192 scanlines

sta GRP0
lda #0x00
sta WSYNC
sta GRP0
; 104 / 192 scanlines

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
ldy #30
OVERSCAN_LOOP:
sta WSYNC
dey
bne OVERSCAN_LOOP
jmp FRAME

*=0x1800
; Happy face sprite
!byte 0b00100100
!byte 0b00000000
!byte 0b01000010
!byte 0b00111100
