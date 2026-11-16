; ------------------------------------------------------------------------
; 名称：fbasic.s
; 说明：Family BASIC v3.0 的 SB2000 移植源码：由 reference/fbasic-nes.s（原版、地址与 PRG 一一对应）复制而来，之后手工维护，把依赖 FC 机器的地方改成 SB2000 的写法。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：所有地址都用标签，改动会让后面的代码整体后移；映像总长固定 32K，向量在段 VECTORS（$FFFA），段 CODE 放不下时链接报溢出。
;       原版分析与对照见 docs/移植对照表.md。
; ------------------------------------------------------------------------
        .setcpu "6502"
        .include        "platform.inc"


; 指向指令或数据中间的地址
L800C                   := L800B+1
L8014                   := L8013+1
L8251                   := L8250+1
L89A4                   := L89A3+1
L89C4                   := L89C3+1
LA86F                   := LA86E+1
LB8BF                   := LB8BE+1
LC0F4                   := LC0F3+1
LC251                   := LC250+1
LC33E                   := LC33D+1
LC70A                   := LC709+1
LCEC0                   := LCEBF+1
LCF42                   := LCEBF+131
LCFA5                   := LCFA4+1
LCFA9                   := LCFA4+5

        jmp     L80BA
L8003:
        .byte   $41
L8004:
        .byte   $65,$8A,$6A,$8C,$6C,$F8,$6C
L800B:
        .addr   $0000,$07FE,$1282,$1F08              ; GAME 0-3 在 GAME 数据里的偏移（数据在扩展 RAM，见 game.s）
L8013:
        .word   $0EAE,$0E4E,$0DF3,$0D9E,$0D4D,$0D01,$0CB9,$0C75
        .word   $0C35,$0BF8,$0BBF,$0B89,$0B57,$0B27,$0AF9,$0ACF
        .word   $0AA6,$0A80,$0A5C,$0A3A,$0A1A,$09FC,$09DF,$09C4
        .word   $09AB,$0993,$097C,$0967,$0952,$093F,$092D,$091C
        .word   $090C,$08FD,$08EE,$08E1,$08D4,$08C8,$08BD,$08B2
        .word   $08A8,$089F,$0896,$088D,$0885,$087E,$0876,$0870
        .word   $0869,$0863,$085E,$0858,$0853,$084F,$084A,$0846
        .word   $0842,$083E,$083A,$0837,$0834,$0831,$082E,$082B
        .word   $0829,$0827,$0824,$0822,$0820,$081E,$081C,$081B
L80A3:
        .byte   $01,$02,$03,$04,$06,$08,$0C,$10,$18,$20
L80AD:
        .byte   $12,$16,$00,$04,$08,$0A,$0E
L80B4:
        .byte   $00,$18,$30,$48,$60,$78
L80BA:  lda     #$00
        sta     $2000
        sta     $2001
        jsr     platform_init
        jsr     LB42B
        jsr     LB42B
        lda     #$4C
        sta     $ED
        lda     #<L8971
        sta     $EE
        lda     #>L8971
        sta     $EF
        jsr     LB3AB
        jsr     LB4A7
        lda     #$20
        sta     $4F
        lda     #$1E
        sta     $28
L80E2:  jsr     LB945
        beq     L8102
        dec     $28
        bne     L80E2
        lda     #$05
        sta     $48
        lda     #$04
        sta     $49
        lda     #$01
        jsr     L820D
        lda     #$06
        jsr     L820D
L80FD:  jsr     LB945
        bne     L80FD
L8102:  jsr     L81F3
        bcc     L8121
        jsr     LB4AB
        lda     #$01
        sta     $48
        lda     #$04
        sta     $49
        lda     #$03
        jsr     L820D
        lda     #$04
        jsr     L820D
L811C:  jsr     L81F3
        bcs     L811C
L8121:  lda     #$00
        sta     $7C
        lda     $9E
        cmp     #$59
        beq     L815A
        lda     $6001
        cmp     #$4C
        bne     L8180
        jsr     LB4A7
        lda     $6004
        sta     $7C
        lda     $6002
        sta     $07
        lda     $6003
        sta     $08
        lda     #$06
        sta     $05
        lda     #$60
        sta     $06
        lda     #$00
        tax
L814F:  sta     $6000,x
        inx
        cpx     #$05
        bne     L814F
        jsr     L97A6
L815A:  jsr     L8662
        jsr     LB327
        jsr     LB56B
        lda     $7C
        beq     L8178
        lda     #<L817B
        sta     $52
        lda     #>L817B
        sta     $53
        jsr     L895A
        jsr     LB56B
        jsr     LB206
L8178:  jmp     L826A
L817B:
        .byte   "[BG]", $00
L8180:  jsr     LB4A7
        lda     #$59
        sta     $9E
        lda     #$00
        sta     $6004
        jmp     L825E
L818F:  lda     $33
        and     #$EF
        sta     $33
        sta     $2001
        lda     #$4C
        sta     $6001
        lda     $07
        sta     $6002
        lda     $08
        sta     $6003
        jsr     LB4AB
        lda     #$02
        sta     $48
        lda     #$04
        sta     $49
        lda     #$03
        jsr     L820D
        lda     #$00
        jsr     L820D
        lda     #$05
        jsr     L820D
L81C1:  jsr     LB945
        cmp     #$03
        beq     L815A
        jsr     L81F3
        bcc     L81C1
        jsr     LB4AB
        lda     #$05
        sta     $48
        lda     #$04
        sta     $49
        lda     #$02
        jsr     L820D
        lda     #$04
        jsr     L820D
L81E2:  jsr     LB945
        cmp     #$03
        bne     L81E2
        jmp     L80BA
; 设置名称表写入地址：A = NES 地址高字节，X = 低字节；转成 CRAM 地址（每个图块 2 字节，地址左移一位），不改 X、Y
L81EC:  sta     V_ADDR_TMP
        txa
        asl     a
        pha
        lda     V_ADDR_TMP
        and     #$0F
        rol     a
        sta     $2006
        pla
        sta     $2006
        rts
L81F3:  lda     $6000
        pha
        lda     #$19
        sta     $6000
        cmp     $6000
        beq     L8207
        pla
        sta     $6000
        sec
        rts
L8207:  pla
        sta     $6000
        clc
        rts
L820D:  asl     a
        tax
        lda     L8250,x
        sta     $52
        lda     L8251,x
        sta     $53
        jmp     L895A
L821C:
        .byte   " ON", $00
L8220:
        .byte   "f-", $AA, "-", $A5, " ", $8D, " mq", $A0, "g", $00
L822D:
        .byte   $A4, $8C, $9A, $8C, " ", $8D, $00
L8234:
        .byte   $A6, $96, "g`", $96, $AD, " la", $96, "p ", $8D, $00
L8242:
        .byte   " OFF"
L8246:
        .byte   " u"
L8248:
        .byte   " krg", $A1, "ja", $00
L8250:
        .addr   L821C,L8220,L822D,L8234,L8242,L8246,L8248
L825E:  ldx     #$FF
        txs
        jsr     L8662
        jsr     L84BC
        jsr     LB2F7
L826A:  jsr     L88C0
        sta     $E7
L826F:  lda     #$00
        sta     $E2
        sta     $A2
        sta     $A3
        ldx     #$FF
        txs
        txa
        pha
        pha
        jsr     LB766
L8280:  tsx
        stx     $17
        cli
L8284:  ldx     $17
        txs
        jsr     LB56B
        jsr     LB2ED
        lda     $24
        beq     L8296
        lda     #$2E
        jsr     LB575
L8296:  jsr     LB56B
L8299:  jsr     L82CC
        jsr     LBD04
        cmp     #$03
        beq     L82C1
        jsr     L8BA4
        jsr     L90B2
        jsr     L90BF
        and     #$FF
        beq     L8299
        jsr     L90C8
        bcs     L82BE
        jsr     L84E5
        jsr     L97A6
        jmp     L8299
L82BE:  jmp     L8334
L82C1:  lda     $E2
        beq     L8299
        lda     #$00
        sta     $E2
        jmp     L8284
L82CC:  lda     $E2
        beq     L8325
        lda     $A0
        sta     $44
        lda     $A1
        sta     $45
        jsr     L8540
        bne     L82FD
        jsr     L9430
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
        jsr     L87EE
        jsr     L9439
        ldx     $49
        dex
        dex
L82F2:  jsr     LB637
        beq     L82FA
        dex
        bpl     L82F2
L82FA:  inx
        stx     $49
L82FD:  lda     $A0
        sta     $28
        lda     $A1
        sta     $29
        jsr     L913E
        jsr     L90D6
        jsr     L87E0
        lda     #$20
        jsr     L894D
        jsr     L895D
L8316:  clc
        lda     $A0
        adc     $A2
        sta     $A0
        lda     $A1
        adc     $A3
        bcs     L8326
        sta     $A1
L8325:  rts
L8326:  lda     #$00
        sta     $E2
        jmp     L8EF2
L832D:  lda     #$FF
        sta     $0B
        sta     $0C
        rts
L8334:  jsr     L925B
        lda     #<LBB89
        sta     $0D
        lda     #>LBB89
        sta     $0E
        lda     #$00
        sta     $09
        lda     #$03
        sta     $0A
        jsr     L832D
L834A:  jsr     L8855
        jsr     L8BCD
        jsr     L90BF
        pha
        lda     $09
        sta     $58
        lda     $0A
        sta     $59
        jsr     L908C
        pla
        beq     L83B4
        cmp     #$3A
        beq     L834A
        pha
        jsr     L83FD
        pla
        cmp     #$27
        beq     L83B4
        cmp     #$80
        bcs     L83B7
        jsr     LA7C4
        bcs     L83B1
        pha
        jsr     L90A6
        pla
        jsr     LA576
        lda     $90
        pha
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     L86D9
        jsr     LA283
        pla
        sta     $8D
        pla
        sta     $8C
        pla
        sta     $90
        ldy     #$00
        and     #$0F
        cmp     #$03
        beq     L83BF
        jsr     LAA0D
        lda     $0400
        sta     ($8C),y
        lda     $0401
        iny
        sta     ($8C),y
        jmp     L834A
L83B1:  jmp     L8491
L83B4:  jmp     L8469
L83B7:  jmp     L8429
L83BA:  lda     #$0C
        jmp     LB237
L83BF:  jsr     LAA10
        jsr     L83CB
        jmp     L834A
L83C8:  sta     ($8C),y
        rts
L83CB:  ldy     #$00
        lda     $0400
        beq     L83C8
        cmp     #$20
        bcs     L83BA
        sta     ($8C),y
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
        iny
        tya
        clc
        adc     $8C
        sta     $8C
        lda     #$00
        adc     $8D
        sta     $8D
        ldx     $0400
        ldy     #$00
L83F4:  lda     ($19),y
        sta     ($8C),y
        iny
        dex
        bne     L83F4
L83FC:  rts
L83FD:  lda     $97
        beq     L83FC
        lda     #$FF
        cmp     $0B
        bne     L840B
        cmp     $0C
        beq     L83FC
L840B:  lda     #$23
        jsr     L894D
        lda     $0B
        sta     $28
        lda     $0C
        sta     $29
        jsr     L913E
        jsr     L90D6
        jsr     L87E0
        lda     #$20
        jsr     L894D
        jmp     L895D
L8429:  sec
        sbc     #$80
        cmp     #$44
        bcs     L8491
        asl     a
        tax
        lda     LCEBF,x
        sta     $19
        lda     LCEC0,x
        sta     $1A
        tsx
        stx     $17
        jsr     L849C
        jsr     LA33E
        jmp     L834A
L8448:  lda     $E7
        cmp     #$02
        beq     L8451
L844E:  jmp     L8280
L8451:  jsr     L8459
        bcs     L844E
        jmp     L951B
L8459:  lda     #$FF
        cmp     $0B
        bne     L8465
        cmp     $0C
        bne     L8465
        sec
        rts
L8465:  clc
        rts
L8467:  pla
        pla
L8469:  lda     $0D
        sta     $09
        lda     $0E
        sta     $0A
L8471:  jsr     L90B2
        beq     L8448
        clc
        adc     $09
        sta     $0D
        lda     #$00
        adc     $0A
        sta     $0E
        iny
        lda     ($09),y
        sta     $0B
        iny
        lda     ($09),y
        sta     $0C
        jsr     L9F10
        jmp     L834A
L8491:  lda     #$01
        .byte   $AE
L8494:  lda     #$04
        jmp     LB237
L8499:  jsr     L908C
L849C:  jsr     L90BF
        and     #$FF
        beq     L84A5
        cmp     #$3A
L84A5:  rts
L84A6:  jsr     L88C0
        sta     $97
L84AB:  sta     $E7
        sta     $B5
        sta     $B6
        sta     $B7
        sta     $B8
        rts
L84B6:  jsr     L84BC
        jmp     L826F
L84BC:  jsr     L84A6
        sta     $6006
        sta     $6007
        sta     $6008
        ldx     #$06
        stx     $05
        inx
        stx     $07
        inx
        stx     $1D
        inx
        stx     $1F
        inx
        stx     $21
        lda     #$60
        sta     $06
        sta     $08
        sta     $1E
        sta     $20
        sta     $22
        rts
L84E5:  jsr     L88C0
        jsr     L832D
        jsr     L90FD
        lda     #$FF
        cmp     $28
        bne     L84FA
        cmp     $29
        bne     L84FA
        beq     L8494
L84FA:  lda     $28
        sta     $A0
        lda     $29
        sta     $A1
        jsr     L8316
        jsr     L90B2
        cmp     #$20
        bne     L850F
        jsr     L9098
L850F:  and     #$FF
        beq     L8574
        lda     $28
        sta     $44
        lda     $29
        sta     $45
        jsr     L925B
        sec
        lda     $4D
        sbc     #$00
        sta     $28
        lda     $4E
        sbc     #$03
        sta     $29
        lda     #$02
        clc
        adc     $28
        sta     $5A
        lda     $44
        sta     $0F
        lda     $45
        sta     $10
        jsr     L857C
        jmp     L85BE
L8540:  lda     $05
        sta     $5B
        lda     $06
        sta     $5C
L8548:  ldy     #$00
        lda     ($5B),y
        bne     L8551
        lda     #$FF
L8550:  rts
L8551:  ldy     #$02
        lda     $45
        cmp     ($5B),y
        bne     L8560
        dey
        lda     $44
        cmp     ($5B),y
        beq     L8550
L8560:  bcc     L8550
        ldy     #$00
        clc
        lda     ($5B),y
        adc     $5B
        sta     $5B
        lda     $5C
        adc     #$00
        sta     $5C
        jmp     L8548
L8574:  lda     $28
        sta     $44
        lda     $29
        sta     $45
L857C:  jsr     L8540
        bne     L8550
L8581:  ldy     #$00
        lda     ($5B),y
        sta     $3C
        clc
        adc     $5B
        sta     $0B
        lda     $5C
        adc     #$00
        sta     $0C
L8592:  ldy     #$00
        lda     ($0B),y
        sta     ($5B),y
        inc     $0B
        bne     L859E
        inc     $0C
L859E:  inc     $5B
        bne     L85A4
        inc     $5C
L85A4:  lda     $5C
        cmp     $08
        bne     L8592
        lda     $5B
        cmp     $07
        bne     L8592
        sec
        lda     $07
        sbc     $3C
        sta     $07
        lda     $08
        sbc     #$00
        sta     $08
        rts
L85BE:  lda     $07
        clc
        adc     $5A
        sta     $28
        lda     $08
        adc     #$00
        sta     $29
        jsr     L8BB6
        jsr     L8540
        lda     $07
        sta     $0B
        clc
        adc     $5A
        sta     $19
        lda     $08
        sta     $0C
        adc     #$00
        sta     $1A
L85E2:  ldy     #$00
        lda     ($0B),y
        sta     ($19),y
        jsr     L864A
        jsr     L8656
        lda     $5B
        cmp     $0B
        bne     L85E2
        lda     $5C
        cmp     $0C
        bne     L85E2
        lda     ($0B),y
        sta     ($19),y
        ldy     #$00
        lda     $5A
        sta     ($5B),y
        iny
        lda     $0F
        sta     ($5B),y
        iny
        lda     $10
        sta     ($5B),y
        iny
        clc
        lda     #$03
        adc     $5B
        sta     $5B
        lda     #$00
        adc     $5C
        sta     $5C
        lda     $5A
        sec
        sbc     #$03
        tax
        ldy     #$00
        lda     #$00
        sta     $4D
        lda     #$03
        sta     $4E
L862C:  lda     ($4D),y
        sta     ($5B),y
        iny
        dex
        bne     L862C
        clc
        lda     $07
        adc     $5A
        sta     $07
        lda     #$00
        adc     $08
        sta     $08
        ldy     #$00
        tya
        sta     ($07),y
        iny
        sta     ($07),y
        rts
L864A:  lda     $0B
        beq     L8651
        dec     $0B
        rts
L8651:  dec     $0B
        dec     $0C
        rts
L8656:  lda     $19
        beq     L865D
        dec     $19
        rts
L865D:  dec     $19
        dec     $1A
        rts
L8662:  lda     #$04
        sta     $B9
        sta     $BA
        sta     $BB
        lda     #$08
        sta     $C3
        sta     $C4
        sta     $C5
        lda     #$03
        sta     $BF
        sta     $C0
        sta     $C1
        lda     #$0F
        sta     $BC
        sta     $BD
        sta     $BE
        lda     #$30
        sta     $CF
        sta     $D0
        sta     $D1
        lda     #$02
        sta     $D2
        sta     $D3
        sta     $D4
        jsr     L84A6
        sta     $4C
        sta     $57
        lda     #$FF
        sta     $03
        lda     #$03
        sta     $56
        sta     $E3
        lda     #$6F
        sta     $04
        jsr     LAEE8
        jsr     LB194
        jmp     LB3AB
L86B0:  jsr     L97A6
        jsr     L88C0
        jsr     L84AB
        jsr     LB194
        jsr     LAEE8
        jsr     L849C
        bne     L86D6
        ldx     #$00
        txs
        lda     #$FF
        pha
        pha
        lda     $05
        sta     $09
        lda     $06
        sta     $0A
        jmp     L8471
L86D6:  jmp     L96C6
L86D9:  jsr     L90BF
        cmp     #$F6
        bne     L86E3
        jmp     L908C
L86E3:  jmp     L8491
L86E6:  php
        ldx     #$00
        stx     $86
        stx     $87
        dex
        stx     $88
        stx     $89
        plp
        beq     L8757
        cmp     #$2C
        beq     L872B
        cmp     #$2D
        beq     L872B
        cmp     #$2E
        bne     L870C
        lda     $11
        sta     $86
        sta     $88
        lda     $12
        jmp     L871A
L870C:  cmp     #$0B
        bne     L86E3
        jsr     L9098
        sta     $86
        sta     $88
        jsr     L9098
L871A:  sta     $87
        sta     $89
        jsr     L8499
        beq     L8757
        cmp     #$2C
        beq     L872B
        cmp     #$2D
        bne     L86E3
L872B:  jsr     L8499
        beq     L8751
        cmp     #$2E
        bne     L873D
        lda     $11
        sta     $88
        lda     $12
        jmp     L8749
L873D:  cmp     #$0B
        bne     L86E3
        jsr     L9098
        sta     $88
        jsr     L9098
L8749:  sta     $89
        jsr     L908C
        jmp     L8757
L8751:  lda     #$FF
        sta     $88
        sta     $89
L8757:  rts
L8758:  beq     L86E3
        ldx     #$FF
        stx     $51
        jsr     L86E6
        jsr     L876E
        jmp     L97A6
L8767:  jsr     L86E6
        lda     #$00
        sta     $51
L876E:  jsr     L9430
        lda     $86
        sta     $44
        lda     $87
        sta     $45
        jsr     L8540
        ldy     #$00
        lda     ($5B),y
        beq     L87DD
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
L878A:  ldy     #$00
        lda     ($09),y
        beq     L87DD
        iny
        lda     ($09),y
        sta     $28
        iny
        lda     ($09),y
        sta     $29
        lda     $88
        sta     $2C
        lda     $89
        sta     $2D
        jsr     L8FE2
        beq     L87A9
        bcs     L87DD
L87A9:  lda     $51
        beq     L87BB
        lda     $09
        sta     $5B
        lda     $0A
        sta     $5C
        jsr     L8581
        jmp     L878A
L87BB:  jsr     L87EE
        jsr     L87C4
        jmp     L878A
L87C4:  jsr     L87D7
        beq     L87D6
        cmp     #$1B
        bne     L87D6
L87CD:  jsr     L87D7
        beq     L87CD
        cmp     #$1B
        beq     L87CD
L87D6:  rts
L87D7:  jsr     L8855
        jmp     LB945
L87DD:  jmp     L9439
L87E0:  lda     #$3D
        clc
        adc     $46
        sta     $52
        lda     #$00
        sta     $53
        jmp     L893C
L87EE:  lda     #$00
        sta     $62
        ldy     #$01
        lda     ($09),y
        sta     $28
        iny
        lda     ($09),y
        sta     $29
        jsr     L913E
        jsr     L90D6
        jsr     L87E0
        lda     #$20
        jsr     L894D
        lda     #$03
        clc
        adc     $09
        sta     $4D
        lda     #$00
        adc     $0A
        sta     $4E
        jsr     L9442
        jsr     L8BAD
        jsr     L895A
        lda     $4D
        sta     $09
        lda     $4E
        sta     $0A
        jsr     LB56F
        lda     #$01
        clc
        adc     $4D
        sta     $09
        lda     #$00
        adc     $4E
        sta     $0A
        rts
L8855:  jsr     L883A
        beq     L885D
        rts
L885B:  pla
        pla
L885D:  lda     #$00
        sta     $63
        jsr     LB945
        bne     L885D
        jsr     LBE14
        jsr     L8883
        lda     #$FF
        sta     $24
        cmp     $0B
        bne     L887B
        cmp     $0C
        bne     L887B
        jsr     L88C0
L887B:  jmp     LB2D8
L887E:  lda     #$0E
        jmp     LB237
L8883:  lda     $0B
        sta     $11
        lda     $0C
        sta     $12
        lda     $0D
        sta     $13
        lda     $0E
        sta     $14
        lda     $58
        sta     $15
        lda     $59
        sta     $16
        rts
L889C:  lda     $24
        beq     L887E
        lda     $15
        sta     $09
        lda     $16
        sta     $0A
L88A8:  jsr     L9EF1
        jsr     L849C
        bne     L88A8
        lda     $13
        sta     $0D
        lda     $14
        sta     $0E
        lda     $11
        sta     $0B
        lda     $12
        sta     $0C
L88C0:  lda     #$00
        sta     $24
        rts
L88C5:  beq     L8939
        jsr     LA283
        lda     $27
        cmp     #$03
        beq     L88E6
        lda     $0400
        sta     $28
        lda     $0401
        sta     $29
        jsr     L9198
        jsr     L87E0
L88E0:  jsr     L895D
        jmp     L8919
L88E6:  lda     $0400
        beq     L8919
        ldy     #$00
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
L88F7:  lda     ($19),y
        cmp     #$20
        bcs     L88FF
        lda     #$00
L88FF:  sta     $0500,y
        iny
        dec     $0400
        bne     L88F7
        lda     #$00
        sta     $0500,y
        jsr     L8BAD
        jsr     L893C
        jmp     L88E0
        jsr     L908C
L8919:  jsr     L849C
        beq     L8939
        cmp     #$3B
        beq     L892B
        cmp     #$2C
        bne     L8933
        lda     #$09
        jsr     LB575
L892B:  jsr     L8499
        beq     L8938
        jmp     L8919
L8933:  jsr     L849C
        bne     L88C5
L8938:  rts
L8939:  jmp     LB56F
L893C:  ldy     #$00
        ldx     $62
L8940:  lda     ($52),y
        beq     L8938
        jsr     L894D
        iny
        jmp     L8940
L894B:  lda     #$00
L894D:  ldx     $62
        sta     $0700,x
        inc     $62
        beq     L8957
        rts
L8957:  dec     $62
        rts
L895A:  jsr     L893C
L895D:  jsr     L894B
        lda     $62
        beq     L8970
        lda     #$01
        ldx     #$00
        stx     $64
L896A:  sta     $63
L896C:  lda     $63
        bne     L896C
L8970:  rts
L8971:  pha
        lda     #$00
        sta     $2003
        lda     #$02
        sta     $4014
        lda     $AB
        bne     L898E
        lda     $AC
        beq     L8988
        lda     #$FF
        sta     $AB
L8988:  lda     $63
        bne     L8991
        pla
        rti
L898E:  jmp     LC8A3
L8991:  tax
        dex
        txa
        asl     a
        tax
        lda     L89A3,x
        sta     $1B
        lda     L89A4,x
        sta     $1C
        jmp     ($001B)
L89A3:
        .addr   L8A0C,L8A87,L8AC3,L89FA,L89D2,L8AF2,L8B30,L89E4
        .addr   L89F2,LA091,L89C4,L89C1,L8B3E,L8B74,L8B8C,nmi_read_group,nmi_write_group
L89C1:  lda     #$00
L89C3:  ldx     $33A5
        sta     $2001
L89C9:  lda     #$00
        sta     $63
        jsr     LB53C
        pla
        rti
L89D2:  jsr     L89D8
        jmp     L89C9
L89D8:  jsr     LB52E
        lda     $2007
        lda     $2007
        sta     $73
        rts
L89E4:  jsr     L89D8
        jsr     LB52E
        lda     $61
L89EC:  put_tile
        jmp     L89C9
L89F2:  jsr     LB52E
        lda     $73
        jmp     L89EC
L89FA:  jsr     LB52E
        ldx     $48
        lda     #$20
L8A01:  put_tile
        inx
        cpx     #$1C
        bne     L8A01
        jmp     L8ABA
L8A0C:  jsr     LB4F4
        ldx     $64
        jsr     LB52E
L8A14:  lda     $0700,x
        beq     L8A7F
        put_tile
        inc     $48
        lda     #$1C
        cmp     $48
        beq     L8A33
        lda     #$0E
        cmp     $48
        beq     L8A2D
        inx
        bne     L8A14
L8A2D:  inx
        stx     $64
        jmp     L8ABE
L8A33:  lda     #$00
        sta     $48
        txa
        pha
        ldx     $49
        lda     #$FF
        jsr     LB645
        pla
        tax
        inc     $49
        lda     #$18
        cmp     $49
        beq     L8A50
        inx
        stx     $64
        jmp     L8ABE
L8A50:  inx
        stx     $64
        jsr     LB748
        jsr     LB53C
        lda     $32
        jsr     video_write_ctrl
        jsr     LB4F4
        jsr     LB42B
        jsr     LB52E
        ldx     #$1C
        lda     #$20
L8A6B:  put_tile
        dex
        bne     L8A6B
        stx     $63
        jsr     LB53C
        jsr     LB5D4
        lda     #$01
        sta     $63
        pla
        rti
L8A7F:  sta     $63
        sta     $62
        sta     $64
        beq     L8ABE
L8A87:  lda     #$00
        sta     $48
        jsr     LB4F4
        jsr     LB52E
        ldy     #$1C
        lda     $2007
        ldx     $76
L8A98:  lda     $2007
        sta     $0500,x
        bit     $2007
        inx
        cpx     #$FF
        beq     L8AB3
        dey
        bne     L8A98
        stx     $76
        ldx     $49
        jsr     LB637
        beq     L8AB3
        inc     $49
        bne     L8ABE
L8AB3:  lda     #$FF
        .byte   $AE
        lda     #$00
        sta     $64
L8ABA:  lda     #$00
L8ABC:  sta     $63
L8ABE:  jsr     LB53C
        pla
        rti
L8AC3:  jsr     LB52E
        lda     $2007
        ldx     #$00
        ldy     $48
L8ACD:  lda     $2007
        sta     $0500,x
        bit     $2007
        inx
        iny
        cpy     #$1C
        bne     L8ACD
        lda     $1A
        ldx     $19
        jsr     L81EC
        ldx     #$00
        ldy     $48
L8AE4:  lda     $0500,x
        put_tile
        inx
        iny
        cpy     #$1C
        bne     L8AE4
        beq     L8ABA
; 调色板命令 6（背景）与 7（精灵）：$0700 是组号，$0701-$0704 是 4 个颜色。直接写 6576 的调色板（$2040 起，背景 16 项、精灵 16 项），
; 0 号色是公共背景色，写到背景 4 个组的 0 号色
L8AF2:  lda     $0700
        asl     a
        asl     a
        tax
L8AFC:  txa
        and     #$0C
        bne     palette_colors                  ; 只有组 0 的 0 号色是背景色，其他组的 0 号色 NES 不显示
        lda     $0701
        sta     $2040
palette_colors:
        lda     $0702
        sta     $2041,x
        lda     $0703
        sta     $2042,x
        lda     $0704
        sta     $2043,x
        jsr     LB53C
        jsr     LBE14
        jmp     L89C9
L8B30:  lda     $0700
        asl     a
        asl     a
        ora     #$10
        tax
        bne     L8AFC
L8B3E:  jsr     LB52E
        ldy     $48
        ldx     #$01
        lda     $B0
        sta     $0500
        lda     $2007
L8B4D:  lda     $2007
        sta     $0500,x
        bit     $2007
        inx
        iny
        cpy     #$1C
        bne     L8B4D
        sta     $B0
        jsr     LB52E
        ldx     #$00
        ldy     $48
L8B62:  lda     $0500,x
        put_tile
        inx
        iny
        cpy     #$1C
        bne     L8B62
L8B6E:  jsr     LB53C
        jmp     L89C9
L8B74:  jsr     LB52E
        ldx     #$00
        lda     $2007
L8B7C:  lda     $2007
        sta     $0500,x
        bit     $2007
        inx
        cpx     #$1C
        bne     L8B7C
        lda     #$0F
        jmp     L8ABC
L8B8C:  inc     $49
        jsr     LB4F4
        jsr     LB52E
        ldx     #$00
L8B96:  lda     $0500,x
        put_tile
        inx
        cpx     #$1C
        bne     L8B96
        jmp     L8B6E
L8BA4:  lda     #$00
        sta     $09
        lda     #$05
        sta     $0A
        rts
L8BAD:  lda     #$00
        sta     $52
        lda     #$05
        sta     $53
L8BB5:  rts
L8BB6:  lda     $03
        sec
        sbc     #$04
        sta     $2C
        lda     $04
        sbc     #$00
        sta     $2D
        jsr     L8FE2
        bcc     L8BB5
L8BC8:  lda     #$06
        jmp     LB237
L8BCD:  tsx
        cpx     #$14
        bcc     L8BC8
L8BD2:  rts
L8BD3:  lda     #$00
        sta     $E2
        jmp     L8491
L8BDA:  php
        pha
        lda     #$0A
        sta     $E2
        sta     $A0
        sta     $A2
        lda     #$00
        sta     $A1
        sta     $A3
        pla
        plp
        beq     L8BD2
        cmp     #$2C
        beq     L8C08
        cmp     #$0B
        bne     L8BD3
        jsr     L908C
        jsr     L9088
        sta     $A0
        jsr     L9088
        sta     $A1
        jsr     L849C
        beq     L8BD2
L8C08:  jsr     LAC14
        jsr     L90BF
        cmp     #$0B
        bne     L8BD3
        jsr     L908C
        jsr     L9088
        sta     $A2
        jsr     L9088
        sta     $A3
        rts
L8C20:  ldy     #$00
        sta     ($09),y
        jmp     L908C
L8C27:  jmp     L8491
L8C2A:  jmp     L9439
L8C2D:  jmp     L8494
L8C30:  php
        pha
        lda     #$0A
        sta     $7C
        sta     $80
        lda     #$00
        sta     $7D
        sta     $7E
        sta     $7F
        sta     $81
        pla
        plp
        beq     L8C9D
        cmp     #$2C
        beq     L8C60
        cmp     #$0B
        bne     L8C27
        jsr     L908C
        jsr     L9088
        sta     $7C
        jsr     L9088
        sta     $7D
        jsr     L849C
        beq     L8C9D
L8C60:  jsr     LAC14
        jsr     L90BF
        cmp     #$2C
        beq     L8C80
        cmp     #$0B
        bne     L8C27
        jsr     L908C
        jsr     L9088
        sta     $7E
        jsr     L9088
        sta     $7F
        jsr     L849C
        beq     L8C9D
L8C80:  jsr     LAC14
        jsr     L90BF
        cmp     #$0B
        bne     L8C27
        jsr     L908C
        jsr     L9088
        sta     $80
        jsr     L9088
        sta     $81
        bne     L8C9D
        lda     $80
        beq     L8C2D
L8C9D:  lda     $7D
        cmp     $7F
        beq     L8CA5
        bcs     L8CB1
L8CA5:  lda     $7C
        cmp     $7E
        bcs     L8CB1
        jmp     L8494
L8CAE:  jmp     L8C2A
L8CB1:  jsr     L9430
        jsr     L8DB1
        lda     $05
        sta     $09
        lda     $06
        sta     $0A
        jsr     L90B2
        beq     L8CAE
        jsr     L9F10
L8CC7:  jsr     L90B2
        beq     L8CD6
        cmp     #$0B
        beq     L8CE1
        jsr     L9EF1
        jmp     L8CC7
L8CD6:  jsr     L9098
        beq     L8D17
        jsr     L9F10
        jmp     L8CC7
L8CE1:  lda     $09
        pha
        lda     $0A
        pha
        jsr     L908C
        jsr     L9088
        sta     $44
        jsr     L9088
        sta     $45
        jsr     L8540
        beq     L8CFF
        lda     #$FF
        sta     $5B
        sta     $5C
L8CFF:  pla
        sta     $0A
        pla
        sta     $09
        lda     #$0C
        jsr     L8C20
        lda     $5B
        jsr     L8C20
        lda     $5C
        jsr     L8C20
        jmp     L8CC7
L8D17:  lda     $7E
        sta     $44
        lda     $7F
        sta     $45
        jsr     L8540
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
L8D2A:  jsr     L90B2
        beq     L8D5C
        pha
        jsr     L908C
        lda     $7C
        jsr     L8C20
        lda     $7D
        jsr     L8C20
        pla
        sec
        sbc     #$03
        clc
        adc     $09
        sta     $09
        lda     #$00
        adc     $0A
        sta     $0A
        clc
        lda     $80
        adc     $7C
        sta     $7C
        lda     $81
        adc     $7D
        sta     $7D
        jmp     L8D2A
L8D5C:  lda     $05
        sta     $09
        lda     $06
        sta     $0A
        jsr     L9F10
L8D67:  jsr     L90B2
        beq     L8D79
        cmp     #$0C
        beq     L8D84
        jsr     L9EF1
        jmp     L8D67
L8D76:  jmp     L9439
L8D79:  jsr     L9098
        beq     L8D76
        jsr     L9F10
        jmp     L8D67
L8D84:  ldy     #$00
        lda     #$0B
        sta     ($09),y
        iny
        lda     ($09),y
        sta     $5B
        iny
        lda     ($09),y
        sta     $5C
        cmp     #$FF
        bne     L8DA4
        lda     $5B
        cmp     #$FF
        bne     L8DA4
L8D9E:  jsr     L9F10
        jmp     L8D67
L8DA4:  dey
        lda     ($5B),y
        sta     ($09),y
        iny
        lda     ($5B),y
        sta     ($09),y
        jmp     L8D9E
L8DB1:  lda     $7C
        sta     $84
        lda     $7D
        sta     $85
        lda     $7E
        sta     $44
        lda     $7F
        sta     $45
        jsr     L8540
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
L8DCC:  jsr     L90B2
        beq     L8DEE
        clc
        adc     $09
        sta     $09
        lda     #$00
        adc     $0A
        sta     $0A
        clc
        lda     $80
        adc     $84
        sta     $84
        lda     $81
        adc     $85
        sta     $85
        bcs     L8DEF
        jmp     L8DCC
L8DEE:  rts
L8DEF:  jmp     L8494
L8DF2:  lda     $29
        cmp     #$80
        beq     L8DF9
        rts
L8DF9:  lda     $28
        sec
        rts
L8DFD:  sec
        lda     #$00
        sbc     $2C
        sta     $2C
        lda     #$00
        sbc     $2D
        sta     $2D
        rts
L8E0B:  lda     #$02
        sta     $27
L8E0F:  jsr     LAA0D
        jsr     L8DF2
        beq     L8E40
        jsr     L8E32
        lda     $29
        bmi     L8E2F
        lda     $2D
        bmi     L8E2F
        jsr     L8EF7
        jsr     L8DF2
        beq     L8E3F
        bcc     L8E32
        jmp     L8EF2
L8E2F:  jsr     L8EAC
L8E32:  sec
        lda     #$00
        sbc     $28
        sta     $28
        lda     #$00
        sbc     $29
        sta     $29
L8E3F:  rts
L8E40:  lda     $2D
        bpl     L8E4A
        jsr     L8DFD
        jmp     L8EAC
L8E4A:  jmp     L8EF2
L8E4D:  lda     $2C
        sta     $28
        lda     $2D
        sta     $29
        lda     $2E
        sta     $2A
L8E59:  rts
L8E5A:  ldx     $21
        stx     $4A
        lda     $28
        beq     L8E4D
        clc
        ldy     $2C
        beq     L8E59
        adc     $2C
        cmp     #$20
        bcs     L8E4A
        sta     $4B
        lda     $29
        sta     $19
        lda     $2A
        sta     $1A
        ldy     #$00
L8E79:  lda     ($19),y
        sta     $0700,x
        inx
        iny
        dec     $28
        bne     L8E79
        lda     $2D
        sta     $19
        lda     $2E
        sta     $1A
        ldy     #$00
L8E8E:  lda     ($19),y
        sta     $0700,x
        iny
        inx
        dec     $2C
        bne     L8E8E
        stx     $21
        lda     $4B
        sta     $28
        lda     $4A
        sta     $29
        lda     #$07
        sta     $2A
        rts
L8EA8:  lda     #$02
        sta     $27
L8EAC:  lda     $27
        cmp     #$03
        beq     L8E5A
        lda     $29
        bmi     L8ED1
        lda     $2D
        bmi     L8EC0
        jsr     L8EF7
        bmi     L8EF2
        rts
L8EC0:  jsr     L8DFD
L8EC3:  sec
        lda     $28
        sbc     $2C
        sta     $28
        lda     $29
        sbc     $2D
        sta     $29
        rts
L8ED1:  lda     $2D
        bmi     L8EDE
        jsr     L8E32
        jsr     L8EC3
        jmp     L8E32
L8EDE:  jsr     L8E32
        jsr     L8DFD
        jsr     L8EF7
        jsr     L8DF2
        beq     L8EF1
        bcs     L8EF2
        jmp     L8E32
L8EF1:  rts
L8EF2:  lda     #$05
        jmp     LB237
L8EF7:  clc
        lda     $2C
        adc     $28
        sta     $28
        lda     $2D
        adc     $29
        sta     $29
        rts
L8F05:  lda     #$00
        ldx     #$08
L8F09:  sta     $34,x
        dex
        bne     L8F09
        rts
L8F0F:  lda     $29
        bmi     L8F24
        lda     $2D
        bmi     L8F28
L8F17:  jsr     L8F35
        jsr     L8F3D
        beq     L8F43
        lda     $29
        bmi     L8EF2
        rts
L8F24:  lda     $2D
        bmi     L8F17
L8F28:  jsr     L8F35
        jsr     L8F3D
        beq     L8F43
        lda     $29
        bpl     L8EF2
        rts
L8F35:  jsr     L8F44
        lda     $36
        bne     L8EF2
        rts
L8F3D:  lda     $28
        bne     L8F43
        lda     $29
L8F43:  rts
L8F44:  jsr     LAA0D
L8F47:  jsr     L8F05
        jsr     L8F7D
        pha
        ldx     #$10
        clc
L8F51:  ror     $29
        ror     $28
        bcc     L8F64
        clc
        lda     $36
        adc     $2C
        sta     $36
        lda     $37
        adc     $2D
        sta     $37
L8F64:  ror     $37
        ror     $36
        ror     $35
        ror     $34
        dex
        bne     L8F51
        lda     $34
        sta     $28
        lda     $35
        sta     $29
L8F77:  pla
        beq     L8F9C
        jmp     L8E32
L8F7D:  lda     #$00
        sta     $34
        lda     $29
        bpl     L8F8C
        jsr     L8E32
        lda     #$FF
        sta     $34
L8F8C:  lda     $2D
        bpl     L8F9A
        jsr     L8DFD
        lda     #$FF
        sec
        sbc     $34
        sta     $34
L8F9A:  lda     $34
L8F9C:  rts
L8F9D:  jsr     LAA0D
        jsr     L8F7D
        pha
        lda     #$00
        sta     $2A
        sta     $2B
        ldx     #$10
L8FAC:  asl     $28
        rol     $29
        rol     $2A
        rol     $2B
        lda     $2A
        sec
        sbc     $2C
        lda     $2B
        sbc     $2D
        bcc     L8FCE
        lda     $2A
        sec
        sbc     $2C
        sta     $2A
        lda     $2B
        sbc     $2D
        sta     $2B
        inc     $28
L8FCE:  dex
        bne     L8FAC
        jmp     L8F77
L8FD4:  sec
        lda     $28
        sbc     $2C
        sta     $28
        lda     $29
        sbc     $2D
        sta     $29
        rts
L8FE2:  lda     $29
        cmp     $2D
        beq     L8FE9
        rts
L8FE9:  lda     $28
        cmp     $2C
        rts
L8FEE:  lda     $27
        cmp     #$03
        beq     L9035
        lda     $29
        bmi     L901B
        lda     $2D
        bmi     L9008
        lda     $29
        cmp     $2D
        beq     L900C
        bcs     L9008
L9004:  clc
        lda     #$FF
        rts
L9008:  sec
        lda     #$FF
        rts
L900C:  lda     $28
        cmp     $2C
        beq     L9017
        bcs     L9008
        jmp     L9004
L9017:  sec
        lda     #$00
        rts
L901B:  lda     $2D
        bpl     L9004
        lda     $29
        cmp     $2D
        beq     L902A
        bcs     L9008
        jmp     L9004
L902A:  lda     $28
        cmp     $2C
        beq     L9017
        bcs     L9008
        jmp     L9004
L9035:  lda     $29
        sta     $19
        lda     $2A
        sta     $1A
        lda     $2D
        sta     $1B
        lda     $2E
        sta     $1C
        lda     $28
        beq     L9071
        sta     $4A
        beq     L9008
        lda     $2C
        sta     $4B
        ldy     #$00
L9053:  lda     ($19),y
        cmp     ($1B),y
        bcc     L9004
        beq     L905E
        jmp     L9008
L905E:  iny
        dec     $4A
        beq     L906A
        dec     $4B
        beq     L9008
        jmp     L9053
L906A:  dec     $4B
        beq     L9017
        jmp     L9004
L9071:  lda     $2C
        beq     L9017
        jmp     L9004
L9078:  ldy     #$00
        sta     ($09),y
        inc     $09
        beq     L9081
        rts
L9081:  dec     $09
        lda     #$00
        sta     ($09),y
        rts
L9088:  ldy     #$00
        lda     ($09),y
L908C:  inc     $09
        beq     L9093
        and     #$FF
        rts
L9093:  inc     $0A
        and     #$FF
        rts
L9098:  ldy     #$00
        inc     $09
        beq     L90A1
        lda     ($09),y
        rts
L90A1:  inc     $0A
        lda     ($09),y
        rts
L90A6:  lda     $09
        beq     L90AD
        dec     $09
        rts
L90AD:  dec     $09
        dec     $0A
        rts
L90B2:  ldy     #$00
        lda     ($09),y
        rts
        ldy     #$00
        sta     ($09),y
        rts
L90BC:  jsr     L908C
L90BF:  ldy     #$00
        lda     ($09),y
        cmp     #$20
        beq     L90BC
        rts
L90C8:  cmp     #$30
        bcc     L90D2
        cmp     #$3A
        bcs     L90D2
        clc
        rts
L90D2:  sec
L90D3:  rts
L90D4:  inc     $46
L90D6:  ldx     $46
        lda     $3D,x
        beq     L90E1
        cmp     #$30
        beq     L90D4
        rts
L90E1:  dec     $46
        rts
L90E4:  jsr     L90BF
        cmp     #$2D
        bne     L912D
        jsr     L908C
        jsr     L90E4
        jmp     L8E32
        jsr     L912D
        jsr     L8DF2
        beq     L913B
        rts
L90FD:  lda     #$02
        sta     $27
        lda     #$00
        sta     $28
        sta     $29
L9107:  lda     #$0A
        sta     $2C
        lda     #$00
        sta     $2D
        ldy     #$00
        lda     ($09),y
        jsr     L90C8
        bcs     L90D3
        pha
        jsr     L8F35
        pla
        sec
        sbc     #$30
        sta     $2C
        jsr     L8EF7
        bcs     L913B
        jsr     L908C
        jmp     L9107
L912D:  jsr     L90FD
        lda     $29
        bmi     L9135
        rts
L9135:  jsr     L8DF2
        bne     L913B
        rts
L913B:  jmp     L8EF2
L913E:  lda     #$00
        sta     $46
L9142:  lda     $46
        pha
        lda     #$10
        sta     $2C
        lda     #$27
        sta     $2D
        jsr     L918A
        lda     #$E8
        sta     $2C
        lda     #$03
        sta     $2D
        jsr     L918A
        lda     #$64
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L918A
        lda     #$0A
        sta     $2C
        jsr     L918A
        lda     $28
        clc
        adc     #$30
        jsr     L9183
        lda     #$00
        jsr     L9183
        pla
        sta     $46
        rts
L917E:  jsr     L8EF7
        lda     $47
L9183:  ldx     $46
        sta     $3D,x
        inc     $46
        rts
L918A:  lda     #$30
        sta     $47
L918E:  jsr     L8FD4
        bcc     L917E
        inc     $47
        jmp     L918E
L9198:  lda     #$00
        sta     $46
        ldy     #$20
        lda     $29
        bpl     L91A7
        jsr     L8E32
        ldy     #$2D
L91A7:  tya
        pha
        jsr     L9142
        jsr     L90D6
        dec     $46
        pla
        jsr     L9183
        dec     $46
        rts
L91B8:  cmp     #$0A
        bcs     L91BF
        adc     #$30
        rts
L91BF:  clc
        adc     #$37
        rts
L91C3:  pha
        and     #$F0
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     L91B8
        sta     $3D,x
        inx
        pla
        and     #$0F
        jsr     L91B8
        sta     $3D,x
        inx
        rts
L91DA:  ldx     #$00
        stx     $46
        lda     $29
        jsr     L91C3
        lda     $28
        jsr     L91C3
        lda     #$00
        sta     $3D,x
L91EC:  rts
L91ED:  lda     #$02
        sta     $27
        ldx     #$00
        lda     #$00
        sta     $28
        sta     $29
L91F9:  jsr     L90B2
        sec
        sbc     #$30
        bcc     L91EC
        cmp     #$0A
        bcc     L9210
        sec
        sbc     #$07
        cmp     #$0A
        bcc     L91EC
        cmp     #$10
        bcs     L91EC
L9210:  pha
        jsr     L9224
        pla
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8EF7
        jsr     L908C
        jmp     L91F9
L9224:  txa
        pha
        lda     #$10
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8F35
        pla
        tax
        rts
L9234:  jsr     L9240
L9237:  ldy     #$00
        lda     ($4D),y
        rts
L923C:  ldy     #$00
        lda     ($4D),y
L9240:  inc     $4D
        bne     L9255
        inc     $4E
        bne     L9255
L9248:  lda     #$00
        jsr     L924D
L924D:  ldy     #$00
        sta     ($4D),y
        inc     $4D
        beq     L9258
L9255:  and     #$FF
        rts
L9258:  jmp     L8EF2
L925B:  lda     #$00
        sta     $4D
        lda     #$03
        sta     $4E
L9263:  jsr     L931E
        and     #$FF
L9268:  beq     L9248
        cmp     #$3F
        beq     L92AF
        cmp     #$22
        beq     L92B8
        cmp     #$27
        beq     L92B5
        cmp     #$2E
        beq     L9288
        cmp     #$26
        beq     L92F9
        jsr     L90C8
        bcc     L92CE
        jsr     L93E6
        bcc     L92B2
L9288:  jsr     L908C
        cmp     #$80
        bcs     L9263
        cmp     #$20
        bcc     L9263
        cmp     #$40
        bcc     L92A9
L9297:  jsr     L924D
        ldy     #$00
        lda     ($09),y
        jsr     LA7BC
        bcs     L9263
        jsr     L908C
        jmp     L9297
L92A9:  jsr     L924D
        jmp     L9263
L92AF:  jmp     L934C
L92B2:  jmp     L9353
L92B5:  ldy     #$00
        .byte   $AE
L92B8:  ldy     #$22
        sty     $99
L92BC:  jsr     L924D
        jsr     L908C
L92C2:  jsr     L90B2
        beq     L9268
        cmp     $99
        bne     L92BC
        jmp     L9288
L92CE:  lda     #$12
        pha
        jsr     L912D
        lda     $29
        bne     L92E5
        lda     $28
        cmp     #$0A
        bcs     L92E5
        tax
        pla
        inx
        txa
        jmp     L9317
L92E5:  jsr     L8DF2
        beq     L92ED
        jmp     L930C
L92ED:  lda     $99
        cmp     #$FA
        bne     L92F6
        jmp     L930C
L92F6:  jmp     L8EF2
L92F9:  jsr     L908C
        jsr     L90B2
        cmp     #$48
        bne     L9306
        jsr     L908C
L9306:  lda     #$11
        pha
        jsr     L91ED
L930C:  pla
        jsr     L924D
        lda     $28
        jsr     L924D
        lda     $29
L9317:  jsr     L924D
        jmp     L9263
L931D:  rts
L931E:  jsr     L90B2
        cmp     #$20
        bne     L931D
        jsr     L924D
        jsr     L908C
        jmp     L931E
L932E:  ldy     #$00
        sta     $99
        jmp     L92C2
L9335:  jsr     L931E
        jsr     L90C8
        bcc     L9380
        cmp     #$2C
        beq     L939D
        cmp     #$2D
        beq     L939D
        cmp     #$2E
        beq     L939D
L9349:  jmp     L9263
L934C:  lda     #$8B
        sta     $99
        jsr     L908C
L9353:  lda     $99
        jsr     L924D
        cmp     #$95
        beq     L932E
        cmp     #$86
        beq     L9335
        cmp     #$B6
        beq     L9335
        cmp     #$B5
        beq     L9335
        cmp     #$B7
        beq     L9335
        cmp     #$BD
        beq     L9378
        cmp     #$91
        beq     L93AE
        cmp     #$87
        bcs     L9349
L9378:  jsr     L931E
        jsr     L90C8
        bcs     L9349
L9380:  jsr     L90FD
        lda     #$0B
        jsr     L924D
        lda     $28
        jsr     L924D
        lda     $29
        jsr     L924D
        jsr     L931E
L9395:  cmp     #$2C
        beq     L939D
        cmp     #$2D
        bne     L9349
L939D:  jsr     L924D
        jsr     L908C
        jsr     L931E
        jsr     L90C8
        bcc     L9380
        jmp     L9395
L93AE:  jsr     L931E
        cmp     #$3A
        beq     L9349
        and     #$FF
L93B7:  beq     L9349
        cmp     #$22
        beq     L93C6
L93BD:  jsr     L924D
        jsr     L908C
        jmp     L93AE
L93C6:  jsr     L924D
        jsr     L908C
        jsr     L931E
        cmp     #$22
        beq     L93BD
        and     #$FF
        beq     L93B7
        jmp     L93C6
L93DA:  ldy     #$00
        lda     ($54),y
L93DE:  rts
L93DF:  inc     $54
        bne     L93DE
        inc     $55
        rts
L93E6:  lda     #<LCCAB
        sta     $54
        lda     #>LCCAB
        sta     $55
L93EE:  jsr     L93DA
        tax
        inx
        beq     L942B
        sta     $99
        jsr     L93DF
        jsr     L9430
L93FD:  jsr     L90B2
        sta     $98
        jsr     L93DA
        cmp     $98
        bne     L9416
        jsr     L908C
        jsr     L93DF
        jsr     L93DA
        bpl     L93FD
L9414:  clc
        rts
L9416:  jsr     L9088
        cmp     #$2E
        beq     L9414
        jsr     L9439
L9420:  jsr     L93DA
        bmi     L93EE
        jsr     L93DF
        jmp     L9420
L942B:  jsr     L90B2
        sec
        rts
L9430:  lda     $09
        sta     $9A
        lda     $0A
        sta     $9B
        rts
L9439:  lda     $9A
        sta     $09
        lda     $9B
        sta     $0A
        rts
L9442:  jsr     L8BA4
L9445:  jsr     L9237
        beq     L9461
        cmp     #$20
        bcc     L9466
        cmp     #$80
        bcc     L94AE
        pha
        jsr     L94DE
        jsr     L9240
        pla
        cmp     #$95
        beq     L94D0
        jmp     L9445
L9461:  lda     #$00
        jmp     L9078
L9466:  cmp     #$0B
        bcs     L9478
        sta     $28
        dec     $28
        lda     #$00
        sta     $29
        jsr     L9240
        jmp     L949B
L9478:  pha
        jsr     L9234
        sta     $28
        jsr     L9234
        sta     $29
        jsr     L9240
        pla
        cmp     #$11
        bne     L949B
        lda     #$26
        jsr     L9078
        lda     #$48
        jsr     L9078
        jsr     L91DA
        jmp     L949E
L949B:  jsr     L913E
L949E:  jsr     L90D6
        ldx     $46
L94A3:  lda     $3D,x
        beq     L9445
        jsr     L9078
        inx
        jmp     L94A3
L94AE:  cmp     #$22
        bne     L94C9
L94B2:  jsr     L94D8
        jsr     L9237
        cmp     #$22
        beq     L94C3
        and     #$FF
        bne     L94B2
        jmp     L9445
L94C3:  jsr     L94D8
        jmp     L9445
L94C9:  cmp     #$27
        bne     L94C3
L94CD:  jsr     L94D8
L94D0:  jsr     L9237
        bne     L94CD
        jmp     L9445
L94D8:  jsr     L923C
        jmp     L9078
L94DE:  sta     $99
        lda     #<LCCAB
        sta     $54
        lda     #>LCCAB
        sta     $55
        jsr     L93DA
L94EB:  cmp     #$FF
        beq     L950F
        cmp     $99
        beq     L94FE
L94F3:  jsr     L93DF
        jsr     L93DA
        bpl     L94F3
        jmp     L94EB
L94FE:  jsr     L93DF
L9501:  jsr     L93DA
        bmi     L950F
        jsr     L9078
        jsr     L93DF
        jmp     L9501
L950F:  rts
L9510:  jsr     LA26E
        jmp     LB237
L9516:  lda     #$13
        jmp     LB237
L951B:  lda     #$00
        sta     $E7
        lda     #$12
        jmp     LB237
L9524:  jmp     L8491
L9527:  php
        ldx     $E7
        beq     L9516
        cpx     #$01
        beq     L951B
        plp
        beq     L9553
        cmp     #$0B
        beq     L9549
        cmp     #$8D
        bne     L9524
        jsr     L9553
L953E:  jsr     L849C
        beq     L956F
        jsr     L9EF1
        jmp     L953E
L9549:  jsr     L96F0
        lda     #$01
        sta     $E7
        jmp     L96C9
L9553:  lda     #$01
        sta     $E7
        lda     $F6
        sta     $09
        lda     $F7
        sta     $0A
        lda     $F8
        sta     $0D
        lda     $F9
        sta     $0E
        lda     $FA
        sta     $0B
        lda     $FB
        sta     $0C
L956F:  rts
L9570:  jmp     L9439
L9573:  jsr     LA27D
        jsr     L9430
        lda     $05
        sta     $09
        lda     $06
        sta     $0A
L9581:  jsr     L90B2
        beq     L9570
        clc
        lda     $09
        adc     #$03
        sta     $4D
        lda     #$00
        adc     $0A
        sta     $4E
        lda     $09
        pha
        lda     $0A
        pha
        jsr     L9442
        pla
        sta     $0A
        pla
        sta     $09
        lda     $0400
        sta     $2C
        lda     $0401
        sta     $2D
        lda     $0402
        sta     $2E
        lda     #$00
        sta     $29
        lda     #$05
        sta     $2A
        ldy     #$00
L95BB:  lda     ($29),y
        beq     L95C2
        iny
        bne     L95BB
L95C2:  sty     $28
        jsr     L95E6
        beq     L95D2
        jsr     L87EE
        jsr     L87C4
        jmp     L9581
L95D2:  jsr     L90B2
        clc
        adc     $09
        sta     $09
        lda     #$00
        adc     $0A
        sta     $0A
        jmp     L9581
L95E3:  lda     #$00
        rts
L95E6:  lda     #$00
        sta     $6B
        sta     $6C
        lda     $28
        sta     $19
        cmp     $2C
        bcc     L95E3
L95F4:  lda     $2C
        sta     $1B
        lda     $6B
        pha
L95FB:  ldy     $6B
        lda     ($29),y
        ldy     $6C
        cmp     ($2D),y
        bne     L9615
        dec     $1B
        beq     L9610
        inc     $6B
        inc     $6C
        jmp     L95FB
L9610:  pla
        clc
        adc     #$01
        rts
L9615:  pla
        sta     $6B
        inc     $6B
        lda     #$00
        sta     $6C
        dec     $19
        beq     L95E3
        jmp     L95F4
L9625:  ldy     #$FF
        sta     $74
        bmi     L962F
L962B:  ldy     #$00
        sty     $74
L962F:  cmp     #$9A
        beq     L963A
        cmp     #$9B
        bne     L96A3
        lda     #$00
        .byte   $2C
L963A:  lda     #$FF
        ldy     $74
        beq     L9643
        sta     $E3
        .byte   $2C
L9643:  sta     $97
        jmp     L908C
L9648:  tax
        pla
        sta     $5F
        pla
        sta     $60
        txa
        jsr     L96F0
        jsr     L9B56
        lda     $09
        pha
        lda     $0A
        pha
        lda     $0B
        pha
        lda     $0C
        pha
        lda     $0D
        pha
        lda     $0E
        pha
        lda     #$00
        pha
        lda     #$FF
        pha
        lda     $60
        pha
        lda     $5F
        pha
        jmp     L96C9
L9677:  lda     #$02
        jmp     LB237
L967C:  pla
        sta     $5F
        pla
        sta     $60
        pla
        cmp     #$FF
        bne     L9677
        pla
        bne     L9677
        pla
        sta     $0E
        pla
        sta     $0D
        pla
        sta     $0C
        pla
        sta     $0B
        pla
        sta     $0A
        pla
        sta     $09
L969C:  lda     $60
        pha
        lda     $5F
        pha
        rts
L96A3:  jmp     L8491
L96A6:  beq     L967C
        pla
        sta     $5F
        pla
        sta     $60
        pla
        cmp     #$FF
        bne     L9677
        pla
        bne     L9677
        tsx
        txa
        clc
        adc     #$06
        tax
        txs
        lda     $60
        pha
        lda     $5F
        pha
        jsr     L849C
L96C6:  jsr     L96F0
L96C9:  ldy     #$00
        lda     ($5B),y
        clc
        adc     $5B
        sta     $0D
        lda     #$00
        adc     $5C
        sta     $0E
        iny
        lda     ($5B),y
        sta     $0B
        iny
        lda     ($5B),y
        sta     $0C
        clc
        lda     #$03
        adc     $5B
        sta     $09
        lda     #$00
        adc     $5C
        sta     $0A
        rts
L96F0:  cmp     #$0B
        bne     L96A3
        ldy     #$01
        lda     ($09),y
        sta     $44
        iny
        lda     ($09),y
        sta     $45
        jsr     L8540
        bne     L9705
L9704:  rts
L9705:  lda     #$07
        jmp     LB237
L970A:  jsr     LA283
        jmp     ($0400)
L9710:  jmp     L8494
L9713:  jmp     LBC20
L9716:  beq     L9713
        lda     #$00
        sta     $74
        jsr     LA283
        jsr     LAA0D
        lda     $0401
        bmi     L9710
        bne     L972E
        lda     $0400
        beq     L9704
L972E:  inc     $0401
L9731:  jsr     L8855
        dec     $74
        bne     L9731
        dec     $0400
        bne     L9731
        lda     $0401
        beq     L9747
        dec     $0401
        bne     L9731
L9747:  rts
L9748:  jsr     LA26E
        cmp     #$1C
        bcs     L9710
        sta     $6B
        jsr     LA26B
        cmp     #$18
        bcs     L9710
        sta     $6C
        jmp     LB65F
L975D:  jsr     LA283
        lda     $0400
        sta     $25
        lda     $0401
        sta     $26
        ldy     #$00
        sty     $46
L976E:  jsr     LA26B
        ldy     $46
        sta     ($25),y
        inc     $46
        jsr     L849C
        bne     L976E
        rts
L977D:  pla
        pla
        jmp     L8469
L9782:  jsr     LA283
        lda     $0400
        ora     $0401
        beq     L977D
        jsr     L849C
        beq     L97F3
        cmp     #$85
        beq     L979A
        rts
L9797:  jmp     L96C6
L979A:  jsr     L8499
        beq     L975D
        cmp     #$0B
        beq     L9797
        rts
L97A4:  bne     L97CB
L97A6:  lda     $08
        sta     $1E
        ldx     $07
        inx
        bne     L97B1
        inc     $1E
L97B1:  stx     $1D
        lda     $1E
        sta     $20
        ldx     $1D
        inx
        bne     L97BE
        inc     $20
L97BE:  stx     $1F
        ldy     #$00
        tya
        sta     ($1D),y
        iny
        sta     ($1D),y
        jmp     L9EE3
L97CB:  jsr     L97A6
        jsr     LA283
        jsr     LAA0D
        lda     $0401
        cmp     #$70
        bcs     L97F6
        cmp     $20
        bcc     L97F6
        bne     L97E8
        lda     $0400
        cmp     $1F
        bcc     L97F6
L97E8:  lda     $0400
        sta     $03
        lda     $0401
        sta     $04
        rts
L97F3:  jmp     L8491
L97F6:  jmp     L8494
L97F9:  jmp     LA9FF
L97FC:  pla
        sta     $5F
        pla
        sta     $60
        jsr     L90BF
        jsr     LA576
        lda     $90
        cmp     #$02
        bne     L97F9
        lda     $8C
        sta     $7C
        pha
        lda     $8D
        sta     $7D
        pha
        jsr     L86D9
        jsr     LA283
        pla
        sta     $8D
        pla
        sta     $8C
        ldy     #$00
        lda     $0400
        sta     ($8C),y
        lda     $0401
        iny
        sta     ($8C),y
        jsr     L90BF
        cmp     #$88
        bne     L97F3
        jsr     L908C
        jsr     LA283
        lda     $0400
        sta     $80
        lda     $0401
        sta     $81
        lda     #$01
        sta     $0400
        lda     #$00
        sta     $0401
        jsr     L849C
        beq     L9861
        cmp     #$89
        bne     L97F3
        jsr     L908C
        jsr     LA283
L9861:  lda     $0400
        sta     $82
        lda     $0401
        sta     $83
        lda     $09
        sta     $88
        lda     $0A
        sta     $89
        lda     $0D
        pha
        lda     $0E
        pha
        lda     $0B
        pha
        lda     $0C
        pha
        jmp     L98DF
L9882:  lda     #$00
        jmp     LB237
L9887:  pla
        sta     $5F
        pla
        sta     $60
        pla
        cmp     #$FD
        bne     L9882
        pla
        bne     L9882
        pla
        sta     $89
        pla
        sta     $88
        pla
        sta     $83
        pla
        sta     $82
        pla
        sta     $81
        pla
        sta     $80
        pla
        sta     $7D
        pla
        sta     $7C
        pla
        sta     $87
        pla
        sta     $86
        pla
        sta     $85
        pla
        sta     $84
        ldy     #$00
        lda     ($7C),y
        sta     $28
        iny
        lda     ($7C),y
        sta     $29
        jsr     L9904
        beq     L98CB
        bcc     L9901
L98CB:  lda     $84
        pha
        sta     $0D
        lda     $85
        pha
        sta     $0E
        lda     $86
        pha
        sta     $0B
        lda     $87
        pha
        sta     $0C
L98DF:  lda     $7C
        pha
        lda     $7D
        pha
        lda     $80
        pha
        lda     $81
        pha
        lda     $82
        pha
        lda     $83
        pha
        lda     $88
        pha
        sta     $09
        lda     $89
        pha
        sta     $0A
        lda     #$00
        pha
        lda     #$FD
        pha
L9901:  jmp     L969C
L9904:  lda     $83
        bmi     L9914
        jsr     L991A
        jsr     L8FEE
        bcs     L9912
        sec
        rts
L9912:  clc
        rts
L9914:  jsr     L991A
        jmp     L8FEE
L991A:  lda     $82
        sta     $2C
        lda     $83
        sta     $2D
        jsr     L8EA8
        ldy     #$00
        lda     $28
        sta     ($7C),y
        iny
        lda     $29
        sta     ($7C),y
        lda     $80
        sta     $2C
        lda     $81
        sta     $2D
L9938:  rts
L9939:  jmp     L8491
L993C:  beq     L9939
        cmp     #$22
        bne     L994D
        jsr     L9A51
        cmp     #$2C
        beq     L9952
        cmp     #$3B
        bne     L9939
L994D:  lda     #$3F
        jsr     LB575
L9952:  lda     $49
        sta     $7E
        lda     $48
        sta     $80
        jsr     LBD6C
        jsr     L8855
        lda     #$00
        sta     $52
L9964:  lda     $7E
        cmp     $47
        beq     L9975
        dec     $7E
        clc
        lda     #$1C
        adc     $52
        sta     $52
        bne     L9964
L9975:  lda     $80
        clc
        adc     $52
        sta     $52
L997C:  jsr     L90BF
        jsr     LA576
        jsr     L9430
        lda     $52
        sta     $09
        lda     #$05
        sta     $0A
        lda     $90
        and     #$0F
        cmp     #$03
        beq     L99D5
        jsr     LAB2D
        lda     $09
        sta     $52
        ldy     #$00
        lda     $28
        sta     ($8C),y
        iny
        lda     $29
        sta     ($8C),y
L99A7:  jsr     L9439
        jsr     L90BF
        cmp     #$2C
        bne     L9938
        jsr     L908C
        jsr     L9430
        lda     $52
        sta     $09
        lda     #$05
        sta     $0A
        ldy     #$00
        lda     ($09),y
        cmp     #$2C
        bne     L99CF
        inc     $52
        jsr     L9439
        jmp     L997C
L99CF:  jsr     L9439
        jmp     L994D
L99D5:  jsr     L90BF
        lda     #$00
        sta     $1B
        tay
        tax
L99DE:  lda     ($09),y
        beq     L99F2
        cmp     #$2C
        beq     L99F2
        sta     $0700,x
        inc     $1B
        inx
        jsr     L908C
        jmp     L99DE
L99F2:  lda     $1B
        sta     $0400
        lda     #$00
        sta     $0401
        lda     #$07
        sta     $0402
        jsr     L83CB
        lda     $09
        sta     $52
        jmp     L99A7
L9A0B:  jmp     L8491
L9A0E:  jsr     LA576
        lda     $90
        and     #$0F
        pha
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     LAC14
        jsr     L90BF
        jsr     LA576
        pla
        sta     $1A
        pla
        sta     $19
        lda     $90
        and     #$0F
        sta     $90
        pla
        cmp     $90
        bne     L9A4E
        tax
        cmp     #$02
        beq     L9A3D
        ldx     #$21
L9A3D:  ldy     #$00
L9A3F:  lda     ($8C),y
        pha
        lda     ($19),y
        sta     ($8C),y
        pla
        sta     ($19),y
        iny
        dex
        bne     L9A3F
        rts
L9A4E:  jmp     LA9FF
L9A51:  jsr     L908C
        ldy     #$00
        lda     ($09),y
        beq     L9A0B
        cmp     #$22
        beq     L9A64
        jsr     LB575
        jmp     L9A51
L9A64:  jsr     L8499
        beq     L9A0B
        jmp     L908C
L9A6C:  beq     L9A0B
        cmp     #$22
        bne     L9A7D
        jsr     L9A51
        cmp     #$2C
        beq     L9A7D
        cmp     #$3B
        bne     L9A0B
L9A7D:  jsr     LBD04
        jsr     L8855
        jsr     L90BF
        jsr     LA576
        lda     $90
        and     #$0F
        cmp     #$03
        bne     L9A4E
        ldx     #$00
        ldy     #$00
        lda     $0500,x
        cmp     #$20
        bne     L9AA3
        inx
        lda     $0500,x
        beq     L9AB0
        dex
L9AA3:  lda     $0500,x
        beq     L9AB0
        sta     $0700,x
        inx
        iny
        jmp     L9AA3
L9AB0:  sty     $0400
        lda     #$00
        sta     $0401
        lda     #$05
        sta     $0402
        jmp     L83CB
L9AC0:  jsr     L8499
        cmp     #$80
        bne     L9AFA
        jsr     L8499
        ldy     #$00
        lda     ($09),y
        cmp     #$0B
        bne     L9AFA
        iny
        lda     ($09),y
        bne     L9AE1
        iny
        lda     ($09),y
        sta     $E9
        bne     L9AE1
        sta     $E7
        rts
L9AE1:  jsr     L849C
        jsr     L96F0
        ldy     #$01
        lda     ($09),y
        sta     $E8
        iny
        lda     ($09),y
        sta     $E9
        jsr     L9B56
        lda     #$01
        sta     $E7
        rts
L9AFA:  jmp     L8491
L9AFD:  cmp     #$BC
        beq     L9AC0
        jsr     LA26E
        jsr     L90BF
        pha
        jsr     L908C
        ldx     $0400
        beq     L9B4E
L9B10:  jsr     L849C
        beq     L9B51
        cmp     #$0B
        bne     L9AFA
        dex
        beq     L9B2A
        jsr     L9F10
        jsr     L849C
        beq     L9B51
        jsr     LAC14
        jmp     L9B10
L9B2A:  pla
        cmp     #$BD
        beq     L9B48
        sec
        sbc     #$80
        cmp     #$05
        bcs     L9AFA
        asl     a
        tax
        lda     LCEBF,x
        sta     $19
        lda     LCEC0,x
        sta     $1A
        jsr     L90BF
        jmp     ($0019)
L9B48:  jsr     L849C
        jmp     L9527
L9B4E:  jsr     L9B56
L9B51:  pla
L9B52:  rts
L9B53:  jsr     L9F10
L9B56:  jsr     L849C
        beq     L9B52
        cmp     #$0B
        beq     L9B53
        jsr     LAC14
        jmp     L9B56
L9B65:  jmp     L8494
L9B68:  cmp     #$86
        beq     L9BB1
        jsr     LA26E
        beq     L9B65
        cmp     #$09
        bcs     L9B65
        pha
        jsr     LAC14
        jsr     LA27D
        pla
        sec
        sbc     #$01
        asl     a
        asl     a
        asl     a
        asl     a
        tax
        lda     $0400
        beq     L9BAB
        ldy     #$00
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
        lda     #$0F
        sta     $90
L9B9A:  lda     ($19),y
        sta     $0480,x
        inx
        iny
        dec     $0400
        beq     L9BAB
        dec     $90
        bne     L9B9A
        rts
L9BAB:  lda     #$00
        sta     $0480,x
        rts
L9BB1:  jsr     L908C
        lda     #$30
        sta     $90
        lda     #$00
        sta     $74
L9BBC:  lda     #$46
        jsr     L894D
        inc     $90
        lda     $90
        jsr     L894D
        lda     #$20
        jsr     L894D
        lda     $74
        asl     a
        asl     a
        asl     a
        asl     a
        tay
L9BD4:  lda     $0480,y
        beq     L9BFA
        cmp     #$20
        bcc     L9BE4
        jsr     L894D
        iny
        jmp     L9BD4
L9BE4:  pha
        lda     #$28
        jsr     L894D
        pla
        clc
        adc     #$40
        jsr     L894D
        lda     #$29
        jsr     L894D
        iny
        jmp     L9BD4
L9BFA:  jsr     L895D
        jsr     LB56F
        inc     $74
        lda     $74
        cmp     #$08
        bcc     L9BBC
        rts
L9C09:  jmp     L8491
L9C0C:  jsr     LA7C4
        bcs     L9C09
        sta     $7C
        lda     #$02
        sta     $84
        lda     #$00
        sta     $7D
        jsr     L9098
        jsr     LA7BC
        bcs     L9C28
        sta     $7D
        jsr     L908C
L9C28:  jsr     L90BF
        cmp     #$24
        bne     L9C34
        inc     $84
        jsr     L8499
L9C34:  cmp     #$28
        bne     L9C09
        lda     #$00
        sta     $82
        jsr     LA265
        sta     $7E
        jsr     L90BF
        cmp     #$29
        beq     L9C56
        inc     $82
        jsr     LA26B
        sta     $7F
        jsr     L849C
        cmp     #$29
        bne     L9C09
L9C56:  jsr     L908C
        jsr     L9C70
        jsr     L849C
        beq     L9C6A
        jsr     LAC14
        jsr     L90BF
        jmp     L9C0C
L9C6A:  rts
L9C6B:  lda     #$09
        jmp     LB237
L9C70:  jsr     L9D50
        bcs     L9C6B
        jsr     L9CD6
        lda     #$01
        sta     $2C
        jsr     LA7CE
        lda     $7E
        sta     $28
        inc     $28
        bne     L9C89
        inc     $29
L9C89:  lda     $82
        beq     L9C97
        lda     $7F
        sta     $2C
        inc     $2C
        bne     L9C97
        inc     $2D
L9C97:  jsr     L8F47
        jsr     LA704
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8F47
        lda     #$05
        sta     $2C
        lda     $82
        beq     L9CB0
        inc     $2C
L9CB0:  jsr     L8EF7
        bcs     L9CD3
        lda     $8C
        sta     $19
        sta     $2C
        lda     $8D
        sta     $1A
        sta     $2D
        jsr     L8EF7
        bcs     L9CD3
        lda     $04
        sta     $2D
        lda     $03
        sta     $2C
        jsr     L8FE2
        bcc     L9CE1
L9CD3:  jmp     L8BC8
L9CD6:  lda     #$FF
        cmp     $7E
        bne     L9CE0
        cmp     $7F
        beq     L9CD3
L9CE0:  rts
L9CE1:  ldy     #$00
        lda     $90
        sta     ($8C),y
        iny
        lda     $8A
        sta     ($8C),y
        iny
        lda     $8B
        sta     ($8C),y
        iny
        lda     $82
        sta     ($8C),y
        iny
        lda     $7E
        sta     ($8C),y
        lda     $82
        beq     L9D04
        iny
        lda     $7F
        sta     ($8C),y
L9D04:  lda     $28
        sta     $1F
        lda     $29
        sta     $20
        clc
        ldx     #$05
        lda     $82
        beq     L9D14
        inx
L9D14:  txa
        adc     $19
        sta     $19
        lda     #$00
        adc     $1A
        sta     $1A
L9D1F:  ldy     #$00
        lda     #$00
        sta     ($19),y
        inc     $19
        bne     L9D2B
        inc     $1A
L9D2B:  lda     $1A
        cmp     $20
        bne     L9D1F
        lda     $19
        cmp     $1F
        bne     L9D1F
        ldy     #$00
        lda     #$00
        sta     ($19),y
        iny
        sta     ($19),y
        iny
        sta     ($19),y
        rts
L9D44:  jsr     L908C
        jsr     L9E52
        jsr     L9E9D
        jmp     LC50D
L9D50:  lda     $7C
        sta     $8A
        lda     $7D
        sta     $8B
        lda     $84
        ora     #$80
        sta     $90
        jsr     LA70F
        jmp     LA608
L9D64:  cmp     #$53
        beq     L9D44
        jsr     L9E52
        jsr     L9E9D
        lda     #$02
        sta     $0500
        lda     $05
        sta     $0514
        sta     $2C
        lda     $06
        sta     $0515
        sta     $2D
        lda     $07
        sta     $28
        lda     $08
        sta     $29
        jsr     L8E0B
        lda     $28
        sta     $0512
        lda     $29
        sta     $0513
        jsr     L9D9F
        jsr     LBF1F
        jmp     LBF30
L9D9F:  lda     #<LB367
        sta     $52
        lda     #>LB367
        sta     $53
        jsr     L895A
        beq     L9DC4
L9DAC:  lda     #<LB370
        sta     $52
        lda     #>LB370
        sta     $53
        jsr     L895A
        beq     L9DC4
L9DC4:  lda     #$01
        sta     $52
        lda     #$05
        sta     $53
        jsr     L895A
        jmp     LB56F
L9DD2:  jmp     disk_error
L9DD8:  jsr     L908C
        jsr     L9E52
        jmp     LC565
L9DE1:  ldx     #$00
        stx     $9F
        cmp     #$53
        beq     L9DD8
        cmp     #$8B
        bne     L9DF2
        dec     $9F
        jsr     L908C
L9DF2:  jsr     L9E52
L9DF5:  jsr     LBE53
        bcs     L9E4D
        jsr     L9EB1
        bcs     L9DD2
        lda     $0500
        cmp     #$02
        bne     L9DD2
        jsr     L9DAC
        lda     $9F
        beq     L9E13
        jsr     LBE1C
        bcs     L9E4D
        rts
L9E13:  lda     $05
        sta     $0514
        lda     $06
        sta     $0515
        jsr     LBE24
        bcs     L9E4D
        lda     $AD
        sta     $07
        lda     $AE
        sta     $08
        jsr     L9E46
        jsr     LBDCE
        lda     $AD
        sta     $1D
        lda     $AE
        sta     $1E
        jsr     L9E46
        jsr     LBDCE
        lda     $AD
        sta     $1F
        lda     $AE
        sta     $20
L9E46:  ldy     #$00
        lda     #$00
        sta     ($AD),y
        rts
L9E4D:  lda     #$11
        jmp     LB237
L9E52:  jsr     L9E8A
        lda     #$00
        sta     $EA
        jsr     L849C
        beq     L9E89
        jsr     LA27D
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
        lda     $0400
        beq     L9E89
        ldy     #$00
        ldx     #$00
L9E74:  lda     ($19),y
        sta     $0580,x
        inx
        iny
        dec     $0400
        bne     L9E74
        lda     #$00
        sta     $0580,x
        lda     #$01
        sta     $EA
L9E89:  rts
L9E8A:  ldx     #$00
        ldy     #$10
        lda     #$20
L9E90:  sta     $0580,x
        inx
        dey
        bne     L9E90
        lda     #$00
        sta     $0580,x
        rts
L9E9D:  ldx     #$00
        ldy     #$10
L9EA1:  lda     $0580,x
        sta     $0501,x
        inx
        dey
        bne     L9EA1
        lda     #$00
        sta     $0501,x
        rts
L9EB1:  lda     $EA
        beq     L9EC5
        ldx     #$00
        ldy     #$10
L9EB9:  lda     $0501,x
        cmp     $0580,x
        bne     L9EC7
        inx
        dey
        bne     L9EB9
L9EC5:  clc
        rts
L9EC7:  sec
        rts
L9EC9:  beq     L9EE3
        jsr     L96F0
        jsr     L9B56
        lda     #$03
        clc
        adc     $5B
        sta     $B2
        lda     #$00
        adc     $5C
L9EDC:  sta     $B3
        lda     #$00
        sta     $B4
        rts
L9EE3:  lda     $05
        clc
        adc     #$03
        sta     $B2
        lda     #$00
        adc     $06
        jmp     L9EDC
L9EF1:  jsr     L90B2
        cmp     #$80
        bcs     L9F0D
        cmp     #$12
        beq     L9F10
        cmp     #$11
        beq     L9F10
        cmp     #$0B
        beq     L9F10
        cmp     #$22
        beq     L9F1D
        jsr     LA7C4
        bcc     L9F2B
L9F0D:  jmp     L908C
L9F10:  jsr     L908C
        jsr     L908C
        jmp     L908C
L9F19:  pla
        jmp     L908C
L9F1D:  pha
L9F1E:  jsr     L9098
        cmp     #$22
        beq     L9F19
        and     #$FF
        bne     L9F1E
        pla
        rts
L9F2B:  pha
L9F2C:  jsr     L9098
        jsr     LA7BC
        bcc     L9F2C
        pla
        rts
L9F36:  jsr     LA576
        jsr     L9430
        lda     $B2
        sta     $09
        lda     $B3
        sta     $0A
        lda     $B4
        bne     L9F6E
L9F48:  jsr     L90B2
        beq     L9F57
        cmp     #$91
        beq     L9F67
        jsr     L9EF1
        jmp     L9F48
L9F57:  jsr     L9098
        beq     L9F62
        jsr     L9F10
        jmp     L9F48
L9F62:  lda     #$03
        jmp     LB237
L9F67:  lda     #$FF
        sta     $B4
        jsr     L9EF1
L9F6E:  lda     $90
        and     #$0F
        cmp     #$03
        beq     L9FC8
        jsr     L849C
        beq     L9F62
        jsr     LAB2D
        ldy     #$00
        lda     $28
        sta     ($8C),y
        lda     $29
        iny
        sta     ($8C),y
        jsr     L849C
        beq     L9FA3
        jsr     L90BF
        cmp     #$2C
        beq     L9F98
        jmp     LA9FF
L9F98:  jsr     L908C
        jsr     L849C
        beq     L9F62
        jmp     L9FA7
L9FA3:  lda     #$00
        sta     $B4
L9FA7:  lda     $09
        sta     $B2
        lda     $0A
        sta     $B3
        jsr     L9439
        jsr     L90BF
        cmp     #$2C
        beq     L9FBA
L9FB9:  rts
L9FBA:  jsr     L908C
        jsr     L90BF
        jsr     LA7C4
        bcs     L9FB9
        jmp     L9F36
L9FC8:  lda     #$00
        sta     $1B
        tax
        lda     #$2C
        sta     $3C
        jsr     L849C
        beq     L9F62
        cmp     #$22
        bne     L9FDF
        sta     $3C
        jsr     L908C
L9FDF:  lda     ($09),y
        beq     LA005
        cmp     $3C
        beq     L9FF3
        sta     $0700,x
        inc     $1B
        inx
        jsr     L908C
        jmp     L9FDF
L9FF3:  jsr     L908C
        jsr     L849C
        beq     LA005
        cmp     #$2C
        bne     LA009
        jsr     L908C
        jmp     LA009
LA005:  lda     #$00
        sta     $B4
LA009:  lda     $1B
        sta     $0400
        lda     #$00
        sta     $0401
        lda     #$07
        sta     $0402
        jsr     L83CB
        jmp     L9FA7
LA01E:  jsr     LA27D
        lda     $0400
        beq     LA086
        lda     $09
        pha
        lda     $0A
        pha
        lda     $0401
        sta     $09
        lda     $0402
        sta     $0A
        lda     #$07
        sta     $92
        lda     #$00
        sta     $91
        sta     $C6
        jsr     LA241
        lda     #$21
        sta     $91
        sta     $C7
        jsr     LA241
        lda     #$42
        sta     $91
        sta     $C8
        jsr     LA241
        lda     #$01
        sta     $CB
        sta     $CC
        sta     $CD
        lda     #$7F
        sta     $4001
        sta     $4005
        lda     #$07
        sta     $C9
        sta     $4015
        lda     #$0A
        sta     $63
LA070:  jsr     L8855
        lda     $C9
        bmi     LA087
        bne     LA070
        lda     #$00
        sta     $63
        sta     $4015
        pla
        sta     $0A
        pla
        sta     $09
LA086:  rts
LA087:  lda     #$00
        sta     $63
        sta     $4015
        jmp     L8491
LA091:  txa
        pha
        tya
        pha
        lda     #$C0
        sta     $4017
        jsr     LA0A3
        pla
        tay
        pla
        tax
        pla
        rti
LA0A3:  lda     #$02
        sta     $CA
        lda     #$04
        sta     $CE
LA0AB:  lda     $C9
        bmi     LA0B8
        jsr     LA0B9
        lsr     $CE
        dec     $CA
        bpl     LA0AB
LA0B8:  rts
LA0B9:  lda     $C9
        and     $CE
        beq     LA0B8
        ldx     $CA
        dec     $CB,x
        bne     LA0B8
        lda     $C6,x
        sta     $91
        lda     #$00
        sta     $C2
LA0CD:  ldy     #$00
        lda     ($91),y
        bne     LA0E4
        lda     $C9
        sec
        sbc     $CE
        sta     $C9
        sta     $4015
        ldx     $CA
        lda     $91
        sta     $C6,x
        rts
LA0E4:  inc     $91
        cmp     #$20
        beq     LA0CD
        cmp     #$4D
        bne     LA106
        lda     ($91),y
        beq     LA16B
        ldx     #$30
        cmp     #$30
        beq     LA0FE
        cmp     #$31
        bne     LA16B
        ldx     #$00
LA0FE:  inc     $91
        ldy     $CA
        stx     $CF,y
        bpl     LA0CD
LA106:  cmp     #$4F
        bne     LA11F
        lda     ($91),y
        beq     LA16B
        inc     $91
        cmp     #$36
        bcs     LA16B
        sec
        sbc     #$30
        bcc     LA16B
        ldx     $CA
        sta     $BF,x
        bpl     LA0CD
LA11F:  cmp     #$54
        bne     LA13A
        lda     ($91),y
        beq     LA16B
        inc     $91
        cmp     #$39
        bcs     LA16B
        sec
        sbc     #$30
        beq     LA16B
        bcc     LA16B
        ldx     $CA
        sta     $B9,x
LA138:  bpl     LA0CD
LA13A:  cmp     #$56
        bne     LA170
        lda     ($91),y
        beq     LA16B
        inc     $91
        cmp     #$3A
        bcs     LA16B
        sec
        sbc     #$30
        bcc     LA16B
        cmp     #$01
        bne     LA165
        lda     ($91),y
        cmp     #$36
        bcs     LA163
        sec
        sbc     #$30
        bcc     LA163
        clc
        adc     #$0A
        inc     $91
        bne     LA165
LA163:  lda     #$01
LA165:  ldx     $CA
        sta     $BC,x
        bpl     LA138
LA16B:  lda     #$80
        sta     $C9
        rts
LA170:  cmp     #$59
        bne     LA18A
        lda     ($91),y
        cmp     #$34
        bcs     LA16B
        sec
        sbc     #$30
        bcc     LA16B
        inc     $91
        lsr     a
        ror     a
        ror     a
        ldx     $CA
        sta     $D2,x
        bpl     LA138
LA18A:  cmp     #$23
        bne     LA194
        lda     #$02
        sta     $C2
        bpl     LA138
LA194:  cmp     #$52
        bne     LA1AC
        jsr     LA20F
        lda     $CA
        asl     a
        asl     a
        tay
        lda     #$90
        cpy     #$08
        bne     LA1A8
        lda     #$00
LA1A8:  sta     $4000,y
        rts
LA1AC:  cmp     #$41
        bcc     LA16B
        cmp     #$48
        bcs     LA16B
        pha
        jsr     LA20F
        ldx     $CA
        txa
        asl     a
        asl     a
        tay
        cmp     #$08
        bne     LA1D9
        lda     $CB,x
        sec
        sbc     #$01
        cmp     #$0E
        bcc     LA1CD
        lda     #$0E
LA1CD:  asl     a
        asl     a
        ldy     $CF,x
        beq     LA1D5
        lda     #$81
LA1D5:  ldy     #$08
        bne     LA1DF
LA1D9:  lda     $BC,x
        ora     $D2,x
        ora     $CF,x
LA1DF:  sta     $4000,y
        pla
        sec
        sbc     #$41
        tay
        lda     $BF,x
        tax
        lda     L80B4,x
        clc
        adc     L80AD,y
        adc     $C2
        tax
        lda     $CA
        asl     a
        asl     a
        tay
        cmp     #$08
        bne     LA202
        txa
        clc
        adc     #$18
        tax
LA202:  lda     L8013,x
        sta     $4002,y
        lda     L8014,x
        sta     $4003,y
        rts
LA20F:  lda     ($91),y
        beq     LA226
        cmp     #$3A
        bcs     LA226
        sec
        sbc     #$30
        bcc     LA226
        tax
        lda     L80A3,x
        ldx     $CA
        sta     $C3,x
        inc     $91
LA226:  jsr     LA7CE
        ldx     $CA
        lda     $C3,x
        sta     $28
        lda     $B9,x
        sta     $2C
        jsr     L8F47
        lda     $28
        ldx     $CA
        sta     $CB,x
        lda     $91
        sta     $C6,x
        rts
LA241:  lda     $0400
        beq     LA258
        dec     $0400
        jsr     L9088
        cmp     #$3A
        beq     LA258
        ldy     #$00
        sta     ($91),y
        inc     $91
        bne     LA241
LA258:  ldy     #$00
        lda     #$00
        sta     ($91),y
        rts
LA25F:  jmp     L8491
LA262:  jmp     L8494
LA265:  jsr     L908C
        jmp     LA26E
LA26B:  jsr     LAC14
LA26E:  jsr     LA283
        jsr     LAA0D
        lda     $0401
        bne     LA262
        lda     $0400
        rts
LA27D:  jsr     LA283
        jmp     LAA10
LA283:  lda     #$00
        sta     $27
        sta     $30
        lda     #$80
        sta     $21
        jsr     LA29B
        lda     $27
        beq     LA25F
        rts
LA295:  jsr     LAA0D
        jmp     L908C
LA29B:  jsr     LA2BE
LA29E:  cmp     #$EF
        beq     LA2A3
        rts
LA2A3:  jsr     LA295
        jsr     LA2BE
        jsr     LA3C1
        lda     $28
        eor     $2C
        sta     $28
        lda     $29
        eor     $2D
        sta     $29
        jsr     LA7D8
        jmp     LA29E
LA2BE:  jsr     LA2E1
LA2C1:  cmp     #$F0
        beq     LA2C6
        rts
LA2C6:  jsr     LA295
        jsr     LA2E1
        jsr     LA3C1
        lda     $28
        ora     $2C
        sta     $28
        lda     $29
        ora     $2D
        sta     $29
        jsr     LA7D8
        jmp     LA2C1
LA2E1:  jsr     LA304
LA2E4:  cmp     #$F1
        beq     LA2E9
        rts
LA2E9:  jsr     LA295
        jsr     LA304
        jsr     LA3C1
        lda     $28
        and     $2C
        sta     $28
        lda     $29
        and     $2D
        sta     $29
        jsr     LA7D8
        jmp     LA2E4
LA304:  jsr     LA341
LA307:  cmp     #$F3
        bcs     LA30C
LA30B:  rts
LA30C:  cmp     #$F9
        bcs     LA30B
        sec
        sbc     #$F3
        asl     a
        tax
        lda     LA86E,x
        pha
        lda     LA86F,x
        pha
        lda     $27
        pha
        jsr     L908C
        jsr     LA341
        pla
        cmp     $27
        bne     LA370
        jsr     LA3C4
        pla
        sta     $1A
        pla
        sta     $19
        jsr     LA33E
        ldx     #$02
        stx     $27
        jmp     LA307
LA33E:  jmp     ($0019)
LA341:  jsr     LA373
LA344:  cmp     #$F9
        beq     LA34D
        cmp     #$FA
        beq     LA356
        rts
LA34D:  jsr     LA35F
        jsr     LA7D5
        jmp     LA344
LA356:  jsr     LA35F
        jsr     LA7EF
        jmp     LA344
LA35F:  lda     $27
        pha
        jsr     L908C
        jsr     LA373
        pla
        cmp     $27
        bne     LA370
        jmp     LA3C4
LA370:  jmp     LA9FF
LA373:  jsr     LA392
LA376:  cmp     #$FB
        beq     LA37B
        rts
LA37B:  lda     $27
        pha
        jsr     L908C
        jsr     LA392
        pla
        cmp     $27
        bne     LA370
        jsr     LA3C4
        jsr     LA80E
        jmp     LA376
LA392:  jsr     LA490
LA395:  cmp     #$FC
        beq     LA39E
        cmp     #$FD
        beq     LA3A7
        rts
LA39E:  jsr     LA3B0
        jsr     LA7FB
        jmp     LA395
LA3A7:  jsr     LA3B0
        jsr     LA7F5
        jmp     LA395
LA3B0:  lda     $27
        pha
        jsr     L908C
        jsr     LA490
        pla
        cmp     $27
        bne     LA370
        jmp     LA3C4
LA3C1:  jsr     LAA0D
LA3C4:  ldx     $30
        cpx     #$7D
        bcs     LA407
        dex
        dex
        dex
        stx     $30
        lda     $0400,x
        sta     $2C
        lda     $0401,x
        sta     $2D
        lda     $0402,x
        sta     $2E
        dex
        dex
        dex
        lda     $0400,x
        sta     $28
        lda     $0401,x
        sta     $29
        lda     $0402,x
        sta     $2A
        rts
LA3F1:  jsr     L908C
        jsr     LA490
        ldx     $30
        dex
        dex
        dex
        lda     $0400,x
        sta     $28
        lda     $0401,x
        sta     $29
        rts
LA407:  jmp     L8494
LA40A:  jsr     LA3F1
        jsr     L8E32
        lda     $28
        sta     $0400,x
        lda     $29
        sta     $0401,x
        jmp     L90BF
LA41D:  jsr     LA3F1
        lda     #$FF
        sec
        sbc     $0400,x
        sta     $0400,x
        lda     #$FF
        sec
        sbc     $0401,x
        sta     $0401,x
        jmp     LA50D
LA435:  jsr     L908C
        ldy     #$00
        ldx     $21
LA43C:  lda     ($09),y
        beq     LA453
        cmp     #$22
        beq     LA450
        sta     $0700,x
        inx
        iny
        cpy     #$20
        beq     LA407
        jmp     LA43C
LA450:  jsr     L908C
LA453:  stx     $19
        sty     $90
        tya
        clc
        adc     $09
        sta     $09
        lda     #$00
        adc     $0A
        sta     $0A
        ldx     $30
        lda     $90
        sta     $0400,x
        lda     $21
        sta     $0401,x
        lda     #$07
        sta     $0402,x
        inx
        inx
        inx
        stx     $30
        lda     $19
        sta     $21
        lda     #$03
        sta     $27
        jmp     L90BF
LA484:  jmp     L8491
LA487:  jmp     LA41D
LA48A:  jmp     LA40A
LA48D:  jsr     L908C
LA490:  jsr     L849C
        beq     LA484
        cmp     #$0B
        bcc     LA514
        cmp     #$FA
        beq     LA48A
        cmp     #$F2
        beq     LA487
        cmp     #$12
        beq     LA4F6
        cmp     #$11
        beq     LA4F6
        cmp     #$F9
        beq     LA48D
        cmp     #$28
        beq     LA4EA
        cmp     #$22
        beq     LA435
        cmp     #$41
        bcc     LA4E4
        cmp     #$5B
        bcc     LA52C
        cmp     #$A1
        beq     LA4E7
        cmp     #$CA
        bcc     LA4E4
        cmp     #$EF
        bcs     LA4E4
        sec
        sbc     #$CA
        asl     a
        tax
        lda     LCF41,x
        sta     $19
        lda     LCF42,x
        sta     $1A
        jsr     L908C
        jsr     L90BF
        jsr     LA33E
        jmp     L90BF
LA4E4:  jmp     L8491
LA4E7:  jmp     LA88E
LA4EA:  jsr     L908C
        jsr     LA29B
        jsr     LA87D
        jmp     L90BF
LA4F6:  ldx     $30
        ldy     #$01
        lda     ($09),y
        sta     $0400,x
        iny
        inx
        lda     ($09),y
        sta     $0400,x
        inx
        inx
        stx     $30
        jsr     L9F10
LA50D:  lda     #$02
        sta     $27
        jmp     L90BF
LA514:  ldx     $30
        sta     $0400,x
        dec     $0400,x
        lda     #$00
        sta     $0401,x
        inx
        inx
        inx
        stx     $30
        jsr     L908C
        jmp     LA50D
LA52C:  jsr     LA57F
        ldx     $30
        lda     $90
        and     #$0F
        cmp     #$03
        beq     LA55A
        ldy     #$00
        lda     ($8C),y
        sta     $0400,x
        iny
        lda     ($8C),y
        sta     $0401,x
        iny
        lda     ($8C),y
        sta     $0402,x
LA54C:  inx
        inx
        inx
        stx     $30
        lda     $90
        and     #$0F
        sta     $27
        jmp     L90BF
LA55A:  ldy     #$00
        lda     ($8C),y
        sta     $0400,x
        lda     #$01
        clc
        adc     $8C
        sta     $0401,x
        lda     #$00
        adc     $8D
        sta     $0402,x
        jmp     LA54C
LA573:  jmp     L8491
LA576:  jsr     LA7C4
        bcs     LA573
        ldx     #$00
        stx     $30
LA57F:  ldx     #$00
        stx     $8B
        sta     $8A
        jsr     L9098
        jsr     LA7BC
        bcs     LA597
        sta     $8B
LA58F:  jsr     L9098
        jsr     LA7BC
        bcc     LA58F
LA597:  lda     #$02
        sta     $90
        jsr     L90BF
        cmp     #$24
        bne     LA5A9
        lda     #$03
        sta     $90
        jsr     L908C
LA5A9:  jsr     L90BF
        cmp     #$28
        beq     LA620
        jsr     LA70F
        bcs     LA5FA
        ldy     #$00
        lda     $90
        sta     ($8E),y
        iny
        lda     $8A
        sta     ($8E),y
        iny
        lda     $8B
        sta     ($8E),y
        iny
        lda     #$00
        sta     ($8E),y
        iny
        sta     ($8E),y
        iny
        sta     ($8E),y
        lda     $90
        cmp     #$02
        beq     LA5E2
        lda     #$00
        ldx     #$21
LA5DA:  iny
        sta     ($8E),y
        dex
        bne     LA5DA
        lda     #$21
LA5E2:  clc
        adc     #$03
        adc     $8E
        sta     $28
        lda     #$00
        adc     $8F
        sta     $29
        jsr     L8BB6
        lda     $28
        sta     $1F
        lda     $29
        sta     $20
LA5FA:  clc
        lda     #$03
        adc     $8E
        sta     $8C
        lda     #$00
        adc     $8F
        sta     $8D
        rts
LA608:  lda     $8E
        sta     $8C
        lda     $8F
        sta     $8D
LA610:  rts
LA611:  jsr     LA29B
        lda     #$02
        cmp     $27
        beq     LA610
LA61A:  jmp     LA9FF
LA61D:  jmp     LA6BD
LA620:  lda     $8A
        pha
        lda     $8B
        pha
        lda     $90
        ora     #$80
        pha
        jsr     L908C
        jsr     LA611
        ldx     $30
        dex
        dex
        dex
        lda     $0400,x
        pha
        jsr     LA6F2
        bcc     LA61D
        jsr     LAC14
        jsr     LA611
        ldx     $30
        dex
        dex
        dex
        lda     $0400,x
        pha
        jsr     LA6F2
        bcs     LA61A
        jsr     L908C
        pla
        sta     $1B
        pla
        sta     $19
        pla
        sta     $90
        pla
        sta     $8B
        pla
        sta     $8A
        jsr     LA70F
        bcc     LA6BA
        ldy     #$03
        lda     ($8E),y
        beq     LA6BA
        iny
        lda     ($8E),y
        cmp     $19
        bcc     LA6BA
        tax
        iny
        lda     ($8E),y
        cmp     $1B
        bcc     LA6BA
        jsr     LA7CE
        inx
        bne     LA687
        inc     $29
LA687:  stx     $28
        lda     $1B
        sta     $2C
        jsr     L8F47
        lda     $19
        sta     $2C
        jsr     L8EA8
        jsr     LA704
        sta     $2C
        jsr     L8F47
        lda     #$06
LA6A1:  sta     $2C
        jsr     L8EA8
        lda     $8E
        sta     $2C
        lda     $8F
        sta     $2D
        jsr     L8EA8
        lda     $28
        sta     $8C
        lda     $29
        sta     $8D
        rts
LA6BA:  jmp     L8494
LA6BD:  jsr     L908C
        pla
        sta     $19
        pla
        sta     $90
        pla
        sta     $8B
        pla
        sta     $8A
        jsr     LA70F
        bcc     LA6BA
        ldy     #$03
        lda     ($8E),y
        bne     LA6BA
        iny
        lda     ($8E),y
        cmp     $19
        bcc     LA6BA
        lda     $19
        sta     $28
        jsr     LA704
        sta     $2C
        jsr     LA7CE
        jsr     L8F47
        lda     #$05
        jmp     LA6A1
LA6F2:  lda     $0401,x
        bne     LA6BA
        stx     $30
        jsr     L90BF
        cmp     #$29
        bne     LA702
        clc
        rts
LA702:  sec
        rts
LA704:  lda     $90
        and     #$0F
LA708:  cmp     #$02
        beq     LA70E
        lda     #$21
LA70E:  rts
LA70F:  lda     $1D
        sta     $8E
        lda     $1E
        sta     $8F
LA717:  ldy     #$00
        lda     ($8E),y
        beq     LA753
        cmp     $90
        bne     LA731
        iny
        lda     ($8E),y
        cmp     $8A
        bne     LA731
        iny
        lda     ($8E),y
        cmp     $8B
        bne     LA731
        sec
        rts
LA731:  ldy     #$00
        lda     ($8E),y
        bmi     LA755
        jsr     LA708
        clc
        adc     #$03
        adc     $8E
        sta     $8E
        lda     #$00
        adc     $8F
        sta     $8F
LA747:  lda     $1F
        cmp     $8E
        bne     LA717
        lda     $20
        cmp     $8F
        bne     LA717
LA753:  clc
        rts
LA755:  and     #$0F
        sta     $73
        ldy     #$03
        lda     ($8E),y
        sta     $74
        iny
        lda     ($8E),y
        sta     $4A
        iny
        lda     ($8E),y
        sta     $4B
        jsr     LA7CE
        sta     $2C
        lda     $4A
        sta     $28
        lda     $74
        beq     LA77A
        lda     $4B
        sta     $2C
LA77A:  inc     $28
        bne     LA780
        inc     $29
LA780:  inc     $2C
        bne     LA786
        inc     $2D
LA786:  jsr     L8F47
        lda     $73
        jsr     LA708
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8F47
        lda     #$05
        sta     $2C
        lda     $74
        beq     LA7A1
        inc     $2C
LA7A1:  jsr     L8EA8
        lda     $8E
        sta     $2C
        lda     $8F
        sta     $2D
        jsr     L8EA8
        lda     $28
        sta     $8E
        lda     $29
        sta     $8F
        jmp     LA747
LA7BA:  sec
        rts
LA7BC:  cmp     #$30
        bcc     LA7BA
        cmp     #$3A
        bcc     LA7CC
LA7C4:  cmp     #$41
        bcc     LA7BA
        cmp     #$5B
        bcs     LA7BA
LA7CC:  clc
        rts
LA7CE:  lda     #$00
        sta     $2D
        sta     $29
        rts
LA7D5:  jsr     L8EAC
LA7D8:  ldx     $30
        dex
        dex
        dex
        lda     $28
        sta     $0400,x
        lda     $29
        sta     $0401,x
        lda     $2A
        sta     $0402,x
        jmp     L90BF
LA7EF:  jsr     L8E0F
        jmp     LA7D8
LA7F5:  jsr     L8F0F
        jmp     LA7D8
LA7FB:  lda     $2C
        bne     LA803
        lda     $2D
        beq     LA809
LA803:  jsr     L8F9D
        jmp     LA7D8
LA809:  lda     #$0A
        jmp     LB237
LA80E:  lda     $29
        pha
        jsr     LA7FB
        lda     $2A
        sta     $28
        lda     $2B
        sta     $29
        pla
        bpl     LA829
        lda     $29
        bmi     LA826
LA823:  jsr     L8E32
LA826:  jmp     LA7D8
LA829:  lda     $29
        bpl     LA826
        bmi     LA823
LA82F:  jsr     L8FEE
        bne     LA83D
LA834:  lda     #$00
LA836:  sta     $28
        sta     $29
        jmp     LA7D8
LA83D:  lda     #$FF
        jmp     LA836
LA842:  jsr     L8FEE
        beq     LA83D
        bcc     LA83D
        jmp     LA834
LA84C:  jsr     L8FEE
        bcs     LA83D
        jmp     LA834
LA854:  jsr     L8FEE
        beq     LA83D
        jmp     LA834
LA85C:  jsr     L8FEE
        beq     LA834
        bcs     LA83D
        jmp     LA834
LA866:  jsr     L8FEE
        bcc     LA83D
        jmp     LA834
LA86E:
        .addr   LA82F,LA84C,LA842,LA854,LA85C,LA866
LA87A:  ldx     #$28
        .byte   $2C
LA87D:  ldx     #$29
        stx     $75
        jsr     L90BF
        cmp     $75
        bne     LA8D1
        jmp     L908C
LA88B:  jmp     L8494
LA88E:  jsr     L908C
        jsr     LAA04
        ldy     $28
        cpy     #$08
        bcs     LA88B
        lda     $0645,y
        and     #$10
        beq     LA8A3
        lda     #$FF
LA8A3:  sta     $28
        sta     $29
        jsr     LA97E
        jmp     L90BF
LA8AD:  lda     #$00
        sta     $28
LA8B1:  sta     $29
        sta     $2A
        rts
LA8B6:  jsr     LA87A
        jsr     LA29B
        ldx     $30
        dex
        lda     $0400,x
        sta     $2A
        dex
        lda     $0400,x
        sta     $29
        dex
        lda     $0400,x
        sta     $28
        rts
LA8D1:  jmp     L8491
        ldx     $30
        lda     $28
        sta     $0400,x
        inx
        lda     $29
        sta     $0400,x
        inx
        inx
        stx     $30
        rts
LA8E6:  lda     $49
        sta     $28
LA8EA:  lda     #$00
        sta     $29
LA8EE:  inc     $30
        inc     $30
        inc     $30
LA8F4:  lda     #$02
        sta     $27
LA8F8:  ldx     $30
        dex
        dex
        dex
        lda     $28
        sta     $0400,x
        lda     $29
        sta     $0401,x
        lda     $2A
        sta     $0402,x
        rts
LA90D:  lda     $B5
        sta     $28
        jmp     LA8EA
LA914:  lda     $B7
        sta     $28
        lda     $B8
        sta     $29
        jmp     LA8EE
LA91F:  jsr     LA925
        jmp     LA8EE
LA925:  sec
        lda     $03
        sbc     $1F
        sta     $28
        lda     $04
        sbc     $20
        sta     $29
        rts
LA933:  jsr     LAA0A
        lda     $48
LA938:  sta     $28
        lda     #$00
        sta     $29
        jmp     LA97E
LA941:  jsr     LAA04
LA944:  lda     $28
        jsr     LAF44
        asl     a
        asl     a
        asl     a
        tax
        rts
LA94E:  jsr     LAA04
        lda     $28
        jsr     LAF44
        tax
        rts
LA958:  jsr     LA941
        lda     $06C6,x
        jmp     LA938
LA961:  jsr     LA941
        lda     $06C7,x
        jmp     LA938
LA96A:  jsr     LA941
        lda     $06C0,x
        jmp     LA938
LA973:  jsr     LAA0A
        ldy     #$00
        lda     ($28),y
        sta     $28
        sty     $29
LA97E:  jsr     LA87D
        jmp     LA8F4
LA984:  jsr     LAA0A
        jsr     L8DF2
        beq     LA999
        lda     $29
        bmi     LA993
        jmp     LA97E
LA993:  jsr     L8E32
        jmp     LA97E
LA999:  jmp     L8EF2
LA99C:  jsr     LAA0A
        lda     $29
        bmi     LA9AC
        bne     LA9B5
        lda     $28
        bne     LA9B5
        lda     #$00
        .byte   $2C
LA9AC:  lda     #$FF
        sta     $28
LA9B0:  sta     $29
        jmp     LA97E
LA9B5:  lda     #$01
        sta     $28
        lda     #$00
        beq     LA9B0
LA9BD:  jsr     LAA0A
        lda     $28
        pha
        lda     $29
        pha
        lda     $56
        sta     $28
        lda     $57
        sta     $29
        lda     #$83
        sta     $2C
        lda     #$03
        sta     $2D
        jsr     L8F44
        lda     #$7F
        and     $29
        sta     $29
        sta     $57
        lda     $28
        sta     $56
        pla
        sta     $2D
        pla
        sta     $2C
        jsr     L8F44
        asl     $29
        rol     $36
        rol     $37
        lda     $36
        sta     $28
        lda     $37
        sta     $29
        jmp     LA97E
LA9FF:  lda     #$0B
        jmp     LB237
LAA04:  jsr     LAA0A
        jmp     LAB80
LAA0A:  jsr     LA8B6
LAA0D:  lda     #$02
        .byte   $AE
LAA10:  lda     #$03
        cmp     $27
        bne     LA9FF
        rts
LAA17:  jsr     LA8B6
        jmp     LAA10
LAA1D:  jsr     LAA17
LAA20:  lda     #$00
        jsr     LA8B1
        jmp     LA97E
LAA28:  jsr     LAA17
        lda     $28
        beq     LAA20
        ldy     #$00
        lda     ($29),y
        sta     $28
        jmp     LAA20
LAA38:  jmp     L8494
LAA3B:  jsr     LB656
        jsr     LAA04
        lda     $28
        cmp     #$1C
        bcs     LAA38
        sta     $48
        jsr     LAC3B
        jsr     LAB80
        lda     $28
        cmp     #$18
        bcs     LAA38
        sta     $49
        jsr     L90BF
        cmp     #$29
        beq     LAA70
        jsr     LAC3B
        lda     $28
        beq     LAA70
        jsr     LB08D
        jsr     LC4B0
        lda     $9C
        jmp     LAA7A
LAA70:  jsr     LB4F4
        lda     #$05
        jsr     L896A
        lda     $73
LAA7A:  sta     $28
        jsr     LB65F
        jmp     LAA85
LAA82:  jsr     LAA04
LAA85:  lda     $28
LAA87:  jsr     LAA94
LAA8A:  lda     #$03
        sta     $27
        jsr     LA87D
        jmp     LA8F8
LAA94:  ldx     $21
        sta     $0700,x
        inc     $21
        lda     #$01
        sta     $28
        stx     $29
        lda     #$07
        sta     $2A
        rts
LAAA6:  jsr     LAA0A
        jsr     L9198
LAAAC:  jsr     L90D6
        lda     #$00
        sta     $90
        ldy     $21
        ldx     $46
LAAB7:  lda     $3D,x
        beq     LAAC5
        sta     $0700,y
        inx
        iny
        inc     $90
        jmp     LAAB7
LAAC5:  lda     $21
        sty     $21
        sta     $29
        lda     #$07
        sta     $2A
        lda     $90
        sta     $28
        jmp     LAA8A
LAAD6:  jsr     LAA0A
        jsr     L91DA
        jmp     LAAAC
LAADF:  jsr     LAA17
        lda     $28
        beq     LAB0D
        jsr     L9430
        lda     $28
        sta     $74
        ldx     #$00
        ldy     #$00
LAAF1:  lda     ($29),y
        sta     $0500,x
        inx
        iny
        dec     $74
        bne     LAAF1
        lda     #$00
        sta     $0500,x
        jsr     L8BA4
        jsr     LAB2D
        jsr     L9439
        jmp     LA97E
LAB0D:  jsr     LA8AD
        jmp     LA97E
LAB13:  jsr     L908C
        jsr     L90BF
        jsr     L90C8
        bcs     LAB24
        jsr     L90E4
        jmp     L8E32
LAB24:  jsr     LAB2D
        jmp     L8E32
LAB2A:  jsr     L908C
LAB2D:  ldy     #$00
        lda     ($09),y
        cmp     #$2D
        beq     LAB13
        cmp     #$2B
        beq     LAB2A
        cmp     #$26
        beq     LAB49
        jsr     L90E4
        jsr     L8DF2
        beq     LAB46
        rts
LAB46:  jmp     L8EF2
LAB49:  jsr     L908C
        lda     ($09),y
        cmp     #$48
        bne     LAB55
        jsr     L908C
LAB55:  jmp     L91ED
LAB58:  jsr     L849C
        beq     LAB6A
        cmp     #$28
        bne     LAB6A
        jsr     LA8B6
        jsr     LBC20
        jmp     LAA87
LAB6A:  jsr     LB945
        beq     LAB7A
        jsr     LAA94
LAB72:  jsr     LA8EE
        lda     #$03
        sta     $27
        rts
LAB7A:  jsr     LA8AD
        jmp     LAB72
LAB80:  lda     $29
        bne     LAB8A
        rts
LAB85:  cmp     #$20
        bcs     LAB8A
        rts
LAB8A:  jmp     L8494
LAB8D:  jsr     LAA17
        lda     $28
        pha
        lda     $29
        pha
        lda     $2A
        pha
        jsr     LAC3B
        jsr     LAB80
        lda     $28
        jsr     LAB85
        pla
        sta     $1A
        pla
        sta     $19
        pla
        rts
        jmp     L8491
LABAF:  jsr     LAA17
        ldx     $28
        ldy     #$00
LABB6:  lda     ($29),y
        sta     $0500,y
        iny
        dex
        bne     LABB6
        lda     $28
        pha
        jsr     LAC1E
        jsr     LAA10
        lda     $28
        sta     $2C
        lda     $29
        sta     $2D
        lda     $2A
        sta     $2E
        pla
        sta     $28
        lda     #$00
        sta     $29
        lda     #$05
        sta     $2A
        jsr     L95E6
        sta     $28
        lda     #$00
        sta     $29
        sta     $2A
        jmp     LA97E
LABED:  jsr     LAB8D
        sta     $4A
        lda     $28
        beq     LAC0C
        cmp     $4A
        bcs     LAC05
LABFA:  lda     $19
        sta     $29
        lda     $1A
LAC00:  sta     $2A
        jmp     LAA8A
LAC05:  lda     $4A
        sta     $28
        jmp     LABFA
LAC0C:  sta     $29
        jmp     LAC00
LAC11:  jmp     L8491
LAC14:  jsr     L90BF
        cmp     #$2C
        bne     LAC11
        jmp     L908C
LAC1E:  jsr     LAC14
        jsr     LA29B
        ldx     $30
        dex
        lda     $0400,x
        sta     $2A
        dex
        lda     $0400,x
        sta     $29
        dex
        lda     $0400,x
        sta     $28
        stx     $30
        rts
LAC3B:  jsr     LAC1E
        jmp     LAA0D
LAC41:  jsr     LAB8D
        sta     $4A
        lda     $28
        beq     LAC0C
        cmp     $4A
        bcs     LAC05
        sec
        lda     $4A
        sbc     $28
        clc
        adc     $19
        sta     $19
        lda     #$00
        adc     $1A
        sta     $1A
        lda     $28
        sta     $90
        ldx     $21
        ldy     #$00
LAC66:  lda     ($19),y
        sta     $0700,x
        inx
        iny
        dec     $90
        bne     LAC66
        lda     $21
        sta     $29
        stx     $21
        lda     #$07
        sta     $2A
        jmp     LAA8A
LAC7E:  jmp     L8494
LAC81:  jsr     LAA17
        lda     $28
        pha
        lda     $29
        pha
        lda     $2A
        pha
        jsr     LAC3B
        jsr     LAB80
        lda     $28
        beq     LAC7E
        jsr     LAB85
        sec
        sbc     #$01
        pha
        jsr     LAC3B
        jsr     LAB80
        lda     $28
        beq     LACF8
        jsr     LAB85
        sta     $1C
        pla
        sta     $1B
        pla
        sta     $1A
        pla
        sta     $19
        pla
        cmp     $1B
        beq     LACFC
        bcc     LACFC
        sec
        sbc     $1B
        cmp     $1C
        bcc     LACF3
LACC4:  lda     $1B
        clc
        adc     $19
        sta     $19
        lda     #$00
        adc     $1A
        sta     $1A
        lda     $1C
        sta     $28
        sta     $90
        ldx     $21
        ldy     #$00
LACDB:  lda     ($19),y
        sta     $0700,x
        inx
        iny
        dec     $90
        bne     LACDB
        lda     $21
        sta     $29
        stx     $21
        lda     #$07
        sta     $2A
        jmp     LAA8A
LACF3:  sta     $1C
        jmp     LACC4
LACF8:  pla
        pla
        pla
        pla
LACFC:  jsr     LA8AD
        jmp     LAA8A
LAD02:  jmp     L8494
LAD05:  jsr     LAA04
LAD08:  lda     $28
        beq     LAD13
        cmp     #$01
        bne     LAD02
        lda     #$17
        .byte   $AE
LAD13:  lda     #$16
        sta     $19
        lda     #$40
        sta     $1A
        lda     #$05
        sta     $4016
        nop
        nop
        lda     #$04
        sta     $4016
        nop
        nop
        ldx     #$08
        ldy     #$00
LAD2D:  lda     ($19),y
        lsr     a
        rol     $90
        nop
        nop
        dex
        bne     LAD2D
        rts
LAD38:  jsr     LAD05
        jsr     LAD08
        lda     $90
        and     #$0F
LAD42:  sta     $28
        jmp     LA97E
LAD47:  jsr     LAD05
        lda     $90
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jmp     LAD42
LAD53:  asl     a
        asl     a
        asl     a
        asl     a
        rts
LAD58:  jmp     L8494
LAD5B:  cmp     #$2C
        beq     LAD8A
        jsr     LA26E
        cmp     #$04
        bcs     LAD58
        sta     $F3
        jsr     LADFE
        jsr     L849C
        beq     LAD7F
        jsr     LAC14
        jsr     LA26E
        beq     LAD7F
        cmp     #$01
        bne     LAD58
        jsr     LADD1
LAD7F:  jsr     L8662
        jsr     LAD96
        lda     #$00
        jmp     L86B0
LAD8A:  jsr     LAC14
        jsr     LA26E
        beq     LAD95
        jsr     LADD1
LAD95:  rts
LAD96:  lda     $05
        sta     $19
        lda     $06
        sta     $1A
        lda     $F3
        asl     a
        tax
        lda     L8003,x
        sta     $07
        lda     L8004,x
        sta     $08
        lda     L800B,x
        sta     $1B
        lda     L800C,x
        sta     $1C
        jsr     game_copy
        jmp     L97A6
LADD1:  lda     #<LD000
        sta     $19
        lda     #>LD000
        sta     $1A
        jsr     LB559
        jsr     bg_write_page
        jsr     LB53C
        jmp     LB562
LADFE:  lda     #$01
        sta     $28
        jmp     LAE13
LAE05:  pla
        pla
LAE07:  jmp     LB4AB
LAE0A:  beq     LAE07
        jsr     LA26E
        cmp     #$02
        bcs     LAE45
LAE13:  lda     $49
        pha
        lda     $4F
        pha
        lda     $28
        beq     LAE2B
        lda     #$24
        cmp     $4F
        beq     LAE05
        sta     $4F
        lda     #$00
        sta     $E5
        beq     LAE37
LAE2B:  lda     #$20
        cmp     $4F
        beq     LAE05
        sta     $4F
        lda     #$00
        sta     $E4
LAE37:  jsr     LB4AB
        pla
        sta     $4F
        pla
        sta     $49
        rts
LAE41:  jsr     LB53C
        rts
LAE45:  jmp     L8494
LAE48:  jsr     LA26E
        beq     LAE51
        cmp     #$01
        bne     LAE45
LAE51:  and     #$FF
        sta     $F0
        beq     LAE88
        lda     #$24
        sta     $4F
        lda     $32
        ora     #$01
LAE5F:  sta     $32
        jsr     LB42B
        jsr     LBE14
        jsr     LB53C
        jsr     LB748
        jsr     L849C
        beq     LAE41
        jsr     LAC14
        jsr     LA26E
        beq     LAE83
        cmp     #$01
        bne     LAE45
        lda     #$24
        sta     $4F
        rts
LAE83:  lda     #$20
        sta     $4F
        rts
LAE88:  lda     #$20
        sta     $4F
        lda     $32
        and     #$FE
        jmp     LAE5F
LAE93:  jmp     L8494
LAE96:  lda     #$00
        sta     $F0
        lda     #$20
        sta     $4F
        lda     $32
        and     #$FE
        sta     $32
        jmp     LBE14
        lda     #$01
        sta     $F0
        lda     #$24
        sta     $4F
        lda     $32
        ora     #$01
        sta     $32
        jmp     LBE14
LAEB8:  jsr     LA26E
        cmp     #$08
        bcs     LAE93
        jsr     LAD53
        asl     a
        sta     $75
        lda     $33
        and     #$1F
        ora     $75
        sta     $33
        sta     $2001
        rts
LAED1:  jsr     LA26E
        cmp     #$04
        bcs     LAE93
        asl     a
        asl     a
        asl     a
        sta     $74
        lda     $32
        and     #$E7
        ora     $74
        sta     $32
        jmp     LBE14
LAEE8:  ldx     #$00
        lda     #$EF
LAEEC:  sta     $0200,x
        inx
        bne     LAEEC
        rts
LAEF3:  ldx     #$00
        stx     $65
        cmp     #$42
        beq     LAF03
        cmp     #$53
        bne     LAF4C
        lda     #$10
        sta     $65
LAF03:  jsr     LA265
        cmp     #$04
        bcs     LAF49
        pha
        lda     #$04
        sta     $02
        lda     #$00
        sta     $7C
LAF13:  jsr     LA26B
        ldy     $7C
        sta     a:$003D,y
        inc     $7C
        dec     $02
        bne     LAF13
        pla
        sta     $0700
        lda     $3D
        sta     $0701
        lda     $3E
        sta     $0702
        lda     $3F
        sta     $0703
        lda     $40
        sta     $0704
        ldy     #$06
        ldx     $65
        beq     LAF40
        iny
LAF40:  tya
        jmp     L896A
LAF44:  cmp     #$08
        bcs     LAF49
        rts
LAF49:  jmp     L8494
LAF4C:  jmp     L8491
LAF4F:  cmp     #$2C
        beq     LAF78
        jsr     LA26E
        beq     LAF5C
        cmp     #$01
        bne     LAF49
LAF5C:  asl     a
        tax
        lda     LCFA4,x
        sta     $52
        lda     LCFA5,x
        sta     $53
        lda     #$00
        sta     $65
        jsr     LAF93
        jsr     L849C
        beq     LAF92
        cmp     #$2C
        bne     LAF4C
LAF78:  jsr     LA265
        cmp     #$03
        bcs     LAF49
        asl     a
        tax
        lda     LCFA8,x
        sta     $52
        lda     LCFA9,x
        sta     $53
        lda     #$01
        sta     $65
        jsr     LAF93
LAF92:  rts
LAF93:  lda     #$00
        sta     $0700
        sta     $75
LAF9A:  lda     #$04
        ldx     #$01
        sta     $74
        ldy     $75
LAFA2:  lda     ($52),y
        sta     $0700,x
        inx
        iny
        dec     $74
        bne     LAFA2
        sty     $75
        lda     #$06
        sta     $73
        lda     $65
        beq     LAFB9
        inc     $73
LAFB9:  lda     $73
        jsr     L896A
        inc     $0700
        lda     $0700
        cmp     #$04
        bne     LAF9A
        rts
LAFC9:  lda     $33
        ora     #$10
LAFCD:  sta     $33
        sta     $2001
        jmp     L908C
LAFD5:  lda     $33
        and     #$EF
        jmp     LAFCD
LAFDC:  pla
        jsr     LAD53
        tax
        ldy     #$04
LAFE3:  lda     #$EF
        sta     $0200,x
        sta     $0203,x
        sta     $0201,x
        inx
        inx
        inx
        inx
        dey
        bne     LAFE3
        rts
        jmp     L8494
        jmp     L8491
LAFFC:  cmp     #$9A
        beq     LAFC9
        cmp     #$9B
        beq     LAFD5
        jsr     LA26E
        jsr     LAF44
        pha
        jsr     L849C
        beq     LAFDC
        jsr     LA26B
        pha
        jsr     LA26B
        sta     $1B
        pla
        sta     $19
        pla
        sta     $6D
        tax
        lda     $063D,x
        sta     $90
        lda     $6D
        asl     a
        asl     a
        tax
        asl     a
        asl     a
        pha
        tay
        lda     #$04
        sta     $74
LB032:  lda     $0680,x
        sta     $0201,y
        inx
        iny
        iny
        iny
        iny
        dec     $74
        bne     LB032
        pla
        tay
        lda     $1B
        sta     $0200,y
        lda     $19
        sta     $0203,y
        lda     $90
        bne     LB052
        rts
LB052:  lda     #$00
        sta     $1C
        lda     #$08
        sta     $1A
        jsr     LB06E
        lda     #$00
        sta     $1A
        lda     #$08
        sta     $1C
        jsr     LB06E
        lda     #$08
        sta     $1A
        sta     $1C
LB06E:  iny
        iny
        iny
        iny
        lda     $19
        clc
        adc     $1A
        sta     $0203,y
        lda     $1B
        clc
        adc     $1C
        sta     $0200,y
LB082:  rts
LB083:  cmp     #$A1
        bne     LB08A
        jmp     LCA5B
LB08A:  jmp     L8491
LB08D:  cmp     #$01
        beq     LB082
        and     #$FF
        beq     LB082
LB095:  jmp     L8494
LB098:  cmp     #$8A
        bne     LB083
        jsr     LA265
        jsr     LAF44
        pha
        jsr     LAC14
        jsr     LA87A
        lda     #$04
        sta     $6D
        jsr     LA26E
        cmp     #$04
        bcs     LB095
        pha
LB0B5:  jsr     LA26B
        pha
        dec     $6D
        bne     LB0B5
        jsr     LA87D
        jsr     L86D9
        pla
        jsr     LB08D
        ror     a
        ror     a
        and     #$80
        sta     $19
        pla
        jsr     LB08D
        ror     a
        ror     a
        ror     a
        and     #$40
        ora     $19
        sta     $19
        pla
        jsr     LB08D
        ror     a
        ror     a
        ror     a
        ror     a
        and     #$20
        ora     $19
        sta     $19
        pla
        jsr     LB08D
        sta     $AD
        pla
        and     #$03
        ora     $19
        sta     $19
        pla
        sta     $6D
        jsr     LAD53
        ldy     #$04
        tax
LB0FE:  lda     #$EF
        sta     $0200,x
        sta     $0203,x
        lda     $19
        sta     $0202,x
        lda     #$20
        sta     $0201,x
        inx
        inx
        inx
        inx
        dey
        bne     LB0FE
        jsr     LA27D
        lda     $0401
        sta     $1B
        lda     $0402
        sta     $1C
        ldy     #$00
        lda     $6D
        asl     a
        asl     a
        tax
        lda     $0400
        beq     LB140
LB130:  lda     ($1B),y
        sta     $0680,x
        inx
        iny
        cpy     #$04
        beq     LB14B
        dec     $0400
        bne     LB130
LB140:  lda     #$20
LB142:  sta     $0680,x
        inx
        iny
        cpy     #$04
        bne     LB142
LB14B:  ldx     $6D
        lda     $AD
        sta     $063D,x
LB152:  rts
LB153:  lda     $F0
        bne     LB152
        lda     #$20
        sta     $74
        lda     #$0C
        jsr     L896A
        ldx     #$00
LB162:  lda     $74
        clc
        adc     #$04
        jsr     L81EC
        lda     $2007
        lda     $2007
        pha
        lda     $2007
        sta     V_TMP1
        lda     $74
        jsr     L81EC
        pla
        sta     $2007
        lda     V_TMP1
        sta     $2007
        inx
        bne     LB162
        inc     $74
        lda     $74
        cmp     #$24
        bne     LB18A
        lda     #$28
        sta     $74
        bne     LB162
LB18A:  cmp     #$2C
        bne     LB162
        lda     $E5
        sta     $E4
        jsr     LB53C
        jsr     LBE14
        lda     #$0B
        jmp     L896A
LB194:  ldx     #$3D
        lda     #$00
LB198:  sta     $0600,x
        inx
        bne     LB198
        ldx     #$00
        lda     #$EF
LB1A2:  sta     $0680,x
        inx
        cpx     #$20
        bne     LB1A2
        ldx     #$C0
        lda     #$00
LB1AE:  sta     $0600,x
        inx
        bne     LB1AE
        rts
LB1B5:  jmp     L8BC8
LB1B8:  jsr     L97A6
        lda     $20
        cmp     #$6C
        bcs     LB1B5
        lda     #$0C
        jsr     L896A
        lda     #$00
        sta     $19
        lda     #$6C
        sta     $1A
        jsr     bg_read_page
        lda     $E5
        sta     $6005
        lda     #$FF
LB1F3:  sta     $6004
        lda     #$0B
        jsr     L896A
        rts
LB1FC:  lda     #$14
        jmp     LB237
LB201:  lda     $6004
        beq     LB1FC
LB206:  lda     #$0C
        jsr     L896A
        lda     #$6C
        sta     $1A
        lda     #$00
        sta     $19
        jsr     bg_write_page
        lda     $6005
        sta     $E5
        lda     #$00
        beq     LB1F3
LB237:  cmp     #$16
        bcc     LB23D
        lda     #$15
LB23D:  sta     $B5
        ldx     $0B
        stx     $B7
        stx     $FA
        ldx     $0C
        stx     $B8
        stx     $FB
        ldx     $E7
        beq     LB279
        cpx     #$02
        beq     LB279
        lda     $58
        sta     $F6
        lda     $59
        sta     $F7
        lda     $0D
        sta     $F8
        lda     $0E
        sta     $F9
        lda     $E8
        sta     $44
        lda     $E9
        sta     $45
        jsr     L8540
        inc     $E7
        ldx     $17
        txs
        jsr     L96C9
        jmp     L834A
LB279:  pha
        jsr     LB56B
        pla
        ldx     #$00
        txs
        asl     a
        tay
        lda     #$3F
        jsr     L894D
        lda     LB37F,y
        jsr     L894D
        lda     LB380,y
        jsr     L894D
        lda     #<LB360
        sta     $52
        lda     #>LB360
        sta     $53
        jsr     L893C
        jsr     L88C0
LB2A2:  lda     #$FF
        cmp     $0C
        bne     LB2AC
        cmp     $0B
        beq     LB2BA
LB2AC:  lda     #<LB355
        sta     $52
        lda     #>LB355
        sta     $53
        jsr     L893C
        jsr     LB2C3
LB2BA:  jsr     L895D
        jsr     LBCD0
        jmp     L8284
LB2C3:  lda     $0B
        sta     $11
        sta     $28
        lda     $0C
        sta     $12
        sta     $29
LB2CF:  jsr     L913E
        jsr     L90D6
        jmp     L87E0
LB2D8:  jsr     LB56B
        lda     #$00
        sta     $62
        lda     #<LB35A
        sta     $52
        lda     #>LB35A
        sta     $53
        jsr     L893C
        jmp     LB2A2
LB2ED:  lda     #$4F
        jsr     LB575
        lda     #$4B
        jmp     LB575
LB2F7:  lda     #<LCF7B
        sta     $52
        lda     #>LCF7B
        sta     $53
        jsr     L895A
        jsr     LB56B
        lda     #<LCF8C
        sta     $52
        lda     #>LCF8C
        sta     $53
        jsr     L895A
        jsr     LB56B
LB313:  jsr     LA925
        jsr     LB2CF
        jsr     L895D
        lda     #<LB349
        sta     $52
        lda     #>LB349
        sta     $53
        jmp     L895A
LB327:  lda     #<LB338
        sta     $52
        lda     #>LB338
        sta     $53
        jsr     L895A
        jsr     LB56B
        jmp     LB313
LB338:
        .byte   "BASIC HOT START ", $00
LB349:
        .byte   " BYTES FREE", $00
LB355:
        .byte   " IN ", $00
LB35A:
        .byte   "BREAK", $00
LB360:
        .byte   " ERROR", $00
LB367:
        .byte   "WRITING ", $00
LB370:
        .byte   "LOADING ", $00
LB37F:
        .byte   "N"
LB380:
        .byte   "FSNRGODILOVOMULSODDDZTMSTFTCCUFMOTPNRREN", "BUP"
LB3AB:  lda     #$10
        sta     $32
        lda     #$0E
        sta     $33
        jsr     LB559
        lda     #$00
        sta     $F5
        sta     $F0
        sta     $E4
        sta     $E5
        sta     $63
        sta     $AB
        sta     $AC
        sta     $62
        sta     $72
        sta     $50
        sta     $18
        sta     $4010
        sta     $4001
        sta     $4005
        sta     $4015
        sta     $4C
        sta     $66
        sta     $67
        lda     #$4F
        sta     $4017
        jsr     LB420
        jsr     LBE14
        lda     #$20
        sta     $4F
        jsr     LB4AB
        jsr     LB436
        ldx     #$00
        txa
        ldy     #$80
LB3FA:  sta     $0480,x
        inx
        dey
        bne     LB3FA
        ldx     #$00
LB403:  lda     $67
        cmp     #$08
        beq     LB435
        jsr     LAD53
        tay
LB40D:  lda     LB44A,x
        beq     LB41A
        sta     $0480,y
        inx
        iny
        jmp     LB40D
LB41A:  inx
        inc     $67
        jmp     LB403
LB420:  lda     #$FD
        sta     $E0
        lda     #$5F
        sta     $E1
        jmp     LB766
LB42B:  lda     $2002
        bmi     LB42B
LB430:  lda     $2002
        bpl     LB430
LB435:  rts
LB436:  lda     #<LCFBE
        sta     $52
        lda     #>LCFBE
        sta     $53
        lda     #$00
        sta     $65
        jsr     LAF93
        inc     $65
        jmp     LAF93
LB44A:
        .byte   "GAME 0", $0D, $00
        .byte   "GAME 1", $0D, $00
        .byte   "GAME 2,1", $0D, $00
        .byte   "GAME 3", $0D, $00
        .byte   "SPRITE ", $00
        .byte   "LOAD", $0D, $00
        .byte   "LIST", $0D, $00
        .byte   "RUN", $0D, $00
; 清屏：每个图块写成空格、调色板组 0（原版调色板组在属性表里，清屏时同时清属性表）
LB485:  ldy     #$04
        ldx     #$00
LB48B:  lda     #$20
        sta     $2007
        lda     V_TILE_HIGH
        sta     $2007
        dex
        bne     LB48B
        dey
        bne     LB48B
        rts
; 把当前名称表的所有图块调色板组清成 0（只写高字节），返回时 A = 0
LB494:  lda     $4F
        jsr     clear_groups_page
        lda     $4F
        clc
        adc     #$08
        jsr     clear_groups_page
        lda     #$00
        rts
LB4A7:  lda     #$24
        sta     $4F
LB4AB:  lda     #$0C
        jsr     L896A
        jsr     LBE0E
        jsr     LB559
        lda     $4F
        ldx     #$00
        jsr     L81EC
        jsr     LB485
        lda     $4F
        clc
        adc     #$08
        ldx     #$00
        jsr     L81EC
        jsr     LB485
        jsr     LB53C
        jsr     LBE14
        lda     #$0B
        jsr     L896A
        jsr     LB748
        ldy     $4F
        cpy     #$20
        beq     LB4D9
        sta     $E5
        jmp     LB4DB
LB4D9:  sta     $E4
LB4DB:  tax
        ldy     $4F
        cpy     #$20
        beq     LB4EB
LB4E2:  sta     $061A,x
        inx
        cpx     #$1A
        bne     LB4E2
        rts
LB4EB:  sta     $0600,x
        inx
        cpx     #$1A
        bne     LB4EB
        rts
LB4F4:  lda     #$20
        sta     $28
        lda     #$00
        sta     $29
        sta     $2D
        lda     $E4
        ldy     $4F
        cpy     #$20
        beq     LB508
        lda     $E5
LB508:  lsr     a
        lsr     a
        clc
        adc     $49
        adc     #$03
        and     #$3F
LB516:  sta     $2C
        jsr     L8F47
        clc
        lda     $48
        adc     #$02
        adc     $28
        sta     $4A
        lda     #$00
        adc     $29
        cmp     #$04
        bcc     LB521
        adc     #$03
LB521:  clc
        adc     $4F
        sta     $4B
        rts
LB52E:  lda     $4A
        asl     a
        pha
        lda     $4B
        and     #$0F
        rol     a
        sta     $2006
        pla
        sta     $2006
        rts
LB539:  put_tile
LB53C:  jsr     LBE14
        lda     $F0
        beq     LB54E
        lda     $E5
        jmp     LB551
LB54E:  lda     $E4
LB551:  asl     a
        tay
        lda     #$00
        sta     $2005
        sty     $2005
        rts
LB559:  jsr     LB42B
        lda     #$00
        sta     $2001
        rts
LB562:  jsr     LB42B
        lda     $33
        sta     $2001
LB56A:  rts
LB56B:  lda     $48
        beq     LB56A
LB56F:  lda     #$00
        sta     $48
        lda     #$0A
LB575:  cmp     #$20
        bcs     LB59C
        cmp     #$08
        beq     LB58D
        cmp     #$01
        beq     LB58D
        cmp     #$12
        beq     LB58D
        cmp     #$09
        beq     LB58D
        ldy     #$00
        sty     $4C
LB58D:  asl     a
        tax
        lda     LB8BE,x
        sta     $28
        lda     LB8BF,x
        sta     $29
        jmp     ($0028)
LB59C:  ldy     $4C
        beq     LB5A7
        pha
        lda     #$12
        jsr     LB575
        pla
LB5A7:  jsr     L894D
        jmp     L895D
LB5AD:  inc     $49
        lda     $49
        cmp     #$18
        beq     LB5C7
LB5B5:  lda     #$00
        rts
LB5B8:  jsr     LB74C
LB5BB:  ldx     $49
        jsr     LB637
        beq     LB5AD
        inc     $49
        jmp     LB5BB
LB5C7:  dec     $7E
        jsr     LB748
        jsr     LB4F4
        lda     #$04
        jsr     L896A
LB5D4:  lda     $32
        jsr     video_write_ctrl
        lda     $E4
        ldy     $4F
        cpy     #$20
        beq     LB5E3
        lda     $E5
LB5E3:  clc
        adc     #$04
LB5EC:  ldy     $4F
        cpy     #$20
        beq     LB5F7
        sta     $E5
        jmp     LB5F9
LB5F7:  sta     $E4
LB5F9:  jsr     LB42B
        jsr     LB53C
        ldy     #$18
        ldx     #$00
        lda     $4F
        cmp     #$20
        beq     LB61C
LB609:  lda     $061C,x
        sta     $061B,x
        inx
        dey
        bne     LB609
        tya
        sty     $0632
        sty     $0633
        beq     LB62E
LB61C:  lda     $0602,x
        sta     $0601,x
        inx
        dey
        bne     LB61C
        lda     #$00
        sta     $0618
        sta     $0619
LB62E:  sta     $48
        lda     #$17
        sta     $49
        jmp     LBE14
LB637:  lda     $4F
        cmp     #$20
        beq     LB641
        lda     $061B,x
        rts
LB641:  lda     $0601,x
        rts
LB645:  pha
        lda     $4F
        cmp     #$20
        beq     LB651
        pla
        sta     $061B,x
        rts
LB651:  pla
        sta     $0601,x
        rts
LB656:  lda     $48
        sta     $6B
        lda     $49
        sta     $6C
        rts
LB65F:  lda     $6B
        sta     $48
        lda     $6C
        sta     $49
        rts
LB668:  jsr     LB559
        lda     #$20
        sta     $4F
        lda     #$00
        sta     $F0
        lda     #$10
        sta     $32
        jsr     LBE14
        lda     #$0E
        sta     $2001
        sta     $33
        jsr     LB436
        jsr     LB494
        sta     $4C
        jsr     LB53C
        jmp     LB562
LB68F:  jsr     LB656
        jsr     LB4F4
        lda     #$04
LB697:  jsr     L896A
        ldx     $49
        jsr     LB637
        beq     LB6AE
        inc     $49
        jsr     LB74C
        jsr     LB4F4
        lda     #$04
        jmp     LB697
LB6AE:  jmp     LB65F
LB6B1:  jsr     LB656
        jsr     LB4F4
LB6B7:  lda     #$04
        jsr     L896A
        ldx     $49
        lda     #$00
        jsr     LB645
        sta     $48
        inc     $49
        inx
        cpx     #$18
        beq     LB6AE
        jsr     LB4F4
        jmp     LB6B7
        ldx     $49
        dex
        txa
        tax
        inx
        jsr     LB637
        rts
        jmp     LB56F
LB6DF:  lda     $48
        cmp     #$15
        bcs     LB6F0
        cmp     #$0E
        bcs     LB6F3
        cmp     #$07
        bcs     LB6F6
        lda     #$07
        .byte   $EC
LB6F0:  lda     #$1C
        .byte   $EC
LB6F3:  lda     #$15
        .byte   $EC
LB6F6:  lda     #$0E
        sec
        sbc     $48
        sta     $62
        tax
        lda     #$20
LB700:  sta     $0700,x
        dex
        bpl     LB700
        jmp     L895D
LB709:  lda     #$FF
        eor     $4C
        sta     $4C
        rts
LB710:  lda     $49
        beq     LB716
        dec     $49
LB716:  rts
LB717:  lda     $48
        pha
        jsr     LB751
        pla
        sta     $48
        rts
LB721:  lda     $48
        beq     LB728
        dec     $48
        rts
LB728:  lda     $49
        beq     LB732
        lda     #$1B
        sta     $48
        dec     $49
LB732:  rts
LB733:  lda     $48
        cmp     #$1B
        beq     LB73C
        inc     $48
        rts
LB73C:  jsr     LB74C
        lda     $49
        cmp     #$17
        beq     LB747
        inc     $49
LB747:  rts
LB748:  lda     #$00
        sta     $49
LB74C:  lda     #$00
        sta     $48
        rts
LB751:  ldx     $49
        inx
        cpx     #$18
        bne     LB75B
        jmp     LB5C7
LB75B:  stx     $49
        rts
LB75E:  lda     #$FF
        sta     $72
        lda     $E1
        bne     LB76C
LB766:  lda     #$00
        sta     $72
        lda     $E0
LB76C:  sta     $61
        rts
LB76F:  ldx     $48
        beq     LB778
        dec     $48
        jmp     LB788
LB778:  ldx     $49
        beq     LB788
        dex
        jsr     LB637
        beq     LB788
        dec     $49
        lda     #$1B
        sta     $48
LB788:  jsr     LB656
LB78B:  lda     $48
        cmp     #$1B
        beq     LB7AC
        jsr     LB4F4
        lda     $4A
        sta     $19
        lda     $4B
        sta     $1A
        inc     $48
        jsr     LB4F4
        dec     $48
        lda     #$03
        jsr     L896A
        lda     #$1B
        sta     $48
LB7AC:  ldx     $49
        jsr     LB637
        bne     LB7C4
        jsr     LB4F4
        jsr     LB42B
        jsr     LB52E
        lda     #$20
        jsr     LB539
        jmp     LB65F
LB7C4:  inc     $49
        lda     #$FF
        sta     $48
        jsr     LB7D5
        inc     $49
        jsr     LB74C
        jmp     LB78B
LB7D5:  inc     $48
        jsr     LB4F4
        jsr     LB42B
        jsr     LB52E
        lda     $2007
        lda     $2007
        pha
        jsr     LB53C
        dec     $48
        lda     $48
        cmp     #$FF
        bne     LB7F8
        lda     #$1B
        sta     $48
        dec     $49
LB7F8:  jsr     LB4F4
        jsr     LB42B
        jsr     LB52E
        pla
        jmp     LB539
LB805:  jsr     LB656
        lda     #$20
        sta     $B0
LB80C:  jsr     LB4F4
        lda     #$0D
        jsr     L896A
        ldx     $49
        jsr     LB637
        beq     LB826
        inc     $49
        jsr     LB74C
        jmp     LB80C
LB823:  jmp     LB65F
LB826:  lda     $B0
        cmp     #$20
        beq     LB823
        lda     $49
        cmp     #$17
        beq     LB893
        jsr     LB74C
        ldy     $49
        iny
        cpy     #$17
        beq     LB862
        lda     $49
        sta     $91
        lda     #$16
        sta     $49
LB844:  jsr     LB4F4
        lda     $49
        pha
        lda     #$0E
        jsr     L896A
        pla
        sta     $49
        tax
        jsr     LB637
        inx
        jsr     LB645
        dec     $49
        lda     $49
        cmp     $91
        bne     LB844
LB862:  ldx     $49
        lda     #$FF
        jsr     LB645
        inc     $49
        inx
        lda     #$00
        jsr     LB645
        ldx     #$17
        jsr     LB645
        lda     $6B
        pha
        lda     $6C
        pha
        lda     $4C
        pha
        lda     #$05
        jsr     LB575
        pla
        sta     $4C
        lda     #$0D
        jsr     L896A
        pla
        sta     $49
        pla
        sta     $48
        rts
LB893:  lda     $6B
        pha
        ldx     $6C
        beq     LB89B
        dex
LB89B:  txa
        pha
        jsr     LB5C7
        lda     #$17
        sta     $49
        lda     #$00
        sta     $48
        jsr     LB4F4
        lda     #$0D
        jsr     L896A
        ldx     #$16
        lda     #$FF
        jsr     LB645
        pla
        sta     $49
        pla
        sta     $48
        rts
LB8BE:
        .addr   LB5B5,LB709,LB5B5,LB5B8,LB668,LB68F,LB5B5,LBCD0
        .addr   LB76F,LB6DF,LB751,LB748,LB4AB,LB74C,LB5B5,LB5B5
        .addr   LB5B5,LB5B5,LB805,LB5B5,LB5B5,LB5B5,LB75E,LB766
        .addr   LB5B5,LB5B5,LB6B1,LB5B5,LB733,LB721,LB710,LB717
LB8FE:  lda     #$00
        rts
LB901:  sta     $6D
        jsr     LBA99
        bcs     LB90D
        lda     $6D
        and     #$FF
        rts
LB90D:  lda     $6D
        ldx     $68
        bne     LB8FE
        sta     $6A
        lda     #$04
        sta     $74
LB919:  jsr     LB958
        cmp     $6A
        bne     LB945
        dec     $74
        bne     LB919
        sec
        sbc     #$F0
        jsr     LAD53
        sta     $67
        lda     #$FF
        sta     $66
LB930:  ldx     $67
        lda     $0480,x
        beq     LB93D
        inx
        stx     $67
        and     #$FF
        rts
LB93D:  lda     #$00
        sta     $66
        ldx     #$FF
        stx     $68
LB945:  lda     $66
        bne     LB930
        jsr     LB958
        beq     LB955
        cmp     #$F0
        bcs     LB901
        and     #$FF
        rts
LB955:  sta     $68
        rts
; 空程序行：长度字节是 0，开机时“下一行”指针（$0D、$0E）指向它（原版里是键码表中间的一个 $00）
LBB89:  .byte   $00
        .include        "keyboard.inc"
LBC11:  jsr     LB4F4
        lda     #$08
        bne     LBC1D
LBC18:  jsr     LB4F4
        lda     #$09
LBC1D:  jmp     L896A
LBC20:  lda     #$00
        sta     $5E
        jsr     LB945
        beq     LBC6F
        ldx     $66
        bne     LBC52
        cmp     $75
        bne     LBC6F
        lda     $7B
        bne     LBC55
        jsr     LBC11
LBC38:  jsr     LB945
        beq     LBC6C
        cmp     $75
        bne     LBC6C
        dec     $77
        bne     LBC38
        dec     $78
        bne     LBC38
        lda     #$FF
        sta     $7B
        jmp     LBCC3
LBC50:  lda     #$03
LBC52:  sta     $75
        rts
LBC55:  jsr     LB945
        beq     LBC6F
        cmp     $75
        bne     LBC6F
        dec     $79
        bne     LBC55
        lda     #$20
        sta     $79
        jsr     LBC11
        jmp     LBCC3
LBC6C:  jsr     LBC18
LBC6F:  lda     #$00
        sta     $7B
        sta     $77
        lda     #$20
        sta     $79
        lda     #$01
        sta     $78
        jsr     LBC11
        lda     #$80
        sta     $74
LBC84:  jsr     LB945
        bne     LBCAE
        dec     $74
        bne     LBC84
        jsr     LBC18
        lda     $5E
        bne     LBC50
        lda     #$80
        sta     $74
LBC98:  jsr     LB945
        bne     LBCAE
        dec     $74
        bne     LBC98
        lda     $5E
        bne     LBC50
        jmp     LBC6F
LBCA8:  jsr     LBC18
        jmp     LBC6F
LBCAE:  sta     $75
        lda     $66
        bne     LBCC3
        lda     #$01
        sta     $74
LBCB8:  jsr     LB945
        cmp     $75
        bne     LBCA8
        dec     $74
        bne     LBCB8
LBCC3:  jsr     LBC18
        lda     $E3
        beq     LBCCD
        jsr     LBCD5
LBCCD:  lda     $75
        rts
LBCD0:  lda     #$F0
        sta     $19
        .byte   $2C
LBCD5:  lda     #$01
        sta     $19
        lda     #$01
        sta     $4015
        lda     #$C0
        sta     $4017
        lda     #$EF
        sta     $4000
        lda     #$00
        sta     $4001
        lda     #$5F
        sta     $4002
        ldy     #$00
        sty     $4003
LBCF7:  dey
        bne     LBCF7
        dec     $19
        bne     LBCF7
        lda     #$00
        sta     $4015
        rts
LBD04:  lda     #$00
        sta     $76
        sta     $63
LBD0A:  jsr     LBC20
        cmp     #$0D
        beq     LBD2F
        cmp     #$03
        beq     LBD1F
        cmp     #$04
        beq     LBD1F
        jsr     LB575
        jmp     LBD0A
LBD1F:  jsr     LB575
        ldx     $76
        lda     #$00
        sta     $0501,x
        lda     #$03
        sta     $0500,x
        rts
LBD2F:  lda     $49
        beq     LBD3F
        tax
        dex
        jsr     LB637
        beq     LBD3F
        dec     $49
        jmp     LBD2F
LBD3F:  lda     $49
        sta     $47
        lda     #$02
        jsr     L896A
        lda     $64
        bne     LBD56
        ldx     $76
        dex
        lda     #$00
        sta     $0500,x
        stx     $76
LBD56:  ldx     $76
LBD58:  dex
        txa
        beq     LBD63
        lda     $0500,x
        cmp     #$20
        beq     LBD58
LBD63:  inx
        lda     #$00
        sta     $0500,x
        jmp     LB5B8
LBD6C:  ldx     $49
        beq     LBD76
        dex
        lda     #$00
        jsr     LB645
LBD76:  lda     #$00
        sta     $76
        sta     $63
LBD7C:  jsr     LBC20
        cmp     #$0D
        beq     LBD91
        cmp     #$03
        beq     LBD1F
        cmp     #$04
        beq     LBD1F
        jsr     LB575
        jmp     LBD7C
LBD91:  lda     $49
        beq     LBDA1
        tax
        dex
        jsr     LB637
        beq     LBDA1
        dec     $49
        jmp     LBD91
LBDA1:  lda     $49
        sta     $47
        lda     #$02
        jsr     L896A
        lda     $64
        bne     LBDB8
        ldx     $76
        dex
        lda     #$00
        sta     $0500,x
        stx     $76
LBDB8:  ldx     $76
LBDBA:  dex
        txa
        beq     LBDC5
        lda     $0500,x
        cmp     #$20
        beq     LBDBA
LBDC5:  inx
        lda     #$00
        sta     $0500,x
        jmp     LB5B8
LBDCE:  inc     $AD
        bne     LBDD4
        inc     $AE
LBDD4:  rts
LBDD5:  inc     $91
        bne     LBDD4
        inc     $92
        rts
LBDDC:  lda     $AD
        cmp     $AF
        bne     LBDD4
        lda     $AE
        cmp     $B0
        rts
LBDE7:  lda     $0514
        sta     $AD
        clc
        adc     $0512
        sta     $AF
        lda     $0515
        sta     $AE
        adc     $0513
        sta     $B0
        rts
LBDFD:  lda     #$00
        sta     $AD
        lda     #$05
        sta     $AE
        lda     #$80
        sta     $AF
        lda     #$05
        sta     $B0
        rts
LBE0E:  lda     $32
        jmp     video_write_ctrl
LBE14:  lda     $32
        ora     #$80
        jmp     video_write_ctrl
        .include        "disk.inc"
LBFCE:  jsr     LB4A7
LBFD1:  jsr     LB436
        ldy     #$08
        ldx     #$00
        txa
LBFD9:  sta     $0645,x
        inx
        dey
        bne     LBFD9
        lda     #$24
        sta     $4F
        lda     #$00
        sta     $48
        lda     #$15
        sta     $49
        lda     #$1A
        jsr     LB575
        lda     #$01
        sta     $F0
        jsr     LAEE8
        jsr     LB42B
        lda     $32
        ora     #$09
        sta     $32
        jsr     LB53C
        jsr     LBE14
        jsr     LAFC9
        lda     #$00
        sta     $0505
        sta     $0506
        sta     $88
        sta     $0504
        jmp     LC2E8
LC01A:  lda     $0505
        sta     $48
        lda     $0506
        sta     $49
        rts
LC025:  sta     $E0
        sta     $E1
        jsr     LB766
        jmp     LBC20
LC02F:  cmp     #$1C
        beq     LC040
        cmp     #$1D
        beq     LC04D
        cmp     #$1E
        beq     LC05D
        cmp     #$1F
        beq     LC06D
        rts
LC040:  lda     $0505
        cmp     #$1B
        beq     LC068
        inc     $0505
LC04A:  lda     #$00
        rts
LC04D:  lda     $0505
        beq     LC058
        dec     $0505
        jmp     LC04A
LC058:  lda     #$1B
        sta     $0505
LC05D:  lda     $0506
        beq     LC04A
        dec     $0506
        jmp     LC04A
LC068:  lda     #$00
        sta     $0505
LC06D:  lda     $0506
        cmp     #$14
        beq     LC04A
        inc     $0506
        bne     LC04A
LC079:  jsr     LAFD5
        jsr     LC105
        jsr     LB748
        jsr     LC25C
        jsr     LB748
LC088:  lda     #$3E
        jsr     LC025
        cmp     #$1E
        beq     LC0CF
        cmp     #$1F
        beq     LC0D8
        cmp     #$20
        beq     LC0E2
        cmp     #$03
        beq     LC09F
        bne     LC088
LC09F:  jsr     LC108
        jsr     LB74C
        lda     #$15
        sta     $49
        lda     #$1A
        jsr     LB575
        jsr     LB420
        lda     #$20
        sta     $4F
        jsr     LB436
        jsr     LB4AB
        jsr     LB194
        jsr     LAEE8
        lda     #$10
        sta     $32
        lda     #$00
        sta     $F0
        jsr     LBE14
        jmp     L826A
LC0CF:  lda     $49
        beq     LC088
        dec     $49
        jmp     LC088
LC0D8:  lda     $49
        cmp     #$05
        beq     LC088
        inc     $49
        bne     LC088
LC0E2:  lda     $49
        asl     a
        tax
        lda     LC0F3,x
        sta     $19
        lda     LC0F4,x
        sta     $1A
        jmp     ($0019)
LC0F3:
        .addr   LC2E5,LC5BE,LC5BB,LBFCE,LC697,LC17D
LC0FF:  jsr     LB53C
        jmp     LB562
LC105:  lda     #$FF
        .byte   $AE
LC108:  lda     #$00
        sta     $0500
        jsr     LB559
        lda     #$09
        sta     $74
        lda     #$00
        sta     $3D
        jsr     LB748
LC11B:  jsr     LB4F4
        jsr     LB52E
        lda     $0500
        beq     LC129
        lda     $2007
LC129:  ldy     #$0A
        ldx     $3D
LC12D:  lda     $0500
        beq     LC13B
        lda     $2007
        sta     $0520,x
        bit     $2007
        jmp     LC141
LC13B:  lda     $0520,x
        put_tile
LC141:  inx
        dey
        bne     LC12D
        stx     $3D
        dec     $74
        beq     LC0FF
        inc     $49
        bne     LC11B
LC14F:  jsr     LAEE8
        lda     #$B0
        sta     $0201
        lda     #$00
        sta     $0202
        lda     $0505
        clc
        adc     #$02
        asl     a
        asl     a
        asl     a
        sta     $0203
        lda     $0506
        clc
        adc     #$03
        asl     a
        asl     a
        asl     a
        sta     $0200
        dec     $0200
        jmp     LC1CC
LC17A:  jmp     LC079
LC17D:  jsr     LB420
        jsr     LC108
        lda     #$05
        jsr     LC276
        jsr     LAFD5
        jsr     LC01A
LC18E:  jsr     LB656
        lda     $48
        sta     $0505
        lda     $49
        sta     $0506
        jsr     LC1CC
        jsr     LB65F
        jsr     LBC20
        cmp     #$0D
        beq     LC1B7
        cmp     #$1B
        beq     LC17A
        bcc     LC18E
        jsr     LB575
        jsr     LC1C0
        jmp     LC18E
LC1B7:  jsr     LB56F
        jsr     LC1C0
        jmp     LC18E
LC1C0:  lda     $49
        cmp     #$15
        bcs     LC1C7
        rts
LC1C7:  lda     #$14
        sta     $49
        rts
LC1CC:  jsr     LB74C
        lda     #$16
        sta     $49
        lda     #$58
        jsr     L894D
        lda     #$3A
        jsr     L894D
        jsr     L895D
        jsr     LB56B
        lda     #$59
        jsr     L894D
        lda     #$3A
        jsr     L894D
        jsr     L895D
        lda     $0505
        sta     $28
        lda     #$02
        sta     $48
        lda     #$16
        sta     $49
        jsr     LC20B
        lda     $0506
        sta     $28
        lda     #$02
        sta     $48
        inc     $49
LC20B:  jsr     LA7CE
        jsr     L913E
        lda     #$3D
        clc
        adc     #$03
        sta     $52
        lda     #$00
        sta     $53
        jmp     L895A
LC21F:
        .byte   "MODE  ", $00
LC226:
        .byte   "SELECT", $00
LC22D:
        .byte   "MOVE  ", $00
LC234:
        .byte   "COPY  ", $00
LC23B:
        .byte   "CHAR  ", $00
LC242:
        .byte   "CLEAR ", $00
LC249:
        .byte   "FILE  ", $00
LC250:
        .addr   LC226,LC234,LC22D,LC242,LC249,LC23B
LC25C:  lda     #$00
        sta     $75
LC260:  lda     #$20
        jsr     L894D
        lda     $75
        jsr     LC293
        jsr     LB56F
        inc     $75
        lda     $75
        cmp     #$06
        bne     LC260
        rts
LC276:  sta     $7C
LC278:  lda     #$05
        sta     $48
        lda     #$16
        sta     $49
        lda     #<LC21F
        sta     $52
        lda     #>LC21F
        sta     $53
        jsr     L895A
        lda     #$05
        sta     $48
        inc     $49
        lda     $7C
LC293:  asl     a
        tax
        lda     LC250,x
        sta     $52
        lda     LC251,x
        sta     $53
        jmp     L895A
LC2A2:
        .byte   $00,$08,$10,$18,$B8,$C0,$C8,$D0,$D8,$E0,$E8,$F0,$F8
LC2AF:  jsr     LC2B5
        jmp     LC303
LC2B5:  jsr     LC353
        lda     $48
        pha
        lda     $49
        pha
        lda     #$0A
        sta     $48
        lda     #$16
        sta     $49
        lda     $88
        ora     #$30
        jsr     LB575
        pla
        sta     $49
        pla
        sta     $48
        rts
LC2D4:  jsr     LB656
        jsr     LC01A
        lda     #$20
        jsr     LB575
        jsr     LB65F
        jmp     LC303
LC2E5:  jsr     LC108
LC2E8:  jsr     LAFC9
        lda     #$00
        sta     $7F
        sta     $7D
        jsr     LC276
        lda     #$03
        sta     $88
        jsr     LC2B5
        lda     #$0C
        sta     $48
        lda     #$17
        sta     $49
LC303:  jsr     LC408
LC306:  jsr     LB656
        jsr     LC14F
        jsr     LB65F
        lda     #$5E
        jsr     LC025
        jsr     LC02F
        ldx     #$00
LC319:  cmp     LC335,x
        beq     LC325
        inx
        cpx     #$08
        bne     LC319
        beq     LC306
LC325:  txa
        asl     a
        tax
        lda     LC33D,x
        sta     $19
        lda     LC33E,x
        sta     $1A
        jmp     ($0019)
LC335:
        .byte   $1B,$0D,$0C,$0B,$08,$12,$20,$44
LC33D:
        .addr   LC37D,LC2AF,LC394,LC39D,LC3A7,LC3B5,LC3C3,LC2D4
LC34D:  lda     #$00
        sta     $88
        beq     LC35B
LC353:  inc     $88
        lda     $88
        cmp     #$04
        beq     LC34D
LC35B:  asl     a
        asl     a
        asl     a
        asl     a
        ora     V_TILE_HIGH
        sta     $F4
        jsr     LB42B
        lda     #$27
        ldx     #$0E
        jsr     L81EC
        jsr     bar_row
        lda     #$27
        ldx     #$2E
        jsr     L81EC
        jsr     bar_row
        jmp     LB53C
LC37D:  lda     #$0C
        sta     $48
        lda     #$16
        sta     $49
        lda     #$05
        jsr     LB575
        inc     $49
        lda     #$05
        jsr     LB575
        jmp     LC079
LC394:  lda     $7D
        beq     LC39A
        dec     $7D
LC39A:  jmp     LC303
LC39D:  lda     $7D
        cmp     #$0C
        beq     LC39A
        inc     $7D
        bne     LC39A
LC3A7:  lda     $48
        cmp     #$1A
        beq     LC39A
        inc     $48
        inc     $48
        inc     $7F
        bne     LC39A
LC3B5:  lda     $48
        cmp     #$0C
        beq     LC39A
        dec     $48
        dec     $48
        dec     $7F
        bpl     LC39A
LC3C3:  jsr     LB656
        jsr     LC01A
        jsr     LB4F4
        ldx     $7D
        lda     LC2A2,x
        clc
        adc     $7F
        sta     $73
        lda     #$09
        jsr     L896A
        lda     $88
        sta     $9C
        jsr     LC4FF
        inc     $0505
        lda     $0505
        cmp     #$1C
        beq     LC3F2
LC3EC:  jsr     LB65F
        jmp     LC303
LC3F2:  lda     #$00
        sta     $0505
        inc     $0506
        lda     $0506
        cmp     #$15
        bne     LC3EC
        lda     #$14
        sta     $0506
        bpl     LC3EC
LC408:  jsr     LB656
        lda     #$0C
        sta     $48
        lda     #$16
        sta     $49
        ldx     $7D
        lda     LC2A2,x
        sta     $7E
        ldy     #$08
        ldx     #$00
LC41E:  lda     $7E
        sta     $0510,x
        inx
        lda     #$20
        sta     $0510,x
        inx
        inc     $7E
        dey
        bne     LC41E
        jsr     LB4F4
        ldx     #$00
        ldy     #$08
        jsr     LB42B
        jsr     LB52E
LC43C:  lda     $0510,x
        put_tile
        inx
        lda     #$20
        put_tile
        inx
        dey
        bne     LC43C
        jsr     LB53C
        jmp     LB65F
LC452:  inc     $48
        inc     $48
        lda     $E4
        ldx     $4F
        cpx     #$20
        beq     LC460
        lda     $E5
LC460:  lsr     a
        lsr     a
        clc
        adc     $49
        adc     #$03
        and     #$3F
        sta     $49
        lsr     a
        lsr     a
        lsr     a
        cmp     #$04
        bcc     LC46E
        adc     #$03
LC46E:  clc
        adc     $4F
        sta     $4B
        lda     $49
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        clc
        adc     $48
        sta     $4A
        rts
LC4B0:  jsr     LC452
        lda     #$10
        jsr     L896A
        lda     $73
        sta     $9C
        rts
LC4C7:  jmp     L8494
LC4CA:  lda     $48
        pha
        lda     $49
        pha
        jsr     LA26E
        cmp     #$1C
        bcs     LC4C7
        pha
        jsr     LA26B
        lda     $0400
        cmp     #$18
        bcs     LC4C7
        sta     $49
        pla
        sta     $48
        jsr     LA26B
        lda     $0400
        cmp     #$04
        bcs     LC4C7
        and     #$03
        sta     $9C
        jsr     LC4FF
        pla
        sta     $49
        pla
        sta     $48
        rts
LC4FF:  jsr     LC452
        lda     $9C
        sta     $73
        lda     #$11
        jmp     L896A
LC50D:  lda     #$03
        sta     $0500
        lda     #$00
        sta     $0512
        sta     $0514
        sta     $0515
        lda     #$04
        sta     $0513
        lda     $E5
        sta     $0516
        jsr     L9D9F
        jmp     bg_save_file
LC561:  rts
LC562:  jmp     disk_error
LC565:  jsr     LBE53
        bcs     LC561
        lda     $0500
        cmp     #$03
        bne     LC562
        jsr     L9EB1
        bcs     LC562
        jsr     L9DAC
        lda     $0516
        sta     $E5
        jmp     bg_load_file
LC5AF:  lda     #$00
        sta     $AE
        lda     #$F8
        sta     $0210
        jmp     LC079
LC5BB:  lda     #$02
        .byte   $AE
LC5BE:  lda     #$01
        sta     $7C
        tax
        dex
        stx     $AF
        jsr     LC108
        jsr     LAFC9
        lda     #$00
        sta     $AE
        jsr     LC278
LC5D3:  jsr     LC14F
        jsr     LC649
        lda     #$20
        jsr     LC025
        jsr     LC02F
        cmp     #$1B
        beq     LC5AF
        cmp     #$12
        beq     LC619
        cmp     #$08
        beq     LC5EF
        bne     LC5D3
LC5EF:  lda     $AE
        beq     LC5D3
        jsr     LB656
        jsr     LC01A
        jsr     LB4F4
        lda     $91
        sta     $73
        lda     #$09
        jsr     L896A
        lda     $88
        sta     $9C
        jsr     LC4FF
        jsr     LB65F
        lda     $AF
        beq     LC617
        lda     #$00
        sta     $AE
LC617:  beq     LC5D3
LC619:  jsr     LB656
        jsr     LC01A
        jsr     LB4F4
        lda     #$05
        jsr     L896A
        lda     $73
        sta     $91
        lda     $AF
        beq     LC638
        lda     #$20
        sta     $73
        lda     #$09
        jsr     L896A
LC638:  lda     #$FF
        sta     $AE
        jsr     LC4B0
        lda     $9C
        sta     $88
        jsr     LB65F
        jmp     LC5D3
LC649:  lda     $AE
        beq     LC675
        lda     $91
        sta     $0211
        lda     $88
        and     #$0F
        sta     $0212
        lda     $0505
        clc
        adc     #$02
        asl     a
        asl     a
        asl     a
        sta     $0213
        lda     $0506
        clc
        adc     #$03
        asl     a
        asl     a
        asl     a
        sta     $0210
        dec     $0210
        rts
LC675:  lda     #$F8
        sta     $0210
        rts
LC67B:
        .byte   "SAVE(S),LOAD(L)?", $00
LC68C:
        .byte   "FILE NAME", $22, $00
LC697:  jsr     LC108
        jsr     LB74C
        lda     #$16
        sta     $49
        lda     #$1A
        jsr     LB575
        jsr     LAE96
        jsr     LB420
        jsr     LB4AB
        lda     #<LC67B
        sta     $52
        lda     #>LC67B
        sta     $53
        jsr     L895A
        jsr     LBC20
        cmp     #$53
        beq     LC6C7
        cmp     #$4C
        beq     LC6D3
        bne     LC6D0
LC6C7:  jsr     LB575
        jsr     LC6DF
        jsr     LC50D
LC6D0:  jmp     LBFD1
LC6D3:  jsr     LB575
        jsr     LC6DF
        jsr     LC565
        jmp     LC6D0
LC6DF:  jsr     LB56F
        lda     #<LC68C
        sta     $52
        lda     #>LC68C
        sta     $53
        jsr     L895A
        jsr     LBD04
        lda     $0500
        cmp     #$03
        beq     LC704
        jsr     L8BA4
        lda     #$09
        sta     $09
        jsr     L9E52
        jmp     L9E9D
LC704:  pla
        pla
        jmp     LC6D0
LC709:
        .addr   LC739,LC739,LC745,LC74B,LC751,LC73F,LC74B,LC745
        .addr   LC757,LC757,LC745,LC745,LC75D,LC73F,LC745,LC73F
LC729:
        .byte   $00,$1C,$00,$00,$00,$00,$38,$58,$00,$0C,$78,$20,$00,$48,$88,$58
LC739:
        .addr   LC763,LC799,LC763
LC73F:
        .addr   LC76C,LC805,LC77E
LC745:
        .addr   LC76C,LC7AB,LC76C
LC74B:
        .addr   LC76C,LC7BD,LC775
LC751:
        .addr   LC77E,LC7CF,LC77E
LC757:
        .addr   LC775,LC7E1,LC775
LC75D:
        .addr   LC775,LC7F3,LC775
LC763:
        .byte   $01,$02,$41,$43,$41,$02,$01,$03,$01
LC76C:
        .byte   $02,$02,$02,$02,$02,$02,$02,$02,$02
LC775:
        .byte   $01,$01,$41,$41,$C1,$81,$81,$01,$01
LC77E:
        .byte   $01,$02,$42,$42,$42,$02,$02,$02,$02
LC787:
        .byte   $00
LC788:
        .byte   $00,$00,$FE,$02,$FE,$02,$00,$02,$02,$00,$02,$FE,$02,$FE,$00,$FE
        .byte   $FE
LC799:
        .addr   LC837,LC833,LC83F,LC82F,LC83F,LC833,LC83B,LC82B
        .addr   LC83B
LC7AB:
        .addr   LC847,LC847,LC847,LC847,LC847,LC847,LC847,LC847
        .addr   LC847
LC7BD:
        .addr   LC853,LC84B,LC85F,LC857,LC867,LC84F,LC863,LC853
        .addr   LC85B
LC7CF:
        .addr   LC88B,LC88F,LC897,LC897,LC897,LC88B,LC893,LC893
        .addr   LC893
LC7E1:
        .addr   LC86B,LC86B,LC87F,LC877,LC887,LC86F,LC883,LC873
        .addr   LC87B
LC7F3:
        .addr   LC81F,LC81F,LC827,LC823,LC827,LC81F,LC827,LC823
        .addr   LC827
LC805:
        .addr   LC817,LC817,LC81B,LC81B,LC81B,LC817,LC817,LC817
        .addr   LC817
LC817:
        .byte   $70,$71,$72,$73
LC81B:
        .byte   $71,$70,$73,$72
LC81F:
        .byte   $D4,$EF,$EF,$EF
LC823:
        .byte   $D0,$EF,$EF,$EF
LC827:
        .byte   $D2,$EF,$EF,$EF
LC82B:
        .byte   $00,$01,$02,$03
LC82F:
        .byte   $01,$00,$03,$02
LC833:
        .byte   $14,$15,$16,$17
LC837:
        .byte   $18,$19,$1A,$1B
LC83B:
        .byte   $0C,$0D,$0E,$0F
LC83F:
        .byte   $0D,$0C,$0F,$0E
LC843:
        .byte   $15,$14,$17,$16
LC847:
        .byte   $38,$39,$3A,$3B
LC84B:
        .byte   $50,$51,$52,$53
LC84F:
        .byte   $52,$53,$50,$51
LC853:
        .byte   $40,$41,$42,$43
LC857:
        .byte   $41,$40,$43,$42
LC85B:
        .byte   $48,$49,$4A,$4B
LC85F:
        .byte   $49,$48,$4B,$4A
LC863:
        .byte   $4A,$4B,$48,$49
LC867:
        .byte   $4B,$4A,$49,$48
LC86B:
        .byte   $A0,$A1,$A2,$A3
LC86F:
        .byte   $A2,$A3,$A0,$A1
LC873:
        .byte   $98,$99,$9A,$9B
LC877:
        .byte   $99,$98,$9B,$9A
LC87B:
        .byte   $9C,$9D,$9E,$9F
LC87F:
        .byte   $9D,$9C,$9F,$9E
LC883:
        .byte   $9E,$9F,$9C,$9D
LC887:
        .byte   $9F,$9E,$9D,$9C
LC88B:
        .byte   $68,$69,$6A,$6B
LC88F:
        .byte   $6C,$6D,$6E,$6F
LC893:
        .byte   $60,$61,$62,$63
LC897:
        .byte   $61,$60,$63,$62
LC89B:
        .byte   $6D,$6C,$6F,$6E
LC89F:
        .byte   $69,$68,$6B,$6A
LC8A3:  txa
        pha
        tya
        pha
        jsr     LC8B4
        pla
        tay
        pla
        tax
        lda     #$00
        sta     $AB
        pla
        rti
LC8B4:  ldx     #$00
        stx     $A9
        lda     #$08
        sta     $5D
LC8BC:  ldx     $A9
        lda     $0645,x
        and     #$10
        bne     LC8EB
        dec     $5D
        beq     LC8D3
        inx
LC8CA:  inc     $A9
        lda     $A9
        cmp     #$08
        bne     LC8BC
        rts
LC8D3:  lda     #$00
        sta     $AC
        rts
LC8D8:  lda     $06C5,x
        sta     $06C3,x
        ldx     $A9
        lda     $0645,x
        and     #$EF
        sta     $0645,x
        jmp     LC8CA
LC8EB:  lda     $A9
        asl     a
        asl     a
        asl     a
        sta     $A6
        tax
        dec     $06C2,x
        bne     LC8CA
        lda     $06C4,x
        sta     $06C2,x
        dec     $06C3,x
        beq     LC8D8
        lda     $06C1,x
        and     #$0F
        tay
        lda     LC729,y
        sta     $DD
        tya
        asl     a
        tay
        lda     LC709,y
        sta     $A7
        lda     LC70A,y
        sta     $A8
        lda     $06C0,x
        asl     a
        tay
        lda     LC787,y
        clc
        adc     $06C6,x
        sta     $06C6,x
        clc
        lda     LC788,y
        adc     $06C7,x
        sta     $06C7,x
        ldy     #$00
        lda     ($A7),y
        sta     $D5
        iny
        lda     ($A7),y
        sta     $D6
        lda     $06C0,x
        tay
        lda     ($D5),y
        and     #$0F
        jsr     LAD53
        and     #$FF
        sta     $D7
        beq     LC970
        lda     $06C1,x
        and     #$F0
        clc
        adc     #$10
        cmp     $D7
        bne     LC95E
        lda     #$00
LC95E:  pha
        lda     #$0F
        and     $06C1,x
        sta     $06C1,x
        pla
        pha
        ora     $06C1,x
        sta     $06C1,x
        pla
LC970:  lsr     a
        lsr     a
        sta     $D7
        ldy     #$02
        lda     ($A7),y
        sta     $D9
        iny
        lda     ($A7),y
        sta     $DA
        lda     $06C0,x
        asl     a
        tay
        lda     ($D9),y
        sta     $DB
        iny
        lda     ($D9),y
        sta     $DC
        lda     $06C0,x
        cmp     #$01
        beq     LC998
        cmp     #$05
        bne     LC9E8
LC998:  lda     $06C1,x
        and     #$0F
        beq     LC9C2
        cmp     #$01
        beq     LC9C2
        cmp     #$04
        bne     LC9E8
        lda     $D7
        beq     LC9DE
        lda     $06C0,x
        cmp     #$05
        beq     LC9BA
        lda     #<LC89B
        sta     $DB
        lda     #>LC89B
        bne     LC9CC
LC9BA:  lda     #<LC89F
        sta     $DB
        lda     #>LC89F
        bne     LC9CC
LC9C2:  lda     $D7
        beq     LC9DE
        lda     #<LC843
        sta     $DB
        lda     #>LC843
LC9CC:  sta     $DC
        ldy     $A9
        lda     #$40
        ora     $0645,y
        sta     $0645,y
        lda     #$00
        sta     $D7
        beq     LC9E8
LC9DE:  ldy     $A9
        lda     $0645,y
        and     #$3F
        sta     $0645,y
LC9E8:  lda     $A9
        jsr     LAD53
        tax
        ldy     #$00
        lda     ($DB),y
        clc
        adc     $DD
        adc     $D7
        sta     $0281,x
        iny
        lda     ($DB),y
        clc
        adc     $DD
        adc     $D7
        sta     $0285,x
        iny
        lda     ($DB),y
        clc
        adc     $DD
        adc     $D7
        sta     $0289,x
        iny
        lda     ($DB),y
        clc
        adc     $DD
        adc     $D7
        sta     $028D,x
        ldy     $A9
        lda     $0645,y
        sta     $0282,x
        sta     $0286,x
        sta     $028A,x
        sta     $028E,x
        ldy     $A6
        lda     $06C6,y
        sta     $0283,x
        sta     $028B,x
        clc
        adc     #$08
        sta     $0287,x
        sta     $028F,x
        lda     $06C7,y
        sta     $0280,x
        sta     $0284,x
        clc
        adc     #$08
        sta     $0288,x
        sta     $028C,x
        jmp     LC8CA
LCA55:  jmp     L8494
LCA58:  jmp     L8491
LCA5B:  jsr     L908C
        jsr     LA87A
        jsr     LA26E
        jsr     LAF44
        jsr     LA87D
        lda     $0400
        tax
        lda     #$E7
        and     $0645,x
        sta     $0645,x
        txa
        sta     $A4
        asl     a
        asl     a
        asl     a
        sta     $A5
        jsr     L86D9
        jsr     L90BF
        cmp     #$8A
        bne     LCA58
        jsr     L908C
        jsr     LA87A
        jsr     LCB7D
        cmp     #$10
        bcs     LCA55
        sta     $06C1,x
        jsr     LCB75
        stx     $AD
        sta     $28
        cmp     #$09
        bcc     LCAB6
        lda     #$08
        sta     $2C
        lda     #$00
        sta     $29
        sta     $2D
        jsr     LA80E
        lda     $28
        bne     LCAB6
        inc     $28
LCAB6:  lda     $28
        ldx     $AD
        sta     $06C0,x
        lda     $06C1,x
        asl     a
        tay
        lda     LC709,y
        sta     $19
        lda     LC70A,y
        sta     $1A
        ldy     #$04
        lda     ($19),y
        sta     $1B
        iny
        lda     ($19),y
        sta     $1C
        lda     $06C0,x
        tay
        lda     ($1B),y
        and     #$F0
        ldy     $A4
        pha
        lda     $0645,y
        and     #$3F
        sta     $0645,y
        pla
        ora     $0645,y
        sta     $0645,y
        jsr     LCB84
        bcs     LCB67
        jsr     LCB75
        bcs     LCB06
        sta     $06C2,x
        sta     $06C4,x
        jsr     LCB84
        bcs     LCB67
LCB06:  jsr     LCB75
        bcs     LCB18
        adc     #$01
        sta     $06C3,x
        sta     $06C5,x
        jsr     LCB84
        bcs     LCB67
LCB18:  jsr     LCB75
        bcs     LCB41
        ldy     #$00
        lda     $0400
        jsr     LB08D
        and     #$FF
        beq     LCB2B
        ldy     #$20
LCB2B:  ldx     $A4
        lda     $0645,x
        and     #$DF
        sta     $0645,x
        tya
        ora     $0645,x
        sta     $0645,x
        jsr     LCB84
        bcs     LCB67
LCB41:  jsr     LCB75
        cmp     #$04
        bcs     LCB72
        ldx     $A4
        pha
        lda     $0645,x
        and     #$FC
        sta     $0645,x
        pla
        ora     $0645,x
        sta     $0645,x
        jsr     LA87D
        lda     #$78
        ldx     $A5
        sta     $06C6,x
        sta     $06C7,x
LCB67:  ldx     $A4
        lda     $0645,x
        ora     #$08
        sta     $0645,x
        rts
LCB72:  jmp     L8494
LCB75:  jsr     LAC14
        jsr     LCB92
        bcs     LCB90
LCB7D:  jsr     LA26E
        ldx     $A5
        clc
        rts
LCB84:  jsr     L90BF
        cmp     #$29
        beq     LCB8D
        clc
        rts
LCB8D:  jsr     L908C
LCB90:  sec
        rts
LCB92:  jsr     L90BF
        cmp     #$2C
        beq     LCB90
        clc
        rts
LCB9B:  jsr     LA26E
        jsr     LAF44
        asl     a
        asl     a
        asl     a
        sta     $A5
        jsr     LCB75
        sta     $06C6,x
        jsr     LCB75
        sta     $06C7,x
        rts
LCBB3:  jsr     LA26E
        jsr     LAF44
        tax
        lda     $0645,x
        and     #$08
        beq     LCBC9
        lda     #$10
        ora     $0645,x
        sta     $0645,x
LCBC9:  jsr     L849C
        beq     LCBD4
        jsr     LAC14
        jmp     LCBB3
LCBD4:  lda     #$FF
        sta     $AC
LCBD8:  rts
LCBD9:  lda     #$FF
        .byte   $AE
LCBDC:  lda     #$00
        sta     $75
        jsr     LA26E
        jsr     LAF44
        tay
        lda     $0645,y
        and     #$EF
        sta     $0645,y
        lda     $75
        beq     LCBF6
        jsr     LCC97
LCBF6:  jsr     L849C
        beq     LCBD8
        jsr     LAC14
        lda     $75
        beq     LCBDC
        bne     LCBD9
LCC04:  jsr     LA94E
        lda     $28
        sta     $74
        lda     $0645,x
        and     #$08
        beq     LCC55
        jsr     LA944
        lda     $06C6,x
        sta     $6B
        lda     $06C7,x
        sta     $6C
        lda     #$00
        sta     $28
        ldy     #$08
LCC25:  lda     $74
        cmp     $28
        beq     LCC47
        jsr     LA944
        lda     $06C6,x
        sec
        sbc     $6B
        jsr     LCC5D
        bcs     LCC47
        lda     $06C7,x
        sec
        sbc     $6C
        jsr     LCC5D
        bcs     LCC47
        jmp     LA97E
LCC47:  inc     $28
        dey
        bne     LCC25
        lda     #$FF
        sta     $28
LCC50:  sta     $29
        jmp     LA97E
LCC55:  lda     #$FE
        sta     $28
        lda     #$FF
        bne     LCC50
LCC5D:  bcs     LCC69
        sta     $75
        lda     #$FF
        sec
        sbc     $75
        clc
        adc     #$01
LCC69:  cmp     #$10
LCC6B:  rts
LCC6C:  jsr     LA26E
        jsr     LAF44
        pha
        tay
        lda     #$00
        sta     $0645,y
        jsr     LCC97
        pla
        asl     a
        asl     a
        asl     a
        tax
        lda     #$00
        sta     $06C0,x
        sta     $06C6,x
        sta     $06C7,x
        jsr     L849C
        beq     LCC6B
        jsr     LAC14
        jmp     LCC6C
LCC97:  tya
        jsr     LAD53
        tay
        lda     #$F8
        sta     $0280,y
        sta     $0284,y
        sta     $0288,y
        sta     $028C,y
        rts
LCCAB:
        .byte   $80,$47,$4F,$54,$4F,$81,$47,$4F,$53,$55,$42,$82,$52,$55,$4E,$83
        .byte   $52,$45,$54,$55,$52,$4E,$84,$52,$45,$53,$54,$4F,$52,$45,$85,$54
        .byte   $48,$45,$4E,$86,$4C,$49,$53,$54,$87,$53,$59,$53,$54,$45,$4D,$88
        .byte   $54,$4F,$89,$53,$54,$45,$50,$8A,$53,$50,$52,$49,$54,$45,$8B,$50
        .byte   $52,$49,$4E,$54,$8C,$46,$4F,$52,$8D,$4E,$45,$58,$54,$8E,$50,$41
        .byte   $55,$53,$45,$8F,$49,$4E,$50,$55,$54,$90,$4C,$49,$4E,$50,$55,$54
        .byte   $91,$44,$41,$54,$41,$92,$49,$46,$93,$52,$45,$41,$44,$94,$44,$49
        .byte   $4D,$95,$52,$45,$4D,$96,$53,$54,$4F,$50,$97,$43,$4F,$4E,$54,$98
        .byte   $43,$4C,$53,$99,$43,$4C,$45,$41,$52,$9A,$4F,$4E,$9B,$4F,$46,$46
        .byte   $9C,$43,$55,$54,$9D,$4E,$45,$57,$9E,$50,$4F,$4B,$45,$9F,$43,$47
        .byte   $53,$45,$54,$A0,$56,$49,$45,$57,$A1,$4D,$4F,$56,$45,$A2,$45,$4E
        .byte   $44,$A3,$50,$4C,$41,$59,$A4,$42,$45,$45,$50,$A5,$4C,$4F,$41,$44
        .byte   $A6,$53,$41,$56,$45,$A7,$50,$4F,$53,$49,$54,$49,$4F,$4E,$A8,$4B
        .byte   $45,$59,$A9,$43,$4F,$4C,$4F,$52,$AA,$44,$45,$46,$AB,$43,$47,$45
        .byte   $4E,$AC,$53,$57,$41,$50,$AD,$43,$41,$4C,$4C,$AE,$4C,$4F,$43,$41
        .byte   $54,$45,$AF,$50,$41,$4C,$45,$54,$B0,$45,$52,$41,$B1,$54,$52,$B2
        .byte   $46,$49,$4E,$44,$B3,$47,$41,$4D,$45,$B4,$42,$47,$54,$4F,$4F,$4C
        .byte   $B5,$41,$55,$54,$4F,$B6,$44,$45,$4C,$45,$54,$45,$B7,$52,$45,$4E
        .byte   $55,$4D,$B8,$46,$49,$4C,$54,$45,$52,$B9,$43,$4C,$49,$43,$4B,$BA
        .byte   $53,$43,$52,$45,$45,$4E,$BB,$42,$41,$43,$4B,$55,$50,$BC,$45,$52
        .byte   $52,$4F,$52,$BD,$52,$45,$53,$55,$4D,$45,$BE,$42,$47,$50,$55,$54
        .byte   $BF,$42,$47,$47,$45,$54,$C0,$43,$41,$4E,$EF,$58,$4F,$52,$F0,$4F
        .byte   $52,$F1,$41,$4E,$44,$F2,$4E,$4F,$54,$F3,$3C,$3E,$F4,$3E,$3D,$F5
        .byte   $3C,$3D,$F6,$3D,$F7,$3E,$F8,$3C,$F9,$2B,$FA,$2D,$FB,$4D,$4F,$44
        .byte   $FC,$2F,$FD,$2A,$CA,$41,$42,$53,$CB,$41,$53,$43,$CC,$53,$54,$52
        .byte   $24,$CD,$46,$52,$45,$CE,$4C,$45,$4E,$CF,$50,$45,$45,$4B,$D0,$52
        .byte   $4E,$44,$D1,$53,$47,$4E,$D2,$53,$50,$43,$D3,$54,$41,$42,$D4,$4D
        .byte   $49,$44,$24,$D5,$53,$54,$49,$43,$4B,$D6,$53,$54,$52,$49,$47,$D7
        .byte   $58,$50,$4F,$53,$D8,$59,$50,$4F,$53,$D9,$56,$41,$4C,$DA,$50,$4F
        .byte   $53,$DB,$43,$53,$52,$4C,$49,$4E,$DC,$43,$48,$52,$24,$DD,$48,$45
        .byte   $58,$24,$DE,$49,$4E,$4B,$45,$59,$24,$DF,$52,$49,$47,$48,$54,$24
        .byte   $E0,$4C,$45,$46,$54,$24,$E1,$53,$43,$52,$24,$E2,$49,$4E,$53,$54
        .byte   $52,$E3,$43,$52,$41,$53,$48,$E4,$45,$52,$52,$E5,$45,$52,$4C,$E6
        .byte   $56,$43,$54,$FF
LCEBF:
        .addr   L96C6,L9648,L86B0,L96A6,L9EC9,L8491,L8767,L818F
        .addr   L8491,L8491,LAFFC,L88C5,L97FC,L9887,L9716,L993C
        .addr   L9A6C,L8467,L9782,L9F36,L9C0C,L8467,L885B,L889C
        .addr   LAE0A,L97A4,L9AFD,L8491,LCBDC,L84B6,L975D,LAF4F
        .addr   LB153,LCBB3,L826A,LA01E,LBCD0,L9DE1,L9D64,LCB9B
        .addr   L9B68,LC4CA,LB098,LAED1,L9A0E,L970A,L9748,LAEF3
        .addr   LCBD9,L962B,L9573,LAD5B,LBFD1,L8BDA,L8758,L8C30
        .addr   LAEB8,L9625,LAE48,L818F,L9510,L9527,LB201,LB1B8
        .addr   LCC6C
LCF41:
        .addr   LA984,LAA28,LAAA6,LA91F,LAA1D,LA973,LA9BD,LA99C
        .addr   L8491,L8491,LAC81,LAD38,LAD47,LA958,LA961,LAADF
        .addr   LA933,LA8E6,LAA82,LAAD6,LAB58,LAC41,LABED,LAA3B
        .addr   LABAF,LCC04,LA90D,LA914,LA96A
LCF7B:
        .byte   $4E,$53,$2D,$48,$55,$42,$41,$53,$49,$43,$20,$56,$33,$2E,$30,$20
        .byte   $00
LCF8C:
        .byte   $B4,$20,$4E,$49,$4E,$54,$45,$4E,$44,$4F,$2F,$53,$48,$41,$52,$50
        .byte   $2F,$48,$55,$44,$53,$4F,$4E,$00
LCFA4:
        .addr   LCFAE,LCFBE
LCFA8:
        .addr   LCFCE,LCFDE,LCFEE
LCFAE:
        .byte   $0F,$2C,$15,$07,$0F,$27,$21,$12,$0F,$29,$36,$17,$0F,$30,$26,$07
LCFBE:
        .byte   $0F,$30,$21,$02,$0F,$30,$27,$18,$0F,$30,$27,$16,$0F,$29,$36,$17
LCFCE:
        .byte   $0F,$36,$16,$02,$0F,$27,$30,$19,$0F,$35,$25,$17,$0F,$30,$27,$16
LCFDE:
        .byte   $0F,$30,$16,$01,$0F,$10,$00,$01,$0F,$30,$29,$09,$0F,$30,$16,$07
LCFEE:
        .byte   $0F,$30,$26,$12,$0F,$30,$15,$12,$0F,$30,$12,$16,$0F,$30,$26,$19
        .byte   $FF,$FF
LD000:
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$32,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$34,$20,$20,$20,$20,$D7,$20,$D7,$20,$D7,$20,$20,$20,$20
        .byte   $20,$20,$C0,$C0,$C0,$D2,$20,$20,$20,$20,$20,$39,$20,$20,$20,$20
        .byte   $20,$C0,$C0,$C0,$ED,$ED,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$20,$20
        .byte   $20,$20,$20,$20,$20,$D2,$20,$20,$C0,$C0,$C0,$C0,$C0,$C0,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$E3,$20,$D2,$20,$20,$20,$20,$20,$20,$20,$20,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$E3,$20,$20,$38,$20,$20,$20
        .byte   $20,$20,$C0,$C0,$C0,$D2,$20,$20,$20,$20,$20,$E3,$20,$20,$D2,$20
        .byte   $20,$20,$D7,$20,$C0,$C0,$C0,$D2,$C0,$C0,$C0,$C0,$C0,$C0,$20,$20
        .byte   $20,$20,$20,$20,$20,$D2,$20,$20,$20,$20,$C0,$C0,$C0,$C0,$D2,$20
        .byte   $20,$C0,$C0,$C0,$C0,$20,$20,$D2,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$D2,$20,$D7,$31,$20,$20,$20,$20,$20,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$E3,$20,$20,$20,$20
        .byte   $20,$20,$C0,$D2,$C0,$C0,$C0,$C0,$C0,$D2,$20,$20,$20,$20,$D2,$20
        .byte   $20,$20,$E3,$20,$20,$20,$20,$D2,$20,$20,$C0,$C0,$C0,$C0,$20,$20
        .byte   $20,$20,$20,$D2,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$20,$D2,$C0
        .byte   $C0,$C0,$C0,$C0,$C0,$20,$20,$D2,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$D2,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$20,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$35,$D2,$20,$20,$20,$E3,$20,$D2,$20,$20,$20,$20,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$20,$D7,$20,$20,$20
        .byte   $20,$20,$C0,$C0,$ED,$ED,$C0,$C0,$C0,$C0,$C0,$D2,$20,$20,$D2,$20
        .byte   $20,$20,$E3,$20,$20,$37,$20,$D2,$20,$20,$20,$D2,$C0,$C0,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$D2,$20
        .byte   $20,$C0,$C0,$C0,$C0,$C0,$C0,$D2,$20,$20,$20,$D2,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$20
        .byte   $20,$20,$20,$D7,$20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$D2,$20
        .byte   $33,$D7,$20,$20,$20,$D7,$20,$20,$20,$D7,$20,$D2,$E3,$E3,$20,$20
        .byte   $20,$20,$C0,$C0,$C0,$D2,$20,$20,$20,$20,$20,$D2,$20,$20,$D2,$C0
        .byte   $C0,$C0,$ED,$ED,$C0,$C0,$ED,$ED,$C0,$C0,$C0,$D2,$C0,$C0,$20,$20
        .byte   $20,$20,$20,$20,$20,$D2,$20,$20,$20,$20,$20,$D2,$20,$20,$D2,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$D2,$20,$20,$20,$20
        .byte   $20,$20,$20,$36,$20,$D2,$20,$20,$20,$30,$20,$D2,$20,$20,$20,$20
        .byte   $20,$E3,$20,$20,$20,$E3,$20,$20,$20,$E3,$20,$D2,$20,$20,$20,$20
        .byte   $20,$20,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0
        .byte   $C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$C0,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20,$20
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$03,$00,$00,$19,$0A,$00
        .byte   $00,$C0,$00,$00,$08,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$30
        .byte   $C0,$01,$00,$00,$30,$E0,$30,$00,$00,$00,$00,$00,$04,$04,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .include        "platform-code.inc"
        .segment "VECTORS"
        .addr   $00ED,L80BA,irq_handler
