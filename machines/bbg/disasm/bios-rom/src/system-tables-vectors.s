; 模块：system-tables-vectors
; 键盘字符转换表与系统区向量（未确认：边界）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_3833"
; 段 1，$FA5C-$FFFF
  .byte $1B, $21, $40, $23, $24, $25, $5E, $26, $2A, $28, $29, $5F, $2B, $08, $00, $51
  .byte $57, $45, $52, $54, $59, $55, $49, $4F, $50, $7B, $7D, $0D, $FF, $41, $53, $44
  .byte $46, $47, $48, $4A, $4B, $4C, $3A, $22, $7E, $FF, $7C, $5A, $58, $43, $56, $42
  .byte $4E, $4D, $3C, $3E, $3F, $FF, $2A, $FF, $20, $FF, $54, $55, $56, $57, $58, $59
  .byte $5A, $5B, $5C, $5D, $FF, $FF, $37, $38, $39, $2D, $34, $35, $36, $2B, $31, $32
  .byte $33, $30, $2E, $FF, $03, $7C, $87
data_FAB3:
  .byte $88, $1B, $31, $32, $33, $34, $35, $36, $37, $38, $39, $30, $2D, $3D, $08, $09
  .byte $71, $77, $65, $72, $74, $79, $75, $69, $6F, $70, $5B, $5D, $0D, $FF, $61, $73
  .byte $64, $66, $67, $68, $6A, $6B, $6C, $3B, $27, $60, $FF, $5C, $7A, $78, $63, $76
  .byte $62, $6E, $6D, $2C, $2E, $2F, $FF, $2A, $FF, $20, $FF, $3B, $3C, $3D, $3E, $3F
  .byte $40, $41, $42, $43, $44, $FF, $FF, $47, $48, $49, $2D, $4B, $F0, $4D, $2B, $4F
  .byte $50, $51, $52, $53, $FF, $03, $5C, $85, $86
loc_FB0C:
  jsr sub_F987
  bcc loc_FB19
  jsr sub_F96F
  bne loc_FB19
loc_FB16:
  jmp loc_FC4D
loc_FB19:
  jsr sub_F963
  bne loc_FB45
  pha
  txa
  tay
  ora var_5FED
  sta var_5FED
  pla
  jsr sub_F96F
  beq loc_FB16
  tya
  jsr sub_F987
  bcc loc_FB3B
  ora var_5FEE
  sta var_5FEE
  bne loc_FB16
loc_FB3B:
  lsr a
  lsr a
  ora var_5FEE
  sta var_5FEE
  bne loc_FB16
loc_FB45:
  tax
  beq loc_FB16
  cmp #$59
  bcs loc_FB16
  cmp #$54
  bne loc_FB60
  lda #$04
  bit var_5FEF
  bne loc_FB16
  ora var_5FEF
  sta var_5FEF
  jmp loc_FC55
loc_FB60:
  lda #$08
  bit var_5FED
  beq loc_FB76
  jsr sub_F97A
  beq loc_FB73
  lda #$04
  bit var_5FEF
  bne loc_FB76
loc_FB73:
  jmp loc_FD85
loc_FB76:
  lda #$04
  bit var_5FED
  beq loc_FB80
  jmp loc_FCAF
loc_FB80:
  cpx #$55
  bne loc_FB8E
  lda #$08
  bit var_5FEF
  bne loc_FB16
  jmp loc_FCBD
loc_FB8E:
  jsr sub_F920
  cpx #$52
  beq loc_FB98
  jsr sub_F907
loc_FB98:
  jsr sub_F4B9
  bcs loc_FBA0
  jmp loc_FC4D
loc_FBA0:
  jsr sub_F987
  bcs loc_FBAE
  lda #$03
  bit var_5FED
  beq loc_FBD5
  bne loc_FBC1
loc_FBAE:
  cpx #$1C
  bne loc_FBB6
  lda #$0D
  bne loc_FBBC
loc_FBB6:
  cpx #$35
  bne loc_FBEA
  lda #$2F
loc_FBBC:
  ldx #$E0
  jmp loc_FC3B
loc_FBC1:
  lda #$20
  bit var_5FED
  beq loc_FBE4
  jsr sub_F949
  cpx #$47
  bcc loc_FBE4
  cpx #$56
  bcs loc_FBE4
  bcc loc_FBED
loc_FBD5:
  lda #$20
  bit var_5FED
  beq loc_FBEA
  cpx #$47
  bcc loc_FBED
  cpx #$56
  bcs loc_FBED
loc_FBE4:
  lda loc_FA59+2,x
  jmp loc_FBF0
loc_FBEA:
  jsr sub_F949
loc_FBED:
  lda data_FAB3,x
loc_FBF0:
  cmp #$FF
  bne loc_FBF7
  jmp loc_FC4D
loc_FBF7:
  cpx #$3B
  bcc loc_FC11
  cmp #$3B
  bcc loc_FC11
  cmp #$F0
  beq loc_FC11
  cpx #$56
  beq loc_FC11
  tax
  lda #$00
  jsr sub_F987
  bcc loc_FC11
  lda #$E0
loc_FC11:
  tay
  lda #$40
  bit var_5FED
  beq loc_FC3A
  lda #$03
  bit var_5FED
  bne loc_FC2D
  cpy #$7B
  bcs loc_FC3A
  cpy #$61
  bcc loc_FC3A
  tya
  and #$DF
  bne loc_FC3B
loc_FC2D:
  cpy #$5B
  bcs loc_FC3A
  cpy #$41
  bcc loc_FC3A
  tya
  ora #$20
  bne loc_FC3B
loc_FC3A:
  tya
loc_FC3B:
  tay
  lda var_5FEF
  and #$FC
  sta var_5FEF
  tya
  cmp #$F0
  bne loc_FC4B
  lda #$00
loc_FC4B:
  clc
  rts
loc_FC4D:
  lda var_5FEF
  and #$FC
  sta var_5FEF
loc_FC55:
  sec
loc_FC56:
  rts
  .byte $1B, $FF, $00, $FF, $FF, $FF, $1E, $FF, $FF, $FF, $FF, $1F, $FF, $7F, $94, $11
  .byte $17, $05, $12, $14, $19, $15, $09, $0F, $10, $1B, $1D, $0A, $FF, $01, $13, $04
  .byte $06, $07, $08, $0A, $0B, $0C, $FF, $FF, $FF, $FF, $1C, $1A, $18, $03, $16, $02
  .byte $0E, $0D, $FF, $FF, $95, $FF, $96, $FF, $20, $FF, $5E, $5F, $60, $61, $62, $63
  .byte $64, $65, $66, $67, $45, $46, $77, $8D, $84, $8E, $73, $8F, $74, $90, $75, $91
  .byte $76, $92, $93, $FF, $03, $FF, $89, $8A
loc_FCAF:
  jsr sub_F907
  lda loc_FC56,x
  cmp #$FF
  beq loc_FCD3
  cpx #$45
  bne loc_FCD6
loc_FCBD:
  lda #$08
  ora var_5FEF
  sta var_5FEF
  lda var_5FF8
  pha
  jsr sub_5A19
  pla
  sta var_5FF8
  jmp loc_FC55
loc_FCD3:
  jmp loc_FC4D
loc_FCD6:
  cpx #$55
  beq loc_FCDE
  cpx #$46
  bne loc_FCED
loc_FCDE:
  lda var_5C78
  sta var_5C79
  jsr sub_5A23
  lda #$00
  tax
  jmp loc_FC3B
loc_FCED:
  cpx #$37
  bne loc_FD01
  tax
  jsr sub_F97A
  beq loc_FCFD
  txa
  jsr sub_F987
  bcc loc_FD27
loc_FCFD:
  lda #$72
  bne loc_FD27
loc_FD01:
  jsr sub_F987
  bcs loc_FD16
  cpx #$0F
  beq loc_FD27
  cpx #$39
  beq loc_FD2A
  cpx #$35
  beq loc_FCD3
  bcc loc_FD2A
  bcs loc_FD27
loc_FD16:
  cpx #$35
  beq loc_FD27
  cpx #$1C
  beq loc_FD23
  tax
  lda #$E0
  bne loc_FD2A
loc_FD23:
  ldx #$E0
  bne loc_FD2A
loc_FD27:
  tax
  lda #$00
loc_FD2A:
  jmp loc_FC3B
  .byte $F0, $78, $79, $7A, $7B, $7C, $7D, $7E, $7F, $80, $81, $82, $83, $F0, $A5, $10
  .byte $11, $12, $13, $14, $15, $16, $17, $18, $19, $F0, $F0, $F0, $FF, $1E, $1F, $20
  .byte $21, $22, $23, $24, $25, $26, $F0, $F0, $F0, $FF, $F0, $2C, $2D, $2E, $2F, $30
  .byte $31, $32, $F0, $F0, $F0, $FF, $F0, $FF, $20, $FF, $68, $69, $6A, $6B, $6C, $6D
  .byte $6E, $6F, $70, $71, $FF, $FF, $07, $08, $09, $F0, $04, $05, $06, $F0, $01, $02
  .byte $03, $00, $53, $FF, $03, $FF, $8B, $8C
loc_FD85:
  lda #$04
  bit var_5FED
  beq loc_FD93
  cpx #$53
  bne loc_FD96
  jmp warm_start_entry_jump
loc_FD93:
  jsr sub_F920
loc_FD96:
  jsr sub_F907
  jsr sub_F941
  jsr sub_F987
  bcc loc_FDBC
  ldy #$00
  sty var_5C7A
  cpx #$1C
  bne loc_FDAE
  lda #$A6
  bne loc_FDEC
loc_FDAE:
  cpx #$35
  bne loc_FDB6
  lda #$A4
  bne loc_FDEC
loc_FDB6:
  txa
  clc
  adc #$50
  bne loc_FDEC
loc_FDBC:
  cmp #$0A
  bcs loc_FDDB
  tay
  lda var_5C7A
  asl a
  sta var_5C7A
  asl a
  asl a
  clc
  adc var_5C7A
  sty var_5C7A
  clc
  adc var_5C7A
  sta var_5C7A
loc_FDD8:
  jmp loc_FC4D
loc_FDDB:
  ldy #$00
  sty var_5C7A
  cpx #$53
  beq loc_FDD8
  cpx #$39
  beq loc_FDEF
  cmp #$F0
  beq loc_FDEF
loc_FDEC:
  tax
  lda #$00
loc_FDEF:
  jmp loc_FC3B
  .res 270, $FF
data_FF00:
  .byte $FF

sub_FF01:
  stx loc_599E+2
loc_FF04:
  sty loc_599E+1
  rts
  .byte $FF

sub_FF09:
  eor #$07
loc_FF0B:
  sta data_FF00
  rts
loc_FF0F:
  rts
data_FF10:
  .byte $FF

sub_FF11:
  lda var_6000
  sta var_2000
  rts
data_FF18:
  .byte $FF

bios_function_38:
  stx var_5FC1
  rts
  .byte $FF, $FF, $FF, $FF

bios_function_16:
  jsr sub_E048
loc_FF24:
  jmp bios_function_16
  .byte $FF, $FF

bios_function_8:
  lda var_5FF9
loc_FF2C:
  stx var_5FF9
  rts
  .byte $FF

; BIOS 功能 7 BIOS_GET_DRAM_SIZE：A、X 返回内存页数（最后一页页号加 1）。
bios_get_dram_size:
  ldx var_5FF9
  inx
  txa
  rts
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF40:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF48:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF50:
  .res 48, $FF
data_FF80:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF88:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF90:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF98:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FFA0:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FFA8:
  .res 16, $FF
data_FFB8:
  .res 57, $FF
data_FFF1:
  .byte $0D, $1A, $30, $33, $FF, $FF, $FF, $FF, $FF, $C7, $58
data_FFFC:
  .word reset_handler, irq_handler
