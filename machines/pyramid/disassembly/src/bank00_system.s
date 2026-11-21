; 模块：bank00_system
; 块 0：键盘、光标与显示等系统例程，系统跳转表的目标

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，bank 0，$8000-$BFFF
sub_b0_8000:
  .byte $C9, $8D, $D0, $14, $48, $20, $12, $C0, $C9, $00, $F0, $0B, $C9, $93, $D0, $07
  .byte $20, $12, $C0, $C9, $00
  .byte $F0
  .byte $F9

sub_b0_8017:
  .byte $68

sub_b0_8018:
  .byte $4C, $FD, $FB

sub_b0_801B:
  .byte $C9, $80, $F0, $07, $A4, $24, $91, $28, $4C, $42, $80

sub_b0_8026:
  .byte $A4, $24, $C8, $C4, $21, $90, $03, $20, $CC, $80

sub_b0_8030:
  .byte $A5, $25, $20, $C1, $FB, $A4, $24, $AD, $FB, $03, $91, $28, $C8, $20, $28, $FB
  .byte $91, $28

sub_b0_8042:
  .byte $20, $40, $F8, $A5, $25, $20, $C1, $FB, $A4, $24, $B1, $28, $30, $02, $E6, $24

sub_b0_8052:
  .byte $E6, $24, $A5, $24, $4C, $06, $97, $72
loc_b0_805A:
  rts

sub_b0_805B:
  cmp #$80
  beq sub_b0_801B
  cmp #$A0
  bcs sub_b0_801B
  cmp #$8D
  beq sub_b0_80CD
  cmp #$8A
  beq sub_b0_80D0
  cmp #$88
  beq loc_b0_8072
  jmp sub_b31_FBD9
loc_b0_8072:
  dec var_0024
  lda var_0025
  jsr sub_b31_FBC1
  ldy var_0024
  lda (var_0028),y
  bmi loc_b0_8081
  dec var_0024
loc_b0_8081:
  lda var_0024
  bpl loc_b0_805A
  lda var_0021
  sta var_0024
  dec var_0024
  lda var_0022
  cmp var_0025
  bcs loc_b0_809C
  dec var_0025
  lda var_0025

sub_b0_8095:
  jsr sub_b31_FBC1
  adc var_0020
  sta var_0028
loc_b0_809C:
  rts
  .byte $49, $C0, $F0, $28, $69, $FD, $90, $A0, $F0, $CB, $69, $FD, $90, $25, $D0, $DE
  .byte $69, $FD, $90, $06, $D0, $E9, $4C, $42, $FC, $EA, $4C, $9C, $FC, $EA, $20, $E0
  .byte $80, $A0, $00, $68, $69, $00, $C5, $23, $90, $F0, $B0, $CA

sub_b0_80C9:
  jmp sub_b0_80E0
  tya

sub_b0_80CD:
  jmp sub_b0_9709

sub_b0_80D0:
  inc var_0025
  lda var_0025
  cmp var_0023
  bcc sub_b0_8095
  dec var_0025
  jsr sub_b31_C006
  jmp sub_b0_80F0

sub_b0_80E0:
  jmp sub_b0_9703
  .byte $A0, $99, $00, $06, $99, $00, $07, $C8, $D0, $F7, $4C, $0C, $C0

sub_b0_80F0:
  lda var_0022
  pha
  jmp sub_b31_FC73
  .byte $EA, $EA, $4C, $BF, $82, $4C, $B1, $82

sub_b0_80FE:
  cmp #$FF
  bne sub_b0_8108
  lda #$01
  sta var_03C1
  rts

sub_b0_8108:
  tay
  lda var_03C1
  php
  tya
  ldy var_0035
  plp
  bmi sub_b0_8128
  beq loc_b0_811E
  and #$7F
  sta var_03FB
  dec var_03C1
  rts
loc_b0_811E:
  and #$7F
  sta var_03FC
  lda #$80
  sta var_03C1

sub_b0_8128:
  jmp sub_b31_FDF5
  .byte $A9, $80, $8D, $C1, $03, $EA

sub_b0_8131:
  txa
  pha
  tya
  pha
  lda #$03

sub_b0_8137:
  clc
  adc #$01
  pha
  and #$03
  bne sub_b0_8142
  jsr sub_b31_C00F

sub_b0_8142:
  jsr sub_b31_C000
  cmp #$00
  bne sub_b0_814D
  pla
  jmp sub_b0_8137

sub_b0_814D:
  sta var_0000
  pla
  lsr a
  lsr a
  lsr a
  bcc sub_b0_8158
  jsr sub_b31_C00F

sub_b0_8158:
  pla
  tay
  pla
  tax
  lda var_0000
  jsr sub_b0_96E0
  jmp sub_b0_86D6
  .byte $60, $6C, $38, $00

sub_b0_8168:
  sta var_2005
  sta var_2005
  jmp sub_b31_FA68
  .byte $EA, $A5, $32, $F0, $04, $A9, $80, $D0, $03, $BD, $00, $02, $20, $ED, $FD, $C9
  .byte $88, $F0, $6E, $C9, $FF, $F0, $6A, $C9, $98, $F0, $0A, $E0, $F0, $90, $03, $20
  .byte $3A, $FF, $E8, $D0, $13, $A9, $DC, $20, $ED, $FD

sub_b0_819B:
  jsr monitor_crout

sub_b0_819E:
  lda var_0033
  jsr monitor_cout
  jmp sub_b0_9700
  .byte $F0, $F3, $CA, $A9, $00, $85, $32, $4C, $CA, $85, $C9, $9C, $D0, $12, $B1, $28
  .byte $30, $0E, $8D, $FB, $03, $C8, $B1, $28, $8D, $FC, $03, $88, $A9, $80, $E6, $32
  .byte $C9, $80, $D0, $17, $E6, $32, $A9, $7F, $9D, $00, $02, $E8, $AD, $FB, $03, $9D
  .byte $00, $02, $E8, $AD, $FC, $03, $9D, $00, $02, $D0, $91, $09, $80, $9D, $00, $02
  .byte $C9, $8D, $D0, $88, $20, $9C, $FC, $A9, $8D, $4C, $ED, $FD, $8A, $F0, $A6, $A5
  .byte $32, $F0, $02, $CA, $CA, $4C, $A8, $81, $00, $00, $20, $12, $C0, $C9, $93, $D0
  .byte $03, $20, $0C, $FD, $C9, $83, $F0, $06, $20, $D0, $F8, $4C, $17, $82, $4C, $6D
  .byte $82, $68, $85, $2C, $68, $85, $2D, $A2, $08, $BD, $D7, $82, $95, $3C, $CA, $D0
  .byte $F8, $A1, $3A, $F0, $42, $A4, $2F, $C9, $20, $F0, $59, $C9, $60, $F0, $45, $C9
  .byte $4C, $F0, $5C, $C9, $6C, $F0, $59, $C9, $40, $F0, $35, $29, $1F, $49, $14, $C9
  .byte $04, $F0, $02, $B1, $3A, $99, $3C, $00, $88, $10, $F8, $20, $3F, $FF, $4C, $3C
  .byte $00, $85, $45, $68, $48, $0A, $0A, $0A, $30, $03, $4C, $65, $FF, $28, $20, $4C
  .byte $FF, $68, $85, $3A, $4C, $C7, $82, $20, $82, $F8, $20, $DA, $FA, $4C, $65, $FF
  .byte $18, $68, $85, $48, $68, $85, $3A, $68, $85, $3B, $A5, $2F, $20, $56, $F9, $84
  .byte $3B, $18, $90, $14, $18, $20, $54, $F9, $AA, $98, $48, $8A, $48, $A0, $02, $18
  .byte $B1, $3A, $AA, $88, $B1, $3A, $86, $3B, $85, $3A, $B0, $F3, $A5, $2D, $48, $A5
  .byte $2C, $48, $4C, $D7, $FA, $18, $A0, $01, $B1, $3A, $20, $56, $F9, $85, $3A, $98
  .byte $38, $B0, $C5, $20, $4A, $FF, $38, $B0, $C1, $EA, $EA, $4C, $B9, $82, $4C, $AB
  .byte $82, $68, $85, $3B, $38, $A5, $3A, $E9, $02, $85, $3A, $B0, $02, $C6, $3B, $4C
  .byte $6D, $82, $EA, $EA, $4C, $B9, $82, $4C, $AB, $82

sub_b0_82E0:
  pha
  lda #$02
  sta var_4015
  lda #$07
  sta var_4004
  lda #$00
  sta var_4005
  lda #$D6
  sta var_4006
  lda #$08
  sta var_4007
  pla
  rts
  .byte $EA, $EA, $EA, $EA, $A0, $4B, $20, $12, $83, $D0, $F9, $69, $FE, $B0, $F5, $A0
  .byte $21, $20, $12, $83, $C8, $C8, $88, $D0, $FD, $90, $05, $A0, $32, $88, $D0, $FD
  .byte $48, $A5, $2F, $49, $01, $85, $2F, $8D, $00, $51, $68, $A0, $2C, $CA, $60, $A2
  .byte $08, $48, $20, $39, $83, $68, $2A, $A0, $3A, $CA, $D0, $F5, $60, $20, $3C, $83
  .byte $88, $AD, $00, $55, $29, $04, $45, $2F, $85, $2F, $C0, $80, $60, $A9, $10, $8D
  .byte $00, $20, $A9, $60, $20, $00, $83, $A0, $27, $A2, $00, $41, $3C, $48, $A1, $3C
  .byte $20, $6E, $83, $20, $BA, $FC, $A0, $1D, $68, $90, $EE, $A0, $22, $20, $6E, $83
  .byte $F0, $43, $A2, $10, $0A, $20, $0D, $83, $D0, $FA, $A9, $90, $8D, $00, $20, $60
  .byte $A9, $10, $8D, $00, $20, $20, $39, $83, $A9, $16, $20, $00, $83, $85, $2E, $20
  .byte $39, $83, $A0, $24, $20, $3C, $83, $B0, $F9, $20, $3C, $83, $A0, $3B, $20, $2B
  .byte $83, $81, $3C, $45, $2E, $85, $2E, $20, $BA, $FC, $A0, $35, $90, $F0, $20, $2B
  .byte $83, $C5, $2E, $D0, $08, $A9, $90, $8D, $00, $20, $4C, $3A, $FF, $A9, $90, $8D
  .byte $00, $20, $4C, $2D, $FF
  .res 53, $EA
  .byte $4C, $35, $FB, $C9, $22, $D0, $02, $A9, $20, $60, $D8, $A2, $01, $B5, $3C, $95
  .byte $3A, $B5, $36, $95, $CE, $F0, $03, $CA, $F0, $F3, $A6, $3A, $A4, $3B, $20, $40
  .byte $F9, $A9, $BA, $20, $F0, $FD, $A2, $03, $86, $FA, $CA, $86, $FB, $CA, $86, $06
  .byte $CA, $86, $07, $20, $6F, $FD, $A0, $00, $AD, $00, $02, $C9, $8D, $D0, $03, $4C
  .byte $69, $FF, $B1, $FA, $C9, $8D, $D0, $03, $4C, $DA, $84, $C9, $A0, $D0, $05, $E6
  .byte $FA, $4C, $38, $84, $C0, $07, $10, $6F, $C9, $B0, $10, $07, $99, $19, $00, $C8
  .byte $4C, $38, $84, $C9, $C7, $10, $F5, $A6, $07, $D0, $5C, $C0, $04, $10, $58, $C0
  .byte $00, $F0, $06, $B6, $18, $E0, $A4, $F0, $0B, $A2, $A4, $96, $19, $C8, $C0, $04
  .byte $10, $45, $C6, $FA, $C8, $C8, $AA, $B1, $FA, $88, $C9, $B0, $30, $10, $C9, $C7
  .byte $10, $0C, $85, $FC, $20, $9D, $84, $85, $09, $C8, $C8, $A5, $FC, $AA, $20, $9D
  .byte $84, $C8, $86, $07, $4C, $38, $84, $8A, $96, $18, $20, $C6, $84, $0A, $0A, $0A
  .byte $0A, $85, $08, $B1, $FA, $99, $19, $00, $20, $C6, $84, $18, $65, $08, $85, $08
  .byte $E6, $06, $60, $68, $68, $68, $68, $20, $2D, $FF, $20, $62, $FC, $4C, $10, $84
  .byte $C9, $B0, $30, $EF, $C9, $BA, $10, $04, $38, $E9, $B0, $60, $C9, $C1, $30, $E3
  .byte $38, $E9, $B7, $60, $84, $07, $A2, $00, $A9, $10, $81, $3A, $20, $61, $85, $E0
  .byte $03, $D0, $39, $A5, $06, $C9, $03, $D0, $CE, $C6, $06, $A6, $3A, $E8, $E8, $86
  .byte $09, $A5, $08, $38, $E5, $09, $A0, $01, $91, $3A, $20, $8A, $85, $6A, $90, $B7
  .byte $20, $1A, $FC, $20, $22, $FC, $20, $D0, $F8, $20, $62, $FC, $A5, $3A, $18, $65
  .byte $06, $85, $3A, $A5, $3B, $69, $00, $85, $3B, $4C, $10, $84, $A2, $00, $A1, $3A
  .byte $18, $69, $20, $81, $3A, $90, $B5, $A4, $06, $C0, $01, $F0, $09, $88, $B9, $07
  .byte $00, $91, $3A, $4C, $2F, $85, $A9, $00, $AA, $81, $3A, $20, $61, $85, $E0, $03
  .byte $D0, $08, $20, $8A, $85, $6A, $90, $02, $B0, $B6, $A2, $00, $A9, $01, $18, $61
  .byte $3A, $81, $3A, $F0, $03, $4C, $41, $85, $4C, $BD, $84, $20, $8C, $F8, $A2, $00
  .byte $A8, $B9, $C0, $F9, $85, $2C, $B9, $00, $FA, $85, $2D, $A9, $00, $A0, $05, $06
  .byte $2D, $26, $2C, $2A, $88, $D0, $F8, $69, $BF, $DD, $00, $02, $D0, $05, $E8, $E0
  .byte $03, $D0, $E8, $60, $A9, $AD, $85, $36, $A9, $85, $85, $37, $A9, $00, $85, $FD
  .byte $BA, $86, $FC, $20, $0C, $F9, $A6, $07, $E4, $FD, $D0, $1C, $A9, $01, $A6, $CE
  .byte $86, $36, $A6, $CF, $86, $37, $60, $86, $09, $A6, $FD, $D5, $19, $D0, $06, $E8
  .byte $86, $FD, $A6, $09, $60, $A6, $FC, $9A, $A9, $00, $F0, $E2, $60, $A6, $FC, $9A
  .byte $A9, $00, $F0, $E2, $20, $0C, $FD, $A4, $24, $C9, $9E, $D0, $03, $4C, $5B, $FC
  .byte $4C, $69, $FC, $A6, $FD, $D5, $19, $D0, $06, $E8, $86, $FD, $A6, $09, $60, $A6
  .byte $FC, $9A, $A9, $00, $F0, $E2, $84, $A4, $24, $A2, $03, $B5, $3E, $95, $06, $CA
  .byte $10, $F9, $A9, $BA, $85, $33, $20, $67, $FD, $A0, $00, $8C, $9F, $02, $B9, $00
  .byte $02, $C9, $8D, $F0, $17, $C8, $C9, $A0, $F0, $F4, $88, $20, $A7, $FF, $A5, $3C
  .byte $AE, $9F, $02, $9D, $A0, $02, $88, $EE, $9F, $02, $D0, $E2, $A0, $00, $B1, $08
  .byte $D9, $A0, $02, $D0, $12, $C8, $CC, $9F, $02, $D0, $F3, $A5, $09, $A6, $08, $20
  .byte $41, $F9, $A9, $A0, $20, $ED, $FD, $A5, $08, $C5, $06, $A5, $09, $E5, $07, $E6
  .byte $08, $D0, $02, $E6, $09, $90, $D5, $4C, $DD, $86, $A0, $00, $B9, $6D, $86, $F0
  .byte $15, $30, $0C, $8D, $FB, $03, $C8, $B9, $6D, $86, $8D, $FC, $03, $A9, $80, $C8
  .byte $20, $ED, $FD, $4C, $52, $86, $60, $8D, $D0, $C5, $C3, $AD, $B9, $B4, $B8, $B8
  .byte $A0, $CD, $CF, $CE, $C9, $D4, $CF, $D2, $A0, $D6, $B2, $AE, $B1, $B6, $87, $8D
  .byte $A8, $C3, $A9, $C3, $EF, $F0, $F9, $F2, $E9, $E7, $E8, $F4, $A0, $CC, $E9
data_b0_8695:
  .byte $F5
data_b0_8696:
  .byte $CD
  .byte $E9
data_b0_8698:
  .byte $EE, $A0, $B1, $B9, $B9, $B3, $AE, $B5, $87, $8D, $8D, $D0, $F2, $EF, $E7, $F2
  .byte $E1, $ED, $ED, $E5, $F2, $BA, $A0, $CC, $E9, $F5, $A0, $CD, $E9, $EE, $87, $8D
  .byte $3C, $60, $3F, $58, $33, $4C, $50, $72, $49, $68, $3C, $46, $BA, $A0, $41, $75
  .byte $A0, $43, $71, $A0, $A8, $C5, $C3, $CE, $D5, $A9, $87, $8D, $00, $EA

sub_b0_86D6:
  rts
  .byte $9D, $D0, $02, $A9, $88, $60, $A2, $00, $4C, $8F, $FE

sub_b0_86E2:
  jmp sub_b0_8C5E

sub_b0_86E5:
  jmp sub_b0_8A41

sub_b0_86E8:
  jmp sub_b0_953B

sub_b0_86EB:
  jmp sub_b0_8728

sub_b0_86EE:
  jmp cursor_cell_read_inverse

sub_b0_86F1:
  jmp key_read_with_repeat

sub_b0_86F4:
  jmp keyboard_scan

sub_b0_86F7:
  jmp sub_b0_877F

; 等待一个键：反复调 key_read_with_repeat 直到返回非 0，每 4 次循环调用一次 cursor_cell_read_inverse（依据：$00 计数与 3 相与），返回 A=键码。保存 X、Y。
wait_key:
  lda #$00
  sta var_0000
  txa
  pha
  tya
  pha

sub_b0_8702:
  inc var_0000
  lda var_0000
  and #$03
  bne sub_b0_870D
  jsr cursor_cell_read_inverse

sub_b0_870D:
  jsr key_read_with_repeat
  cmp #$00
  beq sub_b0_8702
  lsr var_0000
  lsr var_0000
  lsr var_0000
  sta var_0000
  bcc sub_b0_8721
  jsr cursor_cell_read_inverse

sub_b0_8721:
  pla
  tay
  pla
  tax
  lda var_0000
  rts

sub_b0_8728:
  lda #$00
  sta vram_job_count
  lda var_03C6
  sta var_03C5
  txa
  pha
  ldy var_0022
  ldx var_0020
  jsr sub_b31_C01E
  lda var_0023
  sec
  sbc var_0022
  asl a
  sta var_0535
  jsr wait_next_vblank
  jsr ppu_disable_rendering

sub_b0_874B:
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006
  lda var_0021
  sec
  sbc var_0020
  asl a
  asl a
  asl a
  tay
  lda #$00

sub_b0_8760:
  sta var_2007
  dey
  bne sub_b0_8760
  inc var_0027
  lda var_0027
  cmp #$1E
  bcc sub_b0_8772
  lda #$00
  sta var_0027

sub_b0_8772:
  dec var_0535
  bne sub_b0_874B
  pla
  tax
  lda #$00
  sta var_0025
  sta var_0024

sub_b0_877F:
  lda #$90
  sta var_2000
  lda #$08
  sta var_2001
  lda #$00
  sta var_2005
  lda var_00E4
  sta var_2005
  rts

; 读一个键，带去抖与自动重复：调 keyboard_scan，键码与上一次（$053F）不同则立即算新按下（重复计数 $0540 设为 $10）；相同则等计数减到 0 后才算重复；键码 $E6、$EA 分别切换 $EA 里的位 7、位 6（未确认：可能是中英文、大小写状态）。返回 A=键码（高位按状态可能取反）。
key_read_with_repeat:
  stx var_0530
  sty var_0531

sub_b0_879A:
  jsr keyboard_scan
  sta var_053E
  cmp var_053F
  bne sub_b0_87B9

sub_b0_87A5:
  jsr wait_next_vblank
  jsr keyboard_scan
  cmp var_053E
  bne sub_b0_879A
  dec var_0540
  bne sub_b0_87A5
  lda #$02
  bne sub_b0_87BB

sub_b0_87B9:
  lda #$10

sub_b0_87BB:
  sta var_0540
  lda var_00EA
  ldx var_053E
  cpx #$E6
  bne sub_b0_87CC
  eor #$80
  jmp loc_b0_87D2

sub_b0_87CC:
  cpx #$EA
  bne sub_b0_87E3
  eor #$40
loc_b0_87D2:
  sta var_00EA
  lda #$18
  sta var_0540
  lda var_053E
  sta var_053F
  lda #$00
  beq sub_b0_87ED

sub_b0_87E3:
  lda var_053E
  sta var_053F
  beq sub_b0_87ED
  eor #$80

sub_b0_87ED:
  ldx var_0530
  ldy var_0531
  rts

; （未确认用途）把光标所在字格的图案从显存读到 $0400 起的缓冲并取反，读前把 $5000 写成 0（关掉汉字模式，按普通字模读显存），可能是光标闪烁的反显。入口 $24、$25 是光标列行。
cursor_cell_read_inverse:
  ldy var_0025
  ldx var_0024
  jsr sub_b31_C01E

sub_b0_87FB:
  lda vram_job_count
  bne sub_b0_87FB
  jsr wait_next_vblank
  lda #$00
  sta bank_shadow
  sta REG_BANK
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006
  ldy #$00
  lda var_2007

sub_b0_881A:
  lda var_2007
  eor #$FF
  sta var_0400,y
  iny
  cpy #$08
  bcc sub_b0_881A
  lda var_0027
  adc #$00
  sta var_2006
  lda var_0026
  sta var_2006
  lda var_2007

sub_b0_8836:
  lda var_2007
  eor #$FF
  sta var_0400,y
  iny
  cpy #$10
  bcc sub_b0_8836
  ldy #$00
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006

sub_b0_884F:
  lda var_0400,y
  sta var_2007
  iny
  cpy #$08
  bcc sub_b0_884F
  lda var_0027
  adc #$00
  sta var_2006
  lda var_0026
  sta var_2006

sub_b0_8866:
  lda var_0400,y
  sta var_2007
  iny
  cpy #$10
  bcc sub_b0_8866
  lda #$80
  sta bank_shadow
  sta REG_BANK
  jmp sub_b0_877F
data_b0_887C:
  .byte $1B, $20
  .byte $E7, $E6, $EF, $60, $09, $EE, $F6, $F7
  .byte $F5, $F4
  .byte $F8
  .byte $F2, $F1, $F3
  .byte $2B, $80, $8A
  .byte $41, $0D, $31
  .byte $51, $5A, $2A, $83, $86
  .byte $53
  .byte $89
  .byte $32
  .byte $57
  .byte $58
  .byte $2F, $82, $85, $44, $88
  .byte $33, $45, $43
  .byte $EA, $81, $84
  .byte $46
  .byte $87
  .byte $34, $52
  .byte $56
data_b0_88AC:
  .byte $08, $5C, $0D, $47, $5D
  .byte $35, $54, $42
  .byte $39
  .byte $2E, $4C
  .byte $4B
  .byte $4F, $38, $49
  .byte $2C
  .byte $30
  .byte $2F
  .byte $3B
  .byte $4A
  .byte $50, $37, $55
  .byte $4D, $2D
  .byte $3D
  .byte $27, $48, $5B
  .byte $36
  .byte $59
  .byte $4E
  .byte $FB, $FC, $FA
  .byte $00
  .byte $2D, $F9, $00, $00
data_b0_88D4:
  .byte $1B, $20, $E7, $E6, $EF, $7E, $09, $EE, $F6, $F7, $F5, $F4, $F8, $F2, $F1, $F3
  .byte $2B, $ED, $7F, $61, $0D, $21, $71, $7A, $2A, $EC, $1C, $73, $EB, $40, $77, $78
  .byte $2F, $1F, $35, $64, $1E, $23, $65, $63, $EA, $E9, $1D, $66, $E8, $24, $72, $76
  .byte $08, $7C, $0D, $67, $7D, $25, $74, $62, $28, $3E, $6C, $6B, $6F, $2A, $69, $3C
  .byte $29, $3F
  .byte $3A
  .byte $6A, $70, $26, $75, $6D, $5F
  .byte $2B
  .byte $22, $68, $7B, $5E, $79, $6E, $FB, $FC, $FA, $00, $2D, $F9, $00, $00, $30, $31
  .byte $32, $33, $34, $35, $36, $37, $38, $39, $2E

; 扫描键盘，返回 A=键码（0 表示没有键）。协议：13 行。每行先写 $4016=$01、$02，再连读 8 次 $4017，每次取位 1 拼成该行 8 列的状态；读完写 $4016=$07 换到下一行。键码 = 行×8 + 列；第 0 行的列 2、4、7（键码 2、4、7）是修饰类键，存到 $0536，不作为普通键返回。最近按下的键缓存在 $054A-$054E。
keyboard_scan:
  lda #$00
  tax
  sta var_0536
  lda #$53
  sta var_0537
  lda #$01
  sta var_4016

sub_b0_8947:
  lda #$02
  sta var_4016
  ldy #$08

sub_b0_894E:
  pha
  lda var_4017
  lsr a
  lsr a
  pla
  rol a
  dey
  bne sub_b0_894E
  ora #$00
  beq sub_b0_89A7

sub_b0_895D:
  lsr a
  bcc sub_b0_89A2
  sty var_0532
  pha
  txa
  asl a
  asl a
  asl a
  ora var_0532
  cmp #$04
  beq sub_b0_899E
  cmp #$07
  beq sub_b0_899E
  cmp #$02
  beq sub_b0_899E
  sta var_0537
  txa
  pha
  ldx #$04
loc_b0_897E:
  lda var_054A,x
  cmp #$7F
  beq sub_b0_898F
  cmp var_0537
  beq sub_b0_8995
  dex
  bne loc_b0_897E
  beq sub_b0_8995

sub_b0_898F:
  lda var_0537
  sta var_054A

sub_b0_8995:
  ora #$80
  sta var_054A,x
  pla
  tax
  bne sub_b0_89A1

sub_b0_899E:
  sta var_0536

sub_b0_89A1:
  pla

sub_b0_89A2:
  iny
  cpy #$08
  bcc sub_b0_895D

sub_b0_89A7:
  lda #$07
  sta var_4016
  inx
  cpx #$0D
  bcc sub_b0_8947
  ldx #$04
  lda #$53
  cmp var_0537
  bne sub_b0_89C4
  lda #$7F

sub_b0_89BC:
  sta var_054A,x
  dex
  bne sub_b0_89BC
  beq sub_b0_89F2

sub_b0_89C4:
  ldy #$04

sub_b0_89C6:
  lda var_054A,x
  bpl sub_b0_89D1
  and #$7F
  sta var_054A,y
  dey

sub_b0_89D1:
  dex
  bne sub_b0_89C6
  iny
  lda var_054A,y
  cmp var_054A
  bne loc_b0_89E3
  sta var_0537
  jmp sub_b0_89E8
loc_b0_89E3:
  lda #$53
  sta var_0537

sub_b0_89E8:
  lda #$7F

sub_b0_89EA:
  dey
  beq sub_b0_89F2
  sta var_054A,y
  bne sub_b0_89EA

sub_b0_89F2:
  ldy var_0536
  ldx var_0537
  jsr sub_b0_963E
  beq sub_b0_8A40
  cmp #$41
  bcc sub_b0_8A1F
  cmp #$5B
  bcs sub_b0_8A1F
  cpy #$04
  bne sub_b0_8A0D
  sec
  sbc #$40
  rts

sub_b0_8A0D:
  cpy #$07
  beq loc_b0_8A1A
  bit var_00EA
  bpl loc_b0_8A16
  rts
loc_b0_8A16:
  clc
  adc #$20
  rts
loc_b0_8A1A:
  bit var_00EA
  bmi loc_b0_8A16
  rts

sub_b0_8A1F:
  cmp #$80
  bcc sub_b0_8A3C
  cmp #$8B
  bcs sub_b0_8A3C
  bit var_00EA
  cpy #$07
  beq loc_b0_8A38
  bvc loc_b0_8A33

sub_b0_8A2F:
  lda data_b0_88D4,x
  rts
loc_b0_8A33:
  tax
  lda data_b0_88AC,x
  rts
loc_b0_8A38:
  bvc sub_b0_8A2F
  bvs loc_b0_8A33

sub_b0_8A3C:
  cpy #$07
  beq sub_b0_8A2F

sub_b0_8A40:
  rts

sub_b0_8A41:
  pha
  txa
  pha
  tya
  pha
loc_b0_8A46:
  lda vram_job_count
  bne loc_b0_8A46
  ldy #$0E
  ldx #$00
  jsr sub_b31_C01E
  jsr wait_next_vblank
  jsr ppu_disable_rendering
  lda #$00
  sta bank_shadow
  sta REG_BANK
  ldx #$02

sub_b0_8A62:
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006
  ldy #$10
  lda var_2007

sub_b0_8A71:
  lda var_2007
  sta var_03F0,y
  iny
  bne sub_b0_8A71
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006
  lda #$00
  ldy #$F0

sub_b0_8A88:
  sta var_2007
  dey
  bne sub_b0_8A88
  lda var_0027
  clc
  adc #$02
  cmp #$1E
  bcc sub_b0_8A99
  sbc #$1E

sub_b0_8A99:
  sta var_2006
  lda var_0026
  sta var_2006
  ldy #$10

sub_b0_8AA3:
  lda var_03F0,y
  sta var_2007
  iny
  bne sub_b0_8AA3
  inc var_0027
  dex
  bne sub_b0_8A62
  lda var_00E4
  clc
  adc #$10
  cmp #$F0
  bcc sub_b0_8ABC
  sbc #$F0

sub_b0_8ABC:
  sta var_00E4
  jsr wait_next_vblank
  lda #$80
  sta bank_shadow
  sta REG_BANK
  jsr sub_b0_877F
  pla
  tay
  pla
  tax
  pla
  rts

sub_b0_8AD2:
  sta var_052E
  lda var_0025
  pha
  lda var_0024
  pha
  lda #$0E
  sta var_0025
  lda var_0538
  sta var_0024
  lda var_052E
  bpl sub_b0_8AF1
  jsr sub_b31_C003
  inc var_0538
  bne sub_b0_8B16

sub_b0_8AF1:
  lda var_053D
  bpl sub_b0_8AFF
  lda var_052E
  sta var_053D
  jmp sub_b0_8B16

sub_b0_8AFF:
  sta var_03FB
  lda var_052E
  jsr sub_b31_C003
  lda #$FF
  sta var_053D
  lda var_0538
  clc
  adc #$02
  sta var_0538

sub_b0_8B16:
  pla
  sta var_0024
  pla
  sta var_0025
  lda var_052E
  rts
data_b0_8B20:
  .byte $48, $21, $4F, $7B, $32, $69, $55, $52, $31, $60, $42, $6B, $39, $26, $44, $5C
  .byte $AC, $30, $34, $48, $4E, $52, $3B, $3C, $7C, $3C, $4C, $50, $78
data_b0_8B3D:
  .byte $49, $68, $56, $43, $32, $69, $55, $52
  .byte $00, $00, $00, $00
  .byte $42, $6B, $AC, $30, $34, $48, $4E, $52
  .byte $3B, $3C, $7C, $3C, $4C, $50, $78
data_b0_8B58:
  .byte $49, $68, $56, $43, $41, $2A, $4F, $6B, $39, $26, $44, $5C, $AC, $30, $34, $48
  .byte $4E, $52, $3B, $3C, $7C, $3C, $4C, $50, $78
data_b0_8B71:
  .byte $48, $21, $4F, $7B, $41, $2A, $4F, $6B, $39, $26, $44, $5C, $AC, $30, $34, $48
  .byte $4E, $52, $3B, $3C, $7C, $3C, $4C, $50, $78

sub_b0_8B8A:
  lda var_0538
  pha
  txa
  pha
  lda #$00
  sta var_0538
  tay
  jsr sub_b0_9539
  lda var_053A
  bne loc_b0_8BA2
  lda #$71
  bne sub_b0_8BC7
loc_b0_8BA2:
  clc
  adc #$01
  cmp #$74
  bcc sub_b0_8BC7
  sbc #$7A
  beq loc_b0_8BB6
  lda var_0545
  beq loc_b0_8BB6
  lda #$79
  bne sub_b0_8BC7
loc_b0_8BB6:
  sta var_053A
  tay
loc_b0_8BBA:
  lda data_b0_8B20,y
  jsr sub_b0_8AD2
  iny
  cpy #$1D
  bcc loc_b0_8BBA
  bcs loc_b0_8C07

sub_b0_8BC7:
  sta var_053A
  asl a
  asl a
  asl a
  asl a
  tay
  ldx #$00

sub_b0_8BD1:
  lda data_b0_8B3D,x
  jsr sub_b0_8AD2
  inx
  cpx #$08
  bcc sub_b0_8BD1
  cpy #$90
  bne sub_b0_8BF0
  ldy #$01
loc_b0_8BE2:
  lda var_0545,y
  jsr sub_b0_8AD2
  iny
  inx
  cpx #$0C
  bcc loc_b0_8BE2
  bcs sub_b0_8BFC

sub_b0_8BF0:
  lda loc_b0_95BC,y
  jsr sub_b0_8AD2
  iny
  inx
  cpx #$0C
  bcc sub_b0_8BF0

sub_b0_8BFC:
  lda data_b0_8B3D,x
  jsr sub_b0_8AD2
  inx
  cpx #$1B
  bcc sub_b0_8BFC
loc_b0_8C07:
  jsr wait_key
  ldy #$00
  jsr sub_b0_9539
  pla
  tax
  pla
  sta var_0538
  lda #$00
  rts

sub_b0_8C18:
  lda var_0538
  pha
  txa
  pha
  lda #$00
  sta var_0538
  tay
  jsr sub_b0_9539
  ldy #$00
  lda var_053B
  and #$80
  eor #$80
  sta var_053B
  bpl loc_b0_8C42
loc_b0_8C35:
  lda data_b0_8B58,y
  jsr sub_b0_8AD2
  iny
  cpy #$19
  bcc loc_b0_8C35
  bcs loc_b0_8C4D
loc_b0_8C42:
  lda data_b0_8B71,y
  jsr sub_b0_8AD2
  iny
  cpy #$19
  bcc loc_b0_8C42
loc_b0_8C4D:
  jsr wait_key
  ldy #$00
  jsr sub_b0_9539
  pla
  tax
  pla
  sta var_0538
  lda #$00
  rts

sub_b0_8C5E:
  ldy var_053C
  bmi sub_b0_8C7C
  lda var_0503,y
  sta var_03FB
  dey
  lda var_0503,y
  sta var_03FC
  dey
  bpl loc_b0_8C76
  sty var_0541
loc_b0_8C76:
  sty var_053C
  lda #$80
  rts

sub_b0_8C7C:
  stx var_052F
  lda var_053B
  bpl sub_b0_8C90
  and #$7F
  beq sub_b0_8C90
  lda #$00
  sta var_051F
loc_b0_8C8D:
  jmp loc_b0_8E5F

sub_b0_8C90:
  lda var_0539
  beq sub_b0_8CA1
  eor #$72
  bne loc_b0_8D0C
  cmp var_052B
  bne loc_b0_8C8D
  jmp loc_b0_8D0C

sub_b0_8CA1:
  jsr key_read_with_repeat
  cmp #$71
  bcc sub_b0_8CB3
  cmp #$75
  bcc sub_b0_8CCB
  cmp #$75
  bne sub_b0_8CB3
  jsr sub_b0_8B8A

sub_b0_8CB3:
  cmp #$7A
  bne sub_b0_8CBA
  jmp loc_b31_C820

sub_b0_8CBA:
  cmp #$76
  bne sub_b0_8CC1
  jsr sub_b0_8C18

sub_b0_8CC1:
  cmp #$79
  bne sub_b0_8CCA
  ldy var_0545
  bne sub_b0_8CCB

sub_b0_8CCA:
  rts

sub_b0_8CCB:
  sta var_0539
  asl a
  asl a
  asl a
  asl a
  tax
  lda #$00
  sta var_0538
  sta var_0541
  jsr sub_b0_82E0
  ldy #$04
  cpx #$90
  bne sub_b0_8CF2
  ldx #$01
loc_b0_8CE6:
  lda var_0545,x
  jsr sub_b0_8AD2
  inx
  dey
  bne loc_b0_8CE6
  beq loc_b0_8CFC

sub_b0_8CF2:
  lda loc_b0_95BC,x
  jsr sub_b0_8AD2
  inx
  dey
  bne sub_b0_8CF2
loc_b0_8CFC:
  lda #$BA
  jsr sub_b0_8AD2
  ldy var_0538
  jsr sub_b0_9539
  lda #$00
  sta var_052B
loc_b0_8D0C:
  ldx var_052B
loc_b0_8D0F:
  jsr wait_key
loc_b0_8D12:
  cmp var_0539
  bne sub_b0_8D2A
loc_b0_8D17:
  lda #$00
  sta var_0539
  jsr sub_b0_82E0
  ldy #$00
  jsr sub_b0_9539
  ldx var_052F
  jmp sub_b0_8CA1

sub_b0_8D2A:
  cmp #$71
  bcc loc_b0_8D32
  cmp #$75
  bcc sub_b0_8CCB
loc_b0_8D32:
  cmp #$75
  bne loc_b0_8D3F
  jsr sub_b0_8B8A
  lda var_0539
  jmp sub_b0_8CCB
loc_b0_8D3F:
  cmp #$76
  bne loc_b0_8D4C
  jsr sub_b0_8C18
  lda var_0539
loc_b0_8D49:
  jmp sub_b0_8CCB
loc_b0_8D4C:
  cmp #$79
  bne loc_b0_8D55
  ldy var_0545
  bne loc_b0_8D49
loc_b0_8D55:
  cmp #$88
  bne loc_b0_8D7C
  dex
  bpl loc_b0_8D64
  inx
loc_b0_8D5D:
  stx var_052B
  ldx var_052F
  rts
loc_b0_8D64:
  dec var_0538
  lda #$A0
  jsr sub_b0_8AD2
  dec var_0538
loc_b0_8D6F:
  lda var_0539
  cmp #$72
  bne loc_b0_8D0F
  cpx #$00
  bne loc_b0_8DBE
  beq loc_b0_8D8A
loc_b0_8D7C:
  cmp #$8D
  bne loc_b0_8D93
  cpx #$00
  beq loc_b0_8D5D
loc_b0_8D84:
  dec var_0538
  dex
  bne loc_b0_8D84
loc_b0_8D8A:
  ldy var_0538
  jsr sub_b0_9539
  jmp loc_b0_8D0F
loc_b0_8D93:
  cmp #$A0
  bcc loc_b0_8D5D
  ldy #$02
  jsr sub_b0_94DC
  bcs loc_b0_8D5D
  sta var_0525,x
  bit var_0541
  bpl loc_b0_8DB3
  lda #$00
  sta var_0541
  ldy #$05
  jsr sub_b0_9539
  lda var_0525,x
loc_b0_8DB3:
  jsr sub_b0_8AD2
  inx
  ldy #$03
  jsr sub_b0_94DC
  bcc loc_b0_8D6F
loc_b0_8DBE:
  stx var_052B
  lda #$00
  sta var_051F
  sta var_0520
  ldy #$04
  jsr sub_b0_94DC
  ldx var_04F0
  bne loc_b0_8DE8
  jsr sub_b0_82E0
  lda #$00
  sta var_052B
  lda #$05
  sta var_0538
  lda #$FF
  sta var_0541
  jmp loc_b0_8D0F
loc_b0_8DE8:
  lda var_04F1,x
  bne loc_b0_8E5C
loc_b0_8DED:
  lda var_04F0
  cmp #$02
  bne loc_b0_8E36
  bit var_053B
  bpl loc_b0_8E21
  jsr sub_b0_8FB5
  beq loc_b0_8E21
  ldy #$02
  ldx #$00
loc_b0_8E02:
  lda var_04F0,y
  sta var_0503,x
  inx
  dey
  bne loc_b0_8E02
  dex
  stx var_053C
  ldx #$00
  stx var_051F
  stx var_0520
  jsr sub_b0_9015
  jsr sub_b0_9490
  jmp loc_b0_8E4C
loc_b0_8E21:
  ldy var_053A
  beq loc_b0_8E36
  lda var_0539
  pha
  sty var_0539
  ldy #$05
  jsr sub_b0_94DC
  pla
  sta var_0539
loc_b0_8E36:
  jsr sub_b0_9490
  ldy var_04F0
  ldx #$00
loc_b0_8E3E:
  lda var_04F0,y
  sta var_0503,x
  inx
  dey
  bne loc_b0_8E3E
  dex
  stx var_053C
loc_b0_8E4C:
  lda #$00
  sta var_052B
  lda #$05
  sta var_0538
  ldx var_052F
  jmp sub_b0_8C5E
loc_b0_8E5C:
  jsr sub_b0_9490
loc_b0_8E5F:
  jsr wait_key
  cmp var_0539
  bne loc_b0_8E6D
  jsr sub_b0_8FAA
  jmp loc_b0_8D17
loc_b0_8E6D:
  cmp #$71
  bcc loc_b0_8E7B
  cmp #$75
  bcs loc_b0_8E7B
loc_b0_8E75:
  jsr sub_b0_8FAA
  jmp sub_b0_8CCB
loc_b0_8E7B:
  cmp #$79
  bne loc_b0_8E84
  ldy var_0545
  bne loc_b0_8E75
loc_b0_8E84:
  cmp #$8D
  bne loc_b0_8E99
loc_b0_8E88:
  lda #$05
  sta var_0538
  tay
  jsr sub_b0_9539
  ldx #$00
  jsr sub_b0_8FAA
  jmp loc_b0_8D0F
loc_b0_8E99:
  cmp #$9B
  bne loc_b0_8ECB
  lda var_053B
  and #$7F
  bne loc_b0_8E88
  lda var_0539
  cmp #$72
  beq loc_b0_8E88
  ldy #$0C
  jsr sub_b0_9539
  dec var_052B
  lda var_052B
  tax
  clc
  adc #$05
  sta var_0538
  lda #$A0
  jsr sub_b0_8AD2
  dec var_0538
  jsr sub_b0_8FAA
  jmp loc_b0_8D0F
loc_b0_8ECB:
  cmp #$AE
  bne loc_b0_8F05
  lda var_051F
  clc
  adc #$06
  sta var_051F
  bcc loc_b0_8EDD
  inc var_0520
loc_b0_8EDD:
  lda var_053B
  and #$7F
  beq loc_b0_8EEA
  jsr sub_b0_9034
  jmp loc_b0_8EFA
loc_b0_8EEA:
  ldy #$06
  pha
  lda var_0539
  cmp #$72
  bne loc_b0_8EF6
  ldy #$04
loc_b0_8EF6:
  pla
  jsr sub_b0_94DC
loc_b0_8EFA:
  lda var_04F0
  bne loc_b0_8F2E
  jsr sub_b0_82E0
  jmp loc_b0_8F09
loc_b0_8F05:
  cmp #$AC
  bne loc_b0_8F44
loc_b0_8F09:
  lda var_051F
  bne loc_b0_8F19
  cmp var_0520
  bne loc_b0_8F19
  jsr sub_b0_82E0
  jmp loc_b0_8E5F
loc_b0_8F19:
  sec
  sbc #$06
  sta var_051F
  bcs loc_b0_8F24
  dec var_0520
loc_b0_8F24:
  lda var_053B
  and #$7F
  beq loc_b0_8F31
  jsr sub_b0_9034
loc_b0_8F2E:
  jmp loc_b0_8E5C
loc_b0_8F31:
  ldy #$06
  pha
  lda var_0539
  cmp #$72
  bne loc_b0_8F3D
  ldy #$04
loc_b0_8F3D:
  pla
  jsr sub_b0_94DC
  jmp loc_b0_8E5C
loc_b0_8F44:
  cmp #$A0
  bne loc_b0_8F4A
  lda #$B1
loc_b0_8F4A:
  cmp #$B7
  bcs loc_b0_8F52
  cmp #$B1
  bcs loc_b0_8F75
loc_b0_8F52:
  pha
  lda var_053B
  and #$7F
  beq loc_b0_8F63
  jsr sub_b0_8FAA
  pla
  ldx #$00
  jmp loc_b0_8D12
loc_b0_8F63:
  lda var_0539
  cmp #$72
  bne loc_b0_8F71
  pla
  ldx var_052B
  jmp loc_b0_8D12
loc_b0_8F71:
  pla
  jmp loc_b0_8E5F
loc_b0_8F75:
  sbc #$B0
  tay
  ldx #$00
loc_b0_8F7A:
  lda var_04F0,x
  beq loc_b0_8F8C
  dey
  beq loc_b0_8F92
  stx var_0534
  sec
  adc var_0534
  tax
  bne loc_b0_8F7A
loc_b0_8F8C:
  jsr sub_b0_82E0
  jmp loc_b0_8E5F
loc_b0_8F92:
  sta var_0534
loc_b0_8F95:
  lda var_04F0,x
  sta var_04F0,y
  inx
  iny
  dec var_0534
  bpl loc_b0_8F95
  lda #$00
  sta var_04F0,y
  jmp loc_b0_8DED

sub_b0_8FAA:
  pha
  lda var_053B
  and #$80
  sta var_053B
  pla
  rts

sub_b0_8FB5:
  lda #$00
  sta var_0027
  lda var_04F1
  sec
  sbc #$30
  bcs loc_b0_8FC4
  lda #$00
  rts
loc_b0_8FC4:
  pha
  asl a
  rol var_0027
  sta var_00FE
  lda var_0027
  sta var_00FF
  lda var_00FE
  ldy #$04
loc_b0_8FD2:
  asl a
  rol var_0027
  dey
  bne loc_b0_8FD2
  adc var_00FE
  sta var_00FE
  lda var_0027
  adc var_00FF
  sta var_00FF
  lda #$00
  sta var_0027
  pla
  ldy #$07
loc_b0_8FE9:
  asl a
  rol var_0027
  dey
  bne loc_b0_8FE9
  adc var_04F2
  bcc loc_b0_8FF6
  inc var_0027
loc_b0_8FF6:
  sec
  sbc #$21
  bcs loc_b0_8FFD
  dec var_0027
loc_b0_8FFD:
  sec
  sbc var_00FE
  sta var_0513
  lda var_0027
  sbc var_00FF
  sta var_0514
  jsr sub_b31_C030
  php
  ora #$80
  sta var_053B
  plp
  rts

sub_b0_9015:
  lda #$62
  sta var_0026
  lda #$BD
  sta var_0027
  lda #$D2
  sta var_00FE
  lda #$97
  and #$7F
  sta var_00FF
  jsr sub_b31_C033
  lda var_00FE
  sta var_0513
  lda var_00FF
  sta var_0514

sub_b0_9034:
  lda var_0513
  sta var_00FE
  lda var_0514
  sta var_00FF
  lda var_053B
  and #$7F
  jmp loc_b31_C036
  .byte $C9, $A0, $D0, $06, $E0, $02, $90, $0A, $18, $60, $C9, $B0, $90, $04, $C9, $BA
  .byte $90, $F6, $38, $60, $E0, $04, $B0, $FA, $C9, $A0, $F0, $F6, $18, $60, $A2, $00
  .byte $A9, $01, $8D, $14, $05, $A9, $5E, $8D, $1E, $05, $A0, $00, $B9, $25, $05, $29
  .byte $0F, $8D, $13, $05, $0A, $0A, $6D, $13, $05, $0A, $8D, $13, $05, $B9, $26, $05
  .byte $29, $0F, $18, $6D, $13, $05, $8D, $13, $05, $B9, $27, $05, $C9, $A0, $F0, $2A
  .byte $29, $0F, $8D, $14, $05, $0A, $0A, $6D, $14, $05, $0A, $8D, $14, $05, $A9, $0A
  .byte $8D, $1E, $05, $B9, $28, $05, $C9, $A0, $F0, $10, $29, $0F, $18, $6D, $14, $05
  .byte $F0, $44, $8D, $14, $05, $A9, $01, $8D, $1E, $05, $AD, $13, $05, $C9, $03, $F0
  .byte $08, $C9, $10, $90, $31, $C9, $58, $B0, $2D, $18, $69, $20, $8D, $13, $05, $AD
  .byte $14, $05, $D0, $02, $A9, $01, $C9, $5F, $B0, $1C, $69, $20, $8D, $14, $05, $AD
  .byte $14, $05, $48, $A2, $00, $18, $6D, $1F, $05, $8D, $14, $05, $B0, $04, $C9, $FF
  .byte $D0, $07, $68, $8D, $14, $05, $4C, $38, $91, $AD, $1E, $05, $38, $ED, $1F, $05
  .byte $8D, $16, $05, $90, $ED, $F0, $EB, $A9, $02, $9D, $F0, $04, $E8, $AD, $13, $05
  .byte $9D, $F0, $04, $E8, $AD, $14, $05, $9D, $F0, $04, $E8, $E0, $12, $B0, $0F, $CE
  .byte $16, $05, $F0, $0A, $EE, $14, $05, $AD, $14, $05, $C9, $FF, $90, $D9, $68, $8D
  .byte $14, $05, $A9, $00, $9D, $F0, $04, $60, $A9, $04, $8D, $F3, $04, $A0, $00, $8C
  .byte $F8, $04, $AD, $F1, $04, $38, $E9, $20, $C9, $0A, $90, $05, $E9, $0A, $C8, $D0
  .byte $F7, $09, $B0, $8D, $F5, $04, $98, $09, $B0, $8D, $F4, $04, $A0, $00, $AD, $F2
  .byte $04, $38, $E9, $20, $C9, $0A, $90, $05, $E9, $0A, $C8, $D0, $F7, $09, $B0, $8D
  .byte $F7, $04, $98, $09, $B0, $8D, $F6, $04, $60, $C9, $C1, $90, $12, $C9, $DB, $B0
  .byte $04, $69, $20, $90, $08, $C9, $E1, $90, $06, $C9, $FB, $B0, $02, $18, $60, $38
  .byte $60, $E0, $06, $B0, $FA, $C9, $A0, $F0, $F6, $18, $60, $A0, $00, $8C, $13, $05
  .byte $8C, $21, $05, $B9, $25, $05, $C9, $A0, $F0, $31, $38, $E9, $DB, $A2, $05, $0E
  .byte $13, $05, $2E, $14, $05, $2E, $15, $05, $2E, $16, $05, $38, $2E, $21, $05, $2E
  .byte $22, $05, $2E, $23, $05, $2E, $24, $05, $CA, $D0, $E4, $0D, $13, $05, $8D, $13
  .byte $05, $C8, $CC, $2B, $05, $90, $CC, $C0, $06, $B0, $22, $A2, $05, $0E, $13, $05
  .byte $2E, $14, $05, $2E, $15, $05, $2E, $16, $05, $0E, $21, $05, $2E, $22, $05, $2E
  .byte $23, $05, $2E, $24, $05, $CA, $D0, $E5, $C8, $C0, $06, $90, $DE, $A9, $C0, $85
  .byte $26, $A9, $9C, $85, $27, $A9, $C2, $85, $FE, $A9, $A4, $85, $FF, $4C, $21, $C0
  .byte $A9, $C0, $85, $26, $A9, $9C, $85, $27, $A9, $C2, $85, $FE, $A9, $A4, $85, $FF
  .byte $A0, $03, $A9, $00, $99, $13, $05, $88, $10, $FA, $20, $24, $C0, $A2, $05, $AD
  .byte $13, $05, $29, $1F, $F0, $03, $18, $69, $DB, $9D, $F4, $04, $A0, $05, $4E, $16
  .byte $05, $6E, $15, $05, $6E, $14, $05, $6E, $13, $05, $88, $D0, $F1, $CA, $10, $DF
  .byte $E8, $BD, $F4, $04, $F0, $09, $E0, $06, $90, $F6, $A9, $00, $9D, $F4, $04, $8E
  .byte $F3, $04, $60, $C9, $A0, $D0, $06, $E0, $00, $D0, $14, $38, $60, $C9, $C1, $90
  .byte $FA, $C9, $DB, $90, $0A, $C9, $FB, $B0, $F2, $C9, $E1, $90, $EE, $E9, $20, $18
  .byte $60, $E0, $04, $B0, $E6, $C9, $A0, $F0, $E2, $18, $60, $A9, $00, $A0, $0B, $99
  .byte $12, $05, $88, $D0, $FA, $38, $08, $B9, $25, $05, $C9, $A0, $F0, $34, $A2, $05
  .byte $C9, $DA, $D0, $07, $A9, $00, $28, $18, $08, $90, $03, $38, $E9, $C0, $08, $2E
  .byte $21, $05, $2E, $22, $05, $2E, $23, $05, $0E, $13, $05, $2E, $14, $05, $2E, $15
  .byte $05, $28, $CA, $D0, $E9, $0D, $13, $05, $8D, $13, $05, $C8, $C0, $04, $90, $C7
  .byte $B0, $4D, $C0, $01, $D0, $2C, $28, $A9, $FF, $8D, $13, $05, $AD, $25, $05, $C9
  .byte $DA, $F0, $66, $38, $E9, $C1, $0A, $A8, $A2, $00, $A9, $02, $9D, $F0, $04, $E8
  .byte $B9, $0C, $96, $9D, $F0, $04, $C8, $E8, $E0, $03, $90, $F4, $A9, $00, $9D, $F0
  .byte $04, $60, $A2, $05, $0E, $13, $05, $2E, $14, $05, $2E, $15, $05, $38, $2E, $21
  .byte $05, $2E, $22, $05, $2E, $23, $05, $CA, $D0, $EA, $C8, $C0, $04, $90, $E3, $A2
  .byte $04, $0E, $13, $05, $2E, $14, $05, $2E, $15, $05, $0E, $21, $05, $2E, $22, $05
  .byte $2E, $23, $05, $CA, $D0, $EB, $28, $B0, $08, $AD, $21, $05, $09, $01, $8D, $21
  .byte $05, $A9, $D2, $85, $26, $A9, $9D, $85, $27, $AD, $1F, $05, $48, $AD, $20, $05
  .byte $48, $AE, $13, $05, $E0, $FF, $D0, $2B, $E8, $AD, $1F, $05, $C9, $19, $B0, $20
  .byte $0A, $A8, $A9, $02, $9D, $F0, $04, $E8, $B9, $0C, $96, $9D, $F0, $04, $C8, $E8
  .byte $B9, $0C, $96, $9D, $F0, $04, $C8, $E8, $E0, $12, $B0, $04, $C0, $32, $90, $E2
  .byte $4C, $FF, $93, $A2, $00, $8E, $1E, $05, $8E, $16, $05, $8E, $17, $05, $38, $20
  .byte $27, $C0, $C9, $00, $D0, $04, $C0, $00, $F0, $5F, $8C, $1B, $05, $8D, $1A, $05
  .byte $AD, $16, $05, $48, $AD, $17, $05, $48, $A9, $00, $A0, $02, $0E, $16, $05, $2E
  .byte $17, $05, $2A, $88, $D0, $F6, $8D, $19, $05, $AD, $16, $05, $18, $69, $D2, $8D
  .byte $16, $05, $A9, $9F, $29, $7F, $6D, $17, $05, $8D, $17, $05, $90, $03, $EE, $19
  .byte $05, $AD, $16, $05, $85, $FE, $AD, $17, $05, $0A, $2E, $19, $05, $0A, $2E, $19
  .byte $05, $4A, $4A, $09, $80, $85, $FF, $68, $8D, $17, $05, $68, $8D, $16, $05, $AD
  .byte $19, $05, $69, $8F, $20, $2A, $C0, $90, $96, $A9, $00, $9D, $F0, $04, $68, $8D
  .byte $20, $05, $68, $8D, $1F, $05, $60, $60, $A9, $D2, $85, $26, $A9, $9F, $85, $27
  .byte $A9, $00, $8D, $13, $05, $8D, $14, $05, $20, $2D, $C0, $A0, $04, $4A, $6E, $14
  .byte $05, $6E, $13, $05, $88, $D0, $F6, $8D, $15, $05, $A2, $03, $AD, $13, $05, $29
  .byte $1F, $F0, $03, $18, $69, $C0, $9D, $F4, $04, $A0, $05, $4E, $15, $05, $6E, $14
  .byte $05, $6E, $13, $05, $88, $D0, $F4, $CA, $10, $E2, $E8, $BD, $F4, $04, $F0, $09
  .byte $E0, $04, $90, $F6, $A9, $00, $9D, $F4, $04, $8E, $F3, $04, $60, $C9, $FF, $D0
  .byte $02, $38, $60, $18, $60, $38, $60, $A9, $02, $8D, $F0, $04, $A9, $23, $8D, $F1
  .byte $04, $AD, $25, $05, $29, $7F, $C9, $21, $B0, $07, $A9, $57, $8D, $F1, $04, $A9
  .byte $7E, $8D, $F2, $04, $A9, $00, $8D, $F3, $04, $60

sub_b0_9490:
  lda var_0021
  pha
  lda #$1F
  sta var_0021
  lda var_0538
  pha
  lda #$0C
  sta var_0538
  ldy #$00
  ldx #$01
loc_b0_94A4:
  txa
  pha
  ldx var_04F1,y
  bmi loc_b0_94AF
  ora #$B0
  bne loc_b0_94B1
loc_b0_94AF:
  lda #$AD
loc_b0_94B1:
  jsr sub_b0_9659
  ldx var_04F0,y
  iny
loc_b0_94B8:
  lda var_04F0,y
  jsr sub_b0_9659
  iny
  dex
  bne loc_b0_94B8
  pla
  tax
  inx
  lda var_04F0,y
  bne loc_b0_94A4
  ldy var_0538
  cpy #$1E
  bcs loc_b0_94D4
  jsr sub_b0_9539
loc_b0_94D4:
  pla
  sta var_0538
  pla
  sta var_0021
  rts

sub_b0_94DC:
  pha
  lda var_0539
  cmp #$79
  bne loc_b0_94FF
  sty var_052C
  tya
  asl a
  adc var_052C
  pha
  ldy #$19
loc_b0_94EF:
  lda data_b0_951F,y
  sta var_0400,y
  dey
  bpl loc_b0_94EF
  pla
  sta var_040B
  jmp var_0400
loc_b0_94FF:
  sec
  sbc #$71
  asl a
  asl a
  asl a
  asl a
  sta var_052C
  tya
  asl a
  adc var_052C
  tay
  lda data_b0_95CC,y
  sta var_052C
  lda data_b0_95CD,y
  sta var_052D
  pla
  jmp (var_052C)
data_b0_951F:
  .byte $AD, $45, $05, $8D, $C8, $03, $8D, $00, $50, $68, $20, $00, $80, $08, $48, $A9
  .byte $80, $8D, $C8, $03, $8D, $00, $50, $68, $28, $60

sub_b0_9539:
  lda #$0E

sub_b0_953B:
  sta var_0026

sub_b0_953D:
  lda vram_job_count
  bne sub_b0_953D
  txa
  pha
  sty var_0027
  lda #$1E
  sec
  sbc var_0027
  pha
  ldy var_0026
  ldx var_0027
  jsr sub_b31_C01E
  ldx #$02
  pla
  tay
  cmp #$10
  bcs sub_b0_9593
  jsr wait_next_vblank
loc_b0_955E:
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006
  tya
  pha
  lda #$00
loc_b0_956C:
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  dey
  bne loc_b0_956C
  inc var_0027
  pla
  tay
  dex
  bne loc_b0_955E
  pla
  tax
  jmp sub_b0_877F

sub_b0_9593:
  jsr wait_next_vblank
  lda var_0027
  sta var_2006
  lda var_0026
  sta var_2006
  tya
  pha
  lda #$00

sub_b0_95A4:
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
  sta var_2007
loc_b0_95BC:
  dey
  bne sub_b0_95A4
  inc var_0027
  pla
  tay
  jsr sub_b0_877F
  dex
  bne sub_b0_9593
  pla
  tax
  rts
data_b0_95CC:
  .byte $47
data_b0_95CD:
  .byte $78, $4E, $3B
  .byte $46, $90, $5A, $90, $64, $90, $3E, $91, $E5, $90, $93, $87
  .byte $46, $34, $52, $74
  .byte $7F, $91, $97, $91, $A1, $91, $16, $92, $03, $92, $93, $87
  .byte $4E, $65, $31, $4A
  .byte $69, $92, $87, $92
data_b0_95F4:
  .byte $91, $92, $0E, $94, $47, $93, $93, $87
  .byte $48, $2B, $3D, $47
  .byte $63, $94, $6B, $94, $6D, $94, $93, $87, $6D, $94, $93, $87, $39, $24, $41, $4B
  .byte $52, $54, $54, $5A, $53, $50, $35, $58, $52, $3B, $49, $4F, $32, $3B, $4A, $47
  .byte $56, $50, $39, $7A, $4D, $2C, $43, $71, $4E, $2A, $55, $62, $4E, $52, $35, $44
  .byte $52, $2A, $3A, $4D, $32, $7A, $37, $22, $48, $4B, $3E, $2D, $56, $77

sub_b0_963E:
  cpx #$58
  bcc sub_b0_9648
  lda data_b0_95F4,x
  ldy #$00
  rts

sub_b0_9648:
  lda data_b0_887C,x
  rts
  .byte $1E, $1C, $1F, $EC, $1D, $ED, $E9, $7F, $FF, $E8, $EB, $FC, $FD

sub_b0_9659:
  pha
  lda var_0538
  cmp #$1D
  pla
  bcs loc_b0_9665
  jmp sub_b0_8AD2
loc_b0_9665:
  rts
  .byte $8D, $F6, $02, $20, $DE, $98, $C4, $20, $D0, $16, $A5, $25, $C5, $22, $F0, $21
  .byte $38, $E9, $01, $20, $C1, $FB, $A0, $1F, $B1, $28, $30, $15, $C6, $25, $A4, $21
  .byte $88, $B1, $28, $30, $01, $88, $84, $24, $AD, $F6, $02, $20, $38, $99, $20, $75
  .byte $99, $4C, $6C, $95, $C9, $FF, $D0, $5E, $20, $D7, $98, $98, $A0, $1E, $D1, $28
  .byte $90, $03, $4C, $71, $95, $A8, $A5, $25, $8D, $F4, $02, $C8, $C4, $21, $B0, $0B
  .byte $C8, $C4, $21, $90, $28, $88, $88, $B1, $28, $30, $2A, $A0, $1F, $B1, $28, $30
  .byte $15, $AD, $F4, $02, $18, $69, $01, $C5, $23, $B0, $0B, $8D, $F4, $02, $20, $C1
  .byte $FB, $A4, $20, $4C, $EB, $96, $A9, $00, $8D, $F5

sub_b0_96E0:
  cmp #$80
  bne sub_b0_96F2
  lda var_03FC
  cmp #$22
  bne loc_b0_96F0
  lda #$20
  sta var_03FC
loc_b0_96F0:
  lda #$80

sub_b0_96F2:
  rts
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00

sub_b0_9700:
  jmp sub_b0_9767

sub_b0_9703:
  jmp sub_b0_970F

sub_b0_9706:
  jmp sub_b0_974A

sub_b0_9709:
  jmp sub_b0_975C

sub_b0_970C:
  jmp sub_b0_9740

sub_b0_970F:
  ldx var_0022

sub_b0_9711:
  txa
  jsr sub_b31_FBC1
  ldy var_0020
  lda #$A0

sub_b0_9719:
  sta (var_0028),y
  iny
  cpy var_0021
  bcc sub_b0_9719
  ldy #$1E
  lda #$00
  sta (var_0028),y
  ldy #$1F
  lda #$FF
  sta (var_0028),y
  inx
  cpx var_0023
  bcc sub_b0_9711
  jsr sub_b31_C00C
  lda var_0022
  sta var_0025
  jsr sub_b31_FBC1
  lda var_0020
  sta var_0024
  rts

sub_b0_9740:
  jsr sub_b31_FD3F
  ldy #$1E
  lda #$00
  sta (var_0028),y
  rts

sub_b0_974A:
  pha
  sec
  sbc var_0020
  ldy #$1E
  cmp (var_0028),y
  bcc loc_b0_9756
  sta (var_0028),y
loc_b0_9756:
  pla
  cmp var_0021
  bcs sub_b0_975C
  rts

sub_b0_975C:
  ldy #$1F
  sta (var_0028),y
  lda var_0020
  sta var_0024
  jmp sub_b0_80D0

sub_b0_9767:
  lda var_0024
  sta var_02F9
loc_b0_976C:
  lda var_0025
  jsr sub_b31_FBC1

sub_b0_9771:
  ldy var_0024
  jsr monitor_rdkey
  cmp #$9E
  bne sub_b0_97A2
  jsr sub_b0_9ACC
  lda var_0025
  cmp var_0022
  beq sub_b0_9771
  sec
  sbc #$01
loc_b0_9786:
  sta var_0025
  jsr sub_b31_FBC1
  ldy var_0020
loc_b0_978D:
  cpy var_0024
  beq sub_b0_9771
  bcs loc_b0_979B
  lda (var_0028),y
  bmi loc_b0_9798
  iny
loc_b0_9798:
  iny
  bne loc_b0_978D
loc_b0_979B:
  dey
  dey
  sty var_0024
  jmp sub_b0_9771

sub_b0_97A2:
  cmp #$9F
  bne sub_b0_97B4
  jsr sub_b0_9ACC
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcs sub_b0_9771
  bcc loc_b0_9786

sub_b0_97B4:
  cmp #$9D
  bne sub_b0_97DA
  jsr sub_b0_9ACC
  cpy var_0020
  bne loc_b0_97CF
  lda var_0025
  cmp var_0022
  beq sub_b0_9771
  sec
  sbc #$01
  sta var_0025
  jsr sub_b31_FBC1
  ldy var_0021
loc_b0_97CF:
  dey
  lda (var_0028),y
  bmi loc_b0_97D5
  dey
loc_b0_97D5:
  sty var_0024
  jmp sub_b0_9771

sub_b0_97DA:
  cmp #$9C
  bne sub_b0_9811
  jsr sub_b0_9ACC
loc_b0_97E1:
  iny
  cpy var_0021
  bcs loc_b0_97F1
  iny
  cpy var_0021
  bcc loc_b0_9804
  dey
  dey
  lda (var_0028),y
  bmi loc_b0_980B
loc_b0_97F1:
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcs loc_b0_980E
  sta var_0025
  jsr sub_b31_FBC1
  ldy var_0020
  jmp loc_b0_980C
loc_b0_9804:
  dey
  dey
  lda (var_0028),y
  bmi loc_b0_980B
  iny
loc_b0_980B:
  iny
loc_b0_980C:
  sty var_0024
loc_b0_980E:
  jmp sub_b0_9771

sub_b0_9811:
  cmp #$68
  bne sub_b0_9833
  jsr sub_b0_9ACC
  ldx var_0025
loc_b0_981A:
  cpx var_0022
  beq loc_b0_982A
  dex
  txa
  jsr sub_b31_FBC1
  ldy #$1F
  lda (var_0028),y
  bpl loc_b0_981A
  inx
loc_b0_982A:
  stx var_0025
  lda var_0020
  sta var_0024
  jmp loc_b0_976C

sub_b0_9833:
  cmp #$69
  bne sub_b0_985D
  jsr sub_b0_9ACC
loc_b0_983A:
  ldy #$1F
  lda (var_0028),y
  bmi loc_b0_9851
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcs loc_b0_9851
  sta var_0025
  jsr sub_b31_FBC1
  jmp loc_b0_983A
loc_b0_9851:
  ldy #$1E
  lda (var_0028),y
  clc
  adc var_0020
  sta var_0024
  jmp sub_b0_9771

sub_b0_985D:
  cmp #$88
  bne sub_b0_989A
  jsr sub_b0_9ACC
  lda var_0025
  sta var_02F6
  jsr sub_b0_9AD3
  cpy var_0020
  bne loc_b0_9886
  lda var_0025
  cmp var_0022
  beq loc_b0_9897
  sec
  sbc #$01
  jsr sub_b31_FBC1
  ldy #$1F
  lda (var_0028),y
  bmi loc_b0_9897
  dec var_0025
  ldy var_0021
loc_b0_9886:
  dey
  lda (var_0028),y
  bmi loc_b0_988C
  dey
loc_b0_988C:
  sty var_0024
  lda var_02F6
  jsr sub_b0_9B2D
  jsr sub_b0_9B6A
loc_b0_9897:
  jmp loc_b0_976C

sub_b0_989A:
  cmp #$FF
  bne sub_b0_98FC
  jsr sub_b0_9ACC
  tya
  ldy #$1E
  cmp (var_0028),y
  bcc loc_b0_98AB
  jmp sub_b0_9771
loc_b0_98AB:
  tay
  lda var_0025
  sta var_02F4
  iny
  cpy var_0021
  bcs loc_b0_98C1
  iny
  cpy var_0021
  bcc loc_b0_98E3
  dey
  dey
  lda (var_0028),y
  bmi loc_b0_98EB
loc_b0_98C1:
  ldy #$1F
  lda (var_0028),y
  bmi loc_b0_98DC
  lda var_02F4
  clc
  adc #$01
  cmp var_0023
  bcs loc_b0_98DC
  sta var_02F4
  jsr sub_b31_FBC1
  ldy var_0020
  jmp loc_b0_98EB
loc_b0_98DC:
  lda #$00
  sta var_02F5
  beq loc_b0_98F6
loc_b0_98E3:
  dey
  dey
  lda (var_0028),y
  bmi loc_b0_98EA
  iny
loc_b0_98EA:
  iny
loc_b0_98EB:
  lda var_02F4
  pha
  jsr sub_b0_9AD3
  pla
  jsr sub_b0_9B2D
loc_b0_98F6:
  jsr sub_b0_9B6A
  jmp loc_b0_976C

sub_b0_98FC:
  cmp #$6D
  bne sub_b0_991B
  lda var_00EA
  eor #$20
  sta var_00EA
  and #$20
  beq loc_b0_9918
  lda var_0025
  jsr sub_b0_9AD3
  stx var_02F6
  lda var_02F5
  sta var_02F7
loc_b0_9918:
  jmp sub_b0_9771

sub_b0_991B:
  cmp #$8D
  beq sub_b0_9922
  jmp sub_b0_99A8

sub_b0_9922:
  jsr sub_b0_9ACC
  ldx var_0025

sub_b0_9927:
  cpx var_0022
  beq loc_b0_9937
  dex
  txa
  jsr sub_b31_FBC1
  ldy #$1F
  lda (var_0028),y
  bpl sub_b0_9927
  inx
loc_b0_9937:
  txa
  sta var_0025
  inc var_0021
  jsr sub_b31_FBC1
  lda var_0020
  clc
  adc var_02F9
  sta var_0024
  tay
  ldx #$00

sub_b0_994A:
  tya
  pha
  ldy #$1E
  lda (var_0028),y
  clc
  adc var_0020
  sta var_02F4
  pla
  tay

sub_b0_9958:
  cpy var_02F4
  bcs sub_b0_997B
  lda (var_0028),y
  bmi sub_b0_9970
  lda #$7F
  sta var_0200,x
  inx
  lda (var_0028),y
  sta var_0200,x
  iny
  inx
  lda (var_0028),y

sub_b0_9970:
  sta var_0200,x
  iny
  inx
  cpx #$F0
  bcs sub_b0_9994
  bcc sub_b0_9958

sub_b0_997B:
  ldy #$1F
  lda (var_0028),y
  bmi sub_b0_9994
  ldy var_0020
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcs sub_b0_9994
  sta var_0025
  jsr sub_b31_FBC1
  jmp sub_b0_994A

sub_b0_9994:
  dec var_0021
  lda var_0025
  jsr sub_b31_FBC1
  lda #$03
  sta var_64C1
  lda #$8D
  sta var_0200,x
  jmp monitor_cout

sub_b0_99A8:
  cmp #$80
  beq loc_b0_99B0
  cmp #$A0
  bcc loc_b0_9A0C
loc_b0_99B0:
  pha
  lda var_00EA
  and #$20
  beq sub_b0_99BA
  jmp loc_b0_9A63

sub_b0_99BA:
  pla
  cmp #$80
  beq loc_b0_99C2
  jmp sub_b0_9A4F
loc_b0_99C2:
  lda (var_0028),y
  bmi loc_b0_9A0F
loc_b0_99C6:
  lda var_03FB
  sta (var_0028),y
  iny
  lda var_03FC
  sta (var_0028),y
  lda #$80

sub_b0_99D3:
  jsr sub_b0_9C17
  iny
  cpy var_0021
  bcc sub_b0_99FE
  tya
  sec
  sbc var_0020
  ldy #$1E
  sta (var_0028),y
  ldy #$1F
  lda #$00
  sta (var_0028),y
  ldy var_0020
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcc sub_b0_99F9
  jsr sub_b0_9BFF
  lda var_0025

sub_b0_99F9:
  sta var_0025
  jsr sub_b31_FBC1

sub_b0_99FE:
  sty var_0024
  tya
  sec
  sbc var_0020
  ldy #$1E
  cmp (var_0028),y
  bcc loc_b0_9A0C
  sta (var_0028),y
loc_b0_9A0C:
  jmp sub_b0_9771
loc_b0_9A0F:
  iny
  cpy var_0021
  bcc loc_b0_9A3D
  dey
  tya
  sec
  sbc var_0020
  ldy #$1E
  sta (var_0028),y
  ldy #$1F
  lda #$00
  sta (var_0028),y
  ldy var_0020
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcc loc_b0_9A33
  jsr sub_b0_9BFF
  lda var_0025
loc_b0_9A33:
  sta var_0025
  jsr sub_b31_FBC1
  sty var_0024
  jmp loc_b0_99C2
loc_b0_9A3D:
  lda (var_0028),y
  bpl loc_b0_9A45
  dey
  jmp loc_b0_99C6
loc_b0_9A45:
  iny
  lda #$A0
  sta (var_0028),y
  dey
  dey
  jmp loc_b0_99C6

sub_b0_9A4F:
  pha
  lda (var_0028),y
  asl a
  pla
  sta (var_0028),y
  bcs sub_b0_9A60
  iny
  lda #$A0
  sta (var_0028),y
  dey
  lda (var_0028),y

sub_b0_9A60:
  jmp sub_b0_99D3
loc_b0_9A63:
  lda var_02F6
  tax
  tay
  pla
  pha
  iny
  cmp #$80
  bne loc_b0_9A70
  iny
loc_b0_9A70:
  sty var_02F8
  dey
  cpx #$00
  beq loc_b0_9A8B
  dex
  beq loc_b0_9A85
loc_b0_9A7B:
  lda var_0200,x
  sta var_0200,y
  dey
  dex
  bne loc_b0_9A7B
loc_b0_9A85:
  lda var_0200,x
  sta var_0200,y
loc_b0_9A8B:
  ldy #$00
  pla
  cmp #$80
  bne loc_b0_9A9C
  lda var_03FB
  sta var_0200,y
  iny
  lda var_03FC
loc_b0_9A9C:
  sta var_0200,y
  lda var_02F7
  sta var_02F5
  ldx var_02F8
  jsr sub_b0_9B6A
  lda var_02F8
  sec
  sbc var_02F6
  tay
  ldx #$00
loc_b0_9AB5:
  lda var_0200,y
  sta var_0200,x
  iny
  inx
  cpx var_02F6
  bcc loc_b0_9AB5
  lda var_0025
  jsr sub_b31_FBC1
  ldy var_0024
  jmp loc_b0_97E1

sub_b0_9ACC:
  lda var_00EA
  and #$DF
  sta var_00EA
  rts

sub_b0_9AD3:
  sta var_02F5
  tya
  pha
  lda var_0028
  pha
  lda var_0029
  pha
  ldx #$00
loc_b0_9AE0:
  tya
  pha
  ldy #$1E
  lda (var_0028),y
  clc
  adc var_0020
  sta var_02F4
  pla
  tay
loc_b0_9AEE:
  cpy var_02F4
  bcs loc_b0_9B09
  lda (var_0028),y
  sta var_0200,x
  bmi loc_b0_9B01
  iny
  inx
  lda (var_0028),y
  sta var_0200,x
loc_b0_9B01:
  iny
  inx
  cpx #$F0
  bcs loc_b0_9B24
  bcc loc_b0_9AEE
loc_b0_9B09:
  ldy #$1F
  lda (var_0028),y
  bmi loc_b0_9B24
  ldy var_0020
  lda var_02F5
  clc
  adc #$01
  cmp var_0023
  bcs loc_b0_9B24
  sta var_02F5
  jsr sub_b31_FBC1
  jmp loc_b0_9AE0
loc_b0_9B24:
  pla
  sta var_0029
  pla
  sta var_0028
  pla
  tay
  rts

sub_b0_9B2D:
  sta var_02F4
  tya
  pha
  lda var_0028
  pha
  lda var_0029
  pha
loc_b0_9B38:
  tya
  pha
  sec
  sbc var_0020
  ldy #$1E
  sta (var_0028),y
  pla
  tay
  lda #$A0
loc_b0_9B45:
  sta (var_0028),y
  iny
  cpy var_0021
  bcc loc_b0_9B45
  ldy var_0020
  inc var_02F4
  lda var_02F4
  cmp var_02F5
  beq loc_b0_9B5B
  bcs loc_b0_9B61
loc_b0_9B5B:
  jsr sub_b31_FBC1
  jmp loc_b0_9B38
loc_b0_9B61:
  pla
  sta var_0029
  pla
  sta var_0028
  pla
  tay
  rts

sub_b0_9B6A:
  stx var_02F4
  lda var_0025
  pha
  jsr sub_b31_FBC1
  lda var_0024
  pha
  tay
  ldx #$00
loc_b0_9B79:
  cpx var_02F4
  bcs loc_b0_9BD1
  lda var_0200,x
  bmi loc_b0_9B9C
  iny
  cpy var_0021
  dey
  bcs loc_b0_9BA9
  sta (var_0028),y
  sta var_03FB
  inx
  iny
  lda var_0200,x
  sta (var_0028),y
  sta var_03FC
  lda #$80
  bmi loc_b0_9B9E
loc_b0_9B9C:
  sta (var_0028),y
loc_b0_9B9E:
  jsr sub_b0_9C17
  iny
  inx
  cpy var_0021
  bcc loc_b0_9BCC
  bcs loc_b0_9BAE
loc_b0_9BA9:
  lda #$A0
  jsr sub_b0_9C17
loc_b0_9BAE:
  tya
  sec
  sbc var_0020
  ldy #$1E
  sta (var_0028),y
  ldy #$1F
  lda #$00
  sta (var_0028),y
  ldy var_0020
  lda var_0025
  clc
  adc #$01
  cmp var_0023
  bcs loc_b0_9BDE
  sta var_0025
  jsr sub_b31_FBC1
loc_b0_9BCC:
  sty var_0024
  jmp loc_b0_9B79
loc_b0_9BD1:
  lda var_0025
  sta var_02F7
  tya
  sec
  sbc var_0020
  ldy #$1E
  sta (var_0028),y
loc_b0_9BDE:
  ldy #$1F
  lda #$FF
  sta (var_0028),y
  lda var_0025
  ldy var_0024
loc_b0_9BE8:
  jsr sub_b31_C009
  ldy #$00
  inc var_0025
  lda var_0025
  cmp var_02F5
  bcc loc_b0_9BE8
  beq loc_b0_9BE8
  pla
  sta var_0024
  pla
  sta var_0025
  rts

sub_b0_9BFF:
  txa
  pha
  tya
  pha
  lda var_0028
  pha
  lda var_0029
  pha
  jsr sub_b31_FC70
  pla
  sta var_0029
  pla
  sta var_0028
  pla
  tay
  pla
  tax
  rts

sub_b0_9C17:
  cmp #$80
  bne sub_b0_9C23
  jsr sub_b31_FB28
  sta var_03FC
  lda #$80

sub_b0_9C23:
  jmp sub_b31_C003
  .byte $D0, $08, $20, $28, $FB, $8D, $FC, $03, $A9, $80, $4C, $03, $C0
  .res 205, $00
data_b0_9D00:
  .byte $81
data_b0_9D01:
  .byte $6D
data_b0_9D02:
  .byte $2F
data_b0_9D03:
  .byte $9E
data_b0_9D04:
  .byte $60
data_b0_9D05:
  .byte $9E
data_b0_9D06:
  .byte $E4
data_b0_9D07:
  .byte $64
data_b0_9D08:
  .byte $02
data_b0_9D09:
  .byte $65
data_b0_9D0A:
  .byte $CF
data_b0_9D0B:
  .byte $64, $00
data_b0_9D0D:
  .byte $9D, $30, $65
data_b0_9D10:
  .byte $8D
data_b0_9D11:
  .byte $9E
  .byte $B4, $9E, $C5, $9E
  .byte $D1, $9E
  .byte $F4, $9E, $03, $9F, $13, $9F
data_b0_9D1E:
data_b0_9D1F = * + 1
  .word loc_b0_A4E3 - 1, sub_b0_A3CF - 1, loc_b0_A371 - 1, loc_b0_A475 - 1
  .word loc_b0_A475 - 1, loc_b0_A227 - 1, loc_b0_A241 - 1, loc_b0_A245 - 1
  .word sub_b0_A2BA - 1, loc_b0_A4AB - 1, loc_b0_A520 - 1, loc_b0_A4A0 - 1
  .word loc_b0_A537 - 1, sub_b0_A273 - 1, loc_b0_A268 - 1, loc_b0_A251 - 1
  .word loc_b0_A50A - 1, loc_b0_A209 - 1, loc_b0_A213 - 1, loc_b0_A204 - 1
  .word loc_b0_A301 - 1, sub_b0_A32D - 1, loc_b0_A35E - 1, loc_b0_A24D - 1
  .word loc_b0_B65D - 1, loc_b0_B678 - 1, loc_b0_B681 - 1, loc_b0_B69B - 1
  .word loc_b0_A4DF - 1, loc_b0_B77A - 1, loc_b0_B791 - 1, loc_b0_B795 - 1
  .word loc_b0_B67C - 1, loc_b0_B80B - 1, loc_b0_B811 - 1, loc_b0_A23A - 1
  .word loc_b0_B81E - 1, loc_b31_CE2C - 1
data_b0_9D6A:
  .word loc_b31_CE2B
data_b0_9D6C:
  .byte $38, $CE
data_b0_9D6E:
  .byte $00
  .byte $E0
data_b0_9D70:
  .byte $3E, $CE
data_b0_9D72:
  .byte $44, $CE

sub_b0_9D74:
  jsr sub_b0_B401
  lda #$10
  sta var_64D9
  lda var_6458
  sta var_64D7
  lda #$40
  sta var_6525
  sec
  bcs sub_b0_9D9B
  lda var_6525
  bne loc_b0_9D93
  lda #$20
  bne loc_b0_9D98
loc_b0_9D93:
  asl a
  bpl loc_b0_9D9A
  lda #$4C
loc_b0_9D98:
  cmp #$4C
loc_b0_9D9A:
  clc

sub_b0_9D9B:
  php
  jsr sub_b0_A747
  lda #$00
  sta var_64CD
  sta var_64C1
  plp
  ror a
  sta var_64C0
  bmi sub_b0_9DB1
  jmp (data_b0_9D70)

sub_b0_9DB1:
  jmp (data_b0_9D6E)

sub_b0_9DB4:
  lda var_6520
  sta var_64C6
  jsr sub_b0_B445
  lda var_6522
  beq sub_b0_9DCB
  pha
  jsr sub_b0_A5F7
  pla
  ldy #$00
  sta (var_0040),y

sub_b0_9DCB:
  jsr sub_b0_A6C3
  lda var_64CE
  bne sub_b0_9DF3
  ldx #$2F
loc_b0_9DD5:
  lda data_b0_9DFF,x
  sta var_03D0,x
  dex
  bpl loc_b0_9DD5
  lda data_b0_9E01
  sta var_03F3
  eor #$A5
  sta var_03F4
  lda data_b0_9E00
  sta var_03F2
  lda #$06
  bne loc_b0_9DF8

sub_b0_9DF3:
  lda var_64D1
  beq sub_b0_9DFE
loc_b0_9DF8:
  sta var_64CE
  jmp sub_b0_A15B

sub_b0_9DFE:
  rts
data_b0_9DFF:
  .byte $4C
data_b0_9E00:
  .byte $8A
data_b0_9E01:
  .byte $9D, $4C, $74, $9D, $4C, $E0, $A9, $4C, $A1, $B3, $AD, $0F, $9D, $AC, $0E, $9D
  .byte $60, $AD, $A5, $A9, $AC, $A4, $A9, $60, $4C, $47, $A7, $EA, $EA, $4C, $59, $FA
  .byte $4C, $65, $FF, $4C, $58, $FF, $4C, $65, $FF, $4C, $65, $FF, $65, $FF

sub_b0_9E2F:
  jsr sub_b0_9E74
  lda var_64C0
  beq sub_b0_9E41
  pha
  pla
  bmi sub_b0_9E3E
loc_b0_9E3B:
  jmp loc_b0_A580

sub_b0_9E3E:
  jsr sub_b0_9DB4

sub_b0_9E41:
  lda var_6522
  beq sub_b0_9E49
  jsr sub_b0_A5DC

sub_b0_9E49:
  lda #$03
  sta var_64C1
  jsr sub_b0_9F5D
  jsr sub_b0_9E5D
  sta var_64CB
  stx var_64C9
  jmp sub_b0_9F56

sub_b0_9E5D:
  jmp (var_0038)

sub_b0_9E60:
  jsr sub_b0_9E74
  lda var_64C1
  asl a
  tax
  lda data_b0_9D11,x
  pha
  lda data_b0_9D10,x
  pha
  lda var_64CB
  rts

sub_b0_9E74:
  sta var_64CB
  stx var_64C9
  sty var_64CA
  tsx
  inx
  inx
  stx var_64C8
  ldx #$03

sub_b0_9E85:
  lda var_64C2,x
  sta var_0036,x
  dex
  bpl sub_b0_9E85
  rts
loc_b0_9E8E:
  ldx var_6526
  beq sub_b0_9E96
  jmp loc_b0_9F1B

sub_b0_9E96:
  ldx var_64C0
  beq sub_b0_9EA3
  cmp #$BF
  beq loc_b0_9F14
  cmp var_0033
  beq sub_b0_9ECA

sub_b0_9EA3:
  ldx #$02
  stx var_64C1
  cmp var_6521
  bne sub_b0_9EC6
  dex
  stx var_64C1
  dex
  stx var_64CC
  ldx var_64CC

sub_b0_9EB8:
  sta var_0200,x
  inx
  stx var_64CC
  cmp #$8D
  bne loc_b0_9F38
  jmp sub_b0_9F70

sub_b0_9EC6:
  cmp #$8D
  bne sub_b0_9F47

sub_b0_9ECA:
  ldx #$00
  stx var_64C1
  jmp sub_b0_9F47
  ldx #$00
  stx var_64C1
  cmp #$8D
  beq sub_b0_9EE2
loc_b0_9EDB:
  lda var_6522
  beq sub_b0_9F47
  bne loc_b0_9F40

sub_b0_9EE2:
  pha
  sec
  lda var_6522
  bne loc_b0_9EEC
  jsr sub_b0_A5B8
loc_b0_9EEC:
  pla
  bcc loc_b0_9EDB
  ldx var_64C9
  jmp sub_b0_9EB8
loc_b0_9EF5:
  cmp #$8D
  bne loc_b0_9EFE
  lda #$05
  sta var_64C1
loc_b0_9EFE:
  jsr sub_b0_A568
  jmp loc_b0_9F3C
  cmp var_6521
  beq loc_b0_9E8E
  cmp #$8A
  beq loc_b0_9EFE
  ldx #$04
  stx var_64C1
  bne loc_b0_9EF5
loc_b0_9F14:
  lda #$00
  sta var_64C1
  beq loc_b0_9F40
loc_b0_9F1B:
  lda #$00
  sta var_6526
  jsr sub_b0_A747
  jmp loc_b0_A480
loc_b0_9F26:
  lda var_0200
  cmp var_6521
  beq loc_b0_9F38
  lda #$8D
  sta var_0200
  ldx #$00
  stx var_64C9
loc_b0_9F38:
  lda #$40
  bne loc_b0_9F42
loc_b0_9F3C:
  lda #$10
  bne loc_b0_9F42
loc_b0_9F40:
  lda #$20
loc_b0_9F42:
  and var_64CD
  beq sub_b0_9F56

sub_b0_9F47:
  jsr sub_b0_9F5D
  jsr sub_b0_9F68
  sta var_64CB
  sty var_64CA
  stx var_64C9

sub_b0_9F56:
  jsr sub_b0_A747
  ldx var_64C8
  txs

sub_b0_9F5D:
  lda var_64CB
  ldy var_64CA
  ldx var_64C9
  sec
  rts

sub_b0_9F68:
  jmp (var_0036)

sub_b0_9F6B:
  lda #$8D
  jmp sub_b0_9F68

sub_b0_9F70:
  lda data_b0_8695
  eor #$F5
  bne sub_b0_9F68
  sta var_64C1
  lda #$00
  sta var_67E8
  tsx
  stx var_67E9
  lda data_b0_8696
  eor data_b31_F9AD
  sta bank_shadow
  sta REG_BANK
  ldy #$FF
  sty var_64CE
  iny
  sty var_64D1

sub_b0_9F98:
  inc var_64CE
  ldx #$00
  php
  lda var_0200,x
  cmp var_6521
  bne sub_b0_9FA7
  inx

sub_b0_9FA7:
  stx var_64CC

sub_b0_9FAA:
  jsr sub_b0_A17F
  and #$7F
  eor data_b0_A77A,y
  iny
  asl a
  beq loc_b0_9FB8
  pla
  php
loc_b0_9FB8:
  bcc sub_b0_9FAA
  plp
  beq loc_b0_9FDD
  lda data_b0_A77A,y
  bne sub_b0_9F98
loc_b0_9FC2:
  lda var_0200
  cmp var_6521
  beq loc_b0_9FCD
  jmp sub_b0_9F47
loc_b0_9FCD:
  lda var_0201
  cmp #$8D
  bne loc_b0_9FDA
  jsr sub_b0_A6C3
  jmp loc_b0_9F38
loc_b0_9FDA:
  jmp loc_b0_A61A
loc_b0_9FDD:
  asl var_64CE
  ldy var_64CE
  jsr sub_b0_A5B8
  bcc loc_b0_9FF4
  lda #$02
  and data_b0_A843,y
  beq loc_b0_9FF4
  lda #$0F
  jmp loc_b0_A628
loc_b0_9FF4:
  cpy #$06
  bne loc_b0_9FFA
  sty var_0033
loc_b0_9FFA:
  lda #$20
  and data_b0_A843,y
  beq loc_b0_A06A
  jsr sub_b0_A05F
  php
loc_b0_A005:
  jsr sub_b0_A17F
  beq loc_b0_A030
  cmp #$BF
  beq loc_b0_A023
  cmp #$AA
  beq loc_b0_A023
  asl a
  bcc loc_b0_A01A
  bmi loc_b0_A01A
  jmp loc_b0_9FC2
loc_b0_A01A:
  ror a
  jmp loc_b0_A023
loc_b0_A01E:
  jsr sub_b0_A16E
  beq loc_b0_A030
loc_b0_A023:
  sta var_64E4,y
  iny
  cpy #$3C
  bcc loc_b0_A01E
loc_b0_A02B:
  jsr sub_b0_A16E
  bne loc_b0_A02B
loc_b0_A030:
  plp
  bne loc_b0_A042
  ldy var_64CE
  lda #$10
  and data_b0_A843,y
  beq loc_b0_A049
  ldy #$1E
  php
  bne loc_b0_A005
loc_b0_A042:
  lda var_6502
  cmp #$A0
  beq loc_b0_A05C
loc_b0_A049:
  lda var_64E4
  cmp #$A0
  bne loc_b0_A09B
  ldy var_64CE
  lda #$C0
  and data_b0_A843,y
  beq loc_b0_A05C
  bpl loc_b0_A09B
loc_b0_A05C:
  jmp loc_b0_9FC2

sub_b0_A05F:
  ldy #$3C
  lda #$A0
loc_b0_A063:
  sta var_64E3,y
  dey
  bne loc_b0_A063
  rts
loc_b0_A06A:
  sta var_64E4
  lda #$0C
  and data_b0_A843,y
  beq loc_b0_A09B
  jsr sub_b0_A194
  bcs loc_b0_A098
  tay
  bne loc_b0_A093
  cpx #$11
  bcs loc_b0_A093
  ldy var_64CE
  lda #$08
  and data_b0_A843,y
  beq loc_b0_A090
  cpx #$08
  bcs loc_b0_A05C
  bcc loc_b0_A09B
loc_b0_A090:
  txa
  bne loc_b0_A09B
loc_b0_A093:
  lda #$02
  jmp loc_b0_A628
loc_b0_A098:
  jmp loc_b0_A61A
loc_b0_A09B:
  lda #$00
  sta var_64D4
  sta var_64E3
  sta var_64D5
  sta var_64DB
  sta var_64DC
  jsr sub_b0_B3DD
  lda var_64CC
loc_b0_A0B2:
  jsr sub_b0_A17F
  bne loc_b0_A0D6
  cmp #$8D
  bne loc_b0_A0B2
  ldx var_64CE
  lda var_64D4
  ora data_b0_A844,x
  eor data_b0_A844,x
  bne loc_b0_A05C
  ldx var_64D2
  beq loc_b0_A147
  sta var_64D2
  stx var_64CC
  bne loc_b0_A0B2
loc_b0_A0D6:
  ldx #$0A
loc_b0_A0D8:
  cmp data_b0_A88C,x
  beq loc_b0_A0E2
  dex
  bne loc_b0_A0D8
loc_b0_A0E0:
  beq loc_b0_A098
loc_b0_A0E2:
  lda data_b0_A896,x
  bmi loc_b0_A131
  ora var_64D4
  sta var_64D4
  dex
  stx var_64D3
  jsr sub_b0_A194
  bcs loc_b0_A098
  lda var_64D3
  asl a
  asl a
  tay
  lda var_0045
  bne loc_b0_A109
  lda var_0044
  cmp data_b0_A8A1,y
  bcc loc_b0_A093
  lda var_0045
loc_b0_A109:
  cmp data_b0_A8A4,y
  bcc loc_b0_A11C
loc_b0_A10E:
  beq loc_b0_A113
  jmp loc_b0_A093
loc_b0_A113:
  lda var_0044
  cmp data_b0_A8A3,y
  bcc loc_b0_A11C
  bne loc_b0_A10E
loc_b0_A11C:
  lda var_64D2
  bne loc_b0_A0B2
  tya
  lsr a
  tay
  lda var_0045
  sta var_64D6,y
  lda var_0044
  sta var_64D5,y
loc_b0_A12E:
  jmp loc_b0_A0B2
loc_b0_A131:
  pha
  lda #$80
  ora var_64D4
  sta var_64D4
  pla
  and #$7F
  ora var_64E3
  sta var_64E3
  bne loc_b0_A12E
  beq loc_b0_A0E0
loc_b0_A147:
  jsr sub_b0_A15B
  php
  pha
  lda var_03C4
  cmp #$16
  bne loc_b0_A156
  pla
  plp
  rts
loc_b0_A156:
  pla
  plp
  jmp loc_b0_9F26

sub_b0_A15B:
  jsr sub_b0_A6C3
  jsr sub_b0_A189
  lda var_64CE
  tax
  lda data_b0_9D1F,x
  pha
  lda data_b0_9D1E,x
  pha
  rts

sub_b0_A16E:
  ldx var_64CC
  lda var_0200,x
  cmp #$8D
  beq sub_b0_A17E
  inx
  stx var_64CC
  cmp #$AC

sub_b0_A17E:
  rts

sub_b0_A17F:
  jsr sub_b0_A16E
  beq sub_b0_A17E
  cmp #$A0
  beq sub_b0_A17F
  rts

sub_b0_A189:
  lda #$00
  ldy #$16
loc_b0_A18D:
  sta var_62FF,y
  dey
  bne loc_b0_A18D
  rts

sub_b0_A194:
  lda #$00
  sta var_0044
  sta var_0045
  jsr sub_b0_A17F
  php
  cmp #$A4
  beq loc_b0_A1DE
  plp
  jmp loc_b0_A1A9
loc_b0_A1A6:
  jsr sub_b0_A17F
loc_b0_A1A9:
  bne loc_b0_A1B1
  ldx var_0044
  lda var_0045
  clc
  rts
loc_b0_A1B1:
  sec
  sbc #$B0
  bmi loc_b0_A1D7
  cmp #$0A
  bcs loc_b0_A1D7
  jsr sub_b0_A1D9
  adc var_0044
  tax
  lda #$00
  adc var_0045
  tay
  jsr sub_b0_A1D9
  jsr sub_b0_A1D9
  txa
  adc var_0044
  sta var_0044
  tya
  adc var_0045
  sta var_0045
  bcc loc_b0_A1A6
loc_b0_A1D7:
  sec
  rts

sub_b0_A1D9:
  asl var_0044
  rol var_0045
  rts
loc_b0_A1DE:
  plp
loc_b0_A1DF:
  jsr sub_b0_A17F
  beq loc_b0_A1A9
  sec
  sbc #$B0
  bmi loc_b0_A1D7
  cmp #$0A
  bcc loc_b0_A1F5
  sbc #$07
  bmi loc_b0_A1D7
  cmp #$10
  bcs loc_b0_A1D7
loc_b0_A1F5:
  ldx #$04
loc_b0_A1F7:
  jsr sub_b0_A1D9
  dex
  bne loc_b0_A1F7
  ora var_0044
  sta var_0044
  jmp loc_b0_A1DF
loc_b0_A204:
  lda var_0044
  jmp loc_b31_CEBE
loc_b0_A209:
  lda var_64CD
  ora var_64E3
  sta var_64CD
  rts
loc_b0_A213:
  bit var_64E3
  bvc loc_b0_A21B
  jsr sub_b0_9F6B
loc_b0_A21B:
  lda #$70
  eor var_64E3
  and var_64CD
  sta var_64CD
  rts
loc_b0_A227:
  lda #$05
  jsr sub_b0_A27A
loc_b0_A22C:
  jsr sub_b0_A6CC
  lda #$00
  sta var_64BD
  ldy #$00
  tya
  sta (var_0040),y
  rts
loc_b0_A23A:
  lda #$16
  sta var_64BD
  bne loc_b0_A227
loc_b0_A241:
  lda #$07
  bne loc_b0_A247
loc_b0_A245:
  lda #$08
loc_b0_A247:
  jsr sub_b0_A27A
  jmp sub_b0_A2BA
loc_b0_A24D:
  lda #$0C
  bne loc_b0_A247
loc_b0_A251:
  lda data_b0_9D08
  sta var_6532
  lda data_b0_9D09
  sta var_6533
  lda #$09
  sta var_64D2
  jsr sub_b0_A298
  jmp sub_b0_A2BA
loc_b0_A268:
  jsr sub_b0_A273
loc_b0_A26B:
  jsr sub_b0_A5E6
  bne loc_b0_A26B
  jmp loc_b0_9E3B+2

sub_b0_A273:
  lda #$00
  jmp sub_b0_A391

sub_b0_A278:
  lda #$01

sub_b0_A27A:
  sta var_64D2
  lda var_64DB
  bne loc_b0_A28C
  lda var_64DC
  bne loc_b0_A28C
  lda #$01
  sta var_64DB
loc_b0_A28C:
  lda var_64DB
  sta var_6532
  lda var_64DC
  sta var_6533

sub_b0_A298:
  jsr sub_b0_A2BA
  lda var_0045
  bne loc_b0_A2A2
  jmp loc_b0_A61E
loc_b0_A2A2:
  sta var_0041
  lda var_0044
  sta var_0040
  jsr sub_b0_A6AB
  jsr sub_b0_A6B6
  jsr sub_b0_A682
  lda var_64D2
  sta var_6530
  jmp sub_b0_A602

sub_b0_A2BA:
  lda var_64E4
  cmp #$A0
  beq sub_b0_A2E6
  jsr sub_b0_A6CC
  bcs loc_b0_A300
  jsr sub_b0_A2CC
  jmp sub_b0_A2BA

sub_b0_A2CC:
  jsr sub_b0_A717
  bne loc_b0_A2D6
  lda #$00
  sta var_6522
loc_b0_A2D6:
  ldy #$00
  tya
  sta (var_0040),y
  jsr sub_b0_A6B6
  lda #$02
  sta var_6530
  jmp sub_b0_A602

sub_b0_A2E6:
  jsr sub_b0_A6FA
  bne loc_b0_A2F0
loc_b0_A2EB:
  jsr sub_b0_A702
  beq loc_b0_A300
loc_b0_A2F0:
  jsr sub_b0_A717
  beq loc_b0_A2EB
  jsr sub_b0_A712
  beq loc_b0_A2EB
  jsr sub_b0_A2CC
  jmp sub_b0_A2E6
loc_b0_A300:
  rts
loc_b0_A301:
  lda #$09
  and var_64D4
  cmp #$09
  beq loc_b0_A30D
  jmp loc_b0_9FC2
loc_b0_A30D:
  lda #$04
  jsr sub_b0_A391
  lda var_64E2
  ldy var_64E1
  jsr sub_b0_A39C
  lda var_64DC
  ldy var_64DB
  jsr sub_b0_A39C
  lda var_64E2
  ldy var_64E1
  jmp loc_b0_A3BB

sub_b0_A32D:
  jsr sub_b0_A278
  lda #$7F
  and var_6537
  cmp #$04
  beq loc_b0_A33C
  jmp loc_b0_A626
loc_b0_A33C:
  lda #$04
  jsr sub_b0_A391
  jsr sub_b0_A421
  tax
  lda var_64D4
  and #$01
  bne loc_b0_A352
  stx var_64E1
  sty var_64E2
loc_b0_A352:
  jsr sub_b0_A421
  ldx var_64E1
  ldy var_64E2
  jmp sub_b0_A418
loc_b0_A35E:
  jsr sub_b0_A32D
  jsr sub_b0_A747
  lda var_64E1
  sta var_00FA
  lda var_64E2
  sta var_00FB
  jmp loc_b31_CE5A
loc_b0_A371:
  lda var_00D6
  bpl loc_b0_A378
  jmp loc_b0_A622
loc_b0_A378:
  lda #$02
  jsr sub_b0_A391
  sec
  lda var_00AF
  sbc var_0067
  tay
  lda var_00B0
  sbc var_0068
  jsr sub_b0_A39C
  lda var_0068
  ldy var_0067
  jmp loc_b0_A3BB

sub_b0_A391:
  sta var_6537
  pha
  jsr sub_b0_A278
  pla
  jmp loc_b0_A72C

sub_b0_A39C:
  sty var_6536
  sty var_6538
  sta var_6537
  lda #$04
  sta var_6530
  lda #$01
  sta var_6531
  jsr sub_b0_A602
  lda var_6537
  sta var_6538
  jmp sub_b0_A602
loc_b0_A3BB:
  sty var_6538
  sta var_6539
  lda #$02
  jmp loc_b0_B352
loc_b0_A3C6:
  jsr sub_b0_A602
  jmp sub_b0_A2BA
loc_b0_A3CC:
  jmp loc_b0_A626

sub_b0_A3CF:
  jsr sub_b0_A2E6
  jsr sub_b0_A278
  lda #$23
  and var_6537
  beq loc_b0_A3CC
  sta var_6537
  lda #$02
  jsr sub_b0_A458
  jsr sub_b0_A421
  clc
  adc var_0067
  tax
  tya
  adc var_0068
  cmp var_0074
  bcs loc_b0_A452
  sta var_00B0
  sta var_006A
  stx var_00AF
  stx var_0069
  ldx var_0067
  ldy var_0068
  jsr sub_b0_A418
  jsr sub_b0_A747
  jmp (data_b0_9D72)
  .byte $A9, $01, $20, $58, $A4, $20, $21, $A4, $38, $A5, $4C, $ED, $CF, $64, $AA, $A5
  .byte $4D

sub_b0_A418:
  stx var_6538
  sty var_6539
  jmp loc_b0_A3C6

sub_b0_A421:
  lda data_b0_9D0A
  sta var_6538
  lda data_b0_9D0B
  sta var_6539
  lda #$00
  sta var_6537
  lda #$02
  sta var_6536
  lda #$03
  sta var_6530
  lda #$02
  sta var_6531
  jsr sub_b0_A602
  lda var_64D0
  sta var_6537
  tay
  lda var_64CF
  sta var_6536
  rts
loc_b0_A452:
  jsr sub_b0_A2BA
  jmp loc_b0_A622

sub_b0_A458:
  cmp var_6537
  beq loc_b0_A474
  ldx var_64CE
  stx var_64D1
  lsr a
  beq loc_b0_A466
loc_b0_A466:
  ldx #$1D
loc_b0_A468:
  lda var_64E4,x
  sta var_6502,x
  dex
  bpl loc_b0_A468
  jmp loc_b0_A51D
loc_b0_A474:
  rts
loc_b0_A475:
  lda var_6525
  beq loc_b0_A47D
  sta var_6526
loc_b0_A47D:
  jsr sub_b0_A3CF
loc_b0_A480:
  jsr sub_b0_9F6B
  jsr sub_b0_A747
  jmp (data_b0_9D6A)
  .byte $A5, $4A, $85, $CC, $A5, $4B, $85, $CD, $6C, $68, $9D, $20, $D2, $A3, $20, $6B
  .byte $9F, $20, $47, $A7, $6C, $68, $9D
loc_b0_A4A0:
  jsr sub_b0_A4B6
  lda #$05
  sta var_64C1
  jmp loc_b0_9F26
loc_b0_A4AB:
  jsr sub_b0_A4B6
  lda #$01
  sta var_64C0
  jmp loc_b0_9F26

sub_b0_A4B6:
  jsr sub_b0_A6CC
  bcc loc_b0_A4C1
  jsr sub_b0_A273
  jmp loc_b0_A4C4
loc_b0_A4C1:
  jsr sub_b0_A6B6
loc_b0_A4C4:
  lda var_64D4
  and #$06
  beq loc_b0_A4DE
  ldx #$03
loc_b0_A4CD:
  lda var_64DD,x
  sta var_6532,x
  dex
  bpl loc_b0_A4CD
  lda #$0A
  sta var_6530
  jsr sub_b0_A602
loc_b0_A4DE:
  rts
loc_b0_A4DF:
  lda #$80
  bne loc_b0_A4E5
loc_b0_A4E3:
  lda #$00
loc_b0_A4E5:
  sta var_64BD
  jsr sub_b0_B71C
  bne loc_b0_A509
  lda #$40
  and var_64D4
  beq loc_b0_A4F9
  lda var_64D5
  bne loc_b0_A4FE
loc_b0_A4F9:
  lda #$FE
  sta var_64D5
loc_b0_A4FE:
  lda data_b0_9D0D
  sta var_6531
  lda #$0B
  jsr sub_b0_A27A
loc_b0_A509:
  rts
loc_b0_A50A:
  lda #$01
loc_b0_A50C:
  sta var_64BD
  lda #$06
  jsr sub_b0_A27A
  lda var_6534
  sta var_64D5
  jmp loc_b0_A22C
loc_b0_A51D:
  jmp sub_b0_9D74
loc_b0_A520:
  jsr sub_b0_A273
  lda var_64BE
  sta var_6523
  lda var_64BF
  sta var_6524
  lda var_64E4
  sta var_6522
  bne loc_b0_A545
loc_b0_A537:
  jsr sub_b0_A6CC
  bcc loc_b0_A542
  jsr sub_b0_A273
  jmp loc_b0_A545
loc_b0_A542:
  jsr sub_b0_A6B6
loc_b0_A545:
  lda var_64D4
  and #$04
  beq loc_b0_A567
loc_b0_A54C:
  lda var_64DD
  bne loc_b0_A559
  ldx var_64DE
  beq loc_b0_A567
  dec var_64DE
loc_b0_A559:
  dec var_64DD
loc_b0_A55C:
  jsr sub_b0_A5E6
  beq loc_b0_A599
  cmp #$8D
  bne loc_b0_A55C
  beq loc_b0_A54C
loc_b0_A567:
  rts

sub_b0_A568:
  jsr sub_b0_A5B8
  bcs loc_b0_A5D3
  lda var_64CB
  sta var_6538
  lda #$04
  sta var_6530
  lda #$01
  sta var_6531
  jmp sub_b0_A602
loc_b0_A580:
  jsr sub_b0_A5B8
  bcs loc_b0_A5D3
  lda #$06
loc_b0_A587:
  sta var_64C1
  jsr sub_b0_A5E6
  bne loc_b0_A59E
  jsr sub_b0_A2CC
  lda #$03
  cmp var_64C1
  beq loc_b0_A567
loc_b0_A599:
  lda #$05
  jmp loc_b0_A628
loc_b0_A59E:
  cmp #$E0
  bcc loc_b0_A5A4
  and #$7F
loc_b0_A5A4:
  sta var_64CB
  ldx var_64C9
  beq loc_b0_A5B5
  dex
  lda var_0200,x
  ora #$80
  sta var_0200,x
loc_b0_A5B5:
  jmp sub_b0_9F56

sub_b0_A5B8:
  pha
  lda var_6525
  beq loc_b0_A5CC
  ldx var_0076
  inx
  beq sub_b0_A5D0
  ldx var_0033
  cpx #$DD
  beq sub_b0_A5D0
loc_b0_A5C9:
  pla
  clc
  rts
loc_b0_A5CC:
  lda var_00D9
  bmi loc_b0_A5C9

sub_b0_A5D0:
  pla
  sec
  rts
loc_b0_A5D3:
  jsr sub_b0_A2CC
  jsr sub_b0_A6C3
  jmp sub_b0_9F56

sub_b0_A5DC:
  jsr sub_b0_A5F7
  jsr sub_b0_A6B6
  lda #$03
  bne loc_b0_A587

sub_b0_A5E6:
  lda #$03
  sta var_6530
  lda #$01
  sta var_6531
  jsr sub_b0_A602
  lda var_6538
  rts

sub_b0_A5F7:
  lda var_6524
  sta var_0041
  lda var_6523
  sta var_0040
  rts

sub_b0_A602:
  jsr sub_b0_A9E9
  bcc loc_b0_A619
  lda var_653A
  cmp #$05
  beq loc_b0_A611
  jmp loc_b0_B32A
loc_b0_A611:
  jmp loc_b0_B35E
loc_b0_A614:
  ldx #$00
  stx var_6538
loc_b0_A619:
  rts
loc_b0_A61A:
  lda #$0B
  bne loc_b0_A628
loc_b0_A61E:
  lda #$0C
  bne loc_b0_A628
loc_b0_A622:
  lda #$0E
  bne loc_b0_A628
loc_b0_A626:
  lda #$0D
loc_b0_A628:
  sta var_64CB
  sta var_67E8
  jsr sub_b0_B3E7
  lda var_03C4
  cmp #$16
  bne loc_b0_A640
  jsr sub_b0_A747
  ldx var_67E9
  txs
  rts
loc_b0_A640:
  lda var_6525
  beq loc_b0_A649
  lda var_00D8
  bmi loc_b0_A657
loc_b0_A649:
  ldx #$00
  jsr sub_b0_A66A
  ldx var_64CB
  jsr sub_b0_A66A
  jsr sub_b0_9F6B
loc_b0_A657:
  jsr sub_b0_A747
  jsr sub_b0_A5B8
  ldx var_64CB
  lda #$03
  bcs loc_b0_A667
  jmp (data_b0_9D6C)
loc_b0_A667:
  jmp (data_b0_9D70)

sub_b0_A66A:
  lda data_b0_A98B,x
  tax
loc_b0_A66E:
  stx var_64D2
  lda data_b0_A8BD,x
  pha
  ora #$80
  jsr sub_b0_9F68
  ldx var_64D2
  inx
  pla
  bpl loc_b0_A66E
  rts

sub_b0_A682:
  lda var_64D5
  sta var_6534
  lda var_64D7
  sta var_6535
  lda var_64D9
  sta var_6536
  lda data_b0_9D06
  sta var_6538
  lda data_b0_9D07
  sta var_6539
  lda var_0040
  sta var_64BE
  lda var_0041
  sta var_64BF
  rts

sub_b0_A6AB:
  ldy #$1D
loc_b0_A6AD:
  lda var_64E4,y
  sta (var_0040),y
  dey
  bpl loc_b0_A6AD
  rts

sub_b0_A6B6:
  ldy #$1E
loc_b0_A6B8:
  lda (var_0040),y
  sta var_62EE,y
  iny
  cpy #$26
  bne loc_b0_A6B8
  rts

sub_b0_A6C3:
  ldy #$00
  sty var_64C0
  sty var_64C1
  rts

sub_b0_A6CC:
  lda #$00
  sta var_0045
  jsr sub_b0_A6FA
  jmp loc_b0_A6DB
loc_b0_A6D6:
  jsr sub_b0_A702
  beq loc_b0_A6F8
loc_b0_A6DB:
  jsr sub_b0_A712
  bne loc_b0_A6EA
  lda var_0040
  sta var_0044
  lda var_0041
  sta var_0045
  bne loc_b0_A6D6
loc_b0_A6EA:
  ldy #$1D
loc_b0_A6EC:
  lda (var_0040),y
  cmp var_64E4,y
  bne loc_b0_A6D6
  dey
  bpl loc_b0_A6EC
  clc
  rts
loc_b0_A6F8:
  sec
  rts

sub_b0_A6FA:
  lda data_b0_9D00
  ldx data_b0_9D01
  bne loc_b0_A70C

sub_b0_A702:
  ldy #$25
  lda (var_0040),y
  beq loc_b0_A711
  tax
  dey
  lda (var_0040),y
loc_b0_A70C:
  stx var_0041
  sta var_0040
  txa
loc_b0_A711:
  rts

sub_b0_A712:
  ldy #$00
  lda (var_0040),y
  rts

sub_b0_A717:
  lda var_6522
  beq loc_b0_A72A
  lda var_6523
  cmp var_0040
  bne loc_b0_A72B
  lda var_6524
  cmp var_0041
  beq loc_b0_A72B
loc_b0_A72A:
  dex
loc_b0_A72B:
  rts
loc_b0_A72C:
  eor var_6537
  beq loc_b0_A73B
  and #$7F
  beq loc_b0_A73B
  jsr sub_b0_A2BA
  jmp loc_b0_A626
loc_b0_A73B:
  rts
  .byte $68, $85, $74, $85, $70, $68, $85, $73, $85, $6F, $60

sub_b0_A747:
  lda var_0039
  cmp data_b0_9D03
  beq loc_b0_A760
  sta var_64C5
  lda var_0038
  sta var_64C4
  lda data_b0_9D02
  sta var_0038
  lda data_b0_9D03
  sta var_0039
loc_b0_A760:
  lda var_0037
  cmp data_b0_9D05
  beq loc_b0_A779
  sta var_64C3
  lda var_0036
  sta var_64C2
  lda data_b0_9D04
  sta var_0036
  lda data_b0_9D05
  sta var_0037
loc_b0_A779:
  rts
data_b0_A77A:
  .byte $49, $4E, $49, $D4, $4C, $4F, $41, $C4
  .byte $53, $41, $56, $C5, $52, $55, $CE, $43
  .byte $48, $41, $49, $CE, $44, $45, $4C, $45
  .byte $54, $C5, $4C, $4F, $43, $CB, $55, $4E
  .byte $4C, $4F, $43, $CB, $43, $4C, $4F, $53
  .byte $C5, $52, $45, $41, $C4, $45, $58, $45
  .byte $C3, $57, $52, $49, $54, $C5, $50, $4F
  .byte $53, $49, $54, $49, $4F, $CE, $4F, $50
  .byte $45, $CE, $41, $50, $50, $45, $4E, $C4
  .byte $52, $45, $4E, $41, $4D, $C5, $43, $41
  .byte $54, $41, $4C, $4F, $C7, $4D, $4F, $CE
  .byte $4E, $4F, $4D, $4F, $CE, $50, $52, $A3
  .byte $42, $53, $41, $56, $C5, $42, $4C, $4F
  .byte $41, $C4, $42, $52, $55, $CE, $56, $45
  .byte $52, $49, $46, $D9, $4C, $4D, $54, $59
  .byte $50, $C5, $4C, $4D, $44, $49, $D2, $4C
  .byte $4D, $52, $55, $CE, $56, $45, $52, $53
  .byte $49, $4F, $CE, $4C, $4D, $44, $45, $4C
  .byte $44, $49, $53, $CB, $4C, $4D, $52, $45
  .byte $41, $C4, $4C, $4D, $57, $52, $49, $54
  .byte $C5, $4C, $4D, $44, $55, $4D, $D0, $4C
  .byte $4D, $55, $4E, $44, $49, $D2, $4C, $4D
  .byte $48, $45, $D8, $4C, $4D, $44, $45, $C3
  .byte $4C, $4D, $55, $4E, $44, $45, $CC, $4C
  .byte $4D, $44, $49, $53, $4B, $4D, $41, $D0
  .byte $00
data_b0_A843:
  .byte $40
data_b0_A844:
  .byte $70, $A0, $70, $A1, $70, $A0, $70, $20, $70, $20, $70, $20, $70, $20, $70, $60
  .byte $00, $22, $06, $20, $74, $22, $06, $22, $04, $23, $78, $22, $70, $30, $70, $40
  .byte $70, $40, $80, $40, $80, $08, $00, $21, $79, $20, $71, $20, $71, $20, $70, $20
  .byte $70, $20, $71, $20, $71, $40, $00, $40, $70, $00, $77, $00, $77, $00, $76, $40
  .byte $70, $00, $01, $00, $01, $20, $70, $00
data_b0_A88C:
  .byte $20, $D6, $C4, $D3, $CC, $D2, $C2, $C1, $C3, $C9
data_b0_A896:
  .byte $CF, $40, $20, $10, $08, $04, $02, $01, $C0, $A0, $90
data_b0_A8A1:
  .byte $00, $00
data_b0_A8A3:
  .byte $FE
data_b0_A8A4:
  .byte $00, $01, $00, $02, $00, $01, $00, $01, $00, $01, $00, $FF, $FF, $00, $00, $FF
  .byte $FF, $00, $00, $FF, $FF, $00, $00, $FF, $FF
data_b0_A8BD:
  .byte $0D, $07, $8D, $4C, $41, $4E, $47, $55, $41, $47, $45, $20, $4E, $4F, $54, $20
  .byte $41, $56, $41, $49, $4C, $41, $42, $4C, $C5, $52, $41, $4E, $47, $45, $20, $45
  .byte $52, $52, $4F, $D2, $57, $52, $49, $54, $45, $20, $50, $52, $4F, $54, $45, $43
  .byte $54, $45, $C4, $45, $4E, $44, $20, $4F, $46, $20, $44, $41, $54, $C1, $46, $49
  .byte $4C, $45, $20, $4E, $4F, $54, $20, $46, $4F, $55, $4E, $C4, $56, $4F, $4C, $55
  .byte $4D, $45, $20, $4D, $49, $53, $4D, $41, $54, $43, $C8, $49, $2F, $4F, $20, $45
  .byte $52, $52, $4F, $D2, $44, $49, $53, $4B, $20, $46, $55, $4C, $CC, $46, $49, $4C
  .byte $45, $20, $4C, $4F, $43, $4B, $45, $C4, $53, $59, $4E, $54, $41, $58, $20, $45
  .byte $52, $52, $4F, $D2, $4E, $4F, $20, $42, $55, $46, $46, $45, $52, $53, $20, $41
  .byte $56, $41, $49, $4C, $41, $42, $4C, $C5, $46, $49, $4C, $45, $20, $54, $59, $50
  .byte $45, $20, $4D, $49, $53, $4D, $41, $54, $43, $C8, $50, $52, $4F, $47, $52, $41
  .byte $4D, $20, $54, $4F, $4F, $20, $4C, $41, $52, $47, $C5, $4E, $4F, $54, $20, $44
  .byte $49, $52, $45, $43, $54, $20, $43, $4F, $4D, $4D, $41, $4E, $C4, $8D
data_b0_A98B:
  .byte $00, $03, $19, $19, $24, $33, $3E, $4C, $5B, $64, $6D, $78, $84, $98, $AA, $BB
  .byte $D0, $C5, $C3, $CC, $CD, $D3, $CF, $C6, $D4, $56, $64
data_b0_A9A6:
  .byte $00
data_b0_A9A7:
  .byte $61
data_b0_A9A8:
  .byte $00
data_b0_A9A9:
  .byte $62, $00, $C0
data_b0_A9AC:
data_b0_A9AD = * + 1
  .word loc_b0_B303 - 1, loc_b0_AA05 - 1, loc_b0_AAE9 - 1, loc_b0_AB3B - 1
  .word loc_b0_AB53 - 1, loc_b0_AC12 - 1, loc_b0_AC8C - 1, loc_b0_ABD6 - 1
  .word loc_b0_ABDD - 1, loc_b0_AB1D - 1, loc_b0_ABF9 - 1, loc_b0_ADE5 - 1
  .word loc_b0_ABFF - 1, loc_b0_B303 - 1
data_b0_A9C8:
data_b0_A9C9 = * + 1
  .word loc_b0_B303 - 1, loc_b0_AB6D - 1, loc_b0_AB79 - 1, loc_b0_AB6A - 1
  .word loc_b0_AB76 - 1, loc_b0_B303 - 1
data_b0_A9D4:
  .byte $03
data_b0_A9D5:
  .byte $B3, $A2, $AB, $AE, $AB, $9F, $AB, $AB, $AB, $02, $B3, $E0, $00, $F0, $02, $A2
  .byte $02, $8E, $CE, $64

sub_b0_A9E9:
  tsx
  stx var_64A4
  jsr sub_b0_ADBA
  lda var_6530
  cmp #$0D
  bcs loc_b0_AA02
  asl a
  tax
  lda data_b0_A9AD,x
  pha
  lda data_b0_A9AC,x
  pha
  rts
loc_b0_AA02:
  jmp loc_b0_B2E7
loc_b0_AA05:
  jsr sub_b0_AA0B
  jmp loc_b0_B303

sub_b0_AA0B:
  jsr sub_b0_AABF
  lda #$01
  sta var_6558
  ldx var_6533
  lda var_6532
  bne loc_b0_AA20
  cpx #$00
  bne loc_b0_AA20
  inx
loc_b0_AA20:
  sta var_655D
  stx var_655E
  jsr sub_b0_B11D
  bcc loc_b0_AA89
  stx var_64A5
  ldx var_64CE
  lda data_b0_A843,x
  ldx var_64A5
  lsr a
  bcs loc_b0_AA47
  lda var_64C0
  cmp #$C0
  bne loc_b0_AA44
  jmp loc_b0_B2E3
loc_b0_AA44:
  jmp loc_b0_B2F7
loc_b0_AA47:
  lda #$00
  sta var_622D,x
  lda #$01
  sta var_622C,x
  stx var_64A5
  jsr sub_b0_B1B8
  ldx var_64A5
  sta var_620C,x
  sta var_6547
  sta var_6549
  lda var_6566
  sta var_620B,x
  sta var_6546
  sta var_6548
  lda var_6537
  sta var_620D,x
  jsr sub_b0_AF8B
  jsr sub_b0_AE60
  jsr sub_b0_B3C2
  jsr sub_b0_AE8E
  ldx var_64A5
  lda #$06
  sta var_653A
loc_b0_AA89:
  lda var_620B,x
  sta var_6546
  lda var_620C,x
  sta var_6547
  lda var_620D,x
  sta var_6537
  sta var_656B
  lda var_622C,x
  sta var_6563
  lda var_622D,x
  sta var_6564
  stx var_654E
  lda #$FF
  sta var_6555
  sta var_6556
  lda var_6127
  sta var_654F
  clc
  jmp sub_b0_AEB2

sub_b0_AABF:
  lda #$00
  tax
loc_b0_AAC2:
  sta var_6546,x
  inx
  cpx #$2D
  bne loc_b0_AAC2
  lda var_6534
  eor #$FF
  sta var_656E
  lda var_6535
  sta var_656D
  lda var_6536
  asl a
  asl a
  asl a
  asl a
  tax
  stx var_656C
  lda #$11
  sta var_656F
  rts
loc_b0_AAE9:
  jsr sub_b0_AE71
  jsr sub_b0_AE88
  jsr sub_b0_B237
  lda #$02
  and var_654A
  beq loc_b0_AB1A
  jsr sub_b0_AF4B
  lda #$00
  clc
loc_b0_AAFF:
  jsr sub_b0_AF65
  sec
  dec var_654D
  bne loc_b0_AAFF
  ldx var_654E
  lda var_6563
  sta var_622C,x
  lda var_6564
  sta var_622D,x
  jsr sub_b0_AF8B
loc_b0_AB1A:
  jmp loc_b0_B303
loc_b0_AB1D:
  jsr sub_b0_AA0B
  lda var_656B
  bmi loc_b0_AB50
  lda var_6532
  sta var_0042
  lda var_6533
  sta var_0043
  ldx var_64A5
  jsr sub_b0_B18D
  jsr sub_b0_AF8B
  jmp loc_b0_B303
loc_b0_AB3B:
  lda var_6531
  cmp #$05
  bcs loc_b0_AB4D
  asl a
  tax
  lda data_b0_A9C9,x
  pha
  lda data_b0_A9C8,x
  pha
  rts
loc_b0_AB4D:
  jmp loc_b0_B2EB
loc_b0_AB50:
  jmp loc_b0_B2FF
loc_b0_AB53:
  lda var_656B
  bmi loc_b0_AB50
  lda var_6531
  cmp #$05
  bcs loc_b0_AB4D
  asl a
  tax
  lda data_b0_A9D5,x
  pha
  lda data_b0_A9D4,x
  pha
  rts
loc_b0_AB6A:
  jsr sub_b0_B284
loc_b0_AB6D:
  jsr sub_b0_AB8D
  sta var_6538
  jmp loc_b0_B303
loc_b0_AB76:
  jsr sub_b0_B284
loc_b0_AB79:
  jsr sub_b0_B109
  jsr sub_b0_AB8D
  pha
  jsr sub_b0_B0F6
  ldy #$00
  pla
  sec
  jsr var_6DB0
  jmp loc_b0_AB79

sub_b0_AB8D:
  jsr sub_b0_B00A
  bcs loc_b0_AB9D
  lda (var_0042),y
  pha
  jsr sub_b0_B0AF
  jsr sub_b0_B0E8
  pla
  rts
loc_b0_AB9D:
  jmp loc_b0_B2F3
  .byte $20, $84, $B2, $AD, $38, $65, $20, $C1, $AB, $4C, $03, $B3, $20, $84, $B2, $20
  .byte $F6, $B0, $A0, $00, $18, $20, $B0, $6D, $20, $C1, $AB, $20, $09, $B1, $4C, $AF
  .byte $AB, $48, $20, $0A, $B0, $68, $91, $42, $A9, $40, $0D, $4A, $65, $8D, $4A, $65
  .byte $20, $AF, $B0, $4C, $E8, $B0
loc_b0_ABD6:
  lda #$80
  sta var_64A7
  bne loc_b0_ABE2
loc_b0_ABDD:
  lda #$00
  sta var_64A7
loc_b0_ABE2:
  jsr sub_b0_AA0B
  ldx var_64A5
  lda var_620D,x
  and #$7F
  ora var_64A7
  sta var_620D,x
  jsr sub_b0_AF8B
loc_b0_ABF6:
  jmp loc_b0_B303
loc_b0_ABF9:
  jsr sub_b0_B284
  jmp loc_b0_B303
loc_b0_ABFF:
  jsr sub_b0_AA0B
loc_b0_AC02:
  jsr sub_b0_B00A
  bcs loc_b0_ABF6
  inc var_6559
  bne loc_b0_AC02
  inc var_655A
  jmp loc_b0_AC02
loc_b0_AC12:
  jsr sub_b0_AA0B
  ldx var_64A5
  lda var_620D,x
  bpl loc_b0_AC20
  jmp loc_b0_B2FF
loc_b0_AC20:
  ldx var_64A5
  lda var_620B,x
  sta var_6546
  pha
  lda var_64BD
  cmp #$16
  bne loc_b0_AC35
  pla
  jmp loc_b0_AC3E
loc_b0_AC35:
  pla
  sta var_622B,x
  lda #$FF
  sta var_620B,x
loc_b0_AC3E:
  ldy var_620C,x
  sty var_6547
  jsr sub_b0_AF8B
  clc
loc_b0_AC48:
  jsr sub_b0_AEB2
  bcs loc_b0_AC77
  jsr sub_b0_AE60
  ldy #$0C
loc_b0_AC52:
  sty var_64A5
  lda (var_0042),y
  bmi loc_b0_AC64
  beq loc_b0_AC64
  pha
  iny
  lda (var_0042),y
  tay
  pla
  jsr sub_b0_AC7D
loc_b0_AC64:
  ldy var_64A5
  iny
  iny
  bne loc_b0_AC52
  lda var_6548
  ldy var_6549
  jsr sub_b0_AC7D
  sec
  bcs loc_b0_AC48
loc_b0_AC77:
  jsr sub_b0_AF4F
  jmp loc_b0_B303

sub_b0_AC7D:
  sec
  jsr sub_b0_B251
  lda #$00
  ldx #$05
loc_b0_AC85:
  sta var_6565,x
  dex
  bpl loc_b0_AC85
  rts
loc_b0_AC8C:
  jsr sub_b0_AABF
  lda #$00
  sta var_67EF
  lda #$FF
  sta var_656E
  jsr sub_b0_AF4B
  lda #$0A
  sta var_64A6
  jsr sub_b0_AD78
  jsr sub_b0_AD78
  ldx #$00
loc_b0_ACA9:
  lda data_b0_B523,x
  jsr monitor_cout
  inx
  cpx #$1C
  bne loc_b0_ACA9
  ldx #$FF
  stx var_0045
  lda var_6464
  sta var_0044
  jsr sub_b0_AD8B
  jsr sub_b0_AD78
  jsr sub_b0_AD78
  clc
loc_b0_ACC7:
  jsr sub_b0_AF65
  bcc loc_b0_ACCF
  jmp loc_b0_AD72
loc_b0_ACCF:
  ldx #$00
loc_b0_ACD1:
  stx var_64A5
  lda var_620B,x
  bne loc_b0_ACDC
  jmp loc_b0_AD72
loc_b0_ACDC:
  php
  lda var_64BD
  bne loc_b0_ACE8
  plp
  bpl loc_b0_AD0D
  jmp loc_b0_ACEB
loc_b0_ACE8:
  plp
  bmi loc_b0_AD67
loc_b0_ACEB:
  lda var_64BD
  bpl loc_b0_AD1A
  txa
  pha
  tya
  pha
  inx
  inx
  inx
  ldy #$00
loc_b0_ACF9:
  lda var_64E4,y
  cmp #$BF
  beq loc_b0_AD10
  cmp #$AA
  beq loc_b0_AD16
  cmp var_620B,x
  beq loc_b0_AD10
  pla
  tay
  pla
  tax
loc_b0_AD0D:
  jmp loc_b0_AD67
loc_b0_AD10:
  inx
  iny
  cpy #$1E
  bne loc_b0_ACF9
loc_b0_AD16:
  pla
  tay
  pla
  tax
loc_b0_AD1A:
  inc var_67EF
  ldy #$A0
  lda var_620D,x
  bpl loc_b0_AD26
  ldy #$AA
loc_b0_AD26:
  tya
  jsr monitor_cout
  lda var_620D,x
  and #$7F
  ldy #$07
  asl a
loc_b0_AD32:
  asl a
  bcs loc_b0_AD38
  dey
  bne loc_b0_AD32
loc_b0_AD38:
  lda data_b0_B322,y
  jsr monitor_cout
  lda #$A0
  jsr monitor_cout
  lda var_622C,x
  sta var_0044
  lda var_622D,x
  sta var_0045
  jsr sub_b0_AD8B
  lda #$A0
  jsr monitor_cout
  inx
  inx
  inx
  ldy #$14
loc_b0_AD5A:
  lda var_620B,x
  jsr monitor_cout
  inx
  dey
  bpl loc_b0_AD5A
  jsr sub_b0_AD78
loc_b0_AD67:
  jsr sub_b0_B1A1
  bcc loc_b0_AD6F
  jmp loc_b0_ACC7
loc_b0_AD6F:
  jmp loc_b0_ACD1
loc_b0_AD72:
  jsr sub_b0_B6BD
  jmp loc_b0_B303

sub_b0_AD78:
  lda #$8D
  jsr monitor_cout
  dec var_64A6
  bne loc_b0_AD8A
  jsr monitor_rdkey
  lda #$15
  sta var_64A6
loc_b0_AD8A:
  rts

sub_b0_AD8B:
  ldy #$02
loc_b0_AD8D:
  lda #$00
  pha
  lda data_b0_8698
  eor #$EE
  bne loc_b0_ADB6
loc_b0_AD97:
  lda var_0044
  cmp data_b0_B31F,y
  bcc loc_b0_ADB0
  sbc data_b0_B31F,y
  sta var_0044
  lda var_0045
  sbc #$00
  sta var_0045
  pla
  adc #$00
  pha
  jmp loc_b0_AD97
loc_b0_ADB0:
  pla
  ora #$B0
  jsr monitor_cout
loc_b0_ADB6:
  dey
  bpl loc_b0_AD8D
  rts

sub_b0_ADBA:
  jsr sub_b0_AE5C
  ldy #$00
  sty var_653A
loc_b0_ADC2:
  lda data_b0_B4F6
  eor #$CC
  bne loc_b0_ADED
  lda (var_0042),y
  sta var_6546,y
  iny
  cpy #$2D
  bne loc_b0_ADC2
  clc
  rts

sub_b0_ADD5:
  jsr sub_b0_AE5C
  ldy #$00
loc_b0_ADDA:
  lda var_6546,y
  sta (var_0042),y
  iny
  cpy #$2D
  bne loc_b0_ADDA
  rts
loc_b0_ADE5:
  jsr sub_b0_AABF
  lda var_64BD
  bne loc_b0_ADF2
loc_b0_ADED:
  lda #$04
  jsr sub_b0_AFAC
loc_b0_ADF2:
  lda var_656E
  eor #$FF
  sta var_6106
  lda #$11
  sta var_6130
  jsr sub_b0_B486
  ldx #$38
  lda #$00
loc_b0_AE06:
  sta var_6100,x
  inx
  bne loc_b0_AE06
  ldx #$04
loc_b0_AE0E:
  cpx #$A0
  beq loc_b0_AE26
  ldy #$03
loc_b0_AE14:
  lda data_b0_B31B,y
  sta var_6138,x
  inx
  dey
  bpl loc_b0_AE14
  cpx #$44
  bne loc_b0_AE0E
  ldx #$48
  bne loc_b0_AE0E
loc_b0_AE26:
  jsr sub_b0_AF4F
  ldx #$00
  txa
loc_b0_AE2C:
  sta var_6200,x
  inx
  bne loc_b0_AE2C
  jsr sub_b0_AF99
  lda #$11
  ldy #$0C
  sta var_645A
loc_b0_AE3C:
  sta var_6201
loc_b0_AE3F:
  sty var_6202
  iny
  sty var_645B
  lda #$02
  jsr sub_b0_AFAC
  ldy var_6202
  dey
  bmi loc_b0_AE56
  bne loc_b0_AE3F
  tya
  beq loc_b0_AE3C
loc_b0_AE56:
  jsr sub_b0_B3AE
  jmp loc_b0_B303

sub_b0_AE5C:
  ldx #$00
  beq loc_b0_AE66

sub_b0_AE60:
  ldx #$02
  bne loc_b0_AE66

sub_b0_AE64:
  ldx #$04
loc_b0_AE66:
  lda var_653C,x
  sta var_0042
  lda var_653D,x
  sta var_0043
  rts

sub_b0_AE71:
  bit var_654A
  bvs loc_b0_AE77
  rts
loc_b0_AE77:
  jsr sub_b0_AF38
  lda #$02
  jsr sub_b0_AFA6
  lda #$BF
  and var_654A
  sta var_654A
  rts

sub_b0_AE88:
  lda var_654A
  bmi sub_b0_AE8E
  rts

sub_b0_AE8E:
  jsr sub_b0_AE9F
  lda #$02
  jsr sub_b0_AFA6
  lda #$7F
  and var_654A
  sta var_654A
  rts

sub_b0_AE9F:
  lda var_653E
  sta var_645E
  lda var_653F
  sta var_645F
  ldx var_6548
  ldy var_6549
  rts

sub_b0_AEB2:
  php
  jsr sub_b0_AE88
  jsr sub_b0_AE9F
  jsr sub_b0_AE60
  plp
  bcs loc_b0_AEC8
  ldx var_6546
  ldy var_6547
  jmp loc_b0_AF09
loc_b0_AEC8:
  ldy #$01
  lda (var_0042),y
  beq loc_b0_AED6
  tax
  iny
  lda (var_0042),y
  tay
  jmp loc_b0_AF09
loc_b0_AED6:
  lda var_6530
  cmp #$04
  beq loc_b0_AEDF
  sec
  rts
loc_b0_AEDF:
  jsr sub_b0_B1B8
  ldy #$02
  sta (var_0042),y
  pha
  dey
  lda var_6566
  sta (var_0042),y
  pha
  jsr sub_b0_AE8E
  jsr sub_b0_B3C2
  ldy #$05
  lda var_6553
  sta (var_0042),y
  iny
  lda var_6554
  sta (var_0042),y
  pla
  tax
  pla
  tay
  lda #$02
  bne loc_b0_AF0B
loc_b0_AF09:
  lda #$01
loc_b0_AF0B:
  stx var_6548
  sty var_6549
  jsr sub_b0_AFA6
  ldy #$05
  lda (var_0042),y
  sta var_6551
  clc
  adc var_654F
  sta var_6553
  iny
  lda (var_0042),y
  sta var_6552
  adc var_6550
  sta var_6554
  clc
  rts

sub_b0_AF30:
  jsr sub_b0_AF38
  lda #$01
  jmp sub_b0_AFA6

sub_b0_AF38:
  ldy var_6540
  lda var_6541
  sty var_645E
  sta var_645F
  ldx var_654B
  ldy var_654C
  rts

sub_b0_AF4B:
  lda #$01
  bne loc_b0_AF51

sub_b0_AF4F:
  lda #$02
loc_b0_AF51:
  ldy data_b0_A9A6
  sty var_645E
  ldy data_b0_A9A7
  sty var_645F
  ldx var_656F
  ldy #$00
  jmp sub_b0_AFA6

sub_b0_AF65:
  php
  jsr sub_b0_AF99
  plp
  bcs loc_b0_AF74
  ldy var_6102
  ldx var_6101
  bne loc_b0_AF7E
loc_b0_AF74:
  ldx var_6201
  bne loc_b0_AF7B
  sec
  rts
loc_b0_AF7B:
  ldy var_6202
loc_b0_AF7E:
  stx var_64A0
  sty var_64A1
  lda #$01
  jsr sub_b0_AFA6
  clc
  rts

sub_b0_AF8B:
  jsr sub_b0_AF99
  ldx var_64A0
  ldy var_64A1
  lda #$02
  jmp sub_b0_AFA6

sub_b0_AF99:
  lda data_b0_A9A8
  sta var_645E
  lda data_b0_A9A9
  sta var_645F
  rts

sub_b0_AFA6:
  stx var_645A
  sty var_645B

sub_b0_AFAC:
  sta var_6462
  cmp #$02
  bne loc_b0_AFB9
  ora var_654A
  sta var_654A
loc_b0_AFB9:
  lda var_656E
  eor #$FF
  sta var_6459
  lda var_656C
  sta var_6457
  lda var_656D
  sta var_6458
  lda var_6557
  sta var_6460
  lda var_6558
  sta var_6461
  lda #$01
  sta var_6456
  ldy #$56
  nop
  lda #$64
  nop
  jsr sub_b0_B3A1
  lda var_6464
  sta var_6534
  lda #$FF
  sta var_6459
  bcs loc_b0_AFF5
  rts
loc_b0_AFF5:
  lda var_6463
  ldy #$07
  cmp #$20
  beq loc_b0_B006
  ldy #$04
  cmp #$10
  beq loc_b0_B006
  ldy #$08
loc_b0_B006:
  tya
  jmp loc_b0_B309

sub_b0_B00A:
  lda var_6559
  cmp var_6555
  bne loc_b0_B01A
  lda var_655A
  cmp var_6556
  beq loc_b0_B080
loc_b0_B01A:
  jsr sub_b0_AE71
loc_b0_B01D:
  lda var_655A
  cmp var_6552
  bcc loc_b0_B041
  bne loc_b0_B02F
  lda var_6559
  cmp var_6551
  bcc loc_b0_B041
loc_b0_B02F:
  lda var_655A
  cmp var_6554
  bcc loc_b0_B047
  bne loc_b0_B041
  lda var_6559
  cmp var_6553
  bcc loc_b0_B047
loc_b0_B041:
  jsr sub_b0_AEB2
  bcc loc_b0_B01D
  rts
loc_b0_B047:
  sec
  lda var_6559
  sbc var_6551
  asl a
  adc #$0C
  tay
  jsr sub_b0_AE60
  lda (var_0042),y
  bne loc_b0_B068
  lda var_6530
  cmp #$04
  beq loc_b0_B062
  sec
  rts
loc_b0_B062:
  jsr sub_b0_B088
  jmp loc_b0_B074
loc_b0_B068:
  sta var_654B
  iny
  lda (var_0042),y
  sta var_654C
  jsr sub_b0_AF30
loc_b0_B074:
  lda var_6559
  sta var_6555
  lda var_655A
  sta var_6556
loc_b0_B080:
  jsr sub_b0_AE64
  ldy var_655B
  clc
  rts

sub_b0_B088:
  sty var_64A6
  jsr sub_b0_B1B8
  ldy var_64A6
  iny
  sta (var_0042),y
  sta var_654C
  dey
  lda var_6566
  sta (var_0042),y
  sta var_654B
  jsr sub_b0_AE64
  jsr sub_b0_B3C2
  lda #$C0
  ora var_654A
  sta var_654A
  rts

sub_b0_B0AF:
  ldx var_655F
  stx var_6532
  ldx var_6560
  stx var_6533
  ldx var_6561
  ldy var_6562
  stx var_6534
  sty var_6535
  inx
  bne loc_b0_B0CB
  iny
loc_b0_B0CB:
  cpy var_655E
  bne loc_b0_B0E1
  cpx var_655D
  bne loc_b0_B0E1
  ldx #$00
  ldy #$00
  inc var_655F
  bne loc_b0_B0E1
  inc var_6560
loc_b0_B0E1:
  stx var_6561
  sty var_6562
  rts

sub_b0_B0E8:
  inc var_655B
  bne loc_b0_B0F5
  inc var_6559
  bne loc_b0_B0F5
  inc var_655A
loc_b0_B0F5:
  rts

sub_b0_B0F6:
  ldy var_6538
  ldx var_6539
  sty var_0042
  stx var_0043
  inc var_6538
  bne loc_b0_B108
  inc var_6539
loc_b0_B108:
  rts

sub_b0_B109:
  ldy var_6536
  bne loc_b0_B116
  ldx var_6537
  beq loc_b0_B11A
  dec var_6537
loc_b0_B116:
  dec var_6536
  rts
loc_b0_B11A:
  jmp loc_b0_B303

sub_b0_B11D:
  jsr sub_b0_AF4B
  lda var_6538
  sta var_0042
  lda var_6539
  sta var_0043
  lda #$01
loc_b0_B12C:
  sta var_64A6
  lda #$00
  sta var_654D
  clc
loc_b0_B135:
  inc var_654D
  jsr sub_b0_AF65
  bcc loc_b0_B140
  jmp loc_b0_B1AB
loc_b0_B140:
  ldx #$00
loc_b0_B142:
  stx var_64A5
  lda var_620B,x
  beq loc_b0_B183
  bpl loc_b0_B15C
  lda var_64BD
  cmp #$16
  beq loc_b0_B156
  jmp loc_b0_B188
loc_b0_B156:
  lda var_622B,x
  sta var_620B,x
loc_b0_B15C:
  ldy #$00
  inx
  inx
loc_b0_B160:
  inx
  lda (var_0042),y
  cmp #$BF
  beq loc_b0_B160
  cmp #$AA
  beq loc_b0_B175
  cmp var_620B,x
  bne loc_b0_B17C
  iny
  cpy #$1D
  bne loc_b0_B160
loc_b0_B175:
  ldy #$1E
  ldx var_64A5
  clc
  rts
loc_b0_B17C:
  jsr sub_b0_B1A1
  bcc loc_b0_B142
  bcs loc_b0_B135
loc_b0_B183:
  ldy var_64A6
  bne loc_b0_B12C
loc_b0_B188:
  ldy var_64A6
  bne loc_b0_B17C

sub_b0_B18D:
  ldy #$00
  inx
  inx
loc_b0_B191:
  inx
  lda (var_0042),y
  sta var_620B,x
  iny
  cpy #$1E
  bne loc_b0_B191
  ldx var_64A5
  sec
  rts

sub_b0_B1A1:
  clc
  lda var_64A5
  adc #$23
  tax
  cpx #$F5
  rts
loc_b0_B1AB:
  lda #$00
  ldy var_64A6
  beq loc_b0_B1B5
  jmp loc_b0_B12C
loc_b0_B1B5:
  jmp loc_b0_B2FB

sub_b0_B1B8:
  lda var_6566
  beq loc_b0_B1DE
loc_b0_B1BD:
  dec var_6565
  bmi loc_b0_B1D9
  clc
  ldx #$04
loc_b0_B1C5:
  rol var_6566,x
  dex
  bne loc_b0_B1C5
  bcc loc_b0_B1BD
  inc var_6563
  bne loc_b0_B1D5
  inc var_6564
loc_b0_B1D5:
  lda var_6565
  rts
loc_b0_B1D9:
  lda #$00
  sta var_6566
loc_b0_B1DE:
  lda #$00
  sta var_64A7
  jsr sub_b0_AF4B
loc_b0_B1E6:
  clc
  lda var_6130
  adc var_6131
  beq loc_b0_B1F8
  cmp var_6134
  bcc loc_b0_B208
  lda #$FF
  bne loc_b0_B202
loc_b0_B1F8:
  lda var_64A7
  bne loc_b0_B234
  lda #$01
  sta var_64A7
loc_b0_B202:
  sta var_6131
  clc
  adc #$11
loc_b0_B208:
  sta var_6130
  sta var_6566
  tay
  asl a
  asl a
  tay
  ldx #$04
  clc
loc_b0_B215:
  lda var_613B,y
  sta var_6566,x
  beq loc_b0_B223
  sec
  lda #$00
  sta var_613B,y
loc_b0_B223:
  dey
  dex
  bne loc_b0_B215
  bcc loc_b0_B1E6
  jsr sub_b0_AF4F
  lda var_6135
  sta var_6565
  bne loc_b0_B1BD
loc_b0_B234:
  jmp loc_b0_B2FB

sub_b0_B237:
  lda var_6566
  bne loc_b0_B23D
  rts
loc_b0_B23D:
  pha
  jsr sub_b0_AF4B
  ldy var_6565
  pla
  clc
  jsr sub_b0_B251
  lda #$00
  sta var_6566
  jmp sub_b0_AF4F

sub_b0_B251:
  ldx #$FC
loc_b0_B253:
  ror var_646B,x
  inx
  bne loc_b0_B253
  iny
  cpy var_6135
  bne sub_b0_B251
  asl a
  asl a
  tay
  beq loc_b0_B283
  ldx #$04
loc_b0_B266:
  lda var_64BD
  cmp #$16
  bne loc_b0_B276
  lda var_6566,x
  eor var_613B,y
  jmp loc_b0_B27C
loc_b0_B276:
  lda var_6566,x
  ora var_613B,y
loc_b0_B27C:
  sta var_613B,y
  dey
  dex
  bne loc_b0_B266
loc_b0_B283:
  rts

sub_b0_B284:
  lda var_6532
  sta var_655B
  sta var_655F
  lda var_6533
  sta var_6559
  sta var_6560
  lda #$00
  sta var_655A
  ldy #$10
loc_b0_B29D:
  tax
  lda var_655B
  lsr a
  bcs loc_b0_B2A7
  txa
  bcc loc_b0_B2B5
loc_b0_B2A7:
  clc
  lda var_655A
  adc var_655D
  sta var_655A
  txa
  adc var_655E
loc_b0_B2B5:
  ror a
  ror var_655A
  ror var_6559
  ror var_655B
  dey
  bne loc_b0_B29D
  lda var_6534
  sta var_6561
  adc var_655B
  sta var_655B
  lda var_6535
  sta var_6562
  adc var_6559
  sta var_6559
  lda #$00
  adc var_655A
  sta var_655A
  rts
loc_b0_B2E3:
  lda #$01
  bne loc_b0_B309
loc_b0_B2E7:
  lda #$02
  bne loc_b0_B309
loc_b0_B2EB:
  lda #$03
  bne loc_b0_B309
  lda #$04
  bne loc_b0_B309
loc_b0_B2F3:
  lda #$05
  bne loc_b0_B309
loc_b0_B2F7:
  lda #$06
  bne loc_b0_B309
loc_b0_B2FB:
  jmp loc_b0_B3EE
  .byte $EA
loc_b0_B2FF:
  lda #$0A
  bne loc_b0_B309
loc_b0_B303:
  lda var_653A
  clc
  bcc loc_b0_B30A
loc_b0_B309:
  sec
loc_b0_B30A:
  php
  sta var_653A
  lda #$00
  sta var_0048
  jsr sub_b0_ADD5
  plp
  ldx var_64A4
  txs
  rts
data_b0_B31B:
  .byte $00, $00, $FF, $3F
data_b0_B31F:
  .byte $01, $0A, $64
data_b0_B322:
  .byte $D4, $C9, $C1, $C2, $D3, $D2, $C1, $CC
loc_b0_B32A:
  jsr sub_b0_A6CC
  bcs loc_b0_B337
  lda #$00
  tay
  sta var_67EE
  sta (var_0040),y
loc_b0_B337:
  lda var_653A
  jmp loc_b0_A628
  .byte $AD, $EE, $67, $F0, $08, $EE, $32, $65, $D0, $03, $EE, $33, $65, $A9, $00, $8D
  .byte $EE, $67, $4C, $D6, $A4
loc_b0_B352:
  sta var_6531
  jsr sub_b0_A602
  jsr sub_b0_A2BA
  jmp loc_b0_A24D
loc_b0_B35E:
  ldy #$13
loc_b0_B360:
  lda (var_0042),y
  bne loc_b0_B378
  iny
  cpy #$17
  bne loc_b0_B360
  ldy #$19
loc_b0_B36B:
  lda (var_0042),y
  sta var_62E9,y
  iny
  cpy #$1D
  bne loc_b0_B36B
loc_b0_B375:
  jmp loc_b0_A614
loc_b0_B378:
  ldx #$FF
  stx var_67EE
  bne loc_b0_B375
loc_b0_B37F:
  lda data_b0_B3D1
  ldy data_b0_B3D0
  jsr sub_b0_B3A1
  ldy var_645B
  dey
  bpl loc_b0_B395
  ldy #$0F
  nop
  nop
  dec var_645A
loc_b0_B395:
  sty var_645B
  dec var_645F
  dec data_b0_B3CD
  bne loc_b0_B37F
  rts

sub_b0_B3A1:
  php
  sei
  jsr sub_b0_B737
  bcs loc_b0_B3AB
  plp
  clc
  rts
loc_b0_B3AB:
  plp
  sec
  rts

sub_b0_B3AE:
  lda var_6531
  sta var_645F
  lda #$00
  sta var_645E
  lda var_656E
  eor #$FF
  sta var_6459
  rts

sub_b0_B3C2:
  lda #$00
  tay
loc_b0_B3C5:
  sta (var_0042),y
  iny
  bne loc_b0_B3C5
  rts
  .byte $00, $1B
data_b0_B3CD:
  .byte $00, $0A, $1B
data_b0_B3D0:
  .byte $AF
data_b0_B3D1:
  .byte $B8, $20, $93, $FE, $A9, $00, $20, $89, $FE, $4C, $74, $9D

sub_b0_B3DD:
  sta var_64D2
  sta var_64DF
  sta var_64E0
  rts

sub_b0_B3E7:
  jsr sub_b0_A6C3
  sty var_6526
  rts
loc_b0_B3EE:
  jsr sub_b0_ADD5
  ldx var_64A4
  txs
  jsr sub_b0_A2E6
  tsx
  stx var_64A4
  lda #$09
  jmp loc_b0_B309

sub_b0_B401:
  ldx #$31

sub_b0_B403:
  lda data_b0_B749,x
  sta var_6DB0,x
  dex
  bpl sub_b0_B403
  lda #$00
  sta var_67EE
  sta var_64BD
  lda #$FF
  ldx #$0F

sub_b0_B418:
  sta var_67F0,x
  dex
  bpl sub_b0_B418
  ldx #$17

sub_b0_B420:
  lda data_b0_B8AF,x
  sta var_6456,x
  dex
  bpl sub_b0_B420
  ldx #$68

sub_b0_B42B:
  lda data_b0_B552,x
  sta var_64BE,x
  dex
  bpl sub_b0_B42B
  ldx #$4F

sub_b0_B436:
  lda data_b0_B5BB,x
  sta var_6530,x
  dex
  bpl sub_b0_B436
  jsr sub_b0_B445
  jmp sub_b0_B486

sub_b0_B445:
  ldx #$2D

sub_b0_B447:
  lda data_b0_B618,x
  sta var_6D54,x
  sta var_6B00,x
  sta var_6780,x
  dex
  bpl sub_b0_B447
  lda #$A0
  ldx #$28

sub_b0_B45A:
  sta var_6B2E,x
  sta var_67AE,x
  sta var_6D82,x
  dex
  bpl sub_b0_B45A
  ldx #$07

sub_b0_B468:
  lda data_b0_B600,x
  sta var_6D9F,x
  lda data_b0_B608,x
  sta var_6B4B,x
  lda data_b0_B610,x
  sta var_67CB,x
  dex
  bpl sub_b0_B468
  lda #$AD
  sta var_0040
  lda #$67
  sta var_0041
  rts

sub_b0_B486:
  ldx #$00
  txa

sub_b0_B489:
  sta var_6100,x
  inx
  bne sub_b0_B489
  inx
  stx var_6131
  stx var_6137
  lda #$11
  sta var_6101
  sta var_6130
  lda #$0D
  sta var_6102
  lda #$10
  sta var_6135
  lda #$7A
  sta var_6127
  lda #$28
  sta var_6134
  lda #$03
  sta var_6103
  lda #$FE
  sta var_6106
  rts
data_b0_B4BD:
  .byte $D0, $C5, $C3, $A0, $CC, $CD, $AD, $C4, $CF, $D3, $A0, $B3, $AE, $B3, $B1, $87
  .byte $8D, $A8, $C3, $A9, $C3, $EF, $F0, $F9, $F2, $E9, $E7, $E8, $F4, $A0, $CC, $E9
  .byte $F5, $CD, $E9, $EE, $A0, $B1, $B9, $B9, $B3, $AE, $B8, $87, $8D, $D0, $F2, $EF
  .byte $E7, $F2, $E1, $ED, $ED, $E5, $F2, $BA, $A0
data_b0_B4F6:
  .byte $CC, $E9, $F5, $A0, $CD, $E9, $EE, $A0, $A0, $A8, $C5, $C3, $CE, $D5, $A9, $87
  .byte $8D, $8D, $34, $45, $45, $4C, $32, $59, $57, $77, $4F, $35, $4D, $33, $33, $4C
  .byte $50, $72, $49, $68, $3C, $46, $BA, $A0, $41, $75, $A0, $43, $71
data_b0_B523:
  .byte $D0, $C5, $C3, $A0, $CC, $CD, $AD, $C4, $CF, $D3, $A0, $B3, $AE, $B3, $B1, $8D
  .byte $C4, $C9, $D3, $CB, $A0, $D6, $CF, $CC, $D5, $CD, $C5, $A0
data_b0_B53F:
  .byte $8D, $C1, $F2, $E5, $A0, $F9, $EF, $F5, $A0, $F3, $F5, $F2, $E5, $BF, $A8, $D9
  .byte $AF, $CE, $A9
data_b0_B552:
  .byte $AD, $67, $00, $03, $F0, $FD, $1B, $FD
  .byte $03, $03, $CC, $00, $01, $A0, $03, $00
  .byte $06, $00, $00, $00, $00, $06, $00, $00
  .byte $00, $01, $00, $01, $00, $00, $00, $00
  .byte $00, $00, $00, $00, $60, $00
  .res 60, $A0
  .byte $03, $84, $00, $00, $00, $40, $00
data_b0_B5BB:
  .byte $02, $02, $04, $00, $00, $96, $04, $00
  .byte $00, $0D, $00, $54, $80, $67, $80, $66
  .byte $80, $65, $00, $00, $00, $00, $26, $0F
  .byte $26, $0F, $00, $26, $0E, $03, $69, $7A
  .byte $00, $00, $00, $7A, $00, $00, $00, $00
  .byte $01, $00, $00, $04, $00, $01, $00, $04
  .byte $00, $00, $00, $25, $00, $00, $00, $00
  .byte $00, $00, $00, $04, $10, $01, $FF, $11
  .byte $00, $00, $00, $00, $00
data_b0_B600:
  .byte $54, $6D, $54, $6C, $54, $6B, $2D, $6B
data_b0_B608:
  .byte $00, $6B, $00, $6A, $00, $69, $AD, $67
data_b0_B610:
  .byte $80, $67, $80, $66, $80, $65, $00, $00
data_b0_B618:
  .byte $26, $0F, $26, $0D, $00, $26, $0E, $03
  .byte $69, $7A, $00, $00, $00, $7A, $00, $00
  .byte $00, $00, $01, $00, $00, $04, $00, $01
  .byte $00, $04, $00, $00, $00, $25, $00, $00
  .byte $00, $00, $00, $00, $00, $04, $10, $01
  .byte $FF, $11, $00, $00, $00, $00
data_b0_B646:
  .byte $8D, $A9, $D3, $A8, $C5, $CC, $C9, $C6
data_b0_B64E:
  .byte $8D, $C5, $C5, $D2, $C6, $A0, $A9, $D3, $A8, $D2, $CF, $D4, $C3, $C5, $D3
loc_b0_B65D:
  jsr sub_b0_A278
  jsr monitor_crout
loc_b0_B663:
  jsr sub_b0_A5E6
  ora #$80
  jsr monitor_cout
  lda var_653A
  cmp #$05
  bne loc_b0_B663
  jsr monitor_crout
  jmp sub_b0_A2CC
loc_b0_B678:
  lda #$80
  bne loc_b0_B67E
loc_b0_B67C:
  lda #$00
loc_b0_B67E:
  jmp loc_b0_A50C
loc_b0_B681:
  jsr sub_b0_A278
  jsr sub_b0_A2E6
  lda #$7F
  and var_6537
  beq loc_b0_B698
  cmp #$04
  bne loc_b0_B695
  jmp loc_b0_A35E
loc_b0_B695:
  jmp loc_b0_A475
loc_b0_B698:
  jmp loc_b0_A520
loc_b0_B69B:
  jsr sub_b0_AD78
  ldx #$00
loc_b0_B6A0:
  lda data_b0_B4BD,x
  bmi loc_b0_B6B1
  sta var_03FB
  inx
  lda data_b0_B4BD,x
  sta var_03FC
  lda #$80
loc_b0_B6B1:
  jsr monitor_cout
  inx
  cpx #$66
  bne loc_b0_B6A0
  jsr sub_b0_AD78
  rts

sub_b0_B6BD:
  lda var_03C4
  cmp #$16
  bne loc_b0_B6C5
  rts
loc_b0_B6C5:
  jsr sub_b31_F948
  lda #$00
  ldx var_67EF
  jsr sub_b31_CE50
  jsr sub_b31_F948
  ldx #$07
loc_b0_B6D5:
  lda data_b0_B646,x
  jsr monitor_cout
  dex
  bpl loc_b0_B6D5
  jsr sub_b31_F948
  lda #$00
  sta var_67EC
  sta var_67ED
  ldy #$C0
loc_b0_B6EB:
  clc
  lda var_6137,y
loc_b0_B6EF:
  beq loc_b0_B701
  asl a
  bcc loc_b0_B6EF
  pha
  inc var_67EC
  bne loc_b0_B6FD
  inc var_67ED
loc_b0_B6FD:
  pla
  clc
  bcc loc_b0_B6EF
loc_b0_B701:
  dey
  bne loc_b0_B6EB
  ldx var_67EC
  lda var_67ED
  jsr sub_b31_CE50
  jsr sub_b31_F948
  ldx #$0E
loc_b0_B712:
  lda data_b0_B64E,x
  jsr monitor_cout
  dex
  bpl loc_b0_B712
  rts

sub_b0_B71C:
  ldx #$00
loc_b0_B71E:
  lda data_b0_B53F,x
  jsr monitor_cout
  inx
  cpx #$13
  bne loc_b0_B71E
  jsr monitor_rdkey
  pha
  jsr monitor_cout
  jsr monitor_crout
  pla
  cmp #$D9
  rts

sub_b0_B737:
  sty var_0048
  sta var_0049
  jsr sub_b0_B89B
  jsr sub_b0_BC16
  php
  pha
  jsr sub_b0_B8AB
  pla
  plp
  rts
data_b0_B749:
  .byte $08, $48, $A9, $10, $8D, $00, $20, $AD
  .byte $C8, $03, $8D, $C3, $03, $AD, $C2, $03
  .byte $8D, $C8, $03, $8D, $00, $50, $68, $28
  .byte $B0, $04, $B1, $42, $90, $02, $91, $42
  .byte $48, $AD, $C3, $03, $8D, $C8, $03, $8D
  .byte $00, $50, $A9, $90, $8D, $00, $20, $68
  .byte $60
loc_b0_B77A:
  .byte $A9
  .byte $01, $8D, $62, $64, $AD, $D4, $64, $29, $07, $C9, $07, $F0, $03, $4C, $C2, $9F
  .byte $20, $DB, $B7, $4C, $8D, $B8
loc_b0_B791:
  .byte $A9, $02, $D0, $E7
loc_b0_B795:
  .byte $AD, $D4, $64, $29, $06, $C9, $06, $D0, $EA, $20, $DB, $B7, $A9, $00, $8D, $5E
  .byte $64, $A9, $62, $8D, $5F, $64, $A9, $01, $8D, $62, $64, $20, $8D, $B8, $A9, $00
  .byte $85, $3C, $A9, $62, $85, $3D, $A9, $20, $85, $FA, $20, $A3, $FD, $C6, $FA, $D0
  .byte $F9, $60, $4C, $93, $A0, $AD, $D4, $64, $29, $01, $F0, $B7, $20, $8E, $FD, $AE
  .byte $E1, $64, $AD, $E2, $64, $60, $AD, $E0, $64, $D0, $E7, $AD, $DE, $64, $D0, $E2
  .byte $AD, $DF, $64, $C9, $28, $B0, $DB, $8D, $5A, $64, $AD, $DD, $64, $C9, $0E, $B0
  .byte $D1, $8D, $5B, $64, $AD, $E1, $64, $8D, $5E, $64, $AD, $E2, $64, $8D, $5F, $64
  .byte $A9, $00, $8D, $59, $64, $60
loc_b0_B80B:
  .byte $20, $CA, $B7, $4C, $50, $CE
loc_b0_B811:
  .byte $20, $CA, $B7, $A8, $A9, $A4, $20, $ED, $FD, $98, $4C, $41, $F9
loc_b0_B81E:
  .byte $A9, $00, $8D, $59, $64, $AD, $D7, $64, $8D, $6D, $65, $20, $4B, $AF, $A2, $00
  .byte $A0, $00, $20, $8E, $FD, $20, $44, $F9, $A9, $A0, $20, $ED, $FD, $20, $50, $B8
  .byte $C8, $20, $50, $B8, $C8, $C8, $C8, $E8, $E0, $28, $D0, $E6, $20, $8E, $FD, $4C
  .byte $E1, $B6, $98, $29, $01, $D0, $0D, $A9, $06, $8D, $BD, $64, $B9, $38, $61, $0A
  .byte $0A, $4C, $6A, $B8, $A9, $08, $8D, $BD, $64, $B9, $38, $61, $0A, $48, $A9, $AE
  .byte $B0, $02, $A9, $AA, $20, $ED, $FD, $68, $CE, $BD, $64, $D0, $EF, $60, $E8, $E0
  .byte $0E, $90, $A9, $64, $A0, $56, $60, $48, $68, $18, $EA, $20, $8C, $B8, $60, $AD
  .byte $0C, $B5, $C9, $32, $D0, $04, $A9, $64, $A0, $56, $4C, $37, $B7

sub_b0_B89B:
  .byte $A9, $10, $2C, $02, $20, $30, $FB, $2C, $02, $20, $30, $FB, $8D, $00, $20, $60

sub_b0_B8AB:
  .byte $A9, $90, $D0, $EE
data_b0_B8AF:
  .byte $01, $10, $01, $00, $00, $00, $C2, $B8
  .byte $00, $70, $00, $01, $00, $00, $00, $00
  .byte $00, $FF, $FF, $00, $01, $EF, $D8, $00

sub_b0_B8C7:
  ldx #$00
  ldy #$02
loc_b0_B8CB:
  dey
  lda (var_003E),y
  lsr a
  rol var_6400,x
  lsr a
  rol var_6400,x
  sta var_6300,y
  inx
  cpx #$56
  bcc loc_b0_B8CB
  ldx #$00
  tya
  bne loc_b0_B8CB
  ldx #$55
loc_b0_B8E5:
  lda var_6400,x
  and #$3F
  sta var_6400,x
  dex
  bpl loc_b0_B8E5
  rts

sub_b0_B8F1:
  sec
  stx var_0027
  stx var_6490
  lda var_560D,x
  nop
  nop
  lda var_560E,x
  bpl loc_b0_B904
  jmp loc_b0_B99C
loc_b0_B904:
  lda var_6400
  sta var_0026
  lda #$FF
  sta var_560F,x
  nop
  bit var_0000
  ora var_560C,x
  jsr sub_b0_B9B2
  nop
  ldy #$04
loc_b0_B91A:
  pha
  pla
  pha
  pla
  jsr sub_b0_B9A3
  clc
  nop
  dey
  bne loc_b0_B91A
  lda #$D5
  jsr sub_b0_B9A0
  lda #$AA
  jsr sub_b0_B9A0
  lda #$AD
  jsr sub_b0_B9A0
  pha
  pla
  tya
  ldy #$56
  bne loc_b0_B93F
loc_b0_B93C:
  lda var_6400,y
loc_b0_B93F:
  eor var_63FF,y
  tax
  lda data_b0_BF29,x
  ldx var_0027
  sta var_560D,x
  nop
  lda var_0000
  lda var_560C,x
  jsr sub_b0_B9B6
  lda var_0000
  dey
  bne loc_b0_B93C
  lda var_0026
  nop
loc_b0_B95C:
  eor var_6300,y
  tax
  lda data_b0_BF29,x
  ldx var_6490
  sta var_560D,x
  nop
  lda var_0000
  lda var_560C,x
  jsr sub_b0_B9B6
  lda var_0000
  lda var_6300,y
  iny
  bne loc_b0_B95C
  tax
  lda data_b0_BF29,x
  ldx var_0027
  jsr sub_b0_B9A5
  lda #$DE
  jsr sub_b0_B9A0
  lda #$AA
  jsr sub_b0_B9A0
  lda #$EB
  jsr sub_b0_B9A0
  lda #$FF
  jsr sub_b0_B9A0
  lda var_560E,x
  nop
  nop
loc_b0_B99C:
  lda var_560C,x
  rts

sub_b0_B9A0:
  cpx var_0000
  clc

sub_b0_B9A3:
  pha
  pla

sub_b0_B9A5:
  sta var_560D,x
  nop
  bit var_0000
  ora var_560C,x
  jsr sub_b0_B9B6
  rts

sub_b0_B9B2:
  pha
  pla
  clc
  nop

sub_b0_B9B6:
  jsr sub_b0_B9B9

sub_b0_B9B9:
  rts

sub_b0_B9BA:
  ldy #$00
loc_b0_B9BC:
  ldx #$56
loc_b0_B9BE:
  dex
  bmi loc_b0_B9BC
  lda var_6300,y
  lsr var_6400,x
  rol a
  lsr var_6400,x
  rol a
  sta (var_003E),y
  iny
  cpy var_0026
  bne loc_b0_B9BE
  rts

sub_b0_B9D4:
  ldy #$20
loc_b0_B9D6:
  dey
  bne loc_b0_B9DC
  jmp loc_b0_BA55
loc_b0_B9DC:
  lda var_560C,x
  bpl loc_b0_B9DC
  jsr sub_b0_BBF0
loc_b0_B9E4:
  eor #$D5
  bne loc_b0_B9D6
  nop
loc_b0_B9E9:
  lda var_560C,x
  bpl loc_b0_B9E9
  jsr sub_b0_BBF0
  cmp #$AA
  bne loc_b0_B9E4
  ldy #$56
loc_b0_B9F7:
  lda var_560C,x
  bpl loc_b0_B9F7
  jsr sub_b0_BBF0
  cmp #$AD
  bne loc_b0_B9E4
  lda #$00
loc_b0_BA05:
  dey
  sty var_0026
loc_b0_BA08:
  ldy var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BA08
  eor sub_b0_BF00,y
  ldy var_0026
  sta var_6400,y
  bne loc_b0_BA05
loc_b0_BA1A:
  sty var_0026
loc_b0_BA1C:
  ldy var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BA1C
  eor sub_b0_BF00,y
  ldy var_0026
  sta var_6300,y
  iny
  bne loc_b0_BA1A
loc_b0_BA2F:
  ldy var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BA2F
  cmp sub_b0_BF00,y
  bne loc_b0_BA55
loc_b0_BA3C:
  lda var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BA3C
  cmp #$DE
  bne loc_b0_BA55
  nop
loc_b0_BA49:
  lda var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BA49
  cmp #$AA
  beq loc_b0_BA57
loc_b0_BA55:
  sec
  rts
loc_b0_BA57:
  clc
  rts

sub_b0_BA59:
  ldy #$FC
  sty var_0026
loc_b0_BA5D:
  iny
  bne loc_b0_BA64
  inc var_0026
  beq loc_b0_BA55
loc_b0_BA64:
  lda var_560C,x
  bpl loc_b0_BA64
  jsr sub_b0_BBF0
loc_b0_BA6C:
  cmp #$D5
  bne loc_b0_BA5D
  nop
  nop
  nop
  nop
loc_b0_BA74:
  lda var_560C,x
  bpl loc_b0_BA74
  jsr sub_b0_BBF0
  cmp #$AA
  bne loc_b0_BA6C
  nop
  nop
  nop
  ldy #$03
loc_b0_BA85:
  lda var_560C,x
  bpl loc_b0_BA85
  jsr sub_b0_BBF0
  cmp #$96
  bne loc_b0_BA6C
  nop
  nop
  nop
  lda #$00
loc_b0_BA96:
  sta var_0027
loc_b0_BA98:
  lda var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BA98
  nop
  nop
  nop
  rol a
  sta var_0026
loc_b0_BAA6:
  lda var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BAA6
  and var_0026
  sta a:var_002C,y
  eor var_0027
  dey
  bpl loc_b0_BA96
  tay
  bne loc_b0_BA55
loc_b0_BABB:
  lda var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BABB
  cmp #$DE
  bne loc_b0_BA55
  nop
  nop
  nop
  nop
loc_b0_BACB:
  lda var_560C,x
  nop
  nop
  nop
  bpl loc_b0_BACB
  cmp #$AA
  bne loc_b0_BAD9
  clc
  rts
loc_b0_BAD9:
  sec
  rts
loc_b0_BADB:
  stx var_002B
  sta var_002A
  cmp var_6470
  beq loc_b0_BB37
  lda #$00
  sta var_0026
loc_b0_BAE8:
  lda var_6470
  sta var_0027
  sec
  sbc var_002A
  beq loc_b0_BB25
  bcs loc_b0_BAFB
  eor #$FF
  inc var_6470
  bcc loc_b0_BB00
loc_b0_BAFB:
  adc #$FE
  dec var_6470
loc_b0_BB00:
  cmp var_0026
  bcc loc_b0_BB06
  lda var_0026
loc_b0_BB06:
  cmp #$0C
  bcs loc_b0_BB0B
  tay
loc_b0_BB0B:
  sec
  jsr sub_b0_BB29
  lda data_b0_BF11,y
  jsr sub_b0_BF00
  lda var_0027
  clc
  jsr sub_b0_BB2C
  lda data_b0_BF1D,y
  jsr sub_b0_BF00
  inc var_0026
  bne loc_b0_BAE8
loc_b0_BB25:
  jsr sub_b0_BF00
  clc

sub_b0_BB29:
  lda var_6470

sub_b0_BB2C:
  and #$03
  rol a
  ora var_002B
  tax
  lda var_5600,x
  ldx var_002B
loc_b0_BB37:
  rts
  .byte $AA, $A0, $A0

sub_b0_BB3B:
  sec
  lda var_560D,x
  nop
  nop
  lda var_560E,x
  bmi loc_b0_BBB5
  lda #$FF
  sta var_560F,x
  nop
  bit var_0000
  cmp var_560C,x
  jsr sub_b0_BBED
  pha
  pla
loc_b0_BB56:
  jsr sub_b0_BBE9
  sta var_560D,x
  nop
  bit var_0000
  cmp var_560C,x
  jsr sub_b0_BBED
  nop
  dey
  bne loc_b0_BB56
  lda #$D5
  jsr sub_b0_BBD7
  lda #$AA
  jsr sub_b0_BBD7
  lda #$96
  jsr sub_b0_BBD7
  lda var_0041
  jsr sub_b0_BBBE
  lda var_0044
  jsr sub_b0_BBBE
  lda var_003F
  jsr sub_b0_BBBE
  lda var_0041
  eor var_0044
  eor var_003F
  cpx var_0000
  pha
  lsr a
  ora var_003E
  sta var_560D,x
  nop
  cpx var_0000
  lda var_560C,x
  pla
  ora #$AA
  jsr sub_b0_BBED
  jsr sub_b0_BBD6
  lda #$DE
  jsr sub_b0_BBD7
  lda #$AA
  jsr sub_b0_BBD7
  lda #$EB
  jsr sub_b0_BBD7
  clc
loc_b0_BBB5:
  lda var_560E,x
  nop
  nop
  lda var_560C,x
  rts

sub_b0_BBBE:
  bit var_0000
  pha
  lsr a
  ora var_003E
  sta var_560D,x
  nop
  bit var_0000
  cmp var_560C,x
  jsr sub_b0_BBED
  pla
  nop
  nop
  nop
  ora #$AA

sub_b0_BBD6:
  nop

sub_b0_BBD7:
  pha
  nop
  bit var_0000
  pla
  sta var_560D,x
  nop
  bit var_0000
  cmp var_560C,x
  jsr sub_b0_BBED
  rts

sub_b0_BBE9:
  pha
  pla
  clc
  nop

sub_b0_BBED:
  jsr sub_b0_BBF0

sub_b0_BBF0:
  rts
  .byte $EA, $20, $B6, $B0, $4C, $B3, $B6, $20, $DB, $B0, $4C, $B3, $B6, $38, $AD, $C3
  .byte $B5, $ED, $E6, $B5, $8D, $C3, $B5, $B0, $03, $CE, $C4, $B5, $A9, $42, $A2, $44
  .byte $60, $84, $48, $85, $49

sub_b0_BC16:
  ldy #$02
  sty var_6498
  ldy #$04
  sty var_6478
  lda #$10
  tax
  ldy #$0F
  cmp (var_0048),y
  beq loc_b0_BC44
  txa
  pha
  lda (var_0048),y
  tax
  pla
  pha
  sta (var_0048),y
  lda var_560E,x
loc_b0_BC35:
  ldy #$08
  lda var_560C,x
loc_b0_BC3A:
  cmp var_560C,x
  bne loc_b0_BC35
  dey
  bne loc_b0_BC3A
  pla
  tax
loc_b0_BC44:
  lda var_560E,x
  lda var_560C,x
  ldy #$08
loc_b0_BC4C:
  lda var_560C,x
  pha
  pla
  pha
  pla
  stx var_6488
  cmp var_560C,x
  bne loc_b0_BC5E
  dey
  bne loc_b0_BC4C
loc_b0_BC5E:
  php
  lda var_5609,x
  ldy #$06
loc_b0_BC64:
  lda (var_0048),y
  sta a:var_0036,y
  iny
  cpy #$0A
  bne loc_b0_BC64
  ldy #$03
  lda (var_003C),y
  sta var_0047
  ldy #$02
  lda (var_0048),y
  ldy #$10
  cmp (var_0048),y
  beq loc_b0_BC84
  sta (var_0048),y
  plp
  ldy #$00
  php
loc_b0_BC84:
  ror a
  bcc loc_b0_BC8C
  lda var_560A,x
  bcs loc_b0_BC8F
loc_b0_BC8C:
  lda var_560B,x
loc_b0_BC8F:
  ror var_0035
  plp
  php
  bne loc_b0_BCA0
  ldy #$07
loc_b0_BC97:
  jsr sub_b0_BF00
  dey
  bne loc_b0_BC97
  ldx var_6488
loc_b0_BCA0:
  ldy #$04
  lda (var_0048),y
  jsr sub_b0_BD6A
  plp
  bne loc_b0_BCBB
  ldy var_0047
  bpl loc_b0_BCBB
loc_b0_BCAE:
  ldy #$12
loc_b0_BCB0:
  dey
  bne loc_b0_BCB0
  inc var_0046
  bne loc_b0_BCAE
  inc var_0047
  bne loc_b0_BCAE
loc_b0_BCBB:
  ldy #$0C
  lda (var_0048),y
  beq loc_b0_BD1B
  cmp #$04
  beq loc_b0_BD1D
  ror a
  php
  bcs loc_b0_BCCC
  jsr sub_b0_B8C7
loc_b0_BCCC:
  ldy #$30
  sty var_6480
loc_b0_BCD1:
  ldx var_6488
  jsr sub_b0_BA59
  bcc loc_b0_BCFD
loc_b0_BCD9:
  dec var_6480
  bpl loc_b0_BCD1
loc_b0_BCDE:
  lda var_6470
  pha
  lda #$60
  jsr sub_b0_BDA5
  dec var_6498
  beq loc_b0_BD14
  lda #$04
  sta var_6478
  lda #$00
  jsr sub_b0_BD6A
  pla
loc_b0_BCF7:
  jsr sub_b0_BD6A
  jmp loc_b0_BCCC
loc_b0_BCFD:
  ldy var_002E
  cpy var_6470
  beq loc_b0_BD20
  lda var_6470
  pha
  tya
  jsr sub_b0_BDA5
  pla
  dec var_6478
  bne loc_b0_BCF7
  beq loc_b0_BCDE
loc_b0_BD14:
  pla
  lda #$40
loc_b0_BD17:
  plp
  jmp loc_b0_BD58
loc_b0_BD1B:
  beq loc_b0_BD56
loc_b0_BD1D:
  jmp loc_b0_BDBF
loc_b0_BD20:
  ldy #$03
  lda (var_0048),y
  pha
  lda var_002F
  ldy #$0E
  sta (var_0048),y
  pla
  beq loc_b0_BD36
  cmp var_002F
  beq loc_b0_BD36
  lda #$20
  bne loc_b0_BD17
loc_b0_BD36:
  ldy #$05
  lda (var_0048),y
  tay
  lda data_b0_BEB8,y
  cmp var_002D
  bne loc_b0_BCD9
  plp
  bcc loc_b0_BD61
  jsr sub_b0_B9D4
  php
  bcs loc_b0_BCD9
  plp
  ldx #$00
  stx var_0026
  jsr sub_b0_B9BA
  ldx var_6488
loc_b0_BD56:
  clc
  .byte $24
loc_b0_BD58:
  sec
  ldy #$0D
  sta (var_0048),y
  lda var_5608,x
  rts
loc_b0_BD61:
  jsr sub_b0_B8F1
  bcc loc_b0_BD56
  lda #$10
  bcs loc_b0_BD58

sub_b0_BD6A:
  pha
  ldy #$01
  lda (var_003C),y
  ror a
  pla
  bcc sub_b0_BD7B
  asl a
  jsr sub_b0_BD7B
  lsr var_6470
  rts

sub_b0_BD7B:
  sta var_002A
  jsr sub_b0_BD9E
  lda var_6470,y
  bit var_0035
  bmi loc_b0_BD8A
  lda var_6478,y
loc_b0_BD8A:
  sta var_6470
  lda var_002A
  bit var_0035
  bmi loc_b0_BD98
  sta var_6478,y
  bpl loc_b0_BD9B
loc_b0_BD98:
  sta var_6470,y
loc_b0_BD9B:
  jmp loc_b0_BADB

sub_b0_BD9E:
  txa
  lsr a
  lsr a
  lsr a
  lsr a
  tay
  rts

sub_b0_BDA5:
  pha
  ldy #$02
  lda (var_0048),y
  ror a
  ror var_0035
  jsr sub_b0_BD9E
  pla
  asl a
  bit var_0035
  bmi loc_b0_BDBB
  sta var_6478,y
  bpl loc_b0_BDBE
loc_b0_BDBB:
  sta var_6470,y
loc_b0_BDBE:
  rts
loc_b0_BDBF:
  ldy #$03
  lda (var_0048),y
  sta var_0041
  lda #$AA
  sta var_003E
  ldy #$56
  lda #$00
  sta var_0044
loc_b0_BDCF:
  sta var_63FF,y
  dey
  bne loc_b0_BDCF
loc_b0_BDD5:
  sta var_6300,y
  dey
  bne loc_b0_BDD5
  lda #$50
  jsr sub_b0_BDA5
  lda #$28
  sta var_0045
loc_b0_BDE4:
  lda var_0044
  jsr sub_b0_BD6A
  jsr sub_b0_BE1D
  lda #$08
  bcs loc_b0_BE14
  lda #$30
  sta var_6480
loc_b0_BDF5:
  sec
  dec var_6480
  beq loc_b0_BE14
  jsr sub_b0_BA59
  bcs loc_b0_BDF5
  lda var_002D
  bne loc_b0_BDF5
  jsr sub_b0_B9D4
  bcs loc_b0_BDF5
  inc var_0044
  lda var_0044
  cmp #$29
  bcc loc_b0_BDE4
  clc
  bcc loc_b0_BE19
loc_b0_BE14:
  ldy #$0D
  sta (var_0048),y
  sec
loc_b0_BE19:
  lda var_5608,x
  rts

sub_b0_BE1D:
  lda #$00
  sta var_003F
  ldy #$80
  bne loc_b0_BE27
loc_b0_BE25:
  ldy var_0045
loc_b0_BE27:
  jsr sub_b0_BB3B
  bcs sub_b0_BE97
  jsr sub_b0_B8F1
  bcs sub_b0_BE97
  inc var_003F
  lda var_003F
  cmp #$0E
  bcc loc_b0_BE25
  ldy #$0D
  sty var_003F
  lda #$30
  sta var_6480
loc_b0_BE42:
  sta var_67F0,y
  dey
  bpl loc_b0_BE42
  ldy var_0045
loc_b0_BE4A:
  jsr sub_b0_BE97
  jsr sub_b0_BE97
  jsr sub_b0_BE97
  pha
  pla
  nop
  dey
  bne loc_b0_BE4A
  jsr sub_b0_BA59
  bcs loc_b0_BE81
  lda var_002D
  beq loc_b0_BE77
  lda #$10
  cmp var_0045
  lda var_0045
  sbc #$01
  sta var_0045
  cmp #$05
  bcs loc_b0_BE81
  sec
  rts
loc_b0_BE72:
  jsr sub_b0_BA59
  bcs loc_b0_BE7C
loc_b0_BE77:
  jsr sub_b0_B9D4
  bcc loc_b0_BE98
loc_b0_BE7C:
  dec var_6480
  bne loc_b0_BE72
loc_b0_BE81:
  jsr sub_b0_BA59
  bcs loc_b0_BE91
  lda var_002D
  cmp #$0D
  bne loc_b0_BE91
  jsr sub_b0_B9D4
  bcc sub_b0_BE1D
loc_b0_BE91:
  dec var_6480
  bne loc_b0_BE81
  sec

sub_b0_BE97:
  rts
loc_b0_BE98:
  ldy var_002D
  lda var_67F0,y
  bmi loc_b0_BE7C
  lda #$FF
  sta var_67F0,y
  dec var_003F
  bpl loc_b0_BE72
  lda var_0044
  bne loc_b0_BEB6
  lda var_0045
  cmp #$10
  bcc sub_b0_BE97
  dec var_0045
  dec var_0045
loc_b0_BEB6:
  clc
  rts
data_b0_BEB8:
  .byte $00, $0B, $09, $07, $05, $03, $01, $0C, $0A, $08, $06, $04, $02, $0D, $A0, $00
  .byte $98, $59, $E5, $BE, $D0, $03, $4C, $8E, $FD, $8D, $FB, $03, $C8, $98, $59, $E5
  .byte $BE, $8D, $FC, $03, $C8, $A9, $80, $20, $ED, $FD, $4C, $C8, $BE, $36, $2A, $36
  .byte $6C, $39, $75, $51, $51, $43, $77, $3A, $6D, $44, $25, $45, $76, $43, $41, $27
  .byte $52, $24, $73, $27, $4F, $26, $26, $1A

sub_b0_BF00:
  ldx #$1D
loc_b0_BF02:
  dex
  bne loc_b0_BF02
  inc var_0046
  bne loc_b0_BF0B
  inc var_0047
loc_b0_BF0B:
  sec
  sbc #$01
  bne sub_b0_BF00
  rts
data_b0_BF11:
  .byte $01, $30, $28, $24, $20, $1E, $1D, $1C, $1C, $1C, $1C, $1C
data_b0_BF1D:
  .byte $70, $2C, $26, $22, $1F, $1E, $1D, $1C, $1C, $1C, $1C, $1C
data_b0_BF29:
  .byte $96, $97, $9A, $9B, $9D, $9E, $9F, $A6, $A7, $AB, $AC, $AD, $AE, $AF, $B2, $B3
  .byte $B4, $B5, $B6, $B7, $B9, $BA, $BB, $BC, $BD, $BE, $BF, $CB, $CD, $CE, $CF, $D3
  .byte $D6, $D7, $D9, $DA, $DB, $DC, $DD, $DE, $DF, $E5, $E6, $E7, $E9, $EA, $EB, $EC
  .byte $ED, $EE, $EF, $F2, $F3, $F4, $F5, $F6, $F7, $F9, $FA, $FB, $FC, $FD, $FE, $FF
  .byte $EA, $20, $DB, $B0, $B0, $21, $AD, $C3, $B5, $8D, $F0, $B7, $AD, $C4, $B5, $8D
  .byte $F1, $B7, $AE, $BB, $B5, $CA, $CA, $8A, $AE, $D6, $B5, $AC, $D7, $B5, $20, $52
  .byte $B0, $EE, $C4, $B5, $20, $99, $B1, $CE, $C2, $B5, $4C, $EF, $B6, $00, $01, $98
  .byte $99, $02, $03, $9C, $04, $05, $06, $A0, $A1, $A2, $A3, $A4, $A5, $07, $08, $A8
  .byte $A9, $AA, $09, $0A, $0B, $0C, $0D, $B0, $B1, $0E, $0F, $10, $11, $12, $13, $B8
  .byte $14, $15, $16, $17, $18, $19, $1A, $C0, $C1, $C2, $C3, $C4, $C5, $C6, $C7, $C8
  .byte $C9, $CA, $1B, $CC, $1C, $1D, $1E, $D0, $D1, $D2, $1F, $D4, $D5, $20, $21, $D8
  .byte $22, $23, $24, $25, $26, $27, $28, $E0, $E1, $E2, $E3, $E4, $29, $2A, $2B, $E8
  .byte $2C, $2D, $2E, $2F, $30, $31, $32, $F0, $F1, $33, $34, $35, $36, $37, $38, $F8
  .byte $39, $3A, $3B, $3C, $3D, $3E, $3F
