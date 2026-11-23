; 模块：file-io
; 文件调用入口与文件读写：分发、打开、读、写、关闭

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0E15"
; 段 0，$8A15-$97FF
file_call_enter:
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
file_call_leave:
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

; 文件调用 1 FileOpen、20 FilePathOpen：打开文件，通道块里的文件名从偏移 5 起（主名 8 字节加扩展名 3 字节）。
; 未确认：具体行为来自手册功能名，代码尚未逐条核对。
file_call_open:
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
loc_8CBB:
  iny
  lda (var_00EE),y
  adc #$00
  sta (var_00EE),y
  dex
  bne loc_8CBB
  rts

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

; 文件调用 2 FileRead：从已打开的文件读数据，缓冲区页、地址与长度取自通道块。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_read:
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

; 文件调用 3 FileWrite：向已打开的文件写数据，缓冲区页、地址与长度取自通道块。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_write:
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

; 文件调用 5 FileClose：关闭文件通道。
; 未确认：含义来自手册功能名，代码尚未逐条核对。
file_call_close:
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

; 文件调用分发表：功能号 0-24 共 25 项，分成高字节表（本表，$900F）与低字节表（$9028），每项都是处理例程地址减 1。
; 功能号 20（FilePathOpen）与 1（FileOpen）、21（FilePathCreate）与 8（FileCreate）、23（FilePathDelete）与 10（FileDelete）共用处理例程，区别在于 file_call_flags 里的取路径标志。
file_call_table_high:
  .byte $98, $8B, $8D, $8E, $A0, $8F, $9F, $9F
  .byte $9E, $9F, $A0, $A0, $A1, $A0, $A0, $A0
  .byte $A3, $A3, $A4, $A5, $8B, $9E, $9E, $A0
  .byte $9E
file_call_table_low:
  .byte $9D
  .byte $8C, $BF, $0B, $0E, $80, $1F, $38, $BA
  .byte $DC, $A5, $38, $0B, $F3, $C9, $CD, $33
  .byte $43, $C6, $24, $8C, $BA, $7B, $A5, $51

; 文件调用的功能标志（25 项，下标为功能号）：位 7 表示先准备驱动器（检查介质），位 6 表示从通道块里的缓冲区页与地址（偏移 2、3、4）取出文件名并存入偏移 5 起的 11 字节，位 5 表示要解析路径。开头的 $00 表示三个都不需要。
file_call_flags:
  .byte $00
  .byte $C0, $00, $00, $00, $00, $C0, $00, $C0, $00, $C0, $80, $00
  .byte $40, $00, $00, $80, $A0, $A0, $80, $A0, $A0, $A0, $A0, $A0

file_call_fetch_name:
  lda file_call_flags,x
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

file_call_jump_to_handler:
  ldy #$00
  lda (var_00EE),y
  tax
  lda file_call_table_high,x
  pha
  lda file_call_table_low,x
  pha
  rts

; 文件调用分发（入口 $5803 的桩代码先把页切到 DOS 页，再调用本例程）。
; 入口：A 是桩代码取得的调用者当前页号（调用者不用设置 A），X 为通道块地址高字节，Y 为低字节；通道块第 0 字节是功能号（0-24），第 1 字节是盘号（0 取默认盘，$FF 取启动盘）。
; 出口：A 为出错码，0 表示成功；C=1 表示出错；功能号大于 24 时 A=1。
; 处理：先 file_call_enter 保存零页并取得通道块，再按 file_call_flags 准备驱动器、取文件名、解析路径，然后按 file_call_table_high 与 file_call_table_low 分发到各 file_call_ 例程，最后 file_call_leave 恢复页与零页。
file_call_dispatch:
  jsr file_call_enter
  ldy #$00
  lda (var_00EE),y
  cmp #$19
  bcc loc_90CE
  lda #$01
  jmp file_call_leave
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
  lda file_call_flags,x
  bpl loc_9115
  jsr sub_83A8
  bcs loc_913A
  ldy #$00
  lda (var_00EE),y
  tax
loc_9115:
  jsr file_call_fetch_name
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
  jsr file_call_jump_to_handler
  jmp file_call_leave
loc_9138:
  lda #$0F
loc_913A:
  jmp file_call_leave

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
  jsr file_call_fetch_name
  ldy #$03
  pla
  sta (var_00EE),y
  iny
  pla
  sta (var_00EE),y
  bcs loc_91AD
  bcc loc_91A6
loc_9163:
  jsr file_call_fetch_name
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
  jmp file_call_leave

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
