; 模块：bank28
; 块 28

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG28_0000"
; 段 28，bank 28，$8000-$BFFF
  .byte $56, $B7, $AE, $5F, $59, $49, $77, $56
  .byte $49, $7E, $4F, $42, $7F, $68, $75, $4F
  .byte $42, $50, $4F, $77, $42, $52, $51, $FF
  .byte $57, $5F, $5F, $5D, $5F, $5D, $5E, $5B
  .byte $5C, $5F, $D7, $23, $AF, $BF, $9F, $9F
  .byte $DF, $DF, $DE, $5F, $57, $AF, $5F, $5A
  .byte $5F, $5C, $5E, $5F, $5A, $5F, $DC, $DF
  .byte $DC, $E0, $5F, $5D, $5F, $5C, $9F, $5C
  .byte $A0, $5F, $58, $5F, $DE, $A0, $57, $5F
  .byte $58, $5C, $DE, $A0, $57, $5C, $D7, $DE
  .byte $DC, $D8, $D8, $D0, $C0, $C0, $A0, $5F
  .byte $DE, $AF, $5C, $BF, $5D, $9F, $DD, $DF
  .byte $A0, $DE, $AF, $5C, $BF, $5D, $9F, $5D
  .byte $DF, $DE, $9F, $5D, $5F, $DE, $A0, $5B
  .byte $5F, $DE, $9F, $58, $5F, $DE, $5C, $5B
  .byte $20, $5D, $5E, $DD, $A0, $5C, $5B, $20
  .byte $5C, $5E, $5C, $A0, $DE, $AF, $5C, $DF
  .byte $5B, $A0, $DE, $AF, $5B, $DF, $DD, $A0
  .byte $AF, $5A, $9F, $DE, $A0, $DD, $A0, $AF
  .byte $59, $9F, $5A, $5C, $DC, $58, $40, $60
  .byte $5F, $D7, $A0, $A0, $A1, $A3, $A7, $AF
  .byte $BF, $9F, $5F, $DE, $DF, $58, $5F, $5F
  .byte $5B, $5E, $DE, $5C, $5C, $20, $5F, $5D
  .byte $DF, $DD, $DC, $E0, $5C, $A0, $DE, $AF
  .byte $5F, $DE, $9F, $5B, $A1, $DC, $AF, $9F
  .byte $9F, $5F, $5C, $5F, $DE, $A0, $5B, $5F
  .byte $57, $5F, $D7, $5F, $5E, $58, $A0, $60
  .byte $60, $20, $A0, $D7, $5F, $5E, $58, $40
  .byte $60, $60, $20, $A0, $DE, $20, $5C, $A0
  .byte $DB, $A4, $AC, $BC, $DC, $5F, $5D, $5F
  .byte $D9, $5E, $5C, $58, $50, $40, $60, $5F
  .byte $5D, $5F, $DE, $DF, $5D, $BF, $5C, $AF
  .byte $5F, $57, $5F, $5F, $57, $5F, $5F, $57
  .byte $5F, $5F, $57, $5F, $5F, $57, $5F, $5F
  .byte $5C, $DF, $DE, $A0, $5B, $DF, $57, $DF
  .byte $DB, $20, $5E, $5E, $A0, $5B, $5F, $DE
  .byte $20, $5D, $5E, $5A, $5F, $5C, $DF, $DE
  .byte $A0, $5B, $5F, $5C, $DF, $5A, $5F, $DE
  .byte $5F, $5C, $5E, $5D, $5C, $DD, $58, $A0
  .byte $DE, $5F, $5C, $5E, $5D, $5C, $5D, $58
  .byte $D7, $A1, $A3, $A7, $AF, $BF, $9F, $9F
  .byte $A0, $D7, $A1, $A3, $A7, $AF, $BF, $9F
  .byte $9F, $DF, $58, $DF, $DE, $A0, $57, $DF
  .byte $57, $58, $57, $58, $DE, $DF, $5C, $5F
  .byte $DE, $5E, $5C, $5C, $DE, $DF, $5C, $5F
  .byte $DE, $5E, $5C, $5C, $D7, $50, $60, $2C
  .byte $BC, $9C, $DC, $5C, $5C, $D7, $50, $60
  .byte $2C, $BC, $9C, $DC, $5C, $5C, $DC, $A3
  .byte $A0, $DE, $5A, $DF, $DC, $A3, $A0, $DE
  .byte $5A, $DF, $D7, $5F, $DF, $BF, $AF, $27
  .byte $27, $23, $61, $D7, $5F, $DF, $BF, $AF
  .byte $27, $27, $23, $61, $5C, $58, $DA, $A0
  .byte $5C, $5C, $5E, $5F, $5B, $58, $5D, $5C
  .byte $DD, $5E, $5F, $5C, $59, $DA, $A0, $DD
  .byte $9F, $BF, $2F, $5B, $59, $DB, $DD, $9F
  .byte $BF, $2F, $5C, $5C, $DE, $A0, $5B, $5C
  .byte $57, $5C, $5C, $DF, $DE, $A0, $5B, $DF
  .byte $57, $DF, $5C, $61, $DE, $A0, $5C, $61
  .byte $DE, $21, $58, $61, $DE, $21, $D7, $2F
  .byte $67, $43, $51, $58, $5C, $5F, $A0, $D7
  .byte $2F, $67, $43, $51, $58, $5C, $5F, $5F
  .byte $5B, $5C, $DE, $DC, $5C, $A0, $5B, $5C
  .byte $DB, $DC, $A0, $A0, $5C, $5C, $DF, $DD
  .byte $DE, $DC, $5C, $A0, $5C, $DF, $DD, $DE
  .byte $DC, $5D, $A0, $DE, $DF, $D7, $23, $A7
  .byte $A7, $AF, $BF, $9F, $5F, $A0, $D7, $23
  .byte $A7, $A7, $AF, $BF, $9F, $5F, $5F, $58
  .byte $5F, $DE, $5E, $58, $5F, $DE, $5E, $5C
  .byte $5F, $DA, $40, $60, $20, $A0, $A0, $5C
  .byte $5F, $DC, $40, $60, $20, $5D, $A0, $5B
  .byte $5C, $DE, $DC, $5D, $9C, $DE, $BC, $5B
  .byte $5C, $DE, $DC, $5D, $9C, $DE, $BC, $57
  .byte $DF, $57, $DF, $DE, $5E, $58, $5C, $DE
  .byte $5E, $58, $5C, $57, $A0, $57, $A0, $DE
  .byte $BC, $5A, $AC, $5D, $BC, $DE, $BC, $5A
  .byte $AC, $5D, $BC, $5D, $5C, $5D, $5E, $5B
  .byte $5F, $5D, $5C, $5D, $5E, $5B, $5F, $D7
  .byte $E0, $C0, $90, $9F, $AF, $20, $50, $5F
  .byte $D7, $E0, $C0, $90, $9F, $AF, $20, $50
  .byte $5F, $D7, $9C, $DC, $58, $51, $41, $A7
  .byte $BF, $5F, $D7, $9C, $DC, $58, $51, $41
  .byte $A7, $BF, $5F, $5D, $DF, $59, $5F, $5D
  .byte $DF, $59, $5F, $57, $5F, $DD, $4F, $47
  .byte $5C, $67, $DC, $61, $60, $6C, $57, $5F
  .byte $58, $5F, $DE, $DF, $57, $5F, $58, $5F
  .byte $DE, $5E, $57, $5F, $DE, $2E, $5C, $3F
  .byte $5D, $9F, $DD, $90, $E0, $57, $5F, $D7
  .byte $9F, $BF, $2F, $27, $63, $41, $40, $50
  .byte $57, $5F, $58, $5F, $DE, $9F, $57, $5F
  .byte $5C, $5C, $5D, $59, $DC, $52, $42, $47
  .byte $57, $5F, $D7, $27, $37, $53, $53, $50
  .byte $A3, $A3, $93, $57, $5F, $5D, $58, $DE
  .byte $3C, $5D, $AE, $5C, $5F, $57, $5F, $DA
  .byte $BF, $A1, $A1, $AF, $9F, $5C, $5F, $57
  .byte $5F, $DC, $6F, $7F, $1F, $5A, $5F, $57
  .byte $5F, $D7, $53, $53, $D3, $93, $B3, $B3
  .byte $92, $33, $57, $5F, $DD, $DF, $9F, $5C
  .byte $BF, $DD, $9F, $DF, $53, $5F, $DC, $58
  .byte $50, $58, $55, $5F, $5D, $53, $DE, $50
  .byte $5D, $A0, $DD, $BF, $DF, $55, $5F, $DC
  .byte $58, $A0, $A0, $51, $5F, $DE, $DF, $4D
  .byte $5F, $DC, $47, $53, $53, $57, $5F, $DE
  .byte $59, $5A, $58, $DD, $40, $20, $53, $5F
  .byte $DB, $58, $40, $A0, $A7, $53, $5F, $DB
  .byte $9F, $BF, $AF, $27, $57, $5F, $DD, $52
  .byte $53, $5C, $43, $DD, $53, $5B, $56, $5F
  .byte $DD, $A0, $5D, $5C, $5F, $DC, $5E, $5C
  .byte $50, $56, $5F, $D8, $5E, $5E, $5F, $3F
  .byte $AF, $A7, $67, $57, $5F, $5D, $A7, $51
  .byte $5F, $58, $5F, $DE, $5C, $57, $5F, $D7
  .byte $21, $63, $5F, $5F, $5E, $58, $60, $A6
  .byte $57, $5F, $D8, $2F, $BF, $9F, $DF, $58
  .byte $A0, $A0, $52, $5F, $DC, $9F, $BF, $BF
  .byte $56, $5F, $5D, $5E, $51, $5F, $DD, $BE
  .byte $DF, $51, $5F, $DE, $DF, $58, $9F, $55
  .byte $5F, $DA, $4E, $44, $40, $50, $58, $56
  .byte $5F, $5A, $9F, $DE, $DF, $50, $5F, $DC
  .byte $57, $43, $53, $51, $5F, $DD, $47, $43
  .byte $55, $5F, $DE, $47, $5D, $43, $5C, $67
  .byte $57, $5F, $57, $53, $57, $5F, $D7, $43
  .byte $53, $5B, $5F, $58, $40, $63, $DF, $57
  .byte $5F, $5D, $6F, $D9, $5F, $40, $A0, $AF
  .byte $5F, $AF, $57, $5F, $5D, $20, $DD, $63
  .byte $73, $5B, $53, $57, $5F, $DE, $DE, $5D
  .byte $5C, $5C, $59, $5D, $53, $57, $5F, $D7
  .byte $C7, $53, $53, $59, $5C, $5C, $9E, $9F
  .byte $52, $5F, $DC, $DF, $BF, $AF, $57, $5F
  .byte $5D, $53, $D9, $52, $50, $50, $51, $41
  .byte $63, $57, $5F, $D7, $47, $87, $EF, $3F
  .byte $32, $92, $53, $53, $57, $5F, $D7, $BF
  .byte $2F, $51, $60, $AC, $9C, $5C, $59, $57
  .byte $5F, $DC, $A7, $A1, $20, $52, $5F, $DD
  .byte $27, $2F, $5D, $6F, $53, $5F, $DE, $53
  .byte $5D, $59, $5C, $5C, $DE, $5E, $56, $5F
  .byte $5D, $53, $DC, $41, $A0, $DF, $58, $5F
  .byte $DE, $20, $54, $5F, $DD, $4F, $43, $5D
  .byte $47, $DE, $A0, $5C, $47, $55, $5F, $DC
  .byte $57, $43, $A1, $5D, $5F, $DE, $6F, $57
  .byte $5F, $DB, $40, $5F, $57, $50, $5C, $53
  .byte $DE, $50, $57, $5F, $DB, $A0, $5F, $5F
  .byte $A0, $5C, $47, $DE, $A0, $57, $5F, $DB
  .byte $A7, $5F, $6F, $A7, $5C, $6F, $DE, $AF
  .byte $57, $5F, $D8, $53, $53, $47, $47, $4F
  .byte $7F, $1F, $56, $5F, $DD, $6F, $7F, $51
  .byte $5F, $DC, $5D, $5C, $5C, $5C, $59, $DD
  .byte $53, $52, $57, $5F, $D7, $53, $D3, $40
  .byte $6F, $3F, $20, $BC, $3C, $56, $5F, $D8
  .byte $3F, $AF, $9F, $C7, $A3, $47, $47, $57
  .byte $5F, $5D, $43, $5D, $73, $DB, $13, $53
  .byte $52, $53, $57, $5F, $D7, $3C, $20, $59
  .byte $43, $69, $38, $D2, $4C, $57, $5F, $D7
  .byte $47, $A7, $5F, $4F, $67, $7F, $9F, $FF
  .byte $57, $5F, $D7, $53, $52, $53, $53, $52
  .byte $53, $53, $57, $57, $5F, $D7, $B8, $52
  .byte $46, $3E, $DC, $40, $59, $5B, $57, $5F
  .byte $D9, $FF, $EF, $C7, $C3, $D0, $5B, $55
  .byte $5F, $DA, $7F, $63, $6F, $6F, $60, $5C
  .byte $6F, $54, $5F, $DD, $6F, $A7, $53, $5F
  .byte $DE, $20, $51, $5F, $D7, $6F, $A0, $7F
  .byte $67, $6F, $68, $6E, $6F, $57, $5F, $DD
  .byte $53, $A1, $5B, $5F, $DD, $9F, $BF, $57
  .byte $5F, $58, $6F, $DE, $7F, $57, $5F, $DC
  .byte $2F, $2F, $FD, $50, $5F, $DE, $A0, $5A
  .byte $2F, $55, $5F, $D9, $9E, $BF, $27, $67
  .byte $63, $63, $55, $5F, $D9, $AF, $BF, $2F
  .byte $2F, $67, $67, $55, $5F, $D9, $23, $67
  .byte $2F, $2F, $BF, $BF, $55, $5F, $DD, $A0
  .byte $BE, $5B, $BF, $55, $5F, $DB, $DF, $BF
  .byte $AF, $2F, $5D, $27, $55, $5F, $D9, $50
  .byte $40, $40, $66, $66, $6F, $55, $5F, $DE
  .byte $5E, $5D, $DF, $5C, $9F, $55, $5F, $D9
  .byte $BF, $AF, $A7, $A3, $B3, $B3, $55, $5F
  .byte $D9, $58, $50, $40, $60, $68, $68, $55
  .byte $5F, $DE, $D0, $5A, $5C, $55, $5F, $DE
  .byte $BE, $5A, $DF, $55, $5F, $5D, $A0, $5B
  .byte $AF, $55, $5F, $D9, $DF, $BF, $AF, $2F
  .byte $27, $27, $57, $5F, $5C, $2F, $DE, $20
  .byte $5B, $2F, $57, $5F, $DB, $67, $27, $BF
  .byte $9F, $53, $5F, $DC, $47, $42, $50, $5A
  .byte $58, $57, $5F, $DC, $9F, $9F, $DF, $52
  .byte $5F, $D7, $BF, $BF, $BE, $B0, $B1, $B8
  .byte $BC, $BE, $57, $5F, $D7, $2F, $AF, $BF
  .byte $DF, $5F, $5F, $DF, $9E, $57, $5F, $5D
  .byte $2F, $5D, $20, $DE, $A0, $5C, $BF, $57
  .byte $5F, $5C, $BF, $5D, $AF, $DC, $2F, $27
  .byte $67, $57, $5F, $5D, $B9, $5D, $BC, $5D
  .byte $BE, $5D, $BF, $57, $5F, $5D, $38, $5D
  .byte $98, $5D, $D8, $5D, $58, $57, $5F, $57
  .byte $5C, $57, $5F, $57, $DF, $57, $5F, $57
  .byte $AF, $57, $5F, $58, $27, $DE, $2F, $57
  .byte $5F, $DC, $2F, $2F, $A7, $52, $5F, $DC
  .byte $58, $58, $50, $50, $5F, $DE, $DF, $52
  .byte $5F, $DC, $BF, $BF, $AF, $52, $5F, $DC
  .byte $BE, $2E, $26, $52, $5F, $5C, $9F, $52
  .byte $5F, $DC, $67, $67, $66, $52, $5F, $DC
  .byte $58, $58, $50, $52, $5F, $DC, $5C, $5C
  .byte $D0, $52, $5F, $DC, $DF, $DF, $BE, $52
  .byte $5F, $DC, $AF, $A0, $A0, $52, $5F, $DC
  .byte $AF, $BF, $DF, $4F, $5F, $DE, $60, $5B
  .byte $7F, $57, $5F, $5D, $DF, $DD, $DD, $A0
  .byte $5B, $DD, $56, $5F, $D8, $60, $7F, $7F
  .byte $70, $7F, $7F, $78, $57, $5F, $D7, $5D
  .byte $A0, $5D, $4D, $A5, $DD, $DD, $AD, $56
  .byte $5F, $D8, $7C, $45, $53, $1F, $6E, $4F
  .byte $57, $56, $5F, $D8, $A0, $5E, $0D, $D7
  .byte $7B, $7F, $7D, $57, $5F, $5B, $57, $DD
  .byte $55, $20, $5D, $57, $57, $5F, $DD, $5D
  .byte $DD, $59, $CD, $57, $5F, $5C, $5E, $DA
  .byte $20, $5D, $5B, $5B, $57, $55, $5F, $D9
  .byte $5D, $A0, $5F, $DF, $DF, $D7, $56, $5F
  .byte $D8, $7F, $4F, $4F, $5F, $58, $2F, $4F
  .byte $57, $5F, $5B, $1F, $DB, $1B, $A1, $1F
  .byte $DF, $57, $5F, $5B, $5E, $DB, $60, $7E
  .byte $7E, $60, $54, $5F, $DA, $57, $A3, $57
  .byte $57, $A7, $57, $5F, $D7, $5E, $40, $4E
  .byte $4E, $40, $4E, $5F, $20, $57, $5F, $D7
  .byte $5B, $A1, $1B, $1B, $23, $8B, $D5, $A0
  .byte $57, $5F, $5D, $5E, $D9, $20, $5D, $5D
  .byte $58, $5B, $53, $56, $5F, $D8, $5D, $A0
  .byte $5F, $57, $A3, $57, $57, $56, $5F, $D8
  .byte $61, $7D, $7B, $7B, $77, $7B, $7B, $57
  .byte $5F, $D7, $5B, $A1, $DB, $DB, $A3, $DB
  .byte $DB, $A3, $55, $5F, $5D, $5D, $5D, $5B
  .byte $DD, $57, $4F, $57, $5F, $D7, $1F, $3F
  .byte $1F, $7F, $7F, $4F, $D7, $D8, $56, $5F
  .byte $D8, $40, $5F, $5F, $20, $5F, $5F, $40
  .byte $57, $5F, $D7, $5B, $A1, $5B, $7B, $AB
  .byte $5B, $1B, $BB, $57, $5F, $DD, $60, $7F
  .byte $51, $5F, $DD, $A1, $DD, $59, $DF, $57
  .byte $5F, $5C, $7F, $DA, $70, $7F, $7F, $60
  .byte $7F, $57, $5F, $D7, $DD, $FD, $CD, $A5
  .byte $5D, $5D, $A1, $5D, $57, $5F, $DA, $54
  .byte $4F, $4F, $3E, $79, $5C, $7F, $57, $5F
  .byte $DA, $A0, $2F, $F7, $78, $7D, $5C, $7F
  .byte $57, $5F, $D8, $57, $57, $51, $27, $7E
  .byte $5E, $5D, $56, $5F, $5B, $CD, $DE, $4D
  .byte $5C, $5D, $57, $5F, $D7, $40, $5F, $5B
  .byte $5B, $57, $4F, $7D, $5E, $57, $5F, $D8
  .byte $A3, $DF, $FF, $CF, $D7, $D3, $DB, $56
  .byte $5F, $D8, $4F, $4E, $4E, $4D, $4B, $77
  .byte $18, $56, $5F, $D8, $FF, $4F, $57, $53
  .byte $5B, $5F, $A0, $56, $5F, $DB, $7E, $7E
  .byte $60, $7E, $5C, $5E, $56, $5F, $5D, $57
  .byte $DD, $A7, $57, $5C, $5D, $DE, $A1, $57
  .byte $5F, $D7, $5E, $59, $47, $20, $4E, $4E
  .byte $40, $4E, $57, $5F, $D7, $1F, $6F, $53
  .byte $20, $1B, $1B, $23, $1B, $57, $5F, $DB
  .byte $48, $7B, $1B, $58, $5B, $5B, $57, $5F
  .byte $D7, $A7, $57, $57, $A7, $57, $57, $77
  .byte $4F, $57, $5F, $5C, $7D, $DD, $6B, $77
  .byte $5C, $7F, $57, $5F, $D7, $DD, $9B, $F7
  .byte $CF, $D7, $F8, $9D, $DF, $57, $5F, $D8
  .byte $3E, $5E, $5D, $5B, $57, $40, $57, $56
  .byte $5F, $DE, $5D, $5D, $5F, $DB, $7F, $4F
  .byte $A7, $57, $56, $5F, $5B, $4F, $DD, $40
  .byte $4F, $55, $5F, $5B, $1B, $DB, $9B, $1B
  .byte $4B, $57, $57, $5F, $57, $5F, $5F, $57
  .byte $5F, $5F, $57, $5F, $5F, $57, $5F, $5F
  .byte $57, $5F, $5F, $57, $5F, $5F, $57, $5F
  .byte $5F, $57, $5F, $5F, $57, $5F, $57, $5F
  .byte $53, $DE, $9F, $59, $AF, $DD, $5F, $93
  .byte $59, $A0, $DD, $A0, $A0, $5C, $A0, $DA
  .byte $A0, $A0, $A0, $A0, $F5, $5C, $F5, $DA
  .byte $0A, $0A, $0A, $00, $D7, $5C, $F5, $DB
  .byte $5D, $0F, $0A, $0A, $5A, $5F, $DC, $5F
  .byte $5F, $5F, $5B, $5F, $DB, $5F, $5F, $5F
  .byte $5F, $5B, $5F, $DE, $5E, $5C, $5A, $5D
  .byte $5E, $59, $5F, $DC, $45, $44, $5E, $5A
  .byte $5F, $D7, $5C, $5B, $5A, $5F, $59, $58
  .byte $57, $5E, $5C, $5F, $D5, $56, $56, $55
  .byte $54, $53, $56, $51, $50, $4F, $5E, $5A
  .byte $5F, $D8, $4E, $4D, $5F, $5F, $4B, $4A
  .byte $49, $5B, $5F, $DD, $1F, $1E, $5D, $5F
  .byte $DB, $0E, $0D, $0C, $0B, $DA, $5F, $3D
  .byte $3C, $3B, $5E, $5C, $5F, $D6, $48, $47
  .byte $46, $7E, $48, $48, $7D, $7C, $52, $5C
  .byte $5F, $D1, $1D, $1C, $1B, $1A, $5F, $0A
  .byte $09, $08, $07, $5F, $3A, $39, $38, $5E
  .byte $5C, $5F, $DB, $7B, $7A, $55, $79, $5A
  .byte $56, $5C, $5F, $D0, $19, $18, $17, $16
  .byte $5F, $06, $05, $04, $03, $5F, $37, $36
  .byte $35, $34, $5E, $5C, $5F, $DA, $78, $77
  .byte $76, $75, $74, $58, $5F, $DC, $15, $14
  .byte $13, $5D, $5F, $D5, $02, $01, $00, $5F
  .byte $5F, $33, $32, $31, $30, $5E, $5C, $5F
  .byte $DA, $73, $72, $71, $70, $6F, $5B, $48
  .byte $5C, $5F, $D1, $12, $11, $10, $0F, $5F
  .byte $5F, $3F, $3E, $5F, $5F, $2F, $2E, $2D
  .byte $5E, $5C, $5F, $DA, $56, $6E, $6D, $6C
  .byte $6B, $5B, $56, $DE, $5E, $5C, $5F, $DA
  .byte $6A, $69, $68, $67, $5E, $5C, $5F, $DA
  .byte $66, $65, $64, $67, $5E, $5C, $5F, $DB
  .byte $63, $62, $61, $60, $54, $5F, $D3, $2C
  .byte $2B, $2A, $5F, $24, $23, $22, $5F, $5F
  .byte $DB, $DA, $5E, $4D, $5F, $D3, $29, $28
  .byte $27, $5F, $21, $20, $DF, $5F, $D9, $D8
  .byte $D7, $5E, $5D, $5F, $D1, $D4, $D3, $D2
  .byte $D1, $D0, $CF, $CE, $CD, $CC, $CB, $CA
  .byte $C9, $C8, $C7, $5D, $5F, $D8, $26, $5F
  .byte $25, $5F, $DE, $DD, $DC, $5D, $5F, $DC
  .byte $D6, $D5, $5E, $5D, $5F, $D0, $C6, $C5
  .byte $C4, $C3, $C2, $C1, $C0, $FF, $FE, $FD
  .byte $FC, $FB, $FA, $F9, $5E, $5D, $5F, $D0
  .byte $F8, $5F, $F7, $F6, $F5, $F4, $F3, $F2
  .byte $F5, $F1, $F0, $EF, $EE, $ED, $5E, $DE
  .byte $5E, $DE, $5E, $DE, $5E, $DE, $5E, $DE
  .byte $5E, $DE, $5E, $5B, $5F, $C6, $EC, $EB
  .byte $EA, $E9, $E8, $E7, $E6, $E5, $E4, $E3
  .byte $E2, $E1, $E0, $9F, $9E, $9D, $9C, $9B
  .byte $9A, $99, $98, $97, $96, $95, $5E, $5B
  .byte $5F, $C5, $94, $93, $92, $91, $90, $8F
  .byte $8E, $8D, $8C, $8B, $8A, $89, $88, $87
  .byte $86, $85, $84, $83, $82, $81, $80, $BF
  .byte $BE, $BD, $5E, $5E, $A0, $A0
  .res 836, $50
  .byte $FD
  .res 749, $50
  .byte $14, $DE

sub_b28_9002:
  pha
  lda var_0001
  beq sub_b28_9009
  pla
  rti

sub_b28_9009:
  txa
  pha
  tya
  pha
  php
  lda #$00
  sta var_2003
  lda #$02
  sta var_4014
  lda var_2002
  jsr sub_b31_D196
  lda var_004D
  bmi sub_b28_9025
  jsr sub_b31_D0D1

sub_b28_9025:
  lda var_0050
  ora #$B0
  sta var_2000
  lda var_004E
  sta var_2005
  lda var_004F
  sta var_2005
  lda #$1E
  sta var_2001
  jsr sub_b31_D132
  inc var_000B
  lda var_000B
  and #$3F
  bne sub_b28_9048
  inc var_000A

sub_b28_9048:
  plp
  pla
  tay
  pla
  tax
  pla
  rti
  .byte $AA, $68, $40

sub_b28_9052:
  jsr sub_b31_D3C8
  jsr sub_b31_D390
  lda #$10
  sei
  sta var_2000
  cld
  ldx #$02

sub_b28_9061:
  lda var_2002
  bpl sub_b28_9061
  lda #$06
  sta var_2001
  dex
  bne sub_b28_9061
  jsr sub_b31_D083
  lda #$05
  sta var_0073
  lda #$00
  sta var_004E
  sta var_004F
  sta var_0050
  jsr sub_b31_D178
  jmp loc_b31_D45A

sub_b28_9083:
  lda #$00
  sta var_000C
  sta var_000D
  sta var_006D
  lda #$04
  sta var_000E
  lda #$FF
  sta var_004D
  jsr sub_b31_D0AE
  jsr sub_b31_D11F
  jsr sub_b31_D132
  lda #$1C
  sta var_0005
  jsr sub_b31_D14E
  lda #$24
  sta var_0005
  jsr sub_b31_D14E
  jsr sub_b31_DDE7
  rts

sub_b28_90AE:
  jsr sub_b31_D172
  jsr sub_b31_D0BA
  lda #$00
  jsr sub_b31_D0D1
  rts

sub_b28_90BA:
  ldx #$00
  ldy #$20
  lda #$3F
  sta var_2006
  stx var_2006

sub_b28_90C6:
  lda data_b29_7F80,x
  sta var_2007
  inx
  dey
  bne sub_b28_90C6
  rts

sub_b28_90D1:
  cmp #$08
  bne sub_b28_90EF
  lda #$3F
  sta var_2006
  ldx #$00
  stx var_2006
  ldy #$10
  ldx #$00

sub_b28_90E3:
  lda var_00F0,x
  sta var_2007
  inx
  dey
  bne sub_b28_90E3
  jmp loc_b31_D10A

sub_b28_90EF:
  asl a
  asl a
  asl a
  asl a
  tax
  ldy #$10
  lda #$3F
  sta var_2006
  lda #$00
  sta var_2006

sub_b28_9100:
  lda var_6005,x
  sta var_2007
  inx
  dey
  bne sub_b28_9100

sub_b28_910A:
  lda #$FF
  sta var_004D
  lda #$3F
  sta var_2006
  lda #$00
  sta var_2006
  sta var_2006
  sta var_2006
  rts

sub_b28_911F:
  lda #$00
  tax

sub_b28_9122:
  sta var_0400,x
  sta var_0500,x
  sta var_0600,x
  sta var_0700,x
  inx
  bne sub_b28_9122
  rts

sub_b28_9132:
  ldx var_000D
  lda var_000E
  eor #$FF
  clc
  adc #$01
  sta var_000E

sub_b28_913D:
  txa
  clc
  adc var_000E
  tax
  lda #$F0
  sta var_0200,x
  cpx #$04
  bne sub_b28_913D
  stx var_000D
  rts

sub_b28_914E:
  lda #$04
  sta var_0012
  clc
  adc var_0005
  sta var_2006
  lda #$00
  sta var_0011
  tay
  sta var_2006

sub_b28_9160:
  lda (var_0011),y
  sta var_2007
  inc var_0011
  bne sub_b28_9160
  inc var_0012
  lda var_0012
  cmp #$08
  bne sub_b28_9160
  rts

sub_b28_9172:
  lda var_2002
  bpl sub_b28_9172
  rts

sub_b28_9178:
  jsr sub_b31_D172
  lda #$B0
  sta var_2000
  rts

sub_b28_9181:
  jsr sub_b31_D18F
  lda #$10
  sta var_2000
  lda #$06
  sta var_2001
  rts

sub_b28_918F:
  lda var_000B

sub_b28_9191:
  cmp var_000B
  beq sub_b28_9191
  rts

sub_b28_9196:
  ldx var_000C
  lda #$00
  sta var_0180,x
  tax
loc_b28_919E:
  cpx var_000C
  beq sub_b28_91C8
  lda var_0180,x
  inx
  sta var_2006
  lda var_0180,x
  inx
  sta var_2006
  lda var_0180,x
  inx
  cmp #$FF
  bne loc_b28_91C2
  lda var_0180,x
  cmp #$FF
  bne loc_b28_919E
  lda var_017F,x
loc_b28_91C2:
  sta var_2007
  jmp loc_b31_D1B0

sub_b28_91C8:
  lda #$00
  sta var_000C
  rts
  .byte $A2, $01, $8E, $16, $40, $A0, $00, $8C, $16, $40, $84, $DF, $A0, $08, $BD, $16
  .byte $40, $29, $03, $C9, $01, $66, $DF, $88, $D0, $F4, $B5, $06, $49, $FF, $25, $DF
  .byte $95, $08, $A5, $DF, $95, $06, $CA, $10, $E1, $60, $20, $81, $D1, $20, $1F, $D1
  .byte $A9, $1C, $85, $05, $20, $4E, $D1, $20, $78, $D1, $60

sub_b28_9208:
  lda #$00
  sta var_0015
  sta var_0016
  sta var_0072
loc_b28_9210:
  lda var_5500
  eor data_b31_D217
  and #$04
  beq loc_b28_9210

sub_b28_921A:
  inc var_0015
  lda var_5500
  eor data_b31_D217
  and #$04
  bne sub_b28_921A
loc_b28_9226:
  inc var_0016
  lda var_5500
  eor data_b31_D217
  and #$04
  beq loc_b28_9226
  ldx var_0015
  lda var_0016
  sec
  sbc var_0015
  bpl loc_b28_9242
  eor #$FF
  clc
  adc #$01
  ldx var_0016
loc_b28_9242:
  cmp #$03
  bcs sub_b28_9208
  lsr a
  sta var_0016
  txa
  clc
  adc var_0016
  cmp #$14
  bcc sub_b28_9208
  cmp #$28
  bcs sub_b28_9208
  sta var_0072
  lda var_0072
  lsr var_0072
  clc
  adc var_0072
  lsr var_0072
  clc
  adc var_0072
  clc
  adc var_0073
  sta var_0015
  lda #$70
loc_b28_926A:
  lda var_5500
  eor data_b31_D217
  and #$04
  sta var_0072
loc_b28_9274:
  lda var_5500
  eor data_b31_D217
  and #$04
  cmp var_0072
  beq loc_b28_9274
  ldy var_0015
loc_b28_9282:
  nop
  dey
  bne loc_b28_9282
  lda var_5500
  eor data_b31_D217
  and #$04
  cmp var_0072
  bne loc_b28_926A
  sta var_0071
  ldx #$06
loc_b28_9296:
  lda var_5500
  eor data_b31_D217
  and #$04
  cmp var_0071
  beq loc_b28_9296
  ldy var_0015
loc_b28_92A4:
  nop
  dey
  bne loc_b28_92A4
  lda var_5500
  eor data_b31_D217
  and #$04
  cmp var_0071
  beq loc_b28_92B7
  jmp loc_b31_D26A
loc_b28_92B7:
  dex
  bne loc_b28_9296
  sta var_0072
  rts
  .byte $A9, $80, $85, $74, $A9, $00, $85, $75, $AD, $00, $55, $4D, $17, $D2, $29, $04
  .byte $C5, $72, $F0, $F4, $A4, $15, $EA, $88, $D0, $FC, $A9, $70, $AD, $00, $55, $4D
  .byte $17, $D2, $29, $04, $85, $72, $C5, $71, $D0, $06, $A5, $75, $05, $74, $85, $75
  .byte $46, $74, $90, $D4, $60, $A9, $00, $85, $75, $85, $13, $A9, $80, $85, $74, $AD
  .byte $00, $55, $4D, $17, $D2, $29, $04, $C5, $72, $F0, $F4, $A5, $13, $D0, $07, $A4
  .byte $15, $88, $88, $4C, $67, $D3, $A9, $00, $85, $13, $A0, $00, $A5, $75, $91, $40
  .byte $A5, $90, $D0, $06, $A5, $91, $F0, $2D, $C6, $91, $C6, $90, $A5, $04, $18, $65
  .byte $75, $85, $04, $A9, $00, $85, $75, $E6, $40, $F0, $09, $A5, $15, $38, $E9, $0B
  .byte $A8, $4C, $67, $D3, $E6, $41, $A5, $41, $29, $6F, $85, $41, $A5, $15, $38, $E9
  .byte $0D, $A8, $4C, $67, $D3, $A5, $75, $C5, $04, $F0, $03, $4C, $C8, $D4, $A0, $00
  .byte $A9, $FF, $91, $40, $84, $01, $78, $4C, $0A, $D5, $EA, $88, $D0, $FC, $A9, $70
  .byte $AD, $00, $55, $4D, $17, $D2, $29, $04, $85, $72, $C5, $71, $D0, $06, $A5, $75
  .byte $05, $74, $85, $75, $46, $74, $90, $08, $A9, $80, $85, $74, $A9, $01, $85, $13
  .byte $4C, $FC, $D2

sub_b28_9390:
  lda #$60
  sta var_0001
  lda #$C0
  sta var_0003
  lda #$00
  sta var_0000
  sta var_0002
  tay

sub_b28_939F:
  lda (var_0002),y
  eor #$5F
  nop
  sta (var_0000),y
  iny
  bne sub_b28_939F
  inc var_0003
  inc var_0001
  lda var_0001
  cmp #$70
  bne sub_b28_939F
  rts

sub_b28_93B4:
  nop
  nop
  nop
  lda #$AA
  sta data_b29_7027
  lda #$55
  sta data_b29_7028
  rts
  .byte $20, $90, $D3, $4C, $52, $D0

sub_b28_93C8:
  lda #$00
  sta var_2000
  jmp loc_b31_D3B4
  .byte $A9, $00, $85, $20, $98, $4A, $66, $20, $4A, $66, $20, $4A, $66, $20, $48, $8A
  .byte $05, $20, $A8, $68, $09, $04, $60, $20, $D0, $D3, $85, $14, $18, $65, $05, $A6
  .byte $0C, $9D, $80, $01, $E8, $98, $9D, $80, $01, $E8, $85, $13, $A0, $00, $B1, $11
  .byte $9D, $80, $01, $E8, $C9, $FF, $F0, $06, $91, $13, $C8, $4C, $FE, $D3, $86, $0C
  .byte $60, $86, $47, $84, $48, $A6, $0D, $A5, $48, $9D, $00, $02, $A5, $53, $9D, $01
  .byte $02, $A5, $24, $9D, $02, $02, $A5, $47, $9D, $03, $02, $8A, $18, $65, $0E, $85
  .byte $0D, $60, $85, $53, $86, $54, $84, $55, $8A, $18, $69, $08, $AA, $20, $11, $D4
  .byte $A5, $53, $A6, $54, $A4, $55, $20, $11, $D4, $60
  .byte $00, $04, $06, $7F, $7F, $06, $04, $00
  .byte $00, $04, $06, $7F, $7F, $06, $04, $00

sub_b28_945A:
  lda var_00CE
  cmp #$AA
  beq loc_b28_946A
  lda #$00
  sta var_00CF
  lda #$0A
  sta var_00D0
  sta var_00D1
loc_b28_946A:
  lda #$AA
  sta var_00CE
  lda #$00
  sta var_000F
  sta var_0000
  sta var_0002
  sta var_0003
  lda #$FF
  sta var_0001
  cli
  lda #$00
  nop
  sta var_2000
  sta var_2001
  lda #$10
  sta var_2006
  lda #$00
  sta var_2006
  ldy var_2002
  ldx #$10

sub_b28_9495:
  sta var_2007
  dex
  bne sub_b28_9495

sub_b28_949B:
  lda data_b31_D44A,x
  sta var_2007
  inx
  cpx #$10
  bne sub_b28_949B
  lda #$B2
  sta var_2000
  lda #$1E
  sta var_2001
  lda #$00
  sta var_0004
  sta var_0008
  sta var_0092
  sta var_0011
  lda #$03
  sta var_0042
  lda #$60
  sta var_0043
  sty var_0001
  sei
  jmp loc_b31_D50A

sub_b28_94C8:
  lda #$00
  sta var_0040
  sta var_0042
  sta var_0004
  sta var_0008
  sta var_0092
  sta var_0011
  lda #$60
  sta var_0041
  sta var_0043
  lda #$FF
  sta var_0001
  cli
  jsr sub_b31_D208
  jsr sub_b31_D2BD
  lda var_0075
  and #$1F
  sta var_0091
  clc
  adc var_0011
  sta var_0011
  jsr sub_b31_D2BD
  lda var_0075
  sta var_0090
  clc
  adc var_0011
  sta var_0011
  jsr sub_b31_D2BD
  lda var_0075
  cmp var_0011
  bne sub_b28_94C8
  jmp loc_b31_D2F2

sub_b28_950A:
  ldy #$00
  lda (var_0042),y
  cmp #$1F
  bcs sub_b28_94C8
  asl a
  tax
  lda data_b31_D521,x
  sta var_0011
  lda data_b31_D522,x
  sta var_0012
  jmp (var_0011)
  .byte $40, $DA
  .byte $3E, $DD, $2C, $DD, $1D, $DD, $B4, $DA, $01, $DA
  .byte $A4, $D5
  .byte $81, $D6, $61, $D5, $47, $DA, $F9, $DB, $C7, $DC
  .byte $A1, $D6, $1F, $DB
  .byte $FC, $D5, $38, $DD, $19, $D6, $A7, $D6, $D2, $D6, $4D, $DD, $D0, $DD, $38, $DD
  .byte $38, $DD, $38, $DD, $38, $DD, $38, $DD, $38, $DD, $38, $DD, $38, $DD, $38, $DD
  .byte $38, $DD, $38, $DD, $20, $E0, $DD, $A2, $00, $B1, $42, $85, $40, $20, $E0, $DD
  .byte $B1, $42, $85, $41, $20, $E0, $DD, $B1, $42, $C9, $FF, $F0, $07, $9D, $00, $03
  .byte $E8, $4C, $71, $D5, $9D, $00, $03, $A9, $00, $85, $11, $A9, $03, $85, $12, $A6
  .byte $41, $A4, $40, $20, $E7, $D3, $20, $8F, $D1, $20, $E0, $DD, $A0, $00, $B1, $42
  .byte $C9, $AA, $D0, $C3, $4C, $C8, $D4

sub_b28_95A4:
  jsr sub_b31_D11F
  lda var_0050
  bne sub_b28_95B8
  jsr sub_b31_D181
  lda #$24
  sta var_0005
  jsr sub_b31_D14E
  jsr sub_b31_D178

sub_b28_95B8:
  jsr sub_b31_D18F
  lda var_004F
  cmp #$E8
  beq sub_b28_95C9
  clc
  adc #$08
  sta var_004F
  jmp loc_b31_D5B8

sub_b28_95C9:
  jsr sub_b31_D181
  lda #$1C
  sta var_0005
  jsr sub_b31_D14E
  jsr sub_b31_D178
  lda #$00
  sta var_004F
  sta var_0050
  jsr sub_b31_D18F
  jsr sub_b31_DDE0
  ldx #$00
  ldy #$00

sub_b28_95E6:
  lda (var_0042),y
  sta var_00F0,x
  jsr sub_b31_DDE0
  inx
  cpx #$10
  bne sub_b28_95E6
  lda #$08
  sta var_004D
  jsr sub_b31_D18F
  jmp loc_b31_D50A
  .byte $20, $E0, $DD, $A2, $00, $A0, $00, $B1, $42, $95, $F0, $20, $E0, $DD, $E8, $E0
  .byte $10, $D0, $F4, $A9, $08, $85, $4D, $20, $8F, $D1, $4C, $C8, $D4, $20, $E0, $DD
  .byte $A0, $00, $84, $52, $B1, $42, $85, $51, $20, $E0, $DD, $A5, $42, $85, $A0, $A5
  .byte $43, $85, $A1, $A5, $51, $0A, $0A, $0A, $0A, $A8, $B1, $42, $85, $53, $C8, $B1
  .byte $42, $85, $54, $A9, $00, $85, $0B, $A5, $0B, $C5, $53, $D0, $FA, $A5, $52, $0A
  .byte $0A, $0A, $0A, $A8, $A5, $A0, $85, $42, $A5, $A1, $85, $43, $A2, $00, $B1, $42
  .byte $95, $F0, $C8, $E8, $E0, $10, $D0, $F6, $A9, $08, $85, $4D, $20, $8F, $D1, $E6
  .byte $52, $A5, $52, $C5, $51, $D0, $CC, $C6, $54, $F0, $07, $A9, $00, $85, $52, $4C
  .byte $3F, $D6, $4C, $C8, $D4, $20, $E0, $DD, $20, $8F, $D1, $A9, $3F, $8D, $06, $20
  .byte $A2, $10, $8E, $06, $20, $A0, $00, $B1, $42, $8D, $07, $20, $20, $E0, $DD, $CA
  .byte $D0, $F5, $4C, $0A, $D5

sub_b28_96A1:
  jsr sub_b31_D8F9
  jmp loc_b31_D4C8
  .byte $20, $F9, $D8, $A2, $00, $A9, $00, $85, $DA, $20, $54, $D8, $A2, $00, $A0, $00
  .byte $84, $D7, $C5, $D2, $D0, $04, $A2, $01, $E6, $CF, $86, $D6, $20, $B4, $D7, $A5
  .byte $CF, $85, $D7, $A2, $00, $20, $35, $D7, $4C, $C8, $D4, $20, $F9, $D8, $A2, $00
  .byte $A5, $08, $29, $01, $D0, $08, $A2, $01, $A5, $09, $29, $01, $F0, $F0, $86, $D9
  .byte $A9, $01, $85, $DA, $20, $54, $D8, $C5, $D2, $F0, $18, $A2, $00, $A5, $D9, $F0
  .byte $09, $A5, $D1, $F0, $1B, $C6, $D1, $4C, $17, $D7, $A5, $D0, $F0, $12, $C6, $D0
  .byte $4C, $17, $D7, $A2, $01, $A5, $D9, $F0, $05, $E6, $D1, $4C, $17, $D7, $E6, $D0
  .byte $A5, $D9, $85, $D7, $86, $D6, $20, $B4, $D7, $A5, $D0, $85, $D7, $A2, $00, $20
  .byte $35, $D7, $A5, $D1, $85, $D7, $A2, $01, $20, $35, $D7, $4C, $C8, $D4, $A0, $00
  .byte $A9, $FF, $EA, $99, $E1, $00, $A9, $FF, $EA, $99, $F1, $00, $C8, $C0, $08, $D0
  .byte $EF, $8A, $F0, $0E, $A9, $13, $85, $D6, $A9, $50, $85, $E0, $A9, $51, $85, $F0
  .byte $D0, $0C, $A9, $08, $85, $D6, $A9, $4C, $85, $E0, $A9, $4D, $85, $F0, $A5, $D7
  .byte $F0, $29, $A0, $06, $84, $D7, $A2, $00, $C9, $0A, $90, $07, $E8, $38, $E9, $0A
  .byte $4C, $6F, $D7, $A8, $A9, $FF, $EA, $85, $D4, $A9, $FF, $EA, $A4, $D7, $99, $F0
  .byte $00, $A5, $D4, $99, $E0, $00, $C6, $D7, $8A, $D0, $DB, $A9, $00, $85, $12, $A9
  .byte $E0, $85, $11, $A2, $12, $A4, $D6, $20, $E7, $D3, $20, $8F, $D1, $A9, $F0, $85
  .byte $11, $A2, $12, $A4, $D6, $C8, $20, $E7, $D3, $20, $8F, $D1, $60, $20, $1F, $D1
  .byte $A2, $3F, $A9, $FF, $EA, $9D, $C0, $07, $CA, $10, $F7, $20, $81, $D1, $A9, $1C
  .byte $85, $05, $20, $4E, $D1, $A9, $00, $85, $50, $85, $4D, $20, $78, $D1, $A9, $00
  .byte $8D, $00, $20, $8D, $01, $20, $85, $11, $85, $D4, $A9, $FF, $85, $01, $A9, $E7
  .byte $85, $12, $A9, $10, $8D, $06, $20, $A9, $20, $8D, $06, $20, $AD, $02, $20, $A0
  .byte $00, $A2, $10, $B1, $11, $8D, $07, $20, $E6, $11, $D0, $02, $E6, $12, $CA, $D0
  .byte $F2, $E6, $D4, $A5, $D4, $C9, $50, $D0, $E8, $84, $01, $A9, $B0, $8D, $00, $20
  .byte $A9, $1E, $8D, $01, $20, $A9, $EC, $85, $12, $A2, $54, $A5, $D6, $F0, $02, $A2
  .byte $95, $86, $11, $A9, $08, $85, $D4, $A2, $05, $A5, $D7, $F0, $02, $A2, $10, $86
  .byte $D7, $A4, $D7, $A2, $05, $20, $E7, $D3, $20, $8F, $D1, $E6, $D7, $C8, $98, $18
  .byte $65, $11, $85, $11, $C6, $D4, $D0, $E9, $60, $01, $FF, $00, $FF, $20, $E0, $DD
  .byte $A0, $00, $B1, $42, $85, $D2, $20, $E0, $DD, $B1, $42, $85, $D3, $20, $E0, $DD
  .byte $8A, $18, $69, $08, $85, $D4, $A9, $00, $85, $D5, $85, $D6, $85, $D7, $A9, $21
  .byte $85, $DB, $A9, $18, $85, $D8, $A5, $DA, $F0, $07, $C6, $DB, $D0, $03, $A9, $FF
  .byte $60, $A5, $D6, $49, $FF, $85, $D6, $F0, $0A, $A9, $50, $85, $11, $A9, $D8, $85
  .byte $12, $D0, $08, $A9, $52, $85, $11, $A9, $D8, $85, $12, $A5, $D7, $0A, $A8, $B1
  .byte $42, $AA, $C8, $B1, $42, $A8, $20, $E7, $D3, $20, $8F, $D1, $C6, $D8, $F0, $C2
  .byte $A0, $00, $B1, $D4, $29, $01, $F0, $05, $E6, $D7, $A5, $D7, $60, $B1, $D4, $29
  .byte $02, $F0, $E6, $A5, $D6, $F0, $1D, $A9, $00, $85, $D6, $A9, $52, $85, $11, $A9
  .byte $D8, $85, $12, $A5, $D7, $0A, $A8, $B1, $42, $AA, $C8, $B1, $42, $A8, $20, $E7
  .byte $D3, $20, $8F, $D1, $E6, $D7, $A5, $D7, $C5, $D3, $D0, $86, $A9, $00, $85, $D7
  .byte $F0, $80

sub_b28_98F9:
  jsr sub_b31_DDE0
  jsr sub_b31_D11F
  jsr sub_b31_D91F
  lda var_0000
  bne loc_b28_991A
  jsr sub_b31_D181
  lda #$1C
  sta var_0005
  jsr sub_b31_D14E
  jsr sub_b31_D178
  lda #$00
  sta var_0050
  jsr sub_b31_D18F
loc_b28_991A:
  lda #$00
  sta var_0000
  rts

sub_b28_991F:
  lda #$07
  sta var_0045
  lda #$C0
  sta var_0044
  ldx #$40

sub_b28_9929:
  ldy #$00
  lda (var_0042),y
  tax
  and #$80
  beq sub_b28_994A
  txa
  and #$7F
  tax

sub_b28_9936:
  jsr sub_b31_DDE0
  lda (var_0042),y
  sta (var_0044),y
  inc var_0044
  beq loc_b28_995E
  dex
  bne sub_b28_9936
  jsr sub_b31_DDE0
  jmp loc_b31_D929

sub_b28_994A:
  jsr sub_b31_DDE0
  lda (var_0042),y

sub_b28_994F:
  sta (var_0044),y
  inc var_0044
  beq sub_b28_996B
  dex
  bne sub_b28_994F
  jsr sub_b31_DDE0
  jmp loc_b31_D929
loc_b28_995E:
  lda #$04
  sta var_0045
  lda #$00
  sta var_0044
  sta var_0075
  jmp loc_b31_D9A0

sub_b28_996B:
  ldy #$04
  sty var_0045
  ldy #$00
  sty var_0044
  sty var_0075
  dex
  bne loc_b28_99CA
  jsr sub_b31_DDE0

sub_b28_997B:
  ldy #$00
  lda (var_0042),y
  cmp #$FF
  bne sub_b28_9984
  rts

sub_b28_9984:
  tax
  and #$80
  beq sub_b28_99C5
  txa
  and #$7F
  tax

sub_b28_998D:
  jsr sub_b31_DDE0
  lda (var_0042),y
  cmp #$01
  beq sub_b28_99AB
  sta (var_0044),y
  inc var_0075
  inc var_0044
  bne sub_b28_99A0
  inc var_0045

sub_b28_99A0:
  ldy #$00
  dex
  bne sub_b28_998D
  jsr sub_b31_DDE0
  jmp loc_b31_D97B

sub_b28_99AB:
  ldy var_0075
  lda #$00
  sta var_0075
  cpy #$20
  beq sub_b28_99A0
  lda var_0044
  and #$E0
  clc
  adc #$20
  sta var_0044
  bcc sub_b28_99A0
  inc var_0045
  jmp loc_b31_D9A0

sub_b28_99C5:
  jsr sub_b31_DDE0
  lda (var_0042),y
loc_b28_99CA:
  cmp #$01
  beq sub_b28_99E1

sub_b28_99CE:
  sta (var_0044),y
  inc var_0075
  inc var_0044
  bne sub_b28_99D8
  inc var_0045

sub_b28_99D8:
  dex
  bne sub_b28_99CE
  jsr sub_b31_DDE0
  jmp loc_b31_D97B

sub_b28_99E1:
  ldy var_0075
  lda #$00
  sta var_0075
  cpy #$20
  beq sub_b28_99F8

sub_b28_99EB:
  lda var_0044
  and #$E0
  clc
  adc #$20
  sta var_0044
  bcc sub_b28_99F8
  inc var_0045

sub_b28_99F8:
  dex
  bne sub_b28_99EB
  jsr sub_b31_DDE0
  jmp loc_b31_D97B
  .byte $20, $E0, $DD, $A9, $00, $85, $4F, $85, $50, $20, $8F, $D1, $20, $1F, $D1, $A0
  .byte $00, $B1, $42, $85, $51, $20, $E0, $DD, $20, $1F, $D9, $A5, $00, $D0, $14, $20
  .byte $81, $D1, $A9, $24, $85, $05, $20, $4E, $D1, $20, $78, $D1, $20, $EC, $DA, $A9
  .byte $02, $85, $50, $A9, $00, $85, $00, $85, $4F, $20, $8F, $D1, $4C, $C8, $D4

sub_b28_9A40:
  lda #$00
  sta var_000F
  jmp loc_b31_DD38
  .byte $20, $E0, $DD, $20, $65, $DA, $A9, $1C, $85, $05, $20, $4E, $D1, $20, $78, $D1
  .byte $A9, $00, $85, $50, $20, $8F, $D1, $A9, $00, $85, $00, $4C, $C8, $D4, $20, $1F
  .byte $D1, $A9, $04, $85, $45, $A9, $20, $85, $44, $A0, $00, $B1, $42, $C9, $FF, $F0
  .byte $38, $C9, $0D, $F0, $0E, $91, $44, $E6, $44, $D0, $02, $E6, $45, $20, $E0, $DD
  .byte $4C, $70, $DA, $A5, $10, $F0, $12, $A5, $92, $49, $01, $85, $92, $F0, $0A, $A5
  .byte $44, $29, $E0, $18, $69, $20, $4C, $A7, $DA, $A5, $44, $29, $E0, $18, $69, $40
  .byte $85, $44, $90, $D9, $E6, $45, $4C, $84, $DA, $20, $81, $D1, $60, $20, $E0, $DD
  .byte $A0, $00, $B1, $42, $85, $51, $20, $E0, $DD, $A9, $00, $85, $4F, $85, $50, $20
  .byte $8F, $D1, $20, $65, $DA, $A9, $24, $85, $05, $20, $4E, $D1, $20, $78, $D1, $20
  .byte $8F, $D1, $20, $EC, $DA, $A9, $02, $85, $50, $A9, $00, $85, $4F, $85, $00, $20
  .byte $8F, $D1, $4C, $C8, $D4, $A9, $00, $85, $50, $A5, $51, $30, $07, $A9, $00, $85
  .byte $4F, $4C, $06, $DB, $A9, $02, $85, $50, $A9, $F0, $18, $65, $51, $85, $4F, $20
  .byte $8F, $D1, $A5, $51, $30, $0A, $18, $65, $4F, $85, $4F, $C9, $F0, $D0, $F0, $60
  .byte $18, $65, $4F, $85, $4F, $D0, $E8, $60

sub_b28_9B1F:
  jsr sub_b31_DDE0
  jsr sub_b31_D18F
  lda #$FF
  sta var_0001
  cli
  lda #$00
  sta var_2000
  sta var_2001
  lda #$10
  sta var_2006
  lda #$20
  sta var_2006
  lda var_2002
  ldy #$00
  lda (var_0042),y
  sta var_0091
  jsr sub_b31_DDE0
  lda (var_0042),y
  sta var_0090

sub_b28_9B4C:
  ldx #$00

sub_b28_9B4E:
  jsr sub_b31_DDE0
  lda (var_0042),y
  bne sub_b28_9B79
  ldx #$00

sub_b28_9B57:
  lda var_00F0,x
  sta var_2007
  inx
  cpx #$08
  bne sub_b28_9B57
  lda var_0090
  sec
  sbc #$08
  sta var_0090
  lda var_0091
  sbc #$00
  sta var_0091
  lda var_0090
  bne sub_b28_9B4C
  lda var_0091
  bne sub_b28_9B4C
  jmp loc_b31_DBCD

sub_b28_9B79:
  sta var_0075
  sta var_0076
  and #$80
  beq sub_b28_9B9F
  lda var_0075
  and #$7F
  sta var_0075
  sta var_0076

sub_b28_9B89:
  jsr sub_b31_DDE0
  lda (var_0042),y
  sta var_2007
  sta var_00F0,x
  inx
  txa
  and #$07
  tax
  dec var_0075
  bne sub_b28_9B89
  jmp loc_b31_DBB5

sub_b28_9B9F:
  jsr sub_b31_DDE0
  lda (var_0042),y
  tay

sub_b28_9BA5:
  sty var_2007
  sty var_00F0,x
  inx
  txa
  and #$07
  tax
  dec var_0075
  bne sub_b28_9BA5
  ldy #$00

sub_b28_9BB5:
  lda var_0090
  sec
  sbc var_0076
  sta var_0090
  lda var_0091
  sbc #$00
  sta var_0091
  lda var_0090
  bne sub_b28_9BCA
  lda var_0091
  beq sub_b28_9BCD

sub_b28_9BCA:
  jmp loc_b31_DB4E

sub_b28_9BCD:
  jsr sub_b31_DDE0
  ldy #$00
  sty var_0001
  sei
  lda #$10
  sta var_2000
  ldx #$02

sub_b28_9BDC:
  lda var_2002
  bpl sub_b28_9BDC
  lda #$06
  sta var_2001
  dex
  bne sub_b28_9BDC
  lda #$1C
  sta var_0005
  jsr sub_b31_D11F
  jsr sub_b31_D14E
  jsr sub_b31_D178
  jmp loc_b31_D50A
  .byte $20, $1F, $D1, $A5, $50, $D0, $0D, $20, $81, $D1, $A9, $24, $85, $05, $20, $4E
  .byte $D1, $20, $78, $D1, $20, $8F, $D1, $A5, $4F, $C9, $EA, $F0, $08, $18, $69, $06
  .byte $85, $4F, $4C, $0D, $DC, $20, $81, $D1, $A9, $1C, $85, $05, $20, $4E, $D1, $20
  .byte $78, $D1, $A9, $00, $85, $4F, $85, $50, $20, $8F, $D1, $20, $E0, $DD, $A9, $FF
  .byte $85, $01, $58, $A9, $00, $8D, $00, $20, $8D, $01, $20, $A9, $11, $8D, $06, $20
  .byte $A9, $00, $8D, $06, $20, $AD, $02, $20, $A0, $00, $B1, $42, $85, $91, $20, $E0
  .byte $DD, $B1, $42, $85, $90, $20, $E0, $DD, $A2, $08, $A5, $90, $D0, $06, $A5, $91
  .byte $F0, $20, $C6, $91, $C6, $90, $A0, $00, $B1, $42, $8D, $07, $20, $95, $F0, $20
  .byte $E0, $DD, $CA, $D0, $E5, $A2, $08, $B5, $F0, $8D, $07, $20, $CA, $D0, $F8, $4C
  .byte $61, $DC, $A2, $10, $A9, $00, $8E, $06, $20, $8D, $06, $20, $AC, $02, $20, $8D
  .byte $07, $20, $CA, $D0, $FA, $A0, $00, $84, $01, $78, $A9, $10, $8D, $00, $20, $A2
  .byte $02, $AD, $02, $20, $10, $FB, $A9, $06, $8D, $01, $20, $CA, $D0, $F3, $A9, $1C
  .byte $85, $05, $20, $1F, $D1, $20, $4E, $D1, $20, $78, $D1, $4C, $0A, $D5, $20, $E0
  .byte $DD, $A9, $FF, $85, $01, $58, $A9, $00, $8D, $00, $20, $8D, $01, $20, $A9, $00
  .byte $8D, $06, $20, $8D, $06, $20, $AC, $02, $20, $A0, $00, $B1, $42, $85, $91, $20
  .byte $E0, $DD, $B1, $42, $85, $90, $20, $E0, $DD, $A5, $90, $D0, $06, $A5, $91, $F0
  .byte $11, $C6, $91, $C6, $90, $A0, $00, $B1, $42, $8D, $07, $20, $20, $E0, $DD, $4C
  .byte $F2, $DC, $A9, $B0, $8D, $00, $20, $A9, $1E, $8D, $01, $20, $A0, $00, $84, $01
  .byte $78, $4C, $0A, $D5, $A9, $00, $85, $0F, $A9, $01, $85, $10, $A9, $03, $85, $4D
  .byte $4C, $38, $DD, $A9, $01, $85, $0F, $A9, $00, $85, $10, $A9, $03, $85, $4D

sub_b28_9D38:
  jsr sub_b31_DDE0
  jmp loc_b31_D50A
  .byte $A9, $02, $85, $0F, $A9, $00, $85, $10, $A9, $03, $85, $4D, $4C, $38, $DD, $20
  .byte $E0, $DD, $A9, $FF, $85, $01, $58, $A9, $00, $8D, $00, $20, $8D, $01, $20, $A9
  .byte $00, $8D, $06, $20, $8D, $06, $20, $AC, $02, $20, $A0, $00, $B1, $42, $8D, $07
  .byte $20, $C8, $C0, $80, $D0, $F6, $A9, $80, $18, $65, $42, $85, $42, $A9, $00, $65
  .byte $43, $85, $43, $A9, $B0, $8D, $00, $20, $A9, $1E, $8D, $01, $20, $A0, $00, $B1
  .byte $42, $99, $00, $02, $C8, $D0, $F8, $E6, $43, $A0, $00, $84, $01, $78, $B1, $42
  .byte $99, $80, $7F, $C8, $C0, $20, $D0, $F6, $A9, $00, $85, $15, $A2, $20, $BD, $80
  .byte $7F, $18, $65, $15, $9D, $80, $7F, $CA, $D0, $F4, $86, $0B, $20, $BA, $D0, $A5
  .byte $0B, $C9, $10, $D0, $FA, $E6, $15, $E6, $15, $A5, $15, $C9, $30, $D0, $DD, $4C
  .byte $C8, $D4, $48, $8A, $48, $98, $48, $20, $10, $60, $68, $A8, $68, $AA, $68, $4C
  .byte $C8, $D4

sub_b28_9DE0:
  inc var_0042
  bne sub_b28_9DE6
  inc var_0043

sub_b28_9DE6:
  rts

sub_b28_9DE7:
  lda #$0F
  sta var_4015
  lda #$C0
  sta var_4017
  lda #$1C
  sta var_00F0
  lda #$03
  sta var_00F1
  ldx #$00
  ldy #$00

sub_b28_9DFD:
  tya
  sta (var_00F0),y
  sta var_0300,x
  clc
  lda var_00F0
  adc #$08
  sta var_00F0
  bcc sub_b28_9E0E
  inc var_00F1

sub_b28_9E0E:
  inx
  cpx #$1C
  bne sub_b28_9DFD
  rts
  .byte $3A, $0F, $D8, $10, $8A, $87, $39, $58, $88, $88, $58, $A8, $88, $7A, $14, $AA
  .byte $2A, $D8, $64, $D8, $44, $3A, $CC, $D8, $74, $3A, $EE, $D8, $54, $58, $98, $88
  .byte $58, $A8, $88, $AB, $44, $58, $B9, $88, $E4, $E4, $E4, $E4, $58, $08, $88, $3A
  .byte $D2, $58, $E8, $88, $7A, $E8, $AA, $89, $64, $4C, $DD, $FE, $8E, $54, $8E, $74
  .byte $FA, $74, $5C, $2D, $DD, $AD, $7A, $F8, $AA, $58, $20, $88, $7A, $50, $2A, $84
  .byte $75, $DD, $20, $DD, $51, $3D, $75, $DD
  .res 148, $0F
  .byte $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $B6, $B6, $B6, $B6, $B6, $B6, $B6, $B6
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $49, $49, $49, $49, $49, $49, $49, $49
  .byte $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $B6, $B6, $B6, $B6, $B6, $B6, $B6, $B6
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $49, $49, $49, $49, $49, $49, $49, $49
  .byte $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $B6, $B6, $B6, $B6, $B6, $B6, $B6, $B6
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $49, $49, $49, $49, $49, $49, $49, $49
  .byte $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $B6, $B6, $B6, $B6, $B6, $B6, $B6, $B6
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $49, $49, $49, $49, $49, $49, $49, $49
  .byte $F2, $61, $9C, $54, $F8, $F0, $B0, $D0, $B6, $B6, $B6, $B6, $B6, $B6, $B6, $B6
  .byte $E2, $23, $9C, $5C, $F8, $D0, $B0, $D0, $BE, $39, $A5, $38, $58, $22, $95, $69
  .byte $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $B6, $B6, $B6, $B6, $B6, $B6, $B6, $B6
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $49, $49, $49, $49, $49, $49, $49, $49
  .byte $B0, $E0, $F0, $F0, $F1, $14, $F0, $A0, $03, $0F, $1C, $18, $FD, $0F, $B8, $CB
  .byte $62, $27, $9C, $5C, $F8, $D0, $B8, $00, $CA, $8B, $03, $F9, $E5, $09, $55, $49
  .byte $F0, $F0, $F0, $F0, $F0, $F0, $F0, $F0, $03, $0B, $18, $19, $ED, $0F, $39, $49
  .byte $0F, $0F, $0F, $0F, $0F, $0F, $0F, $0F, $01, $0B, $18, $19, $ED, $0F, $39, $49
  .res 6749, $00
  .byte $A9, $80, $8D, $00, $50
  .res 1432, $00
  .byte $02, $D0, $5D, $FA, $00, $00
