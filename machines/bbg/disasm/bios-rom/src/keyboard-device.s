; 模块：keyboard-device
; 键盘与外设调用（BIOS 功能 24）实现

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_31EF"
; 段 1，$F418-$FA5B
sub_F418:
  lda #$05
  sta var_4016
  ldx #$00
loc_F41F:
  lda #$04
  sta var_4016
  lda var_4017
  lsr a
  and #$0F
  ldy #$06
  sty var_4016
  sta var_5C42
  lda var_4017
  asl a
  asl a
  asl a
  and #$F0
  ora var_5C42
  eor #$FF
  sta var_5C4D,x
  inx
  cpx #$09
  bcc loc_F41F
  bne loc_F453
  lda #$04
  sta var_4016
  lda #$06
  sta var_4016
loc_F453:
  cpx #$0C
  bne loc_F41F
  ldx #$00
loc_F459:
  lda var_5C4D,x
  cmp var_5C59,x
  bne loc_F469
  inx
  cpx #$0C
  bne loc_F459
  txa
  sec
  rts
loc_F469:
  eor var_5C59,x
  sta var_5C41
  txa
  asl a
  asl a
  asl a
  tay
  lda #$00
  sec
loc_F477:
  iny
  rol a
  bit var_5C41
  beq loc_F477
  dey
  rts

sub_F480:
  jsr sub_F418
  bcs loc_F4B6
  pha
  eor var_5C59,x
  sta var_5C59,x
  pla
  and var_5C59,x
  tax
  lda #$00
  sta var_5C7C
  lda data_F3B8,y
  bpl loc_F4A7
  tay
  lda var_5FEF
  ora #$12
  sta var_5FEF
  tya
  and #$7F
loc_F4A7:
  dex
  bpl loc_F4AC
  ora #$80
loc_F4AC:
  sta var_5C42
  jsr sub_F9C6
  bcc loc_F4B6
  ldy #$00
loc_F4B6:
  rts
  .byte $38, $60

sub_F4B9:
  cpx #$37
  bne loc_F4C6
  lda #$03
  bit var_5FED
  beq loc_F4C6
  clc
  rts
loc_F4C6:
  sec
  rts
loc_F4C8:
  sty data_FF40
  lda #$17
  ldx #$00
loc_F4CF:
  bit data_FF48
  bmi loc_F4E2
  dey
  bne loc_F4CF
  dex
  bne loc_F4CF
  sec
  sbc #$01
  bne loc_F4CF
  sec
  bcs loc_F500
loc_F4E2:
  lda #$0D
  sta data_FF50
  pha
  pla
  bne loc_F4F8
loc_F4EB:
  lda #$08
  sta data_FF50
  ldy #$0A
loc_F4F2:
  dex
  bne loc_F4F2
  dey
  bne loc_F4F2
loc_F4F8:
  lda #$0C
  sta data_FF50
  pha
  pla
loc_F4FF:
  clc
loc_F500:
  lda data_FF48
  and #$F8
  bcc loc_F509
  ora #$01
loc_F509:
  eor #$48
  ldx #$80
  rts

sub_F50E:
  asl a
  bcc loc_F529
  pha
  lda var_5F00,x
  clc
  adc var_5C77
  bcs loc_F520
  cmp var_5C47,y
  bcc loc_F525
loc_F520:
  lda var_5C47,y
  sbc #$01
loc_F525:
  sta var_5F00,x
  pla
loc_F529:
  asl a
  bcc loc_F541
  pha
  lda var_5F00,x
  sbc var_5C77
  bcc loc_F53A
  cmp var_5C6D,y
  bcs loc_F53D
loc_F53A:
  lda var_5C6D,y
loc_F53D:
  sta var_5F00,x
  pla
loc_F541:
  rts

sub_F542:
  ldx #$00
  jsr sub_F549
  ldx #$01

sub_F549:
  jsr sub_DBE2
  sta var_5C41
  eor var_5FFC,x
  tay
  and var_5FFC,x
  pha
  lda var_5C41
  sta var_5FFC,x
  tya
  and var_5FFC,x
  beq sub_F5B2
  stx var_5C42
  cpx #$00
  bne loc_F579
  ldx #$00
loc_F56C:
  lsr a
  bcc loc_F572
  inc var_5C65,x
loc_F572:
  inx
  cpx #$04
  bne loc_F56C
  beq loc_F586
loc_F579:
  ldx #$00
loc_F57B:
  lsr a
  bcc loc_F581
  inc var_5C69,x
loc_F581:
  inx
  cpx #$04
  bne loc_F57B
loc_F586:
  ldx var_5C42
  asl a
  beq sub_F5B2
  asl a
  asl a
  asl a
  tay
  eor var_5FFC,x
  and #$F0
  bne loc_F59C
  lda #$06
  sta var_5C75,x
loc_F59C:
  tya
  and var_5C71,x
  beq loc_F5AA
  asl var_5C4B,x
  bcc loc_F5AA
  ror var_5C4B,x
loc_F5AA:
  lda #$00
  sta var_5C71,x
  sta var_5C73,x

sub_F5B2:
  pla
  and #$F0
  beq sub_F5C1
  sta var_5C71,x
  lda #$05
  sta var_5C73,x
  bne sub_F5DC

sub_F5C1:
  lda var_5C73,x
  beq sub_F5DC
  dec var_5C73,x
  bne sub_F5DC
  lda var_5C41
  and #$F0
  bne loc_F5D7
  lda #$01
  sta var_5C4B,x
loc_F5D7:
  lda #$00
  sta var_5C71,x

sub_F5DC:
  lda var_5C75,x
  beq sub_F5EE
  ldy var_5C4B,x
  dey
  bne sub_F5EE
  dec var_5C75,x
  cmp #$06
  bne loc_F64E

sub_F5EE:
  lda var_5C41
  and #$F0
  beq loc_F64E
  ldy var_5C4B,x
  sty var_5C77
  dex
  beq loc_F60C
  ldx #$F2
  ldy #$00
  jsr sub_F50E
  ldx #$F3
  ldy #$02
  jmp sub_F50E
loc_F60C:
  ldx #$EB
  ldy #$01
  jsr sub_F50E
  ldx #$EC
  ldy #$03
  jmp sub_F50E

sub_F61A:
  ldy var_5C79
  sta var_5C00,y
  iny
  txa
  sta var_5C00,y
  iny
  cpy #$40
  bne loc_F62C
  ldy #$00
loc_F62C:
  cpy var_5C78
  bne loc_F64B
  lda #$01
  sta var_4015
  lda #$8F
  sta var_4000
  lda #$00
  sta var_4001
  lda #$50
  sta var_4002
  lda #$80
  sta var_4003
  rts
loc_F64B:
  sty var_5C79
loc_F64E:
  rts

sub_F64F:
  cli
  txa
  pha
  tya
  pha
  jsr sub_F480
  bcs loc_F669
  sta var_5C43
  stx var_5C44
  bit var_5C42
  bmi sub_F684
  inc var_5C7C
  bne sub_F684
loc_F669:
  beq loc_F692
  lda var_5C7C
  beq loc_F692
  cmp var_5C45
  bcc loc_F68A
  ldx var_5C7B
  inx
  cpx var_5C46
  bcc loc_F68F
  lda var_5C43
  ldx var_5C44

sub_F684:
  jsr sub_F61A
  jmp loc_F68D
loc_F68A:
  inc var_5C7C
loc_F68D:
  ldx #$00
loc_F68F:
  stx var_5C7B
loc_F692:
  jsr sub_F542
  pla
  tay
  pla
  tax
  rts
data_F69A:
data_F69B = * + 1
  .word sub_F6D0 - 1, loc_F6FA - 1, loc_F71E - 1, loc_F730 - 1
  .word loc_FF0F - 1, loc_F74B - 1, loc_F784 - 1, sub_F78C - 1
  .word loc_F4C8 - 1, loc_F4EB - 1, loc_F4FF - 1, loc_F7BF - 1
  .word loc_F7C0 - 1, loc_F7F5 - 1, loc_F83F - 1, loc_F855 - 1
  .word sub_F86E - 1, loc_F880 - 1, sub_F8F0 - 1

bios_device:
  txa
  asl a
  cmp #$26
  bcs loc_F6CF
  tax
  lda data_F69B,x
  pha
  lda data_F69A,x
  pha
loc_F6CF:
  rts

sub_F6D0:
  ldy var_5C78
  cpy var_5C79
  bne loc_F6E8
  bit var_5C40
  bvc sub_F6D0
  jsr sub_F480
  bcs sub_F6D0
  jsr sub_F61A
  jmp sub_F6D0
loc_F6E8:
  lda var_5C00,y
  iny
  ldx var_5C00,y
  iny
  cpy #$40
  bcc loc_F6F6
  ldy #$00
loc_F6F6:
  sty var_5C78
  rts
loc_F6FA:
  ldy var_5C78
  cpy var_5C79
  beq loc_F70B
  lda var_5C00,y
  ldx var_5C01,y
  ldy #$01
  rts
loc_F70B:
  bit var_5C40
  bvc loc_F71B
  jsr sub_F480
  bcs loc_F71B
  jsr sub_F61A
  jmp loc_F6FA
loc_F71B:
  ldy #$00
  rts
loc_F71E:
  lda var_5FED
  ldx var_5FEE
  ldy var_5FEF
  rts
data_F728:
  .byte $02, $03, $04, $05, $08, $0B, $10, $16
loc_F730:
  tya
  and #$60
  asl a
  rol a
  rol a
  rol a
  tax
  lda data_FFF1,x
  sta var_5C45
  tya
  and #$1C
  lsr a
  lsr a
  tax
  lda data_F728,x
  sta var_5C46
  rts
loc_F74B:
  ldx var_5C78
  dex
  dex
  bpl loc_F754
  ldx #$3E
loc_F754:
  lda var_6000
  pha
  and #$7F
  sta var_2000
  sta var_6000
  cpx var_5C79
  bne loc_F770
  txa
  sec
  sbc #$02
  bpl loc_F76D
  lda #$3E
loc_F76D:
  sta var_5C79
loc_F770:
  stx var_5C78
  pla
  sta var_6000
  sta var_2000
  tya
  sta var_5C00,x
  lda var_00FF
  sta var_5C01,x
  rts
loc_F784:
  sty var_5C40
  ldx #$59
  ldy #$FD
  rts

sub_F78C:
  lda #$00
  sta var_5FED
  sta var_5FEE
  sta var_5FEF
  sta var_5FF0
  ldx #$04
loc_F79C:
  sta var_5C78,x
  dex
  bpl loc_F79C
  ldx #$0B
loc_F7A4:
  lda var_5C4D,x
  sta var_5C59,x
  dex
  bpl loc_F7A4
  ldx #$1A
  stx var_5C45
  ldx #$03
  stx var_5C46
  lda #$00
  jsr sub_FF09
  lda #$00
  rts
loc_F7BF:
  rts
loc_F7C0:
  tya
  and #$01
  tay
  lda #$00
  sta var_5FFC,y
  sta var_5C6D,y
  sta var_5C6F,y
  lda #$F6
  sta var_5C47,y
  lda #$EF
  sta var_5C49,y
  lsr a
  dey
  beq loc_F7E9
  sta var_5FF3
  lda #$52
  sta var_5FF2
  lda #$00
  beq loc_F80C
loc_F7E9:
  sta var_5FEC
  lda #$A4
  sta var_5FEB
  lda #$00
  beq loc_F82E
loc_F7F5:
  tya
  ldx #$03
  lsr a
  bcs loc_F81D
loc_F7FB:
  lda var_5C65,x
  sta var_00FC,x
  dex
  bpl loc_F7FB
  lda var_5FFC
  ldx var_5FF2
  ldy var_5FF3
loc_F80C:
  pha
  lda #$00
  sta var_5C65
  sta var_5C66
  sta var_5C67
  sta var_5C68
  pla
  rts
loc_F81D:
  lda var_5C69,x
  sta var_00FC,x
  dex
  bpl loc_F81D
  lda var_5FFD
  ldx var_5FEB
  ldy var_5FEC
loc_F82E:
  pha
  lda #$00
  sta var_5C69
  sta var_5C6A
  sta var_5C6B
  sta var_5C6C
  pla
  rts
loc_F83F:
  tya
  ldx var_00FE
  ldy var_00FF
  lsr a
  bcs loc_F84E
  stx var_5FF2
  sty var_5FF3
  rts
loc_F84E:
  stx var_5FEB
  sty var_5FEC
  rts
loc_F855:
  tya
  and #$01
  tay
  lda var_00FC
  sta var_5C6D,y
  lda var_00FD
  sta var_5C6F,y
  lda var_00FE
  sta var_5C47,y
  lda var_00FF
  sta var_5C49,y
  rts

sub_F86E:
  lda #$FF
  sta data_FF10
  pha
  pla
  jsr sub_F8F0
  pha
  pla
  lda #$FF
  sta data_FF10
  rts
loc_F880:
  tya
  pha
  jsr sub_F86E
  jsr bios_get_page
  sta var_5FB2
  pla
  tax
  jsr sub_D316
  ldy #$00
  lda (var_00FE),y
  sta var_5FB6
  iny
  lda (var_00FE),y
  sta var_5FB5
  iny
loc_F89E:
  lda data_FF18
  bpl loc_F89E
  cmp #$FF
  beq loc_F89E
  lda var_4CFD
  cmp #$FF
  beq loc_F8B9
  tya
  pha
  jsr sub_59D6
  pla
  cpy #$00
  bne loc_F8DB
  tay
loc_F8B9:
  lda (var_00FE),y
  sta data_FF18
  iny
  bne loc_F8C3
  inc var_00FF
loc_F8C3:
  sec
  lda var_5FB5
  sbc #$01
  sta var_5FB5
  bcs loc_F8D1
  dec var_5FB6
loc_F8D1:
  lda var_5FB5
  ora var_5FB6
  bne loc_F89E
  beq loc_F8EA
loc_F8DB:
  ldx #$F0
loc_F8DD:
  lda data_FF18
  bpl loc_F8DD
  lda #$00
  sta data_FF18
  inx
  bne loc_F8DD
loc_F8EA:
  ldx var_5FB2
  jmp sub_D316

sub_F8F0:
  lda #$00
  sta data_FF10
  rts

sub_F8F6:
  ldx #$40
  cmp #$3A
  beq loc_F906
  ldx #$20
  cmp #$45
  beq loc_F906
  ldx #$10
  cmp #$46
loc_F906:
  rts

sub_F907:
  lda #$08
  bit var_5FEF
  beq loc_F91F
  cpx #$45
  beq loc_F91A
  lda var_5FEF
  and #$F7
  sta var_5FEF
loc_F91A:
  pla
  pla
  jmp loc_FC4D
loc_F91F:
  rts

sub_F920:
  txa
  jsr sub_F8F6
  bne loc_F93F
  txa
  bit var_5FEE
  bne loc_F91A
  ora var_5FEE
  sta var_5FEE
  txa
  eor var_5FED
  sta var_5FED
  jsr sub_F98F
  jmp loc_F91A
loc_F93F:
  tax
  rts

sub_F941:
  lda loc_FD2A+2,x
  cmp #$FF
  beq loc_F91A
  rts

sub_F949:
  cpx #$52
  bne loc_F962
  lda #$80
  bit var_5FEE
  bne loc_F91A
  ora var_5FEE
  sta var_5FEE
  lda #$80
  eor var_5FED
  sta var_5FED
loc_F962:
  rts

sub_F963:
  ldx #$08
  cmp #$38
  beq loc_F979
  ldx #$04
  cmp #$1D
  beq loc_F979

sub_F96F:
  ldx #$02
  cmp #$2A
  beq loc_F979
  ldx #$01
  cmp #$36
loc_F979:
  rts

sub_F97A:
  lda #$10
  bit var_5FEF
  rts

sub_F980:
  pha
  lda var_5FEF
  lsr a
  pla
  rts

sub_F987:
  pha
  lda var_5FEF
  lsr a
  lsr a
  pla
  rts

sub_F98F:
  lda #$40
  bit var_5FF0
  bne loc_F9C5
  ora var_5FF0
  sta var_5FF0
  lda var_5FED
  lsr a
  lsr a
  lsr a
  lsr a
  and #$07
  tax
  eor var_5FF0
  and #$07
  lda var_5FF0
  and #$F8
  stx var_5FF0
  ora var_5FF0
  sta var_5FF0
  txa
  jsr sub_FF09
  lda #$3F
  and var_5FF0
  sta var_5FF0
loc_F9C5:
  rts

sub_F9C6:
  tax
  bmi loc_F9CC
  jmp loc_FB0C
loc_F9CC:
  and #$7F
  jsr sub_F980
  bcc loc_F9DB
  jsr sub_F963
  bne loc_F9F9
  jmp loc_FC55
loc_F9DB:
  jsr sub_F987
  bcc loc_F9E5
  jsr sub_F96F
  beq loc_F9F9
loc_F9E5:
  jsr sub_F8F6
  beq loc_F9F0
  ldx #$80
  cmp #$52
  bne loc_F9FC
loc_F9F0:
  txa
  eor #$FF
  and var_5FEE
  sta var_5FEE
loc_F9F9:
  jmp loc_FC4D
loc_F9FC:
  cmp #$54
  bne loc_FA0B
  lda var_5FEF
  and #$FB
  sta var_5FEF
  jmp loc_FC55
loc_FA0B:
  jsr sub_F963
  bne loc_F9F9
  pha
  txa
  eor #$FF
  tay
  and var_5FED
  sta var_5FED
  pla
  jsr sub_F96F
  beq loc_F9F9
  tax
  jsr sub_F987
  bcc loc_FA30
  tya
  and var_5FEE
  sta var_5FEE
  bcs loc_FA3B
loc_FA30:
  tya
  lsr a
  lsr a
  ora #$C0
  and var_5FEE
  sta var_5FEE
loc_FA3B:
  lda var_5FEE
  asl a
  asl a
  ora var_5FEE
  and #$0C
  ora var_5FED
  sta var_5FED
  cpx #$38
  bne loc_F9F9
  lda var_5C7A
  beq loc_F9F9
  ldx #$00
  stx var_5C7A
loc_FA59:
  jmp loc_FC3B
