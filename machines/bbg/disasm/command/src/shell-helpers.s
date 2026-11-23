; 模块：shell-helpers
; 命令解释器的公用例程：字符与数字处理、输出、路径与文件操作封装（边界未确认）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0874"
; 段 0，$8874-$9083
  .byte $60

sub_8875:
  ldy #$29
loc_8877:
  lda data_884B,y
  sta var_63EB,y
  dey
  bpl loc_8877
  lda #$4C
  ldx #$63
  ldy #$EB
  sta CTRL_C_ENTRY
  stx var_5817
  sty var_5816
  ldx #$64
  ldy #$00
  sta CRITICAL_ERROR_ENTRY
  stx var_581A
  sty var_5819
  rts

sub_889D:
  lda var_6025
  ldx var_6027
  ldy var_6026
  sta CTRL_C_ENTRY
  stx var_5817
  sty var_5816
  lda var_6028
  ldx var_602A
  ldy var_6029
  sta CRITICAL_ERROR_ENTRY
  stx var_581A
  sty var_5819
  rts
data_88C2:
  .byte $03
data_88C3:
  .byte $80, $0D, $80, $20, $80, $F9, $83, $6C, $92, $07, $81, $36, $80, $42, $80, $15
  .byte $81, $73, $80, $87, $80, $96, $80, $AF, $80, $EC, $80, $2F, $80, $57, $80, $20
  .byte $81, $30, $81, $37, $81, $4F, $81, $6E, $81, $8D, $81, $AC, $81, $BE, $81, $D2
  .byte $81, $F0, $81, $07, $82, $1C, $82, $2F, $82, $41, $82, $C9, $B6, $46, $84, $7D
  .byte $82, $2A, $9D, $CF, $A6, $9C, $B7, $20, $B7, $D5, $AF, $9B, $82, $68, $AE, $AD
  .byte $82, $AE, $AE, $C1, $82, $4B, $AF, $D5, $90, $C5, $93, $34, $91, $D7, $82, $4C
  .byte $90, $F0, $94, $F1, $82, $02, $83, $1C, $83, $F6, $97, $29, $83, $3A, $83, $4B
  .byte $83, $6C, $9A, $ED, $9B, $7D, $9C, $58, $83, $63, $83, $75, $83, $90, $83, $C3
  .byte $83, $D1, $83, $14, $80, $08, $84, $15, $84, $29, $84, $5E, $82, $54, $84, $6F
  .byte $84, $8E, $84, $AD, $84, $D2, $A5, $B6, $84, $B9, $84, $02, $94, $BD, $84, $FA
  .byte $B6, $D6, $84
data_8966:
  .byte $D7
data_8967:
  .byte $84, $DF, $84, $EF, $84, $98, $87, $BD, $92, $80, $85, $01, $85, $0B, $85, $89
  .byte $85, $1F, $85, $28, $85, $31, $85, $49, $85, $71, $85, $FC, $84, $14, $85, $91
  .byte $85, $A1, $85, $A8, $85, $B5, $85, $C6, $85, $E1, $85, $F8, $85, $01, $86, $12
  .byte $86, $2F, $86, $48, $86, $55, $86, $60, $86, $6B, $86, $E4, $B6, $D5, $87, $9B
  .byte $86, $2F, $9F, $7A, $A7, $61, $B8, $5C, $B7, $5A, $B0, $AB, $86, $89, $AE, $BD
  .byte $86, $DC, $AE, $CD, $86, $6C, $AF, $04, $91, $DD, $93, $71, $91, $DC, $86, $72
  .byte $90, $95, $95, $F7, $86, $02, $87, $18, $87, $76, $98, $21, $87, $2E, $87, $3B
  .byte $87, $F2, $9A, $27, $9C, $C2, $9C, $44, $87, $4F, $87, $58, $87, $65, $87, $7C
  .byte $87, $84, $87, $E5, $84, $A3, $87, $AA, $87, $BA, $87, $7D, $86, $DE, $87, $ED
  .byte $87, $09, $88, $26, $88, $2C, $A6, $2F, $88, $32, $88, $63, $94, $35, $88, $11
  .byte $B7, $D6, $84, $18, $46, $1A, $1C, $1B, $19, $49, $08, $28, $0B, $02, $1E, $29
  .byte $2A

sub_8A18:
  ldx #$5E
  jsr sub_8A29
  ldx #$43
  jsr sub_8A29
  ldx #$0D
  jsr sub_8A29
  ldx #$0A

sub_8A29:
  lda #$1B
  ldy #$00
  jmp BIOS_ENTRY

sub_8A30:
  cmp #$61
  bcc loc_8A3A
  cmp #$7B
  bcs loc_8A3A
  and #$DF
loc_8A3A:
  rts

sub_8A3B:
  asl a
  tax
  lda data_88C3,x
  ldy data_88C2,x
  bit var_63DF
  bpl loc_8A4E
  lda data_8967,x
  ldy data_8966,x
loc_8A4E:
  tax
  lda #$04
  jmp DOS_ENTRY

sub_8A54:
  pha
  jsr sub_8A5A
  pla

sub_8A59:
  tax

sub_8A5A:
  tya
  pha
  lda #$02
  jsr DOS_ENTRY
  pla
  tay
  rts

sub_8A64:
  jsr sub_8A3B

sub_8A67:
  ldx #$0D
  lda #$0A
  jsr sub_8A54
  clc
  rts

sub_8A70:
  stx var_00FF
  sty var_00FE
  tay
  lda (var_00FE),y
  sta var_0000
  clc
  adc var_00E0
  sta var_00E0
  iny
  lda (var_00FE),y
  sta var_0001
  adc var_00E1
  sta var_00E1
  iny
  lda (var_00FE),y
  sta var_0002
  adc var_00E2
  sta var_00E2
  iny
  lda (var_00FE),y
  sta var_0003
  adc var_00E3
  sta var_00E3
  rts

sub_8A9A:
  jsr sub_8A70
  ldx #$0B
loc_8A9F:
  ldy #$20
  lda #$00
loc_8AA3:
  rol var_0000
  rol var_0001
  rol var_0002
  rol var_0003
  dey
  bmi loc_8AB7
  rol a
  cmp #$0A
  bcc loc_8AA3
  sbc #$0A
  bcs loc_8AA3
loc_8AB7:
  ora #$30
  sta var_007F,x
  dex
  beq loc_8AED
  lda var_0000
  ora var_0001
  ora var_0002
  ora var_0003
  beq loc_8AE6
  cpx #$08
  beq loc_8ADF
  cpx #$04
  bne loc_8A9F
  lda var_63DE
  beq loc_8ADF
  lda var_0001
  bne loc_8A9F
  lda var_0000
  cmp #$0A
  bcs loc_8A9F
loc_8ADF:
  lda #$2C
  sta var_007F,x
  dex
  bne loc_8A9F
loc_8AE6:
  lda #$20
loc_8AE8:
  sta var_007F,x
  dex
  bne loc_8AE8
loc_8AED:
  ldy var_63DE
loc_8AF0:
  ldx var_0080,y
  iny
  jsr sub_8A5A
  cpy #$0B
  bne loc_8AF0
  rts

sub_8AFB:
  sty var_00FD
  sta var_00FC
  txa
  ldy #$08
loc_8B02:
  rol var_00FD
  rol a
  cmp var_00FC
  bcc loc_8B0B
  sbc var_00FC
loc_8B0B:
  dey
  bne loc_8B02
  tax
  lda var_00FD
  rol a
  rts

sub_8B13:
  cmp #$64
  bcc loc_8B19
  sbc #$64
loc_8B19:
  tay
  ldx #$00
  lda #$0A
  jsr sub_8AFB
  stx var_63DE
  ora #$30
  tax
  lda var_63DE
  ora #$30
  jmp sub_8A54

sub_8B2F:
  lda var_6292,y
  cmp #$20
  beq loc_8B4A
  cmp #$09
  beq loc_8B4A
  cmp #$2C
  beq loc_8B4A
  cmp #$3B
  beq loc_8B4A
  cmp #$5D
  beq loc_8B4A
  cmp #$5B
  bne loc_8B4D
loc_8B4A:
  iny
  bne sub_8B2F
loc_8B4D:
  cmp #$0D
  rts

sub_8B50:
  lda var_6292,y
  cmp #$2B
  beq loc_8B78

sub_8B57:
  lda var_6292,y

sub_8B5A:
  cmp #$2F
  beq loc_8B78

sub_8B5E:
  cmp #$0D
  beq loc_8B78
  cmp #$20
  beq loc_8B78
  cmp #$09
  beq loc_8B78
  cmp #$2C
  beq loc_8B78
  cmp #$3B
  beq loc_8B78
  cmp #$5D
  beq loc_8B78
  cmp #$5B
loc_8B78:
  rts

sub_8B79:
  ldx var_00EA
  ldy #$00
loc_8B7D:
  lda var_62E7,x
  beq loc_8B98
  cpy #$08
  beq loc_8B8F
  cmp #$2A
  beq loc_8B99
  cmp #$3F
  beq loc_8B99
  iny
loc_8B8F:
  inx
  cmp #$2E
  bne loc_8B7D
  cpy #$01
  beq loc_8B99
loc_8B98:
  clc
loc_8B99:
  rts

sub_8B9A:
  tya
  pha
  jsr sub_8B79
  bcs loc_8BB7
  ldy #$08
loc_8BA3:
  lda var_62E7,x
  beq loc_8BB6
  cmp #$2A
  beq loc_8BB7
  cmp #$3F
  beq loc_8BB7
  inx
  iny
  cpy #$0B
  bne loc_8BA3
loc_8BB6:
  clc
loc_8BB7:
  pla
  tay
  rts

sub_8BBA:
  ldx #$00
loc_8BBC:
  jsr sub_8B50
  beq loc_8BD0
  sta var_62E7,x
  iny
  inx
  cmp #$5C
  bne loc_8BBC
  stx var_00EA
  sty var_00E8
  beq loc_8BBC
loc_8BD0:
  lda #$00
  sta var_62E7,x
  rts

sub_8BD6:
  ldx var_0404
  ldy var_0403
  stx var_00FF
  sty var_00FE
  rts

sub_8BE1:
  ldx var_00EA
loc_8BE3:
  inx
  lda var_62E7,x
  bne loc_8BE3
  rts

sub_8BEA:
  sta var_00E6
  stx var_0401
  sty var_00E9
  ldx #$00
  stx var_00EA
  jsr sub_8B50
  beq loc_8C14
  sty var_00E8
  jsr sub_8BBA
  sty var_00E9
  ldx var_00EA
  beq loc_8C1D
  ldy var_00E8
  jsr sub_8B50
  bne loc_8C1D
  lda #$03
  cpx #$01
  bne loc_8C1B
  beq sub_8C8A
loc_8C14:
  lda var_00E6
  lsr a
  bcs sub_8C8A
  lda #$02
loc_8C1B:
  sec
  rts
loc_8C1D:
  bit var_00E6
  bpl loc_8C45
  jsr sub_8B9A
  bcs loc_8C45
  lda #$17
  sta var_0405
  lda #$16
loc_8C2D:
  jsr sub_8D69
  bcs loc_8C41
  jsr sub_8BD6
  ldy #$00
  lda (var_00FE),y
  and #$10
  bne loc_8C7F
  lda #$07
  bne loc_8C2D
loc_8C41:
  cmp #$12
  bne loc_8C1B
loc_8C45:
  ldx var_00EA
  ldy var_00E8
  jsr sub_8B50
  cmp #$2E
  bne loc_8C61
  iny
  jsr sub_8B50
  beq sub_8C8A
  bit var_00E6
  bvc loc_8CA1
  dey
  lda #$2A
  sta var_62E7,x
  inx
loc_8C61:
  bit var_00E6
  bvc loc_8CA1
loc_8C65:
  jsr sub_8B50
  beq loc_8C90
  iny
  sta var_62E7,x
  inx
  cmp #$2E
  bne loc_8C65
loc_8C73:
  jsr sub_8B50
  beq loc_8C9C
  iny
  sta var_62E7,x
  inx
  bne loc_8C73
loc_8C7F:
  jsr sub_8BE1
  lda #$5C
  sta var_62E7,x
  inx
  stx var_00EA

sub_8C8A:
  lda #$2A
  sta var_62E7,x
  inx
loc_8C90:
  lda #$2E
  sta var_62E7,x
  inx
  lda #$2A
  sta var_62E7,x
  inx
loc_8C9C:
  lda #$00
  sta var_62E7,x
loc_8CA1:
  ldy var_00E9
  clc
  rts

sub_8CA5:
  tya
  pha
  txa
  jsr sub_8A3B
  ldx #$20
  jsr sub_8A5A
  ldx #$2D
  lda #$20
  jsr sub_8A54
  pla
  tay
  lda var_6292,y
  jsr sub_8B5E
  beq loc_8CCA
loc_8CC1:
  iny
  jsr sub_8A59
  jsr sub_8B57
  bne loc_8CC1
loc_8CCA:
  jsr sub_8A67
  clc
  rts

sub_8CCF:
  lda #$20
  sta var_0080
  sta var_0081
  sta var_0082
  lda #$64
  clc
  jsr sub_8AFB
  beq loc_8CFE
  stx var_63DE
  ldx #$00
  tay
  lda #$0A
  jsr sub_8AFB
  beq loc_8CF1
  ora #$30
  sta var_0080
  sec
loc_8CF1:
  txa
  bne loc_8CF6
  bcc loc_8CFE
loc_8CF6:
  ora #$30
  sta var_0081
  sec
  ldx var_63DE
loc_8CFE:
  txa
  tay
  ldx #$00
  lda #$0A
  jsr sub_8AFB
  bne loc_8D0B
  bcc loc_8D0F
loc_8D0B:
  ora #$30
  sta var_0082
loc_8D0F:
  txa
  ora #$30
  sta var_0083
  lda #$24
  sta var_0084
  ldx #$00
  ldy #$80
  lda #$04
  jmp DOS_ENTRY

sub_8D21:
  ldx var_00E5
  ldy var_00E4

sub_8D25:
  jsr sub_8CCF
  lda #$00
  jmp sub_8A3B

sub_8D2D:
  lda #$0B
  jsr sub_8D73
  bcs loc_8D63
  lda var_0403
loc_8D37:
  lsr a
  bcs loc_8D43
  asl var_0405
  rol var_0406
  jmp loc_8D37
loc_8D43:
  lda #$00
  sta var_0409
  sta var_040A
loc_8D4B:
  lsr var_0406
  ror var_0405
  bcs loc_8D62
  asl var_0407
  rol var_0408
  rol var_0409
  rol var_040A
  jmp loc_8D4B
loc_8D62:
  clc
loc_8D63:
  rts

sub_8D64:
  sta var_0405
  lda #$16

sub_8D69:
  ldx #$62
  ldy #$E7

sub_8D6D:
  stx var_0404
  sty var_0403

sub_8D73:
  sta var_0400
  ldx #$04
  ldy #$00
  jmp FILE_ENTRY

sub_8D7D:
  ldx #$62
  ldy #$E7
  jmp sub_8D88
  .byte $A2, $63, $A0, $38

sub_8D88:
  stx var_6104
  sty var_6103

sub_8D8E:
  sta var_6100
  ldx #$61
  ldy #$00
  jmp FILE_ENTRY

sub_8D98:
  ldx #$63
  ldy #$89
  stx var_6134
  sty var_6133

sub_8DA2:
  sta var_6130
  ldx #$61
  ldy #$30
  jmp FILE_ENTRY

sub_8DAC:
  iny
  iny
  lda (var_00FE),y
  pha
  dey
  lda (var_00FE),y
  pha
  txa
  tay
  rts

sub_8DB8:
  jsr sub_8B2F
  beq loc_8E16
  cmp #$2F
  bne loc_8E16
  iny
  tya
  tax
  lda var_6292,y
  cmp #$0D
  beq loc_8E0C
  cmp #$3F
  beq loc_8E17
  and #$DF
  sta var_00FD
  lda var_63E2
  sta var_00FF
  lda var_63E1
  sta var_00FE
  ldy #$00
  lda var_6293,x
  jsr sub_8B5A
  beq loc_8DF3
loc_8DE7:
  lda (var_00FE),y
  beq loc_8E0C
  bmi loc_8DF2
  iny
  iny
  iny
  bne loc_8DE7
loc_8DF2:
  iny
loc_8DF3:
  lda (var_00FE),y
  beq loc_8E0C
  bmi loc_8E0C
  cmp var_00FD
  beq loc_8E02
  iny
  iny
  iny
  bne loc_8DF3
loc_8E02:
  inx
  clc
  jsr sub_8DAC
  bcc sub_8DB8
  tya
  tax
  dex
loc_8E0C:
  dex
  pla
  pla
  txa
  tay
  ldx #$0A
  jmp sub_8CA5
loc_8E16:
  rts
loc_8E17:
  sta var_63E0
  pla
  pla
  lda var_63DB
  jmp sub_8A64

sub_8E22:
  lda var_6292,y
  cmp #$0D
  beq loc_8E4B
  ldx var_6293,y
  cmp #$0D
  beq loc_8E4B
  cpx #$3A
  bne loc_8E4B
  and #$DF
  cmp #$5B
  bcs loc_8E3E
  cmp #$41
  bcs loc_8E44
loc_8E3E:
  pla
  pla
  lda #$0F
  sec
  rts
loc_8E44:
  sbc #$40
  iny
  iny
  tax
  clc
  rts
loc_8E4B:
  tya
  pha
  lda #$09
  sta var_6160
  ldx #$00
  stx var_6161
  ldx #$61
  ldy #$60
  jsr FILE_ENTRY
  pla
  tay
  ldx var_6161
  lda #$00
  rts

sub_8E66:
  ldx #$FF
loc_8E68:
  inx
  lda var_6389,x
  sta var_62E7,x
  bne loc_8E68
  rts

sub_8E72:
  ldx #$FF
loc_8E74:
  inx
  lda var_62E7,x
  sta var_6389,x
  bne loc_8E74
  rts

sub_8E7E:
  jsr sub_8BD6

sub_8E81:
  ldx var_00E7
  ldy #$09
loc_8E85:
  lda (var_00FE),y
  cmp #$20
  beq loc_8E94
  sta var_62E7,x
  inx
  iny
  cpy #$11
  bne loc_8E85
loc_8E94:
  ldy #$11
  lda (var_00FE),y
  cmp #$20
  beq loc_8EB1
  lda #$2E
  sta var_62E7,x
  inx
loc_8EA2:
  lda (var_00FE),y
  cmp #$20
  beq loc_8EB1
  sta var_62E7,x
  inx
  iny
  cpy #$14
  bne loc_8EA2
loc_8EB1:
  lda #$00
  sta var_62E7,x
  rts

sub_8EB7:
  ldx #$00
  stx var_0401
  jsr sub_9BB3
  bcc loc_8EC6
  lda #$47
  jsr sub_8A3B
loc_8EC6:
  ldx #$3E
  jmp sub_8A5A
  .byte $20, $5B, $59, $2F, $4E, $5D, $3F, $24

sub_8ED3:
  lda var_6021
  beq loc_8F13
  lda var_6292
  cmp #$0D
  beq loc_8F13
  jsr sub_8A67
  ldy #$00
loc_8EE4:
  ldx var_6292,y
  cpx #$0D
  beq loc_8EF1
  jsr sub_8A5A
  iny
  bne loc_8EE4
loc_8EF1:
  lda #$04
  ldx #$8E
  ldy #$CB
  jsr DOS_ENTRY
loc_8EFA:
  lda #$07
  jsr DOS_ENTRY
  and #$DF
  cmp #$59
  beq loc_8F09
  cmp #$4E
  bne loc_8EFA
loc_8F09:
  pha
  jsr sub_8A59
  jsr sub_8A67
  pla
  cmp #$59
loc_8F13:
  rts

sub_8F14:
  ldx #$63
  ldy #$DA
  stx var_6032
  sty var_6031
  ldx #$00
  ldy #$01
  stx var_6034
  sty var_6033
  lda #$02
  sta var_602E
  ldx #$60
  ldy #$2E
  jsr FILE_ENTRY
  lda var_6033
  rts
  .byte $A2, $FD, $9A, $A9, $37, $8D, $1E, $60, $20, $18, $8A, $A9, $45, $20, $3B, $8A
  .byte $A9, $01, $20, $0C, $58, $29, $DF, $C9, $59, $F0, $19, $C9, $4E, $D0, $EC, $20
  .byte $67, $8A, $20, $14, $8F, $B0, $0D, $F0, $0B, $AD, $DA, $63, $C9, $0D, $F0, $4A
  .byte $C9, $1A, $D0, $EE, $20, $67, $8A, $4C, $38, $90
loc_8F72:
  lda var_6022
  beq loc_8F85
  pla
  pla
  lda #$05
  sta var_602E
  ldx #$60
  ldy #$2E
  jsr FILE_ENTRY
loc_8F85:
  inc var_6022
  jsr sub_8A67
  lda var_602B
  sta var_602C
  lda var_0401
  sta var_602F
  ldx #$62
  ldy #$E7
  stx var_6032
  sty var_6031
  lda #$14
  sta var_602E
  ldx #$60
  ldy #$2E
  jsr FILE_ENTRY
  bcc loc_8FB2
  lda #$02
  rts
loc_8FB2:
  lda var_602C
  sta var_63E3
loc_8FB8:
  jsr sub_8F14
  bcs loc_9038
  beq loc_9028
  lda var_63DA
  cmp #$20
  beq loc_8FB8
  cmp #$09
  beq loc_8FB8
  cmp #$0A
  beq loc_8FB8
  cmp #$40
  bne loc_8FDC
  dec var_63E3
  jsr sub_8F14
  bcs loc_9038
  beq loc_9028
loc_8FDC:
  bit var_63E3
  bmi loc_8FE4
  jsr sub_8EB7
loc_8FE4:
  ldy #$00
  beq loc_8FF6
loc_8FE8:
  tya
  pha
loc_8FEA:
  jsr sub_8F14
  pla
  tay
  bcs loc_9038
  lda var_6033
  beq loc_9028
loc_8FF6:
  lda var_63DA
  cmp #$0D
  beq loc_9017
  cmp #$1A
  beq loc_9028
  cpy #$4F
  bcs loc_8FE8
  sta var_6292,y
  iny
  tax
  tya
  pha
  bit var_63E3
  bmi loc_8FEA
  jsr sub_8A5A
  jmp loc_8FEA
loc_9017:
  sta var_6292,y
  jsr sub_8ED3
  bne loc_8FB2
  jsr command_dispatch
  jsr sub_B2C6
  jmp loc_8FB2
loc_9028:
  lda #$0D
  sta var_6292,y
  jsr sub_8ED3
  bne loc_9038
  jsr command_dispatch
  jsr sub_B2C6
loc_9038:
  lda #$05
  sta var_602E
  ldx #$60
  ldy #$2E
  jsr FILE_ENTRY
  ldy #$00
  sty var_6022
  clc
  rts
  .byte $00, $51, $75, $69, $74, $73, $20, $74, $68, $65, $20, $43, $4F, $4D, $4D, $41
  .byte $4E, $44, $2E, $43, $4D, $44, $20, $70, $72, $6F, $67, $72, $61, $6D, $2E, $0D
  .byte $0A, $0A, $45, $58, $49, $54, $24, $CD, $CB, $B3, $F6, $C3, $FC, $C1, $EE, $BD
  .byte $E2, $CA, $CD, $C6, $F7, $B3, $CC, $D0, $F2
