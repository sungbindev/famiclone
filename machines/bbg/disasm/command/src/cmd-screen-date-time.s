; 模块：cmd-screen-date-time
; CLS、SCREEN、DATE、TIME 命令

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_13EF"
; 段 0，$93EF-$9B83
; 内部命令 CLS：清除屏幕（调用 DOS 功能 9 初始化屏幕）。
cmd_cls:
  jsr sub_8DB8
  lda #$09
  jsr DOS_ENTRY
  lda #$1C
  ldx #$01
  ldy #$00
  jsr BIOS_ENTRY
  clc
  rts
  .byte $53, $65, $74, $20, $6E, $65, $77, $20, $73, $63, $72, $65, $65, $6E, $20, $64
  .byte $69, $73, $70, $6C, $61, $79, $20, $6D, $6F, $64, $65, $0D, $0A, $0A, $53, $43
  .byte $52, $45, $45, $4E, $20, $4E, $75, $6D, $0D, $0A, $0A, $6E, $75, $6D, $3A, $20
  .byte $4E, $65, $77, $20, $73, $63, $72, $65, $65, $6E, $20, $6D, $6F, $64, $65, $20
  .byte $6E, $75, $6D, $62, $65, $72, $2E, $0D, $0A, $20, $20, $20, $20, $20, $30, $3A
  .byte $20, $41, $53, $43, $49, $49, $2C, $20, $31, $3A, $20, $48, $41, $4E, $5A, $49
  .byte $24, $C9, $E8, $D6, $C3, $D0, $C2, $B5, $C4, $C6, $C1, $C4, $BB, $CF, $D4, $CA
  .byte $BE, $B7, $BD, $CA, $BD, $0D, $0A, $0A, $53, $43, $52, $45, $45, $4E, $20, $CA
  .byte $FD, $D6, $B5, $0D, $0A, $0A, $CA, $FD, $D6, $B5, $3A, $20, $D0, $C2, $B5, $C4
  .byte $C6, $C1, $C4, $BB, $CF, $D4, $CA, $BE, $B7, $BD, $CA, $BD, $0D, $0A, $20, $20
  .byte $20, $20, $30, $3A, $20, $D3, $A2, $CE, $C4, $2C, $20, $31, $3A, $20, $D6, $D0
  .byte $2F, $D3, $A2, $CE, $C4, $24

; 内部命令 SCREEN num：设置新的屏幕显示方式，0 为 ASCII，1 为 HANZI。
cmd_screen:
  jsr sub_8DB8
  bne loc_94C1
  lda #$0D
  sec
  rts
loc_94C1:
  jsr sub_96A9
  bne loc_94EC
  cmp #$0D
  bne loc_94EC
  ldy var_00FF
  cpy #$01
  bcc loc_94D4
  bne loc_94EC
  ldy #$80
loc_94D4:
  lda #$0B
  ldx #$01
  jsr DOS_ENTRY
  bcs loc_94EC
  lda #$0B
  ldx #$00
  jsr DOS_ENTRY
  sta var_63DF
  lda #$28
  jmp sub_8A64
loc_94EC:
  lda #$4F
  sec
  rts
  .byte $44, $69, $73, $70, $6C, $61, $79, $73, $20, $6F, $72, $20, $73, $65, $74, $73
  .byte $20, $74, $68, $65, $20, $64, $61, $74, $65, $2E, $0D, $0A, $0A, $44, $41, $54
  .byte $45, $20, $5B, $6D, $6D, $2D, $64, $64, $2D, $79, $79, $5D, $0D, $0A, $0A, $6D
  .byte $6D, $2D, $64, $64, $2D, $79, $79, $20, $54, $68, $65, $20, $64, $61, $74, $65
  .byte $20, $74, $68, $61, $74, $20, $79, $6F, $75, $20, $73, $65, $74, $0D, $0A, $0A
  .byte $54, $79, $70, $65, $20, $44, $41, $54, $45, $20, $6F, $6E, $6C, $79, $20, $74
  .byte $6F, $20, $73, $68, $6F, $77, $20, $64, $61, $74, $65, $20, $61, $6E, $64, $77
  .byte $61, $69, $74, $20, $66, $6F, $72, $20, $61, $20, $6E, $65, $77, $20, $6F, $6E
  .byte $65, $2E, $20, $50, $72, $65, $73, $73, $20, $45, $4E, $54, $45, $52, $74, $6F
  .byte $20, $6B, $65, $65, $70, $20, $74, $68, $65, $20, $73, $61, $6D, $65, $20, $64
  .byte $61, $74, $65, $2E, $24, $CF, $D4, $CA, $BE, $BB, $F2, $C9, $E8, $D6, $C3, $C8
  .byte $D5, $C6, $DA, $0D, $0A, $44, $41, $54, $45, $20, $5B, $6D, $6D, $2D, $64, $64
  .byte $2D, $79, $79, $5D, $0D, $0A, $0A, $6D, $6D, $2D, $64, $64, $2D, $79, $79, $20
  .byte $D2, $AA, $C9, $E8, $D6, $C3, $B5, $C4, $C8, $D5, $C6, $DA, $0D, $0A, $0A, $D6
  .byte $BB, $CA, $E4, $C8, $EB, $44, $41, $54, $45, $CF, $D4, $CA, $BE, $B5, $B1, $C7
  .byte $B0, $C8, $D5, $C6, $DA, $B2, $A2, $B5, $C8, $B4, $FD, $CA, $E4, $0D, $0A, $C8
  .byte $EB, $D0, $C2, $C8, $D5, $C6, $DA, $A3, $AC, $B0, $B4, $BB, $D8, $B3, $B5, $D4
  .byte $F2, $B1, $A3, $B3, $D6, $C8, $D5, $C6, $DA, $B2, $BB, $B1, $E4, $24

sub_960E:
  lsr a
  clc
  adc #$BC
  tay
  lda #$00
  adc #$07
  tax
  lda #$64
  jsr sub_8AFB
  stx var_00FE
  jsr sub_8B13
  lda var_00FE
  jmp sub_8B13

sub_9627:
  lsr a
  lda var_00FF
  ror a
  lsr a
  lsr a
  lsr a
  lsr a
  jmp sub_8B13

sub_9632:
  lda var_6292,y
  cmp #$3A
  bcs loc_963E
  cmp #$30
  bcs loc_963F
  sec
loc_963E:
  rts
loc_963F:
  sbc #$30
  clc
  rts
data_9643:
  .byte $B6
data_9644:
  .byte $FE, $C8, $FD, $CB, $C4, $CE, $E5, $C1, $F9, $C8, $D5, $D2, $BB
data_9651:
  .byte $54
data_9652:
  .byte $75
data_9653:
  .byte $65, $57, $65, $6E, $54, $68, $75, $46, $72, $69, $53, $61, $74, $53, $75, $6E
  .byte $4D, $6F
data_9665:
  .byte $6E, $00, $03, $03, $06, $01, $04, $06, $02, $05, $00, $03, $05

sub_9672:
  lsr a
  sta var_00FC
  lda var_00FF
  ror a
  lsr a
  lsr a
  lsr a
  lsr a
  tay
  lda var_00FC
  clc
  adc #$03
  pha
  and #$03
  tax
  pla
  lsr a
  lsr a
  clc
  adc var_00FC
  sta var_00FC
  lda var_00FF
  and #$1F
  sbc #$00
  cpx #$03
  bcc loc_969A
  cpy #$03
loc_969A:
  adc var_00FC
  adc data_9665,y
  tay
  ldx #$00
  lda #$07
  jsr sub_8AFB
  txa
  rts

sub_96A9:
  ldx #$00
  stx var_00FF
loc_96AD:
  jsr sub_9632
  bcs loc_96C4
  iny
  sta var_00FE
  lda #$0A
  jsr sub_98D3
  clc
  adc var_00FE
  sta var_00FF
  bcc loc_96AD
  inx
  bcs loc_96AD
loc_96C4:
  cpx #$00
  rts

; 内部命令 DATE [mm-dd-yy]：显示或设置日期，只输入 DATE 时显示后等待输入新值。
cmd_date:
  jsr sub_8DB8
  beq sub_96CF
  jmp loc_9786

sub_96CF:
  lda #$32
  jsr sub_8A3B
  lda #$3F
  jsr BIOS_ENTRY
  bit var_63DF
  bpl loc_9723
  pha
  jsr sub_960E
  ldx #$C4
  lda #$EA
  jsr sub_8A54
  pla
  pha
  jsr sub_9627
  ldx #$D4
  lda #$C2
  jsr sub_8A54
  lda var_00FF
  and #$1F
  jsr sub_8B13
  ldx #$C8
  lda #$D5
  jsr sub_8A54
  ldx #$D0
  lda #$C7
  jsr sub_8A54
  ldx #$C6
  lda #$DA
  jsr sub_8A54
  pla
  jsr sub_9672
  asl a
  tay
  ldx data_9643,y
  lda data_9644,y
  jsr sub_8A54
  jmp loc_975B
loc_9723:
  pha
  jsr sub_9672
  sta var_00FE
  asl a
  adc var_00FE
  tay
  lda data_9653,y
  pha
  ldx data_9651,y
  lda data_9652,y
  jsr sub_8A54
  pla
  tax
  lda #$20
  jsr sub_8A54
  pla
  pha
  jsr sub_9627
  ldx #$2D
  jsr sub_8A5A
  lda var_00FF
  and #$1F
  jsr sub_8B13
  ldx #$2D
  jsr sub_8A5A
  pla
  jsr sub_960E
loc_975B:
  jsr sub_8A67
  lda #$33
  jsr sub_8A64
  lda #$1F
  sta var_6290
  lda #$06
  ldx #$62
  ldy #$90
  jsr DOS_ENTRY
  bcc loc_9778
  jsr sub_8A18
  clc
  rts
loc_9778:
  ldy #$00
  jsr sub_8B2F
  beq loc_97E9
  tya
  pha
  jsr sub_8A67
  pla
  tay
loc_9786:
  jsr sub_96A9
  bne loc_97EB
  cmp #$2D
  bne loc_97EB
  lda var_00FF
  beq loc_97EB
  cmp #$0D
  bcs loc_97EB
  sta var_00F9
  iny
  jsr sub_96A9
  bne loc_97EB
  cmp #$2D
  bne loc_97EB
  lda var_00FF
  beq loc_97EB
  cmp #$20
  bcs loc_97EB
  sta var_00FA
  iny
  jsr sub_96A9
  jsr sub_8B2F
  bne loc_97EB
  ldy var_00FF
  txa
  bne loc_97C6
  cpy #$64
  bcs loc_97EB
  tya
  sbc #$4F
  bcc loc_97EB
  bcs loc_97D6
loc_97C6:
  cpy #$BC
  sbc #$07
  bcc loc_97EB
  txa
  cpy #$34
  sbc #$08
  bcs loc_97EB
  tya
  sbc #$BB
loc_97D6:
  ldy var_00F9
  cpy #$08
  rol a
  tax
  tya
  ror a
  ror a
  ror a
  ror a
  ora var_00FA
  tay
  lda #$40
  jsr BIOS_ENTRY
loc_97E9:
  clc
  rts
loc_97EB:
  jsr sub_8A67
  lda #$34
  jsr sub_8A3B
  jmp loc_975B
  .byte $44, $69, $73, $70, $6C, $61, $79, $73, $20, $6F, $72, $20, $73, $65, $74, $73
  .byte $20, $74, $68, $65, $20, $74, $69, $6D, $65, $2E, $0D, $0A, $0A, $54, $49, $4D
  .byte $45, $20, $5B, $74, $69, $6D, $65, $5D, $0D, $0A, $0A, $54, $79, $70, $65, $20
  .byte $54, $49, $4D, $45, $20, $6F, $6E, $6C, $79, $20, $74, $6F, $20, $73, $68, $6F
  .byte $77, $20, $74, $69, $6D, $65, $20, $61, $6E, $64, $77, $61, $69, $74, $20, $66
  .byte $6F, $72, $20, $61, $20, $6E, $65, $77, $20, $6F, $6E, $65, $2E, $20, $50, $72
  .byte $65, $73, $73, $20, $45, $4E, $54, $45, $52, $74, $6F, $20, $6B, $65, $65, $70
  .byte $20, $74, $68, $65, $20, $73, $61, $6D, $65, $20, $74, $69, $6D, $65, $2E, $24
  .byte $CF, $D4, $CA, $BE, $BB, $F2, $C9, $E8, $D6, $C3, $CA, $B1, $BC, $E4, $0D, $0A
  .byte $54, $49, $4D, $45, $20, $5B, $74, $69, $6D, $65, $5D, $0D, $0A, $0A, $D6, $BB
  .byte $CA, $E4, $C8, $EB, $54, $49, $4D, $45, $CF, $D4, $CA, $BE, $B5, $B1, $C7, $B0
  .byte $CA, $B1, $BC, $E4, $B2, $A2, $B5, $C8, $B4, $FD, $CA, $E4, $0D, $0A, $C8, $EB
  .byte $D0, $C2, $C8, $D5, $C6, $DA, $A3, $AC, $B0, $B4, $BB, $D8, $B3, $B5, $D4, $F2
  .byte $B1, $A3, $B3, $D6, $CA, $B1, $BC, $E4, $B2, $BB, $B1, $E4, $24

sub_98D3:
  sta var_00FB
  stx var_00FD
  ldx #$08
  lda #$00
  sta var_00FC
loc_98DD:
  ror var_00FB
  bcc loc_98EC
  clc
  adc var_00FF
  pha
  lda var_00FC
  adc var_00FD
  sta var_00FC
  pla
loc_98EC:
  ror var_00FC
  ror a
  dex
  bne loc_98DD
  tax
  lda var_00FB
  ror a
  rts

sub_98F7:
  stx var_00FE
  tax
  jsr sub_8B5E
  beq loc_9910
  ora #$20
  cmp #$61
  beq loc_9910
  cmp #$70
  beq loc_9910
  cpx var_00FE
  bne loc_9910
  iny
  ldx #$00
loc_9910:
  rts

; 内部命令 TIME [time]：显示或设置时间，只输入 TIME 时显示后等待输入新值。
cmd_time:
  jsr sub_8DB8
  beq sub_9919
  jmp loc_99A8

sub_9919:
  lda #$36
  jsr sub_8A3B
  lda #$3F
  jsr BIOS_ENTRY
  txa
  pha
  sty var_00FF
  txa
  ldx #$61
  lsr a
  lsr a
  lsr a
  cmp #$0C
  bcc loc_9935
  sbc #$0C
  ldx #$70
loc_9935:
  stx var_00FA
  tax
  bne loc_993C
  lda #$0C
loc_993C:
  jsr sub_8B13
  ldx #$3A
  jsr sub_8A5A
  pla
  sta var_00FD
  lda var_00FF
  asl a
  rol var_00FD
  asl a
  rol var_00FD
  asl a
  rol var_00FD
  lda var_00FD
  and #$3F
  jsr sub_8B13
  ldx #$3A
  jsr sub_8A5A
  lda var_00FE
  cmp #$32
  lda var_00FF
  and #$1F
  rol a
  jsr sub_8B13
  ldx #$2E
  jsr sub_8A5A
  lda var_00FE
  cmp #$32
  bcc loc_9977
  sbc #$32
loc_9977:
  jsr sub_8B13
  ldx var_00FA
  jsr sub_8A5A
loc_997F:
  jsr sub_8A67
  lda #$37
  jsr sub_8A3B
  lda #$0E
  sta var_6290
  lda #$06
  ldx #$62
  ldy #$90
  jsr DOS_ENTRY
  bcc loc_999C
  jsr sub_8A18
  clc
  rts
loc_999C:
  jsr sub_8A67
  ldy #$00
  jsr sub_8B2F
  bne loc_99A8
  clc
  rts
loc_99A8:
  jsr sub_96A9
  bne loc_9A19
  ldx #$3A
  jsr sub_98F7
  bne loc_9A19
  lda var_00FF
  sta var_00FA
  jsr sub_96A9
  bne loc_9A19
  ldx #$3A
  jsr sub_98F7
  bne loc_9A19
  lda var_00FF
  cmp #$3C
  bcs loc_9A19
  sta var_00F9
  jsr sub_96A9
  bne loc_9A19
  ldx #$2E
  jsr sub_98F7
  bne loc_9A19
  lda var_00FF
  cmp #$3C
  bcs loc_9A19
  sta var_00F8
  jsr sub_96A9
  bne loc_9A19
  ldx #$0D
  jsr sub_98F7
  bne loc_9A19
  tax
  lda var_00FF
  cmp #$64
  bcs loc_9A19
  sta var_00F7
  lda var_00FA
  cpx #$61
  beq loc_9A01
  cpx #$70
  bne loc_9A02
  adc #$0B
loc_9A01:
  iny
loc_9A02:
  cmp #$18
  bcs loc_9A19
  asl var_00F9
  asl var_00F9
  asl var_00F9
  rol a
  asl var_00F9
  rol a
  asl var_00F9
  rol a
  tax
  jsr sub_8B2F
  beq loc_9A24
loc_9A19:
  jsr sub_8A67
  lda #$38
  jsr sub_8A3B
  jmp loc_997F
loc_9A24:
  lda var_00F8
  lsr a
  ora var_00F9
  tay
  lda var_00F7
  bcc loc_9A30
  adc #$63
loc_9A30:
  lsr a
  sta var_00FF
  lda #$41
  jsr BIOS_ENTRY
  clc
  rts
  .byte $55, $44, $9A, $5A, $46, $9A, $FF, $5A, $48, $9A, $00, $E6, $FB, $18, $60, $B9
  .byte $92, $62, $C8, $C9, $30, $90, $04, $C9, $3A, $90, $01, $38, $60

; 内部命令 BBGCDOS：启动中文系统（用法说明在 $9A3A）。未确认：具体行为。
cmd_bbgcdos:
  jsr sub_8DB8
  beq loc_9A61
  ldx #$09
  jmp sub_8CA5
loc_9A61:
  ldy #$80
  lda var_00FB
  beq loc_9A69
  ldy #$00
loc_9A69:
  jmp loc_94D4
  .byte $44, $69, $73, $70, $6C, $61, $79, $73, $20, $6F, $72, $20, $63, $68, $61, $6E
  .byte $67, $65, $73, $20, $64, $69, $72, $65, $63, $74, $6F, $72, $79, $0D, $0A, $0A
  .byte $43, $48, $44, $49, $52, $7C, $43, $44, $20, $5B, $64, $72, $69, $76, $65, $3A
  .byte $5D, $5B, $70, $61, $74, $68, $5D, $0D, $0A, $43, $48, $44, $49, $52, $7C, $43
  .byte $44, $5B, $2E, $2E, $5D, $0D, $0A, $0A, $54, $79, $70, $65, $20, $43, $44, $20
  .byte $77, $69, $74, $68, $6F, $75, $74, $20, $64, $69, $72, $65, $63, $74, $6F, $72
  .byte $79, $20, $77, $69, $6C, $6C, $0D, $0A, $64, $69, $73, $70, $6C, $61, $79, $20
  .byte $74, $68, $65, $20, $63, $75, $72, $72, $65, $6E, $74, $20, $64, $69, $72, $65
  .byte $63, $74, $6F, $72, $79, $24, $CF, $D4, $CA, $BE, $B5, $B1, $C7, $B0, $C4, $BF
  .byte $C2, $BC, $C3, $FB, $BB, $F2, $B8, $C4, $B1, $E4, $B5, $B1, $C7, $B0, $C4, $BF
  .byte $C2, $BC, $0D, $0A, $43, $48, $44, $49, $52, $7C, $43, $44, $20, $5B, $C5, $CC
  .byte $3A, $5D, $5B, $C2, $B7, $BE, $B6, $5D, $0D, $0A, $43, $48, $44, $49, $52, $7C
  .byte $43, $44, $5B, $2E, $2E, $5D, $0D, $0A, $0A, $CA, $E4, $C8, $EB, $20, $43, $44
  .byte $20, $C5, $CC, $3A, $20, $CF, $D4, $CA, $BE, $D6, $B8, $B6, $A8, $C5, $CC, $B5
  .byte $B1, $C7, $B0, $C4, $BF, $C2, $BC, $0D, $0A, $D6, $BB, $CA, $E4, $C8, $EB, $43
  .byte $44, $CF, $D4, $CA, $BE, $B5, $B1, $C7, $B0, $C5, $CC, $BC, $B0, $B5, $B1, $C7
  .byte $B0, $C4, $BF, $C2, $BC, $24

sub_9B72:
  lda #$20
  ldx var_00FD
  jmp BIOS_ENTRY

sub_9B79:
  lda var_0401

sub_9B7C:
  ora #$40
  tax
  lda #$3A
  jmp sub_8A54
