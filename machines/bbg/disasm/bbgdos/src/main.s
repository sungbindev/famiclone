; 模块：main
; 没有归入其他模块的字节

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，$7C00-$B5E5
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
data_8000:
  .byte $00
data_8001:
  .byte $00
data_8002:
  .byte $00
data_8003:
  .byte $03
data_8004:
  .byte $00
data_8005:
  .byte $00
data_8006:
  .byte $03
data_8007:
  .byte $FE
data_8008:
  .byte $FE
data_8009:
  .byte $00
data_800A:
  .byte $00
data_800B:
  .byte $00
data_800C:
  .byte $00
data_800D:
  .byte $00
data_800E:
  .byte $00
data_800F:
  .byte $00
data_8010:
  .byte $00
data_8011:
  .byte $00
data_8012:
  .byte $00
data_8013:
  .byte $00
data_8014:
  .byte $00
data_8015:
  .byte $00
data_8016:
  .byte $00
data_8017:
  .byte $00
data_8018:
  .byte $00
data_8019:
  .byte $00
data_801A:
  .byte $00
data_801B:
  .byte $00
data_801C:
  .byte $00
data_801D:
  .byte $00
data_801E:
  .byte $00
data_801F:
  .byte $00
data_8020:
  .byte $00
data_8021:
  .byte $00
data_8022:
  .byte $00
data_8023:
  .byte $00
data_8024:
  .byte $00
data_8025:
  .byte $FF, $FF
data_8027:
  .byte $00
data_8028:
  .byte $00
data_8029:
  .byte $00
data_802A:
  .byte $00
data_802B:
  .byte $00
data_802C:
  .byte $00
data_802D:
  .byte $00
data_802E:
  .byte $FF
data_802F:
  .byte $00
data_8030:
  .byte $00
data_8031:
  .byte $00
data_8032:
  .res 62, $00
data_8070:
  .byte $00
data_8071:
  .byte $00
data_8072:
  .byte $00
data_8073:
  .byte $00
data_8074:
  .byte $00
data_8075:
  .byte $00
data_8076:
  .byte $00
data_8077:
  .byte $00
data_8078:
  .byte $00
data_8079:
  .byte $00
data_807A:
  .byte $00
data_807B:
  .byte $00
data_807C:
  .byte $00
data_807D:
  .byte $00
data_807E:
  .byte $00
data_807F:
  .byte $00
data_8080:
  .byte $00
data_8081:
  .byte $00
data_8082:
  .res 27, $00
data_809D:
  .byte $00

sub_809E:
  lda data_802E
  bmi loc_80AF
  ldy data_8001
  cpy #$03
  beq loc_80AF
  ldx data_8001
  dex
  txa
loc_80AF:
  sta var_00E6
  rts

sub_80B2:
  sta var_00E5
  lda #$06
  ldx #$00
  ldy #$E5
  jmp BIOS_ENTRY
loc_80BD:
  sta data_809D
  jsr sub_809E
  lda data_809D
  jsr sub_80B2
  cmp #$01
  bcc loc_810F
loc_80CD:
  ldx #$05
  stx data_800B
loc_80D2:
  lda data_809D
  jsr sub_80B2
  cmp #$01
  bcc loc_810F
  sta data_800C
  cmp #$03
  beq loc_80F0
  lda #$00
  jsr sub_80B2
  dec data_800B
  bne loc_80D2
  lda data_800C
loc_80F0:
  bit data_8030
  bpl loc_810C
  ldy #$1A
  jsr sub_98EE
  cpy #$01
  beq loc_80CD
  lda #$00
  bcc loc_810F
  jsr sub_9D99
  cpy #$03
  bcs loc_810C
  jmp loc_B270
loc_810C:
  lda #$1F
  sec
loc_810F:
  rts

sub_8110:
  ldy #$10
loc_8112:
  rol var_00E7
  rol var_00ED
  rol a
  cmp var_00E8
  bcc loc_811D
  sbc var_00E8
loc_811D:
  dey
  bne loc_8112
  rol var_00E7
  rol var_00ED
  rts

sub_8125:
  sty var_00ED
  lda data_8071
  sta var_00E8
  txa
  jsr sub_8110
  tax
  inx
  stx var_00E9
  lda data_8072
  sta var_00E8
  lda #$00
  jsr sub_8110
  sta var_00E8
  lda var_00ED
  lsr a
  ror a
  ror a
  ora var_00E9
  sta var_00E9
  rts

sub_814A:
  pha
  jsr sub_8125
  lda #$01
  sta var_00ED
  lda data_8007
  sta var_00EA
  pla
  jmp loc_80BD

sub_815B:
  bit data_8005
  bmi loc_8175
  ldx data_8071
  inx
  txa
  sec
  sbc var_00E9
  ldx var_00E8
  bne loc_8170
  clc
  adc data_8071
loc_8170:
  cmp data_8009
  bcc loc_8178
loc_8175:
  lda data_8009
loc_8178:
  sta var_00ED
  lda data_8009
  sec
  sbc var_00ED
  sta data_8009
  lda var_00FD
  jmp loc_80BD

sub_8188:
  sta data_8009
  ldy #$02
  lda (var_00EE),y
  sta var_00EA
  ldy #$04
  lda (var_00EE),y
  sta var_00EC
  dey
  lda (var_00EE),y
  sta var_00EB
  ldx data_8011
  ldy data_8010
  lda data_800F
  sta var_00E7
  jsr sub_8125
  jsr sub_815B
  bcs loc_81C9
loc_81AF:
  lda data_8009
  beq loc_81C9
  lda var_00ED
  asl a
  adc var_00EC
  sta var_00EC
  inc var_00E7
  ldx #$00
  stx var_00E8
  inx
  stx var_00E9
  jsr sub_815B
  bcc loc_81AF
loc_81C9:
  rts

sub_81CA:
  cmp data_801E
  bne sub_81DB
  cpx data_801D
  bne sub_81DB
  cpy data_801C
  bne sub_81DB
  clc
  rts

sub_81DB:
  jsr sub_85DB
  bcs loc_8208
  sta data_801E
  stx data_801D
  sty data_801C
  lda #$02

sub_81EB:
  ldx #$92
  ldy #$00
  stx var_00EC
  sty var_00EB
  ldy data_801C
  sty var_00E7
  ldx data_801E
  ldy data_801D
  jsr sub_814A
  bcc loc_8208
  ldy #$FF
  sty data_801E
loc_8208:
  rts

sub_8209:
  lda data_8005

sub_820C:
  ldx #$00
  tay
  bpl loc_8213
  ldx #$A2
loc_8213:
  lsr a
  bcc loc_821A
  txa
  adc #$50
  tax
loc_821A:
  rts

sub_821B:
  jsr sub_8209
  ldy #$00
loc_8220:
  lda data_9C33,x
  sta data_8032,y
  inx
  iny
  cpy #$50
  bne loc_8220
  lda data_9C33,x
  rts

sub_8230:
  lda data_8005
  cmp data_802E
  bne loc_824E
  clc
  tay
  bmi loc_824D
  jsr sub_809E
  lda #$01
  jsr sub_80B2
  clc
  tya
  bpl loc_824D
  jsr sub_9D77
  bcc loc_8288
loc_824D:
  rts
loc_824E:
  jsr sub_9D77
  bcs loc_824D
  lda data_8005
  sta data_802E
  bmi loc_8281
  ldx data_8001
  cpx #$03
  beq loc_8276
  cmp data_802F
  beq loc_8276
  ldx #$00
  sta data_802F
  tay
  jsr var_581B
  jsr sub_8209
  jmp loc_8288
loc_8276:
  jsr sub_809E
  lda #$01
  jsr sub_80B2
  tya
  bmi loc_8288
loc_8281:
  jsr sub_821B
  beq loc_828B
  clc
  rts
loc_8288:
  jsr sub_821B
loc_828B:
  lda #$00
  sta data_807B
  sta data_807A
  ldx data_8005
  lda #$FF
  sta data_8025,x
loc_829B:
  ldx #$FF
  stx data_8072
  stx data_8071
  inx
  txa
  tay
  jsr sub_81DB
  bcs loc_82E0
  lda data_9200
  cmp #$E9
  beq loc_82BD
  cmp #$EB
  bne loc_82C6
  lda data_9202
  cmp #$90
  bne loc_82C6
loc_82BD:
  lda data_9215
  and #$F0
  cmp #$F0
  beq loc_82E4
loc_82C6:
  jsr sub_9D99
  bit data_8030
  bpl loc_82E0
  ldy #$1A
  ldx #$0C
  jsr CRITICAL_ERROR_ENTRY
  cpy #$01
  beq loc_829B
  cpy #$03
  beq loc_82E0
  jmp loc_B270
loc_82E0:
  lda #$1F
  sec
  rts
loc_82E4:
  lda data_920D
  sta data_8070
  lda data_9211
  clc
  adc #$0F
  bcc loc_82F5
  inc data_9212
loc_82F5:
  ldx #$04
loc_82F7:
  lsr data_9212
  ror a
  dex
  bne loc_82F7
  sta data_8079
  lda data_9218
  sta data_8071
  lda data_921A
  sta data_8072
  lda data_920E
  sta data_8075
  clc
  adc data_9216
  sta data_8076
  dec data_9210
  beq loc_8322
  adc data_9216
loc_8322:
  sta data_8077
  tay
  lda #$00
  sta data_802D
  adc #$00
  sta data_8078
  tax
  tya
  adc data_8079
  sta data_807D
  txa
  adc #$00
  sta data_807E
  ldx data_9214
  ldy data_9213
  tya
  ora data_9214
  beq loc_834E
  lda #$00
  beq loc_8357
loc_834E:
  lda data_9222
  ldx data_9221
  ldy data_9220
loc_8357:
  sta var_00F0
  tya
  sec
  sbc data_807D
  sta data_8073
  txa
  sbc data_807E
  sta data_8074
  bcs loc_836C
  dec var_00F0
loc_836C:
  lda data_8070
loc_836F:
  lsr a
  beq loc_837D
  lsr var_00F0
  ror data_8074
  ror data_8073
  jmp loc_836F
loc_837D:
  lda data_8073
  adc #$01
  sta data_8073
  bcc loc_838A
  inc data_8074
loc_838A:
  ldx #$0F
  cpx data_8074
  bcc loc_8397
  bne loc_8399
  cmp #$F0
  bcc loc_8399
loc_8397:
  ldx #$FF
loc_8399:
  stx data_807F
  ldx #$00
  ldy #$02
  stx data_8081
  sty data_8080
  clc
  rts

sub_83A8:
  jsr sub_8230
  bcs loc_83F2
  ldx data_8005
  bmi loc_83EC
  lda data_8025,x
  beq loc_83EC
  inc data_8025,x
  lda data_8071
  sta var_00E9
  lda #$06
  jsr sub_80B2
  lda data_8032
  beq loc_83E4
  lda data_8007
  ldx #$80
  ldy #$32
  sta data_8029
  stx data_8028
  sty data_8027
  ldx #$FF
  jsr sub_A28A
  bcc loc_83E7
  cmp #$03
  bne loc_83F1
loc_83E4:
  jsr sub_A27B
loc_83E7:
  jsr sub_A316
  clc
  rts
loc_83EC:
  jsr sub_A2F8
  clc
  rts
loc_83F1:
  sec
loc_83F2:
  lda #$03
  rts

sub_83F5:
  lda #$05
  clc
  adc var_00EE
  sta var_00F4
  lda #$00
  adc var_00EF
  sta var_00F5
  rts

sub_8403:
  asl a
  sta var_00F1
  lda #$94
  adc #$00
  sta var_00F2
  rts

sub_840D:
  lda #$03
  pha
  lda data_801A
  ldx data_8019
  ldy data_8018
  jmp loc_843C

sub_841C:
  cmp data_801A
  bne loc_842D
  cpx data_8019
  bne loc_8430
  cpy data_8018
  bne loc_8433
  clc
  rts
loc_842D:
  sta data_801A
loc_8430:
  stx data_8019
loc_8433:
  sty data_8018
  lda #$02
  pha
  lda data_801A
loc_843C:
  sty var_00E7
  pha
  txa
  tay
  pla
  tax
  lda #$94
  sta var_00EC
  lda #$00
  sta var_00EB
  pla
  jsr sub_814A
  bcc loc_8456
  ldy #$FF
  sty data_801A
loc_8456:
  rts

sub_8457:
  pha
  jsr sub_83F5
  lda #$00
  sta data_8022
  sta var_00FA
  lda data_8070
  sta var_00FB
  pla
  and #$08
  beq loc_8471
  bit data_802D
  bmi sub_8480
loc_8471:
  lda data_8079
  sta var_00FB
  lda #$00
  ldx data_8078
  ldy data_8077
  clc
  rts

sub_8480:
  ldx data_802C
  ldy data_802B

sub_8486:
  tya
  sec
  sbc #$02
  sta var_00F1
  tay
  txa
  sbc #$00
  sta var_00F2
  cmp data_8074
  beq loc_8499
  bcc loc_84A1
loc_8499:
  cpy data_8073
  bcc loc_84A1
  lda #$1B
  rts
loc_84A1:
  lda #$00
  sta var_00F0
  lda data_8070
loc_84A8:
  lsr a
  beq loc_84B3
  asl var_00F1
  rol var_00F2
  rol var_00F0
  bcc loc_84A8
loc_84B3:
  lda var_00F1
  clc
  adc data_807D
  tay
  sta data_800F
  lda var_00F2
  adc data_807E
  sta data_8010
  tax
  lda var_00F0
  adc #$00
  sta data_8011
  rts

sub_84CE:
  ldx #$FF
  stx var_00FE

sub_84D2:
  sta data_8023
  sty data_8024
  lda #$08
  jsr sub_8457
  bcs loc_84F4
loc_84DF:
  jsr sub_841C
  bcs loc_84F4
  jsr sub_8547
  bcc loc_84F4
  bne loc_84F1
  jsr sub_84F5
  bcc loc_84DF
  rts
loc_84F1:
  lda #$02
  sec
loc_84F4:
  rts

sub_84F5:
  lda data_801A
  ldx data_8019
  ldy data_8018

sub_84FE:
  clc
  iny
  bne loc_8507
  inx
  bne loc_8507
  adc #$01
loc_8507:
  dec var_00FB
  bne loc_8546
  bit data_802D
  bpl loc_8543
  lda data_802C
  ldy data_802B
  sta var_00F7
  sty var_00F6
  ldx #$00
loc_851C:
  stx var_00F8
  tax
  ldy var_00F6
  jsr sub_8667
  bcs loc_8546
  jsr sub_8885
  bcs loc_8546
  ldx var_00F8
  cpx data_8022
  inx
  bcc loc_851C
  stx data_8022
  jsr sub_8652
  bcs loc_8543
  lda data_8070
  sta var_00FB
  jmp sub_8486
loc_8543:
  lda #$02
  sec
loc_8546:
  rts

sub_8547:
  jsr sub_8403
  ldy #$00
  lda (var_00F1),y
  beq sub_8589
  cmp #$E5
  beq loc_8578
  cmp #$05
  bne loc_855A
  lda #$E5
loc_855A:
  cmp (var_00F4),y
  bne loc_857B
  iny
  lda (var_00F1),y
  cpy #$0B
  bne loc_855A
  lda (var_00F1),y
  ora #$20
  bit data_8023
  beq loc_8585
  bit data_8024
  bne loc_8585
  sta data_8023
loc_8576:
  clc
  rts
loc_8578:
  jsr sub_8589
loc_857B:
  lda var_00FA
  clc
  adc #$10
  sta var_00FA
  bne sub_8547
  rts
loc_8585:
  lda #$05
  sec
  rts

sub_8589:
  ldx var_00FE
  inx
  bne loc_859D
  lda var_00FA
  sta var_00FE
  ldx #$02
loc_8594:
  lda data_8018,x
  sta data_801F,x
  dex
  bpl loc_8594
loc_859D:
  sec
  rts

sub_859F:
  lda data_8016
  beq loc_8576
  inc data_8016
  ldx #$96
  ldy #$00
  stx var_00EC
  sty var_00EB
  ldx #$00
  lda data_801B
  sta var_00E7
  lda #$03
  jsr sub_814A
  bcs loc_859D
  ldx #$00
  ldy #$00
  lda data_801B
  sec
  sbc data_8075
  clc
  adc data_8076
  sta var_00E7
  bcc loc_85D1
  iny
loc_85D1:
  lda #$03
  jmp sub_814A

sub_85D6:
  jsr sub_859F
  bcs loc_85F7

sub_85DB:
  bit data_8017
  bpl loc_8576
  inc data_8017
  pha
  txa
  pha
  tya
  pha
  lda #$03
  jsr sub_81EB
  pla
  tay
  pla
  tax
  pla
  bcc loc_85F7
  lda data_800C
loc_85F7:
  rts

sub_85F8:
  pha
  jsr sub_859F
  pla
  bcs loc_8615
  ldx #$96
  ldy #$00
  stx var_00EC
  sty var_00EB
  sta data_801B
  sta var_00E7
  ldx #$00
  lda #$02
  jsr sub_814A
  bcc loc_861A
loc_8615:
  ldy #$FF
  sty data_801B
loc_861A:
  rts

sub_861B:
  tya
  asl a
  tay
  txa
  rol a
  tax
  tya
  adc var_00F6
  tay
  txa
  adc var_00F7
  tax
  lsr a
  lsr a
  rts

sub_862C:
  ldx var_00F7
  ldy var_00F6
  jsr sub_8667
  bcs loc_8651
  beq loc_8651
  sty var_00F0

sub_8639:
  ldy #$15
  lda var_00F4
  sta (var_00EE),y
  iny
  lda var_00F5
  sta (var_00EE),y
  ldy #$17
  lda var_00F6
  sta (var_00EE),y
  iny
  lda var_00F7
  sta (var_00EE),y
  ldy var_00F0
loc_8651:
  rts

sub_8652:
  ldx var_00F7
  ldy var_00F6
  jmp sub_865F

sub_8659:
  ldx data_868F
  ldy data_868E

sub_865F:
  cpx data_807F
  bcc loc_8666
  cpy #$F0
loc_8666:
  rts

sub_8667:
  txa
  bit data_807F
  bmi loc_8670
  jsr sub_861B
loc_8670:
  clc
  adc data_8075
  cmp data_801B
  clc
  beq loc_8689
  stx var_00F0
  sty var_00F3
  jsr sub_85F8
  ldx var_00F0
  ldy var_00F3
  bcs loc_8689
  lda #$01
loc_8689:
  rts
data_868A:
  .byte $F0
data_868B:
  .byte $00, $0F
data_868D:
  .byte $00
data_868E:
  .byte $00
data_868F:
  .byte $00

sub_8690:
  lda #$FF
  sta var_00FC
  lda data_807F
  sta data_868F
  lda #$FF
  sta data_868E
  ldx data_885F
  ldy data_885E

sub_86A5:
  stx var_00F5
  sty var_00F4
  txa
  ora var_00F4
  bne loc_8702
  bit var_00FC
  bpl loc_86E0
  jsr sub_8659
  bcc loc_86BF
  lda #$00
  sta data_868E
  sta data_868F
loc_86BF:
  ldy #$0B
  lda (var_00EE),y
  tax
  lda data_868E
  sta (var_00EE),y
  stx data_868E
  iny
  lda (var_00EE),y
  tax
  lda data_868F
  sta (var_00EE),y
  stx data_868F
  ldy #$19
  lda #$01
  sta (var_00EE),y
  bne loc_86ED
loc_86E0:
  ldy #$0B
  lda (var_00EE),y
  sta data_868E
  iny
  lda (var_00EE),y
  sta data_868F
loc_86ED:
  lda data_868E
  ora data_868F
  bne loc_8700
  lda data_807F
  sta data_868F
  lda #$FF
  sta data_868E
loc_8700:
  clc
  rts
loc_8702:
  bit data_807F
  bpl loc_875A
  txa
  clc
  adc data_8075
  cmp data_801B
  beq loc_8716
  jsr sub_85F8
  bcs loc_8759
loc_8716:
  ldx #$96
  ldy #$00
  bit var_00F4
  bpl loc_871F
  inx
loc_871F:
  lda var_00F4
  stx var_00F5
  sty var_00F4
  asl a
  tay
  bit var_00FC
  bpl loc_874D
  lda (var_00F4),y
  tax
  lda data_868E
  sta (var_00F4),y
  stx data_868E
  iny
  lda (var_00F4),y
  tax
  lda data_868F
  sta (var_00F4),y
  stx data_868F
  lda #$FF
  sta data_8016
  ldy #$1A
  sta (var_00EE),y
  clc
  rts
loc_874D:
  lda (var_00F4),y
  sta data_868E
  iny
  lda (var_00F4),y
  sta data_868F
  clc
loc_8759:
  rts
loc_875A:
  tya
  asl a
  tay
  txa
  rol a
  tax
  tya
  adc var_00F4
  sta var_00F0
  txa
  adc var_00F5
  lsr a
  sta var_00F3
  ror var_00F0
  lsr a
  clc
  adc data_8075
  cmp data_801B
  beq loc_877C
  jsr sub_85F8
  bcs loc_87DB
loc_877C:
  lda var_00F4
  and #$01
  sta data_868D
  beq loc_8790
  ldx #$04
loc_8787:
  asl data_868E
  rol data_868F
  dex
  bne loc_8787
loc_8790:
  ldx #$96
  ldy #$00
  stx var_00F5
  sty var_00F4
  lda var_00F3
  ldy var_00F0
  lsr a
  bcc loc_87A1
  inc var_00F5
loc_87A1:
  lda (var_00F4),y
  bit var_00FC
  bpl loc_87C1
  pha
  ldx data_868D
  and data_868B,x
  ora data_868E
  sta (var_00F4),y
  tya
  tax
  lda #$FF
  sta data_8016
  ldy #$1A
  sta (var_00EE),y
  txa
  tay
  pla
loc_87C1:
  sta data_868E
  iny
  bne loc_87DE
  lda #$96
  cmp var_00F5
  beq loc_87DC
  dec var_00F5
  ldx data_801B
  inx
  txa
  jsr sub_85F8
  ldy #$00
  bcc loc_87DE
loc_87DB:
  rts
loc_87DC:
  inc var_00F5
loc_87DE:
  lda (var_00F4),y
  bit var_00FC
  bpl loc_87FA
  pha
  ldx data_868D
  and data_868A,x
  ora data_868F
  sta (var_00F4),y
  lda #$FF
  sta data_8016
  ldy #$1A
  sta (var_00EE),y
  pla
loc_87FA:
  sta data_868F
  lda data_868D
  beq loc_880F
  ldx #$04
loc_8804:
  lsr data_868F
  ror data_868E
  dex
  bne loc_8804
  clc
  rts
loc_880F:
  lda data_868F
  and #$0F
  sta data_868F
  clc
  rts

sub_8819:
  ldx data_8081
  ldy data_8080
  iny
  bne loc_8823
  inx
loc_8823:
  cpx data_8074
  bne loc_8831
  cpy data_8073
  bne loc_8831
  ldx #$00
  ldy #$02
loc_8831:
  stx data_8081
  sty data_8080
  cpx var_00F7
  bne loc_883D
  cpy var_00F6
loc_883D:
  rts

sub_883E:
  lda #$00
  sta var_00FC
  ldx data_8081
  ldy data_8080
loc_8848:
  jsr sub_86A5
  bcs loc_885D
  lda data_868E
  ora data_868F
  beq loc_885D
  jsr sub_8819
  bne loc_8848
  lda #$27
  sec
loc_885D:
  rts
data_885E:
  .byte $00
data_885F:
  .byte $00

sub_8860:
  jsr sub_883E
  bcs loc_8884
  lda #$FF
  sta var_00FC
  ldx data_885F
  ldy data_885E
  lda data_8081
  sta data_885F
  sta data_868F
  lda data_8080
  sta data_885E
  sta data_868E
  jsr sub_86A5
loc_8884:
  rts

sub_8885:
  lda #$00

sub_8887:
  sta var_00F0
  bit data_807F
  bpl loc_8896
  lsr var_00F7
  tya
  asl a
  tay
  jmp loc_8899
loc_8896:
  jsr sub_88DF
loc_8899:
  bcs loc_88A8
  lda data_9600,y
  sta var_00F6
  iny
  beq loc_88C5
  lda data_9600,y
  bcc loc_88C8
loc_88A8:
  lda data_9700,y
  sta var_00F6
  iny
  bne loc_88C5
  bit var_00F0
  bmi loc_88DE
  ldx data_801B
  inx
  txa
  jsr sub_85F8
  bcs loc_88DE
  inc var_00F0
  lda data_9600
  bcc loc_88C8
loc_88C5:
  lda data_9700,y
loc_88C8:
  bit var_00F7
  bpl loc_88D8
  lsr a
  ror var_00F6
  lsr a
  ror var_00F6
  lsr a
  ror var_00F6
  lsr a
  ror var_00F6
loc_88D8:
  and data_807F
  sta var_00F7
  clc
loc_88DE:
  rts

sub_88DF:
  txa
  lsr a
  tax
  tya
  ror a
  tay
  ror var_00F7
  txa
  lsr a
  rts

sub_88EA:
  pha
  ldy #$0A
  lda (var_00EE),y
  sta var_00F3
  dey
  lda (var_00EE),y
  sta var_00F2
  dey
  lda (var_00EE),y
  sta var_00F1
  lda data_8070
loc_88FE:
  lsr var_00F3
  ror var_00F2
  ror var_00F1
  lsr a
  bne loc_88FE
  lda var_00F1
  ldy #$15
  cmp (var_00EE),y
  iny
  lda var_00F2
  sbc (var_00EE),y
  bcs loc_891E
  lda #$00
  sta var_00F5
  sta var_00F4
  ldy #$0B
  bne loc_8929
loc_891E:
  lda (var_00EE),y
  sta var_00F5
  dey
  lda (var_00EE),y
  sta var_00F4
  ldy #$17
loc_8929:
  lda (var_00EE),y
  sta var_00F6
  iny
  lda (var_00EE),y
  sta var_00F7
  pla
  beq loc_893A
  jsr sub_862C
  bcs loc_8964
loc_893A:
  lda var_00F4
  cmp var_00F1
  bne loc_8946
  lda var_00F5
  cmp var_00F2
  beq loc_8965
loc_8946:
  jsr sub_862C
  bcs loc_8964
  jsr sub_8885
  bcs loc_8964
  inc var_00F4
  bne loc_8956
  inc var_00F5
loc_8956:
  lda var_00F0
  beq loc_895D
  jsr sub_8639
loc_895D:
  jsr sub_8652
  bcc loc_893A
  lda #$00
loc_8964:
  rts
loc_8965:
  ldx var_00F7
  ldy var_00F6
  jsr sub_8486
  lda data_8070
  asl a
  sbc #$00
  ldy #$08
  and (var_00EE),y
  lsr a
  tax
  clc
  adc data_800F
  sta data_800F
  bcc loc_898A
  clc
  inc data_8010
  bne loc_898A
  inc data_8011
loc_898A:
  rts

sub_898B:
  sta data_8009
  lda data_8070
  asl a
  sec
  sbc #$01
  ldy #$08
  and (var_00EE),y
  lsr a
  clc
  adc data_8009
  adc data_8070
  sbc #$00
  sta data_8009
  lda data_8070
loc_89A9:
  lsr a
  beq loc_89B1
  lsr data_8009
  bne loc_89A9
loc_89B1:
  lda #$01
  jsr sub_88EA
  bcc loc_89BB
  cmp #$01
  rts
loc_89BB:
  stx var_00ED
  lda #$01
  sta data_800A
loc_89C2:
  dec data_8009
  beq loc_8A01
  ldx var_00F7
  ldy var_00F6
  jsr sub_865F
  bcs loc_8A01
  stx var_00F9
  sty var_00F8
  txa
  bit data_807F
  bmi loc_89DE
  jsr sub_861B
  clc
loc_89DE:
  adc data_8075
  cmp data_801B
  bne loc_8A01
  lda #$80
  jsr sub_8887
  bcs loc_8A01
  tax
  ldy var_00F6
  bne loc_89F3
  dex
loc_89F3:
  dey
  cpy var_00F8
  bne loc_8A01
  cpx var_00F9
  bne loc_8A01
  inc data_800A
  bcs loc_89C2
loc_8A01:
  lda data_8070
loc_8A04:
  lsr a
  beq loc_8A0D
  asl data_800A
  jmp loc_8A04
loc_8A0D:
  lda data_800A
  sec
  sbc var_00ED
  clc
  rts

sub_8A15:
  sta data_8008
  cld
  txa
  pha
  ldx #$1A
loc_8A1D:
  lda var_00E5,x
  sta data_8082,x
  dex
  bpl loc_8A1D
  pla
  sta data_800E
  sty data_800D
  cmp #$80
  bcc loc_8A58
  cmp #$C0
  bcs loc_8A58
  sta var_00F2
  sty var_00F1
  lda data_8008
  sta var_00F0
  ldx #$07
  ldy #$00
  stx var_00F5
  sty var_00F4
  ldx #$00
  ldy #$30
  stx var_00F7
  sty var_00F6
  lda #$11
  ldx #$F0
  jsr BIOS_ENTRY
  lda #$07
  ldy #$00
loc_8A58:
  sta var_00EF
  sty var_00EE
  rts
loc_8A5D:
  pha
  lda data_800E
  cmp #$80
  bcc loc_8A8C
  cmp #$C0
  bcs loc_8A8C
  sta var_00F2
  lda data_800D
  sta var_00F1
  lda data_8008
  sta var_00F0
  ldx #$07
  ldy #$00
  stx var_00F5
  sty var_00F4
  ldx #$00
  ldy #$30
  stx var_00F7
  sty var_00F6
  lda #$11
  ldx #$F0
  jsr BIOS_ENTRY
loc_8A8C:
  ldx #$1A
loc_8A8E:
  lda data_8082,x
  sta var_00E5,x
  dex
  bpl loc_8A8E
  pla
  cmp #$01
  tay
  lda #$01
  ldx data_8008
  jmp BIOS_ENTRY

sub_8AA2:
  ldy #$0A
loc_8AA4:
  lda (var_00F4),y
  sta (var_00F1),y
  dey
  bne loc_8AA4
  lda (var_00F4),y
  cmp #$E5
  bne loc_8AB3
  lda #$05
loc_8AB3:
  sta (var_00F1),y
  rts

sub_8AB6:
  ldy #$0C
  lda #$00
loc_8ABA:
  sta (var_00F1),y
  iny
  cpy #$20
  bne loc_8ABA
  txa

sub_8AC2:
  ldy #$0B
  sta (var_00F1),y
  lda #$3F
  jsr BIOS_ENTRY
  pha
  tya
  ldy #$16
  sta (var_00F1),y
  iny
  txa
  sta (var_00F1),y
  iny
  lda var_00FF
  sta (var_00F1),y
  iny
  pla
  sta (var_00F1),y
  rts

sub_8ADF:
  ldy #$00
  tya
loc_8AE2:
  sta data_9200,y
  iny
  bne loc_8AE2
loc_8AE8:
  sta data_9300,y
  iny
  bne loc_8AE8
  rts

sub_8AEF:
  lda data_8021
  ldx data_8020
  ldy data_801F
  jsr sub_841C
  bcs loc_8B0B
  lda var_00FE
  sta var_00FA
  jsr sub_8403
  jsr sub_83F5
  jsr sub_8AA2
  clc
loc_8B0B:
  rts

sub_8B0C:
  lda #$00
  sta var_00FC
  ldx data_802C
  ldy data_802B
loc_8B16:
  stx data_885F
  sty data_885E
  jsr sub_86A5
  bcs loc_8B8C
  jsr sub_8659
  bcc loc_8B16
  ldx data_8081
  ldy data_8080
  stx var_00F7
  sty var_00F6
  jsr sub_8860
  bcc loc_8B40
  cmp #$27
  sec
  bne loc_8B8C
  sta data_8031
  lda #$05
  rts
loc_8B40:
  jsr sub_8819
  jsr sub_8690
  bcs loc_8B8C
  jsr sub_85D6
  ldx data_885F
  ldy data_885E
  jsr sub_8486
  bcs loc_8B8C
  sta data_8021
  stx data_8020
  sty data_801F
  sta data_801E
  stx data_801D
  sty data_801C
  lda #$00
  sta var_00FE
  jsr sub_8ADF
  ldx data_8070
  stx var_00F3
loc_8B74:
  lda #$03
  jsr sub_81EB
  bcs loc_8B8C
  inc data_801C
  bne loc_8B88
  inc data_801D
  bne loc_8B88
  inc data_801E
loc_8B88:
  dec var_00F3
  bne loc_8B74
loc_8B8C:
  rts

sub_8B8D:
  lda #$27
  ldy #$18

sub_8B91:
  jsr sub_84CE
  bcs loc_8C07
  ldy #$0B
  lda (var_00F1),y
  ldy #$02
  sta (var_00EE),y
  ldy #$19
loc_8BA0:
  lda (var_00F1),y
  pha
  dey
  cpy #$15
  bne loc_8BA0
  ldy #$05
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
  ldy #$03
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
loc_8BBA:
  ldy #$16
  lda #$FF
  sta (var_00EE),y
  dey
  sta (var_00EE),y
  lda #$00
  ldy #$19
  sta (var_00EE),y
  ldy #$1A
  sta (var_00EE),y
  ldy #$1F
loc_8BCF:
  lda (var_00F1),y
  pha
  dey
  cpy #$1A
  bcs loc_8BCF
  ldy #$0B
loc_8BD9:
  pla
  sta (var_00EE),y
  iny
  cpy #$11
  bne loc_8BD9
  ldy #$07
  lda #$00
loc_8BE5:
  sta (var_00EE),y
  iny
  cpy #$0B
  bne loc_8BE5

sub_8BEC:
  ldy #$11
  lda var_00FA
  sta (var_00EE),y
  iny
  lda data_8018
  sta (var_00EE),y
  iny
  lda data_8019
  sta (var_00EE),y
  iny
  lda data_801A
  sta (var_00EE),y
  lda #$00
  clc
loc_8C07:
  rts

sub_8C08:
  clc
  lda (var_00EE),y
  adc data_8012
  sta (var_00EE),y
  iny
  lda (var_00EE),y
  adc data_8013
  sta (var_00EE),y
loc_8C18:
  rts

sub_8C19:
  lda #$00
  jsr sub_88EA
  bcs loc_8C18
  lda data_8011
  ldx data_8010
  ldy data_800F
  jsr sub_81CA
  bcs loc_8C18
  lda data_8014
  sec
  sbc data_8012
  sta data_8014
  lda data_8015
  sbc data_8013
  sta data_8015
  lda var_00FD
  cmp #$02
  beq loc_8C74
  lda #$FF
  sta data_8017
  lda data_8007
  sta var_00F3
  ldy #$07
  lda (var_00EE),y
  sta var_00F4
  iny
  lda (var_00EE),y
  and #$01
  ora #$92
  sta var_00F5
  ldy #$02
  lda (var_00EE),y
  sta var_00F0
  ldy #$03
  lda (var_00EE),y
  sta var_00F1
  iny
  lda (var_00EE),y
  sta var_00F2
  jmp loc_8C99
loc_8C74:
  lda data_8007
  sta var_00F0
  ldy #$07
  lda (var_00EE),y
  sta var_00F1
  iny
  lda (var_00EE),y
  and #$01
  ora #$92
  sta var_00F2
  ldy #$02
  lda (var_00EE),y
  sta var_00F3
  ldy #$03
  lda (var_00EE),y
  sta var_00F4
  iny
  lda (var_00EE),y
  sta var_00F5
loc_8C99:
  lda data_8012
  sta var_00F6
  lda data_8013
  sta var_00F7
  lda #$0C
  ldx #$F0
  jsr BIOS_ENTRY
  ldy #$03
  jsr sub_8C08
  ldy #$05
  jsr sub_8C08
  ldy #$07
  jsr sub_8C08

sub_8CB9:
  ldx #$02
  iny
  lda (var_00EE),y
  adc #$00
  .byte $91
  inc var_D0CA
  inc var_0060,x

sub_8CC6:
  ldx data_8015
  ldy data_8014
  bne loc_8CCF
  dex
loc_8CCF:
  dey
  tya
  ldy #$07
  clc
  adc (var_00EE),y
  iny
  txa
  adc (var_00EE),y
  sta a:var_00F8
  iny
  lda #$00
  adc (var_00EE),y
  sta a:var_00F9
  iny
  lda #$00
  adc (var_00EE),y
  sta var_00F0
  lda data_8070
loc_8CEF:
  lsr var_00F0
  ror a:var_00F9
  ror a:var_00F8
  lsr a
  bne loc_8CEF
  lda a:var_00F8
  ldy #$15
  cmp (var_00EE),y
  iny
  lda a:var_00F9
  sbc (var_00EE),y
  bcs loc_8D14
  ldx #$FF
  stx var_00F2
  stx var_00F1
  inx
  ldy #$00
  beq loc_8D28
loc_8D14:
  ldy #$16
  lda (var_00EE),y
  sta var_00F2
  dey
  lda (var_00EE),y
  sta var_00F1
  ldy #$18
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  tay
loc_8D28:
  lda #$00
  sta var_00FC
loc_8D2C:
  lda var_00F9
  cmp var_00F2
  bne loc_8D38
  lda var_00F8
  cmp var_00F1
  beq loc_8D7C
loc_8D38:
  stx data_885F
  sty data_885E
  jsr sub_86A5
  bcs loc_8D7D
  jsr sub_8659
  bcs loc_8D51
  inc var_00F1
  bne loc_8D2C
  inc var_00F2
  jmp loc_8D2C
loc_8D51:
  ldx data_8081
  ldy data_8080
  stx var_00F7
  sty var_00F6
loc_8D5B:
  jsr sub_8860
  bcs loc_8D7D
  inc var_00F1
  bne loc_8D66
  inc var_00F2
loc_8D66:
  lda var_00F1
  cmp var_00F8
  lda var_00F2
  sbc var_00F9
  bcs loc_8D79
  jsr sub_8819
  bne loc_8D5B
  lda #$27
  sec
  rts
loc_8D79:
  jmp sub_8690
loc_8D7C:
  clc
loc_8D7D:
  rts

sub_8D7E:
  ldy #$10
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  ldy #$07
  sec
  sbc (var_00EE),y
  sta var_00F0
  txa
  iny
  sbc (var_00EE),y
  sta var_00F1
  pla
  iny
  sbc (var_00EE),y
  tax
  pla
  iny
  sbc (var_00EE),y
  rts

sub_8DA5:
  ldy #$06
  lda (var_00EE),y
  sta data_8015
  lda #$00
  sta (var_00EE),y
  dey
  lda (var_00EE),y
  sta data_8014
  pha
  lda #$00
  sta (var_00EE),y
  pla
  ora data_8015
  rts
  jsr sub_8230
  bcs loc_8E0B
  jsr sub_8D7E
  sta var_00FE
  jsr sub_8DA5
  bcc loc_8DF5
  beq loc_8E09
  lda var_00FE
  bne loc_8DF0
  txa
  bne loc_8DF0
  ldx var_00F1
  ldy var_00F0
  cpy data_8014
  txa
  sbc data_8015
  bcs loc_8DF0
  txa
  ora var_00F0
  beq loc_8E0B
  stx data_8015
  sty data_8014
loc_8DF0:
  lda #$02
  jmp sub_8E4D
loc_8DF5:
  ldy #$0D
loc_8DF7:
  lda (var_00EE),y
  pha
  iny
  cmp #$11
  bcc loc_8DF7
  ldy #$0A
loc_8E01:
  pla
  sta (var_00EE),y
  dey
  cpy #$07
  bcs loc_8E01
loc_8E09:
  lda #$00
loc_8E0B:
  rts
  jsr sub_8230
  bcs loc_8E4A
  jsr sub_8DA5
  bne loc_8E1C
  jsr sub_8EEB
  bcc loc_8E48
  rts
loc_8E1C:
  jsr sub_8D7E
  bcc loc_8E32
  bne loc_8E4B
  txa
  bne loc_8E4B
  lda data_8014
  cmp var_00F0
  lda data_8015
  sbc var_00F1
  bcc loc_8E4B
loc_8E32:
  jsr sub_8CC6
  bcs loc_8E44
  lda #$03
  jsr sub_8E4D
  bcs loc_8E43
  jsr sub_8F46
  lda #$00
loc_8E43:
  rts
loc_8E44:
  cmp #$27
  bne loc_8E4A
loc_8E48:
  lda #$00
loc_8E4A:
  rts
loc_8E4B:
  lda #$03

sub_8E4D:
  sta var_00FD
  ldy #$07
  lda (var_00EE),y
  eor #$FF
  tax
  iny
  lda (var_00EE),y
  eor #$FF
  and #$01
  tay
  inx
  bne loc_8E66
  iny
  cpy #$02
  beq loc_8E81
loc_8E66:
  cpy data_8015
  bne loc_8E6E
  cpx data_8014
loc_8E6E:
  bcc loc_8E76
  ldy data_8015
  ldx data_8014
loc_8E76:
  sty data_8013
  stx data_8012
  jsr sub_8C19
  bcs loc_8EEA
loc_8E81:
  lda data_8015
  lsr a
  beq loc_8ED0
  jsr sub_898B
  bcs loc_8EEA
  asl a
  bcs loc_8E96
  beq loc_8EEA
  cmp data_8015
  bcc loc_8E99
loc_8E96:
  lda data_8015
loc_8E99:
  and #$FE
  sta data_800A
  lsr a
  jsr sub_8188
  bcs loc_8EEA
  lda data_8015
  sec
  sbc data_800A
  sta data_8015
  ldy #$08
  clc
  lda (var_00EE),y
  adc data_800A
  sta (var_00EE),y
  jsr sub_8CB9
  ldy #$06
  lda (var_00EE),y
  adc data_800A
  sta (var_00EE),y
  ldy #$04
  lda (var_00EE),y
  adc data_800A
  sta (var_00EE),y
  jmp loc_8E81
loc_8ED0:
  ldx data_8015
  txa
  ora data_8014
  beq loc_8EE9
  ldy data_8014
  stx data_8013
  sty data_8012
  jsr sub_8C19
  bcs loc_8EEA
  lda #$00
loc_8EE9:
  clc
loc_8EEA:
  rts

sub_8EEB:
  ldy #$07
  ldx #$04
  clc
loc_8EF0:
  lda (var_00EE),y
  sbc #$00
  sta (var_00EE),y
  iny
  dex
  bne loc_8EF0
  bcs loc_8F00
  ldy #$00
  beq loc_8F0E
loc_8F00:
  txa
  jsr sub_88EA
  bcc loc_8F0A
  tax
  beq loc_8F37
  rts
loc_8F0A:
  ldx var_00F7
  ldy var_00F6
loc_8F0E:
  stx data_885F
  sty data_885E
  lda #$FF
  sta var_00FC
  lda data_807F
  sta data_868F
  lda #$FF
  sta data_868E
loc_8F23:
  jsr sub_86A5
  bcs loc_8EEA
  jsr sub_8659
  bcs loc_8F37
  lda #$00
  sta data_868E
  sta data_868F
  beq loc_8F23
loc_8F37:
  ldy #$07
  ldx #$04
  sec
loc_8F3C:
  lda (var_00EE),y
  adc #$00
  sta (var_00EE),y
  iny
  dex
  bne loc_8F3C

sub_8F46:
  ldy #$07
loc_8F48:
  lda (var_00EE),y
  pha
  iny
  cpy #$0B
  bne loc_8F48
  ldy #$11
loc_8F52:
  dey
  pla
  sta (var_00EE),y
  cpy #$0D
  bne loc_8F52
  ldy #$19
  lda #$01
  sta (var_00EE),y
  clc
  rts

sub_8F62:
  jsr sub_8230
  ldy #$14
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  tay
  pla
  jsr sub_841C
  bcs loc_8F80
  ldy #$11
  lda (var_00EE),y
  jsr sub_8403
  clc
loc_8F80:
  rts
  lda data_8017
  beq loc_8F90
  inc data_8017
  lda #$03
  jsr sub_81EB
  bcs loc_8FBF
loc_8F90:
  ldy #$1A
  lda (var_00EE),y
  beq loc_8F99
  jsr sub_859F
loc_8F99:
  ldy #$19
  lda (var_00EE),y
  beq loc_8FBD
  jsr sub_8F62
  bcs loc_8FBF
  ldy #$0B
loc_8FA6:
  lda (var_00EE),y
  pha
  iny
  cpy #$11
  bne loc_8FA6
  ldy #$20
loc_8FB0:
  dey
  pla
  sta (var_00F1),y
  cpy #$1A
  bne loc_8FB0
  jsr sub_840D
  bcs loc_8FBF
loc_8FBD:
  lda #$00
loc_8FBF:
  rts

sub_8FC0:
  jsr sub_841C
  bcs loc_900E
  lda var_00FA
loc_8FC7:
  jsr sub_8403
  ldy #$00
  lda (var_00F1),y
  beq loc_900B
  cmp #$E5
  beq loc_8FFD
loc_8FD4:
  lda (var_00F4),y
  cmp #$3F
  beq loc_8FE6
  lda (var_00F1),y
  cmp #$05
  bne loc_8FE2
  lda #$E5
loc_8FE2:
  cmp (var_00F4),y
  bne loc_8FFD
loc_8FE6:
  iny
  cpy #$0B
  bne loc_8FD4
  lda (var_00F1),y
  bit data_8023
  bmi loc_8FFB
  bne loc_8FFD
  ora data_8023
  and #$08
  beq loc_8FFD
loc_8FFB:
  clc
  rts
loc_8FFD:
  lda var_00FA
  clc
  adc #$10
  sta var_00FA
  bne loc_8FC7
  jsr sub_84F5
  bcc sub_8FC0
loc_900B:
  lda #$12
  sec
loc_900E:
  rts
data_900F:
  .byte $98, $8B, $8D, $8E, $A0, $8F, $9F, $9F
  .byte $9E, $9F, $A0, $A0, $A1, $A0, $A0, $A0
  .byte $A3, $A3, $A4, $A5, $8B, $9E, $9E, $A0
  .byte $9E
data_9028:
  .byte $9D, $8C
  .byte $BF, $0B, $0E, $80, $1F, $38, $BA, $DC
  .byte $A5, $38, $0B, $F3, $C9, $CD, $33, $43
  .byte $C6, $24, $8C, $BA, $7B, $A5, $51
data_9041:
  .byte $00, $C0
  .byte $00, $00, $00, $00, $C0, $00, $C0, $00, $C0, $80, $00, $40, $00, $00, $80, $A0
  .byte $A0, $80, $A0, $A0, $A0, $A0, $A0

sub_905A:
  lda data_9041,x
  asl a
  bpl loc_90A9
  pha
  ldy #$04
  lda (var_00EE),y
  sta var_00FF
  dey
  lda (var_00EE),y
  sta var_00FE
  lda #$00
  sta var_00F0
  ldy #$05
loc_9072:
  lda #$00
  sta var_00F8
loc_9076:
  sty var_00F3
  ldy #$02
  lda (var_00EE),y
  tax
  lda #$20
  ldy var_00F0
  jsr BIOS_ENTRY
  cmp #$20
  bne loc_9090
  ldy var_00F0
  beq loc_90AB
  dec var_00F8
  bne loc_9099
loc_9090:
  bit var_00F8
  bvs loc_90AB
  jsr sub_A623
  bcs loc_90AB
loc_9099:
  ldy var_00F3
  sta (var_00EE),y
  inc var_00F0
  iny
  cpy #$0D
  beq loc_9072
  cpy #$10
  bne loc_9076
  pla
loc_90A9:
  clc
  rts
loc_90AB:
  pla
  lda #$02
  sec
  rts

sub_90B0:
  ldy #$00
  lda (var_00EE),y
  tax
  lda data_900F,x
  pha
  lda data_9028,x
  pha
  rts

sub_90BE:
  jsr sub_8A15
  ldy #$00
  lda (var_00EE),y
  cmp #$19
  bcc loc_90CE
  lda #$01
  jmp loc_8A5D
loc_90CE:
  cmp #$09
  beq loc_9132
  tax
  ldy #$01
  lda (var_00EE),y
  bne loc_90DE
  lda data_8003
  sta (var_00EE),y
loc_90DE:
  cmp #$FF
  bne loc_90E5
  lda data_8004
loc_90E5:
  cmp data_8006
  bcc loc_90F1
  bne loc_9138
  ldy data_8000
  bne loc_9146
loc_90F1:
  sec
  sbc #$01
  cmp #$02
  bcc loc_90FE
  sbc #$02
  ora #$80
  bne loc_9103
loc_90FE:
  ldy data_8001
  beq loc_9138
loc_9103:
  sta data_8005
  lda data_9041,x
  bpl loc_9115
  jsr sub_83A8
  bcs loc_913A
  ldy #$00
  lda (var_00EE),y
  tax
loc_9115:
  jsr sub_905A
  bcs loc_913A
  asl a
  bpl loc_9132
  cpx #$16
  beq loc_9125
  cpx #$18
  bne loc_912D
loc_9125:
  ldy #$05
  lda (var_00EE),y
  ldy #$16
  sta (var_00EE),y
loc_912D:
  jsr sub_9DEA
  bcs loc_913A
loc_9132:
  jsr sub_90B0
  jmp loc_8A5D
loc_9138:
  lda #$0F
loc_913A:
  jmp loc_8A5D

sub_913D:
  lda #$0D
  ldx var_00EF
  ldy var_00EE
  jmp BIOS_ENTRY
loc_9146:
  cpx #$0D
  bne loc_9163
  ldy #$0D
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  pha
  jsr sub_905A
  ldy #$03
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
  bcs loc_91AD
  bcc loc_91A6
loc_9163:
  jsr sub_905A
  bcs loc_91AD
  ldy #$00
  lda (var_00EE),y
  cmp #$06
  beq loc_9190
  cmp #$07
  beq loc_9190
  cmp #$16
  bne loc_91A6
  ldy #$03
  lda (var_00EE),y
  tax
  iny
  lda (var_00EE),y
  ldy #$06
  sta (var_00EE),y
  dey
  lda (var_00EE),y
  pha
  txa
  sta (var_00EE),y
  pla
  ldy #$07
  sta (var_00EE),y
loc_9190:
  ldx var_00EF
  ldy var_00EE
  jsr sub_91B0
  jsr sub_913D
  ldx data_800E
  ldy data_800D
  jsr sub_91B0
  jmp loc_91A9
loc_91A6:
  jsr sub_913D
loc_91A9:
  ldy #$0B
  lda (var_00EE),y
loc_91AD:
  jmp loc_8A5D

sub_91B0:
  tya
  pha
  lda data_8008
  ldy #$02
  sta (var_00EE),y
  pla
  clc
  adc #$17
  ldy #$03
  sta (var_00EE),y
  txa
  adc #$00
  iny
  sta (var_00EE),y
  rts
  .res 56, $FF
data_9200:
  .byte $FF, $FF
data_9202:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_920D:
  .byte $FF
data_920E:
  .byte $FF, $FF
data_9210:
  .byte $FF
data_9211:
  .byte $FF
data_9212:
  .byte $FF
data_9213:
  .byte $FF
data_9214:
  .byte $FF
data_9215:
  .byte $FF
data_9216:
  .byte $FF, $FF
data_9218:
  .byte $FF, $FF
data_921A:
  .byte $FF
data_921B:
  .byte $FF, $FF, $FF, $FF, $FF
data_9220:
  .byte $FF
data_9221:
  .byte $FF
data_9222:
  .res 24, $FF
data_923A:
  .byte $FF
data_923B:
  .res 197, $FF
data_9300:
  .res 256, $FF
data_9400:
  .res 58, $FF
data_943A:
  .byte $FF
data_943B:
  .res 197, $FF
data_9500:
  .res 256, $FF
data_9600:
  .res 256, $FF
data_9700:
  .res 256, $FF
  .byte $0D, $0A, $49, $6E, $73, $65, $72, $74, $20, $64, $69, $73, $6B, $65, $74, $74
  .byte $65, $20, $66, $6F, $72, $20, $64, $72, $69, $76, $65, $20, $3F, $3A, $0D, $0A
  .byte $61, $6E, $64, $20, $70, $72, $65, $73, $73, $20, $61, $6E, $79, $20, $6B, $65
  .byte $79, $20, $77, $68, $65, $6E, $20, $72, $65, $61, $64, $79, $0D, $0A, $0A, $24
  .byte $0D, $0A, $B0, $D1, $3F, $C5, $CC, $B2, $E5, $C8, $EB, $C7, $FD, $B6, $AF, $C6
  .byte $F7, $D6, $D0, $2C, $B0, $B4, $C8, $CE, $D2, $E2, $BC, $FC, $C8, $B7, $C8, $CF
  .byte $0D, $0A, $24, $98, $18, $69, $41, $8D, $1C, $98, $8D, $44, $98, $A2, $98, $A0
  .byte $00, $AD, $44, $A5, $F0, $04, $A2, $98, $A0, $40, $AD, $07, $80, $8D, $3D, $A5
  .byte $20, $51, $A8, $A9, $18, $A2, $01, $20, $00, $58, $98, $F0, $0A, $A9, $18, $A2
  .byte $00, $20, $00, $58, $4C, $83, $98, $A9, $18, $A2, $00, $4C, $00, $58
  jsr sub_8209
  lda #$00
  sta data_9C83,x
  lda data_8005
  cmp data_802E
  bne sub_98B3
  lda #$FF
  sta data_802E

sub_98B3:
  lda #$14
  jsr BIOS_ENTRY
  and #$03
  sta data_8001
  stx data_8002
  tya
  and #$7F
  sta data_8000
  txa
  and #$03
  ldy #$02
loc_98CB:
  lsr a
  bcc loc_98CF
  iny
loc_98CF:
  bne loc_98CB
  lda data_8000
  beq loc_98D7
  iny
loc_98D7:
  sty data_8006
  lda #$00
  rts
data_98DD:
  .byte $CC, $80, $40, $10, $08, $06, $04, $03
data_98E5:
  .byte $0A, $02, $06, $04, $04, $02, $08, $00, $0C

sub_98EE:
  ldx data_809D
  cpx #$03
  bne loc_98F6
  iny
loc_98F6:
  ldx #$00
loc_98F8:
  cmp data_98DD,x
  beq loc_9902
  inx
  cpx #$08
  bne loc_98F8
loc_9902:
  lda data_98E5,x
  tax
  jmp CRITICAL_ERROR_ENTRY
  .byte $57, $72, $69, $74, $65, $20, $70, $72, $6F, $74, $65, $63, $74, $00, $49, $6E
  .byte $76, $61, $6C, $69, $64, $20, $75, $6E, $69, $74, $00, $4E, $6F, $74, $20, $72
  .byte $65, $61, $64, $79, $00, $49, $6E, $76, $61, $6C, $69, $64, $20, $64, $65, $76
  .byte $69, $63, $65, $00, $44, $61, $74, $61, $20, $65, $72, $72, $6F, $72, $00, $49
  .byte $6E, $76, $61, $6C, $69, $64, $20, $70, $61, $72, $61, $6D, $65, $74, $65, $72
  .byte $73, $00, $53, $65, $65, $6B, $20, $65, $72, $72, $6F, $72, $00, $49, $6E, $76
  .byte $61, $6C, $69, $64, $20, $6D, $65, $64, $69, $61, $20, $74, $79, $70, $65, $00
  .byte $53, $65, $63, $74, $6F, $72, $20, $6E, $6F, $74, $20, $66, $6F, $75, $6E, $64
  .byte $00, $50, $72, $69, $6E, $74, $65, $72, $20, $6F, $75, $74, $20, $6F, $66, $20
  .byte $70, $61, $70, $65, $72, $00, $57, $72, $69, $74, $65, $20, $66, $61, $75, $6C
  .byte $74, $20, $65, $72, $72, $6F, $72, $00, $52, $65, $61, $64, $20, $66, $61, $75
  .byte $6C, $74, $20, $65, $72, $72, $6F, $72, $00, $47, $65, $6E, $65, $72, $61, $6C
  .byte $20, $66, $61, $69, $6C, $75, $72, $65, $00, $20, $72, $65, $61, $64, $00, $20
  .byte $77, $72, $69, $74, $65, $00, $20, $64, $72, $69, $76, $65, $20, $00, $0D, $0A
  .byte $41, $62, $6F, $72, $74, $00, $2C, $20, $49, $67, $6E, $6F, $72, $65, $00, $2C
  .byte $20, $52, $65, $74, $72, $79, $00, $2C, $20, $46, $61, $69, $6C, $00, $00, $0E
  .byte $1B, $25, $34, $3F, $52, $5D, $70, $81, $96, $A8, $B9, $D0, $B4, $B1, $A3, $BB
  .byte $A4, $00, $CE, $B4, $D6, $AA, $B5, $A5, $D4, $AA, $00, $CE, $B4, $BE, $CD, $D0
  .byte $F7, $00, $CE, $DE, $D0, $A7, $C9, $E8, $B1, $B8, $00, $CA, $FD, $BE, $DD, $B3
  .byte $F6, $B4, $ED, $00, $B2, $CE, $CA, $FD, $CE, $DE, $D0, $A7, $00, $B2, $E9, $D5
  .byte $D2, $B3, $F6, $B4, $ED, $00, $B4, $ED, $CE, $F3, $BD, $E9, $D6, $CA, $C0, $E0
  .byte $D0, $CD, $00, $CE, $B4, $D5, $D2, $B5, $BD, $C9, $C8, $C7, $F8, $00, $B4, $F2
  .byte $D3, $A1, $BB, $FA, $C8, $B1, $D6, $BD, $00, $D0, $B4, $B4, $ED, $CE, $F3, $00
  .byte $B6, $C1, $B4, $ED, $CE, $F3, $00, $D2, $BB, $B0, $E3, $B4, $ED, $CE, $F3, $00
  .byte $B6, $C1, $00, $D0, $B4, $00, $B4, $C5, $C5, $CC, $20, $00, $0D, $0A, $CD, $CB
  .byte $B3, $F6, $41, $00, $2C, $BA, $F6, $C2, $D4, $49, $00, $2C, $D6, $D8, $CA, $D4
  .byte $52, $00, $2C, $CA, $A7, $B0, $DC, $46, $00, $20, $CA, $B1, $B7, $A2, $CF, $D6
  .byte $00, $0D, $0A, $00, $00, $07, $10, $17, $20, $29, $32, $3B, $48, $53, $5E, $65
  .byte $6C, $00, $BE, $14, $9A, $F0, $0A, $98, $48, $20, $84, $A7, $68, $A8, $C8, $D0
  .byte $F1, $60, $BE, $09, $99, $F0, $0A, $98, $48, $20, $84, $A7, $68, $A8, $C8, $D0
  .byte $F1, $60, $98, $48, $A0, $A6, $20, $CB, $9A, $68, $A8, $60, $A2, $3F, $20, $84
  .byte $A7, $A9, $01, $20, $4E, $A7, $29, $DF, $AA, $A0, $02, $E0, $41, $F0, $1C, $A0
  .byte $00, $A9, $20, $E0, $49, $F0, $0D, $C8, $4A, $E0, $52, $F0, $07, $A0, $03, $4A
  .byte $E0, $46, $D0, $05, $2D, $CA, $9A, $D0, $02, $38, $60, $20, $EB, $9A, $18, $60
  .byte $8C, $CA, $9A, $E0, $0D, $90, $02, $A2, $0C, $AD, $44, $A5, $10, $5F, $BC, $BD
  .byte $9A, $98, $48, $20, $EB, $9A, $AD, $CA, $9A, $A0, $75, $4A, $90, $02, $A0, $78
  .byte $20, $CB, $9A, $AD, $CA, $9A, $10, $00, $A0, $7B, $20, $CB, $9A, $A0, $01, $B1
  .byte $EE, $18, $69, $40, $AA, $20, $84, $A7, $A0, $9E, $20, $CB, $9A, $68, $A8, $20
  .byte $CB, $9A, $A0, $81, $20, $CB, $9A, $AD, $CA, $9A, $0A, $0A, $10, $07, $48, $A0
  .byte $89, $20, $CB, $9A, $68, $0A, $10, $07, $48, $A0, $90, $20, $CB, $9A, $68, $0A
  .byte $10, $05, $A0, $97, $20, $CB, $9A, $20, $F5, $9A, $B0, $D6, $60, $8C, $CA, $9A
  .byte $E0, $0D, $90, $02, $A2, $0C, $BC, $07, $9A, $20, $EB, $9A, $20, $DB, $9A, $AD
  .byte $CA, $9A, $A0, $C9, $4A, $90, $02, $A0, $CF, $20, $DB, $9A, $AD, $CA, $9A, $10
  .byte $00, $A0, $D6, $20, $DB, $9A, $A0, $01, $B1, $EE, $18, $69, $40, $AA, $20, $84
  .byte $A7, $A0, $DE, $20, $DB, $9A, $AD, $CA, $9A, $0A, $0A, $10, $07, $48, $A0, $E6
  .byte $20, $DB, $9A, $68, $0A, $10, $07, $48, $A0, $EF, $20, $DB, $9A, $68, $0A, $10
  .byte $05, $A0, $F7, $20, $DB, $9A, $20, $F5, $9A, $B0, $D6, $60
data_9BF5:
  .res 62, $00
data_9C33:
  .res 80, $00
data_9C83:
  .res 126, $00
  brk
  .res 117, $00

sub_9D77:
  jsr sub_85D6
  bcs sub_9D99
  lda data_802E
  cmp #$FF
  beq sub_9D99
  jsr sub_820C
  ldy #$00
loc_9D88:
  lda data_8032,y
  sta data_9C33,x
  inx
  iny
  cpy #$50
  bne loc_9D88
  lda #$FF
  sta data_9C33,x

sub_9D99:
  ldx #$FF
  stx data_801B
  stx data_801A
  stx data_801E
  inx
  stx data_8017
  stx data_8016
  clc
  rts

sub_9DAD:
  lda #$FF
  sta data_802A

sub_9DB2:
  cpx data_802A
  beq loc_9DD5
  stx var_00F8
  sty var_00F9
  lda data_8028
  sta var_00FF
  lda data_8027
  sta var_00FE
  lda #$20
  ldx data_8029
  ldy var_00F8
  jsr BIOS_ENTRY
  ldx var_00F8
  ldy var_00F9
  cmp #$00
loc_9DD5:
  rts

sub_9DD6:
  ldy #$02
  lda (var_00EE),y
  sta data_8029
  iny
  lda (var_00EE),y
  sta data_8027
  iny
  lda (var_00EE),y
  sta data_8028
  rts

sub_9DEA:
  jsr sub_9DD6
  ldx #$00
  stx var_00F3
loc_9DF1:
  jsr sub_9DAD
  beq loc_9DFF
  inx
  cmp #$5C
  bne loc_9DF1
  stx var_00F3
  beq loc_9DF1
loc_9DFF:
  ldx var_00F3
  beq loc_9E14
  dex
  beq loc_9E0F
  jsr sub_A28A
  bcs loc_9E1C
  ldx var_00F3
  bne loc_9E14
loc_9E0F:
  jsr sub_A27B
  ldx #$01
loc_9E14:
  jsr sub_9DAD
  beq loc_9E1D
  jsr sub_A151
loc_9E1C:
  rts
loc_9E1D:
  lda #$03
  rts

sub_9E20:
  ldy #$05
  ldx #$01
loc_9E24:
  lda (var_00EE),y
  cmp #$3F
  beq loc_9E4F
  cmp #$20
  beq loc_9E34
  inx
  iny
  cpy #$0D
  bne loc_9E24
loc_9E34:
  ldy #$0D
  lda (var_00EE),y
  cmp #$20
  beq loc_9E4D
  inx
loc_9E3D:
  lda (var_00EE),y
  cmp #$3F
  beq loc_9E4F
  cmp #$20
  beq loc_9E4D
  inx
  iny
  cpy #$10
  bne loc_9E3D
loc_9E4D:
  clc
  rts
loc_9E4F:
  lda #$05
  rts
  ldy #$16
  lda (var_00EE),y
  and #$D8
  bne loc_9E79
  lda #$37
  ldy #$08
  jsr sub_84CE
  bcs loc_9E78
  ldy #$0B
  lda (var_00F1),y
  and #$18
  ldy #$16
  ora (var_00EE),y
  ldy #$0B
  sta (var_00F1),y
  jsr sub_840D
  bcs loc_9E78
  lda #$00
loc_9E78:
  rts
loc_9E79:
  lda #$05
  rts
  ldy #$16
  lda (var_00EE),y
  tax
  and #$10
  beq loc_9E89
  txa
  and #$F7
  tax
loc_9E89:
  txa
  ora #$A1
  eor #$FF
  jsr sub_9F22
  pha
  ldy #$2B
  lda data_A53A
  sta (var_00EE),y
  iny
  lda #$F5
  sta (var_00EE),y
  iny
  lda #$9B
  sta (var_00EE),y
  lda #$00
  tax
  bit data_802D
  bpl loc_9EB1
  ldx data_802C
  lda data_802B
loc_9EB1:
  ldy #$2E
  sta (var_00EE),y
  iny
  txa
  sta (var_00EE),y
  pla
  rts
  jsr sub_9E20
  bcs loc_9EE4
  lda #$3F
  ldy #$00
  jsr sub_8B91
  bcs loc_9EE5
  lda data_8023
  and #$18
  beq loc_9ED3
  lda #$05
  rts
loc_9ED3:
  jsr sub_8EEB
  bcs loc_9EE4
  lda #$20
  jsr sub_8AB6
  jsr sub_840D
  bcs loc_9EE4
  lda #$00
loc_9EE4:
  rts
loc_9EE5:
  cmp #$02
  bne loc_9EF5
  ldx var_00FE
  inx
  bne loc_9EFB
  bit data_802D
  bmi loc_9EF6
  lda #$05
loc_9EF5:
  rts
loc_9EF6:
  jsr sub_8B0C
  bcs loc_9EF5
loc_9EFB:
  jsr sub_8AEF
  bcs loc_9EF5
  ldx #$20
  jsr sub_8AB6
  jsr sub_840D
  bcs loc_9EF5
  jmp loc_8BBA

sub_9F0D:
  lda data_800D
  clc
  adc #$17
  ldy #$03
  sta (var_00EE),y
  lda data_800E
  adc #$00
  iny
  sta (var_00EE),y
  rts
  lda #$FF

sub_9F22:
  ldy #$16
  sta (var_00EE),y
  sta data_8023
  pha
  jsr sub_9F0D
  pla
  jsr sub_8457
  bcs loc_9F38
  jsr sub_8FC0
  bcc loc_9F80
loc_9F38:
  rts
  ldy #$16
  lda (var_00EE),y
  sta data_8023
  jsr sub_9F0D
  jsr sub_8230
  jsr sub_83F5
  ldy #$15
  lda (var_00EE),y
  sta data_8022
  dey
  lda (var_00EE),y
  sta var_00FB
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  pha
  dey
  lda (var_00EE),y
  clc
  adc #$10
  sta var_00FA
  pla
  tay
  pla
  bcc loc_9F7B
  jsr sub_84FE
  bcc loc_9F7B
  cmp #$02
  bne loc_9F79
  lda #$12
loc_9F79:
  sec
  rts
loc_9F7B:
  jsr sub_8FC0
  bcs loc_9F79
loc_9F80:
  ldy #$15
  lda data_8022
  sta (var_00EE),y
  dey
  lda var_00FB
  sta (var_00EE),y
  dey
  lda data_801A
  sta (var_00EE),y
  dey
  lda data_8019
  sta (var_00EE),y
  dey
  lda data_8018
  sta (var_00EE),y
  dey
  lda var_00FA
  sta (var_00EE),y
  ldy #$0B
  lda (var_00F1),y
  pha
  ldy #$16
loc_9FAA:
  lda (var_00F1),y
  pha
  iny
  cpy #$1A
  bne loc_9FAA
  ldy #$1C
loc_9FB4:
  lda (var_00F1),y
  pha
  iny
  cpy #$20
  bne loc_9FB4
  ldy #$20
loc_9FBE:
  dey
  pla
  sta (var_00EE),y
  cpy #$17
  bne loc_9FBE
  ldy #$00
loc_9FC8:
  lda (var_00F1),y
  pha
  iny
  cpy #$0B
  bne loc_9FC8
  ldy #$2B
loc_9FD2:
  dey
  pla
  sta (var_00EE),y
  cpy #$20
  bne loc_9FD2
  lda #$00
  rts
  ldy #$02
  lda data_8006
  sta (var_00EE),y
  ldy #$01
  lda (var_00EE),y
  beq loc_9FFD
  cmp data_8006
  bcc loc_9FF1
  bne loc_A00C
loc_9FF1:
  cmp #$03
  bcs loc_9FFA
  ldy data_8001
  beq loc_A00C
loc_9FFA:
  sta data_8003
loc_9FFD:
  lda data_8003
  sta (var_00EE),y
  ldy #$04
  lda data_802F
  sta (var_00EE),y
  lda #$00
  rts
loc_A00C:
  lda #$0F
  rts
  ldx #$03
  ldy #$0D
loc_A013:
  lda (var_00EE),y
  sta var_00F1,x
  iny
  dex
  bpl loc_A013
  ldy #$07
  ldx #$03
  sec
loc_A020:
  lda var_00F1,x
  sbc (var_00EE),y
  iny
  dex
  bpl loc_A020
  bcs loc_A036
  ldx #$03
  ldy #$07
loc_A02E:
  lda var_00F1,x
  sta (var_00EE),y
  iny
  dex
  bpl loc_A02E
loc_A036:
  lda #$00
  rts
  ldx #$00
  ldy #$02
  stx var_00F5
  sty var_00F4
  stx var_00F2
  stx var_00F1
loc_A045:
  stx var_00F7
  sty var_00F6
  jsr sub_8667
  bcs loc_A0A5
  jsr sub_8885
  bcs loc_A0A5
  ora var_00F6
  bne loc_A05D
  inc var_00F1
  bne loc_A05D
  inc var_00F2
loc_A05D:
  inc var_00F4
  bne loc_A063
  inc var_00F5
loc_A063:
  ldx var_00F5
  ldy var_00F4
  cpx data_8074
  bne loc_A045
  cpy data_8073
  bne loc_A045
  ldy #$06
  lda #$02
  sta (var_00EE),y
  lda #$00
  dey
  sta (var_00EE),y
  ldy #$04
  sta (var_00EE),y
  lda data_8070
  dey
  sta (var_00EE),y
  ldy #$09
  lda data_8073
  sec
  sbc #$02
  sta (var_00EE),y
  iny
  lda data_8074
  sbc #$00
  sta (var_00EE),y
  ldy #$08
  lda var_00F2
  sta (var_00EE),y
  dey
  lda var_00F1
  sta (var_00EE),y
  lda #$00
loc_A0A5:
  rts
  jsr sub_8B8D
  bcs loc_A0C9
  jsr sub_8EEB
  bcs loc_A0C9
  jsr sub_859F
  bcs loc_A0C9
  ldy #$11
  lda (var_00EE),y
  jsr sub_8403
  lda #$E5
  ldy #$00
  sta (var_00F1),y
  jsr sub_840D
  bcs loc_A0C9
  lda #$00
loc_A0C9:
  rts
  lda #$02
  bne loc_A0D0
  lda #$03
loc_A0D0:
  sta var_00FD
  jsr sub_8230
  ldy #$09
  lda (var_00EE),y
  sta data_8011
  dey
  lda (var_00EE),y
  sta data_8010
  dey
  lda (var_00EE),y
  sta data_800F
  ldy #$05
  lda (var_00EE),y
  jsr sub_8188
  bcs loc_A0F3
  lda #$00
loc_A0F3:
  rts
  jsr sub_9E20
  bcs loc_A10B
  jsr sub_8F62
  bcs loc_A10B
  jsr sub_83F5
  jsr sub_8AA2
  jsr sub_840D
  bcs loc_A10B
  lda #$00
loc_A10B:
  rts
  jsr sub_8F62
  bcs loc_A14D
  ldy #$02
  lda (var_00EE),y
  sta var_00F0
  and #$18
  bne loc_A14E
  ldy #$0B
  lda (var_00F1),y
  and #$18
  ora var_00F0
  sta (var_00F1),y
  ldy #$03
  lda (var_00EE),y
  tax
  iny
  lda (var_00EE),y
  ldy #$19
  sta (var_00F1),y
  dey
  txa
  sta (var_00F1),y
  ldy #$06
  lda (var_00EE),y
  tax
  dey
  lda (var_00EE),y
  ldy #$16
  sta (var_00F1),y
  iny
  txa
  sta (var_00F1),y
  ldy #$19
  lda #$01
  sta (var_00EE),y
  lda #$00
loc_A14D:
  rts
loc_A14E:
  lda #$05
  rts

sub_A151:
  ldy #$05
  jsr sub_9DB2
  cmp #$2E
  bne loc_A174
  sta (var_00EE),y
  iny
  inx
  jsr sub_9DB2
  beq loc_A192
  cmp #$2E
  bne loc_A170
  sta (var_00EE),y
  iny
  inx
  jsr sub_9DB2
  beq loc_A192
loc_A170:
  lda #$02
  sec
  rts
loc_A174:
  inx
  cmp #$2A
  beq loc_A196
  cpy #$0D
  beq loc_A1A2
  jsr sub_A623
  bcs loc_A1E9
  sta (var_00EE),y
  iny
  jsr sub_9DB2
  beq loc_A192
  cmp #$5C
  beq loc_A192
  cmp #$2E
  bne loc_A174
loc_A192:
  lda #$20
  bne loc_A198
loc_A196:
  lda #$3F
loc_A198:
  cpy #$0D
  beq loc_A1A2
  sta (var_00EE),y
  iny
  jmp loc_A198
loc_A1A2:
  jsr sub_9DB2
  beq loc_A1B0
  cmp #$5C
  beq loc_A1B0
  inx
  cmp #$2E
  bne loc_A1A2
loc_A1B0:
  jsr sub_9DB2
  beq loc_A1D0
  cmp #$5C
  beq loc_A1D0
  inx
  cmp #$2A
  beq loc_A1CC
  cpy #$10
  beq loc_A1DC
  jsr sub_A623
  bcs loc_A1E9
  sta (var_00EE),y
  iny
  bne loc_A1B0
loc_A1CC:
  lda #$3F
  bne loc_A1D2
loc_A1D0:
  lda #$20
loc_A1D2:
  cpy #$10
  beq loc_A1DC
  sta (var_00EE),y
  iny
  jmp loc_A1D2
loc_A1DC:
  jsr sub_9DB2
  beq loc_A1E8
  cmp #$5C
  beq loc_A1E8
  inx
  bne loc_A1DC
loc_A1E8:
  clc
loc_A1E9:
  rts

sub_A1EA:
  ldx #$FF
loc_A1EC:
  inx
  lda data_9BF5,x
  bne loc_A1EC
  stx var_00F0
  lda #$5C
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
  ldy #$00
loc_A200:
  lda (var_00F1),y
  iny
  cmp #$20
  beq loc_A20F
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A20F:
  cpy #$08
  bne loc_A200
  lda (var_00F1),y
  cmp #$20
  beq loc_A236
  lda #$2E
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A223:
  lda (var_00F1),y
  iny
  cmp #$20
  beq loc_A232
  sta data_9BF5,x
  inx
  cpx #$3E
  bcs loc_A239
loc_A232:
  cpy #$0B
  bne loc_A223
loc_A236:
  clc
  bcc loc_A23B
loc_A239:
  ldx var_00F0
loc_A23B:
  lda #$00
  sta data_9BF5,x
  rts

sub_A241:
  bit data_802D
  bpl loc_A277
  jsr sub_8480
  jsr sub_841C
  bcs loc_A27A
  ldx data_943B
  ldy data_943A
  txa
  ora data_943A
  beq sub_A27B
  stx data_802C
  sty data_802B
  ldx #$FF
loc_A262:
  inx
  lda data_9BF5,x
  bne loc_A262
loc_A268:
  dex
  lda data_9BF5,x
  cmp #$5C
  bne loc_A268
  lda #$00
  sta data_9BF5,x
  clc
  rts
loc_A277:
  lda #$03
  sec
loc_A27A:
  rts

sub_A27B:
  lda #$00
  sta data_802D
  sta data_9BF5
  sta data_802C
  sta data_802B
  rts

sub_A28A:
  stx data_802A
  ldx #$00
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$5C
  bne loc_A29C
  jsr sub_A27B
  inx
loc_A29C:
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$2E
  bne loc_A2B8
  inx
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$2E
  bne loc_A2E7
  inx
  stx var_00F9
  jsr sub_A241
  bcc loc_A2E5
  rts
loc_A2B8:
  jsr sub_A151
  bcs loc_A2CD
  stx var_00F9
  lda #$10
  ldy #$08
  jsr sub_84D2
  bcc loc_A2CE
  cmp #$02
  beq loc_A2F5
  sec
loc_A2CD:
  rts
loc_A2CE:
  jsr sub_A1EA
  bcs loc_A2F5
  ldy #$1B
  lda (var_00F1),y
  sta data_802C
  dey
  lda (var_00F1),y
  sta data_802B
  lda #$FF
  sta data_802D
loc_A2E5:
  ldx var_00F9
loc_A2E7:
  jsr sub_9DB2
  beq loc_A2F3
  cmp #$5C
  bne loc_A2F5
  inx
  bne loc_A29C
loc_A2F3:
  clc
  rts
loc_A2F5:
  lda #$03
  rts

sub_A2F8:
  ldx data_807B
  ldy data_807A
  stx data_802C
  sty data_802B
  lda data_807C
  sta data_802D
  ldx #$FF
loc_A30C:
  inx
  lda data_8032,x
  sta data_9BF5,x
  bne loc_A30C
  rts

sub_A316:
  ldx data_802C
  ldy data_802B
  stx data_807B
  sty data_807A
  lda data_802D
  sta data_807C
  ldx #$FF
loc_A32A:
  inx
  lda data_9BF5,x
  sta data_8032,x
  bne loc_A32A
  rts
  jsr sub_9DD6
  ldx #$FF
  jsr sub_A28A
  bcs loc_A343
  jsr sub_A316
  lda #$00
loc_A343:
  rts
  jsr sub_9E20
  bcs loc_A37B
  stx var_00F0
  ldx #$FF
loc_A34D:
  inx
  lda data_9BF5,x
  bne loc_A34D
  txa
  adc var_00F0
  cmp #$3E
  bcs loc_A363
  lda #$3F
  ldy #$00
  jsr sub_84CE
  bcs loc_A366
loc_A363:
  lda #$05
  rts
loc_A366:
  cmp #$02
  bne loc_A37B
  ldx var_00FE
  inx
  bne loc_A381
  bit data_802D
  bmi loc_A37C
  lda #$52
  sta data_8031
  lda #$05
loc_A37B:
  rts
loc_A37C:
  jsr sub_8B0C
  bcs loc_A390
loc_A381:
  ldx data_8081
  ldy data_8080
  stx var_00F7
  sty var_00F6
  jsr sub_883E
  bcc loc_A39B
loc_A390:
  cmp #$27
  sec
  bne loc_A37B
  sta data_8031
  lda #$05
  rts
loc_A39B:
  jsr sub_8AEF
  bcs loc_A37B
  ldx #$10
  jsr sub_8AB6
  ldy #$1B
  lda data_8081
  sta (var_00F1),y
  tax
  dey
  lda data_8080
  sta (var_00F1),y
  tay
  lda data_807F
  sta data_868F
  lda #$FF
  sta data_868E
  sta var_00FC
  jsr sub_86A5
  bcs loc_A37B
  jsr sub_840D
  bcs loc_A37B
  jsr sub_85D6
  bcs loc_A37B
  jsr sub_8ADF
  ldx data_8081
  ldy data_8080
  stx data_921B
  sty data_921A
  jsr sub_8486
  bcs loc_A456
  sta data_801E
  stx data_801D
  sty data_801C
  ldx #$0A
  lda #$20
loc_A3F1:
  sta data_9200,x
  sta data_9220,x
  dex
  bne loc_A3F1
  lda #$2E
  sta data_9200
  sta data_9220
  sta data_9221
  ldx #$92
  ldy #$00
  stx var_00F2
  sty var_00F1
  lda #$10
  jsr sub_8AC2
  lda #$20
  sta var_00F1
  lda #$10
  jsr sub_8AC2
  bit data_802D
  bpl loc_A42C
  ldx data_802C
  ldy data_802B
  stx data_923B
  sty data_923A
loc_A42C:
  lda #$03
  jsr sub_81EB
  bcs loc_A456
  jsr sub_8ADF
  ldx data_8070
  stx var_00F3
loc_A43B:
  dec var_00F3
  bne loc_A442
  lda #$00
  rts
loc_A442:
  inc data_801C
  bne loc_A44F
  inc data_801D
  bne loc_A44F
  inc data_801E
loc_A44F:
  lda #$03
  jsr sub_81EB
  bcc loc_A43B
loc_A456:
  rts

sub_A457:
  lda data_802C
  pha
  lda data_802B
  pha
  stx data_802C
  sty data_802B
  lda #$00
  sta data_8022
  lda data_8070
  sta var_00FB
  lda #$40
  sta var_00FA
  dec data_802D
  jsr sub_8486
  bcs loc_A4B1
loc_A47B:
  jsr sub_841C
  bcs loc_A4B1
  ldy var_00FA
loc_A482:
  lda data_9400,y
  beq loc_A4B1
  cmp #$E5
  bne loc_A4AF
  tya
  adc #$1F
  tay
  bne loc_A482
loc_A491:
  lda data_9500,y
  beq loc_A4B1
  cmp #$E5
  bne loc_A4AF
  tya
  adc #$1F
  tay
  bne loc_A491
  sty var_00FA
  jsr sub_84F5
  bcc loc_A47B
  cmp #$02
  beq loc_A4B1
  lda #$00
  beq loc_A4B1
loc_A4AF:
  lda #$05
loc_A4B1:
  sta var_00F0
  ldx data_802C
  ldy data_802B
  pla
  sta data_802B
  pla
  sta data_802C
  inc data_802D
  lda var_00F0
  rts
  lda #$10
  ldy #$08
  jsr sub_84CE
  bcc loc_A4D7
  cmp #$02
  bne loc_A524
  lda #$03
  rts
loc_A4D7:
  jsr sub_8BEC
  ldy #$1B
  lda (var_00F1),y
  tax
  dey
  lda (var_00F1),y
  tay
  cpx data_807B
  bne loc_A4F0
  cpy data_807A
  bne loc_A4F0
  lda #$10
  rts
loc_A4F0:
  jsr sub_A457
  cmp #$00
  bne loc_A524
  lda #$FF
  sta var_00FC
loc_A4FB:
  lda #$00
  sta data_868E
  sta data_868F
  jsr sub_86A5
  bcs loc_A524
  jsr sub_8659
  bcc loc_A4FB
  jsr sub_859F
  bcs loc_A524
  jsr sub_8F62
  bcs loc_A524
  ldy #$00
  lda #$E5
  sta (var_00F1),y
  jsr sub_840D
  bcs loc_A524
  lda #$00
loc_A524:
  rts
  ldy #$02
  lda data_A53A
  sta (var_00EE),y
  ldy #$04
  lda #$80
  sta (var_00EE),y
  dey
  lda #$32
  sta (var_00EE),y
  lda #$00
  rts
data_A53A:
  .byte $00
data_A53B:
  .byte $00
data_A53C:
  .byte $00
data_A53D:
  .byte $00
data_A53E:
  .byte $00
data_A53F:
  .byte $00
data_A540:
  .byte $00, $00, $00
data_A543:
  .byte $00
data_A544:
  .byte $00
data_A545:
  .byte $00
data_A546:
  .byte $00
data_A547:
  .byte $00
data_A548:
  .byte $00
data_A549:
  .res 40, $00
data_A571:
  .byte $0D
  .res 60, $00
data_A5AE:
  .byte $00
data_A5AF:
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00

sub_A5BC:
  cmp #$03
  beq loc_A5C6
  cmp #$00
  bne loc_A5C6
  cpx #$00
loc_A5C6:
  rts

sub_A5C7:
  lda #$34
  ldx #$00
  jmp BIOS_ENTRY

sub_A5CE:
  lda #$18
  ldx #$00
  jmp BIOS_ENTRY

sub_A5D5:
  lda #$1B
  ldy #$00
  jmp BIOS_ENTRY

sub_A5DC:
  jsr sub_A73A
  beq loc_A5FC
  cmp #$13
  bne loc_A5F1
  jsr sub_A5CE
  jsr sub_A5CE
  jsr sub_A5BC
  beq loc_A5F9
  rts
loc_A5F1:
  jsr sub_A5BC
  bne loc_A5FC
  jsr sub_A5CE
loc_A5F9:
  jsr CTRL_C_ENTRY
loc_A5FC:
  rts

sub_A5FD:
  stx data_A543
  ldx data_A53D
  lda var_00FF
  pha
  lda #$20
  jsr BIOS_ENTRY
  tax
  pla
  sta var_00FF
  txa
  ldx data_A543
  rts

sub_A614:
  lda var_00FF
  pha
  lda #$21
  ldx data_A53D
  jsr BIOS_ENTRY
  pla
  sta var_00FF
  rts

sub_A623:
  cmp #$21
  bcc loc_A659
  beq loc_A662
  cmp #$23
  bcc loc_A659
  cmp #$2A
  bcc loc_A662
  cmp #$2D
  beq loc_A662
  cmp #$30
  bcc loc_A659
  cmp #$3A
  bcc loc_A662
  cmp #$3F
  bcc loc_A659
  cmp #$5B
  bcc loc_A662
  cmp #$5E
  bcc loc_A659
  cmp #$61
  bcc loc_A662
  cmp #$7B
  bcc loc_A660
  cmp #$7C
  beq loc_A659
  cmp #$7F
  bne loc_A662
loc_A659:
  lda #$02
  sta data_A53F
  sec
  rts
loc_A660:
  and #$DF
loc_A662:
  clc
  rts
data_A664:
data_A665 = * + 1
  .word loc_A6A4 - 1, loc_A730 - 1
  .word sub_A784 - 1, loc_A7DA - 1
  .word loc_A851 - 1, sub_B1C7 - 1
  .word loc_A9B1 - 1, sub_A713 - 1
  .word loc_A74C - 1, sub_A827 - 1
  .word loc_A84C - 1, sub_AAA9 - 1
  .word loc_AAF0 - 1, loc_AAD7 - 1
  .word sub_A73A - 1, loc_B1B9 - 1
  .word loc_A69E - 1

sub_A686:
  asl a
  cmp #$22
  bcs loc_A69B
  stx data_A543
  tax
  lda data_A665,x
  pha
  lda data_A664,x
  pha
  ldx data_A543
  clc
loc_A69B:
  lda #$01
  rts
loc_A69E:
  lda #$03
  ldx #$00
  clc
  rts
loc_A6A4:
  stx data_A53F
  sty data_A540
  ldx #$FF
  txs
  lda #$22
  jsr BIOS_ENTRY
  lda data_A540
  and #$02
  beq loc_A6D9
  lda #$18
  ldx #$06
  ldy #$00
  jsr BIOS_ENTRY
  lda #$17
  jsr BIOS_ENTRY
  txa
  pha
  tya
  pha
  lda #$04
  jsr BIOS_ENTRY
  pla
  tay
  pla
  tax
  lda #$16
  jsr BIOS_ENTRY
loc_A6D9:
  lda data_A540
  lsr a
  bcc loc_A6E2
  jsr sub_A827
loc_A6E2:
  ldx data_A548
  dex
  lda data_A549,x
  sta data_A53D
  ldy #$04
loc_A6EE:
  dex
  lda data_A549,x
  pha
  dey
  bne loc_A6EE
  stx data_A548
  lda data_A53F
  ldy data_A540
  clc
  rts

sub_A701:
  cmp #$00
  beq loc_A712
  cpx #$E2
  beq loc_A711
  cpx #$E3
  beq loc_A711
  cmp #$E0
  beq loc_A712
loc_A711:
  clc
loc_A712:
  rts

sub_A713:
  ldx #$00
  lda data_A545
  bne loc_A72C
  jsr sub_A5C7
loc_A71D:
  jsr sub_A5DC
  beq loc_A71D
  jsr sub_A5CE
  jsr sub_A701
  bcc loc_A72F
  lda #$00
loc_A72C:
  stx data_A545
loc_A72F:
  rts
loc_A730:
  jsr sub_A713
  pha
  tax
  jsr sub_A784
  pla
  rts

sub_A73A:
  ldx #$01
  jsr DEVICE_ENTRY
  tya
  beq loc_A74B
  lda #$18
  ldx #$01
  jsr BIOS_ENTRY
  cpy #$00
loc_A74B:
  rts
loc_A74C:
  lda var_00FF
  pha
  lda #$00
  sta data_A545
  txa
  pha
  tya
  pha
  jsr sub_A5C7
loc_A75B:
  jsr sub_A5DC
  beq loc_A766
  jsr sub_A5CE
  jmp loc_A75B
loc_A766:
  pla
  tay
  pla
  tax
  pla
  jmp sub_A686
  .byte $68, $68, $60
data_A771:
  .byte $00

sub_A772:
  ldx data_A771
  beq loc_A783
  lda #$1B
  ldy #$00
  jsr BIOS_ENTRY
  lda #$00
  sta data_A771
loc_A783:
  rts

sub_A784:
  cpx #$09
  bne loc_A7AA
  jsr sub_A772
  lda #$1E
  jsr BIOS_ENTRY
  tya
  sta data_A543
  ora #$07
  sec
  sbc data_A543
  beq loc_A7A8
loc_A79C:
  pha
  ldx #$20
  jsr sub_A5D5
  pla
  sec
  sbc #$01
  bne loc_A79C
loc_A7A8:
  ldx #$20
loc_A7AA:
  bit data_A544
  bpl loc_A7BA
  cpx #$80
  bcs loc_A7BE
  txa
  pha
  jsr sub_A772
  pla
  tax
loc_A7BA:
  ldy #$00
  beq loc_A7C8
loc_A7BE:
  ldy data_A771
  beq loc_A7D5
  lda #$00
  sta data_A771
loc_A7C8:
  lda #$1B
  jsr BIOS_ENTRY
  lda data_A53E
  beq loc_A7D8
  jmp sub_A5DC
loc_A7D5:
  stx data_A771
loc_A7D8:
  rts
data_A7D9:
  .byte $00
loc_A7DA:
  txa
  clc
  adc data_A7D9
  bcs loc_A7EE
  cmp #$FF
  beq loc_A7EE
  ldx data_A7D9
  sta data_A7D9
  txa
  clc
  rts
loc_A7EE:
  lda #$00
  sbc data_A7D9
  sec
  rts
data_A7F5:
  .byte $00
data_A7F6:
  .byte $00, $68, $AA, $AD, $F5, $A7, $C9, $40, $D0, $16, $AD, $F6, $A7, $C9, $62, $B0
  .byte $0F, $E0, $00, $F0, $03, $38, $B0, $0F, $A9, $3C, $20, $00, $58, $18, $90, $07
  .byte $A9, $3C, $20, $00, $58, $C9, $01, $68, $AA, $A9, $01, $4C, $00, $58
data_A824:
  .byte $00
data_A825:
  .byte $F6
data_A826:
  .byte $A7

sub_A827:
  lda #$22
  jsr BIOS_ENTRY
  lda #$00
  sta data_A771
  sta data_A545
  lda data_A53A
  pha
  lda data_A544
  pha
  lda data_A826
  pha
  lda data_A825
  pha
  lda #$01
  ldx data_A824
  jmp BIOS_ENTRY
loc_A84C:
  stx data_A7D9
  clc
  rts
loc_A851:
  lda var_00FF
  pha
  lda var_00FE
  pha
  stx var_00FF
  sty var_00FE
  jsr sub_A772
  ldy #$00
loc_A860:
  jsr sub_A5FD
  cmp #$24
  beq loc_A877
  tax
  tya
  pha
  jsr sub_A784
  pla
  tay
  iny
  bne loc_A860
  inc var_00FF
  jmp loc_A860
loc_A877:
  jsr sub_A772
  pla
  sta var_00FE
  pla
  sta var_00FF
  rts

sub_A881:
  bit data_A544
  bmi loc_A898
  ldx #$DB
  bne loc_A891

sub_A88A:
  bit data_A544
  bmi loc_A898

sub_A88F:
  ldx #$20
loc_A891:
  lda #$1D
  ldy #$01
  jsr BIOS_ENTRY
loc_A898:
  rts
data_A899:
  .byte $00
data_A89A:
  .byte $00
data_A89B:
  .byte $00
data_A89C:
  .byte $00
data_A89D:
  .byte $00

sub_A89E:
  ldx data_A89A
  inx
  cpx data_A899
  bcs loc_A8C6
  sta var_00FD
  ldy data_A89A
  jsr sub_A614
  inc data_A89A
  ldx var_00FD
  cpx #$20
  bcs loc_A8C2
  ldy #$5E
  jsr sub_A784
  lda var_00FD
  ora #$40
  tax
loc_A8C2:
  jsr sub_A784
  clc
loc_A8C6:
  rts

sub_A8C7:
  lda #$00
  sta data_A89C
  ldx data_A89B
  lda data_A571,x
  cmp #$0D
  beq loc_A8DC
  jsr sub_A89E
  inc data_A89B
loc_A8DC:
  rts

sub_A8DD:
  lda #$00
  sta data_A89C
  ldy data_A89A
  beq loc_A908
  dey
  sty data_A89A
  jsr sub_A5FD
  cmp #$20
  bcs loc_A8FD
  jsr sub_A88A
  ldx #$08
  jsr sub_A784
  jsr sub_A88F
loc_A8FD:
  jsr sub_A88A
  ldx #$08
  jsr sub_A784
  jsr sub_A88F
loc_A908:
  dec data_A89B
  bpl loc_A910
  inc data_A89B
loc_A910:
  rts

sub_A911:
  jsr sub_A881
  jsr sub_A5C7
loc_A917:
  jsr sub_A5DC
  beq loc_A917
  jsr sub_A5CE
  rts
  lda #$00
  sta data_A89C
  jsr sub_A911
  beq loc_A951
  ldx data_A89B
  cpx #$3E
  beq loc_A951
loc_A931:
  inx
  cpx #$3E
  beq loc_A951
  cmp data_A571,x
  bne loc_A931
  ldx data_A89B
loc_A93E:
  pha
  lda data_A571,x
  jsr sub_A89E
  pla
  inc data_A89B
  ldx data_A89B
  cmp data_A571,x
  bne loc_A93E
loc_A951:
  rts
  lda #$00
  sta data_A89C
loc_A957:
  jsr sub_A8C7
  bne loc_A957
  rts
  lda #$00
  sta data_A89C
  jsr sub_A911
  cmp #$0D
  beq loc_A97D
  ldx data_A89B
loc_A96C:
  cmp data_A571,x
  beq loc_A97A
  ldy data_A571,x
  inx
  cpy #$0D
  bne loc_A96C
  rts
loc_A97A:
  stx data_A89B
loc_A97D:
  rts
  inc data_A89C
  rts
  ldx data_A89B
  lda data_A571,x
  cmp #$0D
  beq loc_A98F
  inc data_A89B
loc_A98F:
  rts
data_A990:
  .byte $3B, $3C, $3D, $3E, $4B, $4D, $52, $53
data_A998:
  .byte $A8, $A9, $A9, $A9, $A8, $A8, $A9, $A9
data_A9A0:
  .byte $C6, $1F, $51, $5C, $DC, $C6, $7D, $81

sub_A9A8:
  lda data_A998,x
  pha
  lda data_A9A0,x
  pha
  rts
loc_A9B1:
  lda var_00FF
  pha
  lda var_00FE
  pha
  lda var_00FD
  pha
  stx var_00FF
  sty var_00FE
  ldy #$00
  jsr sub_A5FD
  sta data_A899
  lda var_00FE
  clc
  adc #$02
  sta var_00FE
  bcc loc_A9D1
  inc var_00FF
loc_A9D1:
  lda #$00
  sta data_A89A
  sta data_A89B
  sta data_A89C
  lda #$1E
  jsr BIOS_ENTRY
  sty data_A89D
loc_A9E4:
  jsr sub_A911
  cmp #$08
  bne loc_A9F1
  jsr sub_A8DD
  jmp loc_A9E4
loc_A9F1:
  jsr sub_A701
  bcs loc_AA1F
  cmp #$1B
  bne loc_AA30
  ldx #$5C
  jsr sub_A784
  jsr sub_B3D1
  lda data_A89D
  beq loc_AA12
  sta var_00FD
loc_AA09:
  ldx #$20
  jsr sub_A784
  dec var_00FD
  bne loc_AA09
loc_AA12:
  lda #$00
  sta data_A89A
  sta data_A89B
  sta data_A89C
  beq loc_A9E4
loc_AA1F:
  txa
  ldx #$08
loc_AA22:
  dex
  bmi loc_A9E4
  cmp data_A990,x
  bne loc_AA22
  jsr sub_A9A8
  jmp loc_A9E4
loc_AA30:
  cmp #$0D
  beq loc_AA60
  cmp #$09
  bne loc_AA3A
  lda #$20
loc_AA3A:
  cmp #$03
  beq loc_AA58
  jsr sub_A89E
  bcs loc_A9E4
  lda data_A89C
  lsr a
  bcs loc_A9E4
  ldx data_A89B
  lda data_A571,x
  cmp #$0D
  beq loc_A9E4
  inc data_A89B
  bne loc_A9E4
loc_AA58:
  ldy #$00
  sty data_A89A
  sec
  bcs loc_AA6A
loc_AA60:
  pha
  jsr sub_A88A
  pla
  tax
  jsr sub_A784
  clc
loc_AA6A:
  php
  lda #$0D
  sta var_00FD
  ldy data_A89A
  jsr sub_A614
  cpy #$3E
  bcc loc_AA80
  lda #$0D
  sta data_A5AE
  ldy #$3C
loc_AA80:
  jsr sub_A5FD
  sta data_A571,y
  dey
  bpl loc_AA80
  lda var_00FE
  sec
  sbc #$02
  sta var_00FE
  bcs loc_AA94
  dec var_00FF
loc_AA94:
  ldx data_A89A
  stx var_00FD
  ldy #$01
  jsr sub_A614
  plp
  pla
  sta var_00FD
  pla
  sta var_00FE
  pla
  sta var_00FF
  rts

sub_AAA9:
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
  jsr sub_A827
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
loc_AAD7:
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
loc_AAF0:
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
  jsr sub_AAA9
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
  jsr sub_AAA9
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
  .byte $5C, $43, $4F, $4D, $4D, $41, $4E, $44, $2E, $43, $4D, $44, $00
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

sub_B174:
  stx var_00FF
  sty var_00FE
  ldy #$00
loc_B17A:
  jsr sub_A5FD
  tax
  beq loc_B183
  iny
  bne loc_B17A
loc_B183:
  dey
  bmi loc_B1B7
  jsr sub_A5FD
  cmp #$5C
  beq loc_B1B7
  cmp #$2E
  bne loc_B183
  iny
  sty var_00FD
  ldx #$00
loc_B196:
  lda #$03
  sta var_00FC
  ldy var_00FD
loc_B19C:
  jsr sub_A5FD
  and #$DF
  cmp data_B136,x
  bne loc_B1AE
  iny
  inx
  dec var_00FC
  bne loc_B19C
  clc
  rts
loc_B1AE:
  inx
  dec var_00FC
  bne loc_B1AE
  cpx #$09
  bne loc_B196
loc_B1B7:
  sec
  rts
loc_B1B9:
  lda var_00FF
  sta var_0401
  lda data_A53D
  sta var_0402
  jmp loc_B1D3

sub_B1C7:
  lda var_00FF
  sta var_0401
  stx var_00FF
  sty var_00FE
  jsr sub_B13F
loc_B1D3:
  stx var_0404
  sty var_0403
  jsr sub_B174
  ldy #$0B
  bcs loc_B25B
  cpx #$03
  bne loc_B20E
  ldy var_00FD
  dey
  ldx #$07
loc_B1E9:
  dey
  jsr sub_A5FD
  and #$DF
  cmp data_B129,x
  bne loc_B20C
  dex
  bne loc_B1E9
  lda #$FF
  sta var_0401
  lda data_A53A
  ldx #$B1
  ldy #$29
  sta var_0402
  stx var_0404
  sty var_0403
loc_B20C:
  ldx #$03
loc_B20E:
  stx var_00FF
  lda data_A548
  cmp #$28
  ldy #$2A
  bcs loc_B25B
  tay
  lda #$04
  sta var_00FE
  tsx
loc_B21F:
  lda var_0101,x
  inx
  sta data_A549,y
  iny
  dec var_00FE
  bne loc_B21F
  lda data_A53D
  sta data_A549,y
  iny
  sty data_A548
  lda #$14
  jsr sub_B4AA
  bcs loc_B24E
  lda var_00FF
  cmp #$03
  bne loc_B248
  jsr sub_B09B
  jmp loc_B250
loc_B248:
  jsr sub_AE98
  jmp loc_B250
loc_B24E:
  lda #$02
loc_B250:
  tay
  lda data_A548
  sec
  sbc #$05
  sta data_A548
  sec
loc_B25B:
  rts
  .byte $A2, $5E, $20, $D5, $A5, $A2, $43, $20, $D5, $A5, $A2, $0D, $20, $D5, $A5, $A2
  .byte $0A, $20, $D5, $A5
loc_B270:
  ldx #$FF
  stx data_A53F
  inx
  stx data_A540
  jmp loc_A6E2
  .byte $44, $45, $56, $49, $43, $45, $20, $6D, $75, $73, $74, $20, $43, $4D, $44, $20
  .byte $6F, $72, $20, $53, $59, $53, $20, $65, $78, $74, $65, $6E, $64, $24, $0A, $42
  .byte $61, $64, $20, $6F, $72, $20, $6D, $69, $73, $73, $69, $6E, $67, $20, $24
data_B2AB:
  .byte $44
data_B2AC:
  .byte $45
data_B2AD:
  .byte $56, $49, $43, $45, $00, $20, $B4, $53, $48, $45, $4C, $4C, $00, $3F, $B4, $00
data_B2BD:
  .byte $00
data_B2BE:
  .byte $00
data_B2BF:
  .byte $30, $30, $30, $31, $45, $72, $72, $6F, $72, $20, $69, $6E, $20, $43, $4F, $4E
  .byte $46, $49, $47, $2E, $53, $59, $53, $20, $6C, $69, $6E, $65, $20
data_B2DC:
  .byte $58, $58, $58, $58, $0D, $0A, $24
data_B2E3:
  .byte $00
data_B2E4:
  .res 128, $00
  .byte $B9, $32, $61, $C9, $0D, $F0, $0F, $AA, $B9, $33, $61, $C9, $3A, $D0, $07, $8A
  .byte $E9, $40, $C8, $C8, $18, $60, $A9, $00, $18, $60, $C9, $0D, $F0, $06, $C9, $20
  .byte $F0, $02, $C9, $09, $60, $A2, $00, $B9, $32, $61, $20, $7E, $B3, $F0, $07, $9D
  .byte $82, $61, $E8, $C8, $D0, $F1, $A9, $00, $9D, $82, $61, $A2, $00, $B9, $32, $61
  .byte $9D, $01, $01, $C8, $E8, $C9, $0D, $D0, $F4, $CA, $8E, $00, $01, $A2, $61, $A0
  .byte $82, $60

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
  .byte $48, $4A, $4A, $4A, $4A, $20, $EE, $B3, $68, $29, $0F, $18, $69, $30, $9D, $DC
  .byte $B2, $E8, $60
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
  .byte $AD, $01, $04, $85, $FF, $A2, $61, $A0, $82, $20, $B9, $B1, $90, $0A, $20, $F6
  .byte $B3, $A2, $B2, $A0, $9A, $4C, $CE, $B3, $60, $20, $64, $B3, $8D, $01, $04, $20
  .byte $89, $B3, $20, $74, $B1, $B0, $07, $E0, $03, $D0, $03, $4C, $08, $B4, $A2, $B2
  .byte $A0, $7C, $20, $CE, $B3, $4C, $F6, $B3, $20, $64, $B3, $8D, $01, $04, $20, $89
  .byte $B3, $4C, $08, $B4

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
  jsr sub_B1C7
  ldx #$B4
  ldy #$E4
loc_B5E0:
  jsr sub_B3CE
loc_B5E3:
  jmp loc_B5E3
