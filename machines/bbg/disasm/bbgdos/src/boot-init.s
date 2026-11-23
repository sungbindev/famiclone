; 模块：boot-init
; BBGDOS.SYS 装入后的初始化与 $5803、$580C 入口桩的模板（常驻部分，初始化后即可释放）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，$7C00-$7FFF
sub_7C00:
  jmp loc_7C96
  .byte $0A, $53, $74, $61, $72, $74, $69, $6E, $67, $20, $42, $42, $47, $44, $4F, $53
  .byte $2E, $2E, $2E, $0D, $0A, $24, $43, $4F, $4E, $46, $49, $47, $20, $20, $53, $59
  .byte $53
data_7C24:
  .byte $A9, $02, $20, $00, $58, $48, $8A, $48, $A9, $01, $AE
data_7C2F:
  .byte $3C
data_7C30:
  .byte $7C, $20, $00, $58, $68, $AA, $68, $20, $BE, $90, $98, $60
data_7C3C:
  .byte $00
data_7C3D:
  .byte $48, $8A, $48, $A9, $02, $20, $00, $58, $48, $A9, $01, $AE
data_7C49:
  .byte $69
data_7C4A:
  .byte $7C, $20, $00, $58, $68, $8D, $3D, $A5, $68, $AA, $68, $20, $86, $A6, $08, $48
  .byte $8A, $48, $A9, $01, $AE, $3D, $A5, $20, $00, $58, $68, $AA, $68, $28, $60
data_7C69:
  .byte $00, $4E, $6F, $20, $65, $6E, $6F, $75, $67, $68, $20, $6C, $6F, $77, $20, $52
  .byte $41, $4D, $20, $74, $6F, $20, $77, $6F, $72, $6B, $2E, $24

sub_7C85:
  tax
  bpl loc_7C8F
  inx
  beq loc_7C92
  clc
  adc #$83
  rts
loc_7C8F:
  lda #$01
  rts
loc_7C92:
  lda data_8006
  rts
loc_7C96:
  stx var_00FF
  ldx #$FF
  txs
  pha
  jsr sub_98B3
  lda #$FF
  ldx #$FF
  jsr BIOS_ENTRY
  sta data_A7F5
  stx data_A7F6
  pla
  jsr sub_7C85
  sta data_8004
  ldx data_A7F5
  cpx #$40
  bne loc_7CC6
  ldx data_A7F6
  cpx #$62
  bcc loc_7CC6
  lda var_00FF
  jsr sub_7C85
loc_7CC6:
  sta data_8003
  ldx #$7C
  ldy #$03
  jsr sub_B3B6
  lda #$07
  jsr BIOS_ENTRY
  tax
  stx data_A53A
  stx data_7C69
  stx data_8007
  stx data_7C3C
  stx data_A53D
  stx data_A824
  inx
  stx data_A7D9
  lda #$02
  jsr BIOS_ENTRY
  sta var_0080
  lda data_A53A
  sta var_0083
  ldx #$80
  ldy #$00
  stx var_0082
  sty var_0081
  stx var_0085
  sty var_0084
  ldx #$40
  ldy #$00
  stx var_0087
  sty var_0086
  lda #$0C
  ldx #$80
  jsr BIOS_ENTRY
  lda #$01
  ldx data_A53A
  jsr BIOS_ENTRY
  lda #$0F
  ldx #$00
  ldy #$19
  jsr BIOS_ENTRY
  cmp #$00
  bne loc_7D61
  lda #$4C
  sta FILE_ENTRY
  stx var_5805
  sty var_5804
  stx var_00FF
  sty var_00FE
  stx data_A53C
  sty data_A53B
  tya
  clc
  adc #$18
  sta data_7C2F
  txa
  adc #$00
  sta data_7C30
  ldy #$18
loc_7D4C:
  lda data_7C24,y
  sta (var_00FE),y
  dey
  bpl loc_7D4C
  ldx #$00
  ldy #$2D
  lda #$0F
  jsr BIOS_ENTRY
  cmp #$00
  beq loc_7D68
loc_7D61:
  ldx #$7C
  ldy #$6A
  jmp loc_B5E0
loc_7D68:
  lda #$4C
  sta DOS_ENTRY
  stx var_580E
  sty var_580D
  stx var_0081
  sty var_0080
  tya
  clc
  adc #$2C
  sta data_7C49
  txa
  adc #$00
  sta data_7C4A
  ldy #$2C
loc_7D86:
  lda data_7C3D,y
  sta (var_0080),y
  dey
  bpl loc_7D86
  lda #$01
  ldx #$FF
  jsr BIOS_ENTRY
  ldx #$58
  ldy #$06
  stx var_BFFD
  sty var_BFFC
  lda #$01
  ldx data_7C69
  jsr BIOS_ENTRY
  lda data_A7F5
  cmp #$40
  bcc loc_7DB3
  lda #$FF
  sta data_A53E
loc_7DB3:
  lda #$60
  ldx #$B2
  ldy #$5C
  sta CTRL_C_ENTRY
  stx var_5817
  sty var_5816
  lda #$A0
  sta CRITICAL_ERROR_ENTRY
  lda #$03
  sta var_5819
  lda #$60
  sta var_581A
  lda #$4C
  ldx #$98
  ldy #$63
  sta var_581B
  stx var_581D
  sty var_581C
  lda data_8003
  sta var_0401
  lda #$01
  ldx #$7C
  ldy #$19
  jsr sub_B4A4
  bcs loc_7DF4
  jmp loc_B536
loc_7DF4:
  jmp loc_B5AE
  .res 521, $FF
