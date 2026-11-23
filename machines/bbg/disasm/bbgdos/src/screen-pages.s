; 模块：screen-pages
; 显示方式、初始化屏幕向量、文件名转换，以及程序运行用的显存与页保存恢复

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_2EA9"
; 段 0，$AAA9-$B173
; DOS 功能 11 DosGetSetDispMode：取或设置显示方式。
; 入口：X 位 0=0 取方式，A 返回 0（英文）或 $80（中文）；X 位 0=1 设置，Y 为 0（英文）或 $80（中文）；X 位 1=1 时以后才改变屏幕显示。
dos_get_set_disp_mode:
  txa
  lsr a
  bcs loc_AAB1
  lda data_A544
  rts
loc_AAB1:
  lsr a
  bcc sub_AAB9
  sty data_A544
  clc
  rts

sub_AAB9:
  lda data_A544
  pha
  sty data_A544
  jsr dos_initial_screen
  pla
  bcc loc_AAC9
  sta data_A544
loc_AAC9:
  lda data_A544
  and #$FE
  sta data_A544
  lda #$00
  sta data_A771
  rts

; DOS 功能 13 DosGetSetInitScrn：取并设置初始化显示屏幕处理向量。
; 入口：$FF 为新向量所在页，X:Y 为新的处理程序地址减 1（X 高字节）。
; 出口：A 为原向量所在页，X:Y 为原处理程序地址减 1。
dos_get_set_init_screen:
  lda data_A826
  stx data_A826
  tax
  lda data_A825
  sty data_A825
  tay
  lda data_A824
  pha
  lda var_00FF
  sta data_A824
  pla
  rts

; DOS 功能 12 DosFileNameAccs：把 "D:FFFFFFFF.XXX" 形式的文件名转换成 11 字节的内部格式（主名 8 字节加扩展名 3 字节，空格补齐，'*' 展开成 '?'）。
; 入口：X:Y 为以 0 结尾的文件名地址；转换结果写回原地址。
; 出口：A 为盘号（0 默认盘，1 为 A 盘），盘符非法时 A=$0F。
dos_file_name_access:
  lda var_00FF
  pha
  lda var_00FE
  pha
  lda var_00FD
  pha
  stx var_00FF
  sty var_00FE
  ldy #$00
  sty data_A53F
loc_AB02:
  jsr sub_A5FD
  tax
  beq loc_AB36
  iny
  cmp #$20
  beq loc_AB02
  cmp #$09
  beq loc_AB02
  jsr sub_A5FD
  dey
  cmp #$3A
  bne loc_AB36
  jsr sub_A5FD
  and #$DF
  cmp #$5B
  bcs loc_AB27
  sbc #$40
  bcs loc_AB2F
  sec
loc_AB27:
  lda #$0F
  sta data_A53F
  jmp loc_ABAC
loc_AB2F:
  adc #$00
  sta data_A53F
  iny
  iny
loc_AB36:
  ldx #$00
loc_AB38:
  jsr sub_A5FD
  cmp #$00
  beq loc_AB57
  cmp #$2E
  beq loc_AB57
  iny
  cmp #$2A
  beq loc_AB5B
  cpx #$08
  bcs loc_AB38
  jsr sub_A623
  bcs loc_ABAC
  sta data_A5AF,x
  inx
  bne loc_AB38
loc_AB57:
  lda #$20
  bne loc_AB5D
loc_AB5B:
  lda #$3F
loc_AB5D:
  cpx #$08
  bcs loc_AB67
  sta data_A5AF,x
  inx
  bne loc_AB5D
loc_AB67:
  jsr sub_A5FD
  cmp #$00
  beq loc_AB8E
  iny
  cmp #$2E
  bne loc_AB67
loc_AB73:
  jsr sub_A5FD
  iny
  cmp #$00
  beq loc_AB8E
  cmp #$2A
  beq loc_AB92
  cpx #$0B
  bcs loc_AB73
  jsr sub_A623
  bcs loc_ABAC
  sta data_A5AF,x
  inx
  bne loc_AB73
loc_AB8E:
  lda #$20
  bne loc_AB94
loc_AB92:
  lda #$3F
loc_AB94:
  cpx #$0B
  bcs loc_AB9E
  sta data_A5AF,x
  inx
  bne loc_AB94
loc_AB9E:
  ldy #$0A
loc_ABA0:
  lda data_A5AF,y
  sta var_00FD
  jsr sub_A614
  dey
  bpl loc_ABA0
  clc
loc_ABAC:
  pla
  sta var_00FD
  pla
  sta var_00FE
  pla
  sta var_00FF
  lda data_A53F
  rts

sub_ABB9:
  lda data_A53A
  sta var_0080
  ldx #$80
  ldy #$00
  stx var_0082
  sty var_0081
  lda #$00
  sta var_0083
  sta data_8007
  stx var_0085
  sty var_0084
  ldx #$18
  ldy #$00
  stx var_0087
  sty var_0086
  lda #$0C
  ldx #$80
  jmp BIOS_ENTRY

sub_ABE0:
  ldx var_0406
  ldy var_0405
  stx data_A540
  sty data_A53F
  lda #$02
  jsr sub_B4AA
  lda #$1E
  bcs loc_AC09
  ldx data_A540
  cpx var_0406
  bne loc_AC06
  ldy data_A53F
  cpy var_0405
  clc
  beq loc_AC09
loc_AC06:
  lda #$0B
  sec
loc_AC09:
  rts

sub_AC0A:
  lda #$00
  sta var_0406
  lda var_045F,y
  sta var_0409
  lda var_045E,y
  sta var_0408
  lda var_045D,y
  sta var_0407
  rts

sub_AC22:
  sta var_0405
  jsr sub_AC0A
  ldx #$04
  ldy #$57
  stx var_0404
  sty var_0403
  jmp sub_ABE0

sub_AC35:
  ldx var_045C
  ldy var_045B
  stx var_0406
  sty var_0405
  ldx var_045A
  ldy var_0459
  stx var_0404
  sty var_0403
  rts

sub_AC4E:
  lda #$00
  sec
  sbc var_0403
  tay
  lda #$C0
  sbc var_0404
  tax
  cpy var_045A
  sbc var_045B
  bcc loc_AC69
  ldx var_045B
  ldy var_045A
loc_AC69:
  stx var_0406
  sty var_0405
  rts

sub_AC70:
  lda #$05
  jsr sub_AC22
  bcs loc_ACAD
  lda data_A7D9
  sta var_0402
  sta var_0080
  ldx #$80
  ldy #$00
  stx var_0404
  sty var_0403
  stx var_0082
  sty var_0081
  jsr sub_AC4E
  jsr sub_ABE0
  bcs loc_ACAD
  ldx data_A7D9
  lda var_0459
  cmp #$C0
  bne loc_ACA1
  ldx #$FF
loc_ACA1:
  stx var_0083
  lda #$25
  ldx #$80
  ldy #$40
  jsr BIOS_ENTRY
  clc
loc_ACAD:
  rts

sub_ACAE:
  lda #$05
  jsr sub_AC22
  bcs loc_ACFE
  ldx var_0459
  ldy var_0458
  cpx #$C0
  bcs loc_ACEA
  lda data_A7D9
  sta var_0402
  stx var_0404
  sty var_0403
  jsr sub_AC4E
  lda var_045A
  sec
  sbc var_0405
  sta var_045A
  lda var_045B
  sbc var_0406
  sta var_045B
  jsr sub_ABE0
  bcs loc_ACFE
  ldx #$C0
  ldy #$00
loc_ACEA:
  lda #$FF
  sta var_0402
  txa
  and #$BF
  sta var_0404
  sty var_0403
  jsr sub_AC4E
  jsr sub_ABE0
loc_ACFE:
  rts

sub_ACFF:
  cpx #$FF
  bne loc_AD32
  bit data_A547
  bpl loc_AD1F
  inc data_A546
  lda #$04
  ldx var_0447
  jsr BIOS_ENTRY
  bit data_A544
  bpl loc_AD29
  ldx #$01
  ldy #$00
  jsr dos_get_set_disp_mode
loc_AD1F:
  bit data_A544
  bpl loc_AD29
  lda #$00
  sta var_2009
loc_AD29:
  lda var_6000
  and #$7F
  sta var_2000
  sei
loc_AD32:
  rts

sub_AD33:
  lda #$06
  jsr sub_AC22
  bcs loc_AD8F
  lda data_A7D9
  sta var_0402
  sta var_0080
  jsr sub_AC35
  stx var_0082
  sty var_0081
  jsr sub_ABE0
  bcs loc_AD8F
  ldx var_0458
  stx var_0083
  jsr sub_ACFF
  lda #$25
  ldx #$80
  ldy #$40
  jsr BIOS_ENTRY
  jmp loc_AD82

sub_AD62:
  lda #$06
  jsr sub_AC22
  bcs loc_AD8F
  ldx var_0458
  stx var_0402
  cpx data_A7D9
  bcs loc_AD77
  inc data_A546
loc_AD77:
  jsr sub_ACFF
  jsr sub_AC35
  jsr sub_ABE0
  bcs loc_AD8F
loc_AD82:
  lda var_0458
  cmp #$FF
  bcc loc_AD8F
  lda #$09
  jsr BIOS_ENTRY
  clc
loc_AD8F:
  rts

sub_AD90:
  lda #$06
  jsr sub_AC22
  bcs loc_AD8F
  lda data_A7D9
  sec
  adc var_0458
  bcs loc_ADAD
  cmp #$FF
  beq loc_ADAD
  sta var_0402
  jsr sub_AC35
  jmp sub_ABE0
loc_ADAD:
  lda #$08
  rts

sub_ADB0:
  lda #$03
  jsr sub_AC22
  bcs loc_AD8F
  ldx var_0459
  ldy var_0458

sub_ADBD:
  stx var_0406
  sty var_0405
  ldx data_A7D9
  stx var_0402
  ldx #$80
  ldy #$00
  stx var_0404
  sty var_0403
  jmp sub_ABE0

sub_ADD6:
  bit data_A544
  bpl loc_ADE2
  ldx #$01
  ldy #$00
  jsr dos_get_set_disp_mode
loc_ADE2:
  lda #$00
  sta var_2001
  lda data_A546
  bne loc_ADF7
  inc data_A546
  lda #$04
  ldx var_0447
  jsr BIOS_ENTRY
loc_ADF7:
  rts

sub_ADF8:
  lda data_A7D9
  sta var_0080
  lda #$80
  sta var_0082
  lda #$00
  sta var_0081
  lda #$15
  ldx #$80
  jmp BIOS_ENTRY

sub_AE0C:
  jsr sub_ADB0
  bcs loc_AE36
  lda #$00
  sta var_0084
  sta var_0083
  ldx var_0459
  ldy var_0458
  stx var_0086
  sty var_0085
  lda #$01
  bne loc_AE2C

sub_AE25:
  jsr sub_ADB0
  bcs loc_AE36
  lda #$00
loc_AE2C:
  pha
  jsr sub_ADD6
  pla
  tay
  jsr sub_ADF8
  clc
loc_AE36:
  rts

sub_AE37:
  sec
  lda var_0461,y
  sbc var_045D,y
  sta var_0088
  lda var_0462,y
  sbc var_045E,y
  sta var_0089
  lda var_0463,y
  sbc var_045F,y
  sta var_008A
  jsr sub_AC0A
  jsr sub_ADD6
  lda #$00
  sta var_0088
loc_AE5A:
  ldx #$00
loc_AE5C:
  txa
  pha
  lda #$13
  ldy var_0088
  jsr BIOS_ENTRY
  pla
  tax
  inc var_0088
  inx
  cpx #$04
  bcc loc_AE5C
  ldx #$20
  ldy #$00
  stx var_0086
  sty var_0085
  jsr sub_ADBD
  lda #$00
  sta var_0084
  sta var_0083
  ldy #$01
  jsr sub_ADF8
  sec
  lda var_0089
  sbc #$20
  bcs loc_AE8F
  dec var_008A
  bmi loc_AE95
loc_AE8F:
  sta var_0089
  ora var_008A
  bne loc_AE5A
loc_AE95:
  clc
  rts
data_AE97:
  .byte $00

sub_AE98:
  ldx #$04
  ldy #$3D
  stx var_0404
  sty var_0403
  ldx #$00
  ldy #$1A
  stx var_0406
  sty var_0405
  jsr sub_ABE0
  bcs loc_AF10
  lda var_043D
  cmp #$4D
  bne loc_AF0D
  lda var_043E
  cmp #$59
  bne loc_AF0D
  lda #$00
  sta data_A547
  sta data_A546
  lda var_044A
  cmp #$01
  beq loc_AED9
  cmp #$00
  beq loc_AED6
  cmp #$03
  bne loc_AF0D
loc_AED6:
  dec data_A547
loc_AED9:
  lda var_0447
  and #$08
  beq loc_AEE5
  lda data_A544
  bmi loc_AEF1
loc_AEE5:
  lda var_0447
  and #$10
  beq loc_AEF5
  lda data_A544
  bmi loc_AEF5
loc_AEF1:
  lda #$28
  sec
  rts
loc_AEF5:
  ldx data_A7D9
  dex
  bit data_A547
  bpl loc_AF05
  lda #$07
  jsr BIOS_ENTRY
  tax
  dex
loc_AF05:
  cpx var_044D
  bcc loc_AF11
  lda #$08
  rts
loc_AF0D:
  lda #$0B
  sec
loc_AF10:
  rts
loc_AF11:
  lda #$0A
  jsr BIOS_ENTRY
  cmp var_044E
  bcs loc_AF1F
  lda #$29
  sec
  rts
loc_AF1F:
  ldx #$04
  ldy #$5D
  stx var_0404
  sty var_0403
  ldx var_0456
  ldy var_0455
  stx var_0408
  sty var_0407
  lda #$00
  sta var_0406
  lda var_0443
  asl a
  bcc loc_AF43
  inc var_0406
loc_AF43:
  asl a
  bcc loc_AF49
  inc var_0406
loc_AF49:
  sta var_0405
  jsr sub_ABE0
  bcs loc_AF10
  lda #$FF
  sta data_AE97
  lda #$00
loc_AF58:
  pha
  asl a
  asl a
  tay
  lda var_0460,y
  cmp #$0B
  beq loc_AF67
  cmp #$0C
  bne loc_AF6D
loc_AF67:
  jsr sub_AD62
  jmp loc_AFB7
loc_AF6D:
  cmp #$0E
  bne loc_AF77
  jsr sub_AD90
  jmp loc_AFB7
loc_AF77:
  cmp #$0D
  bne loc_AF81
  jsr sub_AD33
  jmp loc_AFB7
loc_AF81:
  cmp #$02
  bne loc_AF8B
  jsr sub_ACAE
  jmp loc_AFB7
loc_AF8B:
  cmp #$04
  bne loc_AF95
  jsr sub_AC70
  jmp loc_AFB7
loc_AF95:
  bit data_A547
  bpl loc_AFB9
  cmp #$05
  bne loc_AFA4
  jsr sub_AE0C
  jmp loc_AFB7
loc_AFA4:
  cmp #$06
  bne loc_AFAE
  jsr sub_AE25
  jmp loc_AFB7
loc_AFAE:
  cmp #$10
  bne loc_AFB9
  sty data_AE97
  beq loc_AFB9
loc_AFB7:
  bcs loc_AFCF
loc_AFB9:
  pla
  clc
  adc #$01
  cmp var_0443
  bne loc_AF58
  bit data_AE97
  bmi loc_AFD8
  ldy data_AE97
  jsr sub_AE37
  bcc loc_AFD8
loc_AFCF:
  tax
  pla
  ldy data_A546
  bne loc_AF58
  txa
  rts
loc_AFD8:
  bit data_A547
  bmi loc_AFFB
  ldx var_0452
  ldy var_0451
  bne loc_AFE6
  dex
loc_AFE6:
  dey
  txa
  pha
  tya
  pha
  ldx data_A7D9
  bit data_A547
  bpl loc_AFF6
  ldx var_0449
loc_AFF6:
  lda #$01
  jmp BIOS_ENTRY
loc_AFFB:
  ldx data_A546
  bne loc_B015
  lda #$04
  ldx var_0447
  jsr BIOS_ENTRY
  ldy #$00
  jsr sub_AAB9
  lda #$00
  sta var_2001
  sta var_2000
loc_B015:
  lda var_0447
  and #$20
  bne loc_B02F
  jsr sub_ABB9
  ldx data_A53C
  ldy data_A53B
  stx var_00FF
  sty var_00FE
  ldy #$18
  lda #$00
  sta (var_00FE),y
loc_B02F:
  lda var_044A
  cmp #$03
  beq loc_B087
  lda var_0447
  and #$04
  beq loc_B087
  lda #$00
  tax
loc_B040:
  pha
  asl a
  asl a
  tay
  lda var_0460,y
  cmp #$01
  beq loc_B053
  cmp #$03
  beq loc_B053
  cmp #$09
  bne loc_B06C
loc_B053:
  sta var_0460,x
  lda var_045F,y
  sta var_045F,x
  lda var_045E,y
  sta var_045E,x
  lda var_045D,y
  sta var_045D,x
  inx
  inx
  inx
  inx
loc_B06C:
  pla
  clc
  adc #$01
  cmp var_0443
  bne loc_B040
  lda #$04
  sta var_0404
  lda #$5D
  sta var_0403
  ldx #$04
  ldy #$00
  stx var_00FF
  sty var_00FE
loc_B087:
  lda #$08
  ldx #$04
  ldy #$3D
  jmp BIOS_ENTRY
data_B090:
  .byte $A9, $01, $AE, $D9, $A7, $20, $00, $58, $6C, $FE, $00

sub_B09B:
  ldx #$61
  ldy #$30
  stx var_0404
  sty var_0403
  ldx #$00
  ldy #$02
  stx var_0406
  sty var_0405
  jsr sub_ABE0
  bcs loc_B0E8
  lda data_A7D9
  sta var_0402
  ldx var_6131
  ldy var_6130
  stx var_00FF
  sty var_00FE
  cpx #$C0
  bcs loc_B0ED
  stx var_0404
  sty var_0403
  lda #$00
  sec
  sbc var_00FE
  sta var_0405
  lda #$C0
  sbc var_00FF
  sta var_0406
  jsr sub_ABE0
  bcc loc_B0E9
  cmp #$1E
  clc
  bne loc_B107
  sec
loc_B0E8:
  rts
loc_B0E9:
  ldx #$C0
  ldy #$00
loc_B0ED:
  lda #$00
  sec
  sbc var_0403
  sta var_0405
  lda #$FF
  sbc var_0404
  sta var_0406
  lda #$0B
  bcc loc_B127
  lda #$02
  jsr sub_B4A4
loc_B107:
  php
  lda #$05
  jsr sub_B4AA
  plp
  bcs loc_B125
  bit data_8030
  bmi loc_B118
  asl data_8030
loc_B118:
  ldx #$0A
loc_B11A:
  lda data_B090,x
  sta var_00E0,x
  dex
  bpl loc_B11A
  jmp a:var_00E0
loc_B125:
  lda #$1E
loc_B127:
  sec
  rts
data_B129:
  .byte $5C, $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D
  .byte $44, $00
data_B136:
  .byte $43, $4D, $44, $45, $58, $43, $47, $41, $4D

sub_B13F:
  ldy #$00
loc_B141:
  jsr sub_A5FD
  cmp #$20
  beq loc_B150
  sta var_0430,y
  iny
  cpy #$08
  bne loc_B141
loc_B150:
  lda #$2E
  sta var_0430,y
  iny
  tya
  tax
  ldy #$08
loc_B15A:
  jsr sub_A5FD
  cmp #$20
  beq loc_B16A
  sta var_0430,x
  inx
  iny
  cpy #$0B
  bne loc_B15A
loc_B16A:
  lda #$00
  sta var_0430,x
  ldx #$04
  ldy #$30
  rts
