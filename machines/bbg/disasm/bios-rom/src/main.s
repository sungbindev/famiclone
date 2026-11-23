; 模块：main
; 没有归入其他模块的字节

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG0_0000"
; 段 0，$5800-$5A28
sub_5800:
  jmp loc_5867

sub_5803:
  jmp loc_588E

sub_5806:
  jmp loc_5892

sub_5809:
  jmp loc_59D2

sub_580C:
  jmp loc_588E

sub_580F:
  jmp loc_5823

sub_5812:
  rts
  .byte $00, $00

sub_5815:
  brk
  .byte $00, $00

sub_5818:
  brk
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
loc_5823:
  jsr sub_5829
  jmp (data_FFFC)

sub_5829:
  pha
  php
  lda #$7F
  and var_6000
  sta var_2008
  lda #$F7
  sei
  and var_5FFF
  sta var_5FFF
  sta sub_FF01
  plp
  lda var_6000
  sta var_2000
  pla
  rts

sub_5848:
  pha
  php
  lda #$7F
  and var_6000
  sta var_2008
  lda #$08
  sei
  ora var_5FFF
  sta var_5FFF
  sta sub_FF01
  plp
  lda var_6000
  sta var_2000
  pla
  rts
loc_5867:
  php
  cmp #$01
  beq loc_587F
  cmp #$02
  beq loc_5884
  cmp #$18
  beq loc_5889
  jsr sub_5829
  jsr sub_D0FB
  jsr sub_5848
  plp
  rts
loc_587F:
  jsr sub_59AA
  plp
  rts
loc_5884:
  jsr sub_59C0
  plp
  rts
loc_5889:
  jsr sub_5809
  plp
  rts
loc_588E:
  lda #$42
  sec

sub_5891:
  rts
loc_5892:
  jsr sub_5829
  jmp (data_FFFC)
  .byte $48, $AD, $FF, $5F, $29, $F7, $8D, $01, $FF, $20, $E1, $DD, $AD, $FF, $5F, $8D
  .byte $01, $FF, $29, $04, $D0, $03, $20
data_58AF:
  .byte $91
data_58B0:
  .byte $58
loc_58B1:
  pla
loc_58B2:
  rti
data_58B3:
data_58B4 = * + 1
  .word sub_5891

sub_58B5:
  jmp (data_58B3)
  .byte $68, $8D, $FA, $5F, $8D, $04, $FF, $AD, $FF, $5F, $8D, $01, $FF, $68, $40

sub_58C7:
  pha
  dec var_5FF8
  dec var_5FA0
  bne loc_58D8
  lda #$0C
  sta var_5FA3
  sta data_FF90
loc_58D8:
  bit var_2002
  bit var_6000
  bpl loc_58B1
  lda var_5FC2
  cmp var_5FC3
  beq loc_58F9
  lda var_5FFF
  and #$F7
  sta sub_FF01
  jsr sub_CF67
  lda var_5FFF
  sta sub_FF01
loc_58F9:
  lda var_5FBB
  beq loc_5914
  lda #$00
  sta var_5FBB
  lda var_5FFF
  and #$F7
  sta sub_FF01
  jsr sub_DDC7
  lda var_5FFF
  sta sub_FF01
loc_5914:
  lda var_6001
  sta var_2001
  jsr sub_58B5
  bit var_5FBD
  bvc loc_5940
  lda var_5FFA
  pha
  lda #$58
  pha
  lda #$B8
  pha
  php
  lda var_5FBE
  sta var_5FFA
  sta loc_FF04
  lda var_5FFF
  ora #$08
  sta sub_FF01
  bne loc_5941
loc_5940:
  pla
loc_5941:
  jmp loc_58B2

sub_5944:
  pha
  lda var_5FFF
  pha
  and #$F7
  sta var_5FFF
  sta sub_FF01
  jsr sub_EAE9
  pla
  and #$08
  ora var_5FFF
  sta var_5FFF
  sta sub_FF01
  pla
  rti
  .byte $98, $48, $AD, $FA, $5F, $48, $AD, $BF, $5F, $8D, $FA, $5F, $8D, $04, $FF, $AD
  .byte $FF, $5F, $48, $09, $08, $8D, $FF, $5F, $8D, $01, $FF, $20
data_597E:
  .byte $91
data_597F:
  .byte $58, $68, $09, $F7, $2D, $FF, $5F, $8D, $FF, $5F, $8D, $01, $FF, $68, $8D, $FA
  .byte $5F, $8D, $04, $FF, $68, $A8, $60

sub_5996:
  pha
  pla
  pha
  pla
  pha
  pla
  nop
  nop
loc_599E:
  jmp loc_58B2

sub_59A1:
  jsr sub_5848
loc_59A4:
  jsr sub_5891
  jmp sub_5829

sub_59AA:
  cpx #$00
  bne loc_59B2
  ldx #$FE
  bne loc_59B9
loc_59B2:
  cpx #$FE
  bne loc_59B9
  ldx var_5FF9
loc_59B9:
  stx var_5FFA
  stx loc_FF04
  rts

sub_59C0:
  lda var_5FFA
  cmp #$FE
  bne loc_59CA
  lda #$00
  rts
loc_59CA:
  cmp var_5FF9
  bne loc_59D1
  lda #$FE
loc_59D1:
  rts
loc_59D2:
  cpx #$01
  bne loc_59EF

sub_59D6:
  ldy var_5C78
  cpy var_5C79
  beq loc_59E7
  lda var_5C00,y
  ldx var_5C01,y
  ldy #$01
  rts
loc_59E7:
  bit var_5C40
  bvs loc_59EF
  ldy #$00
  rts
loc_59EF:
  jsr sub_5829
  jsr sub_F6C0
  jmp sub_5848

sub_59F8:
  bit var_5C40
  bmi loc_5A18
  lda var_5FFF
  pha
  and #$F7
  sta var_5FFF
  sta sub_FF01
  jsr sub_F64F
  pla
  and #$08
  ora var_5FFF
  sta var_5FFF
  sta sub_FF01
loc_5A18:
  rts

sub_5A19:
  lda #$08
loc_5A1B:
  bit var_5FEF
  bne loc_5A1B
  jmp sub_5829

sub_5A23:
  jsr sub_5812
  jmp sub_5829

.segment "SEG1_0000"
; 段 1，$C229-$FFFF
  .byte $CF, $B5, $CD, $B3, $D6, $C6, $D7, $F7, $3A, $C7, $E5, $BB, $AA, $B4, $F3, $D1
  .byte $A7, $BC, $C6, $CB, $E3, $BB, $FA, $CF, $B5, $CC, $C6, $C8, $F0, $B4, $BA, $77
  .byte $72, $69, $74, $74, $65, $6E, $20, $62, $79, $20, $54, $61, $6E, $67, $20, $72
  .byte $75, $69, $63, $68, $75, $6E, $20, $54, $73, $69, $6E, $67, $68, $75, $61, $20
  .byte $55, $6E, $69, $76, $65, $72, $73, $69, $74, $79, $20, $43, $6F, $6D, $70, $75
  .byte $74, $65, $72, $20, $44, $65, $70, $61, $72, $74, $6D, $65, $6E, $74

sub_C287:
  lda #$80
  sta var_4348
  jsr sub_D09C
loc_C28F:
  lda #$A0
  sta var_4340
loc_C294:
  ldx #$14
loc_C296:
  lda #$19
  sta var_5FF8
loc_C29B:
  lda var_5FF8
  beq loc_C2D9
  bit var_4348
  bpl loc_C29B
  lda #$00
  sta var_4348
  lda var_4340
  tay
loc_C2AE:
  lda var_5FF8
  beq loc_C2D9
  bit var_4348
  bmi loc_C2AE
  lda #$80
  sta var_4348
  tya
  cmp #$AA
  bne loc_C294
  dex
  bne loc_C296
  lda #$55
  jsr sub_D146
  jsr sub_D17D
  cmp #$55
  bne loc_C28F
  lda #$5A
  jsr sub_D146
  lda #$01
  rts
loc_C2D9:
  lda #$80
  sta var_4348
  lda #$00
  rts
data_C2E1:
  .byte $0E, $00, $12, $20

sub_C2E5:
  ldx #$3F
  jsr sub_E9F9
loc_C2EA:
  lda data_C2E1,y
  sta var_2007
  iny
  cpy #$04
  bne loc_C2EA
  lda #$00
  sta var_2006
  sta var_2006
  rts

sub_C2FE:
  bit var_2002
  bmi sub_C2FE
loc_C303:
  bit var_2002
  bpl loc_C303
  rts

sub_C309:
  stx var_00FF
  sta var_00FE
  lda var_00FC
  asl a
  asl a
  asl a
  asl a
  php
  asl a
  ora var_00FE
  sty var_00FE
  tay
  lda #$10
  ror a
  plp
  rol a
  rol a
  tax
  rts

sub_C322:
  jsr sub_C309
  pha
  tya
  pha
  and #$E0
  tay
  jsr sub_D093
  jsr sub_C2FE
  jsr sub_E9FB
  ldy #$40
  lda #$6A
loc_C338:
  sta var_2007
  dey
  bne loc_C338
  pla
  pha
  tay
  jmp loc_C34D

sub_C344:
  jsr sub_C309
  pha
  tya
  pha
  jsr sub_C2FE
loc_C34D:
  jsr sub_E9FB
  ldy #$00
loc_C352:
  lda (var_00FE),y
  iny
  cmp #$FF
  beq loc_C35E
  sta var_2007
  bne loc_C352
loc_C35E:
  pla
  clc
  adc #$20
  tay
  pla
  adc #$00
  tax
  jsr sub_E9FB
  ldy #$00
loc_C36C:
  lda (var_00FE),y
  iny
  cmp #$FF
  beq sub_C37A
  adc #$01
  sta var_2007
  bne loc_C36C

sub_C37A:
  lda #$00
  sta var_2005
  sta var_2005
  lda #$0E
  sta var_2001
  rts

sub_C388:
  sty var_00FF
  sta var_00FE
  txa
  ldy #$08
loc_C38F:
  rol var_00FF
  rol a
  cmp var_00FE
  bcc loc_C398
  sbc var_00FE
loc_C398:
  dey
  bne loc_C38F
  tax
  lda var_00FF
  rol a
  rts

sub_C3A0:
  ldx #$00
  lda #$0A
  jsr sub_C388
  bne loc_C3AB
  bcs loc_C3B3
loc_C3AB:
  ldy var_0080
  asl a
  sta a:var_0081,y
  inc var_0080
loc_C3B3:
  ldy var_0080
  txa
  asl a
  sta a:var_0081,y
  inc var_0080
  rts

sub_C3BD:
  pha
  lda #$64
  sec
  jsr sub_C388
  beq loc_C3CF
  tay
  txa
  pha
  jsr sub_C3A0
  pla
  bcc loc_C3D0
loc_C3CF:
  txa
loc_C3D0:
  tay
  jsr sub_C3A0
  ldx var_0080
  pla
  sta var_0081,x
  lda #$FF
  sta var_0082,x
  ldx #$00
  ldy #$81
  lda var_00FD
  jmp sub_C344
  .byte $00, $00, $00, $00, $00, $7C, $C6, $C6, $CE, $D6, $D6, $E6, $C6, $C6, $7C, $00
  .byte $00, $00, $00, $00, $00, $18, $38, $78, $18, $18, $18, $18, $18, $18, $7E, $00
  .byte $00, $00, $00, $00, $00, $7C, $C6, $06, $0C, $18, $30, $60, $C0, $C6, $FE, $00
  .byte $00, $00, $00, $00, $00, $7C, $C6, $06, $06, $3C, $06, $06, $06, $C6, $7C, $00
  .byte $00, $00, $00, $00, $00, $0C, $1C, $3C, $6C, $CC, $FE, $0C, $0C, $0C, $1E, $00
  .byte $00, $00, $00, $00, $00, $FE, $C0, $C0, $C0, $FC, $0E, $06, $06, $C6, $7C, $00
  .byte $00, $00, $00, $00, $00, $38, $60, $C0, $C0, $FC, $C6, $C6, $C6, $C6, $7C, $00
  .byte $00, $00, $00, $00, $00, $FE, $C6, $06, $06, $0C, $18, $30, $30, $30, $30, $00
  .byte $00, $00, $00, $00, $00, $7C, $C6, $C6, $C6, $7C, $C6, $C6, $C6, $C6, $7C, $00
  .byte $00, $00, $00, $00, $00, $7C, $C6, $C6, $C6, $7E, $06, $06, $06, $0C, $78, $00
  .byte $00, $00, $00, $00, $00, $10, $38, $6C, $C6, $C6, $FE, $C6, $C6, $C6, $C6, $00
  .byte $00, $00, $00, $00, $00, $FC, $66, $66, $66, $7C, $66, $66, $66, $66, $FC, $00
  .byte $00, $00, $00, $00, $00, $00, $00, $18, $18, $00, $00, $00, $18, $18, $00, $00
  .byte $00, $00, $00, $00, $00, $C6, $EE, $FE, $FE, $D6, $C6, $C6, $C6, $C6, $C6, $00
  .byte $00, $00, $00, $00, $00, $E6, $66, $6C, $6C, $78, $78, $6C, $66, $66, $E6, $00
  .byte $01, $01, $01, $01, $7F, $41, $41, $42, $42, $44, $48, $50, $40, $40, $40, $40
  .byte $00, $00, $00, $04, $FE, $04, $04, $04, $84, $64, $34, $14, $04, $04, $14, $08
  .byte $00, $1F, $10, $10, $1F, $10, $10, $1F, $14, $44, $34, $14, $04, $04, $FF, $00
  .byte $10, $F8, $10, $10, $F0, $10, $10, $F0, $50, $44, $4C, $50, $40, $44, $FE, $00
  .byte $02, $02, $FF, $04, $08, $0B, $10, $10, $30, $5F, $90, $10, $10, $10, $11, $10
  .byte $00, $04, $FE, $00, $00, $F8, $10, $20, $44, $FE, $40, $40, $40, $40, $40, $80
  .byte $10, $10, $10, $FE, $20, $29, $48, $7E, $08, $08, $0E, $F8, $48, $08, $09, $0A
  .byte $40, $40, $40, $7C, $84, $08, $20, $20, $20, $20, $50, $50, $88, $88, $06, $04
  .byte $08, $FD, $09, $49, $49, $49, $49, $49, $7D, $05, $05, $1D, $E5, $45, $15, $08
  .byte $04, $FE, $00, $44, $44, $28, $28, $10, $10, $28, $28, $44, $84, $00, $FE, $00
  .byte $02, $02, $03, $02, $02, $02, $FF, $02, $02, $02, $02, $02, $02, $02, $02, $02
  .byte $00, $20, $F0, $00, $00, $04, $FE, $00, $00, $80, $60, $30, $10, $00, $00, $00
  .byte $12, $12, $2A, $4A, $8A, $1F, $20, $7F, $A0, $2F, $29, $29, $29, $29, $30, $20
  .byte $10, $10, $90, $A0, $BE, $C4, $24, $A8, $28, $28, $10, $50, $A8, $28, $46, $84
  .byte $10, $10, $11, $11, $FD, $11, $31, $39, $55, $51, $91, $11, $11, $12, $14, $18
  .byte $00, $10, $F8, $10, $10, $10, $10, $10, $10, $10, $10, $10, $12, $12, $0E, $00
  .byte $03, $40, $30, $10, $03, $02, $F2, $13, $12, $12, $13, $12, $12, $2A, $44, $03
  .byte $F8, $10, $A0, $40, $F8, $48, $48, $F8, $48, $48, $F8, $48, $68, $50, $00, $FE
  .byte $00, $47, $30, $11, $01, $01, $F7, $11, $11, $11, $11, $11, $11, $29, $44, $03
  .byte $10, $F8, $10, $10, $10, $10, $F0, $10, $10, $10, $12, $0A, $06, $00, $00, $FE
  .byte $40, $31, $11, $01, $81, $62, $24, $03, $12, $21, $E1, $20, $20, $20, $23, $2C
  .byte $10, $F8, $10, $10, $10, $0E, $00, $F8, $08, $10, $10, $A0, $40, $A0, $1E, $08
  .byte $00, $40, $2F, $21, $01, $02, $E7, $20, $20, $20, $2F, $20, $20, $50, $8F, $00
  .byte $80, $88, $FC, $00, $40, $40, $F8, $40, $40, $48, $FC, $40, $40, $40, $FE, $00
  .byte $00, $7F, $02, $04, $08, $3F, $01, $06, $18, $3F, $11, $01, $11, $21, $45, $02
  .byte $38, $C0, $00, $20, $40, $80, $00, $20, $10, $F8, $08, $00, $10, $08, $08, $00
  .byte $10, $10, $20, $27, $48, $F9, $12, $27, $41, $F9, $41, $01, $1A, $E2, $44, $08
  .byte $80, $40, $48, $FC, $80, $10, $08, $FC, $24, $20, $20, $20, $22, $22, $1E, $00
  .byte $10, $10, $10, $10, $FD, $12, $35, $30, $58, $54, $91, $11, $11, $10, $17, $10
  .byte $40, $40, $A0, $A0, $10, $08, $F6, $04, $00, $48, $48, $50, $10, $04, $FE, $00
  .byte $00, $40, $37, $14, $85, $65, $25, $0D, $15, $15, $65, $21, $22, $24, $28, $20
  .byte $04, $04, $C4, $54, $54, $54, $54, $54, $54, $54, $54, $04, $84, $44, $54, $08
  .byte $01, $00, $1F, $10, $10, $10, $1F, $10, $10, $10, $17, $24, $24, $44, $87, $04
  .byte $00, $84, $FE, $04, $04, $04, $FC, $00, $00, $04, $FE, $04, $04, $04, $FC, $04
  .byte $00, $00, $7C, $00, $01, $FE, $10, $10, $20, $24, $42, $FE, $42, $01, $02, $04
  .byte $40, $40, $40, $44, $FE, $44, $44, $44, $44, $44, $84, $84, $84, $04, $28, $10
  .byte $00, $08, $08, $08, $08, $08, $08, $08, $08, $08, $14, $12, $21, $21, $42, $84
  .byte $00, $20, $20, $20, $20, $20, $20, $50, $50, $50, $50, $88, $88, $0C, $06, $04
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $18, $3C, $3C, $18, $00, $00, $00, $00
  .byte $01, $11, $11, $11, $1F, $21, $41, $01, $FF, $01, $02, $02, $04, $08, $10, $60
  .byte $00, $00, $00, $10, $F8, $00, $00, $04, $FE, $00, $80, $80, $40, $30, $0E, $04
  .byte $00, $7C, $44, $54, $54, $55, $56, $54, $54, $54, $54, $10, $28, $24, $45, $82
  .byte $40, $40, $44, $FE, $88, $08, $88, $88, $88, $50, $50, $20, $60, $90, $0E, $04
  .byte $04, $1F, $10, $12, $11, $FF, $10, $12, $21, $40, $3F, $24, $24, $24, $FF, $00
  .byte $10, $F8, $10, $10, $10, $FE, $10, $10, $50, $20, $F8, $48, $48, $48, $FE, $00
  .byte $10, $10, $10, $13, $FA, $14, $10, $1B, $30, $D0, $11, $10, $10, $10, $50, $23
  .byte $40, $20, $00, $FE, $02, $44, $40, $FE, $88, $88, $10, $90, $60, $50, $8C, $04
  .byte $08, $08, $0B, $10, $10, $30, $50, $9F, $10, $10, $10, $10, $10, $17, $10, $10
  .byte $00, $18, $E0, $40, $40, $40, $44, $FE, $40, $40, $40, $40, $48, $FC, $00, $00
  .byte $01, $3F, $08, $04, $FF, $00, $1F, $10, $1F, $10, $1F, $02, $51, $50, $90, $0F
  .byte $00, $F8, $20, $44, $FE, $00, $F0, $10, $F0, $10, $F0, $00, $84, $92, $12, $F0
  .byte $20, $27, $39, $41, $82, $7A, $24, $27, $F9, $21, $25, $22, $2A, $35, $28, $00
  .byte $10, $10, $7C, $14, $FE, $14, $7C, $10, $7C, $10, $10, $FE, $10, $10, $90, $7E
  .byte $10, $10, $17, $10, $F8, $17, $10, $19, $36, $D4, $14, $17, $14, $14, $57, $24
  .byte $08, $3C, $C0, $40, $44, $FE, $40, $40, $5C, $44, $44, $5C, $44, $44, $FC, $04
  .byte $04, $02, $01, $01, $01, $02, $02, $02, $04, $04, $08, $08, $10, $20, $40, $80
  .byte $00, $00, $00, $00, $00, $80, $80, $80, $40, $40, $20, $20, $10, $10, $0E, $04
  .byte $00, $00, $00, $FF, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $01, $02, $04, $08, $10, $10, $20, $20, $00, $00, $00, $00, $01, $0E, $30, $40
  .byte $80, $00, $08, $0C, $08, $0F, $CE, $C8, $00, $00, $01, $3E, $C0, $00, $00, $00
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00
  .byte $00, $00, $01, $01, $01, $01, $19, $19, $00, $00, $FC, $03, $00, $00, $00, $00
  .byte $00, $00, $00, $80, $00, $E0, $C0, $00, $00, $00, $00, $E0, $1C, $03, $00, $00
  .byte $00, $00, $00, $00, $00, $00, $00, $0F, $00, $00, $00, $00, $00, $80, $60, $18
  .byte $06, $01, $80, $C0, $C0, $04, $FE, $C0, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $00, $80, $40, $20, $20, $10, $10, $40, $40, $41, $46, $40, $41, $40, $40
  .byte $20, $20, $10, $11, $08, $04, $02, $01, $CF, $7F, $E9, $09, $49, $8B, $0A, $04
  .byte $08, $10, $60, $E0, $00, $00, $00, $80, $E0, $E0, $00, $80, $00, $00, $00, $00
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $19, $0F, $3D, $C1, $09, $31, $01, $00
  .byte $01, $02, $0C, $3C, $00, $00, $00, $00, $FC, $FC, $20, $30, $20, $60, $40, $80
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $08, $03, $03, $01, $00, $07, $18, $1B
  .byte $1B, $09, $08, $00, $00, $00, $00, $00, $60, $B0, $70, $C0, $7E, $87, $63, $32
  .byte $62, $86, $06, $0E, $00, $00, $01, $06, $10, $08, $08, $08, $08, $08, $08, $08
  .byte $10, $10, $20, $20, $40, $80
  .res 18, $00
  .byte $40, $30, $0E, $01, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $00, $00, $C0, $3E, $01, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $00, $00, $00, $03, $FC, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $00, $03, $1C, $E0, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $18, $60, $80
  .res 61, $00
data_CA0E:
  .byte $53, $4D, $44, $4F, $53, $4E, $50, $52, $54, $56, $58, $5A, $5C, $FF
data_CA1C:
  .byte $1E, $20, $26, $28, $18
data_CA21:
  .byte $22, $24, $26, $28, $18, $6A, $6A, $6A, $56, $58, $5A, $5C, $2A, $2C, $2E, $30
  .byte $6A, $6A, $6A, $FF, $C1, $C3, $C1, $C3, $2A, $2C, $2E, $30, $18, $14, $C1, $C3
  .byte $C1, $C3, $FF, $C1, $C3, $C1, $C3, $2A, $2C, $2E, $30, $18, $16, $C1, $C3, $C1
  .byte $C3, $FF, $C1, $C3, $C1, $C3, $46, $48, $2A, $2C, $2E, $30, $C1, $C3, $C1, $C3
  .byte $FF, $36, $38, $3A, $3C, $3E, $40, $42, $44, $32, $34, $FF, $46, $48, $4A, $4C
  .byte $3E, $40, $36, $38, $3A, $3C, $FF, $4E, $50, $52, $54, $5E, $60, $62, $64, $FF
  .byte $66, $68, $2A, $2C, $2E, $30, $14, $5E, $60, $62, $64, $6A, $6A, $6A, $FF, $66
  .byte $68, $2A, $2C, $2E, $30, $16, $5E, $60, $62, $64, $6A, $6A, $6A, $FF, $2A, $2C
  .byte $2E, $30, $88, $8A, $8C, $8E, $4E, $50, $52, $54, $74, $76, $FF, $78, $7A, $7C
  .byte $7E, $80, $82, $84, $86, $FF, $66, $68, $36, $38, $3A, $3C, $3E, $40, $42, $44
  .byte $32, $34, $5E, $60, $62, $64, $6A, $6A, $6A, $FF, $4E, $50, $52, $54, $5E, $60
  .byte $62, $64, $6C, $6E, $70, $72, $FF, $91, $93, $95, $97, $99, $9B, $9D, $9F, $FF
  .byte $A1, $A3, $A5, $A7, $A9, $AB, $AD, $AF, $FF, $B1, $B3, $B5, $B7, $B9, $BB, $BD
  .byte $BF, $FF

sub_CAF3:
  ldx #$00
  jsr sub_E9F9
  ldx #$C3
  ldy #$E6
  stx var_00FF
  sty var_00FE
  lda #$C5
  sta var_00FD
  ldy #$00
loc_CB06:
  ldx #$08
loc_CB08:
  lda (var_00FE),y
  iny
  sta var_2007
  dex
  bne loc_CB08
  lda #$FF
  ldx #$08
loc_CB15:
  sta var_2007
  dex
  bne loc_CB15
  tya
  bne loc_CB20
  inc var_00FF
loc_CB20:
  dec var_00FD
  bne loc_CB06
  ldx #$20
  jsr sub_E9F9
  ldx #$04
  lda #$6A
  ldy #$C0
loc_CB2F:
  sta var_2007
  dey
  bne loc_CB2F
  dex
  bne loc_CB2F
  lda #$00
  ldy #$40
loc_CB3C:
  sta var_2007
  dey
  bne loc_CB3C
  rts
  .byte $A2, $5A, $8E, $00, $80, $EC, $00, $80, $D0, $08, $A2, $A5, $8E, $00, $80, $EC
  .byte $00, $80, $60

sub_CB56:
  lda #$00
  sta var_4CFF
  sta var_2000
  sta var_6000
  sta var_2001
  sta var_4015
  sta var_4CFC
  lda #$01
  sta var_4CFB
  lda #$D6
  sta var_4CFE
  lda #$00
  sta var_4CFD
  sei
  cld
  ldx #$FF
  txs
  stx loc_FF2C
  dex
  stx loc_FF24
  lda #$00
  sta data_FF40
  sta data_FF50
  sta sub_FF01
  lda #$00
loc_CB92:
  sta sub_FF01+2
  ldx #$00
  jsr sub_E9F9
loc_CB9A:
  sta var_2007
  inx
  bne loc_CB9A
  sec
  ror a
  bcc loc_CB92
  lda #$00
loc_CBA6:
  sta sub_FF01+2
  ldx #$00
  jsr sub_E9F9
  bit var_2007
loc_CBB1:
  cmp var_2007
  bne loc_CBBB
  inx
  bne loc_CBB1
  beq loc_CBBF
loc_CBBB:
  sec
  ror a
  bcc loc_CBA6
loc_CBBF:
  eor #$FF
  clc
  adc #$01
  pha
  ldx #$04
  stx sub_FF01+2
  inx
  stx loc_FF0B
  jsr sub_C2FE
  jsr sub_C2E5
  jsr sub_CAF3
  ldx #$21
  ldy #$23
  jsr sub_E9FB
  ldx #$1A
  lda #$90
loc_CBE2:
  sta var_2007
  dex
  bne loc_CBE2
  jsr sub_C37A
  lda #$00
  sta var_00FC
  ldx #$CA
  ldy #$D8
  lda #$01
  jsr sub_C344
  lda #$02
  sta var_00FC
  ldx #$CA
  ldy #$E1
  lda #$01
  jsr sub_C344
  lda #$04
  sta var_00FC
  ldx #$CA
  ldy #$EA
  lda #$01
  jsr sub_C344
  lda #$07
  sta var_00FC
  ldx #$CA
  ldy #$13
  lda #$0C
  jsr sub_C344
  lda #$0A
  sta var_00FC
  lda #$40
  sta data_FFA0
  lda #$20
  sta sub_FF09
  pha
  ldx #$04
loc_CC30:
  lda data_CA1C,x
  sta var_0081,x
  dex
  bpl loc_CC30
  lda #$05
  sta var_00FD
  lda #$00
loc_CC3E:
  sta loc_FF04
  ldx #$00
loc_CC43:
  sta var_9A00,x
  inx
  bne loc_CC43
  sec
  ror a
  bcc loc_CC3E
  lda #$00
loc_CC4F:
  sta loc_FF04
  ldx #$00
loc_CC54:
  cmp var_9A00,x
  bne loc_CC5E
  inx
  bne loc_CC54
  beq loc_CC62
loc_CC5E:
  sec
  ror a
  bcc loc_CC4F
loc_CC62:
  sta var_0002
  pha
  sta var_0003
  lda #$00
  sta data_FFA0
  sta var_6000
  sta var_0000
loc_CC71:
  lda #$05
  sta var_0080
  ldx #$00
  lda var_0002
  sec
  sbc var_0003
  adc #$00
  bcc loc_CC81
  inx
loc_CC81:
  stx var_00FF
  ldx #$04
loc_CC85:
  asl a
  rol var_00FF
  dex
  bne loc_CC85
  ldx var_00FF
  tay
  lda #$1C
  jsr sub_C3BD
  lda #$80
  sta var_0001
  lda var_0002
  sta loc_FF04
  ldx #$18
  ldy #$00
loc_CCA0:
  sta (var_0000),y
  iny
  bne loc_CCA0
  inc var_0001
  dex
  bne loc_CCA0
  inc var_0002
  bne loc_CC71
  lda #$08
  sta data_FF90
  ldx #$58
  ldy #$00
  stx var_0001
  sty var_0000
  ldx #$08
  tya
loc_CCBE:
  sta (var_0000),y
  iny
  bne loc_CCBE
  inc var_0001
  dex
  bne loc_CCBE
  ldx #$C2
  stx var_0003
  sty var_0002
  ldx #$5A
  stx var_0001
  sty var_0000
  stx var_5FBA
  ldx #$02
  ldy #$29
  sty var_5FB9
loc_CCDE:
  tya
  bne loc_CCE8
  dex
  bmi loc_CCF0
  dec var_0001
  dec var_0003
loc_CCE8:
  dey
  lda (var_0002),y
  sta (var_0000),y
  jmp loc_CCDE
loc_CCF0:
  pla
  sta var_5FF9
  sta var_5FFB
  pla
  sta var_5FFE
  pla
  sta var_0200
  sta var_5FF5
  bit data_FF98
  php
  lda #$0C
  sta data_FF90
  ldx #$04
loc_CD0D:
  lda data_CA21,x
  sta var_0081,x
  dex
  bpl loc_CD0D
  ldy #$13
  sty var_00FD
  lda #$00
loc_CD1B:
  pha
  ldx #$05
  stx var_0080
  ldx #$00
  stx var_00FF
  clc
  adc #$01
  asl a
  tay
  lda #$00
  rol a
  tax
  cpy #$64
  bcc loc_CD35
  lda #$12
  sta var_00FD
loc_CD35:
  lda #$1C
  jsr sub_C3BD
  pla
  clc
  adc #$01
  cmp var_0200
  bne loc_CD1B
  lda #$0D
  sta var_00FC
  jsr sub_F78C
  plp
  bvs loc_CD52
  bit data_FF98
  bvs loc_CD55
loc_CD52:
  jmp loc_CDD7
loc_CD55:
  ldx #$CA
  ldy #$26
  lda #$09
  jsr sub_C344
  lda #$80
  pha
  lda #$00
  sta var_0001
  lda #$0C
  sta var_5FA3
  jsr sub_D8F9
  jsr sub_D449
  lda #$32
  jsr sub_D920
  bcs loc_CD8E
  jsr sub_DA6A
  lda #$00
  jsr sub_D920
  bcs loc_CD8E
  ldx #$CA
  ldy #$35
  lda #$09
  jsr sub_C344
  pla
  ora #$01
  pha
loc_CD8E:
  lda #$01
  sta var_0001
  lda #$0C
  sta var_5FA3
  jsr sub_D8F9
  lda #$32
  jsr sub_D920
  bcs loc_CDB8
  jsr sub_DA6A
  lda #$00
  jsr sub_D920
  bcs loc_CDB8
  ldx #$CA
  ldy #$44
  lda #$09
  jsr sub_C344
  pla
  ora #$02
  pha
loc_CDB8:
  pla
  sta var_5FA4
  and #$03
  bne loc_CDC9
  ldx #$CA
  ldy #$53
  lda #$09
  jsr sub_C344
loc_CDC9:
  inc var_00FC
  inc var_00FC
  inc var_00FC
  lda #$0C
  sta var_5FA3
  sta data_FF90
loc_CDD7:
  ldy #$00
loc_CDD9:
  lda var_4340
  cmp var_4348
  bne loc_CDEB
  cmp var_4350
  bne loc_CDEB
  cmp var_4358
  beq loc_CE17
loc_CDEB:
  dey
  bne loc_CDD9
  ldx #$CA
  ldy #$62
  lda #$05
  jsr sub_C344
  jsr sub_C287
  tax
  beq loc_CE06
  ldx #$CA
  ldy #$6F
  lda #$13
  jmp loc_CE0C
loc_CE06:
  ldx #$CA
  ldy #$6D
  lda #$11
loc_CE0C:
  jsr sub_C344
  lda #$80
  sta var_5FC0
  jmp loc_CE17
loc_CE17:
  lda #$FF
  sta loc_FF04
  sta var_5FFA
  lda #$FE
  sta var_5FF4
  lda #$15
  sta var_00FC
  ldx #$CA
  ldy #$78
  lda #$0C
  jsr sub_C344
  jsr sub_C2FE
  ldx #$22
  ldy #$E3
  jsr sub_E9FB
  ldx #$1A
  lda #$90
loc_CE3F:
  sta var_2007
  dex
  bne loc_CE3F
  stx var_6005
  jsr sub_D32C
  jsr sub_D377
  ldx #$59
  ldy #$F8
  jsr sub_E03A
  jsr sub_C37A
  lda #$0E
  sta var_6001
  lda #$80
  sta var_6000
  sta var_2000
  ldx #$21
  ldy #$2E
  stx var_5FE7
  sty var_5FE6
  jsr sub_E048
loc_CE72:
  lda #$C0
  sta var_5C40
  lda var_5FC0
  bpl loc_CEAB
  lda #$18
  sta var_00FC
  ldx #$CA
  ldy #$B7
  lda #$07
  jsr sub_C322
  jsr sub_C287
  tax
  beq loc_CEAB
  ora #$80
  sta var_5FC0
  lda #$02
  jsr sub_D146
  jsr sub_D21B
  lda #$00
  sta loc_59A4+1
  lda #$7C
  sta loc_59A4+2
  lda #$FF
  jmp loc_CF29
loc_CEAB:
  lda var_5FA4
  bpl loc_CF06
  and #$03
  beq loc_CF06
  ldx #$CA
  ldy #$90
  lsr a
  bcc loc_CEC0
  lsr a
  ldx #$CA
  ldy #$81
loc_CEC0:
  and #$01
  pha
  lda #$18
  sta var_00FC
  lda #$09
  jsr sub_C322
  pla
  sta var_0081
  ldx #$00
  stx var_0082
  stx var_0083
  lda #$04
  sta var_0087
  stx var_0086
  inx
  stx var_0084
  stx var_0088
  lda #$03
  sta var_5FB6
loc_CEE5:
  lda #$02
  sta var_0080
  ldx #$00
  ldy #$80
  jsr sub_D3D6
  tax
  beq loc_CF0C
  dec var_5FB6
  beq loc_CF06
  lda #$00
  sta var_0080
  ldx #$00
  ldy #$80
  jsr sub_D3D6
  jmp loc_CEE5
loc_CF06:
  ldy #$CB
  ldx #$CA
  bne loc_CF3F
loc_CF0C:
  lda var_0403,x
  cmp data_CA0E,x
  bne loc_CF3B
  inx
  cpx #$05
  bne loc_CF0C
  lda var_0401
  clc
  adc #$02
  sta loc_59A4+1
  lda #$04
  sta loc_59A4+2
  lda var_0081
loc_CF29:
  pha
  jsr sub_EA0E
  lda #$00
  sta var_5C40
  pla
  jsr sub_59A1
  lda #$C0
  sta var_5C40
loc_CF3B:
  ldy #$9F
  ldx #$CA
loc_CF3F:
  lda #$18
  sta var_00FC
  lda #$05
  jsr sub_C322
  lda #$18
  sta var_00FC
  ldx #$CA
  ldy #$AE
  lda #$13
  jsr sub_C344
  jsr sub_D09C
  lda #$00
  sta var_5C40
  ldy #$00
  ldx #$17
  jsr sub_DB23
  jmp loc_CE72

sub_CF67:
  txa
  pha
  tya
  pha
  lda #$00
  sta var_2001
  ldy var_5FC2
  ldx #$1E
  bit var_2002
  lda var_00FF
  pha
loc_CF7B:
  lda var_5D00,y
  sta var_00FF
  iny
  bit var_00FF
  bpl loc_CFF7
  bvc loc_CFDF
  pha
  and #$3F
  sta var_2006
  lda var_5D00,y
  sta var_2006
  sta var_5FC2
  iny
  lda var_5D00,y
  sta var_00FF
  pha
loc_CF9D:
  iny
  lda var_5D00,y
  sta var_2007
  dec var_00FF
  beq loc_CFDA
  iny
  lda var_5D00,y
  sta var_2007
  dec var_00FF
  beq loc_CFDA
  iny
  lda var_5D00,y
  sta var_2007
  dec var_00FF
  beq loc_CFDA
  dex
  bne loc_CF9D
  lda var_00FF
  sta var_5D00,y
  dey
  pla
  sec
  sbc var_00FF
  clc
  adc var_5FC2
  sta var_5D00,y
  dey
  pla
  sta var_5D00,y
  jmp loc_D071
loc_CFDA:
  pla
  pla
  jmp loc_D05A
loc_CFDF:
  lsr a
  bcs loc_CFEF
  lda var_6000
  and #$FC
  ora var_5D00,y
  sta var_6000
  bcc loc_D05A
loc_CFEF:
  lda var_5D00,y
  sta var_5FC5
  bcs loc_D05A
loc_CFF7:
  bvc loc_D046
  and #$3F
  sta var_2006
  lda var_5D00,y
  sta var_2006
  iny
  lda var_5D00,y
  pha
  iny
  sty var_00FF
  lda var_5D00,y
  tay
  pla
loc_D011:
  sta var_2007
  dey
  beq loc_D058
  sta var_2007
  dey
  beq loc_D058
  sta var_2007
  dey
  beq loc_D058
  sta var_2007
  dey
  beq loc_D058
  dex
  bne loc_D011
  tya
  ldy var_00FF
  ldx var_5D00,y
  sta var_5D00,y
  txa
  sec
  sbc var_5D00,y
  dey
  dey
  clc
  adc var_5D00,y
  sta var_5D00,y
  dey
  bcc loc_D071
loc_D046:
  sta var_2006
  lda var_5D00,y
  sta var_2006
  iny
  lda var_5D00,y
  sta var_2007
  bvc loc_D05A
loc_D058:
  ldy var_00FF
loc_D05A:
  iny
  cpy var_5FC3
  beq loc_D066
  dex
  beq loc_D071
  jmp loc_CF7B
loc_D066:
  lda var_6000
  and #$FC
  ora var_5FC4
  sta var_6000
loc_D071:
  sty var_5FC2
  pla
  sta var_00FF
  lda #$00
  sta var_2005
  lda var_5FC5
  sta var_2005
  lda var_6000
  sta var_2000
  lda var_6001
  sta var_2001
  pla
  tay
  pla
  tax
  rts

sub_D093:
  lda var_6000
  and #$7F
  sta var_2000
  rts

sub_D09C:
  lda var_6000
  ora #$80
  sta var_2000
  rts
data_D0A5:
data_D0A6 = * + 1
  .word sub_D2D3 - 1, sub_59AA - 1
  .word sub_59C0 - 1, sub_D32C - 1
  .word sub_D377 - 1, sub_DDA7 - 1
  .word sub_D3D6 - 1, sub_FF31 - 1
  .word sub_FF29 - 1, sub_DA9C - 1
  .word sub_DB1F - 1, sub_DB23 - 1
  .word sub_DC0F - 1, sub_D251 - 1
  .word sub_DD3B - 1, sub_DD49 - 1
  .word sub_FF21 - 1, sub_DC80 - 1
  .word sub_DE83 - 1, sub_DF71 - 1
  .word sub_DF7B - 1, sub_DFEB - 1
  .word sub_E03A - 1, sub_E041 - 1
  .word sub_F6C0 - 1, sub_DAF1 - 1
  .word sub_DB1C - 1, sub_E0A4 - 1
  .word sub_E1E5 - 1, sub_E1A1 - 1
  .word sub_E1EC - 1, sub_DADE - 1
  .word sub_D2E8 - 1, sub_D2FE - 1
  .word sub_E09B - 1, sub_D145 - 1
  .word sub_D17D - 1, sub_EB5A - 1
  .word sub_FF19 - 1, sub_D12E - 1
  .word sub_EAC2 - 1, sub_EAD6 - 1
  .word sub_EADD - 1

sub_D0FB:
  cmp #$3C
  beq loc_D12B
  cmp #$42
  bcs loc_D109
  cmp #$3F
  bcc loc_D109
  sbc #$17
loc_D109:
  asl a
  bcs loc_D120
  cmp #$56
  bcs loc_D12A
  stx var_5FB2
  tax
  lda data_D0A6,x
  pha
  lda data_D0A5,x
  pha
  ldx var_5FB2
  rts
loc_D120:
  cmp #$FE
  bne loc_D12A
  lda #$40
  ldx #$61
  ldy #$01
loc_D12A:
  rts
loc_D12B:
  jmp sub_EA0E

sub_D12E:
  stx data_597F
  sty data_597E
  ldx var_5FFA
  stx var_5FBF
  ldx #$59
  ldy #$62
  stx data_58B0
  sty data_58AF
  rts

sub_D145:
  txa

sub_D146:
  bit var_4348
  bpl sub_D146
  pha
  asl a
  asl a
  asl a
  asl a
  sta var_4340
  lda #$00
  sta var_4348
loc_D158:
  bit var_4348
  bmi loc_D158
  lda #$80
  sta var_4348
loc_D162:
  bit var_4348
  bpl loc_D162
  pla
  and #$F0
  sta var_4340
  lda #$00
  sta var_4348
loc_D172:
  bit var_4348
  bmi loc_D172
  lda #$80
  sta var_4348
  rts

sub_D17D:
  bit var_4348
  bpl sub_D17D
  lda var_4340
  pha
  lda #$00
  sta var_4348
loc_D18B:
  bit var_4348
  bmi loc_D18B
  lda #$80
  sta var_4348
  pla
  rts

sub_D197:
  jsr sub_D17D
  tay
  jsr sub_D17D
  tax
  beq loc_D1A2
  iny
loc_D1A2:
  lda var_00FD
  pha
  sty var_00FD
  jsr sub_D17D
  sta var_00FF
  jsr sub_D17D
  tay
  lda #$00
  sta var_00FE
  lda var_5FFA
  sta loc_FF04
loc_D1BA:
  lda (var_00FE),y
  jsr sub_D146
  iny
  bne loc_D1C4
  inc var_00FF
loc_D1C4:
  dex
  bne loc_D1BA
  dec var_00FD
  bne loc_D1BA
  pla
  sta var_00FD
  rts

sub_D1CF:
  jsr sub_D17D
  tay
  jsr sub_D17D
  tax
  beq loc_D1DA
  iny
loc_D1DA:
  lda var_00FD
  pha
  sty var_00FD
  jsr sub_D17D
  sta var_00FF
  jsr sub_D17D
  tay
  lda #$00
  sta var_00FE
  lda var_5FFA
  sta loc_FF04
loc_D1F2:
  bit var_4348
  bpl loc_D1F2
  lda #$00
  sta var_4348
loc_D1FC:
  bit var_4348
  bmi loc_D1FC
  lda var_4340
  sta (var_00FE),y
  lda #$80
  sta var_4348
  iny
  bne loc_D210
  inc var_00FF
loc_D210:
  dex
  bne loc_D1F2
  dec var_00FD
  bne loc_D1F2
  pla
  sta var_00FD
  rts

sub_D21B:
  jsr sub_D17D
  cmp #$08
  bne loc_D22C
  jsr sub_D17D
  tax
  jsr sub_59AA
  jmp sub_D21B
loc_D22C:
  cmp #$0B
  bne loc_D236
  jsr sub_D1CF
  jmp sub_D21B
loc_D236:
  cmp #$15
  bne loc_D240
  jsr sub_D197
  jmp sub_D21B
loc_D240:
  cmp #$0A
  bne loc_D24D
  jsr sub_59C0
  jsr sub_D146
  jmp sub_D21B
loc_D24D:
  tax
  bne sub_D21B
  rts

sub_D251:
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
  jsr sub_D17D
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

sub_D2D3:
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
  jsr sub_59AA
  tax
  rts

sub_D2E8:
  lda var_00FF
  sta var_5FB2
  jsr sub_D2D4
  lda (var_00FE),y
  stx var_5FFA
  stx loc_FF04
  ldx var_5FB2
  stx var_00FF
  rts

sub_D2FE:
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

sub_D32C:
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
  stx sub_FF19+2
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

sub_D377:
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
  jsr sub_E03A
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

sub_D3D6:
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
  jsr sub_E09B
  ldx #$01
  jsr sub_DD3B
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

sub_DA9C:
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

sub_DADE:
  cpx #$FE
  bne loc_DAE5
  ldx var_5FF9
loc_DAE5:
  stx var_5FBE
  jsr sub_DA9C
  lda #$C0
  sta var_5FBD
  rts

sub_DAF1:
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

sub_DB1C:
  lda #$00
  rts

sub_DB1F:
  lda var_5FF5
  rts

sub_DB23:
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
  jsr sub_DD3B
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

sub_DC0F:
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

sub_DC80:
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

sub_DD3B:
  stx var_5FF8
  jsr sub_D09C
loc_DD41:
  lda var_5FF8
  bne loc_DD41
  jmp sub_FF11

sub_DD49:
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

sub_DDA7:
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
  sta sub_FF19+1
  txa
  clc
  adc #$05
  tax
  jmp loc_DE55
loc_DE72:
  stx var_5FBB
  sta sub_FF21+1
  lda var_5FFF
  ora #$44
  sta var_5FFF
  pla
  tax
  rts

sub_DE83:
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
  jsr sub_DD3B
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

sub_DF71:
  txa
  asl a
  asl a
  asl a
  tax
  tya
  sta sub_FF01+2,x
  rts

sub_DF7B:
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

sub_DFEB:
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

sub_E03A:
  stx data_58B4
  sty data_58B3
  rts

sub_E041:
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

sub_E09B:
  lda var_5FC2
  cmp var_5FC3
  bne sub_E09B
  rts

sub_E0A4:
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

sub_E1A1:
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

sub_E1E5:
  stx var_5FF7
  sty var_5FF6
  rts

sub_E1EC:
  ldx var_5FF7
  ldy var_5FF6
  lda #$1E
  rts
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $7E, $81, $A5, $81, $BD, $99, $81, $7E
  .byte $7E, $FF, $DB, $FF, $C3, $E7, $FF, $7E, $6C, $FE, $FE, $FE, $7C, $38, $10, $00
  .byte $10, $38, $7C, $FE, $7C, $38, $10, $00, $38, $7C, $38, $FE, $FE, $7C, $38, $7C
  .byte $10, $10, $38, $7C, $FE, $7C, $38, $7C, $00, $00, $18, $3C, $3C, $18, $00, $00
  .byte $FF, $FF, $E7, $C3, $C3, $E7, $FF, $FF, $00, $3C, $66, $42, $42, $66, $3C, $00
  .byte $FF, $C3, $99, $BD, $BD, $99, $C3, $FF, $0F, $07, $0F, $7D, $CC, $CC, $CC, $78
  .byte $3C, $66, $66, $66, $3C, $18, $7E, $18, $3F, $33, $3F, $30, $30, $70, $F0, $E0
  .byte $7F, $63, $7F, $63, $63, $67, $E6, $C0, $99, $5A, $3C, $E7, $E7, $3C, $5A, $99
  .byte $80, $E0, $F8, $FE, $F8, $E0, $80, $00, $02, $0E, $3E, $FE, $3E, $0E, $02, $00
  .byte $18, $3C, $7E, $18, $18, $7E, $3C, $18, $66, $66, $66, $66, $66, $00, $66, $00
  .byte $7F, $DB, $DB, $7B, $1B, $1B, $1B, $00, $3E, $63, $38, $6C, $6C, $38, $CC, $78
  .byte $00, $00, $00, $00, $7E, $7E, $7E, $00, $18, $3C, $7E, $18, $7E, $3C, $18, $FF
  .byte $18, $3C, $7E, $18, $18, $18, $18, $00, $18, $18, $18, $18, $7E, $3C, $18, $00
  .byte $00, $18, $0C, $FE, $0C, $18, $00, $00, $00, $30, $60, $FE, $60, $30, $00, $00
  .byte $00, $00, $C0, $C0, $C0, $FE, $00, $00, $00, $24, $66, $FF, $66, $24, $00, $00
  .byte $00, $18, $3C, $7E, $FF, $FF, $00, $00, $00, $FF, $FF, $7E, $3C, $18, $00, $00
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $30, $78, $78, $78, $30, $00, $30, $00
  .byte $6C, $6C, $6C, $00, $00, $00, $00, $00, $6C, $6C, $FE, $6C, $FE, $6C, $6C, $00
  .byte $30, $7C, $C0, $78, $0C, $F8, $30, $00, $00, $C6, $CC, $18, $30, $66, $C6, $00
  .byte $38, $6C, $38, $76, $DC, $CC, $76, $00, $60, $60, $C0, $00, $00, $00, $00, $00
  .byte $18, $30, $60, $60, $60, $30, $18, $00, $60, $30, $18, $18, $18, $30, $60, $00
  .byte $00, $66, $3C, $FF, $3C, $66, $00, $00, $00, $30, $30, $FC, $30, $30, $00, $00
  .byte $00, $00, $00, $00, $00, $30, $30, $60, $00, $00, $00, $FC, $00, $00, $00, $00
  .byte $00, $00, $00, $00, $00, $30, $30, $00, $06, $0C, $18, $30, $60, $C0, $80, $00
  .byte $7C, $C6, $CE, $DE, $F6, $E6, $7C, $00, $30, $70, $30, $30, $30, $30, $FC, $00
  .byte $78, $CC, $0C, $38, $60, $CC, $FC, $00, $78, $CC, $0C, $38, $0C, $CC, $78, $00
  .byte $1C, $3C, $6C, $CC, $FE, $0C, $1E, $00, $FC, $C0, $F8, $0C, $0C, $CC, $78, $00
  .byte $38, $60, $C0, $F8, $CC, $CC, $78, $00, $FC, $CC, $0C, $18, $30, $30, $30, $00
  .byte $78, $CC, $CC, $78, $CC, $CC, $78, $00, $78, $CC, $CC, $7C, $0C, $18, $70, $00
  .byte $00, $30, $30, $00, $00, $30, $30, $00, $00, $30, $30, $00, $00, $30, $30, $60
  .byte $18, $30, $60, $C0, $60, $30, $18, $00, $00, $00, $FC, $00, $00, $FC, $00, $00
  .byte $60, $30, $18, $0C, $18, $30, $60, $00, $78, $CC, $0C, $18, $30, $00, $30, $00
  .byte $7C, $C6, $DE, $DE, $DE, $C0, $78, $00, $30, $78, $CC, $CC, $FC, $CC, $CC, $00
  .byte $FC, $66, $66, $7C, $66, $66, $FC, $00, $3C, $66, $C0, $C0, $C0, $66, $3C, $00
  .byte $F8, $6C, $66, $66, $66, $6C, $F8, $00, $7E, $60, $60, $78, $60, $60, $7E, $00
  .byte $7E, $60, $60, $78, $60, $60, $60, $00, $3C, $66, $C0, $C0, $CE, $66, $3E, $00
  .byte $CC, $CC, $CC, $FC, $CC, $CC, $CC, $00, $78, $30, $30, $30, $30, $30, $78, $00
  .byte $1E, $0C, $0C, $0C, $CC, $CC, $78, $00, $E6, $66, $6C, $78, $6C, $66, $E6, $00
  .byte $60, $60, $60, $60, $60, $60, $7E, $00, $C6, $EE, $FE, $FE, $D6, $C6, $C6, $00
  .byte $C6, $E6, $F6, $DE, $CE, $C6, $C6, $00, $38, $6C, $C6, $C6, $C6, $6C, $38, $00
  .byte $FC, $66, $66, $7C, $60, $60, $F0, $00, $78, $CC, $CC, $CC, $DC, $78, $1C, $00
  .byte $FC, $66, $66, $7C, $6C, $66, $E6, $00, $78, $CC, $E0, $70, $1C, $CC, $78, $00
  .byte $FC, $30, $30, $30, $30, $30, $30, $00, $CC, $CC, $CC, $CC, $CC, $CC, $FC, $00
  .byte $CC, $CC, $CC, $CC, $CC, $78, $30, $00, $C6, $C6, $C6, $D6, $FE, $EE, $C6, $00
  .byte $C6, $C6, $6C, $38, $38, $6C, $C6, $00, $CC, $CC, $CC, $78, $30, $30, $78, $00
  .byte $FE, $06, $0C, $18, $30, $60, $FE, $00, $78, $60, $60, $60, $60, $60, $78, $00
  .byte $C0, $60, $30, $18, $0C, $06, $02, $00, $78, $18, $18, $18, $18, $18, $78, $00
  .byte $10, $38, $6C, $C6, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $FF
  .byte $30, $30, $18, $00, $00, $00, $00, $00, $00, $00, $78, $0C, $7C, $CC, $76, $00
  .byte $E0, $60, $60, $7C, $66, $66, $DC, $00, $00, $00, $78, $CC, $C0, $CC, $78, $00
  .byte $1C, $0C, $0C, $7C, $CC, $CC, $76, $00, $00, $00, $78, $CC, $FC, $C0, $78, $00
  .byte $38, $6C, $60, $F0, $60, $60, $F0, $00, $00, $00, $76, $CC, $CC, $7C, $0C, $F8
  .byte $E0, $60, $6C, $76, $66, $66, $E6, $00, $30, $00, $70, $30, $30, $30, $78, $00
  .byte $0C, $00, $0C, $0C, $0C, $CC, $CC, $78, $E0, $60, $66, $6C, $78, $6C, $E6, $00
  .byte $70, $30, $30, $30, $30, $30, $78, $00, $00, $00, $CC, $FE, $FE, $D6, $C6, $00
  .byte $00, $00, $F8, $CC, $CC, $CC, $CC, $00, $00, $00, $78, $CC, $CC, $CC, $78, $00
  .byte $00, $00, $DC, $66, $66, $7C, $60, $F0, $00, $00, $76, $CC, $CC, $7C, $0C, $1E
  .byte $00, $00, $DC, $76, $66, $60, $F0, $00, $00, $00, $7C, $C0, $78, $0C, $F8, $00
  .byte $10, $30, $7C, $30, $30, $34, $18, $00, $00, $00, $CC, $CC, $CC, $CC, $76, $00
  .byte $00, $00, $CC, $CC, $CC, $78, $30, $00, $00, $00, $C6, $D6, $FE, $FE, $6C, $00
  .byte $00, $00, $C6, $6C, $38, $6C, $C6, $00, $00, $00, $CC, $CC, $CC, $7C, $0C, $F8
  .byte $00, $00, $FC, $98, $30, $64, $FC, $00, $1C, $30, $30, $E0, $30, $30, $1C, $00
  .byte $18, $18, $18, $00, $18, $18, $18, $00, $E0, $30, $30, $1C, $30, $30, $E0, $00
  .byte $76, $DC, $00, $00, $00, $00, $00, $00, $00, $10, $38, $6C, $C6, $C6, $FE, $00
  .byte $78, $CC, $C0, $CC, $78, $18, $0C, $78, $00, $CC, $00, $CC, $CC, $CC, $7E, $00
  .byte $1C, $00, $78, $CC, $FC, $C0, $78, $00, $7E, $C3, $3C, $06, $3E, $66, $3F, $00
  .byte $CC, $00, $78, $0C, $7C, $CC, $7E, $00, $E0, $00, $78, $0C, $7C, $CC, $7E, $00
  .byte $30, $30, $78, $0C, $7C, $CC, $7E, $00, $00, $00, $78, $C0, $C0, $78, $0C, $38
  .byte $7E, $C3, $3C, $66, $7E, $60, $3C, $00, $CC, $00, $78, $CC, $FC, $C0, $78, $00
  .byte $E0, $00, $78, $CC, $FC, $C0, $78, $00, $CC, $00, $70, $30, $30, $30, $78, $00
  .byte $7C, $C6, $38, $18, $18, $18, $3C, $00, $E0, $00, $70, $30, $30, $30, $78, $00
  .byte $C6, $38, $6C, $C6, $FE, $C6, $C6, $00, $30, $30, $00, $78, $CC, $FC, $CC, $00
  .byte $1C, $00, $FC, $60, $78, $60, $FC, $00, $00, $00, $7F, $0C, $7F, $CC, $7F, $00
  .byte $3E, $6C, $CC, $FE, $CC, $CC, $CE, $00, $78, $CC, $00, $78, $CC, $CC, $78, $00
  .byte $00, $CC, $00, $78, $CC, $CC, $78, $00, $00, $E0, $00, $78, $CC, $CC, $78, $00
  .byte $78, $CC, $00, $CC, $CC, $CC, $7E, $00, $00, $E0, $00, $CC, $CC, $CC, $7E, $00
  .byte $00, $CC, $00, $CC, $CC, $7C, $0C, $F8, $C3, $18, $3C, $66, $66, $3C, $18, $00
  .byte $CC, $00, $CC, $CC, $CC, $CC, $78, $00, $18, $18, $7E, $C0, $C0, $7E, $18, $18
  .byte $38, $6C, $64, $F0, $60, $E6, $FC, $00, $CC, $CC, $78, $FC, $30, $FC, $30, $30
  .byte $F8, $CC, $CC, $FA, $C6, $CF, $C6, $C7, $0E, $1B, $18, $3C, $18, $18, $D8, $70
  .byte $1C, $00, $78, $0C, $7C, $CC, $7E, $00, $38, $00, $70, $30, $30, $30, $78, $00
  .byte $00, $1C, $00, $78, $CC, $CC, $78, $00, $00, $1C, $00, $CC, $CC, $CC, $7E, $00
  .byte $00, $F8, $00, $F8, $CC, $CC, $CC, $00, $FC, $00, $CC, $EC, $FC, $DC, $CC, $00
  .byte $3C, $6C, $6C, $3E, $00, $7E, $00, $00, $38, $6C, $6C, $38, $00, $7C, $00, $00
  .byte $30, $00, $30, $60, $C0, $CC, $78, $00, $00, $00, $00, $FC, $C0, $C0, $00, $00
  .byte $00, $00, $00, $FC, $0C, $0C, $00, $00, $C3, $C6, $CC, $DE, $33, $66, $CC, $0F
  .byte $C3, $C6, $CC, $DB, $37, $6F, $CF, $03, $18, $18, $00, $18, $18, $18, $18, $00
  .byte $00, $33, $66, $CC, $66, $33, $00, $00, $00, $CC, $66, $33, $66, $CC, $00, $00
  .byte $22, $88, $22, $88, $22, $88, $22, $88, $55, $AA, $55, $AA, $55, $AA, $55, $AA
  .byte $DB, $77, $DB, $EE, $DB, $77, $DB, $EE, $18, $18, $18, $18, $18, $18, $18, $18
  .byte $18, $18, $18, $18, $F8, $18, $18, $18, $18, $18, $F8, $18, $F8, $18, $18, $18
  .byte $36, $36, $36, $36, $F6, $36, $36, $36, $00, $00, $00, $00, $FE, $36, $36, $36
  .byte $00, $00, $F8, $18, $F8, $18, $18, $18, $36, $36, $F6, $06, $F6, $36, $36, $36
  .byte $36, $36, $36, $36, $36, $36, $36, $36, $00, $00, $FE, $06, $F6, $36, $36, $36
  .byte $36, $36, $F6, $06, $FE, $00, $00, $00, $36, $36, $36, $36, $FE, $00, $00, $00
  .byte $18, $18, $F8, $18, $F8, $00, $00, $00, $00, $00, $00, $00, $F8, $18, $18, $18
  .byte $18, $18, $18, $18, $1F, $00, $00, $00, $18, $18, $18, $18, $FF, $00, $00, $00
  .byte $00, $00, $00, $00, $FF, $18, $18, $18, $18, $18, $18, $18, $1F, $18, $18, $18
  .byte $00, $00, $00, $00, $FF, $00, $00, $00, $18, $18, $18, $18, $FF, $18, $18, $18
  .byte $18, $18, $1F, $18, $1F, $18, $18, $18, $36, $36, $36, $36, $37, $36, $36, $36
  .byte $36, $36, $37, $30, $3F, $00, $00, $00, $00, $00, $3F, $30, $37, $36, $36, $36
  .byte $36, $36, $F7, $00, $FF, $00, $00, $00, $00, $00, $FF, $00, $F7, $36, $36, $36
  .byte $36, $36, $37, $30, $37, $36, $36, $36, $00, $00, $FF, $00, $FF, $00, $00, $00
  .byte $36, $36, $F7, $00, $F7, $36, $36, $36, $18, $18, $FF, $00, $FF, $00, $00, $00
  .byte $36, $36, $36, $36, $FF, $00, $00, $00, $00, $00, $FF, $00, $FF, $18, $18, $18
  .byte $00, $00, $00, $00, $FF, $36, $36, $36, $36, $36, $36, $36, $3F, $00, $00, $00
  .byte $18, $18, $1F, $18, $1F, $00, $00, $00, $00, $00, $1F, $18, $1F, $18, $18, $18
  .byte $00, $00, $00, $00, $3F, $36, $36, $36, $36, $36, $36, $36, $FF, $36, $36, $36
  .byte $18, $18, $FF, $18, $FF, $18, $18, $18, $18, $18, $18, $18, $F8, $00, $00, $00
  .byte $00, $00, $00, $00, $1F, $18, $18, $18, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
  .byte $00, $00, $00, $00, $FF, $FF, $FF, $FF, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $FF, $FF, $FF, $FF, $00, $00, $00, $00
  .byte $00, $00, $76, $DC, $C8, $DC, $76, $00, $00, $78, $CC, $F8, $CC, $F8, $C0, $C0
  .byte $00, $FC, $CC, $C0, $C0, $C0, $C0, $00, $00, $FE, $6C, $6C, $6C, $6C, $6C, $00
  .byte $FC, $CC, $60, $30, $60, $CC, $FC, $00, $00, $00, $7E, $D8, $D8, $D8, $70, $00
  .byte $00, $66, $66, $66, $66, $7C, $60, $C0, $00, $76, $DC, $18, $18, $18, $18, $00
  .byte $FC, $30, $78, $CC, $CC, $78, $30, $FC, $38, $6C, $C6, $FE, $C6, $6C, $38, $00
  .byte $38, $6C, $C6, $C6, $6C, $6C, $EE, $00, $1C, $30, $18, $7C, $CC, $CC, $78, $00
  .byte $00, $00, $7E, $DB, $DB, $7E, $00, $00, $06, $0C, $7E, $DB, $DB, $7E, $60, $C0
  .byte $38, $60, $C0, $F8, $C0, $60, $38, $00, $78, $CC, $CC, $CC, $CC, $CC, $CC, $00
  .byte $00, $FC, $00, $FC, $00, $FC, $00, $00, $30, $30, $FC, $30, $30, $00, $FC, $00
  .byte $60, $30, $18, $30, $60, $00, $FC, $00, $18, $30, $60, $30, $18, $00, $FC, $00
  .byte $0E, $1B, $1B, $18, $18, $18, $18, $18, $18, $18, $18, $18, $18, $D8, $D8, $70
  .byte $30, $30, $00, $FC, $00, $30, $30, $00, $00, $76, $DC, $00, $76, $DC, $00, $00
  .byte $38, $6C, $6C, $38, $00, $00, $00, $00, $00, $00, $00, $18, $18, $00, $00, $00
  .byte $00, $00, $00, $00, $18, $00, $00, $00, $0F, $0C, $0C, $0C, $EC, $6C, $3C, $1C
  .byte $78, $6C, $6C, $6C, $6C, $00, $00, $00, $70, $18, $30, $60, $78, $00, $00, $00
  .byte $00, $00, $3C, $3C, $3C, $3C, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
data_E9F5:
  .byte $0E, $20, $20, $30

sub_E9F9:
  ldy #$00

sub_E9FB:
  bit var_2002
  stx var_2006
  sty var_2006
  rts

sub_EA05:
  lda #$00

sub_EA07:
  sta var_2007
  dey
  bne sub_EA07
  rts

sub_EA0E:
  lda #$00
  sta var_6000
  sta var_2000
  sta var_2001
  jsr sub_D350
  lda var_5FFF
  and #$BB
  ora #$01
  sta var_5FFF
  sta sub_FF01
  jsr sub_D33E
  ldx #$00
  jsr sub_E9F9
  ldx #$E1
  ldy #$F5
  stx var_00FF
  sty var_00FE
  ldx #$00
  ldy #$00
loc_EA3D:
  lda (var_00FE),y
  sta var_2007
  iny
  cpy #$08
  bne loc_EA3D
  jsr sub_EA05
  lda var_00FE
  clc
  adc #$08
  sta var_00FE
  bcc loc_EA55
  inc var_00FF
loc_EA55:
  dex
  bne loc_EA3D
  ldx #$20
  jsr sub_E9F9
  ldx #$04
  ldy #$C0
  lda #$20
loc_EA63:
  jsr sub_EA07
  dex
  bne loc_EA63
  ldy #$40
  jsr sub_EA05
  ldx #$28
  jsr sub_E9F9
  ldx #$04
  ldy #$C0
  lda #$20
loc_EA79:
  jsr sub_EA07
  dex
  bne loc_EA79
  ldy #$40
  jsr sub_EA05
  ldx #$01
  jsr sub_DD3B
  ldx #$3F
  jsr sub_E9F9
  ldy #$00
loc_EA90:
  lda data_E9F5,y
  sta var_2007
  iny
  cpy #$04
  bne loc_EA90
  ldx #$20
  jsr sub_E9F9
  lda #$00
  sta var_2005
  sta var_2005
  sta var_5FF7
  sta var_5FF6
  lda #$0C
  sta var_6001
  sta var_2001
  bit var_2002
  lda #$80
  sta var_6000
  sta var_2000
  rts

sub_EAC2:
  lda var_5FE6
  sta var_00FF
  lda var_5FEA
  sta var_00FE
  lda var_5FE7
  ldx var_5FE9
  ldy var_5FE8
  rts

sub_EAD6:
  stx var_5FE7
  sty var_5FE6
  rts

sub_EADD:
  stx var_5FE9
  sty var_5FE8
  lda var_00FF
  sta var_5FEA
  rts

sub_EAE9:
  txa
  pha
  ldx var_5FEA
  inx
  cpx #$64
  bcc loc_EB54
  ldx var_5FE8
  inx
  txa
  and #$1F
  cmp #$1E
  bne sub_EB4F
  txa
  and #$E0
  adc #$1F
  tax
  bcc loc_EB0C
  inc var_5FE9
  jmp sub_EB4F
loc_EB0C:
  bpl sub_EB4F
  lda var_5FE9
  and #$07
  cmp #$07
  bne sub_EB4F
  ldx var_5FE9
  inx
  cpx #$C0
  bne loc_EB4A
  ldx var_5FE6
  inx
  txa
  and #$1F
  cmp #$1F
  bne loc_EB45
  txa
  and #$E0
  adc #$20
  tax
  bcc loc_EB38
  inc var_5FE7
  jmp loc_EB45
loc_EB38:
  bpl loc_EB45
  lda var_5FE7
  lsr a
  bcc loc_EB45
  inc var_5FE7
  ldx #$21
loc_EB45:
  stx var_5FE6
  ldx #$00
loc_EB4A:
  stx var_5FE9
  ldx #$00

sub_EB4F:
  stx var_5FE8
  ldx #$00
loc_EB54:
  stx var_5FEA
  pla
  tax
  rts

sub_EB5A:
  sty var_5FB2
  ldy #$2F
loc_EB5F:
  lda a:var_00D0,y
  sta var_6100,y
  dey
  bpl loc_EB5F
  lda var_0000,x
  sta var_00DE
  lda var_0001,x
  sta var_00DC
  lda var_0002,x
  sta var_00DD
  lda var_0003,x
  sta var_00E1
  sta var_5FB3
  ldx #$80
  ldy #$00
  stx var_00E0
  sty var_00DF
  lda var_5FFA
  pha
  jsr sub_F2FB
  bit var_5FB2
  bpl loc_EBD3
  lda #$FF
  sta var_00D0
  sta var_00D1
  sta var_00D2
  sta var_00D3
  jsr sub_EF5D
loc_EB9C:
  ldx #$40
  ldy #$00
  lda var_5FB3
  cmp var_00E1
  bcc loc_EBB4
  bne loc_EBC0
  lda var_00E0
  and #$7F
  tax
  ora var_00DF
  beq loc_EBC0
  ldy var_00DF
loc_EBB4:
  lda var_5FB3
  jsr sub_EBE5
  inc var_5FB3
  jmp loc_EB9C
loc_EBC0:
  ldx #$03
loc_EBC2:
  lda var_00D0,x
  eor #$FF
  cmp var_00D4,x
  bne loc_EBD1
  dex
  bpl loc_EBC2
  ldy #$00
  beq loc_EBD3
loc_EBD1:
  ldy #$01
loc_EBD3:
  ldx #$2F
loc_EBD5:
  lda var_6100,x
  sta var_00D0,x
  dex
  bpl loc_EBD5
  pla
  sta var_5FFA
  sta loc_FF04
  rts

sub_EBE5:
  stx var_00FD
  sty var_00FC
  tax
  jsr sub_59AA
  tya
  beq loc_EBF2
  inc var_00FD
loc_EBF2:
  ldx #$80
  ldy #$00
  stx var_00F7
  sty var_00F6
  ldy #$00
loc_EBFC:
  lda (var_00F6),y
  eor var_00D0
  tax
  lda var_00D1
  sta var_00D0
  lda var_00D2
  sta var_00D1
  lda var_00D3
  sta var_00D2
  lda var_00D0
  eor var_6C00,x
  sta var_00D0
  lda var_00D1
  eor var_6D00,x
  sta var_00D1
  lda var_00D2
  eor var_6E00,x
  sta var_00D2
  lda var_6F00,x
  sta var_00D3
  iny
  bne loc_EC2C
  inc var_00F7
loc_EC2C:
  dec var_00FC
  bne loc_EBFC
  dec var_00FD
  bne loc_EBFC
  rts

sub_EC35:
  clc
  adc var_0000,x
  sta var_0000,x
  bcc loc_EC3E
  inc var_0001,x
loc_EC3E:
  rts

sub_EC3F:
  sty var_00FF
  lda var_00D8
  sec
  sbc var_00FF
  bcs loc_EC60
  ldx var_00D9
  bne loc_EC5E
  ldx var_00DA
  bne loc_EC5C
  ldx var_00DB
  bne loc_EC5A
  ldy var_00D8
  lda #$00
  beq loc_EC60
loc_EC5A:
  dec var_00DB
loc_EC5C:
  dec var_00DA
loc_EC5E:
  dec var_00D9
loc_EC60:
  sta var_00D8
  sty var_00EF
  inc var_00EF
  tya
  ldx #$62
  ldy #$00
  stx var_00FF
  sty var_00FE
loc_EC6F:
  sta var_00FA
  sta var_00FB
  ldx var_00DD
  ldy var_00DC
  stx var_00FD
  sty var_00FC
  bit var_5FB2
  bvc loc_EC96
  cpx #$BF
  bcc loc_EC91
  lda var_00FC
  clc
  adc var_00FA
  bcc loc_EC91
  lda #$00
  sbc var_00FC
  sta var_00FB
loc_EC91:
  ldx var_00DE
  jsr sub_59AA
loc_EC96:
  ldy #$00
loc_EC98:
  lda (var_00FC),y
  sta (var_00FE),y
  iny
  cpy var_00FB
  bne loc_EC98
  tya
  clc
  adc var_00DC
  sta var_00DC
  bcc loc_ECBA
  inc var_00DD
  bit var_5FB2
  bvc loc_ECBA
  bit var_00DD
  bvc loc_ECBA
  inc var_00DE
  lda #$80
  sta var_00DC
loc_ECBA:
  lda var_00FA
  cmp var_00FB
  beq loc_ECD1
  sbc var_00FB
  sta var_00FA
  tya
  clc
  adc var_00FE
  sta var_00FE
  bcc loc_EC6F
  inc var_00FF
  jmp loc_EC6F
loc_ECD1:
  rts

sub_ECD2:
  sty var_00F0
  ldy #$01
  jsr sub_EC3F
  ldy var_6200
  iny
  jsr sub_EC3F
  ldx #$00
loc_ECE2:
  lda var_6200,x
  lsr a
  lsr a
  lsr a
  lsr a
  tay
  sty var_00F1
  lda var_6200,x
  and #$0F
  clc
  adc #$01
loc_ECF4:
  sta (var_00F2),y
  dey
  bpl loc_ECF4
  inx
  lda var_00F2
  sec
  adc var_00F1
  sta var_00F2
  bcc loc_ED05
  inc var_00F3
loc_ED05:
  dec var_00F0
  beq loc_ED16
  lda var_00F0
  sec
  sbc var_00F1
loc_ED0E:
  beq loc_ED16
  bcc loc_ED16
  sta var_00F0
  bcs loc_ECE2
loc_ED16:
  rts
data_ED17:
  .byte $01, $02, $04, $08, $10, $20, $40, $80

sub_ED1F:
  ldy #$3F
loc_ED21:
  lda var_6B00,y
  sta var_00FF
  cmp #$10
  bcs loc_ED5E
  tya
  asl a
  tax
  lda var_6B40,x
  pha
  ldx var_00FF
  cpx #$08
  bcs loc_ED3F
  lda data_ED17,x
  tax
  lda #$00
  beq loc_ED44
loc_ED3F:
  lda loc_ED0E+1,x
  ldx #$00
loc_ED44:
  sta var_00F1
  stx var_00F0
  pla
  tax
loc_ED4A:
  lda var_00FF
  sta var_7F00,x
  tya
  sta var_7E00,x
  txa
  clc
  adc var_00F0
  tax
  lda #$00
  adc var_00F1
  beq loc_ED4A
loc_ED5E:
  tya
  asl a
  tax
  lda var_6C01,x
  sta var_00F3
  lda var_6C00,x
  sta var_00F2
  lda var_6BC0,y
  cmp #$09
  bcs loc_ED7D
  ldx #$70
  stx var_00F1
  ldx #$00
  stx var_00F0
  jmp loc_EDB6
loc_ED7D:
  ldx var_00F2
  beq loc_ED9C
  lda #$FF
  sta var_7000,x
  lda #$72
  sta var_00F1
  lda #$00
  sta var_00F0
  ldx #$04
  lda var_00F3
loc_ED92:
  lsr a
  ror var_00F2
  dex
  bne loc_ED92
  lda #$04
  bne loc_EDAE
loc_ED9C:
  lda #$74
  sta var_00F1
  lda #$00
  sta var_00F0
  lda var_00F3
  sta var_00F2
  lda #$00
  sta var_00F3
  lda #$08
loc_EDAE:
  sta var_00FF
  lda var_6BC0,y
  sec
  sbc var_00FF
loc_EDB6:
  tax
  tya
  clc
  adc var_00E7
  jsr sub_EE39
  dey
  bmi loc_EDC4
  jmp loc_ED21
loc_EDC4:
  rts

sub_EDC5:
  ldy #$00
loc_EDC7:
  dey
  tya
  asl a
  tax
  bcs loc_EDD8
  lda var_6E01,x
  sta var_00F3
  lda var_6E00,x
  jmp loc_EDE0
loc_EDD8:
  lda var_6F01,x
  sta var_00F3
  lda var_6F00,x
loc_EDE0:
  sta var_00F2
  ldx var_6D00,y
  cpx #$09
  bcs loc_EDF4
  lda #$76
  sta var_00F1
  lda #$00
  sta var_00F0
  jmp loc_EE31
loc_EDF4:
  ldx var_00F2
  beq loc_EE0C
  lda #$FF
  sta var_7600,x
  lda #$3F
  bit var_00F2
  beq loc_EE15
  lda #$78
  pha
  lda #$00
  ldx #$04
  bne loc_EE1C
loc_EE0C:
  lda #$7C
  pha
  lda #$00
  ldx #$08
  bne loc_EE1C
loc_EE15:
  lda #$7A
  pha
  lda #$00
  ldx #$06
loc_EE1C:
  sta var_00F0
  pla
  sta var_00F1
  stx var_00FF
loc_EE23:
  lsr var_00F3
  ror var_00F2
  dex
  bne loc_EE23
  lda var_6D00,y
  sec
  sbc var_00FF
  tax
loc_EE31:
  jsr sub_EE38
  tya
  bne loc_EDC7
  rts

sub_EE38:
  tya

sub_EE39:
  sty var_00FF
  sta var_00FE
  stx var_00FD
  cpx #$08
  bcs loc_EE4B
  lda data_ED17,x
  tay
  lda #$00
  beq loc_EE50
loc_EE4B:
  lda loc_ED0E+1,x
  ldy #$00
loc_EE50:
  sta var_00F5
  sty var_00F4
  ldy var_00F2
loc_EE56:
  lda var_00FE
  sta (var_00F0),y
  inc var_00F1
  txa
  sta (var_00F0),y
  dec var_00F1
  tya
  clc
  adc var_00F4
  tay
loc_EE66:
  lda var_00F5
  adc #$00
  beq loc_EE56
  ldy var_00FF
  rts
data_EE6F:
  .byte $80, $40, $20, $10, $08, $04, $02, $01

sub_EE77:
  sty var_00FE
  ldy #$10
  lda #$00
loc_EE7D:
  sta var_66FF,y
  dey
  bne loc_EE7D
  ldy #$00
loc_EE85:
  lda (var_00F2),y
  tax
  inc var_66FF,x
  iny
  cpy var_00FE
  bne loc_EE85
  lda #$00
  tax
loc_EE93:
  clc
  adc var_6700,x
  sta var_6700,x
  inx
  cpx #$10
  bne loc_EE93
  ldy var_00FE
loc_EEA1:
  dey
  lda (var_00F2),y
  tax
  dec var_66FF,x
  lda var_66FF,x
  tax
  lda (var_00F2),y
  sta var_6710,x
  tya
  bne loc_EEA1
  lda #$00
  sta var_00FF
  sta var_00F4
  sta var_00F5
  sta var_00F6
  sta var_00F7
  ldy var_00FE
loc_EEC2:
  dey
  clc
  lda var_00F4
  adc var_00F6
  sta var_00F4
  lda var_00F5
  adc var_00F7
  sta var_00F5
  lda var_6710,y
  cmp var_00FF
  beq loc_EEF0
  sta var_00FF
  tax
  dex
  cpx #$08
  bcs loc_EEE6
  lda data_EE6F,x
  ldx #$00
  beq loc_EEEC
loc_EEE6:
  lda loc_EE66+1,x
  tax
  lda #$00
loc_EEEC:
  sta var_00F7
  stx var_00F6
loc_EEF0:
  tya
  asl a
  tax
  lda var_00F4
  bcs loc_EF02
  sta var_6810,x
  lda var_00F5
  sta var_6811,x
  jmp loc_EF0A
loc_EF02:
  sta var_6910,x
  lda var_00F5
  sta var_6911,x
loc_EF0A:
  tya
  bne loc_EEC2
  lda var_00F0
  sta var_00F4
  ldy #$00
loc_EF13:
  lda (var_00F2),y
  tax
  tya
  pha
  asl a
  tay
  lda var_00F1
  adc #$00
  sta var_00F5
  lda var_66FF,x
  inc var_66FF,x
  asl a
  tax
  bcs loc_EF34
  lda var_6810,x
  pha
  lda var_6811,x
  jmp loc_EF3B
loc_EF34:
  lda var_6910,x
  pha
  lda var_6911,x
loc_EF3B:
  ldx #$08
loc_EF3D:
  asl a
  ror var_00FF
  dex
  bne loc_EF3D
  lda var_00FF
  sta (var_00F4),y
  iny
  pla
  ldx #$08
loc_EF4B:
  asl a
  ror var_00FF
  dex
  bne loc_EF4B
  lda var_00FF
  sta (var_00F4),y
  pla
  tay
  iny
  cpy var_00FE
  bne loc_EF13
  rts

sub_EF5D:
  lda #$00
  sta var_00F0
  ldy #$00
loc_EF63:
  dey
  lda #$00
  sta var_00F2
  sta var_00F3
  sta var_00F4
  sty var_00F5
  ldx #$08
loc_EF70:
  lsr var_00F2
  ror var_00F3
  ror var_00F4
  ror var_00F5
  bcc loc_EF92
  lda var_00F2
  eor #$ED
  sta var_00F2
  lda var_00F3
  eor #$B8
  sta var_00F3
  lda var_00F4
  eor #$83
  sta var_00F4
  lda var_00F5
  eor #$20
  sta var_00F5
loc_EF92:
  dex
  bne loc_EF70
  lda #$6C
  sta var_00F1
  ldx #$03
loc_EF9B:
  lda var_00F2,x
  sta (var_00F0),y
  inc var_00F1
  dex
  bpl loc_EF9B
  tya
  bne loc_EF63
  rts

sub_EFA8:
  sty var_00FE
  txa
  ldx var_00E1
  jsr sub_D316
  ldx #$63
  ldy #$00
  stx var_00FD
  sty var_00FC
  ldy #$00
  tax
  beq loc_EFCB
loc_EFBD:
  lda (var_00FC),y
  sta (var_00DF),y
  iny
  bne loc_EFBD
  inc var_00FD
  inc var_00E0
  dex
  bne loc_EFBD
loc_EFCB:
  ldx var_00FE
  beq loc_EFD9
loc_EFCF:
  lda (var_00FC),y
  sta (var_00DF),y
  iny
  dex
  bne loc_EFCF
  sty var_00DF
loc_EFD9:
  lda var_00E0
  cmp #$C0
  bcc loc_EFE5
  lda #$80
  sta var_00E0
  inc var_00E1
loc_EFE5:
  rts

sub_EFE6:
  lda #$00
  sec
  sbc var_00F8
  sta var_00F8
  lda #$63
  sbc var_00F9
  sta var_00F9
  ldx var_00E1
  lda var_00DF
  sec
  sbc var_00F8
  sta var_00EB
  lda var_00E0
  sbc var_00F9
  bcs loc_F00B
loc_F002:
  cpx var_5FB3
  beq loc_F086
  clc
  adc #$40
  dex
loc_F00B:
  cmp #$80
  bcc loc_F002
  sta var_00EC
  stx var_00ED
loc_F013:
  lda var_00F6
  sec
  sbc var_00F8
  tay
  lda var_00F7
  sbc var_00F9
  tax
  bcs loc_F02C
  lda var_00F6
  sta var_00F8
  lda var_00F7
  sta var_00F9
  ldx #$00
  ldy #$00
loc_F02C:
  stx var_00F7
  sty var_00F6
loc_F030:
  ldx var_00ED
  jsr sub_D316
  lda #$FF
  ldx var_00EC
  cpx #$BF
  bcc loc_F045
  ldx var_00EB
  beq loc_F045
  lda #$00
  sbc var_00EB
loc_F045:
  sta var_00FB
  tax
  lda var_00F8
  sec
  sbc var_00FB
  bcs loc_F059
  dec var_00F9
  bpl loc_F059
  ldx var_00F8
  lda #$00
  sta var_00F9
loc_F059:
  sta var_00F8
  ldy #$00
loc_F05D:
  lda (var_00EB),y
  sta (var_00F0),y
  iny
  dex
  bne loc_F05D
  ldx #$EB
  tya
  jsr sub_EC35
  ldx #$F0
  tya
  jsr sub_EC35
  lda var_00F8
  ora var_00F9
  beq loc_F085
  lda var_00EC
  cmp #$C0
  bcc loc_F030
  inc var_00ED
  lda #$80
  sta var_00EC
  bne loc_F030
loc_F085:
  rts
loc_F086:
  stx var_00ED
  sta var_00EC
  lda #$00
  sec
  sbc var_00EB
  tax
  lda #$80
  sbc var_00EC
  tay
  txa
  cmp var_00F6
  tya
  sbc var_00F7
  bcc loc_F0A1
  ldx var_00F6
  ldy var_00F7
loc_F0A1:
  stx var_00EB
  sty var_00EC
  lda var_00F6
  sec
  sbc var_00EB
  sta var_00F6
  lda var_00F7
  sbc var_00EC
  sta var_00F7
  txa
  bne loc_F0B6
  dey
loc_F0B6:
  sty var_00EB
  lda #$00
  tay
loc_F0BB:
  sta (var_00F0),y
  iny
  bne loc_F0C2
  inc var_00F1
loc_F0C2:
  dex
  bne loc_F0BB
  dec var_00EB
  bpl loc_F0BB
  tya
  clc
  adc var_00F0
  sta var_00F0
  bcc loc_F0D3
  inc var_00F1
loc_F0D3:
  lda var_00F6
  ora var_00F7
  beq loc_F085
  sty var_00EB
  lda var_00F8
  sec
  sbc var_00EB
  sta var_00F8
  bcs loc_F0E6
  dec var_00F9
loc_F0E6:
  lda #$80
  sta var_00EC
  lda #$00
  sta var_00EB
  jmp loc_F013

sub_F0F1:
  lda #$00
  sta var_00E4
  ldy #$FF
  jsr sub_EC3F
  ldx #$63
  ldy #$00
  stx var_00F1
  sty var_00F0
  ldx #$00
  stx var_00F5
  lda var_6200,x
  sta var_00F2
  stx var_00EE
  inc var_00EE
  dec var_00EF
  jmp loc_F1A0
loc_F114:
  ldx #$7A
  ldy #$00
  lda #$06
loc_F11A:
  sta var_00F4
  stx var_00F7
  sty var_00F6
  jsr sub_F2C1
  inc var_00F7
  lda (var_00F6),y
  sta var_00F4
  dec var_00F7
  lda (var_00F6),y
  sta var_00F6
  jmp loc_F15A
loc_F132:
  ldx #$7C
  ldy #$00
  lda #$08
  bne loc_F11A
loc_F13A:
  lda #$3F
  bit var_00F2
  beq loc_F114
  ldx #$78
  ldy #$00
  lda #$04
  bne loc_F11A
loc_F148:
  lda var_00F2
  beq loc_F132
  lda var_7600,y
  cmp #$FF
  beq loc_F13A
  sta var_00F6
  lda var_7700,y
  sta var_00F4
loc_F15A:
  jsr sub_F2C1
  lda var_00F6
  ldy #$00
  sta (var_00F0),y
  inc var_00F0
  bne loc_F169
  inc var_00F1
loc_F169:
  jmp loc_F253
loc_F16C:
  lda var_00E3
  bne loc_F148
  lda var_00F2
  sta var_00F6
  lda #$08
  sta var_00F4
  bne loc_F15A
loc_F17A:
  ldx #$74
  ldy #$00
  lda #$08
loc_F180:
  sta var_00F4
  stx var_00F7
  sty var_00F6
  jsr sub_F2C1
  inc var_00F7
  lda (var_00F6),y
  sta var_00F4
  dec var_00F7
  lda (var_00F6),y
  sta var_00F6
  jmp loc_F1E1
loc_F198:
  ldx #$72
  ldy #$00
  lda #$04
  bne loc_F180
loc_F1A0:
  lda #$01
  sta var_00F4
  jsr sub_F2C1
  bcs loc_F16C
  lda var_00F2
  and var_00E8
  sta var_00F7
  lda var_00E9
  sta var_00F4
  jsr sub_F2C1
  lda var_7F00,y
  sta var_00F4
  lda #$00
  sta var_00E6
  lda var_7E00,y
  ldy var_00E9
loc_F1C4:
  asl a
  rol var_00E6
  dey
  bne loc_F1C4
  ora var_00F7
  sta var_00E5
  jsr sub_F2C1
  lda var_00F2
  beq loc_F17A
  lda var_7000,y
  bmi loc_F198
  sta var_00F6
  lda var_7100,y
  sta var_00F4
loc_F1E1:
  jsr sub_F2C1
  lda #$00
  sta var_00F7
  lda var_00F6
  cmp var_00EA
  bne loc_F1FE
  clc
  adc var_00F2
  sta var_00F6
  bcc loc_F1F7
  inc var_00F7
loc_F1F7:
  lda #$08
  sta var_00F4
  jsr sub_F2C1
loc_F1FE:
  lda var_00F0
  clc
  sbc var_00E5
  sta var_00F8
  lda var_00F1
  sbc var_00E6
  sta var_00F9
  bcc loc_F211
  cmp #$63
  bcs loc_F21C
loc_F211:
  jsr sub_EFE6
  ldx #$63
  ldy #$00
  stx var_00F9
  sty var_00F8
loc_F21C:
  lda var_00F6
  ora var_00F7
  beq loc_F253
  ldy #$FF
  lda var_00F6
  sec
  sbc #$FF
  bcs loc_F235
  dec var_00F7
  bpl loc_F235
  ldy var_00F6
  lda #$00
  sta var_00F7
loc_F235:
  sta var_00F6
  sty var_00FF
  ldy #$00
loc_F23B:
  lda (var_00F8),y
  sta (var_00F0),y
  iny
  cpy var_00FF
  bne loc_F23B
  ldx #$F8
  tya
  jsr sub_EC35
  ldx #$F0
  tya
  jsr sub_EC35
  jmp loc_F21C
loc_F253:
  lda var_00F1
  cmp #$6B
  bcs loc_F25C
  jmp loc_F1A0
loc_F25C:
  ldx #$08
  ldy #$00
  jsr sub_EFA8
  lda var_00F0
  sta var_00FF
  ldy #$00
  lda var_00F1
  sec
  sbc #$08
  sta var_00F1
  cmp #$63
  beq loc_F28B
loc_F274:
  lda var_6B00,y
  sta var_6300,y
  iny
  bne loc_F274
loc_F27D:
  cpy var_00FF
  beq loc_F298
  lda var_6C00,y
  sta var_6400,y
  iny
  jmp loc_F27D
loc_F28B:
  cpy var_00FF
  beq loc_F298
  lda var_6B00,y
  sta var_6300,y
  iny
  bne loc_F28B
loc_F298:
  jmp loc_F1A0
loc_F29B:
  pla
  pla
  pla
  pla
  lda var_00F0
  sec
  sbc #$00
  tay
  lda var_00F1
  sbc #$63
  tax
  rts

sub_F2AB:
  lda var_00E4
  bne loc_F29B
  ldy #$FF
  jsr sub_EC3F
  lda #$00
  sta var_00EE
  dec var_00EF
  bne loc_F2C0
  inc var_00EF
  inc var_00E4
loc_F2C0:
  rts

sub_F2C1:
  lda var_00F5
  cmp var_00F4
  bcs loc_F2EB
  tax
  beq loc_F2D1
loc_F2CA:
  lsr var_00F3
  ror var_00F2
  dex
  bne loc_F2CA
loc_F2D1:
  dec var_00EF
  bne loc_F2D8
  jsr sub_F2AB
loc_F2D8:
  ldx var_00EE
  lda var_6200,x
  inc var_00EE
  sta var_00F3
  lda var_00F4
  sec
  sbc var_00F5
  sta var_00F4
  lda #$08
  sec
loc_F2EB:
  sbc var_00F4
  sta var_00F5
  ldx var_00F4
loc_F2F1:
  lsr var_00F3
  ror var_00F2
  dex
  bne loc_F2F1
  ldy var_00F2
  rts

sub_F2FB:
  lda #$FF
  sta var_00D8
  ldy #$0A
  jsr sub_EC3F
  lda var_6200
  sta var_00E2
  ldx #$07
loc_F30B:
  lda var_6202,x
  sta var_00D4,x
  dex
  bpl loc_F30B
  ldx #$00
  lda #$04
  bit var_00E2
  beq loc_F342
  ldx #$6D
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldy #$00
  jsr sub_ECD2
  ldx #$6D
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldx #$6E
  ldy #$00
  stx var_00F1
  sty var_00F0
  ldy #$00
  jsr sub_EE77
  jsr sub_EDC5
  ldx #$01
loc_F342:
  stx var_00E3
  lda #$02
  bit var_00E2
  bne loc_F358
  lda #$02
  sta var_00E7
  lda #$3F
  sta var_00E8
  lda #$06
  sta var_00E9
  bne loc_F364
loc_F358:
  lda #$03
  sta var_00E7
  lda #$7F
  sta var_00E8
  lda #$07
  sta var_00E9
loc_F364:
  lda var_00E7
  clc
  adc #$3F
  sta var_00EA
  ldx #$6B
  ldy #$C0
  stx var_00F3
  sty var_00F2
  ldy #$40
  jsr sub_ECD2
  ldx #$6B
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldy #$40
  jsr sub_ECD2
  ldx #$6B
  ldy #$C0
  stx var_00F3
  sty var_00F2
  ldx #$6C
  ldy #$00
  stx var_00F1
  sty var_00F0
  ldy #$40
  jsr sub_EE77
  ldx #$6B
  ldy #$00
  stx var_00F3
  sty var_00F2
  ldx #$6B
  ldy #$40
  stx var_00F1
  sty var_00F0
  ldy #$40
  jsr sub_EE77
  jsr sub_ED1F
  jsr sub_F0F1
  jmp sub_EFA8
data_F3B8:
  .byte $05, $22, $21, $2E, $3C, $12, $06, $2F, $03, $20, $1F, $CF, $3B, $11, $04, $2D
  .byte $D2, $0E, $D1, $CD, $42, $C9, $D3, $C7, $0A, $17, $26, $33, $3F, $18, $0B, $34
  .byte $1B, $1C, $C8, $CB, $41, $1A, $2B, $D0, $10, $3A, $2C, $55, $01, $1E, $02, $1D
  .byte $08, $15, $25, $32, $3E, $16, $09, $24, $0C, $27, $28, $35, $40, $19, $0D, $2A
  .byte $14, $23, $31, $39, $3D, $13, $07, $30, $FF, $4B, $47, $57, $58, $4F, $50, $48
  .byte $4A, $4E, $37, $49, $44, $4C, $B5, $45, $29, $4D, $38, $0F, $43, $51, $53, $52

sub_F418:
  lda #$05
  sta var_4016
  ldx #$00
loc_F41F:
  lda #$04
  sta var_4016
  lda var_4017
  lsr a
  and #$0F
  ldy #$06
  sty var_4016
  sta var_5C42
  lda var_4017
  asl a
  asl a
  asl a
  and #$F0
  ora var_5C42
  eor #$FF
  sta var_5C4D,x
  inx
  cpx #$09
  bcc loc_F41F
  bne loc_F453
  lda #$04
  sta var_4016
  lda #$06
  sta var_4016
loc_F453:
  cpx #$0C
  bne loc_F41F
  ldx #$00
loc_F459:
  lda var_5C4D,x
  cmp var_5C59,x
  bne loc_F469
  inx
  cpx #$0C
  bne loc_F459
  txa
  sec
  rts
loc_F469:
  eor var_5C59,x
  sta var_5C41
  txa
  asl a
  asl a
  asl a
  tay
  lda #$00
  sec
loc_F477:
  iny
  rol a
  bit var_5C41
  beq loc_F477
  dey
  rts

sub_F480:
  jsr sub_F418
  bcs loc_F4B6
  pha
  eor var_5C59,x
  sta var_5C59,x
  pla
  and var_5C59,x
  tax
  lda #$00
  sta var_5C7C
  lda data_F3B8,y
  bpl loc_F4A7
  tay
  lda var_5FEF
  ora #$12
  sta var_5FEF
  tya
  and #$7F
loc_F4A7:
  dex
  bpl loc_F4AC
  ora #$80
loc_F4AC:
  sta var_5C42
  jsr sub_F9C6
  bcc loc_F4B6
  ldy #$00
loc_F4B6:
  rts
  .byte $38, $60

sub_F4B9:
  cpx #$37
  bne loc_F4C6
  lda #$03
  bit var_5FED
  beq loc_F4C6
  clc
  rts
loc_F4C6:
  sec
  rts
loc_F4C8:
  sty data_FF40
  lda #$17
  ldx #$00
loc_F4CF:
  bit data_FF48
  bmi loc_F4E2
  dey
  bne loc_F4CF
  dex
  bne loc_F4CF
  sec
  sbc #$01
  bne loc_F4CF
  sec
  bcs loc_F500
loc_F4E2:
  lda #$0D
  sta data_FF50
  pha
  pla
  bne loc_F4F8
loc_F4EB:
  lda #$08
  sta data_FF50
  ldy #$0A
loc_F4F2:
  dex
  bne loc_F4F2
  dey
  bne loc_F4F2
loc_F4F8:
  lda #$0C
  sta data_FF50
  pha
  pla
loc_F4FF:
  clc
loc_F500:
  lda data_FF48
  and #$F8
  bcc loc_F509
  ora #$01
loc_F509:
  eor #$48
  ldx #$80
  rts

sub_F50E:
  asl a
  bcc loc_F529
  pha
  lda var_5F00,x
  clc
  adc var_5C77
  bcs loc_F520
  cmp var_5C47,y
  bcc loc_F525
loc_F520:
  lda var_5C47,y
  sbc #$01
loc_F525:
  sta var_5F00,x
  pla
loc_F529:
  asl a
  bcc loc_F541
  pha
  lda var_5F00,x
  sbc var_5C77
  bcc loc_F53A
  cmp var_5C6D,y
  bcs loc_F53D
loc_F53A:
  lda var_5C6D,y
loc_F53D:
  sta var_5F00,x
  pla
loc_F541:
  rts

sub_F542:
  ldx #$00
  jsr sub_F549
  ldx #$01

sub_F549:
  jsr sub_DBE2
  sta var_5C41
  eor var_5FFC,x
  tay
  and var_5FFC,x
  pha
  lda var_5C41
  sta var_5FFC,x
  tya
  and var_5FFC,x
  beq sub_F5B2
  stx var_5C42
  cpx #$00
  bne loc_F579
  ldx #$00
loc_F56C:
  lsr a
  bcc loc_F572
  inc var_5C65,x
loc_F572:
  inx
  cpx #$04
  bne loc_F56C
  beq loc_F586
loc_F579:
  ldx #$00
loc_F57B:
  lsr a
  bcc loc_F581
  inc var_5C69,x
loc_F581:
  inx
  cpx #$04
  bne loc_F57B
loc_F586:
  ldx var_5C42
  asl a
  beq sub_F5B2
  asl a
  asl a
  asl a
  tay
  eor var_5FFC,x
  and #$F0
  bne loc_F59C
  lda #$06
  sta var_5C75,x
loc_F59C:
  tya
  and var_5C71,x
  beq loc_F5AA
  asl var_5C4B,x
  bcc loc_F5AA
  ror var_5C4B,x
loc_F5AA:
  lda #$00
  sta var_5C71,x
  sta var_5C73,x

sub_F5B2:
  pla
  and #$F0
  beq sub_F5C1
  sta var_5C71,x
  lda #$05
  sta var_5C73,x
  bne sub_F5DC

sub_F5C1:
  lda var_5C73,x
  beq sub_F5DC
  dec var_5C73,x
  bne sub_F5DC
  lda var_5C41
  and #$F0
  bne loc_F5D7
  lda #$01
  sta var_5C4B,x
loc_F5D7:
  lda #$00
  sta var_5C71,x

sub_F5DC:
  lda var_5C75,x
  beq sub_F5EE
  ldy var_5C4B,x
  dey
  bne sub_F5EE
  dec var_5C75,x
  cmp #$06
  bne loc_F64E

sub_F5EE:
  lda var_5C41
  and #$F0
  beq loc_F64E
  ldy var_5C4B,x
  sty var_5C77
  dex
  beq loc_F60C
  ldx #$F2
  ldy #$00
  jsr sub_F50E
  ldx #$F3
  ldy #$02
  jmp sub_F50E
loc_F60C:
  ldx #$EB
  ldy #$01
  jsr sub_F50E
  ldx #$EC
  ldy #$03
  jmp sub_F50E

sub_F61A:
  ldy var_5C79
  sta var_5C00,y
  iny
  txa
  sta var_5C00,y
  iny
  cpy #$40
  bne loc_F62C
  ldy #$00
loc_F62C:
  cpy var_5C78
  bne loc_F64B
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
loc_F64B:
  sty var_5C79
loc_F64E:
  rts

sub_F64F:
  cli
  txa
  pha
  tya
  pha
  jsr sub_F480
  bcs loc_F669
  sta var_5C43
  stx var_5C44
  bit var_5C42
  bmi sub_F684
  inc var_5C7C
  bne sub_F684
loc_F669:
  beq loc_F692
  lda var_5C7C
  beq loc_F692
  cmp var_5C45
  bcc loc_F68A
  ldx var_5C7B
  inx
  cpx var_5C46
  bcc loc_F68F
  lda var_5C43
  ldx var_5C44

sub_F684:
  jsr sub_F61A
  jmp loc_F68D
loc_F68A:
  inc var_5C7C
loc_F68D:
  ldx #$00
loc_F68F:
  stx var_5C7B
loc_F692:
  jsr sub_F542
  pla
  tay
  pla
  tax
  rts
data_F69A:
data_F69B = * + 1
  .word sub_F6D0 - 1, loc_F6FA - 1, loc_F71E - 1, loc_F730 - 1
  .word loc_FF0F - 1, loc_F74B - 1, loc_F784 - 1, sub_F78C - 1
  .word loc_F4C8 - 1, loc_F4EB - 1, loc_F4FF - 1, loc_F7BF - 1
  .word loc_F7C0 - 1, loc_F7F5 - 1, loc_F83F - 1, loc_F855 - 1
  .word sub_F86E - 1, loc_F880 - 1, sub_F8F0 - 1

sub_F6C0:
  txa
  asl a
  cmp #$26
  bcs loc_F6CF
  tax
  lda data_F69B,x
  pha
  lda data_F69A,x
  pha
loc_F6CF:
  rts

sub_F6D0:
  ldy var_5C78
  cpy var_5C79
  bne loc_F6E8
  bit var_5C40
  bvc sub_F6D0
  jsr sub_F480
  bcs sub_F6D0
  jsr sub_F61A
  jmp sub_F6D0
loc_F6E8:
  lda var_5C00,y
  iny
  ldx var_5C00,y
  iny
  cpy #$40
  bcc loc_F6F6
  ldy #$00
loc_F6F6:
  sty var_5C78
  rts
loc_F6FA:
  ldy var_5C78
  cpy var_5C79
  beq loc_F70B
  lda var_5C00,y
  ldx var_5C01,y
  ldy #$01
  rts
loc_F70B:
  bit var_5C40
  bvc loc_F71B
  jsr sub_F480
  bcs loc_F71B
  jsr sub_F61A
  jmp loc_F6FA
loc_F71B:
  ldy #$00
  rts
loc_F71E:
  lda var_5FED
  ldx var_5FEE
  ldy var_5FEF
  rts
data_F728:
  .byte $02, $03, $04, $05, $08, $0B, $10, $16
loc_F730:
  tya
  and #$60
  asl a
  rol a
  rol a
  rol a
  tax
  lda data_FFF1,x
  sta var_5C45
  tya
  and #$1C
  lsr a
  lsr a
  tax
  lda data_F728,x
  sta var_5C46
  rts
loc_F74B:
  ldx var_5C78
  dex
  dex
  bpl loc_F754
  ldx #$3E
loc_F754:
  lda var_6000
  pha
  and #$7F
  sta var_2000
  sta var_6000
  cpx var_5C79
  bne loc_F770
  txa
  sec
  sbc #$02
  bpl loc_F76D
  lda #$3E
loc_F76D:
  sta var_5C79
loc_F770:
  stx var_5C78
  pla
  sta var_6000
  sta var_2000
  tya
  sta var_5C00,x
  lda var_00FF
  sta var_5C01,x
  rts
loc_F784:
  sty var_5C40
  ldx #$59
  ldy #$FD
  rts

sub_F78C:
  lda #$00
  sta var_5FED
  sta var_5FEE
  sta var_5FEF
  sta var_5FF0
  ldx #$04
loc_F79C:
  sta var_5C78,x
  dex
  bpl loc_F79C
  ldx #$0B
loc_F7A4:
  lda var_5C4D,x
  sta var_5C59,x
  dex
  bpl loc_F7A4
  ldx #$1A
  stx var_5C45
  ldx #$03
  stx var_5C46
  lda #$00
  jsr sub_FF09
  lda #$00
  rts
loc_F7BF:
  rts
loc_F7C0:
  tya
  and #$01
  tay
  lda #$00
  sta var_5FFC,y
  sta var_5C6D,y
  sta var_5C6F,y
  lda #$F6
  sta var_5C47,y
  lda #$EF
  sta var_5C49,y
  lsr a
  dey
  beq loc_F7E9
  sta var_5FF3
  lda #$52
  sta var_5FF2
  lda #$00
  beq loc_F80C
loc_F7E9:
  sta var_5FEC
  lda #$A4
  sta var_5FEB
  lda #$00
  beq loc_F82E
loc_F7F5:
  tya
  ldx #$03
  lsr a
  bcs loc_F81D
loc_F7FB:
  lda var_5C65,x
  sta var_00FC,x
  dex
  bpl loc_F7FB
  lda var_5FFC
  ldx var_5FF2
  ldy var_5FF3
loc_F80C:
  pha
  lda #$00
  sta var_5C65
  sta var_5C66
  sta var_5C67
  sta var_5C68
  pla
  rts
loc_F81D:
  lda var_5C69,x
  sta var_00FC,x
  dex
  bpl loc_F81D
  lda var_5FFD
  ldx var_5FEB
  ldy var_5FEC
loc_F82E:
  pha
  lda #$00
  sta var_5C69
  sta var_5C6A
  sta var_5C6B
  sta var_5C6C
  pla
  rts
loc_F83F:
  tya
  ldx var_00FE
  ldy var_00FF
  lsr a
  bcs loc_F84E
  stx var_5FF2
  sty var_5FF3
  rts
loc_F84E:
  stx var_5FEB
  sty var_5FEC
  rts
loc_F855:
  tya
  and #$01
  tay
  lda var_00FC
  sta var_5C6D,y
  lda var_00FD
  sta var_5C6F,y
  lda var_00FE
  sta var_5C47,y
  lda var_00FF
  sta var_5C49,y
  rts

sub_F86E:
  lda #$FF
  sta data_FF10
  pha
  pla
  jsr sub_F8F0
  pha
  pla
  lda #$FF
  sta data_FF10
  rts
loc_F880:
  tya
  pha
  jsr sub_F86E
  jsr sub_59C0
  sta var_5FB2
  pla
  tax
  jsr sub_D316
  ldy #$00
  lda (var_00FE),y
  sta var_5FB6
  iny
  lda (var_00FE),y
  sta var_5FB5
  iny
loc_F89E:
  lda data_FF18
  bpl loc_F89E
  cmp #$FF
  beq loc_F89E
  lda var_4CFD
  cmp #$FF
  beq loc_F8B9
  tya
  pha
  jsr sub_59D6
  pla
  cpy #$00
  bne loc_F8DB
  tay
loc_F8B9:
  lda (var_00FE),y
  sta data_FF18
  iny
  bne loc_F8C3
  inc var_00FF
loc_F8C3:
  sec
  lda var_5FB5
  sbc #$01
  sta var_5FB5
  bcs loc_F8D1
  dec var_5FB6
loc_F8D1:
  lda var_5FB5
  ora var_5FB6
  bne loc_F89E
  beq loc_F8EA
loc_F8DB:
  ldx #$F0
loc_F8DD:
  lda data_FF18
  bpl loc_F8DD
  lda #$00
  sta data_FF18
  inx
  bne loc_F8DD
loc_F8EA:
  ldx var_5FB2
  jmp sub_D316

sub_F8F0:
  lda #$00
  sta data_FF10
  rts

sub_F8F6:
  ldx #$40
  cmp #$3A
  beq loc_F906
  ldx #$20
  cmp #$45
  beq loc_F906
  ldx #$10
  cmp #$46
loc_F906:
  rts

sub_F907:
  lda #$08
  bit var_5FEF
  beq loc_F91F
  cpx #$45
  beq loc_F91A
  lda var_5FEF
  and #$F7
  sta var_5FEF
loc_F91A:
  pla
  pla
  jmp loc_FC4D
loc_F91F:
  rts

sub_F920:
  txa
  jsr sub_F8F6
  bne loc_F93F
  txa
  bit var_5FEE
  bne loc_F91A
  ora var_5FEE
  sta var_5FEE
  txa
  eor var_5FED
  sta var_5FED
  jsr sub_F98F
  jmp loc_F91A
loc_F93F:
  tax
  rts

sub_F941:
  lda loc_FD2A+2,x
  cmp #$FF
  beq loc_F91A
  rts

sub_F949:
  cpx #$52
  bne loc_F962
  lda #$80
  bit var_5FEE
  bne loc_F91A
  ora var_5FEE
  sta var_5FEE
  lda #$80
  eor var_5FED
  sta var_5FED
loc_F962:
  rts

sub_F963:
  ldx #$08
  cmp #$38
  beq loc_F979
  ldx #$04
  cmp #$1D
  beq loc_F979

sub_F96F:
  ldx #$02
  cmp #$2A
  beq loc_F979
  ldx #$01
  cmp #$36
loc_F979:
  rts

sub_F97A:
  lda #$10
  bit var_5FEF
  rts

sub_F980:
  pha
  lda var_5FEF
  lsr a
  pla
  rts

sub_F987:
  pha
  lda var_5FEF
  lsr a
  lsr a
  pla
  rts

sub_F98F:
  lda #$40
  bit var_5FF0
  bne loc_F9C5
  ora var_5FF0
  sta var_5FF0
  lda var_5FED
  lsr a
  lsr a
  lsr a
  lsr a
  and #$07
  tax
  eor var_5FF0
  and #$07
  lda var_5FF0
  and #$F8
  stx var_5FF0
  ora var_5FF0
  sta var_5FF0
  txa
  jsr sub_FF09
  lda #$3F
  and var_5FF0
  sta var_5FF0
loc_F9C5:
  rts

sub_F9C6:
  tax
  bmi loc_F9CC
  jmp loc_FB0C
loc_F9CC:
  and #$7F
  jsr sub_F980
  bcc loc_F9DB
  jsr sub_F963
  bne loc_F9F9
  jmp loc_FC55
loc_F9DB:
  jsr sub_F987
  bcc loc_F9E5
  jsr sub_F96F
  beq loc_F9F9
loc_F9E5:
  jsr sub_F8F6
  beq loc_F9F0
  ldx #$80
  cmp #$52
  bne loc_F9FC
loc_F9F0:
  txa
  eor #$FF
  and var_5FEE
  sta var_5FEE
loc_F9F9:
  jmp loc_FC4D
loc_F9FC:
  cmp #$54
  bne loc_FA0B
  lda var_5FEF
  and #$FB
  sta var_5FEF
  jmp loc_FC55
loc_FA0B:
  jsr sub_F963
  bne loc_F9F9
  pha
  txa
  eor #$FF
  tay
  and var_5FED
  sta var_5FED
  pla
  jsr sub_F96F
  beq loc_F9F9
  tax
  jsr sub_F987
  bcc loc_FA30
  tya
  and var_5FEE
  sta var_5FEE
  bcs loc_FA3B
loc_FA30:
  tya
  lsr a
  lsr a
  ora #$C0
  and var_5FEE
  sta var_5FEE
loc_FA3B:
  lda var_5FEE
  asl a
  asl a
  ora var_5FEE
  and #$0C
  ora var_5FED
  sta var_5FED
  cpx #$38
  bne loc_F9F9
  lda var_5C7A
  beq loc_F9F9
  ldx #$00
  stx var_5C7A
loc_FA59:
  jmp loc_FC3B
  .byte $1B, $21, $40, $23, $24, $25, $5E, $26, $2A, $28, $29, $5F, $2B, $08, $00, $51
  .byte $57, $45, $52, $54, $59, $55, $49, $4F, $50, $7B, $7D, $0D, $FF, $41, $53, $44
  .byte $46, $47, $48, $4A, $4B, $4C, $3A, $22, $7E, $FF, $7C, $5A, $58, $43, $56, $42
  .byte $4E, $4D, $3C, $3E, $3F, $FF, $2A, $FF, $20, $FF, $54, $55, $56, $57, $58, $59
  .byte $5A, $5B, $5C, $5D, $FF, $FF, $37, $38, $39, $2D, $34, $35, $36, $2B, $31, $32
  .byte $33, $30, $2E, $FF, $03, $7C, $87
data_FAB3:
  .byte $88, $1B, $31, $32, $33, $34, $35, $36, $37, $38, $39, $30, $2D, $3D, $08, $09
  .byte $71, $77, $65, $72, $74, $79, $75, $69, $6F, $70, $5B, $5D, $0D, $FF, $61, $73
  .byte $64, $66, $67, $68, $6A, $6B, $6C, $3B, $27, $60, $FF, $5C, $7A, $78, $63, $76
  .byte $62, $6E, $6D, $2C, $2E, $2F, $FF, $2A, $FF, $20, $FF, $3B, $3C, $3D, $3E, $3F
  .byte $40, $41, $42, $43, $44, $FF, $FF, $47, $48, $49, $2D, $4B, $F0, $4D, $2B, $4F
  .byte $50, $51, $52, $53, $FF, $03, $5C, $85, $86
loc_FB0C:
  jsr sub_F987
  bcc loc_FB19
  jsr sub_F96F
  bne loc_FB19
loc_FB16:
  jmp loc_FC4D
loc_FB19:
  jsr sub_F963
  bne loc_FB45
  pha
  txa
  tay
  ora var_5FED
  sta var_5FED
  pla
  jsr sub_F96F
  beq loc_FB16
  tya
  jsr sub_F987
  bcc loc_FB3B
  ora var_5FEE
  sta var_5FEE
  bne loc_FB16
loc_FB3B:
  lsr a
  lsr a
  ora var_5FEE
  sta var_5FEE
  bne loc_FB16
loc_FB45:
  tax
  beq loc_FB16
  cmp #$59
  bcs loc_FB16
  cmp #$54
  bne loc_FB60
  lda #$04
  bit var_5FEF
  bne loc_FB16
  ora var_5FEF
  sta var_5FEF
  jmp loc_FC55
loc_FB60:
  lda #$08
  bit var_5FED
  beq loc_FB76
  jsr sub_F97A
  beq loc_FB73
  lda #$04
  bit var_5FEF
  bne loc_FB76
loc_FB73:
  jmp loc_FD85
loc_FB76:
  lda #$04
  bit var_5FED
  beq loc_FB80
  jmp loc_FCAF
loc_FB80:
  cpx #$55
  bne loc_FB8E
  lda #$08
  bit var_5FEF
  bne loc_FB16
  jmp loc_FCBD
loc_FB8E:
  jsr sub_F920
  cpx #$52
  beq loc_FB98
  jsr sub_F907
loc_FB98:
  jsr sub_F4B9
  bcs loc_FBA0
  jmp loc_FC4D
loc_FBA0:
  jsr sub_F987
  bcs loc_FBAE
  lda #$03
  bit var_5FED
  beq loc_FBD5
  bne loc_FBC1
loc_FBAE:
  cpx #$1C
  bne loc_FBB6
  lda #$0D
  bne loc_FBBC
loc_FBB6:
  cpx #$35
  bne loc_FBEA
  lda #$2F
loc_FBBC:
  ldx #$E0
  jmp loc_FC3B
loc_FBC1:
  lda #$20
  bit var_5FED
  beq loc_FBE4
  jsr sub_F949
  cpx #$47
  bcc loc_FBE4
  cpx #$56
  bcs loc_FBE4
  bcc loc_FBED
loc_FBD5:
  lda #$20
  bit var_5FED
  beq loc_FBEA
  cpx #$47
  bcc loc_FBED
  cpx #$56
  bcs loc_FBED
loc_FBE4:
  lda loc_FA59+2,x
  jmp loc_FBF0
loc_FBEA:
  jsr sub_F949
loc_FBED:
  lda data_FAB3,x
loc_FBF0:
  cmp #$FF
  bne loc_FBF7
  jmp loc_FC4D
loc_FBF7:
  cpx #$3B
  bcc loc_FC11
  cmp #$3B
  bcc loc_FC11
  cmp #$F0
  beq loc_FC11
  cpx #$56
  beq loc_FC11
  tax
  lda #$00
  jsr sub_F987
  bcc loc_FC11
  lda #$E0
loc_FC11:
  tay
  lda #$40
  bit var_5FED
  beq loc_FC3A
  lda #$03
  bit var_5FED
  bne loc_FC2D
  cpy #$7B
  bcs loc_FC3A
  cpy #$61
  bcc loc_FC3A
  tya
  and #$DF
  bne loc_FC3B
loc_FC2D:
  cpy #$5B
  bcs loc_FC3A
  cpy #$41
  bcc loc_FC3A
  tya
  ora #$20
  bne loc_FC3B
loc_FC3A:
  tya
loc_FC3B:
  tay
  lda var_5FEF
  and #$FC
  sta var_5FEF
  tya
  cmp #$F0
  bne loc_FC4B
  lda #$00
loc_FC4B:
  clc
  rts
loc_FC4D:
  lda var_5FEF
  and #$FC
  sta var_5FEF
loc_FC55:
  sec
loc_FC56:
  rts
  .byte $1B, $FF, $00, $FF, $FF, $FF, $1E, $FF, $FF, $FF, $FF, $1F, $FF, $7F, $94, $11
  .byte $17, $05, $12, $14, $19, $15, $09, $0F, $10, $1B, $1D, $0A, $FF, $01, $13, $04
  .byte $06, $07, $08, $0A, $0B, $0C, $FF, $FF, $FF, $FF, $1C, $1A, $18, $03, $16, $02
  .byte $0E, $0D, $FF, $FF, $95, $FF, $96, $FF, $20, $FF, $5E, $5F, $60, $61, $62, $63
  .byte $64, $65, $66, $67, $45, $46, $77, $8D, $84, $8E, $73, $8F, $74, $90, $75, $91
  .byte $76, $92, $93, $FF, $03, $FF, $89, $8A
loc_FCAF:
  jsr sub_F907
  lda loc_FC56,x
  cmp #$FF
  beq loc_FCD3
  cpx #$45
  bne loc_FCD6
loc_FCBD:
  lda #$08
  ora var_5FEF
  sta var_5FEF
  lda var_5FF8
  pha
  jsr sub_5A19
  pla
  sta var_5FF8
  jmp loc_FC55
loc_FCD3:
  jmp loc_FC4D
loc_FCD6:
  cpx #$55
  beq loc_FCDE
  cpx #$46
  bne loc_FCED
loc_FCDE:
  lda var_5C78
  sta var_5C79
  jsr sub_5A23
  lda #$00
  tax
  jmp loc_FC3B
loc_FCED:
  cpx #$37
  bne loc_FD01
  tax
  jsr sub_F97A
  beq loc_FCFD
  txa
  jsr sub_F987
  bcc loc_FD27
loc_FCFD:
  lda #$72
  bne loc_FD27
loc_FD01:
  jsr sub_F987
  bcs loc_FD16
  cpx #$0F
  beq loc_FD27
  cpx #$39
  beq loc_FD2A
  cpx #$35
  beq loc_FCD3
  bcc loc_FD2A
  bcs loc_FD27
loc_FD16:
  cpx #$35
  beq loc_FD27
  cpx #$1C
  beq loc_FD23
  tax
  lda #$E0
  bne loc_FD2A
loc_FD23:
  ldx #$E0
  bne loc_FD2A
loc_FD27:
  tax
  lda #$00
loc_FD2A:
  jmp loc_FC3B
  .byte $F0, $78, $79, $7A, $7B, $7C, $7D, $7E, $7F, $80, $81, $82, $83, $F0, $A5, $10
  .byte $11, $12, $13, $14, $15, $16, $17, $18, $19, $F0, $F0, $F0, $FF, $1E, $1F, $20
  .byte $21, $22, $23, $24, $25, $26, $F0, $F0, $F0, $FF, $F0, $2C, $2D, $2E, $2F, $30
  .byte $31, $32, $F0, $F0, $F0, $FF, $F0, $FF, $20, $FF, $68, $69, $6A, $6B, $6C, $6D
  .byte $6E, $6F, $70, $71, $FF, $FF, $07, $08, $09, $F0, $04, $05, $06, $F0, $01, $02
  .byte $03, $00, $53, $FF, $03, $FF, $8B, $8C
loc_FD85:
  lda #$04
  bit var_5FED
  beq loc_FD93
  cpx #$53
  bne loc_FD96
  jmp sub_580F
loc_FD93:
  jsr sub_F920
loc_FD96:
  jsr sub_F907
  jsr sub_F941
  jsr sub_F987
  bcc loc_FDBC
  ldy #$00
  sty var_5C7A
  cpx #$1C
  bne loc_FDAE
  lda #$A6
  bne loc_FDEC
loc_FDAE:
  cpx #$35
  bne loc_FDB6
  lda #$A4
  bne loc_FDEC
loc_FDB6:
  txa
  clc
  adc #$50
  bne loc_FDEC
loc_FDBC:
  cmp #$0A
  bcs loc_FDDB
  tay
  lda var_5C7A
  asl a
  sta var_5C7A
  asl a
  asl a
  clc
  adc var_5C7A
  sty var_5C7A
  clc
  adc var_5C7A
  sta var_5C7A
loc_FDD8:
  jmp loc_FC4D
loc_FDDB:
  ldy #$00
  sty var_5C7A
  cpx #$53
  beq loc_FDD8
  cpx #$39
  beq loc_FDEF
  cmp #$F0
  beq loc_FDEF
loc_FDEC:
  tax
  lda #$00
loc_FDEF:
  jmp loc_FC3B
  .res 270, $FF
data_FF00:
  .byte $FF

sub_FF01:
  stx loc_599E+2
loc_FF04:
  sty loc_599E+1
  rts
  .byte $FF

sub_FF09:
  eor #$07
loc_FF0B:
  sta data_FF00
  rts
loc_FF0F:
  rts
data_FF10:
  .byte $FF

sub_FF11:
  lda var_6000
  sta var_2000
  rts
data_FF18:
  .byte $FF

sub_FF19:
  stx var_5FC1
  rts
  .byte $FF, $FF, $FF, $FF

sub_FF21:
  jsr sub_E048
loc_FF24:
  jmp sub_FF21
  .byte $FF, $FF

sub_FF29:
  lda var_5FF9
loc_FF2C:
  stx var_5FF9
  rts
  .byte $FF

sub_FF31:
  ldx var_5FF9
  inx
  txa
  rts
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF40:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF48:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF50:
  .res 48, $FF
data_FF80:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF88:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF90:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FF98:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FFA0:
  .byte $FF, $FF, $FF, $FF, $FF, $FF, $FF, $FF
data_FFA8:
  .res 16, $FF
data_FFB8:
  .res 57, $FF
data_FFF1:
  .byte $0D, $1A, $30, $33, $FF, $FF, $FF, $FF, $FF, $C7, $58
data_FFFC:
  .word sub_CB56, sub_5996
