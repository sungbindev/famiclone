; 模块：cmd-exit-rem-type-rename
; EXIT、REM、TYPE、RENAME 命令

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_1084"
; 段 0，$9084-$93EE
  .byte $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D, $44, $0D, $0A, $45, $58, $49
  .byte $54, $24

; 内部命令 EXIT：退出 COMMAND.CMD（常驻的命令解释器拒绝退出）。
cmd_exit:
  jsr sub_8DB8
  beq loc_90A0
  ldx #$09
  jmp sub_8CA5
loc_90A0:
  lda var_601F
  beq loc_90A9
  lda #$48
  sec
  rts
loc_90A9:
  lda #$05
  ldx #$00
  ldy #$34
  jsr BIOS_ENTRY
  ldx var_6024
  ldy var_6023
  stx var_00FF
  sty var_00FE
  ldy #$33
loc_90BE:
  lda (var_00FE),y
  sta var_601A,y
  dey
  bpl loc_90BE
  lda #$00
  sta var_602D
  jsr sub_889D
  lda #$00
  ldy #$00
  jmp DOS_ENTRY
  .byte $52, $65, $6D, $61, $72, $6B, $73, $20, $69, $6E, $20, $62, $61, $74, $63, $68
  .byte $20, $6F, $72, $20, $43, $4F, $4E, $46, $49, $47, $2E, $53, $59, $53, $0D, $0A
  .byte $0A, $52, $45, $4D, $20, $5B, $63, $6F, $6D, $6D, $65, $6E, $74, $5D, $24, $C5
  .byte $FA, $B4, $A6, $C0, $ED, $BB, $F2, $43, $4F, $4E, $46, $49, $47, $2E, $53, $59
  .byte $53, $D6, $D0, $B5, $C4, $D7, $A2, $CA, $CD, $C3, $FC, $C1, $EE, $0D, $0A, $52
  .byte $45, $4D, $20, $5B, $63, $6F, $6D, $6D, $65, $6E, $74, $5D, $24

; 内部命令 REM：批处理或 CONFIG.SYS 里的注释，不执行任何操作。
cmd_rem:
  clc
  rts
  .byte $44, $69, $73, $70, $6C, $61, $79, $20, $74, $68, $65, $20, $63, $6F, $6E, $74
  .byte $65, $6E, $74, $73, $20, $6F, $66, $20, $61, $20, $66, $69, $6C, $65, $0D, $0A
  .byte $0A, $54, $59, $50, $45, $20, $5B, $64, $72, $69, $76, $65, $3A, $5D, $5B, $70
  .byte $61, $74, $68, $5D, $66, $69, $6C, $65, $6E, $61, $6D, $65, $24, $CF, $D4, $CA
  .byte $BE, $D2, $BB, $B8, $F6, $CE, $C4, $B1, $BE, $CE, $C4, $BC, $FE, $B5, $C4, $C4
  .byte $DA, $C8, $DD, $0D, $0A, $54, $59, $50, $45, $20, $5B, $C5, $CC, $3A, $5D, $5B
  .byte $C2, $B7, $BE, $B6, $5D, $CE, $C4, $BC, $FE, $C3, $FB, $24

; 内部命令 TYPE [drive:][path]filename：显示文件内容。
cmd_type:
  jsr sub_8DB8
  bne loc_91A9
  lda #$0D
  sec
  rts
loc_91A9:
  jsr sub_8E22
  sta var_6337
  sty var_00FB
  lda #$00
  jsr sub_8BEA
  bcs loc_91CD
  jsr sub_8DB8
  beq loc_91C1
  ldx #$09
  bne loc_91DF
loc_91C1:
  jsr sub_8B9A
  bcs loc_91D0
  lda #$14
  jsr sub_8D69
  bcc loc_91D4
loc_91CD:
  tax
  bne loc_91DD
loc_91D0:
  lda #$15
  sec
  rts
loc_91D4:
  lda var_0402
  and #$18
  beq loc_91E2
  ldx #$05
loc_91DD:
  ldy var_00FB
loc_91DF:
  jmp sub_8CA5
loc_91E2:
  ldx var_63DC
  cpx #$FE
  bcc loc_91ED
  lda #$2F
  sec
  rts
loc_91ED:
  stx var_0402
  ldx #$40
  ldy #$00
  stx var_0406
  sty var_0405
loc_91FA:
  lda #$02
  ldx #$80
  ldy #$00
  stx var_00FA
  jsr sub_8D6D
  bcs loc_926A
  ldx var_0406
  ldy var_0405
  txa
  ora var_0405
  beq loc_9265
  stx var_00F9
  sty var_00F8
loc_9217:
  ldx var_63DC
  stx var_0080
  ldx var_00FA
  ldy #$00
  stx var_0082
  sty var_0081
  inc var_00FA
  ldx #$61
  ldy #$90
  stx var_0085
  sty var_0084
  ldx #$01
  ldy #$00
  stx var_0087
  sty var_0086
  lda #$0C
  ldx #$80
  jsr BIOS_ENTRY
  ldy #$00
loc_923F:
  ldx var_6190,y
  cpx #$1A
  beq loc_9265
  jsr sub_8A5A
  lda var_00F8
  sec
  sbc #$01
  sta var_00F8
  bcs loc_9254
  dec var_00F9
loc_9254:
  ora var_00F9
  beq loc_925E
  iny
  bne loc_923F
  jmp loc_9217
loc_925E:
  lda var_0406
  cmp #$40
  beq loc_91FA
loc_9265:
  lda #$05
  jsr sub_8D73
loc_926A:
  clc
  rts
  .byte $52, $65, $6E, $61, $6D, $65, $73, $20, $61, $20, $66, $69, $6C, $65, $20, $6F
  .byte $72, $20, $66, $69, $6C, $65, $73, $0D, $0A, $0A, $52, $45, $4E, $41, $4D, $45
  .byte $20, $6F, $72, $0D, $0A, $52, $45, $4E, $20, $5B, $64, $72, $69, $76, $65, $3A
  .byte $5D, $5B, $70, $61, $74, $68, $5D, $66, $69, $6C, $65, $6E, $61, $6D, $65, $31
  .byte $0D, $0A, $20, $20, $20, $20, $20, $66, $69, $6C, $65, $6E, $61, $6D, $65, $32
  .byte $24, $B6, $D4, $D2, $BB, $B8, $F6, $CE, $C4, $BC, $FE, $BB, $F2, $B6, $E0, $B8
  .byte $F6, $CE, $C4, $BC, $FE, $B8, $C4, $C3, $FB, $0D, $0A, $52, $45, $4E, $41, $4D
  .byte $45, $20, $BB, $F2, $0D, $0A, $52, $45, $4E, $20, $5B, $C5, $CC, $3A, $5D, $5B
  .byte $C2, $B7, $BE, $B6, $5D, $CE, $C4, $BC, $FE, $C3, $FB, $31, $20, $CE, $C4, $BC
  .byte $FE, $C3, $FB, $32, $24

; 内部命令 RENAME、REN [drive:][path]filename1 filename2：给文件改名。
cmd_rename:
  jsr sub_8DB8
  beq loc_931B
  jsr sub_8E22
  sta var_6101
  sta var_6131
  lda #$00
  jsr sub_8BEA
  bcs loc_931E
  jsr sub_8DB8
  bne loc_931F
loc_931B:
  lda #$0D
  sec
loc_931E:
  rts
loc_931F:
  jsr sub_8E72
  lda var_00EA
  sta var_00EB
  sta var_00E7
  jsr sub_8E22
  tax
  beq loc_9332
  lda #$16
  sec
  rts
loc_9332:
  lda #$00
  jsr sub_8BEA
  bcs loc_934A
  jsr sub_8DB8
  beq loc_9343
  ldx #$09
  jmp sub_8CA5
loc_9343:
  ldx var_00EA
  beq loc_934B
  lda #$15
  sec
loc_934A:
  rts
loc_934B:
  stx var_00EC
  dex
loc_934E:
  inx
  lda var_62E7,x
  sta var_6338,x
  bne loc_934E
  jsr sub_8E66
  lda #$01
  sta var_0405
  lda #$16
loc_9361:
  jsr sub_8D69
  bcs loc_93B6
  lda #$01
  sta var_00FB
  jsr sub_8E7E
  jsr sub_A88D
  lda #$14
  jsr sub_8D98
  bcc loc_93AD
  cmp #$02
  bne loc_93C3
  lda #$14
  jsr sub_8D7D
  bcs loc_93C3
  ldx #$63
  lda #$89
  clc
  adc var_00EB
  bcc loc_938C
  inx
loc_938C:
  stx var_00FF
  sta var_00FE
  tay
  lda #$0C
  jsr DOS_ENTRY
  lda #$0D
  ldx var_00FF
  ldy var_00FE
  jsr sub_8D88
  bcs loc_93BE
  lda #$05
  jsr sub_8D8E
  bcs loc_93C3
  lda #$07
  jmp loc_9361
loc_93AD:
  lda #$05
  jsr sub_8DA2
  lda #$17
  sec
  rts
loc_93B6:
  lda var_00FB
  bne loc_93C3
  lda #$02
  sec
  rts
loc_93BE:
  lda #$05
  jsr sub_8D8E
loc_93C3:
  clc
  rts
  .byte $43, $6C, $65, $61, $72, $73, $20, $74, $68, $65, $20, $73, $63, $72, $65, $65
  .byte $6E, $0D, $0A, $0A, $43, $4C, $53, $24, $C7, $E5, $B3, $FD, $CF, $D4, $CA, $BE
  .byte $C6, $C1, $C4, $BB, $0D, $0A, $43, $4C, $53, $24
