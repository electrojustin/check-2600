FRAME:
lda #$00
sta $01
lda #$02
VSYNC:
sta $00
sta $02
sta $02
sta $02
lda #$00
sta $00
ldy #$25
VBLANK:
sta $2
dey
bne VBLANK
ldx #$00
ldy #$C0
PICTURE:
inx
stx $09
sta $02
dey
bne PICTURE
lda #$42
sta $01
ldy #$1E
OVERSCAN:
sta $02
dey
bne OVERSCAN
jmp $1000
