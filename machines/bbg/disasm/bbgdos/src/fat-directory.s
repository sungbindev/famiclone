; 模块：fat-directory
; FAT12 与目录：簇与扇区换算、FAT 缓存、目录项查找

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_07A8"
; 段 0，$83A8-$8A14
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
