; 模块：command-dispatch
; 内部命令表与命令查找分发

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_31BD"
; 段 0，$B1BD-$B47F
loc_B1BD:
  rts

; 内部命令表：每项依次为主名（0 结尾）、别名（0 结尾，没有则为空）、处理例程地址减 1（2 字节）、用法说明地址（2 字节）、标志字节，共 5 字节的尾部。命令匹配后用 RTS 跳到处理例程。
data_B1BE:
  .byte $42
data_B1BF:
  .byte $42
data_B1C0:
  .byte $47
data_B1C1:
  .byte $43
data_B1C2:
  .byte $44
data_B1C3:
  .byte $4F, $53, $00, $00, $56, $9A, $3A, $9A
  .byte $51
data_B1CC:
  .byte $43, $44, $00, $43, $48, $44, $49, $52
  .byte $00, $83, $9B, $4B, $90, $39, $43, $4C
  .byte $53, $00, $00, $EE, $93, $4B, $90, $2D
  .byte $43, $4F, $50, $59, $00, $00, $75, $AB
  .byte $0E, $A8, $22, $44, $41, $54, $45, $00
  .byte $00, $C6, $96, $4B, $90, $31, $44, $45
  .byte $4C, $00, $45, $52, $41, $53, $45, $00
  .byte $D6, $B0, $D0, $B0, $25, $44, $49, $52
  .byte $00, $00, $0C, $A5, $E0, $A0, $21, $45
  .byte $43, $48, $4F, $00, $00, $68, $A6, $4B
  .byte $90, $4B, $45, $58, $49, $54, $00, $00
  .byte $95, $90, $4B, $90, $30, $4D, $44, $00
  .byte $4D, $4B, $44, $49, $52, $00, $50, $9C
  .byte $4B, $90, $3A, $52, $45, $4D, $00, $00
  .byte $31, $91, $4B, $90, $2C, $52, $44, $00
  .byte $52, $4D, $44, $49, $52, $00, $EB, $9C
  .byte $4B, $90, $3B, $52, $45, $4E, $41, $4D
  .byte $45, $00, $52, $45, $4E, $00, $00, $93
  .byte $4B, $90, $04, $53, $43, $52, $45, $45
  .byte $4E, $00, $00, $B7, $94, $4B, $90, $4E
  .byte $54, $49, $4D, $45, $00, $00, $10, $99
  .byte $4B, $90, $35, $54, $59, $50, $45, $00
  .byte $00, $9F, $91, $4B, $90, $2E, $56, $45
  .byte $52, $00, $00, $9E, $AE, $4B, $90, $27
  .byte $56, $4F, $4C, $00, $00, $F6, $AE, $4B
  .byte $90, $29, $3F, $00, $00, $AC, $AF, $4B
  .byte $90, $2B, $00
data_B29F:
  .byte $43, $4D, $44, $45, $58, $43, $47, $41
  .byte $4D, $42, $41, $54
loc_B2AB:
  bcc loc_B2B0
  jsr sub_8A64
loc_B2B0:
  ldy #$00
  rts
loc_B2B3:
  inx
  iny

; 把命令行里从 Y 开始的字符与命令表里从 X 开始的名字逐字符比较（不区分大小写），Z=1 表示名字到结尾仍然相等。
command_name_match:
  lda data_B1BE,x
  beq loc_B2C5
  lda var_6292,y
  jsr sub_8A30
  cmp data_B1BE,x
  beq loc_B2B3
loc_B2C5:
  rts

sub_B2C6:
  bcs loc_B2CD
  tya
  beq loc_B2DC
  lda #$1D
loc_B2CD:
  pha
  lda #$0B
  ldx #$00
  jsr DOS_ENTRY
  sta var_63DF
  pla
  jsr sub_8A64
loc_B2DC:
  rts

; 在命令行里查找并执行命令：先按内部命令表匹配（命令后必须是空格、点、反斜线或结束），匹配到就把处理例程地址压栈后 RTS；匹配不到就按外部命令查找 CMD、EXC、GAM、BAT 文件并运行。
; 入口与出口：未确认。首先用 DOS 功能 11、3 取显示方式与空闲页。
command_dispatch:
  lda #$0B
  ldx #$00
  jsr DOS_ENTRY
  sta var_63DF
  lda #$03
  ldx #$00
  jsr DOS_ENTRY
  sta var_63DC
  ldy #$00
  sty var_63DD
  jsr sub_8B2F
  beq loc_B321
  tya
  pha
  jsr sub_8A67
  pla
  tay
  jsr sub_8E22
  stx var_0401
  tax
  beq loc_B325
  ldx var_6292,y
  cpx #$0D
  beq loc_B315
  jmp loc_B3AC
loc_B315:
  lda var_0401
  lda #$09
  jsr sub_8D73
  lda #$0F
  bcs loc_B322
loc_B321:
  clc
loc_B322:
  jmp loc_B2AB
loc_B325:
  ldx #$00
loc_B327:
  tya
  pha
  jsr command_name_match
  php
loc_B32D:
  inx
  lda data_B1BE,x
  bne loc_B32D
  plp
  beq loc_B344
  inx
  lda data_B1BE,x
  beq loc_B397
  pla
  pha
  tay
  jsr command_name_match
  bne loc_B397
loc_B344:
  jsr sub_8B57
  beq loc_B351
  cmp #$2E
  beq loc_B351
  cmp #$5C
  bne loc_B397
loc_B351:
  pla
  cpx #$08
  bne loc_B36D
  sta var_00FE
  sty var_00FF
  lda #$FF
  jsr BIOS_ENTRY
  ldy var_00FE
  cmp #$40
  bne loc_B3AC
  cpx #$62
  bcc loc_B3AC
  ldx #$08
  ldy var_00FF
loc_B36D:
  lda data_B1C1,x
  sta var_63E1
  lda data_B1C2,x
  sta var_63E2
  lda data_B1C3,x
  sta var_63DB
  lda #$B2
  pha
  lda #$AA
  pha
  lda data_B1C0,x
  pha
  lda data_B1BF,x
  pha
  lda #$00
  ldx #$1B
loc_B391:
  sta var_00E0,x
  dex
  bpl loc_B391
  rts
loc_B397:
  pla
  tay
loc_B399:
  inx
  lda loc_B1BD,x
  bne loc_B399
  txa
  clc
  adc #$05
  tax
  lda data_B1BE,x
  beq loc_B3AC
  jmp loc_B327
loc_B3AC:
  lda #$40
  ldx var_0401
  jsr sub_8BEA
  bcs loc_B40F
  sty var_62E2
  ldx #$03
  lda #$00
loc_B3BD:
  sta var_63E7,x
  dex
  bpl loc_B3BD
  jsr sub_8B79
  bcs loc_B40F
  lda #$03
  jsr sub_8D64
  bcs loc_B40F
loc_B3CF:
  jsr sub_8BD6
  ldx #$00
  stx var_00FD
loc_B3D6:
  ldy #$11
loc_B3D8:
  lda (var_00FE),y
  cmp data_B29F,x
  bne loc_B3F0
  inx
  iny
  cpy #$14
  bne loc_B3D8
  lda #$FF
  ldx var_00FD
  sta var_63E7,x
  bne loc_B3FC
  beq loc_B403
loc_B3F0:
  inx
  iny
  cpy #$14
  bne loc_B3F0
  inc var_00FD
  cpx #$0C
  bne loc_B3D6
loc_B3FC:
  lda #$07
  jsr sub_8D73
  bcc loc_B3CF
loc_B403:
  ldy #$00
loc_B405:
  lda var_63E7,y
  bne loc_B415
  iny
  cpy #$04
  bne loc_B405
loc_B40F:
  lda #$0B
  sec
  jmp loc_B2AB
loc_B415:
  tya
  sta var_00FD
  asl a
  adc var_00FD
  tay
  jsr sub_8BE1
loc_B41F:
  dex
  lda var_62E7,x
  cmp #$2E
  bne loc_B41F
  lda #$03
  sta var_00FD
loc_B42B:
  lda data_B29F,y
  sta var_62E8,x
  inx
  iny
  dec var_00FD
  bne loc_B42B
  lda #$00
  sta var_62E8,x
  cpy #$0C
  bne loc_B443
  jmp loc_8F72
loc_B443:
  ldy var_62E2
  ldx #$00
loc_B448:
  lda var_6292,y
  sta var_0101,x
  iny
  inx
  cmp #$0D
  bne loc_B448
  dex
  stx var_0100
  tsx
  ldy #$00
loc_B45B:
  lda var_0101,x
  sta var_601A,y
  inx
  iny
  cpy #$04
  bne loc_B45B
  jsr sub_889D
  lda #$22
  jsr BIOS_ENTRY
  lda var_0401
  sta var_00FF
  lda #$0F
  ldx #$62
  ldy #$E7
  jmp $604E
  .byte $20, $75, $88
