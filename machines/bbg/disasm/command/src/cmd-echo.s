; 模块：cmd-echo
; ECHO 命令

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_25D2"
; 段 0，$A5D2-$A6CE
  .byte $44, $69, $73, $70, $6C, $61, $79, $73, $20, $6D, $65, $73, $73, $61, $67, $65
  .byte $73, $2C, $20, $6F, $72, $20, $74, $75, $72, $6E, $73, $0D, $0A, $63, $6F, $6D
  .byte $6D, $61, $6E, $64, $2D, $65, $63, $68, $6F, $69, $6E, $67, $20, $6F, $6E, $20
  .byte $6F, $72, $20, $6F, $66, $66, $2E, $0D, $0A, $0A, $45, $43, $48, $4F, $20, $5B
  .byte $4F, $4E, $20, $7C, $20, $4F, $46, $46, $5D, $0D, $0A, $45, $43, $48, $4F, $20
  .byte $5B, $6D, $65, $73, $73, $61, $67, $65, $5D, $24, $CF, $D4, $CA, $BE, $D0, $C5
  .byte $CF, $A2, $BB, $F2, $B4, $F2, $BF, $AA, $2F, $B9, $D8, $B1, $D5, $C3, $FC, $C1
  .byte $EE, $D0, $D0, $BB, $D8, $CF, $D4, $0D, $0A, $0A, $45, $43, $48, $4F, $20, $5B
  .byte $4F, $4E, $20, $7C, $20, $4F, $46, $46, $5D, $0D, $0A, $45, $43, $48, $4F, $20
  .byte $5B, $D0, $C5, $CF, $A2, $5D, $24

; 内部命令 ECHO [ON | OFF]、ECHO [message]：显示信息或开关命令回显。
cmd_echo:
  jsr sub_8DB8
  jsr sub_8B2F
  bne loc_A682
  lda #$4A
  jsr sub_8A3B
  lda #$4C
  bit var_602B
  bpl loc_A67F
  lda #$4D
loc_A67F:
  jmp sub_8A64
loc_A682:
  sty var_00FF
  lda var_6292,y
  iny
  and #$DF
  cmp #$4F
  bne loc_A6C0
  lda var_6292,y
  iny
  and #$DF
  cmp #$4E
  beq loc_A6A6
  cmp #$46
  bne loc_A6C0
  lda var_6292,y
  iny
  and #$DF
  cmp #$46
  bne loc_A6C0
loc_A6A6:
  tax
  jsr sub_8B2F
  bne loc_A6C0
  ldy #$00
  cpx #$4E
  beq loc_A6B3
  dey
loc_A6B3:
  sty var_602C
  lda var_6022
  bne loc_A6BE
  sty var_602B
loc_A6BE:
  clc
  rts
loc_A6C0:
  ldy var_00FF
loc_A6C2:
  ldx var_6292,y
  cpx #$0D
  beq loc_A6BE
  jsr sub_8A5A
  iny
  bne loc_A6C2
