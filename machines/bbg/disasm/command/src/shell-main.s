; 模块：shell-main
; 命令解释器主循环：启动、批处理、提示符与 COMMAND 命令行选项

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_3480"
; 段 0，$B480-$BBAA
  .byte $68, $A8, $28, $A2, $FF, $9A, $A9, $37, $8D, $1E, $60, $A2, $03, $BD, $1A, $60
  .byte $48, $CA, $10, $F9, $90, $10, $98, $A2, $07, $18, $CA, $30, $09, $DD, $11, $8A
  .byte $D0, $F8, $BD, $0A, $8A, $38, $60

sub_B4A7:
  bit var_63DF
  bmi loc_B4BE
  ldx #$DB
  bne loc_B4B7

sub_B4B0:
  bit var_63DF
  bmi loc_B4BE

sub_B4B5:
  ldx #$20
loc_B4B7:
  lda #$1D
  ldy #$01
  jsr BIOS_ENTRY
loc_B4BE:
  rts

sub_B4BF:
  ldx var_62E3
  inx
  cpx #$50
  beq loc_B4DF
  sta var_6291,x
  inc var_62E3
  cmp #$20
  bcs loc_B4DA
  pha
  ldx #$5E
  jsr sub_8A5A
  pla
  ora #$40
loc_B4DA:
  tax
  jsr sub_8A5A
  clc
loc_B4DF:
  rts

sub_B4E0:
  lda #$00
  sta var_62E5
  ldx var_62E4
  cpx var_602D
  beq loc_B4F6
  lda data_B700,x
  jsr sub_B4BF
  inc var_62E4
loc_B4F6:
  rts

sub_B4F7:
  lda #$00
  sta var_62E5
  ldy var_62E3
  beq loc_B522
  dey
  sty var_62E3
  lda var_6292,y
  cmp #$20
  bcs loc_B517
  jsr sub_B4B0
  ldx #$08
  jsr sub_8A5A
  jsr sub_B4B5
loc_B517:
  jsr sub_B4B0
  ldx #$08
  jsr sub_8A5A
  jsr sub_B4B5
loc_B522:
  dec var_62E4
  bpl loc_B52A
  inc var_62E4
loc_B52A:
  rts

sub_B52B:
  jsr sub_B4A7
  lda #$07
  jsr DOS_ENTRY
  tax
  bne loc_B53D
  lda #$07
  jsr DOS_ENTRY
  ldx #$00
loc_B53D:
  rts
  lda #$00
  sta var_62E5
  jsr sub_B52B
  beq loc_B571
  ldx var_62E4
  cpx var_602D
  beq loc_B571
loc_B550:
  inx
  cpx var_602D
  beq loc_B571
  cmp data_B700,x
  bne loc_B550
  ldx var_62E4
loc_B55E:
  pha
  lda data_B700,x
  jsr sub_B4BF
  pla
  inc var_62E4
  ldx var_62E4
  cmp data_B700,x
  bne loc_B55E
loc_B571:
  rts
  lda #$00
  sta var_62E5
loc_B577:
  jsr sub_B4E0
  bne loc_B577
  rts
  lda #$00
  sta var_62E5
  jsr sub_B52B
  beq loc_B59A
  ldx var_62E4
loc_B58A:
  cpx var_602D
  beq loc_B59A
  cmp data_B700,x
  beq loc_B597
  inx
  bne loc_B58A
loc_B597:
  stx var_62E4
loc_B59A:
  rts
  inc var_62E5
  rts
  ldx var_62E4
  cpx var_602D
  beq loc_B5AA
  inc var_62E4
loc_B5AA:
  rts
data_B5AB:
  .byte $3B, $3C, $3D, $3E, $4B, $4D, $52, $53
data_B5B3:
  .byte $B4, $B5, $B5, $B5, $B4, $B4, $B5, $B5
data_B5BB:
  .byte $DF, $3D, $71, $7C, $F6, $DF, $9A, $9E

sub_B5C3:
  lda data_B5B3,x
  pha
  lda data_B5BB,x
  pha
  rts

sub_B5CC:
  lda #$1E
  jsr BIOS_ENTRY
  sty var_62E6
loc_B5D4:
  lda #$00
  sta var_62E3
  sta var_62E4
  sta var_62E5
loc_B5DF:
  jsr sub_B52B
  beq loc_B60C
  cmp #$08
  bne loc_B5EE
  jsr sub_B4F7
  jmp loc_B5DF
loc_B5EE:
  cmp #$1B
  bne loc_B61C
  ldx #$5C
  jsr sub_8A5A
  jsr sub_8A67
  lda var_62E6
  beq loc_B5D4
  sta var_00FD
loc_B601:
  ldx #$20
  jsr sub_8A5A
  dec var_00FD
  bne loc_B601
  beq loc_B5D4
loc_B60C:
  ldx #$08
loc_B60E:
  dex
  bmi loc_B5DF
  cmp data_B5AB,x
  bne loc_B60E
  jsr sub_B5C3
  jmp loc_B5DF
loc_B61C:
  cmp #$0D
  beq loc_B63E
  cmp #$09
  bne loc_B626
  lda #$20
loc_B626:
  jsr sub_B4BF
  bcs loc_B5DF
  lda var_62E5
  lsr a
  bcs loc_B5DF
  ldx var_62E4
  cpx var_602D
  beq loc_B5DF
  inc var_62E4
  bne loc_B5DF
loc_B63E:
  jsr sub_B4B0
  ldx #$0D
  jsr sub_8A5A
  lda #$0D
  ldy var_62E3
  sta var_6292,y
  sty var_602D
  beq loc_B65C
loc_B653:
  lda var_6291,y
  sta data_B6FF,y
  dey
  bne loc_B653
loc_B65C:
  rts
  .byte $20, $18, $8A
loc_B660:
  lda #$00
  sta var_602B
  sta var_602D
loc_B668:
  lda var_602B
  sta var_63E3
  ldx #$FF
  txs
  lda #$0B
  ldx #$00
  jsr DOS_ENTRY
  sta var_63DF
  jsr sub_8A67
  jsr sub_8EB7
  jsr sub_B5CC
  jsr command_dispatch
  jsr sub_B2C6
  jmp loc_B668
loc_B68D:
  jsr BIOS_ENTRY
  tya
  beq loc_B69F
  cpy #$03
  beq loc_B69C
  jsr sub_8ED3
  bne loc_B69F
loc_B69C:
  jsr command_dispatch
loc_B69F:
  lda var_6020
  cmp #$02
  bne loc_B6A9
  jmp loc_90A9
loc_B6A9:
  lda #$00
  sta var_6021
  jmp loc_B660
  .byte $41, $55, $54, $4F, $45, $58, $45, $43, $42, $41, $54
data_B6BC:
  .byte $41, $55, $54, $4F, $45, $58, $45, $43, $2E, $42, $41, $54, $0D, $4E, $6F, $20
  .byte $65, $6E, $6F, $75, $67, $68, $20, $6C, $6F, $77, $20, $52, $41, $4D, $20, $74
  .byte $6F, $20, $77, $6F, $72, $6B, $2E, $24, $B5, $CD, $B6, $CB, $C4, $DA, $B4, $E6
  .byte $B2, $BB, $D7, $E3, $2C, $B2, $BB, $C4, $DC, $D6, $B4, $D0, $D0, $24, $44, $4F
  .byte $53, $20, $76
data_B6FF:
  .byte $65
data_B700:
  .byte $72, $73, $69, $6F, $6E, $20, $69, $6E, $63, $6F, $72, $72, $65, $63, $74, $2E
  .byte $24, $44, $4F, $53, $B0, $E6, $B1, $BE, $BA, $C5, $B2, $BB, $B6, $D4, $21, $24
  .byte $0A, $42, $42, $47, $28, $52, $29, $20, $42, $42, $47, $44, $4F, $53, $28, $52
  .byte $29, $20, $56, $65, $72, $73, $69, $6F, $6E, $20, $33, $2E, $30, $0D, $0A, $28
  .byte $43, $29, $43, $6F, $70, $79, $72, $69, $67, $68, $74, $20, $52, $2E, $43, $2E
  .byte $54, $61, $6E, $67, $20, $31, $39, $39, $38, $0D, $0A, $24, $0A, $A1, $BA, $B2
  .byte $BD, $B2, $BD, $B8, $DF, $A1, $BB, $20, $42, $42, $47, $44, $4F, $53, $28, $52
  .byte $29, $20, $B5, $DA, $33, $2E, $30, $B0, $E6, $0D, $0A, $B0, $E6, $C8, $A8, $CB
  .byte $F9, $D3, $D0, $20, $C7, $E5, $BB, $AA, $B4, $F3, $D1, $A7, $20, $CC, $C6, $C8
  .byte $F0, $B4, $BA, $20, $31, $39, $39, $38, $2E, $0D, $0A, $24, $0A, $53, $74, $61
  .byte $72, $74, $20, $6E, $65, $77, $20, $42, $42, $47, $44, $4F, $53, $20, $63, $6F
  .byte $6D, $6D, $61, $6E, $64, $20, $73, $68, $65, $6C, $6C, $0D, $0A, $0A, $43, $4F
  .byte $4D, $4D, $41, $4E, $44, $20, $5B, $2F, $50, $5D, $0D, $0A, $20, $20, $20, $20
  .byte $20, $20, $20, $20, $5B, $2F, $59, $20, $5B, $2F, $4B, $20, $43, $6D, $64, $7C
  .byte $2F, $43, $20, $43, $6D, $64, $5D, $5D, $0D, $0A, $0A, $2F, $50, $20, $43, $6F
  .byte $6D, $6D, $61, $6E, $64, $20, $73, $68, $65, $6C, $6C, $20, $70, $65, $72, $6D
  .byte $61, $6E, $65, $6E, $74, $0D, $0A, $2F, $59, $20, $53, $74, $65, $70, $73, $20
  .byte $74, $68, $72, $6F, $75, $67, $68, $20, $43, $6D, $64, $20, $6F, $66, $20, $2F
  .byte $43, $2C, $2F, $4B, $0D, $0A, $2F, $4B, $20, $43, $6D, $64, $3A, $20, $45, $78
  .byte $65, $63, $75, $74, $65, $20, $43, $6D, $64, $20, $26, $20, $72, $65, $73, $69
  .byte $64, $65, $6E, $74, $0D, $0A, $2F, $43, $20, $43, $6D, $64, $3A, $20, $45, $78
  .byte $65, $63, $75, $74, $65, $20, $43, $6D, $64, $20, $26, $20, $65, $78, $69, $74
  .byte $24, $C6, $F4, $B6, $AF, $D2, $BB, $B8, $F6, $D0, $C2, $B5, $C4, $42, $42, $47
  .byte $44, $4F, $53, $C3, $FC, $C1, $EE, $BD, $E2, $CA, $CD, $B3, $CC, $D0, $F2, $0D
  .byte $0A, $43, $4F, $4D, $4D, $41, $4E, $44, $20, $5B, $2F, $50, $5D, $0D, $0A, $20
  .byte $20, $20, $20, $20, $20, $20, $20, $5B, $2F, $59, $20, $5B, $2F, $4B, $20, $C3
  .byte $FC, $C1, $EE, $7C, $2F, $43, $20, $C3, $FC, $C1, $EE, $5D, $5D, $0D, $0A, $0A
  .byte $2F, $50, $20, $43, $4F, $4D, $4D, $41, $4E, $44, $20, $D7, $A4, $C1, $F4, $B2
  .byte $BB, $D0, $ED, $CD, $CB, $B3, $F6, $28, $45, $58, $49, $54, $29, $0D, $0A, $2F
  .byte $59, $20, $B5, $A5, $B8, $F6, $D6, $B4, $D0, $D0, $2F, $43, $BA, $CD, $2F, $4B
  .byte $D6, $B8, $B6, $A8, $B5, $C4, $C3, $FC, $C1, $EE, $0D, $0A, $2F, $4B, $20, $C3
  .byte $FC, $C1, $EE, $3A, $D6, $B4, $D0, $D0, $A1, $B0, $C3, $FC, $C1, $EE, $A1, $B1
  .byte $B2, $A2, $D7, $A4, $C1, $F4, $C4, $DA, $B4, $E6, $0D, $0A, $2F, $43, $20, $C3
  .byte $FC, $C1, $EE, $3A, $D6, $B4, $D0, $D0, $A1, $B0, $C3, $FC, $C1, $EE, $A1, $B1
  .byte $C8, $BB, $BA, $F3, $CD, $CB, $B3, $F6, $24, $50, $35, $B9, $59, $39, $B9, $4B
  .byte $41, $B9, $43, $3D, $B9, $00, $EE, $E4, $63, $60, $EE, $E6, $63, $60, $A9, $02
  .byte $D0, $02, $A9, $01, $8D, $E5, $63, $20, $2F, $8B, $A2, $00, $B9, $92, $62, $9D
  .byte $92, $62, $C8, $E8, $C9, $0D, $D0, $F4, $88, $18, $60

sub_B95B:
  jsr sub_8DB8
  beq loc_B965
  ldx #$09
  jsr sub_8CA5
loc_B965:
  rts
  .byte $20, $0C, $58, $08, $98, $48, $20, $5A, $60, $4C, $7D, $B4, $A9, $01, $A2, $FE
  .byte $20, $00, $58, $A2, $80, $A0, $00, $86, $FF, $84, $FE, $98, $AA, $48, $18, $8A
  .byte $71, $FE, $AA, $C8, $68, $71, $FE, $C8, $D0, $F3, $E6, $FF, $CE, $1E, $60, $D0
  .byte $EC, $CD, $19, $60, $D0, $05, $EC, $18, $60, $F0, $41, $8C, $2D, $60, $A9, $FF
  .byte $85, $41, $A9, $14, $A2, $60, $A0, $F3, $20, $CB, $60, $90, $15, $A9, $04, $A2
  .byte $60, $A0, $D8, $20, $0C, $58, $A9, $07, $85, $FF, $A9, $08, $20, $0C, $58, $4C
  .byte $8C, $60, $A9, $FE, $85, $42, $A2, $37, $A0, $02, $86, $46, $84, $45, $A9, $02
  .byte $A2, $7F, $A0, $FE, $20, $CB, $60, $A9, $05, $20, $CF, $60, $60, $86, $44, $84
  .byte $43, $85, $40, $A2, $00, $A0, $40, $4C, $03, $58, $0D, $0A, $49, $6E, $73, $65
  .byte $72, $74, $20, $73, $79, $73, $74, $65, $6D, $20, $64, $69, $73, $6B, $20, $69
  .byte $6E, $20, $41, $3A, $24, $5C, $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D
  .byte $44, $00

sub_BA18:
  lda CTRL_C_ENTRY
  ldx var_5817
  ldy var_5816
  sta var_6025
  stx var_6027
  sty var_6026
  lda CRITICAL_ERROR_ENTRY
  ldx var_581A
  ldy var_5819
  sta var_6028
  stx var_602A
  sty var_6029
  rts
loc_BA3D:
  ldx #$FF
  txs
  lda #$0B
  ldx #$00
  jsr DOS_ENTRY
  sta var_63DF
  lda #$10
  jsr DOS_ENTRY
  bcs loc_BA59
  cmp #$03
  bne loc_BA59
  cpx #$00
  beq loc_BA5E
loc_BA59:
  lda #$50
  jmp loc_BABB
loc_BA5E:
  ldx #$00
loc_BA60:
  lda var_0101,x
  sta var_6292,x
  inx
  cmp #$0D
  bne loc_BA60
  ldx #$B9
  ldy #$29
  stx var_63E2
  sty var_63E1
  lda #$23
  sta var_63DB
  ldy #$00
  sty var_63E0
  sty var_63E4
  sty var_63E5
  sty var_63E6
  jsr sub_B95B
  lda var_63E0
  beq loc_BA99
  lda #$00
  ldx #$00
  ldy #$00
  jmp DOS_ENTRY
loc_BA99:
  lda var_63E4
  beq loc_BAAC
  lda var_63E5
  beq loc_BAE2
  cmp #$02
  bne loc_BB15
  lda #$00
  sta var_63E4
loc_BAAC:
  lda #$0F
  ldx #$00
  ldy #$34
  jsr BIOS_ENTRY
  cmp #$00
  beq loc_BAC7
  lda #$1E
loc_BABB:
  jsr sub_8A64
  lda #$00
  ldx #$FF
  ldy #$00
  jmp DOS_ENTRY
loc_BAC7:
  stx var_00FB
  sty var_00FA
  ldy #$33
loc_BACD:
  lda var_601A,y
  sta (var_00FA),y
  dey
  bpl loc_BACD
  ldx var_00FB
  ldy var_00FA
  stx var_6024
  sty var_6023
  jmp loc_BB15
loc_BAE2:
  ldy #$B2
loc_BAE4:
  lda loc_B965,y
  sta var_604D,y
  dey
  bne loc_BAE4
  lda #$02
  jsr BIOS_ENTRY
  sta var_0402
  lda #$00
  sta var_0401
  lda #$06
  ldx #$B6
  ldy #$B1
  jsr sub_8D6D
  bcs loc_BB15
  ldx #$0C
loc_BB07:
  lda data_B6BC,x
  sta var_6292,x
  dex
  bpl loc_BB07
  lda #$03
  sta var_63E5
loc_BB15:
  ldx #$80
  ldy #$00
  stx var_00FF
  sty var_00FE
  lda #$37
  sta var_601E
  sta var_00FC
  lda #$00
  tax
loc_BB27:
  pha
  clc
  txa
  adc (var_00FE),y
  tax
  iny
  pla
  adc (var_00FE),y
  iny
  bne loc_BB27
  inc var_00FF
  dec var_00FC
  bne loc_BB27
  sta var_6019
  stx var_6018
  jsr sub_BA18
  lda var_63E5
  bne loc_BB5D
  lda var_63E4
  beq loc_BB5D
  lda #$60
  sta CTRL_C_ENTRY
  jsr sub_96CF
  jsr sub_9919
  lda #$24
  jsr sub_8A3B
loc_BB5D:
  jsr sub_8875
  lda #$02
  jsr BIOS_ENTRY
  sta var_00F8
  ldx #$80
  ldy #$00
  stx var_00FA
  sty var_00F9
  lda #$FE
  sta var_00FB
  stx var_00FD
  sty var_00FC
  ldx #$37
  ldy #$00
  stx var_00FF
  sty var_00FE
  lda #$0C
  ldx #$F8
  jsr BIOS_ENTRY
  lda #$00
  sta var_6022
  sta var_602B
  lda var_63E4
  sta var_601F
  lda #$00
  ldy var_63E5
  beq loc_BB9E
  lda var_63E6
loc_BB9E:
  sta var_6021
  sty var_6020
  lda #$01
  ldx #$FE
  jmp loc_B68D
