; 模块：cmd-directory
; BBGCDOS 的说明与 CD、MD、RD、DIR 命令

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_1B84"
; 段 0，$9B84-$A5D1
; 内部命令 CHDIR、CD [drive:][path]：显示或改变当前目录，不带参数时显示当前目录。
cmd_chdir:
  jsr sub_8DB8
  jsr sub_8E22
  stx var_0401
  jsr sub_8B57
  beq loc_9B9F
  jsr sub_8BBA
  jsr sub_8DB8
  beq loc_9BA9
  ldx #$09
  jmp sub_8CA5
loc_9B9F:
  jsr sub_9BB3
  lda #$0F
  bcs loc_9BB2
  jmp sub_8A67
loc_9BA9:
  lda #$10
  jsr sub_8D69
  bcc loc_9BB2
  lda #$3D
loc_9BB2:
  rts

sub_9BB3:
  lda #$13
  jsr sub_8D73
  bcs loc_9BB2
  jsr sub_9B79

sub_9BBD:
  lda var_0402
  ldx var_0404
  ldy var_0403
loc_9BC6:
  sta var_00FD
  stx var_00FF
  sty var_00FE
  ldy #$00
  jsr sub_9B72
  tax
  bne loc_9BDB
  ldx #$5C
  jsr sub_8A5A
  clc
  rts
loc_9BDB:
  jsr sub_9B72
  tax
  beq loc_9BEB
  tya
  pha
  jsr sub_8A5A
  pla
  tay
  iny
  bne loc_9BDB
loc_9BEB:
  clc
  rts
  .byte $43, $72, $65, $61, $74, $65, $20, $61, $20, $64, $69, $72, $65, $63, $74, $6F
  .byte $72, $79, $0D, $0A, $0A, $4D, $4B, $44, $49, $52, $20, $6F, $72, $0D, $0A, $4D
  .byte $44, $20, $5B, $64, $72, $69, $76, $65, $3A, $5D, $5B, $70, $61, $74, $68, $5D
  .byte $44, $69, $72, $65, $63, $74, $6F, $72, $79, $24, $BD, $A8, $C1, $A2, $D2, $BB
  .byte $B8, $F6, $C4, $BF, $C2, $BC, $0D, $0A, $0A, $4D, $4B, $44, $49, $52, $7C, $4D
  .byte $44, $20, $5B, $C5, $CC, $3A, $5D, $5B, $C2, $B7, $BE, $B6, $5D, $C4, $BF, $C2
  .byte $BC, $C3, $FB, $24

; 内部命令 MKDIR、MD [drive:][path]Directory：建立目录。
cmd_mkdir:
  jsr sub_8DB8
  beq loc_9C79
  jsr sub_8E22
  stx var_0401
  jsr sub_8B57
  beq loc_9C75
  jsr sub_8BBA
  jsr sub_8DB8
  beq loc_9C6E
  ldx #$09
  jmp sub_8CA5
loc_9C6E:
  lda #$11
  jsr sub_8D69
  bcc loc_9C78
loc_9C75:
  lda #$3E
  sec
loc_9C78:
  rts
loc_9C79:
  lda #$0D
  sec
  rts
  .byte $52, $65, $6D, $6F, $76, $65, $73, $20, $28, $64, $65, $6C, $65, $74, $65, $73
  .byte $29, $20, $61, $20, $64, $69, $72, $65, $63, $74, $6F, $72, $79, $0D, $0A, $0A
  .byte $52, $4D, $44, $49, $52, $20, $6F, $72, $0D, $0A, $52, $44, $20, $5B, $64, $72
  .byte $69, $76, $65, $3A, $5D, $5B, $70, $61, $74, $68, $5D, $44, $69, $72, $65, $63
  .byte $74, $6F, $72, $79, $24, $C9, $BE, $B3, $FD, $D2, $BB, $B8, $F6, $C4, $BF, $C2
  .byte $BC, $0D, $0A, $0A, $52, $4D, $44, $49, $52, $7C, $52, $44, $20, $5B, $C5, $CC
  .byte $3A, $5D, $5B, $C2, $B7, $BE, $B6, $5D, $C4, $BF, $C2, $BC, $C3, $FB, $24

; 内部命令 RMDIR、RD [drive:][path]Directory：删除目录。
cmd_rmdir:
  jsr sub_8DB8
  beq loc_9C79
  jsr sub_8E22
  stx var_0401
  jsr sub_8B57
  beq loc_9D14
  jsr sub_8BBA
  jsr sub_8DB8
  beq loc_9D09
  ldx #$09
  jmp sub_8CA5
loc_9D09:
  lda #$12
  jsr sub_8D69
  bcc loc_9D17
  cmp #$10
  beq loc_9D18
loc_9D14:
  lda #$3F
  sec
loc_9D17:
  rts
loc_9D18:
  lda #$13
  jsr sub_8D73
  bcs loc_9D17
  lda #$41
  jsr sub_8A3B
  jsr sub_9BBD
  jmp sub_8A67
  .byte $44, $69, $73, $70, $6C, $61, $79, $73, $20, $20, $61, $20, $6C, $69, $73, $74
  .byte $20, $6F, $66, $20, $20, $66, $69, $6C, $65, $73, $20, $61, $6E, $64, $0D, $0A
  .byte $73, $75, $62, $64, $69, $72, $65, $63, $74, $6F, $72, $69, $65, $73, $20, $69
  .byte $6E, $20, $61, $20, $64, $69, $72, $65, $63, $74, $6F, $72, $79, $2E, $0D, $0A
  .byte $0A, $44, $49, $52, $20, $5B, $64, $72, $69, $76, $65, $3A, $5D, $5B, $70, $61
  .byte $74, $68, $5D, $5B, $66, $69, $6C, $65, $6E, $61, $6D, $65, $5D, $0D, $0A, $20
  .byte $20, $5B, $2F, $50, $5D, $5B, $2F, $57, $5D, $5B, $2F, $42, $5D, $5B, $2F, $41
  .byte $5B, $3A, $5D, $5B, $61, $74, $74, $72, $69, $62, $73, $5D, $5D, $0D, $0A, $20
  .byte $20, $5B, $2F, $4C, $5D, $5B, $2F, $53, $5D, $20, $20, $20, $20, $5B, $2F, $4F
  .byte $5B, $3A, $5D, $5B, $73, $6F, $72, $74, $6F, $72, $64, $5D, $5D, $0D, $0A, $0A
  .byte $2F, $50, $20, $50, $61, $75, $73, $65, $73, $20, $61, $66, $74, $65, $72, $20
  .byte $65, $61, $63, $68, $20, $73, $63, $72, $65, $65, $6E, $0D, $0A, $2F, $57, $20
  .byte $55, $73, $65, $73, $20, $77, $69, $64, $65, $20, $6C, $69, $73, $74, $20, $66
  .byte $6F, $72, $6D, $61, $74, $0D, $0A, $2F, $42, $20, $55, $73, $65, $20, $73, $68
  .byte $6F, $72, $74, $20, $66, $6F, $72, $6D, $61, $74, $0D, $0A, $2F, $41, $20, $44
  .byte $69, $73, $70, $6C, $61, $79, $73, $20, $61, $74, $74, $72, $69, $62, $75, $74
  .byte $65, $73, $20, $66, $69, $6C, $65, $73, $0D, $0A, $61, $74, $74, $72, $69, $62
  .byte $73, $20, $53, $20, $53, $79, $73, $74, $65, $6D, $2C, $52, $20, $52, $65, $61
  .byte $64, $2D, $6F, $6E, $6C, $79, $0D, $0A, $20, $20, $20, $20, $20, $20, $20, $20
  .byte $48, $20, $48, $69, $64, $64, $65, $6E, $2C, $41, $20, $41, $72, $63, $68, $69
  .byte $76, $65, $0D, $0A, $20, $20, $20, $20, $20, $20, $20, $20, $2D, $20, $3C, $6E
  .byte $6F, $74, $3E, $2C, $20, $44, $20, $44, $69, $72, $65, $63, $74, $6F, $72, $79
  .byte $0D, $0A, $2F, $4C, $20, $55, $73, $65, $20, $6C, $6F, $77, $65, $72, $63, $61
  .byte $73, $65, $0D, $0A, $2F, $53, $20, $69, $6E, $63, $6C, $75, $64, $65, $20, $61
  .byte $6C, $6C, $20, $73, $75, $62, $64, $69, $72, $65, $63, $74, $6F, $72, $69, $65
  .byte $73, $0D, $0A, $2F, $4F, $20, $4C, $69, $73, $74, $20, $69, $6E, $20, $73, $6F
  .byte $72, $74, $65, $64, $20, $6F, $72, $64, $65, $72, $0D, $0A, $73, $6F, $72, $74
  .byte $6F, $72, $64, $20, $4E, $20, $4E, $61, $6D, $65, $2C, $20, $20, $45, $20, $45
  .byte $78, $74, $65, $6E, $73, $69, $6F, $6E, $0D, $0A, $20, $20, $20, $20, $20, $20
  .byte $20, $20, $53, $20, $53, $69, $7A, $65, $2C, $20, $20, $44, $20, $44, $61, $74
  .byte $65, $26, $54, $69, $6D, $65, $0D, $0A, $20, $20, $20, $20, $20, $20, $20, $20
  .byte $2D, $20, $3C, $6E, $6F, $74, $3E, $2C, $20, $47, $20, $44, $69, $72, $65, $63
  .byte $74, $6F, $72, $79, $24, $CF, $D4, $CA, $BE, $C4, $BF, $C2, $BC, $CE, $C4, $BC
  .byte $FE, $BC, $B0, $D7, $D3, $C4, $BF, $C2, $BC, $0D, $0A, $44, $49, $52, $20, $5B
  .byte $C5, $CC, $3A, $5D, $5B, $C2, $B7, $BE, $B6, $5D, $5B, $CE, $C4, $BC, $FE, $C3
  .byte $FB, $5D, $0D, $0A, $20, $20, $5B, $2F, $50, $5D, $20, $5B, $2F, $57, $5D, $20
  .byte $5B, $2F, $42, $5D, $20, $5B, $2F, $41, $5B, $3A, $5D, $5B, $CA, $F4, $D0, $D4
  .byte $5D, $5D, $0D, $0A, $20, $20, $5B, $2F, $4C, $5D, $20, $5B, $2F, $53, $5D, $20
  .byte $20, $20, $20, $20, $20, $5B, $2F, $4F, $5B, $3A, $5D, $5B, $C5, $C5, $D0, $F2
  .byte $5D, $5D, $0D, $0A, $2F, $50, $20, $C2, $FA, $D2, $BB, $C6, $C1, $CD, $A3, $CF
  .byte $C2, $20, $20, $2F, $57, $20, $BF, $ED, $C1, $D0, $B7, $BD, $CA, $BD, $CF, $D4
  .byte $CA, $BE, $0D, $0A, $2F, $42, $20, $BC, $F2, $B6, $CC, $B8, $F1, $CA, $BD, $20
  .byte $20, $20, $20, $2F, $4C, $20, $D0, $A1, $D0, $B4, $D7, $D6, $B7, $FB, $CF, $D4
  .byte $CA, $BE, $0D, $0A, $2F, $53, $20, $CF, $D4, $CA, $BE, $B0, $FC, $C0, $A8, $CB
  .byte $F9, $D3, $D0, $D7, $D3, $C4, $BF, $C2, $BC, $B5, $C4, $CE, $C4, $BC, $FE, $0D
  .byte $0A, $2F, $41, $20, $CF, $D4, $CA, $BE, $D6, $B8, $B6, $A8, $CA, $F4, $D0, $D4
  .byte $B5, $C4, $CE, $C4, $BC, $FE, $C4, $BF, $C2, $BC, $0D, $0A, $CA, $F4, $D0, $D4
  .byte $20, $53, $20, $CF, $B5, $CD, $B3, $CE, $C4, $BC, $FE, $20, $52, $20, $D6, $BB
  .byte $B6, $C1, $CE, $C4, $BC, $FE, $0D, $0A, $20, $20, $20, $20, $20, $48, $20, $D2
  .byte $FE, $B2, $D8, $CE, $C4, $BC, $FE, $20, $41, $20, $B5, $B5, $B0, $B8, $CE, $C4
  .byte $BC, $FE, $0D, $0A, $20, $20, $20, $20, $20, $44, $20, $C4, $BF, $C2, $BC, $20
  .byte $20, $20, $20, $20, $2D, $20, $B7, $B4, $CA, $F4, $D0, $D4, $C7, $B0, $D7, $BA
  .byte $0D, $0A, $2F, $4F, $20, $B0, $B4, $D6, $B8, $B6, $A8, $C5, $C5, $D0, $F2, $B7
  .byte $BD, $CA, $BD, $CF, $D4, $CA, $BE, $C4, $BF, $C2, $BC, $0D, $0A, $C5, $C5, $D0
  .byte $F2, $20, $4E, $20, $CE, $C4, $BC, $FE, $C3, $FB, $20, $20, $20, $45, $20, $CE
  .byte $C4, $BC, $FE, $C0, $A9, $D5, $B9, $C3, $FB, $0D, $0A, $20, $20, $20, $20, $20
  .byte $53, $20, $CE, $C4, $BC, $FE, $B4, $F3, $D0, $A1, $20, $44, $20, $C8, $D5, $C6
  .byte $DA, $BA, $CD, $CA, $B1, $BC, $E4, $0D, $0A, $20, $20, $20, $20, $20, $47, $20
  .byte $C4, $BF, $C2, $BC, $D3, $C5, $CF, $C8, $20, $2D, $20, $C4, $E6, $C5, $C5, $D0
  .byte $F2, $C7, $B0, $D7, $BA, $24, $50, $FC, $A0, $57, $08, $A1, $42, $0B, $A1, $4C
  .byte $11, $A1, $53, $0E, $A1, $41, $15, $A1, $4F, $78, $A1, $FF, $41, $24, $A1, $4F
  .byte $78, $A1, $00, $A9, $1E, $2C, $DF, $63, $10, $02, $A9, $0E, $85, $F3, $60, $E6
  .byte $F7, $60, $E6, $F6, $60, $E6, $F2, $60, $CE, $DD, $63, $60, $A9, $00, $85, $F9
  .byte $A9, $08, $85, $F8, $60, $41, $44, $78, $53, $48, $52, $84, $FD, $A9, $00, $85
  .byte $F9, $85, $F8, $B9, $92, $62, $C9, $3A, $D0, $06, $C8, $20, $57, $8B, $F0, $36
  .byte $A9, $00, $85, $FC, $20, $57, $8B, $F0, $27, $C8, $C9, $2D, $F0, $F4, $29, $DF
  .byte $A2, $06, $CA, $30, $21, $DD, $1F, $A1, $D0, $F8, $A9, $40, $4A, $CA, $10, $FC
  .byte $A6, $FC, $D0, $06, $05, $F9, $85, $F9, $D0, $D6, $05, $F8, $85, $F8, $D0, $D0
  .byte $A5, $FC, $D0, $02, $18, $60, $A4, $FD, $38, $60, $4E, $45, $53, $44, $47, $84
  .byte $FD, $20, $57, $8B, $D0, $0E, $A9, $00, $8D, $92, $62, $A9, $01, $8D, $93, $62
  .byte $A2, $02, $D0, $37, $C9, $3A, $D0, $04, $C8, $B9, $92, $62, $A2, $00, $86, $FE
  .byte $A2, $00, $C9, $2D, $D0, $08, $C8, $20, $57, $8B, $F0, $23, $A2, $80, $86, $FF
  .byte $29, $DF, $A2, $05, $CA, $30, $18, $DD, $74, $A1, $D0, $F8, $8A, $05, $FF, $A6
  .byte $FE, $9D, $92, $62, $E8, $C8, $20, $57, $8B, $D0, $D3, $86, $F5, $18, $60, $A4
  .byte $FD, $38, $60
data_A1CD:
  .byte $09, $11, $08, $04
data_A1D1:
  .byte $08, $03, $04, $04

sub_A1D5:
  ldx #$00
loc_A1D7:
  txa
  pha
  lda var_6292,x
  and #$7F
  cmp #$04
  beq loc_A20B
  tax
  ldy data_A1CD,x
  lda data_A1D1,x
  cpx #$02
  bcs loc_A1FC
  tax
loc_A1EE:
  lda (var_00FC),y
  cmp (var_00FE),y
  bcc loc_A223
  bne loc_A223
  iny
  dex
  bne loc_A1EE
  beq loc_A21B
loc_A1FC:
  tax
loc_A1FD:
  lda (var_00FC),y
  cmp (var_00FE),y
  bcc loc_A223
  bne loc_A223
  dey
  dex
  bne loc_A1FD
  beq loc_A21B
loc_A20B:
  ldy var_0000
  lda (var_00FC),y
  and #$10
  sta var_00DE
  lda (var_00FE),y
  and #$10
  cmp var_00DE
  bne loc_A223
loc_A21B:
  pla
  tax
  inx
  cpx var_00F5
  bne loc_A1D7
  rts
loc_A223:
  pla
  tax
  ror a
  eor var_6292,x
  sec
  rol a
  rts

sub_A22C:
  jsr sub_A255
  lda #$40
  jsr sub_8A3B
  jsr sub_A23D
  jsr sub_8A67
  jmp sub_A255

sub_A23D:
  jsr sub_9B79
  jsr sub_8BD6
  ldy #$14
  lda (var_00FE),y
  pha
  ldy #$16
  lda (var_00FE),y
  tax
  dey
  lda (var_00FE),y
  tay
  pla
  jmp loc_9BC6

sub_A255:
  lda var_00F3
  beq loc_A294
  inc var_00F4
  cmp var_00F4
  bne loc_A294
  lda var_00FF
  pha
  lda var_00FE
  pha
  lda #$20
  jsr sub_8A3B
  lda #$07
  jsr DOS_ENTRY
  tax
  bne loc_A277
  lda #$07
  jsr DOS_ENTRY
loc_A277:
  jsr sub_8A67
  lda #$43
  jsr sub_8A3B
  jsr sub_A23D
  ldx #$29
  jsr sub_8A5A
  jsr sub_8A67
  lda #$02
  sta var_00F4
  pla
  sta var_00FE
  pla
  sta var_00FF
loc_A294:
  rts

sub_A295:
  iny
  bit var_63DD
  bpl loc_A29D
  ora #$20
loc_A29D:
  jmp sub_8A59

sub_A2A0:
  lda var_00FC
  clc
  adc #$14
  sta var_00FC
  bcc loc_A2AB
  inc var_00FD
loc_A2AB:
  rts

sub_A2AC:
  lda var_00FD
  cmp var_00FB
  bne loc_A2B6
  lda var_00FC
  cmp var_00FA
loc_A2B6:
  rts

sub_A2B7:
  jsr sub_8BD6
  ldy #$00
  lda var_00F9
  beq loc_A2C4
  and (var_00FE),y
  beq loc_A2CA
loc_A2C4:
  lda var_00F8
  and (var_00FE),y
  beq loc_A2CC
loc_A2CA:
  sec
  rts
loc_A2CC:
  clc
  rts

sub_A2CE:
  inc var_00E4
  bne loc_A2D4
  inc var_00E5
loc_A2D4:
  ldy #$13
loc_A2D6:
  lda (var_00FE),y
  sta (var_00FA),y
  dey
  bpl loc_A2D6
  lda var_00F5
  beq sub_A2F3
  lda var_00FB
  cmp #$FE
  bcs loc_A2F2
  lda var_00FA
  clc
  adc #$14
  sta var_00FA
  bcc loc_A2F2
  inc var_00FB
loc_A2F2:
  rts

sub_A2F3:
  lda var_00F6
  bne loc_A310
  lda var_00F7
  beq loc_A345
  lsr var_00DF
  bcc loc_A30E
  lda #$1E
  jsr BIOS_ENTRY
  lda #$1C
  ldy #$0F
  jsr BIOS_ENTRY
  jmp loc_A316
loc_A30E:
  inc var_00DF
loc_A310:
  jsr sub_8A67
  jsr sub_A255
loc_A316:
  ldy #$00
  lda (var_00FE),y
  and #$10
  php
  beq loc_A324
  ldx #$5B
  jsr sub_8A5A
loc_A324:
  jsr sub_8E81
  ldy var_00E7
loc_A329:
  lda var_62E7,y
  beq loc_A334
  jsr sub_A295
  jmp loc_A329
loc_A334:
  plp
  beq loc_A33C
  ldx #$5D
  jsr sub_8A5A
loc_A33C:
  ldx var_00FF
  ldy var_00FE
  lda #$05
  jmp sub_8A70
loc_A345:
  jsr sub_8A67
  jsr sub_A255
  ldy #$09
loc_A34D:
  lda (var_00FE),y
  jsr sub_A295
  cpy #$11
  bne loc_A34D
  ldx #$20
  jsr sub_8A5A
loc_A35B:
  lda (var_00FE),y
  jsr sub_A295
  cpy #$14
  bne loc_A35B
  ldy #$00
  lda (var_00FE),y
  and #$10
  beq loc_A374
  lda #$3C
  jsr sub_8A3B
  jmp loc_A382
loc_A374:
  lda #$01
  sta var_63DE
  ldx var_00FF
  ldy var_00FE
  lda #$05
  jsr sub_8A9A
loc_A382:
  ldx #$20
  jsr sub_8A5A
  ldy #$04
  lda (var_00FE),y
  lsr a
  dey
  lda (var_00FE),y
  rol a
  rol a
  rol a
  rol a
  and #$0F
  jsr sub_8B13
  ldx #$2D
  jsr sub_8A5A
  ldy #$03
  lda (var_00FE),y
  and #$1F
  jsr sub_8B13
  ldx #$2D
  jsr sub_8A5A
  ldy #$04
  lda (var_00FE),y
  lsr a
  clc
  adc #$50
  jsr sub_8B13
  ldx #$08
  jmp sub_8A5A

sub_A3BB:
  lda #$B8
  sta var_00FB
  lda #$00
  sta var_00DF
  sta var_00FA
  sta var_00E5
  sta var_00E4
  sta var_00E3
  sta var_00E2
  sta var_00E1
  sta var_00E0
loc_A3D1:
  jsr sub_A2B7
  bcs loc_A3E5
  lda var_00E4
  ora var_00E5
  bne loc_A3DF
  jsr sub_A22C
loc_A3DF:
  jsr sub_8BD6
  jsr sub_A2CE
loc_A3E5:
  lda #$07
  jsr sub_8D73
  bcc loc_A3D1
  lda var_00F5
  beq loc_A435
loc_A3F0:
  ldx #$B8
  ldy #$00
  stx var_00FD
  sty var_00FC
loc_A3F8:
  jsr sub_A2AC
  beq loc_A435
  ldy #$00
  lda (var_00FC),y
  bpl loc_A41B
  jsr sub_A2A0
  jmp loc_A3F8
loc_A409:
  jsr sub_A2AC
  beq loc_A429
  ldy #$00
  lda (var_00FC),y
  bmi loc_A423
  jsr sub_A1D5
  beq loc_A423
  bcs loc_A423
loc_A41B:
  ldx var_00FD
  ldy var_00FC
  stx var_00FF
  sty var_00FE
loc_A423:
  jsr sub_A2A0
  jmp loc_A409
loc_A429:
  jsr sub_A2F3
  ldy #$00
  lda #$80
  sta (var_00FE),y
  jmp loc_A3F0
loc_A435:
  lda var_00E5
  ora var_00E4
  beq loc_A479
  clc
  lda var_00EC
  adc var_00E4
  sta var_00EC
  lda var_00ED
  adc var_00E5
  sta var_00ED
  ldy #$04
  ldx #$00
loc_A44C:
  lda var_00EE,x
  adc var_00E0,x
  sta var_00EE,x
  inx
  dey
  bne loc_A44C
  lda var_00F6
  bne loc_A479
  jsr sub_8A67
  jsr sub_A255
  jsr sub_8D21
  ldx #$00
  ldy #$E0
  lda #$00
  sta var_63DE
  jsr sub_8A9A
  lda #$01
  jsr sub_8A64
  jsr sub_A255
  lda #$01
loc_A479:
  rts

sub_A47A:
  lda #$17
  jsr sub_8D64
  bcs loc_A489
  jsr sub_A3BB
  beq loc_A489
  jsr sub_8A67
loc_A489:
  ldx var_00E7
  lda #$2A
  sta var_62E7,x
  sta var_62E9,x
  lda #$2E
  sta var_62E8,x
  lda #$00
  sta var_62EA,x
  lda #$17
  jsr sub_8D64
loc_A4A2:
  jsr sub_8BD6
  ldy #$00
  lda (var_00FE),y
  and #$10
  beq loc_A505
  ldy #$09
  lda (var_00FE),y
  cmp #$2E
  beq loc_A505
  ldy #$30
  lda var_00E7
  sta (var_00EA),y
  dey
loc_A4BC:
  lda var_0400,y
  sta (var_00EA),y
  dey
  bpl loc_A4BC
  lda var_00EA
  clc
  adc #$31
  sta var_00EA
  bcc loc_A4CF
  inc var_00EB
loc_A4CF:
  jsr sub_8E7E
  lda #$5C
  sta var_62E7,x
  inx
  stx var_00E7
  ldy #$00
loc_A4DC:
  lda var_6338,y
  sta var_62E7,x
  inx
  iny
  cmp #$00
  bne loc_A4DC
  jsr sub_A47A
  lda var_00EA
  sec
  sbc #$31
  sta var_00EA
  bcs loc_A4F6
  dec var_00EB
loc_A4F6:
  ldy #$30
  lda (var_00EA),y
  sta var_00E7
  dey
loc_A4FD:
  lda (var_00EA),y
  sta var_0400,y
  dey
  bpl loc_A4FD
loc_A505:
  lda #$07
  jsr sub_8D73
  bcc loc_A4A2
  rts

; 内部命令 DIR [drive:][path][filename] [/P][/W][/B][/A[:][attribs]][/L][/S][/O[:][sortord]]：列目录。用法说明在 $A0E0。
cmd_dir:
  lda #$0E
  sta var_00F8
  jsr sub_8DB8
  jsr sub_8E22
  sta var_0401
  lda #$C1
  jsr sub_8BEA
  bcs loc_A547
  jsr sub_8DB8
  beq loc_A52B
  ldx #$09
  jmp sub_8CA5
loc_A52B:
  lda var_00EA
  sta var_00E7
  jsr sub_8E72
  jsr sub_AF0C
  bcs loc_A547
  jsr sub_8E66
  inc var_00F4
  lda #$17
  jsr sub_8D64
  bcc loc_A549
  cmp #$12
  beq loc_A549
loc_A547:
  sec
  rts
loc_A549:
  ldx #$B8
  stx var_00FB
  ldx #$00
  stx var_00FA
  ldx var_00F2
  bne loc_A563
  tax
  bne loc_A55D
  jsr sub_A3BB
  bne loc_A5AB
loc_A55D:
  jsr sub_A22C
  jmp loc_A5CB
loc_A563:
  ldx var_00E7
  ldy #$00
loc_A567:
  lda var_62E7,x
  sta var_6338,y
  inx
  iny
  cmp #$00
  bne loc_A567
  ldx #$64
  ldy #$15
  stx var_00EB
  sty var_00EA
  jsr sub_A47A
  lda var_00ED
  ora var_00EC
  beq loc_A5CB
  lda var_00F6
  bne loc_A5CF
  jsr sub_A255
  lda #$44
  jsr sub_8A64
  jsr sub_A255
  ldx var_00ED
  ldy var_00EC
  jsr sub_8D25
  ldx #$00
  ldy #$EE
  lda #$00
  sta var_63DE
  jsr sub_8A9A
  lda #$01
  jsr sub_8A64
loc_A5AB:
  lda var_00F6
  bne loc_A5CF
  jsr sub_A255
  ldx #$09
  jsr sub_8A5A
  jsr sub_8D2D
  ldx #$04
  ldy #$07
  lda #$00
  sta var_63DE
  jsr sub_8A9A
  lda #$42
  jmp sub_8A64
loc_A5CB:
  lda #$02
  sec
  rts
loc_A5CF:
  jmp sub_8A67
