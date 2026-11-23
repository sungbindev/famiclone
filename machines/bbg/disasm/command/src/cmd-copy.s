; 模块：cmd-copy
; COPY 命令

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_26CF"
; 段 0，$A6CF-$AE67
  .byte $43, $6F, $70, $79, $20, $66, $69, $6C, $65, $28, $73, $29, $20, $74, $6F, $20
  .byte $6F, $74, $68, $65, $72, $20, $6C, $6F, $63, $61, $74, $69, $6F, $6E, $0D, $0A
  .byte $0A, $43, $4F, $50, $59, $20, $5B, $64, $72, $69, $76, $65, $3A, $5D, $5B, $70
  .byte $61, $74, $68, $5D, $73, $6F, $75, $72, $63, $65, $0D, $0A, $20, $20, $20, $5B
  .byte $2B, $5B, $64, $72, $69, $76, $65, $3A, $5D, $5B, $70, $61, $74, $68, $5D, $73
  .byte $6F, $75, $72, $63, $65, $5D, $0D, $0A, $20, $20, $20, $20, $20, $2E, $2E, $2E
  .byte $0D, $0A, $20, $20, $20, $5B, $64, $72, $69, $76, $65, $3A, $5D, $5B, $70, $61
  .byte $74, $68, $5D, $5B, $64, $65, $73, $74, $69, $6E, $61, $74, $69, $6F, $6E, $5D
  .byte $0D, $0A, $20, $20, $20, $5B, $2F, $59, $5D, $0D, $0A, $0A, $2F, $59, $20, $4E
  .byte $6F, $20, $63, $6F, $6E, $66, $69, $72, $6D, $20, $74, $6F, $20, $6F, $76, $65
  .byte $72, $77, $72, $69, $74, $65, $20, $61, $6C, $6C, $24, $B8, $B4, $D6, $C6, $D2
  .byte $BB, $B8, $F6, $BB, $F2, $B6, $E0, $B8, $F6, $CE, $C4, $BC, $FE, $B5, $BD, $D0
  .byte $C2, $B5, $C4, $CE, $BB, $D6, $C3, $0D, $0A, $43, $4F, $50, $59, $20, $5B, $C5
  .byte $CC, $3A, $5D, $5B, $C2, $B7, $BE, $B6, $5D, $D4, $B4, $CE, $C4, $BC, $FE, $0D
  .byte $0A, $20, $20, $20, $5B, $2B, $5B, $C5, $CC, $3A, $5D, $5B, $C2, $B7, $BE, $B6
  .byte $5D, $D4, $B4, $CE, $C4, $BC, $FE, $5D, $0D, $0A, $20, $20, $20, $20, $20, $2E
  .byte $2E, $2E, $0D, $0A, $20, $20, $20, $20, $20, $5B, $C5, $CC, $3A, $5D, $5B, $C2
  .byte $B7, $BE, $B6, $5D, $5B, $C4, $BF, $B5, $C4, $CE, $C4, $BC, $FE, $5D, $5B, $2F
  .byte $59, $5D, $0D, $0A, $0A, $2F, $59, $20, $CE, $C4, $BC, $FE, $D2, $D1, $B4, $E6
  .byte $D4, $DA, $CA, $B1, $B2, $BB, $D3, $C3, $D4, $D9, $C8, $B7, $C8, $CF, $24, $41
  .byte $17, $A8, $56, $1A, $A8, $59, $1D, $A8, $00, $E6, $F5, $60, $E6, $F6, $60, $E6
  .byte $FB, $60

sub_A821:
  ldx var_00EF
  lda var_62E7,x
  beq loc_A862
  inc var_00EF
  cmp var_00ED
  beq loc_A862
  lda var_6338,y
  beq loc_A84C
  cmp #$2A
  beq loc_A840
  iny
  cmp var_00ED
  beq loc_A84C
  cmp #$3F
  bne loc_A843
loc_A840:
  lda var_62E7,x
loc_A843:
  ldx var_00EE
  sta var_6389,x
  inc var_00EE
  bne sub_A821
loc_A84C:
  ldx var_00EE
  sta var_6389,x
  inc var_00EE
  ldx var_00EF
loc_A855:
  lda var_62E7,x
  beq loc_A85F
  inx
  cmp var_00ED
  bne loc_A855
loc_A85F:
  stx var_00EF
  rts
loc_A862:
  ldx var_00EE
loc_A864:
  lda var_6338,y
  beq loc_A88A
  iny
  cmp #$3F
  beq loc_A864
  cmp #$2A
  beq loc_A87C
  cmp var_00ED
  beq loc_A886
  sta var_6389,x
  inx
  bne loc_A864
loc_A87C:
  lda var_6338,y
  beq loc_A88A
  iny
  cmp var_00ED
  bne loc_A87C
loc_A886:
  sta var_6389,x
  inx
loc_A88A:
  stx var_00EE
  rts

sub_A88D:
  lda var_00E7
  sta var_00EF
  lda var_00EB
  sta var_00EE
  lda #$2E
  sta var_00ED
  ldy var_00EC
  jsr sub_A821
  lda #$00
  sta var_00ED
  jsr sub_A821
  ldx var_00EE
  lda #$00
  sta var_6389,x
  rts

sub_A8AD:
  ldx #$00
  ldy #$08
loc_A8B1:
  lda var_62E7,x
  cmp var_6389,x
  beq loc_A8D7
  ldy var_6389,x
  cmp #$00
  beq loc_A8CD
  cmp #$2E
  bne loc_A919
  tya
  bne loc_A919
  lda var_62E8,x
  beq loc_A913
  rts
loc_A8CD:
  cpy #$2E
  bne loc_A919
  lda var_638A,x
  beq loc_A913
  rts
loc_A8D7:
  cmp #$00
  beq loc_A913
  cmp #$2E
  beq loc_A8FB
  inx
  dey
  bne loc_A8B1
  txa
  tay
loc_A8E5:
  lda var_62E7,x
  beq loc_A8EF
  inx
  cmp #$2E
  bne loc_A8E5
loc_A8EF:
  lda var_6389,y
  beq loc_A8FD
  iny
  cmp #$2E
  bne loc_A8EF
  bne loc_A8FD
loc_A8FB:
  txa
  tay
loc_A8FD:
  lda #$03
  sta var_00F2
loc_A901:
  lda var_62E7,x
  cmp var_6389,y
  bne loc_A919
  cmp #$00
  beq loc_A913
  inx
  iny
  dec var_00F2
  bne loc_A901
loc_A913:
  lda var_6101
  cmp var_6131
loc_A919:
  rts

sub_A91A:
  jsr sub_A920
  jmp sub_8A67

sub_A920:
  ldx #$62
  ldy #$E7
  lda var_6388

sub_A927:
  stx var_00FF
  sty var_00FE

sub_A92B:
  beq loc_A930
  jsr sub_9B7C
loc_A930:
  ldy #$00
loc_A932:
  lda (var_00FE),y
  beq loc_A940
  sty var_00F2
  jsr sub_8A59
  ldy var_00F2
  iny
  bne loc_A932
loc_A940:
  rts

sub_A941:
  stx var_00FF
  sty var_00FE
  pha
  ldx #$2D
  lda #$20
  jsr sub_8A54
  pla
  jsr sub_A92B
  jmp sub_8A67

sub_A954:
  ldx var_63DC
  stx var_6102
loc_A95A:
  ldx #$80
  ldy #$00
  stx var_6104
  sty var_6103
  ldx #$40
  ldy #$00
  stx var_6106
  sty var_6105
  lda #$02
  jsr sub_8D8E
  bcs loc_A98D
  lda var_00F5
  beq loc_A979
loc_A979:
  lda var_6106
  cmp #$40
  bne loc_A98E
  inc var_6102
  lda var_6102
  cmp #$FE
  bne loc_A95A
  sta var_00FA
  clc
loc_A98D:
  rts
loc_A98E:
  lda var_6102
  sta var_00FA
  ldx var_6106
  ldy var_6105
  stx var_00F9
  sty var_00F8
  rts

sub_A99E:
  clc
  lda var_00F6
  beq loc_A9D2
  sec
  lda var_6137
  sbc var_6135
  sta var_6137
  lda var_6138
  sbc var_6136
  sta var_6138
  lda var_6139
  sbc #$00
  sta var_6139
  bcs loc_A9C3
  dec var_613A
loc_A9C3:
  ldx #$80
  ldy #$00
  stx var_6134
  sty var_6133
  lda #$02
  jsr sub_8DA2
loc_A9D2:
  rts

sub_A9D3:
  ldx var_63DC
  stx var_6132
loc_A9D9:
  ldx #$80
  ldy #$00
  stx var_6134
  sty var_6133
  ldx #$40
  ldy #$00
  lda var_6132
  cmp var_00FA
  bne loc_A9F2
  ldx var_00F9
  ldy var_00F8
loc_A9F2:
  stx var_6136
  sty var_6135
  lda #$03
  jsr sub_8DA2
  bcs loc_AA31
  ldx var_6136
  ldy var_6135
  lda var_6132
  cmp var_00FA
  bne loc_AA17
  cpx var_00F9
  bne loc_AA2C
  cpy var_00F8
  bne loc_AA2C
  jmp sub_A99E
loc_AA17:
  cpx #$40
  bne loc_AA2C
  jsr sub_A99E
  bcs loc_AA31
  inc var_6132
  lda var_6132
  cmp #$FE
  bne loc_A9D9
  clc
  rts
loc_AA2C:
  lda #$12
  jsr sub_8A64
loc_AA31:
  sec
  rts

sub_AA33:
  lda #$08
  jsr sub_8A3B
  ldx #$63
  ldy #$89
  lda var_63D9
  jsr sub_A927
  lda #$10
  jsr sub_8A3B
  lda #$00
  sta var_00F7
loc_AA4B:
  lda #$07
  jsr DOS_ENTRY
  cmp #$0D
  beq loc_AA6D
  tax
  and #$DF
  cmp #$59
  beq loc_AA63
  cmp #$4E
  beq loc_AA63
  cmp #$41
  bne loc_AA4B
loc_AA63:
  sta var_00F7
  lda #$08
  jsr sub_8A54
  jmp loc_AA4B
loc_AA6D:
  lda var_00F7
  beq loc_AA4B
  jsr sub_8A67
  lda var_00F7
  cmp #$59
  beq loc_AA83
  cmp #$41
  bne loc_AA83
  ldx #$01
  stx var_00FB
  dex
loc_AA83:
  rts

sub_AA84:
  lda var_00F0
  sta var_6132
  ldx var_00E1
  ldy var_00E0
  stx var_6134
  sty var_6133
  ldx var_00E3
  ldy var_00E2
  stx var_6136
  sty var_6135
  lda #$0C
  jsr sub_8DA2
  bcs loc_AAC9
  lda #$05
  jmp sub_8DA2

sub_AAA9:
  lda #$14
  jsr sub_8D7D
  bcs loc_AAC9
  lda var_6102
  sta var_00F0
  lda var_6103
  sta var_00E0
  lda var_6104
  sta var_00E1
  lda var_6105
  sta var_00E2
  lda var_6106
  sta var_00E3
loc_AAC9:
  rts
loc_AACA:
  lda #$02
  jsr sub_8A3B
  ldx #$62
  ldy #$E7
  lda var_6388
  jsr sub_A941
  jmp loc_AE60
loc_AADC:
  lda #$05
  jsr sub_8A3B
  ldx #$63
  ldy #$89
  lda var_63D9
  jsr sub_A941
  jmp loc_AE60

sub_AAEE:
  lda var_6292,y
  cmp #$2B
  bne loc_AB02
loc_AAF5:
  iny
  jsr sub_8B2F
  lda var_6292,y
  cmp #$2B
  beq loc_AAF5
  lda #$00
loc_AB02:
  rts

sub_AB03:
  lda #$14
  jsr sub_8D98
  bcc loc_AB10
  cmp #$02
  beq loc_AB27
  bne loc_AB2E
loc_AB10:
  lda #$05
  jsr sub_8DA2
  bcs loc_AB2E
  lda var_00FB
  bne loc_AB20
  jsr sub_AA33
  bne loc_AB2E
loc_AB20:
  lda var_6132
  and #$1F
  bne loc_AB30
loc_AB27:
  lda #$15
  jsr sub_8D98
  bcc loc_AB31
loc_AB2E:
  lda #$00
loc_AB30:
  sec
loc_AB31:
  rts

sub_AB32:
  jsr sub_8E7E
  jsr sub_A91A
  lda #$14
  jsr sub_8D7D
  bcs loc_AB74
  jsr sub_A8AD
  bne loc_AB4C
  lda #$14
  jsr sub_8A64
  jmp loc_AB62
loc_AB4C:
  jsr sub_A954
  bcs loc_AB6F

sub_AB51:
  jsr sub_A9D3
  bcs loc_AB6F
  lda var_6106
  cmp #$40
  beq loc_AB4C
  lda #$05
  jsr sub_8D8E
loc_AB62:
  lda #$07
  jsr sub_8D73
  bcc sub_AB32
  cmp #$12
  bne loc_AB74
  clc
  rts
loc_AB6F:
  lda #$05
  jsr sub_8D8E
loc_AB74:
  sec
  rts

; 内部命令 COPY [drive:][path]source [+[drive:][path]source ...] [[drive:][path][destination]] [/Y]：复制文件，/Y 表示覆盖时不询问。用法说明在 $A80E。
cmd_copy:
  jsr sub_8DB8
  bne loc_AB7F
  lda #$0D
  sec
  rts
loc_AB7F:
  jsr sub_8E22
  sta var_6388
  stx var_6101
  sta var_6416
  lda #$80
  jsr sub_8BEA
  bcs loc_ABCE
  jsr sub_8B9A
  bcc loc_AB99
  inc var_00F4
loc_AB99:
  ldx #$FF
loc_AB9B:
  inx
  lda var_62E7,x
  sta var_6445,x
  bne loc_AB9B
  lda var_00EA
  sta var_00E7
  jsr sub_8DB8
  jsr sub_AAEE
  bne loc_ABF3
  lda #$64
  sta var_00FD
  lda #$6D
  sta var_00FC
  inc var_00E0
  inc var_00F4
loc_ABBC:
  inc var_00E0
  jsr sub_8E22
  sty var_00FF
  ldy #$01
  sta (var_00FC),y
  ldy var_00FF
  lda #$80
  jsr sub_8BEA
loc_ABCE:
  bcs loc_AC3C
  sty var_00FF
  ldy #$2F
loc_ABD4:
  iny
  lda var_62B7,y
  sta (var_00FC),y
  bne loc_ABD4
  lda var_00FC
  clc
  adc #$58
  bcc loc_ABE5
  inc var_00FD
loc_ABE5:
  sta var_00FC
  ldy var_00FF
  jsr sub_8DB8
  beq loc_ABF3
  jsr sub_AAEE
  beq loc_ABBC
loc_ABF3:
  jsr sub_8E22
  sta var_63D9
  stx var_6131
  lda #$81
  jsr sub_8BEA
  bcs loc_AC3C
  jsr sub_8B9A
  bcc loc_AC0A
  inc var_00F3
loc_AC0A:
  ldx #$FF
loc_AC0C:
  inx
  lda var_62E7,x
  sta var_6338,x
  sta var_6389,x
  bne loc_AC0C
  ldx #$FF
loc_AC1A:
  inx
  lda var_6445,x
  sta var_62E7,x
  bne loc_AC1A
  lda var_00EA
  sta var_00EC
  sta var_00EB
  jsr sub_8DB8
  beq loc_AC33
  ldx #$09
  jmp sub_8CA5
loc_AC33:
  lda var_63DC
  cmp #$FE
  bcc loc_AC3E
  lda #$2F
loc_AC3C:
  sec
  rts
loc_AC3E:
  lda var_00F4
  bne loc_AC45
  jmp loc_ADD2
loc_AC45:
  lda var_00F3
  bne loc_AC4C
  jmp loc_ACE7
loc_AC4C:
  lda var_00E0
  beq loc_AC53
  jmp loc_ACEE
loc_AC53:
  lda var_6101
  sta var_0401
  lda #$01
  sta var_0405
  lda #$16
loc_AC60:
  jsr sub_8D69
  bcs loc_ACD4
  lda #$01
  sta var_00F1
  jsr sub_8E7E
  jsr sub_A88D
  jsr sub_A8AD
  beq loc_ACE1
  jsr sub_AAA9
  bcs loc_ACD4
  lda #$14
  jsr sub_8D98
  bcs loc_AC92
  lda #$05
  jsr sub_8DA2
  bcs loc_ACDB
  lda var_00FB
  bne loc_AC9B
  jsr sub_AA33
  beq loc_AC9E
  bne loc_ACD0
loc_AC92:
  ldx #$00
  stx var_6132
  cmp #$02
  bne loc_ACDB
loc_AC9B:
  jsr sub_A91A
loc_AC9E:
  lda var_6132
  and #$17
  beq loc_ACA8
  jmp loc_AADC
loc_ACA8:
  lda #$15
  jsr sub_8D98
  bcs loc_ACDB
loc_ACAF:
  jsr sub_A954
  bcs loc_ACDE
  jsr sub_A9D3
  bcs loc_ACDE
  lda var_6106
  cmp #$40
  beq loc_ACAF
  lda #$05
  jsr sub_8D8E
  jsr sub_AA84
  bcs loc_ACDE
  inc var_00E4
  bne loc_ACD0
  inc var_00E5
loc_ACD0:
  lda #$07
  bne loc_AC60
loc_ACD4:
  lda var_00F1
  bne loc_ACDB
  jmp loc_AACA
loc_ACDB:
  jmp loc_AE60
loc_ACDE:
  jmp loc_AE37
loc_ACE1:
  jsr sub_A91A
  jmp loc_AE44
loc_ACE7:
  lda var_00E0
  bne loc_ACEE
  jmp loc_AD96
loc_ACEE:
  lda var_00E0
  sta var_00E1
  lda #$64
  sta var_00FD
  lda #$15
  sta var_00FC
loc_ACFA:
  ldy #$01
  lda (var_00FC),y
  sta var_0401
  sta var_6101
  sta var_6388
  ldx var_00FD
  lda var_00FC
  clc
  adc #$30
  bcc loc_AD11
  inx
loc_AD11:
  tay
  stx var_00FD
  sty var_00FC
  stx var_0404
  sty var_0403
  ldy #$00
loc_AD1E:
  sty var_00E7
loc_AD20:
  lda (var_00FC),y
  sta var_62E7,y
  iny
  cmp #$5C
  beq loc_AD1E
  tax
  bne loc_AD20
  lda #$01
  sta var_0405
  lda #$16
  jsr sub_8D73
  bcc loc_AD40
  cmp #$12
  beq loc_AD7D
  jmp loc_ADC5
loc_AD40:
  lda var_00E2
  bne loc_AD78
  inc var_00E2
  jsr sub_8E7E
  lda var_00F3
  beq loc_AD50
  jsr sub_A88D
loc_AD50:
  jsr sub_A91A
  lda #$14
  jsr sub_8D7D
  bcs loc_AD70
  jsr sub_A954
  bcs loc_AD70
  jsr sub_AB03
  bcs loc_AD6B
  jsr sub_AB51
  bcs loc_ADC5
  bcc loc_AD7D
loc_AD6B:
  beq loc_ADCF
  jmp loc_AADC
loc_AD70:
  lda #$05
  jsr sub_8D8E
  jmp loc_ADC5
loc_AD78:
  jsr sub_AB32
  bcs loc_ADC5
loc_AD7D:
  lda var_00FC
  clc
  adc #$28
  sta var_00FC
  bcc loc_AD88
  inc var_00FD
loc_AD88:
  dec var_00E1
  beq loc_AD8F
  jmp loc_ACFA
loc_AD8F:
  lda var_00E2
  bne loc_ADB9
  jmp loc_AACA
loc_AD96:
  lda var_6101
  sta var_0401
  lda #$01
  jsr sub_8D64
  bcc loc_ADAA
  cmp #$12
  bne loc_ADCF
  jmp loc_AACA
loc_ADAA:
  jsr sub_AB03
  bcc loc_ADB4
  beq loc_ADCF
  jmp loc_AADC
loc_ADB4:
  jsr sub_AB32
  bcs loc_ADC5
loc_ADB9:
  lda #$05
  jsr sub_8DA2
  bcs loc_ADCA
  inc var_00E4
  jmp loc_AE60
loc_ADC5:
  lda #$05
  jsr sub_8DA2
loc_ADCA:
  lda #$17
  jsr sub_8D98
loc_ADCF:
  jmp loc_AE60
loc_ADD2:
  jsr sub_AAA9
  bcs loc_ADE2
  lda #$1E
  and var_00F0
  beq loc_ADE5
  lda #$05
  jsr sub_8D8E
loc_ADE2:
  jmp loc_AACA
loc_ADE5:
  jsr sub_A88D
  jsr sub_A8AD
  beq loc_AE3F
  jsr sub_A954
  bcs loc_AE5B
  lda #$14
  jsr sub_8D98
  bcs loc_AE16
  lda #$05
  jsr sub_8DA2
  lda var_00FB
  bne loc_AE07
  jsr sub_AA33
  bne loc_AE5B
loc_AE07:
  lda var_6132
  and #$17
  beq loc_AE1A
  lda #$05
  jsr sub_8D8E
  jmp loc_AADC
loc_AE16:
  cmp #$02
  bne loc_AE5B
loc_AE1A:
  lda #$15
  jsr sub_8D98
  bcs loc_AE5B
  jsr sub_A9D3
  bcs loc_AE37
loc_AE26:
  lda var_6106
  cmp #$40
  bne loc_AE4C
  jsr sub_A954
  bcs loc_AE37
  jsr sub_A9D3
  bcc loc_AE26
loc_AE37:
  lda #$05
  jsr sub_8DA2
  jmp loc_AE51
loc_AE3F:
  lda #$05
  jsr sub_8D8E
loc_AE44:
  lda #$13
  jsr sub_8A64
  jmp loc_AE60
loc_AE4C:
  jsr sub_AA84
  bcc loc_AE59
loc_AE51:
  lda #$17
  jsr sub_8D98
  jmp loc_AE5B
loc_AE59:
  inc var_00E4
loc_AE5B:
  lda #$05
  jsr sub_8D8E
loc_AE60:
  jsr sub_8D21
  lda #$11
  jmp sub_8A64
