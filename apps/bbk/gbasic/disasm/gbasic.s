; da65 V2.19 - Git 357f64e
; Created:    2026-11-13 11:20:28
; Input file: gbasic.bin
; Page:       1


        .setcpu "6502"

L0019           := $0019
L001B           := $001B
L0028           := $0028
PPUCTRL         := $2000                        ; 控制寄存器：名称表、图案表、NMI
PPUMASK         := $2001                        ; 显示开关：背景、精灵
PPUSTATUS       := $2002                        ; 状态：位 7 为垂直消隐；读取会复位 PPUADDR 与 PPUSCROLL 的写入顺序
OAMADDR         := $2003                        ; 精灵内存地址
OAMDATA         := $2004                        ; 精灵内存数据
PPUSCROLL       := $2005                        ; 滚动，连续写两次：水平、垂直
PPUADDR         := $2006                        ; 显存地址，连续写两次：高字节、低字节
PPUDATA         := $2007                        ; 显存数据，写入后地址自动递增
L5800           := $5800
L5809           := $5809
L580C           := $580C
        jmp     L7D00

        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $8D,$03,$7B,$A2,$7B,$A0,$03,$4C
        .byte   $03,$58,$A2,$00,$8E,$0A,$7B,$8E
        .byte   $0B,$7B,$8E,$0C,$7B,$8E,$0D,$7B
        .byte   $86,$86,$86,$87,$CA,$86,$88,$86
        .byte   $89,$20,$F8,$8F,$A5,$86,$85,$44
        .byte   $A5,$87,$85,$45,$20,$04,$84,$A0
        .byte   $00,$B1,$5B,$F0,$2B,$A5,$5B,$85
        .byte   $09,$A5,$5C,$85,$0A,$A0,$00,$B1
        .byte   $09,$F0,$1D,$C8,$B1,$09,$85,$28
        .byte   $C8,$B1,$09,$85,$29,$A5,$88,$85
        .byte   $2C,$A5,$89,$85,$2D,$20,$F7,$8B
        .byte   $B0,$06,$20,$A8,$7B,$4C,$70,$7B
        .byte   $A9,$05,$20,$33,$7B,$A9,$0C,$8D
        .byte   $A3,$5F,$8D,$90,$FF,$A9,$00,$85
        .byte   $62,$20,$F1,$B3,$60,$A9,$00,$85
        .byte   $62,$A0,$01,$B1,$09,$85,$28,$C8
        .byte   $B1,$09,$85,$29,$20,$3C,$8D,$20
        .byte   $EB,$8C,$20,$5F,$86,$A9,$20,$20
        .byte   $F0,$87,$A9,$03,$18,$65,$09,$85
        .byte   $4D,$A9,$00,$65,$0A,$85,$4E,$20
        .byte   $0A,$90,$20,$3B,$8A,$20,$B7,$87
        .byte   $20,$EE,$87,$A6,$62,$CA,$A9,$0D
        .byte   $9D,$00,$07,$E8,$A9,$0A,$9D,$00
        .byte   $07,$E8,$86,$62,$20,$30,$7C,$EA
        .byte   $EA,$EA,$EA,$EA,$EA,$20,$09,$7C
        .byte   $A9,$01,$18,$65,$4D,$85,$09,$A9
        .byte   $00,$65,$4E,$85,$0A,$60,$AD,$03
        .byte   $7D,$8D,$05,$7B,$A2,$00,$A0,$05
        .byte   $8E,$06,$7B,$8C,$07,$7B,$A9,$00
        .byte   $8D,$09,$7B,$A5,$62,$8D,$08,$7B
        .byte   $A9,$03,$4C,$33,$7B,$A9,$03,$4C
        .byte   $33,$7B,$77,$77,$77,$A2,$00,$BD
        .byte   $00,$07,$9D,$00,$05,$E8,$E4,$62
        .byte   $D0,$F5,$60,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77,$77,$77,$77
        .byte   $77,$77,$77,$77,$77
L7D00:  jmp     L7D34

L7D03:  .byte   $00
L7D04:  lda     #$FF
        sta     $FF04
        ldx     #$C7
        ldy     #$58
        jsr     L7D2D
        lda     L7D03
        jsr     L7D27
        lda     #$09
        jsr     L580C
        ldx     #$07
        jsr     L5809
        lda     #$00
        ldy     #$01
        jmp     L580C

L7D27:  tax
        lda     #$01
        jmp     L5800

L7D2D:  stx     LBFFA
        sty     LBFFB
        rts

L7D34:  lda     #$0C
        sta     $5FA3
        sta     $FF90
        ldx     #$00
L7D3E:  lda     #$00
        sta     $00,x
        sta     $0600,x
        dex
        bne     L7D3E
        nop
        nop
        lda     #$02
        jsr     L5800
        sta     L7D03
        lda     #$FF
        jsr     L7D27
        lda     #$10
        sta     PPUCTRL
        ldx     #$ED
        ldy     #$00
        jsr     L7D2D
        lda     L7D03
        jsr     L7D27
        nop
        jsr     L7D8C
        jmp     L7DE0

L7D70:  ldx     #$04
        stx     $FF03
        inx
        stx     $FF0B
        inx
        stx     $FF13
        inx
        stx     $FF1B
        rts

L7D82:  bit     PPUSTATUS
        stx     PPUADDR
        sty     PPUADDR
        rts

L7D8C:  jsr     LABF1
        lda     $5FFF
        and     #$BB
        ora     #$01
        sta     $5FFF
        sta     $FF01
        jsr     L7D70
        ldx     #$00
        jsr     L7D82
        ldx     #$C4
        ldy     #$10
        stx     $FF
        sty     $FE
        ldx     #$20
        ldy     #$00
L7DB0:  lda     ($FE),y
        sta     PPUDATA
        iny
        bne     L7DB0
        inc     $FF
        dex
        bne     L7DB0
        lda     #$00
        sta     PPUADDR
        sta     PPUADDR
        bit     PPUSTATUS
        rts

        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$A9
        .byte   $08,$8D,$FF,$5F,$8D,$01,$FF,$60
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF
L7DE0:  lda     #$08
        sta     $5FFF
        sta     $FF01
        jmp     L8000

        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$20,$07,$AC
        .byte   $A9,$2A,$20,$34,$AC,$20,$43,$B3
        .byte   $A0,$00,$84,$90,$F0,$26,$A2,$03
        .byte   $0A,$0A,$0A,$0A,$0A,$26,$8E,$26
        .byte   $8F,$CA,$10,$F8,$A5,$90,$D0,$04
        .byte   $B5,$8F,$95,$8D,$E8,$F0,$F5,$D0
        .byte   $11,$20,$6B,$7E,$4C,$00,$7E,$20
        .byte   $6B,$7E,$A4,$25,$A2,$00,$86,$8E
        .byte   $86,$8F,$B9,$01,$05,$C8,$49,$30
        .byte   $C9,$0A,$90,$CA,$69,$88,$C9,$FA
        .byte   $B0,$C4,$84,$25,$C9,$B9,$F0,$D9
        .byte   $C9,$99,$F0,$DB,$C9,$A6,$F0,$0B
        .byte   $C9,$93,$F0,$07,$C9,$EA,$D0,$9D
        .byte   $4C,$1D,$82,$85,$90,$4C,$35,$7E
        .byte   $A9,$00,$85,$E2,$A5,$90,$F0,$2D
        .byte   $C9,$A6,$F0,$29,$C9,$93,$D0,$85
        .byte   $A5,$8E,$A0,$00,$91,$8C,$4C,$8C
        .byte   $7E,$A5,$8C,$C5,$8E,$A5,$8D,$E5
        .byte   $8F,$E6,$8C,$D0,$02,$E6,$8D,$60
        .byte   $20,$B9,$86,$F0,$FA,$E6,$E2,$A5
        .byte   $E2,$29,$07,$D0,$0D,$20,$07,$AC
        .byte   $A5,$8D,$20,$BF,$7E,$A5,$8C,$20
        .byte   $BF,$7E,$A9,$20,$20,$34,$AC,$A0
        .byte   $00,$B1,$8C,$20,$BF,$7E,$20,$84
        .byte   $7E,$90,$D5,$60,$48,$4A,$4A,$4A
        .byte   $4A,$20,$C8,$7E,$68,$29,$0F,$09
        .byte   $30,$C9,$3A,$90,$02,$69,$06,$4C
        .byte   $34,$AC,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$8D,$EA,$7E
        .byte   $A2,$7E,$A0,$EA,$4C,$03,$58,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$A9
        .byte   $0C,$8D,$A3,$5F,$8D,$90,$FF,$60
        .byte   $20,$1A,$7F,$20,$F1,$B3,$A9,$11
        .byte   $4C,$8F,$A9,$20,$EB,$B3,$A9,$09
        .byte   $20,$E0,$7E,$A9,$0C,$A2,$05,$A0
        .byte   $80,$20,$0C,$58,$A2,$05,$A0,$80
        .byte   $8E,$EE,$7E,$8C,$ED,$7E,$AD,$03
        .byte   $7D,$8D,$EC,$7E,$A9,$01,$20,$E0
        .byte   $7E,$90,$03,$4C,$23,$7F,$A9,$00
        .byte   $8D,$C0,$04,$8D,$ED,$7E,$8D,$F0
        .byte   $7E,$A9,$05,$8D,$EE,$7E,$A9,$01
        .byte   $8D,$C1,$04,$AD,$03,$7D,$8D,$EC
        .byte   $7E,$AD,$C1,$04,$8D,$EF,$7E,$A9
        .byte   $02,$20,$E0,$7E,$AD,$EF,$7E,$F0
        .byte   $27,$AC,$C0,$04,$C0,$FF,$F0,$98
        .byte   $B9,$00,$05,$C9,$0A,$F0,$06,$EE
        .byte   $C0,$04,$4C,$6E,$7F,$AC,$C0,$04
        .byte   $88,$A9,$00,$99,$00,$05,$C8,$99
        .byte   $00,$05,$20,$C6,$7F,$4C,$59,$7F
        .byte   $A9,$05,$20,$E0,$7E,$A9,$20,$8D
        .byte   $00,$05,$A9,$00,$8D,$01,$05,$8D
        .byte   $02,$05,$20,$F1,$B3,$20,$1A,$7F
        .byte   $4C,$21,$82,$20,$32,$8A,$20,$B1
        .byte   $83,$20,$28,$92,$60,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF
L8000:  jmp     L80AD

        .byte   $4C,$88,$AF
L8006:  .byte   $AE
L8007:  .byte   $0E,$4E,$0E,$F3,$0D,$9E,$0D,$4D
        .byte   $0D,$01,$0D,$B9,$0C,$75,$0C,$35
        .byte   $0C,$F8,$0B,$BF,$0B,$89,$0B,$57
        .byte   $0B,$27,$0B,$F9,$0A,$CF,$0A,$A6
        .byte   $0A,$80,$0A,$5C,$0A,$3A,$0A,$1A
        .byte   $0A,$FC,$09,$DF,$09,$C4,$09,$AB
        .byte   $09,$93,$09,$7C,$09,$67,$09,$52
        .byte   $09,$3F,$09,$2D,$09,$1C,$09,$0C
        .byte   $09,$FD,$08,$EE,$08,$E1,$08,$D4
        .byte   $08,$C8,$08,$BD,$08,$B2,$08,$A8
        .byte   $08,$9F,$08,$96,$08,$8D,$08,$85
        .byte   $08,$7E,$08,$76,$08,$70,$08,$69
        .byte   $08,$63,$08,$5E,$08,$58,$08,$53
        .byte   $08,$4F,$08,$4A,$08,$46,$08,$42
        .byte   $08,$3E,$08,$3A,$08,$37,$08,$34
        .byte   $08,$31,$08,$2E,$08,$2B,$08,$29
        .byte   $08,$27,$08,$24,$08,$22,$08,$20
        .byte   $08,$1E,$08,$1C,$08,$1B,$08
L8096:  .byte   $01,$02,$03,$04,$06,$08,$0C,$10
        .byte   $18,$20
L80A0:  .byte   $12,$16,$00,$04,$08,$0A,$0E
L80A7:  .byte   $00,$18,$30,$48,$60,$78
L80AD:  lda     #$4C
        sta     $ED
        lda     #$0F
        sta     $EE
        lda     #$88
        sta     $EF
        jsr     LAA82
        jsr     L8131
L80BF:  jsr     LADA9
        jsr     LAF88
        cmp     #$31
        beq     L80F0
        cmp     #$32
        beq     L80D4
        cmp     #$33
        bne     L80BF
        jmp     L7D04

L80D4:  jmp     LB5AF

        .byte   $A9,$5A,$8D,$3A,$06,$A9,$33,$8D
        .byte   $3B,$06,$85,$00,$A5,$07,$8D,$3C
        .byte   $06,$A5,$08,$8D,$3D,$06,$4C,$AD
        .byte   $80
L80F0:  jsr     LAF88
        bne     L80F0
        lda     $00
        cmp     #$59
        beq     L8121
        lda     $063A
        cmp     #$5A
        bne     L8127
        lda     $063B
        cmp     #$33
        bne     L8127
        lda     $063C
        sta     $07
        lda     $063D
        sta     $08
        lda     #$10
        sta     $05
        lda     #$C4
        sta     $06
        jsr     L9228
        jsr     L814B
L8121:  jsr     L852E
        jmp     L820B

L8127:  jsr     L814B
        lda     #$59
        sta     $00
        jmp     L81FF

L8131:  jsr     LAF88
        bne     L8131
        jsr     L819C
        lda     #$00
        sta     $44
L813D:  lda     $44
        cmp     #$05
        beq     L814A
        jsr     L8154
        inc     $44
        bne     L813D
L814A:  rts

L814B:  lda     $9E
        cmp     #$81
        beq     L814A
        jmp     LAB71

L8154:  asl     a
        asl     a
        tax
        lda     LC391,x
        sta     $48
        lda     LC392,x
        sta     $49
        lda     LC393,x
        sta     $52
        lda     LC394,x
        sta     $53
        jmp     L87F8

        .byte   $31,$2D,$2D,$42,$41,$53,$49,$43
        .byte   $00,$32,$2D,$2D,$42,$47,$20,$47
        .byte   $52,$41,$50,$48,$49,$43,$00,$33
        .byte   $2D,$2D,$44,$4F,$53,$00,$31,$2C
        .byte   $32,$2C,$33,$20,$4B,$45,$59,$20
        .byte   $49,$4E,$20,$21,$21,$00
L819C:  jsr     LABF1
        lda     #$DE
        sta     $AD
        lda     #$20
        ldx     #$C5
        jsr     L81D3
        lda     #$E0
        sta     $AD
        lda     #$22
        ldx     #$E5
        jsr     L81D3
        lda     $32
        ora     #$04
        sta     PPUCTRL
        lda     #$20
        ldx     #$E5
        jsr     L81EA
        lda     #$20
        ldx     #$FA
        jsr     L81EA
        jsr     LABE6
        jsr     LB3F1
        jmp     LABFA

L81D3:  jsr     L81F8
        lda     $AD
        sta     PPUDATA
        lda     #$E3
        ldy     #$14
        jsr     L81F1
        inc     $AD
        lda     $AD
        sta     PPUDATA
        rts

L81EA:  jsr     L81F8
        lda     #$E2
        ldy     #$10
L81F1:  sta     PPUDATA
        dey
        bne     L81F1
        rts

L81F8:  sta     PPUADDR
        stx     PPUADDR
        rts

L81FF:  ldx     #$FF
        txs
        jsr     L852E
        jsr     L8388
        jsr     LAA09
L820B:  jsr     L873B
        sta     $063A
        sta     $063B
        ldx     #$FF
        txs
        txa
        pha
        pha
        jsr     LADA1
L821D:  tsx
        stx     $17
        cli
L8221:  ldx     $17
        txs
        jsr     LAC03
        jsr     LA9FF
        lda     $24
        beq     L8233
        lda     #$2E
        jsr     LAC0D
L8233:  jsr     LAC03
L8236:  jsr     LB343
        jsr     L8A32
        jsr     L8CC7
        cmp     #$03
        beq     L8236
        jsr     L8CD4
        and     #$FF
        beq     L8236
        jsr     L8CDD
        bcs     L825F
        jsr     L83B1
        jsr     L9228
        jmp     L8236

L8258:  lda     #$FF
        sta     $0B
        sta     $0C
        rts

L825F:  jsr     L8E59
        lda     #$44
        sta     $0D
        lda     #$B1
        sta     $0E
        lda     #$00
        sta     $09
        lda     #$03
        sta     $0A
        jsr     L8258
L8275:  jsr     L86D4
        jsr     L8A5B
        jsr     L8CD4
        pha
        lda     $09
        sta     $58
        lda     $0A
        sta     $59
        jsr     L8CA1
        pla
        beq     L82DA
        cmp     #$3A
        beq     L8275
        cmp     #$27
        beq     L82DA
        cmp     #$80
        bcs     L82DD
        jsr     LA1C3
        bcs     L82D7
        pha
        jsr     L8CBB
        pla
        jsr     L9F75
        lda     $90
        pha
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     L85A2
        jsr     L9C82
        pla
        sta     $8D
        pla
        sta     $8C
        pla
        sta     $90
        ldy     #$00
        and     #$0F
        cmp     #$03
        beq     L82E3
        jsr     LA3EF
        lda     $0400
        sta     ($8C),y
        lda     $0401
        iny
        sta     ($8C),y
        jmp     L8275

L82D7:  jmp     L836D

L82DA:  jmp     L8345

L82DD:  jmp     L8321

L82E0:  jmp     L8370

L82E3:  jsr     LA3F2
        jsr     L82EF
        jmp     L8275

L82EC:  sta     ($8C),y
        rts

L82EF:  ldy     #$00
        lda     $0400
        beq     L82EC
        cmp     #$20
        bcs     L82E0
        sta     ($8C),y
        lda     $0401
        sta     L0019
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
L8318:  lda     (L0019),y
        sta     ($8C),y
        iny
        dex
        bne     L8318
        rts

L8321:  sec
        sbc     #$80
        cmp     #$44
        bcs     L836D
        asl     a
        tax
        lda     LC2C6,x
        sta     L0019
        lda     LC2C7,x
        sta     $1A
        tsx
        stx     $17
        jsr     L8378
        jsr     L9D4B
        jmp     L8275

L8340:  jmp     L821D

        .byte   $68,$68
L8345:  lda     $0D
        sta     $09
        lda     $0E
        sta     $0A
        jsr     L8CC7
        beq     L8340
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
        jsr     L9927
        jmp     L8275

L836D:  lda     #$01
        .byte   $AE
L8370:  lda     #$04
        jmp     LA98F

        .byte   $20,$A1,$8C
L8378:  jsr     L8CD4
        and     #$FF
        beq     L8381
        cmp     #$3A
L8381:  rts

        .byte   $20,$88,$83,$4C,$14,$82
L8388:  jsr     L873B
        sta     LC410
        sta     LC411
        sta     LC412
        ldx     #$10
        stx     $05
        inx
        stx     $07
        inx
        stx     $1D
        inx
        stx     $1F
L83A1:  inx
        stx     $21
        lda     #$C4
        sta     $06
        sta     $08
        sta     $1E
        sta     $20
        sta     $22
        rts

L83B1:  jsr     L873B
        jsr     L8258
        jsr     L8D09
        lda     #$FF
        cmp     L0028
        bne     L83C6
        cmp     $29
        bne     L83C6
        beq     L8370
L83C6:  jsr     L8CC7
        cmp     #$20
        bne     L83D0
        jsr     L8CAD
L83D0:  and     #$FF
        beq     L8401
        lda     L0028
        sta     $44
        lda     $29
        sta     $45
        jsr     L8E59
        sec
        lda     $4D
        sbc     #$00
        sta     L0028
        lda     $4E
        sbc     #$03
        sta     $29
        lda     #$02
        clc
        adc     L0028
        sta     $5A
        lda     $44
        sta     $0F
        lda     $45
        sta     $10
        jsr     L8448
        jmp     L848A

L8401:  jmp     L8440

L8404:  lda     $05
        sta     $5B
        lda     $06
        sta     $5C
L840C:  ldy     #$00
        lda     ($5B),y
        bne     L8415
        lda     #$FF
L8414:  rts

L8415:  iny
        lda     ($5B),y
        sta     $2C
        iny
        lda     ($5B),y
        sta     $2D
        lda     $44
        sta     L0028
        lda     $45
        sta     $29
        jsr     L8BF7
        beq     L8414
        bcc     L8414
        ldy     #$00
        clc
        lda     ($5B),y
        adc     $5B
        sta     $5B
        lda     $5C
        adc     #$00
        sta     $5C
        jmp     L840C

L8440:  lda     L0028
        sta     $44
        lda     $29
        sta     $45
L8448:  jsr     L8404
        bne     L8414
        ldy     #$00
        lda     ($5B),y
        sta     $3C
        clc
        adc     $5B
        sta     $0B
        lda     $5C
        adc     #$00
        sta     $0C
L845E:  ldy     #$00
        lda     ($0B),y
        sta     ($5B),y
        inc     $0B
        bne     L846A
        inc     $0C
L846A:  inc     $5B
        bne     L8470
        inc     $5C
L8470:  lda     $5C
        cmp     $08
        bne     L845E
        lda     $5B
        cmp     $07
        bne     L845E
        sec
        lda     $07
        sbc     $3C
        sta     $07
        lda     $08
        sbc     #$00
        sta     $08
        rts

L848A:  lda     $07
        clc
        adc     $5A
        sta     L0028
        lda     $08
        adc     #$00
        sta     $29
        jsr     L8A44
        jsr     L8404
        lda     $07
        sta     $0B
        clc
        adc     $5A
        sta     L0019
        lda     $08
        sta     $0C
        adc     #$00
        sta     $1A
L84AE:  ldy     #$00
        lda     ($0B),y
        sta     (L0019),y
        jsr     L8516
        jsr     L8522
        lda     $5B
        cmp     $0B
        bne     L84AE
        lda     $5C
        cmp     $0C
        bne     L84AE
        lda     ($0B),y
        sta     (L0019),y
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
L84F8:  lda     ($4D),y
        sta     ($5B),y
        iny
        dex
        bne     L84F8
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

L8516:  lda     $0B
        beq     L851D
        dec     $0B
        rts

L851D:  dec     $0B
        dec     $0C
        rts

L8522:  lda     L0019
        beq     L8529
        dec     L0019
        rts

L8529:  dec     L0019
        dec     $1A
        rts

L852E:  lda     #$04
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
        jsr     L873B
        sta     $4C
        sta     $57
        sta     $97
        lda     #$FF
        sta     $03
        lda     #$03
        sta     $56
        lda     #$FE
        sta     $04
        jsr     LA6DF
        jsr     LA96E
        jmp     LAA82

        .byte   $20,$28,$92,$20,$3B,$87,$20,$6E
        .byte   $A9,$20,$DF,$A6,$20,$78,$83,$D0
        .byte   $12,$A2,$FF,$9A,$A9,$FF,$48,$48
        .byte   $A5,$05,$85,$09,$A5,$06,$85,$0A
        .byte   $4C,$4D,$83,$4C,$46,$91
L85A2:  jsr     L8CD4
        cmp     #$F6
        bne     L85AC
        jmp     L8CA1

L85AC:  jmp     L836D

        .byte   $08,$A2,$00,$86,$86,$86,$87,$CA
        .byte   $86,$88,$86,$89,$28,$F0,$46,$C9
        .byte   $2C,$F0,$23,$C9,$2D,$F0,$1F,$C9
        .byte   $0B,$D0,$E2,$20,$AD,$8C,$85,$86
        .byte   $85,$88,$20,$AD,$8C,$85,$87,$85
        .byte   $89,$20,$75,$83,$F0,$27,$C9,$2C
        .byte   $F0,$04,$C9,$2D,$D0,$C7,$20,$75
        .byte   $83,$F0,$14,$C9,$0B,$D0,$BE,$20
        .byte   $AD,$8C,$85,$88,$20,$AD,$8C,$85
        .byte   $89,$20,$A1,$8C,$4C,$04,$86,$A9
        .byte   $FF,$85,$88,$85,$89,$20,$F8,$8F
        .byte   $A5,$86,$85,$44,$A5,$87,$85,$45
        .byte   $20,$04,$84,$A0,$00,$B1,$5B,$F0
        .byte   $44,$A5,$5B,$85,$09,$A5,$5C,$85
        .byte   $0A,$A0,$00,$B1,$09,$F0,$36,$C8
        .byte   $B1,$09,$85,$28,$C8,$B1,$09,$85
        .byte   $29,$A5,$88,$85,$2C,$A5,$89,$85
        .byte   $2D,$20,$F7,$8B,$F0,$02,$B0,$1D
        .byte   $20,$6D,$86,$20,$56,$86,$F0,$D9
        .byte   $C9,$1B,$D0,$D5,$20,$56,$86,$F0
        .byte   $FB,$C9,$1B,$F0,$F7,$D0,$CA,$20
        .byte   $D4,$86,$4C,$88,$AF,$4C,$01,$90
L865F:  lda     #$3D
        clc
        adc     $46
        sta     $52
        lda     #$00
        sta     $53
        jmp     L87B7

        .byte   $A9,$00,$85,$62,$A0,$01,$B1,$09
        .byte   $85,$28,$C8,$B1,$09,$85,$29,$20
        .byte   $3C,$8D,$20,$EB,$8C,$20,$5F,$86
        .byte   $A9,$20,$20,$F0,$87,$A9,$03,$18
        .byte   $65,$09,$85,$4D,$A9,$00,$65,$0A
        .byte   $85,$4E,$20,$0A,$90,$20,$3B,$8A
        .byte   $20,$F8,$87,$A5,$4D,$85,$09,$A5
        .byte   $4E,$85,$0A,$20,$07,$AC,$A9,$01
        .byte   $18,$65,$4D,$85,$09,$A9,$00,$65
        .byte   $4E,$85,$0A,$60
L86B9:  jsr     LAF88
        cmp     #$03
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        rts

L86D4:  jsr     L86B9
        beq     L86DC
        rts

        .byte   $68,$68
L86DC:  lda     #$00
        sta     $63
        jsr     LAF88
        bne     L86DC
        jsr     LB3F1
        lda     $0B
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
        lda     #$FF
        sta     $24
        cmp     $0B
        bne     L870F
        cmp     $0C
        bne     L870F
        jsr     L873B
L870F:  jmp     LA9EA

        .byte   $A9,$0E,$4C,$8F,$A9,$A5,$24,$F0
        .byte   $F7,$A5,$15,$85,$09,$A5,$16,$85
        .byte   $0A,$20,$08,$99,$20,$78,$83,$D0
        .byte   $F8,$A5,$13,$85,$0D,$A5,$14,$85
        .byte   $0E,$A5,$11,$85,$0B,$A5,$12,$85
        .byte   $0C
L873B:  lda     #$00
        sta     $24
        rts

        .byte   $F0,$72,$20,$82,$9C,$A5,$27,$C9
        .byte   $03,$F0,$16,$AD,$00,$04,$85,$28
        .byte   $AD,$01,$04,$85,$29,$20,$96,$8D
        .byte   $20,$5F,$86,$20,$FB,$87,$4C,$94
        .byte   $87,$AD,$00,$04,$F0,$2E,$A0,$00
        .byte   $AD,$01,$04,$85,$19,$AD,$02,$04
        .byte   $85,$1A,$B1,$19,$C9,$20,$B0,$02
        .byte   $A9,$00,$99,$00,$05,$C8,$CE,$00
        .byte   $04,$D0,$EF,$A9,$00,$99,$00,$05
        .byte   $20,$3B,$8A,$20,$B7,$87,$4C,$5B
        .byte   $87,$20,$A1,$8C,$20,$78,$83,$F0
        .byte   $1B,$C9,$3B,$F0,$09,$C9,$2C,$D0
        .byte   $0D,$A9,$09,$20,$0D,$AC,$20,$75
        .byte   $83,$F0,$08,$4C,$94,$87,$20,$78
        .byte   $83,$D0,$8D,$60,$4C,$07,$AC
L87B7:  ldy     #$00
        ldx     $62
L87BB:  lda     ($52),y
        beq     L87CF
        cmp     #$20
        bcc     L87D2
        sta     $0700,x
        iny
        inx
        beq     L87CD
        jmp     L87BB

L87CD:  dex
        dex
L87CF:  stx     $62
        rts

L87D2:  pha
        sty     $47
        cpx     #$00
        beq     L87DE
        stx     $62
        jsr     L87FB
L87DE:  pla
        jsr     LAC0D
        lda     #$00
        sta     $62
        ldy     $47
        iny
        ldx     $62
        jmp     L87BB

L87EE:  lda     #$00
L87F0:  ldx     $62
        sta     $0700,x
        inc     $62
        rts

L87F8:  jsr     L87B7
L87FB:  jsr     L87EE
        lda     $62
        beq     L880E
        lda     #$01
        ldx     #$00
        stx     $64
L8808:  sta     $63
L880A:  lda     $63
        bne     L880A
L880E:  rts

        pha
        lda     #$00
        sta     OAMADDR
        lda     #$02
        sta     $4014
        lda     $AB
        bne     L882C
        lda     $AC
        beq     L8826
        lda     #$FF
        sta     $AB
L8826:  lda     $63
        bne     L882F
        pla
        rti

L882C:  jmp     LBE33

L882F:  tax
        dex
        txa
        asl     a
        tax
        lda     L8841,x
        sta     L001B
        lda     L8841+1,x
        sta     $1C
        jmp     (L001B)

L8841:  .addr   L88AA
        .addr   L8925
        .addr   L8961
        .addr   L8898
        .addr   L8870
        .addr   L8990
        .addr   L89BE
        .addr   L8882
        .addr   L8890
        .addr   L9A90
        .addr   L8862
        .addr   L885F
        .addr   L89CC
        .addr   L8A02
        .addr   L8A1A
L885F:  lda     #$00
L8862           := * + 1
        ldx     $33A5
        sta     PPUMASK
L8867:  lda     #$00
        sta     $63
        jsr     LABE6
        pla
        rti

L8870:  jsr     L8876
        jmp     L8867

L8876:  jsr     LABD8
        lda     PPUDATA
        lda     PPUDATA
        sta     $73
        rts

L8882:  jsr     L8876
        jsr     LABD8
        lda     $61
L888A:  sta     PPUDATA
        jmp     L8867

L8890:  jsr     LABD8
        lda     $73
        jmp     L888A

L8898:  jsr     LABD8
        ldx     $48
        lda     #$20
L889F:  sta     PPUDATA
        inx
        cpx     #$1C
        bne     L889F
        jmp     L8958

L88AA:  jsr     LABA6
        ldx     $64
        jsr     LABD8
L88B2:  lda     $0700,x
        beq     L891D
        sta     PPUDATA
        inc     $48
        lda     #$1C
        cmp     $48
        beq     L88D1
        lda     #$0E
        cmp     $48
        beq     L88CB
        inx
        bne     L88B2
L88CB:  inx
        stx     $64
        jmp     L895C

L88D1:  lda     #$00
        sta     $48
        txa
        pha
        ldx     $49
        lda     #$FF
        sta     $0601,x
        pla
        tax
        inc     $49
        lda     #$18
        cmp     $49
        beq     L88EE
        inx
        stx     $64
        jmp     L895C

L88EE:  inx
        stx     $64
        jsr     LAD8B
        jsr     LABE6
        lda     $32
        sta     PPUCTRL
        jsr     LABA6
        jsr     LAAFE
        jsr     LABD8
        ldx     #$1C
        lda     #$20
L8909:  sta     PPUDATA
        dex
        bne     L8909
        stx     $63
        jsr     LABE6
        jsr     LAC6A
        lda     #$01
        sta     $63
        pla
        rti

L891D:  sta     $63
        sta     $62
        sta     $64
        beq     L895C
L8925:  lda     #$00
        sta     $48
        jsr     LABA6
        jsr     LABD8
        ldy     #$1C
        lda     PPUDATA
        ldx     $76
L8936:  lda     PPUDATA
        sta     $0500,x
        inx
        cpx     #$FF
        beq     L8951
        dey
        bne     L8936
        stx     $76
        ldx     $49
        lda     $0601,x
        beq     L8951
        inc     $49
        bne     L895C
L8951:  lda     #$FF
        ldx     a:$A9
        sta     $64
L8958:  lda     #$00
L895A:  sta     $63
L895C:  jsr     LABE6
        pla
        rti

L8961:  jsr     LABD8
        lda     PPUDATA
        ldx     #$00
        ldy     $48
L896B:  lda     PPUDATA
        sta     $0500,x
        inx
        iny
        cpy     #$1C
        bne     L896B
        lda     $1A
        ldx     L0019
        jsr     L81F8
        ldx     #$00
        ldy     $48
L8982:  lda     $0500,x
        sta     PPUDATA
        inx
        iny
        cpy     #$1C
        bne     L8982
        beq     L8958
L8990:  lda     #$3F
        sta     PPUADDR
        lda     $0700
        asl     a
        asl     a
L899A:  sta     PPUADDR
        lda     $0701
        sta     PPUDATA
        lda     $0702
        sta     PPUDATA
        lda     $0703
        sta     PPUDATA
        lda     $0704
        sta     PPUDATA
        jsr     LABE6
        jsr     LB3F1
        jmp     L8867

L89BE:  lda     #$3F
        sta     PPUADDR
        lda     $0700
        asl     a
        asl     a
        ora     #$10
        bne     L899A
L89CC:  jsr     LABD8
        ldy     $48
        ldx     #$01
        lda     $B0
        sta     $0500
        lda     PPUDATA
L89DB:  lda     PPUDATA
        sta     $0500,x
        inx
        iny
        cpy     #$1C
        bne     L89DB
        sta     $B0
        jsr     LABD8
        ldx     #$00
        ldy     $48
L89F0:  lda     $0500,x
        sta     PPUDATA
        inx
        iny
        cpy     #$1C
        bne     L89F0
L89FC:  jsr     LABE6
        jmp     L8867

L8A02:  jsr     LABD8
        ldx     #$00
        lda     PPUDATA
L8A0A:  lda     PPUDATA
        sta     $0500,x
        inx
        cpx     #$1C
        bne     L8A0A
        lda     #$0F
        jmp     L895A

L8A1A:  inc     $49
        jsr     LABA6
        jsr     LABD8
        ldx     #$00
L8A24:  lda     $0500,x
        sta     PPUDATA
        inx
        cpx     #$1C
        bne     L8A24
        jmp     L89FC

L8A32:  lda     #$00
        sta     $09
        lda     #$05
        sta     $0A
        rts

        .byte   $A9,$00,$85,$52,$A9,$05,$85,$53
L8A43:  rts

L8A44:  lda     $03
        sec
        sbc     #$04
        sta     $2C
        lda     $04
        sbc     #$00
        sta     $2D
        jsr     L8BF7
        bcc     L8A43
L8A56:  lda     #$06
        jmp     LA98F

L8A5B:  tsx
        cpx     #$14
        bcc     L8A56
        rts

L8A61:  sec
        lda     #$00
        sbc     $2C
        sta     $2C
        lda     #$00
        sbc     $2D
        sta     $2D
        rts

        .byte   $A9,$02,$85,$27,$20,$EF,$A3,$20
        .byte   $7C,$8A,$20,$EC,$8A
L8A7C:  sec
        lda     #$00
        sbc     L0028
        sta     L0028
        lda     #$00
        sbc     $29
        sta     $29
        rts

L8A8A:  jmp     L8B34

L8A8D:  lda     $2C
        sta     L0028
        lda     $2D
        sta     $29
        lda     $2E
        sta     $2A
L8A99:  rts

L8A9A:  ldx     $21
        stx     $4A
        lda     L0028
        beq     L8A8D
        clc
        ldy     $2C
        beq     L8A99
        adc     $2C
        cmp     #$20
        bcs     L8A8A
        sta     $4B
        lda     $29
        sta     L0019
        lda     $2A
        sta     $1A
        ldy     #$00
L8AB9:  lda     (L0019),y
        sta     $0700,x
        inx
        iny
        dec     L0028
        bne     L8AB9
        lda     $2D
        sta     L0019
        lda     $2E
        sta     $1A
        ldy     #$00
L8ACE:  lda     (L0019),y
        sta     $0700,x
        iny
        inx
        dec     $2C
        bne     L8ACE
        stx     $21
        lda     $4B
        sta     L0028
        lda     $4A
        sta     $29
        lda     #$07
        sta     $2A
        rts

L8AE8:  lda     #$02
        sta     $27
        lda     $27
        cmp     #$03
        beq     L8A9A
        lda     $29
        bmi     L8B1B
        lda     $2D
        bmi     L8B0A
L8AFA:  clc
        lda     $2C
        adc     L0028
        sta     L0028
        lda     $2D
        adc     $29
        sta     $29
        bmi     L8B34
        rts

L8B0A:  jsr     L8A61
L8B0D:  sec
        lda     L0028
        sbc     $2C
        sta     L0028
        lda     $29
        sbc     $2D
        sta     $29
        rts

L8B1B:  lda     $2D
        bmi     L8B28
        jsr     L8A7C
        jsr     L8B0D
        jmp     L8A7C

L8B28:  jsr     L8A7C
        jsr     L8A61
        jsr     L8AFA
        jmp     L8A7C

L8B34:  lda     #$05
        jmp     LA98F

L8B39:  clc
        lda     $2C
        adc     L0028
        sta     L0028
        lda     $2D
        adc     $29
        sta     $29
        rts

L8B47:  lda     #$00
        ldx     #$08
L8B4B:  sta     $34,x
        dex
        bne     L8B4B
        rts

L8B51:  jsr     L8B59
        lda     $36
        bne     L8B34
        rts

L8B59:  jsr     LA3EF
L8B5C:  jsr     L8B47
        jsr     L8B92
        pha
        ldx     #$10
        clc
L8B66:  ror     $29
        ror     L0028
        bcc     L8B79
        clc
        lda     $36
        adc     $2C
        sta     $36
        lda     $37
        adc     $2D
        sta     $37
L8B79:  ror     $37
        ror     $36
        ror     $35
        ror     $34
        dex
        bne     L8B66
        lda     $34
        sta     L0028
        lda     $35
        sta     $29
L8B8C:  pla
        beq     L8BB1
        jmp     L8A7C

L8B92:  lda     #$00
        sta     $34
        lda     $29
        bpl     L8BA1
        jsr     L8A7C
        lda     #$FF
        sta     $34
L8BA1:  lda     $2D
        bpl     L8BAF
        jsr     L8A61
        lda     #$FF
        sec
        sbc     $34
        sta     $34
L8BAF:  lda     $34
L8BB1:  rts

L8BB2:  jsr     LA3EF
        jsr     L8B92
        pha
        lda     #$00
        sta     $2A
        sta     $2B
        ldx     #$10
L8BC1:  asl     L0028
        rol     $29
        rol     $2A
        rol     $2B
        lda     $2A
        sec
        sbc     $2C
        lda     $2B
        sbc     $2D
        bcc     L8BE3
        lda     $2A
        sec
        sbc     $2C
        sta     $2A
        lda     $2B
        sbc     $2D
        sta     $2B
        inc     L0028
L8BE3:  dex
        bne     L8BC1
        jmp     L8B8C

L8BE9:  sec
        lda     L0028
        sbc     $2C
        sta     L0028
        lda     $29
        sbc     $2D
        sta     $29
        rts

L8BF7:  lda     $29
        cmp     $2D
        beq     L8BFE
        rts

L8BFE:  lda     L0028
        cmp     $2C
        rts

L8C03:  lda     $27
        cmp     #$03
        beq     L8C4A
        lda     $29
        bmi     L8C30
        lda     $2D
        bmi     L8C1D
        lda     $29
        cmp     $2D
        beq     L8C21
        bcs     L8C1D
L8C19:  clc
        lda     #$FF
        rts

L8C1D:  sec
        lda     #$FF
        rts

L8C21:  lda     L0028
        cmp     $2C
        beq     L8C2C
        bcs     L8C1D
        jmp     L8C19

L8C2C:  sec
        lda     #$00
        rts

L8C30:  lda     $2D
        bpl     L8C19
        lda     $29
        cmp     $2D
        beq     L8C3F
        bcs     L8C1D
        jmp     L8C19

L8C3F:  lda     L0028
        cmp     $2C
        beq     L8C2C
        bcs     L8C1D
        jmp     L8C19

L8C4A:  lda     $29
        sta     L0019
        lda     $2A
        sta     $1A
        lda     $2D
        sta     L001B
        lda     $2E
        sta     $1C
        lda     L0028
        beq     L8C86
        sta     $4A
        beq     L8C1D
        lda     $2C
        sta     $4B
        ldy     #$00
L8C68:  lda     (L0019),y
        cmp     (L001B),y
        bcc     L8C19
        beq     L8C73
        jmp     L8C1D

L8C73:  iny
        dec     $4A
        beq     L8C7F
        dec     $4B
        beq     L8C1D
        jmp     L8C68

L8C7F:  dec     $4B
        beq     L8C2C
        jmp     L8C19

L8C86:  lda     $2C
        beq     L8C2C
        jmp     L8C19

        .byte   $A0,$00,$91,$09,$E6,$09,$F0,$01
        .byte   $60,$C6,$09,$A9,$00,$91,$09,$60
L8C9D:  ldy     #$00
        lda     ($09),y
L8CA1:  inc     $09
        beq     L8CA8
        and     #$FF
        rts

L8CA8:  inc     $0A
        and     #$FF
        rts

L8CAD:  ldy     #$00
        inc     $09
        beq     L8CB6
        lda     ($09),y
        rts

L8CB6:  inc     $0A
        lda     ($09),y
        rts

L8CBB:  lda     $09
        beq     L8CC2
        dec     $09
        rts

L8CC2:  dec     $09
        dec     $0A
        rts

L8CC7:  ldy     #$00
        lda     ($09),y
        rts

        .byte   $A0,$00,$91,$09,$60
L8CD1:  jsr     L8CA1
L8CD4:  ldy     #$00
        lda     ($09),y
        cmp     #$20
        beq     L8CD1
        rts

L8CDD:  cmp     #$30
        bcc     L8CE7
        cmp     #$3A
        bcs     L8CE7
        clc
        rts

L8CE7:  sec
L8CE8:  rts

L8CE9:  inc     $46
L8CEB:  ldx     $46
        lda     $3D,x
        beq     L8CF6
        cmp     #$30
        beq     L8CE9
        rts

L8CF6:  dec     $46
        rts

        .byte   $20,$D4,$8C,$C9,$2D,$D0,$09,$20
        .byte   $A1,$8C,$20,$F9,$8C,$4C,$7C,$8A
L8D09:  lda     #$02
        sta     $27
        lda     #$00
        sta     L0028
        sta     $29
L8D13:  lda     #$0A
        sta     $2C
        lda     #$00
        sta     $2D
        ldy     #$00
        lda     ($09),y
        jsr     L8CDD
        bcs     L8CE8
        pha
        jsr     L8B51
        pla
        sec
        sbc     #$30
        sta     $2C
        jsr     L8B39
        bcs     L8D39
        jsr     L8CA1
        jmp     L8D13

L8D39:  jmp     L8B34

L8D3C:  lda     #$00
        sta     $46
        lda     $46
        pha
        lda     #$10
        sta     $2C
        lda     #$27
        sta     $2D
        jsr     L8D88
        lda     #$E8
        sta     $2C
        lda     #$03
        sta     $2D
        jsr     L8D88
        lda     #$64
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8D88
        lda     #$0A
        sta     $2C
        jsr     L8D88
        lda     L0028
        clc
        adc     #$30
        jsr     L8D81
        lda     #$00
        jsr     L8D81
        pla
        sta     $46
        rts

L8D7C:  jsr     L8B39
        lda     $47
L8D81:  ldx     $46
        sta     $3D,x
        inc     $46
        rts

L8D88:  lda     #$30
        sta     $47
L8D8C:  jsr     L8BE9
        bcc     L8D7C
        inc     $47
        jmp     L8D8C

        .byte   $A9,$00,$85,$46,$A0,$20,$A5,$29
        .byte   $10,$05,$20,$7C,$8A,$A0,$2D,$98
        .byte   $48,$20,$40,$8D,$20,$EB,$8C,$C6
        .byte   $46,$68,$20,$81,$8D,$C6,$46,$60
        .byte   $C9,$0A,$B0,$03,$69,$30,$60,$18
        .byte   $69,$37,$60,$48,$29,$F0,$4A,$4A
        .byte   $4A,$4A,$20,$B6,$8D,$95,$3D,$E8
        .byte   $68,$29,$0F,$20,$B6,$8D,$95,$3D
        .byte   $E8,$60,$A2,$00,$86,$46,$A5,$29
        .byte   $20,$C1,$8D,$A5,$28,$20,$C1,$8D
        .byte   $A9,$00,$95,$3D
L8DEA:  rts

L8DEB:  lda     #$02
        sta     $27
        ldx     #$00
        lda     #$00
        sta     L0028
        sta     $29
L8DF7:  jsr     L8CC7
        sec
        sbc     #$30
        bcc     L8DEA
        cmp     #$0A
        bcc     L8E0E
        sec
        sbc     #$07
        cmp     #$0A
        bcc     L8DEA
        cmp     #$10
        bcs     L8DEA
L8E0E:  pha
        jsr     L8E22
        pla
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8B39
        jsr     L8CA1
        jmp     L8DF7

L8E22:  txa
        pha
        lda     #$10
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8B51
        pla
        tax
        rts

        .byte   $20,$3E,$8E,$A0,$00,$B1,$4D,$60
        .byte   $A0,$00,$B1,$4D,$E6,$4D,$D0,$11
        .byte   $E6,$4E,$D0,$0D
L8E46:  lda     #$00
        jsr     L8E4B
L8E4B:  ldy     #$00
        sta     ($4D),y
        inc     $4D
        beq     L8E56
        and     #$FF
        rts

L8E56:  jmp     L8B34

L8E59:  lda     #$00
        sta     $4D
        lda     #$03
        sta     $4E
L8E61:  jsr     L8EFA
        and     #$FF
L8E66:  beq     L8E46
        cmp     #$3F
        beq     L8EAD
        cmp     #$22
        beq     L8EB6
        cmp     #$27
        beq     L8EB3
        cmp     #$2E
        beq     L8E86
        cmp     #$26
        beq     L8ED5
        jsr     L8CDD
        bcc     L8ECC
        jsr     L8FAE
        bcc     L8EB0
L8E86:  jsr     L8CA1
        cmp     #$80
        bcs     L8E61
        cmp     #$20
        bcc     L8E61
        cmp     #$40
        bcc     L8EA7
L8E95:  jsr     L8E4B
        ldy     #$00
        lda     ($09),y
        jsr     LA1BB
        bcs     L8E61
        jsr     L8CA1
        jmp     L8E95

L8EA7:  jsr     L8E4B
        jmp     L8E61

L8EAD:  jmp     L8F24

L8EB0:  jmp     L8F2B

L8EB3:  ldy     #$00
L8EB6           := * + 1
        ldx     $22A0
        sty     $99
L8EBA:  jsr     L8E4B
        jsr     L8CA1
L8EC0:  jsr     L8CC7
        beq     L8E66
        cmp     $99
        bne     L8EBA
        jmp     L8E86

L8ECC:  lda     #$12
        pha
        jsr     L8D09
        jmp     L8EE8

L8ED5:  jsr     L8CA1
        jsr     L8CC7
        cmp     #$48
        bne     L8EE2
        jsr     L8CA1
L8EE2:  lda     #$11
        pha
        jsr     L8DEB
L8EE8:  pla
        jsr     L8E4B
        lda     L0028
        jsr     L8E4B
        lda     $29
        jsr     L8E4B
        jmp     L8E61

L8EF9:  rts

L8EFA:  jsr     L8CC7
        cmp     #$20
        bne     L8EF9
        jsr     L8E4B
        jsr     L8CA1
        jmp     L8EFA

L8F0A:  ldy     #$00
        sta     $99
        jmp     L8EC0

L8F11:  jsr     L8EFA
        jsr     L8CDD
        bcc     L8F48
        cmp     #$2C
        beq     L8F65
        cmp     #$2D
        beq     L8F65
L8F21:  jmp     L8E61

L8F24:  lda     #$8B
        sta     $99
        jsr     L8CA1
L8F2B:  lda     $99
        jsr     L8E4B
        cmp     #$95
        beq     L8F0A
        cmp     #$86
        beq     L8F11
        cmp     #$91
        beq     L8F76
        cmp     #$87
        bcs     L8F21
        jsr     L8EFA
        jsr     L8CDD
        bcs     L8F21
L8F48:  jsr     L8D09
        lda     #$0B
        jsr     L8E4B
        lda     L0028
        jsr     L8E4B
        lda     $29
        jsr     L8E4B
        jsr     L8EFA
L8F5D:  cmp     #$2C
        beq     L8F65
        cmp     #$2D
        bne     L8F21
L8F65:  jsr     L8E4B
        jsr     L8CA1
        jsr     L8EFA
        jsr     L8CDD
        bcc     L8F48
        jmp     L8F5D

L8F76:  jsr     L8EFA
        cmp     #$3A
        beq     L8F21
        and     #$FF
L8F7F:  beq     L8F21
        cmp     #$22
        beq     L8F8E
L8F85:  jsr     L8E4B
        jsr     L8CA1
        jmp     L8F76

L8F8E:  jsr     L8E4B
        jsr     L8CA1
        jsr     L8EFA
        cmp     #$22
        beq     L8F85
        and     #$FF
        beq     L8F7F
        jmp     L8F8E

L8FA2:  ldy     #$00
        lda     ($54),y
L8FA6:  rts

L8FA7:  inc     $54
        bne     L8FA6
        inc     $55
        rts

L8FAE:  lda     #$28
        sta     $54
        lda     #$C1
        sta     $55
L8FB6:  jsr     L8FA2
        tax
        inx
        beq     L8FF3
        sta     $99
        jsr     L8FA7
        jsr     L8FF8
L8FC5:  jsr     L8CC7
        sta     $98
        jsr     L8FA2
        cmp     $98
        bne     L8FDE
        jsr     L8CA1
        jsr     L8FA7
        jsr     L8FA2
        bpl     L8FC5
L8FDC:  clc
        rts

L8FDE:  jsr     L8C9D
        cmp     #$2E
        beq     L8FDC
        jsr     L9001
L8FE8:  jsr     L8FA2
        bmi     L8FB6
        jsr     L8FA7
        jmp     L8FE8

L8FF3:  jsr     L8CC7
        sec
        rts

L8FF8:  lda     $09
        sta     $9A
        lda     $0A
        sta     $9B
        rts

L9001:  lda     $9A
        sta     $09
        lda     $9B
        sta     $0A
        rts

        .byte   $20,$32,$8A,$20,$35,$8E,$F0,$17
        .byte   $C9,$20,$90,$18,$C9,$80,$90,$4A
        .byte   $48,$20,$94,$90,$20,$3E,$8E,$68
        .byte   $C9,$95,$F0,$60,$4C,$0D,$90,$A9
        .byte   $00,$4C,$8D,$8C,$48,$20,$32,$8E
        .byte   $85,$28,$20,$32,$8E,$85,$29,$20
        .byte   $3E,$8E,$68,$C9,$11,$D0,$10,$A9
        .byte   $26,$20,$8D,$8C,$A9,$48,$20,$8D
        .byte   $8C,$20,$D8,$8D,$4C,$54,$90,$20
        .byte   $3C,$8D,$20,$EB,$8C,$A6,$46,$B5
        .byte   $3D,$F0,$B0,$20,$8D,$8C,$E8,$4C
        .byte   $59,$90,$C9,$22,$D0,$17,$20,$8E
        .byte   $90,$20,$35,$8E,$C9,$22,$F0,$07
        .byte   $29,$FF,$D0,$F2,$4C,$0D,$90,$20
        .byte   $8E,$90,$4C,$0D,$90,$C9,$27,$D0
        .byte   $F6,$20,$8E,$90,$20,$35,$8E,$D0
        .byte   $F8,$4C,$0D,$90,$20,$3A,$8E,$4C
        .byte   $8D,$8C,$85,$99,$A9,$28,$85,$54
        .byte   $A9,$C1,$85,$55,$20,$A2,$8F,$C9
        .byte   $FF,$F0,$20,$C5,$99,$F0,$0B,$20
        .byte   $A7,$8F,$20,$A2,$8F,$10,$F8,$4C
        .byte   $A1,$90,$20,$A7,$8F,$20,$A2,$8F
        .byte   $30,$09,$20,$8D,$8C,$20,$A7,$8F
        .byte   $4C,$B7,$90,$60,$F0,$5B,$AA,$68
        .byte   $85,$5F,$68,$85,$60,$8A,$20,$72
        .byte   $91,$20,$98,$95,$A5,$09,$48,$A5
        .byte   $0A,$48,$A5,$0B,$48,$A5,$0C,$48
        .byte   $A5,$0D,$48,$A5,$0E,$48,$A9,$00
        .byte   $48,$A9,$FF,$48,$A5,$60,$48,$A5
        .byte   $5F,$48,$4C,$4B,$91,$A9,$02,$4C
        .byte   $8F,$A9,$68,$85,$5F,$68,$85,$60
        .byte   $68,$C9,$FF,$D0,$F0,$68,$D0,$ED
        .byte   $68,$85,$0E,$68,$85,$0D,$68,$85
        .byte   $0C,$68,$85,$0B,$68,$85,$0A,$68
        .byte   $85,$09,$A5,$60,$48,$A5,$5F,$48
        .byte   $60,$4C,$6D,$83,$F0,$D4,$68,$85
        .byte   $5F,$68,$85,$60,$68,$C9,$FF,$D0
        .byte   $C4,$68,$D0,$C1,$BA,$8A,$18,$69
        .byte   $06,$AA,$9A,$A5,$60,$48,$A5,$5F
        .byte   $48,$20,$78,$83,$F0,$DB,$20,$72
        .byte   $91,$A0,$00,$B1,$5B,$18,$65,$5B
        .byte   $85,$0D,$A9,$00,$65,$5C,$85,$0E
        .byte   $C8,$B1,$5B,$85,$0B,$C8,$B1,$5B
        .byte   $85,$0C,$18,$A9,$03,$65,$5B,$85
        .byte   $09,$A9,$00,$65,$5C,$85,$0A,$60
        .byte   $C9,$0B,$D0,$AD,$A0,$01,$B1,$09
        .byte   $85,$44,$C8,$B1,$09,$85,$45,$20
        .byte   $04,$84,$D0,$01,$60,$A9,$07,$4C
        .byte   $8F,$A9,$20,$82,$9C,$6C,$00,$04
        .byte   $4C,$70,$83,$4C,$63,$B2,$F0,$FB
        .byte   $A9,$00,$85,$74,$20,$82,$9C,$20
        .byte   $EF,$A3,$AD,$01,$04,$30,$E9,$D0
        .byte   $05,$AD,$00,$04,$F0,$D6,$EE,$01
        .byte   $04,$20,$D4,$86,$C6,$74,$D0,$F9
        .byte   $CE,$00,$04,$D0,$F4,$AD,$01,$04
        .byte   $F0,$05,$CE,$01,$04,$D0,$EA,$60
        .byte   $20,$6D,$9C,$C9,$1C,$B0,$C1,$85
        .byte   $6B,$20,$6A,$9C,$C9,$18,$B0,$B8
        .byte   $85,$6C,$4C,$AA,$AC,$20,$82,$9C
        .byte   $AD,$00,$04,$85,$25,$AD,$01,$04
        .byte   $85,$26,$A0,$00,$84,$46,$20,$6A
        .byte   $9C,$A4,$46,$91,$25,$E6,$46,$20
        .byte   $78,$83,$D0,$F2,$60,$68,$68,$4C
        .byte   $45,$83,$20,$82,$9C,$AD,$00,$04
        .byte   $0D,$01,$04,$F0,$F0,$20,$78,$83
        .byte   $F0,$61,$C9,$85,$F0,$04,$60,$4C
        .byte   $48,$91,$20,$75,$83,$F0,$BE,$C9
        .byte   $0B,$F0,$F4,$60,$D0,$25
L9228:  lda     $08
        sta     $1E
        ldx     $07
        inx
        bne     L9233
        inc     $1E
L9233:  stx     $1D
        lda     $1E
        sta     $20
        ldx     $1D
        inx
        bne     L9240
        inc     $20
L9240:  stx     $1F
        ldy     #$00
        tya
        sta     ($1D),y
        iny
        sta     ($1D),y
        jmp     L98FA

        .byte   $20,$28,$92,$20,$82,$9C,$20,$EF
        .byte   $A3,$AD,$01,$04,$C9,$78,$B0,$1B
        .byte   $C5,$20,$90,$17,$D0,$07,$AD,$00
        .byte   $04,$C5,$1F,$90,$0E,$AD,$00,$04
        .byte   $85,$03,$AD,$01,$04,$85,$04,$60
        .byte   $4C,$6D,$83,$4C,$70,$83,$4C,$E1
        .byte   $A3,$F0,$F5,$68,$85,$5F,$68,$85
        .byte   $60,$A5,$0D,$48,$A5,$0E,$48,$A5
        .byte   $0B,$48,$A5,$0C,$48,$20,$D4,$8C
        .byte   $20,$75,$9F,$A5,$90,$C9,$02,$D0
        .byte   $DD,$A5,$8C,$48,$A5,$8D,$48,$20
        .byte   $A2,$85,$20,$82,$9C,$68,$85,$89
        .byte   $48,$85,$8C,$A0,$00,$AD,$00,$04
        .byte   $91,$8C,$AD,$01,$04,$C8,$91,$8C
        .byte   $A5,$8C,$48,$A5,$8D,$48,$20,$D4
        .byte   $8C,$C9,$88,$D0,$AB,$20,$A1,$8C
        .byte   $20,$82,$9C,$AD,$00,$04,$48,$AD
        .byte   $01,$04,$48,$A9,$01,$8D,$00,$04
        .byte   $A9,$00,$8D,$01,$04,$20,$78,$83
        .byte   $F0,$0A,$C9,$89,$D0,$8A,$20,$A1
        .byte   $8C,$20,$82,$9C,$AD,$00,$04,$48
        .byte   $AD,$01,$04,$48,$A5,$09,$48,$A5
        .byte   $0A,$48,$A9,$00,$48,$A9,$FD,$48
        .byte   $4C,$1C,$91,$A9,$00,$4C,$8F,$A9
        .byte   $68,$85,$5F,$68,$85,$60,$68,$C9
        .byte   $FD,$D0,$F0,$68,$D0,$ED,$68,$85
        .byte   $89,$68,$85,$88,$68,$85,$83,$68
        .byte   $85,$82,$68,$85,$81,$68,$85,$80
        .byte   $68,$85,$7D,$68,$85,$7C,$68,$85
        .byte   $87,$68,$85,$86,$68,$85,$85,$68
        .byte   $85,$84,$A0,$00,$B1,$7C,$85,$28
        .byte   $C8,$B1,$7C,$85,$29,$20,$8A,$93
        .byte   $F0,$02,$90,$36,$A5,$84,$48,$85
        .byte   $0D,$A5,$85,$48,$85,$0E,$A5,$86
        .byte   $48,$85,$0B,$A5,$87,$48,$85,$0C
        .byte   $A5,$7C,$48,$A5,$7D,$48,$A5,$80
        .byte   $48,$A5,$81,$48,$A5,$82,$48,$A5
        .byte   $83,$48,$A5,$88,$48,$85,$09,$A5
        .byte   $89,$48,$85,$0A,$A9,$00,$48,$A9
        .byte   $FD,$48,$4C,$1C,$91,$A5,$83,$30
        .byte   $0C,$20,$A0,$93,$20,$03,$8C,$B0
        .byte   $02,$38,$60,$18,$60,$20,$A0,$93
        .byte   $4C,$03,$8C,$A5,$82,$85,$2C,$A5
        .byte   $83,$85,$2D,$20,$E8,$8A,$A0,$00
        .byte   $A5,$28,$91,$7C,$C8,$A5,$29,$91
        .byte   $7C,$A5,$80,$85,$2C,$A5,$81,$85
        .byte   $2D,$60,$4C,$6D,$83,$F0,$FB,$C9
        .byte   $22,$D0,$0B,$20,$DB,$94,$C9,$2C
        .byte   $F0,$09,$C9,$3B,$D0,$EC,$A9,$3F
        .byte   $20,$0D,$AC,$A5,$49,$85,$7E,$A5
        .byte   $48,$85,$80,$20,$43,$B3,$20,$D4
        .byte   $86,$A9,$00,$85,$52,$A5,$7E,$C5
        .byte   $47,$F0,$0B,$C6,$7E,$18,$A9,$1C
        .byte   $65,$52,$85,$52,$D0,$EF,$A5,$80
        .byte   $18,$65,$52,$85,$52,$20,$D4,$8C
        .byte   $20,$75,$9F,$20,$F8,$8F,$A5,$52
        .byte   $85,$09,$A9,$05,$85,$0A,$A5,$90
        .byte   $29,$0F,$C9,$03,$F0,$40,$20,$FE
        .byte   $A4,$A5,$09,$85,$52,$A0,$00,$A5
        .byte   $28,$91,$8C,$C8,$A5,$29,$91,$8C
        .byte   $20,$01,$90,$20,$D4,$8C,$C9,$2C
        .byte   $D0,$87,$20,$A1,$8C,$20,$F8,$8F
        .byte   $A5,$52,$85,$09,$A9,$05,$85,$0A
        .byte   $A0,$00,$B1,$09,$C9,$2C,$D0,$08
        .byte   $E6,$52,$20,$01,$90,$4C,$02,$94
        .byte   $20,$01,$90,$4C,$D3,$93,$20,$D4
        .byte   $8C,$A9,$00,$81,$5B,$A0,$00,$A2
        .byte   $00,$B1,$09,$F0,$10,$C9,$2C,$F0
        .byte   $0C,$9D,$00,$07,$E6,$1B,$E8,$20
        .byte   $A1,$8C,$4C,$69,$94,$A5,$1B,$8D
        .byte   $00,$04,$A9,$00,$8D,$01,$04,$A9
        .byte   $07,$8D,$02,$04,$20,$EF,$82,$A5
        .byte   $09,$85,$52,$4C,$2D,$94,$4C,$6D
        .byte   $83,$F0,$FB,$20,$75,$9F,$A5,$90
        .byte   $29,$0F,$48,$A5,$8C,$48,$A5,$8D
        .byte   $48,$20,$94,$A5,$20,$D4,$8C,$20
        .byte   $75,$9F,$68,$85,$1A,$68,$85,$19
        .byte   $A5,$90,$29,$0F,$85,$90,$68,$C5
        .byte   $90,$D0,$18,$AA,$C9,$02,$F0,$02
        .byte   $A2,$21,$A0,$00,$B1,$8C,$48,$B1
        .byte   $19,$91,$8C,$68,$91,$19,$C8,$CA
        .byte   $D0,$F2,$60,$4C,$E1,$A3,$20,$A1
        .byte   $8C,$A0,$00,$B1,$09,$F0,$AF,$C9
        .byte   $22,$F0,$06,$20,$0D,$AC,$4C,$DB
        .byte   $94,$20,$75,$83,$F0,$A0,$4C,$A1
        .byte   $8C,$F0,$9B,$C9,$22,$D0,$0B,$20
        .byte   $DB,$94,$C9,$2C,$F0,$04,$C9,$3B
        .byte   $D0,$8C,$20,$43,$B3,$20,$D4,$86
        .byte   $20,$D4,$8C,$20,$75,$9F,$A5,$90
        .byte   $29,$0F,$C9,$03,$D0,$BD,$A2,$00
        .byte   $A0,$00,$BD,$00,$05,$C9,$20,$D0
        .byte   $07,$E8,$BD,$00,$05,$F0,$0E,$CA
        .byte   $BD,$00,$05,$F0,$08,$9D,$00,$07
        .byte   $E8,$C8,$4C,$2D,$95,$8C,$00,$04
        .byte   $A9,$00,$8D,$01,$04,$A9,$05,$8D
        .byte   $02,$04,$4C,$EF,$82,$4C,$6D,$83
        .byte   $20,$6D,$9C,$20,$D4,$8C,$48,$20
        .byte   $A1,$8C,$AE,$00,$04,$F0,$34,$20
        .byte   $78,$83,$F0,$32,$C9,$0B,$D0,$E5
        .byte   $CA,$F0,$0E,$20,$27,$99,$20,$78
        .byte   $83,$F0,$23,$20,$94,$A5,$4C,$5C
        .byte   $95,$68,$38,$E9,$80,$C9,$05,$B0
        .byte   $CC,$0A,$AA,$BD,$C6,$C2,$85,$19
        .byte   $BD,$C7,$C2,$85,$1A,$20,$D4,$8C
        .byte   $6C,$19,$00,$20,$98,$95,$68,$60
        .byte   $20,$27,$99,$20,$78,$83,$F0,$F7
        .byte   $C9,$0B,$F0,$F4,$20,$94,$A5,$4C
        .byte   $98,$95,$4C,$70,$83,$C9,$86,$F0
        .byte   $45,$20,$6D,$9C,$F0,$F4,$48,$20
        .byte   $94,$A5,$20,$7C,$9C,$68,$38,$E9
        .byte   $01,$C9,$09,$B0,$E5,$0A,$0A,$0A
        .byte   $EA,$AA,$AD,$00,$04,$F0,$21,$A0
        .byte   $00,$AD,$01,$04,$85,$19,$AD,$02
        .byte   $04,$85,$1A,$A9,$0F,$85,$90,$B1
        .byte   $19,$9D,$80,$04,$E8,$C8,$CE,$00
        .byte   $04,$F0,$05,$C6,$90,$D0,$F0,$60
        .byte   $A9,$00,$9D,$80,$04,$60,$20,$A1
        .byte   $8C,$A9,$30,$85,$90,$A9,$00,$85
        .byte   $74,$A9,$46,$20,$F0,$87,$E6,$90
        .byte   $A5,$90,$20,$F0,$87,$A9,$20,$20
        .byte   $F0,$87,$A5,$74,$0A,$0A,$0A,$EA
        .byte   $A8,$B9,$80,$04,$F0,$21,$C9,$20
        .byte   $90,$07,$20,$F0,$87,$C8,$4C,$16
        .byte   $96,$48,$A9,$28,$20,$F0,$87,$68
        .byte   $18,$69,$40,$20,$F0,$87,$A9,$29
        .byte   $20,$F0,$87,$C8,$4C,$16,$96,$20
        .byte   $FB,$87,$20,$07,$AC,$E6,$74,$A5
        .byte   $74,$C9,$08,$90,$B4,$60,$4C,$6D
        .byte   $83,$20,$C3,$A1,$B0,$F8,$85,$7C
        .byte   $A9,$02,$85,$84,$A9,$00,$85,$7D
        .byte   $20,$AD,$8C,$20,$BB,$A1,$B0,$05
        .byte   $85,$7D,$20,$A1,$8C,$20,$D4,$8C
        .byte   $C9,$24,$D0,$05,$E6,$84,$20,$75
        .byte   $83,$C9,$28,$D0,$D1,$A9,$00,$85
        .byte   $82,$20,$64,$9C,$85,$7E,$20,$D4
        .byte   $8C,$C9,$29,$F0,$0E,$E6,$82,$20
        .byte   $6A,$9C,$85,$7F,$20,$78,$83,$C9
        .byte   $29,$D0,$B3,$20,$A1,$8C,$20,$B2
        .byte   $96,$20,$78,$83,$F0,$09,$20,$94
        .byte   $A5,$20,$D4,$8C,$4C,$4E,$96,$60
        .byte   $A9,$09,$4C,$8F,$A9,$20,$78,$97
        .byte   $B0,$F6,$A9,$01,$85,$2C,$20,$CD
        .byte   $A1,$A5,$7E,$85,$28,$E6,$28,$D0
        .byte   $02,$E6,$29,$A5,$82,$F0,$0A,$A5
        .byte   $7F,$85,$2C,$E6,$2C,$D0,$02,$E6
        .byte   $2D,$20,$5C,$8B,$20,$03,$A1,$85
        .byte   $2C,$A9,$00,$85,$2D,$20,$5C,$8B
        .byte   $A9,$05,$85,$2C,$A5,$82,$F0,$02
        .byte   $E6,$2C,$20,$39,$8B,$B0,$1E,$A5
        .byte   $8C,$85,$19,$85,$2C,$A5,$8D,$85
        .byte   $1A,$85,$2D,$20,$39,$8B,$B0,$0D
        .byte   $A5,$04,$85,$2D,$A5,$03,$85,$2C
        .byte   $20,$F7,$8B,$90,$03,$4C,$56,$8A
        .byte   $A0,$00,$A5,$90,$91,$8C,$C8,$A5
        .byte   $8A,$91,$8C,$C8,$A5,$8B,$91,$8C
        .byte   $C8,$A5,$82,$91,$8C,$C8,$A5,$7E
        .byte   $91,$8C,$A5,$82,$F0,$05,$C8,$A5
        .byte   $7F,$91,$8C,$A5,$28,$85,$1F,$A5
        .byte   $29,$85,$20,$18,$A2,$05,$A5,$82
        .byte   $F0,$01,$E8,$8A,$65,$19,$85,$19
        .byte   $A9,$00,$65,$1A,$85,$1A,$A0,$00
        .byte   $A9,$00,$91,$19,$E6,$19,$D0,$02
        .byte   $E6,$1A,$A5,$1A,$C5,$20,$D0,$EE
        .byte   $A5,$19,$C5,$1F,$D0,$E8,$A0,$00
        .byte   $A9,$00,$91,$19,$C8,$91,$19,$C8
        .byte   $91,$19,$60,$A5,$7C,$85,$8A,$A5
        .byte   $7D,$85,$8B,$A5,$84,$09,$80,$85
        .byte   $90,$20,$0E,$A1,$4C,$07,$A0,$20
        .byte   $69,$98,$A5,$B5,$D0,$05,$A9,$11
        .byte   $4C,$8F,$A9,$A9,$46,$85,$52,$A9
        .byte   $AA,$85,$53,$20,$F8,$87,$20,$EB
        .byte   $B3,$A9,$0C,$A2,$05,$A0,$80,$20
        .byte   $0C,$58,$A2,$05,$A0,$80,$8C,$06
        .byte   $7B,$8E,$07,$7B,$A9,$08,$20,$33
        .byte   $7B,$4C,$3D,$7B,$EA,$EA
L97C3:  nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        beq     L97E8
L97D0:  lda     #$4F
        sta     $52
        lda     #$AA
        sta     $53
        jsr     L87F8
        beq     L97E8
L97DD:  lda     #$58
        sta     $52
        lda     #$AA
        sta     $53
        jsr     L87F8
L97E8:  lda     #$01
        sta     $52
        lda     #$05
        sta     $53
        jsr     L87F8
        jmp     LAC07

        .byte   $20,$DD,$97,$4C,$0C,$98,$A2,$00
        .byte   $86,$9F,$C9,$8B,$D0,$05,$C6,$9F
        .byte   $20,$A1,$8C,$20,$69,$98,$A5,$B5
        .byte   $D0,$05,$A9,$01,$4C,$8F,$A9,$A9
        .byte   $4F,$85,$52,$A9,$AA,$85,$53,$20
        .byte   $F8,$87,$20,$88,$83,$4C,$2E,$7F
        .byte   $EA,$EA,$EA,$60,$A5,$05,$8D,$14
        .byte   $05,$A5,$06,$8D,$15,$05,$20,$01
        .byte   $B4,$B0,$2B,$A5,$AD,$85,$07,$A5
        .byte   $AE,$85,$08,$20,$5D,$98,$20,$AB
        .byte   $B3,$A5,$AD,$85,$1D,$A5,$AE,$85
        .byte   $1E,$20,$5D,$98,$20,$AB,$B3,$A5
        .byte   $AD,$85,$1F,$A5,$AE,$85,$20,$A0
        .byte   $00,$A9,$00,$91,$AD,$60,$A9,$11
        .byte   $4C,$8F,$A9
L9869:  jsr     L98A1
        lda     #$00
        sta     $B5
        jsr     L8378
        beq     L98A0
        jsr     L9C7C
        lda     $0401
        sta     L0019
        lda     $0402
        sta     $1A
        lda     $0400
        beq     L98A0
        ldy     #$00
        ldx     #$00
L988B:  lda     (L0019),y
        sta     $0580,x
        inx
        iny
        dec     $0400
        bne     L988B
        lda     #$00
        sta     $0580,x
        lda     #$01
        sta     $B5
L98A0:  rts

L98A1:  ldx     #$00
        ldy     #$10
        lda     #$20
L98A7:  sta     $0580,x
        inx
        dey
        bne     L98A7
        lda     #$00
        sta     $0580,x
        rts

L98B4:  ldx     #$00
        ldy     #$10
L98B8:  lda     $0580,x
        sta     $0501,x
        inx
        dey
        bne     L98B8
        lda     #$00
        sta     $0501,x
        rts

L98C8:  lda     $B5
        beq     L98DC
        ldx     #$00
        ldy     #$10
L98D0:  lda     $0501,x
        cmp     $0580,x
        bne     L98DE
        inx
        dey
        bne     L98D0
L98DC:  clc
        rts

L98DE:  sec
        rts

        .byte   $F0,$18,$20,$72,$91,$20,$98,$95
        .byte   $A9,$03,$18,$65,$5B,$85,$B2,$A9
        .byte   $00,$65,$5C
L98F3:  sta     $B3
        lda     #$00
        sta     $B4
        rts

L98FA:  lda     $05
        clc
        adc     #$03
        sta     $B2
        lda     #$00
        adc     $06
        jmp     L98F3

        .byte   $20,$C7,$8C,$C9,$80,$B0,$15,$C9
        .byte   $12,$F0,$14,$C9,$11,$F0,$10,$C9
        .byte   $0B,$F0,$0C,$C9,$22,$F0,$15,$20
        .byte   $C3,$A1,$90,$1E,$4C,$A1,$8C
L9927:  jsr     L8CA1
        jsr     L8CA1
        jmp     L8CA1

        .byte   $68,$4C,$A1,$8C,$48,$20,$AD,$8C
        .byte   $C9,$22,$F0,$F4,$29,$FF,$D0,$F5
        .byte   $68,$60,$48,$20,$AD,$8C,$20,$BB
        .byte   $A1,$90,$F8,$68,$60,$20,$75,$9F
        .byte   $20,$F8,$8F,$A5,$B2,$85,$09,$A5
        .byte   $B3,$85,$0A,$A5,$B4,$D0,$26,$20
        .byte   $C7,$8C,$F0,$0A,$C9,$91,$F0,$16
        .byte   $20,$08,$99,$4C,$5F,$99,$20,$AD
        .byte   $8C,$F0,$06,$20,$27,$99,$4C,$5F
        .byte   $99,$A9,$03,$4C,$8F,$A9,$A9,$FF
        .byte   $85,$B4,$20,$08,$99,$A5,$90,$29
        .byte   $0F,$C9,$03,$F0,$3E,$20,$FE,$A4
        .byte   $A0,$00,$A5,$28,$91,$8C,$A5,$29
        .byte   $C8,$91,$8C,$20,$78,$83,$F0,$06
        .byte   $20,$94,$A5,$4C,$AA,$99,$A9,$00
        .byte   $85,$B4,$A5,$09,$85,$B2,$A5,$0A
        .byte   $85,$B3,$20,$01,$90,$20,$D4,$8C
        .byte   $C9,$2C,$F0,$01,$60,$20,$A1,$8C
        .byte   $20,$D4,$8C,$20,$C3,$A1,$B0,$F4
        .byte   $4C,$4D,$99,$A9,$00,$85,$1B,$AA
        .byte   $A9,$2C,$85,$3C,$20,$D4,$8C,$C9
        .byte   $22,$D0,$05,$85,$3C,$20,$A1,$8C
        .byte   $B1,$09,$F0,$20,$C5,$3C,$F0,$0C
        .byte   $9D,$00,$07,$E6,$1B,$E8,$20,$A1
        .byte   $8C,$4C,$E0,$99,$20,$A1,$8C,$20
        .byte   $D4,$8C,$C9,$2C,$D0,$0A,$20,$A1
        .byte   $8C,$4C,$08,$9A,$A9,$00,$85,$B4
        .byte   $A5,$1B,$8D,$00,$04,$A9,$00,$8D
        .byte   $01,$04,$A9,$07,$8D,$02,$04,$20
        .byte   $EF,$82,$4C,$AA,$99,$20,$7C,$9C
        .byte   $AD,$00,$04,$F0,$60,$A5,$09,$48
        .byte   $A5,$0A,$48,$AD,$01,$04,$85,$09
        .byte   $AD,$02,$04,$85,$0A,$A9,$07,$85
        .byte   $92,$A9,$00,$85,$91,$85,$C6,$20
        .byte   $40,$9C,$A9,$21,$85,$91,$85,$C7
        .byte   $20,$40,$9C,$A9,$42,$85,$91,$85
        .byte   $C8,$20,$40,$9C,$A9,$01,$85,$CB
        .byte   $85,$CC,$85,$CD,$A9,$7F,$8D,$01
        .byte   $04,$8D,$05,$04,$A9,$07,$85,$C9
        .byte   $8D,$15,$40,$A9,$0A,$85,$63,$20
        .byte   $D4,$86,$A5,$C9,$30,$10,$D0,$F7
        .byte   $A9,$00,$85,$63,$8D,$15,$40,$68
        .byte   $85,$0A,$68,$85,$09,$60,$A9,$00
        .byte   $85,$63,$8D,$15,$40,$4C,$6D,$83
L9A90:  txa
        pha
        tya
        pha
        lda     #$C0
        sta     $4017
        jsr     L9AA2
        pla
        tay
        pla
        tax
        pla
        rti

L9AA2:  lda     #$02
        sta     $CA
        lda     #$04
        sta     $CE
L9AAA:  lda     $C9
        bmi     L9AB7
        jsr     L9AB8
        lsr     $CE
        dec     $CA
        bpl     L9AAA
L9AB7:  rts

L9AB8:  lda     $C9
        and     $CE
        beq     L9AB7
        ldx     $CA
        dec     $CB,x
        bne     L9AB7
        lda     $C6,x
        sta     $91
        lda     #$00
        sta     $C2
L9ACC:  ldy     #$00
        lda     ($91),y
        bne     L9AE3
        lda     $C9
        sec
        sbc     $CE
        sta     $C9
        sta     $4015
        ldx     $CA
        lda     $91
        sta     $C6,x
        rts

L9AE3:  inc     $91
        cmp     #$20
        beq     L9ACC
        cmp     #$4D
        bne     L9B05
        lda     ($91),y
        beq     L9B6A
        ldx     #$30
        cmp     #$30
        beq     L9AFD
        cmp     #$31
        bne     L9B6A
        ldx     #$00
L9AFD:  inc     $91
        ldy     $CA
        stx     $CF,y
        bpl     L9ACC
L9B05:  cmp     #$4F
        bne     L9B1E
        lda     ($91),y
        beq     L9B6A
        inc     $91
        cmp     #$36
        bcs     L9B6A
        sec
        sbc     #$30
        bcc     L9B6A
        ldx     $CA
        sta     $BF,x
        bpl     L9ACC
L9B1E:  cmp     #$54
        bne     L9B39
        lda     ($91),y
        beq     L9B6A
        inc     $91
        cmp     #$39
        bcs     L9B6A
        sec
        sbc     #$30
        beq     L9B6A
        bcc     L9B6A
        ldx     $CA
        sta     $B9,x
L9B37:  bpl     L9ACC
L9B39:  cmp     #$56
        bne     L9B6F
        lda     ($91),y
        beq     L9B6A
        inc     $91
        cmp     #$3A
        bcs     L9B6A
        sec
        sbc     #$30
        bcc     L9B6A
        cmp     #$01
        bne     L9B64
        lda     ($91),y
        cmp     #$36
        bcs     L9B62
        sec
        sbc     #$30
        bcc     L9B62
        clc
        adc     #$0A
        inc     $91
        bne     L9B64
L9B62:  lda     #$01
L9B64:  ldx     $CA
        sta     $BC,x
        bpl     L9B37
L9B6A:  lda     #$80
        sta     $C9
        rts

L9B6F:  cmp     #$59
        bne     L9B89
        lda     ($91),y
        cmp     #$34
        bcs     L9B6A
        sec
        sbc     #$30
        bcc     L9B6A
        inc     $91
        lsr     a
        ror     a
        ror     a
        ldx     $CA
        sta     $D2,x
        bpl     L9B37
L9B89:  cmp     #$23
        bne     L9B93
        lda     #$02
        sta     $C2
        bpl     L9B37
L9B93:  cmp     #$52
        bne     L9BAB
        jsr     L9C0E
        lda     $CA
        asl     a
        asl     a
        tay
        lda     #$90
        cpy     #$08
        bne     L9BA7
        lda     #$00
L9BA7:  sta     $4000,y
        rts

L9BAB:  cmp     #$41
        bcc     L9B6A
        cmp     #$48
        bcs     L9B6A
        pha
        jsr     L9C0E
        ldx     $CA
        txa
        asl     a
        asl     a
        tay
        cmp     #$08
        bne     L9BD8
        lda     $CB,x
        sec
        sbc     #$01
        cmp     #$0E
        bcc     L9BCC
        lda     #$0E
L9BCC:  asl     a
        asl     a
        ldy     $CF,x
        beq     L9BD4
        lda     #$81
L9BD4:  ldy     #$08
        bne     L9BDE
L9BD8:  lda     $BC,x
        ora     $D2,x
        ora     $CF,x
L9BDE:  sta     $4000,y
        pla
        sec
        sbc     #$41
        tay
        lda     $BF,x
        tax
        lda     L80A7,x
        clc
        adc     L80A0,y
        adc     $C2
        tax
        lda     $CA
        asl     a
        asl     a
        tay
        cmp     #$08
        bne     L9C01
        txa
        clc
        adc     #$18
        tax
L9C01:  lda     L8006,x
        sta     $4002,y
        lda     L8007,x
        sta     $4003,y
        rts

L9C0E:  lda     ($91),y
        beq     L9C25
        cmp     #$3A
        bcs     L9C25
        sec
        sbc     #$30
        bcc     L9C25
        tax
        lda     L8096,x
        ldx     $CA
        sta     $C3,x
        inc     $91
L9C25:  jsr     LA1CD
        ldx     $CA
        lda     $C3,x
        sta     L0028
        lda     $B9,x
        sta     $2C
        jsr     L8B5C
        lda     L0028
        ldx     $CA
        sta     $CB,x
        lda     $91
        sta     $C6,x
        rts

        .byte   $AD,$00,$04,$F0,$12,$CE,$00,$04
        .byte   $20,$9D,$8C,$C9,$3A,$F0,$08,$A0
        .byte   $00,$91,$91,$E6,$91,$D0,$E9,$A0
        .byte   $00,$A9,$00,$91,$91,$60
L9C5E:  jmp     L836D

        .byte   $4C,$70,$83,$20,$A1,$8C,$4C,$6D
        .byte   $9C,$20,$94,$A5,$20,$82,$9C,$20
        .byte   $EF,$A3,$AD,$01,$04,$D0,$E9,$AD
        .byte   $00,$04,$60
L9C7C:  jsr     L9C82
        jmp     LA3F2

L9C82:  lda     #$00
        sta     $27
        sta     $30
        lda     #$80
        sta     $21
        jsr     L9C94
        lda     $27
        beq     L9C5E
        rts

L9C94:  jsr     L9CBD
L9C97:  cmp     #$EF
        beq     L9C9C
        rts

L9C9C:  jsr     LA3EF
        jsr     L8CA1
        jsr     L9CBD
        jsr     LA3EF
        jsr     L9DE0
        lda     L0028
        eor     $2C
        sta     L0028
        lda     $29
        eor     $2D
        sta     $29
        jsr     LA1D7
        jmp     L9C97

L9CBD:  jsr     L9CE6
L9CC0:  cmp     #$F0
        beq     L9CC5
        rts

L9CC5:  jsr     LA3EF
        jsr     L8CA1
        jsr     L9CE6
        jsr     LA3EF
        jsr     L9DE0
        lda     L0028
        ora     $2C
        sta     L0028
        lda     $29
        ora     $2D
        sta     $29
        jsr     LA1D7
        jmp     L9CC0

L9CE6:  jsr     L9D0F
L9CE9:  cmp     #$F1
        beq     L9CEE
        rts

L9CEE:  jsr     LA3EF
        jsr     L8CA1
        jsr     L9D0F
        jsr     LA3EF
        jsr     L9DE0
        lda     L0028
        and     $2C
        sta     L0028
        lda     $29
        and     $2D
        sta     $29
        jsr     LA1D7
        jmp     L9CE9

L9D0F:  jsr     L9D4E
L9D12:  cmp     #$F3
        bcs     L9D17
L9D16:  rts

L9D17:  cmp     #$F9
        bcs     L9D16
        sec
        sbc     #$F3
        asl     a
        tax
        lda     LA27D,x
        pha
        lda     LA27E,x
        pha
        lda     $27
        pha
        jsr     L8CA1
        jsr     L9D4E
        pla
        cmp     $27
        bne     L9D86
        jsr     L9DE0
        pla
        sta     $1A
        pla
        sta     L0019
        jsr     L9D4B
        pha
        lda     #$02
        sta     $27
        pla
        jmp     L9D12

L9D4B:  jmp     (L0019)

L9D4E:  jsr     L9D89
L9D51:  cmp     #$F9
        beq     L9D5A
        cmp     #$FA
        beq     L9D63
        rts

L9D5A:  lda     #$D4
        pha
        lda     #$A1
        pha
        jmp     L9D69

L9D63:  lda     #$EE
        pha
        lda     #$A1
        pha
L9D69:  lda     $27
        pha
        jsr     L8CA1
        jsr     L9D89
        pla
        cmp     $27
        bne     L9D86
        jsr     L9DE0
        pla
        sta     $1A
        pla
        sta     L0019
        jsr     L9D4B
        jmp     L9D51

L9D86:  jmp     LA3E1

L9D89:  jsr     L9DA8
L9D8C:  cmp     #$FB
        beq     L9D91
        rts

L9D91:  lda     $27
        pha
        jsr     L8CA1
        jsr     L9DA8
        pla
        cmp     $27
        bne     L9D86
        jsr     L9DE0
        jsr     LA21D
        jmp     L9D8C

L9DA8:  jsr     L9EAB
L9DAB:  cmp     #$FC
        beq     L9DB4
        cmp     #$FD
        beq     L9DBD
        rts

L9DB4:  lda     #$FA
        pha
        lda     #$A1
        pha
        jmp     L9DC3

L9DBD:  lda     #$F4
        pha
        lda     #$A1
        pha
L9DC3:  lda     $27
        pha
        jsr     L8CA1
        jsr     L9EAB
        pla
        cmp     $27
        bne     L9D86
        jsr     L9DE0
        pla
        sta     $1A
        pla
        sta     L0019
        jsr     L9D4B
        jmp     L9DAB

L9DE0:  ldx     $30
        cpx     #$7D
        bcs     L9E23
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
        sta     L0028
        lda     $0401,x
        sta     $29
        lda     $0402,x
        sta     $2A
        rts

L9E0D:  jsr     L8CA1
        jsr     L9EAB
        ldx     $30
        dex
        dex
        dex
        lda     $0400,x
        sta     L0028
        lda     $0401,x
        sta     $29
        rts

L9E23:  jmp     L8370

L9E26:  jsr     L9E0D
        jsr     L8A7C
        lda     L0028
        sta     $0400,x
        lda     $29
        sta     $0401,x
        jmp     L8CD4

L9E39:  jsr     L9E0D
        lda     #$FF
        sec
        sbc     $0400,x
        sta     $0400,x
        lda     #$FF
        sec
        sbc     $0401,x
        sta     $0401,x
        lda     #$02
        sta     $27
        rts

L9E53:  jsr     L8CA1
        ldy     #$00
        ldx     $21
L9E5A:  lda     ($09),y
        beq     L9E71
        cmp     #$22
        beq     L9E6E
        sta     $0700,x
        inx
        iny
        cpy     #$20
        beq     L9E23
        jmp     L9E5A

L9E6E:  jsr     L8CA1
L9E71:  stx     L0019
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
        lda     L0019
        sta     $21
        lda     #$03
        sta     $27
        jmp     L8CD4

L9EA2:  jmp     L836D

L9EA5:  jmp     L9E26

L9EA8:  jsr     L83A1
L9EAB:  jsr     L8378
        beq     L9EA2
        cmp     #$FA
        beq     L9EA5
        cmp     #$F2
        beq     L9E39
        cmp     #$12
        beq     L9F0D
        cmp     #$11
        beq     L9F0D
        cmp     #$F9
        beq     L9EA8
        cmp     #$28
        beq     L9F01
        cmp     #$22
        beq     L9E53
        cmp     #$41
        bcc     L9EFB
        cmp     #$5B
        bcc     L9F2B
        cmp     #$A1
        beq     L9EFE
        cmp     #$CA
        bcc     L9EFB
        cmp     #$EF
        bcs     L9EFB
        sec
        sbc     #$CA
        asl     a
        tax
        lda     LC328,x
        sta     L0019
        lda     LC329,x
        sta     $1A
        jsr     L8CA1
        jsr     L8CD4
        jsr     L9D4B
        jmp     L8CD4

L9EFB:  jmp     L836D

L9EFE:  jmp     LA29D

L9F01:  jsr     L8CA1
        jsr     L9C94
        jsr     LA293
        jmp     L8CD4

L9F0D:  ldx     $30
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
        lda     #$02
        sta     $27
        jsr     L9927
        jmp     L8CD4

L9F2B:  jsr     L9F7E
        ldx     $30
        lda     $90
        and     #$0F
        cmp     #$03
        beq     L9F59
        ldy     #$00
        lda     ($8C),y
        sta     $0400,x
        iny
        lda     ($8C),y
        sta     $0401,x
        iny
        lda     ($8C),y
        sta     $0402,x
L9F4B:  inx
        inx
        inx
        stx     $30
        lda     $90
        and     #$0F
        sta     $27
        jmp     L8CD4

L9F59:  ldy     #$00
        lda     ($8C),y
        sta     $0400,x
        lda     #$01
        clc
        adc     $8C
        sta     $0401,x
        lda     #$00
        adc     $8D
        sta     $0402,x
        jmp     L9F4B

L9F72:  jmp     L836D

L9F75:  jsr     LA1C3
        bcs     L9F72
        ldx     #$00
        stx     $30
L9F7E:  ldx     #$00
        stx     $8B
        sta     $8A
        jsr     L8CAD
        jsr     LA1BB
        bcs     L9F96
        sta     $8B
L9F8E:  jsr     L8CAD
        jsr     LA1BB
        bcc     L9F8E
L9F96:  lda     #$02
        sta     $90
        jsr     L8CD4
        cmp     #$24
        bne     L9FA8
        lda     #$03
        sta     $90
        jsr     L8CA1
L9FA8:  jsr     L8CD4
        cmp     #$28
        beq     LA01F
        jsr     LA10E
        bcs     L9FF9
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
        beq     L9FE1
        lda     #$00
        ldx     #$21
L9FD9:  iny
        sta     ($8E),y
        dex
        bne     L9FD9
        lda     #$21
L9FE1:  clc
        adc     #$03
        adc     $8E
        sta     L0028
        lda     #$00
        adc     $8F
        sta     $29
        jsr     L8A44
        lda     L0028
        sta     $1F
        lda     $29
        sta     $20
L9FF9:  clc
        lda     #$03
        adc     $8E
        sta     $8C
        lda     #$00
        adc     $8F
        sta     $8D
        rts

        .byte   $A5,$8E,$85,$8C,$A5,$8F,$85,$8D
LA00F:  rts

LA010:  jsr     L9C94
        lda     #$02
        cmp     $27
        beq     LA00F
LA019:  jmp     LA3E1

LA01C:  jmp     LA0BC

LA01F:  lda     $8A
        pha
        lda     $8B
        pha
        lda     $90
        ora     #$80
        pha
        jsr     L8CA1
        jsr     LA010
        ldx     $30
        dex
        dex
        dex
        lda     $0400,x
        pha
        jsr     LA0F1
        bcc     LA01C
        jsr     LA594
        jsr     LA010
        ldx     $30
        dex
        dex
        dex
        lda     $0400,x
        pha
        jsr     LA0F1
        bcs     LA019
        jsr     L8CA1
        pla
        sta     L001B
        pla
        sta     L0019
        pla
        sta     $90
        pla
        sta     $8B
        pla
        sta     $8A
        jsr     LA10E
        bcc     LA0B9
        ldy     #$03
        lda     ($8E),y
        beq     LA0B9
        iny
        lda     ($8E),y
        cmp     L0019
        bcc     LA0B9
        tax
        iny
        lda     ($8E),y
        cmp     L001B
        bcc     LA0B9
        jsr     LA1CD
        inx
        bne     LA086
        inc     $29
LA086:  stx     L0028
        lda     L001B
        sta     $2C
        jsr     L8B5C
        lda     L0019
        sta     $2C
        jsr     L8AE8
        jsr     LA103
        sta     $2C
        jsr     L8B5C
        lda     #$06
LA0A0:  sta     $2C
        jsr     L8AE8
        lda     $8E
        sta     $2C
        lda     $8F
        sta     $2D
        jsr     L8AE8
LA0B0:  lda     L0028
        sta     $8C
        lda     $29
        sta     $8D
        rts

LA0B9:  jmp     L8370

LA0BC:  jsr     L8CA1
        pla
        sta     L0019
        pla
        sta     $90
        pla
        sta     $8B
        pla
        sta     $8A
        jsr     LA10E
        bcc     LA0B0
        ldy     #$03
        lda     ($8E),y
        bne     LA0B9
        iny
        lda     ($8E),y
        cmp     L0019
        bcc     LA0B9
        lda     L0019
        sta     L0028
        jsr     LA103
        sta     $2C
        jsr     LA1CD
        jsr     L8B5C
        lda     #$05
        jmp     LA0A0

LA0F1:  lda     $0401,x
        bne     LA0B9
        stx     $30
        jsr     L8CD4
        cmp     #$29
        bne     LA101
        clc
        rts

LA101:  sec
        rts

LA103:  lda     $90
        and     #$0F
LA107:  cmp     #$02
        beq     LA10D
        lda     #$21
LA10D:  rts

LA10E:  lda     $1D
        sta     $8E
        lda     $1E
        sta     $8F
LA116:  ldy     #$00
        lda     ($8E),y
        beq     LA152
        cmp     $90
        bne     LA130
        iny
        lda     ($8E),y
        cmp     $8A
        bne     LA130
        iny
        lda     ($8E),y
        cmp     $8B
        bne     LA130
        sec
        rts

LA130:  ldy     #$00
        lda     ($8E),y
        bmi     LA154
        jsr     LA107
        clc
        adc     #$03
        adc     $8E
        sta     $8E
        lda     #$00
        adc     $8F
        sta     $8F
LA146:  lda     $1F
        cmp     $8E
        bne     LA116
        lda     $20
        cmp     $8F
        bne     LA116
LA152:  clc
        rts

LA154:  and     #$0F
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
        jsr     LA1CD
        sta     $2C
        lda     $4A
        sta     L0028
        lda     $74
        beq     LA179
        lda     $4B
        sta     $2C
LA179:  inc     L0028
        bne     LA17F
        inc     $29
LA17F:  inc     $2C
        bne     LA185
        inc     $2D
LA185:  jsr     L8B5C
        lda     $73
        jsr     LA107
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8B5C
        lda     #$05
        sta     $2C
        lda     $74
        beq     LA1A0
        inc     $2C
LA1A0:  jsr     L8AE8
        lda     $8E
        sta     $2C
        lda     $8F
        sta     $2D
        jsr     L8AE8
        lda     L0028
        sta     $8E
        lda     $29
        sta     $8F
        jmp     LA146

LA1B9:  sec
        rts

LA1BB:  cmp     #$30
        bcc     LA1B9
        cmp     #$3A
        bcc     LA1CB
LA1C3:  cmp     #$41
        bcc     LA1B9
        cmp     #$5B
        bcs     LA1B9
LA1CB:  clc
        rts

LA1CD:  lda     #$00
        sta     $2D
        sta     $29
        rts

        .byte   $20,$EC,$8A
LA1D7:  ldx     $30
        dex
        dex
        dex
        lda     L0028
        sta     $0400,x
        lda     $29
        sta     $0401,x
        lda     $2A
        sta     $0402,x
        jmp     L8CD4

        .byte   $20,$73,$8A,$4C,$D7,$A1,$20,$51
        .byte   $8B,$4C,$D7,$A1
LA1FA:  lda     $2C
        bne     LA202
        lda     $2D
        beq     LA218
LA202:  nop
        nop
        nop
        nop
        nop
        jsr     L8BB2
        jmp     LA1D7

        .byte   $A9,$01,$85,$28,$A9,$00,$85,$29
        .byte   $4C,$D7,$A1
LA218:  lda     #$0A
        jmp     LA98F

LA21D:  lda     $29
        pha
        jsr     LA1FA
        lda     $2A
        sta     L0028
        lda     $2B
        sta     $29
        pla
        bpl     LA238
        lda     $29
        bmi     LA235
LA232:  jsr     L8A7C
LA235:  jmp     LA1D7

LA238:  lda     $29
        bpl     LA235
        bmi     LA232
        jsr     L8C03
        bne     LA24C
        lda     #$00
LA245:  sta     L0028
        sta     $29
        jmp     LA1D7

LA24C:  lda     #$FF
        jmp     LA245

        .byte   $20,$03,$8C,$F0,$F6,$90,$F4,$4C
        .byte   $43,$A2,$20,$03,$8C,$B0,$EC,$4C
        .byte   $43,$A2,$20,$03,$8C,$F0,$E4,$4C
        .byte   $43,$A2,$20,$03,$8C,$F0,$D3,$B0
        .byte   $DA,$4C,$43,$A2,$20,$03,$8C,$90
        .byte   $D2,$4C,$43,$A2
LA27D:  .byte   $3E
LA27E:  .byte   $A2,$5B,$A2,$51,$A2,$63,$A2,$6B
        .byte   $A2,$75,$A2
LA289:  jsr     L8CD4
        cmp     #$28
        bne     LA2DE
        jmp     L8CA1

LA293:  jsr     L8CD4
        cmp     #$29
        bne     LA2DE
        jmp     L8CA1

LA29D:  jsr     L8CA1
        jsr     LA3E6
        ldy     L0028
        lda     $0645,y
        ldy     #$00
        and     #$10
        beq     LA2B0
        ldy     #$FF
LA2B0:  sty     L0028
        sty     $29
        jsr     LA363
        jmp     L8CD4

        .byte   $A9,$00,$85,$28,$85,$29,$85,$2A
        .byte   $60
LA2C3:  jsr     LA289
        jsr     L9C94
        ldx     $30
        dex
        lda     $0400,x
        sta     $2A
        dex
        lda     $0400,x
        sta     $29
        dex
        lda     $0400,x
        sta     L0028
        rts

LA2DE:  jmp     L836D

        .byte   $A6,$30,$A5,$28,$9D,$00,$04,$E8
        .byte   $A5,$29,$9D,$00,$04,$E8,$E8,$86
        .byte   $30,$60,$A5,$49,$85,$28,$A9,$00
        .byte   $85,$29,$E6,$30,$E6,$30,$E6,$30
LA301:  lda     #$02
        sta     $27
        ldx     $30
        dex
        dex
        dex
        lda     L0028
        sta     $0400,x
        lda     $29
        sta     $0401,x
        lda     $2A
        sta     $0402,x
        rts

        .byte   $20,$20,$A3,$4C,$FB,$A2
LA320:  sec
        lda     $03
        sbc     $1F
        sta     L0028
        lda     $04
        sbc     $20
        sta     $29
        rts

        .byte   $20,$EC,$A3,$A5,$48,$85,$28,$A9
        .byte   $00,$85,$29,$4C,$63,$A3,$20,$E6
        .byte   $A3,$A5,$28,$0A,$0A,$0A,$AA,$60
        .byte   $20,$3C,$A3,$BD,$C6,$06,$4C,$33
        .byte   $A3,$20,$3C,$A3,$BD,$C7,$06,$4C
        .byte   $33,$A3,$20,$EC,$A3,$A0,$00,$B1
        .byte   $28,$85,$28,$84,$29
LA363:  jsr     LA293
        jmp     LA301

        .byte   $20,$EC,$A3,$A5,$29,$30,$03,$4C
        .byte   $63,$A3,$20,$7C,$8A,$4C,$63,$A3
        .byte   $20,$EC,$A3,$A5,$29,$30,$0D,$F0
        .byte   $14,$A9,$00,$85,$29,$A9,$01,$85
        .byte   $28,$4C,$63,$A3,$A9,$FF,$85,$28
        .byte   $85,$29,$4C,$63,$A3,$A5,$28,$D0
        .byte   $E8,$A9,$00,$4C,$8F,$A3,$20,$EC
        .byte   $A3,$A5,$28,$48,$A5,$29,$48,$A5
        .byte   $56,$85,$28,$A5,$57,$85,$29,$A9
        .byte   $83,$85,$2C,$A9,$03,$85,$2D,$20
        .byte   $59,$8B,$A9,$7F,$25,$29,$85,$29
        .byte   $85,$57,$A5,$28,$85,$56,$68,$85
        .byte   $2D,$68,$85,$2C,$20,$59,$8B,$06
        .byte   $29,$26,$36,$26,$37,$A5,$36,$85
        .byte   $28,$A5,$37,$85,$29,$4C,$63,$A3
LA3E1:  lda     #$0B
        jmp     LA98F

LA3E6:  jsr     LA3EC
        jmp     LA548

LA3EC:  jsr     LA2C3
LA3EF:  lda     #$02
LA3F2           := * + 1
        ldx     $03A9
        cmp     $27
        bne     LA3E1
        rts

        .byte   $20,$C3,$A2,$4C,$F2,$A3,$20,$F9
        .byte   $A3,$A9,$00,$20,$BE,$A2,$4C,$63
        .byte   $A3,$20,$F9,$A3,$A5,$28,$F0,$F1
        .byte   $A0,$00,$B1,$29,$85,$28,$4C,$02
        .byte   $A4,$4C,$70,$83,$20,$A1,$AC,$20
        .byte   $E6,$A3,$A5,$28,$C9,$1C,$B0,$F1
        .byte   $85,$48,$20,$9E,$A5,$20,$48,$A5
        .byte   $A5,$28,$C9,$18,$B0,$E3,$85,$49
        .byte   $20,$D4,$8C,$C9,$29,$F0,$0F,$20
        .byte   $9E,$A5,$A5,$28,$F0,$08,$20,$37
        .byte   $BA,$A5,$9C,$4C,$59,$A4,$20,$A6
        .byte   $AB,$A9,$05,$20,$08,$88,$A5,$73
        .byte   $85,$28,$20,$AA,$AC,$4C,$64,$A4
        .byte   $20,$E6,$A3,$A5,$28,$20,$73,$A4
        .byte   $A9,$03,$85,$27,$20,$93,$A2,$4C
        .byte   $05,$A3,$A6,$21,$9D,$00,$07,$E6
        .byte   $21,$A9,$01,$85,$28,$86,$29,$A9
        .byte   $07,$85,$2A,$60,$20,$EC,$A3,$20
        .byte   $96,$8D,$20,$EB,$8C,$A9,$00,$85
        .byte   $90,$A4,$21,$A6,$46,$B5,$3D,$F0
        .byte   $0A,$99,$00,$07,$E8,$C8,$E6,$90
        .byte   $4C,$96,$A4,$A5,$21,$84,$21,$85
        .byte   $29,$A9,$07,$85,$2A,$A5,$90,$85
        .byte   $28,$4C,$69,$A4,$20,$EC,$A3,$20
        .byte   $D8,$8D,$4C,$8B,$A4,$20,$F9,$A3
        .byte   $A5,$28,$F0,$27,$20,$F8,$8F,$A5
        .byte   $28,$85,$74,$A2,$00,$A0,$00,$B1
        .byte   $29,$9D,$00,$05,$E8,$C8,$C6,$74
        .byte   $D0,$F5,$A9,$00,$9D,$00,$05,$20
        .byte   $32,$8A,$20,$FE,$A4,$20,$01,$90
        .byte   $4C,$63,$A3,$20,$BA,$A2,$4C,$63
        .byte   $A3,$20,$A1,$8C,$20,$FE,$A4,$4C
        .byte   $7C,$8A,$20,$A1,$8C,$A0,$00,$B1
        .byte   $09,$C9,$2D,$F0,$EC,$C9,$2B,$F0
        .byte   $F1,$C9,$26,$F0,$03,$4C,$F9,$8C
        .byte   $20,$A1,$8C,$B1,$09,$C9,$48,$D0
        .byte   $03,$20,$A1,$8C,$4C,$EB,$8D,$20
        .byte   $78,$83,$F0,$0D,$C9,$28,$D0,$09
        .byte   $20,$C3,$A2,$20,$63,$B2,$4C,$66
        .byte   $A4,$20,$88,$AF,$F0,$0B,$20,$73
        .byte   $A4,$20,$FB,$A2,$A9,$03,$85,$27
        .byte   $60,$20,$BA,$A2,$4C,$3A,$A5
LA548:  lda     $29
        bne     LA552
        rts

        .byte   $C9,$20,$B0,$01,$60
LA552:  jmp     L8370

        .byte   $20,$F9,$A3,$A5,$28,$48,$A5,$29
        .byte   $48,$A5,$2A,$48,$20,$9E,$A5,$20
        .byte   $48,$A5,$A5,$28,$20,$4D,$A5,$68
        .byte   $85,$1A,$68,$85,$19,$68,$60,$20
        .byte   $55,$A5,$85,$4A,$A5,$28,$C5,$4A
        .byte   $B0,$0B,$A5,$19,$85,$29,$A5,$1A
        .byte   $85,$2A,$4C,$69,$A4,$A5,$4A,$85
        .byte   $28,$4C,$7F,$A5
LA591:  jmp     L836D

LA594:  jsr     L8CD4
        cmp     #$2C
        bne     LA591
        jmp     L8CA1

        .byte   $20,$94,$A5,$20,$94,$9C,$A6,$30
        .byte   $CA,$BD,$00,$04,$85,$2A,$CA,$BD
        .byte   $00,$04,$85,$29,$CA,$BD,$00,$04
        .byte   $85,$28,$86,$30,$4C,$EF,$A3,$20
        .byte   $55,$A5,$85,$4A,$A5,$28,$C5,$4A
        .byte   $B0,$C2,$38,$A5,$4A,$E5,$28,$18
        .byte   $65,$19,$85,$19,$A9,$00,$65,$1A
        .byte   $85,$1A,$A5,$28,$85,$90,$A6,$21
        .byte   $A0,$00,$B1,$19,$9D,$00,$07,$E8
        .byte   $C8,$C6,$90,$D0,$F5,$A5,$21,$85
        .byte   $29,$86,$21,$A9,$07,$85,$2A,$4C
        .byte   $69,$A4,$4C,$70,$83,$20,$F9,$A3
        .byte   $A5,$28,$48,$A5,$29,$48,$A5,$2A
        .byte   $48,$20,$9E,$A5,$20,$48,$A5,$A5
        .byte   $28,$F0,$E7,$20,$4D,$A5,$38,$E9
        .byte   $01,$48,$20,$9E,$A5,$20,$48,$A5
        .byte   $A5,$28,$20,$4D,$A5,$85,$1C,$68
        .byte   $85,$1B,$68,$85,$1A,$68,$85,$19
        .byte   $68,$C5,$1B,$90,$3B,$38,$E5,$1B
        .byte   $C5,$1C,$90,$2F,$A5,$1B,$18,$65
        .byte   $19,$85,$19,$A9,$00,$65,$1A,$85
        .byte   $1A,$A5,$1C,$85,$28,$85,$90,$A6
        .byte   $21,$A0,$00,$B1,$19,$9D,$00,$07
        .byte   $E8,$C8,$C6,$90,$D0,$F5,$A5,$21
        .byte   $85,$29,$86,$21,$A9,$07,$85,$2A
        .byte   $4C,$69,$A4,$85,$1C,$4C,$3A,$A6
        .byte   $20,$BA,$A2,$4C,$69,$A4,$4C,$70
        .byte   $83,$20,$E6,$A3,$A5,$28,$F0,$07
        .byte   $C9,$01,$D0,$F2,$A9,$17,$AE,$A9
        .byte   $16,$85,$19,$A9,$40,$85,$1A,$A9
        .byte   $05,$8D,$16,$40,$EA,$EA,$A9,$04
        .byte   $8D,$16,$40,$EA,$EA,$A2,$08,$A0
        .byte   $00,$B1,$19,$4A,$26,$90,$EA,$EA
        .byte   $CA,$D0,$F6,$60,$20,$77,$A6,$20
        .byte   $7A,$A6,$A5,$90,$29,$0F,$85,$28
        .byte   $4C,$63,$A3,$20,$77,$A6,$A5,$90
        .byte   $4A,$4A,$4A,$4A,$4C,$B4,$A6
LA6C5:  asl     a
        asl     a
        asl     a
        asl     a
        rts

        .byte   $20,$6D,$9C,$29,$03,$0A,$0A,$0A
        .byte   $85,$74,$A5,$32,$29,$E7,$05,$74
        .byte   $85,$32,$4C,$F1,$B3
LA6DF:  ldx     #$00
        lda     #$EF
LA6E3:  sta     $0200,x
        inx
        bne     LA6E3
        rts

        .byte   $A2,$00,$86,$65,$C9,$42,$F0,$08
        .byte   $C9,$53,$D0,$4D,$A9,$10,$85,$65
        .byte   $20,$64,$9C,$C9,$04,$B0,$3F,$48
        .byte   $A9,$04,$85,$02,$A9,$00,$85,$7C
        .byte   $20,$6A,$9C,$A4,$7C,$99,$3D,$00
        .byte   $E6,$7C,$C6,$02,$D0,$F2,$68,$8D
        .byte   $00,$07,$A5,$3D,$8D,$01,$07,$A5
        .byte   $3E,$8D,$02,$07,$A5,$3F,$8D,$03
        .byte   $07,$A5,$40,$8D,$04,$07,$A0,$06
        .byte   $A6,$65,$F0,$01,$C8,$98,$4C,$08
        .byte   $88,$C9,$08,$B0,$01,$60,$4C,$70
        .byte   $83,$4C,$6D,$83,$C9,$2C,$F0,$25
        .byte   $20,$6D,$9C,$F0,$04,$C9,$01,$D0
        .byte   $ED,$0A,$AA,$BD,$A5,$C3,$85,$52
        .byte   $BD,$A6,$C3,$85,$53,$A9,$00,$85
        .byte   $65,$20,$8A,$A7,$20,$78,$83,$F0
        .byte   $1E,$C9,$2C,$D0,$D4,$20,$64,$9C
        .byte   $C9,$03,$B0,$CA,$0A,$AA,$BD,$A9
        .byte   $C3,$85,$52,$BD,$AA,$C3,$85,$53
        .byte   $A9,$01,$85,$65,$20,$8A,$A7,$60
LA78A:  lda     #$00
        sta     $0700
        sta     $75
LA791:  lda     #$04
        ldx     #$01
        sta     $74
        ldy     $75
LA799:  lda     ($52),y
        sta     $0700,x
        inx
        iny
        dec     $74
        bne     LA799
        sty     $75
        lda     #$06
        sta     $73
        lda     $65
        beq     LA7B0
        inc     $73
LA7B0:  lda     $73
        jsr     L8808
        inc     $0700
        lda     $0700
        cmp     #$04
        bne     LA791
        rts

LA7C0:  lda     $33
        ora     #$10
LA7C4:  sta     $33
        sta     PPUMASK
        jmp     L8CA1

LA7CC:  lda     $33
        and     #$EF
        jmp     LA7C4

        .byte   $68,$20,$C5,$A6,$AA,$A0,$04,$A9
        .byte   $EF,$9D,$00,$02,$9D,$03,$02,$9D
        .byte   $01,$02,$E8,$E8,$E8,$E8,$88,$D0
        .byte   $EE,$60,$4C,$70,$83,$4C,$6D,$83
        .byte   $C9,$9A,$F0,$C9,$C9,$9B,$F0,$D1
        .byte   $20,$6D,$9C,$20,$3B,$A7,$48,$20
        .byte   $78,$83,$F0,$CC,$20,$6A,$9C,$48
        .byte   $20,$6A,$9C,$85,$1B,$68,$85,$19
        .byte   $68,$85,$75,$AA,$BD,$3D,$06,$85
        .byte   $90,$A5,$75,$0A,$0A,$AA,$0A,$0A
        .byte   $48,$A8,$A9,$04,$85,$74,$BD,$80
        .byte   $06,$99,$01,$02,$E8,$C8,$C8,$C8
        .byte   $C8,$C6,$74,$D0,$F1,$68,$A8,$A5
        .byte   $1B,$99,$00,$02,$A5,$19,$99,$03
        .byte   $02,$A5,$90,$D0,$01,$60,$A9,$00
        .byte   $85,$1C,$A9,$08,$85,$1A,$20,$65
        .byte   $A8,$A9,$00,$85,$1A,$A9,$08,$85
        .byte   $1C,$20,$65,$A8,$A9,$08,$85,$1A
        .byte   $85,$1C,$C8,$C8,$C8,$C8,$A5,$19
        .byte   $18,$65,$1A,$99,$03,$02,$A5,$1B
        .byte   $18,$65,$1C,$99,$00,$02,$60,$C9
        .byte   $A1,$D0,$03,$4C,$E4,$BF,$4C,$6D
        .byte   $83,$4C,$70,$83,$C9,$8A,$D0,$EF
        .byte   $20,$64,$9C,$20,$3B,$A7,$48,$20
        .byte   $94,$A5,$20,$89,$A2,$A9,$04,$85
        .byte   $75,$20,$6D,$9C,$48,$20,$6A,$9C
        .byte   $48,$C6,$75,$D0,$F8,$20,$93,$A2
        .byte   $20,$A2,$85,$68,$6A,$6A,$29,$80
        .byte   $85,$19,$68,$6A,$6A,$6A,$29,$40
        .byte   $05,$19,$85,$19,$68,$6A,$6A,$6A
        .byte   $6A,$29,$20,$05,$19,$85,$19,$68
        .byte   $85,$AD,$68,$29,$03,$05,$19,$85
        .byte   $19,$68,$85,$75,$20,$C5,$A6,$A0
        .byte   $04,$AA,$A9,$00,$9D,$00,$02,$9D
        .byte   $03,$02,$A5,$19,$9D,$02,$02,$A9
        .byte   $20,$9D,$01,$02,$E8,$E8,$E8,$E8
        .byte   $88,$D0,$E7,$20,$7C,$9C,$AD,$01
        .byte   $04,$85,$1B,$AD,$02,$04,$85,$1C
        .byte   $A0,$00,$A5,$75,$0A,$0A,$AA,$AD
        .byte   $00,$04,$F0,$10,$B1,$1B,$9D,$80
        .byte   $06,$E8,$C8,$C0,$04,$F0,$10,$CE
        .byte   $00,$04,$D0,$F0,$A9,$20,$9D,$80
        .byte   $06,$E8,$C8,$C0,$04,$D0,$F7,$A6
        .byte   $75,$A5,$AD,$9D,$3D,$06,$60,$20
        .byte   $75,$AB,$A9,$20,$85,$74,$A9,$0C
        .byte   $20,$08,$88,$A2,$00,$A5,$74,$18
        .byte   $69,$04,$20,$F8,$81,$AD,$07,$20
        .byte   $AD,$07,$20,$48,$A5,$74,$20,$F8
        .byte   $81,$68,$8D,$07,$20,$E8,$D0,$E5
        .byte   $E6,$74,$A5,$74,$C9,$24,$D0,$DD
        .byte   $20,$E6,$AB,$20,$F1,$B3,$A9,$0B
        .byte   $4C,$08,$88
LA96E:  ldx     #$3D
        lda     #$00
LA972:  sta     $0600,x
        inx
        bne     LA972
        ldx     #$00
        lda     #$EF
LA97C:  sta     $0680,x
        inx
        cpx     #$20
        bne     LA97C
        ldx     #$C0
        lda     #$01
LA988:  sta     $0600,x
        inx
        bne     LA988
        rts

LA98F:  pha
        jsr     LAC03
        pla
        ldx     #$FF
        txs
        asl     a
        tay
        lda     #$3F
        jsr     L87F0
        lda     LAA5E,y
        jsr     L87F0
        lda     LAA5F,y
        jsr     L87F0
        lda     #$3F
        sta     $52
        lda     #$AA
        sta     $53
        jsr     L87B7
        jsr     L873B
LA9B8:  lda     #$FF
        cmp     $0C
        bne     LA9C2
        cmp     $0B
        beq     LA9D0
LA9C2:  lda     #$34
        sta     $52
        lda     #$AA
        sta     $53
        jsr     L87B7
        jsr     LA9D9
LA9D0:  jsr     L87FB
        jsr     LB30F
        jmp     L8221

LA9D9:  lda     $0B
        sta     L0028
        lda     $0C
        sta     $29
LA9E1:  jsr     L8D3C
        jsr     L8CEB
        jmp     L865F

LA9EA:  jsr     LAC03
        lda     #$00
        sta     $62
        lda     #$39
        sta     $52
        lda     #$AA
        sta     $53
        jsr     L87B7
        jmp     LA9B8

LA9FF:  lda     #$4F
        jsr     LAC0D
        lda     #$4B
        jmp     LAC0D

LAA09:  lda     #$58
        sta     $52
        lda     #$C3
        sta     $53
        jsr     L87F8
        jsr     LA320
        jsr     LA9E1
        jsr     L87FB
        lda     #$28
        sta     $52
        lda     #$AA
        sta     $53
        jmp     L87F8

        .byte   $20,$42,$59,$54,$45,$53,$20,$46
        .byte   $52,$45,$45,$00,$20,$49,$4E,$20
        .byte   $00,$42,$52,$45,$41,$4B,$00,$20
        .byte   $45,$52,$52,$4F,$52,$00,$57,$52
        .byte   $49,$54,$49,$4E,$47,$20,$00,$4C
        .byte   $4F,$41,$44,$49,$4E,$47,$20,$00
        .byte   $53,$4B,$49,$50,$20,$00
LAA5E:  .byte   $4E
LAA5F:  .byte   $46,$53,$4E,$52,$47,$4F,$44,$49
        .byte   $4C,$4F,$56,$4F,$4D,$55,$4C,$53
        .byte   $4F,$44,$44,$44,$5A,$54,$4D,$53
        .byte   $54,$46,$54,$43,$43,$55,$46,$4D
        .byte   $4F,$54,$50
LAA82:  lda     #$10
        sta     $32
        lda     #$0E
        sta     $33
        jsr     LABF1
        lda     #$00
        sta     $9D
        sta     $69
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
        lda     #$40
        sta     $4017
        jsr     LAAF3
        jsr     LB3F1
        lda     #$20
        sta     $4F
        jsr     LAB75
        jsr     LAB09
        ldx     #$00
        txa
        ldy     #$80
LAACD:  sta     $0480,x
        inx
        dey
        bne     LAACD
        ldx     #$00
LAAD6:  lda     $67
        cmp     #$08
        beq     LAB08
        jsr     LB0E1
        tay
LAAE0:  lda     LAB1D,x
        beq     LAAED
        sta     $0480,y
        inx
        iny
        jmp     LAAE0

LAAED:  inx
        inc     $67
        jmp     LAAD6

LAAF3:  lda     #$FD
        sta     $E0
        lda     #$5F
        sta     $E1
        jmp     LADA9

LAAFE:  lda     PPUSTATUS
        bmi     LAAFE
LAB03:  lda     PPUSTATUS
        bpl     LAB03
LAB08:  rts

LAB09:  lda     #$BF
        sta     $52
        lda     #$C3
        sta     $53
        lda     #$00
        sta     $65
        jsr     LA78A
        inc     $65
        jmp     LA78A

LAB1D:  .byte   $43,$4C,$53,$20,$0D,$00,$50,$52
        .byte   $49,$4E,$54,$20,$00,$47,$4F,$54
        .byte   $4F,$20,$00,$43,$48,$52,$24,$28
        .byte   $00,$53,$59,$53,$54,$45,$4D,$0D
        .byte   $00,$43,$4F,$4E,$54,$0D,$00,$4C
        .byte   $49,$53,$54,$0D,$00,$52,$55,$4E
        .byte   $0D,$00
LAB4F:  ldy     #$04
        ldx     #$C0
        lda     #$20
LAB55:  sta     PPUDATA
        dex
        bne     LAB55
        dey
        bne     LAB55
LAB5E:  ldx     #$C0
        lda     $4F
        clc
        adc     #$03
        jsr     L81F8
        lda     #$00
LAB6A:  sta     PPUDATA
        inx
        bne     LAB6A
        rts

LAB71:  lda     #$24
        sta     $4F
LAB75:  lda     #$0C
        jsr     L8808
        jsr     LB3EB
        jsr     LABF1
        lda     $4F
        ldx     #$00
        jsr     L81F8
        jsr     LAB4F
        jsr     LABE6
        jsr     LB3F1
        lda     #$0B
        jsr     L8808
        jsr     LAD8B
        sta     $69
        sta     $9D
        tax
LAB9D:  sta     $0600,x
        inx
        cpx     #$1A
        bne     LAB9D
        rts

LABA6:  lda     #$20
        sta     L0028
        lda     #$00
        sta     $29
        sta     $2D
        lda     $69
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     $49
        adc     #$03
        cmp     #$1E
        bcc     LABC0
        sbc     #$1E
LABC0:  sta     $2C
        jsr     L8B5C
        clc
        lda     $48
        adc     #$02
        adc     L0028
        sta     $4A
        lda     #$00
        adc     $29
        clc
        adc     $4F
        sta     $4B
        rts

LABD8:  lda     $4B
        sta     PPUADDR
        lda     $4A
        sta     PPUADDR
        rts

LABE3:  sta     PPUDATA
LABE6:  lda     $9D
        sta     PPUSCROLL
        lda     $69
        sta     PPUSCROLL
        rts

LABF1:  jsr     LAAFE
        lda     #$00
        sta     PPUMASK
        rts

LABFA:  jsr     LAAFE
        lda     $33
        sta     PPUMASK
LAC02:  rts

LAC03:  lda     $48
        beq     LAC02
LAC07:  lda     #$00
        sta     $48
        lda     #$0A
LAC0D:  cmp     #$20
        bcs     LAC34
        cmp     #$08
        beq     LAC25
        cmp     #$01
        beq     LAC25
        cmp     #$12
        beq     LAC25
        cmp     #$09
        beq     LAC25
        ldy     #$00
        sty     $4C
LAC25:  asl     a
        tax
        lda     LAF01,x
        sta     L0028
        lda     LAF01+1,x
        sta     $29
        jmp     (L0028)

LAC34:  ldy     $4C
        beq     LAC3F
        pha
        lda     #$12
        jsr     LAC0D
        pla
LAC3F:  jsr     L87F0
        jmp     L87FB

LAC45:  inc     $49
        lda     $49
        cmp     #$18
        beq     LAC5D
LAC4D:  rts

LAC4E:  jsr     LAD8F
LAC51:  ldx     $49
        lda     $0601,x
        beq     LAC45
        inc     $49
        jmp     LAC51

LAC5D:  dec     $7E
        jsr     LAD8B
        jsr     LABA6
        lda     #$04
        jsr     L8808
LAC6A:  lda     $32
        sta     PPUCTRL
        lda     $69
        clc
        adc     #$08
        cmp     #$F0
        bcc     LAC7A
        lda     #$00
LAC7A:  sta     $69
        jsr     LAAFE
        jsr     LABE6
        ldy     #$18
        ldx     #$00
LAC86:  lda     $0602,x
        sta     $0601,x
        inx
        dey
        bne     LAC86
        lda     #$00
        sta     $0618
        sta     $0619
        sta     $48
        lda     #$17
        sta     $49
        jmp     LB3F1

LACA1:  lda     $48
        sta     $6B
        lda     $49
        sta     $6C
        rts

LACAA:  lda     $6B
        sta     $48
        lda     $6C
        sta     $49
        rts

LACB3:  jsr     LABF1
        lda     #$10
        sta     $32
        jsr     LB3F1
        lda     #$0E
        sta     PPUMASK
        sta     $33
        jsr     LAB09
        jsr     LAB5E
        sta     $4C
        jsr     LABE6
        jmp     LABFA

LACD2:  jsr     LACA1
        jsr     LABA6
        lda     #$04
LACDA:  jsr     L8808
        ldx     $49
        lda     $0601,x
        beq     LACF1
        inc     $49
        jsr     LAD8F
        jsr     LABA6
        lda     #$04
        jmp     LACDA

LACF1:  jmp     LACAA

LACF4:  jsr     LACA1
        jsr     LABA6
LACFA:  lda     #$04
        jsr     L8808
        ldx     $49
        lda     #$00
        sta     $0601,x
        sta     $48
        inc     $49
        inx
        cpx     #$18
        beq     LACF1
        jsr     LABA6
        jmp     LACFA

        .byte   $A6,$49,$CA,$8A,$AA,$E8,$BD,$01
        .byte   $06,$60,$4C,$07,$AC
LAD22:  lda     $48
        cmp     #$15
        bcs     LAD33
        cmp     #$0E
        bcs     LAD36
        cmp     #$07
        bcs     LAD39
        lda     #$07
LAD33           := * + 1
        cpx     $1CA9
LAD36           := * + 1
        cpx     $15A9
LAD39           := * + 1
        cpx     $0EA9
        sec
        sbc     $48
        sta     $62
        tax
        lda     #$20
LAD43:  sta     $0700,x
        dex
        bpl     LAD43
        jmp     L87FB

LAD4C:  lda     #$FF
        eor     $4C
        sta     $4C
        rts

LAD53:  lda     $49
        beq     LAD59
        dec     $49
LAD59:  rts

LAD5A:  lda     $48
        pha
        jsr     LAD94
        pla
        sta     $48
        rts

LAD64:  lda     $48
        beq     LAD6B
        dec     $48
        rts

LAD6B:  lda     $49
        beq     LAD75
        lda     #$1B
        sta     $48
        dec     $49
LAD75:  rts

LAD76:  lda     $48
        cmp     #$1B
        beq     LAD7F
        inc     $48
        rts

LAD7F:  jsr     LAD8F
        lda     $49
        cmp     #$17
        beq     LAD8A
        inc     $49
LAD8A:  rts

LAD8B:  lda     #$00
        sta     $49
LAD8F:  lda     #$00
        sta     $48
        rts

LAD94:  ldx     $49
        inx
        cpx     #$18
        bne     LAD9E
        jmp     LAC5D

LAD9E:  stx     $49
        rts

LADA1:  lda     #$FF
        sta     $72
        lda     $E1
        bne     LADAF
LADA9:  lda     #$00
        sta     $72
        lda     $E0
LADAF:  sta     $61
        rts

LADB2:  ldx     $48
        beq     LADBB
        dec     $48
        jmp     LADCB

LADBB:  ldx     $49
        beq     LADCB
        dex
        lda     $0601,x
        beq     LADCB
        dec     $49
        lda     #$1B
        sta     $48
LADCB:  jsr     LACA1
LADCE:  lda     $48
        cmp     #$1B
        beq     LADEF
        jsr     LABA6
        lda     $4A
        sta     L0019
        lda     $4B
        sta     $1A
        inc     $48
        jsr     LABA6
        dec     $48
        lda     #$03
        jsr     L8808
        lda     #$1B
        sta     $48
LADEF:  ldx     $49
        lda     $0601,x
        bne     LAE07
        jsr     LABA6
        jsr     LAAFE
        jsr     LABD8
        lda     #$20
        jsr     LABE3
        jmp     LACAA

LAE07:  inc     $49
        lda     #$FF
        sta     $48
        jsr     LAE18
        inc     $49
        jsr     LAD8F
        jmp     LADCE

LAE18:  inc     $48
        jsr     LABA6
        jsr     LAAFE
        jsr     LABD8
        lda     PPUDATA
        lda     PPUDATA
        pha
        jsr     LABE6
        dec     $48
        lda     $48
        cmp     #$FF
        bne     LAE3B
        lda     #$1B
        sta     $48
        dec     $49
LAE3B:  jsr     LABA6
        jsr     LAAFE
        jsr     LABD8
        pla
        jmp     LABE3

LAE48:  jsr     LACA1
        lda     #$20
        sta     $B0
LAE4F:  jsr     LABA6
        lda     #$0D
        jsr     L8808
        ldx     $49
        lda     $0601,x
        beq     LAE69
        inc     $49
        jsr     LAD8F
        jmp     LAE4F

LAE66:  jmp     LACAA

LAE69:  lda     $B0
        cmp     #$20
        beq     LAE66
        lda     $49
        cmp     #$17
        beq     LAED6
        jsr     LAD8F
        ldy     $49
        iny
        cpy     #$17
        beq     LAEA5
        lda     $49
        sta     $91
        lda     #$16
        sta     $49
LAE87:  jsr     LABA6
        lda     $49
        pha
        lda     #$0E
        jsr     L8808
        pla
        sta     $49
        tax
        lda     $0601,x
        inx
        sta     $0601,x
        dec     $49
        lda     $49
        cmp     $91
        bne     LAE87
LAEA5:  ldx     $49
        lda     #$FF
        sta     $0601,x
        inc     $49
        inx
        lda     #$00
        sta     $0601,x
        ldx     #$17
        sta     $0601,x
        lda     $6B
        pha
        lda     $6C
        pha
        lda     $4C
        pha
        lda     #$05
        jsr     LAC0D
        pla
        sta     $4C
        lda     #$0D
        jsr     L8808
        pla
        sta     $49
        pla
        sta     $48
        rts

LAED6:  lda     $6B
        pha
        ldx     $6C
        beq     LAEDE
        dex
LAEDE:  txa
        pha
        jsr     LAC5D
        lda     #$17
        sta     $49
        lda     #$00
        sta     $48
        jsr     LABA6
        lda     #$0D
        jsr     L8808
        ldx     #$16
        lda     #$FF
        sta     $0601,x
        pla
        sta     $49
        pla
        sta     $48
        rts

LAF01:  .addr   LAC4D
        .addr   LAD4C
        .addr   LAC4D
        .addr   LAC4E
        .addr   LACB3
        .addr   LACD2
        .addr   LAC4D
        .addr   LB30F
        .addr   LADB2
        .addr   LAD22
        .addr   LAD94
        .addr   LAD8B
        .addr   LAB75
        .addr   LAD8F
        .addr   LAC4D
        .addr   LAC4D
        .addr   LAC4D
        .addr   LAC4D
        .addr   LAE48
        .addr   LAC4D
        .addr   LAC4D
        .addr   LAC4D
        .addr   LADA1
        .addr   LADA9
        .addr   LAC4D
        .addr   LAC4D
        .addr   LACF4
        .addr   LAC4D
        .addr   LAD76
        .addr   LAD64
        .addr   LAD53
        .addr   LAD5A
LAF41:  lda     #$00
        rts

LAF44:  sta     $6D
        jsr     LB0CB
        bcs     LAF50
        lda     $6D
        and     #$FF
        rts

LAF50:  lda     $6D
        ldx     $68
        bne     LAF41
        sta     $6A
        lda     #$04
        sta     $74
LAF5C:  jsr     LAF9B
        cmp     $6A
        bne     LAF88
        dec     $74
        bne     LAF5C
        sec
        sbc     #$F0
        jsr     LB0E1
        sta     $67
        lda     #$FF
        sta     $66
LAF73:  ldx     $67
        lda     $0480,x
        beq     LAF80
        inx
        stx     $67
        and     #$FF
        rts

LAF80:  lda     #$00
        sta     $66
        ldx     #$FF
        stx     $68
LAF88:  lda     $66
        bne     LAF73
        jsr     LAF9B
        beq     LAF98
        cmp     #$F0
        bcs     LAF44
        and     #$FF
        rts

LAF98:  sta     $68
        rts

LAF9B:  lda     #$05
        sta     $4016
        ldx     #$00
LAFA2:  lda     #$04
        sta     $4016
        lda     $4017
        lsr     a
        and     #$0F
        ldy     #$06
        sty     $4016
        sta     $04FF
        lda     $4017
        asl     a
        asl     a
        asl     a
        and     #$F0
        ora     $04FF
        eor     #$FF
        sta     $04E8,x
        inx
        cpx     #$09
        bcc     LAFA2
        bne     LAFD6
        lda     #$04
        sta     $4016
        lda     #$06
        sta     $4016
LAFD6:  cpx     #$0C
        bne     LAFA2
        lda     #$00
        sta     $18
        sta     $6F
        sta     $70
        sta     $71
        lda     $04ED
        and     #$80
        bne     LB00F
        lda     $04EF
        and     #$80
        bne     LB03F
        lda     $04F3
        and     #$04
        bne     LB047
        jsr     LB02D
        bne     LB001
        sta     $18
        rts

LB001:  ldy     $18
        bne     LB00C
        ldy     $50
        bne     LB00C
        and     #$FF
        rts

LB00C:  lda     #$00
        rts

LB00F:  lda     #$FF
        sta     $6F
        sta     $18
LB015:  lda     $04ED
        and     #$7F
        sta     $04ED
        lda     $04EF
        and     #$7F
        sta     $04EF
        lda     $04F3
        and     #$FB
        sta     $04F3
LB02D:  ldx     #$00
        ldy     #$09
LB031:  lda     $04E8,x
        bne     LB04F
        inx
        dey
        bne     LB031
        sta     $6D
        sta     $50
        rts

LB03F:  lda     #$FF
        sta     $70
        sta     $18
        bne     LB015
LB047:  lda     #$FF
        sta     $71
        sta     $18
        bne     LB015
LB04F:  stx     $6E
        ldy     #$00
        clc
LB054:  lsr     a
        bcs     LB05B
        iny
        jmp     LB054

LB05B:  txa
        asl     a
        asl     a
        asl     a
        sty     $6D
        clc
        adc     $6D
        tax
        lda     $6F
        bne     LB08B
        lda     $70
        bne     LB0A1
        lda     $71
        bne     LB0A9
        lda     $72
        bne     LB0B2
        lda     LB0EC,x
        cmp     #$FF
        beq     LB07F
        and     #$FF
        rts

LB07F:  cmp     $50
        beq     LB088
        sta     $50
        jsr     LADA1
LB088:  lda     #$00
        rts

LB08B:  lda     LB0EC,x
        cmp     #$20
        bcs     LB095
        and     #$FF
        rts

LB095:  sec
        sbc     #$60
        bcc     LB088
        cmp     #$20
        bcs     LB088
LB09E:  and     #$FF
        rts

LB0A1:  lda     $72
        bne     LB0B8
        lda     LB134,x
        rts

LB0A9:  lda     $72
        bne     LB0AE
        rts

LB0AE:  lda     LB20C,x
        rts

LB0B2:  lda     LB17C,x
        jmp     LB0BB

LB0B8:  lda     LB1C4,x
LB0BB:  cmp     #$FF
        bne     LB09E
        cmp     $50
        beq     LB088
        sta     $50
        jsr     LADA9
        lda     #$00
LB0CA:  rts

LB0CB:  jsr     LC400
        ldy     #$08
        ldx     #$00
LB0D2:  lda     $04E8,x
        asl     a
        asl     a
        asl     a
        asl     a
        bcs     LB0CA
        inx
        dey
        bne     LB0D2
        clc
        rts

LB0E1:  asl     a
        asl     a
        asl     a
        rts

        .byte   $77,$77,$77,$77,$77,$77,$77
LB0EC:  .byte   $34,$67,$66,$63,$F1,$65,$35,$76
        .byte   $32,$64,$73,$00,$F0,$77,$33,$78
        .byte   $12,$08,$0B,$1C,$F7,$0C,$08,$0B
        .byte   $39,$69,$6C,$2C,$F4,$6F,$30,$2E
        .byte   $5D,$0D,$1E,$1D,$F6,$5B,$5C,$1F
        .byte   $71,$FF,$7A,$03,$1B,$61,$31,$00
        .byte   $37,$79,$6B,$6D,$F3,$75,$38,$6A
        .byte   $2D,$3B,$27,$2F,$F5,$70,$3D,$00
        .byte   $74,$68,$6E,$20,$F2,$72,$36,$62
LB134:  .byte   $24,$47,$46,$43,$06,$45,$25,$56
        .byte   $40,$44,$53,$00,$02,$57,$23,$58
        .byte   $00,$08,$0B,$1C,$18,$0C,$08,$0C
        .byte   $28,$49,$4C,$3C,$10,$4F,$29,$3E
        .byte   $4D,$0D,$1E,$1D,$17,$7B,$7C,$1F
        .byte   $51,$00,$5A,$03,$1B,$41,$21,$00
        .byte   $26,$59,$4B,$4D,$0F,$55,$2A,$4A
        .byte   $5F,$3A,$22,$3F,$11,$50,$2B,$00
        .byte   $54,$48,$4E,$20,$0E,$52,$5E,$42
LB17C:  .byte   $34,$47,$46,$43,$F1,$45,$35,$56
        .byte   $32,$44,$53,$00,$F0,$57,$33,$58
        .byte   $12,$08,$0B,$1C,$F7,$0C,$08,$0B
        .byte   $39,$49,$4C,$2C,$F4,$4F,$30,$2E
        .byte   $5D,$0D,$1E,$1D,$F6,$5B,$5C,$1F
        .byte   $51,$FF,$5A,$03,$1B,$41,$31,$00
        .byte   $37,$59,$4B,$4D,$F3,$55,$38,$4A
        .byte   $2D,$3B,$27,$2F,$F5,$50,$3D,$00
        .byte   $54,$48,$4E,$20,$F2,$52,$36,$42
LB1C4:  .byte   $24,$67,$66,$63,$00,$65,$25,$76
        .byte   $40,$64,$73,$00,$F0,$77,$23,$78
        .byte   $12,$08,$0B,$1C,$F7,$0C,$08,$0B
        .byte   $28,$69,$6C,$3C,$F3,$6F,$29,$3E
        .byte   $7D,$0A,$1E,$1D,$F6,$7B,$7C,$1F
        .byte   $71,$00,$7A,$03,$1B,$61,$21,$00
        .byte   $26,$79,$6B,$6D,$F3,$75,$2A,$6A
        .byte   $5F,$3A,$22,$3F,$F5,$70,$2B,$00
        .byte   $74,$68,$6E,$20,$F2,$72,$5E,$62
LB20C:  .byte   $03,$FE,$00,$00,$00,$00,$0D,$F7
        .byte   $FD,$F2,$00,$FD,$00,$00,$00,$F6
        .byte   $12,$00,$00,$00,$00,$00,$00,$F5
        .byte   $EF,$F0,$00,$00,$00,$00,$00,$F4
        .byte   $ED,$EE,$00,$00,$00,$00,$00,$F3
        .byte   $EB,$EC,$00,$00,$00,$00,$00,$F2
        .byte   $EA,$00,$00,$00,$00,$00,$00,$F1
        .byte   $E9,$E8,$00,$00,$00,$00,$1B,$F0
        .byte   $F1,$08,$20,$1F,$1C,$1D,$1E,$0B
LB254:  jsr     LABA6
        lda     #$08
        bne     LB260
LB25B:  jsr     LABA6
        lda     #$09
LB260:  jmp     L8808

LB263:  lda     #$00
        sta     $5E
        jsr     LAF88
        beq     LB2B2
        ldx     $66
        bne     LB295
        cmp     $75
        bne     LB2B2
        lda     $7B
        bne     LB298
        jsr     LB254
LB27B:  jsr     LAF88
        beq     LB2AF
        cmp     $75
        bne     LB2AF
        dec     $77
        bne     LB27B
        dec     $78
        bne     LB27B
        lda     #$FF
        sta     $7B
        jmp     LB306

LB293:  lda     #$03
LB295:  sta     $75
        rts

LB298:  jsr     LAF88
        beq     LB2B2
        cmp     $75
        bne     LB2B2
        dec     $79
        bne     LB298
        lda     #$20
        sta     $79
        jsr     LB254
        jmp     LB306

LB2AF:  jsr     LB25B
LB2B2:  lda     #$00
        sta     $7B
        sta     $77
        lda     #$20
        sta     $79
        lda     #$01
        sta     $78
        jsr     LB254
        lda     #$80
        sta     $74
LB2C7:  jsr     LAF88
        bne     LB2F1
        dec     $74
        bne     LB2C7
        jsr     LB25B
        lda     $5E
        bne     LB293
        lda     #$80
        sta     $74
LB2DB:  jsr     LAF88
        bne     LB2F1
        dec     $74
        bne     LB2DB
        lda     $5E
        bne     LB293
        jmp     LB2B2

LB2EB:  jsr     LB25B
        jmp     LB2B2

LB2F1:  sta     $75
        lda     $66
        bne     LB306
        lda     #$01
        sta     $74
LB2FB:  jsr     LAF88
        cmp     $75
        bne     LB2EB
        dec     $74
        bne     LB2FB
LB306:  jsr     LB25B
        jsr     LB314
        lda     $75
        rts

LB30F:  lda     #$F0
        sta     L0019
        .byte   $AE
LB314:  lda     #$01
        sta     L0019
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
LB336:  dey
        bne     LB336
        dec     L0019
        bne     LB336
        lda     #$00
        sta     $4015
        rts

LB343:  lda     #$00
        sta     $76
        sta     $63
LB349:  jsr     LB263
        cmp     #$0D
        beq     LB36E
        cmp     #$03
        beq     LB35E
        cmp     #$04
        beq     LB35E
        jsr     LAC0D
        jmp     LB349

LB35E:  jsr     LAC0D
        ldx     $76
        lda     #$00
        sta     $0501,x
        lda     #$03
        sta     $0500,x
        rts

LB36E:  lda     $49
        beq     LB37E
        tax
        dex
        lda     $0601,x
        beq     LB37E
        dec     $49
        jmp     LB36E

LB37E:  lda     $49
        sta     $47
        lda     #$02
        jsr     L8808
        lda     $64
        bne     LB395
        ldx     $76
        dex
        lda     #$00
        sta     $0500,x
        stx     $76
LB395:  ldx     $76
LB397:  dex
        txa
        beq     LB3A2
        lda     $0500,x
        cmp     #$20
        beq     LB397
LB3A2:  inx
        lda     #$00
        sta     $0500,x
        jmp     LAC4E

LB3AB:  inc     $AD
        bne     LB3B1
        inc     $AE
LB3B1:  rts

LB3B2:  inc     $91
        bne     LB3B1
        inc     $92
        rts

LB3B9:  lda     $AD
        cmp     $AF
        bne     LB3B1
        lda     $AE
        cmp     $B0
        rts

LB3C4:  lda     $0514
        sta     $AD
        clc
        adc     $0512
        sta     $AF
        lda     $0515
        sta     $AE
        adc     $0513
        sta     $B0
        rts

LB3DA:  lda     #$00
        sta     $AD
        lda     #$05
        sta     $AE
        lda     #$80
        sta     $AF
        lda     #$05
        sta     $B0
        rts

LB3EB:  lda     $32
        sta     PPUCTRL
        rts

LB3F1:  lda     $32
        ora     #$80
        sta     PPUCTRL
        rts

        .byte   $20,$EB,$B3,$A9,$01,$4C,$06,$B4
LB401:  jsr     LB3EB
        lda     #$00
        sta     $B1
        lda     #$00
        sta     $92
        jsr     LB3C4
LB40F:  lda     #$14
        sta     $91
LB413:  jsr     LB4C0
        bcs     LB425
        bne     LB40F
        dec     $91
        bne     LB413
        lda     #$14
        sta     $91
LB422:  jsr     LB4C0
LB425:  bcs     LB498
        beq     LB40F
        dec     $91
        bne     LB422
        jmp     LB458

LB430:  jsr     LB3EB
        lda     #$00
        sta     $B1
        jsr     LB3DA
LB43A:  lda     #$28
        sta     $91
LB43E:  jsr     LB4C0
        bcs     LB498
        bne     LB43A
        dec     $91
        bne     LB43E
        lda     #$28
        sta     $91
LB44D:  jsr     LB4C0
        bcs     LB498
        beq     LB43A
        dec     $91
        bne     LB44D
LB458:  lda     #$00
        sta     $91
        sta     $92
        jsr     LB4A6
        jsr     LB4A6
LB464:  jsr     LB4D4
        bcs     LB498
        ldx     $B1
        bne     LB474
        ldy     #$00
        sta     ($AD),y
        jmp     LB47A

LB474:  ldy     #$00
        cmp     ($AD),y
        bne     LB498
LB47A:  jsr     LB3AB
        jsr     LB3B9
        bne     LB464
        lda     $91
        sta     $2C
        lda     $92
        sta     $2D
        jsr     LB4D4
        cmp     $2D
        bne     LB498
        jsr     LB4D4
        cmp     $2C
        beq     LB49F
LB498:  jsr     LB3F1
        lda     #$02
LB49D:  sec
        rts

LB49F:  jsr     LB3F1
        lda     #$00
        clc
        rts

LB4A6:  jsr     L86B9
        beq     LB49D
        lda     $4016
        and     #$02
        beq     LB4A6
LB4B2:  jsr     L86B9
        beq     LB49D
        lda     $4016
        and     #$02
        bne     LB4B2
        clc
        rts

LB4C0:  jsr     LB4A6
        bcs     LB49D
        ldy     #$4E
        sty     $75
LB4C9:  dec     $75
        bne     LB4C9
        lda     $4016
        and     #$02
        clc
        rts

LB4D4:  lda     #$08
        sta     $74
        lda     #$00
        sta     L0019
LB4DC:  lda     L0019
        clc
        adc     L0019
        sta     L0019
        jsr     LB4C0
        bcs     LB49D
        bne     LB4EF
        jsr     LB3B2
        inc     L0019
LB4EF:  dec     $74
        bne     LB4DC
        jsr     LB4A6
        bcs     LB49D
        lda     L0019
        clc
        rts

LB4FC:  jsr     LB3EB
        jsr     LB3DA
        lda     #$28
        sta     $91
        lda     #$00
        sta     $92
        jmp     LB51B

LB50D:  jsr     LB3EB
        jsr     LB3C4
        lda     #$14
        sta     $91
        lda     #$00
        sta     $92
LB51B:  lda     #$20
        sta     L0019
        lda     #$4E
        sta     $1A
LB523:  jsr     LB586
        dec     L0019
        bne     LB523
        dec     $1A
        bne     LB523
        lda     $91
        sta     L0019
LB532:  jsr     LB590
        dec     L0019
        bne     LB532
LB539:  jsr     LB586
        dec     $91
        bne     LB539
        jsr     LB590
        lda     #$00
        sta     $91
        sta     $92
LB549:  ldy     #$00
        lda     ($AD),y
        jsr     LB56A
        jsr     LB3AB
        jsr     LB3B9
        bne     LB549
        lda     $91
        pha
        lda     $92
        jsr     LB56A
        pla
        jsr     LB56A
        jsr     LB590
        jmp     LB3F1

LB56A:  jsr     LB590
        ldx     #$08
LB56F:  sta     L0019
        clc
        adc     L0019
        bcs     LB57D
        jsr     LB586
LB579:  dex
        bne     LB56F
        rts

LB57D:  jsr     LB590
        jsr     LB3B2
        jmp     LB579

LB586:  pha
        lda     #$34
        sta     L001B
        sta     $1C
        jmp     LB597

LB590:  pha
        lda     #$6A
        sta     L001B
        sta     $1C
LB597:  lda     #$04
        sta     $4016
LB59C:  dec     $1C
        bne     LB59C
        lda     #$FF
        sta     $4016
LB5A5:  dec     L001B
        bne     LB5A5
        pla
        rts

LB5AB:  lda     #$00
        sta     $9E
LB5AF:  jsr     LA6DF
        jsr     LAA82
        lda     $9E
        cmp     #$81
        beq     LB5BE
        jsr     LAB71
LB5BE:  jsr     LAAFE
        jsr     LABE6
        lda     $32
        ora     #$08
        sta     $32
        lda     #$24
        sta     $4F
        jsr     LB3F1
        jsr     LA7C0
        lda     #$00
        sta     $0505
        sta     $0506
        sta     $88
        sta     $0504
        jmp     LB882

LB5E4:  lda     $0505
        sta     $48
        lda     $0506
        sta     $49
        rts

LB5EF:  sta     $E0
        sta     $E1
        jsr     LADA9
        jmp     LB263

LB5F9:  cmp     #$1C
        beq     LB60A
        cmp     #$1D
        beq     LB617
        cmp     #$1E
        beq     LB627
        cmp     #$1F
        beq     LB637
        rts

LB60A:  lda     $0505
        cmp     #$1B
        beq     LB632
        inc     $0505
LB614:  lda     #$00
        rts

LB617:  lda     $0505
        beq     LB622
        dec     $0505
        jmp     LB614

LB622:  lda     #$1B
        sta     $0505
LB627:  lda     $0506
        beq     LB614
        dec     $0506
        jmp     LB614

LB632:  lda     #$00
        sta     $0505
LB637:  lda     $0506
        cmp     #$14
        beq     LB614
        inc     $0506
        bne     LB614
LB643:  jsr     LA7CC
        jsr     LB6B5
        jsr     LAD8B
        jsr     LB801
        jsr     LAD8B
LB652:  lda     #$3E
        jsr     LB5EF
        cmp     #$1E
        beq     LB67F
        cmp     #$1F
        beq     LB688
        cmp     #$0D
        beq     LB692
        cmp     #$03
        beq     LB669
        bne     LB652
LB669:  jsr     LB6B8
        jsr     LAD8F
        lda     #$15
        sta     $49
        lda     #$1A
        jsr     LAC0D
        lda     #$81
        sta     $9E
        jmp     L80AD

LB67F:  lda     $49
        beq     LB652
        dec     $49
        jmp     LB652

LB688:  lda     $49
        cmp     #$05
        beq     LB652
        inc     $49
        bne     LB652
LB692:  lda     $49
        asl     a
        tax
        lda     LB6A3,x
        sta     L0019
        lda     LB6A3+1,x
        sta     $1A
        jmp     (L0019)

LB6A3:  .addr   LB87F
        .addr   LBB22
        .addr   LBB1F
        .addr   LB5AB
        .addr   LBBFB
        .addr   LB72D
LB6AF:  jsr     LABE6
        jmp     LABFA

LB6B5:  lda     #$FF
        .byte   $AE
LB6B8:  lda     #$00
        sta     $0500
        jsr     LABF1
        lda     #$09
        sta     $74
        lda     #$00
        sta     $3D
        jsr     LAD8B
LB6CB:  jsr     LABA6
        jsr     LABD8
        lda     $0500
        beq     LB6D9
        lda     PPUDATA
LB6D9:  ldy     #$0A
        ldx     $3D
LB6DD:  lda     $0500
        beq     LB6EB
        lda     PPUDATA
        sta     $0520,x
        jmp     LB6F1

LB6EB:  lda     $0520,x
        sta     PPUDATA
LB6F1:  inx
        dey
        bne     LB6DD
        stx     $3D
        dec     $74
        beq     LB6AF
        inc     $49
        bne     LB6CB
LB6FF:  jsr     LA6DF
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
        jmp     LB783

LB72A:  jmp     LB643

LB72D:  jsr     LAAF3
        jsr     LB6B8
        lda     #$05
        jsr     LB81B
        jsr     LA7CC
        jsr     LB5E4
LB73E:  jsr     LACA1
        lda     $48
        sta     $0505
        lda     $49
        sta     $0506
        jsr     LB783
        jsr     LACAA
        jsr     LB263
        cmp     #$0D
        beq     LB767
        cmp     #$1B
        beq     LB72A
        bcc     LB73E
        jsr     LAC0D
        jsr     LB770
        jmp     LB73E

LB767:  jsr     LAC07
        jsr     LB770
        jmp     LB73E

LB770:  lda     $49
        cmp     #$15
        bcs     LB777
        rts

LB777:  lda     #$14
        sta     $49
        rts

        .byte   $58,$3A,$0D,$0A,$59,$3A,$00
LB783:  jsr     LAD8F
        lda     #$16
        sta     $49
        lda     #$7C
        sta     $52
        lda     #$B7
        sta     $53
        jsr     L87F8
        lda     $0505
        sta     L0028
        lda     #$02
        sta     $48
        lda     #$16
        sta     $49
        jsr     LB7B0
        lda     $0506
        sta     L0028
        lda     #$02
        sta     $48
        inc     $49
LB7B0:  jsr     LA1CD
        jsr     L8D3C
        lda     #$3D
        clc
        adc     #$03
        sta     $52
        lda     #$00
        sta     $53
        jmp     L87F8

        .byte   $4D,$4F,$44,$45,$20,$20,$00,$53
        .byte   $45,$4C,$45,$43,$54,$00,$4D,$4F
        .byte   $56,$45,$20,$20,$00,$43,$4F,$50
        .byte   $59,$20,$20,$00,$43,$48,$41,$52
        .byte   $20,$20,$00,$43,$4C,$45,$41,$52
        .byte   $20,$00,$46,$49,$4C,$45,$20,$20
        .byte   $00
LB7F5:  .byte   $CB
LB7F6:  .byte   $B7,$D9,$B7,$D2,$B7,$E7,$B7,$EE
        .byte   $B7,$E0,$B7
LB801:  lda     #$00
        sta     $75
LB805:  lda     #$20
        jsr     L87F0
        lda     $75
        jsr     LB838
        jsr     LAC07
        inc     $75
        lda     $75
        cmp     #$06
        bne     LB805
        rts

LB81B:  sta     $7C
LB81D:  lda     #$05
        sta     $48
        lda     #$16
        sta     $49
        lda     #$C4
        sta     $52
        lda     #$B7
        sta     $53
        jsr     L87F8
        lda     #$05
        sta     $48
        inc     $49
        lda     $7C
LB838:  asl     a
        tax
        lda     LB7F5,x
        sta     $52
        lda     LB7F6,x
        sta     $53
        jmp     L87F8

LB847:  .byte   $80,$88,$90,$98,$B8,$C0,$C8,$D0
        .byte   $D8,$E0,$E8,$F0,$F8
LB854:  jsr     LB85A
        jmp     LB89D

LB85A:  jsr     LB8ED
        jsr     LAAFE
        lda     #$27
        ldx     #$2C
        jsr     L81F8
        lda     $88
        ora     #$30
        jmp     LABE3

LB86E:  jsr     LACA1
        jsr     LB5E4
        lda     #$20
        jsr     LAC0D
        jsr     LACAA
        jmp     LB89D

LB87F:  jsr     LB6B8
LB882:  jsr     LA7C0
        lda     #$00
        sta     $7F
        sta     $7D
        jsr     LB81B
        lda     #$03
        sta     $88
        jsr     LB85A
        lda     #$0C
        sta     $48
        lda     #$17
        sta     $49
LB89D:  jsr     LB9A2
LB8A0:  jsr     LACA1
        jsr     LB6FF
        jsr     LACAA
        lda     #$5E
        jsr     LB5EF
        jsr     LB5F9
        ldx     #$00
LB8B3:  cmp     LB8CF,x
        beq     LB8BF
        inx
        cpx     #$08
        bne     LB8B3
        beq     LB8A0
LB8BF:  txa
        asl     a
        tax
        lda     LB8D7,x
        sta     L0019
        lda     LB8D7+1,x
        sta     $1A
        jmp     (L0019)

LB8CF:  .byte   $1B,$0D,$0C,$0B,$08,$12,$20,$64
LB8D7:  .addr   LB917
        .addr   LB854
        .addr   LB92E
        .addr   LB937
        .addr   LB941
        .addr   LB94F
        .addr   LB95D
        .addr   LB86E
LB8E7:  lda     #$00
        sta     $88
        beq     LB8F5
LB8ED:  inc     $88
        lda     $88
        cmp     #$04
        beq     LB8E7
LB8F5:  asl     a
        asl     a
        ora     $88
        pha
        jsr     LAAFE
        lda     #$27
        ldx     #$F3
        jsr     L81F8
        pla
        tax
        and     #$FC
        sta     PPUDATA
        txa
        ldx     #$04
LB90E:  sta     PPUDATA
        dex
        bne     LB90E
        jmp     LABE6

LB917:  lda     #$0C
        sta     $48
        lda     #$16
        sta     $49
        lda     #$05
        jsr     LAC0D
        inc     $49
        lda     #$05
        jsr     LAC0D
        jmp     LB643

LB92E:  lda     $7D
        beq     LB934
        dec     $7D
LB934:  jmp     LB89D

LB937:  lda     $7D
        cmp     #$0C
        beq     LB934
        inc     $7D
        bne     LB934
LB941:  lda     $48
        cmp     #$1A
        beq     LB934
        inc     $48
        inc     $48
        inc     $7F
        bne     LB934
LB94F:  lda     $48
        cmp     #$0C
        beq     LB934
        dec     $48
        dec     $48
        dec     $7F
        bpl     LB934
LB95D:  jsr     LACA1
        jsr     LB5E4
        jsr     LABA6
        ldx     $7D
        lda     LB847,x
        clc
        adc     $7F
        sta     $73
        lda     #$09
        jsr     L8808
        lda     $88
        sta     $9C
        jsr     LBA67
        inc     $0505
        lda     $0505
        cmp     #$1C
        beq     LB98C
LB986:  jsr     LACAA
        jmp     LB89D

LB98C:  lda     #$00
        sta     $0505
        inc     $0506
        lda     $0506
        cmp     #$15
        bne     LB986
        lda     #$14
        sta     $0506
        bpl     LB986
LB9A2:  jsr     LACA1
        lda     #$0C
        sta     $48
        lda     #$16
        sta     $49
        ldx     $7D
        lda     LB847,x
        sta     $7E
        ldy     #$08
        ldx     #$00
LB9B8:  lda     $7E
        sta     $0510,x
        inx
        lda     #$20
        sta     $0510,x
        inx
        inc     $7E
        dey
        bne     LB9B8
        jsr     LABA6
        ldx     #$00
        ldy     #$08
        jsr     LAAFE
        jsr     LABD8
LB9D6:  lda     $0510,x
        sta     PPUDATA
        inx
        lda     #$20
        sta     PPUDATA
        inx
        dey
        bne     LB9D6
        jsr     LABE6
        jmp     LACAA

LB9EC:  inc     $48
        inc     $48
        lda     $49
        clc
        adc     #$03
        sta     $49
        and     #$FC
        asl     a
        sta     $4A
        lda     $48
        lsr     a
        lsr     a
        clc
        adc     $4A
        adc     #$C0
        sta     $4A
        lda     $4F
        adc     #$03
        sta     $4B
        lda     #$02
        and     $49
        sta     L0019
        lda     $48
        and     #$03
        lsr     a
        clc
        adc     L0019
        sta     L0019
        lda     #$FC
        sta     $1A
        asl     L0019
        ldx     L0019
        beq     LBA2F
LBA27:  asl     $9C
        sec
        rol     $1A
        dex
        bne     LBA27
LBA2F:  lda     #$05
        jsr     L8808
        lda     $73
        rts

LBA37:  jsr     LB9EC
        sta     $9C
        lda     $1A
        eor     #$FF
        and     $9C
        sta     $9C
        ldy     L0019
        beq     LBA4D
LBA48:  lsr     $9C
        dey
        bne     LBA48
LBA4D:  rts

        .byte   $20,$6D,$9C,$48,$20,$6A,$9C,$AD
        .byte   $00,$04,$85,$49,$68,$85,$48,$20
        .byte   $6A,$9C,$AD,$00,$04,$29,$03,$85
        .byte   $9C
LBA67:  jsr     LB9EC
        and     $1A
        ora     $9C
        sta     $73
        lda     #$09
        jmp     L8808

LBA75:  lda     #$03
        sta     $0500
        lda     #$00
        sta     $0514
        sta     $0516
        lda     #$07
        sta     $0515
        sta     $0517
        ldx     #$00
        stx     $0512
        inx
        stx     $0513
        lda     #$00
        sta     $5F
        lda     #$24
        sta     $60
        jsr     L97C3
LBA9E:  jsr     LABF1
        lda     $60
        ldx     $5F
        jsr     L81F8
        lda     PPUDATA
        ldx     #$00
LBAAD:  lda     PPUDATA
        sta     $0700,x
        inx
        bne     LBAAD
        jsr     LABE6
        jsr     LABFA
        jsr     LB4FC
        jsr     LB50D
        inc     $60
        lda     $60
        cmp     #$28
        bne     LBA9E
LBACA:  rts

LBACB:  jsr     L97DD
LBACE:  lda     #$00
        sta     $5F
        lda     #$24
        sta     $60
LBAD6:  jsr     LB430
        bcs     LBACA
        lda     $0500
        cmp     #$03
        bne     LBACB
        jsr     L98C8
        bcs     LBACB
        jsr     L97D0
        jsr     LB401
        bcs     LBACA
        jsr     LABF1
        lda     $60
        ldx     $5F
        jsr     L81F8
        ldx     #$00
LBAFB:  lda     $0700,x
        sta     PPUDATA
        inx
        bne     LBAFB
        jsr     LABE6
        jsr     LABFA
        inc     $60
        lda     $60
        cmp     #$28
        bne     LBAD6
        rts

LBB13:  lda     #$00
        sta     $AE
        lda     #$F8
        sta     $0210
        jmp     LB643

LBB1F:  lda     #$02
LBB22           := * + 1
        ldx     $01A9
        sta     $7C
        tax
        dex
        stx     $AF
        jsr     LB6B8
        jsr     LA7C0
        lda     #$00
        sta     $AE
        jsr     LB81D
LBB37:  jsr     LB6FF
        jsr     LBBAD
        lda     #$20
        jsr     LB5EF
        jsr     LB5F9
        cmp     #$1B
        beq     LBB13
        cmp     #$12
        beq     LBB7D
        cmp     #$08
        beq     LBB53
        bne     LBB37
LBB53:  lda     $AE
        beq     LBB37
        jsr     LACA1
        jsr     LB5E4
        jsr     LABA6
        lda     $91
        sta     $73
        lda     #$09
        jsr     L8808
        lda     $88
        sta     $9C
        jsr     LBA67
        jsr     LACAA
        lda     $AF
        beq     LBB7B
        lda     #$00
        sta     $AE
LBB7B:  beq     LBB37
LBB7D:  jsr     LACA1
        jsr     LB5E4
        jsr     LABA6
        lda     #$05
        jsr     L8808
        lda     $73
        sta     $91
        lda     $AF
        beq     LBB9C
        lda     #$20
        sta     $73
        lda     #$09
        jsr     L8808
LBB9C:  lda     #$FF
        sta     $AE
        jsr     LBA37
        lda     $9C
        sta     $88
        jsr     LACAA
        jmp     LBB37

LBBAD:  lda     $AE
        beq     LBBD9
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

LBBD9:  lda     #$F8
        sta     $0210
        rts

        .byte   $53,$41,$56,$45,$28,$53,$29,$2C
        .byte   $4C,$4F,$41,$44,$28,$4C,$29,$3F
        .byte   $00,$46,$49,$4C,$45,$20,$4E,$41
        .byte   $4D,$45,$22,$00
LBBFB:  jsr     LB6B8
        jsr     LAD8F
        lda     #$16
        sta     $49
        lda     #$1A
        jsr     LAC0D
        lda     #$20
        sta     $4F
        jsr     LB3F1
        jsr     LAAF3
        jsr     LAB75
        lda     #$DF
        sta     $52
        lda     #$BB
        sta     $53
        jsr     L87F8
        jsr     LB263
        cmp     #$53
        beq     LBC2F
        cmp     #$4C
        beq     LBC3F
        bne     LBC38
LBC2F:  jsr     LAC0D
        jsr     LBC4B
        jsr     LBA75
LBC38:  lda     #$81
        sta     $9E
        jmp     LB5AF

LBC3F:  jsr     LAC0D
        jsr     LBC4B
        jsr     LBACE
        jmp     LBC38

LBC4B:  jsr     LAC07
        lda     #$F0
        sta     $52
        lda     #$BB
        sta     $53
        jsr     L87F8
        jsr     LB343
        lda     $0500
        cmp     #$03
        beq     LBC70
        jsr     L8A32
        lda     #$09
        sta     $09
        jsr     L9869
        jmp     L98B4

LBC70:  pla
        pla
        jmp     LBC38

LBC75:  .byte   $A5
LBC76:  .byte   $BC,$A5,$BC,$B1,$BC,$B7,$BC,$BD
        .byte   $BC,$AB,$BC,$B7,$BC,$B1,$BC,$C3
        .byte   $BC,$C3,$BC,$B1,$BC,$B1,$BC,$C9
        .byte   $BC,$AB,$BC,$B1,$BC,$AB,$BC
LBC95:  .byte   $00,$1C,$00,$00,$00,$00,$38,$58
        .byte   $00,$0C,$78,$20,$00,$48,$88,$58
        .byte   $F3,$BC,$29,$BD,$CF,$BC,$FC,$BC
        .byte   $95,$BD,$EA,$BC,$FC,$BC,$3B,$BD
        .byte   $D8,$BC,$FC,$BC,$4D,$BD,$E1,$BC
        .byte   $0E,$BD,$5F,$BD,$EA,$BC,$05,$BD
        .byte   $71,$BD,$E1,$BC,$05,$BD,$83,$BD
        .byte   $E1,$BC,$00,$00,$40,$40,$40,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$40,$40
        .byte   $C0,$80,$80,$00,$00,$00,$00,$40
        .byte   $40,$40,$00,$00,$00,$00,$10,$20
        .byte   $10,$30,$10,$20,$10,$30,$10,$20
        .byte   $20,$20,$20,$20,$20,$20,$20,$20
        .byte   $10,$10,$10,$10,$10,$10,$10,$10
        .byte   $10,$10,$20,$20,$20,$20,$20,$20
        .byte   $20,$20
LBD17:  .byte   $00
LBD18:  .byte   $00,$00,$FE,$02,$FE,$02,$00,$02
        .byte   $02,$00,$02,$FE,$02,$FE,$00,$FE
        .byte   $FE,$C7,$BD,$C3,$BD,$CF,$BD,$BF
        .byte   $BD,$CF,$BD,$C3,$BD,$CB,$BD,$BB
        .byte   $BD,$CB,$BD,$D7,$BD,$D7,$BD,$D7
        .byte   $BD,$D7,$BD,$D7,$BD,$D7,$BD,$D7
        .byte   $BD,$D7,$BD,$D7,$BD,$E3,$BD,$DB
        .byte   $BD,$EF,$BD,$E7,$BD,$F7,$BD,$DF
        .byte   $BD,$F3,$BD,$E3,$BD,$EB,$BD,$1B
        .byte   $BE,$1F,$BE,$27,$BE,$27,$BE,$27
        .byte   $BE,$1B,$BE,$23,$BE,$23,$BE,$23
        .byte   $BE,$FB,$BD,$FB,$BD,$0F,$BE,$07
        .byte   $BE,$17,$BE,$FF,$BD,$13,$BE,$03
        .byte   $BE,$0B,$BE,$AF,$BD,$AF,$BD,$B7
        .byte   $BD,$B3,$BD,$B7,$BD,$AF,$BD,$B7
        .byte   $BD,$B3,$BD,$B7,$BD,$A7,$BD,$A7
        .byte   $BD,$AB,$BD,$AB,$BD,$AB,$BD,$A7
        .byte   $BD,$A7,$BD,$A7,$BD,$A7,$BD,$70
        .byte   $71,$72,$73,$71,$70,$73,$72,$D4
        .byte   $EF,$EF,$EF,$D0,$EF,$EF,$EF,$D2
        .byte   $EF,$EF,$EF,$00,$01,$02,$03,$01
        .byte   $00,$03,$02,$14,$15,$16,$17,$18
        .byte   $19,$1A,$1B,$0C,$0D,$0E,$0F,$0D
        .byte   $0C,$0F,$0E,$15,$14,$17,$16,$38
        .byte   $39,$3A,$3B,$50,$51,$52,$53,$52
        .byte   $53,$50,$51,$40,$41,$42,$43,$41
        .byte   $40,$43,$42,$48,$49,$4A,$4B,$49
        .byte   $48,$4B,$4A,$4A,$4B,$48,$49,$4B
        .byte   $4A,$49,$48,$A0,$A1,$A2,$A3,$A2
        .byte   $A3,$A0,$A1,$98,$99,$9A,$9B,$99
        .byte   $98,$9B,$9A,$9C,$9D,$9E,$9F,$9D
        .byte   $9C,$9F,$9E,$9E,$9F,$9C,$9D,$9F
        .byte   $9E,$9D,$9C,$68,$69,$6A,$6B,$6C
        .byte   $6D,$6E,$6F,$60,$61,$62,$63,$61
        .byte   $60,$63,$62,$6D,$6C,$6F,$6E,$69
        .byte   $68,$6B,$6A
LBE33:  txa
        pha
        tya
        pha
        jsr     LBE44
        pla
        tay
        pla
        tax
        lda     #$00
        sta     $AB
        pla
        rti

LBE44:  ldx     #$00
        stx     $A9
        lda     #$08
        sta     $5D
LBE4C:  ldx     $A9
        lda     $0645,x
        and     #$10
        bne     LBE7B
        dec     $5D
        beq     LBE63
        inx
LBE5A:  inc     $A9
        lda     $A9
        cmp     #$08
        bne     LBE4C
        rts

LBE63:  lda     #$00
        sta     $AC
        rts

LBE68:  lda     $06C5,x
        sta     $06C3,x
        ldx     $A9
        lda     $0645,x
        and     #$EF
        sta     $0645,x
        jmp     LBE5A

LBE7B:  lda     $A9
        asl     a
        asl     a
        asl     a
        sta     $A6
        tax
        dec     $06C2,x
        bne     LBE5A
        lda     $06C4,x
        sta     $06C2,x
        dec     $06C3,x
        beq     LBE68
        lda     $06C1,x
        and     #$0F
        tay
        lda     LBC95,y
        sta     $DD
        tya
        asl     a
        tay
        lda     LBC75,y
        sta     $A7
        lda     LBC76,y
        sta     $A8
        lda     $06C0,x
        asl     a
        tay
        lda     LBD17,y
        clc
        adc     $06C6,x
        sta     $06C6,x
        clc
        lda     LBD18,y
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
        sta     $D7
        beq     LBEF9
        lda     $06C1,x
        and     #$F0
        clc
        adc     #$10
        cmp     $D7
        bne     LBEE7
        lda     #$00
LBEE7:  pha
        lda     #$0F
        and     $06C1,x
        sta     $06C1,x
        pla
        pha
        ora     $06C1,x
        sta     $06C1,x
        pla
LBEF9:  lsr     a
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
        beq     LBF21
        cmp     #$05
        bne     LBF71
LBF21:  lda     $06C1,x
        and     #$0F
        beq     LBF4B
        cmp     #$01
        beq     LBF4B
        cmp     #$04
        bne     LBF71
        lda     $D7
        beq     LBF67
        lda     $06C0,x
        cmp     #$05
        beq     LBF43
        lda     #$2B
        sta     $DB
        lda     #$BE
        bne     LBF55
LBF43:  lda     #$2F
        sta     $DB
        lda     #$BE
        bne     LBF55
LBF4B:  lda     $D7
        beq     LBF67
        lda     #$D3
        sta     $DB
        lda     #$BD
LBF55:  sta     $DC
        ldy     $A9
        lda     #$40
        ora     $0645,y
        sta     $0645,y
        lda     #$00
        sta     $D7
        beq     LBF71
LBF67:  ldy     $A9
        lda     $0645,y
        and     #$3F
        sta     $0645,y
LBF71:  lda     $A9
        jsr     LA6C5
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
        jmp     LBE5A

        .byte   $4C,$70,$83,$4C,$6D,$83,$20,$A1
        .byte   $8C,$20,$89,$A2,$20,$6D,$9C,$20
        .byte   $3B,$A7,$20,$93,$A2,$AD,$00,$04
        .byte   $AA,$A9,$00,$9D
LBFFA:  .byte   $45
LBFFB:  .byte   $06,$8A,$85,$A4,$0A,$0A,$0A,$85
        .byte   $A5,$20,$A2,$85,$20,$D4,$8C,$C9
        .byte   $8A,$D0,$D3,$20,$A1,$8C,$20,$89
        .byte   $A2,$20,$A2,$C0,$29,$0F,$9D,$C1
        .byte   $06,$20,$9F,$C0,$C9,$09,$B0,$BB
        .byte   $9D,$C0,$06,$BD,$C1,$06,$0A,$A8
        .byte   $B9,$75,$BC,$85,$19,$B9,$76,$BC
        .byte   $85,$1A,$A0,$04,$B1,$19,$85,$1B
        .byte   $C8,$B1,$19,$85,$1C,$BD,$C0,$06
        .byte   $A8,$B1,$1B,$A4,$A4,$99,$45,$06
        .byte   $20,$A8,$C0,$B0,$4E,$20,$9F,$C0
        .byte   $9D,$C2,$06,$9D,$C4,$06,$20,$9F
        .byte   $C0,$18,$69,$01,$9D,$C3,$06,$9D
        .byte   $C5,$06,$20,$A8,$C0,$B0,$34,$20
        .byte   $9F,$C0,$A0,$00,$AD,$00,$04,$F0
        .byte   $02,$A0,$20,$A6,$A4,$98,$1D,$45
        .byte   $06,$9D,$45,$06,$20,$A8,$C0,$B0
        .byte   $1A,$20,$9F,$C0,$29,$03,$A6,$A4
        .byte   $1D,$45,$06,$9D,$45,$06,$20,$93
        .byte   $A2,$A9,$78,$A6,$A5,$9D,$C6,$06
        .byte   $9D,$C7,$06,$60,$20,$94,$A5,$20
        .byte   $6D,$9C,$A6,$A5,$60,$20,$D4,$8C
        .byte   $C9,$29,$F0,$02,$18,$60,$20,$A1
        .byte   $8C,$38,$60,$20,$6D,$9C,$20,$3B
        .byte   $A7,$0A,$0A,$0A,$85,$A5,$20,$9F
        .byte   $C0,$9D,$C6,$06,$20,$9F,$C0,$9D
        .byte   $C7,$06,$60,$20,$6D,$9C,$20,$3B
        .byte   $A7,$AA,$A9,$10,$1D,$45,$06,$9D
        .byte   $45,$06,$20,$78,$83,$F0,$06,$20
        .byte   $94,$A5,$4C,$CE,$C0,$A9,$FF,$85
        .byte   $AC,$60,$A9,$FF,$AE,$A9,$00,$85
        .byte   $75,$20,$6D,$9C,$20,$3B,$A7,$A8
        .byte   $B9,$45,$06,$29,$EF,$99,$45,$06
        .byte   $A5,$75,$F0,$13,$98,$20,$C5,$A6
        .byte   $A8,$A9,$F8,$99,$80,$02,$99,$84
        .byte   $02,$99,$88,$02,$99,$8C,$02,$20
        .byte   $78,$83,$F0,$CD,$20,$94,$A5,$A5
        .byte   $75,$F0,$CA,$D0,$C5,$80,$47,$4F
        .byte   $54,$4F,$81,$47,$4F,$53,$55,$42
        .byte   $82,$52,$55,$4E,$83,$52,$45,$54
        .byte   $55,$52,$4E,$84,$52,$45,$53,$54
        .byte   $4F,$52,$45,$85,$54,$48,$45,$4E
        .byte   $86,$4C,$49,$53,$54,$87,$53,$59
        .byte   $53,$54,$45,$4D,$88,$54,$4F,$89
        .byte   $53,$54,$45,$50,$8A,$53,$50,$52
        .byte   $49,$54,$45,$8B,$50,$52,$49,$4E
        .byte   $54,$8C,$46,$4F,$52,$8D,$4E,$45
        .byte   $58,$54,$8E,$50,$41,$55,$53,$45
        .byte   $8F,$49,$4E,$50,$55,$54,$90,$4C
        .byte   $49,$4E,$50,$55,$54,$91,$44,$41
        .byte   $54,$41,$92,$49,$46,$93,$52,$45
        .byte   $41,$44,$94,$44,$49,$4D,$95,$52
        .byte   $45,$4D,$96,$53,$54,$4F,$50,$97
        .byte   $43,$4F,$4E,$54,$98,$43,$4C,$53
        .byte   $99,$43,$4C,$45,$41,$52,$9A,$4F
        .byte   $4E,$9B,$4F,$46,$46,$9C,$43,$55
        .byte   $54,$9D,$4E,$45,$57,$9E,$50,$4F
        .byte   $4B,$45,$9F,$43,$47,$53,$45,$54
        .byte   $A0,$56,$49,$45,$57,$A1,$4D,$4F
        .byte   $56,$45,$A2,$45,$4E,$44,$A3,$50
        .byte   $4C,$41,$59,$A4,$42,$45,$45,$50
        .byte   $A5,$4C,$4F,$41,$44,$A6,$53,$41
        .byte   $56,$45,$A7,$50,$4F,$53,$49,$54
        .byte   $49,$4F,$4E,$A8,$4B,$45,$59,$A9
        .byte   $43,$4F,$4C,$4F,$52,$AA,$44,$45
        .byte   $46,$AB,$43,$47,$45,$4E,$AC,$53
        .byte   $57,$41,$50,$AD,$43,$41,$4C,$4C
        .byte   $AE,$4C,$4F,$43,$41,$54,$45,$AF
        .byte   $50,$41,$4C,$45,$54,$B0,$45,$52
        .byte   $41,$EF,$58,$4F,$52,$F0,$4F,$52
        .byte   $F1,$41,$4E,$44,$F2,$4E,$4F,$54
        .byte   $F3,$3C,$3E,$F4,$3E,$3D,$F5,$3C
        .byte   $3D,$F6,$3D,$F7,$3E,$F8,$3C,$F9
        .byte   $2B,$FA,$2D,$FB,$4D,$4F,$44,$FC
        .byte   $2F,$FD,$2A,$CA,$41,$42,$53,$CB
        .byte   $41,$53,$43,$CC,$53,$54,$52,$24
        .byte   $CD,$46,$52,$45,$CE,$4C,$45,$4E
        .byte   $CF,$50,$45,$45,$4B,$D0,$52,$4E
        .byte   $44,$D1,$53,$47,$4E,$D2,$53,$50
        .byte   $43,$D3,$54,$41,$42,$D4,$4D,$49
        .byte   $44,$24,$D5,$53,$54,$49,$43,$4B
        .byte   $D6,$53,$54,$52,$49,$47,$D7,$58
        .byte   $50,$4F,$53,$D8,$59,$50,$4F,$53
        .byte   $D9,$56,$41,$4C,$DA,$50,$4F,$53
        .byte   $DB,$43,$53,$52,$4C,$49,$4E,$DC
        .byte   $43,$48,$52,$24,$DD,$48,$45,$58
        .byte   $24,$DE,$49,$4E,$4B,$45,$59,$24
        .byte   $DF,$52,$49,$47,$48,$54,$24,$E0
        .byte   $4C,$45,$46,$54,$24,$E1,$53,$43
        .byte   $52,$24,$FF
LC2C6:  .byte   $46
LC2C7:  .byte   $91,$C6,$90,$7C,$85,$26,$91,$E0
        .byte   $98,$6D,$83,$AF,$85,$D7,$80,$6D
        .byte   $83,$6D,$83,$F3,$A7,$40,$87,$7E
        .byte   $92,$0D,$93,$98,$91,$C2,$93,$F6
        .byte   $94,$43,$83,$04,$92,$4D,$99,$4E
        .byte   $96,$43,$83,$DA,$86,$17,$87,$75
        .byte   $AB,$26,$92,$4D,$95,$6D,$83,$F0
        .byte   $C0,$82,$83,$DF,$91,$46,$A7,$32
        .byte   $A9,$CE,$C0,$0B,$82,$1D,$9A,$0F
        .byte   $B3,$FC,$97,$8C,$97,$B6,$C0,$AA
        .byte   $95,$4E,$BA,$87,$A8,$CA,$A6,$96
        .byte   $94,$8C,$91,$CA,$91,$EA,$A6,$ED
        .byte   $C0
LC328:  .byte   $69
LC329:  .byte   $A3,$0A,$A4,$85,$A4,$1A,$A3,$FF
        .byte   $A3,$58,$A3,$9F,$A3,$79,$A3,$6D
        .byte   $83,$6D,$83,$FB,$A5,$AA,$A6,$B9
        .byte   $A6,$46,$A3,$4F,$A3,$BE,$A4,$2E
        .byte   $A3,$F3,$A2,$61,$A4,$B5,$A4,$20
        .byte   $A5,$BD,$A5,$74,$A5,$1D,$A4,$47
        .byte   $20,$42,$41,$53,$49,$43,$20,$42
        .byte   $53,$2E,$32,$41,$0D,$0A,$40,$20
        .byte   $42,$42,$4B,$20,$43,$4F,$4D,$50
        .byte   $55,$54,$45,$52,$20,$43,$4F,$52
        .byte   $50,$2E,$4C,$54,$44,$2E,$0D,$0A
        .byte   $00,$00,$00,$20,$20,$20,$47,$2D
        .byte   $42,$41,$53,$49,$43,$20,$20,$00
LC391:  .byte   $08
LC392:  .byte   $03
LC393:  .byte   $85
LC394:  .byte   $C3,$07,$07,$6E,$81,$07,$09,$77
        .byte   $81,$07,$0B,$85,$81,$06,$12,$8C
        .byte   $81,$AF,$C3,$BF,$C3,$CF,$C3,$DF
        .byte   $C3,$EF,$C3,$0F,$2C,$15,$07,$0F
        .byte   $27,$21,$12,$0F,$29,$36,$17,$0F
        .byte   $30,$26,$07,$0F,$30,$21,$02,$0F
        .byte   $30,$27,$18,$0F,$30,$27,$16,$0F
        .byte   $29,$36,$17,$0F,$36,$16,$02,$0F
        .byte   $27,$30,$19,$0F,$35,$25,$17,$0F
        .byte   $30,$27,$16,$0F,$30,$16,$01,$0F
        .byte   $10,$00,$01,$0F,$30,$29,$09,$0F
        .byte   $30,$16,$07,$0F,$30,$26,$12,$0F
        .byte   $30,$15,$12,$0F,$30,$12,$16,$0F
        .byte   $30,$26,$19,$FF
LC400:  ldy     #$03
        ldx     #$00
LC404:  lda     $04EE,x
        sta     $04ED,x
        inx
        dey
        bne     LC404
        rts

        .byte   $77
LC410:  .byte   $00
LC411:  .byte   $00
LC412:  .byte   $07,$1F,$3F,$1F,$0F,$30,$03,$0F
        .byte   $00,$02,$04,$1E,$00,$03,$00,$00
        .byte   $E0,$F0,$F0,$F0,$C0,$40,$C0,$E0
        .byte   $E0,$50,$D0,$30,$00,$E0,$30,$00
        .byte   $04,$0F,$0F,$0C,$00,$00,$0F,$1F
        .byte   $27,$3F,$3F,$3C,$00,$00,$48,$58
        .byte   $E0,$E0,$F0,$E0,$00,$00,$F0,$E0
        .byte   $E0,$E0,$F0,$F8,$30,$60,$00,$00
        .byte   $07,$1F,$3F,$1F,$0F,$00,$03,$0F
        .byte   $00,$02,$04,$1E,$00,$00,$00,$00
        .byte   $E0,$F0,$F0,$F0,$C0,$80,$C0,$E0
        .byte   $E0,$50,$D0,$30,$00,$E0,$01,$11
        .byte   $1F,$0F,$07,$01,$00,$00,$07,$0F
        .byte   $0F,$0E,$06,$07,$0E,$01,$00,$80
        .byte   $80,$A0,$E0,$C0,$00,$00,$E0,$E0
        .byte   $60,$60,$60,$C0,$C0,$C0,$00,$00
        .byte   $00,$0F,$3F,$7F,$3F,$1F,$00,$07
        .byte   $1F,$01,$04,$09,$3C,$00,$00,$00
        .byte   $00,$C0,$E0,$E0,$E0,$80,$00,$80
        .byte   $C0,$C0,$A0,$A0,$60,$00,$01,$02
        .byte   $66,$6F,$0F,$07,$00,$00,$07,$0F
        .byte   $1F,$1D,$0F,$07,$03,$07,$00,$00
        .byte   $30,$F0,$E0,$60,$00,$00,$C0,$E0
        .byte   $C0,$C0,$F0,$78,$08,$00,$38,$38
        .byte   $31,$07,$0F,$07,$03,$02,$00,$03
        .byte   $00,$30,$31,$3F,$1C,$0F,$00,$00
        .byte   $F8,$FC,$FC,$FC,$F0,$FF,$FF,$FF
        .byte   $38,$94,$34,$8C,$00,$F8,$04,$04
        .byte   $0F,$0F,$0F,$03,$00,$00,$07,$27
        .byte   $3F,$3E,$3F,$03,$00,$00,$80,$86
        .byte   $A6,$E0,$F0,$F0,$E0,$00,$FC,$F8
        .byte   $E0,$F8,$FC,$FC,$E4,$00,$00,$00
        .byte   $00,$01,$07,$0F,$07,$03,$00,$00
        .byte   $03,$00,$00,$01,$07,$00,$00,$00
        .byte   $00,$F8,$FC,$FC,$FC,$F0,$00,$F0
        .byte   $F8,$38,$94,$34,$8C,$00,$0D,$0E
        .byte   $0A,$07,$03,$09,$04,$00,$01,$03
        .byte   $07,$0F,$17,$1F,$1F,$0D,$20,$40
        .byte   $C0,$F2,$F6,$F0,$C2,$08,$F8,$FC
        .byte   $FE,$74,$F0,$F0,$C0,$00,$00,$00
        .byte   $00,$0C,$0F,$0F,$17,$24,$03,$07
        .byte   $07,$07,$07,$0F,$00,$1F,$00,$08
        .byte   $0C,$30,$F0,$F0,$E0,$20,$C0,$E0
        .byte   $A0,$EC,$EC,$FC,$18,$F8,$04,$06
        .byte   $07,$0F,$0F,$0F,$07,$00,$3F,$3F
        .byte   $07,$0F,$0F,$0F,$07,$0E,$20,$60
        .byte   $F0,$F0,$E0,$00,$00,$00,$F0,$E0
        .byte   $F0,$F0,$F8,$30,$00,$00,$00,$00
        .byte   $07,$0F,$0F,$0F,$46,$C4,$03,$07
        .byte   $05,$01,$04,$0B,$38,$38,$00,$00
        .byte   $E0,$F4,$F6,$F7,$62,$20,$C0,$E0
        .byte   $20,$40,$20,$D8,$1C,$18,$A3,$02
        .byte   $07,$2F,$27,$17,$08,$00,$0C,$07
        .byte   $35,$1F,$1F,$0F,$00,$00,$C0,$40
        .byte   $E0,$E0,$F8,$F0,$64,$08,$30,$E0
        .byte   $A0,$E0,$F8,$FC,$78,$10,$03,$0F
        .byte   $07,$0F,$07,$03,$01,$18,$03,$0F
        .byte   $00,$02,$00,$00,$00,$03,$80,$C0
        .byte   $C0,$C8,$F0,$F8,$70,$20,$80,$C0
        .byte   $C0,$C8,$70,$F8,$F0,$E0,$18,$03
        .byte   $00,$20,$10,$18,$08,$00,$07,$03
        .byte   $07,$3F,$1F,$1F,$08,$00,$00,$C0
        .byte   $40,$00,$00,$00,$60,$C0,$80,$80
        .byte   $80,$80,$C0,$E0,$60,$C0,$03,$0F
        .byte   $07,$0F,$07,$03,$01,$00,$03,$0F
        .byte   $00,$02,$00,$00,$00,$03,$80,$C0
        .byte   $C0,$C8,$F0,$F8,$70,$20,$80,$C0
        .byte   $C0,$C8,$70,$F8,$F0,$E0,$00,$07
        .byte   $01,$01,$00,$00,$0C,$01,$07,$03
        .byte   $02,$02,$07,$0F,$0F,$01,$80,$00
        .byte   $00,$80,$00,$00,$00,$C0,$C0,$C0
        .byte   $80,$40,$C0,$E0,$E0,$C0,$00,$07
        .byte   $1F,$0F,$1F,$0F,$07,$02,$00,$07
        .byte   $1F,$01,$05,$00,$01,$01,$00,$00
        .byte   $80,$80,$90,$E0,$F0,$E0,$00,$00
        .byte   $80,$80,$90,$E0,$F0,$E0,$00,$00
        .byte   $33,$10,$00,$00,$00,$0E,$07,$0F
        .byte   $0B,$03,$03,$07,$07,$0E,$40,$00
        .byte   $80,$C0,$10,$30,$10,$00,$C0,$C0
        .byte   $50,$20,$F0,$F0,$10,$00,$19,$1F
        .byte   $1B,$07,$03,$01,$00,$00,$01,$07
        .byte   $00,$19,$0C,$0C,$07,$03,$C0,$E0
        .byte   $E0,$E4,$F8,$FC,$98,$00,$C0,$E0
        .byte   $60,$64,$38,$7C,$78,$F0,$00,$21
        .byte   $30,$10,$18,$00,$00,$00,$01,$21
        .byte   $3F,$1F,$1F,$03,$00,$00,$08,$D8
        .byte   $00,$00,$00,$30,$F8,$48,$D0,$C0
        .byte   $C0,$D0,$E0,$F0,$18,$08,$00,$01
        .byte   $07,$03,$07,$03,$01,$04,$00,$01
        .byte   $07,$00,$01,$00,$00,$00,$00,$C0
        .byte   $E0,$E0,$E4,$F8,$FC,$98,$00,$C0
        .byte   $E0,$60,$64,$38,$7C,$78,$06,$00
        .byte   $01,$00,$14,$16,$0A,$0A,$01,$03
        .byte   $01,$03,$17,$1F,$0F,$0A,$00,$08
        .byte   $D8,$00,$00,$02,$00,$10,$F0,$D0
        .byte   $C0,$C0,$E0,$F0,$C0,$00,$03,$07
        .byte   $07,$07,$07,$03,$1E,$08,$03,$07
        .byte   $07,$07,$07,$03,$07,$17,$80,$C0
        .byte   $D0,$F0,$C0,$80,$80,$00,$80,$C0
        .byte   $C0,$C0,$D0,$B0,$E0,$C0,$00,$03
        .byte   $00,$00,$00,$00,$06,$0C,$1F,$0B
        .byte   $03,$07,$0F,$0F,$06,$0C,$00,$80
        .byte   $00,$00,$30,$00,$00,$00,$80,$C0
        .byte   $E0,$E0,$F0,$80,$00,$00,$03,$07
        .byte   $07,$2F,$1F,$06,$42,$E1,$03,$07
        .byte   $05,$29,$1C,$3C,$3C,$0E,$D0,$E4
        .byte   $E6,$E7,$A0,$40,$40,$80,$D0,$E0
        .byte   $20,$48,$3C,$38,$30,$60,$20,$03
        .byte   $00,$00,$70,$18,$00,$00,$03,$03
        .byte   $0F,$1F,$7F,$1B,$00,$00,$00,$C0
        .byte   $00,$00,$00,$00,$3C,$30,$C0,$C0
        .byte   $E0,$F0,$F0,$F8,$3C,$30,$00,$00
        .byte   $20,$70,$7B,$7F,$3F,$0E,$00,$00
        .byte   $18,$05,$03,$05,$07,$07,$00,$00
        .byte   $02,$07,$6F,$7F,$7E,$38,$00,$00
        .byte   $0C,$D0,$E0,$D0,$F0,$F0,$01,$07
        .byte   $17,$20,$0F,$0B,$01,$00,$06,$0A
        .byte   $1A,$27,$03,$33,$79,$00,$C0,$F0
        .byte   $F4,$02,$F0,$E8,$D0,$00,$B0,$A8
        .byte   $AC,$F2,$E0,$E0,$CC,$1E,$00,$00
        .byte   $00,$00,$03,$07,$1F,$3E,$00,$00
        .byte   $18,$05,$03,$05,$07,$07,$00,$00
        .byte   $00,$00,$60,$70,$7C,$3E,$00,$00
        .byte   $0C,$D0,$E0,$D0,$F0,$F0,$79,$77
        .byte   $37,$20,$07,$0B,$05,$00,$06,$0A
        .byte   $1A,$27,$03,$03,$19,$3C,$CF,$F7
        .byte   $F6,$02,$F8,$E8,$C0,$00,$B0,$A8
        .byte   $AC,$F2,$E0,$E6,$CF,$00,$00,$00
        .byte   $07,$0F,$1F,$1F,$37,$B7,$00,$00
        .byte   $07,$0B,$11,$11,$39,$79,$00,$1F
        .byte   $FE,$FC,$FC,$F8,$F8,$F8,$00,$00
        .byte   $CE,$C0,$9C,$80,$18,$38,$3F,$3F
        .byte   $1F,$1F,$0F,$07,$00,$00,$FB,$3F
        .byte   $1F,$1F,$0F,$07,$00,$00,$E0,$D8
        .byte   $DC,$ED,$E2,$C0,$00,$00,$F8,$E0
        .byte   $80,$F3,$00,$C3,$01,$00,$00,$07
        .byte   $0F,$1F,$1F,$37,$B7,$3F,$00,$07
        .byte   $0B,$11,$11,$39,$79,$FF,$00,$C0
        .byte   $E0,$F0,$F0,$F8,$F8,$F0,$00,$C0
        .byte   $E0,$F0,$F0,$F8,$38,$3B,$3F,$1F
        .byte   $1F,$0F,$07,$00,$00,$00,$3F,$1F
        .byte   $1F,$0F,$07,$00,$00,$00,$ED,$EE
        .byte   $A0,$F8,$80,$3C,$00,$00,$13,$13
        .byte   $03,$81,$98,$00,$00,$00,$00,$00
        .byte   $27,$0B,$1D,$0F,$37,$3F,$00,$00
        .byte   $10,$3C,$1E,$1E,$0F,$03,$1C,$3E
        .byte   $FF,$F0,$F0,$F0,$F8,$F8,$00,$00
        .byte   $C0,$60,$70,$70,$F8,$F8,$3F,$3F
        .byte   $3F,$FF,$FF,$E7,$60,$20,$23,$3F
        .byte   $1F,$0F,$0F,$07,$00,$00,$F8,$FC
        .byte   $A1,$F2,$A0,$E0,$08,$10,$F8,$FB
        .byte   $A3,$F6,$A0,$D8,$38,$30,$00,$27
        .byte   $0B,$1D,$0F,$37,$3F,$3F,$00,$10
        .byte   $3C,$1E,$1E,$0F,$03,$23,$00,$C0
        .byte   $F8,$FC,$AA,$FA,$F8,$F8,$00,$C0
        .byte   $60,$70,$70,$F8,$F8,$F8,$3F,$1F
        .byte   $1F,$1F,$1F,$1C,$0C,$06,$3F,$1F
        .byte   $1F,$0F,$07,$00,$00,$00,$FC,$FD
        .byte   $AA,$E2,$E0,$60,$10,$20,$FB,$F3
        .byte   $F6,$E6,$C0,$10,$70,$60,$00,$00
        .byte   $83,$E5,$6D,$3F,$3F,$1F,$01,$01
        .byte   $01,$03,$0B,$09,$0B,$0F,$80,$00
        .byte   $83,$A7,$B6,$FC,$A8,$F8,$00,$80
        .byte   $80,$C0,$D0,$90,$D0,$F0,$1F,$1F
        .byte   $0F,$0F,$0F,$07,$00,$06,$1F,$1F
        .byte   $0F,$0F,$07,$03,$0E,$0E,$F8,$F8
        .byte   $A0,$F0,$F0,$E0,$00,$60,$F8,$F8
        .byte   $A0,$F0,$E0,$C0,$70,$70,$00,$00
        .byte   $03,$05,$0D,$0F,$1F,$3F,$01,$01
        .byte   $01,$03,$0B,$09,$1B,$1F,$80,$00
        .byte   $C0,$A0,$B0,$F0,$F8,$FC,$00,$80
        .byte   $80,$C0,$D0,$90,$D8,$F8,$7F,$FF
        .byte   $EF,$CF,$82,$87,$30,$1C,$1F,$1F
        .byte   $4F,$4F,$47,$1B,$3C,$1C,$FE,$FF
        .byte   $A3,$F3,$A3,$E1,$0C,$38,$F8,$F8
        .byte   $A2,$F2,$A2,$D8,$3C,$38,$00,$07
        .byte   $0F,$1F,$3D,$3D,$7F,$FF,$00,$01
        .byte   $09,$19,$2B,$0B,$59,$7F,$00,$E0
        .byte   $F0,$F8,$BC,$BC,$FE,$FF,$00,$80
        .byte   $90,$98,$DC,$DC,$9E,$FE,$FD,$6D
        .byte   $6D,$2D,$3D,$1D,$0F,$07,$78,$E0
        .byte   $80,$E0,$30,$18,$0F,$07,$BF,$B6
        .byte   $B6,$B4,$BC,$B8,$F0,$E0,$1E,$07
        .byte   $03,$07,$0C,$18,$F0,$E0,$00,$07
        .byte   $0D,$1D,$3F,$3F,$7F,$FF,$00,$01
        .byte   $0B,$1B,$29,$09,$59,$7F,$00,$E0
        .byte   $B0,$B8,$FC,$FC,$FE,$FF,$00,$80
        .byte   $D0,$D8,$9C,$9C,$9E,$FE,$FD,$6D
        .byte   $6F,$3F,$3F,$1F,$0F,$07,$78,$E0
        .byte   $83,$DF,$3F,$1F,$0F,$07,$BF,$B6
        .byte   $F6,$FC,$FC,$F8,$F0,$E0,$1E,$07
        .byte   $83,$FB,$FC,$F8,$F0,$E0,$0F,$1F
        .byte   $1F,$3F,$2F,$2F,$03,$01,$0F,$13
        .byte   $01,$00,$00,$01,$7F,$FF,$00,$80
        .byte   $C0,$C0,$E0,$E0,$A0,$F0,$00,$80
        .byte   $C0,$C0,$E0,$E0,$A0,$F0,$23,$3F
        .byte   $3F,$3F,$38,$1C,$07,$00,$1D,$00
        .byte   $00,$00,$07,$03,$18,$7E,$FE,$FC
        .byte   $F8,$F0,$A0,$78,$20,$00,$FE,$FC
        .byte   $78,$70,$70,$F8,$E0,$00,$01,$03
        .byte   $07,$0D,$0B,$03,$00,$00,$01,$00
        .byte   $00,$00,$00,$3C,$1F,$0F,$C0,$E0
        .byte   $F0,$F0,$A0,$F0,$A0,$F8,$80,$E0
        .byte   $70,$30,$30,$70,$F0,$F8,$1D,$1F
        .byte   $3F,$3F,$1F,$0F,$07,$00,$03,$00
        .byte   $00,$00,$00,$70,$38,$01,$F8,$FC
        .byte   $82,$FE,$A0,$F0,$F8,$10,$F8,$FC
        .byte   $7E,$7E,$70,$F0,$F8,$F0,$07,$0F
        .byte   $0F,$1F,$1D,$1D,$1C,$30,$07,$0F
        .byte   $09,$18,$10,$10,$13,$3F,$C0,$E0
        .byte   $A0,$F0,$70,$70,$7C,$1E,$C0,$E0
        .byte   $20,$10,$10,$10,$BC,$FE,$38,$7F
        .byte   $2B,$EF,$0F,$0F,$01,$00,$3F,$78
        .byte   $78,$E8,$00,$00,$1E,$3F,$3F,$F8
        .byte   $A8,$F8,$F8,$E0,$80,$00,$FF,$18
        .byte   $08,$00,$06,$1C,$78,$00,$07,$0F
        .byte   $0F,$1F,$1F,$1F,$1F,$3F,$07,$0F
        .byte   $0F,$1F,$1F,$1F,$1F,$3F,$80,$E0
        .byte   $A0,$F0,$80,$F0,$FC,$FF,$80,$E0
        .byte   $A0,$F0,$F0,$F0,$FC,$FF,$3F,$7F
        .byte   $7F,$FF,$0F,$0F,$07,$00,$3F,$7F
        .byte   $2B,$EF,$0F,$0F,$1F,$3E,$FF,$F8
        .byte   $F8,$F8,$80,$F0,$C0,$80,$FF,$F8
        .byte   $F8,$F8,$FE,$FC,$F8,$80,$00,$00
        .byte   $1E,$33,$60,$45,$45,$4F,$00,$00
        .byte   $1E,$3F,$7F,$70,$70,$70,$00,$04
        .byte   $00,$90,$C0,$49,$48,$4C,$00,$04
        .byte   $00,$90,$C0,$C9,$C8,$CC,$2E,$24
        .byte   $60,$60,$70,$3C,$1F,$07,$31,$3B
        .byte   $7F,$7F,$7F,$3F,$1F,$07,$DC,$B4
        .byte   $6C,$08,$18,$38,$30,$E0,$DC,$BC
        .byte   $FC,$F8,$80,$F8,$F0,$E0,$00,$00
        .byte   $1F,$31,$60,$55,$55,$2F,$00,$00
        .byte   $1F,$3F,$7F,$60,$60,$30,$00,$00
        .byte   $00,$80,$80,$C0,$40,$42,$00,$00
        .byte   $00,$80,$80,$C0,$C0,$C2,$6F,$4F
        .byte   $C6,$C0,$C0,$E0,$70,$1F,$70,$70
        .byte   $A9,$FF,$FF,$FF,$7F,$1F,$46,$5E
        .byte   $7C,$64,$08,$18,$70,$C0,$C6,$DE
        .byte   $FC,$FC,$F8,$F8,$F0,$C0,$00,$00
        .byte   $38,$38,$38,$10,$03,$27,$00,$00
        .byte   $18,$18,$18,$03,$3C,$79,$00,$F8
        .byte   $F8,$FE,$2E,$0E,$5E,$7E,$00,$78
        .byte   $78,$70,$00,$F0,$A0,$F0,$27,$27
        .byte   $03,$10,$38,$38,$38,$00,$79,$79
        .byte   $3C,$03,$18,$18,$18,$00,$7E,$7E
        .byte   $5E,$0E,$2E,$FE,$A8,$F8,$80,$F0
        .byte   $A0,$F0,$00,$70,$78,$78,$00,$00
        .byte   $38,$38,$38,$10,$03,$27,$00,$00
        .byte   $30,$30,$30,$03,$3C,$79,$00,$F8
        .byte   $F8,$FE,$2E,$0E,$5E,$7E,$00,$98
        .byte   $98,$90,$00,$F0,$A0,$F0,$27,$27
        .byte   $03,$10,$38,$38,$38,$00,$79,$79
        .byte   $3C,$03,$30,$30,$30,$00,$7E,$7E
        .byte   $5E,$0E,$2E,$FE,$80,$F8,$A0,$F0
        .byte   $A0,$F0,$00,$90,$98,$98,$01,$03
        .byte   $01,$08,$10,$07,$47,$E7,$00,$01
        .byte   $1D,$3E,$3F,$38,$19,$6B,$00,$80
        .byte   $C0,$8C,$1E,$1F,$8F,$06,$00,$80
        .byte   $C0,$80,$06,$CF,$6F,$F2,$72,$20
        .byte   $00,$0C,$1E,$1F,$0F,$06,$75,$27
        .byte   $03,$01,$06,$0E,$0F,$06,$6E,$FF
        .byte   $FE,$7C,$F8,$F0,$E0,$40,$D0,$E0
        .byte   $40,$80,$00,$00,$00,$00,$01,$03
        .byte   $01,$08,$10,$07,$47,$E7,$01,$03
        .byte   $1D,$3E,$3F,$38,$59,$FF,$00,$80
        .byte   $C0,$8C,$1E,$1F,$8F,$06,$00,$80
        .byte   $80,$0C,$18,$D3,$67,$F2,$72,$20
        .byte   $00,$0C,$1E,$1F,$0F,$06,$65,$07
        .byte   $03,$0D,$18,$12,$07,$06,$6E,$FF
        .byte   $FE,$7C,$F8,$F0,$E0,$40,$D0,$E0
        .byte   $40,$80,$00,$00,$00,$00,$00,$18
        .byte   $1D,$1C,$18,$01,$03,$03,$01,$03
        .byte   $1B,$1B,$1B,$02,$04,$05,$00,$18
        .byte   $B8,$38,$18,$80,$C0,$C0,$80,$C0
        .byte   $D8,$D8,$D8,$40,$20,$A0,$73,$70
        .byte   $7B,$79,$73,$7F,$1F,$1F,$05,$77
        .byte   $75,$77,$75,$60,$00,$00,$CE,$0E
        .byte   $DE,$9E,$CE,$FE,$F8,$F8,$A0,$EE
        .byte   $AE,$EE,$AE,$06,$00,$00,$00,$18
        .byte   $1D,$1C,$18,$01,$03,$03,$01,$1B
        .byte   $1B,$1B,$03,$02,$04,$05,$00,$18
        .byte   $B8,$38,$18,$80,$C0,$C0,$80,$D8
        .byte   $D8,$D8,$C0,$40,$20,$A0,$73,$70
        .byte   $7B,$79,$73,$7F,$1F,$1F,$75,$07
        .byte   $05,$77,$75,$60,$00,$00,$CE,$0E
        .byte   $DE,$9E,$CE,$FE,$F8,$F8,$AE,$E0
        .byte   $A0,$EE,$AE,$06,$00,$00,$07,$00
        .byte   $00,$01,$01,$C0,$A1,$FB,$07,$07
        .byte   $03,$01,$01,$87,$86,$9D,$E0,$00
        .byte   $00,$80,$80,$03,$87,$FF,$E0,$E0
        .byte   $C0,$80,$80,$E1,$61,$B9,$DB,$81
        .byte   $80,$01,$01,$03,$07,$07,$BD,$E6
        .byte   $83,$01,$01,$00,$00,$07,$83,$81
        .byte   $01,$80,$80,$C0,$A0,$E0,$BD,$67
        .byte   $A3,$80,$80,$00,$00,$E0,$00,$10
        .byte   $38,$78,$B8,$1C,$09,$03,$00,$10
        .byte   $20,$40,$C9,$7F,$0E,$0D,$00,$10
        .byte   $38,$3C,$3A,$70,$20,$80,$00,$10
        .byte   $08,$04,$26,$FC,$A0,$60,$07,$03
        .byte   $09,$7C,$F8,$60,$20,$10,$1B,$0D
        .byte   $0E,$1F,$89,$58,$38,$10,$C0,$80
        .byte   $20,$7C,$3E,$0C,$08,$10,$B0,$60
        .byte   $E0,$F0,$22,$34,$38,$10,$03,$07
        .byte   $07,$0F,$1F,$3F,$7F,$FF,$01,$01
        .byte   $01,$0F,$00,$0E,$14,$1C,$E0,$F8
        .byte   $FC,$FC,$FE,$80,$FE,$D4,$00,$00
        .byte   $00,$FC,$00,$7E,$7E,$7E,$7F,$3D
        .byte   $1E,$00,$0F,$07,$01,$01,$1C,$5E
        .byte   $2F,$1F,$0F,$01,$07,$03,$D4,$FE
        .byte   $7E,$00,$FC,$FC,$00,$00,$7E,$7E
        .byte   $80,$FE,$80,$00,$F8,$E0,$00,$0F
        .byte   $1F,$3F,$1F,$1F,$0F,$27,$00,$01
        .byte   $00,$03,$25,$2E,$3C,$3C,$F0,$F8
        .byte   $FC,$FC,$FE,$FE,$DE,$AE,$00,$88
        .byte   $50,$20,$90,$08,$24,$72,$31,$30
        .byte   $19,$1C,$26,$03,$01,$00,$5E,$5F
        .byte   $6E,$77,$3B,$1D,$0E,$03,$76,$EA
        .byte   $D4,$EC,$78,$30,$90,$C0,$F8,$FC
        .byte   $FE,$7E,$BC,$D8,$E0,$40,$01,$03
        .byte   $03,$07,$37,$37,$35,$73,$02,$04
        .byte   $04,$0B,$1F,$5F,$5E,$7C,$C0,$E0
        .byte   $A0,$F0,$FC,$FE,$AA,$FE,$00,$00
        .byte   $00,$C0,$A8,$E8,$28,$0E,$33,$37
        .byte   $37,$36,$37,$36,$17,$06,$5C,$5B
        .byte   $5B,$5B,$1B,$1B,$1B,$03,$FE,$DE
        .byte   $8A,$5E,$DC,$5C,$D8,$40,$08,$E8
        .byte   $A8,$E8,$A8,$E8,$E8,$E0,$03,$07
        .byte   $01,$01,$03,$07,$3F,$FF,$00,$00
        .byte   $01,$00,$03,$08,$06,$1C,$60,$70
        .byte   $C0,$E0,$80,$FA,$FA,$82,$80,$80
        .byte   $C0,$00,$F0,$1E,$0E,$7E,$7F,$3F
        .byte   $07,$03,$01,$01,$07,$03,$1C,$06
        .byte   $08,$03,$00,$01,$00,$00,$82,$FA
        .byte   $AA,$F0,$E0,$C0,$70,$60,$7E,$0E
        .byte   $1E,$F0,$00,$C0,$80,$80,$00,$00
        .byte   $38,$3C,$3F,$1F,$1F,$0F,$00,$00
        .byte   $00,$02,$0C,$0F,$0C,$14,$00,$70
        .byte   $38,$30,$F6,$FE,$F2,$70,$00,$00
        .byte   $00,$2C,$98,$48,$20,$90,$0E,$0E
        .byte   $4F,$7F,$7F,$27,$0C,$0E,$05,$09
        .byte   $04,$12,$09,$1C,$10,$00,$38,$10
        .byte   $00,$84,$C8,$90,$20,$00,$D8,$F8
        .byte   $FC,$7C,$F8,$F0,$20,$00,$01,$01
        .byte   $03,$03,$03,$47,$8B,$FF,$00,$00
        .byte   $00,$01,$05,$03,$0B,$2A,$80,$80
        .byte   $C0,$C0,$C0,$E2,$A3,$FF,$00,$00
        .byte   $00,$80,$A0,$C0,$D0,$54,$FE,$3E
        .byte   $FE,$DE,$4E,$06,$00,$07,$28,$E9
        .byte   $29,$09,$0D,$07,$07,$07,$FF,$7C
        .byte   $7F,$7B,$72,$60,$00,$E0,$14,$97
        .byte   $94,$90,$B0,$E0,$E0,$E0,$00,$00
        .byte   $00,$00,$01,$01,$03,$3F,$00,$00
        .byte   $00,$08,$10,$06,$0C,$00,$00,$00
        .byte   $00,$00,$00,$00,$80,$F8,$00,$00
        .byte   $00,$20,$10,$C0,$60,$00,$03,$01
        .byte   $01,$00,$00,$00,$00,$00,$0C,$06
        .byte   $10,$08,$00,$00,$00,$00,$80,$00
        .byte   $00,$00,$00,$00,$00,$00,$60,$C0
        .byte   $10,$20,$00,$00,$00,$00,$18,$B2
        .byte   $22,$02,$43,$33,$5F,$8F,$18,$B0
        .byte   $21,$15,$0C,$0C,$40,$B0,$60,$F4
        .byte   $18,$18,$32,$63,$81,$F2,$60,$F0
        .byte   $10,$A0,$CA,$93,$19,$0A,$9F,$9F
        .byte   $5B,$7F,$35,$61,$89,$26,$A0,$E0
        .byte   $64,$40,$0A,$16,$09,$26,$B9,$E2
        .byte   $F0,$DA,$A0,$84,$96,$8C,$41,$1A
        .byte   $0C,$24,$50,$24,$16,$0C,$08,$1C
        .byte   $1C,$0C,$0C,$18,$00,$00,$00,$00
        .byte   $20,$62,$62,$A6,$FE,$FC,$00,$00
        .byte   $00,$00,$00,$78,$B4,$CE,$00,$00
        .byte   $00,$00,$00,$70,$B0,$CC,$01,$03
        .byte   $02,$03,$03,$01,$00,$00,$CD,$0D
        .byte   $1C,$3D,$0C,$02,$0F,$1E,$B6,$7A
        .byte   $B4,$CE,$A3,$FE,$FC,$00,$B4,$7A
        .byte   $B5,$CE,$B4,$78,$02,$07,$08,$1C
        .byte   $1C,$0C,$0C,$18,$00,$01,$00,$00
        .byte   $20,$62,$62,$A6,$FE,$FF,$00,$00
        .byte   $00,$00,$78,$B4,$CE,$B6,$00,$00
        .byte   $00,$00,$70,$B0,$CC,$B4,$03,$02
        .byte   $03,$03,$01,$00,$00,$00,$DD,$3C
        .byte   $1D,$0C,$00,$03,$03,$01,$7A,$B4
        .byte   $CE,$B7,$FE,$FC,$00,$00,$7A,$B5
        .byte   $CE,$B4,$78,$02,$8C,$DC,$30,$66
        .byte   $D6,$D4,$F4,$E6,$82,$47,$30,$60
        .byte   $D0,$D2,$F2,$E0,$C2,$47,$00,$C0
        .byte   $CC,$4E,$57,$DB,$9F,$DF,$00,$00
        .byte   $00,$8C,$96,$1A,$9F,$CF,$65,$37
        .byte   $07,$37,$43,$50,$54,$54,$67,$3F
        .byte   $0F,$7F,$4F,$57,$54,$54,$43,$FE
        .byte   $A0,$F8,$82,$1A,$28,$20,$83,$DE
        .byte   $A0,$FC,$E2,$DA,$28,$20,$06,$06
        .byte   $34,$64,$D6,$D2,$00,$FF,$00,$00
        .byte   $32,$62,$D0,$D2,$00,$FF,$DC,$CE
        .byte   $56,$5B,$8B,$8F,$83,$46,$00,$0C
        .byte   $94,$9A,$1E,$8F,$80,$88,$D7,$67
        .byte   $17,$43,$D0,$A8,$08,$00,$DF,$6F
        .byte   $3F,$4F,$D7,$A8,$08,$00,$FC,$E0
        .byte   $F8,$C8,$14,$54,$54,$94,$DC,$E0
        .byte   $F8,$EC,$D4,$54,$54,$94,$00,$00
        .byte   $01,$03,$0E,$06,$02,$03,$00,$00
        .byte   $00,$00,$0C,$1C,$34,$7E,$00,$00
        .byte   $80,$C0,$F0,$FD,$AB,$FF,$00,$00
        .byte   $00,$00,$30,$3D,$3F,$FF,$03,$03
        .byte   $03,$07,$07,$00,$00,$00,$FF,$0F
        .byte   $7E,$1F,$07,$00,$00,$00,$FF,$FF
        .byte   $FE,$FC,$FE,$3F,$0E,$00,$CF,$FF
        .byte   $E6,$7F,$B0,$3F,$0E,$00,$00,$00
        .byte   $00,$01,$03,$0E,$06,$02,$00,$00
        .byte   $00,$00,$00,$FC,$7C,$34,$1E,$3F
        .byte   $7F,$FE,$FC,$FD,$89,$FF,$1E,$3F
        .byte   $78,$7E,$30,$3D,$3D,$3F,$03,$03
        .byte   $03,$07,$0F,$03,$00,$00,$1E,$0F
        .byte   $1F,$3F,$7F,$03,$00,$00,$FF,$FF
        .byte   $FE,$F0,$80,$F0,$00,$00,$7F,$FF
        .byte   $FE,$FF,$FA,$F0,$00,$00,$00,$00
        .byte   $00,$00,$18,$00,$00,$00,$00,$00
        .byte   $00,$18,$24,$18,$00,$00,$00,$00
        .byte   $3C,$42,$BD,$42,$3C,$00,$00,$00
        .byte   $3C,$7E,$83,$7E,$3C,$00,$00,$00
        .byte   $00,$10,$08,$00,$00,$00,$00,$00
        .byte   $30,$28,$14,$0C,$00,$00,$00,$38
        .byte   $64,$5A,$5A,$26,$1C,$00,$00,$78
        .byte   $5C,$66,$66,$3A,$1E,$00,$00,$00
        .byte   $00,$10,$10,$00,$00,$00,$00,$00
        .byte   $10,$28,$28,$10,$00,$00,$10,$28
        .byte   $54,$54,$54,$54,$28,$10,$10,$38
        .byte   $6C,$6C,$6C,$6C,$38,$10,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$80,$80
        .byte   $80,$80,$80,$80,$80,$7F,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$01
        .byte   $01,$01,$01,$01,$01,$01,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$FE
        .byte   $82,$82,$82,$FE,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$FE
        .byte   $FE,$FE,$FE,$FE,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$3C,$24
        .byte   $24,$24,$24,$3C,$00,$00,$3C,$3C
        .byte   $3C,$3C,$3C,$3C,$00,$00,$00,$00
        .byte   $00,$10,$00,$00,$00,$00,$00,$00
        .byte   $18,$2C,$3C,$18,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$C0,$60
        .byte   $30,$10,$18,$0C,$04,$06,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$05,$04
        .byte   $0E,$0F,$0F,$07,$07,$07,$02,$03
        .byte   $01,$00,$00,$00,$00,$00,$80,$C0
        .byte   $E0,$70,$78,$B8,$BC,$DC,$00,$00
        .byte   $00,$80,$80,$60,$58,$A4,$03,$03
        .byte   $03,$01,$01,$00,$00,$00,$00,$00
        .byte   $01,$01,$01,$00,$00,$00,$DC,$EE
        .byte   $82,$F6,$F7,$F7,$7B,$7B,$A0,$98
        .byte   $14,$2A,$28,$2C,$4E,$4E,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$3B,$1B
        .byte   $1F,$0F,$0F,$07,$06,$02,$0F,$1F
        .byte   $1F,$0F,$0F,$07,$06,$02,$00,$00
        .byte   $00,$00,$00,$00,$01,$03,$00,$00
        .byte   $00,$00,$00,$00,$01,$01,$03,$0F
        .byte   $1E,$3E,$76,$EE,$DC,$DC,$03,$0F
        .byte   $1E,$3E,$3E,$3E,$3C,$20,$07,$07
        .byte   $0F,$0F,$1E,$1E,$1C,$3D,$01,$01
        .byte   $08,$08,$05,$05,$03,$02,$BC,$B8
        .byte   $78,$70,$F0,$F0,$A0,$E0,$40,$40
        .byte   $F8,$80,$00,$00,$E0,$00,$39,$3B
        .byte   $33,$37,$27,$0E,$00,$00,$06,$04
        .byte   $0C,$08,$18,$10,$10,$30,$E0,$C0
        .byte   $C0,$80,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$20,$20
        .byte   $60,$40,$40,$C0,$80,$80,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$70,$70
        .byte   $30,$30,$30,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$78,$18
        .byte   $78,$60,$78,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$78,$18
        .byte   $78,$18,$78,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$B0,$B0
        .byte   $B0,$F8,$30,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$3D
        .byte   $31,$31,$31,$3D,$3D,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$F4
        .byte   $B6,$B7,$B6,$F6,$F6,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$80,$FF,$00,$5E
        .byte   $DB,$DF,$DE,$D8,$D8,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$6D
        .byte   $6C,$6C,$6C,$7C,$7C,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$EF
        .byte   $CC,$CF,$CC,$CF,$83,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$78
        .byte   $6C,$7C,$78,$6C,$64,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$3E
        .byte   $36,$36,$36,$3E,$3E,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$F3
        .byte   $8B,$FB,$A3,$C3,$80,$00,$AB,$FF
        .byte   $BB,$FF,$AB,$FF,$AB,$FF,$00,$DE
        .byte   $1B,$DF,$1E,$DB,$89,$00,$AB,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$39
        .byte   $6C,$6C,$7C,$7C,$6C,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$EF
        .byte   $CD,$CD,$81,$CF,$8F,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$BC
        .byte   $B6,$BE,$BC,$B6,$B2,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$11,$FC
        .byte   $00,$F8,$F8,$00,$00,$00,$00,$01
        .byte   $03,$FB,$83,$03,$FD,$FF,$7F,$BF
        .byte   $80,$FF,$DF,$C0,$80,$00,$00,$00
        .byte   $00,$1F,$1F,$00,$3F,$FF,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$FC,$FC
        .byte   $FC,$FC,$FC,$FC,$A8,$FC,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$E0,$80,$20
        .byte   $A0,$00,$80,$20,$B0,$18,$70,$DC
        .byte   $47,$C4,$44,$C4,$44,$C0,$8B,$27
        .byte   $BC,$3F,$BF,$3F,$BF,$3B,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$C0,$B8,$30
        .byte   $A0,$00,$80,$20,$A0,$00,$00,$92
        .byte   $FF,$92,$92,$92,$92,$00,$6D,$FF
        .byte   $92,$FF,$FF,$FF,$FF,$FF,$11,$11
        .byte   $11,$11,$11,$11,$11,$11,$0C,$0C
        .byte   $0C,$0C,$0C,$0C,$0C,$0C,$11,$51
        .byte   $A1,$51,$51,$51,$51,$11,$AC,$EC
        .byte   $4C,$EC,$EC,$EC,$EC,$AC,$00,$00
        .byte   $00,$01,$07,$0C,$08,$18,$00,$00
        .byte   $00,$00,$00,$03,$07,$07,$10,$10
        .byte   $10,$10,$10,$10,$10,$11,$0F,$0F
        .byte   $0F,$0F,$0F,$0F,$0C,$0C,$11,$10
        .byte   $10,$10,$10,$10,$10,$10,$0C,$0C
        .byte   $0F,$0F,$0F,$0F,$0F,$0F,$18,$08
        .byte   $0C,$07,$01,$00,$00,$00,$07,$07
        .byte   $03,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$AB
        .byte   $FF,$FF,$FF,$FF,$00,$00,$FF,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $83,$FF,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$FF,$00,$00,$00,$FF,$FF
        .byte   $AB,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$80,$E0,$30,$10,$18,$00,$00
        .byte   $00,$00,$00,$C0,$E0,$E0,$08,$08
        .byte   $08,$08,$08,$08,$08,$88,$F0,$F0
        .byte   $A0,$F0,$F0,$F0,$30,$30,$88,$88
        .byte   $88,$88,$88,$88,$88,$88,$30,$30
        .byte   $30,$30,$30,$30,$30,$30,$88,$8A
        .byte   $8F,$8A,$8A,$8A,$8A,$88,$35,$37
        .byte   $32,$37,$37,$37,$37,$35,$88,$08
        .byte   $08,$08,$08,$08,$08,$08,$30,$30
        .byte   $F0,$F0,$A0,$F0,$F0,$F0,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$3F,$3F
        .byte   $3F,$3F,$3F,$3F,$3F,$3F,$00,$49
        .byte   $FF,$49,$49,$49,$49,$00,$B6,$FF
        .byte   $49,$FF,$FF,$FF,$AB,$B6,$18,$10
        .byte   $30,$E0,$80,$00,$00,$00,$E0,$E0
        .byte   $C0,$00,$00,$00,$00,$00,$02,$03
        .byte   $02,$03,$02,$03,$02,$07,$01,$04
        .byte   $05,$00,$01,$04,$0D,$18,$0E,$3B
        .byte   $A2,$23,$22,$23,$22,$03,$81,$E4
        .byte   $3D,$FC,$A9,$FC,$A9,$DC,$02,$03
        .byte   $02,$03,$02,$03,$02,$03,$1D,$0C
        .byte   $05,$00,$01,$04,$05,$00,$00,$00
        .byte   $18,$3C,$3C,$18,$00,$00,$FF,$C3
        .byte   $99,$AD,$BD,$99,$C3,$FF,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$00,$3F
        .byte   $3F,$3F,$3F,$3F,$3F,$3F,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7F,$7F
        .byte   $7F,$7F,$7F,$7F,$7F,$7F,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$00,$FC
        .byte   $FC,$FC,$FC,$FC,$FC,$FC,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0C,$0C
        .byte   $18,$10,$00,$60,$60,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$28,$28
        .byte   $28,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$08,$2E
        .byte   $F8,$28,$3E,$E8,$20,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$7C
        .byte   $90,$FE,$12,$FC,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$42,$A4
        .byte   $A8,$54,$2A,$4A,$84,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$18,$28
        .byte   $28,$10,$6A,$44,$3A,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$30,$10
        .byte   $20,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0C,$10
        .byte   $20,$20,$20,$10,$0C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$30,$08
        .byte   $04,$04,$04,$08,$30,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$54
        .byte   $38,$10,$38,$54,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$10
        .byte   $10,$7C,$10,$10,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$30,$10,$20,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$7C,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$30,$30,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$02,$04
        .byte   $08,$10,$20,$40,$80,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $8A,$92,$A2,$C6,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$30
        .byte   $10,$10,$10,$10,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $02,$1C,$70,$C0,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$04
        .byte   $18,$0C,$02,$C6,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$1C,$34
        .byte   $64,$C4,$84,$FE,$04,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FC,$80
        .byte   $FC,$06,$02,$C6,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C0
        .byte   $80,$FC,$86,$C2,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$86
        .byte   $0C,$18,$10,$10,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$86
        .byte   $C2,$7C,$86,$C2,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$86
        .byte   $C2,$7E,$02,$06,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$30
        .byte   $30,$00,$30,$30,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$30
        .byte   $30,$00,$30,$10,$20,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$08,$10
        .byte   $20,$40,$20,$10,$08,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $7C,$00,$7C,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$20,$10
        .byte   $08,$04,$08,$10,$20,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$3C,$66
        .byte   $46,$0C,$08,$00,$18,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$82
        .byte   $9A,$AA,$BC,$80,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$38,$6C
        .byte   $C6,$82,$FE,$82,$82,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FC,$86
        .byte   $82,$FC,$86,$82,$FC,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $80,$80,$80,$C6,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$F8,$8C
        .byte   $86,$82,$86,$8C,$F8,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$80
        .byte   $80,$FC,$80,$80,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$80
        .byte   $80,$FC,$80,$80,$80,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $82,$80,$8E,$C2,$7E,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$FE,$82,$82,$82,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$10
        .byte   $10,$10,$10,$10,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$1E,$04
        .byte   $04,$04,$04,$CC,$78,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$86,$8C
        .byte   $98,$F0,$98,$8C,$86,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$40,$40
        .byte   $40,$40,$40,$40,$7E,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$C6
        .byte   $FE,$BA,$92,$82,$82,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$C2
        .byte   $E2,$B2,$9A,$8E,$86,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $82,$82,$82,$C6,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FC,$86
        .byte   $82,$86,$FC,$80,$80,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $82,$92,$8E,$C4,$7A,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FC,$86
        .byte   $82,$86,$F8,$8C,$86,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7C,$C0
        .byte   $80,$7C,$06,$02,$FC,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$10
        .byte   $10,$10,$10,$10,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$82,$82,$C6,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$C6,$6C,$38,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$92,$BA,$EE,$C6,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$C6
        .byte   $6C,$38,$6C,$C6,$82,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$82,$C6
        .byte   $6C,$38,$10,$10,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FE,$0C
        .byte   $18,$30,$60,$C0,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$1C
        .byte   $10,$10,$10,$10,$1C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$6C,$28
        .byte   $7C,$10,$7C,$10,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$0E
        .byte   $02,$02,$02,$02,$0E,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$28
        .byte   $44,$82,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$FE,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$20,$38
        .byte   $18,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$0C,$7C,$CC,$76,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$E0,$60
        .byte   $60,$7C,$66,$66,$DC,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$CC,$C0,$CC,$78,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$1C,$0C
        .byte   $0C,$7C,$CC,$CC,$76,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$CC,$FC,$C0,$78,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$38,$6C
        .byte   $60,$F0,$60,$60,$F0,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $76,$CC,$CC,$7C,$0C,$F8,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$E0,$60
        .byte   $6C,$76,$66,$66,$E6,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$30,$00
        .byte   $70,$30,$30,$30,$78,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0C,$00
        .byte   $0C,$0C,$0C,$CC,$CC,$78,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$E0,$60
        .byte   $66,$6C,$78,$6C,$E6,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$70,$30
        .byte   $30,$30,$30,$30,$78,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$FE,$FE,$D6,$C6,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $F8,$CC,$CC,$CC,$CC,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$CC,$CC,$CC,$78,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $DC,$66,$66,$7C,$60,$F0,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $76,$CC,$CC,$7C,$0C,$1E,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $DC,$76,$66,$60,$F0,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $7C,$C0,$78,$0C,$F8,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$30
        .byte   $7C,$30,$30,$34,$18,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$CC,$CC,$CC,$76,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$CC,$CC,$78,$30,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $C6,$D6,$FE,$FE,$6C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $C6,$6C,$38,$6C,$C6,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$CC,$CC,$7C,$0C,$F8,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $FC,$98,$30,$64,$FC,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$1C,$30
        .byte   $30,$E0,$30,$30,$1C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$18,$18
        .byte   $18,$00,$18,$18,$18,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$E0,$30
        .byte   $30,$1C,$30,$30,$E0,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$76,$DC
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$10
        .byte   $38,$6C,$C6,$C6,$FE,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$11,$FC
        .byte   $00,$F8,$F8,$00,$00,$00,$00,$01
        .byte   $03,$FB,$83,$03,$FD,$FF,$7F,$BF
        .byte   $80,$FF,$DF,$C0,$80,$00,$00,$00
        .byte   $00,$1F,$1F,$00,$3F,$FF,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$FC,$FC
        .byte   $FC,$FC,$FC,$FC,$A8,$FC,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$E0,$80,$20
        .byte   $A0,$00,$80,$20,$B0,$18,$70,$DC
        .byte   $47,$C4,$44,$C4,$44,$C0,$8B,$27
        .byte   $BC,$3F,$BF,$3F,$BF,$3B,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$C0,$B8,$30
        .byte   $A0,$00,$80,$20,$A0,$00,$00,$92
        .byte   $FF,$92,$92,$92,$92,$00,$6D,$FF
        .byte   $92,$FF,$FF,$FF,$FF,$FF,$11,$11
        .byte   $11,$11,$11,$11,$11,$11,$0C,$0C
        .byte   $0C,$0C,$0C,$0C,$0C,$0C,$11,$51
        .byte   $A1,$51,$51,$51,$51,$11,$AC,$EC
        .byte   $4C,$EC,$EC,$EC,$EC,$AC,$00,$00
        .byte   $00,$01,$07,$0C,$08,$18,$00,$00
        .byte   $00,$00,$00,$03,$07,$07,$10,$10
        .byte   $10,$10,$10,$10,$10,$11,$0F,$0F
        .byte   $0F,$0F,$0F,$0F,$0C,$0C,$11,$10
        .byte   $10,$10,$10,$10,$10,$10,$0C,$0C
        .byte   $0F,$0F,$0F,$0F,$0F,$0F,$18,$08
        .byte   $0C,$07,$01,$00,$00,$00,$07,$07
        .byte   $03,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$AB
        .byte   $FF,$FF,$FF,$FF,$00,$00,$FF,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $83,$FF,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$FF,$00,$00,$00,$FF,$FF
        .byte   $AB,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$80,$E0,$30,$10,$18,$00,$00
        .byte   $00,$00,$00,$C0,$E0,$E0,$08,$08
        .byte   $08,$08,$08,$08,$08,$88,$F0,$F0
        .byte   $A0,$F0,$F0,$F0,$30,$30,$88,$88
        .byte   $88,$88,$88,$88,$88,$88,$30,$30
        .byte   $30,$30,$30,$30,$30,$30,$88,$8A
        .byte   $8F,$8A,$8A,$8A,$8A,$88,$35,$37
        .byte   $32,$37,$37,$37,$37,$35,$88,$08
        .byte   $08,$08,$08,$08,$08,$08,$30,$30
        .byte   $F0,$F0,$A0,$F0,$F0,$F0,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$3F,$3F
        .byte   $3F,$3F,$3F,$3F,$3F,$3F,$00,$49
        .byte   $FF,$49,$49,$49,$49,$00,$B6,$FF
        .byte   $49,$FF,$FF,$FF,$AB,$B6,$18,$10
        .byte   $30,$E0,$80,$00,$00,$00,$E0,$E0
        .byte   $C0,$00,$00,$00,$00,$00,$02,$03
        .byte   $02,$03,$02,$03,$02,$07,$01,$04
        .byte   $05,$00,$01,$04,$0D,$18,$0E,$3B
        .byte   $A2,$23,$22,$23,$22,$03,$81,$E4
        .byte   $3D,$FC,$A9,$FC,$A9,$DC,$02,$03
        .byte   $02,$03,$02,$03,$02,$03,$1D,$0C
        .byte   $05,$00,$01,$04,$05,$00,$00,$00
        .byte   $18,$3C,$3C,$18,$00,$00,$FF,$C3
        .byte   $99,$AD,$BD,$99,$C3,$FF,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$00,$3F
        .byte   $3F,$3F,$3F,$3F,$3F,$3F,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7F,$7F
        .byte   $7F,$7F,$7F,$7F,$7F,$7F,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$00,$FC
        .byte   $FC,$FC,$FC,$FC,$FC,$FC,$10,$10
        .byte   $38,$7C,$FE,$7C,$38,$7C,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$6C,$FE
        .byte   $FE,$FE,$7C,$38,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$38,$7C
        .byte   $38,$FE,$FE,$7C,$38,$7C,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$38
        .byte   $7C,$FE,$7C,$38,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$04,$7A
        .byte   $00,$FE,$10,$10,$30,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$4A,$40
        .byte   $40,$78,$4C,$40,$40,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0A,$10
        .byte   $98,$88,$8C,$84,$84,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$8A,$80
        .byte   $FC,$80,$80,$80,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0A,$F0
        .byte   $0C,$04,$0C,$18,$30,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0A,$20
        .byte   $70,$D8,$8C,$06,$02,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$0A,$10
        .byte   $FE,$10,$92,$92,$92,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$16,$1A
        .byte   $8E,$8C,$84,$84,$84,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$86,$8A
        .byte   $FC,$80,$80,$80,$7C,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$06,$FA
        .byte   $06,$04,$0C,$18,$30,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$06,$2A
        .byte   $76,$D8,$8C,$06,$02,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$16,$1A
        .byte   $FE,$10,$92,$92,$92,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$81
        .byte   $81,$81,$81,$81,$FF,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$70,$50,$70,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $E0,$18,$04,$02,$01,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$80,$80
        .byte   $40,$20,$18,$07,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$3C,$42
        .byte   $99,$A1,$A1,$99,$42,$3C,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$44
        .byte   $28,$10,$28,$44,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$10
        .byte   $00,$7C,$00,$10,$10,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$01
        .byte   $02,$04,$18,$E0,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$FE
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$E7,$3C,$99,$5A,$00,$00
        .byte   $00,$00,$00,$C3,$66,$24,$5A,$5A
        .byte   $5A,$5A,$5A,$5A,$5A,$5A,$24,$24
        .byte   $24,$24,$24,$24,$24,$24,$99,$3C
        .byte   $80,$00,$00,$00,$00,$00,$66,$C3
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$FF,$00,$FF,$FF,$00
        .byte   $AB,$00,$00,$00,$00,$00,$00,$FF
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$1F,$18,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$1E,$18
        .byte   $18,$00,$FF,$00,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$FF,$00,$FF,$00,$7E
        .byte   $3E,$1E,$0E,$06,$02,$00,$FE,$82
        .byte   $C6,$EE,$EE,$C6,$82,$FE,$00,$FF
        .byte   $FF,$C3,$80,$C3,$81,$00,$FF,$81
        .byte   $83,$FF,$AB,$C3,$81,$FF,$F7,$BB
        .byte   $AA,$ED,$89,$DD,$FF,$FF,$08,$44
        .byte   $01,$13,$B3,$BB,$BB,$FB,$F8,$F9
        .byte   $80,$00,$C7,$CF,$A3,$00,$FB,$FB
        .byte   $00,$00,$DF,$DF,$DF,$00,$00,$0F
        .byte   $6F,$00,$00,$3F,$1F,$00,$3F,$70
        .byte   $90,$FF,$80,$FF,$7F,$3F,$00,$FF
        .byte   $AB,$00,$00,$FF,$FF,$00,$FF,$00
        .byte   $00,$FF,$00,$FF,$83,$FF,$00,$F0
        .byte   $F6,$00,$00,$FC,$F8,$00,$FC,$0E
        .byte   $09,$FF,$01,$FF,$FE,$FC,$70,$7C
        .byte   $7E,$7F,$7C,$40,$40,$00,$40,$40
        .byte   $40,$40,$40,$40,$40,$E0,$00,$00
        .byte   $01,$01,$02,$15,$2B,$57,$01,$01
        .byte   $03,$17,$3E,$3D,$6F,$FF,$80,$C0
        .byte   $E0,$B0,$90,$EC,$F6,$D7,$80,$C0
        .byte   $E0,$F0,$B0,$EC,$F6,$FF,$DD,$B9
        .byte   $A3,$FD,$81,$19,$33,$FF,$DD,$B9
        .byte   $80,$FF,$FD,$D9,$B3,$FF,$B9,$DC
        .byte   $82,$F3,$A1,$F9,$00,$FF,$A8,$DC
        .byte   $FE,$F7,$F7,$FB,$23,$FF,$FF,$F1
        .byte   $B9,$FF,$9F,$F0,$A0,$FF,$FF,$FF
        .byte   $F9,$FF,$80,$FF,$F8,$FF,$00,$00
        .byte   $00,$20,$00,$00,$00,$00,$00,$00
        .byte   $00,$20,$02,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$80,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$3C,$4E
        .byte   $AF,$9F,$FF,$FF,$7E,$3C,$3C,$7E
        .byte   $80,$FF,$AB,$FF,$7E,$3C,$00,$FF
        .byte   $00,$00,$00,$00,$00,$FF,$FF,$FF
        .byte   $38,$6C,$C6,$83,$FF,$FF,$00,$FF
        .byte   $00,$00,$00,$00,$00,$FF,$FF,$FF
        .byte   $C3,$81,$81,$C3,$80,$FF,$81,$FF
        .byte   $81,$81,$81,$FF,$81,$81,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$10,$28
        .byte   $28,$10,$10,$28,$28,$10,$10,$28
        .byte   $28,$10,$10,$28,$28,$10,$08,$08
        .byte   $08,$08,$28,$6C,$6E,$CA,$18,$18
        .byte   $18,$18,$18,$18,$18,$18,$7F,$6F
        .byte   $4F,$4D,$65,$7D,$7B,$5F,$80,$FF
        .byte   $EF,$ED,$F5,$FD,$AB,$FF,$00,$FF
        .byte   $83,$C1,$DF,$FD,$E3,$00,$FF,$FF
        .byte   $FF,$E3,$DF,$FD,$80,$FF,$00,$6C
        .byte   $FE,$FE,$FE,$FE,$7C,$83,$10,$7C
        .byte   $FE,$9E,$BE,$FE,$7C,$83,$0F,$3B
        .byte   $77,$EF,$FF,$FF,$80,$71,$00,$04
        .byte   $08,$10,$00,$00,$00,$00,$FF,$FF
        .byte   $DD,$F3,$83,$EF,$82,$B9,$00,$00
        .byte   $22,$0C,$00,$00,$10,$39,$F0,$CC
        .byte   $A2,$FF,$AB,$FF,$FE,$74,$00,$30
        .byte   $08,$00,$00,$00,$00,$00,$3C,$7E
        .byte   $5A,$BD,$80,$FF,$FF,$FF,$00,$00
        .byte   $24,$42,$02,$00,$00,$00,$FF,$FF
        .byte   $FF,$FF,$BF,$DB,$80,$FF,$FF,$FF
        .byte   $FF,$FF,$BF,$DB,$80,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24
        .byte   $C3,$00,$00,$C3,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $0F,$10,$20,$23,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $F0,$08,$04,$C4,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24
        .byte   $23,$20,$10,$0F,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24
        .byte   $C4,$04,$08,$F0,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24
        .byte   $24,$24,$24,$24,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $FF,$00,$00,$FF,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $18,$24,$24,$24,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24
        .byte   $24,$24,$24,$18,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $1F,$20,$20,$1F,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $F8,$04,$04,$F8,$00,$00,$18,$18
        .byte   $3C,$FF,$FF,$3C,$18,$18,$18,$18
        .byte   $3C,$FF,$FF,$3C,$18,$18,$18,$18
        .byte   $3C,$FF,$00,$00,$00,$00,$18,$18
        .byte   $3C,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$FF,$FF,$3C,$18,$18,$00,$00
        .byte   $00,$FF,$AB,$3C,$18,$18,$18,$18
        .byte   $38,$F8,$F8,$38,$18,$18,$18,$18
        .byte   $38,$F8,$F8,$38,$18,$18,$18,$18
        .byte   $1C,$1F,$1F,$1C,$18,$18,$18,$18
        .byte   $1C,$1F,$1F,$1C,$18,$18,$00,$00
        .byte   $00,$FF,$FF,$00,$00,$00,$00,$00
        .byte   $00,$FF,$FF,$00,$00,$00,$18,$18
        .byte   $18,$18,$18,$18,$18,$18,$18,$18
        .byte   $18,$18,$18,$18,$18,$18,$00,$00
        .byte   $00,$07,$0F,$1C,$18,$18,$00,$00
        .byte   $00,$07,$0F,$1C,$18,$18,$00,$00
        .byte   $00,$E0,$F0,$38,$18,$18,$00,$00
        .byte   $00,$E0,$F0,$38,$18,$18,$18,$18
        .byte   $1C,$0F,$07,$00,$00,$00,$18,$18
        .byte   $1C,$0F,$07,$00,$00,$00,$18,$18
        .byte   $38,$F0,$E0,$00,$00,$00,$18,$18
        .byte   $38,$F0,$E0,$00,$00,$00,$FF,$C0
        .byte   $A0,$90,$88,$84,$82,$81,$00,$3F
        .byte   $5F,$6F,$77,$7B,$7D,$7E,$81,$81
        .byte   $81,$81,$81,$81,$81,$81,$7E,$7E
        .byte   $7E,$7E,$7E,$7E,$7E,$7E,$81,$82
        .byte   $84,$88,$90,$A0,$C0,$FF,$7E,$7D
        .byte   $7B,$77,$6F,$5F,$3F,$00,$FF,$00
        .byte   $00,$00,$00,$00,$00,$FF,$00,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$00,$FF,$03
        .byte   $05,$09,$11,$21,$41,$81,$00,$FC
        .byte   $FA,$F6,$EE,$DE,$BE,$7E,$81,$41
        .byte   $21,$11,$09,$05,$03,$FF,$7E,$BE
        .byte   $DE,$EE,$F6,$FA,$FC,$00,$00,$00
        .byte   $00,$07,$00,$07,$00,$00,$00,$00
        .byte   $0F,$07,$0F,$07,$3F,$FF,$3F,$3F
        .byte   $3F,$3F,$3F,$3F,$3F,$00,$40,$54
        .byte   $D6,$D5,$D4,$D4,$80,$FF,$00,$00
        .byte   $30,$E0,$30,$E0,$1C,$0E,$00,$00
        .byte   $F0,$E0,$F0,$E0,$FC,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$07,$02,$AA
        .byte   $AB,$B3,$AB,$AB,$03,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF
