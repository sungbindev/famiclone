; 模块：startup-config
; 启动流程：解析 CONFIG.SYS、装入 DEVICE 与 SHELL、启动命令解释器与错误提示

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_37B6"
; 段 0，$B3B6-$B5E5
sub_B3B6:
  stx var_00FD
  sty var_00FC
  lda #$00
loc_B3BC:
  tay
  lda (var_00FC),y
  cmp #$24
  beq loc_B3CD
  tax
  iny
  tya
  pha
  jsr sub_B3DC
  pla
  bne loc_B3BC
loc_B3CD:
  rts

sub_B3CE:
  jsr sub_B3B6

sub_B3D1:
  ldx #$0D
  lda #$0A
  clc
  pha
  jsr sub_B3DC
  pla
  tax

sub_B3DC:
  lda #$1B
  ldy #$00
  jmp BIOS_ENTRY

sub_B3E3:
  pha
  lsr a
  lsr a
  lsr a
  lsr a
  jsr sub_B3EE
  pla
  and #$0F

sub_B3EE:
  clc
  adc #$30
  sta data_B2DC,x
  inx
  rts
loc_B3F6:
  ldx #$03
loc_B3F8:
  lda data_B2BF,x
  sta data_B2DC,x
  dex
  bpl loc_B3F8
  ldx #$B2
  ldy #$C3
  jmp sub_B3CE

sub_B408:
  lda var_0401
  sta var_00FF
  ldx #$61
  ldy #$82
  jsr dos_run_path_program
  bcc loc_B420
  jsr loc_B3F6
  ldx #$B2
  ldy #$9A
  jmp sub_B3CE
loc_B420:
  rts
  .byte $20, $64, $B3, $8D, $01, $04, $20, $89, $B3, $20, $74, $B1, $B0, $07, $E0, $03
  .byte $D0, $03, $4C, $08, $B4, $A2, $B2, $A0, $7C, $20, $CE, $B3, $4C, $F6, $B3, $20
  .byte $64, $B3, $8D, $01, $04, $20, $89, $B3, $4C, $08, $B4

sub_B44C:
  lda var_6132,y
  cmp #$20
  beq loc_B457
  cmp #$09
  bne loc_B45A
loc_B457:
  iny
  bne sub_B44C
loc_B45A:
  rts

sub_B45B:
  ldy #$00
  jsr sub_B44C
  cmp #$0D
  beq loc_B490
  ldx #$00
loc_B466:
  tya
  pha
loc_B468:
  lda var_6132,y
  and #$DF
  cmp data_B2AB,x
  bne loc_B491
  iny
  inx
  lda data_B2AB,x
  bne loc_B468
  jsr sub_B44C
  cmp #$3D
  bne loc_B497
  iny
  pla
  jsr sub_B44C
  sty data_B2E3
  lda data_B2AD,x
  pha
  lda data_B2AC,x
  pha
loc_B490:
  rts
loc_B491:
  inx
  lda data_B2AB,x
  bne loc_B491
loc_B497:
  pla
  tay
  inx
  inx
  inx
  lda data_B2AB,x
  bne loc_B466
  jmp loc_B3F6

sub_B4A4:
  stx var_0404
  sty var_0403

sub_B4AA:
  sta var_0400
  ldx #$04
  ldy #$00
  jmp FILE_ENTRY

sub_B4B4:
  cpx data_B2BD
  bcc loc_B4E3
  tya
  pha
  lda data_A53A
  sta var_0402
  ldx #$00
  ldy #$80
  stx var_0406
  sty var_0405
  lda #$02
  ldx #$B2
  ldy #$E4
  jsr sub_B4A4
  bcs loc_B4DF
  lda var_0405
  sta data_B2BD
  bne loc_B4DF
  sec
loc_B4DF:
  pla
  tay
  ldx #$00
loc_B4E3:
  rts
  .byte $07, $0A, $4D, $69, $73, $73, $69, $6E, $67, $20, $44, $4F, $53, $20, $73, $68
  .byte $65, $6C, $6C, $20, $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D, $44, $0D
  .byte $0A, $42, $42, $47, $44, $4F, $53, $20, $62, $6F, $6F, $74, $20, $66, $61, $69
  .byte $6C, $75, $72, $65, $21, $20, $48, $61, $6C, $74, $65, $64, $20, $6E, $6F, $77
  .byte $0D, $0A, $24, $43, $4F, $4D, $4D, $41, $4E, $44, $20, $43, $4D, $44
data_B532:
  .byte $02, $2F, $50, $0D
loc_B536:
  ldx data_B2BE
  ldy #$00
loc_B53B:
  jsr sub_B4B4
  bcs loc_B59E
  lda data_B2E4,x
  cmp #$1A
  beq loc_B59E
  inx
  cmp #$0A
  beq loc_B57A
  cmp #$0D
  beq loc_B56D
  sta var_6132,y
  iny
  cpy #$4F
  bcc loc_B53B
loc_B558:
  jsr sub_B4B4
  bcs loc_B59E
  lda data_B2E4,x
  cmp #$1A
  beq loc_B59E
  inx
  cmp #$0A
  beq loc_B57A
  cmp #$0D
  bne loc_B558
loc_B56D:
  jsr sub_B4B4
  bcs loc_B59E
  lda data_B2E4,x
  cmp #$0A
  bne loc_B57A
  inx
loc_B57A:
  lda #$0D
  sta var_6132,y
  stx data_B2BE
  jsr sub_B45B
  ldx #$03
  sec
loc_B588:
  lda data_B2BF,x
  adc #$00
  sta data_B2BF,x
  cmp #$39
  bcc loc_B536
  lda #$30
  sta data_B2BF,x
  dex
  bpl loc_B588
  bmi loc_B536
loc_B59E:
  tya
  beq loc_B5A9
  lda #$0D
  sta var_6132,y
  jsr sub_B45B
loc_B5A9:
  lda #$05
  jsr sub_B4AA
loc_B5AE:
  lda #$4C
  ldx #$9B
  ldy #$29
  sta CRITICAL_ERROR_ENTRY
  stx var_581A
  sty var_5819
  sta CTRL_C_ENTRY
  ldx #$03
loc_B5C2:
  lda data_B532,x
  sta var_0100,x
  dex
  bpl loc_B5C2
  lda #$40
  sta data_8030
  lda data_8004
  sta var_00FF
  ldx #$B5
  ldy #$27
  jsr dos_run_program
  ldx #$B4
  ldy #$E4
loc_B5E0:
  jsr sub_B3CE
loc_B5E3:
  jmp loc_B5E3
