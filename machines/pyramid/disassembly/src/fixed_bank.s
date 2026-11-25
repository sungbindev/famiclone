; 模块：fixed_bank
; 固定在 $C000 的块 31：系统跳转表、NMI/IRQ/复位、苹果机监控程序同址的入口、复位初始化

.setcpu "6502"
.include "constants.inc"
.include "globals.inc"

.segment "SEG31_0000"
; 段 31，bank 31，$C000-$FFFF
sub_b31_C000:
  jmp $86E2

sub_b31_C003:
  jmp sub_b31_C13E

sub_b31_C006:
  jmp $86E5

sub_b31_C009:
  jmp $86E8

sub_b31_C00C:
  jmp $86EB

sub_b31_C00F:
  jmp $86EE

sub_b31_C012:
  jmp $86F1
  .byte $4C, $F4, $86, $4C, $E0, $C1, $4C, $72
  .byte $C2

sub_b31_C01E:
  .byte $4C, $01, $C8
  .byte $4C, $11, $C6, $4C, $93, $C2, $4C, $20
  .byte $C3, $4C, $84, $C3, $4C, $1E, $C5

sub_b31_C030:
  .byte $4C, $FB, $C6

sub_b31_C033:
  .byte $4C, $2C, $C7
loc_b31_C036:
  .byte $4C, $80, $C7

; NMI 处理（固定在 $C000 窗口的块 31 里）：写 $2003=0 并用 $4014 从 $C0 页做精灵 DMA（未确认：$C000 页是系统跳转表，可能用来把所有精灵移出屏幕）；32 位帧计数 $03CC-$03CF 加 1；$03C7 不为 0 时这一帧还要把 $6800 缓冲里排队的显存任务写进 PPU（用到 $5000=$80 切到块 0，调用 $86F7，之后按 $03C8 恢复 $5000）；最后 $03C9=6 时调用 nmi_call_hook。
nmi_handler:
  .byte $48

sub_b31_C03A:
  .byte $08

sub_b31_C03B:
  .byte $A9, $00

sub_b31_C03D:
  .byte $8D, $03, $20

sub_b31_C040:
  .byte $A9, $C0

sub_b31_C042:
  .byte $8D, $14, $40

sub_b31_C045:
  .byte $EE, $CC, $03

sub_b31_C048:
  .byte $D0, $0D

sub_b31_C04A:
  .byte $EE, $CD, $03

sub_b31_C04D:
  .byte $D0, $08
  .byte $EE, $CE, $03, $D0, $03, $EE, $CF, $03
loc_b31_C057:
  .byte $AD, $C7, $03

sub_b31_C05A:
  .byte $D0, $03

sub_b31_C05C:
  .byte $4C, $31, $C1
loc_b31_C05F:
  .byte $2C, $02, $20

sub_b31_C062:
  .byte $30, $FB

sub_b31_C064:
  .byte $8A

sub_b31_C065:
  .byte $48

sub_b31_C066:
  .byte $98

sub_b31_C067:
  .byte $48

sub_b31_C068:
  .byte $A5, $FE

sub_b31_C06A:
  .byte $48

sub_b31_C06B:
  .byte $A5, $FF

sub_b31_C06D:
  .byte $48

sub_b31_C06E:
  .byte $A5, $26

sub_b31_C070:
  .byte $48

sub_b31_C071:
  .byte $A5, $27

sub_b31_C073:
  .byte $48

sub_b31_C074:
  .byte $AC, $C5, $03

sub_b31_C077:
  .byte $A9, $0E

sub_b31_C079:
  .byte $8D, $C5, $03
loc_b31_C07C:
  .byte $A2, $08

sub_b31_C07E:
  .byte $B9, $00, $68

sub_b31_C081:
  .byte $D0, $44, $8D, $00, $50, $C8, $B9, $00, $68, $8D, $06, $20, $C8, $B9, $00, $68
  .byte $8D, $06, $20, $AD, $07, $20, $AD, $07, $20, $85, $FE, $88, $BE, $00, $68, $8E
  .byte $06, $20, $C8, $BE, $00, $68, $8E, $06, $20, $C8, $B9, $00, $68, $45, $FE, $C8
  .byte $39, $00, $68, $45, $FE, $8D, $07, $20, $C8, $CE, $C7, $03, $F0, $51, $CE, $C5
  .byte $03, $10, $B8, $4C, $10, $C1
loc_b31_C0C7:
  .byte $30, $11, $09, $80, $AA, $AD, $C5, $03, $38, $E9, $04, $30, $3C, $8D, $C5, $03
  .byte $8A, $A2, $10
loc_b31_C0DA:
  .byte $8D, $00, $50

sub_b31_C0DD:
  .byte $C8

sub_b31_C0DE:
  .byte $B9, $00, $68

sub_b31_C0E1:
  .byte $85, $FE

sub_b31_C0E3:
  .byte $C8

sub_b31_C0E4:
  .byte $B9, $00, $68

sub_b31_C0E7:
  .byte $85, $FF

sub_b31_C0E9:
  .byte $C8

sub_b31_C0EA:
  .byte $B9, $00, $68

sub_b31_C0ED:
  .byte $85, $26

sub_b31_C0EF:
  .byte $C8

sub_b31_C0F0:
  .byte $B9, $00, $68

sub_b31_C0F3:
  .byte $85, $27

sub_b31_C0F5:
  .byte $C8

sub_b31_C0F6:
  .byte $98

sub_b31_C0F7:
  .byte $48

sub_b31_C0F8:
  .byte $20, $42, $05

sub_b31_C0FB:
  .byte $68

sub_b31_C0FC:
  .byte $A8

sub_b31_C0FD:
  .byte $CE, $C7, $03

sub_b31_C100:
  .byte $F0, $0E

sub_b31_C102:
  .byte $AD, $C5, $03

sub_b31_C105:
  .byte $38

sub_b31_C106:
  .byte $E9, $04

sub_b31_C108:
  .byte $8D, $C5, $03

sub_b31_C10B:
  .byte $30, $03

sub_b31_C10D:
  .byte $4C, $7C, $C0
loc_b31_C110:
  .byte $8C, $C5, $03

sub_b31_C113:
  .byte $A9, $80

sub_b31_C115:
  .byte $8D, $00, $50

sub_b31_C118:
  .byte $20, $F7, $86

sub_b31_C11B:
  .byte $AD, $C8, $03

sub_b31_C11E:
  .byte $8D, $00, $50

sub_b31_C121:
  .byte $68

sub_b31_C122:
  .byte $85, $27

sub_b31_C124:
  .byte $68

sub_b31_C125:
  .byte $85, $26

sub_b31_C127:
  .byte $68

sub_b31_C128:
  .byte $85, $FF

sub_b31_C12A:
  .byte $68

sub_b31_C12B:
  .byte $85, $FE

sub_b31_C12D:
  .byte $68

sub_b31_C12E:
  .byte $A8

sub_b31_C12F:
  .byte $68

sub_b31_C130:
  .byte $AA
loc_b31_C131:
  .byte $AD, $C9, $03

sub_b31_C134:
  .byte $C9, $06

sub_b31_C136:
  .byte $D0, $03, $20, $F4, $FC
loc_b31_C13B:
  .byte $28

sub_b31_C13C:
  .byte $68

sub_b31_C13D:
  .byte $40

sub_b31_C13E:
  .byte $8D, $2E, $05

sub_b31_C141:
  .byte $8A

sub_b31_C142:
  .byte $48

sub_b31_C143:
  .byte $98

sub_b31_C144:
  .byte $48

sub_b31_C145:
  .byte $AD, $2E, $05

sub_b31_C148:
  .byte $10, $46

sub_b31_C14A:
  .byte $C9, $80

sub_b31_C14C:
  .byte $F0, $45

sub_b31_C14E:
  .byte $20, $72, $C2

sub_b31_C151:
  .byte $09, $80

sub_b31_C153:
  .byte $48

sub_b31_C154:
  .byte $A4, $25

sub_b31_C156:
  .byte $A6, $24

sub_b31_C158:
  .byte $20, $01, $C8

sub_b31_C15B:
  .byte $68

sub_b31_C15C:
  .byte $AC, $C7, $03

sub_b31_C15F:
  .byte $C0, $33

sub_b31_C161:
  .byte $B0, $F9

sub_b31_C163:
  .byte $AC, $C6, $03

sub_b31_C166:
  .byte $99, $00, $68

sub_b31_C169:
  .byte $C8

sub_b31_C16A:
  .byte $A5, $FE

sub_b31_C16C:
  .byte $99, $00, $68

sub_b31_C16F:
  .byte $C8

sub_b31_C170:
  .byte $A5, $FF

sub_b31_C172:
  .byte $99, $00, $68

sub_b31_C175:
  .byte $C8

sub_b31_C176:
  .byte $A5, $26

sub_b31_C178:
  .byte $99, $00, $68

sub_b31_C17B:
  .byte $C8

sub_b31_C17C:
  .byte $A5, $27

sub_b31_C17E:
  .byte $99, $00, $68

sub_b31_C181:
  .byte $C8

sub_b31_C182:
  .byte $8C, $C6, $03

sub_b31_C185:
  .byte $EE, $C7, $03

sub_b31_C188:
  .byte $68

sub_b31_C189:
  .byte $A8

sub_b31_C18A:
  .byte $68

sub_b31_C18B:
  .byte $AA

sub_b31_C18C:
  .byte $AD, $2E, $05

sub_b31_C18F:
  .byte $60

sub_b31_C190:
  .byte $8D, $FC, $03, $20, $E0, $C1, $29, $7F, $4C, $53, $C1

sub_b31_C19B:
  .byte $8E, $35, $05

sub_b31_C19E:
  .byte $A5, $27

sub_b31_C1A0:
  .byte $8D, $06, $20

sub_b31_C1A3:
  .byte $A5, $26

sub_b31_C1A5:
  .byte $8D, $06, $20

sub_b31_C1A8:
  .byte $AC, $35, $05

sub_b31_C1AB:
  .byte $C0, $08

sub_b31_C1AD:
  .byte $D0, $0A

sub_b31_C1AF:
  .byte $A9, $00

sub_b31_C1B1:
  .byte $8D, $07, $20

sub_b31_C1B4:
  .byte $88

sub_b31_C1B5:
  .byte $D0, $FA

sub_b31_C1B7:
  .byte $F0, $0F

sub_b31_C1B9:
  .byte $A0, $00

sub_b31_C1BB:
  .byte $B1, $FE, $8D, $07, $20, $C8, $C0, $10, $90, $F6, $0E, $35, $05

sub_b31_C1C8:
  .byte $E6, $27

sub_b31_C1CA:
  .byte $A5, $27

sub_b31_C1CC:
  .byte $8D, $06, $20

sub_b31_C1CF:
  .byte $A5, $26

sub_b31_C1D1:
  .byte $8D, $06, $20

sub_b31_C1D4:
  .byte $B1, $FE

sub_b31_C1D6:
  .byte $8D, $07, $20

sub_b31_C1D9:
  .byte $C8

sub_b31_C1DA:
  .byte $CC, $35, $05

sub_b31_C1DD:
  .byte $90, $F5

sub_b31_C1DF:
  .byte $60

sub_b31_C1E0:
  .byte $AD, $FB, $03, $29, $7F, $AA, $AD, $FC, $03, $29, $7F, $E0, $23, $D0, $02, $A2
  .byte $2F

sub_b31_C1F1:
  .byte $38, $E9, $21, $90, $11, $C9, $5E, $B0, $0D, $48, $8A, $38, $E9, $2F, $AA, $68
  .byte $90, $04, $E0, $49, $90, $04
  .byte $A2, $28, $A9, $5D

sub_b31_C20B:
  .byte $48, $A9, $00, $85, $FF, $8A, $0A, $26, $FF, $85, $FE, $A4, $FF, $0A, $26, $FF
  .byte $0A, $26, $FF, $0A, $26, $FF, $0A, $26, $FF, $48, $65, $FE, $85, $FE, $98, $65
  .byte $FF, $A8, $68, $0A, $26, $FF, $0A, $26, $FF, $38, $E5, $FE, $85, $FE, $A5, $FF
  .byte $84, $FF, $E5, $FF, $85, $FF, $68, $18, $65, $FE, $90, $02, $E6, $FF

sub_b31_C249:
  .byte $A0, $00, $84, $FE, $A0, $05

sub_b31_C24F:
  .byte $0A, $26, $FF, $26, $FE, $88, $D0, $F8, $A4, $FE, $85, $FE, $98, $06, $FF, $2A
  .byte $06, $FF, $2A, $46, $FF, $46, $FF, $18, $69, $81, $48, $A5, $FF, $09, $80, $85
  .byte $FF, $68, $60

sub_b31_C272:
  .byte $38

sub_b31_C273:
  .byte $E9, $A0

sub_b31_C275:
  .byte $B0, $02
  .byte $A9, $00

sub_b31_C279:
  .byte $A0, $00

sub_b31_C27B:
  .byte $84, $FF

sub_b31_C27D:
  .byte $A0, $03

sub_b31_C27F:
  .byte $0A

sub_b31_C280:
  .byte $26, $FF

sub_b31_C282:
  .byte $88

sub_b31_C283:
  .byte $D0, $FA

sub_b31_C285:
  .byte $18

sub_b31_C286:
  .byte $69, $C0

sub_b31_C288:
  .byte $85, $FE

sub_b31_C28A:
  .byte $A5, $FF

sub_b31_C28C:
  .byte $69, $99

sub_b31_C28E:
  .byte $85, $FF

sub_b31_C290:
  .byte $A9, $8E

sub_b31_C292:
  .byte $60
  .byte $A9, $8E, $8D, $C8, $03, $8D, $00, $50
  .byte $18, $08, $28, $08, $90, $08, $A9, $8E
  .byte $8D, $C8, $03, $8D, $00, $50, $A0, $04
  .byte $B1, $26, $AA, $90, $08, $A9, $8F, $8D
  .byte $C8, $03, $8D, $00, $50, $A0, $01, $B1
  .byte $FE, $D9, $F1, $04, $D0, $21, $88, $10
  .byte $F6, $28, $90, $08, $A9, $8E, $8D, $C8
  .byte $03, $8D, $00, $50, $A0, $03, $B1, $26
  .byte $99, $13, $05, $88, $10, $F8, $A9, $80
  .byte $8D, $C8, $03, $8D, $00, $50, $60, $A5
  .byte $FE, $18, $69, $02, $85, $FE, $90, $17
  .byte $E6, $FF, $A5, $FF, $C9, $C0, $D0, $0F
  .byte $A9, $80, $85, $FF, $A9, $8F, $8D, $C8
  .byte $03, $8D, $00, $50, $28, $38, $08, $CA
  .byte $D0, $B3, $A5, $26, $18, $69, $05, $85
  .byte $26, $90, $02, $E6, $27, $A5, $27, $C9
  .byte $A4, $D0, $87, $A5, $26, $C9, $C2, $D0
  .byte $81, $28, $4C, $D9, $C2, $A9, $8F, $8D
  .byte $C8, $03, $8D, $00, $50, $8A, $48, $AE
  .byte $1E, $05, $90, $09, $8A, $2D, $23, $05
  .byte $CD, $15, $05, $F0, $2A, $A0, $00, $B1
  .byte $26, $18, $6D, $16, $05, $8D, $16, $05
  .byte $C8, $B1, $26, $6D, $17, $05, $8D, $17
  .byte $05, $A5, $26, $18, $69, $02, $85, $26
  .byte $90, $02, $E6, $27, $E8, $D0, $D5, $68
  .byte $AA, $A9, $00, $48, $A8, $F0, $18, $A0
  .byte $00, $B1, $26, $D0, $06, $C8, $D1, $26
  .byte $F0, $DF, $88, $8E, $1E, $05, $68, $AA
  .byte $B1, $26, $48, $C8, $B1, $26, $A8, $A9
  .byte $80, $8D, $C8, $03, $8D, $00, $50, $68
  .byte $60, $8D, $19, $05, $29, $10, $F0, $09
  .byte $AD, $19, $05, $18, $69, $10, $8D, $19
  .byte $05, $AD, $19, $05, $8D, $C8, $03, $8D
  .byte $00, $50, $A0, $01, $B1, $FE, $2D, $22
  .byte $05, $CD, $14, $05, $D0, $0D, $88, $B1
  .byte $FE, $29, $F0, $2D, $21, $05, $CD, $13
  .byte $05, $F0, $45, $A0, $00, $B1, $FE, $29
  .byte $08, $D0, $3A, $B1, $FE, $4A, $90, $19
  .byte $AD, $13, $05, $D0, $30, $C8, $B1, $FE
  .byte $2D, $22, $05, $29, $FE, $4D, $14, $05
  .byte $4A, $B0, $22, $F0, $23, $88, $B1, $FE
  .byte $4A, $4A, $90, $19, $AD, $13, $05, $D0
  .byte $14, $AD, $14, $05, $29, $3F, $D0, $0D
  .byte $C8, $B1, $FE, $2D, $22, $05, $29, $C0
  .byte $CD, $14, $05, $F0, $03, $4C, $D0, $C4
  .byte $A0, $00, $B1, $FE, $29, $08, $F0, $06
  .byte $AD, $21, $05, $4A, $B0, $EF, $AD, $1F
  .byte $05, $D0, $05, $CD, $20, $05, $F0, $0D
  .byte $38, $E9, $01, $8D, $1F, $05, $B0, $DD
  .byte $CE, $20, $05, $10, $D8, $B1, $FE, $29
  .byte $08, $D0, $18, $A9, $02, $9D, $F0, $04
  .byte $E8, $A0, $02, $B1, $FE, $9D, $F0, $04
  .byte $C8, $E8, $B1, $FE, $9D, $F0, $04, $E8
  .byte $4C, $CC, $C4, $B1, $FE, $29, $07, $D0
  .byte $02, $A9, $08, $8D, $1C, $05, $0A, $9D
  .byte $F0, $04, $E8, $A5, $26, $48, $A5, $27
  .byte $48, $A0, $02, $B1, $FE, $18, $69, $62
  .byte $85, $26, $A9, $B2, $29, $7F, $C8, $71
  .byte $FE, $A0, $00, $90, $01, $C8, $8C, $1D
  .byte $05, $0A, $2E, $1D, $05, $0A, $2E, $1D
  .byte $05, $4A, $4A, $09, $80, $85, $27, $AD
  .byte $1D, $05, $69, $A3, $8D, $C8, $03, $8D
  .byte $00, $50, $8D, $1D, $05, $A0, $00, $B1
  .byte $26, $9D, $F0, $04, $C8, $E8, $C0, $02
  .byte $90, $F5, $CE, $1C, $05, $F0, $23, $A5
  .byte $26, $18, $69, $02, $85, $26, $90, $E5
  .byte $E6, $27, $A5, $27, $C9, $C0, $D0, $DD
  .byte $A9, $80, $85, $27, $EE, $1D, $05, $AD
  .byte $1D, $05, $8D, $C8, $03, $8D, $00, $50
  .byte $D0, $CB, $AD, $19, $05, $8D, $C8, $03
  .byte $8D, $00, $50, $68, $85, $27, $68, $85
  .byte $26, $E0, $12, $B0, $45, $A5, $FE, $18
  .byte $69, $04, $85, $FE, $90, $21, $E6, $FF
  .byte $A5, $FF, $C9, $C0, $D0, $19, $A9, $80
  .byte $85, $FF, $EE, $19, $05, $AD, $19, $05
  .byte $C9, $90, $D0, $05, $A9, $A0, $8D, $19
  .byte $05, $8D, $C8, $03, $8D, $00, $50, $AD
  .byte $1A, $05, $38, $E9, $01, $8D, $1A, $05
  .byte $F0, $0A, $C9, $FF, $D0, $03, $CE, $1B
  .byte $05, $4C, $9D, $C3, $AD, $1B, $05, $D0
  .byte $F8, $18, $A9, $80, $8D, $C8, $03, $8D
  .byte $00, $50, $60, $A9, $8F, $8D, $C8, $03
  .byte $8D, $00, $50, $8D, $19, $05, $A0, $00
  .byte $B1, $26, $29, $08, $D0, $11, $A0, $02
  .byte $B1, $26, $CD, $F1, $04, $D0, $08, $C8
  .byte $B1, $26, $CD, $F2, $04, $F0, $44, $A5
  .byte $26, $18, $69, $04, $85, $26, $90, $21
  .byte $E6, $27, $A5, $27, $C9, $C0, $D0, $19
  .byte $A9, $80, $85, $27, $EE, $19, $05, $AD
  .byte $19, $05, $C9, $90, $D0, $05, $A9, $A0
  .byte $8D, $19, $05, $8D, $C8, $03, $8D, $00
  .byte $50, $AD, $19, $05, $C9, $A3, $D0, $B6
  .byte $A5, $26, $C9, $62, $D0, $B0, $A5, $27
  .byte $C9, $B2, $D0, $AA, $A9, $00, $D0, $03
  .byte $4C, $08, $C6, $A0, $01, $B1, $26, $99
  .byte $13, $05, $88, $10, $F8, $AD, $19, $05
  .byte $C9, $90, $90, $02, $E9, $10, $38, $E9
  .byte $8F, $06, $27, $06, $27, $4A, $66, $27
  .byte $4A, $66, $27, $AA, $A5, $26, $38, $E9
  .byte $D2, $8D, $16, $05, $A5, $27, $E9, $1F
  .byte $8D, $17, $05, $B0, $01, $CA, $8A, $A0
  .byte $01, $4A, $6E, $17, $05, $6E, $16, $05
  .byte $88, $10, $F6, $A9, $D2, $85, $26, $A9
  .byte $9D, $85, $27, $A9, $8F, $8D, $C8, $03
  .byte $8D, $00, $50, $A0, $00, $AD, $16, $05
  .byte $38, $F1, $26, $8D, $16, $05, $C8, $AD
  .byte $17, $05, $F1, $26, $8D, $17, $05, $90
  .byte $0C, $A5, $26, $69, $01, $85, $26, $90
  .byte $E2, $E6, $27, $D0, $DE, $A5, $26, $38
  .byte $E9, $D2, $85, $26, $A5, $27, $E9, $9D
  .byte $4A, $66, $26, $A5, $26, $A0, $80, $8C
  .byte $C8, $03, $8C, $00, $50, $60, $A9, $8E
  .byte $8D, $C8, $03, $8D, $00, $50, $AD, $1F
  .byte $05, $8D, $17, $05, $AD, $20, $05, $8D
  .byte $18, $05, $A2, $00, $A0, $03, $B1, $26
  .byte $39, $21, $05, $D9, $13, $05, $D0, $15
  .byte $88, $10, $F3, $A0, $04, $AD, $17, $05
  .byte $38, $F1, $26, $8D, $17, $05, $B0, $05
  .byte $CE, $18, $05, $30, $35, $A0, $04, $B1
  .byte $26, $0A, $65, $FE, $85, $FE, $A5, $FF
  .byte $69, $00, $85, $FF, $A5, $26, $69, $05
  .byte $85, $26, $A5, $27, $69, $00, $85, $27
  .byte $C9, $A4, $90, $C0, $D0, $06, $A5, $26
  .byte $C9, $C2, $90, $B8, $A9, $00, $9D, $F0
  .byte $04, $A0, $80, $8C, $C8, $03, $8C, $00
  .byte $50, $60, $A5, $FF, $48, $A5, $FE, $48
  .byte $AD, $17, $05, $18, $71, $26, $0A, $65
  .byte $FE, $85, $FE, $A5, $FF, $69, $00, $C9
  .byte $C0, $90, $02, $E9, $40, $85, $FF, $A9
  .byte $8E, $69, $00, $8D, $C8, $03, $8D, $00
  .byte $50, $A0, $00, $A9, $02, $9D, $F0, $04
  .byte $E8, $B1, $FE, $9D, $F0, $04, $E8, $C8
  .byte $B1, $FE, $9D, $F0, $04, $E8, $C8, $E0
  .byte $12, $B0, $27, $A5, $FE, $18, $69, $02
  .byte $85, $FE, $90, $14, $E6, $FF, $A5, $FF
  .byte $C9, $C0, $D0, $0C, $A9, $80, $85, $FF
  .byte $A9, $8F, $8D, $C8, $03, $8D, $00, $50
  .byte $EE, $17, $05, $D0, $C4, $EE, $18, $05
  .byte $D0, $BF, $68, $85, $FE, $68, $85, $FF
  .byte $A9, $8E, $8D, $C8, $03, $8D, $00, $50
  .byte $B0, $03, $4C, $48, $C6, $4C, $6F, $C6
  .byte $A0, $A6, $AD, $13, $05, $18, $69
data_b31_C702:
  .byte $62, $85, $FE, $A9, $BD, $29, $7F, $6D
  .byte $14, $05, $C9, $40, $90, $03, $E9, $40
  .byte $C8, $09, $80, $85, $FF, $8C, $C8, $03
  .byte $8C, $00, $50, $A0, $00, $B1, $FE, $08
  .byte $A0, $80, $8C, $C8, $03, $8C, $00, $50

sub_b31_C72A:
  .byte $28, $60, $A9, $A6, $8D, $C8, $03, $8D
  .byte $00, $50, $AD, $13, $05, $8D, $15, $05
  .byte $AD, $14, $05, $8D, $16, $05, $A0, $00
  .byte $AD, $15, $05, $38, $E9, $01, $8D, $15
  .byte $05, $B0, $05, $CE, $16, $05, $30, $25
  .byte $B1, $26, $0A, $65, $FE, $85, $FE, $90
  .byte $02, $E6, $FF, $E6, $26, $D0, $E1, $E6
  .byte $27, $A5, $27, $C9, $C0, $90, $D9, $A9
  .byte $80, $85, $27, $A9, $A7, $8D, $C8, $03
  .byte $8D, $00, $50, $D0, $CB, $A9, $80, $8D
  .byte $C8, $03, $8D, $00, $50, $60, $A2, $00
  .byte $38, $ED, $1F, $05, $90, $02, $D0, $03
  .byte $4C, $F3, $C7, $8D, $15, $05, $AD, $1F
  .byte $05, $0A, $65, $FE, $85, $FE, $90, $02
  .byte $E6, $FF, $A0, $A7, $A5, $FF, $C9, $40
  .byte $90, $05, $E9, $40, $C8, $B0, $F7, $09
  .byte $80, $85, $FF, $8C, $C8, $03, $8C, $00
  .byte $50, $98, $48, $A0, $00, $A9, $02, $9D
  .byte $F0, $04, $E8, $B1, $FE, $9D, $F0, $04
  .byte $C8, $E8, $B1, $FE, $9D, $F0, $04, $E8
  .byte $68, $A8, $CE, $15, $05, $F0, $22, $E0
  .byte $12, $B0, $1E, $A5, $FE, $18, $69, $02
  .byte $85, $FE, $90, $D5, $E6, $FF, $A5, $FF
  .byte $C9, $C0, $90, $CD, $C8, $8C, $C8, $03
  .byte $8C, $00, $50, $A9, $80, $85, $FF, $D0
  .byte $C0, $A9, $00, $9D, $F0, $04, $A9, $80
  .byte $8D, $C8, $03, $8D, $00, $50, $60

sub_b31_C801:
  .byte $98

sub_b31_C802:
  .byte $0A

sub_b31_C803:
  .byte $8D, $32, $05
loc_b31_C806:
  .byte $A5, $E4

sub_b31_C808:
  .byte $4A
loc_b31_C809:
  .byte $4A

sub_b31_C80A:
  .byte $4A

sub_b31_C80B:
  .byte $18
loc_b31_C80C:
  .byte $6D, $32, $05

sub_b31_C80F:
  .byte $C9, $1E

sub_b31_C811:
  .byte $90
loc_b31_C812:
  .byte $02

sub_b31_C813:
  .byte $E9, $1E

sub_b31_C815:
  .byte $85, $27

sub_b31_C817:
  .byte $8A

sub_b31_C818:
  .byte $0A

sub_b31_C819:
  .byte $0A

sub_b31_C81A:
  .byte $0A

sub_b31_C81B:
  .byte $69, $08

sub_b31_C81D:
  .byte $85, $26

sub_b31_C81F:
  .byte $60
loc_b31_C820:
  .byte $4C, $29, $C8
loc_b31_C823:
  .byte $20
loc_b31_C824:
  .byte $E4, $C8
loc_b31_C826:
  .byte $4C, $81, $C8

sub_b31_C829:
  .byte $A9

sub_b31_C82A:
  .byte $12, $8D, $43, $05, $A9, $C9, $8D, $44, $05, $A9, $1E, $85, $21, $8D, $47, $03
  .byte $A9, $00, $A0, $26

sub_b31_C83E:
  .byte $99

sub_b31_C83F:
  .byte $20, $03, $88, $10, $FA, $8D, $A8, $03, $A8, $A9, $AA, $8D, $C8, $03, $8D, $00
  .byte $50

sub_b31_C850:
  .byte $B9, $00, $BF, $99, $00, $60, $C8, $D0, $F7, $8C, $71, $03, $8C
loc_b31_C85D:
  .byte $73, $03, $8C, $75, $03, $A9, $70, $8D, $72, $03, $A9, $73, $8D, $74, $03, $A9
  .byte $80, $CD, $C2, $03, $F0, $05
  .byte $20, $6A, $60, $A9, $FF

sub_b31_C878:
  .byte $8D, $76, $03, $20, $E4, $C8, $4C
loc_b31_C87F:
  .byte $80, $B7
  .byte $20, $21, $84, $A0, $00, $A2, $07, $20
  .byte $0C, $84, $A0, $00, $20, $12, $84, $A5
  .byte $27, $8D, $06, $20, $A5, $26, $8D, $06
  .byte $20, $A2, $00, $B9, $9B, $86, $8D, $07
  .byte $20, $C8, $E8, $C0, $B8, $B0, $0C, $E0
  .byte $40, $90, $F0, $A5, $26, $69, $3F, $85
  .byte $26, $90, $D9, $20, $0F, $84, $A9, $00
  .byte $85, $25, $AC, $46
loc_b31_C8BD:
  .byte $03, $F0, $1C, $88, $A9, $1F, $85, $21
  .byte $A9, $FF, $8D, $20, $03, $B9, $3B, $03
  .byte $85, $24, $98, $09, $B0, $20, $03, $84
  .byte $88, $10, $F2, $A9, $1E, $85, $21, $A9
  .byte $00, $8D, $20, $03, $4C, $80, $99

sub_b31_C8E4:
  .byte $AD, $71, $03, $85, $FA, $A9, $00, $A8, $AE, $72, $03

sub_b31_C8EF:
  .byte $86, $FB

sub_b31_C8F1:
  .byte $20, $33, $60, $C8, $D0, $FA, $E8, $EC, $76, $03, $90, $F2, $60
  .byte $99, $4C

sub_b31_C900:
  .byte $4C, $33, $C9

sub_b31_C903:
  .byte $4C, $4F, $C9, $4C, $5B, $C9
loc_b31_C909:
  .byte $4C, $67, $C9
loc_b31_C90C:
  .byte $4C, $73, $C9
loc_b31_C90F:
  .byte $4C, $7C, $C9

sub_b31_C912:
  .byte $4C, $87, $C9

sub_b31_C915:
  .byte $4C, $99, $CA

sub_b31_C918:
  .byte $4C, $A2, $CA

sub_b31_C91B:
  .byte $4C, $B0, $CA

sub_b31_C91E:
  .byte $4C, $BB, $CA

sub_b31_C921:
  .byte $4C, $C4, $CA

sub_b31_C924:
  .byte $4C, $CF, $CA

sub_b31_C927:
  .byte $4C, $DB, $CA

sub_b31_C92A:
  .byte $4C, $E4, $CA

sub_b31_C92D:
  .byte $4C, $F2, $CA

sub_b31_C930:
  .byte $4C, $04, $CB, $20, $2D, $CB, $20, $18
  .byte $C0, $A0, $1F, $48, $8D, $C8, $03, $8D
  .byte $00, $50, $B1, $FE, $99, $00, $03, $88
  .byte $10, $F8, $20, $36, $CB, $68, $60, $48
  .byte $20, $2D, $CB, $68, $20, $1B, $C0, $A0
  .byte $07, $D0, $E0, $20, $2D, $CB, $20, $00
  .byte $C0, $48, $20, $36, $CB, $68, $60, $20
  .byte $2D, $CB, $20, $12, $C0, $48, $20, $36
  .byte $CB, $68, $60, $20, $2D, $CB, $20, $0C
  .byte $C0, $4C, $36, $CB, $48, $20, $2D, $CB
  .byte $68, $20, $09, $C0, $4C, $36, $CB

sub_b31_C987:
  .byte $A0, $00, $E0, $10, $A2, $00, $A5, $27, $10, $03
  .byte $CA, $29, $7F

sub_b31_C994:
  .byte $85, $27, $B0, $0D, $69, $01, $8D, $06, $20, $A5, $26, $8D, $06, $20, $4C, $61
  .byte $CA

sub_b31_C9A5:
  .byte $8D, $06, $20, $A5, $26, $8D, $06, $20, $8A, $51, $FE, $8D, $07, $20, $C8, $8A
  .byte $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE
  .byte $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07
  .byte $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8
  .byte $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51
  .byte $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D
  .byte $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20
  .byte $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $E6, $27, $A5, $27, $8D, $06, $20, $A5
  .byte $26, $8D, $06, $20, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07
  .byte $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D

sub_b31_CA42:
  .byte $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20
  .byte $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8

sub_b31_CA61:
  .byte $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51
  .byte $FE, $8D

sub_b31_CA73:
  .byte $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20
  .byte $C8
data_b31_CA84:
  .byte $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51, $FE, $8D, $07, $20, $C8, $8A, $51
  .byte $FE, $8D, $07, $20

sub_b31_CA98:
  .byte $60

sub_b31_CA99:
  .byte $20, $2D, $CB, $20, $58, $FC, $4C, $36, $CB

sub_b31_CAA2:
  .byte $48, $20, $2D, $CB, $68, $20, $F0, $FD, $48, $20, $36, $CB, $68, $60

sub_b31_CAB0:
  .byte $48, $20, $2D, $CB, $68, $20, $14, $F4, $4C, $36, $CB

sub_b31_CABB:
  .byte $20, $2D, $CB, $20, $42, $FC, $4C, $36, $CB

sub_b31_CAC4:
  .byte $48, $20, $2D, $CB, $68, $20, $09, $C0, $4C, $36, $CB

sub_b31_CACF:
  .byte $20, $2D, $CB, $20, $0C, $FD, $48, $20, $36, $CB, $68, $60
  .byte $20, $2D, $CB, $20, $E0, $82, $4C, $36
loc_b31_CAE3:
  .byte $CB, $A9, $8D

sub_b31_CAE6:
  .byte $9D, $00, $02, $20, $2D, $CB, $20, $70
  .byte $9F, $4C, $36, $CB

sub_b31_CAF2:
  .byte $20, $2D, $CB, $A9, $CA, $48, $A9, $FD, $48, $6C, $75, $9D, $20, $B4, $9D, $4C
  .byte $36, $CB
  .byte $A9, $AD, $8D, $C8, $03, $8D

sub_b31_CB0A:
  .byte $00, $50, $A9, $00, $85, $FE, $85, $26
  .byte $A9, $84, $85

sub_b31_CB15:
  .byte $FF, $A0, $00, $A2, $70, $86, $27, $B1
  .byte $FE, $91, $26, $C8, $D0, $F9, $E6, $FF
  .byte $E8, $E0, $80, $90, $F0, $4C, $36, $CB

sub_b31_CB2D:
  lda #$80

sub_b31_CB2F:
  sta bank_shadow
  sta REG_BANK
  rts

sub_b31_CB36:
  lda #$AA
  sta bank_shadow
  sta REG_BANK
  rts
  .byte $69, $DB, $9D, $F4, $04, $A0

sub_b31_CB45:
  .byte $05, $4E, $16, $05, $6E, $15, $05, $6E
  .byte $14, $05, $6E, $13, $05, $88, $D0, $F1
  .byte $CA, $10
loc_b31_CB57:
  .byte $DF, $E8, $BD, $F4, $04, $F0, $09, $E0
  .byte $06, $90, $F6, $A9, $00, $9D, $F4, $04
  .byte $8E, $F3, $04, $60, $C9, $A0, $D0, $06
  .byte $E0, $00, $D0, $14, $38, $60, $C9, $C1
  .byte $90, $FA, $C9, $DB, $90, $0A, $C9, $FB
  .byte $B0

; 把 PRG 块 29 里的 4KB 程序复制到 RAM $7000-$7FFF：先写 $5000=$2D（块 29 出现在 $8000 窗口），从 $9400-$A3FF 逐页复制到 $7000-$7FFF；再写 $5000=0（回到块 0）并跳到 $7000 运行。复制之后 RAM 里的程序不再依赖窗口切换。
boot_copy_ram_program:
  lda #$00
  sta nmi_hook_mode
  lda #$2D
  sta REG_BANK
  lda #$94
  sta var_001C
  lda #$00
  sta var_001B
  sta var_0026
  tay
  ldx #$70
loc_b31_CB97:
  stx var_0027
loc_b31_CB99:
  lda (var_001B),y
  sta (var_0026),y
  iny
  bne loc_b31_CB99

sub_b31_CBA0:
  inc var_001C
  inx
  cpx #$80
  bcc loc_b31_CB97
  lda #$00

sub_b31_CBA9:
  sta REG_BANK
  jmp ram_program_start
  .byte $18

sub_b31_CBB0:
  lda #$AD
  sta bank_shadow
  sta REG_BANK
  lda #$80
  sta var_001C

sub_b31_CBBC:
  lda #$00
  sta var_001B
  sta var_0026
  tay
  ldx #$70

sub_b31_CBC5:
  stx var_0027

sub_b31_CBC7:
  lda (var_001B),y
  sta (var_0026),y
  iny
  bne sub_b31_CBC7
loc_b31_CBCE:
  inc var_001C
  inx
  cpx #$80
  bcc sub_b31_CBC5
  lda #$80
  sta bank_shadow
  sta REG_BANK
  jmp ram_program_start
  .byte $14, $05, $2E, $15, $05, $0E, $21, $05
  .byte $2E, $22, $05, $2E, $23, $05, $CA, $D0

; 复位初始化：清除显存任务队列的计数（$03C5-$03C7），然后进入 boot_copy_ram_program。
reset_init_state:
  lda #$00
  sta var_03C5
  sta var_03C6
  sta vram_job_count
  jmp boot_copy_ram_program
  .byte $2E, $15

sub_b31_CC00:
  .byte $A0, $42

sub_b31_CC02:
  .byte $B9, $B2, $CC, $99, $FF, $02, $88
loc_b31_CC09:
  .byte $D0, $F7, $4C, $00, $03

sub_b31_CC0E:
  .byte $20, $3C, $CC

sub_b31_CC11:
  .byte $20, $ED, $FD

sub_b31_CC14:
  .byte $4C, $51, $CC
  .byte $20, $3C, $CC, $20, $0C, $FD, $4C, $51
  .byte $CC

sub_b31_CC20:
  .byte $20, $3C, $CC

sub_b31_CC23:
  .byte $20, $6A, $FD

sub_b31_CC26:
  .byte $4C, $51, $CC, $20, $D5, $DA, $08, $48, $20, $3C, $CC, $AD, $C7, $03, $D0, $FB
  .byte $20, $51, $CC, $68, $28, $60

sub_b31_CC3C:
  .byte $08

sub_b31_CC3D:
  .byte $48

sub_b31_CC3E:
  .byte $20, $9A, $CC

sub_b31_CC41:
  .byte $A9, $80

sub_b31_CC43:
  .byte $8D, $C8, $03

sub_b31_CC46:
  .byte $8D, $00, $50

sub_b31_CC49:
  .byte $A9, $90

sub_b31_CC4B:
  .byte $8D, $00, $20

sub_b31_CC4E:
  .byte $68

sub_b31_CC4F:
  .byte $28

sub_b31_CC50:
  .byte $60

sub_b31_CC51:
  .byte $08

sub_b31_CC52:
  .byte $48

sub_b31_CC53:
  .byte $A9, $10

sub_b31_CC55:
  .byte $8D, $00, $20

sub_b31_CC58:
  .byte $AD, $C2, $03

sub_b31_CC5B:
  .byte $8D, $C8, $03

sub_b31_CC5E:
  .byte $8D, $00, $50

sub_b31_CC61:
  .byte $68

sub_b31_CC62:
  .byte $28

sub_b31_CC63:
  .byte $60
  .byte $20, $17, $CC, $C9, $80, $D0, $13, $AD
  .byte $FB, $03, $8D, $01, $02, $AD, $FC, $03
  .byte $8D, $02, $02, $A9, $00, $8D, $03, $02
  .byte $A9, $7F, $60, $20, $B7, $00, $D0, $03
  .byte $4C, $49, $D8, $20, $67, $DD, $20, $52
  .byte $E7, $20, $1A, $D6
loc_b31_CC90:
  .byte $38, $A5, $9B, $E9, $01, $A4, $9C, $4C
  .byte $50, $D8

sub_b31_CC9A:
  .byte $2C, $02, $20

sub_b31_CC9D:
  .byte $30, $06

sub_b31_CC9F:
  .byte $2C, $02, $20

sub_b31_CCA2:
  .byte $10, $FB

sub_b31_CCA4:
  .byte $60

sub_b31_CCA5:
  .byte $2C, $02, $20

sub_b31_CCA8:
  .byte $30, $FB

sub_b31_CCAA:
  .byte $60
  .byte $A9, $A0, $20, $0E, $CC, $4C, $24, $ED
  .byte $A9, $10, $8D, $00, $20, $A9, $00, $85

sub_b31_CCBB:
  .byte $06, $A9, $C0, $85, $07, $A0, $00, $A9
  .byte $80, $8D, $C8, $03, $8D, $00, $50, $B1
  .byte $06, $99, $00, $7F, $C8, $D0, $F8, $AD
  .byte $C2, $03, $8D, $C8, $03, $8D, $00, $50
  .byte $B9, $00, $7F, $91, $06, $C8, $D0, $F8
  .byte $E6, $07, $D0, $DB, $A2, $00, $BD, $00
  .byte $56, $E8, $E8, $E0, $10, $D0, $F7, $4C
  .byte $28, $F1, $A9, $10, $8D, $00, $20, $4C
  .byte $4D, $CD, $00, $00, $00, $A0, $A0, $20
  .byte $12, $CD, $D0, $F9, $69, $FE, $B0, $F5
  .byte $A0

sub_b31_CD0C:
  .byte $3A, $20, $12, $CD, $C8, $C8, $88, $D0
  .byte $FD, $90, $05, $A0, $59, $88, $D0, $FD
  .byte $48, $A5, $2F, $49, $01, $85, $2F, $8D
  .byte $00, $51, $68, $A0, $4E, $CA, $60, $A2
  .byte $08, $48, $20, $39, $CD, $68, $2A, $A0
  .byte $5A, $CA, $D0, $F5, $60, $20, $3C

sub_b31_CD3B:
  .byte $CD, $88, $AD, $00, $55, $29, $04, $45
  .byte $2F, $F0, $F6, $45, $2F, $85, $2F, $C0
  .byte $80, $60, $A9, $40, $20, $00, $CD, $A0
  .byte $4A, $A2, $00, $41, $3C, $48, $A1, $3C
  .byte $20, $6D, $CD, $20, $BA, $FC, $A0, $40
  .byte $68, $90, $EE, $A0, $45, $20, $6D, $CD
  .byte $F0, $4B, $A2, $10, $0A

sub_b31_CD70:
  .byte $20, $0D, $CD, $D0, $FA, $60, $A9, $10
  .byte $8D, $00
loc_b31_CD7A:
  .byte $20, $20, $39, $CD, $A9, $16, $20, $00
  .byte $CD, $85, $2E, $20, $39, $CD, $A0, $36
  .byte $20, $3C, $CD, $B0, $F9, $20, $3C, $CD
  .byte $A0, $5B, $20, $2B, $CD, $81, $3C, $45
  .byte $2E, $85, $2E, $20, $BA, $FC, $A0, $53
  .byte $90, $F0, $20, $2B, $CD, $C5, $2E, $F0
  .byte $0D, $A9, $C5, $20, $0E, $CC, $A9, $D2
  .byte $20, $0E
loc_b31_CDB4:
  .byte $CC, $20, $0E, $CC, $A9, $87, $20, $0E
  .byte $CC, $A9, $90, $8D, $00, $20, $60, $4C
  .byte $90, $F3, $C9, $8D, $F0, $0A, $CE, $E0
  .byte $67, $D0, $2A, $A9, $8D, $20, $DF, $CD
  .byte $A9, $8A, $20, $DF, $CD, $AD, $E1, $67
  .byte $4C, $91, $F1, $48, $48, $20, $38, $F8
  .byte $F0, $FB, $68, $20, $2B, $FB, $20, $25
  .byte $F2, $A9, $01, $8D, $00, $51, $A9, $03
  .byte $8D, $00, $51, $68, $60, $4C, $94, $F1
  .byte $67, $68, $A8, $A9

sub_b31_CE00:
  .byte $48

sub_b31_CE01:
  .byte $A9, $10
loc_b31_CE03:
  .byte $8D, $00, $20

sub_b31_CE06:
  .byte $AD, $C2, $03

sub_b31_CE09:
  .byte $8D, $C8, $03

sub_b31_CE0C:
  .byte $8D, $00, $50

sub_b31_CE0F:
  .byte $68

sub_b31_CE10:
  .byte $60

sub_b31_CE11:
  .byte $48

sub_b31_CE12:
  .byte $2C, $02, $20

sub_b31_CE15:
  .byte $30
loc_b31_CE16:
  .byte $FB

sub_b31_CE17:
  .byte $2C, $02, $20

sub_b31_CE1A:
  .byte $10, $FB

sub_b31_CE1C:
  .byte $A9, $80

sub_b31_CE1E:
  .byte $8D, $C8, $03

sub_b31_CE21:
  .byte $8D, $00, $50

sub_b31_CE24:
  .byte $A9, $90

sub_b31_CE26:
  .byte $8D, $00, $20

sub_b31_CE29:
  .byte $68

sub_b31_CE2A:
  .byte $60
loc_b31_CE2B:
  .byte $20
loc_b31_CE2C:
  .byte $00, $CE, $20, $65, $D6, $85, $33, $85
  .byte $D8, $4C, $D2, $D7, $20, $00, $CE, $4C
  .byte $65, $D8, $20, $00, $CE, $4C, $3C, $D4

sub_b31_CE44:
  .byte $20, $00, $CE

sub_b31_CE47:
  .byte $4C, $F2, $D4, $20, $11, $CE, $4C, $69
  .byte $FF

sub_b31_CE50:
  .byte $20, $00, $CE

sub_b31_CE53:
  .byte $20, $24, $ED

sub_b31_CE56:
  .byte $20, $11, $CE

sub_b31_CE59:
  .byte $60
loc_b31_CE5A:
  .byte $EA, $EA, $EA, $6C, $FA, $00, $A2, $00
  .byte $BD, $6E, $CE, $20, $0E, $CC, $E8, $E0
  .byte $48, $D0, $F5, $60, $CC, $CD, $D3, $CF
  .byte $C6, $D4, $C2, $C1, $D3, $C9, $C3, $A0
  .byte $D6, $B2, $AE, $B1, $B6, $87, $8D, $A8
  .byte $C3, $A9, $C3, $EF, $F0, $F9, $D2, $E9
  .byte $E7, $E8, $F4, $A0, $A0, $CC, $E9, $F5
  .byte $A0
loc_b31_CE93:
  .byte $CD, $E9, $EE, $A0, $B1, $B9, $B9, $B3
  .byte $AE, $B9, $87, $8D, $FF, $33, $4C, $FF
  .byte $50, $72, $FF, $49, $68, $FF, $3C, $46
  .byte $BA, $A0, $FF, $41, $75, $A0, $FF, $43
  .byte $71, $87, $8D, $DB, $4C, $28, $F1, $28
  .byte $F1, $A5, $3E
loc_b31_CEBE:
  .byte $F0, $22, $AD, $E2, $67, $D0, $1C, $A5
  .byte $36, $8D, $E3, $67, $A5, $37, $8D, $E4
  .byte $67, $A9, $FF, $8D, $E2, $67, $A9, $40
  .byte $8D, $E0
data_b31_CED8:
  .byte $67, $A9, $C3, $85, $36, $A9, $CD, $4C
  .byte $A1, $F6, $AD, $E2, $67, $F0, $FA, $A9
  .byte $00, $8D, $E2, $67, $AD, $E3, $67, $85
  .byte $36, $AD, $E4, $67, $85, $37, $60, $FA
  .byte $18, $69, $C0, $85, $D6, $A5, $D7, $69
data_b31_CF00:
  .byte $A9, $3F, $8D, $06, $20, $A9, $00, $8D
  .byte $06, $20, $A8, $20, $35, $FB, $B9, $C0
  .byte $03, $8D, $07, $20, $C8, $C0, $20, $D0
  .byte $F8, $20, $35, $FB, $A9, $70, $8D, $00
  .byte $50, $8D, $00, $54, $A9, $00, $8D, $00
  .byte $50, $A9, $10, $8D, $00, $54, $A9, $F0
  .byte $8D, $00, $54, $A9, $80, $8D, $00, $54
  .byte $A9, $C0, $8D, $00, $54, $20, $35, $FB
  .byte $20, $40, $FB, $A9, $20, $8D, $06, $20
  .byte $A9, $00, $8D, $06

sub_b31_CF4C:
  .byte $20, $A2, $10, $A0, $00, $A9, $20, $8D
  .byte $07, $20, $C8, $D0, $FA, $CA, $D0, $F7
  .byte $A9, $23, $8D, $06, $20, $A9

sub_b31_CF62:
  .byte $C0, $8D, $06, $20, $A2, $40, $A9, $FF
  .byte $8D, $07, $20, $CA, $D0, $FA, $A9, $27
  .byte $8D, $06, $20, $A9, $C0, $8D

sub_b31_CF78:
  .byte $06, $20, $A2, $40, $A9, $FF, $8D, $07
  .byte $20, $CA, $D0, $FA, $A9, $90, $8D, $00
  .byte $20, $A9, $08, $8D, $01, $20, $A9, $00
  .byte $8D, $05, $20, $8D, $05, $20, $A9, $C0
  .byte $85, $EA, $A9, $00, $85, $E4, $A0, $02
  .byte $99, $39, $05, $88, $10, $FA, $8C, $3C
  .byte $05, $8C, $3D, $05, $4C, $00, $F8

sub_b31_CFAF:
  .byte $A9, $00, $85, $20, $85, $22, $A9, $10, $85, $23, $A9, $1F, $85, $21, $4C, $0C
  .byte $C0
  .byte $0D, $0D, $0D, $0D, $0D, $0D, $0D, $0D
  .byte $0D, $0D, $0D, $0D, $0D, $30, $30, $30
  .byte $0D, $0D, $0D, $0D, $0D, $0D, $0D, $0D
  .byte $0D, $0D, $0D, $0D, $0D, $0D, $0D, $26

sub_b31_CFE0:
  sei
  cld
  ldx #$FF
  txs
  sta var_4017
  jsr sub_b31_F660
  jsr ppu_disable_rendering
  ldx #$00

sub_b31_CFF0:
  lda data_b31_CF00,x
  sta var_0300,x
  inx
  bne sub_b31_CFF0
  jmp var_0300
  .byte $20, $20, $CD, $01, $6F, $D8, $65, $D7
  .byte $F8, $DC, $94, $D9, $B1, $DB, $30, $F3
  .byte $D8, $DF, $E1, $DB, $C8, $DE, $C8, $DE
  .byte $E4, $F1, $5F, $CE, $D4, $F1, $C8, $DE
  .byte $C8, $DE, $C8, $DE, $C8, $DE, $C8, $DE
  .byte $E8, $F6, $FD, $F6, $C8, $DE, $C8, $DE
  .byte $E6, $F7, $87, $F1, $C8, $DE, $C8, $DE
  .byte $74, $F7, $6C, $F2, $6E, $F2, $C8, $DE
  .byte $C8, $DE, $49, $CE, $C8, $DE, $6A, $D9
  .byte $55, $F2, $85, $F2, $A5, $F2, $CA, $F2
  .byte $17, $F3, $BB, $F3, $9E, $F3, $61, $F2
  .byte $45, $DA, $3D, $D9, $11, $D9, $C8, $D9
  .byte $7E, $CC, $F4, $03, $20, $D9, $6A, $D9
  .byte $DB, $D9, $6D, $D8, $EB, $D9, $83

sub_b31_D06B:
  .byte $E7, $C8, $D8, $AF, $D8, $12, $E3, $7A
  .byte $E7, $28, $CC, $95, $D8, $A4, $D6, $69
  .byte $D6, $9F, $DB, $48, $D6, $90, $EB, $23

sub_b31_D083:
  .byte $EC, $AF, $EB, $0A, $00, $DE, $E2, $C8
  .byte $DE, $C8, $DE, $FF, $E2, $8D, $EE, $AE
  .byte $EF, $41, $E9, $09, $EF, $EA, $EF, $F1
  .byte $EF, $3A
loc_b31_D09D:
  .byte $F0, $9E, $F0, $64, $E7, $D6, $E6, $C5
  .byte $E3, $07, $E7, $E5, $E6, $46, $E6, $5A
  .byte $E6

sub_b31_D0AE:
  .byte $86, $E6, $91, $E6, $79, $C0, $E7, $79
  .byte $A9, $E7, $7B, $81

sub_b31_D0BA:
  .byte $E9, $7B, $68, $EA, $7D, $96, $EE, $50
  .byte $54, $DF, $46, $4E, $DF, $7F, $CF, $EE
  .byte $7F, $97, $DE, $64, $64, $DF, $45

sub_b31_D0D1:
  .byte $4E, $C4, $46, $4F, $D2, $4E, $45, $58
  .byte $D4, $44, $41, $54, $C1, $49, $4E, $50
  .byte $55, $D4, $44, $45, $CC, $44, $49, $CD
  .byte $52
loc_b31_D0EA:
  .byte $45, $41, $C4, $47, $D2, $43, $52
loc_b31_D0F1:
  .byte $44, $C1, $50, $52, $A3, $56, $45
loc_b31_D0F8:
  .byte $D2, $43, $41, $4C, $CC, $50, $4C, $4F
  .byte $D4, $48, $4C, $49, $CE, $56, $4C, $49
  .byte $CE, $48
loc_b31_D10A:
  .byte $47, $52, $B2, $48, $47, $D2, $48, $43
  .byte $4F, $4C, $4F, $52, $BD, $48, $50, $4C
  .byte $4F
loc_b31_D11B:
  .byte $D4, $44, $52, $41

sub_b31_D11F:
  .byte $D7, $58, $44
loc_b31_D122:
  .byte $52, $41, $D7, $48, $54, $41, $C2, $48
  .byte $4F, $4D, $C5, $52, $4F, $54, $BD, $53

sub_b31_D132:
  .byte $43, $41, $4C, $45, $BD, $53, $48, $4C
  .byte $4F, $41, $C4, $54, $52, $41, $43, $C5
  .byte $4E, $4F, $54, $52, $41, $43, $C5, $4E
  .byte $4F, $52, $4D, $41

sub_b31_D14E:
  .byte $CC, $49, $4E, $56, $45, $52, $53, $C5
  .byte $4C, $4D, $4D, $4F, $CE, $43, $4F, $4C
  .byte $4F, $52, $BD, $50, $4F, $D0, $56, $54
  .byte $41, $C2, $48, $49, $4D, $45, $4D, $BA
  .byte $4C, $4F, $4D, $45

sub_b31_D172:
  .byte $4D, $BA, $4F, $4E, $45, $52

sub_b31_D178:
  .byte $D2, $52, $45, $53, $55, $4D, $C5, $52
  .byte $45

sub_b31_D181:
  .byte $43, $41, $4C, $CC, $53, $54, $4F, $52
  .byte $C5, $53, $50, $45, $45, $44

sub_b31_D18F:
  .byte $BD, $4C, $45, $D4, $47, $4F, $54

sub_b31_D196:
  .byte $CF, $52, $55, $CE, $49, $C6, $52, $45
  .byte $53, $54, $4F, $52, $C5, $A6, $47, $4F
  .byte $53, $55, $C2, $52, $45, $54, $55, $52
  .byte $CE, $52
loc_b31_D1B0:
  .byte $45, $CD, $53, $54, $4F, $D0, $4F, $CE
  .byte $57, $41, $49, $D4, $4C, $4F, $41, $C4
  .byte $53, $41, $56, $C5, $44, $45, $C6, $50
  .byte $4F, $4B, $C5, $50, $52, $49, $4E, $D4
  .byte $43, $4F, $4E, $D4, $4C, $49, $53, $D4
  .byte $43, $4C, $45, $41, $D2, $47, $45, $D4
  .byte $4E, $45, $D7, $54, $41, $42, $A8, $54
  .byte $CF, $46, $CE, $53, $50, $43, $A8, $54
  .byte $48, $45, $CE, $41, $D4, $4E, $4F, $D4
  .byte $53, $54, $45, $D0

sub_b31_D1FC:
  .byte $AB, $AD, $AA, $AF, $DE, $41, $4E, $C4
  .byte $4F, $D2, $BE, $BD

sub_b31_D208:
  .byte $BC, $53, $47, $CE, $49, $4E, $D4, $41
  .byte $42, $D3, $55, $53, $D2, $46, $52
data_b31_D217:
  .byte $C5, $53, $43, $52, $4E, $A8, $50, $44
  .byte $CC, $50, $4F, $D3, $53, $51, $D2, $52
  .byte $4E, $C4, $4C, $4F, $C7, $45, $58, $D0
  .byte $43, $4F, $D3, $53, $49, $CE, $54, $41
  .byte $CE, $41, $54, $CE, $50, $45, $45, $CB
  .byte $4C, $45
loc_b31_D241:
  .byte $CE, $53, $54, $52, $A4, $56, $41, $CC
  .byte $41, $53, $C3, $43, $48, $52, $A4, $4C
  .byte $45, $46, $54, $A4, $52, $49, $47, $48
  .byte $54, $A4, $4D, $49, $44, $A4, $00, $4E
  .byte $45, $58, $54, $20, $57, $49, $54, $48
  .byte $4F
loc_b31_D26A:
  .byte $55, $54, $20, $46, $4F, $D2, $53, $59
  .byte $4E, $54, $41, $D8, $52, $45, $54, $55
  .byte $52, $4E, $20, $57, $49, $54, $48, $4F
  .byte $55, $54, $20, $47, $4F, $53, $55, $C2
  .byte $4F, $55, $54, $20, $4F, $46, $20, $44
  .byte $41, $54, $C1, $49, $4C, $4C, $45, $47
  .byte $41, $4C, $20, $51, $55, $41, $4E, $54
  .byte $49, $54, $D9, $4F, $56, $45, $52, $46
  .byte $4C, $4F, $D7, $4F, $55, $54, $20, $4F
  .byte $46, $20, $4D, $45, $4D, $4F, $52, $D9
  .byte $55, $4E, $44

sub_b31_D2BD:
  .byte $45, $46, $27, $44, $20, $53, $54, $41
  .byte $54, $45, $4D, $45, $4E, $D4, $42, $41
  .byte $44, $20, $53, $55, $42, $53, $43, $52
  .byte $49, $50, $D4, $52, $45, $44, $49, $4D
  .byte $27, $44, $20, $41, $52, $52, $41, $D9
  .byte $44, $49, $56, $49, $53, $49, $4F, $4E
  .byte $20, $42, $59, $20, $5A
loc_b31_D2F2:
  .byte $45, $52, $CF, $49, $4C, $4C, $45, $47
  .byte $41, $4C, $20, $44, $49, $52, $45, $43
  .byte $D4, $54, $59, $50, $45, $20, $4D, $49
  .byte $53, $4D, $41, $54, $43, $C8, $53, $54
  .byte $52, $49, $4E, $47, $20, $54, $4F, $4F
  .byte $20, $4C, $4F, $4E, $C7, $46, $4F, $52
  .byte $4D, $55, $4C, $41, $20, $54, $4F, $4F
  .byte $20, $43, $4F, $4D, $50, $4C, $45, $D8
  .byte $43, $41, $4E, $27, $54, $20, $43, $4F
  .byte $4E, $54, $49, $4E, $55, $C5, $55, $4E
  .byte $44, $45, $46, $27, $44, $20, $46, $55
  .byte $4E, $43, $54, $49, $4F, $CE, $20, $45
  .byte $52, $52, $4F, $52, $07, $00, $20, $49
  .byte $4E, $20, $00, $0D, $42, $52, $45, $41
  .byte $4B, $07, $00

sub_b31_D365:
  .byte $BA, $E8, $E8, $E8, $E8, $BD, $01, $01, $C9, $81, $D0, $21, $A5, $86, $D0, $0A
  .byte $BD, $02, $01, $85, $85, $BD, $03, $01, $85, $86, $DD, $03, $01, $D0, $07, $A5
  .byte $85, $DD, $02, $01, $F0, $07
  .byte $8A, $18, $69, $12, $AA

sub_b31_D390:
  .byte $D0, $D8

sub_b31_D392:
  .byte $60

sub_b31_D393:
  .byte $20, $E3, $D3, $85, $6D, $84, $6E, $38, $A5, $96, $E5, $9B, $85, $5E, $A8, $A5
  .byte $97, $E5, $9C, $AA, $E8, $98, $F0, $23, $A5, $96, $38, $E5, $5E, $85, $96, $B0
  .byte $03
loc_b31_D3B4:
  .byte $C6, $97, $38

sub_b31_D3B7:
  .byte $A5, $94, $E5, $5E, $85, $94, $B0, $08
  .byte $C6, $95, $90, $04

sub_b31_D3C3:
  .byte $B1, $96, $91, $94

sub_b31_D3C7:
  .byte $88

sub_b31_D3C8:
  .byte $D0, $F9, $B1, $96, $91, $94

sub_b31_D3CE:
  .byte $C6, $97, $C6, $95, $CA, $D0

sub_b31_D3D4:
  .byte $F2, $60

sub_b31_D3D6:
  .byte $0A, $69, $36, $B0, $35, $85, $5E, $BA, $E4, $5E, $90, $2E, $60

sub_b31_D3E3:
  .byte $C4, $70, $90, $28
  .byte $D0, $04, $C5, $6F, $90, $22, $48, $A2
  .byte $09, $98, $48, $B5, $93, $CA, $10, $FA
  .byte $20, $84, $E4, $A2, $F7, $68, $95, $9D
  .byte $E8, $30, $FA, $68, $A8, $68, $C4, $70
  .byte $90, $06, $D0, $05, $C5, $6F, $B0, $01

sub_b31_D40F:
  .byte $60
  .byte $A2, $4D, $24, $D8, $10, $03, $4C, $E9
  .byte $F2, $20, $FB, $DA, $20, $5A, $DB, $BD
  .byte $60, $D2, $48, $20, $5C, $DB, $E8, $68
  .byte $10, $F5, $20, $83, $D6, $A9, $50, $A0
  .byte $D3, $20, $3A, $DB, $A4, $76, $C8, $F0
  .byte $03, $20, $19, $ED

sub_b31_D43C:
  .byte $20, $FB, $DA

sub_b31_D43F:
  .byte $A2, $DD

sub_b31_D441:
  .byte $20, $2E, $D5

sub_b31_D444:
  .byte $86, $B8

sub_b31_D446:
  .byte $84, $B9

sub_b31_D448:
  .byte $46, $D8
data_b31_D44A:
  .byte $20, $B1, $00

sub_b31_D44D:
  .byte $AA

sub_b31_D44E:
  .byte $F0, $EC, $A2, $FF, $86, $76, $90, $06, $20, $59, $D5, $4C
loc_b31_D45A:
  .byte $05, $D8

sub_b31_D45C:
  .byte $A6, $AF, $86, $69, $A6, $B0, $86, $6A, $20, $0C, $DA

sub_b31_D467:
  .byte $20, $59, $D5, $84, $0F, $20, $1A, $D6, $90, $44
  .byte $A0, $01, $B1, $9B, $85, $5F, $A5, $69
  .byte $85, $5E, $A5, $9C, $85, $61, $A5, $9B
  .byte $88, $F1, $9B, $18, $65, $69, $85, $69
  .byte $85, $60, $A5, $6A, $69, $FF, $85, $6A
  .byte $E5, $9C, $AA, $38, $A5, $9B, $E5, $69
  .byte $A8, $B0, $03, $E8, $C6, $61, $18, $65
  .byte $5E, $90, $03, $C6, $5F, $18, $B1, $5E
  .byte $91, $60, $C8, $D0, $F9, $E6, $5F, $E6
  .byte $61, $CA, $D0, $F2

sub_b31_D4B5:
  .byte $AD, $00, $02, $F0, $38, $A5, $73, $A4, $74, $85, $6F, $84, $70, $A5, $69, $85
  .byte $96, $65, $0F
loc_b31_D4C8:
  .byte $85, $94
loc_b31_D4CA:
  .byte $A4, $6A, $84, $97, $90, $01
  .byte $C8

sub_b31_D4D1:
  .byte $84
loc_b31_D4D2:
  .byte $95, $20, $93, $D3, $A5, $50, $A4, $51, $8D, $FE, $01, $8C, $FF, $01, $A5, $6D
  .byte $A4, $6E, $85, $69, $84, $6A, $A4, $0F

sub_b31_D4EA:
  .byte $B9, $FB, $01, $88, $91, $9B, $D0, $F8

sub_b31_D4F2:
  .byte $20, $65, $D6

sub_b31_D4F5:
  .byte $A5, $67

sub_b31_D4F7:
  .byte $A4, $68

sub_b31_D4F9:
  .byte $85, $5E

sub_b31_D4FB:
  .byte $84, $5F

sub_b31_D4FD:
  .byte $18

sub_b31_D4FE:
  .byte $A0, $01

sub_b31_D500:
  .byte $B1, $5E

sub_b31_D502:
  .byte $D0, $0B

sub_b31_D504:
  .byte $A5, $69

sub_b31_D506:
  .byte $85, $AF

sub_b31_D508:
  .byte $A5, $6A
loc_b31_D50A:
  .byte $85, $B0

sub_b31_D50C:
  .byte $4C, $3C, $D4

sub_b31_D50F:
  .byte $A0, $04

sub_b31_D511:
  .byte $C8

sub_b31_D512:
  .byte $B1, $5E

sub_b31_D514:
  .byte $D0, $FB

sub_b31_D516:
  .byte $C8

sub_b31_D517:
  .byte $98

sub_b31_D518:
  .byte $65, $5E

sub_b31_D51A:
  .byte $AA

sub_b31_D51B:
  .byte $A0, $00

sub_b31_D51D:
  .byte $91, $5E

sub_b31_D51F:
  .byte $A5, $5F
data_b31_D521:
  .byte $69
data_b31_D522:
  .byte $00

sub_b31_D523:
  .byte $C8

sub_b31_D524:
  .byte $91, $5E

sub_b31_D526:
  .byte $86, $5E

sub_b31_D528:
  .byte $85, $5F

sub_b31_D52A:
  .byte $90, $D2
  .byte $A2, $81

sub_b31_D52E:
  .byte $86, $33

sub_b31_D530:
  .byte $20, $20, $CC

sub_b31_D533:
  .byte $E0, $EF

sub_b31_D535:
  .byte $90, $02
  .byte $A2, $EF

sub_b31_D539:
  .byte $A9, $00

sub_b31_D53B:
  .byte $9D, $00, $02

sub_b31_D53E:
  .byte $8A

sub_b31_D53F:
  .byte $F0, $0B

sub_b31_D541:
  .byte $BD, $FF, $01, $29, $7F, $9D, $FF, $01, $CA, $D0, $F5

sub_b31_D54C:
  .byte $A9, $00

sub_b31_D54E:
  .byte $A2, $FF

sub_b31_D550:
  .byte $A0, $01

sub_b31_D552:
  .byte $60
  .byte $20, $17, $CC, $29, $7F, $60

sub_b31_D559:
  .byte $A6, $B8, $CA, $A0, $04, $84, $13, $24, $D6, $10, $08
  .byte $68, $68, $20, $65, $D6, $4C, $D2, $D7

sub_b31_D56C:
  .byte $E8

sub_b31_D56D:
  .byte $BD, $00, $02, $24, $13, $70, $04, $C9, $20, $F0, $F4, $85, $0E, $C9, $22, $F0
  .byte $74, $70, $4D, $C9, $3F, $D0, $04
  .byte $A9, $BA, $D0, $45

sub_b31_D588:
  .byte $C9, $30, $90, $04, $C9, $3C, $90, $3D

sub_b31_D590:
  .byte $84, $AD, $A9, $D0, $85, $9D, $A9, $CF, $85, $9E, $A0, $00, $84, $0F, $88, $86
  .byte $B8, $CA

sub_b31_D5A2:
  .byte $C8, $D0, $02, $E6, $9E

sub_b31_D5A7:
  .byte $E8

sub_b31_D5A8:
  .byte $BD, $00, $02, $C9, $20, $F0, $F8, $38, $F1, $9D, $F0, $EE, $C9, $80, $D0, $41
loc_b31_D5B8:
  .byte $05, $0F, $C9, $C5, $D0, $0D
  .byte $BD, $01, $02, $C9, $4E, $F0, $34, $C9
  .byte $4F, $F0, $30, $A9, $C5

sub_b31_D5CB:
  .byte $A4, $AD

sub_b31_D5CD:
  .byte $E8, $C8, $99, $FB, $01, $B9, $FB, $01, $F0, $39, $38, $E9, $3A, $F0, $04, $C9
  .byte $49, $D0, $02

sub_b31_D5E0:
  .byte $85, $13

sub_b31_D5E2:
  .byte $38, $E9, $78, $D0, $86
  .byte $85, $0E, $BD, $00, $02, $F0, $DF, $C5
  .byte $0E, $F0, $DB, $C8, $99, $FB, $01, $E8
  .byte $D0, $F0

sub_b31_D5F9:
  .byte $A6, $B8, $E6, $0F

sub_b31_D5FD:
  .byte $B1, $9D, $C8, $D0, $02, $E6, $9E

sub_b31_D604:
  .byte $0A, $90, $F6, $B1, $9D, $D0, $9D, $BD, $00, $02, $10, $BB

sub_b31_D610:
  .byte $99, $FD, $01, $C6, $B9, $A9, $FF, $85, $B8, $60

sub_b31_D61A:
  .byte $A5, $67, $A6, $68, $A0, $01, $85, $9B, $86, $9C, $B1, $9B, $F0, $1F, $C8, $C8
  .byte $A5, $51, $D1, $9B, $90, $18, $F0, $03
  .byte $88, $D0, $09

sub_b31_D635:
  .byte $A5, $50, $88, $D1, $9B, $90, $0C
  .byte $F0, $0A, $88, $B1, $9B, $AA, $88, $B1
  .byte $9B, $B0, $D7

sub_b31_D647:
  .byte $18

sub_b31_D648:
  .byte $60
  .byte $D0, $FD

sub_b31_D64B:
  lda #$00
  sta var_00D6
  tay
  sta (var_0067),y
  iny
  sta (var_0067),y
  lda var_0067
  adc #$02
  sta var_0069
  sta var_00AF
  lda var_0068
  adc #$00
  sta var_006A
  sta var_00B0

sub_b31_D665:
  jsr sub_b31_D697

sub_b31_D668:
  lda #$00

sub_b31_D66A:
  bne loc_b31_D696

sub_b31_D66C:
  lda var_0073

sub_b31_D66E:
  ldy var_0074

sub_b31_D670:
  sta var_006F

sub_b31_D672:
  sty var_0070

sub_b31_D674:
  lda var_0069

sub_b31_D676:
  ldy var_006A

sub_b31_D678:
  sta var_006B

sub_b31_D67A:
  sty var_006C

sub_b31_D67C:
  sta var_006D

sub_b31_D67E:
  sty var_006E

sub_b31_D680:
  jsr sub_b31_D849

sub_b31_D683:
  ldx #$55

sub_b31_D685:
  stx var_0052

sub_b31_D687:
  pla

sub_b31_D688:
  tay

sub_b31_D689:
  pla

sub_b31_D68A:
  ldx #$F8

sub_b31_D68C:
  txs

sub_b31_D68D:
  pha

sub_b31_D68E:
  tya

sub_b31_D68F:
  pha

sub_b31_D690:
  lda #$00

sub_b31_D692:
  sta var_007A

sub_b31_D694:
  sta var_0014
loc_b31_D696:
  rts

sub_b31_D697:
  clc

sub_b31_D698:
  lda var_0067

sub_b31_D69A:
  adc #$FF

sub_b31_D69C:
  sta var_00B8

sub_b31_D69E:
  lda var_0068

sub_b31_D6A0:
  adc #$FF

sub_b31_D6A2:
  sta var_00B9

sub_b31_D6A4:
  rts
  .byte $90, $0A, $F0, $08
  .byte $C9, $C9, $F0, $04, $C9, $2C, $D0, $E5

sub_b31_D6B1:
  .byte $20, $0C, $DA, $20, $1A, $D6, $20, $B7, $00, $F0, $10
  .byte $C9, $C9, $F0, $04, $C9, $2C, $D0, $84
  .byte $20, $B1, $00, $20, $0C, $DA, $D0, $CA

sub_b31_D6CC:
  .byte $68, $68, $A5, $50, $05, $51, $D0, $06, $A9, $FF, $85, $50, $85, $51, $A0, $01
  .byte $B1, $9B, $F0, $44, $20, $58, $D8, $20, $FB, $DA, $C8, $B1, $9B, $AA, $C8, $B1
  .byte $9B, $C5, $51, $D0, $04
  .byte $E4, $50, $F0, $02

sub_b31_D6F5:
  .byte $B0, $2D, $84, $85, $20, $72, $F6, $A9, $20

sub_b31_D6FE:
  .byte $A4, $85, $29, $7F

sub_b31_D702:
  .byte $20, $5C, $DB, $A5, $24, $C9, $21, $90, $07
  .byte $20, $FB, $DA, $A9, $05, $85, $24

sub_b31_D712:
  .byte $C8, $B1, $9B, $D0, $1D
  .byte $A8, $B1, $9B, $AA, $C8, $B1, $9B, $86
  .byte $9B, $85, $9C, $D0, $B6, $A9, $0D, $20
  .byte $5C, $DB, $4C, $D2, $D7

sub_b31_D72C:
  .byte $C8, $D0, $02, $E6, $9E

sub_b31_D731:
  .byte $B1, $9D, $60

sub_b31_D734:
  .byte $10, $CC, $38, $E9, $7F, $AA, $84, $85

sub_b31_D73C:
  .byte $A0, $D0, $84, $9D, $A0, $CF, $84, $9E, $A0, $FF

sub_b31_D746:
  .byte $CA, $F0, $07

sub_b31_D749:
  .byte $20, $2C, $D7, $10, $FB, $30, $F6

sub_b31_D750:
  .byte $A9, $20, $20, $5C, $DB

sub_b31_D755:
  .byte $20, $2C, $D7, $30, $05, $20, $5C, $DB, $D0, $F6

sub_b31_D75F:
  .byte $20, $5C, $DB, $A9

sub_b31_D763:
  .byte $20, $D0, $98, $A9
loc_b31_D767:
  .byte $80, $85, $14, $20, $46, $DA, $20, $65, $D3, $D0, $05
  .byte $8A, $69, $0F, $AA, $9A

sub_b31_D777:
  .byte $68, $68, $A9, $09, $20, $D6, $D3, $20, $A3, $D9, $18, $98, $65, $B8, $48, $A5
  .byte $B9, $69, $00, $48, $A5, $76, $48, $A5, $75, $48, $A9, $C1

sub_b31_D793:
  .byte $20, $C0, $DE, $20, $6A, $DD, $20, $67, $DD, $A5, $A2, $09, $7F, $25, $9E, $85
  .byte $9E, $A9, $AF, $A0, $D7, $85, $5E, $84, $5F, $4C, $20, $DE

sub_b31_D7AF:
  .byte $A9, $13, $A0, $E9, $20, $F9, $EA, $20, $B7, $00, $C9, $C7, $D0, $06
  .byte $20, $B1, $00, $20, $67, $DD

sub_b31_D7C3:
  .byte $20, $82, $EB, $20, $15, $DE

sub_b31_D7C9:
  .byte $A5, $86, $48, $A5, $85, $48, $A9, $81, $48

sub_b31_D7D2:
  .byte $BA, $86, $F8, $20, $58, $D8, $A5, $B8, $A4, $B9, $A6, $76, $E8, $F0, $04, $85
  .byte $79, $84, $7A

sub_b31_D7E5:
  .byte $A0, $00, $B1, $B8, $D0, $57, $A0, $02, $B1, $B8, $18, $F0, $34, $C8, $B1, $B8
  .byte $85, $75, $C8, $B1, $B8, $85, $76, $98, $65, $B8, $85, $B8, $90, $02
  .byte $E6, $B9

sub_b31_D805:
  .byte $24, $F2, $10, $14
  .byte $A6, $76, $E8, $F0, $0F, $A9, $23, $20
  .byte $5C, $DB, $A6, $75, $A5, $76, $20, $24
  .byte $ED, $20, $57, $DB

sub_b31_D81D:
  .byte $20, $B1, $00, $20, $28, $D8, $4C, $D2, $D7

sub_b31_D826:
  .byte $F0, $62

sub_b31_D828:
  .byte $F0, $2D, $E9, $80, $90, $11, $C9, $40, $B0, $14, $0A, $A8, $B9, $01, $D0, $48
  .byte $B9, $00, $D0, $48, $4C, $B1, $00
  .byte $4C, $46, $DA

sub_b31_D842:
  .byte $C9, $3A, $F0, $BF
  .byte $4C, $C9, $DE

sub_b31_D849:
  .byte $38

sub_b31_D84A:
  .byte $A5, $67

sub_b31_D84C:
  .byte $E9, $01

sub_b31_D84E:
  .byte $A4, $68

sub_b31_D850:
  .byte $B0, $01
  .byte $88

sub_b31_D853:
  .byte $85, $7D

sub_b31_D855:
  .byte $84, $7E

sub_b31_D857:
  .byte $60

sub_b31_D858:
  .byte $20, $7C, $F6, $C9, $83, $F0, $01, $60
  .byte $20, $53, $D5, $A2, $FF, $24, $D8, $10
  .byte $03, $4C, $E9, $F2, $C9, $03, $B0, $01
  .byte $18, $D0, $3C, $A5, $B8, $A4, $B9, $A6
  .byte $76, $E8, $F0, $0C, $85, $79, $84, $7A
  .byte $A5, $75, $A4, $76, $85, $77, $84, $78
  .byte $68, $68

sub_b31_D88A:
  .byte $A9, $5D, $A0, $D3, $90, $03
  .byte $4C, $31, $D4

sub_b31_D893:
  .byte $4C, $3C, $D4
  .byte $D0, $17, $A2, $D2, $A4, $7A, $D0, $03
  .byte $4C, $12, $D4, $A5, $79, $85, $B8, $84
  .byte $B9, $A5, $77, $A4, $78, $85, $75, $84
  .byte $76, $60, $38, $A5, $AF, $E5, $67, $85
  .byte $50, $A5, $B0, $E5, $68, $85

sub_b31_D8BC:
  .byte $51, $20, $F0, $D8, $20, $F5, $CC, $20
  .byte $01, $D9, $4C, $F5, $CC, $20, $F0, $D8
  .byte $20, $76, $CD, $18, $A5, $67, $65, $50
  .byte $85, $69, $A5, $68, $65, $51, $85, $6A
  .byte $A5, $52, $85, $D6, $20, $01, $D9, $20
  .byte $76, $CD, $24, $D6, $10, $03, $4C, $65
  .byte $D6, $4C

sub_b31_D8EE:
  .byte $F2, $D4, $A9, $50, $A0, $00, $85, $3C
  .byte $84, $3D, $A9

sub_b31_D8F9:
  .byte $52, $85, $3E, $84, $3F, $84, $D6, $60
  .byte $A5, $67, $A4, $68, $85, $3C, $84, $3D
  .byte $A5, $69, $A4, $6A, $85, $3E, $84, $3F
  .byte $60

sub_b31_D912:
  .byte $08, $C6, $76, $28, $D0, $03, $4C, $65, $D6
  .byte $20, $6C, $D6, $4C

sub_b31_D91F:
  .byte $35, $D9, $A9, $03, $20, $D6, $D3, $A5
  .byte $B9, $48
loc_b31_D929:
  .byte $A5, $B8, $48, $A5, $76, $48, $A5, $75
  .byte $48, $A9, $B0, $48, $20, $B7, $00, $20
  .byte $3E, $D9, $4C, $D2, $D7, $20, $0C, $DA
  .byte $20, $A6, $D9, $A5, $76, $C5, $51, $B0
  .byte $0B, $98, $38, $65, $B8, $A6, $B9, $90
  .byte $07, $E8, $B0, $04, $A5, $67, $A6, $68
  .byte $20, $1E, $D6, $90, $1E, $A5, $9B, $E9
  .byte $01, $85, $B8, $A5, $9C, $E9, $00, $85
  .byte $B9, $60, $D0, $FD, $A9, $FF, $85, $85
  .byte $20

sub_b31_D972:
  .byte $65, $D3, $9A, $C9, $B0, $F0, $0B, $A2
  .byte $16
loc_b31_D97B:
  .byte $2C, $A2, $5A, $4C, $12, $D4, $4C, $C9
  .byte $DE, $68, $68, $C0, $42, $F0, $3B, $85
  .byte $75, $68, $85, $76, $68, $85, $B8, $68
  .byte $85

sub_b31_D994:
  .byte $B9, $20, $A3, $D9, $98, $18, $65, $B8
  .byte $85, $B8, $90, $02
loc_b31_D9A0:
  .byte $E6, $B9

sub_b31_D9A2:
  .byte $60

sub_b31_D9A3:
  .byte $A2, $3A, $2C
loc_b31_D9A6:
  .byte $A2, $00, $86, $0D, $A0, $00, $84, $0E, $A5, $0E, $A6, $0D, $85, $0D, $86, $0E

sub_b31_D9B6:
  .byte $B1, $B8, $F0, $E8, $C5, $0E, $F0, $E4, $C8, $C9, $22, $D0, $F3
  .byte $F0, $E9, $68, $68, $68, $60, $20, $7B
  .byte $DD, $20, $B7, $00, $C9, $AB, $F0, $05
  .byte $A9, $C4, $20, $C0, $DE, $A5, $9D, $D0
  .byte $05, $20, $A6, $D9, $F0, $B7, $20, $B7
  .byte $00, $B0, $03, $4C, $3E, $D9, $4C, $28
  .byte $D8, $20, $F8, $E6, $48, $C9, $B0, $F0
  .byte $04, $C9, $AB, $D0, $89, $C6, $A1, $D0
  .byte $04, $68, $4C, $2A, $D8, $20, $B1, $00
  .byte $20, $0C, $DA, $C9, $2C, $F0, $EE, $68

sub_b31_DA0B:
  .byte $60

sub_b31_DA0C:
  .byte $A2, $00, $86, $50, $86, $51

sub_b31_DA12:
  .byte $B0, $F7, $E9, $2F, $85, $0D, $A5, $51, $85, $5E, $C9, $19, $B0, $D4, $A5, $50
  .byte $0A, $26, $5E, $0A, $26, $5E, $65, $50, $85, $50, $A5, $5E, $65, $51, $85, $51
  .byte $06, $50, $26, $51, $A5, $50, $65, $0D, $85, $50, $90, $02
  .byte $E6, $51

sub_b31_DA40:
  .byte $20, $B1, $00, $4C, $12, $DA

sub_b31_DA46:
  .byte $20, $E3, $DF, $85, $85, $84, $86, $A9, $D0, $20, $C0, $DE, $A5, $12, $48, $A5
  .byte $11, $48, $20, $7B, $DD, $68, $2A, $20, $6D, $DD, $D0, $18, $68, $10, $12
  .byte $20, $72, $EB, $20, $0C, $E1, $A0, $00
  .byte $A5, $A0, $91, $85, $C8, $A5, $A1, $91
  .byte $85, $60

sub_b31_DA77:
  .byte $4C, $27, $EB
  .byte $68, $A0, $02, $B1, $A0, $C5, $70, $90
  .byte $17, $D0, $07, $88, $B1, $A0, $C5, $6F
  .byte $90, $0E, $A4, $A1, $C4, $6A, $90, $08
  .byte $D0, $0D, $A5, $A0, $C5, $69, $B0, $07
  .byte $A5, $A0, $A4, $A1, $4C, $B7, $DA, $A0
  .byte $00, $B1, $A0, $20, $D5, $E3, $A5, $8C
  .byte $A4, $8D, $85, $AB, $84, $AC, $20, $D4
  .byte $E5, $A9, $9D, $A0, $00, $85, $8C, $84
  .byte $8D, $20, $35, $E6, $A0, $00, $B1, $8C
  .byte $91, $85, $C8, $B1, $8C, $91, $85, $C8
  .byte $B1

sub_b31_DACB:
  .byte $8C, $91, $85, $60

sub_b31_DACF:
  .byte $20, $3D, $DB, $20, $B7, $00

sub_b31_DAD5:
  .byte $F0, $24, $F0, $29, $C9, $C0, $F0, $39, $C9, $C3, $18, $F0, $34, $C9, $2C, $18
  .byte $F0, $1C, $C9, $3B, $F0, $44, $20, $7B, $DD, $24, $11, $30, $DD, $20, $34, $ED
  .byte $20, $E7, $E3, $4C, $CF, $DA

sub_b31_DAFB:
  .byte $A9, $0D

sub_b31_DAFD:
  .byte $20, $5C, $DB

sub_b31_DB00:
  .byte $49, $FF

sub_b31_DB02:
  .byte $60
  .byte $A5, $24, $C9, $10, $90, $05, $20, $FB
  .byte $DA, $D0, $21, $69, $0F, $29, $F0, $85
  .byte $24, $90, $19, $08, $20, $F5, $E6, $C9
  .byte $29, $F0, $03, $4C, $C9, $DE, $28, $90
  .byte $07, $CA, $8A, $E5, $24, $90, $05, $AA
  .byte $E8, $CA, $D0, $06, $20, $B1, $00, $4C
  .byte $D7, $DA, $20, $57, $DB, $D0, $F2

sub_b31_DB3A:
  .byte $20, $E7, $E3

sub_b31_DB3D:
  .byte $20, $00, $E6

sub_b31_DB40:
  .byte $AA

sub_b31_DB41:
  .byte $A0, $00

sub_b31_DB43:
  .byte $E8

sub_b31_DB44:
  .byte $CA

sub_b31_DB45:
  .byte $F0, $BB

sub_b31_DB47:
  .byte $B1, $5E

sub_b31_DB49:
  .byte $20, $5C, $DB

sub_b31_DB4C:
  .byte $C8

sub_b31_DB4D:
  .byte $C9
loc_b31_DB4E:
  .byte $0D

sub_b31_DB4F:
  .byte $D0, $F3
  .byte $20, $00, $DB, $4C, $44, $DB, $A9, $20
  .byte $2C, $A9, $3F

sub_b31_DB5C:
  .byte $09, $80

sub_b31_DB5E:
  .byte $C9, $A0

sub_b31_DB60:
  .byte $90, $02

sub_b31_DB62:
  .byte $EA

sub_b31_DB63:
  .byte $EA

sub_b31_DB64:
  .byte $20, $0E, $CC

sub_b31_DB67:
  .byte $29, $7F

sub_b31_DB69:
  .byte $48

sub_b31_DB6A:
  .byte $A5, $F1

sub_b31_DB6C:
  .byte $20, $A8, $FC

sub_b31_DB6F:
  .byte $68

sub_b31_DB70:
  .byte $60
  .byte $A5, $15, $F0, $12, $30, $04, $A0, $FF
  .byte $D0, $04, $A5, $7B, $A4, $7C, $85, $75
  .byte $84, $76, $4C, $C9, $DE, $68, $24, $D8
  .byte $10, $05, $A2, $FE, $4C, $E9, $F2, $A9
  .byte $EF, $A0, $DC, $20, $3A, $DB, $A5, $79
  .byte $A4, $7A, $85, $B8, $84, $B9, $60, $20
  .byte $06, $E3, $A2, $01, $A0, $02, $A9, $00
  .byte $8D, $01, $02, $A9, $40, $20, $EB, $DB
  .byte $60, $C9, $22, $D0
loc_b31_DBB5:
  .byte $0E, $20, $81, $DE, $A9, $3B, $20, $C0
  .byte $DE, $20, $3D, $DB, $4C, $C7, $DB, $20
  .byte $5A, $DB, $20, $06, $E3, $A9, $2C, $8D
loc_b31_DBCD:
  .byte $FF, $01, $20, $2C, $D5, $AD, $00, $02
  .byte $C9, $03, $D0, $10, $4C, $63, $D8, $20
  .byte $5A, $DB, $4C, $2C, $D5, $A6, $7D, $A4
  .byte $7E, $A9, $98, $2C, $A9, $00, $85, $15
  .byte $86, $7F, $84, $80, $20, $E3, $DF, $85
  .byte $85, $84, $86, $A5, $B8, $A4, $B9, $85
  .byte $87, $84, $88, $A6, $7F, $A4, $80, $86
  .byte $B8, $84, $B9, $20, $B7, $00, $D0, $1E
  .byte $24, $15, $50, $0E, $20, $64, $CC, $29
  .byte $7F, $8D, $00, $02, $A2, $FF, $A0, $01
  .byte $D0, $08, $30, $7F, $20, $5A, $DB, $20
  .byte $DC, $DB, $86, $B8, $84, $B9, $20, $B1
  .byte $00, $24, $11, $10, $31, $24, $15, $50
  .byte $09, $E8, $86, $B8, $A9, $00, $85, $0D
  .byte $F0, $0C, $85, $0D, $C9, $22, $F0, $07
  .byte $A9, $3A, $85, $0D, $A9, $2C, $18, $85
  .byte $0E, $A5, $B8, $A4, $B9, $69, $00, $90
  .byte $01, $C8, $20, $ED, $E3, $20, $3D, $E7
  .byte $20, $7B, $DA, $4C, $72, $DC, $48, $AD
  .byte $00, $02, $F0, $30, $68, $20, $4A, $EC
  .byte $A5, $12, $20, $63, $DA, $20, $B7, $00
  .byte $F0, $07, $C9, $2C, $F0, $03, $4C, $71
  .byte $DB, $A5, $B8, $A4, $B9, $85, $7F, $84
  .byte $80, $A5, $87, $A4, $88, $85, $B8, $84
  .byte $B9, $20, $B7, $00, $F0, $33, $20, $BE
  .byte $DE, $4C, $F1, $DB, $A5, $15, $D0, $CC
  .byte $4C, $86, $DB, $20, $A3, $D9, $C8, $AA
  .byte $D0, $12, $A2, $2A, $C8, $B1, $B8, $F0
  .byte $5F, $C8, $B1, $B8, $85, $7B, $C8, $B1
  .byte $B8, $C8, $85, $7C, $B1, $B8, $AA, $20
  .byte $98, $D9, $E0, $83, $D0, $DD, $4C, $2B
  .byte $DC, $A5, $7F, $A4, $80, $A6, $15, $10
  .byte $03, $4C, $53, $D8, $A0, $00, $B1, $7F
  .byte $F0, $07, $A9, $DF, $A0, $DC, $4C, $3A
  .byte $DB, $60, $3F, $45, $58, $54, $52, $41
  .byte $20, $49, $47, $4E, $4F, $52, $45, $44
  .byte $0D, $00, $3F, $52, $45, $45, $4E, $54
  .byte $45, $52, $0D, $00

sub_b31_DCF9:
  .byte $D0, $04, $A0, $00, $F0, $03
  .byte $20, $E3, $DF

sub_b31_DD02:
  .byte $85, $85, $84, $86, $20, $65, $D3, $F0, $04
  .byte $A2, $00, $F0, $69

sub_b31_DD0F:
  .byte $9A, $E8, $E8, $E8, $E8, $8A, $E8, $E8, $E8, $E8, $E8, $E8, $86, $60, $A0, $01
  .byte $20, $F9, $EA, $BA, $BD, $09, $01, $85, $A2, $A5, $85, $A4, $86, $20, $BE, $E7
  .byte $20, $27, $EB, $A0, $01, $20, $B4, $EB, $BA
loc_b31_DD38:
  .byte $38, $FD, $09, $01, $F0, $17, $BD, $0F, $01, $85, $75, $BD, $10, $01, $85, $76
  .byte $BD, $12, $01, $85, $B8, $BD, $11, $01, $85, $B9

sub_b31_DD52:
  .byte $4C, $D2, $D7

sub_b31_DD55:
  .byte $8A, $69, $11, $AA, $9A, $20, $B7, $00, $C9, $2C, $D0, $F1
  .byte $20, $B1, $00, $20, $FF, $DC

sub_b31_DD67:
  .byte $20, $7B, $DD

sub_b31_DD6A:
  .byte $18, $24, $38

sub_b31_DD6D:
  .byte $24, $11, $30, $03, $B0, $03, $60
  .byte $B0, $FD, $A2, $A3, $4C, $12, $D4

sub_b31_DD7B:
  .byte $A6, $B8, $D0, $02
  .byte $C6, $B9

sub_b31_DD81:
  .byte $C6, $B8, $A2, $00, $24

sub_b31_DD86:
  .byte $48, $8A, $48, $A9, $01, $20, $D6, $D3, $20, $60, $DE, $A9, $00, $85, $89, $20
  .byte $B7, $00, $38, $E9, $CF, $90, $17
  .byte $C9, $03, $B0, $13, $C9, $01, $2A, $49
  .byte $01, $45, $89, $C5, $89, $90, $61, $85
  .byte $89, $20, $B1, $00, $4C, $98, $DD

sub_b31_DDB4:
  .byte $A6, $89, $D0, $2C, $B0, $7B, $69, $07, $90, $77, $65, $11, $D0, $03
  .byte $4C, $97, $E5

sub_b31_DDC5:
  .byte $69, $FF, $85, $5E, $0A, $65, $5E, $A8, $68, $D9, $B2, $D0, $B0, $67, $20, $6A
  .byte $DD, $48, $20, $FD, $DD, $68, $A4, $87, $10, $17, $AA

sub_b31_DDE0:
  .byte $F0, $56
  .byte $D0, $5F, $46, $11, $8A

sub_b31_DDE7:
  .byte $2A, $A6, $B8, $D0, $02, $C6, $B9, $C6
  .byte $B8, $A0, $1B, $85, $89, $D0, $D7, $D9
  .byte $B2, $D0, $B0, $48, $90, $D9

sub_b31_DDFD:
  .byte $B9, $B4, $D0, $48, $B9, $B3, $D0, $48, $20, $10, $DE

sub_b31_DE08:
  .byte $A5, $89, $4C, $86, $DD
  .byte $4C, $C9, $DE

sub_b31_DE10:
  .byte $A5, $A2, $BE, $B2, $D0

sub_b31_DE15:
  .byte $A8, $68, $85, $5E, $E6, $5E, $68, $85, $5F, $98, $48

sub_b31_DE20:
  .byte $20, $72, $EB, $A5, $A1, $48, $A5, $A0, $48, $A5, $9F, $48, $A5, $9E, $48, $A5
  .byte $9D, $48, $6C, $5E, $00

sub_b31_DE35:
  .byte $A0, $FF, $68

sub_b31_DE38:
  .byte $F0, $23, $C9, $64, $F0, $03, $20, $6A, $DD, $84, $87, $68, $4A, $85, $16, $68
  .byte $85, $A5, $68, $85, $A6, $68, $85, $A7, $68, $85, $A8, $68, $85, $A9, $68, $85
  .byte $AA, $45, $A2, $85, $AB

sub_b31_DE5D:
  .byte $A5, $9D, $60

sub_b31_DE60:
  .byte $A9, $00, $85, $11, $20, $B1, $00, $B0, $03, $4C, $4A, $EC

sub_b31_DE6C:
  .byte $20, $7D, $E0, $B0, $64
  .byte $C9, $2E, $F0, $F4, $C9, $C9, $F0, $55
  .byte $C9, $C8, $F0, $E7, $C9, $22, $D0, $0F
  .byte $A5, $B8, $A4, $B9, $69, $00, $90, $01
  .byte $C8, $20, $E7, $E3, $4C, $3D, $E7, $C9
  .byte $C6, $D0, $10, $A0, $18, $D0, $38, $A5
  .byte $9D, $D0, $03, $A0, $01, $2C, $A0, $00
  .byte $4C, $01, $E3, $C9, $C2, $D0, $03, $4C
  .byte $54, $E3, $C9, $D2, $90, $03, $4C, $0C
  .byte $DF, $20, $BB, $DE, $20, $7B, $DD, $A9
  .byte $29, $2C, $A9, $28, $2C, $A9, $2C

sub_b31_DEC0:
  .byte $A0, $00, $D1, $B8, $D0, $03, $4C, $B1, $00
  .byte $A2, $10, $4C, $12, $D4, $A0, $15, $68
  .byte $68, $4C, $D7, $DD

sub_b31_DED5:
  .byte $20, $E3, $DF, $85, $A0, $84, $A1, $A6, $11, $F0, $05
  .byte $A2, $00, $86, $AC, $60

sub_b31_DEE5:
  .byte $A6, $12, $10, $0D
  .byte $A0, $00, $B1, $A0, $AA, $C8, $B1, $A0
  .byte $A8, $8A, $4C, $F2, $E2

sub_b31_DEF6:
  .byte $4C, $F9, $EA
  .byte $20, $B1, $00, $20, $EC, $F1, $8A, $A4
  .byte $F0, $20, $71, $F8, $A8, $20, $01, $E3
  .byte $4C, $B8, $DE, $C9, $D7, $F0, $E9, $0A
  .byte $48, $AA, $20, $B1, $00, $E0, $CF, $90
  .byte $20, $20, $BB, $DE, $20, $7B, $DD, $20
  .byte $BE, $DE, $20, $6C, $DD, $68, $AA, $A5
  .byte $A1, $48, $A5, $A0, $48, $8A, $48, $20
  .byte $F8, $E6, $68, $A8, $8A, $48, $4C, $3F
  .byte $DF, $20, $B2, $DE, $68

sub_b31_DF3E:
  .byte $A8, $B9, $DC, $CF, $85, $91, $B9, $DD
  .byte $CF, $85, $92, $20, $90, $00, $4C, $6A
  .byte $DD, $A5, $A5, $05, $9D, $D0, $0B, $A5
  .byte $A5, $F0, $04, $A5, $9D, $D0, $03, $A0
  .byte $00, $2C, $A0, $01, $4C, $01, $E3, $20
  .byte $6D, $DD, $B0, $13, $A5, $AA, $09, $7F
  .byte $25, $A6, $85, $A6, $A9, $A5, $A0, $00
  .byte $20, $B2, $EB, $AA, $4C, $B0, $DF, $A9
  .byte $00, $85, $11, $C6, $89, $20, $00, $E6
  .byte $85, $9D, $86, $9E, $84, $9F, $A5, $A8
  .byte $A4, $A9, $20, $04, $E6, $86, $A8, $84
  .byte $A9, $AA, $38, $E5, $9D, $F0, $08, $A9
  .byte $01, $90, $04, $A6, $9D, $A9, $FF, $85
  .byte $A2, $A0, $FF, $E8, $C8, $CA, $D0, $07
  .byte $A6, $A2, $30, $0F, $18, $90, $0C, $B1
  .byte $A8, $D1, $9E, $F0, $EF, $A2, $FF, $B0
  .byte $02, $A2, $01, $E8, $8A, $2A, $25, $16
  .byte $F0, $02, $A9, $01, $4C, $93, $EB, $20
  .byte $FB, $E6, $20, $1E, $FB, $4C, $01, $E3
  .byte $20, $BE, $DE, $AA, $20, $E8, $DF, $20
  .byte $B7, $00, $D0, $F4, $60

sub_b31_DFE3:
  .byte $A2, $00, $20, $B7, $00, $86
loc_b31_DFE9:
  .byte $10, $85, $81, $20, $B7, $00, $20, $7D, $E0, $B0, $03
  .byte $4C, $C9, $DE

sub_b31_DFF7:
  .byte $A2, $00, $86, $11, $86, $12, $4C, $07, $E0

sub_b31_E000:
  .byte $4C, $00, $CC

sub_b31_E003:
  .byte $4C, $3C, $D4, $00

sub_b31_E007:
  .byte $20, $B1, $00, $90, $05, $20, $7D, $E0, $90, $0B
  .byte $AA, $20, $B1, $00, $90, $FB, $20, $7D
  .byte $E0, $B0, $F6

sub_b31_E01C:
  .byte $C9, $24, $D0, $06
  .byte $A9, $FF, $85, $11, $D0, $10

sub_b31_E026:
  .byte $C9, $25, $D0, $13
  .byte $A5, $14, $30, $C6, $A9, $80, $85, $12
  .byte $05, $81, $85, $81, $8A

sub_b31_E037:
  .byte $09, $80, $AA, $20, $B1, $00

sub_b31_E03D:
  .byte $86, $82, $38, $05, $14, $E9, $28, $D0, $03
  .byte $4C, $1E, $E1

sub_b31_E049:
  .byte $24, $14, $30, $02, $70, $F7

sub_b31_E04F:
  .byte $A9, $00, $85, $14, $A5

sub_b31_E054:
  .byte $69, $A6, $6A, $A0, $00, $86, $9C, $85, $9B, $E4, $6C, $D0, $04, $C5, $6B, $F0
  .byte $22, $A5, $81, $D1, $9B, $D0, $08, $A5, $82, $C8, $D1, $9B, $F0, $6C
  .byte $88, $18, $A5, $9B, $69, $07, $90, $E1
  .byte $E8, $D0, $DC

sub_b31_E07D:
  .byte $C9, $41, $90, $05, $E9, $5B, $38, $E9, $A5

sub_b31_E086:
  .byte $60

sub_b31_E087:
  .byte $68, $48, $C9, $D7, $D0, $0F
  .byte $BA, $BD, $02, $01, $C9, $DE, $D0, $07
  .byte $A9, $9A, $A0, $E0, $60, $00, $00

sub_b31_E09C:
  .byte $A5, $6B, $A4, $6C, $85, $9B, $84, $9C, $A5, $6D, $A4, $6E, $85, $96, $84, $97
  .byte $18, $69, $07, $90, $01
  .byte $C8

sub_b31_E0B2:
  .byte $85, $94, $84, $95, $20, $93, $D3, $A5, $94, $A4, $95, $C8, $85, $6B, $84, $6C
  .byte $A0, $00, $A5, $81, $91, $9B, $C8, $A5, $82, $91, $9B, $A9, $00, $C8, $91, $9B
  .byte $C8, $91, $9B, $C8, $91, $9B, $C8, $91, $9B, $C8, $91, $9B

sub_b31_E0DE:
  .byte $A5, $9B, $18, $69, $02, $A4, $9C, $90, $01
  .byte $C8

sub_b31_E0E8:
  .byte $85, $83, $84, $84, $60
  .byte $A5, $0F, $0A, $69, $05, $65, $9B, $A4
  .byte $9C, $90, $01, $C8, $85, $94, $84, $95
  .byte $60, $90, $80, $00, $00, $20, $B1, $00
  .byte $20, $67, $DD, $A5, $A2, $30, $0D, $A5
  .byte $9D, $C9, $90, $90, $09, $A9, $FE, $A0
  .byte $E0, $20, $B2, $EB, $D0, $7E, $4C, $F2
  .byte $EB, $A5, $14, $D0, $47, $A5, $10, $05
  .byte $12, $48, $A5, $11, $48, $A0, $00, $98
  .byte $48, $A5, $82, $48, $A5, $81, $48, $20
  .byte $02, $E1, $68, $85, $81, $68, $85, $82
  .byte $68, $A8, $BA, $BD, $02, $01, $48, $BD
  .byte $01, $01, $48, $A5, $A0, $9D, $02, $01
  .byte $A5, $A1, $9D, $01, $01, $C8, $20, $B7
  .byte $00, $C9, $2C, $F0, $D2, $84, $0F, $20
  .byte $B8, $DE, $68, $85, $11, $68, $85, $12
  .byte $29, $7F, $85, $10, $A6, $6B, $A5, $6C
  .byte $86, $9B, $85, $9C, $C5, $6E, $D0, $04
  .byte $E4, $6D, $F0, $3F, $A0, $00, $B1, $9B
  .byte $C8, $C5, $81, $D0, $06, $A5, $82, $D1
  .byte $9B, $F0, $16, $C8, $B1, $9B, $18, $65
  .byte $9B, $AA, $C8, $B1, $9B, $65, $9C, $90
  .byte $D7, $A2, $6B, $2C, $A2, $35, $4C, $12
  .byte $D4, $A2

sub_b31_E19F:
  .byte $78, $A5, $10, $D0, $F7, $A5, $14, $F0
  .byte $02, $38, $60, $20, $ED, $E0, $A5, $0F
  .byte $A0, $04, $D1, $9B, $D0

sub_b31_E1B4:
  .byte $E1, $4C, $4B, $E2, $A5, $14, $F0, $05
  .byte $A2, $2A, $4C, $12, $D4, $20, $ED, $E0
  .byte $20, $E3, $D3, $A9, $00, $A8, $85, $AE
  .byte $A2, $05, $A5, $81, $91, $9B, $10, $01
  .byte $CA, $C8, $A5, $82, $91, $9B, $10, $02
  .byte $CA, $CA, $86, $AD, $A5, $0F, $C8, $C8
  .byte $C8, $91, $9B, $A2, $0B, $A9, $00, $24
  .byte $10, $50, $08, $68, $18, $69, $01, $AA
  .byte $68, $69, $00, $C8, $91, $9B, $C8, $8A
  .byte $91, $9B, $20, $AD, $E2, $86, $AD, $85
  .byte $AE, $A4, $5E, $C6, $0F, $D0, $DC, $65
  .byte $95, $B0, $5D, $85, $95, $A8, $8A, $65
  .byte $94, $90, $03, $C8, $F0, $52, $20, $E3
  .byte $D3, $85, $6D, $84, $6E, $A9, $00, $E6
  .byte $AE, $A4, $AD, $F0, $05, $88

sub_b31_E22A:
  .byte $91, $94, $D0, $FB, $C6, $95, $C6, $AE
  .byte $D0, $F5, $E6, $95, $38, $A5, $6D, $E5

sub_b31_E23A:
  .byte $9B, $A0, $02, $91, $9B, $A5, $6E, $C8
  .byte $E5, $9C, $91, $9B, $A5, $10, $D0, $62
  .byte $C8, $B1, $9B, $85

sub_b31_E24E:
  .byte $0F, $A9, $00, $85, $AD, $85, $AE, $C8
  .byte $68, $AA, $85, $A0

sub_b31_E25A:
  .byte $68, $85, $A1, $D1, $9B, $90, $0E, $D0
  .byte $06, $C8, $8A, $D1, $9B, $90, $07, $4C
  .byte $96, $E1, $4C, $10, $D4, $C8, $A5, $AE
  .byte $05, $AD, $18, $F0, $0A, $20, $AD, $E2
  .byte $8A, $65, $A0

sub_b31_E27D:
  .byte $AA, $98, $A4, $5E, $65, $A1, $86, $AD
  .byte $C6, $0F, $D0, $CA, $85, $AE, $A2
loc_b31_E28C:
  .byte $05, $A5, $81
loc_b31_E28F:
  .byte $10, $01, $CA, $A5, $82, $10, $02, $CA
  .byte $CA, $86, $64, $A9, $00, $20, $B6, $E2
  .byte $8A, $65, $94, $85, $83, $98, $65, $95
  .byte $85, $84, $A8, $A5, $83, $60, $84, $5E
  .byte $B1, $9B, $85, $64, $88, $B1, $9B, $85
  .byte $65, $A9, $10, $85, $99, $A2, $00, $A0
  .byte $00, $8A, $0A, $AA, $98, $2A, $A8, $B0
  .byte $A4, $06, $AD, $26, $AE, $90, $0B, $18
  .byte $8A, $65, $64, $AA, $98, $65, $65, $A8
  .byte $B0, $93, $C6, $99, $D0, $E3, $60, $A5
  .byte $11, $F0, $03, $20, $00, $E6, $20, $84
  .byte $E4, $38, $A5, $6F, $E5, $6D, $A8, $A5
  .byte $70, $E5, $6E, $A2, $00, $86, $11, $85
  .byte $9E, $84, $9F, $A2, $90, $4C, $9B, $EB
  .byte $A4, $24, $A9, $00, $38, $F0, $EC, $A6
  .byte $76, $E8, $D0, $A1, $A2, $95, $2C, $A2
  .byte $E0, $4C, $12, $D4, $20, $41, $E3, $20
  .byte $06, $E3, $20, $BB, $DE, $A9, $80, $85
  .byte $14, $20, $E3, $DF, $20, $6A, $DD, $20
  .byte $B8, $DE, $A9, $D0, $20, $C0, $DE, $48
  .byte $A5, $84, $48, $A5, $83, $48, $A5, $B9
  .byte $48, $A5, $B8, $48, $20, $95, $D9, $4C
  .byte $AF, $E3, $A9, $C2, $20, $C0, $DE, $09
  .byte $80, $85, $14, $20, $EA, $DF, $85, $8A
  .byte $84, $8B, $4C, $6A, $DD, $20, $41, $E3
  .byte $A5, $8B, $48, $A5, $8A, $48, $20, $B2
  .byte $DE, $20, $6A, $DD, $68, $85, $8A, $68
  .byte $85
loc_b31_E368:
  .byte $8B, $A0, $02, $B1, $8A, $85, $83, $AA
  .byte $C8, $B1, $8A, $F0, $99, $85, $84, $C8
  .byte $B1, $83, $48, $88, $10, $FA, $A4, $84
  .byte $20, $2B, $EB, $A5, $B9, $48, $A5, $B8
  .byte $48, $B1, $8A, $85, $B8, $C8, $B1, $8A
  .byte $85, $B9, $A5, $84, $48, $A5, $83, $48
  .byte $20, $67, $DD, $68, $85, $8A, $68, $85
  .byte $8B, $20, $B7, $00, $F0, $03, $4C, $C9
  .byte $DE, $68, $85, $B8, $68, $85, $B9, $A0
  .byte $00, $68, $91, $8A, $68, $C8, $91, $8A
  .byte $68, $C8, $91, $8A, $68, $C8, $91, $8A
  .byte $68, $C8, $91, $8A, $60, $20, $6A, $DD
  .byte $A0, $00, $20, $36, $ED, $68, $68, $A9
  .byte $FF, $A0, $00, $F0, $12, $A6, $A0, $A4
  .byte $A1, $86, $8C, $84, $8D, $20, $52, $E4
  .byte $86, $9E, $84, $9F, $85, $9D, $60

sub_b31_E3E7:
  .byte $A2, $22

sub_b31_E3E9:
  .byte $86, $0D

sub_b31_E3EB:
  .byte $86, $0E

sub_b31_E3ED:
  .byte $85, $AB

sub_b31_E3EF:
  .byte $84, $AC

sub_b31_E3F1:
  .byte $85, $9E

sub_b31_E3F3:
  .byte $84, $9F

sub_b31_E3F5:
  .byte $A0, $FF

sub_b31_E3F7:
  .byte $C8

sub_b31_E3F8:
  .byte $B1, $AB

sub_b31_E3FA:
  .byte $F0, $0C

sub_b31_E3FC:
  .byte $C5, $0D

sub_b31_E3FE:
  .byte $F0, $04

sub_b31_E400:
  .byte $C5, $0E

sub_b31_E402:
  .byte $D0, $F3
  .byte $C9, $22, $F0, $01

sub_b31_E408:
  .byte $18

sub_b31_E409:
  .byte $84, $9D

sub_b31_E40B:
  .byte $98

sub_b31_E40C:
  .byte $65, $AB

sub_b31_E40E:
  .byte $85, $AD

sub_b31_E410:
  .byte $A6, $AC

sub_b31_E412:
  .byte $90, $01
  .byte $E8

sub_b31_E415:
  .byte $86, $AE

sub_b31_E417:
  .byte $A5, $AC

sub_b31_E419:
  .byte $F0, $04

sub_b31_E41B:
  .byte $C9, $02

sub_b31_E41D:
  .byte $D0, $0B
  .byte $98, $20, $D5, $E3, $A6, $AB, $A4, $AC
  .byte $20, $E2, $E5

sub_b31_E42A:
  .byte $A6, $52

sub_b31_E42C:
  .byte $E0, $5E

sub_b31_E42E:
  .byte $D0, $05
  .byte $A2, $BF, $4C, $12, $D4

sub_b31_E435:
  .byte $A5, $9D

sub_b31_E437:
  .byte $95, $00

sub_b31_E439:
  .byte $A5, $9E

sub_b31_E43B:
  .byte $95, $01

sub_b31_E43D:
  .byte $A5, $9F

sub_b31_E43F:
  .byte $95, $02

sub_b31_E441:
  .byte $A0, $00

sub_b31_E443:
  .byte $86, $A0

sub_b31_E445:
  .byte $84, $A1

sub_b31_E447:
  .byte $88

sub_b31_E448:
  .byte $84, $11

sub_b31_E44A:
  .byte $86, $53

sub_b31_E44C:
  .byte $E8

sub_b31_E44D:
  .byte $E8

sub_b31_E44E:
  .byte $E8

sub_b31_E44F:
  .byte $86, $52

sub_b31_E451:
  .byte $60
  .byte $46, $13, $48, $49, $FF, $38, $65, $6F
  .byte $A4, $70, $B0, $01, $88, $C4, $6E, $90
  .byte $11, $D0, $04, $C5, $6D, $90, $0B, $85
  .byte $6F, $84, $70, $85, $71, $84, $72, $AA
  .byte $68, $60, $A2, $4D, $A5, $13, $30, $B8
  .byte $20, $84, $E4, $A9, $80, $85, $13, $68
  .byte $D0, $D0, $A6, $73, $A5, $74, $86, $6F
  .byte $85, $70, $A0, $00, $84, $8B, $A5, $6D
  .byte $A6, $6E, $85, $9B, $86, $9C, $A9, $55
  .byte $A2, $00, $85, $5E, $86, $5F, $C5, $52
  .byte $F0, $05, $20, $23, $E5, $F0, $F7, $A9
  .byte $07, $85, $8F, $A5, $69, $A6, $6A, $85
  .byte $5E, $86, $5F, $E4, $6C, $D0, $04, $C5
  .byte $6B, $F0, $05, $20, $19, $E5, $F0, $F3
  .byte $85, $94, $86, $95, $A9, $03, $85, $8F
  .byte $A5, $94, $A6, $95, $E4, $6E, $D0, $07
  .byte $C5, $6D, $D0, $03, $4C, $62, $E5, $85
  .byte $5E, $86, $5F, $A0, $00, $B1, $5E, $AA
  .byte $C8, $B1, $5E, $08, $C8, $B1, $5E, $65
  .byte $94, $85, $94, $C8, $B1, $5E, $65, $95
  .byte $85, $95, $28, $10, $D3, $8A, $30, $D0
  .byte $C8, $B1, $5E, $A0, $00, $0A, $69, $05
  .byte $65, $5E, $85, $5E, $90, $02, $E6, $5F
  .byte $A6, $5F, $E4, $95, $D0, $04, $C5, $94
  .byte $F0, $BA, $20, $23, $E5, $F0, $F3, $B1
  .byte $5E, $30, $35, $C8, $B1, $5E, $10, $30
  .byte $C8, $B1, $5E, $F0, $2B, $C8, $B1, $5E
  .byte $AA, $C8, $B1, $5E, $C5, $70, $90, $06
  .byte $D0, $1E, $E4, $6F, $B0, $1A, $C5, $9C
  .byte $90, $16, $D0, $04, $E4, $9B, $90, $10
  .byte $86, $9B, $85, $9C, $A5, $5E, $A6, $5F
  .byte $85, $8A, $86, $8B, $A5, $8F, $85, $91
  .byte $A5, $8F, $18, $65, $5E, $85, $5E, $90
  .byte $02, $E6, $5F, $A6, $5F, $A0, $00, $60
  .byte $A6, $8B, $F0, $F7, $A5, $91, $29, $04
  .byte $4A, $A8, $85, $91, $B1, $8A, $65, $9B
  .byte $85, $96, $A5, $9C, $69, $00, $85, $97
  .byte $A5, $6F, $A6, $70, $85, $94, $86, $95
  .byte $20, $9A, $D3, $A4, $91, $C8, $A5, $94
  .byte $91, $8A, $AA, $E6, $95, $A5, $95, $C8
  .byte $91, $8A, $4C, $88, $E4, $A5, $A1, $48
  .byte $A5, $A0, $48, $20, $60, $DE, $20, $6C
  .byte $DD, $68, $85, $AB, $68, $85, $AC, $A0
  .byte $00, $B1, $AB, $18, $71, $A0, $90, $05
  .byte $A2, $B0, $4C, $12, $D4, $20, $D5, $E3
  .byte $20, $D4, $E5, $A5, $8C, $A4, $8D, $20
  .byte $04, $E6, $20, $E6, $E5, $A5, $AB, $A4
  .byte $AC, $20, $04, $E6, $20, $2A, $E4, $4C
  .byte $95, $DD, $A0, $00, $B1, $AB, $48, $C8
  .byte $B1, $AB, $AA, $C8, $B1, $AB, $A8, $68
  .byte $86, $5E, $84, $5F, $A8, $F0, $0A, $48
  .byte $88, $B1, $5E, $91, $71, $98, $D0, $F8
  .byte $68, $18, $65, $71, $85, $71, $90, $02
  .byte $E6, $72, $60, $20, $6C, $DD

sub_b31_E600:
  .byte $A5, $A0

sub_b31_E602:
  .byte $A4, $A1

sub_b31_E604:
  .byte $85, $5E

sub_b31_E606:
  .byte $84, $5F

sub_b31_E608:
  .byte $20, $35, $E6

sub_b31_E60B:
  .byte $08

sub_b31_E60C:
  .byte $A0, $00

sub_b31_E60E:
  .byte $B1, $5E

sub_b31_E610:
  .byte $48

sub_b31_E611:
  .byte $C8

sub_b31_E612:
  .byte $B1, $5E

sub_b31_E614:
  .byte $AA

sub_b31_E615:
  .byte $C8

sub_b31_E616:
  .byte $B1, $5E

sub_b31_E618:
  .byte $A8

sub_b31_E619:
  .byte $68

sub_b31_E61A:
  .byte $28

sub_b31_E61B:
  .byte $D0, $13

sub_b31_E61D:
  .byte $C4, $70

sub_b31_E61F:
  .byte $D0, $0F
  .byte $E4, $6F, $D0, $0B, $48, $18, $65, $6F
  .byte $85, $6F, $90, $02, $E6, $70, $68

sub_b31_E630:
  .byte $86, $5E

sub_b31_E632:
  .byte $84, $5F

sub_b31_E634:
  .byte $60

sub_b31_E635:
  .byte $C4, $54

sub_b31_E637:
  .byte $D0, $0C

sub_b31_E639:
  .byte $C5, $53

sub_b31_E63B:
  .byte $D0, $08

sub_b31_E63D:
  .byte $85, $52

sub_b31_E63F:
  .byte $E9, $03

sub_b31_E641:
  .byte $85, $53

sub_b31_E643:
  .byte $A0, $00

sub_b31_E645:
  .byte $60
  .byte $20, $FB, $E6, $8A, $48, $A9, $01, $20
  .byte $DD, $E3, $68, $A0, $00, $91, $9E, $68
  .byte $68, $4C, $2A, $E4, $20, $B9, $E6, $D1
  .byte $8C, $98, $90, $04, $B1, $8C, $AA, $98
  .byte $48, $8A, $48, $20, $DD, $E3, $A5, $8C
  .byte $A4, $8D, $20, $04, $E6, $68, $A8, $68
  .byte $18, $65, $5E, $85, $5E, $90, $02, $E6
  .byte $5F, $98, $20, $E6, $E5, $4C, $2A, $E4
  .byte $20, $B9, $E6, $18, $F1, $8C, $49, $FF
  .byte $4C, $60, $E6, $A9, $FF, $85, $A1, $20
  .byte $B7, $00, $C9, $29, $F0, $06, $20, $BE
  .byte $DE, $20, $F8, $E6, $20, $B9, $E6, $CA
  .byte $8A, $48, $18, $A2, $00, $F1, $8C, $B0
  .byte $B8, $49, $FF, $C5, $A1, $90, $B3, $A5
  .byte $A1, $B0, $AF, $20, $B8, $DE, $68, $A8
  .byte $68, $85, $91, $68, $68, $68, $AA, $68
  .byte $85, $8C, $68, $85, $8D, $A5, $91, $48
  .byte $98, $48, $A0, $00, $8A, $F0, $1D, $60
  .byte $20, $DC, $E6, $4C, $01, $E3, $20, $FD
  .byte $E5, $A2, $00, $86, $11, $A8, $60, $20
  .byte $DC, $E6, $F0, $08, $A0, $00, $B1, $5E
  .byte $A8, $4C, $01, $E3, $4C, $99, $E1, $20
  .byte $B1, $00, $20, $67, $DD, $20, $08, $E1
  .byte $A6, $A0, $D0, $F0, $A6, $A1, $4C, $B7
  .byte $00, $20, $DC, $E6, $D0, $03, $4C, $4E
  .byte $E8, $A6, $B8, $A4, $B9, $86, $AD, $84
  .byte $AE, $A6, $5E, $86, $B8, $18, $65, $5E
  .byte $85, $60, $A6, $5F, $86, $B9, $90, $01
  .byte $E8, $86, $61, $A0, $00, $B1, $60, $48
  .byte $A9, $00, $91, $60, $20, $B7, $00, $20
  .byte $4A, $EC, $68, $A0, $00, $91, $60, $A6
  .byte $AD, $A4, $AE, $86, $B8, $84, $B9, $60
  .byte $20, $67, $DD, $20, $52, $E7, $20, $BE
  .byte $DE, $4C, $F8, $E6, $A5, $9D, $C9, $91
  .byte $B0, $9A, $20, $F2, $EB, $A5, $A0, $A4
  .byte $A1, $84, $50, $85, $51, $60, $A5, $50
  .byte $48, $A5, $51, $48, $20, $52, $E7, $A0
  .byte $00, $B1, $50, $A8, $68, $85, $51, $68
  .byte $85, $50, $4C, $01, $E3, $20, $46, $E7
  .byte $8A, $A0, $00, $91, $50, $60, $20, $46
  .byte $E7, $86, $85, $A2, $00, $20, $B7, $00
  .byte $F0, $03, $20, $4C, $E7, $86, $86, $A0
  .byte $00, $B1, $50, $45, $86, $25, $85, $F0
  .byte $F8

sub_b31_E79F:
  rts

sub_b31_E7A0:
  lda #$64

sub_b31_E7A2:
  ldy #$EE

sub_b31_E7A4:
  jmp sub_b31_E7BE
  .byte $20, $E3, $E9, $A5, $A2, $49, $FF, $85
  .byte $A2, $45, $AA, $85, $AB, $A5, $9D, $4C
  .byte $C1, $E7

sub_b31_E7B9:
  .byte $20, $F0, $E8

sub_b31_E7BC:
  .byte $90, $3C

sub_b31_E7BE:
  .byte $20, $E3, $E9

sub_b31_E7C1:
  .byte $D0, $03
  .byte $4C, $53, $EB

sub_b31_E7C6:
  .byte $A6, $AC

sub_b31_E7C8:
  .byte $86, $92

sub_b31_E7CA:
  .byte $A2, $A5

sub_b31_E7CC:
  .byte $A5, $A5

sub_b31_E7CE:
  .byte $A8

sub_b31_E7CF:
  .byte $F0, $CE

sub_b31_E7D1:
  .byte $38

sub_b31_E7D2:
  .byte $E5, $9D

sub_b31_E7D4:
  .byte $F0, $24

sub_b31_E7D6:
  .byte $90, $12

sub_b31_E7D8:
  .byte $84, $9D

sub_b31_E7DA:
  .byte $A4, $AA

sub_b31_E7DC:
  .byte $84, $A2

sub_b31_E7DE:
  .byte $49, $FF

sub_b31_E7E0:
  .byte $69, $00

sub_b31_E7E2:
  .byte $A0, $00

sub_b31_E7E4:
  .byte $84, $92

sub_b31_E7E6:
  .byte $A2, $9D

sub_b31_E7E8:
  .byte $D0, $04

sub_b31_E7EA:
  .byte $A0, $00

sub_b31_E7EC:
  .byte $84, $AC

sub_b31_E7EE:
  .byte $C9, $F9

sub_b31_E7F0:
  .byte $30, $C7

sub_b31_E7F2:
  .byte $A8

sub_b31_E7F3:
  .byte $A5, $AC

sub_b31_E7F5:
  .byte $56, $01

sub_b31_E7F7:
  .byte $20, $07, $E9

sub_b31_E7FA:
  .byte $24, $AB

sub_b31_E7FC:
  .byte $10, $57
  .byte $A0, $9D, $E0, $A5, $F0, $02, $A0, $A5
  .byte $38, $49, $FF, $65, $92, $85, $AC, $B9
  .byte $04, $00, $F5, $04, $85, $A1, $B9, $03
  .byte $00, $F5, $03, $85, $A0, $B9, $02, $00
  .byte $F5, $02, $85, $9F, $B9, $01, $00, $F5
  .byte $01, $85, $9E

sub_b31_E829:
  .byte $B0, $03
  .byte $20, $9E, $E8

sub_b31_E82E:
  .byte $A0, $00

sub_b31_E830:
  .byte $98

sub_b31_E831:
  .byte $18

sub_b31_E832:
  .byte $A6, $9E

sub_b31_E834:
  .byte $D0, $4A

sub_b31_E836:
  .byte $A6, $9F

sub_b31_E838:
  .byte $86, $9E

sub_b31_E83A:
  .byte $A6, $A0

sub_b31_E83C:
  .byte $86, $9F

sub_b31_E83E:
  .byte $A6, $A1

sub_b31_E840:
  .byte $86, $A0

sub_b31_E842:
  .byte $A6, $AC

sub_b31_E844:
  .byte $86, $A1

sub_b31_E846:
  .byte $84, $AC

sub_b31_E848:
  .byte $69, $08

sub_b31_E84A:
  .byte $C9, $20

sub_b31_E84C:
  .byte $D0, $E4
  .byte $A9, $00, $85, $9D, $85, $A2, $60

sub_b31_E855:
  .byte $65, $92

sub_b31_E857:
  .byte $85, $AC

sub_b31_E859:
  .byte $A5, $A1

sub_b31_E85B:
  .byte $65, $A9

sub_b31_E85D:
  .byte $85, $A1

sub_b31_E85F:
  .byte $A5, $A0

sub_b31_E861:
  .byte $65, $A8

sub_b31_E863:
  .byte $85, $A0

sub_b31_E865:
  .byte $A5, $9F

sub_b31_E867:
  .byte $65, $A7

sub_b31_E869:
  .byte $85, $9F

sub_b31_E86B:
  .byte $A5, $9E

sub_b31_E86D:
  .byte $65, $A6

sub_b31_E86F:
  .byte $85, $9E

sub_b31_E871:
  .byte $4C, $8D, $E8

sub_b31_E874:
  .byte $69, $01

sub_b31_E876:
  .byte $06, $AC

sub_b31_E878:
  .byte $26, $A1

sub_b31_E87A:
  .byte $26, $A0

sub_b31_E87C:
  .byte $26, $9F

sub_b31_E87E:
  .byte $26, $9E

sub_b31_E880:
  .byte $10, $F2

sub_b31_E882:
  .byte $38

sub_b31_E883:
  .byte $E5, $9D

sub_b31_E885:
  .byte $B0, $C7

sub_b31_E887:
  .byte $49, $FF

sub_b31_E889:
  .byte $69, $01

sub_b31_E88B:
  .byte $85, $9D

sub_b31_E88D:
  .byte $90, $0E

sub_b31_E88F:
  .byte $E6, $9D

sub_b31_E891:
  .byte $F0, $42

sub_b31_E893:
  .byte $66, $9E

sub_b31_E895:
  .byte $66, $9F

sub_b31_E897:
  .byte $66, $A0

sub_b31_E899:
  .byte $66, $A1

sub_b31_E89B:
  .byte $66, $AC

sub_b31_E89D:
  .byte $60
  .byte $A5, $A2, $49, $FF, $85, $A2, $A5, $9E
  .byte $49, $FF, $85, $9E, $A5, $9F, $49, $FF
  .byte $85, $9F, $A5, $A0, $49, $FF, $85, $A0
  .byte $A5, $A1, $49, $FF, $85, $A1, $A5, $AC
  .byte $49, $FF, $85, $AC, $E6, $AC, $D0, $0E
  .byte $E6, $A1, $D0, $0A, $E6, $A0, $D0, $06
  .byte $E6, $9F, $D0, $02, $E6, $9E, $60, $A2
  .byte $45, $4C, $12, $D4, $A2, $61

sub_b31_E8DC:
  .byte $B4, $04

sub_b31_E8DE:
  .byte $84, $AC

sub_b31_E8E0:
  .byte $B4, $03

sub_b31_E8E2:
  .byte $94, $04

sub_b31_E8E4:
  .byte $B4, $02

sub_b31_E8E6:
  .byte $94, $03

sub_b31_E8E8:
  .byte $B4, $01

sub_b31_E8EA:
  .byte $94, $02

sub_b31_E8EC:
  .byte $A4, $A4

sub_b31_E8EE:
  .byte $94, $01

sub_b31_E8F0:
  .byte $69, $08

sub_b31_E8F2:
  .byte $30, $E8

sub_b31_E8F4:
  .byte $F0, $E6

sub_b31_E8F6:
  .byte $E9, $08

sub_b31_E8F8:
  .byte $A8

sub_b31_E8F9:
  .byte $A5, $AC

sub_b31_E8FB:
  .byte $B0, $14

sub_b31_E8FD:
  .byte $16, $01

sub_b31_E8FF:
  .byte $90, $02
  .byte $F6, $01

sub_b31_E903:
  .byte $76, $01

sub_b31_E905:
  .byte $76, $01

sub_b31_E907:
  .byte $76, $02

sub_b31_E909:
  .byte $76, $03

sub_b31_E90B:
  .byte $76, $04

sub_b31_E90D:
  .byte $6A

sub_b31_E90E:
  .byte $C8

sub_b31_E90F:
  .byte $D0, $EC

sub_b31_E911:
  .byte $18

sub_b31_E912:
  .byte $60
  .byte $81, $00, $00, $00, $00, $03, $7F, $5E
  .byte $56, $CB, $79, $80, $13, $9B, $0B, $64
  .byte $80, $76, $38, $93, $16, $82, $38, $AA
  .byte $3B, $20, $80, $35, $04, $F3, $34, $81
  .byte $35, $04, $F3, $34, $80, $80, $00, $00
  .byte $00, $80, $31, $72, $17, $F8, $20, $82
  .byte $EB, $F0, $02, $10, $03, $4C, $99, $E1
  .byte $A5, $9D, $E9, $7F, $48, $A9, $80, $85
  .byte $9D, $A9, $2D, $A0, $E9, $20, $BE, $E7
  .byte $A9, $32, $A0, $E9, $20, $66, $EA, $A9
  .byte $13, $A0, $E9, $20, $A7, $E7, $A9, $18
  .byte $A0, $E9, $20, $5C, $EF, $A9, $37, $A0
  .byte $E9, $20, $BE, $E7, $68, $20, $D5, $EC
  .byte $A9, $3C, $A0, $E9, $20, $E3, $E9, $D0
  .byte $03, $4C, $E2, $E9, $20, $0E, $EA, $A9
  .byte $00, $85, $62, $85, $63, $85, $64, $85
  .byte $65, $A5, $AC, $20, $B0, $E9, $A5, $A1
  .byte $20, $B0, $E9, $A5, $A0, $20, $B0, $E9
  .byte $A5, $9F, $20, $B0, $E9, $A5, $9E, $20
  .byte $B5, $E9, $4C, $E6, $EA, $D0, $03, $4C
  .byte $DA, $E8, $4A, $09, $80, $A8, $90, $19
  .byte $18, $A5, $65, $65, $A9, $85, $65, $A5
  .byte $64, $65, $A8, $85, $64, $A5, $63, $65
  .byte $A7, $85, $63, $A5, $62, $65, $A6, $85
  .byte $62, $66, $62, $66, $63, $66, $64, $66
  .byte $65, $66, $AC, $98, $4A, $D0, $D6, $60

sub_b31_E9E3:
  .byte $85, $5E

sub_b31_E9E5:
  .byte $84, $5F

sub_b31_E9E7:
  .byte $A0, $04

sub_b31_E9E9:
  .byte $B1, $5E

sub_b31_E9EB:
  .byte $85, $A9

sub_b31_E9ED:
  .byte $88

sub_b31_E9EE:
  .byte $B1, $5E

sub_b31_E9F0:
  .byte $85, $A8

sub_b31_E9F2:
  .byte $88

sub_b31_E9F3:
  .byte $B1, $5E

sub_b31_E9F5:
  .byte $85, $A7

sub_b31_E9F7:
  .byte $88

sub_b31_E9F8:
  .byte $B1, $5E

sub_b31_E9FA:
  .byte $85, $AA

sub_b31_E9FC:
  .byte $45, $A2

sub_b31_E9FE:
  .byte $85, $AB

sub_b31_EA00:
  .byte $A5, $AA

sub_b31_EA02:
  .byte $09, $80

sub_b31_EA04:
  .byte $85, $A6

sub_b31_EA06:
  .byte $88

sub_b31_EA07:
  .byte $B1, $5E

sub_b31_EA09:
  .byte $85, $A5

sub_b31_EA0B:
  .byte $A5, $9D

sub_b31_EA0D:
  .byte $60
  .byte $A5, $A5, $F0, $1F, $18, $65, $9D, $90
  .byte $04, $30, $1D, $18, $2C, $10, $14, $69
  .byte $80, $85, $9D, $D0, $03, $4C, $52, $E8
  .byte $A5, $AB, $85, $A2, $60, $A5, $A2, $49
  .byte $FF, $30, $05, $68, $68, $4C, $4E, $E8
  .byte $4C, $D5, $E8

sub_b31_EA39:
  .byte $20, $63, $EB

sub_b31_EA3C:
  .byte $AA

sub_b31_EA3D:
  .byte $F0, $10

sub_b31_EA3F:
  .byte $18

sub_b31_EA40:
  .byte $69, $02

sub_b31_EA42:
  .byte $B0, $F2

sub_b31_EA44:
  .byte $A2, $00

sub_b31_EA46:
  .byte $86, $AB

sub_b31_EA48:
  .byte $20, $CE, $E7

sub_b31_EA4B:
  .byte $E6, $9D

sub_b31_EA4D:
  .byte $F0, $E7

sub_b31_EA4F:
  .byte $60
  .byte $84, $20, $00, $00, $00, $20, $63, $EB
  .byte $A9, $50, $A0, $EA, $A2, $00, $86, $AB
  .byte $20, $F9, $EA, $4C, $69, $EA, $20, $E3
  .byte $E9, $F0, $76, $20, $72, $EB, $A9, $00
  .byte $38, $E5, $9D, $85, $9D, $20, $0E, $EA
  .byte $E6, $9D, $F0, $BA, $A2, $FC, $A9, $01
  .byte $A4, $A6, $C4, $9E, $D0, $10, $A4, $A7
  .byte $C4, $9F, $D0, $0A, $A4, $A8, $C4, $A0
  .byte $D0, $04, $A4, $A9, $C4, $A1, $08, $2A
  .byte $90, $09, $E8, $95, $65, $F0, $32, $10
  .byte $34, $A9, $01, $28, $B0, $0E, $06, $A9
  .byte $26, $A8, $26, $A7, $26, $A6, $B0, $E6
  .byte $30, $CE, $10, $E2, $A8, $A5, $A9, $E5
  .byte $A1, $85, $A9, $A5, $A8, $E5, $A0, $85
  .byte $A8, $A5, $A7, $E5, $9F, $85, $A7, $A5
  .byte $A6, $E5, $9E, $85, $A6, $98, $4C, $A6
  .byte $EA, $A9, $40, $D0, $CE, $0A, $0A, $0A
  .byte $0A, $0A, $0A, $85, $AC, $28, $4C, $E6
  .byte $EA, $A2, $85, $4C, $12, $D4, $A5, $62
  .byte $85, $9E, $A5, $63, $85, $9F, $A5, $64
  .byte $85, $A0, $A5, $65, $85, $A1, $4C, $2E
  .byte $E8

sub_b31_EAF9:
  .byte $85, $5E, $84, $5F, $A0, $04, $B1, $5E, $85, $A1, $88, $B1, $5E, $85, $A0, $88
  .byte $B1, $5E, $85, $9F, $88, $B1, $5E, $85, $A2, $09, $80, $85, $9E, $88, $B1, $5E
  .byte $85, $9D, $84, $AC, $60
  .byte $A2, $98, $2C, $A2, $93, $A0, $00, $F0
  .byte $04

sub_b31_EB27:
  .byte $A6, $85, $A4, $86, $20, $72, $EB, $86, $5E, $84, $5F, $A0, $04, $A5, $A1, $91
  .byte $5E, $88, $A5, $A0, $91, $5E, $88, $A5, $9F, $91, $5E, $88, $A5, $A2, $09, $7F
  .byte $25, $9E, $91, $5E, $88, $A5, $9D, $91, $5E, $84, $AC, $60
  .byte $A5, $AA, $85, $A2, $A2, $05, $B5, $A4
  .byte $95, $9C, $CA, $D0, $F9, $86, $AC, $60

sub_b31_EB63:
  .byte $20, $72, $EB

sub_b31_EB66:
  .byte $A2, $06

sub_b31_EB68:
  .byte $B5, $9C

sub_b31_EB6A:
  .byte $95, $A4

sub_b31_EB6C:
  .byte $CA

sub_b31_EB6D:
  .byte $D0, $F9

sub_b31_EB6F:
  .byte $86, $AC

sub_b31_EB71:
  .byte $60

sub_b31_EB72:
  .byte $A5, $9D

sub_b31_EB74:
  .byte $F0, $FB

sub_b31_EB76:
  .byte $06, $AC

sub_b31_EB78:
  .byte $90, $F7
  .byte $20, $C6, $E8, $D0, $F2, $4C, $8F, $E8

sub_b31_EB82:
  .byte $A5, $9D, $F0, $09, $A5, $A2

sub_b31_EB88:
  .byte $2A

sub_b31_EB89:
  .byte $A9, $FF

sub_b31_EB8B:
  .byte $B0, $02

sub_b31_EB8D:
  .byte $A9, $01

sub_b31_EB8F:
  .byte $60
  .byte $20, $82, $EB

sub_b31_EB93:
  sta var_009E
  lda #$00
  sta var_009F
  ldx #$88
  lda var_009E
  eor #$FF
  rol a

sub_b31_EBA0:
  lda #$00

sub_b31_EBA2:
  sta var_00A1

sub_b31_EBA4:
  sta var_00A0

sub_b31_EBA6:
  stx var_009D

sub_b31_EBA8:
  sta var_00AC

sub_b31_EBAA:
  sta var_00A2

sub_b31_EBAC:
  jmp sub_b31_E829
  .byte $46, $A2, $60

sub_b31_EBB2:
  .byte $85, $60

sub_b31_EBB4:
  .byte $84, $61

sub_b31_EBB6:
  .byte $A0, $00

sub_b31_EBB8:
  .byte $B1, $60

sub_b31_EBBA:
  .byte $C8

sub_b31_EBBB:
  .byte $AA

sub_b31_EBBC:
  .byte $F0, $C4

sub_b31_EBBE:
  .byte $B1, $60

sub_b31_EBC0:
  .byte $45, $A2

sub_b31_EBC2:
  .byte $30, $C2

sub_b31_EBC4:
  .byte $E4, $9D

sub_b31_EBC6:
  .byte $D0, $21

sub_b31_EBC8:
  .byte $B1, $60

sub_b31_EBCA:
  .byte $09, $80

sub_b31_EBCC:
  .byte $C5, $9E

sub_b31_EBCE:
  .byte $D0, $19

sub_b31_EBD0:
  .byte $C8

sub_b31_EBD1:
  .byte $B1, $60

sub_b31_EBD3:
  .byte $C5, $9F

sub_b31_EBD5:
  .byte $D0, $12

sub_b31_EBD7:
  .byte $C8

sub_b31_EBD8:
  .byte $B1, $60

sub_b31_EBDA:
  .byte $C5, $A0

sub_b31_EBDC:
  .byte $D0, $0B, $C8, $A9, $7F, $C5, $AC, $B1, $60, $E5, $A1, $F0, $28

sub_b31_EBE9:
  .byte $A5, $A2

sub_b31_EBEB:
  .byte $90, $02

sub_b31_EBED:
  .byte $49, $FF

sub_b31_EBEF:
  .byte $4C, $88, $EB

sub_b31_EBF2:
  .byte $A5, $9D

sub_b31_EBF4:
  .byte $F0, $4A

sub_b31_EBF6:
  .byte $38

sub_b31_EBF7:
  .byte $E9, $A0

sub_b31_EBF9:
  .byte $24, $A2

sub_b31_EBFB:
  .byte $10, $09
  .byte $AA, $A9, $FF, $85, $A4, $20, $A4, $E8
  .byte $8A

sub_b31_EC06:
  .byte $A2, $9D

sub_b31_EC08:
  .byte $C9, $F9

sub_b31_EC0A:
  .byte $10, $06
  .byte $20, $F0, $E8, $84, $A4

sub_b31_EC11:
  .byte $60

sub_b31_EC12:
  .byte $A8

sub_b31_EC13:
  .byte $A5, $A2

sub_b31_EC15:
  .byte $29, $80

sub_b31_EC17:
  .byte $46, $9E

sub_b31_EC19:
  .byte $05, $9E

sub_b31_EC1B:
  .byte $85, $9E

sub_b31_EC1D:
  .byte $20, $07, $E9

sub_b31_EC20:
  .byte $84, $A4

sub_b31_EC22:
  .byte $60
  .byte $A5, $9D, $C9, $A0, $B0, $20, $20, $F2
  .byte $EB, $84, $AC, $A5, $A2, $84, $A2, $49
  .byte $80, $2A, $A9, $A0, $85, $9D, $A5, $A1
  .byte $85, $0D, $4C, $29, $E8, $85, $9E, $85
  .byte $9F, $85, $A0, $85, $A1, $A8, $60

sub_b31_EC4A:
  .byte $A0, $00, $A2, $0A

sub_b31_EC4E:
  .byte $94, $99, $CA, $10, $FB, $90, $0F
  .byte $C9, $2D, $D0, $04, $86, $A3, $F0, $04
  .byte $C9, $2B, $D0, $05

sub_b31_EC61:
  .byte $20, $B1, $00

sub_b31_EC64:
  .byte $90, $5B, $C9, $2E, $F0, $2E, $C9, $45, $D0, $30
  .byte $20, $B1, $00, $90, $17, $C9, $C9, $F0
  .byte $0E, $C9, $2D, $F0, $0A, $C9, $C8, $F0
  .byte $08, $C9, $2B, $F0, $04, $D0, $07, $66
  .byte $9C, $20, $B1, $00, $90, $5C, $24, $9C
  .byte $10, $0E, $A9, $00, $38, $E5, $9A, $4C
  .byte $A0, $EC, $66, $9B, $24, $9B, $50, $C3

sub_b31_EC9E:
  .byte $A5, $9A, $38, $E5, $99, $85, $9A, $F0, $12
  .byte $10, $09, $20, $55, $EA, $E6, $9A, $D0
  .byte $F9, $F0, $07, $20, $39, $EA, $C6, $9A
  .byte $D0, $F9

sub_b31_ECB9:
  .byte $A5, $A3, $30, $01, $60
  .byte $4C, $D0, $EE

sub_b31_ECC1:
  .byte $48, $24, $9B, $10, $02
  .byte $E6, $99

sub_b31_ECC8:
  .byte $20, $39, $EA, $68, $38, $E9, $30, $20, $D5, $EC, $4C, $61, $EC

sub_b31_ECD5:
  .byte $48, $20, $63, $EB, $68, $20, $93, $EB, $A5, $AA, $45, $A2, $85, $AB, $A6, $9D
  .byte $4C, $C1, $E7
  .byte $A5, $9A, $C9, $0A, $90, $09, $A9, $64
  .byte $24, $9C, $30, $11, $4C, $D5, $E8, $0A
  .byte $0A, $18, $65, $9A, $0A, $18, $A0, $00
  .byte $71, $B8, $38, $E9, $30, $85, $9A, $4C
  .byte $87, $EC, $9B, $3E, $BC, $1F, $FD, $9E
  .byte $6E, $6B, $27, $FD, $9E, $6E, $6B, $28
  .byte $00, $A9, $58, $A0, $D3, $20, $31, $ED
  .byte $A5, $76, $A6, $75

sub_b31_ED24:
  .byte $85, $9E

sub_b31_ED26:
  .byte $86, $9F

sub_b31_ED28:
  .byte $A2, $90

sub_b31_ED2A:
  .byte $38

sub_b31_ED2B:
  .byte $20, $A0, $EB

sub_b31_ED2E:
  .byte $20, $34, $ED

sub_b31_ED31:
  .byte $4C, $3A, $DB

sub_b31_ED34:
  .byte $A0, $01

sub_b31_ED36:
  .byte $A9, $2D

sub_b31_ED38:
  .byte $88

sub_b31_ED39:
  .byte $24, $A2

sub_b31_ED3B:
  .byte $10, $04
  .byte $C8, $99, $FF, $00

sub_b31_ED41:
  .byte $85, $A2

sub_b31_ED43:
  .byte $84, $AD

sub_b31_ED45:
  .byte $C8

sub_b31_ED46:
  .byte $A9, $30

sub_b31_ED48:
  .byte $A6, $9D

sub_b31_ED4A:
  .byte $D0, $03
  .byte $4C, $57, $EE

sub_b31_ED4F:
  .byte $A9, $00

sub_b31_ED51:
  .byte $E0, $80

sub_b31_ED53:
  .byte $F0, $02

sub_b31_ED55:
  .byte $B0, $09
  .byte $A9, $14, $A0, $ED, $20, $7F, $E9, $A9
  .byte $F7

sub_b31_ED60:
  .byte $85, $99

sub_b31_ED62:
  .byte $A9, $0F

sub_b31_ED64:
  .byte $A0, $ED

sub_b31_ED66:
  .byte $20, $B2, $EB

sub_b31_ED69:
  .byte $F0, $1E

sub_b31_ED6B:
  .byte $10, $12

sub_b31_ED6D:
  .byte $A9, $0A

sub_b31_ED6F:
  .byte $A0, $ED

sub_b31_ED71:
  .byte $20, $B2, $EB

sub_b31_ED74:
  .byte $F0, $02

sub_b31_ED76:
  .byte $10, $0E

sub_b31_ED78:
  .byte $20, $39, $EA

sub_b31_ED7B:
  .byte $C6, $99

sub_b31_ED7D:
  .byte $D0, $EE
  .byte $20, $55, $EA, $E6, $99, $D0, $DC

sub_b31_ED86:
  .byte $20, $A0, $E7

sub_b31_ED89:
  .byte $20, $F2, $EB

sub_b31_ED8C:
  .byte $A2, $01

sub_b31_ED8E:
  .byte $A5, $99

sub_b31_ED90:
  .byte $18

sub_b31_ED91:
  .byte $69, $0A

sub_b31_ED93:
  .byte $30, $09

sub_b31_ED95:
  .byte $C9, $0B

sub_b31_ED97:
  .byte $B0, $06

sub_b31_ED99:
  .byte $69, $FF

sub_b31_ED9B:
  .byte $AA

sub_b31_ED9C:
  .byte $A9, $02

sub_b31_ED9E:
  .byte $38

sub_b31_ED9F:
  .byte $E9, $02

sub_b31_EDA1:
  .byte $85, $9A

sub_b31_EDA3:
  .byte $86, $99

sub_b31_EDA5:
  .byte $8A

sub_b31_EDA6:
  .byte $F0, $02

sub_b31_EDA8:
  .byte $10, $13
  .byte $A4, $AD, $A9, $2E, $C8, $99, $FF, $00
  .byte $8A, $F0, $06, $A9, $30, $C8, $99, $FF
  .byte $00, $84, $AD

sub_b31_EDBD:
  .byte $A0, $00

sub_b31_EDBF:
  .byte $A2, $80

sub_b31_EDC1:
  .byte $A5, $A1

sub_b31_EDC3:
  .byte $18

sub_b31_EDC4:
  .byte $79, $6C, $EE

sub_b31_EDC7:
  .byte $85, $A1

sub_b31_EDC9:
  .byte $A5, $A0

sub_b31_EDCB:
  .byte $79, $6B, $EE

sub_b31_EDCE:
  .byte $85, $A0

sub_b31_EDD0:
  .byte $A5, $9F

sub_b31_EDD2:
  .byte $79, $6A, $EE

sub_b31_EDD5:
  .byte $85, $9F

sub_b31_EDD7:
  .byte $A5, $9E

sub_b31_EDD9:
  .byte $79, $69, $EE

sub_b31_EDDC:
  .byte $85, $9E

sub_b31_EDDE:
  .byte $E8

sub_b31_EDDF:
  .byte $B0, $04

sub_b31_EDE1:
  .byte $10, $DE

sub_b31_EDE3:
  .byte $30, $02

sub_b31_EDE5:
  .byte $30, $DA

sub_b31_EDE7:
  .byte $8A

sub_b31_EDE8:
  .byte $90, $04

sub_b31_EDEA:
  .byte $49, $FF

sub_b31_EDEC:
  .byte $69, $0A

sub_b31_EDEE:
  .byte $69, $2F

sub_b31_EDF0:
  .byte $C8

sub_b31_EDF1:
  .byte $C8

sub_b31_EDF2:
  .byte $C8

sub_b31_EDF3:
  .byte $C8

sub_b31_EDF4:
  .byte $84, $83

sub_b31_EDF6:
  .byte $A4, $AD

sub_b31_EDF8:
  .byte $C8

sub_b31_EDF9:
  .byte $AA

sub_b31_EDFA:
  .byte $29, $7F

sub_b31_EDFC:
  .byte $99, $FF, $00

sub_b31_EDFF:
  .byte $C6, $99

sub_b31_EE01:
  .byte $D0, $06

sub_b31_EE03:
  .byte $A9, $2E

sub_b31_EE05:
  .byte $C8

sub_b31_EE06:
  .byte $99, $FF, $00

sub_b31_EE09:
  .byte $84, $AD

sub_b31_EE0B:
  .byte $A4, $83

sub_b31_EE0D:
  .byte $8A

sub_b31_EE0E:
  .byte $49, $FF

sub_b31_EE10:
  .byte $29, $80

sub_b31_EE12:
  .byte $AA

sub_b31_EE13:
  .byte $C0, $24

sub_b31_EE15:
  .byte $D0, $AA

sub_b31_EE17:
  .byte $A4, $AD

sub_b31_EE19:
  .byte $B9, $FF, $00

sub_b31_EE1C:
  .byte $88

sub_b31_EE1D:
  .byte $C9, $30

sub_b31_EE1F:
  .byte $F0, $F8

sub_b31_EE21:
  .byte $C9, $2E

sub_b31_EE23:
  .byte $F0, $01
  .byte $C8

sub_b31_EE26:
  .byte $A9, $2B

sub_b31_EE28:
  .byte $A6, $9A

sub_b31_EE2A:
  .byte $F0, $2E
  .byte $10, $08, $A9, $00, $38, $E5, $9A, $AA
  .byte $A9, $2D, $99, $01, $01, $A9, $45, $99
  .byte $00, $01, $8A, $A2, $2F, $38, $E8, $E9
  .byte $0A, $B0, $FB, $69, $3A, $99, $03, $01
  .byte $8A, $99, $02, $01, $A9, $00, $99, $04
  .byte $01, $F0, $08, $99, $FF, $00

sub_b31_EE5A:
  .byte $A9, $00

sub_b31_EE5C:
  .byte $99, $00, $01

sub_b31_EE5F:
  .byte $A9, $00

sub_b31_EE61:
  .byte $A0, $01

sub_b31_EE63:
  .byte $60
  .byte $80, $00, $00, $00, $00, $FA, $0A, $1F
  .byte $00, $00, $98, $96, $80, $FF, $F0, $BD
  .byte $C0, $00, $01, $86, $A0, $FF, $FF, $D8
  .byte $F0, $00, $00, $03, $E8, $FF, $FF, $FF
  .byte $9C, $00, $00, $00, $0A, $FF, $FF, $FF
  .byte $FF, $20, $63, $EB, $A9, $64, $A0, $EE
  .byte $20, $F9, $EA, $F0, $70, $A5, $A5, $D0
  .byte $03, $4C, $50, $E8, $A2, $8A, $A0, $00
  .byte $20, $2B, $EB, $A5, $AA, $10, $0F, $20
  .byte $23, $EC, $A9, $8A, $A0, $00, $20, $B2
  .byte $EB, $D0, $03, $98, $A4, $0D, $20, $55
  .byte $EB, $98, $48, $20, $41, $E9, $A9, $8A
  .byte $A0, $00, $20, $7F, $E9, $20, $09, $EF
  .byte $68, $4A, $90, $0A, $A5, $9D, $F0, $06
  .byte $A5, $A2, $49, $FF, $85, $A2, $60, $81
  .byte $38, $AA, $3B, $29, $07, $71, $34, $58
  .byte $3E, $56, $74, $16, $7E, $B3, $1B, $77
  .byte $2F, $EE, $E3, $85, $7A, $1D, $84, $1C
  .byte $2A, $7C, $63, $59, $58, $0A, $7E, $75
  .byte $FD, $E7, $C6, $80, $31, $72, $18, $10
  .byte $81, $00, $00, $00, $00, $A9, $DB, $A0
  .byte $EE, $20, $7F, $E9, $A5, $AC, $69, $50
  .byte $90, $03, $20, $7A, $EB, $85, $92, $20
  .byte $66, $EB, $A5, $9D, $C9, $88, $90, $03
  .byte $20, $2B, $EA, $20, $23, $EC, $A5, $0D
  .byte $18, $69, $81, $F0, $F3, $38, $E9, $01
  .byte $48, $A2, $05, $B5, $A5, $B4, $9D, $95
  .byte $9D, $94, $A5, $CA, $10, $F5, $A5, $92
  .byte $85, $AC, $20, $AA, $E7, $20, $D0, $EE
  .byte $A9, $E0, $A0, $EE, $20, $72, $EF, $A9
  .byte $00, $85, $AB, $68, $20, $10, $EA, $60
  .byte $85, $AD, $84, $AE, $20, $21, $EB, $A9
  .byte $93, $20, $7F, $E9, $20, $76, $EF, $A9
  .byte $93, $A0, $00, $4C, $7F, $E9, $85, $AD
  .byte $84, $AE, $20, $1E, $EB, $B1, $AD, $85
  .byte $A3, $A4, $AD, $C8, $98, $D0, $02, $E6
  .byte $AE, $85, $AD, $A4, $AE, $20, $7F, $E9
  .byte $A5, $AD, $A4, $AE, $18, $69, $05, $90
  .byte $01, $C8, $85, $AD, $84, $AE, $20, $BE
  .byte $E7, $A9, $98, $A0, $00, $C6, $A3, $D0
  .byte $E4, $60, $98, $35, $44, $7A, $68, $28
  .byte $B1, $46, $20, $82, $EB, $AA, $30, $18
  .byte $A9, $C9, $A0, $00, $20, $F9, $EA, $8A
  .byte $F0, $E7, $A9, $A6, $A0, $EF, $20, $7F
  .byte $E9, $A9, $AA, $A0, $EF, $20, $BE, $E7
  .byte $A6, $A1, $A5, $9E, $85, $A1, $86, $9E
  .byte $A9, $00, $85, $A2, $A5, $9D, $85, $AC
  .byte $A9, $80, $85, $9D, $20, $2E, $E8, $A2
  .byte $C9, $A0, $00, $4C, $2B, $EB, $A9, $66
  .byte $A0, $F0, $20, $BE, $E7, $20, $63, $EB
  .byte $A9, $6B, $A0, $F0, $A6, $AA, $20, $5E
  .byte $EA, $20, $63, $EB, $20, $23, $EC, $A9
  .byte $00, $85, $AB, $20, $AA, $E7, $A9, $70
  .byte $A0, $F0, $20, $A7, $E7, $A5, $A2, $48
  .byte $10, $0D, $20, $A0, $E7, $A5, $A2, $30
  .byte $09, $A5, $16, $49, $FF, $85, $16, $20
  .byte $D0, $EE, $A9, $70, $A0, $F0, $20, $BE
  .byte $E7, $68, $10, $03, $20, $D0, $EE, $A9
  .byte $75, $A0, $F0, $4C, $5C, $EF, $20, $21
  .byte $EB, $A9, $00, $85, $16, $20, $F1, $EF
  .byte $A2, $8A, $A0, $00, $20, $E7, $EF, $A9
  .byte $93, $A0, $00, $20, $F9, $EA, $A9, $00
  .byte $85, $A2, $A5, $16, $20, $62, $F0, $A9
  .byte $8A, $A0, $00, $4C, $66, $EA, $48, $4C
  .byte $23, $F0, $81, $49, $0F, $DA, $A2, $83
  .byte $49, $0F, $DA, $A2, $7F, $00, $00, $00
  .byte $00, $05, $84, $E6, $1A, $2D, $1B, $86
  .byte $28, $07, $FB, $F8, $87, $99, $68, $89
  .byte $01, $87, $23, $35, $DF, $E1, $86, $A5
  .byte $5D, $E7, $28, $83, $49, $0F, $DA, $A2
  .byte $A6, $D3, $C1, $C8, $D4, $C8, $D5, $C4
  .byte $CE, $CA, $A5, $A2, $48, $10, $03, $20
  .byte $D0, $EE, $A5, $9D, $48, $C9, $81, $90
  .byte $07, $A9, $13, $A0, $E9, $20, $66, $EA
  .byte $A9, $CE, $A0, $F0, $20, $5C, $EF, $68
  .byte $C9, $81, $90, $07, $A9, $66, $A0, $F0
  .byte $20, $A7, $E7, $68, $10, $03, $4C, $D0
  .byte $EE, $60, $0B, $76, $B3, $83, $BD, $D3
  .byte $79, $1E, $F4, $A6, $F5, $7B, $83, $FC
  .byte $B0, $10, $7C, $0C, $1F, $67, $CA, $7C
  .byte $DE, $53, $CB, $C1, $7D, $14, $64, $70
  .byte $4C, $7D, $B7, $EA, $51, $7A, $7D, $63
  .byte $30, $88, $7E, $7E, $92, $44, $99, $3A
  .byte $7E, $4C, $CC, $91, $C7, $7F, $AA, $AA
  .byte $AA, $13, $81, $00, $00, $00, $00, $E6
  .byte $B8, $D0, $02, $E6, $B9, $AD, $60, $EA
  .byte $C9, $3A, $B0, $0A, $C9, $20, $F0, $EF
  .byte $38, $E9, $30, $38, $E9, $D0, $60, $80
  .byte $4F, $C7, $52, $58

sub_b31_F128:
  .byte $A2, $FF, $86, $76, $A2, $FB, $9A, $A9, $28, $A0, $F1, $85, $01, $84, $02, $85
  .byte $04, $84, $05, $EA, $EA, $EA, $A9, $4C, $85, $00, $85, $03, $85, $90, $85, $0A
  .byte $A9, $99, $A0, $E1, $85, $0B, $84, $0C, $A2, $1C

sub_b31_F152:
  .byte $BD, $0A, $F1, $95, $B0, $86, $F1, $CA, $D0, $F6, $86, $F2, $8A, $85, $A4, $85
  .byte $54, $48, $A9, $03, $85, $8F, $20, $FB, $DA, $A9, $01, $8D, $FD, $01, $8D, $FC
  .byte $01, $A2, $55, $86, $52, $AD, $C2, $03, $C9, $80, $D0, $04, $A9, $80, $D0, $02
  .byte $A9, $CD

sub_b31_F184:
  .byte $A0, $00, $F0, $13
  .byte $20, $11, $CE, $20, $58, $FC, $4C, $00
  .byte $CE, $8D, $E0, $67, $68, $20, $DF, $CD
  .byte $6C, $E3, $67

sub_b31_F19B:
  .byte $84, $73, $85, $74, $84, $6F, $85, $70, $A2, $00, $A0, $70, $86, $67, $84, $68
  .byte $A0, $00, $84, $D6, $98, $91, $67, $E6, $67, $D0, $02
  .byte $E6, $68

sub_b31_F1B8:
  .byte $A5, $67, $A4, $68, $20, $E3, $D3, $20, $4B, $D6, $A9, $3A, $A0, $DB, $85, $04
  .byte $84, $05, $A9, $3C, $A0, $D4, $85, $01, $84, $02, $6C, $01, $00
  .byte $20, $67, $DD, $20, $52, $E7, $6C, $50
  .byte $00, $20, $F8, $E6, $8A, $4C, $8B, $FE
  .byte $20, $F8, $E6, $8A, $4C, $D8, $F3, $20
  .byte $F8, $E6, $E0, $30, $B0, $13, $86, $F0
  .byte $A9, $2C, $20, $C0, $DE, $20, $F8, $E6
  .byte $E0, $30, $B0, $05, $86, $2C, $86, $2D
  .byte $60, $4C, $99, $E1, $20, $EC, $F1, $E4
  .byte $F0, $B0, $08, $A5, $F0, $85, $2C, $85
  .byte $2D, $86, $F0, $A9, $C5, $20, $C0, $DE
  .byte $20, $F8, $E6, $E0, $30, $B0, $E2, $60
  .byte $48, $AD, $FF, $67, $C9, $02, $F0, $06
  .byte $68, $49, $80, $4C, $3E, $F2, $CE, $FE
  .byte $67, $D0, $03, $4C, $42, $F2, $68, $09
  .byte $80, $8D, $00, $52, $60, $CE, $FF, $67
  .byte $68, $C9, $A0, $D0, $02, $A9, $22, $4C
  .byte $3C, $F2, $20, $F8, $E6, $8A, $4C, $64
  .byte $F8, $20, $F8, $E6, $CA, $8A, $C9, $18
  .byte $B0, $A7, $4C, $5B, $FB, $20, $F8, $E6
  .byte $8A, $49, $FF, $AA, $E8, $86, $F1, $60
  .byte $38, $90, $18, $66, $F2, $60, $48, $A9
  .byte $02, $8D, $FE, $67, $8D, $FF, $67, $68
  .byte $6C, $E3, $67, $A9, $7F, $A2, $40, $D0
  .byte $F5, $20, $67, $DD, $20, $52, $E7, $A5
  .byte $50, $C5, $6D, $A5, $51, $E5, $6E, $B0
  .byte $03, $4C, $10, $D4, $A5, $50, $85, $73
  .byte $85, $6F, $A5, $51, $85, $74, $85, $70
  .byte $60, $20, $67, $DD, $20, $52, $E7, $A5
  .byte $50, $C5, $73, $A5, $51, $E5, $74, $B0
  .byte $E0, $A5, $50, $C5, $69, $A5, $51, $E5
  .byte $6A, $90, $D6, $A5, $50, $85, $69, $A5
  .byte $51, $85, $6A, $4C, $6C, $D6, $A9, $AB
  .byte $20, $C0, $DE, $A5, $B8, $85, $F4, $A5
  .byte $B9, $85, $F5, $38, $66, $D8, $A5, $75
  .byte $85, $F6, $A5, $76, $85, $F7, $20, $A6
  .byte $D9, $4C, $98, $D9, $86, $DE, $A6, $F8
  .byte $86, $DF, $A5, $75, $85, $DA, $A5, $76
  .byte $85, $DB, $A5, $79, $85, $DC, $A5, $7A
  .byte $85, $DD, $A5, $F4, $85, $B8, $A5, $F5
  .byte $85, $B9, $A5, $F6, $85, $75, $A5, $F7
  .byte $85, $76, $20, $B7, $00, $20, $3E, $D9
  .byte $4C, $D2, $D7, $A5, $DA, $85, $75, $A5
  .byte $DB, $85, $76, $A5, $DC, $85, $B8, $A5
  .byte $DD, $85, $B9, $A6, $DF, $9A, $4C, $D2
  .byte $D7, $4C, $C9, $DE, $B0, $FB, $A6, $AF
  .byte $86, $69, $A6, $B0, $86, $6A, $20, $0C
  .byte $DA, $20, $1A, $D6, $A5, $9B, $85, $60
  .byte $A5, $9C, $85, $61, $A9, $2C, $20, $C0
  .byte $DE, $20, $0C, $DA, $E6, $50, $D0, $02
  .byte $E6, $51, $20, $1A, $D6, $A5, $9B, $C5
  .byte $60, $A5, $9C, $E5, $61, $B0, $01, $60
  .byte $A0, $00, $B1, $9B, $91, $60, $E6, $9B
  .byte $D0, $02, $E6, $9C, $E6, $60, $D0, $02
  .byte $E6, $61, $A5, $69, $C5, $9B, $A5, $6A
  .byte $E5, $9C, $B0, $E6, $A6, $61, $A4, $60
  .byte $D0, $01, $CA, $88, $86, $6A, $84, $69
  .byte $4C, $F2, $D4, $C9, $7F, $F0, $04, $C9
  .byte $FF, $D0, $03, $4C, $73, $F2, $48, $4C
  .byte $C6, $CD, $20, $D9, $F7, $A0, $03, $B1
  .byte $9B, $AA, $88, $B1, $9B, $E9, $01, $B0
  .byte $01, $CA, $85, $50, $86, $51, $20, $F5
  .byte $CC, $20, $BC, $F7, $4C, $F5, $CC, $20
  .byte $D9, $F7, $20, $76, $CD, $A0, $02, $B1
  .byte $9B, $C5, $50, $C8, $B1, $9B, $E5, $51
  .byte $B0, $03, $4C, $10, $D4, $20, $BC, $F7
  .byte $4C, $76, $CD, $20, $11, $CE, $20, $BE
  .byte $CE, $4C, $00, $CE, $08, $A9, $20, $2C
  .byte $54, $C0, $2C, $53, $C0, $85, $E6, $AD
  .byte $57, $C0, $AD, $50, $C0, $A9, $00, $85
  .byte $1C, $A5, $E6, $85, $1B, $A0, $00, $84
  .byte $1A, $A5, $1C, $91, $1A, $20, $7E, $F4
  .byte $C8, $D0, $F6, $E6, $1B, $A5, $1B, $29
  .byte $1F, $D0, $EE, $60, $4C, $4E, $F4

sub_b31_F414:
  .byte $4C, $D0, $F4

sub_b31_F417:
  .byte $85, $E2, $86, $E0, $4A, $4A, $4A, $85, $27, $A5, $E4, $4A, $4A, $4A, $18, $65
  .byte $27, $C9, $1E, $90, $02
  .byte $E9, $1E

sub_b31_F42E:
  .byte $85, $27, $8A, $29, $F8, $85, $26, $A5, $E2, $29, $07, $05, $26, $18, $69, $08
  .byte $85, $26, $8A, $29, $07, $AA, $38, $A9, $00

sub_b31_F447:
  .byte $6A, $CA, $10, $FC, $85, $1C, $60
  .byte $20, $3A, $F6, $20, $17, $F4, $20, $5A
  .byte $F4, $4C, $24, $F6

sub_b31_F45A:
  .byte $AD, $C7, $03, $C9, $33, $B0, $F9, $AC, $C6, $03, $A9, $00, $99, $00, $68, $C8
  .byte $A5, $27, $99, $00, $68, $C8, $A5, $26, $99, $00, $68, $C8, $A5, $E6, $99, $00
  .byte $68, $C8, $A5, $1C, $99, $00, $68, $C8, $8C, $C6, $03, $EE, $C7, $03, $60

sub_b31_F489:
  .byte $A5, $26, $48, $A8, $A5, $27, $48, $C5, $E1, $D0, $13, $C4, $D4, $D0, $0F, $A5
  .byte $1C, $05, $30, $85, $30, $68, $68, $60

sub_b31_F4A1:
  .byte $A5, $26, $48, $A5, $27, $48

sub_b31_F4A7:
  .byte $A5, $30, $F0, $15, $A5, $E1, $85, $27, $A5, $D4, $85, $26, $A5, $1C, $48, $A5
  .byte $30, $85, $1C, $20, $5A, $F4, $68, $85, $1C

sub_b31_F4C0:
  .byte $68, $85, $27, $A8, $68, $85, $26, $85, $D4, $84, $E1, $A5, $1C, $85, $30, $60

sub_b31_F4D0:
  .byte $20, $3A, $F6, $A2, $00, $48, $38, $E5, $E0, $B0, $09
  .byte $48, $8A, $49, $FF, $AA, $68, $20, $4D
  .byte $F6

sub_b31_F4E4:
  .byte $85, $D0, $98, $38, $E5, $E2, $B0, $09
  .byte $48, $8A, $49, $FF, $AA, $68, $20, $4D
  .byte $F6

sub_b31_F4F5:
  .byte $85, $D1, $86, $D3, $C5, $D0, $68, $90, $14, $C4, $E2, $B0, $04
  .byte $84, $E2, $85, $E0

sub_b31_F506:
  .byte $A5, $D1, $D0, $03
  .byte $4C, $24, $F6

sub_b31_F50D:
  .byte $85, $D2, $18, $90, $15

sub_b31_F512:
  .byte $C5, $E0, $B0, $04
  .byte $85, $E0, $84, $E2

sub_b31_F51A:
  .byte $A5, $D0, $48, $A5, $D1, $85, $D0, $68, $85, $D1, $85, $D2, $38

sub_b31_F527:
  .byte $08, $A0, $00, $98, $A6, $D0, $E0, $00, $D0, $04, $C5, $D1, $90, $14
  .byte $38, $E5, $D1, $B0, $01, $CA, $C8, $D0
  .byte $EF, $A5, $D3, $49, $01, $85, $D3, $84
  .byte $D1, $4C, $5A, $F5

sub_b31_F549:
  .byte $84, $D1, $24, $D3, $10, $0B
  .byte $A9, $00, $38, $E5, $D1, $85, $D1, $D0
  .byte $02, $85, $D3

sub_b31_F55A:
  .byte $A9, $00, $85, $D0, $85, $30, $A6, $E0, $A5, $E2, $20, $17, $F4

sub_b31_F567:
  .byte $20, $89, $F4, $28, $08, $90, $5E, $46, $1C, $90, $09, $66, $1C, $A5, $26, $18
  .byte $69, $08, $85, $26

sub_b31_F57B:
  .byte $A5, $D0, $18, $65, $D1, $85, $D0, $A5, $E2, $65, $D3, $C5, $E2, $85, $E2, $F0
  .byte $1F
  .byte $A4, $26, $24, $D3, $10, $1C, $88, $84
  .byte $26, $98, $29, $07, $C9, $07, $D0, $7D
  .byte $A5, $26, $18, $69, $08, $85, $26, $C6
  .byte $27, $10, $72, $A9, $1D, $85, $27

sub_b31_F5AB:
  .byte $4C, $19, $F6
  .byte $C8, $84, $26, $98, $29, $07, $D0, $63
  .byte $A5, $26, $38, $E9, $08, $85, $26, $E6
  .byte $27, $A5, $27, $C9, $1E, $90, $54, $E9
  .byte $1E, $85, $27, $4C, $19, $F6

sub_b31_F5CC:
  .byte $E6, $26, $A5, $26, $29, $07, $D0, $13, $A5, $26, $38, $E9, $08, $85, $26, $E6
  .byte $27, $A5, $27, $C9, $1E, $90, $04
  .byte $E9, $1E, $85, $27

sub_b31_F5E7:
  .byte $A5, $D0, $18, $65, $D1, $85, $D0, $A5, $E0, $65, $D3, $C5, $E0, $85, $E0, $F0
  .byte $21
  .byte $24, $D3, $10, $10, $06, $1C, $90, $19
  .byte $26, $1C, $A5, $26, $38, $E9, $08, $85
  .byte $26, $4C, $19, $F6, $46, $1C, $90, $09
  .byte $66, $1C, $A5, $26, $18, $69, $08, $85
  .byte $26

sub_b31_F619:
  .byte $C6, $D2, $F0, $03, $4C, $67, $F5

sub_b31_F620:
  .byte $20, $A1, $F4, $28, $AD, $C4, $03, $C9, $16, $F0, $0E
  .byte $A9, $10, $8D, $00, $20, $AD, $C2, $03
  .byte $8D, $C8, $03, $8D, $00, $50

sub_b31_F639:
  .byte $60

sub_b31_F63A:
  .byte $20, $35, $FB, $48, $A9, $80, $8D, $C8, $03, $8D, $00, $50, $A9, $90, $8D, $00
  .byte $20, $68, $60
  .byte $49, $FF, $18, $69, $01, $60, $8D, $00
  .byte $50, $A9, $90, $8D, $00, $20, $60, $48
  .byte $A9, $A0, $20

sub_b31_F660:
  .byte $A9, $4C, $8D, $42, $05, $A9, $9B, $8D, $43, $05, $A9, $C1, $8D, $44, $05, $4C
  .byte $35, $FB

sub_b31_F672:
  .byte $48, $A9, $A0, $20, $0E, $CC, $68, $4C, $24, $ED

sub_b31_F67C:
  .byte $8A, $48, $A2, $08, $A9, $01, $8D, $16, $40, $A9, $02, $8D, $16, $40

sub_b31_F68A:
  .byte $AD, $17, $40, $CA, $D0, $FA, $29, $02, $F0, $05
  .byte $68, $AA, $A9, $83, $60

sub_b31_F699:
  .byte $68, $AA, $A9, $A0, $60
  .byte $00, $20, $60, $85, $37, $A2, $00, $BD
  .byte $B1, $F6, $20, $DF, $CD, $E8, $E0, $05
  .byte $D0, $F5, $60, $9B, $C0, $9B, $C9, $C5
  .byte $D0, $CA, $60, $20, $67, $DD, $20, $52
  .byte $E7, $A6, $50, $A4, $51, $D0, $21, $EA
  .byte $EA, $EA, $EA, $E0, $F0, $B0, $19, $8A
  .byte $48, $98, $48, $A9, $2C, $20, $C0, $DE
  .byte $20, $F8, $E6, $E0, $F0, $B0, $09, $86
  .byte $9D, $68, $A8, $68, $AA, $A5, $9D, $60
  .byte $4C, $06, $F2, $20, $F8, $E6, $E0, $02
  .byte $B0, $F6, $BD, $F6, $F6, $85, $E6, $60
  .byte $00, $FF, $55, $7F, $80, $AA, $D5, $FF
  .byte $C9, $C1, $F0, $0D, $20, $B9, $F6, $20
  .byte $11, $F4, $20, $B7, $00, $C9, $C1, $D0
  .byte $E6, $20, $C0, $DE, $20, $B9, $F6, $84
  .byte $9D, $A8, $8A, $A6, $9D, $20, $14, $F4
  .byte $4C, $08, $F7, $20, $F8, $E6, $86, $F9
  .byte $60, $20, $F8, $E6, $86, $E7, $60, $20
  .byte $F8, $E6, $A5, $E8, $85, $1A, $A5, $E9
  .byte $85, $1B, $8A, $A2, $00, $C1, $1A, $F0
  .byte $02, $B0, $A5, $0A, $90, $03, $E6, $1B
  .byte $18, $A8, $B1, $1A, $65, $1A, $AA, $C8
  .byte $B1, $1A, $65, $E9, $85, $1B, $86, $1A
  .byte $20, $B7, $00, $C9, $C5, $D0, $09, $20
  .byte $C0, $DE, $20, $B9, $F6, $20, $11, $F4
  .byte $A5, $F9, $60, $48, $A9, $A0, $20, $00
  .byte $03, $68, $4C, $24, $ED, $EA, $EA, $A9
  .byte $00, $85, $3D, $85, $3F, $A0, $50, $84
  .byte $3C, $C8, $84, $3E, $20, $76, $CD, $18
  .byte $A5, $73, $AA, $CA, $86, $3E, $E5, $50
  .byte $48, $A5, $74, $A8, $E8, $D0, $01, $88
  .byte $84, $3F, $E5, $51, $C5, $6E, $90, $02
  .byte $D0, $03, $4C, $10, $D4, $85, $74, $85
  .byte $70, $85, $3D, $85, $E9, $68, $85, $E8
  .byte $85, $73, $85, $6F, $85, $3C, $20, $39
  .byte $CD, $A9, $03, $4C, $84, $CD, $18, $A5
  .byte $9B, $65, $50, $85, $3E, $A5, $9C, $65
  .byte $51, $85, $3F, $A0, $04, $B1, $9B, $20
  .byte $EF, $E0, $A5, $94, $85, $3C, $A5, $95
  .byte $85, $3D, $60, $A9, $40, $85, $14, $20
  .byte $E3, $DF, $A9, $00, $85, $14, $4C, $F0
  .byte $D8, $20, $F8, $E6, $CA, $8A, $C9, $28
  .byte $90, $0A, $E9, $28, $48, $20, $FB, $DA
  .byte $68, $4C, $EC, $F7, $85, $24, $60, $CB
  .byte $D2, $D7

sub_b31_F800:
  lda #$00
  ldy #$0F

sub_b31_F804:
  sta var_03C0,y
  dey
  bpl sub_b31_F804
  ldx #$03

sub_b31_F80C:
  lda data_b31_F817,x
  sta var_0036,x
  dex
  bpl sub_b31_F80C
  jmp sub_b31_FD0F
data_b31_F817:
  .byte $F0, $FD, $1B, $FD, $20, $35, $FD, $A4
  .byte $24, $60, $48, $A5, $24, $48, $A5, $25
  .byte $48, $84, $24, $A0, $00, $A5, $29, $C9
  .byte $07, $68, $68, $4C, $73, $FF, $25, $A5
  .byte $29, $AD, $17, $40, $29, $04, $60, $65
  .byte $25

sub_b31_F840:
  sta var_0000

sub_b31_F842:
  pha

sub_b31_F843:
  txa

sub_b31_F844:
  pha

sub_b31_F845:
  tya

sub_b31_F846:
  pha

sub_b31_F847:
  lda var_0000

sub_b31_F849:
  jsr sub_b31_C003

sub_b31_F84C:
  pla

sub_b31_F84D:
  tay

sub_b31_F84E:
  pla

sub_b31_F84F:
  tax

sub_b31_F850:
  pla

sub_b31_F851:
  rts
  .byte $60, $26, $0D, $0D, $0D, $0D, $0D, $0D
  .byte $0D, $0D, $0D, $0D, $0D, $0D, $0D, $0D
  .byte $0D, $30, $30, $30, $0D, $0D, $0D, $0D
  .byte $0D, $0D, $0D, $0D, $0D, $0D, $0D, $0D
  .byte $0D, $20, $47, $F8, $B1, $26, $28, $90
  .byte $04, $4A, $4A, $4A, $4A, $29, $0F, $60
  .byte $A6, $3A, $A4, $3B, $20, $96, $FD, $20
  .byte $1E, $FB, $A1, $3A, $A8, $4A, $90, $09
  .byte $6A, $B0, $10, $C9, $A2, $F0, $0C, $29
  .byte $87, $4A, $AA, $BD, $62, $F9, $20, $79
  .byte $F8, $D0, $04, $A0, $80, $A9, $00, $AA
  .byte $BD, $A6, $F9, $85, $2E, $29, $03, $85
  .byte $2F, $98, $29, $8F, $AA, $98, $A0, $03
  .byte $E0, $8A, $F0, $0B, $4A, $90, $08, $4A
  .byte $4A, $09, $20, $88, $D0, $FA, $C8, $88
  .byte $D0, $F2, $60, $FF, $FF, $FF, $20, $82
  .byte $F8, $48, $B1, $3A, $20, $DA, $FD, $A2
  .byte $01, $20, $4A, $F9, $C4, $2F, $C8, $90
  .byte $F1, $A2, $03, $C0, $04, $90, $F2, $68
  .byte $A8, $B9, $C0, $F9, $85, $2C, $B9, $00
  .byte $FA, $85, $2D, $A9, $00, $A0, $05, $06
  .byte $2D, $26, $2C, $2A, $88, $D0, $F8, $69
  .byte $BF, $20, $ED, $FD, $CA, $D0, $EC, $20
  .byte $1E, $FB, $A4, $2F, $A2, $06, $E0, $03
  .byte $F0, $1C, $06, $2E, $90, $0E, $BD, $B3
  .byte $F9, $20, $ED, $FD, $BD, $B9, $F9, $F0
  .byte $03, $20, $ED, $FD, $CA, $D0, $E7, $60
  .byte $88, $30, $E7, $20, $DA, $FD, $A5, $2E
  .byte $C9, $E8, $B1, $3A, $90, $F2, $20, $56
  .byte $F9, $AA, $E8, $D0, $01, $C8

sub_b31_F940:
  .byte $98, $20, $DA, $FD, $8A, $4C, $DA, $FD

sub_b31_F948:
  .byte $A2, $03
loc_b31_F94A:
  .byte $A9, $A0

sub_b31_F94C:
  .byte $20, $ED, $FD

sub_b31_F94F:
  .byte $CA

sub_b31_F950:
  .byte $D0, $F8

sub_b31_F952:
  .byte $60, $38, $A5, $2F, $A4, $3B, $AA, $10
  .byte $01, $88, $65, $3A, $90, $01, $C8, $60
  .byte $04, $20, $54, $30, $0D, $80, $04, $90
  .byte $03, $22, $54, $33, $0D, $80, $04, $90
  .byte $04, $20, $54, $33, $0D, $80, $04, $90
  .byte $04, $20, $54, $3B, $0D, $80, $04, $90
  .byte $00, $22, $44, $33, $0D, $C8, $44, $00
  .byte $11, $22, $44, $33, $0D, $C8, $44, $A9
  .byte $01, $22, $44, $33, $0D, $80, $04, $90
  .byte $01, $22, $44, $33, $0D, $80, $04, $90
  .byte $26, $31, $87, $9A, $00, $21, $81, $82
  .byte $00, $00, $59
data_b31_F9AD:
  .byte $4D, $91, $92, $86, $4A, $85, $9D, $AC
  .byte $A9, $AC, $A3, $A8, $A4, $D9, $00, $D8
  .byte $A4, $A4, $00, $1C, $8A, $1C, $23, $5D
  .byte $8B, $1B, $A1, $9D, $8A, $1D, $23, $9D
  .byte $8B, $1D, $A1, $00, $29, $19, $AE, $69
  .byte $A8, $19, $23, $24, $53, $1B, $23, $24
  .byte $53, $19, $A1, $00, $1A, $5B, $5B, $A5
  .byte $69, $24, $24, $AE, $AE, $A8, $AD, $29
  .byte $00, $7C, $00, $15, $9C, $6D, $9C, $A5
  .byte $69, $29, $53, $84, $13, $34, $11, $A5
  .byte $69, $23, $A0, $D8, $62, $5A, $48, $26
  .byte $62, $94, $88, $54, $44, $C8, $54, $68
  .byte $44, $E8, $94, $00, $B4, $08, $84, $74
  .byte $B4, $28, $6E, $74, $F4, $CC, $4A, $72
  .byte $F2, $A4, $8A, $00, $AA, $A2, $A2, $74
  .byte $74, $74, $72, $44, $68, $B2, $32, $B2
  .byte $00, $22, $00, $1A, $1A, $26, $26, $72
  .byte $72, $88, $C8, $C4, $CA, $26, $48, $44
  .byte $44, $A2, $C8

; IRQ 与 BRK 入口（未确认具体用法）：先把 A 存到 $45，再看栈里的状态字：不是 BRK 时经 $03FE 里的向量间接跳转；是 BRK 时调用 $FF4C 后经 $03F0 里的向量间接跳转。
irq_handler:
  .byte $85, $45, $68, $48, $0A, $0A, $0A, $30
  .byte $03, $6C, $FE, $03
loc_b31_FA4C:
  .byte $28, $20, $4C, $FF, $68, $85, $3A, $68
  .byte $85, $3B, $6C, $F0, $03, $20, $82, $F8
  .byte $20, $DA, $FA, $4C, $65, $FF

; 复位入口（复位向量指向这里）：跳到 reset_init_state。
reset_entry:
  jmp reset_init_state
  .byte $4C, $1E, $FD

sub_b31_FA68:
  .byte $A9, $3F, $8D, $06, $20, $A9, $00, $8D, $06, $20, $A0, $1F, $20, $35, $FB

sub_b31_FA77:
  .byte $B9, $53, $F8, $8D, $07, $20, $88, $10, $F7, $60

sub_b31_FA81:
  .byte $D8, $20, $3A, $FF, $AD, $F3, $03, $49, $A5, $CD, $F4, $03, $D0, $17
  .byte $AD, $F2, $03, $D0, $0F, $A9, $E0, $CD
  .byte $F3, $03, $D0, $08, $A0, $03, $8C, $F2
  .byte $03, $4C, $69, $FF, $4C, $69, $FF

sub_b31_FAA6:
  .byte $20, $60, $FB, $A2, $05

sub_b31_FAAB:
  .byte $BD, $FC, $FA, $9D, $EF, $03, $CA, $D0, $F7, $4C, $B0, $CB

sub_b31_FAB7:
  .byte $78, $A2, $FF, $9A, $20, $35, $FB, $20, $40, $FB, $A9, $00, $8D, $15, $40, $8E
  .byte $17, $40, $20, $68, $FA, $20, $D3, $FE, $20, $49, $FB, $4C, $81, $FA
  .byte $EA, $EA, $20, $8E, $FD, $A9, $45, $85
  .byte $40, $A9, $00, $85, $41, $A2, $FB, $BD
  .byte $1E, $FA, $20, $ED, $FD, $A9, $BD, $20
  .byte $ED, $FD, $B5, $4A, $20, $DA, $FD, $A9
  .byte $A0, $20, $ED, $FD, $E8, $30, $E8, $60
  .byte $59, $FA, $00, $E0, $A5, $20, $FF, $00
  .byte $FF, $03, $FF, $3C, $C1, $D0, $D0, $CC
  .byte $C5, $A0, $DD, $DB, $C4, $C2, $C1, $FF
  .byte $C3, $FF, $FF, $FF, $C1, $D8, $D9, $D0
  .byte $D3

sub_b31_FB1E:
  .byte $A2, $01, $4C, $4A, $F9, $A9, $A0, $4C
  .byte $ED, $FD

sub_b31_FB28:
  .byte $AD, $FC, $03, $C9, $20, $D0, $02
  .byte $A9, $22

sub_b31_FB31:
  .byte $60
  .byte $4C, $49, $FB

; 等下一次 VBlank 开始：先等标志变 0，再等它变 1。
wait_next_vblank:
  bit var_2002

sub_b31_FB38:
  bmi wait_next_vblank

sub_b31_FB3A:
  bit var_2002

sub_b31_FB3D:
  bpl sub_b31_FB3A

sub_b31_FB3F:
  rts

; 关闭 PPU 的渲染：$2000 与 $2001 都写 0。
ppu_disable_rendering:
  lda #$00

sub_b31_FB42:
  sta var_2000

sub_b31_FB45:
  sta var_2001

sub_b31_FB48:
  rts

sub_b31_FB49:
  jsr sub_b31_CFAF
  nop
  nop
  nop
  nop
  nop
  lda #$1E
  sta var_0021
  lda #$0E
  sta var_0023
  lda #$00
  sta var_0025
  jmp sub_b31_FC22

sub_b31_FB60:
  jsr monitor_home
  lda #$00
  jsr sub_b31_FBC1
  lda #$0B
  sta var_0024
  jsr sub_b31_FD1E
  lda var_03F3
  eor #$A5
  sta var_03F4
  rts
  .byte $C9, $8D, $D0, $11, $20, $5E, $F8, $90
  .byte $0C, $C9, $93, $D0, $08, $20, $5E, $F8
  .byte $90, $FB, $A9, $8D, $EA, $4C, $FD, $FB
  .byte $03, $2C, $10, $C0, $4C, $FD, $FB, $38
  .byte $4C, $9D, $80, $A8, $B9, $48, $FA, $20
  .byte $97, $FB, $20, $0C, $FD, $C9, $CE, $B0
  .byte $EE, $C9, $C9, $90, $EA, $C9, $CC, $F0
  .byte $E6, $D0, $E8, $A5, $2F, $49, $02, $8D
  .byte $16, $40, $85, $2F, $60, $EA, $EA, $EA
  .byte $EA

sub_b31_FBC1:
  pha

sub_b31_FBC2:
  lda #$06

sub_b31_FBC4:
  sta var_0029

sub_b31_FBC6:
  pla

sub_b31_FBC7:
  asl a

sub_b31_FBC8:
  asl a

sub_b31_FBC9:
  asl a

sub_b31_FBCA:
  asl a

sub_b31_FBCB:
  asl a

sub_b31_FBCC:
  sta var_0028

sub_b31_FBCE:
  bcc loc_b31_FBD2

sub_b31_FBD0:
  inc var_0029
loc_b31_FBD2:
  clc

sub_b31_FBD3:
  rts
  .byte $05, $28, $85, $28, $60

sub_b31_FBD9:
  .byte $C9, $87, $D0, $FB, $A9, $40, $20, $A8, $FC, $A0, $C0, $4C, $E0, $82
data_b31_FBE7:
  .byte $D0, $C5, $C3, $AD, $B9, $B3, $B8, $B8
  .byte $8D, $20, $60, $80, $EA, $20, $05, $F8
  .byte $EA, $EA, $EA, $20, $19, $F8

sub_b31_FBFD:
  jmp $805B
  .byte $EF, $A8, $10, $EC, $C9, $8D, $F0, $5A
  .byte $C9, $8A, $F0, $5A, $C9, $88, $D0, $C9
  .byte $4C, $2F, $F8, $EA, $A5, $21, $85, $24
  .byte $C6, $24, $20, $8B, $80, $EA, $EA, $EA
  .byte $EA, $EA

sub_b31_FC22:
  lda var_0025

sub_b31_FC24:
  jsr sub_b31_FBC1

sub_b31_FC27:
  adc var_0020

sub_b31_FC29:
  sta var_0028

sub_b31_FC2B:
  rts
  .byte $60, $20, $00, $FE, $68, $68, $4C, $69
  .byte $FF

sub_b31_FC35:
  .byte $20, $24, $FC, $4C, $9E, $FC

sub_b31_FC3B:
  .byte $68, $85, $25, $B0, $E2
  .byte $EA, $EA

sub_b31_FC42:
  .byte $A4, $24, $A5, $25, $48

sub_b31_FC47:
  .byte $20, $35, $FC, $A0, $00, $E6, $25, $A5, $25, $C5, $23, $90, $F3, $B0, $E5
  .byte $EA, $EA

; 清屏并把光标放到左上角（地址与用法同苹果机监控程序的 HOME，未逐条核对实现）。
monitor_home:
  .byte $4C, $C9, $80
  .byte $20, $1A, $FC, $4C, $AD, $81, $EA, $4C
  .byte $CD, $80, $EA, $4C, $D0, $80, $C9, $9F
  .byte $F0, $34, $4C, $B0, $81

sub_b31_FC70:
  .byte $4C, $DA, $80

sub_b31_FC73:
  .byte $20, $24, $FC

sub_b31_FC76:
  .byte $A5, $28

sub_b31_FC78:
  .byte $85, $2A

sub_b31_FC7A:
  .byte $A5, $29

sub_b31_FC7C:
  .byte $85, $2B

sub_b31_FC7E:
  .byte $A0, $20

sub_b31_FC80:
  .byte $88

sub_b31_FC81:
  .byte $68

sub_b31_FC82:
  .byte $69, $01

sub_b31_FC84:
  .byte $C5, $23

sub_b31_FC86:
  .byte $B0, $0D

sub_b31_FC88:
  .byte $48

sub_b31_FC89:
  .byte $20, $24, $FC

sub_b31_FC8C:
  .byte $B1, $28

sub_b31_FC8E:
  .byte $91, $2A

sub_b31_FC90:
  .byte $88

sub_b31_FC91:
  .byte $10, $F9

sub_b31_FC93:
  .byte $30, $E1

sub_b31_FC95:
  .byte $A0, $00

sub_b31_FC97:
  .byte $20, $9E, $FC

sub_b31_FC9A:
  .byte $B0, $86
  .byte $A4, $24

sub_b31_FC9E:
  .byte $4C, $0C, $97
  .byte $20, $66, $FC, $4C, $AD, $81, $EA

sub_b31_FCA8:
  sec

sub_b31_FCA9:
  pha

sub_b31_FCAA:
  sbc #$01

sub_b31_FCAC:
  bne sub_b31_FCAA

sub_b31_FCAE:
  pla

sub_b31_FCAF:
  sbc #$01

sub_b31_FCB1:
  bne sub_b31_FCA9

sub_b31_FCB3:
  rts
  .byte $E6, $42, $D0, $02, $E6, $43

sub_b31_FCBA:
  .byte $A5, $3C, $C5, $3E, $A5, $3D, $E5, $3F, $E6, $3C, $D0, $02
  .byte $E6, $3D
loc_b31_FCC8:
  .byte $60
  .byte $48, $08, $A9, $00, $8D, $03, $20, $A9
  .byte $C0, $8D, $14, $40, $EE, $CC, $03, $D0
  .byte $0D, $EE, $CD, $03, $D0, $08, $EE, $CE
  .byte $03, $D0, $03, $EE, $CF, $03, $AD, $C9
  .byte $03, $C9, $06, $D0, $03, $20, $F4, $FC
  .byte $28, $68, $40

; NMI 里的钩子：经 $03CA 里的地址间接跳转。位于 RAM 程序里的该程序每帧让 $D3 减 1（作为等待计时）并继续处理显存更新。
nmi_call_hook:
  jmp (var_03CA)
  .byte $60, $F5, $60, $20, $FD, $FC, $88, $AD
  .byte $60, $C0, $45, $2F, $10, $F8, $45, $2F
  .byte $85, $2F, $C0, $80, $60

; 读一个键（地址与用法同苹果机监控程序的 RDKEY，未逐条核对实现）。
monitor_rdkey:
  jmp (var_0038)

sub_b31_FD0F:
  lda #$80
  sta var_03C1
  jmp sub_b31_FAB7
  .byte $00, $00, $00, $00

; 读一个键并回显光标（地址与用法同苹果机监控程序的 KEYIN，未逐条核对实现）。
monitor_keyin:
  jmp $8131

sub_b31_FD1E:
  ldx #$00
  stx var_0025
loc_b31_FD22:
  lda data_b31_FBE7,x
  bit monitor_cout
  inx
  cpx #$09
  bne loc_b31_FD22
  rts
  .byte $60, $20, $0C, $FD, $20, $A5, $FB, $20
  .byte $0C, $FD, $C9, $9B, $F0, $F3, $60, $A4
  .byte $24

sub_b31_FD3F:
  txa

sub_b31_FD40:
  pha

sub_b31_FD41:
  tya

sub_b31_FD42:
  pha

sub_b31_FD43:
  lda var_0025

sub_b31_FD45:
  jsr sub_b31_C009

sub_b31_FD48:
  pla

sub_b31_FD49:
  tay

sub_b31_FD4A:
  pla

sub_b31_FD4B:
  tax

sub_b31_FD4C:
  lda #$A0

sub_b31_FD4E:
  sta (var_0028),y

sub_b31_FD50:
  iny

sub_b31_FD51:
  cpy var_0021

sub_b31_FD53:
  bcc sub_b31_FD4E

sub_b31_FD55:
  rts
  .byte $F0, $0A, $E0, $F8, $90, $03, $20, $3A
  .byte $FF, $E8, $D0, $13, $A9, $DC, $20, $ED
  .byte $FD

sub_b31_FD67:
  jmp $819B

; 读一行输入（地址与用法同苹果机监控程序的 GETLN，未逐条核对实现）。
monitor_getln:
  jmp $819E
  .byte $EA, $EA, $4C, $A3, $81

sub_b31_FD72:
  ldx #$20
  tay

sub_b31_FD75:
  sta var_2007
  iny
  bne sub_b31_FD75
  dex
  bne sub_b31_FD75
  lda #$90
  sta var_2000
  lda #$08
  sta var_2001
  lda #$00
  jmp $8168
  .byte $EA

; 输出回车（地址与用法同苹果机监控程序的 CROUT，未逐条核对实现）。
monitor_crout:
  .byte $A9, $8D

sub_b31_FD90:
  .byte $D0, $5B

sub_b31_FD92:
  .byte $A4, $3D, $A6, $3C, $20, $8E, $FD, $20, $40, $F9, $A0, $00, $A9, $AD, $4C, $ED
  .byte $FD
loc_b31_FDA3:
  .byte $A5, $3C, $09, $07, $85, $3E, $A5, $3D
  .byte $85, $3F
loc_b31_FDAD:
  .byte $A5, $3C, $29, $07, $D0, $03
loc_b31_FDB3:
  .byte $20, $92, $FD
loc_b31_FDB6:
  .byte $A9, $A0, $20, $ED, $FD, $B1, $3C, $20, $DA, $FD, $20, $BA, $FC, $90, $E8, $60
loc_b31_FDC6:
  .byte $4A, $90, $EA
  .byte $4A, $4A, $A5, $3E, $90, $02, $49, $FF
loc_b31_FDD1:
  .byte $65, $3C, $48, $A9, $BD, $20, $ED, $FD
  .byte $68

; 把 A 输出成两位十六进制（地址与用法同苹果机监控程序的 PRBYTE，未逐条核对实现）。
monitor_prbyte:
  .byte $48, $4A, $4A, $4A, $4A, $20, $E5, $FD, $68, $29, $0F

sub_b31_FDE5:
  .byte $09, $B0, $C9, $BA, $90, $02, $69, $06

; 输出一个字符（A=字符码，高位为 1）。地址、用法与苹果机监控程序的 COUT 一致（按地址与用法命名，未逐条核对实现）。
monitor_cout:
  .byte $6C, $36, $00

sub_b31_FDF0:
  .byte $84, $35

sub_b31_FDF2:
  .byte $4C, $FE, $80

sub_b31_FDF5:
  .byte $EA

sub_b31_FDF6:
  .byte $EA

sub_b31_FDF7:
  .byte $EA

sub_b31_FDF8:
  .byte $48

sub_b31_FDF9:
  .byte $20, $00, $80

sub_b31_FDFC:
  .byte $68

sub_b31_FDFD:
  .byte $A4, $35

sub_b31_FDFF:
  .byte $60

sub_b31_FE00:
  .byte $C6, $34, $F0, $9F, $CA, $D0, $16, $C9, $BA, $D0, $BB
  .byte $85, $31, $A5, $3E, $91, $40, $E6, $40
  .byte $D0, $02, $E6, $41
loc_b31_FE17:
  .byte $60

sub_b31_FE18:
  .byte $A4, $34, $B9, $FF, $01
loc_b31_FE1D:
  .byte $85, $31, $60
  .byte $A2, $01, $B5, $3E, $95, $42, $95, $44
  .byte $CA, $10, $F7, $60, $B1, $3C, $91, $42
  .byte $20, $B4, $FC, $90, $F7, $60, $B1, $3C
  .byte $D1, $42, $F0, $1C, $20, $92, $FD, $B1
  .byte $3C, $20, $DA, $FD, $A9, $A0, $20, $ED
  .byte $FD, $A9, $A8, $20, $ED, $FD, $B1, $42
  .byte $20, $DA, $FD, $A9, $A9, $20, $ED, $FD
  .byte $20, $B4, $FC, $90, $D9, $60, $20, $75
  .byte $FE, $A9, $0A, $48, $20, $D0, $F8, $20
  .byte $53, $F9, $85, $3A, $84, $3B, $68, $38
  .byte $E9, $01, $D0, $EF, $60, $8A, $F0, $07
  .byte $B5, $3C, $95, $3A, $CA, $10, $F9, $60
  .byte $C6, $34, $20, $75, $FE, $4C, $00, $82
  .byte $60, $4C, $00, $84, $4C, $50, $86

sub_b31_FE8F:
  jsr sub_b31_FE00
  pla
  pla
  jmp sub_b31_FF69
  .byte $4C, $EF, $85, $F0, $A5, $3E, $29, $0F
  .byte $F0, $06, $09, $C0, $A0, $00, $F0, $02
  .byte $A9, $FD, $94, $00, $95, $01, $60, $EA
  .byte $EA, $4C, $00, $E0, $4C, $03, $E0, $20
  .byte $75, $FE, $20, $3F, $FF, $6C, $3A, $00
  .byte $4C, $D7, $FA, $4C, $BC, $CE, $4C, $2D
  .byte $FC, $EA, $EA, $4C, $F8, $03, $4C, $F5
  .byte $CC, $4C, $76, $CD

sub_b31_FED3:
  jsr wait_next_vblank
  jsr ppu_disable_rendering
  lda #$20
  sta var_2006
  lda #$00
  sta var_2006
  ldx #$04
  bit data_b31_FF4A
  ldy #$00

sub_b31_FEEA:
  tya
  sta var_2007
  sta var_2007
  iny
  bne sub_b31_FEEA
  dex
  nop
  nop
  bne sub_b31_FEEA
  lda #$23
  sta var_2006
  lda #$C0
  sta var_2006
  ldx #$40
  lda #$FF

sub_b31_FF07:
  sta var_2007
  dex
  bne sub_b31_FF07
  lda #$27
  sta var_2006
  lda #$C0
  sta var_2006
  ldx #$40
  lda #$FF

sub_b31_FF1B:
  sta var_2007
  dex
  bne sub_b31_FF1B
  lda #$00
  sta var_2006
  sta var_2006
  jmp sub_b31_FD72
  .byte $8D, $A9, $C5, $20, $ED, $FD, $A9, $D2
  .byte $20, $ED, $FD, $20, $ED, $FD

sub_b31_FF3A:
  lda #$87
  jmp monitor_cout
  .byte $A5, $48, $48, $A5, $45, $A6, $46, $A4
  .byte $47, $28, $60
data_b31_FF4A:
  .byte $85, $45

sub_b31_FF4C:
  .byte $86, $46, $84, $47, $08, $68, $85, $48
  .byte $BA, $86, $49, $D8, $60, $20, $84, $FE
  .byte $20, $30, $FB, $EA, $EA, $EA, $EA, $EA
  .byte $EA, $D8, $20, $3A, $FF

sub_b31_FF69:
  .byte $A9, $AA, $85, $33, $20, $67, $FD, $20, $C7, $FF

sub_b31_FF73:
  .byte $20, $A7, $FF, $84, $34, $A0, $17

sub_b31_FF7A:
  .byte $88, $30, $E8, $D9, $CC, $FF, $D0, $F8, $20, $BE, $FF, $A4, $34, $4C, $73, $FF

sub_b31_FF8A:
  .byte $A2, $03, $0A, $0A, $0A, $0A

sub_b31_FF90:
  .byte $0A, $26, $3E, $26, $3F, $CA, $10, $F8

sub_b31_FF98:
  .byte $A5, $31, $D0, $06, $B5, $3F, $95, $3D, $95, $41

sub_b31_FFA2:
  .byte $E8, $F0, $F3, $D0, $06

sub_b31_FFA7:
  .byte $A2, $00, $86, $3E, $86, $3F

sub_b31_FFAD:
  .byte $B9, $00, $02, $C8, $49, $B0, $C9, $0A, $90, $D3, $69, $88, $C9, $FA, $B0, $CD
  .byte $60

sub_b31_FFBE:
  .byte $A9, $FE, $48, $B9, $E3, $FF, $48, $A5, $31

sub_b31_FFC7:
  .byte $A0, $00, $84, $31, $60
  .byte $F2, $EE, $F1, $ED, $EF, $F9, $EC, $04
  .byte $F3, $A6, $A4, $06, $95, $E9, $98, $05
  .byte $F0, $00, $EB, $93, $A7, $C6, $99, $B2
  .byte $C9, $BE, $7F, $35, $88, $81, $8B, $AF
  .byte $17, $17, $2B, $1F, $C1, $96, $5D, $CC
  .byte $B5, $CF, $17, $17, $8E, $03, $39, $C0
  .byte $62, $FA, $40
data_b31_FFFF:
  .byte $FA
