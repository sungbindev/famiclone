; 模块：bios-core
; BIOS 功能实现：磁盘 I/O、块移动、键盘读取、显示、光标、NMI 向量（未确认：边界）

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG1_1028"
; 段 1，$D251-$E1F4
bios_function_13:
  lda var_00FF
  pha
  lda var_00FE
  pha
  lda var_00FD
  pha
  lda var_00FC
  pha
  stx var_00FD
  sty var_00FC
  lda var_5FFA
  pha
  tax
  lda var_00FD
  cmp #$C0
  bcc loc_D278
  and #$BF
  sta var_00FD
  ldx #$FF
  stx var_5FFA
  stx loc_FF04
loc_D278:
  ldy #$04
  lda (var_00FC),y
  cmp #$C0
  php
  bcc loc_D28B
  and #$BF
  sta (var_00FC),y
  ldy #$02
  lda #$FF
  sta (var_00FC),y
loc_D28B:
  txa
  pha
  lda #$01
  jsr sub_D146
  ldy #$00
loc_D294:
  lda (var_00FC),y
  jsr sub_D146
  iny
  cpy #$10
  bne loc_D294
  jsr sub_D21B
  pla
  sta var_5FFA
  sta loc_FF04
  ldy #$00
loc_D2AA:
  jsr bios_function_36
  sta (var_00FC),y
  iny
  cpy #$0E
  bne loc_D2AA
  plp
  bcc loc_D2C0
  ldy #$04
  lda (var_00FC),y
  clc
  adc #$40
  sta (var_00FC),y
loc_D2C0:
  pla
  sta var_5FFA
  sta loc_FF04
  pla
  sta var_00FC
  pla
  sta var_00FD
  pla
  sta var_00FE
  pla
  sta var_00FF

bios_no_operation:
  rts

sub_D2D4:
  lda var_00FF
  cmp #$C0
  bcc loc_D2E0
  and #$BF
  sta var_00FF
  ldx #$FF
loc_D2E0:
  lda var_5FFA
  jsr bios_set_page
  tax
  rts

; BIOS 功能 32 BIOS_GET_PAGED_CHAR：读其他页的一个字节。X 为页号，零页 $FE、$FF 为地址，Y 为偏移；A 返回字节，页恢复原值。
bios_get_paged_char:
  lda var_00FF
  sta var_5FB2
  jsr sub_D2D4
  lda (var_00FE),y
  stx var_5FFA
  stx loc_FF04
  ldx var_5FB2
  stx var_00FF
  rts

; BIOS 功能 33 BIOS_SET_PAGED_CHAR：写其他页的一个字节。X 为页号，零页 $FE、$FF 为地址，Y 为偏移，零页 $FD 为要写的值。
bios_set_paged_char:
  lda var_00FF
  sta var_5FB2
  jsr sub_D2D4
  lda var_00FD
  sta (var_00FE),y
  stx var_5FFA
  stx loc_FF04
  ldx var_5FB2
  stx var_00FF
  rts

sub_D316:
  cpx #$00
  bne loc_D31E
  ldx #$FE
  bne loc_D325
loc_D31E:
  cpx #$FE
  bne loc_D325
  ldx var_5FF9
loc_D325:
  stx var_5FFA
  stx loc_FF04
  rts

; BIOS 功能 3 BIOS_SET_AV10：X 的位 0、1 写入系统控制寄存器映像 [$5FFF] 的位 0、1。
bios_set_av10:
  txa
  and #$03
  sta var_5FB2
  lda var_5FFF
  and #$FC
  ora var_5FB2
  sta var_5FFF
  rts

sub_D33E:
  ldx #$04
  stx sub_FF01+2
  inx
  stx loc_FF0B
  inx
  stx sub_FF11+2
  inx
  stx bios_function_38+2
  rts

sub_D350:
  lda #$00
  sta var_5FBD
  sta var_5FBB
  sta var_5FF6
  sta var_5FF7
  sta var_5FC5
  sta var_5FC6
  sta var_5FC4
  sta var_5FC2
  sta var_5FC3
  sta var_5FC1
  ldx #$58
  ldy #$B2
  jmp sub_FF01

bios_run_initial:
  jsr sub_D350
  jsr sub_D093
  sei
  lda #$FF
  sta loc_FF04
  ldx #$59
  ldy #$96
  stx var_BFFF
  sty var_BFFE
  ldx #$58
  ldy #$C7
  stx var_BFFB
  sty var_BFFA
  lda var_5FFA
  sta loc_FF04
  ldx #$59
  ldy #$44
  stx loc_5941+2
  sty loc_5941+1
  ldx #$58
  ldy #$91
  stx data_58B0
  sty data_58AF
  jsr bios_set_nmi_vector
  jsr sub_D09C
  sta var_6000
  jsr sub_D33E
  lda #$00
  sta var_4015
  lda #$C0
  sta var_4017
  rts
data_D3C8:
data_D3C9 = * + 1
  .word sub_D449 - 1, loc_D489 - 1, loc_D580 - 1, loc_D57C - 1
  .word loc_D580 - 1, loc_D4B2 - 1, loc_D49D - 1

bios_disk_io:
  txa
  pha
  ldx #$0D
loc_D3DA:
  lda var_0000,x
  sta var_5EA0,x
  dex
  bpl loc_D3DA
  lda var_5FFA
  sta var_0009
  pla
  cmp #$C0
  bcc loc_D3F6
  and #$BF
  ldx #$FF
  stx var_5FFA
  stx loc_FF04
loc_D3F6:
  sta var_000B
  sty var_000A
  ldy #$08
loc_D3FC:
  lda (var_000A),y
  sta a:var_0000,y
  dey
  bpl loc_D3FC
  lda var_0000
  asl a
  cmp #$0E
  bcc loc_D412
  lda #$01
  sta var_5FA1
  bne loc_D415
loc_D412:
  jsr sub_D426
loc_D415:
  bit var_2002
  jsr sub_FF11
  lda #$64
  sta var_5FA0
  lda var_5FA1
  jmp loc_D430

sub_D426:
  tax
  lda data_D3C9,x
  pha
  lda data_D3C8,x
  pha
  rts
loc_D430:
  pha
  lda var_0009
  sta var_5FFA
  sta loc_FF04
  txa
  pha
  ldx #$0D
loc_D43D:
  lda var_5EA0,x
  sta var_0000,x
  dex
  bpl loc_D43D
  pla
  tax
  pla
  rts

sub_D449:
  lda var_5FA3
  ora var_0001
  and #$FB
  sta data_FF90
  ldx #$00
  stx var_5FA1
  stx var_5FA2
  ora #$04
  sta data_FF90
  jsr sub_D9C6
  lda var_5FAB
  cmp #$C0
  bne loc_D47F
  lda #$02
  sta data_FFB8
  lda #$03
  jsr sub_D8C3
  lda #$DF
  jsr sub_D8C3
  lda #$02
  jsr sub_D8C3
  rts
loc_D47F:
  lda var_5FA1
  ora #$20
  sta var_5FA1
  sec
loc_D488:
  rts
loc_D489:
  lda var_0001
  clc
  adc #$01
  bit var_5FA4
  beq loc_D496
  jsr sub_D8F9
loc_D496:
  ldx var_5FA4
  ldy data_FFB8
  rts
loc_D49D:
  lda #$00
  ldx var_0004
  cpx #$12
  beq loc_D4A7
  lda #$02
loc_D4A7:
  ldx var_0001
  sta var_5FA7,x
  lda #$01
  sta var_5FA5,x
  rts
loc_D4B2:
  jsr sub_D5DF
  bcs loc_D488
  ldx var_0001
  lda var_5FA7,x
  sta data_FFB8
  jsr sub_D5C2
  bcs loc_D488
  jsr sub_DA73
  lda #$12
  ldx var_0001
  ldy var_5FA7,x
  beq loc_D4D2
  lda #$09
loc_D4D2:
  sta var_5FB4
  asl a
  asl a
  sta var_5FB3
  adc var_000A
  sta var_000A
  bcs loc_D4E2
  dec var_000B
loc_D4E2:
  lda #$01
  sec
  sbc var_5FB3
  sta var_5FB3
  lda #$CD
  jsr sub_D8C3
  lda var_0003
  asl a
  asl a
  ora var_0001
  jsr sub_D8C3
  lda #$02
  jsr sub_D8C3
  lda var_5FB4
  jsr sub_D8C3
  lda #$6C
  ldx var_0001
  ldy var_5FA7,x
  beq loc_D50F
  lda #$50
loc_D50F:
  jsr sub_D8C3
  lda #$F6
  jsr sub_D8C3
  ldy #$FF
  lda (var_000A),y
  pha
  lda var_000A
  bne loc_D522
  dec var_000B
loc_D522:
  dec var_000A
  ldx #$00
  ldy var_5FB3
  lda (var_000A),y
loc_D52B:
  bit data_FF90
  bvs loc_D53D
  dey
  bne loc_D52B
  bit data_FF90
  bvs loc_D53D
  dex
  bne loc_D52B
  beq loc_D573
loc_D53D:
  ldy var_5FB3
loc_D540:
  sta data_FF80
  iny
  beq loc_D568
  lda (var_000A),y
  ldx #$40
  bit data_FF90
  bvs loc_D540
  stx var_5FB4
  ldx #$00
loc_D554:
  bit data_FF90
  bvs loc_D540
  dex
  bne loc_D554
  bit data_FF90
  bvs loc_D540
  dec var_5FB4
  bne loc_D554
  beq loc_D573
loc_D568:
  pla
  ldx #$00
loc_D56B:
  bit data_FF90
  bvs loc_D576
  dex
  bne loc_D56B
loc_D573:
  jmp loc_D86C
loc_D576:
  sta data_FF88
  jmp loc_D87F
loc_D57C:
  lda #$C5
  bne loc_D582
loc_D580:
  lda #$E6
loc_D582:
  sta var_5FB3
  jsr sub_D62F
  lda #$00
  ldy var_0003
  cpy var_5FAF
  beq loc_D5A5
  clc
  adc #$09
  ldx var_0001
  ldy var_5FA7,x
  bne loc_D59D
  adc #$09
loc_D59D:
  ldy var_0003
  beq loc_D5A5
  eor #$FF
  adc #$01
loc_D5A5:
  ldy var_5FAE
  cpy var_0002
  beq loc_D5B9
  clc
  adc #$12
  ldx var_0001
  ldy var_5FA7,x
  bne loc_D5B9
  clc
  adc #$12
loc_D5B9:
  clc
  adc var_5FB0
  sec
  sbc var_0004
  tay
  rts

sub_D5C2:
  lda #$03
  jsr sub_D8C3
  lda #$BF
  ldx var_0001
  ldy var_5FA7,x
  beq loc_D5D2
  lda #$DF
loc_D5D2:
  jsr sub_D8C3
  lda #$02
  jsr sub_D8C3
  lda var_0002
  jmp sub_D920

sub_D5DF:
  jsr sub_D8F9
  lda #$00
  sta var_5FA1
  lda data_FFB8
  bpl loc_D617
  ldx var_0001
  lda #$00
  sta var_5FA5,x
  jsr sub_D449
  bcc loc_D5FD
  jsr sub_D449
  bcs loc_D616
loc_D5FD:
  lda #$01
  jsr sub_D920
  bcs loc_D616
  lda #$00
  jsr sub_D920
  bcs loc_D616
  ldx data_FFB8
  bpl loc_D617
  lda #$06
loc_D612:
  sta var_5FA1
  sec
loc_D616:
  rts
loc_D617:
  jsr bios_wait_display
  ldx #$01
  jsr bios_wait_vsync_count
  lda var_5FC1
  beq loc_D629
  lda #$00
  sta var_2001
loc_D629:
  sei
  jsr sub_D093
  clc
  rts

sub_D62F:
  jsr sub_D5DF
  bcs loc_D616
  ldx var_0001
  lda var_5FA5,x
  bne loc_D676
  lda var_0001
  clc
  adc #$01
  ora var_5FA2
  sta var_5FA2
  jsr sub_D9B8
  bcc loc_D650
  jsr sub_D9B8
  bcs loc_D616
loc_D650:
  lda #$00
  jsr sub_D97D
  bcs loc_D616
  beq loc_D666
  lda #$02
  jsr sub_D97D
  bcs loc_D616
  beq loc_D66A
  lda #$02
  bne loc_D612
loc_D666:
  lda #$00
  beq loc_D66C
loc_D66A:
  lda #$02
loc_D66C:
  ldx var_0001
  sta var_5FA7,x
  lda #$01
  sta var_5FA5,x
loc_D676:
  lda var_5FA7,x
  sta data_FFB8
  jsr sub_D5C2
  bcs loc_D616
  lda #$00
  sta var_5FA0
  lda var_0002
  jsr sub_D920
  bcs loc_D616
  jsr sub_DA73
  ldy var_0000
  cpy #$03
  beq loc_D697
  clc
loc_D697:
  adc #$FF
  sta var_000C
  txa
  adc #$00
  sta var_000D
  lda var_5FB3
  jsr sub_D8C3
  lda var_0003
  asl a
  asl a
  ora var_0001
  jsr sub_D8C3
  lda var_0002
  jsr sub_D8C3
  lda var_0003
  jsr sub_D8C3
  lda var_0004
  jsr sub_D8C3
  lda #$02
  jsr sub_D8C3
  lda #$12
  ldx var_0001
  ldy var_5FA7,x
  beq loc_D6CE
  lda #$09
loc_D6CE:
  jsr sub_D8C3
  lda #$1B
  ldx var_0001
  ldy var_5FA7,x
  beq loc_D6DC
  lda #$2A
loc_D6DC:
  jsr sub_D8C3
  lda #$FF
  jsr sub_D8C3
  lda var_0008
  ldx var_0000
  cpx #$02
  bne loc_D757
loc_D6EC:
  pha
  ldx #$00
  ldy #$00
loc_D6F1:
  bit data_FF90
  bvs loc_D703
  dey
  bne loc_D6F1
  bit data_FF90
  bvs loc_D703
  dex
  bne loc_D6F1
  beq loc_D737
loc_D703:
  ldy #$00
loc_D705:
  lda data_FF80
  sta (var_000A),y
  iny
  beq loc_D71E
  ldx #$00
loc_D70F:
  bit data_FF90
  bvs loc_D705
  dex
  bne loc_D70F
  beq loc_D737
loc_D719:
  lda data_FF80
  sta (var_000C),y
loc_D71E:
  iny
  beq loc_D72D
  ldx #$00
loc_D723:
  bit data_FF90
  bvs loc_D719
  dex
  bne loc_D723
  beq loc_D737
loc_D72D:
  ldx #$00
loc_D72F:
  bit data_FF90
  bvs loc_D73A
  dex
  bne loc_D72F
loc_D737:
  jmp loc_D86C
loc_D73A:
  pla
  tax
  dex
  bne loc_D744
  lda data_FF88
  bvs loc_D747
loc_D744:
  lda data_FF80
loc_D747:
  inc var_000D
  sta (var_000C),y
  inc var_000D
  inc var_000B
  inc var_000B
  txa
  bne loc_D6EC
  jmp loc_D87F
loc_D757:
  cpx #$04
  bne loc_D7B8
loc_D75B:
  pha
  ldx #$00
  ldy #$00
loc_D760:
  bit data_FF90
  bvs loc_D772
  dey
  bne loc_D760
  bit data_FF90
  bvs loc_D772
  dex
  bne loc_D760
  beq loc_D7A2
loc_D772:
  ldy #$00
loc_D774:
  lda data_FF80
  iny
  beq loc_D789
  ldx #$00
loc_D77C:
  bit data_FF90
  bvs loc_D774
  dex
  bne loc_D77C
  beq loc_D7A2
loc_D786:
  lda data_FF80
loc_D789:
  iny
  beq loc_D798
  ldx #$00
loc_D78E:
  bit data_FF90
  bvs loc_D786
  dex
  bne loc_D78E
  beq loc_D7A2
loc_D798:
  ldx #$00
loc_D79A:
  bit data_FF90
  bvs loc_D7A5
  dex
  bne loc_D79A
loc_D7A2:
  jmp loc_D86C
loc_D7A5:
  pla
  tax
  dex
  bne loc_D7AF
  lda data_FF88
  bvs loc_D7B2
loc_D7AF:
  lda data_FF80
loc_D7B2:
  txa
  bne loc_D75B
  jmp loc_D87F
loc_D7B8:
  tax
  dex
  beq loc_D813
  txa
  pha
  ldx #$00
  ldy #$00
  lda (var_000A),y
loc_D7C4:
  bit data_FF90
  bvs loc_D7D6
  dey
  bne loc_D7C4
  bit data_FF90
  bvs loc_D7D6
  dex
  bne loc_D7C4
  beq loc_D803
loc_D7D6:
  ldy #$00
loc_D7D8:
  sta data_FF80
  iny
  beq loc_D7F2
  lda (var_000A),y
  ldx #$00
loc_D7E2:
  bit data_FF90
  bvs loc_D7D8
  dex
  bne loc_D7E2
  beq loc_D803
loc_D7EC:
  sta data_FF80
  iny
  beq loc_D806
loc_D7F2:
  lda (var_000C),y
  bit data_FF90
  bvs loc_D7EC
  ldx #$00
loc_D7FB:
  bit data_FF90
  bvs loc_D7EC
  dex
  bne loc_D7FB
loc_D803:
  jmp loc_D86C
loc_D806:
  inc var_000D
  inc var_000D
  inc var_000B
  inc var_000B
  pla
  bne loc_D7B8
  beq loc_D87F
loc_D813:
  ldy #$FF
  lda (var_000C),y
  pha
  lda var_000C
  bne loc_D81E
  dec var_000D
loc_D81E:
  dec var_000C
  ldx #$00
  iny
  lda (var_000A),y
loc_D825:
  bit data_FF90
  bvs loc_D837
  dey
  bne loc_D825
  bit data_FF90
  bvs loc_D837
  dex
  bne loc_D825
  beq loc_D86C
loc_D837:
  ldy #$00
loc_D839:
  sta data_FF80
  iny
  beq loc_D850
  lda (var_000A),y
  ldx #$00
loc_D843:
  bit data_FF90
  bvs loc_D839
  dex
  bne loc_D843
  beq loc_D86C
loc_D84D:
  sta data_FF80
loc_D850:
  iny
  beq loc_D861
  lda (var_000C),y
  ldx #$00
loc_D857:
  bit data_FF90
  bvs loc_D84D
  dex
  bne loc_D857
  beq loc_D86C
loc_D861:
  pla
  ldx #$00
loc_D864:
  bit data_FF90
  bvs loc_D87C
  dex
  bne loc_D864
loc_D86C:
  pla
  bit data_FF98
  bvs loc_D884
  lda var_5FA1
  ora #$80
  sta var_5FA1
  sec
  rts
loc_D87C:
  sta data_FF88
loc_D87F:
  jsr sub_D9EB
  bcs loc_D8C2
loc_D884:
  lda #$07
  jsr sub_DA0F
  bcs loc_D8C2
  lda var_5FAB
  and #$C0
  beq loc_D8C2
  cmp #$40
  bne loc_D8B9
  lda var_5FAC
  asl a
  ldx #$04
  bcs loc_D8BB
  asl a
  asl a
  ldx #$10
  bcs loc_D8BB
  asl a
  ldx #$08
  bcs loc_D8BB
  asl a
  asl a
  ldx #$04
  bcs loc_D8BB
  asl a
  ldx #$03
  bcs loc_D8BB
  asl a
  ldx #$02
  bcs loc_D8BB
loc_D8B9:
  ldx #$20
loc_D8BB:
  txa
  ora var_5FA1
  sta var_5FA1
loc_D8C2:
  rts

sub_D8C3:
  pha
  ldx #$00
  ldy #$00
loc_D8C8:
  lda data_FFA0
  and #$40
  beq loc_D8D7
  dey
  bne loc_D8C8
  dex
  bne loc_D8C8
  beq loc_D8E6
loc_D8D7:
  ldx #$00
  ldy #$00
loc_D8DB:
  lda data_FFA0
  bmi loc_D8F3
  dey
  bne loc_D8DB
  dex
  bne loc_D8DB
loc_D8E6:
  pla
  lda var_5FA1
  ora #$80
  sta var_5FA1
  pla
  pla
  sec
  rts
loc_D8F3:
  pla
  sta data_FFA8
  clc
  rts

sub_D8F9:
  lda #$00
  sta var_5FA0
  lda var_0001
  clc
  adc #$1F
  and #$30
  bit var_5FA3
  php
  ora var_5FA3
  sta var_5FA3
  ora var_0001
  sta data_FF90
  plp
  bne loc_D91A
  jsr sub_DA5F
loc_D91A:
  lda #$00
  sta var_5FA0
  rts

sub_D920:
  sta var_5FB2
  lda var_0001
  clc
  adc #$01
  bit var_5FA2
  bne loc_D93E
  jsr sub_D9B8
  bcc loc_D937
  jsr sub_D9B8
  bcs loc_D97C
loc_D937:
  lda #$00
  ldx var_0001
  sta var_5FA9,x
loc_D93E:
  lda var_5FB2
  ldx var_0001
  cmp var_5FA9,x
  clc
  beq loc_D97C
  lda #$0F
  jsr sub_D8C3
  lda var_0001
  jsr sub_D8C3
  lda var_5FB2
  jsr sub_D8C3
  jsr sub_D9C6
  ldx #$0F
loc_D95E:
  ldy #$8A
loc_D960:
  pha
  pla
  dey
  bne loc_D960
  dex
  bne loc_D95E
  bcs loc_D97C
  lda var_0001
  tax
  clc
  adc #$01
  ora var_5FA2
  sta var_5FA2
  lda var_5FAC
  sta var_5FA9,x
loc_D97C:
  rts

sub_D97D:
  sta data_FFB8
  lda #$05
  sta var_5FB2
loc_D985:
  lda #$4A
  jsr sub_D8C3
  lda var_0001
  jsr sub_D8C3
  jsr sub_D9EB
  bcs loc_D9B7
  lda #$07
  jsr sub_DA0F
  bcs loc_D9B7
  lda var_5FAB
  and #$C0
  beq loc_D9B7
  dec var_5FB2
  bne loc_D985
  lda #$08
  bit var_5FAB
  beq loc_D9B5
  lda #$80
  sta var_5FA1
  sec
  rts
loc_D9B5:
  lda #$01
loc_D9B7:
  rts

sub_D9B8:
  lda #$07
  jsr sub_D8C3
  lda var_0001
  jsr sub_D8C3
  jsr sub_D9C6
  rts

sub_D9C6:
  jsr sub_D9EB
  bcs loc_D9EA
  lda #$08
  jsr sub_D8C3
  lda #$02
  jsr sub_DA0F
  bcs loc_D9EA
  lda var_5FAB
  and #$60
  cmp #$60
  clc
  bne loc_D9EA
  lda var_5FA1
  ora #$40
  sta var_5FA1
  sec
loc_D9EA:
  rts

sub_D9EB:
  lda #$0A
loc_D9ED:
  pha
  ldx #$76
loc_D9F0:
  ldy #$00
loc_D9F2:
  bit data_FF98
  bvs loc_DA0C
  dey
  bne loc_D9F2
  dex
  bne loc_D9F0
  pla
  sec
  sbc #$01
  bne loc_D9ED
  lda var_5FA1
  ora #$80
  sta var_5FA1
  rts
loc_DA0C:
  pla
  clc
  rts

sub_DA0F:
  ldx #$00
loc_DA11:
  pha
  txa
  pha
  ldx #$00
  ldy #$00
loc_DA18:
  lda data_FFA0
  bmi loc_DA2F
  dey
  bne loc_DA18
  dex
  bne loc_DA18
  lda var_5FA1
  ora #$80
  sta var_5FA1
  pla
  pla
  sec
  rts
loc_DA2F:
  lda data_FFA0
  and #$40
  beq loc_DA53
  pla
  tax
  lda data_FFA8
  sta var_5FAB,x
  inx
  pla
  sec
  sbc #$01
  bne loc_DA11
  ldx #$05
loc_DA47:
  dex
  bne loc_DA47
  lda #$10
  bit data_FFA0
  bne loc_DA55
  clc
  rts
loc_DA53:
  pla
  pla
loc_DA55:
  lda var_5FA1
  ora #$20
  sta var_5FA1
  sec
  rts

sub_DA5F:
  lda #$32
loc_DA61:
  jsr sub_DA6A
  sec
  sbc #$01
  bne loc_DA61
  rts

sub_DA6A:
  ldy #$1C
loc_DA6C:
  dex
  bne loc_DA6C
  dey
  bne loc_DA6C
  rts

sub_DA73:
  lda var_0007
  ldx var_0005
  cmp #$C0
  bcc loc_DA7F
  and #$BF
  ldx #$FF
loc_DA7F:
  sta var_000B
  cpx #$00
  bne loc_DA89
  ldx #$FE
  bne loc_DA90
loc_DA89:
  cpx #$FE
  bne loc_DA90
  ldx var_5FF9
loc_DA90:
  stx var_5FFA
  stx loc_FF04
  tax
  lda var_0006
  sta var_000A
  rts

bios_function_9:
  jsr sub_D093
  lda var_5FFA
  pha
  lda #$FF
  sta loc_FF04
  ldx var_BFFF
  ldy var_BFFE
  jsr sub_FF01
  ldx var_BFFB
  ldy var_BFFA
  stx loc_5941+2
  sty loc_5941+1
  ldx #$59
  ldy #$96
  stx var_BFFF
  sty var_BFFE
  ldx #$58
  ldy #$C7
  stx var_BFFB
  sty var_BFFA
  pla
  sta loc_FF04
  jsr sub_FF11
  lda #$80
  sta var_5FBD
  rts

bios_function_31:
  cpx #$FE
  bne loc_DAE5
  ldx var_5FF9
loc_DAE5:
  stx var_5FBE
  jsr bios_function_9
  lda #$C0
  sta var_5FBD
  rts

bios_function_25:
  jsr sub_D093
  lda var_5FFA
  pha
  lda #$FF
  sta loc_FF04
  ldx loc_599E+2
  ldy loc_599E+1
  stx var_BFFF
  sty var_BFFE
  ldx loc_5941+2
  ldy loc_5941+1
  stx var_BFFB
  sty var_BFFA
  pla
  sta loc_FF04
  jmp sub_FF11

bios_function_26:
  lda #$00
  rts

bios_function_10:
  lda var_5FF5
  rts

bios_read_key:
  stx var_5FB3
  sty var_5FB4
  txa
  and #$07
  beq loc_DB8A
  txa
  and #$10
  beq loc_DB54
  txa
  lsr a
  bcc loc_DB3D
  ldx var_5FFC
  stx var_5FB5
loc_DB3D:
  lsr a
  bcc loc_DB46
  ldx var_5FFD
  stx var_5FB6
loc_DB46:
  lsr a
  bcc loc_DB54
loc_DB49:
  jsr sub_59D6
  beq loc_DB54
  jsr sub_F6D0
  jmp loc_DB49
loc_DB54:
  lda var_5FB3
  bpl loc_DB5E
  ldx #$01
  jsr bios_wait_vsync_count
loc_DB5E:
  lda var_5FB3
  lsr a
  bcc loc_DB8B
  lda var_5FFC
  eor var_5FB5
  bne loc_DB7A
  lda var_5FB4
  beq loc_DB8B
  lda var_5FB3
  and #$06
  bne loc_DB8B
  beq loc_DB88
loc_DB7A:
  tay
  lda var_5FB3
  and #$10
  beq loc_DB88
  tya
  and var_5FFC
  beq loc_DB8B
loc_DB88:
  lda #$00
loc_DB8A:
  rts
loc_DB8B:
  lda var_5FB3
  and #$02
  beq loc_DBB9
  lda var_5FFD
  eor var_5FB6
  bne loc_DBA8
  lda var_5FB4
  beq loc_DBB9
  lda var_5FB3
  and #$04
  bne loc_DBB9
  beq loc_DBB6
loc_DBA8:
  tay
  lda var_5FB3
  and #$10
  beq loc_DBB6
  tya
  and var_5FFD
  beq loc_DBB9
loc_DBB6:
  lda #$01
  rts
loc_DBB9:
  lda var_5FB3
  and #$04
  beq loc_DBDF
  jsr sub_59D6
  cpy #$00
  beq loc_DBD7
  ldy var_5FB4
  bne loc_DBCF
  jsr sub_F6D0
loc_DBCF:
  pha
  txa
  tay
  pla
  tax
  lda #$03
  rts
loc_DBD7:
  lda var_5FB4
  beq loc_DBDF
  lda #$02
  rts
loc_DBDF:
  jmp loc_DB54

sub_DBE2:
  lda #$01
  sta var_4016
  lda #$00
  sta var_4016
  lda #$80
loc_DBEE:
  pha
  lda var_4016,x
  lsr a
  pla
  ror a
  bcc loc_DBEE
  rts

sub_DBF8:
  cmp #$C0
  bcc loc_DC00
  and #$BF
  ldy #$FF
loc_DC00:
  cpy #$00
  bne loc_DC07
  ldy #$FE
  rts
loc_DC07:
  cpy #$FE
  bne loc_DC0E
  ldy var_5FF9
loc_DC0E:
  rts

bios_move_block:
  ldy var_0000,x
  lda var_0002,x
  pha
  jsr sub_DBF8
  sta var_0002,x
  sty var_5FB3
  ldy var_0003,x
  lda var_0005,x
  pha
  jsr sub_DBF8
  sta var_0005,x
  sty var_5FB4
  ldy var_0006,x
  beq loc_DC31
  inc var_0007,x
  ldy #$00
loc_DC31:
  sty var_5FB2
  lda var_5FFA
  pha
loc_DC38:
  lda var_0007,x
  cmp #$01
  bne loc_DC43
  lda var_0006,x
  sta var_5FB2
loc_DC43:
  lda var_5FB3
  sta var_5FFA
  sta loc_FF04
loc_DC4C:
  lda (var_0001,x)
  sta var_5EA0,y
  inc var_0001,x
  bne loc_DC57
  inc var_0002,x
loc_DC57:
  iny
  cpy var_5FB2
  bne loc_DC4C
  ldy #$00
  lda var_5FB4
  sta var_5FFA
  sta loc_FF04
loc_DC68:
  lda var_5EA0,y
  sta (var_0004,x)
  inc var_0004,x
  bne loc_DC73
  inc var_0005,x
loc_DC73:
  iny
  cpy var_5FB2
  bne loc_DC68
  dec var_0007,x
  bne loc_DC38
  jmp loc_DD1D

bios_exchange_block:
  ldy var_0000,x
  lda var_0002,x
  pha
  jsr sub_DBF8
  sta var_0002,x
  sty var_5FB3
  ldy var_0003,x
  lda var_0005,x
  pha
  jsr sub_DBF8
  sta var_0005,x
  sty var_5FB4
  ldy var_0006,x
  beq loc_DCA2
  inc var_0007,x
  ldy #$00
loc_DCA2:
  sty var_5FB2
  lda var_5FFA
  pha
  lda var_5FB3
  sta var_5FFA
  sta loc_FF04
loc_DCB2:
  lda var_0007,x
  cmp #$01
  bne loc_DCBD
  lda var_0006,x
  sta var_5FB2
loc_DCBD:
  lda var_0002,x
  pha
  lda var_0001,x
  pha
loc_DCC3:
  lda (var_0001,x)
  sta var_5EA0,y
  inc var_0001,x
  bne loc_DCCE
  inc var_0002,x
loc_DCCE:
  iny
  cpy var_5FB2
  bne loc_DCC3
  ldy #$00
  lda var_5FB4
  sta var_5FFA
  sta loc_FF04
loc_DCDF:
  lda (var_0004,x)
  pha
  lda var_5EA0,y
  sta (var_0004,x)
  pla
  sta var_5EA0,y
  inc var_0004,x
  bne loc_DCF1
  inc var_0005,x
loc_DCF1:
  iny
  cpy var_5FB2
  bne loc_DCDF
  pla
  sta var_0001,x
  pla
  sta var_0002,x
  lda var_5FB3
  sta var_5FFA
  sta loc_FF04
  ldy #$00
loc_DD08:
  lda var_5EA0,y
  sta (var_0001,x)
  inc var_0001,x
  bne loc_DD13
  inc var_0002,x
loc_DD13:
  iny
  cpy var_5FB2
  bne loc_DD08
  dec var_0007,x
  bne loc_DCB2
loc_DD1D:
  pla
  sta var_5FFA
  sta loc_FF04
  pla
  cmp #$C0
  bcc loc_DD2F
  lda var_0005,x
  adc #$3F
  sta var_0005,x
loc_DD2F:
  pla
  cmp #$C0
  bcc loc_DD3A
  lda var_0002,x
  adc #$3F
  sta var_0002,x
loc_DD3A:
  rts

bios_wait_vsync_count:
  stx var_5FF8
  jsr sub_D09C
loc_DD41:
  lda var_5FF8
  bne loc_DD41
  jmp sub_FF11

bios_low_ram_alloc:
  stx var_5FB3
  sty var_5FB2
  lda var_5FBA
  cmp #$58
  bcc loc_DD72
  lda var_5FB9
  clc
  adc var_5FB2
  tay
  lda var_5FBA
  adc var_5FB3
  cmp #$5C
  bcc loc_DD95
  ldx #$4D
  ldy #$00
  stx var_5FBA
  sty var_5FB9
loc_DD72:
  lda var_5FB9
  clc
  adc var_5FB2
  tay
  lda var_5FBA
  adc var_5FB3
  tax
  cmp #$58
  bcc loc_DD95
  lda #$58
  sec
  sbc var_5FB9
  tay
  lda #$00
  sbc var_5FBA
  tax
  lda #$01
  rts
loc_DD95:
  tax
  lda var_5FBA
  stx var_5FBA
  tax
  lda var_5FB9
  sty var_5FB9
  tay
  lda #$00
  rts

bios_low_ram_free:
  stx var_5FB3
  sty var_5FB2
  lda var_5FB9
  sec
  sbc var_5FB2
  sta var_5FB9
  lda var_5FBA
  sbc var_5FB3
  cmp #$4D
  bcs loc_DDC3
  adc #$0F
loc_DDC3:
  sta var_5FBA
  rts

sub_DDC7:
  lda #$20
  bit var_5FBC
  bvs loc_DDD0
  lda #$00
loc_DDD0:
  sta var_2006
  lda #$00
  sta var_2006
  bit var_2007
  bit var_2007
  bit var_2007

sub_DDE1:
  txa
  pha
  ldx var_5FBB
  bit var_2002
  lda var_5E03,x
  sta var_2005
  lda var_5E00,x
  beq loc_DE32
  bit var_5FBC
  bvs loc_DE42
  lda var_5E01,x
  sta sub_FF01+2
  lda var_5E02,x
  sta loc_FF0B
  lda var_5E04,x
  sta var_2005
  lda var_5FFF
  and #$B3
  sta sub_FF01
  pha
  lda var_5E00,x
  sta sub_FF01+1
  pla
  ora #$04
  sta sub_FF01
  ora var_5FFF
  and #$BF
  sta var_5FFF
  txa
  clc
  adc #$05
  sta var_5FBB
  pla
  tax
  rts
loc_DE32:
  lda var_5FFF
  and #$FB
  sta var_5FFF
  and #$F3
  sta sub_FF01
  pla
  tax
  rts
loc_DE42:
  lda var_5E04,x
  sta var_2005
  lda var_5FFF
  and #$F3
  ora #$40
  sta sub_FF01
  sta sub_FF11+1
loc_DE55:
  lda var_5E00,x
  beq loc_DE72
  sta sub_FF09+1
  lda var_5E02,x
  asl a
  asl a
  asl a
  asl a
  ora var_5E01,x
  sta bios_function_38+1
  txa
  clc
  adc #$05
  tax
  jmp loc_DE55
loc_DE72:
  stx var_5FBB
  sta bios_function_16+1
  lda var_5FFF
  ora #$44
  sta var_5FFF
  pla
  tax
  rts

bios_split_screen:
  lda var_00FF
  pha
  lda var_00FE
  pha
  stx var_00FF
  sty var_00FE
  ldy #$03
  lda (var_00FE),y
  tax
loc_DE92:
  iny
  iny
  lda (var_00FE),y
  beq loc_DEBF
  iny
  iny
  iny
  txa
  cmp (var_00FE),y
  beq loc_DE92
  ldy #$00
  lda (var_00FE),y
  sec
  sbc #$01
  sta (var_00FE),y
  bit var_5FBC
  bvc loc_DEB4
  jsr sub_DEF3
  jmp loc_DEB7
loc_DEB4:
  jsr sub_DF1F
loc_DEB7:
  lda #$00
  sta var_5FBC
  jmp loc_DEE6
loc_DEBF:
  tya
  sec
  sbc #$05
  tay
  lda (var_00FE),y
  sbc #$01
  sta (var_00FE),y
  bit var_5FBC
  bvs loc_DEDE
  ldy #$00
  jsr sub_DF1F
loc_DED4:
  ldx var_5FBB
  lda var_5E00,x
  bne loc_DED4
  bne loc_DEE1
loc_DEDE:
  jsr sub_DEF3
loc_DEE1:
  lda #$40
  sta var_5FBC
loc_DEE6:
  pla
  sta var_00FE
  pla
  sta var_00FF
  ldx #$58
  ldy #$98
  jmp sub_FF01

sub_DEF3:
  jsr sub_D093
  ldy #$00
loc_DEF8:
  ldx #$05
loc_DEFA:
  lda (var_00FE),y
  sta var_5E00,y
  iny
  dex
  bne loc_DEFA
  lda (var_00FE),y
  bne loc_DEF8
  sty var_5FBB
  rts

sub_DF0B:
  lda (var_00FE),y
  sta var_5E00,y
  beq loc_DF1E
  iny
  ldx #$03
loc_DF15:
  lda (var_00FE),y
  sta var_5E00,y
  iny
  dex
  bpl loc_DF15
loc_DF1E:
  rts

sub_DF1F:
  lda var_5FBB
  beq loc_DF68
  bit var_6000
  bpl loc_DF68
  lda var_6001
  and #$18
  beq loc_DF68
  tya
loc_DF31:
  sec
  sbc var_5E00,y
  iny
  iny
  iny
  iny
  iny
  cpy var_5FBB
  bcc loc_DF31
  cmp #$D2
  bcc loc_DF48
  ldx #$01
  jsr bios_wait_vsync_count
loc_DF48:
  ldy #$00
loc_DF4A:
  cpy var_5FBB
  beq loc_DF55
  jsr sub_DF0B
  bne loc_DF4A
  rts
loc_DF55:
  lda var_5E00,y
  bne loc_DF4A
  tya
  pha
loc_DF5C:
  jsr sub_DF0B
  bne loc_DF5C
  pla
  cmp var_5FBB
  beq loc_DF6D
  rts
loc_DF68:
  jsr sub_DF0B
  bne loc_DF68
loc_DF6D:
  sty var_5FBB
  rts

bios_function_19:
  txa
  asl a
  asl a
  asl a
  tax
  tya
  sta sub_FF01+2,x
  rts

bios_get_drive_message:
  lda var_5FA4
  ldx #$00
  ldy var_5FC0
  rts

sub_DF84:
  clc
  adc var_00FE
  sta var_00FE
  bcc loc_DF8D
  inc var_00FF
loc_DF8D:
  rts

sub_DF8E:
  lda var_00FE
  pha
  lda var_00FF
  pha
  stx var_00FF
  sty var_00FE
  lda var_2002
loc_DF9B:
  ldy #$01
  lda (var_00FE),y
  sta var_2006
  dey
  lda (var_00FE),y
  sta var_2006
  lda #$02
loc_DFAA:
  jsr sub_DF84
  ldy #$00
  lda (var_00FE),y
  cmp #$FF
  beq loc_DFE4
  cmp #$7F
  beq loc_DFDC
  tay
  bpl loc_DFCD
  and #$7F
  tax
  ldy #$01
loc_DFC1:
  lda (var_00FE),y
  sta var_2007
  iny
  dex
  bne loc_DFC1
  tya
  bne loc_DFAA
loc_DFCD:
  tax
  ldy #$01
  lda (var_00FE),y
loc_DFD2:
  sta var_2007
  dex
  bne loc_DFD2
  lda #$02
  bne loc_DFAA
loc_DFDC:
  lda #$01
  jsr sub_DF84
  jmp loc_DF9B
loc_DFE4:
  pla
  sta var_00FF
  pla
  sta var_00FE
  rts

bios_function_21:
  lda var_5FFA
  pha
  lda var_0000,x
  cmp #$FE
  bne loc_DFF8
  lda var_5FF9
loc_DFF8:
  sta var_5FFA
  sta loc_FF04
  tya
  bne loc_E00C
  ldy var_0001,x
  lda var_0002,x
  tax
  jsr sub_DF8E
  jmp loc_E032
loc_E00C:
  bit var_2002
  lda var_0004,x
  sta var_2006
  lda var_0003,x
  sta var_2006
  lda var_0005,x
  beq loc_E01F
  inc var_0006,x
loc_E01F:
  lda (var_0001,x)
  sta var_2007
  inc var_0001,x
  bne loc_E02A
  inc var_0002,x
loc_E02A:
  dec var_0005,x
  bne loc_E01F
  dec var_0006,x
  bne loc_E01F
loc_E032:
  pla
  sta var_5FFA
  sta loc_FF04
  rts

bios_set_nmi_vector:
  stx data_58B4
  sty data_58B3
  rts

bios_get_nmi_vector:
  ldx data_58B4
  ldy data_58B3
  rts

sub_E048:
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

sub_E062:
  lda var_5FF7
  asl a
  asl a
  asl a
  ldx #$20
  adc var_5FC6
  bcs loc_E073
  cmp #$F0
  bcc loc_E078
loc_E073:
  sec
  sbc #$F0
  ldx #$28
loc_E078:
  ldy #$00
  sty var_5FB2
  asl a
  rol var_5FB2
  asl a
  rol var_5FB2
  ora var_5FF6
  tay
  iny
  txa
  ora var_5FB2
  tax
  lda var_5FC4
  lsr a
  lsr a
  bcc loc_E09A
  txa
  eor #$08
  tax
loc_E09A:
  rts

bios_wait_display:
  lda var_5FC2
  cmp var_5FC3
  bne bios_wait_display
  rts

bios_display_tty:
  txa
  cmp #$07
  bne loc_E0AC
  jmp sub_E048
loc_E0AC:
  cmp #$08
  bne loc_E0C4
  ldy var_5FF6
  bne loc_E0BF
  ldx var_5FF7
  beq loc_E0C3
  dec var_5FF7
  ldy #$1F
loc_E0BF:
  dey
  sty var_5FF6
loc_E0C3:
  rts
loc_E0C4:
  cmp #$0D
  bne loc_E0CE
  ldy #$00
  sty var_5FF6
  rts
loc_E0CE:
  cmp #$0A
  beq loc_E109
  pha
  jsr sub_E062
loc_E0D6:
  lda var_5FC2
  sec
  sbc var_5FC3
  beq loc_E0E3
  cmp #$04
  bcc loc_E0D6
loc_E0E3:
  txa
  ldx var_5FC3
  sta var_5D00,x
  inx
  tya
  sta var_5D00,x
  inx
  pla
  sta var_5D00,x
  inx
  stx var_5FC3
  ldy var_5FF6
  iny
  cpy #$1F
  bcs loc_E104
  sty var_5FF6
loc_E103:
  rts
loc_E104:
  ldy #$00
  sty var_5FF6
loc_E109:
  inc var_5FF7
  lda var_5FF7
  cmp #$1E
  bcc loc_E103
  lda #$00
  sta var_5FB2
  lda var_5FC6
  asl a
  rol var_5FB2
  asl a
  rol var_5FB2
  pha
  lda var_5FB2
  ora #$20
  tax
  lda var_5FC4
  lsr a
  lsr a
  bcc loc_E135
  txa
  eor #$08
  tax
loc_E135:
  txa
  pha
  lda var_5FC6
  clc
  adc #$08
  cmp #$F0
  bcc loc_E16A
  sbc #$F0
  tay
loc_E144:
  lda var_5FC2
  sec
  sbc var_5FC3
  beq loc_E151
  cmp #$03
  bcc loc_E144
loc_E151:
  ldx var_5FC3
  lda #$80
  sta var_5D00,x
  inx
  lda var_5FC4
  eor #$02
  sta var_5FC4
  sta var_5D00,x
  inx
  stx var_5FC3
  tya
loc_E16A:
  sta var_5FC6
  tay
loc_E16E:
  lda var_5FC2
  sec
  sbc var_5FC3
  beq loc_E17B
  cmp #$03
  bcc loc_E16E
loc_E17B:
  ldx var_5FC3
  lda #$81
  sta var_5D00,x
  inx
  tya
  sta var_5D00,x
  inx
  stx var_5FC3
  pla
  tax
  pla
  tay
  iny
  lda #$1F
  sta var_5FB3
  lda #$20
  jsr sub_E1B8
  lda #$1D
  sta var_5FF7
  rts

bios_display_chars:
  txa
  pha
  sty var_5FB3
  lda #$1F
  sec
  sbc var_5FF6
  cmp var_5FB3
  bcs loc_E1B4
  sta var_5FB3
loc_E1B4:
  jsr sub_E062
  pla

sub_E1B8:
  pha
loc_E1B9:
  lda var_5FC2
  sec
  sbc var_5FC3
  beq loc_E1C6
  cmp #$05
  bcc loc_E1B9
loc_E1C6:
  txa
  ora #$40
  ldx var_5FC3
  sta var_5D00,x
  inx
  tya
  sta var_5D00,x
  inx
  pla
  sta var_5D00,x
  inx
  lda var_5FB3
  sta var_5D00,x
  inx
  stx var_5FC3
  rts

bios_set_cursor:
  stx var_5FF7
  sty var_5FF6
  rts

bios_get_cursor:
  ldx var_5FF7
  ldy var_5FF6
  lda #$1E
  rts
