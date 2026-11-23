; 模块：cmd-ver-vol-help-del
; VER、VOL、?、DEL、ERASE 命令

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_2E68"
; 段 0，$AE68-$B1BC
  .byte $44, $69, $73, $70, $6C, $61, $79, $20, $74, $68, $65, $20, $42, $42, $47, $44
  .byte $4F, $53, $20, $76, $65, $72, $73, $69, $6F, $6E, $0D, $0A, $0A, $56, $45, $52
  .byte $24, $CF, $D4, $CA, $BE, $42, $42, $47, $44, $4F, $53, $B0, $E6, $B1, $BE, $BA
  .byte $C5, $0D, $0A, $56, $45, $52, $24

; 内部命令 VER：显示 BBGDOS 版本（“BBG(R) BBGDOS(R) Version 3.0”）。
cmd_ver:
  jsr sub_8DB8
  beq loc_AEA9
  ldx #$09
  jmp sub_8CA5
loc_AEA9:
  lda #$28
  jmp sub_8A64
  .byte $44, $69, $73, $70, $6C, $61, $79, $73, $20, $74, $68, $65, $20, $64, $69, $73
  .byte $6B, $20, $76, $6F, $6C, $75, $6D, $65, $20, $6C, $61, $62, $65, $6C, $0D, $0A
  .byte $0A, $56, $4F, $4C, $20, $5B, $64, $72, $69, $76, $65, $3A, $5D, $24, $CF, $D4
  .byte $CA, $BE, $B4, $C5, $C5, $CC, $B5, $C4, $BE, $ED, $B1, $EA, $0D, $0A, $0A, $56
  .byte $4F, $4C, $20, $5B, $C5, $CC, $3A, $5D, $24

; 内部命令 VOL [drive:]：显示磁盘卷标。
cmd_vol:
  jsr sub_8DB8
  jsr sub_8E22
  bcs loc_AF49
  sta var_0401
  jsr sub_8DB8
  beq sub_AF0C
  ldx #$09
  jmp sub_8CA5

sub_AF0C:
  ldx #$00
  jsr sub_8C8A
  lda #$08
  jsr sub_8D64
  bcc loc_AF1C
  cmp #$12
  bne loc_AF49
loc_AF1C:
  php
  lda #$0E
  jsr sub_8A3B
  lda var_0401
  ora #$40
  jsr sub_8A59
  plp
  bcs loc_AF44
  lda #$06
  jsr sub_8A3B
  jsr sub_8BD6
  ldy #$09
loc_AF37:
  lda (var_00FE),y
  jsr sub_8A59
  iny
  cpy #$14
  bne loc_AF37
  jmp sub_8A67
loc_AF44:
  lda #$07
  jmp sub_8A64
loc_AF49:
  sec
  rts
  .byte $44, $69, $73, $70, $6C, $61, $79, $20, $44, $4F, $53, $20, $69, $6E, $74, $65
  .byte $72, $6E, $61, $6C, $20, $63, $6F, $6D, $6D, $61, $6E, $64, $0D, $0A, $0A, $3F
  .byte $24, $CF, $D4, $CA, $BE, $44, $4F, $53, $C4, $DA, $B2, $BF, $C3, $FC, $C1, $EE
  .byte $0D, $0A, $3F, $24

sub_AF7F:
  tya
  pha
  lda #$1E
  jsr BIOS_ENTRY
  tya
  beq loc_AF9F
  ldy #$0A
  cmp #$0A
  bcc loc_AF9A
  ldy #$14
  cmp #$14
  bcc loc_AF9A
  jsr sub_8A67
  bcc loc_AF9F
loc_AF9A:
  lda #$1C
  jsr BIOS_ENTRY
loc_AF9F:
  pla
  tay
loc_AFA1:
  ldx data_B1CC,y
  beq loc_AFAC
  jsr sub_8A5A
  iny
  bne loc_AFA1
loc_AFAC:
  rts

; 内部命令 ?：列出内部命令。
cmd_help:
  jsr sub_8DB8
  beq loc_AFB7
  ldx #$09
  jmp sub_8CA5
loc_AFB7:
  jsr sub_8A67
  ldy #$00
loc_AFBC:
  jsr sub_AF7F
  iny
  lda data_B1CC,y
  beq loc_AFC8
  jsr sub_AF7F
loc_AFC8:
  tya
  clc
  adc #$06
  tay
  lda data_B1CC,y
  bne loc_AFBC
  jmp sub_8A67
  .byte $44, $65, $6C, $65, $74, $65, $73, $20, $6F, $6E, $65, $20, $6F, $72, $20, $6D
  .byte $6F, $72, $65, $20, $66, $69, $6C, $65, $73, $0D, $0A, $0A, $45, $52, $41, $53
  .byte $45, $20, $6F, $72, $0D, $0A, $44, $45, $4C, $20, $5B, $64, $72, $69, $76, $65
  .byte $3A, $5D, $5B, $70, $61, $74, $68, $5D, $66, $69, $6C, $65, $6E, $61, $6D, $65
  .byte $5B, $2F, $50, $5D, $0D, $0A, $0A, $4D, $6F, $72, $65, $20, $66, $69, $6C, $65
  .byte $73, $20, $62, $79, $20, $75, $73, $65, $69, $6E, $67, $20, $77, $69, $6C, $64
  .byte $63, $61, $72, $64, $73, $0D, $0A, $0A, $2F, $50, $20, $50, $72, $6F, $6D, $70
  .byte $74, $73, $20, $66, $6F, $72, $20, $44, $45, $4C, $20, $65, $61, $63, $68, $20
  .byte $66, $69, $6C, $65, $24, $C9, $BE, $B3, $FD, $D2, $BB, $B8, $F6, $BB, $F2, $B6
  .byte $E0, $B8, $F6, $CE, $C4, $BC, $FE, $0D, $0A, $45, $52, $41, $53, $45, $20, $BB
  .byte $F2, $0D, $0A, $44, $45, $4C, $20, $5B, $C5, $CC, $3A, $5D, $5B, $C2, $B7, $BE
  .byte $B6, $5D, $CE, $C4, $BC, $FE, $C3, $FB, $20, $5B, $2F, $50, $5D, $0D, $0A, $0A
  .byte $C9, $BE, $B3, $FD, $B6, $E0, $B8, $F6, $CE, $C4, $BC, $FE, $BF, $C9, $D3, $C3
  .byte $CD, $A8, $C5, $E4, $B7, $FB, $28, $2A, $29, $BB, $F2, $28, $3F, $29, $0D, $0A
  .byte $0A, $2F, $50, $20, $C3, $BF, $C9, $BE, $B3, $FD, $D2, $BB, $B8, $F6, $CE, $C4
  .byte $BC, $FE, $C7, $B0, $D2, $AA, $C8, $B7, $C8, $CF, $24, $50, $D3, $B0, $00, $E6
  .byte $FB, $60

; 内部命令 DEL、ERASE [drive:][path]filename [/P]：删除一个或多个文件，可用通配符，/P 表示每个文件都询问。
cmd_del:
  jsr sub_8DB8
  bne loc_B0E0
  lda #$0D
  sec
loc_B0DF:
  rts
loc_B0E0:
  jsr sub_8E22
  sta var_6388
  lda #$80
  jsr sub_8BEA
  bcs loc_B0DF
  lda var_00EA
  sta var_00E7
  jsr sub_8DB8
  beq loc_B0FB
  ldx #$09
  jmp sub_8CA5
loc_B0FB:
  lda var_00FB
  bne loc_B153
  ldx var_00E7
  ldy #$08
loc_B103:
  lda var_62E7,x
  beq loc_B153
  inx
  cmp #$2A
  beq loc_B114
  cmp #$3F
  bne loc_B153
  dey
  bne loc_B103
loc_B114:
  lda var_62E7,x
  beq loc_B153
  inx
  cmp #$2E
  bne loc_B114
loc_B11E:
  lda var_62E7,x
  beq loc_B153
  inx
  cmp #$2A
  beq loc_B12F
  cmp #$3F
  bne loc_B153
  dey
  bne loc_B11E
loc_B12F:
  lda #$0C
  jsr sub_8A3B
  lda #$03
  sta var_00E0
  lda #$06
  ldx #$00
  ldy #$E0
  jsr DOS_ENTRY
  jsr sub_8A67
  lda var_00E2
  and #$DF
  cmp #$59
  beq loc_B153
  cmp #$4E
  beq loc_B1BC
  jmp loc_B12F
loc_B153:
  lda #$01
  sta var_0405
  lda #$16
loc_B15A:
  jsr sub_8D69
  bcs loc_B1A8
  jsr sub_8BD6
  ldy #$00
  lda (var_00FE),y
  and #$01
  beq loc_B16E
  sta var_00F9
  bne loc_B1A3
loc_B16E:
  jsr sub_8E7E
  lda var_00FB
  beq loc_B192
loc_B175:
  jsr sub_A920
  lda #$26
  jsr sub_8A3B
  lda #$01
  jsr DOS_ENTRY
  pha
  jsr sub_8A67
  pla
  and #$DF
  cmp #$4E
  beq loc_B19F
  tax
  cmp #$59
  bne loc_B175
loc_B192:
  lda var_6388
  sta var_6101
  lda #$17
  jsr sub_8D7D
  bcs loc_B1BA
loc_B19F:
  lda #$01
  sta var_00FA
loc_B1A3:
  lda #$07
  jmp loc_B15A
loc_B1A8:
  cmp #$12
  bne loc_B1BA
  lda var_00FA
  bne loc_B1BC
  lda var_00F9
  beq loc_B1B8
  lda #$05
  sec
  rts
loc_B1B8:
  lda #$02
loc_B1BA:
  sec
  rts
loc_B1BC:
  clc
