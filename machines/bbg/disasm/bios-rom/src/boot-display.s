; 模块：boot-display
; 开机显示用的例程（未确认：边界）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_005E"
; 段 1，$C287-$C3E5
sub_C287:
  lda #$80
  sta var_4348
  jsr sub_D09C
loc_C28F:
  lda #$A0
  sta var_4340
loc_C294:
  ldx #$14
loc_C296:
  lda #$19
  sta var_5FF8
loc_C29B:
  lda var_5FF8
  beq loc_C2D9
  bit var_4348
  bpl loc_C29B
  lda #$00
  sta var_4348
  lda var_4340
  tay
loc_C2AE:
  lda var_5FF8
  beq loc_C2D9
  bit var_4348
  bmi loc_C2AE
  lda #$80
  sta var_4348
  tya
  cmp #$AA
  bne loc_C294
  dex
  bne loc_C296
  lda #$55
  jsr sub_D146
  jsr bios_function_36
  cmp #$55
  bne loc_C28F
  lda #$5A
  jsr sub_D146
  lda #$01
  rts
loc_C2D9:
  lda #$80
  sta var_4348
  lda #$00
  rts
data_C2E1:
  .byte $0E, $00, $12, $20

sub_C2E5:
  ldx #$3F
  jsr sub_E9F9
loc_C2EA:
  lda data_C2E1,y
  sta var_2007
  iny
  cpy #$04
  bne loc_C2EA
  lda #$00
  sta var_2006
  sta var_2006
  rts

sub_C2FE:
  bit var_2002
  bmi sub_C2FE
loc_C303:
  bit var_2002
  bpl loc_C303
  rts

sub_C309:
  stx var_00FF
  sta var_00FE
  lda var_00FC
  asl a
  asl a
  asl a
  asl a
  php
  asl a
  ora var_00FE
  sty var_00FE
  tay
  lda #$10
  ror a
  plp
  rol a
  rol a
  tax
  rts

sub_C322:
  jsr sub_C309
  pha
  tya
  pha
  and #$E0
  tay
  jsr sub_D093
  jsr sub_C2FE
  jsr sub_E9FB
  ldy #$40
  lda #$6A
loc_C338:
  sta var_2007
  dey
  bne loc_C338
  pla
  pha
  tay
  jmp loc_C34D

sub_C344:
  jsr sub_C309
  pha
  tya
  pha
  jsr sub_C2FE
loc_C34D:
  jsr sub_E9FB
  ldy #$00
loc_C352:
  lda (var_00FE),y
  iny
  cmp #$FF
  beq loc_C35E
  sta var_2007
  bne loc_C352
loc_C35E:
  pla
  clc
  adc #$20
  tay
  pla
  adc #$00
  tax
  jsr sub_E9FB
  ldy #$00
loc_C36C:
  lda (var_00FE),y
  iny
  cmp #$FF
  beq sub_C37A
  adc #$01
  sta var_2007
  bne loc_C36C

sub_C37A:
  lda #$00
  sta var_2005
  sta var_2005
  lda #$0E
  sta var_2001
  rts

sub_C388:
  sty var_00FF
  sta var_00FE
  txa
  ldy #$08
loc_C38F:
  rol var_00FF
  rol a
  cmp var_00FE
  bcc loc_C398
  sbc var_00FE
loc_C398:
  dey
  bne loc_C38F
  tax
  lda var_00FF
  rol a
  rts

sub_C3A0:
  ldx #$00
  lda #$0A
  jsr sub_C388
  bne loc_C3AB
  bcs loc_C3B3
loc_C3AB:
  ldy var_0080
  asl a
  sta a:var_0081,y
  inc var_0080
loc_C3B3:
  ldy var_0080
  txa
  asl a
  sta a:var_0081,y
  inc var_0080
  rts

sub_C3BD:
  pha
  lda #$64
  sec
  jsr sub_C388
  beq loc_C3CF
  tay
  txa
  pha
  jsr sub_C3A0
  pla
  bcc loc_C3D0
loc_C3CF:
  txa
loc_C3D0:
  tay
  jsr sub_C3A0
  ldx var_0080
  pla
  sta var_0081,x
  lda #$FF
  sta var_0082,x
  ldx #$00
  ldy #$81
  lda var_00FD
  jmp sub_C344
