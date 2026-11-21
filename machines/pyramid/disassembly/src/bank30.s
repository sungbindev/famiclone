; 模块：bank30
; 块 30

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG30_0000"
; 段 30，bank 30，$8000-$BFFF
  .byte $F0, $C9, $00, $F0, $22, $C9, $FF, $F0, $0F, $20, $37, $BA, $20, $56, $BB, $20
  .byte $CB, $BD, $20, $CE, $B8, $4C, $8A, $BF, $20, $D4, $B9, $20, $56, $BB, $20, $92
  .byte $BD, $20, $CE, $B8, $4C, $8A, $BF, $20, $D4, $B9, $20, $56, $BB, $20, $E0, $BD
  .byte $20, $CE, $B8, $4C, $8A, $BF, $20, $03, $BD, $20, $C4, $BA, $A0, $06, $A9, $06
  .byte $20, $30, $F0, $A0, $06, $A9, $1E, $20, $12, $F0, $A0, $06, $A9, $4E, $20, $4E
  .byte $F0, $C9, $FF, $F0, $47, $A0, $06, $A9, $00, $20, $30, $F0, $A0, $06, $A9, $18
  .byte $20, $12, $F0, $A0, $06, $A9, $48, $20, $4E, $F0, $C9, $FF, $F0, $2E, $A0, $06
  .byte $A9, $06, $20, $30, $F0, $A0, $06, $A9, $4E, $20, $0C, $F0, $A0, $06, $A2, $4E
  .byte $20, $3C, $F0, $A0, $06, $A9, $00, $20, $30, $F0, $A0, $06, $A9, $48, $20, $0C
  .byte $F0, $A0, $06, $A2, $48, $20, $3C, $F0, $20, $5D, $BB, $60, $AD, $4B, $00, $C9
  .byte $EA, $D0, $03, $4C, $D3, $C4, $A0, $06, $A9, $0C, $20, $30, $F0, $A0, $06, $A9
  .byte $42, $20, $4E, $F0, $C9, $00, $F0, $22, $C9, $FF, $D0, $0F, $20, $37, $BA, $20
  .byte $56, $BB, $20, $CB, $BD, $20, $CE, $B8, $4C, $36, $C0, $20, $E4, $B8, $20, $56
  .byte $BB, $20, $8B, $BD, $20, $CE, $B8, $4C, $36, $C0, $20, $37, $BA, $20, $56, $BB
  .byte $20, $E7, $BD, $20, $CE, $B8, $4C, $36, $C0, $81, $49, $0F, $DA, $A2, $82, $49
  .byte $0F, $DA, $A2, $83, $16, $CB, $E3, $F9, $83, $49, $0F, $DA, $A2, $81, $49, $0F
  .byte $DA, $A2, $83, $49, $0F, $DA, $A2, $7E, $32, $B8, $C2, $54, $0B, $0A, $09, $08
  .byte $07, $06, $05, $04, $03, $02, $03, $04, $05, $06, $07, $08, $09, $0A, $0B, $0A
  .byte $09, $08, $07, $06, $05, $04, $03, $02, $03, $04, $05, $06, $07, $08, $09, $0A
  .byte $0B, $88, $00, $00, $00, $00, $87, $70, $00, $00, $00, $81, $49, $0F, $E9, $45
  .byte $81, $49, $0F, $CB, $FD, $83, $16, $CB, $E7, $A1, $83, $16, $CB, $E0, $51, $A0
  .byte $06, $A9, $0C, $20, $30, $F0, $A0, $C1, $A9, $3B, $20, $4E, $F0, $C9, $01, $F0
  .byte $20, $A0, $06, $A9, $0C, $20, $30, $F0, $A0, $C1, $A9, $40, $20, $4E, $F0, $C9
  .byte $FF, $F0, $0E, $A0, $C0, $A9, $E9, $20, $30, $F0, $A0, $06, $A2, $0C, $20, $3C
  .byte $F0, $60, $A0, $06, $A9, $0C, $20, $30, $F0, $A0, $C1, $A9, $45, $20, $4E, $F0
  .byte $C9, $01, $F0, $20, $A0, $06, $A9, $0C, $20, $30, $F0, $A0, $C1, $A9, $4A, $20
  .byte $4E, $F0, $C9, $FF, $F0, $0E, $A0, $C0, $A9, $F3, $20, $30, $F0, $A0, $06, $A2
  .byte $0C, $20, $3C, $F0, $60, $20, $4F, $C1, $20, $82, $C1, $60, $A0, $06, $A9, $6C
  .byte $20, $30, $F0, $A0, $C1, $A9, $31, $20, $0C, $F0, $A0, $06, $A2, $00, $20, $3C
  .byte $F0, $A0, $06, $A9, $72, $20, $30, $F0, $A0, $04, $A9, $AA, $20, $00, $F0, $A0
  .byte $C1, $A9, $36, $20, $12, $F0, $A0, $06, $A2, $06, $20, $3C, $F0, $60, $A0, $06
  .byte $A9, $66, $20, $30, $F0, $A0, $06, $A2, $0C, $20, $3C, $F0, $60, $A0, $06, $A9
  .byte $0C, $20, $30, $F0, $20, $2A, $F0, $A0, $04, $A9, $AA, $20, $00, $F0, $20, $2D
  .byte $F0, $A0, $06, $A2, $0C, $20, $3C, $F0, $60, $A0, $06, $A9, $0C, $20, $30, $F0
  .byte $20, $27, $F0, $A0, $06, $A9, $12, $20, $00, $F0, $A0, $04, $A9, $AA, $20, $00
  .byte $F0, $A0, $06, $A2, $5A, $20, $3C, $F0, $A0, $06, $A9, $5A, $20, $00, $F0, $A0
  .byte $06, $A2, $5A, $20, $3C, $F0, $A0, $06, $A9, $0C, $20, $30, $F0, $20, $24, $F0
  .byte $A0, $06, $A9, $12, $20, $00, $F0, $A0, $06, $A2, $60, $20, $3C, $F0, $A0, $06
  .byte $A9, $60, $20, $00, $F0, $A0, $06, $A9, $5A, $20, $0C, $F0, $20, $21, $F0, $A0
  .byte $06, $A2, $12, $20, $3C, $F0, $60, $20, $19, $BC, $20, $B5, $C1, $A0, $C0, $A9
  .byte $E9, $20, $30, $F0, $A0, $06, $A9, $0C, $20, $4E, $F0, $C9, $00, $F0, $2F, $C9
  .byte $01, $F0, $2C, $A0, $C0, $A9, $EE, $20, $30, $F0, $A0, $06, $A9, $0C, $20, $4E
  .byte $F0, $C9, $01, $F0, $1E, $A0, $C0, $A9, $F3, $20, $30, $F0, $A0, $06, $A9, $0C
  .byte $20, $4E, $F0, $C9, $00, $F0, $07, $C9, $01, $F0, $08, $4C, $BF, $C2, $60, $20
  .byte $FD, $C1, $60, $20, $FD, $C1, $A0, $06, $A9, $0C, $20, $30, $F0, $A0, $C0, $A9
  .byte $EE, $20, $0C, $F0, $A0, $06, $A2, $0C, $20, $3C, $F0, $60, $EA, $20, $EE, $C1
  .byte $20, $BC, $C1, $20, $19, $C2, $20, $77, $C2, $20, $EE, $BD, $60, $EA, $20, $EE
  .byte $C1, $A0, $06, $A9, $0C, $20, $30, $F0, $A0, $C0, $A9, $EE, $20, $0C, $F0, $A0
  .byte $06, $A2, $0C, $20, $3C, $F0, $20, $E0, $C2, $60, $A0, $06, $A9, $6C, $20, $30
  .byte $F0, $A0, $06, $A9, $78, $20, $12, $F0, $A0, $06, $A2, $5A, $20, $3C, $F0, $A0
  .byte $06, $A9, $5A, $20, $00, $F0, $A0, $06, $A2, $5A, $20, $3C, $F0, $A0, $06, $A9
  .byte $72, $20, $30, $F0, $A0, $06, $A9, $7E, $20, $12, $F0, $A0, $06, $A2, $60, $20
  .byte $3C, $F0, $A0, $06, $A9, $60, $20, $00, $F0, $A0, $06, $A9, $5A, $20, $0C, $F0
  .byte $20, $21, $F0, $A0, $06, $A2, $12, $20, $3C, $F0, $60, $A0, $06, $A9, $72, $20
  .byte $30, $F0, $A0, $06, $A9, $7E, $20, $12, $F0, $A0, $06, $A2, $5A, $20, $3C, $F0
  .byte $A0, $06, $A9, $6C, $20, $30, $F0, $A0, $06, $A9, $78, $20, $12, $F0, $A0, $06
  .byte $A9, $5A, $20, $06, $F0, $20, $2D, $F0, $A0, $06, $A2, $0C, $20, $3C, $F0, $60
  .byte $A0, $06, $A9, $6C, $20, $30, $F0, $A0, $06, $A9, $78, $20, $4E, $F0, $C9, $00
  .byte $F0, $07, $C9, $01, $F0, $33, $4C, $F2, $C3, $A0, $06, $A9, $72, $20, $30, $F0
  .byte $A0, $06, $A9, $7E, $20, $4E, $F0, $C9, $01, $F0, $0F, $A0, $C0, $A9, $E9, $20
  .byte $30, $F0, $A0, $06, $A2, $0C, $20, $3C, $F0, $60, $A0, $C0, $A9, $F3, $20, $30
  .byte $F0, $A0, $06, $A2, $0C, $20, $3C, $F0, $60, $20, $5B, $C3, $A0, $C0, $A9, $EE
  .byte $20, $30, $F0, $A0, $06, $A9, $0C, $20, $0C, $F0, $A0, $06, $A2, $0C, $20, $3C
  .byte $F0, $60, $20, $5B, $C3, $60, $20, $0A, $C3, $20, $90, $C3, $20, $E0, $C2, $60
  .byte $A0, $04, $A9, $A0, $20, $30, $F0, $A0, $C0, $A9, $FD, $20, $00, $F0, $A0, $04
  .byte $A2, $A0, $20, $3C, $F0, $A0, $5A, $20, $51, $F0, $A0, $04, $A9, $A0, $20, $06
  .byte $F0, $A0, $04, $A2, $A0, $20, $3C, $F0, $60, $20, $00, $C4, $A0, $04, $A9, $A0
  .byte $20, $30, $F0, $A0, $06, $A9, $66, $20, $12, $F0, $A0, $06, $A2, $66, $20, $3C
  .byte $F0, $20, $C9, $C4, $60, $20, $00, $C4, $A0, $04, $A9, $A0, $20, $30, $F0, $A0
  .byte $06, $A9, $66, $20, $0C, $F0, $A0, $06, $A2, $66, $20, $3C, $F0, $20, $C9, $C4
  .byte $60, $A0, $C0, $A9, $F8, $20, $30, $F0, $A0, $06, $A9, $66, $20, $4E, $F0, $C9
  .byte $FF, $D0, $18, $A0, $C0, $A9, $F8, $20, $30, $F0, $A0, $06, $A9, $66, $20, $12
  .byte $F0, $A0, $06, $A2, $66, $20, $3C, $F0, $4C, $61, $C4, $60, $A0, $06, $A9, $66
  .byte $20, $30, $F0, $20, $48, $F0, $C9, $FF, $D0, $18, $A0, $C0, $A9, $F8, $20, $30
  .byte $F0, $A0, $06, $A9, $66, $20, $0C, $F0, $A0, $06, $A2, $66, $20, $3C, $F0, $4C
  .byte $8C, $C4, $60, $A0, $06, $A9, $66, $20, $30, $F0, $20, $48, $F0, $C9, $FF, $D0
  .byte $04, $20, $8C, $C4, $60, $20, $61, $C4, $60, $20, $B3, $C4, $20, $ED, $B7, $20
  .byte $3B, $B8, $60, $A0, $00, $B9, $E4, $C4, $C9, $00, $F0, $07, $20, $03, $A0, $C8
  .byte $4C, $D5, $C4, $60, $54, $55, $52, $54, $4C, $45, $20, $4F, $55, $54, $20, $4F
  .byte $46, $20, $42, $4F, $55, $4E, $44, $0A, $0D, $00, $52, $41, $50, $30, $20, $20
  .byte $20, $D9, $20, $24, $34, $42, $0D, $10, $44, $52, $41, $57, $4C, $45, $20, $20
  .byte $D9, $20, $24, $41, $30, $32, $34, $0D, $1F
  .res 30, $3B
  .byte $0D, $0F, $49, $4E, $49, $54, $58, $59, $20, $20, $CF, $02, $23, $24, $30, $30
  .byte $0D, $09, $CB, $01, $53, $4E, $47, $46, $4C, $54, $0D, $08, $CF, $02, $2F, $44
  .byte $47, $58, $41, $0D, $08, $CE, $02, $23, $44, $47, $58, $41, $0D, $08, $CB, $01
  .byte $4D, $4F, $56, $4D, $46, $0D, $07, $CF, $02, $23, $24, $30, $30, $0D, $09, $CB
  .byte $01, $53, $4E, $47, $46, $4C, $54, $0D, $08, $CF, $02, $2F, $44, $47, $59, $41
  .byte $0D, $08, $CE, $02, $23, $44, $47, $59, $41, $0D, $08, $CB, $01, $4D, $4F, $56
  .byte $4D, $46, $0D, $03, $A5, $00, $0D, $11, $49, $4E, $49, $54, $41, $47, $20, $20
  .byte $CF, $02, $2F, $42, $55, $46, $39, $30, $0D, $09, $CD, $02, $23, $42, $55, $46
  .byte $39, $30, $0D, $08, $CB, $01, $4D, $4F, $56, $46, $4D, $0D, $08, $CF, $02, $2F
  .byte $44, $51, $41, $41, $0D, $08, $CE, $02, $23, $44, $51, $41, $41, $0D, $08, $CB
  .byte $01, $4D, $4F, $56, $4D, $46, $0D, $03, $A5, $00, $0D, $0F, $49, $4E, $4C, $49
  .byte $50, $20, $20, $20, $CD, $02, $23, $24, $37, $35, $0D, $08, $D0, $01, $48, $41
  .byte $49, $47, $30, $0D, $07, $CD, $02, $23, $24, $44, $50, $32, $0D, $07, $C5, $01
  .byte $50, $43, $4E, $54, $0D, $06, $86, $01, $50, $43, $31, $0D, $07, $CA, $01, $50
  .byte $52, $54, $53, $0D, $0F, $5E, $32, $20, $20, $20, $20, $20, $20, $C4, $02, $23
  .byte $24, $30, $32, $0D, $05, $86, $01, $3E, $33, $0D, $0E, $50, $43, $32, $20, $20
  .byte $20, $20, $20, $CB, $01, $44, $50, $32, $0D, $06, $CB, $01, $44, $50, $31, $0D
  .byte $07, $C5, $01, $50, $43, $4E, $54, $0D, $06, $86, $01, $50, $43, $32, $0D, $07
  .byte $CA, $01, $50, $52, $54, $53, $0D, $0E, $5E, $33, $20, $20, $20, $20, $20, $20
  .byte $CB, $01, $44, $50, $32, $0D, $06, $CB, $01, $44, $50, $32, $0D, $07, $C5, $01
  .byte $50, $43, $4E, $54, $0D, $05, $86, $01, $3C, $33, $0D, $07, $CA, $01, $50, $52
  .byte $54, $53, $0D, $13, $44, $50, $31, $20, $20, $20, $20, $20, $CD, $06, $50, $4F
  .byte $49, $4E, $54, $42, $2C, $58, $0D, $08, $D0, $01, $24, $32, $30, $30, $36, $0D
  .byte $03, $9E, $00, $0D, $0B, $CD, $06, $50, $4F, $49, $4E, $54, $42, $2C, $58, $0D
  .byte $08, $D0, $01, $24, $32, $30, $30, $36, $0D, $08, $CD, $01, $24, $32, $30, $30
  .byte $37, $0D, $08, $CD, $01, $24, $32, $30, $30, $37, $0D, $03, $9E, $00, $0D, $0B
  .byte $C1, $06, $50, $4F, $49, $4E, $54, $42, $2C, $58, $0D, $03, $AD, $00, $0D, $03
  .byte $9C, $00, $0D, $03, $9C, $00, $0D, $0B, $CD, $06, $50, $4F, $49, $4E, $54, $42
  .byte $2C, $58, $0D, $08, $D0, $01, $24, $32, $30
  .byte $50, $45, $43, $20, $4C, $4F, $47, $4F
  .byte $20, $43, $43, $33, $2E, $33, $0A, $0A
  .byte $0D, $44, $4F, $4E, $47, $44, $41, $20
  .byte $45, $4C, $45, $43, $54, $52, $4F, $4E
  .byte $49, $43, $20, $43, $4F, $2E, $20, $0A
  .byte $0D, $00

sub_b30_872A:
  lda #$C7
  sta var_006C
  lda #$00
  sta var_006B
  lda #$02
  sta var_007A
  lda #$03
  sta var_007B
  jsr sub_b31_CAE6
  rts
  .byte $20, $20, $D9, $20, $24, $44, $44, $46, $43, $0D, $02, $3B, $0D, $0E, $55, $55
  .byte $4E, $50, $20, $20, $20, $20, $D9, $20, $24, $30, $30, $0D, $0E, $55, $55, $4E
  .byte $50, $48, $20, $20, $20, $D9, $20, $24, $36, $36, $0D, $0E, $55, $55, $58, $50
  .byte $20, $20, $20, $20, $D9, $20, $24, $30, $30, $0D, $0E, $55, $55, $58, $50, $48
  .byte $20, $20, $20, $D9, $20, $24, $36, $37, $0D, $0E, $55, $55, $52, $45, $50, $20
  .byte $20, $20, $D9, $20, $24, $30, $30, $0D, $0E, $55, $55, $52, $45, $50, $48, $20
  .byte $20, $D9, $20, $24, $36, $38, $0D, $0E, $55, $55, $54, $58, $54, $20, $20, $20
  .byte $D9, $20, $24, $30, $30, $0D, $0E, $55, $55, $54, $58, $54, $48, $20, $20, $D9
  .byte $20, $24, $36, $39, $0D, $0E, $55, $55, $56, $41, $4C, $20, $20, $20, $D9, $20
  .byte $24, $30, $30, $0D, $0E, $55, $55, $56, $41, $4C, $48, $20, $20, $D9, $20, $24
  .byte $36, $42, $0D, $0E, $55, $55, $50, $52, $4F, $20, $20, $20, $D9, $20, $24, $30
  .byte $30, $0D, $0E, $55, $55, $50, $52, $4F, $48, $20, $20, $D9, $20, $24, $37, $30
  .byte $0D, $0E, $4C, $63, $D7

sub_b30_8803:
  jmp sub_b31_D793

sub_b30_8806:
  jmp loc_b31_C85D

sub_b30_8809:
  jmp loc_b31_C87F
  .byte $4C, $69, $C8, $4C, $B8, $C8

sub_b30_8812:
  jmp loc_b31_C8BD
  .byte $4C, $03, $CE, $4C, $16, $CE, $4C, $A9, $CB, $4C, $EE, $D8, $4C, $0E, $D7, $4C
  .byte $A4, $CA, $4C, $E6, $CA

sub_b30_882A:
  jmp sub_b31_CB0A
  .byte $4C, $48, $D6, $4C, $9C, $D3, $4C, $80, $D3, $4C, $D4, $D3, $4C, $8F, $D1, $4C
  .byte $54, $D4

sub_b30_883F:
  jmp loc_b31_D4CA
  .byte $4C, $06, $A0, $4C, $BC, $D7, $4C, $49, $CB, $4C, $71, $C8, $4C, $9C, $D8, $4C
  .byte $EA, $D0, $4C, $58, $D1, $4C, $88, $D1, $4C, $A0, $CB

sub_b30_885D:
  jsr sub_b30_A009
  jsr sub_b31_C72A
  jsr sub_b31_CB15
  jsr sub_b31_CB2F
  lda #$00
  sta var_00D3
  lda #$00
  sta var_00F4
  lda #$00
  sta var_00D6
  jsr sub_b31_CB0A
  lda #$19
  sta var_007B
  jsr sub_b31_CB0A

sub_b30_887F:
  jsr sub_b31_CA42
  jsr sub_b31_CA73
  ldx #$3F
  jsr sub_b31_CB45
  jsr sub_b31_CA98
  jsr sub_b31_CBA9
  lda #$BB
  jsr sub_b31_CBBC
  lda #$80
  jsr sub_b31_CBBC
  lda #$FF
  sta var_00B8
  lda #$77
  sta var_00B9
  lda #$00
  sta var_00DE
  sta var_00DF
  lda var_0006
  sta var_0018
  lda var_0007
  sta var_0019
  lda #$00
  sta var_00D0
  lda #$68
  sta var_00D1
  lda #$00
  jsr sub_b31_CF4C

sub_b30_88BD:
  ldx #$FF
  txs
  jsr sub_b31_DACB
  bcc sub_b30_88DC
  lda #$C8
  sta var_006C
  lda #$D6
  sta var_006B
  jsr sub_b31_CAE6
  jsr sub_b31_CB0A
  jmp loc_b31_C87F
  .byte $53, $54, $4F, $50, $21, $00

sub_b30_88DC:
  lda var_00B8
  sta var_004E
  lda var_00B9
  sta var_004F
  ldy #$01
  lda (var_00B8),y
  cmp #$80
  bne sub_b30_88F2
  jsr a:$00B1
  jmp loc_b31_C8BD

sub_b30_88F2:
  lda var_00F4
  beq sub_b30_891F
  lda var_00DE
  bne loc_b30_88FE
  lda var_00DF
  beq sub_b30_891F
loc_b30_88FE:
  lda var_00B8
  pha
  lda var_00B9
  pha
  ldy #$01
  lda (var_00B8),y
  cmp #$9E
  beq loc_b30_8919
  cmp #$9F
  beq loc_b30_8919
  jsr sub_b31_D8EE
  jsr sub_b31_D763
  jsr sub_b31_CB0A
loc_b30_8919:
  pla
  sta var_00B9
  pla
  sta var_00B8

sub_b30_891F:
  jmp loc_b31_CDB4
  .byte $00, $04, $00, $00, $02, $02, $03, $03, $01, $01, $01, $4D, $49, $53, $53, $20
  .byte $29, $00, $46, $55, $4E, $43, $54, $49, $4F, $4E, $20, $4E, $45, $45, $44, $20
  .byte $4D, $4F, $52, $45, $20, $49, $4E, $50, $55, $54, $00, $46, $55, $4E, $43, $54
  .byte $49, $4F, $4E, $20, $4E, $4F, $54, $20, $4C, $49, $4B, $45, $20, $49, $54, $20
  .byte $41, $53, $20, $49, $4E, $50, $55, $54, $00, $4E, $4F, $20, $45, $4E, $4F, $55
  .byte $47, $48, $20, $53, $54, $4F, $52, $41, $47, $45, $00, $59, $4F, $55, $20, $4E
  .byte $4F, $20, $53, $41, $59, $20, $57, $41, $54, $48, $20, $54, $4F, $20, $44, $4F
  .byte $20, $57, $49, $54, $48, $20, $00, $50, $52, $4F, $43, $45, $44, $55, $52, $45
  .byte $20, $4E, $4F, $20, $46, $4F, $55, $4E, $44, $00, $46, $55, $4E, $43, $54, $49
  .byte $4F, $4E, $20, $48, $41, $56, $45, $20, $4E, $4F, $20, $52, $45, $54, $55, $52
  .byte $4E, $20, $00, $46, $55, $4E, $43, $54, $49, $4F, $4E, $20, $43, $41, $4E, $20
  .byte $4F, $4E, $4C, $59, $20, $42, $45, $20, $55, $53, $45, $44, $20, $49, $4E, $20
  .byte $41, $20, $50, $52, $4F, $43, $45, $44, $55, $52, $45, $00, $56, $41, $4C, $55
  .byte $52, $45, $20, $4E, $4F, $20, $46, $4F, $55, $4E, $44, $00, $56, $41, $4C, $55
  .byte $52, $45, $20, $4E, $41, $4D, $45, $20, $54, $4F, $4F, $20, $53, $48, $4F, $52
  .byte $54, $00, $45, $52, $52, $4F, $52, $20, $49, $4E, $20, $50, $52, $4F, $43, $45
  .byte $44, $55, $52, $45, $20, $44, $45, $46, $49, $4E, $45, $21, $00, $57, $4F, $52
  .byte $44, $20, $4F, $52, $20, $4C, $49, $53, $54, $20, $45, $4D, $50, $54, $59, $00

sub_b30_8A42:
  lda #$00
  sta var_0006
  lda #$78
  sta var_0007
  lda #$00
  sta var_0018
  lda #$78
  sta var_0019
  lda #$FC
  sta var_00E8
  lda #$DD
  sta var_00E9
  lda #$66
  sta var_0003
  lda #$00
  sta var_0002
  lda #$67
  sta var_0005
  lda #$00
  sta var_0004
  lda #$00
  sta var_00E0
  lda #$69
  sta var_00E1
  rts

sub_b30_8A73:
  ldy #$B1
  ldx #$00

sub_b30_8A77:
  lda data_b31_CA84,x
  sta a:var_0000,y
  inx
  iny
  cpy #$C5
  bne sub_b30_8A77
  rts
  .byte $E6, $B8, $D0, $02, $E6, $B9, $AD, $00
  .byte $00, $C9, $3A, $B0, $06, $38, $E9, $30
  .byte $38, $E9, $D0, $60

sub_b30_8A98:
  lda #$FF
  sta var_00B8
  lda #$02
  sta var_00B9
  rts
  .byte $20, $2C, $D0, $A0, $00, $B1, $6B, $C9, $01, $D0, $33, $E6, $6B, $D0, $02, $E6
  .byte $6C, $A4, $6C, $A5, $6B, $20, $30, $F0, $20, $4B, $F0, $A5, $6B, $48, $A5, $6C
  .byte $48, $A9, $00, $85, $6B, $A9, $01, $85, $6C, $20, $E6, $CA, $68, $85, $6C, $68
  .byte $85, $6B, $A9, $04, $18, $65, $6B, $85, $6B, $90, $02, $E6, $6C, $60, $C9, $22
  .byte $D0, $10

sub_b30_8AE3:
  jsr sub_b31_D8BC

sub_b30_8AE6:
  ldy #$00
  lda (var_006B),y
  beq sub_b30_8AF2
  jsr sub_b31_D793
  jmp loc_b31_CAE3

sub_b30_8AF2:
  rts
  .byte $A0, $00, $B1, $6B, $C9, $00, $F0, $09, $20, $93, $D7, $20, $BC, $D8, $4C, $F5
  .byte $CA, $A9, $5D, $20, $93, $D7, $60

sub_b30_8B0A:
  lda #$0D
  jsr sub_b31_D793
  lda #$0A
  jsr sub_b31_D793
  rts

sub_b30_8B15:
  lda #$00
  sta var_0069
  lda #$6B
  sta var_006A
  lda #$00
  sta var_0067
  lda #$6B
  sta var_0068
  lda #$00
  ldy #$08

sub_b30_8B29:
  sta (var_0067),y
  dey
  bpl sub_b30_8B29
  rts

sub_b30_8B2F:
  lda #$00
  sta var_0000
  sta var_0014
  lda #$70
  sta var_0001
  sta var_0015
  ldy #$08
  lda #$00

sub_b30_8B3F:
  sta (var_0000),y
  dey
  bpl sub_b30_8B3F
  rts

sub_b30_8B45:
  txa
  jsr sub_b31_D793
  lda #$20
  ldx #$00

sub_b30_8B4D:
  sta var_0300,x
  inx
  cpx #$00
  bne sub_b30_8B4D
  ldx #$00

sub_b30_8B57:
  jsr sub_b31_D763
  cmp #$0D
  beq sub_b30_8B99
  cmp #$F2
  bne sub_b30_8B6D
  cpx #$00
  beq sub_b30_8B57
  dex
  jsr sub_b31_D793
  jmp loc_b31_CB57

sub_b30_8B6D:
  cmp #$F3
  bne sub_b30_8B7F
  cpx #$FF
  beq sub_b30_8B57
  lda var_0300,x
  jsr sub_b31_D793
  inx
  jmp loc_b31_CB57

sub_b30_8B7F:
  cmp #$20
  bcc sub_b30_8B57
  cmp #$80
  bcs sub_b30_8B57
  cpx #$FF
  beq loc_b30_8B95
  sta var_0300,x
  jsr sub_b31_D793
  inx
  jmp loc_b31_CB57
loc_b30_8B95:
  cmp #$0D
  bne sub_b30_8B57

sub_b30_8B99:
  sta var_0300,x
  jsr sub_b30_A027
  rts

sub_b30_8BA0:
  lda var_00B8
  bne sub_b30_8BA6
  dec var_00B9

sub_b30_8BA6:
  dec var_00B8
  rts

sub_b30_8BA9:
  jsr a:$00B1
  cmp #$20
  beq sub_b30_8BA9
  ldy #$00
  cmp #$00
  beq loc_b30_8BBA
  cmp #$0D
  bne sub_b30_8BC7
loc_b30_8BBA:
  lda #$80

sub_b30_8BBC:
  ldy #$00
  sta (var_0006),y
  inc var_0006
  bne sub_b30_8BC6
  inc var_0007

sub_b30_8BC6:
  rts

sub_b30_8BC7:
  cmp #$22
  bne sub_b30_8BFF
loc_b30_8BCB:
  jsr sub_b31_CBBC
  jsr a:$00B1
  cmp #$27
  bne loc_b30_8BDB
  jsr sub_b31_CD3B
  jmp loc_b31_CBCE
loc_b30_8BDB:
  cmp #$20
  beq loc_b30_8BE7
  cmp #$00
  beq loc_b30_8BE7
  cmp #$0D
  bne loc_b30_8BCB
loc_b30_8BE7:
  jsr sub_b31_CBA0
  lda #$00
  jsr sub_b31_CBBC
  jmp sub_b31_CBA9
loc_b30_8BF2:
  dec var_0007
  ldy #$FF
  lda #$00
  sta (var_0006),y
  inc var_0007
  jmp sub_b31_CBA9

sub_b30_8BFF:
  cmp #$5B
  bne sub_b30_8C42
  ldx #$00
loc_b30_8C05:
  inx
loc_b30_8C06:
  jsr sub_b31_CBBC
loc_b30_8C09:
  jsr a:$00B1
  jsr sub_b31_CCBB
  cmp #$27
  bne loc_b30_8C19
  jsr sub_b31_CD3B
  jmp loc_b31_CC09
loc_b30_8C19:
  cmp #$5B
  beq loc_b30_8C05
  cmp #$5D
  bne loc_b30_8C29
  jsr sub_b31_CBBC
  dex
  bne loc_b30_8C09
  beq loc_b30_8BF2
loc_b30_8C29:
  cmp #$00
  beq loc_b30_8C31
  cmp #$0D
  bne loc_b30_8C06
loc_b30_8C31:
  jsr sub_b31_CBA0
  lda #$5D
  cpx #$00
  beq loc_b30_8BF2
loc_b30_8C3A:
  jsr sub_b31_CBBC
  dex
  bne loc_b30_8C3A
  beq loc_b30_8BF2

sub_b30_8C42:
  cmp #$3A
  bne sub_b30_8C53
loc_b30_8C46:
  jsr sub_b31_CBBC
  jsr a:$00B1
  jsr sub_b31_CD0C
  bne loc_b30_8C46
  beq loc_b30_8BE7

sub_b30_8C53:
  jsr sub_b31_CD70
  bne sub_b30_8C6A
  dey
  tya
  clc
  adc var_00B8
  sta var_00B8
  bcc sub_b30_8C63
  inc var_00B9

sub_b30_8C63:
  txa
  jsr sub_b31_CBBC
  jmp sub_b31_CBA9

sub_b30_8C6A:
  jsr sub_b31_CBA0
  lda var_0006
  sta var_00E2
  lda var_0007
  sta var_00E3
  jsr sub_b31_CBBC
  jsr a:$00B1
  bcc sub_b30_8C83
  cmp #$2E
  beq sub_b30_8C83
  bne loc_b30_8C9F

sub_b30_8C83:
  jsr sub_b31_CBA0
  jsr sub_b31_D972
  jsr sub_b31_CD0C
  bne loc_b30_8CA5
  lda #$22
  ldy #$00
  sta (var_00E2),y
  jsr sub_b31_CBA0
  lda #$00
  jsr sub_b31_CBBC
  jmp sub_b31_CBA9
loc_b30_8C9F:
  jsr sub_b31_CBBC
  jsr a:$00B1
loc_b30_8CA5:
  cmp #$3A
  beq loc_b30_8CB3
  jsr sub_b31_CD0C
  bne loc_b30_8C9F
  lda #$FF
  jmp loc_b31_CC90
loc_b30_8CB3:
  jsr a:$00B1
  lda #$C7
  jmp loc_b31_CC90
  .byte $C9, $5B, $D0, $1E, $20, $B1, $00, $C9, $20, $F0, $F9, $20, $A0, $CB, $C6, $07
  .byte $A0, $FF, $B1, $06, $E6, $07, $C9, $5B, $F0, $05, $A9, $20, $20, $BC, $CB, $A9
  .byte $5B, $60, $C9, $20, $D0, $FB, $20, $B1, $00, $C9, $20, $F0, $F9, $C6, $07, $A0
  .byte $FF, $B1, $06, $E6, $07, $C9, $5B, $F0, $14, $20, $B7, $00, $C9, $5D, $F0, $0D
  .byte $C9, $00, $F0, $09, $C9, $0D, $F0, $05, $A9, $20, $20, $BC, $CB, $20, $B7, $00
  .byte $60

sub_b30_8D0C:
  cmp #$28
  beq sub_b30_8D3A
  cmp #$29
  beq sub_b30_8D3A
  cmp #$2B
  beq sub_b30_8D3A
  cmp #$2D
  beq sub_b30_8D3A
  cmp #$2A
  beq sub_b30_8D3A
  cmp #$2F
  beq sub_b30_8D3A
  cmp #$20
  beq sub_b30_8D3A
  cmp #$0D
  beq sub_b30_8D3A
  cmp #$3E
  beq sub_b30_8D3A
  cmp #$3D
  beq sub_b30_8D3A
  cmp #$3C
  beq sub_b30_8D3A
  cmp #$00

sub_b30_8D3A:
  rts
  .byte $20, $BC, $CB, $20, $B1, $00, $C9, $00, $F0, $22, $C9, $0D, $F0, $1E, $C9, $27
  .byte $F0, $06, $20, $BC, $CB, $4C, $3E, $CD, $A0, $01, $B1, $B8, $C9, $27, $D0, $0F
  .byte $20, $BC, $CB, $20, $BC, $CB, $20, $B1, $00, $4C, $3E, $CD, $20, $A0, $CB, $A9
  .byte $27, $20, $BC, $CB, $60

sub_b30_8D70:
  ldx #$82
  lda #$04
  sta var_000A
  lda #$DB
  sta var_000B

sub_b30_8D7A:
  ldy #$FF

sub_b30_8D7C:
  iny
  lda (var_000A),y
  beq sub_b30_8DA5
  cmp (var_00B8),y
  beq sub_b30_8D7C
  bne sub_b30_8D92

sub_b30_8D87:
  lda #$FF
loc_b30_8D89:
  rts

sub_b30_8D8A:
  lda (var_00B8),y
  jsr sub_b31_CD0C
  bne loc_b30_8D97
  rts

sub_b30_8D92:
  iny
  lda (var_000A),y
  bne sub_b30_8D92
loc_b30_8D97:
  tya
  sec
  adc var_000A
  sta var_000A
  bcc sub_b30_8DA1
  inc var_000B

sub_b30_8DA1:
  inx
  jmp loc_b31_CD7A

sub_b30_8DA5:
  cpy #$01
  beq loc_b30_8D89
  cpy #$00
  beq sub_b30_8D87
  bne sub_b30_8D8A
  lda #$80
  jsr sub_b31_CF4C

sub_b30_8DB4:
  jsr a:$00B1
  cmp #$85
  bne sub_b30_8DC3
  lda #$81
  jsr sub_b31_CF4C
  jsr a:$00B1

sub_b30_8DC3:
  cmp #$82
  bne sub_b30_8DCD
  jsr sub_b31_CF4C
  jmp loc_b31_CDB4

sub_b30_8DCD:
  cmp #$80
  bcc sub_b30_8E00
  cmp #$8B
  bcs sub_b30_8DD8
  jmp loc_b31_D0F8

sub_b30_8DD8:
  cmp #$FF
  beq loc_b30_8DF8
  jsr sub_b31_CF4C
  and #$7F
  tay
  lda (var_00E8),y
  tax
  beq sub_b30_8DF2
  lda #$80
loc_b30_8DE9:
  jsr sub_b31_CF4C
  dex
  bne loc_b30_8DE9
  jmp loc_b31_CDB4

sub_b30_8DF2:
  jmp sub_b31_D06B
  .byte $4C, $03, $CE
loc_b30_8DF8:
  jsr sub_b31_D1FC
  beq sub_b30_8DF2
  jmp loc_b31_CDB4

sub_b30_8E00:
  jsr sub_b31_CF78
  jsr a:$00B1
  cmp #$83
  bcc sub_b30_8E13
  beq sub_b30_8E13
  cmp #$88
  bcs sub_b30_8E13
  jmp loc_b31_CE93

sub_b30_8E13:
  jsr sub_b31_CBA0
  ldy #$00
  lda (var_0004),y
  bne sub_b30_8E1F
  jmp loc_b31_D09D

sub_b30_8E1F:
  cmp #$80
  bne sub_b30_8E31
  jsr sub_b31_CF62
  ldy #$00
  lda (var_0004),y
  cmp #$80
  bne sub_b30_8E31
  jmp loc_b31_CDB4

sub_b30_8E31:
  cmp #$82
  beq loc_b30_8E71
  cmp #$8B
  bcs sub_b30_8E43
  cmp #$80
  bcc sub_b30_8E43
  jsr sub_b31_D06B
  jmp loc_b31_CE16

sub_b30_8E43:
  ldy #$00
  lda (var_0004),y
  cmp #$80
  bcc loc_b30_8E63
  and #$7F
  tay
  lda data_b31_CED8,y
  bne loc_b30_8E63
  ldy #$01
  lda (var_00B8),y
  cmp #$88
  beq loc_b30_8E66
  cmp #$89
  beq loc_b30_8E66
  cmp #$8A
  beq loc_b30_8E66
loc_b30_8E63:
  jmp sub_b31_D06B
loc_b30_8E66:
  lda #$80
  jsr sub_b31_CF4C
  jsr a:$00B1
  jmp loc_b31_CE93
loc_b30_8E71:
  jsr a:$00B1
  cmp #$83
  beq loc_b30_8E87
  cmp #$88
  beq loc_b30_8E8D
  cmp #$89
  beq loc_b30_8E8D
  cmp #$8A
  beq loc_b30_8E8D
  jmp loc_b31_D0F1
loc_b30_8E87:
  jsr sub_b31_CF62
  jmp loc_b31_CE03
loc_b30_8E8D:
  jsr sub_b31_CF4C
  jmp loc_b31_CDB4
  .byte $A0, $00, $B1, $B8, $29, $7F, $AA, $B1, $04, $29, $7F, $A8, $B9, $22, $C9, $38
  .byte $FD, $22, $C9, $90, $06, $20, $6B, $D0, $4C, $93, $CE, $A0, $00, $B1, $B8, $20
  .byte $4C, $CF, $4C, $B4, $CD, $A0, $00, $B1, $04, $C9, $80, $F0, $10, $C9, $82, $F0
  .byte $06, $20, $6B, $D0, $4C, $B8, $CE, $20, $62, $CF, $4C, $89, $CD, $A0, $00, $B1
  .byte $B8, $20, $62, $CF, $60, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $01, $01
  .byte $00
  .byte $00, $00, $00, $01, $00, $01, $01, $01, $01, $01, $01, $01, $01, $01, $00, $00
  .byte $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00, $01, $01, $01, $01, $01
  .byte $01, $01, $01, $01, $01, $01, $00, $00, $00, $00, $00, $00, $00, $00, $00, $00
  .byte $00, $00, $01, $01
  .res 49, $00

sub_b30_8F4C:
  ldy #$00
  pha
  inc var_0004
  bne sub_b30_8F55
  inc var_0005

sub_b30_8F55:
  lda var_0005
  cmp #$68
  bcc sub_b30_8F5E
  jmp loc_b31_D0EA

sub_b30_8F5E:
  pla
  sta (var_0004),y
  rts

sub_b30_8F62:
  lda var_0004
  sec
  sbc #$01
  sta var_0004
  bcs sub_b30_8F6D
  dec var_0005

sub_b30_8F6D:
  ldy #$01
  lda (var_0004),y
  rts
loc_b30_8F72:
  jmp loc_b31_D122
loc_b30_8F75:
  jmp loc_b31_D11B

sub_b30_8F78:
  lda #$02
  clc
  adc var_0002
  sta var_0002
  bcc sub_b30_8F83
  inc var_0003

sub_b30_8F83:
  lda var_0003
  cmp #$67
  bcc sub_b30_8F8C
  jmp loc_b31_D0EA

sub_b30_8F8C:
  ldy #$00
  lda (var_00B8),y
  cmp #$3A
  bne sub_b30_8FFB
  jsr a:$00B1
  cmp #$00
  beq loc_b30_8F72
  lda var_00B8
  sta var_006B
  lda var_00B9
  sta var_006C
  jsr sub_b31_D3D4
  lda var_006F
  cmp #$80
  beq loc_b30_8F75
  ldy #$FF
loc_b30_8FAE:
  iny
  lda (var_0099),y
  bne loc_b30_8FAE
  tya
  clc
  adc var_00B8
  sta var_00B8
  bcc loc_b30_8FBD
  inc var_00B9
loc_b30_8FBD:
  tya
  sec
  adc var_0099
  sta var_0099
  bcc loc_b30_8FC7
  inc var_009A
loc_b30_8FC7:
  ldy #$00
  lda var_0006
  sta (var_0002),y
  iny
  lda var_0007
  sta (var_0002),y
  ldy #$00
  lda (var_0099),y
  cmp #$01
  bne loc_b30_8FEB
  ldx #$05
loc_b30_8FDC:
  lda (var_0099),y
  jsr sub_b31_CBBC
  inc var_0099
  bne loc_b30_8FE7
  inc var_009A
loc_b30_8FE7:
  dex
  bpl loc_b30_8FDC
  rts
loc_b30_8FEB:
  lda (var_0099),y
  jsr sub_b31_CBBC
  inc var_0099
  bne loc_b30_8FF6
  inc var_009A
loc_b30_8FF6:
  cmp #$00
  bne loc_b30_8FEB
  rts

sub_b30_8FFB:
  ldy #$01
  lda var_00B9
  sta (var_0002),y
  dey
  lda var_00B8
  sta (var_0002),y
  lda (var_00B8),y
  cmp #$01
  bne sub_b30_9018
  lda #$05
  clc
  adc var_00B8
  sta var_00B8
  bcc loc_b30_9017
  inc var_00B9
loc_b30_9017:
  rts

sub_b30_9018:
  jsr a:$00B1
  cmp #$00
  bne sub_b30_9018
  rts
  .byte $A5, $02, $38, $E9, $02, $85, $02, $B0, $02, $C6, $03, $60, $A0, $00, $B1, $02
  .byte $85, $6B, $C8, $B1, $02, $85, $6C, $38, $A5, $02, $E9, $02, $85, $02, $B0, $02
  .byte $C6, $03, $A2, $6B, $A0, $18, $20, $F7, $D6, $90, $08, $A5, $6B, $85, $06, $A5
  .byte $6C, $85, $07, $60, $18, $A5, $02, $69, $02, $85, $02, $90, $02, $E6, $03, $A0
  .byte $00, $A5, $06, $91, $02, $C8, $A5, $07, $91, $02, $60

sub_b30_906B:
  ldy #$00
  lda (var_0004),y
  bmi sub_b30_9074
  jmp loc_b31_D241

sub_b30_9074:
  lda #$20
  sta var_00E2
  lda #$DD
  sta var_00E3
  ldy #$00
  lda (var_0004),y
  asl a
  tay
  lda (var_00E2),y
  sta var_00D4
  iny
  lda (var_00E2),y
  sta var_00D5
  jmp (var_00D4)
  .byte $52, $45, $53, $55, $4C, $54, $3A, $20, $00, $20, $B1, $00, $4C, $93, $CE, $A0
  .byte $01, $B1, $B8, $C9, $88, $F0, $F2, $C9, $89, $F0, $EE, $C9, $8A, $F0, $EA, $A5
  .byte $DE, $D0, $18, $A5, $DF, $D0, $14, $A9, $D0, $85, $6C, $A9, $8E, $85, $6B, $20
  .byte $E6, $CA, $20, $A1, $CA, $20, $0A, $CB, $4C, $7F, $C8, $A9, $C9, $85, $6C, $A9
  .byte $7D, $85, $6B, $20, $E6, $CA, $20, $A1, $CA, $20, $0A, $CB, $4C, $7F, $C8, $20
  .byte $62, $CF, $A0, $00, $B1, $04, $F0, $03, $4C, $14, $D1, $60, $A0, $C9, $A2, $6B
  .byte $4C, $37, $D1, $A0, $C9, $A2, $2D, $4C, $37, $D1, $A0, $C9, $A2, $34, $4C, $37
  .byte $D1, $A0, $C9, $A2, $4D, $4C, $37, $D1, $A0, $C9, $A2, $7D, $4C, $37, $D1, $A0
  .byte $C9, $A2, $99, $4C, $37, $D1, $A0, $C9, $A2, $AC, $4C, $37, $D1, $A0, $C9, $A2
  .byte $EE, $4C, $37, $D1, $A0, $C9, $A2, $FE, $4C, $37, $D1, $A0, $CA, $A2, $14, $4C
  .byte $37, $D1, $A0, $CA, $A2, $2F, $4C, $37, $D1, $84, $E3, $86, $E2, $A0, $FF, $C8
  .byte $B1, $E2, $F0, $06, $20, $93, $D7, $4C, $3D, $D1, $20, $58, $D1, $4C, $7F, $C8
  .byte $20, $49, $4E, $20, $4C, $49, $4E, $45, $3A, $00, $A9, $D1, $85, $6C, $A9, $4E
  .byte $85, $6B, $20, $E6, $CA, $A5, $4F, $85, $B9, $A5, $4E, $85, $B8, $20, $EE, $D8
  .byte $20, $0A, $CB, $60, $46, $4C, $4F, $41, $54, $20, $4F, $56, $45, $52, $46, $4C
  .byte $4F, $57, $20, $45, $52, $52, $4F, $52, $20, $00, $A0, $D1, $A2, $72, $4C, $37
  .byte $D1, $A9, $00, $85, $1C, $A5, $00, $85, $E2, $A5, $01, $85, $E3, $A5, $E3, $D0
  .byte $07, $A5, $E2, $D0, $03, $A9, $FF, $60, $A9, $06, $18, $65, $E2, $85, $D4, $A9
  .byte $00, $65, $E3, $85, $D5, $A0, $FF, $C8, $B1, $D4, $F0, $06, $D1, $12, $F0, $F7
  .byte $D0, $05, $B1, $12, $D0, $01, $60, $A0, $02, $B1, $E2, $AA, $C8, $B1, $E2, $85
  .byte $E3, $86, $E2, $E6, $1C, $4C, $9B, $D1, $A5, $73, $85, $E2, $A5, $74, $85, $E3
  .byte $A2, $00, $A0, $00, $B1, $E2, $C9, $80, $D0, $01, $60, $C8, $B1, $E2, $D0, $FB
  .byte $98, $38, $65, $E2, $85, $E2, $90, $02, $E6, $E3, $E8, $4C, $E0, $D1, $20, $B1
  .byte $00, $A5, $B8, $85, $12, $A5, $B9, $85, $13, $20, $8F, $D1, $F0, $03, $4C, $0D
  .byte $D1, $98, $38, $65, $D4, $85, $E2, $A5, $D5, $69, $00, $85, $E3, $A5, $1C, $20
  .byte $4C, $CF, $20, $DE, $D1, $8A, $48, $F0, $08, $A9, $80, $20, $4C, $CF, $CA, $D0
  .byte $FA, $20, $B1, $00, $D0, $FB, $68, $60, $45, $58, $45, $43, $55, $54, $49, $4E
  .byte $47, $20, $00, $A9, $00, $85, $B0, $E6, $DE, $D0, $02, $E6, $DF, $A5, $00, $85
  .byte $E2, $A5, $01, $85, $E3, $A0, $00, $B1, $04, $AA, $F0, $10, $A0, $02, $B1, $E2
  .byte $48, $C8, $B1, $E2, $85, $E3, $68, $85, $E2, $CA, $D0, $F0, $A9, $06, $18, $65
  .byte $E2, $85, $E2, $90, $02, $E6, $E3, $A5, $F4, $F0, $1D, $A9, $D2, $85, $6C, $A9
  .byte $36, $85, $6B, $20, $E6, $CA, $A0, $FF, $C8, $B1, $E2, $20, $93, $D7, $C9, $00
  .byte $D0, $F6, $20, $0A, $CB, $4C, $9D, $D2, $A0, $FF, $C8, $B1, $E2, $D0, $FB, $98
  .byte $38, $65, $E2, $85, $E2, $90, $02, $E6, $E3, $A5, $DF, $D0, $08, $A5, $DE, $C9
  .byte $02, $B0, $02, $90, $3E, $A5, $05, $85, $D5, $C6, $D5, $A5, $04, $85, $D4, $A0
  .byte $FF, $B1, $D4, $D0, $2E, $A0, $01, $B1, $B8, $C9, $80, $D0, $26, $C8, $B1, $B8
  .byte $C9, $8B, $D0, $1F, $20, $62, $CF, $48, $20, $62, $CF, $20, $62, $CF, $68, $20
  .byte $4C, $CF, $A5, $DE, $D0, $02, $C6, $DF, $C6, $DE, $A0, $02, $A5, $D3, $91, $E0
  .byte $4C, $F4, $D2, $20, $5B, $D3, $A5, $E2, $85, $B8, $A5, $E3, $85, $B9, $20, $DE
  .byte $D1, $E0, $00, $F0, $47, $8A, $48, $20, $2C, $D0, $68, $AA, $CA, $D0, $F6, $A5
  .byte $02, $85, $9D, $A5, $03, $85, $9E, $20, $A0, $CB, $20, $B1, $00, $C9, $80, $F0
  .byte $2B, $A5, $B8, $85, $6B, $A5, $B9, $85, $6C, $20, $B1, $00, $C9, $00, $D0, $F9
  .byte $A0, $02, $B1, $9D, $85, $6D, $C8, $B1, $9D, $85, $6E, $A9, $02, $18, $65, $9D
  .byte $85, $9D, $90, $02, $E6, $9E, $20, $56, $D6, $4C, $18, $D3, $EA, $A9, $FF, $85
  .byte $B0, $4C, $B8, $C8, $A5, $DE, $D0, $02, $C6, $DF, $C6, $DE, $60, $A5, $E0, $18
  .byte $69, $03, $85, $E0, $90, $02, $E6, $E1, $A5, $E1, $C9, $6B, $90, $03, $4C, $EA
  .byte $D0, $A0, $00, $A5, $B8, $91, $E0, $C8, $A5, $B9, $91, $E0, $C8, $A5, $D3, $91
  .byte $E0, $60, $A0, $00, $B1, $E0, $85, $B8, $C8, $B1, $E0, $85, $B9, $C8, $B1, $E0
  .byte $85, $D3, $A5, $E0, $38, $E9, $03, $85, $E0, $B0, $02, $C6, $E1, $60, $A5, $67
  .byte $85, $E2, $A5, $68, $85, $E3, $A5, $E3, $D0, $05, $A5, $E2, $D0, $01, $60, $A0
  .byte $04, $A5, $DE, $D1, $E2, $D0, $10, $C8, $A5, $DF, $D1, $E2, $D0, $09, $A0, $06
  .byte $A9, $00, $91, $E2, $20, $67, $D4, $A0, $02, $B1, $E2, $AA, $C8, $B1, $E2, $85
  .byte $E3, $86, $E2, $4C, $A4, $D3, $A5, $67, $85, $E2, $A5, $68, $85, $E3, $A9, $00
  .byte $85, $99, $85, $9A, $A9, $80, $85, $6F, $A5, $E3, $D0, $05, $A5, $E2, $D0, $01
  .byte $60, $A9, $06, $18, $65, $E2, $85, $D4, $A9, $00, $65, $E3, $85, $D5, $A0, $FF
  .byte $C8, $B1, $D4, $F0, $06, $D1, $6B, $F0, $F7, $D0, $3C, $B1, $6B, $D0, $38, $A0
  .byte $04, $B1, $E2, $85, $E6, $C8, $B1, $E2, $85, $E7, $A5, $E6, $C5, $DE, $D0, $13
  .byte $A5, $E7, $C5, $DF, $D0, $0D, $A5, $D4, $85, $99, $A5, $D5, $85, $9A, $A9, $FF
  .byte $85, $6F, $60, $A5, $E6, $D0, $10, $A5, $E7, $D0, $0C, $A5, $D4, $85, $99, $A5
  .byte $D5, $85, $9A, $A9, $00, $85, $6F, $A0, $02, $B1, $E2, $AA, $C8, $B1, $E2, $85
  .byte $E3, $86, $E2, $4C, $E6, $D3, $A9, $00, $A0, $00, $91, $99, $A5, $99, $38, $E9
  .byte $06, $85, $E2, $A5, $9A, $E9, $00, $85, $E3, $A0, $02, $B1, $E2, $85, $D4, $C8
  .byte $B1, $E2, $85, $D5, $A0, $06, $B1, $D4, $D0, $0D, $A0, $03, $B1, $D4, $F0, $07
  .byte $91, $E2, $88, $B1, $D4, $91, $E2, $A0, $00, $B1, $E2, $85, $D4, $C8, $B1, $E2
  .byte $85, $D5, $A5, $D5, $D0, $05, $A5, $D4, $D0, $01, $60, $A0, $06, $B1, $D4, $D0
  .byte $F9, $A0, $02, $B1, $E2, $91, $D4, $C8, $B1, $E2, $91, $D4, $A0, $02, $B1, $E2
  .byte $AA, $C8, $B1, $E2, $85, $E3, $86, $E2, $A0, $00, $A5, $D4, $91, $E2, $C8, $A5
  .byte $D5, $91, $E2, $A5, $D4, $85, $E2, $A5, $D5, $85, $E3, $60

sub_b30_94CA:
  lda var_0067
  sta var_00E2
  lda var_0068
  sta var_00E3

sub_b30_94D2:
  lda var_00E2
  bne sub_b30_94DB
  lda var_00E3
  bne sub_b30_94DB
  rts

sub_b30_94DB:
  ldy #$04
  lda (var_00E2),y
  bne loc_b30_94E6
  iny
  lda (var_00E2),y
  beq sub_b30_94EF
loc_b30_94E6:
  ldy #$06
  lda #$00
  sta (var_00E2),y
  jsr sub_b31_D467

sub_b30_94EF:
  ldy #$02
  lda (var_00E2),y
  tax
  iny
  lda (var_00E2),y
  sta var_00E3
  stx var_00E2
  jmp loc_b31_D4D2
  .byte $20, $2F, $D6, $A5, $9C, $C5, $98, $90, $06, $D0, $05, $A5, $9B, $C5, $97, $60
  .byte $38, $60, $20, $FE, $D4, $B0, $FA, $20, $20, $D5, $20, $FE, $D4, $B0, $F2, $4C
  .byte $EA, $D0, $A5, $67, $85, $E2, $A5, $68, $85, $E3, $A0, $02, $B1, $E2, $85, $D4
  .byte $C8, $B1, $E2, $85, $D5, $A5, $D5, $D0, $1C, $A5, $D4, $D0, $18, $A5, $E2, $85
  .byte $69, $A5, $E3, $85, $6A, $A9, $00, $A0, $04, $91, $E2, $C8, $91, $E2, $C8, $91
  .byte $E2, $C8, $91, $E2, $60, $A0, $06, $B1, $E2, $F0, $16, $A5, $D4, $85, $E2, $A5
  .byte $D5, $85, $E3, $A0, $02, $B1, $E2, $85, $D4, $C8, $B1, $E2, $85, $D5, $4C, $33
  .byte $D5, $A9, $04, $A2, $E2, $A0, $E6, $20, $E7, $D5, $A9, $04, $A2, $D4, $A0, $93
  .byte $20, $E7, $D5, $A0, $02, $B1, $D4, $AA, $C8, $B1, $D4, $85, $D5, $86, $D4, $A5
  .byte $D4, $D0, $04, $A5, $D5, $F0, $A6, $A0, $00, $B1, $93, $91, $E6, $A2, $E6, $20
  .byte $E0, $D5, $A2, $93, $20, $E0, $D5, $A5, $93, $C5, $D4, $D0, $EC, $A5, $94, $C5
  .byte $D5, $D0, $E6, $A0, $00, $A5, $E2, $91, $E6, $C8, $A5, $E3, $91, $E6, $A0, $02
  .byte $A5, $E6, $91, $E2, $A9, $00, $91, $E6, $C8, $A5, $E7, $91, $E2, $A9, $00, $91
  .byte $E6, $A5, $E6, $85, $E2, $A5, $E7, $85, $E3, $A0, $06, $A9, $00, $91, $E6, $4C
  .byte $33, $D5, $F6, $00, $D0, $02, $F6, $01, $60, $18, $75, $00, $99, $00, $00, $A9
  .byte $00, $75, $01, $99, $01, $00, $60, $18, $75, $00, $95, $00, $90, $32, $F6, $01
  .byte $60, $A9, $07, $85, $97, $A9, $00, $85, $98, $A0, $FF, $C8, $B1, $6B, $D0, $FB
  .byte $20, $24, $D6, $A0, $00, $B1, $6D, $C9, $01, $D0, $04, $A0, $05, $D0, $07, $A0
  .byte $FF, $C8, $B1, $6D, $D0, $FB, $98, $38, $65, $97, $85, $97, $90, $02, $E6, $98
  .byte $60, $A9, $FF, $38, $E5, $69, $85, $9B, $A9, $6F, $E5, $6A, $85, $9C, $A5, $9B
  .byte $38, $E9, $08, $85, $9B, $B0, $02, $C6, $9C, $60, $A0, $00, $B1, $6B, $C9, $22
  .byte $F0, $03, $4C, $FF, $D0, $20, $BC, $D8, $20, $FF, $D5, $20, $D4, $D3, $A5, $6F
  .byte $C9, $80, $F0, $0B, $C9, $FF, $F0, $04, $A5, $B0, $F0, $03, $20, $54, $D4, $20
  .byte $10, $D5, $A5, $69, $85, $E6, $A5, $6A, $85, $E7, $A0, $03, $A5, $6F, $C9, $FF
  .byte $D0, $0E, $A0, $04, $A5, $DE, $91, $69, $C8, $A5, $DF, $91, $69, $4C, $9B, $D6
  .byte $A5, $B0, $F0, $EE, $A0, $04, $A9, $00, $91, $69, $C8, $91, $69, $A0, $05, $20
  .byte $03, $D7, $A0, $FF, $C8, $B1, $6B, $91, $69, $D0, $F9, $20, $03, $D7, $A0, $00
  .byte $B1, $6D, $C9, $01, $D0, $0E, $A0, $05, $B1, $6D, $91, $69, $88, $10, $F9, $A0
  .byte $05, $4C, $CB, $D6, $A0, $FF, $C8, $B1, $6D, $91, $69, $D0, $F9, $20, $03, $D7
  .byte $A0, $00, $A5, $E6, $91, $69, $C8, $A5, $E7, $91, $69, $A9, $00, $C8, $91, $69
  .byte $C8, $91, $69, $C8, $91, $69, $C8, $91, $69, $A0, $06, $91, $69, $A0, $02, $A5
  .byte $69, $91, $E6, $C8, $A5, $6A, $91, $E6, $60, $38, $B5, $00, $F9, $00, $00, $B5
  .byte $01, $F9, $01, $00, $60, $98, $38, $65, $69, $85, $69, $90, $02, $E6, $6A, $60
  .byte $20, $DD, $D0, $20, $2C, $D0, $20, $1D, $D7, $20, $BC, $D7, $4C, $7F, $C8, $C9
  .byte $0D, $D0, $0A, $A9, $20, $20, $93, $D7, $A5, $7A, $D0, $F7, $60, $48, $A5, $7A
  .byte $C5, $87, $D0, $05, $A9, $21, $20, $93, $D7, $68, $20, $93, $D7, $60, $A5, $D6
  .byte $D0, $22, $20, $2D, $A0, $20, $0F, $B6, $20, $12, $B6, $20, $15, $B6, $20, $21
  .byte $B6, $20, $30, $B6, $20, $36, $B6, $20, $18, $A0, $20, $0F, $A0, $20, $39, $B6
  .byte $A9, $FF, $85, $D6, $60

sub_b30_9763:
  stx var_0052
  sty var_0053
  jsr sub_b30_A000
  cmp #$B8
  bne sub_b30_9774
  jsr sub_b30_A011+1
  jmp loc_b31_D767

sub_b30_9774:
  cmp #$B9
  bne sub_b30_9781
  jsr sub_b31_D73C
  jsr sub_b30_A017+1
  jmp loc_b31_D767

sub_b30_9781:
  cmp #$BA
  bne sub_b30_978E
  jsr sub_b31_D73C
  jsr sub_b30_A015
  jmp loc_b31_D767

sub_b30_978E:
  ldy var_0053
  ldx var_0052
  rts

sub_b30_9793:
  pha
  lda var_00FA
  cmp #$AA
  beq loc_b30_979E
  pla
  jmp loc_b30_A003
loc_b30_979E:
  pla
  beq loc_b30_97B3
  cmp #$0A
  beq loc_b30_97B3
  sty var_00FB
  ldy #$00
  sta (var_00FC),y
  inc var_00FC
  bne loc_b30_97B1
  inc var_00FD
loc_b30_97B1:
  ldy var_00FB
loc_b30_97B3:
  rts
  .byte $86, $7A, $84, $7B, $20, $22, $FC, $60, $20, $9C, $D8, $A9, $FF, $85, $B8, $A9
  .byte $77, $85, $B9, $A5, $14, $85, $06, $48, $A5, $15, $85, $07, $48, $A9, $06, $A2
  .byte $06, $20, $F5, $D5, $20, $8A, $D8, $C9, $0D, $F0, $F9, $C9, $54, $D0, $D8, $20
  .byte $B1, $00, $C9, $4F, $D0, $D1, $20, $B1, $00, $C9, $20, $D0, $CA, $20, $8A, $D8
  .byte $C9, $0D, $F0, $C3, $20, $BC, $CB, $20, $B1, $00, $C9, $20, $F0, $0C, $C9, $0D
  .byte $D0, $F2, $A9, $00, $20, $BC, $CB, $4C, $2D, $D8, $A9, $00, $20, $BC, $CB, $20
  .byte $8A, $D8, $C9, $0D, $F0, $13, $20, $BC, $CB, $20, $B1, $00, $C9, $20, $F0, $EA
  .byte $C9, $0D, $D0, $F2, $A9, $00, $20, $BC, $CB, $A9, $80, $20, $BC, $CB, $20, $B1
  .byte $00, $C9, $20, $F0, $F9, $C9, $0D, $F0, $F5, $C9, $00, $F0, $06, $20, $AC, $CB
  .byte $4C, $32, $D8, $A9, $8B, $20, $BC, $CB, $A9, $80, $20, $BC, $CB, $68, $85, $E3
  .byte $68, $85, $E2, $A0, $02, $A5, $06, $91, $E2, $C8, $A5, $07, $91, $E2, $A0, $00
  .byte $A5, $E2, $91, $06, $C8, $A5, $E3, $91, $06, $A9, $00, $C8, $91, $06, $C8, $91
  .byte $06, $C8, $91, $06, $C8, $91, $06, $C8, $91, $06, $C8, $91, $06, $A5, $06, $85
  .byte $14, $A5, $07, $85, $15, $60, $20, $B1, $00, $C9, $20, $F0, $F9, $60, $A9, $00
  .byte $A0, $06, $91, $00, $88, $10, $F9, $60, $A5, $69, $48, $A5, $6A, $48, $A5, $00
  .byte $85, $E2, $A5, $01, $85, $E3, $20, $28, $D5, $A5, $E2, $85, $14, $A5, $E3, $85
  .byte $15, $68, $85, $6A, $68, $85, $69, $60

sub_b30_98BC:
  inc var_006B
  bne sub_b30_98C2
  inc var_006C

sub_b30_98C2:
  rts
  .byte $E6, $6D, $D0, $02, $E6, $6E, $60, $A9, $DB, $85, $E3, $A9, $00, $85, $E2, $8A
  .byte $29, $7F, $AA, $F0, $14, $A0, $FF, $C8, $B1, $E2, $D0, $FB, $98, $38, $65, $E2
  .byte $85, $E2, $90, $02, $E6, $E3, $CA, $D0, $EC, $60, $60, $20, $B1, $00, $C9, $BB
  .byte $F0, $F8, $A9, $20, $20, $93, $D7, $20, $B7, $00, $C9, $80, $F0, $EC, $90, $40
  .byte $C9, $C7, $D0, $15, $20, $B1, $00, $C9, $00, $F0, $06, $20, $93, $D7, $4C, $07
  .byte $D9, $A9, $3A, $20, $93, $D7, $4C, $EE, $D8, $C9, $FF, $D0, $10, $20, $B1, $00
  .byte $20, $93, $D7, $20, $B1, $00, $C9, $00, $D0, $F6, $4C, $EE, $D8, $AA, $20, $CA
  .byte $D8, $A0, $FF, $C8, $B1, $E2, $20, $93, $D7, $C9, $00, $D0, $F6, $4C, $EE, $D8
  .byte $A5, $B8, $85, $6B, $A5, $B9, $85, $6C, $A0, $00, $B1, $6B, $C9, $22, $D0, $05
  .byte $A9, $22, $20, $93, $D7, $C9, $3A, $D0, $08, $A9, $3A, $20, $93, $D7, $4C, $26
  .byte $D9, $20, $A4, $CA, $A5, $6B, $85, $B8, $A5, $6C, $85, $B9, $4C, $EE, $D8

sub_b30_9972:
  jsr sub_b31_D994
  cmp #$45
  bne sub_b30_9993
  jsr sub_b31_CBBC
  jsr a:$00B1
  bcc loc_b30_998B
  cmp #$2D
  bne sub_b30_9993
  jsr sub_b31_CBBC
  jsr a:$00B1
loc_b30_998B:
  jsr sub_b31_CBBC
  jsr a:$00B1
  bcc loc_b30_998B

sub_b30_9993:
  rts

sub_b30_9994:
  jsr a:$00B1
  bcs sub_b30_999F
  jsr sub_b31_CBBC
  jmp sub_b31_D994

sub_b30_999F:
  cmp #$2E
  bne sub_b30_99B1
  jsr sub_b31_CBBC
  jsr a:$00B1
  bcs sub_b30_99B1
  jsr sub_b31_CBBC
  jmp loc_b31_D9A6

sub_b30_99B1:
  rts
  .byte $A9, $00, $8D, $16, $40, $A0, $08, $A9, $07, $A2, $02, $8D, $16, $40, $8E, $16
  .byte $40, $88, $D0, $F7, $AD, $17, $40, $AD, $17, $40, $49, $FF, $29, $10, $60, $4D
  .byte $32, $0D, $07, $D0, $01, $50, $41, $52, $48, $0D, $09, $CD, $02, $23, $55, $55
  .byte $4E, $50, $48, $0D, $06, $D0, $01, $4E, $50, $48, $0D, $08, $CD, $02, $23, $55
  .byte $55, $4E, $50, $0D, $05, $D0, $01, $4E, $50, $0D, $09, $CD, $02, $23, $E6, $E3
  .byte $CA, $D0, $EC, $60, $60, $20, $B1, $00, $C9, $BB, $F0, $F8, $A9, $20, $20, $45
  .byte $D7, $20, $B7, $00, $C9, $80, $F0, $EC, $90, $40, $C9, $C7, $D0, $15, $20, $B1
  .byte $00, $C9, $00, $F0, $06, $20, $45, $D7, $4C, $20, $DA, $A9, $3A, $20, $45, $D7
  .byte $4C, $07, $DA, $C9, $FF, $D0, $10, $20, $B1, $00, $20, $45, $D7, $20, $B1, $00
  .byte $C9, $00, $D0, $F6, $4C, $07, $DA, $AA, $20, $E3, $D9, $A0, $FF, $C8, $B1, $E2
  .byte $20, $45, $D7, $C9, $00, $D0, $F6, $4C, $07, $DA, $A5, $B8, $85, $6B, $A5, $B9
  .byte $85, $6C, $A0, $00, $B1, $6B, $C9, $22, $D0, $05, $A9, $22, $20, $45, $D7, $C9
  .byte $3A, $D0, $08, $A9, $3A, $20, $45, $D7, $4C, $3F, $DA, $20, $A4, $CA, $A5, $6B
  .byte $85, $B8, $A5, $6C, $85, $B9, $4C, $07, $DA, $20, $AD, $DA, $C9, $45, $D0, $1A
  .byte $20, $BC, $CB, $20, $B1, $00, $90, $0A, $C9, $2D, $D0, $0E, $20, $BC, $CB, $20
  .byte $B1, $00, $20, $BC, $CB, $20, $B1, $00, $90, $F8, $60, $20, $B1, $00, $B0, $06
  .byte $20, $BC, $CB, $4C, $AD, $DA, $C9, $2E, $D0, $0E, $20, $BC, $CB, $20, $B1, $00
  .byte $B0, $06, $20, $BC, $CB, $4C, $BF, $DA, $60

sub_b30_9ACB:
  jsr sub_b30_AC86
  cmp #$03
  beq loc_b30_9AD4
  clc
  rts
loc_b30_9AD4:
  sec
  rts
  .byte $A9, $07, $A2, $02, $8D, $16, $40, $8E, $16, $40, $88, $D0, $F7, $AD, $17, $40
  .byte $AD, $17, $40, $49, $FF, $29, $10, $60, $A9, $05, $8D, $16, $40, $A9, $06, $8D
  .byte $16, $40, $AD, $17, $40, $29, $10, $60, $09, $5E, $FF, $00, $2D, $00
  .byte $28, $00, $29, $00, $2B, $00, $2D, $00
  .byte $2A, $00, $2F, $00, $3C, $00, $3D, $00
  .byte $3E, $00, $45, $4E, $44, $00, $53, $51
  .byte $52, $54, $00, $53, $49, $4E, $00, $50
  .byte $52, $00, $4F, $50, $00, $4D, $41, $4B
  .byte $45, $00, $FF, $00, $54, $4F, $00, $47
  .byte $4F, $4F, $44, $42, $59, $45, $00, $46
  .byte $49, $52, $53, $54, $00, $4C, $41, $53
  .byte $54, $00, $42, $46, $00, $42, $4C, $00
  .byte $57, $4F, $52, $44, $00, $53, $45, $00
  .byte $4C, $49, $53, $54, $00, $46, $50, $55
  .byte $54, $00, $4C, $50, $55, $54, $00, $52
  .byte $45, $50, $45, $41, $54, $00, $FF, $00
  .byte $FF, $00, $41, $4E, $59, $4F, $46, $00
  .byte $41, $4C, $4C, $4F, $46, $00, $4E, $4F
  .byte $54, $00, $54, $45, $53, $54, $00, $49
  .byte $46, $54, $00, $49, $46, $46, $00, $49
  .byte $46, $00, $54, $48, $45, $4E, $00, $45
  .byte $4C, $53, $45, $00, $54, $4F, $50, $4C
  .byte $45, $56, $45, $4C, $00, $52, $4F, $55
  .byte $4E, $44, $00, $49, $4E, $54, $45, $47
  .byte $45, $52, $00, $51, $55, $4F, $54, $49
  .byte $45, $4E, $54, $00, $41, $53, $43, $49
  .byte $49, $00, $43, $48, $41, $52, $00, $43
  .byte $4F, $53, $00, $41, $54, $41, $4E, $00
  .byte $52, $45, $4D, $41, $49, $4E, $44, $45
  .byte $52, $00, $4E, $55, $4D, $42, $45, $52
  .byte $3F, $00, $57, $4F, $52, $44, $3F, $00
  .byte $4C, $49, $53, $54, $3F, $00, $50, $52
  .byte $49, $4E, $54, $31, $00, $52, $43, $00
  .byte $52, $51, $00, $53, $54, $4F, $50, $00
  .byte $54, $52, $41, $43, $45, $00, $4E, $4F
  .byte $54, $52, $41, $43, $45, $00, $20, $00
  .byte $50, $4F, $54, $53, $00, $50, $4F, $4E
  .byte $41, $4D, $45, $53, $00, $50, $4F, $00
  .byte $50, $4F, $50, $52, $4F, $00, $50, $4F
  .byte $41, $4C, $4C, $00, $54, $48, $49, $4E
  .byte $47, $3F, $00, $54, $48, $49, $4E, $47
  .byte $00, $45, $52, $4E, $41, $4D, $45, $00
  .byte $45, $52, $50, $52, $4F, $00, $45, $52
  .byte $4E, $41, $4D, $45, $53, $00, $45, $52
  .byte $41, $4C, $4C, $00, $20, $00, $47, $4F
  .byte $00, $53, $41, $56, $45, $00, $52, $45
  .byte $41, $44, $00, $4F, $55, $54, $50, $52
  .byte $4E, $00, $46, $44, $00, $42, $4B, $00
  .byte $53, $45, $54, $58, $59, $00, $52, $54
  .byte $00, $4C, $54, $00, $53, $50, $4C, $49
  .byte $54, $53, $43, $52, $45, $45, $4E, $00
  .byte $46, $55, $4C, $4C, $53, $43, $52, $45
  .byte $45, $4E, $00, $54, $45, $58, $54, $53
  .byte $43, $52, $45, $45, $4E, $00, $44, $52
  .byte $41, $57, $00, $48, $4F, $4D, $45, $00
  .byte $50, $44, $00, $50, $55, $00, $53, $54
  .byte $00, $48, $54, $00, $42, $47, $00, $50
  .byte $43, $00, $54, $53, $00, $57, $52, $41
  .byte $50, $00, $4E, $4F, $57, $52, $41, $50
  .byte $00, $2E, $41, $53, $50, $45, $43, $54
  .byte $00, $43, $53, $00, $43, $4C, $45, $41
  .byte $52, $54, $45, $58, $54, $00, $4E, $44
  .byte $00, $53, $45, $54, $58, $00, $53, $45
  .byte $54, $59, $00, $53, $45, $54, $48, $00
  .byte $58, $43, $4F, $52, $00, $59, $43, $4F
  .byte $52, $00, $48, $45, $41, $44, $49, $4E
  .byte $47, $00, $42, $52, $49, $47, $48, $54
  .byte $00, $45, $52, $00, $50, $49, $43, $4B
  .byte $50, $45, $4E, $00, $52, $41, $4E, $44
  .byte $4F, $4D, $00, $43, $55, $52, $53, $4F
  .byte $52, $00, $00
  .byte $00, $00, $00, $AE, $E3, $00, $00, $00, $00, $BB, $E3, $C4, $E3, $CD, $E3, $D6
  .byte $E3, $5F, $E8, $8B, $E8, $75, $E8, $1D, $EF, $FA, $E3, $2C, $E4
  .byte $12, $EA
  .byte $39, $EA, $2D, $EA, $00, $00, $51, $EF, $00, $94, $8E, $E4, $E2, $E4, $78, $E5
  .byte $D6, $E5, $62, $E6, $BF, $E6, $1C, $E7, $5E, $E7, $AF, $E7, $C6, $EC, $38, $ED
  .byte $64, $ED, $30, $E9, $46, $E9, $5C, $E9, $6D, $E9, $89, $E9, $AE, $E9, $B9, $E9
  .byte $E4, $E9, $EA, $E9, $09, $EA, $DF, $E3, $EE, $E3, $06, $E4, $5C, $E4, $6D, $E4
  .byte $3B, $E4, $4A, $E4, $12, $E4, $FA, $E7, $37, $E8, $4B, $E8, $21, $EA, $CF, $EA
  .byte $E8, $EA, $1D, $EF, $F5, $EE, $0D, $EF
  .byte $09, $EA
  .byte $0E, $EB, $49, $EB, $A6, $EB, $7F, $EC, $88, $EC, $5C, $EA, $7B, $EA, $91, $EC
  .byte $A0, $EC, $A9, $EC, $B2, $EC, $19, $EE, $79, $EE, $00, $FC, $0C, $FC, $1B, $FC
  .byte $24, $FC, $33, $FC, $42, $FC, $51, $FC, $60, $FC, $6F, $FC, $7B, $FC, $87, $FC
  .byte $90, $FC, $9D, $FC, $B5, $FC, $C1, $FC, $CD, $FC, $E0, $FC, $0A, $FF, $EA, $FE
  .byte $F3, $FC, $50, $FD, $5C, $FD, $68, $FD, $77, $FD, $83, $FD, $8C, $FD, $9C, $FD
  .byte $AB, $FD, $BA, $FD, $C9, $FD, $D5, $FD, $E1, $FD, $CC, $FE, $34, $FE, $22, $FF
  .byte $3B, $FF, $A7, $FF, $00, $00, $00, $00, $02, $02, $02, $02, $02, $02, $02, $00
  .byte $01, $01
  .byte $01
  .byte $01, $02, $01, $00, $00, $01, $01, $01, $01, $02, $02, $02, $02, $02, $02, $00
  .byte $00, $02, $02, $01, $01, $00, $00, $01, $00, $00, $00, $01, $01, $02, $01, $01
  .byte $01, $02, $02, $01, $01, $01, $01, $00, $00, $00, $00, $00
  .byte $00
  .byte $00, $00, $00, $00, $00, $01, $01, $01, $00, $00, $00, $00, $01, $01, $01, $00
  .byte $01, $01, $02, $01, $01, $00, $00, $00, $00, $00, $00, $00, $00, $00, $01, $02
  .byte $00, $00, $00, $01, $00, $00, $00, $01, $01, $01, $00, $00, $00, $02, $00, $01
  .byte $01, $02, $4C, $61, $E3, $20, $B4, $E1, $A0, $00, $B1, $6B, $C9, $5B, $F0, $F2
  .byte $C9, $01, $D0, $0E, $E6, $6B, $D0, $02, $E6, $6C, $A5, $6B, $A4, $6C, $20, $30
  .byte $F0, $60, $A5, $B9, $48, $A5, $B8, $48, $A5, $6B, $85, $B8, $A5, $6C, $85, $B9
  .byte $20, $B1, $00, $20, $60, $F0, $20, $B7, $00, $C9, $00, $D0, $C5, $68, $85, $B8
  .byte $68, $85, $B9, $60, $20, $DC, $E1, $20, $70, $DE, $A2, $93, $20, $39, $F0, $A5
  .byte $6E, $85, $6C, $A5, $6D, $85, $6B, $20, $70, $DE, $A0, $00, $A9, $93, $20, $45
  .byte $F0, $60, $A2, $05, $B5, $A5, $9D, $00, $01, $CA, $10, $F8, $20, $63, $F0, $20
  .byte $1E, $F0, $A0, $01, $A2, $06, $20, $3C, $F0, $A2, $05, $BD, $00, $01, $95, $9D
  .byte $CA, $10, $F8, $20, $63, $F0, $20, $1E, $F0, $A0, $01, $A2, $00, $20, $3C, $F0
  .byte $A0, $01, $A9, $06, $20, $30, $F0, $A0, $01, $A9, $00, $20, $06, $F0, $20, $1E
  .byte $F0, $60, $20, $B4, $E1, $A0, $00, $B1, $6B, $E6, $6B, $D0, $02, $E6, $6C, $C9
  .byte $22, $F0, $18, $C9, $5B, $F0, $14, $A4, $6C, $A5, $6B, $20, $30, $F0, $20, $4B
  .byte $F0, $A9, $00, $85, $6B, $A9, $01, $85, $6C, $A9, $22, $60, $20, $0A, $DF, $C9
  .byte $22, $F0, $F8, $4C, $61, $E3

sub_b30_9F3E:
  jsr sub_b31_E22A
  ldy #$00
  lda (var_0004),y
  beq sub_b30_9F4A
  jmp loc_b31_E368

sub_b30_9F4A:
  rts
  .byte $20, $2A, $E2, $20, $13, $E2, $A9, $01, $20, $3A, $E2, $A4, $07, $A6, $06, $20
  .byte $3C, $F0, $A9, $05, $18, $65, $06, $85, $06, $90, $02, $E6, $07, $60, $A2, $FF
  .byte $E8, $BD, $83, $FF, $20, $3A, $E2, $C9, $00, $D0, $F5, $20, $2A, $E2, $4C, $18
  .byte $C8, $A2, $FF, $E8, $BD, $89, $FF, $20, $3A, $E2, $C9, $00, $D0, $F5, $20, $2A
  .byte $E2, $4C, $18, $C8, $4C, $6F, $E3, $A0, $00, $B1, $6B, $F0, $F7, $C9, $27, $D0
  .byte $06, $A0, $02, $B1, $6B, $F0, $ED, $60, $A0, $00, $B1, $6B, $F0, $E6, $60, $A0
  .byte $00, $B1, $6B, $C9, $5B, $F0, $06, $20, $BE, $DF, $A9, $22, $60, $20, $DE, $DF
  .byte $A9, $5B, $60, $A0, $00, $B1, $6B, $C9, $20, $F0, $17, $C9, $00, $F0, $13, $C9
  .byte $27, $F0, $09, $20, $3A, $E2, $20, $9F, $E1, $4C, $BE, $DF, $20, $03, $E0, $4C
  .byte $BE, $DF, $60, $A9, $5B, $A2, $00
loc_b30_9FE2:
  inx
loc_b30_9FE3:
  jsr sub_b31_E23A
  jsr sub_b31_E19F
  ldy #$00
  lda (var_006B),y
  cmp #$27
  bne loc_b30_9FF7
  jsr sub_b31_E003
  jmp loc_b31_DFE9
loc_b30_9FF7:
  cmp #$5B
  beq loc_b30_9FE2
  cmp #$5D
  bne loc_b30_9FE3
  dex

sub_b30_A000:
  bne loc_b30_9FE3
  rts
loc_b30_A003:
  ldy #$00
  lda (var_006B),y
  cmp #$27

sub_b30_A009:
  bne loc_b30_A022
loc_b30_A00B:
  jsr sub_b31_E23A
  jsr sub_b31_E19F

sub_b30_A011:
  ldy #$00
  lda (var_006B),y

sub_b30_A015:
  cmp #$27

sub_b30_A017:
  bne loc_b30_A00B
  jsr sub_b31_E23A
  jsr sub_b31_E19F
  jmp sub_b31_E003
loc_b30_A022:
  rts
  .byte $A0, $00, $B1, $6B

sub_b30_A027:
  cmp #$5B
  beq loc_b30_A031
  jsr sub_b31_E037
  lda #$22
  rts
loc_b30_A031:
  jsr sub_b31_E054
  lda #$5B
  rts
  .byte $A0, $00, $B1, $6B, $C9, $20, $F0, $14, $C9, $00, $F0, $10, $C9, $27, $F0, $06
  .byte $20, $9F, $E1, $4C, $37, $E0, $20, $78, $E0, $4C, $37, $E0, $60, $A2, $00, $E8
  .byte $EA, $20, $9F, $E1, $A0, $00, $B1, $6B, $C9, $27, $D0, $06, $20, $78, $E0, $4C
  .byte $5B, $E0, $C9, $5B, $F0, $E9, $C9, $5D, $D0, $E6, $CA, $D0, $E3, $20, $9F, $E1
  .byte $60, $A0, $00, $B1, $6B, $C9, $27, $D0, $13, $EA, $20, $9F, $E1, $A0, $00, $B1
  .byte $6B, $C9, $27, $D0, $F4, $EA, $20, $9F, $E1, $4C, $78, $E0, $60, $4C, $6F, $E3
  .byte $A0, $00, $B1, $6B, $C9, $20, $D0, $09, $E6, $6B, $D0, $02, $E6, $6C, $4C, $99
  .byte $E0, $60, $A5, $6B, $C5, $E2, $D0, $07, $A5, $6C, $C5, $E3, $D0, $01, $60, $A0
  .byte $00, $B1, $6B, $20, $3A, $E2, $20, $9F, $E1, $4C, $A9, $E0, $A2, $00, $E8, $E6
  .byte $6B, $D0, $02, $E6, $6C, $A0, $00, $B1, $6B, $C9, $5B, $F0, $F1, $C9, $5D, $D0
  .byte $EE, $CA, $D0, $EB, $E6, $6B, $D0, $02, $E6, $6C, $60, $A0, $00, $20, $9F, $E1
  .byte $B1, $6B, $C9, $20, $F0, $04, $C9, $00, $D0, $F3, $60, $4C, $61, $E3, $A0, $00
  .byte $B1, $6B, $C9, $22, $F0, $1A, $C9, $01, $D0, $F1, $20, $9F, $E1, $A4, $6C, $A5
  .byte $6B, $20, $30, $F0, $20, $4B, $F0, $A9, $01, $85, $6C, $A9, $00, $85, $6B, $60
  .byte $20, $9F, $E1, $60, $A0, $00, $91, $E6, $E6, $E6, $D0, $02, $E6, $E7, $60, $A0
  .byte $00, $B1, $E6, $20, $3A, $E2, $20, $AD, $E1, $B1, $E6, $C9, $20, $F0, $F7, $D0
  .byte $02, $B1, $E6, $20, $3A, $E2, $20, $AD, $E1, $C9, $00, $D0, $F4, $60, $20, $0D
  .byte $DF, $C9, $5B, $F0, $0F, $A0, $00, $B1, $6B, $F0, $22, $20, $1B, $E1, $20, $9F
  .byte $E1, $4C, $4E, $E1, $A9, $5B, $20, $1B, $E1, $A0, $00, $B1, $6B, $F0, $09, $20
  .byte $1B, $E1, $20, $9F, $E1, $4C, $62, $E1, $A9, $5D, $20, $1B, $E1, $60, $C6, $07
  .byte $A0, $FF, $B1, $06, $E6, $07, $C9, $20, $D0, $08, $A5, $06, $D0, $02, $C6, $07
  .byte $C6, $06, $60, $C6, $E7, $A0, $FF, $B1, $E6, $E6, $E7, $C9, $20, $D0, $08, $A5
  .byte $E6, $D0, $02, $C6, $E7, $C6, $E6, $60

sub_b30_A19F:
  inc var_006B
  bne sub_b30_A1A5
  inc var_006C

sub_b30_A1A5:
  rts
  .byte $E6, $6D, $D0, $02, $E6, $6E, $60, $E6, $E6, $D0, $02, $E6, $E7, $60

sub_b30_A1B4:
  ldy #$00
  lda (var_0002),y
  sta var_006B
  iny
  lda (var_0002),y
  sta var_006C
  sec
  lda var_0002
  sbc #$02
  sta var_0002
  bcs sub_b30_A1CA
  dec var_0003

sub_b30_A1CA:
  ldx #$6B
  ldy #$18
  jsr sub_b31_E24E
  bcc sub_b30_A1DB
  lda var_006B
  sta var_0006
  lda var_006C
  sta var_0007

sub_b30_A1DB:
  rts
  .byte $A5, $02, $38, $E9, $04, $85, $02, $B0, $02, $C6, $03, $A0, $02, $B1, $02, $85
  .byte $6B, $C8, $B1, $02, $85, $6C, $C8, $B1, $02, $85, $6D, $C8, $B1, $02, $85, $6E
  .byte $20, $CA, $E1, $B0, $11, $A2, $6D, $A0, $18, $20, $4E, $E2, $90, $08, $A5, $6D
  .byte $85, $06, $A5, $6E, $85, $07, $60, $18, $A5, $02, $69, $02, $85, $02, $90, $02
  .byte $E6, $03, $A0, $00, $A5, $06, $91, $02, $C8, $A5, $07, $91, $02, $60

sub_b30_A22A:
  lda var_0004
  sec
  sbc #$01
  sta var_0004
  bcs sub_b30_A235
  dec var_0005

sub_b30_A235:
  ldy #$01
  lda (var_0004),y
  rts
  .byte $A0, $00, $91, $06, $E6, $06, $D0, $02, $E6, $07, $60, $A5, $DE, $D0, $02, $C6
  .byte $DF, $C6, $DE, $60

sub_b30_A24E:
  sec
  lda var_0000,x
  sbc a:var_0000,y
  lda var_0001,x
  sbc a:var_0001,y
  rts
  .byte $20, $9F, $E1, $A0, $00, $B1, $6B, $C9, $27, $F0, $06, $20, $03, $C8, $4C, $5A
  .byte $E2, $20, $9F, $E1, $A0, $00, $B1, $6B, $C9, $27, $D0, $06, $20, $03, $C8, $4C
  .byte $5A, $E2, $60

sub_b30_A27D:
  ldy #$00
  lda (var_006B),y
  cmp #$01
  bne sub_b30_A288
  jmp loc_b31_C824

sub_b30_A288:
  cmp #$22
  bne loc_b30_A2A5

sub_b30_A28C:
  jsr sub_b31_E19F
  ldy #$00
  lda (var_006B),y
  beq sub_b30_A2AB
  cmp #$27
  beq loc_b30_A29F
  jsr sub_b31_C803
  jmp loc_b31_E28C
loc_b30_A29F:
  jsr sub_b31_E25A
  jmp loc_b31_E28F
loc_b30_A2A5:
  jmp loc_b31_E28C
  .byte $4C, $8C, $E2

sub_b30_A2AB:
  rts
  .byte $46, $55, $4E, $43, $54, $49, $4F, $4E, $20, $4E, $4F, $54, $20, $4C, $49, $4B
  .byte $45, $20, $49, $54, $20, $41, $53, $20, $49, $4E, $50, $55, $54, $20, $00, $46
  .byte $55, $4E, $43, $54, $49, $4F, $4E, $20, $44, $49, $44, $4E, $27, $54, $20, $4F
  .byte $55, $54, $50, $55, $54, $00, $57, $4F, $52, $44, $20, $4F, $52, $20, $4C, $49
  .byte $53, $54, $20, $45, $4D, $50, $54, $59, $00, $44, $4F, $4E, $27, $54, $20, $53
  .byte $41, $59, $20, $57, $41, $54, $48, $20, $54, $4F, $20, $44, $4F, $20, $57, $49
  .byte $54, $48, $20, $4F, $55, $54, $50, $55, $54, $00, $56, $41, $4C, $55, $52, $45
  .byte $20, $4E, $4F, $54, $20, $46, $4F, $55, $4E, $44, $00, $50, $52, $4F, $43, $45
  .byte $44, $55, $52, $45, $20, $4E, $4F, $54, $20, $46, $4F, $55, $4E, $44, $00, $46
  .byte $55, $4E, $43, $54, $49, $4F, $4E, $20, $55, $53, $45, $44, $20, $4F, $55, $54
  .byte $20, $4F, $46, $20, $50, $52, $4F, $43, $45, $44, $55, $52, $45, $00, $20, $45
  .byte $52, $52, $4F, $52, $00, $A0, $E2, $A2, $AC, $4C, $9E, $E3, $A0, $E2, $A2, $CB
  .byte $4C, $9E, $E3, $A0, $E2, $A2, $E2, $4C, $9E, $E3, $A0, $E2, $A2, $F5, $4C, $9E
  .byte $E3, $A0, $E3, $A2, $16, $4C, $9E, $E3, $A0, $E3, $A2, $27, $4C, $9E, $E3, $A0
  .byte $E3, $A2, $3B, $4C, $9E, $E3, $A0, $E3, $A2, $5A, $84, $6C, $86, $6B, $20, $27
  .byte $C8, $60, $84, $6C, $86, $6B, $20, $27, $C8, $20, $92, $E3, $20, $54, $C8, $4C
  .byte $09, $C8, $20, $6D, $DE, $A9, $00, $85, $A5, $20, $15, $F0, $4C, $4B, $DF, $20
  .byte $AC, $DE, $20, $0F, $F0, $4C, $4B, $DF, $20, $AC, $DE, $20, $15, $F0, $4C, $4B
  .byte $DF, $20, $AC, $DE, $20, $03, $F0, $4C, $4B, $DF, $20, $AC, $DE, $20, $09, $F0
  .byte $4C, $4B, $DF, $20, $6D, $DE, $20, $63, $F0, $20, $1E, $F0, $20, $4B, $DF, $4C
  .byte $18, $C8, $20, $6D, $DE, $20, $1E, $F0, $20, $4B, $DF, $4C, $18, $C8, $20, $6D
  .byte $DE, $20, $21, $F0, $20, $4B, $DF, $4C, $18, $C8, $20, $AC, $DE, $20, $CA, $DE
  .byte $20, $4B, $DF, $4C, $18, $C8, $20, $AC, $DE, $20, $CA, $DE, $A0, $01, $A9, $06
  .byte $20, $00, $F0, $A0, $01, $A9, $00, $20, $12, $F0, $20, $4B, $DF, $4C, $18, $C8
  .byte $20, $6D, $DE, $20, $1E, $FE, $20, $27, $F0, $20, $4B, $DF, $4C, $18, $C8, $20
  .byte $6D, $DE, $20, $1E, $FE, $20, $24, $F0, $20, $4B, $DF, $4C, $18, $C8, $20, $AC
  .byte $DE, $20, $09, $F0, $20, $2D, $F0, $20, $26, $FE, $20, $4B, $DF, $4C, $18, $C8
  .byte $20, $34, $DF, $A0, $00, $B1, $6B, $A8, $20, $51, $F0, $20, $4B, $DF, $4C, $18
  .byte $C8, $20, $6D, $DE, $20, $2A, $E2, $20, $5D, $F0, $20, $13, $E2, $A9, $22, $20
  .byte $3A, $E2, $A5, $A1, $20, $3A, $E2, $A9, $00, $20, $3A, $E2, $4C, $18, $C8, $4C
  .byte $6F, $E3, $20, $2A, $E2, $20, $0A, $DF, $C9, $22, $D0, $29, $20, $92, $DF, $20
  .byte $13, $E2, $A9, $22, $20, $3A, $E2, $A0, $00, $B1, $6B, $C9, $27, $D0, $0B, $A0
  .byte $01, $B1, $6B, $C9, $27, $D0, $03, $20, $3A, $E2, $20, $3A, $E2, $A9, $00, $20
  .byte $3A, $E2, $4C, $18, $C8, $20, $13, $E2, $20, $A3, $DF, $A0, $00, $B1, $6B, $C9
  .byte $5B, $F0, $05, $A9, $22, $20, $3A, $E2, $20, $AA, $DF, $A9, $00, $20, $3A, $E2
  .byte $4C, $18, $C8, $4C, $6F, $E3, $20, $2A, $E2, $20, $0A, $DF, $C9, $22, $D0, $47
  .byte $20, $92, $DF, $20, $13, $E2, $A9, $22, $20, $3A, $E2, $A0, $01, $B1, $6B, $F0
  .byte $06, $20, $9F, $E1, $4C, $F9, $E4, $A0, $00, $B1, $6B, $C9, $27, $D0, $1D, $C6
  .byte $6C, $A0, $FF, $B1, $6B, $C9, $27, $D0, $13, $A0, $FE, $B1, $6B, $C9, $27, $D0
  .byte $06, $20, $3A, $E2, $20, $3A, $E2, $A9, $27, $20, $3A, $E2, $20, $3A, $E2, $A9
  .byte $00, $20, $3A, $E2, $4C, $18, $C8, $20, $13, $E2, $20, $A3, $DF, $A6, $6B, $86
  .byte $6D, $A6, $6C, $86, $6E, $20, $23, $E0, $AA, $20, $97, $E0, $A0, $00, $B1, $6B
  .byte $D0, $EB, $A5, $6D, $85, $6B, $A5, $6E, $85, $6C, $E0, $22, $D0, $10, $A9, $22
  .byte $20, $3A, $E2, $20, $AA, $DF, $A9, $00, $20, $3A, $E2, $4C, $18, $C8, $20, $AA
  .byte $DF, $A9, $00, $20, $3A, $E2, $4C, $18, $C8, $4C, $6F, $E3, $20, $2A, $E2, $20
  .byte $0A, $DF, $C9, $22, $D0, $2F, $20, $92, $DF, $20, $13, $E2, $A9, $22, $20, $3A
  .byte $E2, $A0, $00, $B1, $6B, $C9, $27, $F0, $0E, $20, $9F, $E1, $20, $BE, $DF, $A9
  .byte $00, $20, $3A, $E2, $4C, $18, $C8, $20, $78, $E0, $20, $BE, $DF, $A9, $00, $20
  .byte $3A, $E2, $4C, $18, $C8, $20, $13, $E2, $20, $A3, $DF, $A9, $5B, $20, $3A, $E2
  .byte $20, $23, $E0, $20, $97, $E0, $A0, $00, $B1, $6B, $20, $3A, $E2, $20, $9F, $E1
  .byte $C9, $00, $D0, $F2, $4C, $18, $C8, $4C, $6F, $E3, $20, $2A, $E2, $20, $0A, $DF
  .byte $C9, $22, $D0, $43, $20, $92, $DF, $20, $13, $E2, $A5, $6B, $48, $A5, $6C, $48
  .byte $A5, $6B, $85, $E2, $A5, $6C, $85, $E3, $A0, $00, $B1, $6B, $C9, $27, $F0, $0B
  .byte $20, $9F, $E1, $A0, $00, $B1, $6B, $D0, $E7, $F0, $06, $20, $78, $E0, $4C, $FF
  .byte $E5, $A9, $22, $20, $3A, $E2, $68, $85, $6C, $68, $85, $6B, $20, $A9, $E0, $A9
  .byte $00, $20, $3A, $E2, $4C, $18, $C8, $20, $A3, $DF, $20, $13, $E2, $A5, $6B, $48
  .byte $A5, $6C, $48, $A5, $6B, $85, $E2, $A5, $6C, $85, $E3, $20, $97, $E0, $20, $23
  .byte $E0, $A0, $00, $B1, $6B, $D0, $EC, $68, $85, $6C, $68, $85, $6B, $A9, $5B, $20
  .byte $3A, $E2, $20, $A9, $E0, $C9, $5B, $D0, $05, $A9, $5D, $20, $3A, $E2, $A9, $00
  .byte $20, $3A, $E2, $4C, $18, $C8, $20, $2A, $E2, $A5, $06, $85, $E6, $48, $A5, $07
  .byte $85, $E7, $48, $20, $DC, $E1, $20, $13, $E2, $20, $F5, $E0, $A9, $22, $20, $1B
  .byte $E1, $A0, $00, $B1, $6B, $F0, $09, $20, $1B, $E1, $20, $9F, $E1, $4C, $7D, $E6
  .byte $A5, $6D, $85, $6B, $A5, $6E, $85, $6C, $20, $F5, $E0, $A0, $00, $B1, $6B, $20
  .byte $1B, $E1, $20, $9F, $E1, $C9, $00, $D0, $F4, $68, $85, $E7, $68, $85, $E6, $A0
  .byte $00, $B1, $E6, $E6, $E6, $D0, $02, $E6, $E7, $20, $3A, $E2, $C9, $00, $D0, $F1
  .byte $4C, $18, $C8, $20, $2A, $E2, $A5, $06, $85, $E6, $48, $A5, $07, $85, $E7, $48
  .byte $20, $DC, $E1, $20, $13, $E2, $A9, $5B, $20, $1B, $E1, $20, $0D, $DF, $A0, $00
  .byte $B1, $6B, $F0, $09, $20, $1B, $E1, $20, $9F, $E1, $4C, $DC, $E6, $A9, $20, $20
  .byte $1B, $E1, $A5, $6D, $85, $6B, $A5, $6E, $85, $6C, $20, $0D, $DF, $A0, $00, $B1
  .byte $6B, $F0, $09, $20, $1B, $E1, $20, $9F, $E1, $4C, $FB, $E6, $20, $8A, $E1, $A9
  .byte $00, $20, $1B, $E1, $68, $85, $E7, $68, $85, $E6, $20, $26, $E1, $4C, $18, $C8
  .byte $20, $2A, $E2, $A5, $06, $85, $E6, $48, $A5, $07, $85, $E7, $48, $20, $DC, $E1
  .byte $20, $13, $E2, $A9, $5B, $20, $1B, $E1, $20, $45, $E1, $A9, $20, $20, $1B, $E1
  .byte $A5, $6D, $85, $6B, $A5, $6E, $85, $6C, $20, $45, $E1, $20, $8A, $E1, $A9, $00
  .byte $20, $1B, $E1, $68, $85, $E7, $68, $85, $E6, $20, $26, $E1, $4C, $18, $C8, $4C
  .byte $61, $E3, $20, $2A, $E2, $A5, $06, $85, $E6, $48, $A5, $07, $85, $E7, $48, $20
  .byte $DC, $E1, $20, $13, $E2, $A9, $5B, $20, $1B, $E1, $A0, $00, $B1, $6D, $C9, $5B
  .byte $D0, $DD, $20, $A6, $E1, $20, $45, $E1, $A9, $20, $20, $1B, $E1, $A0, $00, $B1
  .byte $6D, $F0, $09, $20, $1B, $E1, $20, $A6, $E1, $4C, $8B, $E7, $20, $8A, $E1, $A9
  .byte $00, $20, $1B, $E1, $68, $85, $E7, $68, $85, $E6, $20, $26, $E1, $4C, $18, $C8
  .byte $4C, $61, $E3, $20, $2A, $E2, $A5, $06, $85, $E6, $48, $A5, $07, $85, $E7, $48
  .byte $20, $DC, $E1, $20, $13, $E2, $A9, $5B, $20, $1B, $E1, $A0, $00, $B1, $6D, $C9
  .byte $5B, $D0, $DD, $20, $A6, $E1, $A0, $00, $B1, $6D, $F0, $06, $20, $1B, $E1, $4C
  .byte $CF, $E7, $A9, $20, $20, $1B, $E1, $20, $45, $E1, $20, $8A, $E1, $A9, $00, $20
  .byte $1B, $E1, $68, $85, $E7, $68, $85, $E6, $20, $26, $E1, $4C, $18, $C8, $20, $B4
  .byte $E1, $20, $13, $E2, $A0, $00, $B1, $6B, $C9, $01, $D0, $03, $4C, $69, $DF, $C9
  .byte $5B, $D0, $03, $4C, $7C, $DF, $A5, $B8, $48, $A5, $B9, $48, $A5, $6B, $85, $B8
  .byte $A5, $6C, $85, $B9, $20, $B1, $00, $20, $60, $F0, $20, $B7, $00, $AA, $68, $85
  .byte $B9, $68, $85, $B8, $E0, $00, $F0, $D4, $4C, $7C, $DF, $20, $B4, $E1, $20, $13
  .byte $E2, $A0, $00, $B1, $6B, $C9, $5B, $D0, $03, $4C, $7C, $DF, $4C, $69, $DF, $20
  .byte $B4, $E1, $20, $13, $E2, $A0, $00, $B1, $6B, $C9, $5B, $D0, $03, $4C, $69, $DF
  .byte $4C, $7C, $DF, $20, $AC, $DE, $20, $13, $E2, $20, $15, $F0, $20, $48, $F0, $C9
  .byte $FF, $F0, $03, $4C, $7C, $DF, $4C, $69, $DF, $20, $AC, $DE, $20, $13, $E2, $20
  .byte $15, $F0, $20, $48, $F0, $C9, $01, $F0, $03, $4C, $7C, $DF, $4C, $69, $DF, $20
  .byte $DC, $E1, $20, $13, $E2, $A0, $00, $B1, $6B, $D1, $6D, $D0, $33, $C9, $01, $D0
  .byte $20, $20, $9F, $E1, $A4, $6C, $A5, $6B, $20, $30, $F0, $20, $A6, $E1, $A4, $6E
  .byte $A5, $6D, $20, $12, $F0, $A5, $9D, $C9, $6D, $B0, $03, $4C, $69, $DF, $4C, $7C
  .byte $DF, $A0, $00, $B1, $6B, $D1, $6D, $D0, $F5, $C8, $C9, $00, $D0, $F5, $F0, $EB
  .byte $A0, $00, $B1, $6B, $C9, $5B, $F0, $E6, $B1, $6D, $C9, $5B, $F0, $E0, $B1, $6B
  .byte $C9, $01, $D0, $10, $A4, $6C, $A6, $6B, $A5, $6E, $85, $6C, $A5, $6D, $85, $6B
  .byte $84, $6E, $86, $6D, $A5, $B8, $48, $A5, $B9, $48, $A5, $6C, $85, $B9, $A5, $6B
  .byte $85, $B8, $20, $B1, $00, $20, $60, $F0, $20, $B7, $00, $AA, $68, $85, $B9, $68
  .byte $85, $B8, $E0, $00, $D0, $A8, $F0, $93, $4C, $69, $DF, $A5, $6D, $85, $6B, $A5
  .byte $6E, $85, $6C, $A0, $FF, $C8, $B1, $6B, $F0, $06, $D9, $83, $FF, $F0, $F6, $60
  .byte $B9, $83, $FF, $60, $20, $DC, $E1, $20, $13, $E2, $20, $1F, $E9, $F0, $08, $20
  .byte $17, $E9, $F0, $03, $4C, $7C, $DF, $4C, $69, $DF, $20, $DC, $E1, $20, $13, $E2
  .byte $20, $1F, $E9, $D0, $08, $20, $17, $E9, $D0, $03, $4C, $69, $DF, $4C, $7C, $DF
  .byte $20, $B4, $E1, $20, $13, $E2, $20, $1F, $E9, $D0, $03, $4C, $7C, $DF, $4C, $69
  .byte $DF, $20, $3E, $DF, $20, $B4, $E1, $20, $13, $E2, $20, $1F, $E9, $F0, $07, $A9
  .byte $FF, $85, $D3, $4C, $12, $C8, $A9, $00, $85, $D3, $4C, $12, $C8, $20, $3E, $DF
  .byte $A5, $D3, $C9, $00, $F0, $19, $20, $B1, $00, $C9, $80, $F0, $12, $C9, $01, $D0
  .byte $F5, $A9, $05, $18, $65, $B8, $85, $B8, $90, $02, $E6, $B9, $4C, $92, $E9, $4C
  .byte $12, $C8, $20, $3E, $DF, $A5, $D3, $C9, $FF, $F0, $F4, $D0, $D9, $20, $3E, $DF
  .byte $20, $B4, $E1, $20, $1F, $E9, $F0, $1D, $20, $B1, $00, $C9, $01, $D0, $0E, $A9
  .byte $05, $18, $65, $B8, $85, $B8, $90, $02, $E6, $B9, $4C, $C4, $E9, $C9, $A8, $F0
  .byte $04, $C9, $80, $D0, $E3, $4C, $12, $C8, $20, $3E, $DF, $4C, $12, $C8, $20, $3E
  .byte $DF, $20, $B1, $00, $C9, $01, $D0, $0E, $A9, $05, $18, $65, $B8, $85, $B8, $90
  .byte $02, $E6, $B9, $4C, $ED, $E9, $C9, $80, $D0, $E7, $4C, $12, $C8

sub_b30_AA09:
  jsr sub_b31_DF3E
  jsr sub_b31_C83F
  jmp loc_b31_C809

sub_b30_AA12:
  jsr sub_b31_E1B4
  jsr sub_b31_E27D
  jsr sub_b31_C82A
  jsr sub_b31_DF3E
  jmp loc_b31_C812
  .byte $20, $B4, $E1, $20, $7D, $E2, $20, $3E, $DF, $4C, $12, $C8, $20, $DC, $E1, $20
  .byte $2D, $C8, $20, $3E, $DF, $4C, $12, $C8, $A5, $DE, $D0, $07, $A5, $DF, $D0, $03
  .byte $4C, $8B, $E3, $20, $3E, $DF, $20, $2A, $E2, $20, $2A, $E2, $20, $30, $C8, $20
  .byte $45, $E2, $20, $33, $C8, $4C, $15, $C8, $4C, $61, $E3, $20, $0A, $DF, $C9, $22
  .byte $D0, $F6, $20, $13, $E2, $20, $36, $C8, $A5, $6F, $C9, $80, $F0, $03, $4C, $69
  .byte $DF, $4C, $7C, $DF, $4C, $7D, $E3, $4C, $61, $E3, $20, $2A, $E2, $20, $0A, $DF
  .byte $C9, $22, $D0, $F3, $20, $36, $C8, $A5, $6F, $C9, $80, $F0, $E7, $A0, $FF, $C8
  .byte $B1, $99, $D0, $FB, $98, $38, $65, $99, $85, $99, $90, $02, $E6, $9A, $20, $13
  .byte $E2, $A0, $00, $B1, $99, $C9, $01, $D0, $13, $A2, $05, $B1, $99, $20, $3A, $E2
  .byte $E6, $99, $D0, $02, $E6, $9A, $CA, $10, $F2, $4C, $18, $C8, $B1, $99, $20, $3A
  .byte $E2, $E6, $99, $D0, $02, $E6, $9A, $C9, $00, $D0, $F1, $4C, $18, $C8, $20, $13
  .byte $E2, $20, $2A, $E2, $A9, $22, $20, $3A, $E2, $20, $00, $C8, $20, $3A, $E2, $A9
  .byte $00, $20, $3A, $E2, $4C, $18, $C8, $20, $13, $E2, $20, $2A, $E2, $A9, $5B, $20
  .byte $3A, $E2, $20, $48, $C8, $A2, $FF, $E8, $BD, $00, $03, $C9, $0D, $F0, $06, $20
  .byte $3A, $E2, $4C, $F8, $EA, $A9, $00, $20, $3A, $E2, $4C, $18, $C8, $20, $3E, $DF
  .byte $A5, $00, $85, $E2, $A5, $01, $85, $E3, $A5, $E2, $D0, $07, $A5, $E3, $D0, $03
  .byte $4C, $12, $C8, $A0, $06, $B1, $E2, $F0, $0B, $20, $03, $C8, $C8, $B1, $E2, $D0
  .byte $F8, $20, $2A, $C8, $A0, $02, $B1, $E2, $AA, $C8, $B1, $E2, $85, $E3, $86, $E2
  .byte $4C, $19, $EB, $20, $49, $53, $20, $00, $20, $3E, $DF, $A5, $67, $85, $E2, $A5
  .byte $68, $85, $E3, $A5, $E2, $D0, $07, $A5, $E3, $D0, $03, $4C, $12, $C8, $20, $65
  .byte $EB, $4C, $54, $EB, $A0, $06, $B1, $E2, $F0, $27, $20, $03, $C8, $C8, $B1, $E2
  .byte $D0, $F8, $98, $48, $A9, $EB, $85, $6C, $A9, $44, $85, $6B, $20, $27, $C8, $68
  .byte $38, $65, $E2, $85, $6B, $A5, $E3, $69, $00, $85, $6C, $20, $24, $C8, $20, $2A
  .byte $C8, $A0, $02, $B1, $E2, $AA, $C8, $B1, $E2, $85, $E3, $86, $E2, $60, $54, $4F
  .byte $20, $00, $4C, $61, $E3, $20, $3E, $DF, $A0, $01, $B1, $B8, $C9, $FF, $D0, $F2
  .byte $20, $B1, $00, $20, $B1, $00, $A5, $B8, $85, $12, $A5, $B9, $85, $13, $20, $39
  .byte $C8, $F0, $03, $4C, $84, $E3, $A5, $B8, $48, $A5, $B9, $48, $A5, $D4, $85, $B8
  .byte $A5, $D5, $85, $B9, $20, $ED, $EB, $68, $85, $B9, $68, $85, $B8, $20, $B1, $00
  .byte $C9, $00, $D0, $F9, $4C, $12, $C8, $20, $45, $4E, $44, $00, $A9, $EB, $85, $6C
  .byte $A9, $9F, $85, $6B, $20, $27, $C8, $20, $B7, $00, $20, $03, $C8, $20, $B1, $00
  .byte $C9, $80, $F0, $12, $C9, $00, $F0, $06, $20, $03, $C8, $4C, $FE, $EB, $A9, $20
  .byte $20, $03, $C8, $4C, $FE, $EB, $20, $2A, $C8, $20, $B1, $00, $C9, $8B, $F0, $09
  .byte $20, $5A, $C8, $20, $1E, $C8, $4C, $17, $EC, $A9, $EB, $85, $6C, $A9, $E8, $85
  .byte $6B, $20, $27, $C8, $20, $2A, $C8, $60, $A5, $B8, $48, $A5, $B9, $48, $A5, $00
  .byte $85, $E2, $A5, $01, $85, $E3, $A0, $06, $B1, $E2, $F0, $2B, $A9, $06, $18, $65
  .byte $E2, $85, $B8, $A5, $E3, $69, $00, $85, $B9, $A5, $E2, $48, $A5, $E3, $48, $20
  .byte $ED, $EB, $68, $85, $E3, $68, $85, $E2, $A0, $02, $B1, $E2, $AA, $C8, $B1, $E2
  .byte $85, $E3, $86, $E2, $4C, $47, $EC, $68, $85, $B9, $68, $85, $B8, $60, $20, $3E
  .byte $DF, $20, $39, $EC, $4C

sub_b30_AC86:
  .byte $12, $C8, $20, $3E, $DF, $20, $39, $EC, $4C, $4C, $EB, $20, $3E, $DF, $20, $34
  .byte $DF, $20, $36, $C8, $20, $3C, $C8, $4C, $12, $C8, $20, $3E, $DF, $20, $78, $FE
  .byte $4C, $12, $C8, $20, $3E, $DF, $20, $5E, $FE, $4C, $12, $C8, $20, $3E, $DF, $20
  .byte $5E, $FE, $20, $78, $FE, $4C, $12, $C8, $9F, $7F, $FF, $FF, $FE, $4C, $61, $E3
  .byte $20, $3E, $DF, $A5, $06, $48, $A5, $07, $48, $20, $DC, $E1, $20, $70, $DE, $20
  .byte $48, $F0, $C9, $FF, $F0, $E7, $A0, $EC, $A9, $BE, $20, $4E, $F0, $C9, $01, $F0
  .byte $DC, $20, $1E, $F0, $20, $5D, $F0, $20, $92, $ED, $A0, $00, $B1, $6D, $C9, $5B
  .byte $D0, $CB, $A5, $6D, $85, $B8, $A5, $6E, $85, $B9, $68, $85, $07, $68, $85, $06
  .byte $A5, $07, $48, $A5, $06, $48, $A9, $9E, $20, $3A, $E2, $A9, $80, $20, $3A, $E2
  .byte $20, $1B, $C8, $A9, $9F, $20, $3A, $E2, $68, $20, $3A, $E2, $85, $B8, $68, $85
  .byte $B9, $20, $3A, $E2, $A5, $06, $85, $18, $A5, $07, $85, $19, $20, $5A, $C8, $4C
  .byte $12, $C8, $20, $2A, $E2, $A0, $05, $C8, $B1, $D0, $99, $98, $00, $C0, $09, $D0
  .byte $F6, $20, $79, $ED, $F0, $12, $20, $85, $ED, $A0, $05, $C8, $B9, $98, $00, $91
  .byte $D0, $C0, $09, $D0, $F6, $4C, $12, $C8, $20, $D0, $ED, $4C, $12, $C8, $20, $2A
  .byte $E2, $A0, $01, $B1, $B8, $AA, $C8, $B1, $B8, $85, $B9, $86, $B8, $20, $5A, $C8
  .byte $4C, $12, $C8, $A2, $FF, $E8, $B5, $9E, $D0, $04, $E0, $03, $D0, $F7, $60, $A2
  .byte $03, $18, $B5, $9E, $E9, $00, $95, $9E, $CA, $10, $F7, $60, $A9, $0A, $18, $65
  .byte $D0, $85, $D0, $90, $02, $E6, $D1, $A5, $D1, $C9, $69, $90, $03, $4C, $51, $C8
  .byte $A0, $00, $A5, $B8, $91, $D0, $C8, $A5, $B9, $91, $D0, $C8, $A5, $18, $91, $D0
  .byte $C8, $A5, $19, $91, $D0, $C8, $A5, $06, $91, $D0, $C8, $A5, $07, $91, $D0, $C8
  .byte $B9, $98, $00, $91, $D0, $C0, $09, $D0, $F6, $60, $A0, $00, $B1, $D0, $85, $B8
  .byte $C8, $B1, $D0, $85, $B9, $C8, $B1, $D0, $85, $18, $C8, $B1, $D0, $85, $19, $C8
  .byte $B1, $D0, $85, $06, $C8, $B1, $D0, $85, $07, $38, $A5, $D0, $E9, $0A, $85, $D0
  .byte $B0, $02, $C6, $D1, $60, $4C, $41, $42, $4C, $45, $20, $49, $53, $20, $4F, $55
  .byte $54, $20, $4F, $46, $20, $50, $4C, $41, $43, $45, $20, $00, $A0, $ED, $A2, $FB
  .byte $4C, $9E, $E3, $20, $3E, $DF, $C6, $B9, $A0, $FF, $B1, $B8, $E6, $B9, $C9, $80
  .byte $D0, $EA, $20, $B1, $00, $C9, $00, $D0, $F9, $4C, $12, $C8, $4C, $41, $42, $4C
  .byte $45, $20, $4E, $4F, $20, $46, $49, $4E, $44, $20, $00, $46, $55, $4E, $43, $54
  .byte $49, $4F, $4E, $20, $43, $41, $4E, $20, $4F, $4E, $4C, $59, $20, $42, $45, $20
  .byte $55, $53, $45, $44, $20, $49, $4E, $20, $41, $20, $50, $52, $4F, $43, $45, $44
  .byte $55, $52, $45, $20, $00, $A0, $EE, $A2, $32, $4C, $9E, $E3, $A0, $EE, $A2, $41
  .byte $4C, $9E, $E3, $A5, $DE, $F0, $F5, $20, $34, $DF, $20, $3E, $DF, $C6, $05, $A0
  .byte $FF, $B1, $04, $E6, $05, $AA, $A5, $00, $85, $E2, $A5, $01, $85, $E3, $E0, $00
  .byte $F0, $10, $A0, $02, $B1, $E2, $48, $C8, $B1, $E2, $85, $E3, $68, $85, $E2, $CA
  .byte $D0, $F0, $18, $A9, $03, $65, $E2, $85, $B8, $A5, $E3, $69, $00, $85, $B9, $20
  .byte $B1, $00, $C9, $8B, $F0, $AF, $C9, $80, $D0, $F5, $20, $B1, $00, $C9, $C7, $D0
  .byte $F1, $20, $B1, $00, $A0, $FF, $C8, $B1, $6B, $F0, $04, $D1, $B8, $F0, $F7, $B1
  .byte $B8, $D0, $DC, $20, $5A, $C8, $20, $5A, $C8, $4C, $12, $C8, $54, $52, $41, $43
  .byte $45, $20, $4F, $4E, $00, $54, $52, $41, $43, $45, $20, $4F, $46, $46, $00, $20
  .byte $3E, $DF, $A9, $EE, $85, $6C, $A9, $E2, $85, $6B, $A9, $FF, $85, $F4, $20, $27
  .byte $C8, $20, $2A, $C8, $4C, $12, $C8, $20, $3E, $DF, $A9, $EE, $85, $6C, $A9, $EB
  .byte $85, $6B, $A9, $00, $4C, $02, $EF, $A5, $DE, $D0, $07, $A5, $DF, $D0, $03, $4C
  .byte $09, $C8, $20, $30, $C8, $A5, $DE, $D0, $02, $C6, $DF, $C6, $DE, $20, $3E, $DF
  .byte $20, $2A, $E2, $20, $3E, $DF, $20, $33, $C8, $4C, $12, $C8, $20, $3E, $DF, $4C
  .byte $09, $C8, $20, $3E, $DF, $4C, $06, $C8, $4C, $61, $E3, $20, $3E, $DF, $A9, $00
  .byte $85, $FC, $A9, $78, $85, $FD, $A0, $01, $B1, $B8, $C9, $FF, $D0, $EA, $20, $B1
  .byte $00, $20, $B1, $00, $A5, $B8, $85, $12, $A5, $B9, $85, $13, $A0, $FF, $C8, $B1
  .byte $12, $99, $00, $03, $D0, $F8, $20, $39, $C8, $85, $FE, $D0, $2B, $A5, $D4, $85
  .byte $B8, $A5, $D5, $85, $B9, $A5, $B8, $48, $A5, $B9, $48, $A9, $AA, $85, $FA, $20
  .byte $ED, $EB, $A9, $00, $85, $FA, $68, $85, $B9, $68, $85, $B8, $A9, $00, $A0, $00
  .byte $91, $FC, $C8, $91, $FC, $4C, $D5, $EF, $A9, $54, $8D, $00, $78, $A9, $4F, $8D
  .byte $01, $78, $A9, $20, $8D, $02, $78, $A0, $FF, $C8, $B9, $00, $03, $F0, $06, $99
  .byte $03, $78, $4C, $BF, $EF, $A9, $0D, $99, $03, $78, $A9, $00, $99, $04, $78, $20
  .byte $12, $A0, $20, $42, $C8, $C9, $07, $F0, $10, $A5, $FE, $D0, $09, $A0, $00, $A9
  .byte $00, $91, $B8, $C8, $91, $B8, $20, $45, $C8, $20, $09, $A0, $4C, $0C, $C8, $C4
  .byte $02, $23, $46, $4C, $4F, $41, $54, $0D, $0A, $86, $4C, $8F, $F2, $4C, $92, $F2
  .byte $4C, $76, $F3, $4C, $79, $F3, $4C, $CE, $F0, $4C, $D1, $F0, $4C, $B7, $F0, $4C
  .byte $BA, $F0, $4C, $7C, $F0, $4C, $BF, $F4, $4C, $33, $F5, $4C, $82, $F7, $4C, $DF
  .byte $F8, $4C, $E6, $F8, $4C, $2F, $F9, $4C, $96, $F9, $4C, $09, $F4, $4C, $2E, $F4
  .byte $4C, $3E, $F4, $4C, $33, $F4, $4C, $3B, $F4, $4C, $63, $F4, $4C, $73, $F4, $4C
  .byte $F3, $F2, $4C, $92, $F4, $4C, $29, $F6, $4C, $C2, $F4, $4C, $9C, $F0, $4C, $8F
  .byte $F0, $4C, $A3, $F4, $4C, $A5, $F0, $4C, $02, $F5, $4C, $5A, $F5, $4C, $AF, $F0
  .byte $A5, $A2, $30, $1F, $A5, $9D, $C9, $90, $90, $09, $A9, $FE, $A0, $E0, $20, $C2
  .byte $F4, $D0, $10, $4C, $02, $F5, $4C, $57, $C8, $48, $A9, $5C, $20, $03, $A0, $68
  .byte $4C, $09, $D0, $A2, $35, $4C, $7C, $F0, $EA, $A2, $00, $86, $11, $85, $9E, $84
  .byte $9F, $A2, $90, $4C, $AB, $F4, $A9, $00, $38, $F0, $EE, $60, $4C, $89, $F0, $20
  .byte $66, $F0, $A6, $A0, $D0, $F6, $A6, $A1, $60, $A9, $59, $A0, $F7, $4C, $CE, $F0
  .byte $60, $20, $F3, $F2, $A5, $A2, $49, $FF, $85, $A2, $45, $AA, $85, $AB, $A5, $9D
  .byte $4C, $D1, $F0, $20, $00, $F2, $90, $3C, $20, $F3, $F2, $D0, $03, $4C, $63, $F4
  .byte $A6, $AC, $86, $92, $A2, $A5, $A5, $A5, $A8, $F0, $D5, $38, $E5, $9D, $F0, $24
  .byte $90, $12, $84, $9D, $A4, $AA, $84, $A2, $49, $FF, $69, $00, $A0, $00, $84, $92
  .byte $A2, $9D, $D0, $04, $A0, $00, $84, $AC, $C9, $F9, $30, $C7, $A8, $A5, $AC, $56
  .byte $01, $20, $17, $F2, $24, $AB, $10, $57, $A0, $9D, $E0, $A5, $F0, $02, $A0, $A5
  .byte $38, $49, $FF, $65, $92, $85, $AC, $B9, $04, $00, $F5, $04, $85, $A1, $B9, $03
  .byte $00, $F5, $03, $85, $A0, $B9, $02, $00, $F5, $02, $85, $9F, $B9, $01, $00, $F5
  .byte $01, $85, $9E, $B0, $03, $20, $AE, $F1, $A0, $00, $98, $18, $A6, $9E, $D0, $4A
  .byte $A6, $9F, $86, $9E, $A6, $A0, $86, $9F, $A6, $A1, $86, $A0, $A6, $AC, $86, $A1
  .byte $84, $AC, $69, $08, $C9, $20, $D0, $E4, $A9, $00, $85, $9D, $85, $A2, $60, $65
  .byte $92, $85, $AC, $A5, $A1, $65, $A9, $85, $A1, $A5, $A0, $65, $A8, $85, $A0, $A5
  .byte $9F, $65, $A7, $85, $9F, $A5, $9E, $65, $A6, $85, $9E, $4C, $9D, $F1, $69, $01
  .byte $06, $AC, $26, $A1, $26, $A0, $26, $9F, $26, $9E, $10, $F2, $38, $E5, $9D, $B0
  .byte $C7, $49, $FF, $69, $01, $85, $9D, $90, $0E, $E6, $9D, $F0, $42, $66, $9E, $66
  .byte $9F, $66, $A0, $66, $A1, $66, $AC, $60, $A5, $A2, $49, $FF, $85, $A2, $A5, $9E
  .byte $49, $FF, $85, $9E, $A5, $9F, $49, $FF, $85, $9F, $A5, $A0, $49, $FF, $85, $A0
  .byte $A5, $A1, $49, $FF, $85, $A1, $A5, $AC, $49, $FF, $85, $AC, $E6, $AC, $D0, $0E
  .byte $E6, $A1, $D0, $0A, $E6, $A0, $D0, $06, $E6, $9F, $D0, $02, $E6, $9E, $60, $A2
  .byte $45, $4C, $7C, $F0, $A2, $61, $B4, $04, $84, $AC, $B4, $03, $94, $04, $B4, $02
  .byte $94, $03, $B4, $01, $94, $02, $A4, $A4, $94, $01, $69, $08, $30, $E8, $F0, $E6
  .byte $E9, $08, $A8, $A5, $AC, $B0, $14, $16, $01, $90, $02, $F6, $01, $76, $01, $76
  .byte $01, $76, $02, $76, $03, $76, $04, $6A, $C8, $D0, $EC, $18, $60, $81, $00, $00
  .byte $00, $00, $03, $7F, $5E, $56, $CB, $79, $80, $13, $9B, $0B, $64, $80, $76, $38
  .byte $93, $16, $82, $38, $AA, $3B, $20, $80, $35, $04, $F3, $34, $81, $35, $04, $F3
  .byte $34, $80, $80, $00, $00, $00, $80, $31, $72, $17, $F8, $20, $92, $F4, $F0, $02
  .byte $10, $03, $4C, $89, $F0, $A5, $9D, $E9, $7F, $48, $A9, $80, $85, $9D, $A9, $3D
  .byte $A0, $F2, $20, $CE, $F0, $A9, $42, $A0, $F2, $20, $76, $F3, $A9, $23, $A0, $F2
  .byte $20, $B7, $F0, $A9, $28, $A0, $F2, $20, $51, $F8, $A9, $47, $A0, $F2, $20, $CE
  .byte $F0, $68, $20, $E5, $F5, $A9, $4C, $A0, $F2, $20, $F3, $F2, $D0, $03, $4C, $22
  .byte $F2, $20, $1E, $F3, $A9, $00, $85, $62, $85, $63, $85, $64, $85, $65, $A5, $AC
  .byte $20, $C0, $F2, $A5, $A1, $20, $C0, $F2, $A5, $A0, $20, $C0, $F2, $A5, $9F, $20
  .byte $C0, $F2, $A5, $9E, $20, $C5, $F2, $4C, $F6, $F3, $D0, $03, $4C, $EA, $F1, $4A
  .byte $09, $80, $A8, $90, $19, $18, $A5, $65, $65, $A9, $85, $65, $A5, $64, $65, $A8
  .byte $85, $64, $A5, $63, $65, $A7, $85, $63, $A5, $62, $65, $A6, $85, $62, $66, $62
  .byte $66, $63, $66, $64, $66, $65, $66, $AC, $98, $4A, $D0, $D6, $60, $85, $5E, $84
  .byte $5F, $A0, $04, $B1, $5E, $85, $A9, $88, $B1, $5E, $85, $A8, $88, $B1, $5E, $85
  .byte $A7, $88, $B1, $5E, $85, $AA, $45, $A2, $85, $AB, $A5, $AA, $09, $80, $85, $A6
  .byte $88, $B1, $5E, $85, $A5, $A5, $9D, $60, $A5, $A5, $F0, $1F, $18, $65, $9D, $90
  .byte $04, $30, $1D, $18, $2C, $10, $14, $69, $80, $85, $9D, $D0, $03, $4C, $62, $F1
  .byte $A5, $AB, $85, $A2, $60, $A5, $A2, $49, $FF, $30, $05, $68, $68, $4C, $5E, $F1
  .byte $4C, $E5, $F1, $20, $73, $F4, $AA, $F0, $10, $18, $69, $02, $B0, $F2, $A2, $00
  .byte $86, $AB, $20, $DE, $F0, $E6, $9D, $F0, $E7, $60, $84, $20, $00, $00, $00, $20
  .byte $73, $F4, $A9, $60, $A0, $F3, $A2, $00, $86, $AB, $20, $09, $F4, $4C, $79, $F3
  .byte $20, $F3, $F2, $F0, $76, $20, $82, $F4, $A9, $00, $38, $E5, $9D, $85, $9D, $20
  .byte $1E, $F3, $E6, $9D, $F0, $BA, $A2, $FC, $A9, $01, $A4, $A6, $C4, $9E, $D0, $10
  .byte $A4, $A7, $C4, $9F, $D0, $0A, $A4, $A8, $C4, $A0, $D0, $04, $A4, $A9, $C4, $A1
  .byte $08, $2A, $90, $09, $E8, $95, $65, $F0, $32, $10, $34, $A9, $01, $28, $B0, $0E
  .byte $06, $A9, $26, $A8, $26, $A7, $26, $A6, $B0, $E6, $30, $CE, $10, $E2, $A8, $A5
  .byte $A9, $E5, $A1, $85, $A9, $A5, $A8, $E5, $A0, $85, $A8, $A5, $A7, $E5, $9F, $85
  .byte $A7, $A5, $A6, $E5, $9E, $85, $A6, $98, $4C, $B6, $F3, $A9, $40, $D0, $CE, $0A
  .byte $0A, $0A, $0A, $0A, $0A, $85, $AC, $28, $4C, $F6, $F3, $A2, $85, $4C, $7C, $F0
  .byte $A5, $62, $85, $9E, $A5, $63, $85, $9F, $A5, $64, $85, $A0, $A5, $65, $85, $A1
  .byte $4C, $3E, $F1, $85, $5E, $84, $5F, $A0, $04, $B1, $5E, $85, $A1, $88, $B1, $5E
  .byte $85, $A0, $88, $B1, $5E, $85, $9F, $88, $B1, $5E, $85, $A2, $09, $80, $85, $9E
  .byte $88, $B1, $5E, $85, $9D, $84, $AC, $60, $A2, $98, $2C, $A2, $93, $A0, $00, $F0
  .byte $04, $A6, $85, $A4, $86, $20, $82, $F4, $86, $5E, $84, $5F, $A0, $04, $A5, $A1
  .byte $91, $5E, $88, $A5, $A0, $91, $5E, $88, $A5, $9F, $91, $5E, $88, $A5, $A2, $09
  .byte $7F, $25, $9E, $91, $5E, $88, $A5, $9D, $91, $5E, $84, $AC, $60, $A5, $AA, $85
  .byte $A2, $A2, $05, $B5, $A4, $95, $9C, $CA, $D0, $F9, $86, $AC, $60, $20, $82, $F4
  .byte $A2, $06, $B5, $9C, $95, $A4, $CA, $D0, $F9, $86, $AC, $60, $A5, $9D, $F0, $FB
  .byte $06, $AC, $90, $F7, $20, $D6, $F1, $D0, $F2, $4C, $9F, $F1, $A5, $9D, $F0, $09
  .byte $A5, $A2, $2A, $A9, $FF, $B0, $02, $A9, $01, $60, $20, $92, $F4, $85, $9E, $A9
  .byte $00, $85, $9F, $A2, $88, $A5, $9E, $49, $FF, $2A, $A9, $00, $85, $A1, $85, $A0
  .byte $86, $9D, $85, $AC, $85, $A2, $4C, $39, $F1, $46, $A2, $60, $85, $60, $84, $61
  .byte $A0, $00, $B1, $60, $C8, $AA, $F0, $C4, $B1, $60, $45, $A2, $30, $C2, $E4, $9D
  .byte $D0, $21, $B1, $60, $09, $80, $C5, $9E, $D0, $19, $C8, $B1, $60, $C5, $9F, $D0
  .byte $12, $C8, $B1, $60, $C5, $A0, $D0, $0B, $C8, $A9, $7F, $C5, $AC, $B1, $60, $E5
  .byte $A1, $F0, $A6, $A5, $A2, $90, $02, $49, $FF, $4C, $98, $F4, $A5, $9D, $F0, $4A
  .byte $38, $E9, $A0, $24, $A2, $10, $09, $AA, $A9, $FF, $85, $A4, $20, $B4, $F1, $8A
  .byte $A2, $9D, $C9, $F9, $10, $06, $20, $00, $F2, $84, $A4, $60, $A8, $A5, $A2, $29
  .byte $80, $46, $9E, $05, $9E, $85, $9E, $20, $17, $F2, $84, $A4, $60, $A5, $9D, $C9
  .byte $A0, $B0, $20, $20, $02, $F5, $84, $AC, $A5, $A2, $84, $A2, $49, $80, $2A, $A9
  .byte $A0, $85, $9D, $A5, $A1, $85, $0D, $4C, $39, $F1, $85, $9E, $85, $9F, $85, $A0
  .byte $85, $A1, $A8, $60, $A0, $00, $A2, $0A, $94, $99, $CA, $10, $FB, $90, $0F, $C9
  .byte $2D, $D0, $04, $86, $A3, $F0, $04, $C9, $2B, $D0, $05, $20, $B1, $00, $90, $5B
  .byte $C9, $2E, $F0, $2E, $C9, $45, $D0, $30, $20, $B1, $00, $90, $17, $C9, $C9, $F0
  .byte $0E, $C9, $2D, $F0, $0A, $C9, $C8, $F0, $08, $C9, $2B, $F0, $04, $D0, $07, $66
  .byte $9C, $20, $B1, $00, $90, $5C, $24, $9C, $10, $0E, $A9, $00, $38, $E5, $9A, $4C
  .byte $B0, $F5, $66, $9B, $24, $9B, $50, $C3, $A5, $9A, $38, $E5, $99, $85, $9A, $F0
  .byte $12, $10, $09, $20, $65, $F3, $E6, $9A, $D0, $F9, $F0, $07, $20, $49, $F3, $C6
  .byte $9A, $D0, $F9, $A5, $A3, $30, $01, $60, $4C, $C5, $F7, $48, $24, $9B, $10, $02
  .byte $E6, $99, $20, $49, $F3, $68, $38, $E9, $30, $20, $E5, $F5, $4C, $71, $F5, $48
  .byte $20, $73, $F4, $68, $20, $A3, $F4, $A5, $AA, $45, $A2, $85, $AB, $A6, $9D, $4C
  .byte $D1, $F0, $A5, $9A, $C9, $0A, $90, $09, $A9, $64, $24, $9C, $30, $11, $4C, $E5
  .byte $F1, $0A, $0A, $18, $65, $9A, $0A, $18, $A0, $00, $71, $B8, $38, $E9, $30, $85
  .byte $9A, $4C, $97, $F5, $9B, $3E, $BC, $1F, $FD, $9E, $6E, $6B, $27, $FD, $9E, $6E
  .byte $6B, $28, $00, $A0, $01, $A9, $2D, $88, $24, $A2, $10, $04, $C8, $99, $FF, $00
  .byte $85, $A2, $84, $AD, $C8, $A9, $30, $A6, $9D, $D0, $03, $4C, $4C, $F7, $A9, $00
  .byte $E0, $80, $F0, $02, $B0, $09, $A9, $24, $A0, $F6, $20, $8F, $F2, $A9, $F7, $85
  .byte $99, $A9, $1F, $A0, $F6, $20, $C2, $F4, $F0, $1E, $10, $12, $A9, $1A, $A0, $F6
  .byte $20, $C2, $F4, $F0, $02, $10, $0E, $20, $49, $F3, $C6, $99, $D0, $EE, $20, $65
  .byte $F3, $E6, $99, $D0, $DC, $20, $AF, $F0, $20, $02, $F5, $A2, $01, $A5, $99, $18
  .byte $69, $0A, $30, $09, $C9, $0B, $B0, $06, $69, $FF, $AA, $A9, $02, $38, $E9, $02
  .byte $85, $9A, $86, $99, $8A, $F0, $02, $10, $13, $A4, $AD, $A9, $2E, $C8, $99, $FF
  .byte $00, $8A, $F0, $06, $A9, $30, $C8, $99, $FF, $00, $84, $AD, $A0, $00, $A2, $80
  .byte $A5, $A1, $18, $79, $61, $F7, $85, $A1, $A5, $A0, $79, $60, $F7, $85, $A0, $A5
  .byte $9F, $79, $5F, $F7, $85, $9F, $A5, $9E, $79, $5E, $F7, $85, $9E, $E8, $B0, $04
  .byte $10, $DE, $30, $02, $30, $DA, $8A, $90, $04, $49, $FF, $69, $0A, $69, $2F, $C8
  .byte $C8, $C8, $C8, $84, $83, $A4, $AD, $C8, $AA, $29, $7F, $99, $FF, $00, $C6, $99
  .byte $D0, $06, $A9, $2E, $C8, $99, $FF, $00, $84, $AD, $A4, $83, $8A, $49, $FF, $29
  .byte $80, $AA, $C0, $24, $D0, $AA, $A4, $AD, $B9, $FF, $00, $88, $C9, $30, $F0, $F8
  .byte $C9, $2E, $F0, $01, $C8, $A9, $2B, $A6, $9A, $F0, $2E, $10, $08, $A9, $00, $38
  .byte $E5, $9A, $AA, $A9, $2D, $99, $01, $01, $A9, $45, $99, $00, $01, $8A, $A2, $2F
  .byte $38, $E8, $E9, $0A, $B0, $FB, $69, $3A, $99, $03, $01, $8A, $99, $02, $01, $A9
  .byte $00, $99, $04, $01, $F0, $08, $99, $FF, $00, $A9, $00, $99, $00, $01, $A9, $00
  .byte $A0, $01, $60, $80, $00, $00, $00, $00, $FA, $0A, $1F, $00, $00, $98, $96, $80
  .byte $FF, $F0, $BD, $C0, $00, $01, $86, $A0, $FF, $FF, $D8, $F0, $00, $00, $03, $E8
  .byte $FF, $FF, $FF, $9C, $00, $00, $00, $0A, $FF, $FF, $FF, $FF, $20, $73, $F4, $A9
  .byte $59, $A0, $F7, $20, $09, $F4, $F0, $70, $A5, $A5, $D0, $03, $4C, $60, $F1, $A2
  .byte $8A, $A0, $00, $20, $3B, $F4, $A5, $AA, $10, $0F, $20, $33, $F5, $A9, $8A, $A0
  .byte $00, $20, $C2, $F4, $D0, $03, $98, $A4, $0D, $20, $65, $F4, $98, $48, $20, $51
  .byte $F2, $A9, $8A, $A0, $00, $20, $8F, $F2, $20, $FE, $F7, $68, $4A, $90, $0A, $A5
  .byte $9D, $F0, $06, $A5, $A2, $49, $FF, $85, $A2, $60, $81, $38, $AA, $3B, $29, $07
  .byte $71, $34, $58, $3E, $56, $74, $16, $7E, $B3, $1B, $77, $2F, $EE, $E3, $85, $7A
  .byte $1D, $84, $1C, $2A, $7C, $63, $59, $58, $0A, $7E, $75, $FD, $E7, $C6, $80, $31
  .byte $72, $18, $10, $81, $00, $00, $00, $00, $A9, $D0, $A0, $F7, $20, $8F, $F2, $A5
  .byte $AC, $69, $50, $90, $03, $20, $8A, $F4, $85, $92, $20, $76, $F4, $A5, $9D, $C9
  .byte $88, $90, $03, $20, $3B, $F3, $20, $33, $F5, $A5, $0D, $18, $69, $81, $F0, $F3
  .byte $38, $E9, $01, $48, $A2, $05, $B5, $A5, $B4, $9D, $95, $9D, $94, $A5, $CA, $10
  .byte $F5, $A5, $92, $85, $AC, $20, $BA, $F0, $20, $C5, $F7, $A9, $D5, $A0, $F7, $20
  .byte $67, $F8, $A9, $00, $85, $AB, $68, $20, $20, $F3, $60, $85, $AD, $84, $AE, $20
  .byte $31, $F4, $A9, $93, $20, $8F, $F2, $20, $6B, $F8, $A9, $93, $A0, $00, $4C, $8F
  .byte $F2, $85, $AD, $84, $AE, $20, $2E, $F4, $B1, $AD, $85, $A3, $A4, $AD, $C8, $98
  .byte $D0, $02, $E6, $AE, $85, $AD, $A4, $AE, $20, $8F, $F2, $A5, $AD, $A4, $AE, $18
  .byte $69, $05, $90, $01, $C8, $85, $AD, $84, $AE, $20, $CE, $F0, $A9, $98, $A0, $00
  .byte $C6, $A3, $D0, $E4, $60, $98, $35, $44, $7A, $68, $28, $B1, $46, $20, $92, $F4
  .byte $AA, $30, $18, $A9, $C9, $A0, $00, $20, $09, $F4, $8A, $F0, $E7, $A9, $9B, $A0
  .byte $F8, $20, $8F, $F2, $A9, $9F, $A0, $F8, $20, $CE, $F0, $A6, $A1, $A5, $9E, $85
  .byte $A1, $86, $9E, $A9, $00, $85, $A2, $A5, $9D, $85, $AC, $A9, $80, $85, $9D, $20
  .byte $3E, $F1, $A2, $C9, $A0, $00, $4C, $3B, $F4, $A9, $5E, $A0, $F9, $20, $CE, $F0
  .byte $20, $73, $F4, $A9, $63, $A0, $F9, $A6, $AA, $20, $6E, $F3, $20, $73, $F4, $20
  .byte $33, $F5, $A9, $00, $85, $AB, $20, $BA, $F0, $A9, $68, $A0, $F9, $20, $B7, $F0
  .byte $A5, $A2, $48, $10, $0D, $20, $AF, $F0, $A5, $A2, $30, $09, $A5, $16, $49, $FF
  .byte $85, $16, $20, $C5, $F7, $A9, $68, $A0, $F9, $20, $CE, $F0, $68, $10, $03, $20
  .byte $C5, $F7, $A9, $6D, $A0, $F9, $4C, $51, $F8, $20, $31, $F4, $A9, $00, $85, $16
  .byte $20, $E6, $F8, $A2, $8A, $A0, $00, $20, $DC, $F8, $A9, $93, $A0, $00, $20, $09
  .byte $F4, $A9, $00, $85, $A2, $A5, $16, $20, $57, $F9, $A9, $8A, $A0, $00, $4C, $76
  .byte $F3, $48, $4C, $18, $F9, $4C, $23, $F0, $81, $49, $0F, $DA, $A2, $83, $49, $0F
  .byte $DA, $A2, $7F, $00, $00, $00, $00, $05, $84, $E6, $1A, $2D, $1B, $86, $28, $07
  .byte $FB, $F8, $87, $99, $68, $89, $01, $87, $23, $35, $DF, $E1, $86, $A5, $5D, $E7
  .byte $28, $83, $49, $0F, $DA, $A2, $A6, $D3, $C1, $C8, $D4, $C8, $D5, $C4, $CE, $CA
  .byte $A5, $A2, $48, $10, $03, $20, $C5, $F7, $A5, $9D, $48, $C9, $81, $90, $07, $A9
  .byte $23, $A0, $F2, $20, $76, $F3, $A9, $C6, $A0, $F9, $20, $51, $F8, $68, $C9, $81
  .byte $90, $07, $A9, $5E, $A0, $F9, $20, $B7, $F0, $68, $10, $03, $4C, $C5, $F7, $60
  .byte $0B, $76, $B3, $83, $BD, $D3, $79, $1E, $F4, $A6, $F5, $7B, $83, $FC, $B0, $10
  .byte $7C, $0C, $1F, $67, $CA, $7C, $DE, $53, $CB, $C1, $7D, $14, $64, $70, $4C, $7D
  .byte $B7, $EA, $51, $7A, $7D, $63, $30, $88, $7E, $7E, $92, $44, $99, $3A, $7E, $4C
  .byte $CC, $91, $C7, $7F, $AA, $AA, $AA, $13, $81, $00, $00, $00, $00, $E6, $B8, $D0
  .byte $02, $E6, $B9, $AD, $60, $EA, $C9, $3A, $B0, $0A, $C9, $20, $F0, $EF, $38, $E9
  .byte $30, $38, $E9, $D0, $60, $80, $4F, $C7, $52, $58, $2A, $F3, $30, $F4, $A0, $00
  .byte $0D, $0F, $47, $49, $56, $41, $59, $46, $20, $20, $CE, $02, $23, $24, $30, $30
  .byte $0D, $06, $D1, $01, $24, $31, $31, $0D, $06, $D0, $01, $24, $39, $45, $0D, $06
  .byte $D2, $01, $24, $39, $46, $0D, $07, $CE, $02, $23, $24, $39, $30, $0D, $08, $CA
  .byte $01, $46, $47, $49, $56, $31, $0D, $0F, $53, $4E, $47, $46, $4C, $54, $20, $20
  .byte $CD, $02, $23, $24, $30, $30, $0D, $03, $A9, $00, $0D, $09, $87, $01, $47, $49
  .byte $56, $41, $59, $46, $0D, $03, $A5, $00, $0D, $11, $43, $49, $4E, $54, $31, $20
  .byte $20, $20, $CA, $01, $4C, $4F, $47, $45, $52, $52, $0D, $11, $43, $4F, $4E, $49
  .byte $4E, $54, $20, $20, $CB, $01, $43, $4F, $41, $49, $4E, $54, $0D, $06, $CE, $01
  .byte $24, $41, $30, $0D, $08, $86, $01, $43, $49, $4E, $54, $31, $0D, $06, $CE, $01
  .byte $24, $41, $31, $0D, $03, $A5, $00, $0D, $10, $46, $41, $44, $44, $48, $20, $20
  .byte $20, $CD, $02, $23, $42, $55, $46, $32, $0D, $08, $CF, $02, $2F, $42, $55, $46
  .byte $32, $0D, $07, $CA, $01, $46, $41, $44, $44, $0D, $0B, $5E, $38, $20, $20, $20
  .byte $20, $20, $20, $A5, $00, $0D, $11, $46, $53, $55, $42, $20, $20, $20, $20, $CB
  .byte $01, $43, $4F, $4E, $55, $50, $4B, $0D, $0E, $46, $CB, $01, $58, $59, $32, $4C
  .byte $49, $4E, $0D, $03, $B1, $00, $0D, $03, $99, $00, $0D, $06, $C0, $02, $23, $42
  .byte $56, $0D, $07, $D0, $01, $42, $55, $46, $59, $0D, $05, $CD, $02, $23, $30, $0D
  .byte $07, $D0, $01, $42, $55, $46, $58, $0D, $03, $AF, $00, $0D, $03, $AD, $00, $0D
  .byte $03, $A2, $00, $0D, $0B, $D0, $05, $28, $42, $55, $46, $58, $29, $2C, $59, $0D
  .byte $03, $A5, $00, $0D, $11, $50, $44, $52, $54, $53, $20, $20, $20, $CB, $01, $44
  .byte $53, $50, $43, $55, $52, $0D, $08, $CB, $01, $57, $42, $54, $4F, $56, $0D, $03
  .byte $A5, $00, $0D, $0F, $57, $42, $54, $4F, $56, $20, $20, $20, $CE, $02, $23, $24
  .byte $32, $30, $0D, $05, $CF, $02, $23, $30, $0D, $0B, $CD, $02, $23, $44, $42, $55
  .byte $46, $46, $45, $52, $0D, $07, $D0, $01, $42, $55, $46, $58, $0D, $0B, $CD, $02
  .byte $2F, $44, $42, $55, $46, $46, $45, $52, $0D, $07, $D0, $01, $42, $55, $46, $59
  .byte $0D, $09, $CB, $01, $44, $49, $53, $4E, $4D, $49, $0D, $11, $57, $42, $54, $4F
  .byte $56, $31, $20, $20, $CB, $01, $57, $42, $4C, $41, $4E, $4B, $0D, $08, $D1, $01
  .byte $24, $32, $30, $30, $36, $0D, $08, $D2, $01, $24, $32, $30, $30, $36, $0D, $13
  .byte $5E, $31, $20, $20, $20, $20, $20, $20, $CD, $05, $28, $42, $55, $46, $58, $29
  .byte $2C, $59, $0D, $08, $D0, $01, $24, $32, $30, $30, $37, $0D, $03, $9F, $00, $0D
  .byte $07, $C6, $02, $23, $24, $38, $30, $0D, $05, $86, $20, $3E, $DF, $20, $34, $DF
  .byte $20, $1B, $A0, $4C, $09, $C8, $20, $3E, $DF, $20, $34, $DF, $20, $1E, $A0, $20
  .byte $4E, $C8, $4C, $09, $C8, $20, $3E, $DF, $20, $21, $A0, $4C, $09, $C8, $20, $3E
  .byte $DF, $20, $ED, $FD, $20, $6D, $DE, $20, $00, $B6, $4C, $12, $C8, $20, $3E, $DF
  .byte $20, $ED, $FD, $20, $6D, $DE, $20, $03, $B6, $4C, $12, $C8, $20, $3E, $DF, $20
  .byte $ED, $FD, $20, $AC, $DE, $20, $06, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED
  .byte $FD, $20, $6D, $DE, $20, $09, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD
  .byte $20, $6D, $DE, $20, $0C, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20
  .byte $18, $A0, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $15, $A0, $4C, $12
  .byte $C8, $20, $3E, $DF, $20, $12, $A0, $4C, $12, $C8, $20, $3E, $DF, $A9, $00, $85
  .byte $D6, $20, $ED, $FD, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $A9, $00, $85
  .byte $9D, $85, $A5, $20, $06, $B6, $20, $12, $B6, $20, $39, $B6, $4C, $12, $C8, $20
  .byte $3E, $DF, $20, $ED, $FD, $20, $24, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED
  .byte $FD, $20, $27, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $A9, $00, $8D
  .byte $C5, $07, $A5, $40, $8D, $05, $02, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD
  .byte $A9, $20, $8D, $C5, $07, $A9, $01, $8D, $05, $02, $4C, $12, $C8, $20, $2A, $E2
  .byte $20, $ED, $FD, $20, $13, $E2, $A9, $5B, $20, $3A, $E2, $A5, $4A, $C9, $EA, $F0
  .byte $06, $20, $34, $FD, $4C, $10, $FD, $20, $42, $FD, $A9, $20, $20, $3A, $E2, $AD
  .byte $C5, $07, $F0, $06, $20, $42, $FD, $4C, $23, $FD, $20, $34, $FD, $A9, $00, $20
  .byte $3A, $E2, $4C, $18, $C8, $54, $52, $55, $45, $46, $41, $4C, $53, $45, $A2, $FF
  .byte $E8, $BD, $2B, $FD, $20, $3A, $E2, $E0, $03, $D0, $F5, $60, $A2, $FF, $E8, $BD
  .byte $2F, $FD, $20, $3A, $E2, $E0, $04, $D0, $F5, $60, $20, $3E, $DF, $20, $ED, $FD
  .byte $20, $2A, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $2D, $B6, $4C
  .byte $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $6D, $DE, $20, $33, $B6, $4C, $12
  .byte $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $0F, $A0, $4C, $12, $C8, $20, $3E, $DF
  .byte $20, $09, $A0, $4C, $12, $C8, $20, $3E, $DF, $A9, $00, $85, $D6, $20, $09, $A0
  .byte $20, $12, $A0, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $6D, $DE, $20
  .byte $3F, $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $6D, $DE, $20, $42
  .byte $B6, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $6D, $DE, $20, $3C, $B6
  .byte $4C, $12, $C8, $20, $ED, $FD, $20, $18, $B6, $20, $4B, $DF, $4C, $15, $C8, $20
  .byte $ED, $FD, $20, $1B, $B6, $20, $4B, $DF, $4C, $15, $C8, $20, $ED, $FD, $20, $1E
  .byte $B6, $20, $4B, $DF, $4C, $15, $C8, $A5, $D6, $D0, $22, $20, $2D, $A0, $20, $0F
  .byte $B6, $20, $12, $B6, $20, $15, $B6, $20, $21, $B6, $20, $30, $B6, $20, $36, $B6
  .byte $20, $18, $A0, $20, $0F, $A0, $20, $39, $B6, $A9, $FF, $85, $D6, $60, $7B, $0E
  .byte $FA, $35, $12, $86, $65, $2E, $E0, $03, $A0, $FE, $A9, $14, $20, $00, $F0, $60
  .byte $A0, $FE, $A9, $19, $20, $00, $F0, $60, $4C, $84, $E3, $4C, $61, $E3, $20, $3E
  .byte $DF, $A0, $01, $B1, $B8, $C9, $FF, $D0, $F2, $20, $B1, $00, $20, $B1, $00, $A5
  .byte $B8, $85, $12, $A5, $B9, $85, $13, $20, $39, $C8, $D0, $DC, $A0, $00, $A9, $00
  .byte $91, $D4, $20, $4E, $C8, $4C, $09, $C8, $A9, $00, $85, $69, $A9, $6B, $85, $6A
  .byte $A9, $00, $85, $67, $A9, $6B, $85, $68, $A9, $00, $A0, $08, $91, $67, $88, $10
  .byte $FB, $60, $A9, $00, $85, $00, $85, $14, $A9, $70, $85, $01, $85, $15, $A0, $08
  .byte $A9, $00, $91, $00, $88, $10, $FB, $60, $4C, $61, $E3, $A5, $A2, $30, $F9, $A5
  .byte $9D, $C9, $85, $B0, $F3, $20, $5A, $F0, $60, $A5, $A2, $30, $EB, $A5, $9D, $C9
  .byte $83, $B0, $E5, $20, $5A, $F0, $60, $20, $9F, $FE, $E0, $00, $F0, $DA, $60, $A5
  .byte $A2, $30, $D5, $A5, $9D, $C9, $82, $B0, $CF, $20, $5A, $F0, $E0, $00, $F0, $03
  .byte $A2, $20, $60, $A2, $10, $60, $20, $3E, $DF, $20, $ED, $FD, $20, $AC, $DE, $20
  .byte $B5, $FE, $8A, $48, $20, $3F, $F0, $20, $9F, $FE, $68, $20, $1B, $99, $20, $2A
  .byte $A0, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $AC, $DE, $20, $91, $FE
  .byte $8A, $48, $20, $3F, $F0, $20, $AD, $FE, $68, $20, $25, $99, $95, $3B, $20, $2A
  .byte $A0, $4C, $12, $C8, $20, $3E, $DF, $20, $ED, $FD, $20, $6D, $DE, $20, $91, $FE
  .byte $8A, $0D, $D0, $07, $85, $3B, $20, $2A, $A0, $4C, $12, $C8, $20, $3E, $DF, $20
  .byte $ED, $FD, $20, $6D, $DE, $20, $9F, $FE, $86, $3A, $4C, $12, $C8, $90, $00, $00
  .byte $00, $00, $4C, $61, $E3, $A5, $51, $29, $7F, $A4, $50, $20, $54, $F0, $A5, $9F
  .byte $85, $51, $A5, $A0, $85, $50, $A0, $01, $A2, $00, $20, $3C, $F0, $A0, $FF, $EA
  .byte $A9, $33, $EA, $20, $30, $F0, $A0, $01, $A9, $00, $20, $06, $F0, $A0, $01, $A2
  .byte $00, $20, $3C, $F0, $20, $6D, $DE, $A5, $9D, $C9, $90, $B0, $C5, $A0, $01, $A9
  .byte $00, $20, $00, $F0, $20, $1E, $F0, $20, $4B, $DF, $4C, $18, $C8, $22, $54, $52
  .byte $55, $45, $00, $22, $46, $41, $4C, $53, $45, $00, $82, $00, $00, $00, $00, $82
  .byte $40, $00, $00, $00, $85, $60, $00, $00, $00, $85, $70, $00, $00, $00, $4C, $61
  .byte $E3, $20, $3E, $DF, $20, $AC, $DE, $A0, $01, $A2, $00, $20, $3C, $F0, $20, $3F
  .byte $F0, $A0, $FF, $A9, $90, $20, $4E, $F0, $30, $E4, $A0, $FF, $A9, $9F, $20, $4E
  .byte $F0, $10, $DB, $20, $5A, $F0, $8A, $48, $A0, $01, $A9, $00, $20, $30, $F0, $A0
  .byte $FF, $A9, $9A, $20, $4E, $F0, $10, $C6, $A0, $FF, $A9, $95, $20, $4E, $F0, $30
  .byte $BD, $20, $5A, $F0, $86, $7B, $68, $85, $7A, $4C, $12, $C8, $00, $DB, $04, $DB
  .byte $0D, $0C, $57, $52, $F4, $A1, $00, $9B, $00, $9F
