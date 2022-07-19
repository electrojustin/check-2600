VSYNC = 0x00
VBLANK = 0x01
WSYNC = 0x02
COLUPF = 0x08
COLUBK = 0x09
CTRLPF = 0x0A
PF0 = 0x0D
PF1 = 0x0E
PF2 = 0x0F

*=0x1000
FRAME:
lda #0x00
sta VBLANK
VSYNC_LOOP:
lda #0x02
sta VSYNC
lda #0xF0
sta PF0
lda #0x66
sta PF1
lda #0x99
sta PF2
lda #0x00
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
ldx #0
ldy #100
PIC:
inx
stx COLUBK
sta WSYNC
dey
bne PIC
lda #0x01
sta CTRLPF
ldy #92
PIC2:
inx
stx COLUBK
sta WSYNC
dey
bne PIC2
lda #0x42
sta VBLANK
ldy #30
OVERSCAN_LOOP:
sta WSYNC
dey
bne OVERSCAN_LOOP
jmp 0x1000
