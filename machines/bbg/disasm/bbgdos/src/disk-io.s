; 模块：disk-io
; 磁盘扇区读写：LBA 到柱面、磁头、扇区的换算，经 BIOS 功能 6 读写，介质检测与引导扇区参数解析

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_049E"
; 段 0，$809E-$83A7
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
