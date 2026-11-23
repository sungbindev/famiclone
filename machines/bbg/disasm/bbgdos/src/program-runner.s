; 模块：program-runner
; 程序运行：DosRunProgram、DosRunPathProgram 与程序类型判断

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_3574"
; 段 0，$B174-$B3B5
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

; DOS 功能 15 DosRunPathProgram：运行一个程序，程序名可以带路径。入口：$FF 为程序所在盘号；X:Y（X 高）为以 0 结尾的路径与文件名，文件名必须带扩展名 CMD、EXC 或 GAM；$0100 起是传给程序的参数。出口同功能 5。嵌套最多 4 层。Ember 实测。
dos_run_path_program:
  lda var_00FF
  sta var_0401
  lda data_A53D
  sta var_0402
  jmp loc_B1D3

; DOS 功能 5 DosRunProgram：运行一个程序（CMD、EXC、GAM）。入口：$FF 为程序所在盘号（0 默认，1 为 A 盘）；X:Y（X 高）指向 11 字节文件名（主名 8 加扩展名 3，空格补齐），经 sub_B13F 转成“主名.扩展名”放在 $0430 再继续；$0100 起是传给程序的参数。程序类型由扩展名判断（表在 $B136：CMD、EXC、GAM），扩展名不是这三种时返回 C=1（Ember 实测 Y=$0B）。出口：C=0 时 A 为程序的退出码。嵌套最多 4 层；返回后寄存器内容被破坏。Ember 实测。
dos_run_program:
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

sub_B25C:
  ldx #$5E
  jsr sub_A5D5
  ldx #$43
  jsr sub_A5D5
  ldx #$0D
  jsr sub_A5D5
  ldx #$0A
  jsr sub_A5D5
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

sub_B364:
  lda var_6132,y
  cmp #$0D
  beq loc_B37A
  tax
  lda var_6133,y
  cmp #$3A
  bne loc_B37A
  txa

sub_B374:
  sbc #$40
  iny
  iny
  clc
  rts
loc_B37A:
  lda #$00

sub_B37C:
  clc
  rts
  .byte $C9, $0D, $F0, $06, $C9, $20, $F0, $02, $C9, $09

sub_B388:
  rts
  .byte $A2, $00, $B9, $32, $61, $20, $7E, $B3, $F0, $07, $9D, $82, $61, $E8, $C8
  .byte $D0, $F1, $A9, $00, $9D, $82, $61, $A2, $00, $B9, $32, $61, $9D, $01, $01
  .byte $C8, $E8, $C9, $0D, $D0, $F4, $CA, $8E, $00, $01, $A2, $61, $A0, $82, $60
