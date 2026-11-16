; ------------------------------------------------------------------------
; 名称：gbasic.s
; 说明：GBASIC（步步高 BASIC 开发程序）移植到 SB2000 的主体源码：解释器与图形编辑器，平台相关的部分逐步改写成 SB2000 的原生做法。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：起点是 reference/gbasic-bbk.s（原版的可重定位源码），切掉了 $7B00-$7FFF 里的步步高系统存根；从此手工维护，不再由脚本生成。
;       标签沿用原版地址命名（L8000 等），改写过的例程在 docs/移植对照表.md 里登记。
; ------------------------------------------------------------------------
        .setcpu "6502"
        .include        "sb2000.inc"
        .include        "platform.inc"

        .segment        "CODE"

        .setcpu "6502"

PPUCTRL                 := $2000  ; 控制寄存器：名称表、图案表、NMI
PPUMASK                 := $2001  ; 显示开关：背景、精灵
PPUSTATUS               := $2002  ; 状态：位 7 为垂直消隐
OAMADDR                 := $2003  ; 精灵内存地址
PPUSCROLL               := $2005  ; 滚动，连续写两次
PPUADDR                 := $2006  ; 显存地址，连续写两次
PPUDATA                 := $2007  ; 显存数据
OAMDMA                  := $4014  ; 精灵 DMA
BBG_BIOS                := $5800  ; 步步高 BIOS 入口（A 是功能号）
BBG_FILE                := $5803  ; 步步高文件服务入口
BBG_DEVICE              := $5809  ; 步步高外设服务入口
BBG_DOS                 := $580C  ; 步步高 DOS 服务入口
BBG_NMI_VECTOR_LO       := $BFFA  ; NMI 向量低字节：BIOS 的 SetPage($FF) 使 $8000-$BFFF 与 $C000-$FFFF 同页，写 $BFFA 就是写 $FFFA
BBG_NMI_VECTOR_HI       := $BFFB  ; NMI 向量高字节

; 指向指令或数据中间的地址
L8007                   := L8006+1
L8842                   := L8841+1
L9469                   := L9468+1
LA27E                   := LA27D+1
LAF02                   := LAF01+1
LB6A4                   := LB6A3+1
LB7F6                   := LB7F5+1
LB8D8                   := LB8D7+1
LBC76                   := LBC75+1
LC2C7                   := LC2C6+1
LC329                   := LC2C6+99
LC394                   := LC391+3
LC3A6                   := LC3A5+1
LC3AA                   := LC3A5+5


L7B3D:  ldx     #$00
        stx     L7B0A
        stx     L7B0B
        stx     L7B0C
        stx     L7B0D
        stx     $86
        stx     $87
        dex
        stx     $88
        stx     $89
        jsr     L8FF8
        lda     $86
        sta     $44
        lda     $87
        sta     $45
        jsr     L8404
        ldy     #$00
        lda     ($5B),y
        beq     L7B93
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
L7B70:  ldy     #$00
        lda     ($09),y
        beq     L7B93
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
        jsr     L8BF7
        bcs     L7B93
        jsr     L7BA8
        jmp     L7B70
L7B93:  lda     #$05
        jsr     L7B33
        lda     #$0C
        sta     $5FA3
        sta     $FF90
        lda     #$00
        sta     $62
        jsr     LB3F1
        rts
L7BA8:  lda     #$00
        sta     $62
        ldy     #$01
        lda     ($09),y
        sta     $28
        iny
        lda     ($09),y
        sta     $29
        jsr     L8D3C
        jsr     L8CEB
        jsr     L865F
        lda     #$20
        jsr     L87F0
        lda     #$03
        clc
        adc     $09
        sta     $4D
        lda     #$00
        adc     $0A
        sta     $4E
        jsr     L900A
        jsr     L8A3B
        jsr     L87B7
        jsr     L87EE
        ldx     $62
        dex
        lda     #$0D
        sta     $0700,x
        inx
        lda     #$0A
        sta     $0700,x
        inx
        stx     $62
        jsr     L7C30
        nop
        nop
        nop
        nop
        nop
        nop
        jsr     L7C09
        lda     #$01
        clc
        adc     $4D
        sta     $09
        lda     #$00
        adc     $4E
        sta     $0A
        rts
L7C09:  lda     L7D03
        sta     L7B05
        ldx     #$00
        ldy     #$05
        stx     L7B06
        sty     L7B07
        lda     #$00
        sta     L7B09
        lda     $62
        sta     L7B08
        lda     #$03
        jmp     L7B33
        lda     #$03
        jmp     L7B33
L7C30:  ldx     #$00
L7C32:  lda     $0700,x
        sta     $0500,x
        inx
        cpx     $62
        bne     L7C32
        rts
L7E00:  jsr     LAC07
        lda     #$2A
        jsr     LAC34
        jsr     LB343
        ldy     #$00
        sty     $90
        beq     L7E37
L7E11:  ldx     #$03
        asl     a
        asl     a
        asl     a
        asl     a
L7E17:  asl     a
        rol     $8E
        rol     $8F
        dex
        bpl     L7E17
L7E1F:  lda     $90
        bne     L7E27
        lda     $8F,x
        sta     $8D,x
L7E27:  inx
        beq     L7E1F
        bne     L7E3D
L7E2C:  jsr     L7E6B
        jmp     L7E00
L7E32:  jsr     L7E6B
L7E35:  ldy     $25
L7E37:  ldx     #$00
        stx     $8E
        stx     $8F
L7E3D:  lda     $0501,y
        iny
        eor     #$30
        cmp     #$0A
        bcc     L7E11
        adc     #$88
        cmp     #$FA
        bcs     L7E11
        sty     $25
        cmp     #$B9
        beq     L7E2C
        cmp     #$99
        beq     L7E32
        cmp     #$A6
        beq     L7E66
        cmp     #$93
        beq     L7E66
        cmp     #$EA
        bne     L7E00
        jmp     L821D
L7E66:  sta     $90
        jmp     L7E35
L7E6B:  lda     #$00
        sta     $E2
        lda     $90
        beq     L7EA0
        cmp     #$A6
        beq     L7EA0
        cmp     #$93
        bne     L7E00
        lda     $8E
        ldy     #$00
        sta     ($8C),y
        jmp     L7E8C
L7E84:  lda     $8C
        cmp     $8E
        lda     $8D
        sbc     $8F
L7E8C:  inc     $8C
        bne     L7E92
        inc     $8D
L7E92:  rts
L7E93:  jsr     L86B9
        beq     L7E92
        inc     $E2
        lda     $E2
        and     #$07
        bne     L7EAD
L7EA0:  jsr     LAC07
        lda     $8D
        jsr     L7EBF
        lda     $8C
        jsr     L7EBF
L7EAD:  lda     #$20
        jsr     LAC34
        ldy     #$00
        lda     ($8C),y
        jsr     L7EBF
        jsr     L7E84
        bcc     L7E93
        rts
L7EBF:  pha
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     L7EC8
        pla
L7EC8:  and     #$0F
        ora     #$30
        cmp     #$3A
        bcc     L7ED2
        adc     #$06
L7ED2:  jmp     LAC34
L7F1A:  rts
L7F23:  jsr     L7F1A
        jsr     LB3F1
        lda     #$11
        jmp     LA98F
L7F2E:  jsr     LB3EB
        lda     #$09
        jsr     L7EE0
        lda     #$0C
        ldx     #$05
        ldy     #$80
        jsr     BBG_DOS
        ldx     #$05
        ldy     #$80
        stx     L7EEE
        sty     L7EED
        lda     L7D03
        sta     L7EEC
        lda     #$01
        jsr     L7EE0
        bcc     L7F59
        jmp     L7F23
L7F59:  lda     #$00
        sta     $04C0
        sta     L7EED
        sta     L7EF0
        lda     #$05
        sta     L7EEE
        lda     #$01
        sta     $04C1
L7F6E:  lda     L7D03
        sta     L7EEC
        lda     $04C1
        sta     L7EEF
        lda     #$02
        jsr     L7EE0
        lda     L7EEF
        beq     L7FAB
        ldy     $04C0
        cpy     #$FF
        beq     L7F23
        lda     $0500,y
        cmp     #$0A
        beq     L7F98
        inc     $04C0
        jmp     L7F6E
L7F98:  ldy     $04C0
        dey
        lda     #$00
        sta     $0500,y
        iny
        sta     $0500,y
        jsr     L7FC6
        jmp     L7F59
L7FAB:  lda     #$05
        jsr     L7EE0
        lda     #$20
        sta     $0500
        lda     #$00
        sta     $0501
        sta     $0502
        jsr     LB3F1
        jsr     L7F1A
        jmp     L8221
L7FC6:  jsr     L8A32
        jsr     L83B1
        jsr     L9228
        rts
L8000:  jmp     L80AD
        jmp     LAF88
L8006:
        .word   $0EAE,$0E4E,$0DF3,$0D9E,$0D4D,$0D01,$0CB9,$0C75
        .word   $0C35,$0BF8,$0BBF,$0B89,$0B57,$0B27,$0AF9,$0ACF
        .word   $0AA6,$0A80,$0A5C,$0A3A,$0A1A,$09FC,$09DF,$09C4
        .word   $09AB,$0993,$097C,$0967,$0952,$093F,$092D,$091C
        .word   $090C,$08FD,$08EE,$08E1,$08D4,$08C8,$08BD,$08B2
        .word   $08A8,$089F,$0896,$088D,$0885,$087E,$0876,$0870
        .word   $0869,$0863,$085E,$0858,$0853,$084F,$084A,$0846
        .word   $0842,$083E,$083A,$0837,$0834,$0831,$082E,$082B
        .word   $0829,$0827,$0824,$0822,$0820,$081E,$081C,$081B
L8096:
        .byte   $01,$02,$03,$04,$06,$08,$0C,$10,$18,$20
L80A0:
        .byte   $12,$16,$00,$04,$08,$0A,$0E
L80A7:
        .byte   $00,$18,$30,$48,$60,$78
L80AD:  lda     #$4C
        sta     $ED
        lda     #<L880F
        sta     $EE
        lda     #>L880F
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
L80D7:  lda     #$5A
        sta     $063A
        lda     #$33
        sta     $063B
        sta     $00
        lda     $07
        sta     $063C
        lda     $08
        sta     $063D
        jmp     L80AD
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
        lda     #<LC410
        sta     $05
        lda     #>LC410
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
L816E:
        .byte   "1--BASIC", $00
L8177:
        .byte   "2--BG GRAPHIC", $00
L8185:
        .byte   "3--DOS", $00
L818C:
        .byte   "1,2,3 KEY IN !!", $00
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
        put_tile
        lda     #$E3
        ldy     #$14
        jsr     L81F1
        inc     $AD
        lda     $AD
        put_tile
        rts
; 竖线 16 个图块：A = 起点的 NES 名称表地址高字节，X = 低字节；6576 的图块条目是 2 字节，逐格重设地址
L81EA:  sta     $F4
        stx     $F5
        ldy     #$10
L81EF:  lda     $F4
        ldx     $F5
        jsr     L81F8
        lda     #$E2
        put_tile
        lda     $F5
        clc
        adc     #$20
        sta     $F5
        bcc     L81EE
        inc     $F4
L81EE:  dey
        bne     L81EF
        rts
; 横向连续写 Y 个相同的图块：A = 图案号
L81F1:  sta     $F2
L81F3:  lda     $F2
        put_tile
        dey
        bne     L81F3
        rts
; 设名称表写入地址：A = NES 名称表地址高字节，X = 低字节；转成 CRAM 地址（每个图块 2 字节，地址左移一位），不改 X、Y
L81F8:  sta     $F0
        txa
        asl     a
        sta     $F1
        lda     $F0
        and     #$0F
        rol     a
        sta     PPUADDR
        lda     $F1
        sta     PPUADDR
        rts
L81FF:  ldx     #$FF
        txs
        jsr     L852E
        jsr     L8388
        jsr     LAA09
L820B:  jsr     L873B
        sta     $063A
        sta     $063B
L8214:  ldx     #$FF
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
        lda     #<LB144
        sta     $0D
        lda     #>LB144
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
L8318:  lda     ($19),y
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
        sta     $19
        lda     LC2C7,x
        sta     $1A
        tsx
        stx     $17
        jsr     L8378
        jsr     L9D4B
        jmp     L8275
L8340:  jmp     L821D
L8343:  pla
        pla
L8345:  lda     $0D
        sta     $09
        lda     $0E
        sta     $0A
L834D:  jsr     L8CC7
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
L8375:  jsr     L8CA1
L8378:  jsr     L8CD4
        and     #$FF
        beq     L8381
        cmp     #$3A
L8381:  rts
L8382:  jsr     L8388
        jmp     L8214
L8388:  jsr     L873B
        sta     LC410
        sta     LC411
        sta     LC412
        ldx     #<LC410
        stx     $05
        inx
        stx     $07
        inx
        stx     $1D
        inx
        stx     $1F
L83A1:  inx
        stx     $21
        lda     #>LC410
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
        cmp     $28
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
        lda     $28
        sta     $44
        lda     $29
        sta     $45
        jsr     L8E59
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
        sta     $28
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
L8440:  lda     $28
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
        sta     $28
        lda     $08
        adc     #$00
        sta     $29
        jsr     L8A44
        jsr     L8404
        lda     $07
        sta     $0B
        clc
        adc     $5A
        sta     $19
        lda     $08
        sta     $0C
        adc     #$00
        sta     $1A
L84AE:  ldy     #$00
        lda     ($0B),y
        sta     ($19),y
        jsr     L8516
        jsr     L8522
        lda     $5B
        cmp     $0B
        bne     L84AE
        lda     $5C
        cmp     $0C
        bne     L84AE
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
L8522:  lda     $19
        beq     L8529
        dec     $19
        rts
L8529:  dec     $19
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
        lda     #$F6                            ; 内存上限 $F6FF：$F700 起是 DOS 现场交换缓冲
        sta     $04
        jsr     LA6DF
        jsr     LA96E
        jmp     LAA82
L857C:  jsr     L9228
        jsr     L873B
        jsr     LA96E
        jsr     LA6DF
        jsr     L8378
        bne     L859F
        ldx     #$FF
        txs
        lda     #$FF
        pha
        pha
        lda     $05
        sta     $09
        lda     $06
        sta     $0A
        jmp     L834D
L859F:  jmp     L9146
L85A2:  jsr     L8CD4
        cmp     #$F6
        bne     L85AC
        jmp     L8CA1
L85AC:  jmp     L836D
L85AF:  php
        ldx     #$00
        stx     $86
        stx     $87
        dex
        stx     $88
        stx     $89
        plp
        beq     L8604
        cmp     #$2C
        beq     L85E5
        cmp     #$2D
        beq     L85E5
        cmp     #$0B
        bne     L85AC
        jsr     L8CAD
        sta     $86
        sta     $88
        jsr     L8CAD
        sta     $87
        sta     $89
        jsr     L8375
        beq     L8604
        cmp     #$2C
        beq     L85E5
        cmp     #$2D
        bne     L85AC
L85E5:  jsr     L8375
        beq     L85FE
        cmp     #$0B
        bne     L85AC
        jsr     L8CAD
        sta     $88
        jsr     L8CAD
        sta     $89
        jsr     L8CA1
        jmp     L8604
L85FE:  lda     #$FF
        sta     $88
        sta     $89
L8604:  jsr     L8FF8
        lda     $86
        sta     $44
        lda     $87
        sta     $45
        jsr     L8404
        ldy     #$00
        lda     ($5B),y
        beq     L865C
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
L8620:  ldy     #$00
        lda     ($09),y
        beq     L865C
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
        jsr     L8BF7
        beq     L863F
        bcs     L865C
L863F:  jsr     L866D
        jsr     L8656
        beq     L8620
        cmp     #$1B
        bne     L8620
L864B:  jsr     L8656
        beq     L864B
        cmp     #$1B
        beq     L864B
        bne     L8620
L8656:  jsr     L86D4
        jmp     LAF88
L865C:  jmp     L9001
L865F:  lda     #$3D
        clc
        adc     $46
        sta     $52
        lda     #$00
        sta     $53
        jmp     L87B7
L866D:  lda     #$00
        sta     $62
        ldy     #$01
        lda     ($09),y
        sta     $28
        iny
        lda     ($09),y
        sta     $29
        jsr     L8D3C
        jsr     L8CEB
        jsr     L865F
        lda     #$20
        jsr     L87F0
        lda     #$03
        clc
        adc     $09
        sta     $4D
        lda     #$00
        adc     $0A
        sta     $4E
        jsr     L900A
        jsr     L8A3B
        jsr     L87F8
        lda     $4D
        sta     $09
        lda     $4E
        sta     $0A
        jsr     LAC07
        lda     #$01
        clc
        adc     $4D
        sta     $09
        lda     #$00
        adc     $4E
        sta     $0A
        rts
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
L86DA:  pla
        pla
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
L8712:  lda     #$0E
        jmp     LA98F
L8717:  lda     $24
        beq     L8712
        lda     $15
        sta     $09
        lda     $16
        sta     $0A
L8723:  jsr     L9908
        jsr     L8378
        bne     L8723
        lda     $13
        sta     $0D
        lda     $14
        sta     $0E
        lda     $11
        sta     $0B
        lda     $12
        sta     $0C
L873B:  lda     #$00
        sta     $24
        rts
L8740:  beq     L87B4
        jsr     L9C82
        lda     $27
        cmp     #$03
        beq     L8761
        lda     $0400
        sta     $28
        lda     $0401
        sta     $29
        jsr     L8D96
        jsr     L865F
L875B:  jsr     L87FB
        jmp     L8794
L8761:  lda     $0400
        beq     L8794
        ldy     #$00
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
L8772:  lda     ($19),y
        cmp     #$20
        bcs     L877A
        lda     #$00
L877A:  sta     $0500,y
        iny
        dec     $0400
        bne     L8772
        lda     #$00
        sta     $0500,y
        jsr     L8A3B
        jsr     L87B7
        jmp     L875B
        jsr     L8CA1
L8794:  jsr     L8378
        beq     L87B4
        cmp     #$3B
        beq     L87A6
        cmp     #$2C
        bne     L87AE
        lda     #$09
        jsr     LAC0D
L87A6:  jsr     L8375
        beq     L87B3
        jmp     L8794
L87AE:  jsr     L8378
        bne     L8740
L87B3:  rts
L87B4:  jmp     LAC07
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
L880F:  pha
        inc     gk_frame
        lda     #$00
        sta     OAMADDR
        lda     #$02
        sta     OAMDMA
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
        sta     $1B
        lda     L8842,x
        sta     $1C
        jmp     ($001B)
L8841:
        .addr   L88AA,L8925,L8961,L8898,L8870,L8990,L89BE,L8882
        .addr   L8890,L9A90,L8862,L885F,L89CC,L8A02,L8A1A,L_nmi_group_read,L_nmi_group_write,L_nmi_group_row
L885F:  lda     #$00
        .byte   $AE
L8862:  lda     $33
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
L888A:  put_tile
        jmp     L8867
L8890:  jsr     LABD8
        lda     $73
        jmp     L888A
L8898:  jsr     LABD8
        ldx     $48
L889F:  lda     #$20
        put_tile
        inx
        cpx     #$1C
        bne     L889F
        jmp     L8958
L88AA:  jsr     LABA6
        ldx     $64
        jsr     LABD8
L88B2:  lda     $0700,x
        beq     L891D
        put_tile
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
        jsr     video_write_ctrl
        jsr     LABA6
        jsr     LAAFE
        jsr     LABD8
        ldx     #$1C
L8909:  lda     #$20
        put_tile
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
        bit     PPUDATA
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
        ldx     a:$00A9
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
        bit     PPUDATA
        inx
        iny
        cpy     #$1C
        bne     L896B
        lda     $1A
        ldx     $19
        jsr     L81F8
        ldx     #$00
        ldy     $48
L8982:  lda     $0500,x
        put_tile
        inx
        iny
        cpy     #$1C
        bne     L8982
        beq     L8958
; 调色板命令 6（背景）与 7（精灵）：$0700 是组号，$0701-$0704 是 4 个颜色。直接写 6576 的调色板（$2040 起，背景 16 项、精灵 16 项），
; 0 号色是公共背景色，写到背景 4 个组的 0 号色
L8990:  lda     $0700
        asl     a
        asl     a
        tax
L899A:  lda     $0701
        sta     HW_PALETTE
        sta     HW_PALETTE + 4
        sta     HW_PALETTE + 8
        sta     HW_PALETTE + 12
        lda     $0702
        sta     HW_PALETTE + 1,x
        lda     $0703
        sta     HW_PALETTE + 2,x
        lda     $0704
        sta     HW_PALETTE + 3,x
        jsr     LABE6
        jsr     LB3F1
        jmp     L8867
L89BE:  lda     $0700
        asl     a
        asl     a
        ora     #$10
        tax
        bne     L899A
L89CC:  jsr     LABD8
        ldy     $48
        ldx     #$01
        lda     $B0
        sta     $0500
        lda     PPUDATA
L89DB:  lda     PPUDATA
        sta     $0500,x
        bit     PPUDATA
        inx
        iny
        cpy     #$1C
        bne     L89DB
        sta     $B0
        jsr     LABD8
        ldx     #$00
        ldy     $48
L89F0:  lda     $0500,x
        put_tile
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
        bit     PPUDATA
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
        put_tile
        inx
        cpx     #$1C
        bne     L8A24
        jmp     L89FC
L8A32:  lda     #$00
        sta     $09
        lda     #$05
        sta     $0A
        rts
L8A3B:  lda     #$00
        sta     $52
        lda     #$05
        sta     $53
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
        lda     #$02
        sta     $27
L8A73:  jsr     LA3EF
        jsr     L8A7C
        jsr     L8AEC
L8A7C:  sec
        lda     #$00
        sbc     $28
        sta     $28
        lda     #$00
        sbc     $29
        sta     $29
        rts
L8A8A:  jmp     L8B34
L8A8D:  lda     $2C
        sta     $28
        lda     $2D
        sta     $29
        lda     $2E
        sta     $2A
L8A99:  rts
L8A9A:  ldx     $21
        stx     $4A
        lda     $28
        beq     L8A8D
        clc
        ldy     $2C
        beq     L8A99
        adc     $2C
        cmp     #$20
        bcs     L8A8A
        sta     $4B
        lda     $29
        sta     $19
        lda     $2A
        sta     $1A
        ldy     #$00
L8AB9:  lda     ($19),y
        sta     $0700,x
        inx
        iny
        dec     $28
        bne     L8AB9
        lda     $2D
        sta     $19
        lda     $2E
        sta     $1A
        ldy     #$00
L8ACE:  lda     ($19),y
        sta     $0700,x
        iny
        inx
        dec     $2C
        bne     L8ACE
        stx     $21
        lda     $4B
        sta     $28
        lda     $4A
        sta     $29
        lda     #$07
        sta     $2A
        rts
L8AE8:  lda     #$02
        sta     $27
L8AEC:  lda     $27
        cmp     #$03
        beq     L8A9A
        lda     $29
        bmi     L8B1B
        lda     $2D
        bmi     L8B0A
L8AFA:  clc
        lda     $2C
        adc     $28
        sta     $28
        lda     $2D
        adc     $29
        sta     $29
        bmi     L8B34
        rts
L8B0A:  jsr     L8A61
L8B0D:  sec
        lda     $28
        sbc     $2C
        sta     $28
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
        adc     $28
        sta     $28
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
        ror     $28
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
        sta     $28
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
L8BC1:  asl     $28
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
        inc     $28
L8BE3:  dex
        bne     L8BC1
        jmp     L8B8C
L8BE9:  sec
        lda     $28
        sbc     $2C
        sta     $28
        lda     $29
        sbc     $2D
        sta     $29
        rts
L8BF7:  lda     $29
        cmp     $2D
        beq     L8BFE
        rts
L8BFE:  lda     $28
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
L8C21:  lda     $28
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
L8C3F:  lda     $28
        cmp     $2C
        beq     L8C2C
        bcs     L8C1D
        jmp     L8C19
L8C4A:  lda     $29
        sta     $19
        lda     $2A
        sta     $1A
        lda     $2D
        sta     $1B
        lda     $2E
        sta     $1C
        lda     $28
        beq     L8C86
        sta     $4A
        beq     L8C1D
        lda     $2C
        sta     $4B
        ldy     #$00
L8C68:  lda     ($19),y
        cmp     ($1B),y
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
L8C8D:  ldy     #$00
        sta     ($09),y
        inc     $09
        beq     L8C96
        rts
L8C96:  dec     $09
        lda     #$00
        sta     ($09),y
        rts
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
        ldy     #$00
        sta     ($09),y
        rts
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
L8CF9:  jsr     L8CD4
        cmp     #$2D
        bne     L8D09
        jsr     L8CA1
        jsr     L8CF9
        jmp     L8A7C
L8D09:  lda     #$02
        sta     $27
        lda     #$00
        sta     $28
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
L8D40:  lda     $46
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
        lda     $28
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
L8D96:  lda     #$00
        sta     $46
        ldy     #$20
        lda     $29
        bpl     L8DA5
        jsr     L8A7C
        ldy     #$2D
L8DA5:  tya
        pha
        jsr     L8D40
        jsr     L8CEB
        dec     $46
        pla
        jsr     L8D81
        dec     $46
        rts
L8DB6:  cmp     #$0A
        bcs     L8DBD
        adc     #$30
        rts
L8DBD:  clc
        adc     #$37
        rts
L8DC1:  pha
        and     #$F0
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     L8DB6
        sta     $3D,x
        inx
        pla
        and     #$0F
        jsr     L8DB6
        sta     $3D,x
        inx
        rts
L8DD8:  ldx     #$00
        stx     $46
        lda     $29
        jsr     L8DC1
        lda     $28
        jsr     L8DC1
        lda     #$00
        sta     $3D,x
L8DEA:  rts
L8DEB:  lda     #$02
        sta     $27
        ldx     #$00
        lda     #$00
        sta     $28
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
L8E32:  jsr     L8E3E
L8E35:  ldy     #$00
        lda     ($4D),y
        rts
L8E3A:  ldy     #$00
        lda     ($4D),y
L8E3E:  inc     $4D
        bne     L8E53
        inc     $4E
        bne     L8E53
L8E46:  lda     #$00
        jsr     L8E4B
L8E4B:  ldy     #$00
        sta     ($4D),y
        inc     $4D
        beq     L8E56
L8E53:  and     #$FF
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
        .byte   $AE
L8EB6:  ldy     #$22
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
        lda     $28
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
        lda     $28
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
L8FAE:  lda     #<LC128
        sta     $54
        lda     #>LC128
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
L900A:  jsr     L8A32
L900D:  jsr     L8E35
        beq     L9029
        cmp     #$20
        bcc     L902E
        cmp     #$80
        bcc     L9064
        pha
        jsr     L9094
        jsr     L8E3E
        pla
        cmp     #$95
        beq     L9086
        jmp     L900D
L9029:  lda     #$00
        jmp     L8C8D
L902E:  pha
        jsr     L8E32
        sta     $28
        jsr     L8E32
        sta     $29
        jsr     L8E3E
        pla
        cmp     #$11
        bne     L9051
        lda     #$26
        jsr     L8C8D
        lda     #$48
        jsr     L8C8D
        jsr     L8DD8
        jmp     L9054
L9051:  jsr     L8D3C
L9054:  jsr     L8CEB
        ldx     $46
L9059:  lda     $3D,x
        beq     L900D
        jsr     L8C8D
        inx
        jmp     L9059
L9064:  cmp     #$22
        bne     L907F
L9068:  jsr     L908E
        jsr     L8E35
        cmp     #$22
        beq     L9079
        and     #$FF
        bne     L9068
        jmp     L900D
L9079:  jsr     L908E
        jmp     L900D
L907F:  cmp     #$27
        bne     L9079
L9083:  jsr     L908E
L9086:  jsr     L8E35
        bne     L9083
        jmp     L900D
L908E:  jsr     L8E3A
        jmp     L8C8D
L9094:  sta     $99
        lda     #<LC128
        sta     $54
        lda     #>LC128
        sta     $55
        jsr     L8FA2
L90A1:  cmp     #$FF
        beq     L90C5
        cmp     $99
        beq     L90B4
L90A9:  jsr     L8FA7
        jsr     L8FA2
        bpl     L90A9
        jmp     L90A1
L90B4:  jsr     L8FA7
L90B7:  jsr     L8FA2
        bmi     L90C5
        jsr     L8C8D
        jsr     L8FA7
        jmp     L90B7
L90C5:  rts
L90C6:  beq     L9123
        tax
        pla
        sta     $5F
        pla
        sta     $60
        txa
        jsr     L9172
        jsr     L9598
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
        jmp     L914B
L90F7:  lda     #$02
        jmp     LA98F
L90FC:  pla
        sta     $5F
        pla
        sta     $60
        pla
        cmp     #$FF
        bne     L90F7
        pla
        bne     L90F7
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
L911C:  lda     $60
        pha
        lda     $5F
        pha
        rts
L9123:  jmp     L836D
L9126:  beq     L90FC
        pla
        sta     $5F
        pla
        sta     $60
        pla
        cmp     #$FF
        bne     L90F7
        pla
        bne     L90F7
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
        jsr     L8378
L9146:  beq     L9123
L9148:  jsr     L9172
L914B:  ldy     #$00
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
L9172:  cmp     #$0B
        bne     L9123
        ldy     #$01
        lda     ($09),y
        sta     $44
        iny
        lda     ($09),y
        sta     $45
        jsr     L8404
        bne     L9187
L9186:  rts
L9187:  lda     #$07
        jmp     LA98F
L918C:  jsr     L9C82
        jmp     ($0400)
L9192:  jmp     L8370
L9195:  jmp     LB263
L9198:  beq     L9195
        lda     #$00
        sta     $74
        jsr     L9C82
        jsr     LA3EF
        lda     $0401
        bmi     L9192
        bne     L91B0
        lda     $0400
        beq     L9186
L91B0:  inc     $0401
L91B3:  jsr     L86D4
        dec     $74
        bne     L91B3
        dec     $0400
        bne     L91B3
        lda     $0401
        beq     L91C9
        dec     $0401
        bne     L91B3
L91C9:  rts
L91CA:  jsr     L9C6D
        cmp     #$1C
        bcs     L9192
        sta     $6B
        jsr     L9C6A
        cmp     #$18
        bcs     L9192
        sta     $6C
        jmp     LACAA
L91DF:  jsr     L9C82
        lda     $0400
        sta     $25
        lda     $0401
        sta     $26
        ldy     #$00
        sty     $46
L91F0:  jsr     L9C6A
        ldy     $46
        sta     ($25),y
        inc     $46
        jsr     L8378
        bne     L91F0
        rts
L91FF:  pla
        pla
        jmp     L8345
L9204:  jsr     L9C82
        lda     $0400
        ora     $0401
        beq     L91FF
        jsr     L8378
        beq     L9275
        cmp     #$85
        beq     L921C
        rts
L9219:  jmp     L9148
L921C:  jsr     L8375
        beq     L91DF
        cmp     #$0B
        beq     L9219
        rts
L9226:  bne     L924D
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
L924D:  jsr     L9228
        jsr     L9C82
        jsr     LA3EF
        lda     $0401
        cmp     #$78
        bcs     L9278
        cmp     $20
        bcc     L9278
        bne     L926A
        lda     $0400
        cmp     $1F
        bcc     L9278
L926A:  lda     $0400
        sta     $03
        lda     $0401
        sta     $04
        rts
L9275:  jmp     L836D
L9278:  jmp     L8370
L927B:  jmp     LA3E1
L927E:  beq     L9275
        pla
        sta     $5F
        pla
        sta     $60
        lda     $0D
        pha
        lda     $0E
        pha
        lda     $0B
        pha
        lda     $0C
        pha
        jsr     L8CD4
        jsr     L9F75
        lda     $90
        cmp     #$02
        bne     L927B
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     L85A2
        jsr     L9C82
        pla
        sta     $89
        pha
        sta     $8C
        ldy     #$00
        lda     $0400
        sta     ($8C),y
        lda     $0401
        iny
        sta     ($8C),y
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     L8CD4
        cmp     #$88
        bne     L9275
        jsr     L8CA1
        jsr     L9C82
        lda     $0400
        pha
        lda     $0401
        pha
        lda     #$01
        sta     $0400
        lda     #$00
        sta     $0401
        jsr     L8378
        beq     L92F1
        cmp     #$89
        bne     L9275
        jsr     L8CA1
        jsr     L9C82
L92F1:  lda     $0400
        pha
        lda     $0401
        pha
        lda     $09
        pha
        lda     $0A
        pha
        lda     #$00
        pha
        lda     #$FD
        pha
        jmp     L911C
L9308:  lda     #$00
        jmp     LA98F
L930D:  pla
        sta     $5F
        pla
        sta     $60
        pla
        cmp     #$FD
        bne     L9308
        pla
        bne     L9308
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
        jsr     L938A
        beq     L9351
        bcc     L9387
L9351:  lda     $84
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
        lda     $7C
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
L9387:  jmp     L911C
L938A:  lda     $83
        bmi     L939A
        jsr     L93A0
        jsr     L8C03
        bcs     L9398
        sec
        rts
L9398:  clc
        rts
L939A:  jsr     L93A0
        jmp     L8C03
L93A0:  lda     $82
        sta     $2C
        lda     $83
        sta     $2D
        jsr     L8AE8
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
L93BE:  rts
L93BF:  jmp     L836D
L93C2:  beq     L93BF
        cmp     #$22
        bne     L93D3
        jsr     L94DB
        cmp     #$2C
        beq     L93D8
        cmp     #$3B
        bne     L93BF
L93D3:  lda     #$3F
        jsr     LAC0D
L93D8:  lda     $49
        sta     $7E
        lda     $48
        sta     $80
        jsr     LB343
        jsr     L86D4
        lda     #$00
        sta     $52
L93EA:  lda     $7E
        cmp     $47
        beq     L93FB
        dec     $7E
        clc
        lda     #$1C
        adc     $52
        sta     $52
        bne     L93EA
L93FB:  lda     $80
        clc
        adc     $52
        sta     $52
L9402:  jsr     L8CD4
        jsr     L9F75
        jsr     L8FF8
        lda     $52
        sta     $09
        lda     #$05
        sta     $0A
        lda     $90
        and     #$0F
        cmp     #$03
        beq     L945B
        jsr     LA4FE
        lda     $09
        sta     $52
        ldy     #$00
        lda     $28
        sta     ($8C),y
        iny
        lda     $29
        sta     ($8C),y
L942D:  jsr     L9001
        jsr     L8CD4
        cmp     #$2C
        bne     L93BE
        jsr     L8CA1
        jsr     L8FF8
        lda     $52
        sta     $09
        lda     #$05
        sta     $0A
        ldy     #$00
        lda     ($09),y
        cmp     #$2C
        bne     L9455
        inc     $52
        jsr     L9001
        jmp     L9402
L9455:  jsr     L9001
        jmp     L93D3
L945B:  jsr     L8CD4
        lda     #$00
        sta     ($5B,x)
        ldy     #$00
        ldx     #$00
        lda     ($09),y
L9468:  beq     L947A
        cmp     #$2C
        beq     L947A
        sta     $0700,x
        inc     $1B
        inx
        jsr     L8CA1
        jmp     L9469
L947A:  lda     $1B
        sta     $0400
        lda     #$00
        sta     $0401
        lda     #$07
        sta     $0402
        jsr     L82EF
        lda     $09
        sta     $52
        jmp     L942D
L9493:  jmp     L836D
L9496:  beq     L9493
        jsr     L9F75
        lda     $90
        and     #$0F
        pha
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     LA594
        jsr     L8CD4
        jsr     L9F75
        pla
        sta     $1A
        pla
        sta     $19
        lda     $90
        and     #$0F
        sta     $90
        pla
        cmp     $90
        bne     L94D8
        tax
        cmp     #$02
        beq     L94C7
        ldx     #$21
L94C7:  ldy     #$00
L94C9:  lda     ($8C),y
        pha
        lda     ($19),y
        sta     ($8C),y
        pla
        sta     ($19),y
        iny
        dex
        bne     L94C9
        rts
L94D8:  jmp     LA3E1
L94DB:  jsr     L8CA1
        ldy     #$00
        lda     ($09),y
        beq     L9493
        cmp     #$22
        beq     L94EE
        jsr     LAC0D
        jmp     L94DB
L94EE:  jsr     L8375
        beq     L9493
        jmp     L8CA1
L94F6:  beq     L9493
        cmp     #$22
        bne     L9507
        jsr     L94DB
        cmp     #$2C
        beq     L9507
        cmp     #$3B
        bne     L9493
L9507:  jsr     LB343
        jsr     L86D4
        jsr     L8CD4
        jsr     L9F75
        lda     $90
        and     #$0F
        cmp     #$03
        bne     L94D8
        ldx     #$00
        ldy     #$00
        lda     $0500,x
        cmp     #$20
        bne     L952D
        inx
        lda     $0500,x
        beq     L953A
        dex
L952D:  lda     $0500,x
        beq     L953A
        sta     $0700,x
        inx
        iny
        jmp     L952D
L953A:  sty     $0400
        lda     #$00
        sta     $0401
        lda     #$05
        sta     $0402
        jmp     L82EF
L954A:  jmp     L836D
L954D:  jsr     L9C6D
        jsr     L8CD4
        pha
        jsr     L8CA1
        ldx     $0400
        beq     L9590
L955C:  jsr     L8378
        beq     L9593
        cmp     #$0B
        bne     L954A
        dex
        beq     L9576
        jsr     L9927
        jsr     L8378
        beq     L9593
        jsr     LA594
        jmp     L955C
L9576:  pla
        sec
        sbc     #$80
        cmp     #$05
        bcs     L954A
        asl     a
        tax
        lda     LC2C6,x
        sta     $19
        lda     LC2C7,x
        sta     $1A
        jsr     L8CD4
        jmp     ($0019)
L9590:  jsr     L9598
L9593:  pla
L9594:  rts
L9595:  jsr     L9927
L9598:  jsr     L8378
        beq     L9594
        cmp     #$0B
        beq     L9595
        jsr     LA594
        jmp     L9598
L95A7:  jmp     L8370
L95AA:  cmp     #$86
        beq     L95F3
        jsr     L9C6D
        beq     L95A7
        pha
        jsr     LA594
        jsr     L9C7C
        pla
        sec
        sbc     #$01
        cmp     #$09
        bcs     L95A7
        asl     a
        asl     a
        asl     a
        nop
        tax
        lda     $0400
        beq     L95ED
        ldy     #$00
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
        lda     #$0F
        sta     $90
L95DC:  lda     ($19),y
        sta     $0480,x
        inx
        iny
        dec     $0400
        beq     L95ED
        dec     $90
        bne     L95DC
        rts
L95ED:  lda     #$00
        sta     $0480,x
        rts
L95F3:  jsr     L8CA1
        lda     #$30
        sta     $90
        lda     #$00
        sta     $74
L95FE:  lda     #$46
        jsr     L87F0
        inc     $90
        lda     $90
        jsr     L87F0
        lda     #$20
        jsr     L87F0
        lda     $74
        asl     a
        asl     a
        asl     a
        nop
        tay
L9616:  lda     $0480,y
        beq     L963C
        cmp     #$20
        bcc     L9626
        jsr     L87F0
        iny
        jmp     L9616
L9626:  pha
        lda     #$28
        jsr     L87F0
        pla
        clc
        adc     #$40
        jsr     L87F0
        lda     #$29
        jsr     L87F0
        iny
        jmp     L9616
L963C:  jsr     L87FB
        jsr     LAC07
        inc     $74
        lda     $74
        cmp     #$08
        bcc     L95FE
        rts
L964B:  jmp     L836D
L964E:  jsr     LA1C3
        bcs     L964B
        sta     $7C
        lda     #$02
        sta     $84
        lda     #$00
        sta     $7D
        jsr     L8CAD
        jsr     LA1BB
        bcs     L966A
        sta     $7D
        jsr     L8CA1
L966A:  jsr     L8CD4
        cmp     #$24
        bne     L9676
        inc     $84
        jsr     L8375
L9676:  cmp     #$28
        bne     L964B
        lda     #$00
        sta     $82
        jsr     L9C64
        sta     $7E
        jsr     L8CD4
        cmp     #$29
        beq     L9698
        inc     $82
        jsr     L9C6A
        sta     $7F
        jsr     L8378
        cmp     #$29
        bne     L964B
L9698:  jsr     L8CA1
        jsr     L96B2
        jsr     L8378
        beq     L96AC
        jsr     LA594
        jsr     L8CD4
        jmp     L964E
L96AC:  rts
L96AD:  lda     #$09
        jmp     LA98F
L96B2:  jsr     L9778
        bcs     L96AD
        lda     #$01
        sta     $2C
        jsr     LA1CD
        lda     $7E
        sta     $28
        inc     $28
        bne     L96C8
        inc     $29
L96C8:  lda     $82
        beq     L96D6
        lda     $7F
        sta     $2C
        inc     $2C
        bne     L96D6
        inc     $2D
L96D6:  jsr     L8B5C
        jsr     LA103
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8B5C
        lda     #$05
        sta     $2C
        lda     $82
        beq     L96EF
        inc     $2C
L96EF:  jsr     L8B39
        bcs     L9712
        lda     $8C
        sta     $19
        sta     $2C
        lda     $8D
        sta     $1A
        sta     $2D
        jsr     L8B39
        bcs     L9712
        lda     $04
        sta     $2D
        lda     $03
        sta     $2C
        jsr     L8BF7
        bcc     L9715
L9712:  jmp     L8A56
L9715:  ldy     #$00
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
        beq     L9738
        iny
        lda     $7F
        sta     ($8C),y
L9738:  lda     $28
        sta     $1F
        lda     $29
        sta     $20
        clc
        ldx     #$05
        lda     $82
        beq     L9748
        inx
L9748:  txa
        adc     $19
        sta     $19
        lda     #$00
        adc     $1A
        sta     $1A
L9753:  ldy     #$00
        lda     #$00
        sta     ($19),y
        inc     $19
        bne     L975F
        inc     $1A
L975F:  lda     $1A
        cmp     $20
        bne     L9753
        lda     $19
        cmp     $1F
        bne     L9753
        ldy     #$00
        lda     #$00
        sta     ($19),y
        iny
        sta     ($19),y
        iny
        sta     ($19),y
        rts
L9778:  lda     $7C
        sta     $8A
        lda     $7D
        sta     $8B
        lda     $84
        ora     #$80
        sta     $90
        jsr     LA10E
        jmp     LA007
L978C:  jsr     L9869
        lda     $B5
        bne     L9798
        lda     #$11
        jmp     LA98F
L9798:  lda     #<LAA46
        sta     $52
        lda     #>LAA46
        sta     $53
        jsr     L87F8
        jsr     LB3EB
        lda     #$0C
        ldx     #$05
        ldy     #$80
        jsr     BBG_DOS
        ldx     #$05
        ldy     #$80
        sty     L7B06
        stx     L7B07
        lda     #$08
        jsr     L7B33
        jmp     L7B3D
        nop
        nop
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
L97D0:  lda     #<LAA4F
        sta     $52
        lda     #>LAA4F
        sta     $53
        jsr     L87F8
        beq     L97E8
L97DD:  lda     #<LAA58
        sta     $52
        lda     #>LAA58
        sta     $53
        jsr     L87F8
L97E8:  lda     #$01
        sta     $52
        lda     #$05
        sta     $53
        jsr     L87F8
        jmp     LAC07
        jsr     L97DD
        jmp     L980C
L97FC:  ldx     #$00
        stx     $9F
        cmp     #$8B
        bne     L9809
        dec     $9F
        jsr     L8CA1
L9809:  jsr     L9869
L980C:  lda     $B5
        bne     L9815
        lda     #$01
        jmp     LA98F
L9815:  lda     #<LAA4F
        sta     $52
        lda     #>LAA4F
        sta     $53
        jsr     L87F8
        jsr     L8388
        jmp     L7F2E
        nop
        nop
        nop
        rts
        lda     $05
        sta     $0514
        lda     $06
        sta     $0515
        jsr     LB401
        bcs     L9864
        lda     $AD
        sta     $07
        lda     $AE
        sta     $08
        jsr     L985D
        jsr     LB3AB
        lda     $AD
        sta     $1D
        lda     $AE
        sta     $1E
        jsr     L985D
        jsr     LB3AB
        lda     $AD
        sta     $1F
        lda     $AE
        sta     $20
L985D:  ldy     #$00
        lda     #$00
        sta     ($AD),y
        rts
L9864:  lda     #$11
        jmp     LA98F
L9869:  jsr     L98A1
        lda     #$00
        sta     $B5
        jsr     L8378
        beq     L98A0
        jsr     L9C7C
        lda     $0401
        sta     $19
        lda     $0402
        sta     $1A
        lda     $0400
        beq     L98A0
        ldy     #$00
        ldx     #$00
L988B:  lda     ($19),y
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
L98E0:  beq     L98FA
        jsr     L9172
        jsr     L9598
        lda     #$03
        clc
        adc     $5B
        sta     $B2
        lda     #$00
        adc     $5C
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
L9908:  jsr     L8CC7
        cmp     #$80
        bcs     L9924
        cmp     #$12
        beq     L9927
        cmp     #$11
        beq     L9927
        cmp     #$0B
        beq     L9927
        cmp     #$22
        beq     L9934
        jsr     LA1C3
        bcc     L9942
L9924:  jmp     L8CA1
L9927:  jsr     L8CA1
        jsr     L8CA1
        jmp     L8CA1
L9930:  pla
        jmp     L8CA1
L9934:  pha
L9935:  jsr     L8CAD
        cmp     #$22
        beq     L9930
        and     #$FF
        bne     L9935
        pla
        rts
L9942:  pha
L9943:  jsr     L8CAD
        jsr     LA1BB
        bcc     L9943
        pla
        rts
L994D:  jsr     L9F75
        jsr     L8FF8
        lda     $B2
        sta     $09
        lda     $B3
        sta     $0A
        lda     $B4
        bne     L9985
L995F:  jsr     L8CC7
        beq     L996E
        cmp     #$91
        beq     L997E
        jsr     L9908
        jmp     L995F
L996E:  jsr     L8CAD
        beq     L9979
        jsr     L9927
        jmp     L995F
L9979:  lda     #$03
        jmp     LA98F
L997E:  lda     #$FF
        sta     $B4
        jsr     L9908
L9985:  lda     $90
        and     #$0F
        cmp     #$03
        beq     L99CB
        jsr     LA4FE
        ldy     #$00
        lda     $28
        sta     ($8C),y
        lda     $29
        iny
        sta     ($8C),y
        jsr     L8378
        beq     L99A6
        jsr     LA594
        jmp     L99AA
L99A6:  lda     #$00
        sta     $B4
L99AA:  lda     $09
        sta     $B2
        lda     $0A
        sta     $B3
        jsr     L9001
        jsr     L8CD4
        cmp     #$2C
        beq     L99BD
L99BC:  rts
L99BD:  jsr     L8CA1
        jsr     L8CD4
        jsr     LA1C3
        bcs     L99BC
        jmp     L994D
L99CB:  lda     #$00
        sta     $1B
        tax
        lda     #$2C
        sta     $3C
        jsr     L8CD4
        cmp     #$22
        bne     L99E0
        sta     $3C
        jsr     L8CA1
L99E0:  lda     ($09),y
        beq     L9A04
        cmp     $3C
        beq     L99F4
        sta     $0700,x
        inc     $1B
        inx
        jsr     L8CA1
        jmp     L99E0
L99F4:  jsr     L8CA1
        jsr     L8CD4
        cmp     #$2C
        bne     L9A08
        jsr     L8CA1
        jmp     L9A08
L9A04:  lda     #$00
        sta     $B4
L9A08:  lda     $1B
        sta     $0400
        lda     #$00
        sta     $0401
        lda     #$07
        sta     $0402
        jsr     L82EF
        jmp     L99AA
L9A1D:  jsr     L9C7C
        lda     $0400
        beq     L9A85
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
        jsr     L9C40
        lda     #$21
        sta     $91
        sta     $C7
        jsr     L9C40
        lda     #$42
        sta     $91
        sta     $C8
        jsr     L9C40
        lda     #$01
        sta     $CB
        sta     $CC
        sta     $CD
        lda     #$7F
        sta     $0401
        sta     $0405
        lda     #$07
        sta     $C9
        sta     $4015
        lda     #$0A
        sta     $63
L9A6F:  jsr     L86D4
        lda     $C9
        bmi     L9A86
        bne     L9A6F
        lda     #$00
        sta     $63
        sta     $4015
        pla
        sta     $0A
        pla
        sta     $09
L9A85:  rts
L9A86:  lda     #$00
        sta     $63
        sta     $4015
        jmp     L836D
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
        sta     $28
        lda     $B9,x
        sta     $2C
        jsr     L8B5C
        lda     $28
        ldx     $CA
        sta     $CB,x
        lda     $91
        sta     $C6,x
        rts
L9C40:  lda     $0400
        beq     L9C57
        dec     $0400
        jsr     L8C9D
        cmp     #$3A
        beq     L9C57
        ldy     #$00
        sta     ($91),y
        inc     $91
        bne     L9C40
L9C57:  ldy     #$00
        lda     #$00
        sta     ($91),y
        rts
L9C5E:  jmp     L836D
L9C61:  jmp     L8370
L9C64:  jsr     L8CA1
        jmp     L9C6D
L9C6A:  jsr     LA594
L9C6D:  jsr     L9C82
        jsr     LA3EF
        lda     $0401
        bne     L9C61
        lda     $0400
        rts
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
        lda     $28
        eor     $2C
        sta     $28
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
        lda     $28
        ora     $2C
        sta     $28
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
        lda     $28
        and     $2C
        sta     $28
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
        sta     $19
        jsr     L9D4B
        pha
        lda     #$02
        sta     $27
        pla
        jmp     L9D12
L9D4B:  jmp     ($0019)
L9D4E:  jsr     L9D89
L9D51:  cmp     #$F9
        beq     L9D5A
        cmp     #$FA
        beq     L9D63
        rts
L9D5A:  lda     #<LA1D4
        pha
        lda     #>LA1D4
        pha
        jmp     L9D69
L9D63:  lda     #<LA1EE
        pha
        lda     #>LA1EE
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
        sta     $19
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
L9DB4:  lda     #<LA1FA
        pha
        lda     #>LA1FA
        pha
        jmp     L9DC3
L9DBD:  lda     #<LA1F4
        pha
        lda     #>LA1F4
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
        sta     $19
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
        sta     $28
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
        sta     $28
        lda     $0401,x
        sta     $29
        rts
L9E23:  jmp     L8370
L9E26:  jsr     L9E0D
        jsr     L8A7C
        lda     $28
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
L9E71:  stx     $19
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
        sta     $19
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
        sta     $28
        lda     #$00
        adc     $8F
        sta     $29
        jsr     L8A44
        lda     $28
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
LA007:  lda     $8E
        sta     $8C
        lda     $8F
        sta     $8D
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
        sta     $1B
        pla
        sta     $19
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
        cmp     $19
        bcc     LA0B9
        tax
        iny
        lda     ($8E),y
        cmp     $1B
        bcc     LA0B9
        jsr     LA1CD
        inx
        bne     LA086
        inc     $29
LA086:  stx     $28
        lda     $1B
        sta     $2C
        jsr     L8B5C
        lda     $19
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
LA0B0:  lda     $28
        sta     $8C
        lda     $29
        sta     $8D
        rts
LA0B9:  jmp     L8370
LA0BC:  jsr     L8CA1
        pla
        sta     $19
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
        cmp     $19
        bcc     LA0B9
        lda     $19
        sta     $28
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
        sta     $28
        lda     $74
        beq     LA179
        lda     $4B
        sta     $2C
LA179:  inc     $28
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
        lda     $28
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
LA1D4:  jsr     L8AEC
LA1D7:  ldx     $30
        dex
        dex
        dex
        lda     $28
        sta     $0400,x
        lda     $29
        sta     $0401,x
        lda     $2A
        sta     $0402,x
        jmp     L8CD4
LA1EE:  jsr     L8A73
        jmp     LA1D7
LA1F4:  jsr     L8B51
        jmp     LA1D7
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
        lda     #$01
        sta     $28
        lda     #$00
        sta     $29
        jmp     LA1D7
LA218:  lda     #$0A
        jmp     LA98F
LA21D:  lda     $29
        pha
        jsr     LA1FA
        lda     $2A
        sta     $28
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
LA23E:  jsr     L8C03
        bne     LA24C
LA243:  lda     #$00
LA245:  sta     $28
        sta     $29
        jmp     LA1D7
LA24C:  lda     #$FF
        jmp     LA245
LA251:  jsr     L8C03
        beq     LA24C
        bcc     LA24C
        jmp     LA243
LA25B:  jsr     L8C03
        bcs     LA24C
        jmp     LA243
LA263:  jsr     L8C03
        beq     LA24C
        jmp     LA243
LA26B:  jsr     L8C03
        beq     LA243
        bcs     LA24C
        jmp     LA243
LA275:  jsr     L8C03
        bcc     LA24C
        jmp     LA243
LA27D:
        .addr   LA23E,LA25B,LA251,LA263,LA26B,LA275
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
        ldy     $28
        lda     $0645,y
        ldy     #$00
        and     #$10
        beq     LA2B0
        ldy     #$FF
LA2B0:  sty     $28
        sty     $29
        jsr     LA363
        jmp     L8CD4
LA2BA:  lda     #$00
        sta     $28
LA2BE:  sta     $29
        sta     $2A
        rts
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
        sta     $28
        rts
LA2DE:  jmp     L836D
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
LA2F3:  lda     $49
        sta     $28
        lda     #$00
        sta     $29
LA2FB:  inc     $30
        inc     $30
        inc     $30
LA301:  lda     #$02
        sta     $27
LA305:  ldx     $30
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
LA31A:  jsr     LA320
        jmp     LA2FB
LA320:  sec
        lda     $03
        sbc     $1F
        sta     $28
        lda     $04
        sbc     $20
        sta     $29
        rts
LA32E:  jsr     LA3EC
        lda     $48
LA333:  sta     $28
        lda     #$00
        sta     $29
        jmp     LA363
LA33C:  jsr     LA3E6
        lda     $28
        asl     a
        asl     a
        asl     a
        tax
        rts
LA346:  jsr     LA33C
        lda     $06C6,x
        jmp     LA333
LA34F:  jsr     LA33C
        lda     $06C7,x
        jmp     LA333
LA358:  jsr     LA3EC
        ldy     #$00
        lda     ($28),y
        sta     $28
        sty     $29
LA363:  jsr     LA293
        jmp     LA301
LA369:  jsr     LA3EC
        lda     $29
        bmi     LA373
        jmp     LA363
LA373:  jsr     L8A7C
        jmp     LA363
LA379:  jsr     LA3EC
        lda     $29
        bmi     LA38D
        beq     LA396
LA382:  lda     #$00
        sta     $29
        lda     #$01
        sta     $28
        jmp     LA363
LA38D:  lda     #$FF
LA38F:  sta     $28
        sta     $29
        jmp     LA363
LA396:  lda     $28
        bne     LA382
        lda     #$00
        jmp     LA38F
LA39F:  jsr     LA3EC
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
        jsr     L8B59
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
        jsr     L8B59
        asl     $29
        rol     $36
        rol     $37
        lda     $36
        sta     $28
        lda     $37
        sta     $29
        jmp     LA363
LA3E1:  lda     #$0B
        jmp     LA98F
LA3E6:  jsr     LA3EC
        jmp     LA548
LA3EC:  jsr     LA2C3
LA3EF:  lda     #$02
        .byte   $AE
LA3F2:  lda     #$03
        cmp     $27
        bne     LA3E1
        rts
LA3F9:  jsr     LA2C3
        jmp     LA3F2
LA3FF:  jsr     LA3F9
LA402:  lda     #$00
        jsr     LA2BE
        jmp     LA363
LA40A:  jsr     LA3F9
        lda     $28
        beq     LA402
        ldy     #$00
        lda     ($29),y
        sta     $28
        jmp     LA402
LA41A:  jmp     L8370
LA41D:  jsr     LACA1
        jsr     LA3E6
        lda     $28
        cmp     #$1C
        bcs     LA41A
        sta     $48
        jsr     LA59E
        jsr     LA548
        lda     $28
        cmp     #$18
        bcs     LA41A
        sta     $49
        jsr     L8CD4
        cmp     #$29
        beq     LA44F
        jsr     LA59E
        lda     $28
        beq     LA44F
        jsr     LBA37
        lda     $9C
        jmp     LA459
LA44F:  jsr     LABA6
        lda     #$05
        jsr     L8808
        lda     $73
LA459:  sta     $28
        jsr     LACAA
        jmp     LA464
LA461:  jsr     LA3E6
LA464:  lda     $28
LA466:  jsr     LA473
LA469:  lda     #$03
        sta     $27
        jsr     LA293
        jmp     LA305
LA473:  ldx     $21
        sta     $0700,x
        inc     $21
        lda     #$01
        sta     $28
        stx     $29
        lda     #$07
        sta     $2A
        rts
LA485:  jsr     LA3EC
        jsr     L8D96
LA48B:  jsr     L8CEB
        lda     #$00
        sta     $90
        ldy     $21
        ldx     $46
LA496:  lda     $3D,x
        beq     LA4A4
        sta     $0700,y
        inx
        iny
        inc     $90
        jmp     LA496
LA4A4:  lda     $21
        sty     $21
        sta     $29
        lda     #$07
        sta     $2A
        lda     $90
        sta     $28
        jmp     LA469
LA4B5:  jsr     LA3EC
        jsr     L8DD8
        jmp     LA48B
LA4BE:  jsr     LA3F9
        lda     $28
        beq     LA4EC
        jsr     L8FF8
        lda     $28
        sta     $74
        ldx     #$00
        ldy     #$00
LA4D0:  lda     ($29),y
        sta     $0500,x
        inx
        iny
        dec     $74
        bne     LA4D0
        lda     #$00
        sta     $0500,x
        jsr     L8A32
        jsr     LA4FE
        jsr     L9001
        jmp     LA363
LA4EC:  jsr     LA2BA
        jmp     LA363
LA4F2:  jsr     L8CA1
        jsr     LA4FE
        jmp     L8A7C
LA4FB:  jsr     L8CA1
LA4FE:  ldy     #$00
        lda     ($09),y
        cmp     #$2D
        beq     LA4F2
        cmp     #$2B
        beq     LA4FB
        cmp     #$26
        beq     LA511
        jmp     L8CF9
LA511:  jsr     L8CA1
        lda     ($09),y
        cmp     #$48
        bne     LA51D
        jsr     L8CA1
LA51D:  jmp     L8DEB
LA520:  jsr     L8378
        beq     LA532
        cmp     #$28
        bne     LA532
        jsr     LA2C3
        jsr     LB263
        jmp     LA466
LA532:  jsr     LAF88
        beq     LA542
        jsr     LA473
LA53A:  jsr     LA2FB
        lda     #$03
        sta     $27
        rts
LA542:  jsr     LA2BA
        jmp     LA53A
LA548:  lda     $29
        bne     LA552
        rts
LA54D:  cmp     #$20
        bcs     LA552
        rts
LA552:  jmp     L8370
LA555:  jsr     LA3F9
        lda     $28
        pha
        lda     $29
        pha
        lda     $2A
        pha
        jsr     LA59E
        jsr     LA548
        lda     $28
        jsr     LA54D
        pla
        sta     $1A
        pla
        sta     $19
        pla
        rts
LA574:  jsr     LA555
        sta     $4A
        lda     $28
        cmp     $4A
        bcs     LA58A
LA57F:  lda     $19
        sta     $29
        lda     $1A
        sta     $2A
        jmp     LA469
LA58A:  lda     $4A
        sta     $28
        jmp     LA57F
LA591:  jmp     L836D
LA594:  jsr     L8CD4
        cmp     #$2C
        bne     LA591
        jmp     L8CA1
LA59E:  jsr     LA594
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
        sta     $28
        stx     $30
        jmp     LA3EF
LA5BD:  jsr     LA555
        sta     $4A
        lda     $28
        cmp     $4A
        bcs     LA58A
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
LA5E0:  lda     ($19),y
        sta     $0700,x
        inx
        iny
        dec     $90
        bne     LA5E0
        lda     $21
        sta     $29
        stx     $21
        lda     #$07
        sta     $2A
        jmp     LA469
LA5F8:  jmp     L8370
LA5FB:  jsr     LA3F9
        lda     $28
        pha
        lda     $29
        pha
        lda     $2A
        pha
        jsr     LA59E
        jsr     LA548
        lda     $28
        beq     LA5F8
        jsr     LA54D
        sec
        sbc     #$01
        pha
        jsr     LA59E
        jsr     LA548
        lda     $28
        jsr     LA54D
        sta     $1C
        pla
        sta     $1B
        pla
        sta     $1A
        pla
        sta     $19
        pla
        cmp     $1B
        bcc     LA66E
        sec
        sbc     $1B
        cmp     $1C
        bcc     LA669
LA63A:  lda     $1B
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
LA651:  lda     ($19),y
        sta     $0700,x
        inx
        iny
        dec     $90
        bne     LA651
        lda     $21
        sta     $29
        stx     $21
        lda     #$07
        sta     $2A
        jmp     LA469
LA669:  sta     $1C
        jmp     LA63A
LA66E:  jsr     LA2BA
        jmp     LA469
LA674:  jmp     L8370
LA677:  jsr     LA3E6
LA67A:  lda     $28
        beq     LA685
        cmp     #$01
        bne     LA674
        lda     #$17
        .byte   $AE
LA685:  lda     #$16
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
LA69F:  lda     ($19),y
        lsr     a
        rol     $90
        nop
        nop
        dex
        bne     LA69F
        rts
LA6AA:  jsr     LA677
        jsr     LA67A
        lda     $90
        and     #$0F
LA6B4:  sta     $28
        jmp     LA363
LA6B9:  jsr     LA677
        lda     $90
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jmp     LA6B4
LA6C5:  asl     a
        asl     a
        asl     a
        asl     a
        rts
LA6CA:  jsr     L9C6D
        and     #$03
        asl     a
        asl     a
        asl     a
        sta     $74
        lda     $32
        and     #$E7
        ora     $74
        sta     $32
        jmp     LB3F1
LA6DF:  ldx     #$00
        lda     #$EF
LA6E3:  sta     $0200,x
        inx
        bne     LA6E3
        rts
LA6EA:  ldx     #$00
        stx     $65
        cmp     #$42
        beq     LA6FA
        cmp     #$53
        bne     LA743
        lda     #$10
        sta     $65
LA6FA:  jsr     L9C64
        cmp     #$04
        bcs     LA740
        pha
        lda     #$04
        sta     $02
        lda     #$00
        sta     $7C
LA70A:  jsr     L9C6A
        ldy     $7C
        sta     a:$003D,y
        inc     $7C
        dec     $02
        bne     LA70A
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
        beq     LA737
        iny
LA737:  tya
        jmp     L8808
LA73B:  cmp     #$08
        bcs     LA740
        rts
LA740:  jmp     L8370
LA743:  jmp     L836D
LA746:  cmp     #$2C
        beq     LA76F
        jsr     L9C6D
        beq     LA753
        cmp     #$01
        bne     LA740
LA753:  asl     a
        tax
        lda     LC3A5,x
        sta     $52
        lda     LC3A6,x
        sta     $53
        lda     #$00
        sta     $65
        jsr     LA78A
        jsr     L8378
        beq     LA789
        cmp     #$2C
        bne     LA743
LA76F:  jsr     L9C64
        cmp     #$03
        bcs     LA740
        asl     a
        tax
        lda     LC3A9,x
        sta     $52
        lda     LC3AA,x
        sta     $53
        lda     #$01
        sta     $65
        jsr     LA78A
LA789:  rts
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
LA7D3:  pla
        jsr     LA6C5
        tax
        ldy     #$04
LA7DA:  lda     #$EF
        sta     $0200,x
        sta     $0203,x
        sta     $0201,x
        inx
        inx
        inx
        inx
        dey
        bne     LA7DA
        rts
        jmp     L8370
        jmp     L836D
LA7F3:  cmp     #$9A
        beq     LA7C0
        cmp     #$9B
        beq     LA7CC
        jsr     L9C6D
        jsr     LA73B
        pha
        jsr     L8378
        beq     LA7D3
        jsr     L9C6A
        pha
        jsr     L9C6A
        sta     $1B
        pla
        sta     $19
        pla
        sta     $75
        tax
        lda     $063D,x
        sta     $90
        lda     $75
        asl     a
        asl     a
        tax
        asl     a
        asl     a
        pha
        tay
        lda     #$04
        sta     $74
LA829:  lda     $0680,x
        sta     $0201,y
        inx
        iny
        iny
        iny
        iny
        dec     $74
        bne     LA829
        pla
        tay
        lda     $1B
        sta     $0200,y
        lda     $19
        sta     $0203,y
        lda     $90
        bne     LA849
        rts
LA849:  lda     #$00
        sta     $1C
        lda     #$08
        sta     $1A
        jsr     LA865
        lda     #$00
        sta     $1A
        lda     #$08
        sta     $1C
        jsr     LA865
        lda     #$08
        sta     $1A
        sta     $1C
LA865:  iny
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
        rts
LA87A:  cmp     #$A1
        bne     LA881
        jmp     LBFE4
LA881:  jmp     L836D
        jmp     L8370
LA887:  cmp     #$8A
        bne     LA87A
        jsr     L9C64
        jsr     LA73B
        pha
        jsr     LA594
        jsr     LA289
        lda     #$04
        sta     $75
        jsr     L9C6D
        pha
LA8A0:  jsr     L9C6A
        pha
        dec     $75
        bne     LA8A0
        jsr     LA293
        jsr     L85A2
        pla
        ror     a
        ror     a
        and     #$80
        sta     $19
        pla
        ror     a
        ror     a
        ror     a
        and     #$40
        ora     $19
        sta     $19
        pla
        ror     a
        ror     a
        ror     a
        ror     a
        and     #$20
        ora     $19
        sta     $19
        pla
        sta     $AD
        pla
        and     #$03
        ora     $19
        sta     $19
        pla
        sta     $75
        jsr     LA6C5
        ldy     #$04
        tax
LA8DD:  lda     #$00
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
        bne     LA8DD
        jsr     L9C7C
        lda     $0401
        sta     $1B
        lda     $0402
        sta     $1C
        ldy     #$00
        lda     $75
        asl     a
        asl     a
        tax
        lda     $0400
        beq     LA91F
LA90F:  lda     ($1B),y
        sta     $0680,x
        inx
        iny
        cpy     #$04
        beq     LA92A
        dec     $0400
        bne     LA90F
LA91F:  lda     #$20
LA921:  sta     $0680,x
        inx
        iny
        cpy     #$04
        bne     LA921
LA92A:  ldx     $75
        lda     $AD
        sta     $063D,x
        rts
LA932:  jsr     LAB75
        lda     #$20
        sta     $74
        lda     #$0C
        jsr     L8808
        ldx     #$00
LA940:  lda     $74
        clc
        adc     #$04
        jsr     L81F8
        lda     PPUDATA
        lda     PPUDATA
        pha
        lda     PPUDATA
        sta     $F3
        lda     $74
        jsr     L81F8
        pla
        sta     PPUDATA
        lda     $F3
        sta     PPUDATA
        inx
        bne     LA940
        inc     $74
        lda     $74
        cmp     #$24
        bne     LA940
        jsr     LABE6
        jsr     LB3F1
        lda     #$0B
        jmp     L8808
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
        lda     #<LAA3F
        sta     $52
        lda     #>LAA3F
        sta     $53
        jsr     L87B7
        jsr     L873B
LA9B8:  lda     #$FF
        cmp     $0C
        bne     LA9C2
        cmp     $0B
        beq     LA9D0
LA9C2:  lda     #<LAA34
        sta     $52
        lda     #>LAA34
        sta     $53
        jsr     L87B7
        jsr     LA9D9
LA9D0:  jsr     L87FB
        jsr     LB30F
        jmp     L8221
LA9D9:  lda     $0B
        sta     $28
        lda     $0C
        sta     $29
LA9E1:  jsr     L8D3C
        jsr     L8CEB
        jmp     L865F
LA9EA:  jsr     LAC03
        lda     #$00
        sta     $62
        lda     #<LAA39
        sta     $52
        lda     #>LAA39
        sta     $53
        jsr     L87B7
        jmp     LA9B8
LA9FF:  lda     #$4F
        jsr     LAC0D
        lda     #$4B
        jmp     LAC0D
LAA09:  lda     #<LC358
        sta     $52
        lda     #>LC358
        sta     $53
        jsr     L87F8
        jsr     LA320
        jsr     LA9E1
        jsr     L87FB
        lda     #<LAA28
        sta     $52
        lda     #>LAA28
        sta     $53
        jmp     L87F8
LAA28:
        .byte   " BYTES FREE", $00
LAA34:
        .byte   " IN ", $00
LAA39:
        .byte   "BREAK", $00
LAA3F:
        .byte   " ERROR", $00
LAA46:
        .byte   "WRITING ", $00
LAA4F:
        .byte   "LOADING ", $00
LAA58:
        .byte   "SKIP ", $00
LAA5E:
        .byte   "N"
LAA5F:
        .byte   "FSNRGODILOVOMULSODDDZTMSTFTCCUFMOTP"
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
        sta     $4001
        sta     $4005
        sta     $4015
        sta     $4C
        sta     $66
        sta     $67
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
LAB09:  lda     #<LC3BF
        sta     $52
        lda     #>LC3BF
        sta     $53
        lda     #$00
        sta     $65
        jsr     LA78A
        inc     $65
        jmp     LA78A
LAB1D:
        .byte   "CLS ", $0D, $00
        .byte   "PRINT ", $00
        .byte   "GOTO ", $00
        .byte   "CHR$(", $00
        .byte   "SYSTEM", $0D, $00
        .byte   "CONT", $0D, $00
        .byte   "LIST", $0D, $00
        .byte   "RUN", $0D, $00
LAB4F:  ldy     #$04
        ldx     #$C0
LAB55:  lda     #$20
        put_tile
        dex
        bne     LAB55
        dey
        bne     LAB55
; 原版在这里清属性表；6576 的调色板组在图块条目里，已随图块写成 0
LAB5E:  lda     #$00
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
        sta     $28
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
        adc     $28
        sta     $4A
        lda     #$00
        adc     $29
        clc
        adc     $4F
        sta     $4B
        rts
LABD8:  lda     $4A
        asl     a
        sta     $F1
        lda     $4B
        and     #$0F
        rol     a
        sta     PPUADDR
        lda     $F1
        sta     PPUADDR
        rts
LABE3:  put_tile
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
        sta     $28
        lda     LAF02,x
        sta     $29
        jmp     ($0028)
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
        jsr     video_write_ctrl
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
        ldx     $49
        dex
        txa
        tax
        inx
        lda     $0601,x
        rts
        jmp     LAC07
LAD22:  lda     $48
        cmp     #$15
        bcs     LAD33
        cmp     #$0E
        bcs     LAD36
        cmp     #$07
        bcs     LAD39
        lda     #$07
        .byte   $EC
LAD33:  lda     #$1C
        .byte   $EC
LAD36:  lda     #$15
        .byte   $EC
LAD39:  lda     #$0E
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
        sta     $19
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
LAF01:
        .addr   LAC4D,LAD4C,LAC4D,LAC4E,LACB3,LACD2,LAC4D,LB30F
        .addr   LADB2,LAD22,LAD94,LAD8B,LAB75,LAD8F,LAC4D,LAC4D
        .addr   LAC4D,LAC4D,LAE48,LAC4D,LAC4D,LAC4D,LADA1,LADA9
        .addr   LAC4D,LAC4D,LACF4,LAC4D,LAD76,LAD64,LAD53,LAD5A
LAF41:  lda     #$00
        rts
; 功能键 F1-F8（$F0-$F7）展开成 $0480 起的 8 字节一条的预设字符串；一次按下只展开一遍，展开完 $68 置位，松开键后清除
LAF44:  ldx     $68
        bne     LAF41
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
LB0E1:  asl     a
        asl     a
        asl     a
        rts
LB254:  jsr     LABA6
        lda     #$08
        bne     LB260
LB25B:  jsr     LABA6
        lda     #$09
LB260:  jmp     L8808
; 取一个键（带按键重复、光标闪烁与去抖）：计时以帧为单位，每次取键等一帧（gk_poll）
; 首次重复前 30 帧（$77），之后每 4 帧（$79）重复一次，光标每 30 帧（$74）翻转一次
LB263:  lda     #$00
        sta     $5E
        jsr     gk_poll
        beq     LB2B2
        ldx     $66
        bne     LB295
        cmp     $75
        bne     LB2B2
        lda     $7B
        bne     LB298
        jsr     LB254
LB27B:  jsr     gk_poll
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
LB298:  jsr     gk_poll
        beq     LB2B2
        cmp     $75
        bne     LB2B2
        dec     $79
        bne     LB298
        lda     #$04
        sta     $79
        jsr     LB254
        jmp     LB306
LB2AF:  jsr     LB25B
LB2B2:  lda     #$00
        sta     $7B
        lda     #$1E
        sta     $77
        lda     #$04
        sta     $79
        lda     #$01
        sta     $78
        jsr     LB254
        lda     #$1E
        sta     $74
LB2C7:  jsr     gk_poll
        bne     LB2F1
        dec     $74
        bne     LB2C7
        jsr     LB25B
        lda     $5E
        bne     LB293
        lda     #$1E
        sta     $74
LB2DB:  jsr     gk_poll
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
LB2FB:  jsr     gk_poll
        cmp     $75
        bne     LB2EB
        dec     $74
        bne     LB2FB
LB306:  jsr     LB25B
        jsr     LB314
        lda     $75
        rts
LB30F:  lda     #$F0
        sta     $19
        .byte   $AE
LB314:  lda     #$01
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
LB336:  dey
        bne     LB336
        dec     $19
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
        jmp     video_write_ctrl
LB3F1:  lda     $32
        ora     #$80
        jmp     video_write_ctrl
        jsr     LB3EB
        lda     #$01
        jmp     LB406
LB401:  jsr     LB3EB
        lda     #$00
LB406:  sta     $B1
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
        sta     $19
LB4DC:  lda     $19
        clc
        adc     $19
        sta     $19
        jsr     LB4C0
        bcs     LB49D
        bne     LB4EF
        jsr     LB3B2
        inc     $19
LB4EF:  dec     $74
        bne     LB4DC
        jsr     LB4A6
        bcs     LB49D
        lda     $19
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
        sta     $19
        lda     #$4E
        sta     $1A
LB523:  jsr     LB586
        dec     $19
        bne     LB523
        dec     $1A
        bne     LB523
        lda     $91
        sta     $19
LB532:  jsr     LB590
        dec     $19
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
LB56F:  sta     $19
        clc
        adc     $19
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
        sta     $1B
        sta     $1C
        jmp     LB597
LB590:  pha
        lda     #$6A
        sta     $1B
        sta     $1C
LB597:  lda     #$04
        sta     $4016
LB59C:  dec     $1C
        bne     LB59C
        lda     #$FF
        sta     $4016
LB5A5:  dec     $1B
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
        sta     $19
        lda     LB6A4,x
        sta     $1A
        jmp     ($0019)
LB6A3:
        .addr   LB87F,LBB22,LBB1F,LB5AB,LBBFB,LB72D
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
        bit     PPUDATA
        jmp     LB6F1
LB6EB:  lda     $0520,x
        put_tile
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
LB77C:
        .byte   "X:", $0D, $0A, "Y:", $00
LB783:  jsr     LAD8F
        lda     #$16
        sta     $49
        lda     #<LB77C
        sta     $52
        lda     #>LB77C
        sta     $53
        jsr     L87F8
        lda     $0505
        sta     $28
        lda     #$02
        sta     $48
        lda     #$16
        sta     $49
        jsr     LB7B0
        lda     $0506
        sta     $28
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
LB7C4:
        .byte   "MODE  ", $00
LB7CB:
        .byte   "SELECT", $00
LB7D2:
        .byte   "MOVE  ", $00
LB7D9:
        .byte   "COPY  ", $00
LB7E0:
        .byte   "CHAR  ", $00
LB7E7:
        .byte   "CLEAR ", $00
LB7EE:
        .byte   "FILE  ", $00
LB7F5:
        .addr   LB7CB,LB7D9,LB7D2,LB7E7,LB7EE,LB7E0
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
        lda     #<LB7C4
        sta     $52
        lda     #>LB7C4
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
LB847:
        .byte   $80,$88,$90,$98,$B8,$C0,$C8,$D0,$D8,$E0,$E8,$F0,$F8
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
        sta     $19
        lda     LB8D8,x
        sta     $1A
        jmp     ($0019)
LB8CF:
        .byte   $1B,$0D,$0C,$0B,$08,$12,$20,$64
LB8D7:
        .addr   LB917,LB854,LB92E,LB937,LB941,LB94F,LB95D,LB86E
LB8E7:  lda     #$00
        sta     $88
        beq     LB8F5
LB8ED:  inc     $88
        lda     $88
        cmp     #$04
        beq     LB8E7
LB8F5:  sta     $9C                             ; 选中的调色板组，写进图块选择行的 16 个图块
        lda     #$2E
        sta     $4A
        lda     #$27
        sta     $4B
        lda     #$12
        jmp     L8808
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
        put_tile
        inx
        lda     #$20
        put_tile
        inx
        dey
        bne     LB9D6
        jsr     LABE6
        jmp     LACAA
; 调色板组在 6576 的每个图块条目里；原版的“属性”是每 2x2 个图块一组，这里仍按 2x2 块读写
; 求 $48、$49 所在 2x2 块左上角图块的名称表地址，放进 $4A/$4B；和原版一样把 $48 加 2、$49 加 3
LB9EC:  inc     $48
        inc     $48
        lda     $49
        clc
        adc     #$03
        sta     $49
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     $4F
        sta     $4B
        lda     $49
        and     #$06
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        sta     $4A
        lda     $48
        and     #$1E
        ora     $4A
        sta     $4A
        rts
; 读 2x2 块的调色板组到 $9C（0-3）
LBA37:  jsr     LB9EC
        lda     #$10
        jsr     L8808
        lda     $73
        sta     $9C
        rts
LBA4E:  jsr     L9C6D
        pha
        jsr     L9C6A
        lda     $0400
        sta     $49
        pla
        sta     $48
        jsr     L9C6A
        lda     $0400
        and     #$03
        sta     $9C
; 把 $9C 的调色板组写进 2x2 块的 4 个图块
LBA67:  jsr     LB9EC
        lda     #$11
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
        sec
        sbc     #$24
        jsr     video_read_nt_page
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
        sec
        sbc     #$24
        jsr     video_write_nt_page
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
        .byte   $AE
LBB22:  lda     #$01
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
LBBDF:
        .byte   "SAVE(S),LOAD(L)?", $00
LBBF0:
        .byte   "FILE NAME", $22, $00
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
        lda     #<LBBDF
        sta     $52
        lda     #>LBBDF
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
        lda     #<LBBF0
        sta     $52
        lda     #>LBBF0
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
LBC75:
        .addr   LBCA5,LBCA5,LBCB1,LBCB7,LBCBD,LBCAB,LBCB7,LBCB1
        .addr   LBCC3,LBCC3,LBCB1,LBCB1,LBCC9,LBCAB,LBCB1,LBCAB
LBC95:
        .byte   $00,$1C,$00,$00,$00,$00,$38,$58,$00,$0C,$78,$20,$00,$48,$88,$58
LBCA5:
        .addr   LBCF3,LBD29,LBCCF
LBCAB:
        .addr   LBCFC,LBD95,LBCEA
LBCB1:
        .addr   LBCFC,LBD3B,LBCD8
LBCB7:
        .addr   LBCFC,LBD4D,LBCE1
LBCBD:
        .addr   LBD0E,LBD5F,LBCEA
LBCC3:
        .addr   LBD05,LBD71,LBCE1
LBCC9:
        .addr   LBD05,LBD83,LBCE1
LBCCF:
        .byte   $00,$00,$40,$40,$40,$00,$00,$00,$00
LBCD8:
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00
LBCE1:
        .byte   $00,$00,$40,$40,$C0,$80,$80,$00,$00
LBCEA:
        .byte   $00,$00,$40,$40,$40,$00,$00,$00,$00
LBCF3:
        .byte   $10,$20,$10,$30,$10,$20,$10,$30,$10
LBCFC:
        .byte   $20,$20,$20,$20,$20,$20,$20,$20,$20
LBD05:
        .byte   $10,$10,$10,$10,$10,$10,$10,$10,$10
LBD0E:
        .byte   $10,$20,$20,$20,$20,$20,$20,$20,$20
LBD17:
        .byte   $00
LBD18:
        .byte   $00,$00,$FE,$02,$FE,$02,$00,$02,$02,$00,$02,$FE,$02,$FE,$00,$FE
        .byte   $FE
LBD29:
        .addr   LBDC7,LBDC3,LBDCF,LBDBF,LBDCF,LBDC3,LBDCB,LBDBB
        .addr   LBDCB
LBD3B:
        .addr   LBDD7,LBDD7,LBDD7,LBDD7,LBDD7,LBDD7,LBDD7,LBDD7
        .addr   LBDD7
LBD4D:
        .addr   LBDE3,LBDDB,LBDEF,LBDE7,LBDF7,LBDDF,LBDF3,LBDE3
        .addr   LBDEB
LBD5F:
        .addr   LBE1B,LBE1F,LBE27,LBE27,LBE27,LBE1B,LBE23,LBE23
        .addr   LBE23
LBD71:
        .addr   LBDFB,LBDFB,LBE0F,LBE07,LBE17,LBDFF,LBE13,LBE03
        .addr   LBE0B
LBD83:
        .addr   LBDAF,LBDAF,LBDB7,LBDB3,LBDB7,LBDAF,LBDB7,LBDB3
        .addr   LBDB7
LBD95:
        .addr   LBDA7,LBDA7,LBDAB,LBDAB,LBDAB,LBDA7,LBDA7,LBDA7
        .addr   LBDA7
LBDA7:
        .byte   $70,$71,$72,$73
LBDAB:
        .byte   $71,$70,$73,$72
LBDAF:
        .byte   $D4,$EF,$EF,$EF
LBDB3:
        .byte   $D0,$EF,$EF,$EF
LBDB7:
        .byte   $D2,$EF,$EF,$EF
LBDBB:
        .byte   $00,$01,$02,$03
LBDBF:
        .byte   $01,$00,$03,$02
LBDC3:
        .byte   $14,$15,$16,$17
LBDC7:
        .byte   $18,$19,$1A,$1B
LBDCB:
        .byte   $0C,$0D,$0E,$0F
LBDCF:
        .byte   $0D,$0C,$0F,$0E
LBDD3:
        .byte   $15,$14,$17,$16
LBDD7:
        .byte   $38,$39,$3A,$3B
LBDDB:
        .byte   $50,$51,$52,$53
LBDDF:
        .byte   $52,$53,$50,$51
LBDE3:
        .byte   $40,$41,$42,$43
LBDE7:
        .byte   $41,$40,$43,$42
LBDEB:
        .byte   $48,$49,$4A,$4B
LBDEF:
        .byte   $49,$48,$4B,$4A
LBDF3:
        .byte   $4A,$4B,$48,$49
LBDF7:
        .byte   $4B,$4A,$49,$48
LBDFB:
        .byte   $A0,$A1,$A2,$A3
LBDFF:
        .byte   $A2,$A3,$A0,$A1
LBE03:
        .byte   $98,$99,$9A,$9B
LBE07:
        .byte   $99,$98,$9B,$9A
LBE0B:
        .byte   $9C,$9D,$9E,$9F
LBE0F:
        .byte   $9D,$9C,$9F,$9E
LBE13:
        .byte   $9E,$9F,$9C,$9D
LBE17:
        .byte   $9F,$9E,$9D,$9C
LBE1B:
        .byte   $68,$69,$6A,$6B
LBE1F:
        .byte   $6C,$6D,$6E,$6F
LBE23:
        .byte   $60,$61,$62,$63
LBE27:
        .byte   $61,$60,$63,$62
LBE2B:
        .byte   $6D,$6C,$6F,$6E
LBE2F:
        .byte   $69,$68,$6B,$6A
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
        lda     #<LBE2B
        sta     $DB
        lda     #>LBE2B
        bne     LBF55
LBF43:  lda     #<LBE2F
        sta     $DB
        lda     #>LBE2F
        bne     LBF55
LBF4B:  lda     $D7
        beq     LBF67
        lda     #<LBDD3
        sta     $DB
        lda     #>LBDD3
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
LBFDE:  jmp     L8370
LBFE1:  jmp     L836D
LBFE4:  jsr     L8CA1
        jsr     LA289
        jsr     L9C6D
        jsr     LA73B
        jsr     LA293
        lda     $0400
        tax
        lda     #$00
        sta     $0645,x
        txa
        sta     $A4
        asl     a
        asl     a
        asl     a
        sta     $A5
        jsr     L85A2
        jsr     L8CD4
        cmp     #$8A
        bne     LBFE1
        jsr     L8CA1
        jsr     LA289
        jsr     LC0A2
        and     #$0F
        sta     $06C1,x
        jsr     LC09F
        cmp     #$09
        bcs     LBFDE
        sta     $06C0,x
        lda     $06C1,x
        asl     a
        tay
        lda     LBC75,y
        sta     $19
        lda     LBC76,y
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
        ldy     $A4
        sta     $0645,y
        jsr     LC0A8
        bcs     LC09E
        jsr     LC09F
        sta     $06C2,x
        sta     $06C4,x
        jsr     LC09F
        clc
        adc     #$01
        sta     $06C3,x
        sta     $06C5,x
        jsr     LC0A8
        bcs     LC09E
        jsr     LC09F
        ldy     #$00
        lda     $0400
        beq     LC076
        ldy     #$20
LC076:  ldx     $A4
        tya
        ora     $0645,x
        sta     $0645,x
        jsr     LC0A8
        bcs     LC09E
        jsr     LC09F
        and     #$03
        ldx     $A4
        ora     $0645,x
        sta     $0645,x
        jsr     LA293
        lda     #$78
        ldx     $A5
        sta     $06C6,x
        sta     $06C7,x
LC09E:  rts
LC09F:  jsr     LA594
LC0A2:  jsr     L9C6D
        ldx     $A5
        rts
LC0A8:  jsr     L8CD4
        cmp     #$29
        beq     LC0B1
        clc
        rts
LC0B1:  jsr     L8CA1
        sec
        rts
LC0B6:  jsr     L9C6D
        jsr     LA73B
        asl     a
        asl     a
        asl     a
        sta     $A5
        jsr     LC09F
        sta     $06C6,x
        jsr     LC09F
        sta     $06C7,x
        rts
LC0CE:  jsr     L9C6D
        jsr     LA73B
        tax
        lda     #$10
        ora     $0645,x
        sta     $0645,x
        jsr     L8378
        beq     LC0E8
        jsr     LA594
        jmp     LC0CE
LC0E8:  lda     #$FF
        sta     $AC
LC0EC:  rts
LC0ED:  lda     #$FF
        .byte   $AE
LC0F0:  lda     #$00
        sta     $75
        jsr     L9C6D
        jsr     LA73B
        tay
        lda     $0645,y
        and     #$EF
        sta     $0645,y
        lda     $75
        beq     LC11A
        tya
        jsr     LA6C5
        tay
        lda     #$F8
        sta     $0280,y
        sta     $0284,y
        sta     $0288,y
        sta     $028C,y
LC11A:  jsr     L8378
        beq     LC0EC
        jsr     LA594
        lda     $75
        beq     LC0F0
        bne     LC0ED
LC128:
        .byte   $80, "GOTO", $81, "GOSUB", $82, "RUN", $83, "RETURN", $84, "RESTORE", $85, "THEN", $86
        .byte   "LIST", $87, "SYSTEM", $88, "TO", $89, "STEP", $8A, "SPRITE", $8B, "PRINT", $8C
        .byte   "FOR", $8D, "NEXT", $8E, "PAUSE", $8F, "INPUT", $90, "LINPUT", $91, "DATA", $92
        .byte   "IF", $93, "READ", $94, "DIM", $95, "REM", $96, "STOP", $97, "CONT", $98
        .byte   "CLS", $99, "CLEAR", $9A, "ON", $9B, "OFF", $9C, "CUT", $9D, "NEW", $9E
        .byte   "POKE", $9F, "CGSET", $A0, "VIEW", $A1, "MOVE", $A2, "END", $A3, "PLAY", $A4
        .byte   "BEEP", $A5, "LOAD", $A6, "SAVE", $A7, "POSITION", $A8, "KEY", $A9, "COLOR", $AA
        .byte   "DEF", $AB, "CGEN", $AC, "SWAP", $AD, "CALL", $AE, "LOCATE", $AF, "PALET", $B0
        .byte   "ERA", $EF, "XOR", $F0, "OR", $F1, "AND", $F2, "NOT", $F3, "<>", $F4
        .byte   ">=", $F5, "<=", $F6, "=", $F7, ">", $F8, "<", $F9, "+", $FA
        .byte   "-", $FB, "MOD", $FC, "/", $FD, "*", $CA, "ABS", $CB, "ASC", $CC
        .byte   "STR$", $CD, "FRE", $CE, "LEN", $CF, "PEEK", $D0, "RND", $D1, "SGN", $D2
        .byte   "SPC", $D3, "TAB", $D4, "MID$", $D5, "STICK", $D6, "STRIG", $D7, "XPOS", $D8
        .byte   "YPOS", $D9, "VAL", $DA, "POS", $DB, "CSRLIN", $DC, "CHR$", $DD, "HEX$", $DE
        .byte   "INKEY$", $DF, "RIGHT$", $E0, "LEFT$", $E1, "SCR$", $FF
LC2C6:
        .addr   L9146,L90C6,L857C,L9126,L98E0,L836D,L85AF,L80D7
        .addr   L836D,L836D,LA7F3,L8740,L927E,L930D,L9198,L93C2
        .addr   L94F6,L8343,L9204,L994D,L964E,L8343,L86DA,L8717
        .addr   LAB75,L9226,L954D,L836D,LC0F0,L8382,L91DF,LA746
        .addr   LA932,LC0CE,L820B,L9A1D,LB30F,L97FC,L978C,LC0B6
        .addr   L95AA,LBA4E,LA887,LA6CA,L9496,L918C,L91CA,LA6EA
        .addr   LC0ED
LC328:
        .addr   LA369,LA40A,LA485,LA31A,LA3FF,LA358,LA39F,LA379
        .addr   L836D,L836D,LA5FB,LA6AA,LA6B9,LA346,LA34F,LA4BE
        .addr   LA32E,LA2F3,LA461,LA4B5,LA520,LA5BD,LA574,LA41D
LC358:
        .byte   "G BASIC BS.2A", $0D, $0A, "@ BBK COMPUTER CORP.LTD.", $0D, $0A, $00
        .byte   $00
        .byte   $00
        .byte   " "
LC385:
        .byte   "  G-BASIC  ", $00
LC391:
        .byte   $08
LC392:
        .byte   $03
LC393:
        .addr   LC385
        .byte   $07,$07
        .addr   L816E
        .byte   $07,$09
        .addr   L8177
        .byte   $07,$0B
        .addr   L8185
        .byte   $06,$12
        .addr   L818C
LC3A5:
        .addr   LC3AF,LC3BF
LC3A9:
        .addr   LC3CF,LC3DF,LC3EF
LC3AF:
        .byte   $0F,$2C,$15,$07,$0F,$27,$21,$12,$0F,$29,$36,$17,$0F,$30,$26,$07
LC3BF:
        .byte   $0F,$30,$21,$02,$0F,$30,$27,$18,$0F,$30,$27,$16,$0F,$29,$36,$17
LC3CF:
        .byte   $0F,$36,$16,$02,$0F,$27,$30,$19,$0F,$35,$25,$17,$0F,$30,$27,$16
LC3DF:
        .byte   $0F,$30,$16,$01,$0F,$10,$00,$01,$0F,$30,$29,$09,$0F,$30,$16,$07
LC3EF:
        .byte   $0F,$30,$26,$12,$0F,$30,$15,$12,$0F,$30,$12,$16,$0F,$30,$26,$19
        .byte   $FF
LC410:
        .byte   $00
LC411:
        .byte   $00
LC412:
        .byte   $07,$1F,$3F,$1F,$0F,$30,$03,$0F,$00,$02,$04,$1E,$00,$03,$00,$00
        .byte   $E0,$F0,$F0,$F0,$C0,$40,$C0,$E0,$E0,$50,$D0,$30,$00,$E0,$30,$00
        .byte   $04,$0F,$0F,$0C,$00,$00,$0F,$1F,$27,$3F,$3F,$3C,$00,$00,$48,$58
        .byte   $E0,$E0,$F0,$E0,$00,$00,$F0,$E0,$E0,$E0,$F0,$F8,$30,$60,$00,$00
        .byte   $07,$1F,$3F,$1F,$0F,$00,$03,$0F,$00,$02,$04,$1E,$00,$00,$00,$00
        .byte   $E0,$F0,$F0,$F0,$C0,$80,$C0,$E0,$E0,$50,$D0,$30,$00,$E0,$01,$11
        .byte   $1F,$0F,$07,$01,$00,$00,$07,$0F,$0F,$0E,$06,$07,$0E,$01,$00,$80
        .byte   $80,$A0,$E0,$C0,$00,$00,$E0,$E0,$60,$60,$60,$C0,$C0,$C0,$00,$00
        .byte   $00,$0F,$3F,$7F,$3F,$1F,$00,$07,$1F,$01,$04,$09,$3C,$00,$00,$00
        .byte   $00,$C0,$E0,$E0,$E0,$80,$00,$80,$C0,$C0,$A0,$A0,$60,$00,$01,$02
        .byte   $66,$6F,$0F,$07,$00,$00,$07,$0F,$1F,$1D,$0F,$07,$03,$07,$00,$00
        .byte   $30,$F0,$E0,$60,$00,$00,$C0,$E0,$C0,$C0,$F0,$78,$08,$00,$38,$38
        .byte   $31,$07,$0F,$07,$03,$02,$00,$03,$00,$30,$31,$3F,$1C,$0F,$00,$00
        .byte   $F8,$FC,$FC,$FC,$F0,$FF,$FF,$FF,$38,$94,$34,$8C,$00,$F8,$04,$04
        .byte   $0F,$0F,$0F,$03,$00,$00,$07,$27,$3F,$3E,$3F,$03,$00,$00,$80,$86
        .byte   $A6,$E0,$F0,$F0,$E0,$00,$FC,$F8,$E0,$F8,$FC,$FC,$E4,$00,$00,$00
        .byte   $00,$01,$07,$0F,$07,$03,$00,$00,$03,$00,$00,$01,$07,$00,$00,$00
        .byte   $00,$F8,$FC,$FC,$FC,$F0,$00,$F0,$F8,$38,$94,$34,$8C,$00,$0D,$0E
        .byte   $0A,$07,$03,$09,$04,$00,$01,$03,$07,$0F,$17,$1F,$1F,$0D,$20,$40
        .byte   $C0,$F2,$F6,$F0,$C2,$08,$F8,$FC,$FE,$74,$F0,$F0,$C0,$00,$00,$00
        .byte   $00,$0C,$0F,$0F,$17,$24,$03,$07,$07,$07,$07,$0F,$00,$1F,$00,$08
        .byte   $0C,$30,$F0,$F0,$E0,$20,$C0,$E0,$A0,$EC,$EC,$FC,$18,$F8,$04,$06
        .byte   $07,$0F,$0F,$0F,$07,$00,$3F,$3F,$07,$0F,$0F,$0F,$07,$0E,$20,$60
        .byte   $F0,$F0,$E0,$00,$00,$00,$F0,$E0,$F0,$F0,$F8,$30,$00,$00,$00,$00
        .byte   $07,$0F,$0F,$0F,$46,$C4,$03,$07,$05,$01,$04,$0B,$38,$38,$00,$00
        .byte   $E0,$F4,$F6,$F7,$62,$20,$C0,$E0,$20,$40,$20,$D8,$1C,$18,$A3,$02
        .byte   $07,$2F,$27,$17,$08,$00,$0C,$07,$35,$1F,$1F,$0F,$00,$00,$C0,$40
        .byte   $E0,$E0,$F8,$F0,$64,$08,$30,$E0,$A0,$E0,$F8,$FC,$78,$10,$03,$0F
        .byte   $07,$0F,$07,$03,$01,$18,$03,$0F,$00,$02,$00,$00,$00,$03,$80,$C0
        .byte   $C0,$C8,$F0,$F8,$70,$20,$80,$C0,$C0,$C8,$70,$F8,$F0,$E0,$18,$03
        .byte   $00,$20,$10,$18,$08,$00,$07,$03,$07,$3F,$1F,$1F,$08,$00,$00,$C0
        .byte   $40,$00,$00,$00,$60,$C0,$80,$80,$80,$80,$C0,$E0,$60,$C0,$03,$0F
        .byte   $07,$0F,$07,$03,$01,$00,$03,$0F,$00,$02,$00,$00,$00,$03,$80,$C0
        .byte   $C0,$C8,$F0,$F8,$70,$20,$80,$C0,$C0,$C8,$70,$F8,$F0,$E0,$00,$07
        .byte   $01,$01,$00,$00,$0C,$01,$07,$03,$02,$02,$07,$0F,$0F,$01,$80,$00
        .byte   $00,$80,$00,$00,$00,$C0,$C0,$C0,$80,$40,$C0,$E0,$E0,$C0,$00,$07
        .byte   $1F,$0F,$1F,$0F,$07,$02,$00,$07,$1F,$01,$05,$00,$01,$01,$00,$00
        .byte   $80,$80,$90,$E0,$F0,$E0,$00,$00,$80,$80,$90,$E0,$F0,$E0,$00,$00
        .byte   $33,$10,$00,$00,$00,$0E,$07,$0F,$0B,$03,$03,$07,$07,$0E,$40,$00
        .byte   $80,$C0,$10,$30,$10,$00,$C0,$C0,$50,$20,$F0,$F0,$10,$00,$19,$1F
        .byte   $1B,$07,$03,$01,$00,$00,$01,$07,$00,$19,$0C,$0C,$07,$03,$C0,$E0
        .byte   $E0,$E4,$F8,$FC,$98,$00,$C0,$E0,$60,$64,$38,$7C,$78,$F0,$00,$21
        .byte   $30,$10,$18,$00,$00,$00,$01,$21,$3F,$1F,$1F,$03,$00,$00,$08,$D8
        .byte   $00,$00,$00,$30,$F8,$48,$D0,$C0,$C0,$D0,$E0,$F0,$18,$08,$00,$01
        .byte   $07,$03,$07,$03,$01,$04,$00,$01,$07,$00,$01,$00,$00,$00,$00,$C0
        .byte   $E0,$E0,$E4,$F8,$FC,$98,$00,$C0,$E0,$60,$64,$38,$7C,$78,$06,$00
        .byte   $01,$00,$14,$16,$0A,$0A,$01,$03,$01,$03,$17,$1F,$0F,$0A,$00,$08
        .byte   $D8,$00,$00,$02,$00,$10,$F0,$D0,$C0,$C0,$E0,$F0,$C0,$00,$03,$07
        .byte   $07,$07,$07,$03,$1E,$08,$03,$07,$07,$07,$07,$03,$07,$17,$80,$C0
        .byte   $D0,$F0,$C0,$80,$80,$00,$80,$C0,$C0,$C0,$D0,$B0,$E0,$C0,$00,$03
        .byte   $00,$00,$00,$00,$06,$0C,$1F,$0B,$03,$07,$0F,$0F,$06,$0C,$00,$80
        .byte   $00,$00,$30,$00,$00,$00,$80,$C0,$E0,$E0,$F0,$80,$00,$00,$03,$07
        .byte   $07,$2F,$1F,$06,$42,$E1,$03,$07,$05,$29,$1C,$3C,$3C,$0E,$D0,$E4
        .byte   $E6,$E7,$A0,$40,$40,$80,$D0,$E0,$20,$48,$3C,$38,$30,$60,$20,$03
        .byte   $00,$00,$70,$18,$00,$00,$03,$03,$0F,$1F,$7F,$1B,$00,$00,$00,$C0
        .byte   $00,$00,$00,$00,$3C,$30,$C0,$C0,$E0,$F0,$F0,$F8,$3C,$30,$00,$00
        .byte   $20,$70,$7B,$7F,$3F,$0E,$00,$00,$18,$05,$03,$05,$07,$07,$00,$00
        .byte   $02,$07,$6F,$7F,$7E,$38,$00,$00,$0C,$D0,$E0,$D0,$F0,$F0,$01,$07
        .byte   $17,$20,$0F,$0B,$01,$00,$06,$0A,$1A,$27,$03,$33,$79,$00,$C0,$F0
        .byte   $F4,$02,$F0,$E8,$D0,$00,$B0,$A8,$AC,$F2,$E0,$E0,$CC,$1E,$00,$00
        .byte   $00,$00,$03,$07,$1F,$3E,$00,$00,$18,$05,$03,$05,$07,$07,$00,$00
        .byte   $00,$00,$60,$70,$7C,$3E,$00,$00,$0C,$D0,$E0,$D0,$F0,$F0,$79,$77
        .byte   $37,$20,$07,$0B,$05,$00,$06,$0A,$1A,$27,$03,$03,$19,$3C,$CF,$F7
        .byte   $F6,$02,$F8,$E8,$C0,$00,$B0,$A8,$AC,$F2,$E0,$E6,$CF,$00,$00,$00
        .byte   $07,$0F,$1F,$1F,$37,$B7,$00,$00,$07,$0B,$11,$11,$39,$79,$00,$1F
        .byte   $FE,$FC,$FC,$F8,$F8,$F8,$00,$00,$CE,$C0,$9C,$80,$18,$38,$3F,$3F
        .byte   $1F,$1F,$0F,$07,$00,$00,$FB,$3F,$1F,$1F,$0F,$07,$00,$00,$E0,$D8
        .byte   $DC,$ED,$E2,$C0,$00,$00,$F8,$E0,$80,$F3,$00,$C3,$01,$00,$00,$07
        .byte   $0F,$1F,$1F,$37,$B7,$3F,$00,$07,$0B,$11,$11,$39,$79,$FF,$00,$C0
        .byte   $E0,$F0,$F0,$F8,$F8,$F0,$00,$C0,$E0,$F0,$F0,$F8,$38,$3B,$3F,$1F
        .byte   $1F,$0F,$07,$00,$00,$00,$3F,$1F,$1F,$0F,$07,$00,$00,$00,$ED,$EE
        .byte   $A0,$F8,$80,$3C,$00,$00,$13,$13,$03,$81,$98,$00,$00,$00,$00,$00
        .byte   $27,$0B,$1D,$0F,$37,$3F,$00,$00,$10,$3C,$1E,$1E,$0F,$03,$1C,$3E
        .byte   $FF,$F0,$F0,$F0,$F8,$F8,$00,$00,$C0,$60,$70,$70,$F8,$F8,$3F,$3F
        .byte   $3F,$FF,$FF,$E7,$60,$20,$23,$3F,$1F,$0F,$0F,$07,$00,$00,$F8,$FC
        .byte   $A1,$F2,$A0,$E0,$08,$10,$F8,$FB,$A3,$F6,$A0,$D8,$38,$30,$00,$27
        .byte   $0B,$1D,$0F,$37,$3F,$3F,$00,$10,$3C,$1E,$1E,$0F,$03,$23,$00,$C0
        .byte   $F8,$FC,$AA,$FA,$F8,$F8,$00,$C0,$60,$70,$70,$F8,$F8,$F8,$3F,$1F
        .byte   $1F,$1F,$1F,$1C,$0C,$06,$3F,$1F,$1F,$0F,$07,$00,$00,$00,$FC,$FD
        .byte   $AA,$E2,$E0,$60,$10,$20,$FB,$F3,$F6,$E6,$C0,$10,$70,$60,$00,$00
        .byte   $83,$E5,$6D,$3F,$3F,$1F,$01,$01,$01,$03,$0B,$09,$0B,$0F,$80,$00
        .byte   $83,$A7,$B6,$FC,$A8,$F8,$00,$80,$80,$C0,$D0,$90,$D0,$F0,$1F,$1F
        .byte   $0F,$0F,$0F,$07,$00,$06,$1F,$1F,$0F,$0F,$07,$03,$0E,$0E,$F8,$F8
        .byte   $A0,$F0,$F0,$E0,$00,$60,$F8,$F8,$A0,$F0,$E0,$C0,$70,$70,$00,$00
        .byte   $03,$05,$0D,$0F,$1F,$3F,$01,$01,$01,$03,$0B,$09,$1B,$1F,$80,$00
        .byte   $C0,$A0,$B0,$F0,$F8,$FC,$00,$80,$80,$C0,$D0,$90,$D8,$F8,$7F,$FF
        .byte   $EF,$CF,$82,$87,$30,$1C,$1F,$1F,$4F,$4F,$47,$1B,$3C,$1C,$FE,$FF
        .byte   $A3,$F3,$A3,$E1,$0C,$38,$F8,$F8,$A2,$F2,$A2,$D8,$3C,$38,$00,$07
        .byte   $0F,$1F,$3D,$3D,$7F,$FF,$00,$01,$09,$19,$2B,$0B,$59,$7F,$00,$E0
        .byte   $F0,$F8,$BC,$BC,$FE,$FF,$00,$80,$90,$98,$DC,$DC,$9E,$FE,$FD,$6D
        .byte   $6D,$2D,$3D,$1D,$0F,$07,$78,$E0,$80,$E0,$30,$18,$0F,$07,$BF,$B6
        .byte   $B6,$B4,$BC,$B8,$F0,$E0,$1E,$07,$03,$07,$0C,$18,$F0,$E0,$00,$07
        .byte   $0D,$1D,$3F,$3F,$7F,$FF,$00,$01,$0B,$1B,$29,$09,$59,$7F,$00,$E0
        .byte   $B0,$B8,$FC,$FC,$FE,$FF,$00,$80,$D0,$D8,$9C,$9C,$9E,$FE,$FD,$6D
        .byte   $6F,$3F,$3F,$1F,$0F,$07,$78,$E0,$83,$DF,$3F,$1F,$0F,$07,$BF,$B6
        .byte   $F6,$FC,$FC,$F8,$F0,$E0,$1E,$07,$83,$FB,$FC,$F8,$F0,$E0,$0F,$1F
        .byte   $1F,$3F,$2F,$2F,$03,$01,$0F,$13,$01,$00,$00,$01,$7F,$FF,$00,$80
        .byte   $C0,$C0,$E0,$E0,$A0,$F0,$00,$80,$C0,$C0,$E0,$E0,$A0,$F0,$23,$3F
        .byte   $3F,$3F,$38,$1C,$07,$00,$1D,$00,$00,$00,$07,$03,$18,$7E,$FE,$FC
        .byte   $F8,$F0,$A0,$78,$20,$00,$FE,$FC,$78,$70,$70,$F8,$E0,$00,$01,$03
        .byte   $07,$0D,$0B,$03,$00,$00,$01,$00,$00,$00,$00,$3C,$1F,$0F,$C0,$E0
        .byte   $F0,$F0,$A0,$F0,$A0,$F8,$80,$E0,$70,$30,$30,$70,$F0,$F8,$1D,$1F
        .byte   $3F,$3F,$1F,$0F,$07,$00,$03,$00,$00,$00,$00,$70,$38,$01,$F8,$FC
        .byte   $82,$FE,$A0,$F0,$F8,$10,$F8,$FC,$7E,$7E,$70,$F0,$F8,$F0,$07,$0F
        .byte   $0F,$1F,$1D,$1D,$1C,$30,$07,$0F,$09,$18,$10,$10,$13,$3F,$C0,$E0
        .byte   $A0,$F0,$70,$70,$7C,$1E,$C0,$E0,$20,$10,$10,$10,$BC,$FE,$38,$7F
        .byte   $2B,$EF,$0F,$0F,$01,$00,$3F,$78,$78,$E8,$00,$00,$1E,$3F,$3F,$F8
        .byte   $A8,$F8,$F8,$E0,$80,$00,$FF,$18,$08,$00,$06,$1C,$78,$00,$07,$0F
        .byte   $0F,$1F,$1F,$1F,$1F,$3F,$07,$0F,$0F,$1F,$1F,$1F,$1F,$3F,$80,$E0
        .byte   $A0,$F0,$80,$F0,$FC,$FF,$80,$E0,$A0,$F0,$F0,$F0,$FC,$FF,$3F,$7F
        .byte   $7F,$FF,$0F,$0F,$07,$00,$3F,$7F,$2B,$EF,$0F,$0F,$1F,$3E,$FF,$F8
        .byte   $F8,$F8,$80,$F0,$C0,$80,$FF,$F8,$F8,$F8,$FE,$FC,$F8,$80,$00,$00
        .byte   $1E,$33,$60,$45,$45,$4F,$00,$00,$1E,$3F,$7F,$70,$70,$70,$00,$04
        .byte   $00,$90,$C0,$49,$48,$4C,$00,$04,$00,$90,$C0,$C9,$C8,$CC,$2E,$24
        .byte   $60,$60,$70,$3C,$1F,$07,$31,$3B,$7F,$7F,$7F,$3F,$1F,$07,$DC,$B4
        .byte   $6C,$08,$18,$38,$30,$E0,$DC,$BC,$FC,$F8,$80,$F8,$F0,$E0,$00,$00
        .byte   $1F,$31,$60,$55,$55,$2F,$00,$00,$1F,$3F,$7F,$60,$60,$30,$00,$00
        .byte   $00,$80,$80,$C0,$40,$42,$00,$00,$00,$80,$80,$C0,$C0,$C2,$6F,$4F
        .byte   $C6,$C0,$C0,$E0,$70,$1F,$70,$70,$A9,$FF,$FF,$FF,$7F,$1F,$46,$5E
        .byte   $7C,$64,$08,$18,$70,$C0,$C6,$DE,$FC,$FC,$F8,$F8,$F0,$C0,$00,$00
        .byte   $38,$38,$38,$10,$03,$27,$00,$00,$18,$18,$18,$03,$3C,$79,$00,$F8
        .byte   $F8,$FE,$2E,$0E,$5E,$7E,$00,$78,$78,$70,$00,$F0,$A0,$F0,$27,$27
        .byte   $03,$10,$38,$38,$38,$00,$79,$79,$3C,$03,$18,$18,$18,$00,$7E,$7E
        .byte   $5E,$0E,$2E,$FE,$A8,$F8,$80,$F0,$A0,$F0,$00,$70,$78,$78,$00,$00
        .byte   $38,$38,$38,$10,$03,$27,$00,$00,$30,$30,$30,$03,$3C,$79,$00,$F8
        .byte   $F8,$FE,$2E,$0E,$5E,$7E,$00,$98,$98,$90,$00,$F0,$A0,$F0,$27,$27
        .byte   $03,$10,$38,$38,$38,$00,$79,$79,$3C,$03,$30,$30,$30,$00,$7E,$7E
        .byte   $5E,$0E,$2E,$FE,$80,$F8,$A0,$F0,$A0,$F0,$00,$90,$98,$98,$01,$03
        .byte   $01,$08,$10,$07,$47,$E7,$00,$01,$1D,$3E,$3F,$38,$19,$6B,$00,$80
        .byte   $C0,$8C,$1E,$1F,$8F,$06,$00,$80,$C0,$80,$06,$CF,$6F,$F2,$72,$20
        .byte   $00,$0C,$1E,$1F,$0F,$06,$75,$27,$03,$01,$06,$0E,$0F,$06,$6E,$FF
        .byte   $FE,$7C,$F8,$F0,$E0,$40,$D0,$E0,$40,$80,$00,$00,$00,$00,$01,$03
        .byte   $01,$08,$10,$07,$47,$E7,$01,$03,$1D,$3E,$3F,$38,$59,$FF,$00,$80
        .byte   $C0,$8C,$1E,$1F,$8F,$06,$00,$80,$80,$0C,$18,$D3,$67,$F2,$72,$20
        .byte   $00,$0C,$1E,$1F,$0F,$06,$65,$07,$03,$0D,$18,$12,$07,$06,$6E,$FF
        .byte   $FE,$7C,$F8,$F0,$E0,$40,$D0,$E0,$40,$80,$00,$00,$00,$00,$00,$18
        .byte   $1D,$1C,$18,$01,$03,$03,$01,$03,$1B,$1B,$1B,$02,$04,$05,$00,$18
        .byte   $B8,$38,$18,$80,$C0,$C0,$80,$C0,$D8,$D8,$D8,$40,$20,$A0,$73,$70
        .byte   $7B,$79,$73,$7F,$1F,$1F,$05,$77,$75,$77,$75,$60,$00,$00,$CE,$0E
        .byte   $DE,$9E,$CE,$FE,$F8,$F8,$A0,$EE,$AE,$EE,$AE,$06,$00,$00,$00,$18
        .byte   $1D,$1C,$18,$01,$03,$03,$01,$1B,$1B,$1B,$03,$02,$04,$05,$00,$18
        .byte   $B8,$38,$18,$80,$C0,$C0,$80,$D8,$D8,$D8,$C0,$40,$20,$A0,$73,$70
        .byte   $7B,$79,$73,$7F,$1F,$1F,$75,$07,$05,$77,$75,$60,$00,$00,$CE,$0E
        .byte   $DE,$9E,$CE,$FE,$F8,$F8,$AE,$E0,$A0,$EE,$AE,$06,$00,$00,$07,$00
        .byte   $00,$01,$01,$C0,$A1,$FB,$07,$07,$03,$01,$01,$87,$86,$9D,$E0,$00
        .byte   $00,$80,$80,$03,$87,$FF,$E0,$E0,$C0,$80,$80,$E1,$61,$B9,$DB,$81
        .byte   $80,$01,$01,$03,$07,$07,$BD,$E6,$83,$01,$01,$00,$00,$07,$83,$81
        .byte   $01,$80,$80,$C0,$A0,$E0,$BD,$67,$A3,$80,$80,$00,$00,$E0,$00,$10
        .byte   $38,$78,$B8,$1C,$09,$03,$00,$10,$20,$40,$C9,$7F,$0E,$0D,$00,$10
        .byte   $38,$3C,$3A,$70,$20,$80,$00,$10,$08,$04,$26,$FC,$A0,$60,$07,$03
        .byte   $09,$7C,$F8,$60,$20,$10,$1B,$0D,$0E,$1F,$89,$58,$38,$10,$C0,$80
        .byte   $20,$7C,$3E,$0C,$08,$10,$B0,$60,$E0,$F0,$22,$34,$38,$10,$03,$07
        .byte   $07,$0F,$1F,$3F,$7F,$FF,$01,$01,$01,$0F,$00,$0E,$14,$1C,$E0,$F8
        .byte   $FC,$FC,$FE,$80,$FE,$D4,$00,$00,$00,$FC,$00,$7E,$7E,$7E,$7F,$3D
        .byte   $1E,$00,$0F,$07,$01,$01,$1C,$5E,$2F,$1F,$0F,$01,$07,$03,$D4,$FE
        .byte   $7E,$00,$FC,$FC,$00,$00,$7E,$7E,$80,$FE,$80,$00,$F8,$E0,$00,$0F
        .byte   $1F,$3F,$1F,$1F,$0F,$27,$00,$01,$00,$03,$25,$2E,$3C,$3C,$F0,$F8
        .byte   $FC,$FC,$FE,$FE,$DE,$AE,$00,$88,$50,$20,$90,$08,$24,$72,$31,$30
        .byte   $19,$1C,$26,$03,$01,$00,$5E,$5F,$6E,$77,$3B,$1D,$0E,$03,$76,$EA
        .byte   $D4,$EC,$78,$30,$90,$C0,$F8,$FC,$FE,$7E,$BC,$D8,$E0,$40,$01,$03
        .byte   $03,$07,$37,$37,$35,$73,$02,$04,$04,$0B,$1F,$5F,$5E,$7C,$C0,$E0
        .byte   $A0,$F0,$FC,$FE,$AA,$FE,$00,$00,$00,$C0,$A8,$E8,$28,$0E,$33,$37
        .byte   $37,$36,$37,$36,$17,$06,$5C,$5B,$5B,$5B,$1B,$1B,$1B,$03,$FE,$DE
        .byte   $8A,$5E,$DC,$5C,$D8,$40,$08,$E8,$A8,$E8,$A8,$E8,$E8,$E0,$03,$07
        .byte   $01,$01,$03,$07,$3F,$FF,$00,$00,$01,$00,$03,$08,$06,$1C,$60,$70
        .byte   $C0,$E0,$80,$FA,$FA,$82,$80,$80,$C0,$00,$F0,$1E,$0E,$7E,$7F,$3F
        .byte   $07,$03,$01,$01,$07,$03,$1C,$06,$08,$03,$00,$01,$00,$00,$82,$FA
        .byte   $AA,$F0,$E0,$C0,$70,$60,$7E,$0E,$1E,$F0,$00,$C0,$80,$80,$00,$00
        .byte   $38,$3C,$3F,$1F,$1F,$0F,$00,$00,$00,$02,$0C,$0F,$0C,$14,$00,$70
        .byte   $38,$30,$F6,$FE,$F2,$70,$00,$00,$00,$2C,$98,$48,$20,$90,$0E,$0E
        .byte   $4F,$7F,$7F,$27,$0C,$0E,$05,$09,$04,$12,$09,$1C,$10,$00,$38,$10
        .byte   $00,$84,$C8,$90,$20,$00,$D8,$F8,$FC,$7C,$F8,$F0,$20,$00,$01,$01
        .byte   $03,$03,$03,$47,$8B,$FF,$00,$00,$00,$01,$05,$03,$0B,$2A,$80,$80
        .byte   $C0,$C0,$C0,$E2,$A3,$FF,$00,$00,$00,$80,$A0,$C0,$D0,$54,$FE,$3E
        .byte   $FE,$DE,$4E,$06,$00,$07,$28,$E9,$29,$09,$0D,$07,$07,$07,$FF,$7C
        .byte   $7F,$7B,$72,$60,$00,$E0,$14,$97,$94,$90,$B0,$E0,$E0,$E0,$00,$00
        .byte   $00,$00,$01,$01,$03,$3F,$00,$00,$00,$08,$10,$06,$0C,$00,$00,$00
        .byte   $00,$00,$00,$00,$80,$F8,$00,$00,$00,$20,$10,$C0,$60,$00,$03,$01
        .byte   $01,$00,$00,$00,$00,$00,$0C,$06,$10,$08,$00,$00,$00,$00,$80,$00
        .byte   $00,$00,$00,$00,$00,$00,$60,$C0,$10,$20,$00,$00,$00,$00,$18,$B2
        .byte   $22,$02,$43,$33,$5F,$8F,$18,$B0,$21,$15,$0C,$0C,$40,$B0,$60,$F4
        .byte   $18,$18,$32,$63,$81,$F2,$60,$F0,$10,$A0,$CA,$93,$19,$0A,$9F,$9F
        .byte   $5B,$7F,$35,$61,$89,$26,$A0,$E0,$64,$40,$0A,$16,$09,$26,$B9,$E2
        .byte   $F0,$DA,$A0,$84,$96,$8C,$41,$1A,$0C,$24,$50,$24,$16,$0C,$08,$1C
        .byte   $1C,$0C,$0C,$18,$00,$00,$00,$00,$20,$62,$62,$A6,$FE,$FC,$00,$00
        .byte   $00,$00,$00,$78,$B4,$CE,$00,$00,$00,$00,$00,$70,$B0,$CC,$01,$03
        .byte   $02,$03,$03,$01,$00,$00,$CD,$0D,$1C,$3D,$0C,$02,$0F,$1E,$B6,$7A
        .byte   $B4,$CE,$A3,$FE,$FC,$00,$B4,$7A,$B5,$CE,$B4,$78,$02,$07,$08,$1C
        .byte   $1C,$0C,$0C,$18,$00,$01,$00,$00,$20,$62,$62,$A6,$FE,$FF,$00,$00
        .byte   $00,$00,$78,$B4,$CE,$B6,$00,$00,$00,$00,$70,$B0,$CC,$B4,$03,$02
        .byte   $03,$03,$01,$00,$00,$00,$DD,$3C,$1D,$0C,$00,$03,$03,$01,$7A,$B4
        .byte   $CE,$B7,$FE,$FC,$00,$00,$7A,$B5,$CE,$B4,$78,$02,$8C,$DC,$30,$66
        .byte   $D6,$D4,$F4,$E6,$82,$47,$30,$60,$D0,$D2,$F2,$E0,$C2,$47,$00,$C0
        .byte   $CC,$4E,$57,$DB,$9F,$DF,$00,$00,$00,$8C,$96,$1A,$9F,$CF,$65,$37
        .byte   $07,$37,$43,$50,$54,$54,$67,$3F,$0F,$7F,$4F,$57,$54,$54,$43,$FE
        .byte   $A0,$F8,$82,$1A,$28,$20,$83,$DE,$A0,$FC,$E2,$DA,$28,$20,$06,$06
        .byte   $34,$64,$D6,$D2,$00,$FF,$00,$00,$32,$62,$D0,$D2,$00,$FF,$DC,$CE
        .byte   $56,$5B,$8B,$8F,$83,$46,$00,$0C,$94,$9A,$1E,$8F,$80,$88,$D7,$67
        .byte   $17,$43,$D0,$A8,$08,$00,$DF,$6F,$3F,$4F,$D7,$A8,$08,$00,$FC,$E0
        .byte   $F8,$C8,$14,$54,$54,$94,$DC,$E0,$F8,$EC,$D4,$54,$54,$94,$00,$00
        .byte   $01,$03,$0E,$06,$02,$03,$00,$00,$00,$00,$0C,$1C,$34,$7E,$00,$00
        .byte   $80,$C0,$F0,$FD,$AB,$FF,$00,$00,$00,$00,$30,$3D,$3F,$FF,$03,$03
        .byte   $03,$07,$07,$00,$00,$00,$FF,$0F,$7E,$1F,$07,$00,$00,$00,$FF,$FF
        .byte   $FE,$FC,$FE,$3F,$0E,$00,$CF,$FF,$E6,$7F,$B0,$3F,$0E,$00,$00,$00
        .byte   $00,$01,$03,$0E,$06,$02,$00,$00,$00,$00,$00,$FC,$7C,$34,$1E,$3F
        .byte   $7F,$FE,$FC,$FD,$89,$FF,$1E,$3F,$78,$7E,$30,$3D,$3D,$3F,$03,$03
        .byte   $03,$07,$0F,$03,$00,$00,$1E,$0F,$1F,$3F,$7F,$03,$00,$00,$FF,$FF
        .byte   $FE,$F0,$80,$F0,$00,$00,$7F,$FF,$FE,$FF,$FA,$F0,$00,$00,$00,$00
        .byte   $00,$00,$18,$00,$00,$00,$00,$00,$00,$18,$24,$18,$00,$00,$00,$00
        .byte   $3C,$42,$BD,$42,$3C,$00,$00,$00,$3C,$7E,$83,$7E,$3C,$00,$00,$00
        .byte   $00,$10,$08,$00,$00,$00,$00,$00,$30,$28,$14,$0C,$00,$00,$00,$38
        .byte   $64,$5A,$5A,$26,$1C,$00,$00,$78,$5C,$66,$66,$3A,$1E,$00,$00,$00
        .byte   $00,$10,$10,$00,$00,$00,$00,$00,$10,$28,$28,$10,$00,$00,$10,$28
        .byte   $54,$54,$54,$54,$28,$10,$10,$38,$6C,$6C,$6C,$6C,$38,$10,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$80,$80
        .byte   $80,$80,$80,$80,$80,$7F,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$01
        .byte   $01,$01,$01,$01,$01,$01,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$FE
        .byte   $82,$82,$82,$FE,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$FE
        .byte   $FE,$FE,$FE,$FE,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$3C,$24
        .byte   $24,$24,$24,$3C,$00,$00,$3C,$3C,$3C,$3C,$3C,$3C,$00,$00,$00,$00
        .byte   $00,$10,$00,$00,$00,$00,$00,$00,$18,$2C,$3C,$18,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$C0,$60,$30,$10,$18,$0C,$04,$06,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$05,$04
        .byte   $0E,$0F,$0F,$07,$07,$07,$02,$03,$01,$00,$00,$00,$00,$00,$80,$C0
        .byte   $E0,$70,$78,$B8,$BC,$DC,$00,$00,$00,$80,$80,$60,$58,$A4,$03,$03
        .byte   $03,$01,$01,$00,$00,$00,$00,$00,$01,$01,$01,$00,$00,$00,$DC,$EE
        .byte   $82,$F6,$F7,$F7,$7B,$7B,$A0,$98,$14,$2A,$28,$2C,$4E,$4E,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$3B,$1B
        .byte   $1F,$0F,$0F,$07,$06,$02,$0F,$1F,$1F,$0F,$0F,$07,$06,$02,$00,$00
        .byte   $00,$00,$00,$00,$01,$03,$00,$00,$00,$00,$00,$00,$01,$01,$03,$0F
        .byte   $1E,$3E,$76,$EE,$DC,$DC,$03,$0F,$1E,$3E,$3E,$3E,$3C,$20,$07,$07
        .byte   $0F,$0F,$1E,$1E,$1C,$3D,$01,$01,$08,$08,$05,$05,$03,$02,$BC,$B8
        .byte   $78,$70,$F0,$F0,$A0,$E0,$40,$40,$F8,$80,$00,$00,$E0,$00,$39,$3B
        .byte   $33,$37,$27,$0E,$00,$00,$06,$04,$0C,$08,$18,$10,$10,$30,$E0,$C0
        .byte   $C0,$80,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$20,$20,$60,$40,$40,$C0,$80,$80,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$70,$70
        .byte   $30,$30,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$78,$18
        .byte   $78,$60,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$78,$18
        .byte   $78,$18,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$B0,$B0
        .byte   $B0,$F8,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$3D
        .byte   $31,$31,$31,$3D,$3D,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$F4
        .byte   $B6,$B7,$B6,$F6,$F6,$00,$FF,$FF,$FF,$FF,$FF,$FF,$80,$FF,$00,$5E
        .byte   $DB,$DF,$DE,$D8,$D8,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$6D
        .byte   $6C,$6C,$6C,$7C,$7C,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$EF
        .byte   $CC,$CF,$CC,$CF,$83,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$78
        .byte   $6C,$7C,$78,$6C,$64,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$3E
        .byte   $36,$36,$36,$3E,$3E,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$F3
        .byte   $8B,$FB,$A3,$C3,$80,$00,$AB,$FF,$BB,$FF,$AB,$FF,$AB,$FF,$00,$DE
        .byte   $1B,$DF,$1E,$DB,$89,$00,$AB,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$39
        .byte   $6C,$6C,$7C,$7C,$6C,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$EF
        .byte   $CD,$CD,$81,$CF,$8F,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$00,$BC
        .byte   $B6,$BE,$BC,$B6,$B2,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$11,$FC
        .byte   $00,$F8,$F8,$00,$00,$00,$00,$01,$03,$FB,$83,$03,$FD,$FF,$7F,$BF
        .byte   $80,$FF,$DF,$C0,$80,$00,$00,$00,$00,$1F,$1F,$00,$3F,$FF,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$FC,$FC,$FC,$FC,$FC,$FC,$A8,$FC,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$E0,$80,$20,$A0,$00,$80,$20,$B0,$18,$70,$DC
        .byte   $47,$C4,$44,$C4,$44,$C0,$8B,$27,$BC,$3F,$BF,$3F,$BF,$3B,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$C0,$B8,$30,$A0,$00,$80,$20,$A0,$00,$00,$92
        .byte   $FF,$92,$92,$92,$92,$00,$6D,$FF,$92,$FF,$FF,$FF,$FF,$FF,$11,$11
        .byte   $11,$11,$11,$11,$11,$11,$0C,$0C,$0C,$0C,$0C,$0C,$0C,$0C,$11,$51
        .byte   $A1,$51,$51,$51,$51,$11,$AC,$EC,$4C,$EC,$EC,$EC,$EC,$AC,$00,$00
        .byte   $00,$01,$07,$0C,$08,$18,$00,$00,$00,$00,$00,$03,$07,$07,$10,$10
        .byte   $10,$10,$10,$10,$10,$11,$0F,$0F,$0F,$0F,$0F,$0F,$0C,$0C,$11,$10
        .byte   $10,$10,$10,$10,$10,$10,$0C,$0C,$0F,$0F,$0F,$0F,$0F,$0F,$18,$08
        .byte   $0C,$07,$01,$00,$00,$00,$07,$07,$03,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$FF,$00,$00,$00,$00,$00,$00,$00,$00,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$AB,$FF,$FF,$FF,$FF,$00,$00,$FF,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$83,$FF,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$FF,$00,$00,$00,$FF,$FF,$AB,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$80,$E0,$30,$10,$18,$00,$00,$00,$00,$00,$C0,$E0,$E0,$08,$08
        .byte   $08,$08,$08,$08,$08,$88,$F0,$F0,$A0,$F0,$F0,$F0,$30,$30,$88,$88
        .byte   $88,$88,$88,$88,$88,$88,$30,$30,$30,$30,$30,$30,$30,$30,$88,$8A
        .byte   $8F,$8A,$8A,$8A,$8A,$88,$35,$37,$32,$37,$37,$37,$37,$35,$88,$08
        .byte   $08,$08,$08,$08,$08,$08,$30,$30,$F0,$F0,$A0,$F0,$F0,$F0,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$00,$49
        .byte   $FF,$49,$49,$49,$49,$00,$B6,$FF,$49,$FF,$FF,$FF,$AB,$B6,$18,$10
        .byte   $30,$E0,$80,$00,$00,$00,$E0,$E0,$C0,$00,$00,$00,$00,$00,$02,$03
        .byte   $02,$03,$02,$03,$02,$07,$01,$04,$05,$00,$01,$04,$0D,$18,$0E,$3B
        .byte   $A2,$23,$22,$23,$22,$03,$81,$E4,$3D,$FC,$A9,$FC,$A9,$DC,$02,$03
        .byte   $02,$03,$02,$03,$02,$03,$1D,$0C,$05,$00,$01,$04,$05,$00,$00,$00
        .byte   $18,$3C,$3C,$18,$00,$00,$FF,$C3,$99,$AD,$BD,$99,$C3,$FF,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$00,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7F,$7F,$7F,$7F,$7F,$7F,$7F,$7F,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$00,$FC,$FC,$FC,$FC,$FC,$FC,$FC,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0C,$0C
        .byte   $18,$10,$00,$60,$60,$00,$00,$00,$00,$00,$00,$00,$00,$00,$28,$28
        .byte   $28,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$08,$2E
        .byte   $F8,$28,$3E,$E8,$20,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$7C
        .byte   $90,$FE,$12,$FC,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$42,$A4
        .byte   $A8,$54,$2A,$4A,$84,$00,$00,$00,$00,$00,$00,$00,$00,$00,$18,$28
        .byte   $28,$10,$6A,$44,$3A,$00,$00,$00,$00,$00,$00,$00,$00,$00,$30,$10
        .byte   $20,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0C,$10
        .byte   $20,$20,$20,$10,$0C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$30,$08
        .byte   $04,$04,$04,$08,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$54
        .byte   $38,$10,$38,$54,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$10
        .byte   $10,$7C,$10,$10,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$30,$10,$20,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$30,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$02,$04
        .byte   $08,$10,$20,$40,$80,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $8A,$92,$A2,$C6,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$30
        .byte   $10,$10,$10,$10,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $02,$1C,$70,$C0,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$04
        .byte   $18,$0C,$02,$C6,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$1C,$34
        .byte   $64,$C4,$84,$FE,$04,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FC,$80
        .byte   $FC,$06,$02,$C6,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C0
        .byte   $80,$FC,$86,$C2,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$86
        .byte   $0C,$18,$10,$10,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$86
        .byte   $C2,$7C,$86,$C2,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$86
        .byte   $C2,$7E,$02,$06,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$30
        .byte   $30,$00,$30,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$30
        .byte   $30,$00,$30,$10,$20,$00,$00,$00,$00,$00,$00,$00,$00,$00,$08,$10
        .byte   $20,$40,$20,$10,$08,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $7C,$00,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$20,$10
        .byte   $08,$04,$08,$10,$20,$00,$00,$00,$00,$00,$00,$00,$00,$00,$3C,$66
        .byte   $46,$0C,$08,$00,$18,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$82
        .byte   $9A,$AA,$BC,$80,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$38,$6C
        .byte   $C6,$82,$FE,$82,$82,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FC,$86
        .byte   $82,$FC,$86,$82,$FC,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $80,$80,$80,$C6,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$F8,$8C
        .byte   $86,$82,$86,$8C,$F8,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$80
        .byte   $80,$FC,$80,$80,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$80
        .byte   $80,$FC,$80,$80,$80,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $82,$80,$8E,$C2,$7E,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$FE,$82,$82,$82,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$10
        .byte   $10,$10,$10,$10,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$1E,$04
        .byte   $04,$04,$04,$CC,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$86,$8C
        .byte   $98,$F0,$98,$8C,$86,$00,$00,$00,$00,$00,$00,$00,$00,$00,$40,$40
        .byte   $40,$40,$40,$40,$7E,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$C6
        .byte   $FE,$BA,$92,$82,$82,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$C2
        .byte   $E2,$B2,$9A,$8E,$86,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $82,$82,$82,$C6,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FC,$86
        .byte   $82,$86,$FC,$80,$80,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C6
        .byte   $82,$92,$8E,$C4,$7A,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FC,$86
        .byte   $82,$86,$F8,$8C,$86,$00,$00,$00,$00,$00,$00,$00,$00,$00,$7C,$C0
        .byte   $80,$7C,$06,$02,$FC,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$10
        .byte   $10,$10,$10,$10,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$82,$82,$C6,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$C6,$6C,$38,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$82
        .byte   $82,$92,$BA,$EE,$C6,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$C6
        .byte   $6C,$38,$6C,$C6,$82,$00,$00,$00,$00,$00,$00,$00,$00,$00,$82,$C6
        .byte   $6C,$38,$10,$10,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$0C
        .byte   $18,$30,$60,$C0,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$1C
        .byte   $10,$10,$10,$10,$1C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$6C,$28
        .byte   $7C,$10,$7C,$10,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0E
        .byte   $02,$02,$02,$02,$0E,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$28
        .byte   $44,$82,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$20,$38
        .byte   $18,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$0C,$7C,$CC,$76,$00,$00,$00,$00,$00,$00,$00,$00,$00,$E0,$60
        .byte   $60,$7C,$66,$66,$DC,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$CC,$C0,$CC,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$1C,$0C
        .byte   $0C,$7C,$CC,$CC,$76,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$CC,$FC,$C0,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$38,$6C
        .byte   $60,$F0,$60,$60,$F0,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $76,$CC,$CC,$7C,$0C,$F8,$00,$00,$00,$00,$00,$00,$00,$00,$E0,$60
        .byte   $6C,$76,$66,$66,$E6,$00,$00,$00,$00,$00,$00,$00,$00,$00,$30,$00
        .byte   $70,$30,$30,$30,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0C,$00
        .byte   $0C,$0C,$0C,$CC,$CC,$78,$00,$00,$00,$00,$00,$00,$00,$00,$E0,$60
        .byte   $66,$6C,$78,$6C,$E6,$00,$00,$00,$00,$00,$00,$00,$00,$00,$70,$30
        .byte   $30,$30,$30,$30,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$FE,$FE,$D6,$C6,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $F8,$CC,$CC,$CC,$CC,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $78,$CC,$CC,$CC,$78,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $DC,$66,$66,$7C,$60,$F0,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $76,$CC,$CC,$7C,$0C,$1E,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $DC,$76,$66,$60,$F0,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $7C,$C0,$78,$0C,$F8,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$30
        .byte   $7C,$30,$30,$34,$18,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$CC,$CC,$CC,$76,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$CC,$CC,$78,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $C6,$D6,$FE,$FE,$6C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $C6,$6C,$38,$6C,$C6,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $CC,$CC,$CC,$7C,$0C,$F8,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $FC,$98,$30,$64,$FC,$00,$00,$00,$00,$00,$00,$00,$00,$00,$1C,$30
        .byte   $30,$E0,$30,$30,$1C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$18,$18
        .byte   $18,$00,$18,$18,$18,$00,$00,$00,$00,$00,$00,$00,$00,$00,$E0,$30
        .byte   $30,$1C,$30,$30,$E0,$00,$00,$00,$00,$00,$00,$00,$00,$00,$76,$DC
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10
        .byte   $38,$6C,$C6,$C6,$FE,$00,$00,$00,$00,$00,$00,$00,$00,$00,$11,$FC
        .byte   $00,$F8,$F8,$00,$00,$00,$00,$01,$03,$FB,$83,$03,$FD,$FF,$7F,$BF
        .byte   $80,$FF,$DF,$C0,$80,$00,$00,$00,$00,$1F,$1F,$00,$3F,$FF,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$FC,$FC,$FC,$FC,$FC,$FC,$A8,$FC,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$E0,$80,$20,$A0,$00,$80,$20,$B0,$18,$70,$DC
        .byte   $47,$C4,$44,$C4,$44,$C0,$8B,$27,$BC,$3F,$BF,$3F,$BF,$3B,$40,$C0
        .byte   $40,$C0,$40,$C0,$40,$C0,$B8,$30,$A0,$00,$80,$20,$A0,$00,$00,$92
        .byte   $FF,$92,$92,$92,$92,$00,$6D,$FF,$92,$FF,$FF,$FF,$FF,$FF,$11,$11
        .byte   $11,$11,$11,$11,$11,$11,$0C,$0C,$0C,$0C,$0C,$0C,$0C,$0C,$11,$51
        .byte   $A1,$51,$51,$51,$51,$11,$AC,$EC,$4C,$EC,$EC,$EC,$EC,$AC,$00,$00
        .byte   $00,$01,$07,$0C,$08,$18,$00,$00,$00,$00,$00,$03,$07,$07,$10,$10
        .byte   $10,$10,$10,$10,$10,$11,$0F,$0F,$0F,$0F,$0F,$0F,$0C,$0C,$11,$10
        .byte   $10,$10,$10,$10,$10,$10,$0C,$0C,$0F,$0F,$0F,$0F,$0F,$0F,$18,$08
        .byte   $0C,$07,$01,$00,$00,$00,$07,$07,$03,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$FF,$00,$00,$00,$00,$00,$00,$00,$00,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$AB,$FF,$FF,$FF,$FF,$00,$00,$FF,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$83,$FF,$FF,$FF,$FF,$FF,$00,$00
        .byte   $00,$00,$FF,$00,$00,$00,$FF,$FF,$AB,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$80,$E0,$30,$10,$18,$00,$00,$00,$00,$00,$C0,$E0,$E0,$08,$08
        .byte   $08,$08,$08,$08,$08,$88,$F0,$F0,$A0,$F0,$F0,$F0,$30,$30,$88,$88
        .byte   $88,$88,$88,$88,$88,$88,$30,$30,$30,$30,$30,$30,$30,$30,$88,$8A
        .byte   $8F,$8A,$8A,$8A,$8A,$88,$35,$37,$32,$37,$37,$37,$37,$35,$88,$08
        .byte   $08,$08,$08,$08,$08,$08,$30,$30,$F0,$F0,$A0,$F0,$F0,$F0,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$00,$49
        .byte   $FF,$49,$49,$49,$49,$00,$B6,$FF,$49,$FF,$FF,$FF,$AB,$B6,$18,$10
        .byte   $30,$E0,$80,$00,$00,$00,$E0,$E0,$C0,$00,$00,$00,$00,$00,$02,$03
        .byte   $02,$03,$02,$03,$02,$07,$01,$04,$05,$00,$01,$04,$0D,$18,$0E,$3B
        .byte   $A2,$23,$22,$23,$22,$03,$81,$E4,$3D,$FC,$A9,$FC,$A9,$DC,$02,$03
        .byte   $02,$03,$02,$03,$02,$03,$1D,$0C,$05,$00,$01,$04,$05,$00,$00,$00
        .byte   $18,$3C,$3C,$18,$00,$00,$FF,$C3,$99,$AD,$BD,$99,$C3,$FF,$80,$80
        .byte   $80,$80,$80,$80,$80,$80,$00,$3F,$3F,$3F,$3F,$3F,$3F,$3F,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$7F,$7F,$7F,$7F,$7F,$7F,$7F,$7F,$01,$01
        .byte   $01,$01,$01,$01,$01,$01,$00,$FC,$FC,$FC,$FC,$FC,$FC,$FC,$10,$10
        .byte   $38,$7C,$FE,$7C,$38,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$6C,$FE
        .byte   $FE,$FE,$7C,$38,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$38,$7C
        .byte   $38,$FE,$FE,$7C,$38,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$10,$38
        .byte   $7C,$FE,$7C,$38,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$04,$7A
        .byte   $00,$FE,$10,$10,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$4A,$40
        .byte   $40,$78,$4C,$40,$40,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0A,$10
        .byte   $98,$88,$8C,$84,$84,$00,$00,$00,$00,$00,$00,$00,$00,$00,$8A,$80
        .byte   $FC,$80,$80,$80,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0A,$F0
        .byte   $0C,$04,$0C,$18,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0A,$20
        .byte   $70,$D8,$8C,$06,$02,$00,$00,$00,$00,$00,$00,$00,$00,$00,$0A,$10
        .byte   $FE,$10,$92,$92,$92,$00,$00,$00,$00,$00,$00,$00,$00,$00,$16,$1A
        .byte   $8E,$8C,$84,$84,$84,$00,$00,$00,$00,$00,$00,$00,$00,$00,$86,$8A
        .byte   $FC,$80,$80,$80,$7C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$06,$FA
        .byte   $06,$04,$0C,$18,$30,$00,$00,$00,$00,$00,$00,$00,$00,$00,$06,$2A
        .byte   $76,$D8,$8C,$06,$02,$00,$00,$00,$00,$00,$00,$00,$00,$00,$16,$1A
        .byte   $FE,$10,$92,$92,$92,$00,$00,$00,$00,$00,$00,$00,$00,$00,$FF,$81
        .byte   $81,$81,$81,$81,$FF,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$70,$50,$70,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $E0,$18,$04,$02,$01,$00,$00,$00,$00,$00,$00,$00,$00,$00,$80,$80
        .byte   $40,$20,$18,$07,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$3C,$42
        .byte   $99,$A1,$A1,$99,$42,$3C,$00,$00,$00,$00,$00,$00,$00,$00,$00,$44
        .byte   $28,$10,$28,$44,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$10,$10
        .byte   $00,$7C,$00,$10,$10,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$01
        .byte   $02,$04,$18,$E0,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$FE,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$E7,$3C,$99,$5A,$00,$00,$00,$00,$00,$C3,$66,$24,$5A,$5A
        .byte   $5A,$5A,$5A,$5A,$5A,$5A,$24,$24,$24,$24,$24,$24,$24,$24,$99,$3C
        .byte   $80,$00,$00,$00,$00,$00,$66,$C3,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$FF,$FF,$00,$00,$00,$00,$00,$FF,$00,$FF,$FF,$00
        .byte   $AB,$00,$00,$00,$00,$00,$00,$FF,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$1F,$18,$00,$00,$00,$00,$00,$00,$00,$00,$1E,$18
        .byte   $18,$00,$FF,$00,$FF,$FF,$00,$00,$00,$00,$00,$FF,$00,$FF,$00,$7E
        .byte   $3E,$1E,$0E,$06,$02,$00,$FE,$82,$C6,$EE,$EE,$C6,$82,$FE,$00,$FF
        .byte   $FF,$C3,$80,$C3,$81,$00,$FF,$81,$83,$FF,$AB,$C3,$81,$FF,$F7,$BB
        .byte   $AA,$ED,$89,$DD,$FF,$FF,$08,$44,$01,$13,$B3,$BB,$BB,$FB,$F8,$F9
        .byte   $80,$00,$C7,$CF,$A3,$00,$FB,$FB,$00,$00,$DF,$DF,$DF,$00,$00,$0F
        .byte   $6F,$00,$00,$3F,$1F,$00,$3F,$70,$90,$FF,$80,$FF,$7F,$3F,$00,$FF
        .byte   $AB,$00,$00,$FF,$FF,$00,$FF,$00,$00,$FF,$00,$FF,$83,$FF,$00,$F0
        .byte   $F6,$00,$00,$FC,$F8,$00,$FC,$0E,$09,$FF,$01,$FF,$FE,$FC,$70,$7C
        .byte   $7E,$7F,$7C,$40,$40,$00,$40,$40,$40,$40,$40,$40,$40,$E0,$00,$00
        .byte   $01,$01,$02,$15,$2B,$57,$01,$01,$03,$17,$3E,$3D,$6F,$FF,$80,$C0
        .byte   $E0,$B0,$90,$EC,$F6,$D7,$80,$C0,$E0,$F0,$B0,$EC,$F6,$FF,$DD,$B9
        .byte   $A3,$FD,$81,$19,$33,$FF,$DD,$B9,$80,$FF,$FD,$D9,$B3,$FF,$B9,$DC
        .byte   $82,$F3,$A1,$F9,$00,$FF,$A8,$DC,$FE,$F7,$F7,$FB,$23,$FF,$FF,$F1
        .byte   $B9,$FF,$9F,$F0,$A0,$FF,$FF,$FF,$F9,$FF,$80,$FF,$F8,$FF,$00,$00
        .byte   $00,$20,$00,$00,$00,$00,$00,$00,$00,$20,$02,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$80,$00,$00,$00,$00,$00,$00,$00,$00,$3C,$4E
        .byte   $AF,$9F,$FF,$FF,$7E,$3C,$3C,$7E,$80,$FF,$AB,$FF,$7E,$3C,$00,$FF
        .byte   $00,$00,$00,$00,$00,$FF,$FF,$FF,$38,$6C,$C6,$83,$FF,$FF,$00,$FF
        .byte   $00,$00,$00,$00,$00,$FF,$FF,$FF,$C3,$81,$81,$C3,$80,$FF,$81,$FF
        .byte   $81,$81,$81,$FF,$81,$81,$00,$00,$00,$00,$00,$00,$00,$00,$10,$28
        .byte   $28,$10,$10,$28,$28,$10,$10,$28,$28,$10,$10,$28,$28,$10,$08,$08
        .byte   $08,$08,$28,$6C,$6E,$CA,$18,$18,$18,$18,$18,$18,$18,$18,$7F,$6F
        .byte   $4F,$4D,$65,$7D,$7B,$5F,$80,$FF,$EF,$ED,$F5,$FD,$AB,$FF,$00,$FF
        .byte   $83,$C1,$DF,$FD,$E3,$00,$FF,$FF,$FF,$E3,$DF,$FD,$80,$FF,$00,$6C
        .byte   $FE,$FE,$FE,$FE,$7C,$83,$10,$7C,$FE,$9E,$BE,$FE,$7C,$83,$0F,$3B
        .byte   $77,$EF,$FF,$FF,$80,$71,$00,$04,$08,$10,$00,$00,$00,$00,$FF,$FF
        .byte   $DD,$F3,$83,$EF,$82,$B9,$00,$00,$22,$0C,$00,$00,$10,$39,$F0,$CC
        .byte   $A2,$FF,$AB,$FF,$FE,$74,$00,$30,$08,$00,$00,$00,$00,$00,$3C,$7E
        .byte   $5A,$BD,$80,$FF,$FF,$FF,$00,$00,$24,$42,$02,$00,$00,$00,$FF,$FF
        .byte   $FF,$FF,$BF,$DB,$80,$FF,$FF,$FF,$FF,$FF,$BF,$DB,$80,$FF,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24,$C3,$00,$00,$C3,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$0F,$10,$20,$23,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$F0,$08,$04,$C4,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24,$23,$20,$10,$0F,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24,$C4,$04,$08,$F0,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24,$24,$24,$24,$24,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$FF,$00,$00,$FF,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$18,$24,$24,$24,$24,$24,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$24,$24,$24,$24,$24,$18,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$1F,$20,$20,$1F,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$F8,$04,$04,$F8,$00,$00,$18,$18
        .byte   $3C,$FF,$FF,$3C,$18,$18,$18,$18,$3C,$FF,$FF,$3C,$18,$18,$18,$18
        .byte   $3C,$FF,$00,$00,$00,$00,$18,$18,$3C,$FF,$00,$00,$00,$00,$00,$00
        .byte   $00,$FF,$FF,$3C,$18,$18,$00,$00,$00,$FF,$AB,$3C,$18,$18,$18,$18
        .byte   $38,$F8,$F8,$38,$18,$18,$18,$18,$38,$F8,$F8,$38,$18,$18,$18,$18
        .byte   $1C,$1F,$1F,$1C,$18,$18,$18,$18,$1C,$1F,$1F,$1C,$18,$18,$00,$00
        .byte   $00,$FF,$FF,$00,$00,$00,$00,$00,$00,$FF,$FF,$00,$00,$00,$18,$18
        .byte   $18,$18,$18,$18,$18,$18,$18,$18,$18,$18,$18,$18,$18,$18,$00,$00
        .byte   $00,$07,$0F,$1C,$18,$18,$00,$00,$00,$07,$0F,$1C,$18,$18,$00,$00
        .byte   $00,$E0,$F0,$38,$18,$18,$00,$00,$00,$E0,$F0,$38,$18,$18,$18,$18
        .byte   $1C,$0F,$07,$00,$00,$00,$18,$18,$1C,$0F,$07,$00,$00,$00,$18,$18
        .byte   $38,$F0,$E0,$00,$00,$00,$18,$18,$38,$F0,$E0,$00,$00,$00,$FF,$C0
        .byte   $A0,$90,$88,$84,$82,$81,$00,$3F,$5F,$6F,$77,$7B,$7D,$7E,$81,$81
        .byte   $81,$81,$81,$81,$81,$81,$7E,$7E,$7E,$7E,$7E,$7E,$7E,$7E,$81,$82
        .byte   $84,$88,$90,$A0,$C0,$FF,$7E,$7D,$7B,$77,$6F,$5F,$3F,$00,$FF,$00
        .byte   $00,$00,$00,$00,$00,$FF,$00,$FF,$FF,$FF,$FF,$FF,$FF,$00,$FF,$03
        .byte   $05,$09,$11,$21,$41,$81,$00,$FC,$FA,$F6,$EE,$DE,$BE,$7E,$81,$41
        .byte   $21,$11,$09,$05,$03,$FF,$7E,$BE,$DE,$EE,$F6,$FA,$FC,$00,$00,$00
        .byte   $00,$07,$00,$07,$00,$00,$00,$00,$0F,$07,$0F,$07,$3F,$FF,$3F,$3F
        .byte   $3F,$3F,$3F,$3F,$3F,$00,$40,$54,$D6,$D5,$D4,$D4,$80,$FF,$00,$00
        .byte   $30,$E0,$30,$E0,$1C,$0E,$00,$00,$F0,$E0,$F0,$E0,$FC,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$07,$02,$AA,$AB,$B3,$AB,$AB,$03,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF
        .byte   $FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF


        .include        "platform-code.inc"
