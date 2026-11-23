; 模块：boot-reset
; 复位、硬件检测、拷贝入口桩到 $5800 与从磁盘引导

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_08CA"
; 段 1，$CAF3-$D0A4
sub_CAF3:
  ldx #$00
  jsr sub_E9F9
  ldx #$C3
  ldy #$E6
  stx var_00FF
  sty var_00FE
  lda #$C5
  sta var_00FD
  ldy #$00
loc_CB06:
  ldx #$08
loc_CB08:
  lda (var_00FE),y
  iny
  sta var_2007
  dex
  bne loc_CB08
  lda #$FF
  ldx #$08
loc_CB15:
  sta var_2007
  dex
  bne loc_CB15
  tya
  bne loc_CB20
  inc var_00FF
loc_CB20:
  dec var_00FD
  bne loc_CB06
  ldx #$20
  jsr sub_E9F9
  ldx #$04
  lda #$6A
  ldy #$C0
loc_CB2F:
  sta var_2007
  dey
  bne loc_CB2F
  dex
  bne loc_CB2F
  lda #$00
  ldy #$40
loc_CB3C:
  sta var_2007
  dey
  bne loc_CB3C
  rts
  .byte $A2, $5A, $8E, $00, $80, $EC, $00, $80, $D0, $08, $A2, $A5, $8E, $00, $80, $EC, $00, $80, $60

; 复位入口：清硬件寄存器、检测内存容量、把 $C000-$C228 复制到 $5800、检测软驱，随后显示系统检测画面并从软盘引导（引导扇区读到 $0400 后执行）。
reset_handler:
  lda #$00
  sta var_4CFF
  sta var_2000
  sta var_6000
  sta var_2001
  sta var_4015
  sta var_4CFC
  lda #$01
  sta var_4CFB
  lda #$D6
  sta var_4CFE
  lda #$00
  sta var_4CFD
  sei
  cld
  ldx #$FF
  txs
  stx loc_FF2C
  dex
  stx loc_FF24
  lda #$00
  sta data_FF40
  sta data_FF50
  sta sub_FF01
  lda #$00
loc_CB92:
  sta sub_FF01+2
  ldx #$00
  jsr sub_E9F9
loc_CB9A:
  sta var_2007
  inx
  bne loc_CB9A
  sec
  ror a
  bcc loc_CB92
  lda #$00
loc_CBA6:
  sta sub_FF01+2
  ldx #$00
  jsr sub_E9F9
  bit var_2007
loc_CBB1:
  cmp var_2007
  bne loc_CBBB
  inx
  bne loc_CBB1
  beq loc_CBBF
loc_CBBB:
  sec
  ror a
  bcc loc_CBA6
loc_CBBF:
  eor #$FF
  clc
  adc #$01
  pha
  ldx #$04
  stx sub_FF01+2
  inx
  stx loc_FF0B
  jsr sub_C2FE
  jsr sub_C2E5
  jsr sub_CAF3
  ldx #$21
  ldy #$23
  jsr sub_E9FB
  ldx #$1A
  lda #$90
loc_CBE2:
  sta var_2007
  dex
  bne loc_CBE2
  jsr sub_C37A
  lda #$00
  sta var_00FC
  ldx #$CA
  ldy #$D8
  lda #$01
  jsr sub_C344
  lda #$02
  sta var_00FC
  ldx #$CA
  ldy #$E1
  lda #$01
  jsr sub_C344
  lda #$04
  sta var_00FC
  ldx #$CA
  ldy #$EA
  lda #$01
  jsr sub_C344
  lda #$07
  sta var_00FC
  ldx #$CA
  ldy #$13
  lda #$0C
  jsr sub_C344
  lda #$0A
  sta var_00FC
  lda #$40
  sta data_FFA0
  lda #$20
  sta sub_FF09
  pha
  ldx #$04
loc_CC30:
  lda data_CA1C,x
  sta var_0081,x
  dex
  bpl loc_CC30
  lda #$05
  sta var_00FD
  lda #$00
loc_CC3E:
  sta loc_FF04
  ldx #$00
loc_CC43:
  sta var_9A00,x
  inx
  bne loc_CC43
  sec
  ror a
  bcc loc_CC3E
  lda #$00
loc_CC4F:
  sta loc_FF04
  ldx #$00
loc_CC54:
  cmp var_9A00,x
  bne loc_CC5E
  inx
  bne loc_CC54
  beq loc_CC62
loc_CC5E:
  sec
  ror a
  bcc loc_CC4F
loc_CC62:
  sta var_0002
  pha
  sta var_0003
  lda #$00
  sta data_FFA0
  sta var_6000
  sta var_0000
loc_CC71:
  lda #$05
  sta var_0080
  ldx #$00
  lda var_0002
  sec
  sbc var_0003
  adc #$00
  bcc loc_CC81
  inx
loc_CC81:
  stx var_00FF
  ldx #$04
loc_CC85:
  asl a
  rol var_00FF
  dex
  bne loc_CC85
  ldx var_00FF
  tay
  lda #$1C
  jsr sub_C3BD
  lda #$80
  sta var_0001
  lda var_0002
  sta loc_FF04
  ldx #$18
  ldy #$00
loc_CCA0:
  sta (var_0000),y
  iny
  bne loc_CCA0
  inc var_0001
  dex
  bne loc_CCA0
  inc var_0002
  bne loc_CC71
  lda #$08
  sta data_FF90
  ldx #$58
  ldy #$00
  stx var_0001
  sty var_0000
  ldx #$08
  tya
loc_CCBE:
  sta (var_0000),y
  iny
  bne loc_CCBE
  inc var_0001
  dex
  bne loc_CCBE
  ldx #$C2
  stx var_0003
  sty var_0002
  ldx #$5A
  stx var_0001
  sty var_0000
  stx var_5FBA
  ldx #$02
  ldy #$29
  sty var_5FB9
loc_CCDE:
  tya
  bne loc_CCE8
  dex
  bmi loc_CCF0
  dec var_0001
  dec var_0003
loc_CCE8:
  dey
  lda (var_0002),y
  sta (var_0000),y
  jmp loc_CCDE
loc_CCF0:
  pla
  sta var_5FF9
  sta var_5FFB
  pla
  sta var_5FFE
  pla
  sta var_0200
  sta var_5FF5
  bit data_FF98
  php
  lda #$0C
  sta data_FF90
  ldx #$04
loc_CD0D:
  lda data_CA21,x
  sta var_0081,x
  dex
  bpl loc_CD0D
  ldy #$13
  sty var_00FD
  lda #$00
loc_CD1B:
  pha
  ldx #$05
  stx var_0080
  ldx #$00
  stx var_00FF
  clc
  adc #$01
  asl a
  tay
  lda #$00
  rol a
  tax
  cpy #$64
  bcc loc_CD35
  lda #$12
  sta var_00FD
loc_CD35:
  lda #$1C
  jsr sub_C3BD
  pla
  clc
  adc #$01
  cmp var_0200
  bne loc_CD1B
  lda #$0D
  sta var_00FC
  jsr sub_F78C
  plp
  bvs loc_CD52
  bit data_FF98
  bvs loc_CD55
loc_CD52:
  jmp loc_CDD7
loc_CD55:
  ldx #$CA
  ldy #$26
  lda #$09
  jsr sub_C344
  lda #$80
  pha
  lda #$00
  sta var_0001
  lda #$0C
  sta var_5FA3
  jsr sub_D8F9
  jsr sub_D449
  lda #$32
  jsr sub_D920
  bcs loc_CD8E
  jsr sub_DA6A
  lda #$00
  jsr sub_D920
  bcs loc_CD8E
  ldx #$CA
  ldy #$35
  lda #$09
  jsr sub_C344
  pla
  ora #$01
  pha
loc_CD8E:
  lda #$01
  sta var_0001
  lda #$0C
  sta var_5FA3
  jsr sub_D8F9
  lda #$32
  jsr sub_D920
  bcs loc_CDB8
  jsr sub_DA6A
  lda #$00
  jsr sub_D920
  bcs loc_CDB8
  ldx #$CA
  ldy #$44
  lda #$09
  jsr sub_C344
  pla
  ora #$02
  pha
loc_CDB8:
  pla
  sta var_5FA4
  and #$03
  bne loc_CDC9
  ldx #$CA
  ldy #$53
  lda #$09
  jsr sub_C344
loc_CDC9:
  inc var_00FC
  inc var_00FC
  inc var_00FC
  lda #$0C
  sta var_5FA3
  sta data_FF90
loc_CDD7:
  ldy #$00
loc_CDD9:
  lda var_4340
  cmp var_4348
  bne loc_CDEB
  cmp var_4350
  bne loc_CDEB
  cmp var_4358
  beq loc_CE17
loc_CDEB:
  dey
  bne loc_CDD9
  ldx #$CA
  ldy #$62
  lda #$05
  jsr sub_C344
  jsr sub_C287
  tax
  beq loc_CE06
  ldx #$CA
  ldy #$6F
  lda #$13
  jmp loc_CE0C
loc_CE06:
  ldx #$CA
  ldy #$6D
  lda #$11
loc_CE0C:
  jsr sub_C344
  lda #$80
  sta var_5FC0
  jmp loc_CE17
loc_CE17:
  lda #$FF
  sta loc_FF04
  sta var_5FFA
  lda #$FE
  sta var_5FF4
  lda #$15
  sta var_00FC
  ldx #$CA
  ldy #$78
  lda #$0C
  jsr sub_C344
  jsr sub_C2FE
  ldx #$22
  ldy #$E3
  jsr sub_E9FB
  ldx #$1A
  lda #$90
loc_CE3F:
  sta var_2007
  dex
  bne loc_CE3F
  stx var_6005
  jsr bios_set_av10
  jsr bios_run_initial
  ldx #$59
  ldy #$F8
  jsr bios_set_nmi_vector
  jsr sub_C37A
  lda #$0E
  sta var_6001
  lda #$80
  sta var_6000
  sta var_2000
  ldx #$21
  ldy #$2E
  stx var_5FE7
  sty var_5FE6
  jsr sub_E048
loc_CE72:
  lda #$C0
  sta var_5C40
  lda var_5FC0
  bpl loc_CEAB
  lda #$18
  sta var_00FC
  ldx #$CA
  ldy #$B7
  lda #$07
  jsr sub_C322
  jsr sub_C287
  tax
  beq loc_CEAB
  ora #$80
  sta var_5FC0
  lda #$02
  jsr sub_D146
  jsr sub_D21B
  lda #$00
  sta loc_59A4+1
  lda #$7C
  sta loc_59A4+2
  lda #$FF
  jmp loc_CF29
loc_CEAB:
  lda var_5FA4
  bpl loc_CF06
  and #$03
  beq loc_CF06
  ldx #$CA
  ldy #$90
  lsr a
  bcc loc_CEC0
  lsr a
  ldx #$CA
  ldy #$81
loc_CEC0:
  and #$01
  pha
  lda #$18
  sta var_00FC
  lda #$09
  jsr sub_C322
  pla
  sta var_0081
  ldx #$00
  stx var_0082
  stx var_0083
  lda #$04
  sta var_0087
  stx var_0086
  inx
  stx var_0084
  stx var_0088
  lda #$03
  sta var_5FB6
loc_CEE5:
  lda #$02
  sta var_0080
  ldx #$00
  ldy #$80
  jsr bios_disk_io
  tax
  beq loc_CF0C
  dec var_5FB6
  beq loc_CF06
  lda #$00
  sta var_0080
  ldx #$00
  ldy #$80
  jsr bios_disk_io
  jmp loc_CEE5
loc_CF06:
  ldy #$CB
  ldx #$CA
  bne loc_CF3F
loc_CF0C:
  lda var_0403,x
  cmp data_CA0E,x
  bne loc_CF3B
  inx
  cpx #$05
  bne loc_CF0C
  lda var_0401
  clc
  adc #$02
  sta loc_59A4+1
  lda #$04
  sta loc_59A4+2
  lda var_0081
loc_CF29:
  pha
  jsr bios_display_initial
  lda #$00
  sta var_5C40
  pla
  jsr sub_59A1
  lda #$C0
  sta var_5C40
loc_CF3B:
  ldy #$9F
  ldx #$CA
loc_CF3F:
  lda #$18
  sta var_00FC
  lda #$05
  jsr sub_C322
  lda #$18
  sta var_00FC
  ldx #$CA
  ldy #$AE
  lda #$13
  jsr sub_C344
  jsr sub_D09C
  lda #$00
  sta var_5C40
  ldy #$00
  ldx #$17
  jsr bios_read_key
  jmp loc_CE72

sub_CF67:
  txa
  pha
  tya
  pha
  lda #$00
  sta var_2001
  ldy var_5FC2
  ldx #$1E
  bit var_2002
  lda var_00FF
  pha
loc_CF7B:
  lda var_5D00,y
  sta var_00FF
  iny
  bit var_00FF
  bpl loc_CFF7
  bvc loc_CFDF
  pha
  and #$3F
  sta var_2006
  lda var_5D00,y
  sta var_2006
  sta var_5FC2
  iny
  lda var_5D00,y
  sta var_00FF
  pha
loc_CF9D:
  iny
  lda var_5D00,y
  sta var_2007
  dec var_00FF
  beq loc_CFDA
  iny
  lda var_5D00,y
  sta var_2007
  dec var_00FF
  beq loc_CFDA
  iny
  lda var_5D00,y
  sta var_2007
  dec var_00FF
  beq loc_CFDA
  dex
  bne loc_CF9D
  lda var_00FF
  sta var_5D00,y
  dey
  pla
  sec
  sbc var_00FF
  clc
  adc var_5FC2
  sta var_5D00,y
  dey
  pla
  sta var_5D00,y
  jmp loc_D071
loc_CFDA:
  pla
  pla
  jmp loc_D05A
loc_CFDF:
  lsr a
  bcs loc_CFEF
  lda var_6000
  and #$FC
  ora var_5D00,y
  sta var_6000
  bcc loc_D05A
loc_CFEF:
  lda var_5D00,y
  sta var_5FC5
  bcs loc_D05A
loc_CFF7:
  bvc loc_D046
  and #$3F
  sta var_2006
  lda var_5D00,y
  sta var_2006
  iny
  lda var_5D00,y
  pha
  iny
  sty var_00FF
  lda var_5D00,y
  tay
  pla
loc_D011:
  sta var_2007
  dey
  beq loc_D058
  sta var_2007
  dey
  beq loc_D058
  sta var_2007
  dey
  beq loc_D058
  sta var_2007
  dey
  beq loc_D058
  dex
  bne loc_D011
  tya
  ldy var_00FF
  ldx var_5D00,y
  sta var_5D00,y
  txa
  sec
  sbc var_5D00,y
  dey
  dey
  clc
  adc var_5D00,y
  sta var_5D00,y
  dey
  bcc loc_D071
loc_D046:
  sta var_2006
  lda var_5D00,y
  sta var_2006
  iny
  lda var_5D00,y
  sta var_2007
  bvc loc_D05A
loc_D058:
  ldy var_00FF
loc_D05A:
  iny
  cpy var_5FC3
  beq loc_D066
  dex
  beq loc_D071
  jmp loc_CF7B
loc_D066:
  lda var_6000
  and #$FC
  ora var_5FC4
  sta var_6000
loc_D071:
  sty var_5FC2
  pla
  sta var_00FF
  lda #$00
  sta var_2005
  lda var_5FC5
  sta var_2005
  lda var_6000
  sta var_2000
  lda var_6001
  sta var_2001
  pla
  tay
  pla
  tax
  rts

sub_D093:
  lda var_6000
  and #$7F
  sta var_2000
  rts

sub_D09C:
  lda var_6000
  ora #$80
  sta var_2000
  rts
