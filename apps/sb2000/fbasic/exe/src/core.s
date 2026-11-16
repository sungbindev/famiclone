; ------------------------------------------------------------------------
; 名称：core.s
; 说明：Family BASIC v3 解释器的 SB2000 移植版（由 tools/port.mjs 生成，不要手改）：按 analysis/banks.txt 分成常驻页与各库页，转向库页的 jsr、jmp 与地址表都经跳板。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：平台相关的部分（入口、NMI 钩子、跳板进入例程 far_enter、入口门、键盘与画面等）在 platform.s 等手写文件里；要改动请改分析文件或生成器后重新生成。
; ------------------------------------------------------------------------
        .setcpu "6502"
        .import far_enter, fb_page6
        .export L80BA_far, L8971_far


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

        .segment "CODE"
; ---- $81EC-$81F2（代码 7 字节）
L81EC:  sta     $2006
        stx     $2006
        rts
; ---- $820D-$821B（代码 15 字节）
L820D:  asl     a
        tax
        lda     L8250,x
        sta     $52
        lda     L8251,x
        sta     $53
        jmp     L895A
; ---- $8662-$86AF（代码 78 字节）
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
        jsr     L84A6_far
        sta     $4C
        sta     $57
        lda     #$FF
        sta     $03
        lda     #$03
        sta     $56
        sta     $E3
        lda     #$BF
        sta     $04
        jsr     LAEE8_far
        jsr     LB194
        jmp     LB3AB_far
; ---- $894B-$8959（代码 15 字节）
L894B:  lda     #$00
L894D:  ldx     $62
        sta     $0700,x
        inc     $62
        beq     L8957
        rts
L8957:  dec     $62
        rts
; ---- $895A-$8970（代码 23 字节）
L895A:  jsr     L893C_far
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
; ---- $89C1-$89D1（代码 17 字节）
L89C1:  lda     #$00
L89C3:  ldx     $33A5
        sta     $2001
L89C9:  lda     #$00
        sta     $63
        jsr     LB53C
        rts
; ---- $89D2-$89D7（代码 6 字节）
L89D2:  jsr     L89D8
        jmp     L89C9
; ---- $89D8-$89E3（代码 12 字节）
L89D8:  jsr     LB52E
        lda     $2007
        lda     $2007
        sta     $73
        rts
; ---- $89E4-$89F1（代码 14 字节）
L89E4:  jsr     L89D8
        jsr     LB52E
        lda     $61
L89EC:  sta     $2007
        jmp     L89C9
; ---- $89F2-$89F9（代码 8 字节）
L89F2:  jsr     LB52E
        lda     $73
        jmp     L89EC
; ---- $89FA-$8A0B（代码 18 字节）
L89FA:  jsr     LB52E
        ldx     $48
        lda     #$20
L8A01:  sta     $2007
        inx
        cpx     #$1C
        bne     L8A01
        jmp     L8ABA_far
; ---- $8B74-$8B8B（代码 24 字节）
L8B74:  jsr     LB52E
        ldx     #$00
        lda     $2007
L8B7C:  lda     $2007
        sta     $0500,x
        inx
        cpx     #$1C
        bne     L8B7C
        lda     #$0F
        jmp     L8ABC_far
; ---- $8B8C-$8BA3（代码 24 字节）
L8B8C:  inc     $49
        jsr     LB4F4_far
        jsr     LB52E
        ldx     #$00
L8B96:  lda     $0500,x
        sta     $2007
        inx
        cpx     #$1C
        bne     L8B96
        jmp     L8B6E_far
; ---- $8BA4-$8BAC（代码 9 字节）
L8BA4:  lda     #$00
        sta     $09
        lda     #$05
        sta     $0A
        rts
; ---- $8C20-$8C26（代码 7 字节）
L8C20:  ldy     #$00
        sta     ($09),y
        jmp     L908C
; ---- $8DB1-$8DF1（代码 65 字节）
L8DB1:  lda     $7C
        sta     $84
        lda     $7D
        sta     $85
        lda     $7E
        sta     $44
        lda     $7F
        sta     $45
        jsr     L8540_far
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
L8DEF:  jmp     L8494_far
; ---- $8DF2-$8DFC（代码 11 字节）
L8DF2:  lda     $29
        cmp     #$80
        beq     L8DF9
        rts
L8DF9:  lda     $28
        sec
        rts
; ---- $8F9D-$8FD3（代码 55 字节）
L8F9D:  jsr     LAA0D
        jsr     L8F7D_far
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
        jmp     L8F77_far
; ---- $9088-$9097（代码 16 字节）
L9088:  ldy     #$00
        lda     ($09),y
L908C:  inc     $09
        beq     L9093
        and     #$FF
        rts
L9093:  inc     $0A
        and     #$FF
        rts
; ---- $9098-$90A5（代码 14 字节）
L9098:  ldy     #$00
        inc     $09
        beq     L90A1
        lda     ($09),y
        rts
L90A1:  inc     $0A
        lda     ($09),y
        rts
; ---- $90A6-$90B1（代码 12 字节）
L90A6:  lda     $09
        beq     L90AD
        dec     $09
        rts
L90AD:  dec     $09
        dec     $0A
        rts
; ---- $90B2-$90B6（代码 5 字节）
L90B2:  ldy     #$00
        lda     ($09),y
        rts
; ---- $90BC-$90C7（代码 12 字节）
L90BC:  jsr     L908C
L90BF:  ldy     #$00
        lda     ($09),y
        cmp     #$20
        beq     L90BC
        rts
; ---- $91B8-$91C2（代码 11 字节）
L91B8:  cmp     #$0A
        bcs     L91BF
        adc     #$30
        rts
L91BF:  clc
        adc     #$37
        rts
; ---- $91C3-$91D9（代码 23 字节）
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
; ---- $9224-$9233（代码 16 字节）
L9224:  txa
        pha
        lda     #$10
        sta     $2C
        lda     #$00
        sta     $2D
        jsr     L8F35_far
        pla
        tax
        rts
; ---- $9234-$923B（代码 8 字节）
L9234:  jsr     L9240_far
L9237:  ldy     #$00
        lda     ($4D),y
        rts
; ---- $93DA-$93E5（代码 12 字节）
L93DA:  ldy     #$00
        lda     ($54),y
L93DE:  rts
L93DF:  inc     $54
        bne     L93DE
        inc     $55
        rts
; ---- $93E6-$942F（代码 74 字节）
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
; ---- $9430-$9438（代码 9 字节）
L9430:  lda     $09
        sta     $9A
        lda     $0A
        sta     $9B
        rts
; ---- $9439-$9441（代码 9 字节）
L9439:  lda     $9A
        sta     $09
        lda     $9B
        sta     $0A
        rts
; ---- $94DE-$950F（代码 50 字节）
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
        jsr     L9078_far
        jsr     L93DF
        jmp     L9501
L950F:  rts
; ---- $9510-$9515（代码 6 字节）
L9510:  jsr     LA26E
        jmp     LB237_far
; ---- $9570-$95E2（代码 115 字节）
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
        jsr     L9442_far
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
        jsr     L87EE_far
        jsr     L87C4_far
        jmp     L9581
L95D2:  jsr     L90B2
        clc
        adc     $09
        sta     $09
        lda     #$00
        adc     $0A
        sta     $0A
        jmp     L9581
; ---- $95E3-$9624（代码 66 字节）
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
; ---- $9882-$9903（代码 130 字节）
L9882:  lda     #$00
        jmp     LB237_far
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
        jsr     L9904_far
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
L9901:  jmp     L969C_far
; ---- $9B65-$9C08（代码 164 字节）
L9B65:  jmp     L8494_far
L9B68:  cmp     #$86
        beq     L9BB1
        jsr     LA26E
        beq     L9B65
        cmp     #$09
        bcs     L9B65
        pha
        jsr     LAC14_far
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
; ---- $9E8A-$9E9C（代码 19 字节）
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
; ---- $9E9D-$9EB0（代码 20 字节）
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
; ---- $9EB1-$9EC8（代码 24 字节）
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
; ---- $9EC9-$9EF0（代码 40 字节）
L9EC9:  beq     L9EE3
        jsr     L96F0_far
        jsr     L9B56_far
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
; ---- $A01E-$A090（代码 115 字节）
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
LA070:  jsr     L8855_far
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
        jmp     L8491_far
; ---- $A241-$A25E（代码 30 字节）
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
; ---- $A25F-$A294（代码 54 字节）
LA25F:  jmp     L8491_far
LA262:  jmp     L8494_far
LA265:  jsr     L908C
        jmp     LA26E
LA26B:  jsr     LAC14_far
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
        jsr     LA29B_far
        lda     $27
        beq     LA25F
        rts
; ---- $A7CE-$A7D4（代码 7 字节）
LA7CE:  lda     #$00
        sta     $2D
        sta     $29
        rts
; ---- $A91F-$A924（代码 6 字节）
LA91F:  jsr     LA925
        jmp     LA8EE_far
; ---- $A925-$A932（代码 14 字节）
LA925:  sec
        lda     $03
        sbc     $39
        sta     $28
        lda     $04
        sbc     $20
        sta     $29
        rts
; ---- $A933-$A940（代码 14 字节）
LA933:  jsr     LAA0A
        lda     $48
LA938:  sta     $28
        lda     #$00
        sta     $29
        jmp     LA97E
; ---- $A958-$A960（代码 9 字节）
LA958:  jsr     LA941_far
        lda     fb_page6+198,x
        jmp     LA938
; ---- $A961-$A969（代码 9 字节）
LA961:  jsr     LA941_far
        lda     fb_page6+199,x
        jmp     LA938
; ---- $A96A-$A972（代码 9 字节）
LA96A:  jsr     LA941_far
        lda     fb_page6+192,x
        jmp     LA938
; ---- $A973-$A983（代码 17 字节）
LA973:  jsr     LAA0A
        ldy     #$00
        lda     ($28),y
        sta     $28
        sty     $29
LA97E:  jsr     LA87D_far
        jmp     LA8F4_far
; ---- $A984-$A99B（代码 24 字节）
LA984:  jsr     LAA0A
        jsr     L8DF2
        beq     LA999
        lda     $29
        bmi     LA993
        jmp     LA97E
LA993:  jsr     L8E32_far
        jmp     LA97E
LA999:  jmp     L8EF2_far
; ---- $A99C-$A9FE（代码 99 字节）
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
        jsr     L8F44_far
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
        jsr     L8F44_far
        asl     $29
        rol     $36
        rol     $37
        lda     $36
        sta     $28
        lda     $37
        sta     $29
        jmp     LA97E
; ---- $A9FF-$AA16（代码 24 字节）
LA9FF:  lda     #$0B
        jmp     LB237_far
LAA04:  jsr     LAA0A
        jmp     LAB80
LAA0A:  jsr     LA8B6_far
LAA0D:  lda     #$02
        .byte   $AE
LAA10:  lda     #$03
        cmp     $27
        bne     LA9FF
        rts
; ---- $AA17-$AA1C（代码 6 字节）
LAA17:  jsr     LA8B6_far
        jmp     LAA10
; ---- $AA1D-$AA37（代码 27 字节）
LAA1D:  jsr     LAA17
LAA20:  lda     #$00
        jsr     LA8B1_far
        jmp     LA97E
LAA28:  jsr     LAA17
        lda     $28
        beq     LAA20
        ldy     #$00
        lda     ($29),y
        sta     $28
        jmp     LAA20
; ---- $AAD6-$AADE（代码 9 字节）
LAAD6:  jsr     LAA0A
        jsr     L91DA_far
        jmp     LAAAC_far
; ---- $AADF-$AB12（代码 52 字节）
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
        jsr     LAB2D_far
        jsr     L9439
        jmp     LA97E
LAB0D:  jsr     LA8AD_far
        jmp     LA97E
; ---- $AB80-$AB8C（代码 13 字节）
LAB80:  lda     $29
        bne     LAB8A
        rts
LAB85:  cmp     #$20
        bcs     LAB8A
        rts
LAB8A:  jmp     L8494_far
; ---- $ABAF-$ABEC（代码 62 字节）
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
        jsr     LAC1E_far
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
; ---- $AD02-$AD37（代码 54 字节）
LAD02:  jmp     L8494_far
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
; ---- $AD38-$AD46（代码 15 字节）
LAD38:  jsr     LAD05
        jsr     LAD08
        lda     $90
        and     #$0F
LAD42:  sta     $28
        jmp     LA97E
; ---- $AD47-$AD52（代码 12 字节）
LAD47:  jsr     LAD05
        lda     $90
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jmp     LAD42
; ---- $AD53-$AD57（代码 5 字节）
LAD53:  asl     a
        asl     a
        asl     a
        asl     a
        rts
; ---- $AD96-$ADD0（代码 59 字节）
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
        ldy     #$00
LADB8:  lda     ($1B),y
        sta     ($19),y
        inc     $1B
        bne     LADC2
        inc     $1C
LADC2:  inc     $19
        bne     LADC8
        inc     $1A
LADC8:  lda     #$C0
        cmp     $1A
        bne     LADB8
        jmp     L97A6_far
; ---- $ADFE-$AE04（代码 7 字节）
LADFE:  lda     #$01
        sta     $28
        jmp     LAE13_far
; ---- $AE93-$AEE7（代码 85 字节）
LAE93:  jmp     L8494_far
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
; ---- $B194-$B1B4（代码 33 字节）
LB194:  ldx     #$3D
        lda     #$00
LB198:  sta     fb_page6,x
        inx
        bne     LB198
        ldx     #$00
        lda     #$EF
LB1A2:  sta     fb_page6+128,x
        inx
        cpx     #$20
        bne     LB1A2
        ldx     #$C0
        lda     #$00
LB1AE:  sta     fb_page6,x
        inx
        bne     LB1AE
        rts
; ---- $B2F7-$B326（代码 48 字节）
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
        jsr     LB2CF_far
        jsr     L895D
        lda     #<LB349
        sta     $52
        lda     #>LB349
        sta     $53
        jmp     L895A
; ---- $B327-$B337（代码 17 字节）
LB327:  lda     #<LB338
        sta     $52
        lda     #>LB338
        sta     $53
        jsr     L895A
        jsr     LB56B
        jmp     LB313
; ---- $B485-$B4A6（代码 34 字节）
LB485:  ldy     #$04
        ldx     #$C0
        lda     #$20
LB48B:  sta     $2007
        dex
        bne     LB48B
        dey
        bne     LB48B
LB494:  ldx     #$C0
        lda     $4F
        clc
        adc     #$03
        jsr     L81EC
        lda     #$00
LB4A0:  sta     $2007
        inx
        bne     LB4A0
        rts
; ---- $B52E-$B538（代码 11 字节）
LB52E:  lda     $4B
        sta     $2006
        lda     $4A
        sta     $2006
        rts
; ---- $B539-$B558（代码 32 字节）
LB539:  sta     $2007
LB53C:  jsr     LBE14
        lda     $F0
        beq     LB54E
        lda     #$00
        sta     $2005
        lda     $E5
        sta     $2005
        rts
LB54E:  lda     #$00
        sta     $2005
        lda     $E4
        sta     $2005
        rts
; ---- $B559-$B561（代码 9 字节）
LB559:  jsr     LB42B_far
        lda     #$00
        sta     $2001
        rts
; ---- $B562-$B5AC（代码 75 字节）
LB562:  jsr     LB42B_far
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
; ---- $B637-$B644（代码 14 字节）
LB637:  lda     $4F
        cmp     #$20
        beq     LB641
        lda     fb_page6+27,x
        rts
LB641:  lda     fb_page6+1,x
        rts
; ---- $B656-$B65E（代码 9 字节）
LB656:  lda     $48
        sta     $6B
        lda     $49
        sta     $6C
        rts
; ---- $B65F-$B667（代码 9 字节）
LB65F:  lda     $6B
        sta     $48
        lda     $6C
        sta     $49
        rts
; ---- $B668-$B68E（代码 39 字节）
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
        jsr     LB436_far
        jsr     LB494
        sta     $4C
        jsr     LB53C
        jmp     LB562
; ---- $B6DF-$B708（代码 42 字节）
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
; ---- $B709-$B70F（代码 7 字节）
LB709:  lda     #$FF
        eor     $4C
        sta     $4C
        rts
; ---- $B710-$B716（代码 7 字节）
LB710:  lda     $49
        beq     LB716
        dec     $49
LB716:  rts
; ---- $B717-$B720（代码 10 字节）
LB717:  lda     $48
        pha
        jsr     LB751
        pla
        sta     $48
        rts
; ---- $B721-$B732（代码 18 字节）
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
; ---- $B733-$B747（代码 21 字节）
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
; ---- $B748-$B750（代码 9 字节）
LB748:  lda     #$00
        sta     $49
LB74C:  lda     #$00
        sta     $48
        rts
; ---- $B751-$B75D（代码 13 字节）
LB751:  ldx     $49
        inx
        cpx     #$18
        bne     LB75B
        jmp     LB5C7_far
LB75B:  stx     $49
        rts
; ---- $BCD0-$BD03（代码 52 字节）
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
; ---- $BDE7-$BDFC（代码 22 字节）
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
; ---- $BDFD-$BE0D（代码 17 字节）
LBDFD:  lda     #$00
        sta     $AD
        lda     #$05
        sta     $AE
        lda     #$80
        sta     $AF
        lda     #$05
        sta     $B0
        rts
; ---- $BE0E-$BE13（代码 6 字节）
LBE0E:  lda     $32
        sta     $2000
        rts
; ---- $BE14-$BE1B（代码 8 字节）
LBE14:  lda     $32
        ora     #$80
        sta     $2000
        rts
; ---- $BE1C-$BE23（代码 8 字节）
LBE1C:  jsr     LBE0E
        lda     #$01
        jmp     LBE29_far
; ---- $C1C0-$C1CB（代码 12 字节）
LC1C0:  lda     $49
        cmp     #$15
        bcs     LC1C7
        rts
LC1C7:  lda     #$14
        sta     $49
        rts
; ---- $C1CC-$C21E（代码 83 字节）
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
        jsr     L913E_far
        lda     #$3D
        clc
        adc     #$03
        sta     $52
        lda     #$00
        sta     $53
        jmp     L895A
; ---- $C25C-$C275（代码 26 字节）
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
; ---- $C276-$C2A1（代码 44 字节）
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
; ---- $C2AF-$C2B4（代码 6 字节）
LC2AF:  jsr     LC2B5
        jmp     LC303_far
; ---- $C2B5-$C2D3（代码 31 字节）
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
; ---- $C2D4-$C2E4（代码 17 字节）
LC2D4:  jsr     LB656
        jsr     LC01A_far
        lda     #$20
        jsr     LB575
        jsr     LB65F
        jmp     LC303_far
; ---- $C34D-$C37C（代码 48 字节）
LC34D:  lda     #$00
        sta     $88
        beq     LC35B
LC353:  inc     $88
        lda     $88
        cmp     #$04
        beq     LC34D
LC35B:  asl     a
        asl     a
        ora     $88
        pha
        jsr     LB42B_far
        lda     #$27
        ldx     #$F3
        jsr     L81EC
        pla
        tax
        and     #$FC
        sta     $2007
        txa
        ldx     #$04
LC374:  sta     $2007
        dex
        bne     LC374
        jmp     LB53C
; ---- $C37D-$C393（代码 23 字节）
LC37D:  lda     #$0C
        sta     $48
        lda     #$16
        sta     $49
        lda     #$05
        jsr     LB575
        inc     $49
        lda     #$05
        jsr     LB575
        jmp     LC079_far
; ---- $C452-$C4AF（代码 94 字节）
LC452:  inc     $48
        inc     $48
        lda     $E4
        ldx     $4F
        cpx     #$20
        beq     LC460
        lda     $E5
LC460:  lsr     a
        lsr     a
        lsr     a
        clc
        adc     $49
        adc     #$03
        cmp     #$1E
        bcc     LC46E
        sbc     #$1E
LC46E:  sta     $49
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
        sta     $19
        lda     $48
        and     #$03
        lsr     a
        clc
        adc     $19
        sta     $19
        lda     #$FC
        sta     $1A
        asl     $19
        ldx     $19
        beq     LC4A8
LC4A0:  asl     $9C
        sec
        rol     $1A
        dex
        bne     LC4A0
LC4A8:  lda     #$05
        jsr     L896A
        lda     $73
        rts
; ---- $C4B0-$C4C6（代码 23 字节）
LC4B0:  jsr     LC452
        sta     $9C
        lda     $1A
        eor     #$FF
        and     $9C
        sta     $9C
        ldy     $19
        beq     LC4C6
LC4C1:  lsr     $9C
        dey
        bne     LC4C1
LC4C6:  rts
; ---- $C4C7-$C4FE（代码 56 字节）
LC4C7:  jmp     L8494_far
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
; ---- $C4FF-$C50C（代码 14 字节）
LC4FF:  jsr     LC452
        and     $1A
        ora     $9C
        sta     $73
        lda     #$09
        jmp     L896A
; ---- $C649-$C67A（代码 50 字节）
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
; ---- $C6DF-$C708（代码 42 字节）
LC6DF:  jsr     LB56F
        lda     #<LC68C
        sta     $52
        lda     #>LC68C
        sta     $53
        jsr     L895A
        jsr     LBD04_far
        lda     $0500
        cmp     #$03
        beq     LC704
        jsr     L8BA4
        lda     #$09
        sta     $09
        jsr     L9E52_far
        jmp     L9E9D
LC704:  pla
        pla
        jmp     LC6D0_far

; ---- 跳板
L80BA_far:
        jsr     far_enter
        .word   L80BA
        .byte   4
L818F_far:
        jsr     far_enter
        .word   L818F
        .byte   4
L825E_far:
        jsr     far_enter
        .word   L825E
        .byte   2
L826A_far:
        jsr     far_enter
        .word   L826A
        .byte   2
L82CC_far:
        jsr     far_enter
        .word   L82CC
        .byte   3
L8316_far:
        jsr     far_enter
        .word   L8316
        .byte   3
L83CB_far:
        jsr     far_enter
        .word   L83CB
        .byte   2
L8467_far:
        jsr     far_enter
        .word   L8467
        .byte   2
L8491_far:
        jsr     far_enter
        .word   L8491
        .byte   2
L8494_far:
        jsr     far_enter
        .word   L8494
        .byte   2
L8499_far:
        jsr     far_enter
        .word   L8499
        .byte   2
L849C_far:
        jsr     far_enter
        .word   L849C
        .byte   2
L84A6_far:
        jsr     far_enter
        .word   L84A6
        .byte   2
L84B6_far:
        jsr     far_enter
        .word   L84B6
        .byte   2
L8540_far:
        jsr     far_enter
        .word   L8540
        .byte   2
L8581_far:
        jsr     far_enter
        .word   L8581
        .byte   2
L85BE_far:
        jsr     far_enter
        .word   L85BE
        .byte   3
L86B0_far:
        jsr     far_enter
        .word   L86B0
        .byte   2
L86D9_far:
        jsr     far_enter
        .word   L86D9
        .byte   2
L86E6_far:
        jsr     far_enter
        .word   L86E6
        .byte   2
L8758_far:
        jsr     far_enter
        .word   L8758
        .byte   2
L8767_far:
        jsr     far_enter
        .word   L8767
        .byte   3
L876E_far:
        jsr     far_enter
        .word   L876E
        .byte   3
L87C4_far:
        jsr     far_enter
        .word   L87C4
        .byte   3
L87E0_far:
        jsr     far_enter
        .word   L87E0
        .byte   3
L87EE_far:
        jsr     far_enter
        .word   L87EE
        .byte   1
L883A_far:
        jsr     far_enter
        .word   L883A
        .byte   3
L8855_far:
        jsr     far_enter
        .word   L8855
        .byte   3
L885B_far:
        jsr     far_enter
        .word   L885B
        .byte   3
L8883_far:
        jsr     far_enter
        .word   L8883
        .byte   2
L889C_far:
        jsr     far_enter
        .word   L889C
        .byte   2
L88C0_far:
        jsr     far_enter
        .word   L88C0
        .byte   2
L88C5_far:
        jsr     far_enter
        .word   L88C5
        .byte   3
L893C_far:
        jsr     far_enter
        .word   L893C
        .byte   3
L8971_far:
        jsr     far_enter
        .word   L8971
        .byte   5
L8A0C_far:
        jsr     far_enter
        .word   L8A0C
        .byte   4
L8A87_far:
        jsr     far_enter
        .word   L8A87
        .byte   4
L8ABA_far:
        jsr     far_enter
        .word   L8ABA
        .byte   4
L8ABC_far:
        jsr     far_enter
        .word   L8ABC
        .byte   4
L8AC3_far:
        jsr     far_enter
        .word   L8AC3
        .byte   4
L8AF2_far:
        jsr     far_enter
        .word   L8AF2
        .byte   4
L8B30_far:
        jsr     far_enter
        .word   L8B30
        .byte   4
L8B3E_far:
        jsr     far_enter
        .word   L8B3E
        .byte   4
L8B6E_far:
        jsr     far_enter
        .word   L8B6E
        .byte   4
L8BAD_far:
        jsr     far_enter
        .word   L8BAD
        .byte   3
L8BB6_far:
        jsr     far_enter
        .word   L8BB6
        .byte   3
L8BC8_far:
        jsr     far_enter
        .word   L8BC8
        .byte   3
L8BCD_far:
        jsr     far_enter
        .word   L8BCD
        .byte   3
L8BDA_far:
        jsr     far_enter
        .word   L8BDA
        .byte   3
L8C30_far:
        jsr     far_enter
        .word   L8C30
        .byte   2
L8E0F_far:
        jsr     far_enter
        .word   L8E0F
        .byte   1
L8E32_far:
        jsr     far_enter
        .word   L8E32
        .byte   1
L8EAC_far:
        jsr     far_enter
        .word   L8EAC
        .byte   1
L8EF2_far:
        jsr     far_enter
        .word   L8EF2
        .byte   1
L8EF7_far:
        jsr     far_enter
        .word   L8EF7
        .byte   1
L8F0F_far:
        jsr     far_enter
        .word   L8F0F
        .byte   1
L8F35_far:
        jsr     far_enter
        .word   L8F35
        .byte   1
L8F44_far:
        jsr     far_enter
        .word   L8F44
        .byte   1
L8F47_far:
        jsr     far_enter
        .word   L8F47
        .byte   1
L8F77_far:
        jsr     far_enter
        .word   L8F77
        .byte   1
L8F7D_far:
        jsr     far_enter
        .word   L8F7D
        .byte   1
L8FE2_far:
        jsr     far_enter
        .word   L8FE2
        .byte   3
L9078_far:
        jsr     far_enter
        .word   L9078
        .byte   1
L90C8_far:
        jsr     far_enter
        .word   L90C8
        .byte   1
L90D6_far:
        jsr     far_enter
        .word   L90D6
        .byte   1
L90FD_far:
        jsr     far_enter
        .word   L90FD
        .byte   1
L913E_far:
        jsr     far_enter
        .word   L913E
        .byte   3
L91DA_far:
        jsr     far_enter
        .word   L91DA
        .byte   1
L9240_far:
        jsr     far_enter
        .word   L9240
        .byte   1
L925B_far:
        jsr     far_enter
        .word   L925B
        .byte   1
L9442_far:
        jsr     far_enter
        .word   L9442
        .byte   1
L9527_far:
        jsr     far_enter
        .word   L9527
        .byte   2
L9625_far:
        jsr     far_enter
        .word   L9625
        .byte   2
L962B_far:
        jsr     far_enter
        .word   L962B
        .byte   2
L9648_far:
        jsr     far_enter
        .word   L9648
        .byte   2
L969C_far:
        jsr     far_enter
        .word   L969C
        .byte   2
L96A6_far:
        jsr     far_enter
        .word   L96A6
        .byte   2
L96C6_far:
        jsr     far_enter
        .word   L96C6
        .byte   2
L96F0_far:
        jsr     far_enter
        .word   L96F0
        .byte   2
L970A_far:
        jsr     far_enter
        .word   L970A
        .byte   2
L9716_far:
        jsr     far_enter
        .word   L9716
        .byte   2
L9748_far:
        jsr     far_enter
        .word   L9748
        .byte   2
L975D_far:
        jsr     far_enter
        .word   L975D
        .byte   2
L9782_far:
        jsr     far_enter
        .word   L9782
        .byte   2
L97A4_far:
        jsr     far_enter
        .word   L97A4
        .byte   2
L97A6_far:
        jsr     far_enter
        .word   L97A6
        .byte   2
L97FC_far:
        jsr     far_enter
        .word   L97FC
        .byte   2
L9904_far:
        jsr     far_enter
        .word   L9904
        .byte   3
L991A_far:
        jsr     far_enter
        .word   L991A
        .byte   1
L993C_far:
        jsr     far_enter
        .word   L993C
        .byte   1
L9A0E_far:
        jsr     far_enter
        .word   L9A0E
        .byte   1
L9A6C_far:
        jsr     far_enter
        .word   L9A6C
        .byte   1
L9AFD_far:
        jsr     far_enter
        .word   L9AFD
        .byte   2
L9B56_far:
        jsr     far_enter
        .word   L9B56
        .byte   2
L9C0C_far:
        jsr     far_enter
        .word   L9C0C
        .byte   3
L9C70_far:
        jsr     far_enter
        .word   L9C70
        .byte   1
L9D64_far:
        jsr     far_enter
        .word   L9D64
        .byte   1
L9DE1_far:
        jsr     far_enter
        .word   L9DE1
        .byte   1
L9E52_far:
        jsr     far_enter
        .word   L9E52
        .byte   1
L9F36_far:
        jsr     far_enter
        .word   L9F36
        .byte   2
LA091_far:
        jsr     far_enter
        .word   LA091
        .byte   5
LA29B_far:
        jsr     far_enter
        .word   LA29B
        .byte   3
LA304_far:
        jsr     far_enter
        .word   LA304
        .byte   2
LA3C1_far:
        jsr     far_enter
        .word   LA3C1
        .byte   2
LA576_far:
        jsr     far_enter
        .word   LA576
        .byte   1
LA57F_far:
        jsr     far_enter
        .word   LA57F
        .byte   1
LA7BC_far:
        jsr     far_enter
        .word   LA7BC
        .byte   2
LA7C4_far:
        jsr     far_enter
        .word   LA7C4
        .byte   2
LA7D5_far:
        jsr     far_enter
        .word   LA7D5
        .byte   3
LA7EF_far:
        jsr     far_enter
        .word   LA7EF
        .byte   3
LA7F5_far:
        jsr     far_enter
        .word   LA7F5
        .byte   3
LA7FB_far:
        jsr     far_enter
        .word   LA7FB
        .byte   3
LA80E_far:
        jsr     far_enter
        .word   LA80E
        .byte   3
LA82F_far:
        jsr     far_enter
        .word   LA82F
        .byte   3
LA842_far:
        jsr     far_enter
        .word   LA842
        .byte   3
LA84C_far:
        jsr     far_enter
        .word   LA84C
        .byte   3
LA854_far:
        jsr     far_enter
        .word   LA854
        .byte   3
LA85C_far:
        jsr     far_enter
        .word   LA85C
        .byte   3
LA866_far:
        jsr     far_enter
        .word   LA866
        .byte   3
LA87D_far:
        jsr     far_enter
        .word   LA87D
        .byte   3
LA88E_far:
        jsr     far_enter
        .word   LA88E
        .byte   3
LA8AD_far:
        jsr     far_enter
        .word   LA8AD
        .byte   3
LA8B1_far:
        jsr     far_enter
        .word   LA8B1
        .byte   3
LA8B6_far:
        jsr     far_enter
        .word   LA8B6
        .byte   3
LA8E6_far:
        jsr     far_enter
        .word   LA8E6
        .byte   3
LA8EE_far:
        jsr     far_enter
        .word   LA8EE
        .byte   3
LA8F4_far:
        jsr     far_enter
        .word   LA8F4
        .byte   3
LA90D_far:
        jsr     far_enter
        .word   LA90D
        .byte   3
LA914_far:
        jsr     far_enter
        .word   LA914
        .byte   3
LA941_far:
        jsr     far_enter
        .word   LA941
        .byte   3
LAA3B_far:
        jsr     far_enter
        .word   LAA3B
        .byte   3
LAA82_far:
        jsr     far_enter
        .word   LAA82
        .byte   3
LAAA6_far:
        jsr     far_enter
        .word   LAAA6
        .byte   3
LAAAC_far:
        jsr     far_enter
        .word   LAAAC
        .byte   3
LAB2D_far:
        jsr     far_enter
        .word   LAB2D
        .byte   1
LAB58_far:
        jsr     far_enter
        .word   LAB58
        .byte   3
LABED_far:
        jsr     far_enter
        .word   LABED
        .byte   3
LAC14_far:
        jsr     far_enter
        .word   LAC14
        .byte   3
LAC1E_far:
        jsr     far_enter
        .word   LAC1E
        .byte   3
LAC41_far:
        jsr     far_enter
        .word   LAC41
        .byte   3
LAC81_far:
        jsr     far_enter
        .word   LAC81
        .byte   3
LAD5B_far:
        jsr     far_enter
        .word   LAD5B
        .byte   3
LAE0A_far:
        jsr     far_enter
        .word   LAE0A
        .byte   4
LAE13_far:
        jsr     far_enter
        .word   LAE13
        .byte   4
LAE48_far:
        jsr     far_enter
        .word   LAE48
        .byte   4
LAEE8_far:
        jsr     far_enter
        .word   LAEE8
        .byte   4
LAEF3_far:
        jsr     far_enter
        .word   LAEF3
        .byte   3
LAF4F_far:
        jsr     far_enter
        .word   LAF4F
        .byte   3
LAFC9_far:
        jsr     far_enter
        .word   LAFC9
        .byte   3
LAFD5_far:
        jsr     far_enter
        .word   LAFD5
        .byte   3
LAFFC_far:
        jsr     far_enter
        .word   LAFFC
        .byte   3
LB098_far:
        jsr     far_enter
        .word   LB098
        .byte   3
LB153_far:
        jsr     far_enter
        .word   LB153
        .byte   3
LB1B8_far:
        jsr     far_enter
        .word   LB1B8
        .byte   2
LB201_far:
        jsr     far_enter
        .word   LB201
        .byte   2
LB206_far:
        jsr     far_enter
        .word   LB206
        .byte   2
LB237_far:
        jsr     far_enter
        .word   LB237
        .byte   2
LB2A2_far:
        jsr     far_enter
        .word   LB2A2
        .byte   2
LB2C3_far:
        jsr     far_enter
        .word   LB2C3
        .byte   3
LB2CF_far:
        jsr     far_enter
        .word   LB2CF
        .byte   3
LB3AB_far:
        jsr     far_enter
        .word   LB3AB
        .byte   4
LB42B_far:
        jsr     far_enter
        .word   LB42B
        .byte   4
LB436_far:
        jsr     far_enter
        .word   LB436
        .byte   3
LB4AB_far:
        jsr     far_enter
        .word   LB4AB
        .byte   4
LB4F4_far:
        jsr     far_enter
        .word   LB4F4
        .byte   4
LB5B5_far:
        jsr     far_enter
        .word   LB5B5
        .byte   4
LB5B8_far:
        jsr     far_enter
        .word   LB5B8
        .byte   4
LB5C7_far:
        jsr     far_enter
        .word   LB5C7
        .byte   4
LB68F_far:
        jsr     far_enter
        .word   LB68F
        .byte   4
LB6B1_far:
        jsr     far_enter
        .word   LB6B1
        .byte   4
LB75E_far:
        jsr     far_enter
        .word   LB75E
        .byte   4
LB766_far:
        jsr     far_enter
        .word   LB766
        .byte   4
LB76F_far:
        jsr     far_enter
        .word   LB76F
        .byte   4
LB805_far:
        jsr     far_enter
        .word   LB805
        .byte   4
LB945_far:
        jsr     far_enter
        .word   LB945
        .byte   4
LBC20_far:
        jsr     far_enter
        .word   LBC20
        .byte   4
LBD04_far:
        jsr     far_enter
        .word   LBD04
        .byte   4
LBD6C_far:
        jsr     far_enter
        .word   LBD6C
        .byte   4
LBE29_far:
        jsr     far_enter
        .word   LBE29
        .byte   1
LBFCE_far:
        jsr     far_enter
        .word   LBFCE
        .byte   4
LBFD1_far:
        jsr     far_enter
        .word   LBFD1
        .byte   4
LC01A_far:
        jsr     far_enter
        .word   LC01A
        .byte   4
LC079_far:
        jsr     far_enter
        .word   LC079
        .byte   4
LC17D_far:
        jsr     far_enter
        .word   LC17D
        .byte   4
LC2E5_far:
        jsr     far_enter
        .word   LC2E5
        .byte   4
LC303_far:
        jsr     far_enter
        .word   LC303
        .byte   4
LC394_far:
        jsr     far_enter
        .word   LC394
        .byte   4
LC39D_far:
        jsr     far_enter
        .word   LC39D
        .byte   4
LC3A7_far:
        jsr     far_enter
        .word   LC3A7
        .byte   4
LC3B5_far:
        jsr     far_enter
        .word   LC3B5
        .byte   4
LC3C3_far:
        jsr     far_enter
        .word   LC3C3
        .byte   4
LC50D_far:
        jsr     far_enter
        .word   LC50D
        .byte   1
LC565_far:
        jsr     far_enter
        .word   LC565
        .byte   1
LC5BB_far:
        jsr     far_enter
        .word   LC5BB
        .byte   4
LC5BE_far:
        jsr     far_enter
        .word   LC5BE
        .byte   4
LC697_far:
        jsr     far_enter
        .word   LC697
        .byte   4
LC6D0_far:
        jsr     far_enter
        .word   LC6D0
        .byte   4
LCB9B_far:
        jsr     far_enter
        .word   LCB9B
        .byte   3
LCBB3_far:
        jsr     far_enter
        .word   LCBB3
        .byte   3
LCBD9_far:
        jsr     far_enter
        .word   LCBD9
        .byte   3
LCBDC_far:
        jsr     far_enter
        .word   LCBDC
        .byte   3
LCC04_far:
        jsr     far_enter
        .word   LCC04
        .byte   3
LCC6C_far:
        jsr     far_enter
        .word   LCC6C
        .byte   3

        .segment "RODATA"
; ---- $8003-$800A（数据 8 字节）
L8003:
        .byte   $41
L8004:
        .byte   $65,$8A,$6A,$8C,$6C,$F8,$6C
; ---- $800B-$8012（数据 8 字节）
L800B:
        .addr   LD400,LDBFE,LE682,LF308
; ---- $821C-$824F（数据 52 字节）
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
; ---- $8250-$825D（数据 14 字节）
L8250:
        .addr   L821C,L8220,L822D,L8234,L8242,L8246,L8248
; ---- $B338-$B3AA（数据 115 字节）
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
LB379:
        .byte   "SKIP ", $00
LB37F:
        .byte   "N"
LB380:
        .byte   "FSNRGODILOVOMULSODDDZTMSTFTCCUFMOTPNRREN", "BUP"
; ---- $B8BE-$B8FD（数据 64 字节）
LB8BE:
        .addr   LB5B5_far,LB709,LB5B5_far,LB5B8_far,LB668,LB68F_far,LB5B5_far,LBCD0
        .addr   LB76F_far,LB6DF,LB751,LB748,LB4AB_far,LB74C,LB5B5_far,LB5B5_far
        .addr   LB5B5_far,LB5B5_far,LB805_far,LB5B5_far,LB5B5_far,LB5B5_far,LB75E_far,LB766_far
        .addr   LB5B5_far,LB5B5_far,LB6B1_far,LB5B5_far,LB733,LB721,LB710,LB717
; ---- $BAA9-$BC10（数据 360 字节）
LBAA9:
        .byte   $03,$5C,$00,$FF,$B3,$B2,$0D,$F7,$5E,$2D,$2F,$00,$3B,$3A,$40,$F6
        .byte   $30,$50,$2C,$2E,$4B,$4C,$4F,$F5,$38,$39,$4E,$4D,$4A,$55,$49,$F4
        .byte   $36,$37,$56,$42,$48,$47,$59,$F3,$34,$35,$43,$46,$44,$52,$54,$F2
        .byte   $33,$45,$5A,$58,$41,$53,$57,$F1,$32,$31,$00,$00,$00,$51,$1B,$F0
        .byte   $12,$08,$20,$1F,$1D,$1C,$1E,$0B
LBAF1:
        .byte   $03,$00,$00,$00,$00,$00,$0D,$18,$00,$3D,$3F,$5F,$2B,$2A,$00,$17
        .byte   $00,$00,$3C,$3E,$00,$00,$00,$11,$28,$29,$00,$00,$00,$00,$00,$10
        .byte   $26,$27,$00,$00,$00,$00,$00,$0F,$24,$25,$00,$00,$00,$00,$00,$0E
        .byte   $23,$00,$00,$00,$00,$00,$00,$06,$22,$21,$00,$00,$00,$00,$1B,$02
        .byte   $12,$08,$20,$1F,$1D,$1C,$1E,$0C
LBB39:
        .byte   $03,$88,$00,$FF,$B1,$8A,$0D,$F7,$87,$86,$8D,$8C,$82,$2D,$89,$F6
        .byte   $78,$7D,$85,$8B,$80,$81,$7C,$F5,$76,$77,$83,$84,$7F,$7A,$7B,$F4
        .byte   $74,$75,$72,$73,$7E,$6E,$79,$F3,$63,$64,$71,$6D,$6C,$68,$69,$F2
        .byte   $62,$67,$6F,$70,$6A,$6B,$66,$F1,$61,$60,$00,$00,$00,$65,$1B,$F0
        .byte   $12,$08,$20,$1F,$1D,$1C,$1E,$0B
LBB81:
        .byte   $03,$00,$00,$FF,$5D,$5B,$0D,$F7
LBB89:
        .byte   $00,$00,$00,$00,$00,$00,$00,$F6,$00,$AF,$95,$00,$00,$00,$AE,$F5
        .byte   $00,$00,$93,$94,$00,$AC,$AD,$F4,$00,$00,$00,$00,$00,$00,$AB,$F3
        .byte   $91,$92,$96,$00,$00,$00,$00,$F2,$90,$00,$00,$00,$00,$00,$00,$F1
        .byte   $8F,$8E,$00,$00,$00,$00,$1B,$F0,$12,$08,$20,$1F,$1D,$1C,$1E,$0C
LBBC9:
        .byte   $03,$FE,$00,$00,$00,$00,$0D,$F7,$FD,$F2,$00,$00,$00,$00,$00,$F6
        .byte   $F1,$AA,$00,$00,$00,$00,$A9,$F5,$EF,$F0,$00,$00,$00,$A7,$A8,$F4
        .byte   $ED,$EE,$A4,$A5,$00,$A0,$A6,$F3,$EB,$EC,$A3,$9F,$9E,$9A,$9B,$F2
        .byte   $EA,$99,$A1,$A2,$9C,$9D,$98,$F1,$E9,$E8,$00,$00,$00,$97,$1B,$F0
        .byte   $12,$08,$20,$1F,$1D,$1C,$1E,$0B
; ---- $C21F-$C24F（数据 49 字节）
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
; ---- $C250-$C25B（数据 12 字节）
LC250:
        .addr   LC226,LC234,LC22D,LC242,LC249,LC23B
; ---- $C67B-$C696（数据 28 字节）
LC67B:
        .byte   "SAVE(S),LOAD(L)?", $00
LC68C:
        .byte   "FILE NAME", $22, $00
; ---- $C709-$C728（数据 32 字节）
LC709:
        .addr   LC739,LC739,LC745,LC74B,LC751,LC73F,LC74B,LC745
        .addr   LC757,LC757,LC745,LC745,LC75D,LC73F,LC745,LC73F
; ---- $C739-$C762（数据 42 字节）
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
; ---- $C763-$C798（数据 54 字节）
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
; ---- $C799-$C816（数据 126 字节）
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
; ---- $C817-$C8A2（数据 140 字节）
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
; ---- $CCAB-$CEBE（数据 532 字节）
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
; ---- $CF7B-$CFA3（数据 41 字节）
LCF7B:
        .byte   $4E,$53,$2D,$48,$55,$42,$41,$53,$49,$43,$20,$56,$33,$2E,$30,$20
        .byte   $00
LCF8C:
        .byte   $B4,$20,$4E,$49,$4E,$54,$45,$4E,$44,$4F,$2F,$53,$48,$41,$52,$50
        .byte   $2F,$48,$55,$44,$53,$4F,$4E,$00
; ---- $CFAE-$CFFF（数据 82 字节）
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
; ---- $FFFA-$FFFF（数据 6 字节）
        .addr   $00ED,L80BA_far,L80BA_far

        .segment "BANK1"
; ---- $87EE-$8839（代码 76 字节）
L87EE:  lda     #$00
        sta     $62
        ldy     #$01
        lda     ($09),y
        sta     $28
        iny
        lda     ($09),y
        sta     $29
        jsr     L913E_far
        jsr     L90D6
        jsr     L87E0_far
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
        jsr     L8BAD_far
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
; ---- $8DFD-$8E0A（代码 14 字节）
L8DFD:  sec
        lda     #$00
        sbc     $2C
        sta     $2C
        lda     #$00
        sbc     $2D
        sta     $2D
        rts
; ---- $8E0B-$8F43（代码 313 字节）
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
        jmp     LB237_far
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
; ---- $8F44-$8F9C（代码 89 字节）
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
; ---- $9078-$9087（代码 16 字节）
L9078:  ldy     #$00
        sta     ($09),y
        inc     $09
        beq     L9081
        rts
L9081:  dec     $09
        lda     #$00
        sta     ($09),y
        rts
; ---- $90C8-$913D（代码 118 字节）
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
; ---- $91DA-$9223（代码 74 字节）
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
; ---- $923C-$931C（代码 225 字节）
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
        jsr     LA7BC_far
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
; ---- $931D-$932D（代码 17 字节）
L931D:  rts
L931E:  jsr     L90B2
        cmp     #$20
        bne     L931D
        jsr     L924D
        jsr     L908C
        jmp     L931E
; ---- $932E-$93D9（代码 172 字节）
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
; ---- $9442-$94D7（代码 150 字节）
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
L949B:  jsr     L913E_far
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
; ---- $94D8-$94DD（代码 6 字节）
L94D8:  jsr     L923C
        jmp     L9078
; ---- $991A-$9A0A（代码 241 字节）
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
L9939:  jmp     L8491_far
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
        jsr     LBD6C_far
        jsr     L8855_far
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
        jsr     L83CB_far
        lda     $09
        sta     $52
        jmp     L99A7
; ---- $9A0B-$9ABF（代码 181 字节）
L9A0B:  jmp     L8491_far
L9A0E:  jsr     LA576
        lda     $90
        and     #$0F
        pha
        lda     $8C
        pha
        lda     $8D
        pha
        jsr     LAC14_far
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
L9A64:  jsr     L8499_far
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
L9A7D:  jsr     LBD04_far
        jsr     L8855_far
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
        jmp     L83CB_far
; ---- $9C6B-$9D43（代码 217 字节）
L9C6B:  lda     #$09
        jmp     LB237_far
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
        jsr     L8FE2_far
        bcc     L9CE1
L9CD3:  jmp     L8BC8_far
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
        sta     $39
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
        cmp     $39
        bne     L9D1F
        ldy     #$00
        lda     #$00
        sta     ($19),y
        iny
        sta     ($19),y
        iny
        sta     ($19),y
        rts
; ---- $9D44-$9D9E（代码 91 字节）
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
; ---- $9D9F-$9DD1（代码 51 字节）
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
L9DB9:  lda     #<LB379
        sta     $52
        lda     #>LB379
        sta     $53
        jsr     L895A
L9DC4:  lda     #$01
        sta     $52
        lda     #$05
        sta     $53
        jsr     L895A
        jmp     LB56F
; ---- $9DD2-$9E51（代码 128 字节）
L9DD2:  jsr     L9DB9
        jmp     L9DF5
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
        sta     $38
        jsr     L9E46
        jsr     LBDCE
        lda     $AD
        sta     $39
        lda     $AE
        sta     $20
L9E46:  ldy     #$00
        lda     #$00
        sta     ($AD),y
        rts
L9E4D:  lda     #$11
        jmp     LB237_far
; ---- $9E52-$9E89（代码 56 字节）
L9E52:  jsr     L9E8A
        lda     #$00
        sta     $EA
        jsr     L849C_far
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
; ---- $A573-$A703（代码 401 字节）
LA573:  jmp     L8491_far
LA576:  jsr     LA7C4_far
        bcs     LA573
        ldx     #$00
        stx     $30
LA57F:  ldx     #$00
        stx     $8B
        sta     $8A
        jsr     L9098
        jsr     LA7BC_far
        bcs     LA597
        sta     $8B
LA58F:  jsr     L9098
        jsr     LA7BC_far
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
        jsr     L8BB6_far
        lda     $28
        sta     $39
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
LA611:  jsr     LA29B_far
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
        jsr     LAC14_far
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
LA6BA:  jmp     L8494_far
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
; ---- $A704-$A70E（代码 11 字节）
LA704:  lda     $90
        and     #$0F
LA708:  cmp     #$02
        beq     LA70E
        lda     #$21
LA70E:  rts
; ---- $A70F-$A7B9（代码 171 字节）
LA70F:  lda     $1D
        sta     $8E
        lda     $38
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
LA747:  lda     $39
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
; ---- $AB13-$AB57（代码 69 字节）
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
; ---- $BDCE-$BDE6（代码 25 字节）
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
; ---- $BE24-$BF1E（代码 251 字节）
LBE24:  jsr     LBE0E
        lda     #$00
LBE29:  sta     $B1
        lda     #$00
        sta     $92
        jsr     LBDE7
LBE32:  lda     #$14
        sta     $91
LBE36:  jsr     LBEE3
        bcs     LBE48
        bne     LBE32
        dec     $91
        bne     LBE36
        lda     #$14
        sta     $91
LBE45:  jsr     LBEE3
LBE48:  bcs     LBEBB
        beq     LBE32
        dec     $91
        bne     LBE45
        jmp     LBE7B
LBE53:  jsr     LBE0E
        lda     #$00
        sta     $B1
        jsr     LBDFD
LBE5D:  lda     #$28
        sta     $91
LBE61:  jsr     LBEE3
        bcs     LBEBB
        bne     LBE5D
        dec     $91
        bne     LBE61
        lda     #$28
        sta     $91
LBE70:  jsr     LBEE3
        bcs     LBEBB
        beq     LBE5D
        dec     $91
        bne     LBE70
LBE7B:  lda     #$00
        sta     $91
        sta     $92
        jsr     LBEC9
        jsr     LBEC9
LBE87:  jsr     LBEF7
        bcs     LBEBB
        ldx     $B1
        bne     LBE97
        ldy     #$00
        sta     ($AD),y
        jmp     LBE9D
LBE97:  ldy     #$00
        cmp     ($AD),y
        bne     LBEBB
LBE9D:  jsr     LBDCE
        jsr     LBDDC
        bne     LBE87
        lda     $91
        sta     $2C
        lda     $92
        sta     $2D
        jsr     LBEF7
        cmp     $2D
        bne     LBEBB
        jsr     LBEF7
        cmp     $2C
        beq     LBEC2
LBEBB:  jsr     LBE14
        lda     #$02
LBEC0:  sec
        rts
LBEC2:  jsr     LBE14
        lda     #$00
        clc
        rts
LBEC9:  jsr     L883A_far
        beq     LBEC0
        lda     $4016
        and     #$02
        beq     LBEC9
LBED5:  jsr     L883A_far
        beq     LBEC0
        lda     $4016
        and     #$02
        bne     LBED5
        clc
        rts
LBEE3:  jsr     LBEC9
        bcs     LBEC0
        ldy     #$4E
        sty     $75
LBEEC:  dec     $75
        bne     LBEEC
        lda     $4016
        and     #$02
        clc
        rts
LBEF7:  lda     #$08
        sta     $74
        lda     #$00
        sta     $19
LBEFF:  lda     $19
        clc
        adc     $19
        sta     $19
        jsr     LBEE3
        bcs     LBEC0
        bne     LBF12
        jsr     LBDD5
        inc     $19
LBF12:  dec     $74
        bne     LBEFF
        jsr     LBEC9
        bcs     LBEC0
        lda     $19
        clc
        rts
; ---- $BF1F-$BF2F（代码 17 字节）
LBF1F:  jsr     LBE0E
        jsr     LBDFD
        lda     #$28
        sta     $91
        lda     #$00
        sta     $92
        jmp     LBF3E
; ---- $BF30-$BF8C（代码 93 字节）
LBF30:  jsr     LBE0E
        jsr     LBDE7
        lda     #$14
        sta     $91
        lda     #$00
        sta     $92
LBF3E:  lda     #$20
        sta     $19
        lda     #$4E
        sta     $1A
LBF46:  jsr     LBFA9
        dec     $19
        bne     LBF46
        dec     $1A
        bne     LBF46
        lda     $91
        sta     $19
LBF55:  jsr     LBFB3
        dec     $19
        bne     LBF55
LBF5C:  jsr     LBFA9
        dec     $91
        bne     LBF5C
        jsr     LBFB3
        lda     #$00
        sta     $91
        sta     $92
LBF6C:  ldy     #$00
        lda     ($AD),y
        jsr     LBF8D
        jsr     LBDCE
        jsr     LBDDC
        bne     LBF6C
        lda     $91
        pha
        lda     $92
        jsr     LBF8D
        pla
        jsr     LBF8D
        jsr     LBFB3
        jmp     LBE14
; ---- $BF8D-$BFA8（代码 28 字节）
LBF8D:  jsr     LBFB3
        ldx     #$08
LBF92:  sta     $19
        clc
        adc     $19
        bcs     LBFA0
        jsr     LBFA9
LBF9C:  dex
        bne     LBF92
        rts
LBFA0:  jsr     LBFB3
        jsr     LBDD5
        jmp     LBF9C
; ---- $BFA9-$BFB2（代码 10 字节）
LBFA9:  pha
        lda     #$34
        sta     $1B
        sta     $1C
        jmp     LBFBA
; ---- $BFB3-$BFCD（代码 27 字节）
LBFB3:  pha
        lda     #$6A
        sta     $1B
        sta     $1C
LBFBA:  lda     #$04
        sta     $4016
LBFBF:  dec     $1C
        bne     LBFBF
        lda     #$FF
        sta     $4016
LBFC8:  dec     $1B
        bne     LBFC8
        pla
        rts
; ---- $C50D-$C5AE（代码 162 字节）
LC50D:  lda     #$03
        sta     $0500
        lda     #$00
        sta     $0514
        lda     #$07
        sta     $0515
        lda     $E5
        sta     $0516
        ldx     #$00
        stx     $0512
        inx
        stx     $0513
        lda     #$00
        sta     $5F
        lda     #$24
        sta     $60
        jsr     L9D9F
LC535:  jsr     LB559
        lda     $60
        ldx     $5F
        jsr     L81EC
        lda     $2007
        ldx     #$00
LC544:  lda     $2007
        sta     $0700,x
        inx
        bne     LC544
        jsr     LB53C
        jsr     LB562
        jsr     LBF1F
        jsr     LBF30
        inc     $60
        lda     $60
        cmp     #$28
        bne     LC535
LC561:  rts
LC562:  jsr     L9DB9
LC565:  lda     #$00
        sta     $5F
        lda     #$24
        sta     $60
LC56D:  jsr     LBE53
        bcs     LC561
        lda     $0500
        cmp     #$03
        bne     LC562
        jsr     L9EB1
        bcs     LC562
        jsr     L9DAC
        lda     $0516
        sta     $E5
        jsr     LBE24
        bcs     LC561
        jsr     LB559
        lda     $60
        ldx     $5F
        jsr     L81EC
        ldx     #$00
LC597:  lda     $0700,x
        sta     $2007
        inx
        bne     LC597
        jsr     LB53C
        jsr     LB562
        inc     $60
        lda     $60
        cmp     #$28
        bne     LC56D
        rts

        .segment "BANK2"
; ---- $825E-$82CB（代码 110 字节）
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
        jsr     LB766_far
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
L8299:  jsr     L82CC_far
        jsr     LBD04_far
        cmp     #$03
        beq     L82C1
        jsr     L8BA4
        jsr     L90B2
        jsr     L90BF
        and     #$FF
        beq     L8299
        jsr     L90C8_far
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
; ---- $832D-$8333（代码 7 字节）
L832D:  lda     #$FF
        sta     $0B
        sta     $0C
        rts
; ---- $8334-$8428（代码 245 字节）
L8334:  jsr     L925B_far
        lda     #<LBB89
        sta     $0D
        lda     #>LBB89
        sta     $0E
        lda     #$00
        sta     $09
        lda     #$03
        sta     $0A
        jsr     L832D
L834A:  jsr     L8855_far
        jsr     L8BCD_far
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
        jsr     LA576_far
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
        jsr     L913E_far
        jsr     L90D6_far
        jsr     L87E0_far
        lda     #$20
        jsr     L894D
        jmp     L895D
; ---- $8429-$85BD（代码 405 字节）
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
        sta     $B006
        sta     $B007
        sta     $B008
        ldx     #$06
        stx     $05
        inx
        stx     $07
        inx
        stx     $1D
        inx
        stx     $39
        inx
        stx     $21
        lda     #$B0
        sta     $06
        sta     $08
        sta     $38
        sta     $20
        sta     $22
        rts
L84E5:  jsr     L88C0
        jsr     L832D
        jsr     L90FD_far
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
        jsr     L8316_far
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
        jsr     L925B_far
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
        jmp     L85BE_far
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
; ---- $86B0-$86D8（代码 41 字节）
L86B0:  jsr     L97A6
        jsr     L88C0
        jsr     L84AB
        jsr     LB194
        jsr     LAEE8_far
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
; ---- $86D9-$8766（代码 142 字节）
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
        jsr     L876E_far
        jmp     L97A6
; ---- $887E-$88C4（代码 71 字节）
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
; ---- $8C27-$8DB0（代码 394 字节）
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
L8C60:  jsr     LAC14_far
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
L8C80:  jsr     LAC14_far
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
; ---- $9516-$956F（代码 90 字节）
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
; ---- $9625-$975C（代码 312 字节）
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
L9713:  jmp     LBC20_far
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
L9731:  jsr     L8855_far
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
; ---- $975D-$9881（代码 293 字节）
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
        sta     $38
        ldx     $07
        inx
        bne     L97B1
        inc     $38
L97B1:  stx     $1D
        lda     $38
        sta     $20
        ldx     $1D
        inx
        bne     L97BE
        inc     $20
L97BE:  stx     $39
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
        cmp     #$C0
        bcs     L97F6
        cmp     $20
        bcc     L97F6
        bne     L97E8
        lda     $0400
        cmp     $39
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
        jsr     LA576_far
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
; ---- $9AC0-$9B64（代码 165 字节）
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
        jsr     LAC14_far
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
        jsr     LAC14_far
        jmp     L9B56
; ---- $9EF1-$9F35（代码 69 字节）
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
; ---- $9F36-$A01D（代码 232 字节）
L9F36:  jsr     LA576_far
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
        jsr     LAB2D_far
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
; ---- $A304-$A3C0（代码 189 字节）
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
        jsr     LA7D5_far
        jmp     LA344
LA356:  jsr     LA35F
        jsr     LA7EF_far
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
        jsr     LA80E_far
        jmp     LA376
LA392:  jsr     LA490
LA395:  cmp     #$FC
        beq     LA39E
        cmp     #$FD
        beq     LA3A7
        rts
LA39E:  jsr     LA3B0
        jsr     LA7FB_far
        jmp     LA395
LA3A7:  jsr     LA3B0
        jsr     LA7F5_far
        jmp     LA395
LA3B0:  lda     $27
        pha
        jsr     L908C
        jsr     LA490
        pla
        cmp     $27
        bne     LA370
        jmp     LA3C4
; ---- $A3C1-$A572（代码 434 字节）
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
        jsr     L8E32_far
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
LA4E7:  jmp     LA88E_far
LA4EA:  jsr     L908C
        jsr     LA29B_far
        jsr     LA87D_far
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
LA52C:  jsr     LA57F_far
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
; ---- $A7BA-$A7CD（代码 20 字节）
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
; ---- $A86E-$A879（数据 12 字节）
LA86E:
        .addr   LA82F_far,LA84C_far,LA842_far,LA854_far,LA85C_far,LA866_far
; ---- $B1B5-$B2C2（代码 270 字节）
LB1B5:  jmp     L8BC8_far
LB1B8:  jsr     L97A6
        lda     $20
        cmp     #$BC
        bcs     LB1B5
        lda     #$0C
        jsr     L896A
        lda     #$00
        sta     $19
        lda     #$BC
        sta     $1A
        lda     #$24
        sta     $2006
        lda     #$00
        sta     $2006
        lda     $2007
        ldy     #$00
        ldx     #$04
LB1DF:  lda     $2007
        sta     ($19),y
        iny
        bne     LB1DF
        inc     $1A
        dex
        bne     LB1DF
        lda     $E5
        sta     $B005
        lda     #$FF
LB1F3:  sta     $B004
        lda     #$0B
        jsr     L896A
        rts
LB1FC:  lda     #$14
        jmp     LB237
LB201:  lda     $B004
        beq     LB1FC
LB206:  lda     #$0C
        jsr     L896A
        lda     #$BC
        sta     $1A
        lda     #$00
        sta     $19
        ldy     #$00
        ldx     #$04
        lda     #$24
        sta     $2006
        lda     #$00
        sta     $2006
LB221:  lda     ($19),y
        sta     $2007
        iny
        bne     LB221
        inc     $1A
        dex
        bne     LB221
        lda     $B005
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
        jsr     L893C_far
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
        jsr     L893C_far
        jsr     LB2C3_far
LB2BA:  jsr     L895D
        jsr     LBCD0
        jmp     L8284
; ---- $B2ED-$B2F6（代码 10 字节）
LB2ED:  lda     #$4F
        jsr     LB575
        lda     #$4B
        jmp     LB575
; ---- $CEBF-$CF7A（数据 188 字节）
LCEBF:
        .addr   L96C6_far,L9648_far,L86B0_far,L96A6_far,L9EC9,L8491_far,L8767_far,L818F_far
        .addr   L8491_far,L8491_far,LAFFC_far,L88C5_far,L97FC_far,L9887,L9716_far,L993C_far
        .addr   L9A6C_far,L8467_far,L9782_far,L9F36_far,L9C0C_far,L8467_far,L885B_far,L889C_far
        .addr   LAE0A_far,L97A4_far,L9AFD_far,L8491_far,LCBDC_far,L84B6_far,L975D_far,LAF4F_far
        .addr   LB153_far,LCBB3_far,L826A_far,LA01E,LBCD0,L9DE1_far,L9D64_far,LCB9B_far
        .addr   L9B68,LC4CA,LB098_far,LAED1,L9A0E_far,L970A_far,L9748_far,LAEF3_far
        .addr   LCBD9_far,L962B_far,L9573,LAD5B_far,LBFD1_far,L8BDA_far,L8758_far,L8C30_far
        .addr   LAEB8,L9625_far,LAE48_far,L818F_far,L9510,L9527_far,LB201_far,LB1B8_far
        .addr   LCC6C_far
LCF41:
        .addr   LA984,LAA28,LAAA6_far,LA91F,LAA1D,LA973,LA9BD,LA99C
        .addr   L8491_far,L8491_far,LAC81_far,LAD38,LAD47,LA958,LA961,LAADF
        .addr   LA933,LA8E6_far,LAA82_far,LAAD6,LAB58_far,LAC41_far,LABED_far,LAA3B_far
        .addr   LABAF,LCC04_far,LA90D_far,LA914_far,LA96A

        .segment "BANK3"
; ---- $82CC-$832C（代码 97 字节）
L82CC:  lda     $E2
        beq     L8325
        lda     $A0
        sta     $44
        lda     $A1
        sta     $45
        jsr     L8540_far
        bne     L82FD
        jsr     L9430
        lda     $5B
        sta     $09
        lda     $5C
        sta     $0A
        jsr     L87EE_far
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
        jsr     L90D6_far
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
        jmp     L8EF2_far
; ---- $85BE-$8649（代码 140 字节）
L85BE:  lda     $07
        clc
        adc     $5A
        sta     $28
        lda     $08
        adc     #$00
        sta     $29
        jsr     L8BB6
        jsr     L8540_far
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
; ---- $864A-$8655（代码 12 字节）
L864A:  lda     $0B
        beq     L8651
        dec     $0B
        rts
L8651:  dec     $0B
        dec     $0C
        rts
; ---- $8656-$8661（代码 12 字节）
L8656:  lda     $19
        beq     L865D
        dec     $19
        rts
L865D:  dec     $19
        dec     $1A
        rts
; ---- $8767-$87DF（代码 121 字节）
L8767:  jsr     L86E6_far
        lda     #$00
        sta     $51
L876E:  jsr     L9430
        lda     $86
        sta     $44
        lda     $87
        sta     $45
        jsr     L8540_far
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
        jsr     L8581_far
        jmp     L878A
L87BB:  jsr     L87EE_far
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
        jmp     LB945_far
L87DD:  jmp     L9439
; ---- $87E0-$87ED（代码 14 字节）
L87E0:  lda     #$3D
        clc
        adc     $46
        sta     $52
        lda     #$00
        sta     $53
        jmp     L893C
; ---- $883A-$8854（代码 27 字节）
L883A:  lda     #$05
        sta     $4016
        pha
        pla
        pha
        pla
        pha
        pla
        lda     #$04
        sta     $4016
        lda     #$06
        sta     $4016
        lda     $4017
        and     #$10
        rts
; ---- $8855-$887D（代码 41 字节）
L8855:  jsr     L883A
        beq     L885D
        rts
L885B:  pla
        pla
L885D:  lda     #$00
        sta     $63
        jsr     LB945_far
        bne     L885D
        jsr     LBE14
        jsr     L8883_far
        lda     #$FF
        sta     $24
        cmp     $0B
        bne     L887B
        cmp     $0C
        bne     L887B
        jsr     L88C0_far
L887B:  jmp     LB2D8
; ---- $88C5-$894A（代码 134 字节）
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
L8919:  jsr     L849C_far
        beq     L8939
        cmp     #$3B
        beq     L892B
        cmp     #$2C
        bne     L8933
        lda     #$09
        jsr     LB575
L892B:  jsr     L8499_far
        beq     L8938
        jmp     L8919
L8933:  jsr     L849C_far
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
; ---- $8BAD-$8C1F（代码 115 字节）
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
        jmp     LB237_far
L8BCD:  tsx
        cpx     #$14
        bcc     L8BC8
L8BD2:  rts
L8BD3:  lda     #$00
        sta     $E2
        jmp     L8491_far
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
        jsr     L849C_far
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
; ---- $8FD4-$8FE1（代码 14 字节）
L8FD4:  sec
        lda     $28
        sbc     $2C
        sta     $28
        lda     $29
        sbc     $2D
        sta     $29
        rts
; ---- $8FE2-$8FED（代码 12 字节）
L8FE2:  lda     $29
        cmp     $2D
        beq     L8FE9
        rts
L8FE9:  lda     $28
        cmp     $2C
        rts
; ---- $8FEE-$9077（代码 138 字节）
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
; ---- $913E-$917D（代码 64 字节）
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
; ---- $917E-$9197（代码 26 字节）
L917E:  jsr     L8EF7_far
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
; ---- $9198-$91B7（代码 32 字节）
L9198:  lda     #$00
        sta     $46
        ldy     #$20
        lda     $29
        bpl     L91A7
        jsr     L8E32_far
        ldy     #$2D
L91A7:  tya
        pha
        jsr     L9142
        jsr     L90D6_far
        dec     $46
        pla
        jsr     L9183
        dec     $46
        rts
; ---- $9904-$9919（代码 22 字节）
L9904:  lda     $83
        bmi     L9914
        jsr     L991A_far
        jsr     L8FEE
        bcs     L9912
        sec
        rts
L9912:  clc
        rts
L9914:  jsr     L991A_far
        jmp     L8FEE
; ---- $9C09-$9C6A（代码 98 字节）
L9C09:  jmp     L8491_far
L9C0C:  jsr     LA7C4_far
        bcs     L9C09
        sta     $7C
        lda     #$02
        sta     $84
        lda     #$00
        sta     $7D
        jsr     L9098
        jsr     LA7BC_far
        bcs     L9C28
        sta     $7D
        jsr     L908C
L9C28:  jsr     L90BF
        cmp     #$24
        bne     L9C34
        inc     $84
        jsr     L8499_far
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
        jsr     L849C_far
        cmp     #$29
        bne     L9C09
L9C56:  jsr     L908C
        jsr     L9C70_far
        jsr     L849C_far
        beq     L9C6A
        jsr     LAC14
        jsr     L90BF
        jmp     L9C0C
L9C6A:  rts
; ---- $A295-$A29A（代码 6 字节）
LA295:  jsr     LAA0D
        jmp     L908C
; ---- $A29B-$A2BD（代码 35 字节）
LA29B:  jsr     LA2BE
LA29E:  cmp     #$EF
        beq     LA2A3
        rts
LA2A3:  jsr     LA295
        jsr     LA2BE
        jsr     LA3C1_far
        lda     $28
        eor     $2C
        sta     $28
        lda     $29
        eor     $2D
        sta     $29
        jsr     LA7D8
        jmp     LA29E
; ---- $A2BE-$A2E0（代码 35 字节）
LA2BE:  jsr     LA2E1
LA2C1:  cmp     #$F0
        beq     LA2C6
        rts
LA2C6:  jsr     LA295
        jsr     LA2E1
        jsr     LA3C1_far
        lda     $28
        ora     $2C
        sta     $28
        lda     $29
        ora     $2D
        sta     $29
        jsr     LA7D8
        jmp     LA2C1
; ---- $A2E1-$A303（代码 35 字节）
LA2E1:  jsr     LA304_far
LA2E4:  cmp     #$F1
        beq     LA2E9
        rts
LA2E9:  jsr     LA295
        jsr     LA304_far
        jsr     LA3C1_far
        lda     $28
        and     $2C
        sta     $28
        lda     $29
        and     $2D
        sta     $29
        jsr     LA7D8
        jmp     LA2E4
; ---- $A7D5-$A7EE（代码 26 字节）
LA7D5:  jsr     L8EAC_far
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
; ---- $A7EF-$A7F4（代码 6 字节）
LA7EF:  jsr     L8E0F_far
        jmp     LA7D8
; ---- $A7F5-$A7FA（代码 6 字节）
LA7F5:  jsr     L8F0F_far
        jmp     LA7D8
; ---- $A7FB-$A80D（代码 19 字节）
LA7FB:  lda     $2C
        bne     LA803
        lda     $2D
        beq     LA809
LA803:  jsr     L8F9D
        jmp     LA7D8
LA809:  lda     #$0A
        jmp     LB237_far
; ---- $A80E-$A86D（代码 96 字节）
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
LA823:  jsr     L8E32_far
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
; ---- $A87A-$A8D3（代码 90 字节）
LA87A:  ldx     #$28
        .byte   $2C
LA87D:  ldx     #$29
        stx     $75
        jsr     L90BF
        cmp     $75
        bne     LA8D1
        jmp     L908C
LA88B:  jmp     L8494_far
LA88E:  jsr     L908C
        jsr     LAA04
        ldy     $28
        cpy     #$08
        bcs     LA88B
        lda     fb_page6+69,y
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
LA8D1:  jmp     L8491_far
; ---- $A8E6-$A90C（代码 39 字节）
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
; ---- $A90D-$A913（代码 7 字节）
LA90D:  lda     $B5
        sta     $28
        jmp     LA8EA
; ---- $A914-$A91E（代码 11 字节）
LA914:  lda     $B7
        sta     $28
        lda     $B8
        sta     $29
        jmp     LA8EE
; ---- $A941-$A94D（代码 13 字节）
LA941:  jsr     LAA04
LA944:  lda     $28
        jsr     LAF44
        asl     a
        asl     a
        asl     a
        tax
        rts
; ---- $A94E-$A957（代码 10 字节）
LA94E:  jsr     LAA04
        lda     $28
        jsr     LAF44
        tax
        rts
; ---- $AA38-$AA81（代码 74 字节）
LAA38:  jmp     L8494_far
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
LAA70:  jsr     LB4F4_far
        lda     #$05
        jsr     L896A
        lda     $73
LAA7A:  sta     $28
        jsr     LB65F
        jmp     LAA85
; ---- $AA82-$AA93（代码 18 字节）
LAA82:  jsr     LAA04
LAA85:  lda     $28
LAA87:  jsr     LAA94
LAA8A:  lda     #$03
        sta     $27
        jsr     LA87D
        jmp     LA8F8
; ---- $AA94-$AAA5（代码 18 字节）
LAA94:  ldx     $21
        sta     $0700,x
        inc     $21
        lda     #$01
        sta     $28
        stx     $29
        lda     #$07
        sta     $2A
        rts
; ---- $AAA6-$AAD5（代码 48 字节）
LAAA6:  jsr     LAA0A
        jsr     L9198
LAAAC:  jsr     L90D6_far
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
; ---- $AB58-$AB7F（代码 40 字节）
LAB58:  jsr     L849C_far
        beq     LAB6A
        cmp     #$28
        bne     LAB6A
        jsr     LA8B6
        jsr     LBC20_far
        jmp     LAA87
LAB6A:  jsr     LB945_far
        beq     LAB7A
        jsr     LAA94
LAB72:  jsr     LA8EE
        lda     #$03
        sta     $27
        rts
LAB7A:  jsr     LA8AD
        jmp     LAB72
; ---- $AB8D-$ABAB（代码 31 字节）
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
; ---- $ABED-$AC7D（代码 145 字节）
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
LAC11:  jmp     L8491_far
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
; ---- $AC7E-$AD01（代码 132 字节）
LAC7E:  jmp     L8494_far
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
; ---- $AD58-$AD95（代码 62 字节）
LAD58:  jmp     L8494_far
LAD5B:  cmp     #$2C
        beq     LAD8A
        jsr     LA26E
        cmp     #$04
        bcs     LAD58
        sta     $F3
        jsr     LADFE
        jsr     L849C_far
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
        jmp     L86B0_far
LAD8A:  jsr     LAC14
        jsr     LA26E
        beq     LAD95
        jsr     LADD1
LAD95:  rts
; ---- $ADD1-$ADFD（代码 45 字节）
LADD1:  lda     #<LD000
        sta     $19
        lda     #>LD000
        sta     $1A
        jsr     LB559
        lda     #$24
        sta     $2006
        lda     #$00
        sta     $2006
        ldy     #$00
LADE8:  lda     ($19),y
        sta     $2007
        iny
        bne     LADE8
        inc     $1A
        lda     #>(LD000+1024)
        cmp     $1A
        bne     LADE8
        jsr     LB53C
        jmp     LB562
; ---- $AEF3-$AF92（代码 160 字节）
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
LAF49:  jmp     L8494_far
LAF4C:  jmp     L8491_far
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
        jsr     L849C_far
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
; ---- $AF93-$AFC8（代码 54 字节）
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
; ---- $AFC9-$B193（代码 459 字节）
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
        jmp     L8494_far
        jmp     L8491_far
LAFFC:  cmp     #$9A
        beq     LAFC9
        cmp     #$9B
        beq     LAFD5
        jsr     LA26E
        jsr     LAF44
        pha
        jsr     L849C_far
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
        lda     fb_page6+61,x
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
LB032:  lda     fb_page6+128,x
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
LB08A:  jmp     L8491_far
LB08D:  cmp     #$01
        beq     LB082
        and     #$FF
        beq     LB082
LB095:  jmp     L8494_far
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
        jsr     L86D9_far
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
        sta     fb_page6+128,x
        inx
        iny
        cpy     #$04
        beq     LB14B
        dec     $0400
        bne     LB130
LB140:  lda     #$20
LB142:  sta     fb_page6+128,x
        inx
        iny
        cpy     #$04
        bne     LB142
LB14B:  ldx     $6D
        lda     $AD
        sta     fb_page6+61,x
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
        lda     $74
        jsr     L81EC
        pla
        sta     $2007
        inx
        bne     LB162
        inc     $74
        lda     $74
        cmp     #$24
        bne     LB162
        lda     $E5
        sta     $E4
        jsr     LB53C
        jsr     LBE14
        lda     #$0B
        jmp     L896A
; ---- $B2C3-$B2D7（代码 21 字节）
LB2C3:  lda     $0B
        sta     $11
        sta     $28
        lda     $0C
        sta     $12
        sta     $29
LB2CF:  jsr     L913E
        jsr     L90D6_far
        jmp     L87E0
; ---- $B2D8-$B2EC（代码 21 字节）
LB2D8:  jsr     LB56B
        lda     #$00
        sta     $62
        lda     #<LB35A
        sta     $52
        lda     #>LB35A
        sta     $53
        jsr     L893C
        jmp     LB2A2_far
; ---- $B436-$B449（代码 20 字节）
LB436:  lda     #<LCFBE
        sta     $52
        lda     #>LCFBE
        sta     $53
        lda     #$00
        sta     $65
        jsr     LAF93
        inc     $65
        jmp     LAF93
; ---- $CA55-$CB74（代码 288 字节）
LCA55:  jmp     L8494_far
LCA58:  jmp     L8491_far
LCA5B:  jsr     L908C
        jsr     LA87A
        jsr     LA26E
        jsr     LAF44
        jsr     LA87D
        lda     $0400
        tax
        lda     #$E7
        and     fb_page6+69,x
        sta     fb_page6+69,x
        txa
        sta     $A4
        asl     a
        asl     a
        asl     a
        sta     $A5
        jsr     L86D9_far
        jsr     L90BF
        cmp     #$8A
        bne     LCA58
        jsr     L908C
        jsr     LA87A
        jsr     LCB7D
        cmp     #$10
        bcs     LCA55
        sta     fb_page6+193,x
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
        sta     fb_page6+192,x
        lda     fb_page6+193,x
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
        lda     fb_page6+192,x
        tay
        lda     ($1B),y
        and     #$F0
        ldy     $A4
        pha
        lda     fb_page6+69,y
        and     #$3F
        sta     fb_page6+69,y
        pla
        ora     fb_page6+69,y
        sta     fb_page6+69,y
        jsr     LCB84
        bcs     LCB67
        jsr     LCB75
        bcs     LCB06
        sta     fb_page6+194,x
        sta     fb_page6+196,x
        jsr     LCB84
        bcs     LCB67
LCB06:  jsr     LCB75
        bcs     LCB18
        adc     #$01
        sta     fb_page6+195,x
        sta     fb_page6+197,x
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
        lda     fb_page6+69,x
        and     #$DF
        sta     fb_page6+69,x
        tya
        ora     fb_page6+69,x
        sta     fb_page6+69,x
        jsr     LCB84
        bcs     LCB67
LCB41:  jsr     LCB75
        cmp     #$04
        bcs     LCB72
        ldx     $A4
        pha
        lda     fb_page6+69,x
        and     #$FC
        sta     fb_page6+69,x
        pla
        ora     fb_page6+69,x
        sta     fb_page6+69,x
        jsr     LA87D
        lda     #$78
        ldx     $A5
        sta     fb_page6+198,x
        sta     fb_page6+199,x
LCB67:  ldx     $A4
        lda     fb_page6+69,x
        ora     #$08
        sta     fb_page6+69,x
        rts
LCB72:  jmp     L8494_far
; ---- $CB75-$CB9A（代码 38 字节）
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
; ---- $CB9B-$CBB2（代码 24 字节）
LCB9B:  jsr     LA26E
        jsr     LAF44
        asl     a
        asl     a
        asl     a
        sta     $A5
        jsr     LCB75
        sta     fb_page6+198,x
        jsr     LCB75
        sta     fb_page6+199,x
        rts
; ---- $CBB3-$CC96（代码 228 字节）
LCBB3:  jsr     LA26E
        jsr     LAF44
        tax
        lda     fb_page6+69,x
        and     #$08
        beq     LCBC9
        lda     #$10
        ora     fb_page6+69,x
        sta     fb_page6+69,x
LCBC9:  jsr     L849C_far
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
        lda     fb_page6+69,y
        and     #$EF
        sta     fb_page6+69,y
        lda     $75
        beq     LCBF6
        jsr     LCC97
LCBF6:  jsr     L849C_far
        beq     LCBD8
        jsr     LAC14
        lda     $75
        beq     LCBDC
        bne     LCBD9
LCC04:  jsr     LA94E
        lda     $28
        sta     $74
        lda     fb_page6+69,x
        and     #$08
        beq     LCC55
        jsr     LA944
        lda     fb_page6+198,x
        sta     $6B
        lda     fb_page6+199,x
        sta     $6C
        lda     #$00
        sta     $28
        ldy     #$08
LCC25:  lda     $74
        cmp     $28
        beq     LCC47
        jsr     LA944
        lda     fb_page6+198,x
        sec
        sbc     $6B
        jsr     LCC5D
        bcs     LCC47
        lda     fb_page6+199,x
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
        sta     fb_page6+69,y
        jsr     LCC97
        pla
        asl     a
        asl     a
        asl     a
        tax
        lda     #$00
        sta     fb_page6+192,x
        sta     fb_page6+198,x
        sta     fb_page6+199,x
        jsr     L849C_far
        beq     LCC6B
        jsr     LAC14
        jmp     LCC6C
; ---- $CC97-$CCAA（代码 20 字节）
LCC97:  tya
        jsr     LAD53
        tay
        lda     #$F8
        sta     $0280,y
        sta     $0284,y
        sta     $0288,y
        sta     $028C,y
        rts
; ---- $CFA4-$CFAD（数据 10 字节）
LCFA4:
        .addr   LCFAE,LCFBE
LCFA8:
        .addr   LCFCE,LCFDE,LCFEE

        .segment "BANK4"
; ---- $8000-$8002（代码 3 字节）
        jmp     L80BA
; ---- $80BA-$81EB（代码 306 字节）
L80BA:  lda     #$00
        sta     $2000
        sta     $2001
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
        lda     $B001
        cmp     #$4C
        bne     L8180
        jsr     LB4A7
        lda     $B004
        sta     $7C
        lda     $B002
        sta     $07
        lda     $B003
        sta     $08
        lda     #$06
        sta     $05
        lda     #$B0
        sta     $06
        lda     #$00
        tax
L814F:  sta     $B000,x
        inx
        cpx     #$05
        bne     L814F
        jsr     L97A6_far
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
        jsr     LB206_far
L8178:  jmp     L826A_far
L817B:
        .byte   "[BG]", $00
L8180:  jsr     LB4A7
        lda     #$59
        sta     $9E
        lda     #$00
        sta     $B004
        jmp     L825E_far
L818F:  lda     $33
        and     #$EF
        sta     $33
        sta     $2001
        lda     #$4C
        sta     $B001
        lda     $07
        sta     $B002
        lda     $08
        sta     $B003
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
; ---- $81F3-$820C（代码 26 字节）
L81F3:  lda     $B000
        pha
        lda     #$19
        sta     $B000
        cmp     $B000
        beq     L8207
        pla
        sta     $B000
        sec
        rts
L8207:  pla
        sta     $B000
        clc
        rts
; ---- $8A0C-$8B73（代码 360 字节）
L8A0C:  jsr     LB4F4
        ldx     $64
        jsr     LB52E
L8A14:  lda     $0700,x
        beq     L8A7F
        sta     $2007
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
        sta     $2000
        jsr     LB4F4
        jsr     LB42B
        jsr     LB52E
        ldx     #$1C
        lda     #$20
L8A6B:  sta     $2007
        dex
        bne     L8A6B
        stx     $63
        jsr     LB53C
        jsr     LB5D4
        lda     #$01
        sta     $63
        rts
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
        rts
L8AC3:  jsr     LB52E
        lda     $2007
        ldx     #$00
        ldy     $48
L8ACD:  lda     $2007
        sta     $0500,x
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
        sta     $2007
        inx
        iny
        cpy     #$1C
        bne     L8AE4
        beq     L8ABA
L8AF2:  lda     #$3F
        sta     $2006
        lda     $0700
        asl     a
        asl     a
L8AFC:  sta     $2006
        lda     $0701
        sta     $2007
        lda     $0702
        sta     $2007
        lda     $0703
        sta     $2007
        lda     $0704
        sta     $2007
        lda     #$3F
        sta     $2006
        lda     #$00
        sta     $2006
        sta     $2006
        sta     $2006
        jsr     LB53C
        jsr     LBE14
        jmp     L89C9
L8B30:  lda     #$3F
        sta     $2006
        lda     $0700
        asl     a
        asl     a
        ora     #$10
        bne     L8AFC
L8B3E:  jsr     LB52E
        ldy     $48
        ldx     #$01
        lda     $B0
        sta     $0500
        lda     $2007
L8B4D:  lda     $2007
        sta     $0500,x
        inx
        iny
        cpy     #$1C
        bne     L8B4D
        sta     $B0
        jsr     LB52E
        ldx     #$00
        ldy     $48
L8B62:  lda     $0500,x
        sta     $2007
        inx
        iny
        cpy     #$1C
        bne     L8B62
L8B6E:  jsr     LB53C
        jmp     L89C9
; ---- $AE05-$AE92（代码 142 字节）
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
LAE45:  jmp     L8494_far
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
        jsr     L849C_far
        beq     LAE41
        jsr     LAC14_far
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
; ---- $AEE8-$AEF2（代码 11 字节）
LAEE8:  ldx     #$00
        lda     #$EF
LAEEC:  sta     $0200,x
        inx
        bne     LAEEC
        rts
; ---- $B3AB-$B435（代码 139 字节）
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
        jsr     LB436_far
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
; ---- $B44A-$B484（数据 59 字节）
LB44A:
        .byte   "GAME 0", $0D, $00
        .byte   "GAME 1", $0D, $00
        .byte   "GAME 2,1", $0D, $00
        .byte   "GAME 3", $0D, $00
        .byte   "SPRITE ", $00
        .byte   "LOAD", $0D, $00
        .byte   "LIST", $0D, $00
        .byte   "RUN", $0D, $00
; ---- $B4A7-$B4F3（代码 77 字节）
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
LB4E2:  sta     fb_page6+26,x
        inx
        cpx     #$1A
        bne     LB4E2
        rts
LB4EB:  sta     fb_page6,x
        inx
        cpx     #$1A
        bne     LB4EB
        rts
; ---- $B4F4-$B52D（代码 58 字节）
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
        lsr     a
        clc
        adc     $49
        adc     #$03
        cmp     #$1E
        bcc     LB516
        sbc     #$1E
LB516:  sta     $2C
        jsr     L8F47_far
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
; ---- $B5AD-$B636（代码 138 字节）
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
        sta     $2000
        lda     $E4
        ldy     $4F
        cpy     #$20
        beq     LB5E3
        lda     $E5
LB5E3:  clc
        adc     #$08
        cmp     #$F0
        bcc     LB5EC
        lda     #$00
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
LB609:  lda     fb_page6+28,x
        sta     fb_page6+27,x
        inx
        dey
        bne     LB609
        tya
        sty     fb_page6+50
        sty     fb_page6+51
        beq     LB62E
LB61C:  lda     fb_page6+2,x
        sta     fb_page6+1,x
        inx
        dey
        bne     LB61C
        lda     #$00
        sta     fb_page6+24
        sta     fb_page6+25
LB62E:  sta     $48
        lda     #$17
        sta     $49
        jmp     LBE14
; ---- $B645-$B655（代码 17 字节）
LB645:  pha
        lda     $4F
        cmp     #$20
        beq     LB651
        pla
        sta     fb_page6+27,x
        rts
LB651:  pla
        sta     fb_page6+1,x
        rts
; ---- $B68F-$B6D1（代码 67 字节）
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
; ---- $B75E-$B76E（代码 17 字节）
LB75E:  lda     #$FF
        sta     $72
        lda     $E1
        bne     LB76C
LB766:  lda     #$00
        sta     $72
        lda     $E0
LB76C:  sta     $61
        rts
; ---- $B76F-$B7D4（代码 102 字节）
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
; ---- $B7D5-$B804（代码 48 字节）
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
; ---- $B805-$B8BD（代码 185 字节）
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
; ---- $B8FE-$B957（代码 90 字节）
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
; ---- $B958-$BAA8（代码 337 字节）
LB958:  lda     #$05
        sta     $4016
        nop
        nop
        nop
        nop
        nop
        nop
        ldx     #$00
LB965:  lda     #$04
        sta     $4016
        ldy     #$0A
LB96C:  dey
        bne     LB96C
        nop
        nop
        lda     $4017
        lsr     a
        and     #$0F
        sta     fb_page6+52,x
        lda     #$06
        sta     $4016
        ldy     #$0A
LB981:  dey
        bne     LB981
        nop
        nop
        lda     $4017
        rol     a
        rol     a
        rol     a
        and     #$F0
        ora     fb_page6+52,x
        eor     #$FF
        ldy     #$08
LB995:  asl     a
        ror     fb_page6+52,x
        dey
        bne     LB995
        inx
        cpx     #$09
        bne     LB965
        lda     #$00
        sta     $6F
        sta     $70
        sta     $71
        lda     fb_page6+59
        and     #$10
        bne     LB9DB
        lda     fb_page6+59
        and     #$08
        bne     LB9FB
        lda     fb_page6+52
        and     #$04
        bne     LBA03
        lda     fb_page6+59
        and     #$04
        bne     LBA14
        jsr     LB9E9
        bne     LB9CD
        sta     $18
        rts
LB9CD:  ldy     $18
        bne     LB9D8
        ldy     $50
        bne     LB9D8
        and     #$FF
        rts
LB9D8:  lda     #$00
        rts
LB9DB:  lda     #$FF
        sta     $6F
        sta     $18
LB9E1:  lda     fb_page6+59
        and     #$E3
        sta     fb_page6+59
LB9E9:  ldx     #$00
        ldy     #$09
LB9ED:  lda     fb_page6+52,x
        bne     LBA1D
        inx
        dey
        bne     LB9ED
        sta     $6D
        sta     $50
        rts
LB9FB:  lda     #$FF
        sta     $18
        sta     $70
        bne     LB9E1
LBA03:  lda     #$FF
        sta     $18
        sta     $70
        lda     fb_page6+52
        and     #$FB
        sta     fb_page6+52
        jmp     LB9E1
LBA14:  lda     #$FF
        sta     $71
        sta     $18
        jmp     LB9E1
LBA1D:  stx     $6E
        ldy     #$00
        clc
LBA22:  lsr     a
        bcs     LBA29
        iny
        jmp     LBA22
LBA29:  txa
        asl     a
        asl     a
        asl     a
        sty     $6D
        clc
        adc     $6D
        tax
        lda     $6F
        bne     LBA59
        lda     $70
        bne     LBA6F
        lda     $71
        bne     LBA77
        lda     $72
        bne     LBA80
        lda     LBAA9,x
        cmp     #$FF
        beq     LBA4D
        and     #$FF
        rts
LBA4D:  cmp     $50
        beq     LBA56
        sta     $50
        jsr     LB75E
LBA56:  lda     #$00
        rts
LBA59:  lda     LBAA9,x
        cmp     #$20
        bcs     LBA63
        and     #$FF
        rts
LBA63:  sec
        sbc     #$40
        bcc     LBA56
        cmp     #$20
        bcs     LBA56
LBA6C:  and     #$FF
        rts
LBA6F:  lda     $72
        bne     LBA86
        lda     LBAF1,x
        rts
LBA77:  lda     $72
        bne     LBA7C
        rts
LBA7C:  lda     LBBC9,x
        rts
LBA80:  lda     LBB39,x
        jmp     LBA89
LBA86:  lda     LBB81,x
LBA89:  cmp     #$FF
        bne     LBA6C
        cmp     $50
        beq     LBA56
        sta     $50
        jsr     LB766
        lda     #$00
LBA98:  rts
LBA99:  ldy     #$08
        ldx     #$00
LBA9D:  lda     fb_page6+52,x
        asl     a
        bcs     LBA98
        inx
        dey
        bne     LBA9D
        clc
        rts
; ---- $BC11-$BC1F（代码 15 字节）
LBC11:  jsr     LB4F4
        lda     #$08
        bne     LBC1D
LBC18:  jsr     LB4F4
        lda     #$09
LBC1D:  jmp     L896A
; ---- $BC20-$BCCF（代码 176 字节）
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
; ---- $BD04-$BDCD（代码 202 字节）
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
; ---- $BFCE-$C019（代码 76 字节）
LBFCE:  jsr     LB4A7
LBFD1:  jsr     LB436_far
        ldy     #$08
        ldx     #$00
        txa
LBFD9:  sta     fb_page6+69,x
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
        jsr     LAFC9_far
        lda     #$00
        sta     $0505
        sta     $0506
        sta     $88
        sta     $0504
        jmp     LC2E8
; ---- $C01A-$C024（代码 11 字节）
LC01A:  lda     $0505
        sta     $48
        lda     $0506
        sta     $49
        rts
; ---- $C025-$C02E（代码 10 字节）
LC025:  sta     $E0
        sta     $E1
        jsr     LB766
        jmp     LBC20
; ---- $C02F-$C0F2（代码 196 字节）
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
LC079:  jsr     LAFD5_far
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
        jsr     LB436_far
        jsr     LB4AB
        jsr     LB194
        jsr     LAEE8
        lda     #$10
        sta     $32
        lda     #$00
        sta     $F0
        jsr     LBE14
        jmp     L826A_far
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
; ---- $C0F3-$C0FE（数据 12 字节）
LC0F3:
        .addr   LC2E5_far,LC5BE_far,LC5BB_far,LBFCE_far,LC697_far,LC17D_far
; ---- $C0FF-$C179（代码 123 字节）
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
        jmp     LC141
LC13B:  lda     $0520,x
        sta     $2007
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
; ---- $C17A-$C1BF（代码 70 字节）
LC17A:  jmp     LC079
LC17D:  jsr     LB420
        jsr     LC108
        lda     #$05
        jsr     LC276
        jsr     LAFD5_far
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
; ---- $C2A2-$C2AE（数据 13 字节）
LC2A2:
        .byte   $00,$08,$10,$18,$B8,$C0,$C8,$D0,$D8,$E0,$E8,$F0,$F8
; ---- $C2E5-$C334（代码 80 字节）
LC2E5:  jsr     LC108
LC2E8:  jsr     LAFC9_far
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
; ---- $C335-$C33C（数据 8 字节）
LC335:
        .byte   $1B,$0D,$0C,$0B,$08,$12,$20,$44
; ---- $C33D-$C34C（数据 16 字节）
LC33D:
        .addr   LC37D,LC2AF,LC394_far,LC39D_far,LC3A7_far,LC3B5_far,LC3C3_far,LC2D4
; ---- $C394-$C451（代码 190 字节）
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
        sta     $2007
        inx
        lda     #$20
        sta     $2007
        inx
        dey
        bne     LC43C
        jsr     LB53C
        jmp     LB65F
; ---- $C5AF-$C648（代码 154 字节）
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
        jsr     LAFC9_far
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
; ---- $C697-$C6DE（代码 72 字节）
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
        jsr     LC50D_far
LC6D0:  jmp     LBFD1
LC6D3:  jsr     LB575
        jsr     LC6DF
        jsr     LC565_far
        jmp     LC6D0

        .segment "BANK5"
; ---- $8013-$80A2（数据 144 字节）
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
; ---- $80A3-$80B9（数据 23 字节）
L80A3:
        .byte   $01,$02,$03,$04,$06,$08,$0C,$10,$18,$20
L80AD:
        .byte   $12,$16,$00,$04,$08,$0A,$0E
L80B4:
        .byte   $00,$18,$30,$48,$60,$78
; ---- $8971-$89A2（代码 50 字节）
L8971:
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
        rts
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
; ---- $89A3-$89C0（数据 30 字节）
L89A3:
        .addr   L8A0C_far,L8A87_far,L8AC3_far,L89FA,L89D2,L8AF2_far,L8B30_far,L89E4
        .addr   L89F2,LA091_far,L89C4,L89C1,L8B3E_far,L8B74,L8B8C
; ---- $90B7-$90BB（代码 5 字节）
        ldy     #$00
        sta     ($09),y
        rts
; ---- $A091-$A0A2（代码 18 字节）
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
        rts
; ---- $A0A3-$A20E（代码 364 字节）
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
; ---- $A20F-$A240（代码 50 字节）
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
        jsr     L8F47_far
        lda     $28
        ldx     $CA
        sta     $CB,x
        lda     $91
        sta     $C6,x
        rts
; ---- $A8D4-$A8E5（代码 18 字节）
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
; ---- $ABAC-$ABAE（代码 3 字节）
        jmp     L8491_far
; ---- $B6D2-$B6DB（代码 10 字节）
        ldx     $49
        dex
        txa
        tax
        inx
        jsr     LB637
        rts
; ---- $B6DC-$B6DE（代码 3 字节）
        jmp     LB56F
; ---- $C729-$C738（数据 16 字节）
LC729:
        .byte   $00,$1C,$00,$00,$00,$00,$38,$58,$00,$0C,$78,$20,$00,$48,$88,$58
; ---- $C8A3-$C8B3（代码 17 字节）
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
        rts
; ---- $C8B4-$CA54（代码 417 字节）
LC8B4:  ldx     #$00
        stx     $A9
        lda     #$08
        sta     $5D
LC8BC:  ldx     $A9
        lda     fb_page6+69,x
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
LC8D8:  lda     fb_page6+197,x
        sta     fb_page6+195,x
        ldx     $A9
        lda     fb_page6+69,x
        and     #$EF
        sta     fb_page6+69,x
        jmp     LC8CA
LC8EB:  lda     $A9
        asl     a
        asl     a
        asl     a
        sta     $A6
        tax
        dec     fb_page6+194,x
        bne     LC8CA
        lda     fb_page6+196,x
        sta     fb_page6+194,x
        dec     fb_page6+195,x
        beq     LC8D8
        lda     fb_page6+193,x
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
        lda     fb_page6+192,x
        asl     a
        tay
        lda     LC787,y
        clc
        adc     fb_page6+198,x
        sta     fb_page6+198,x
        clc
        lda     LC788,y
        adc     fb_page6+199,x
        sta     fb_page6+199,x
        ldy     #$00
        lda     ($A7),y
        sta     $D5
        iny
        lda     ($A7),y
        sta     $D6
        lda     fb_page6+192,x
        tay
        lda     ($D5),y
        and     #$0F
        jsr     LAD53
        and     #$FF
        sta     $D7
        beq     LC970
        lda     fb_page6+193,x
        and     #$F0
        clc
        adc     #$10
        cmp     $D7
        bne     LC95E
        lda     #$00
LC95E:  pha
        lda     #$0F
        and     fb_page6+193,x
        sta     fb_page6+193,x
        pla
        pha
        ora     fb_page6+193,x
        sta     fb_page6+193,x
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
        lda     fb_page6+192,x
        asl     a
        tay
        lda     ($D9),y
        sta     $DB
        iny
        lda     ($D9),y
        sta     $DC
        lda     fb_page6+192,x
        cmp     #$01
        beq     LC998
        cmp     #$05
        bne     LC9E8
LC998:  lda     fb_page6+193,x
        and     #$0F
        beq     LC9C2
        cmp     #$01
        beq     LC9C2
        cmp     #$04
        bne     LC9E8
        lda     $D7
        beq     LC9DE
        lda     fb_page6+192,x
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
        ora     fb_page6+69,y
        sta     fb_page6+69,y
        lda     #$00
        sta     $D7
        beq     LC9E8
LC9DE:  ldy     $A9
        lda     fb_page6+69,y
        and     #$3F
        sta     fb_page6+69,y
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
        lda     fb_page6+69,y
        sta     $0282,x
        sta     $0286,x
        sta     $028A,x
        sta     $028E,x
        ldy     $A6
        lda     fb_page6+198,y
        sta     $0283,x
        sta     $028B,x
        clc
        adc     #$08
        sta     $0287,x
        sta     $028F,x
        lda     fb_page6+199,y
        sta     $0280,x
        sta     $0284,x
        clc
        adc     #$08
        sta     $0288,x
        sta     $028C,x
        jmp     LC8CA

        .segment "BANK6"
; ---- $D000-$D3FF（数据 1024 字节）
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
; ---- $D400-$DBFD（数据 2046 字节）
LD400:
        .byte   $1D,$0A,$00,$A3,$22,$4D,$31,$56,$39,$59,$32,$54,$33,$3A,$4D,$31
        .byte   $56,$37,$59,$31,$54,$33,$3A,$4D,$31,$54,$33,$22,$00,$18,$14,$00
        .byte   $A3,$22,$4F,$33,$45,$35,$47,$3A,$4F,$33,$43,$35,$43,$3A,$4F,$32
        .byte   $47,$35,$47,$22,$00,$18,$1E,$00,$A3,$22,$4F,$34,$43,$4F,$33,$42
        .byte   $34,$41,$31,$3A,$43,$43,$3A,$23,$46,$23,$46,$22,$00,$19,$28,$00
        .byte   $A3,$22,$47,$34,$41,$31,$3A,$46,$34,$46,$31,$3A,$4F,$33,$44,$34
        .byte   $4F,$32,$42,$31,$22,$00,$17,$32,$00,$A3,$22,$42,$34,$4F,$34,$43
        .byte   $37,$3A,$46,$34,$45,$37,$3A,$47,$34,$43,$37,$22,$00,$40,$3C,$00
        .byte   $98,$3A,$AE,$20,$09,$2C,$12,$0A,$00,$3A,$51,$F6,$CF,$28,$11,$16
        .byte   $40,$29,$20,$F1,$20,$11,$04,$00,$3A,$8F,$22,$50,$4C,$41,$59,$20
        .byte   $3F,$20,$59,$2F,$4E,$3D,$3E,$22,$2C,$59,$24,$3A,$92,$20,$59,$24
        .byte   $F6,$22,$4E,$22,$20,$85,$20,$51,$F6,$12,$E7,$03,$00,$16,$46,$00
        .byte   $98,$3A,$B0,$01,$2C,$02,$2C,$03,$2C,$04,$2C,$05,$2C,$06,$2C,$07
        .byte   $2C,$08,$00,$12,$50,$00,$8A,$20,$9A,$3A,$9F,$01,$2C,$01,$3A,$55
        .byte   $F6,$11,$15,$00,$00,$16,$5A,$00,$AF,$42,$20,$01,$2C,$12,$0F,$00
        .byte   $2C,$12,$30,$00,$2C,$12,$30,$00,$2C,$55,$00,$1D,$64,$00,$41,$24
        .byte   $F6,$22,$41,$49,$49,$49,$49,$49,$49,$49,$49,$49,$49,$49,$49,$47
        .byte   $49,$47,$47,$47,$47,$48,$22,$00,$15,$6E,$00,$42,$24,$F6,$22,$48
        .byte   $48,$42,$48,$42,$42,$42,$45,$42,$45,$45,$45,$22,$00,$12,$78,$00
        .byte   $4B,$24,$F6,$22,$41,$42,$43,$44,$45,$46,$47,$48,$49,$22,$00,$12
        .byte   $82,$00,$49,$24,$F6,$22,$41,$42,$43,$47,$48,$49,$44,$45,$46,$22
        .byte   $00,$1F,$8C,$00,$98,$3A,$AE,$0A,$2C,$12,$0A,$00,$3A,$8B,$22,$53
        .byte   $54,$41,$52,$54,$20,$21,$21,$22,$3A,$8E,$12,$50,$00,$3A,$98,$00
        .byte   $13,$96,$00,$58,$F6,$12,$0D,$00,$3A,$59,$F6,$12,$16,$00,$3A,$49
        .byte   $F6,$01,$00,$14,$A0,$00,$81,$0B,$DC,$00,$3A,$AC,$41,$24,$2C,$42
        .byte   $24,$3A,$81,$0B,$DC,$00,$00,$19,$AA,$00,$92,$20,$49,$F6,$01,$85
        .byte   $AC,$4B,$24,$2C,$49,$24,$3A,$49,$F6,$02,$3A,$80,$0B,$A0,$00,$00
        .byte   $0D,$B4,$00,$81,$0B,$36,$01,$3A,$81,$0B,$AE,$01,$00,$0F,$BE,$00
        .byte   $92,$20,$51,$F3,$12,$E7,$03,$85,$0B,$3C,$00,$00,$08,$C8,$00,$80
        .byte   $0B,$46,$00,$00,$05,$D2,$00,$A2,$00,$12,$DC,$00,$4C,$F6,$CE,$28
        .byte   $41,$24,$29,$3A,$8C,$50,$F6,$02,$88,$4C,$00,$1A,$E6,$00,$92,$28
        .byte   $CF,$28,$11,$16,$40,$29,$20,$F1,$20,$11,$04,$00,$29,$F6,$51,$20
        .byte   $85,$0B,$E6,$00,$00,$07,$F0,$00,$4B,$F6,$50,$00,$0D,$FA,$00,$92
        .byte   $20,$49,$F6,$01,$85,$0B,$0E,$01,$00,$0B,$04,$01,$4B,$F6,$4C,$F9
        .byte   $02,$FA,$50,$00,$17,$0E,$01,$43,$F6,$E2,$28,$4B,$24,$2C,$D4,$28
        .byte   $41,$24,$2C,$4B,$2C,$02,$29,$29,$FA,$02,$00,$10,$18,$01,$56,$F6
        .byte   $43,$FC,$04,$3A,$48,$F6,$43,$20,$FB,$04,$00,$1F,$22,$01,$58,$F6
        .byte   $58,$F9,$48,$F9,$04,$FD,$28,$48,$F6,$03,$29,$3A,$59,$F6,$59,$F9
        .byte   $56,$F9,$04,$FD,$28,$56,$F6,$03,$29,$00,$12,$2C,$01,$AE,$58,$2C
        .byte   $59,$3A,$8B,$22,$CF,$22,$3B,$3A,$8D,$3A,$83,$00,$17,$36,$01,$59
        .byte   $F6,$12,$16,$00,$3A,$53,$F6,$01,$3A,$45,$F6,$12,$0D,$00,$3A,$54
        .byte   $F6,$01,$00,$17,$40,$01,$AE,$12,$12,$00,$2C,$12,$14,$00,$3A,$8B
        .byte   $22,$53,$43,$4F,$52,$45,$3A,$22,$3B,$00,$1A,$4A,$01,$92,$28,$CF
        .byte   $28,$11,$16,$40,$29,$20,$F1,$20,$11,$04,$00,$29,$F6,$51,$20,$85
        .byte   $0B,$4A,$01,$00,$15,$54,$01,$54,$F6,$54,$F9,$02,$3A,$AE,$12,$18
        .byte   $00,$2C,$12,$14,$00,$3A,$8B,$54,$00,$2C,$5E,$01,$58,$F6,$58,$F9
        .byte   $02,$3A,$92,$E1,$28,$58,$2C,$59,$29,$F6,$22,$20,$22,$F1,$20,$58
        .byte   $F5,$45,$20,$85,$20,$AE,$20,$58,$2C,$59,$3A,$8B,$22,$CF,$22,$3A
        .byte   $80,$0B,$4A,$01,$00,$12,$68,$01,$59,$F6,$59,$FA,$02,$3A,$92,$59
        .byte   $F8,$02,$85,$0B,$86,$01,$00,$12,$72,$01,$81,$0B,$9A,$01,$3A,$92
        .byte   $58,$F6,$FA,$02,$85,$0B,$68,$01,$00,$08,$7C,$01,$80,$0B,$5E,$01
        .byte   $00,$24,$86,$01,$92,$53,$F6,$01,$85,$53,$F6,$12,$0D,$00,$3A,$45
        .byte   $F6,$12,$1A,$00,$3A,$59,$F6,$12,$14,$00,$3A,$81,$0B,$9A,$01,$3A
        .byte   $80,$0B,$4A,$01,$00,$05,$90,$01,$83,$00,$33,$9A,$01,$58,$F6,$FA
        .byte   $02,$3A,$8C,$43,$F6,$53,$20,$88,$45,$3A,$92,$E1,$28,$43,$2C,$59
        .byte   $29,$F6,$22,$CF,$22,$F1,$20,$E1,$28,$43,$F9,$02,$2C,$59,$29,$F6
        .byte   $22,$20,$22,$85,$20,$58,$F6,$43,$3A,$43,$F6,$45,$00,$07,$A4,$01
        .byte   $8D,$3A,$83,$00,$38,$AE,$01,$57,$F6,$11,$34,$00,$3A,$8C,$4E,$F6
        .byte   $01,$88,$02,$3A,$AA,$A1,$28,$4E,$29,$F6,$8A,$28,$4E,$2C,$04,$F9
        .byte   $05,$FD,$4E,$2C,$02,$2C,$02,$2C,$01,$2C,$01,$29,$3A,$A7,$4E,$2C
        .byte   $12,$EC,$00,$FD,$4E,$2C,$12,$C8,$00,$3A,$8D,$00,$1A,$B8,$01,$92
        .byte   $28,$CF,$28,$11,$16,$40,$29,$20,$F1,$20,$11,$04,$00,$29,$F6,$51
        .byte   $20,$85,$0B,$B8,$01,$00,$15,$C2,$01,$54,$F6,$54,$F9,$02,$3A,$AE
        .byte   $12,$18,$00,$2C,$12,$14,$00,$3A,$8B,$54,$00,$14,$CC,$01,$A1,$01
        .byte   $2C,$02,$3A,$92,$E3,$28,$01,$29,$F6,$02,$85,$0B,$E0,$01,$00,$08
        .byte   $D6,$01,$80,$0B,$B8,$01,$00,$38,$E0,$01,$AE,$12,$0A,$00,$2C,$12
        .byte   $0A,$00,$3A,$8B,$22,$50,$45,$52,$46,$45,$43,$54,$22,$3A,$8C,$4E
        .byte   $F6,$01,$88,$12,$64,$00,$3A,$AF,$42,$20,$01,$2C,$07,$2C,$11,$30
        .byte   $00,$2C,$11,$30,$00,$2C,$55,$3A,$AC,$55,$2C,$57,$3A,$8D,$00,$1C
        .byte   $EA,$01,$A3,$22,$4D,$31,$56,$32,$59,$31,$54,$32,$3A,$4D,$31,$56
        .byte   $31,$30,$59,$31,$54,$32,$3A,$54,$32,$22,$00,$20,$F4,$01,$A3,$22
        .byte   $4F,$33,$45,$33,$47,$23,$44,$47,$44,$47,$23,$44,$47,$3A,$4F,$31
        .byte   $23,$42,$35,$42,$23,$41,$42,$3A,$52,$22,$00,$1F,$FE,$01,$A3,$22
        .byte   $45,$47,$4F,$34,$43,$4F,$33,$45,$3A,$59,$32,$4F,$32,$23,$42,$36
        .byte   $45,$33,$3A,$4F,$31,$43,$35,$43,$22,$00,$18,$08,$02,$A3,$22,$23
        .byte   $47,$41,$4F,$34,$44,$43,$3A,$46,$35,$23,$46,$3A,$23,$43,$23,$43
        .byte   $22,$00,$1D,$12,$02,$A3,$22,$4F,$33,$42,$23,$41,$42,$4F,$34,$41
        .byte   $3A,$46,$33,$4F,$33,$44,$46,$41,$3A,$44,$36,$46,$33,$22,$00,$1E
        .byte   $1C,$02,$A3,$22,$47,$4F,$33,$41,$4F,$34,$46,$4F,$33,$42,$3A,$45
        .byte   $35,$44,$56,$31,$35,$3A,$47,$41,$23,$41,$42,$22,$00,$23,$26,$02
        .byte   $A3,$22,$23,$42,$33,$47,$54,$33,$46,$47,$45,$31,$44,$56,$31,$32
        .byte   $45,$37,$3A,$43,$39,$3A,$23,$42,$35,$54,$33,$47,$43,$37,$22,$00
        .byte   $1A,$30,$02,$AF,$42,$20,$01,$2C,$12,$0F,$00,$2C,$12,$30,$00,$2C
        .byte   $12,$30,$00,$2C,$12,$15,$00,$3A,$83,$00,$00,$53,$44,$24,$F3,$22
        .byte   $20,$22,$20,$85,$20,$4B,$44,$F6,$06,$20,$80,$20,$0B,$24,$04,$00
        .byte   $1A,$10,$04,$92,$20,$53,$55,$24,$F3,$22,$20,$22,$20,$85,$20,$4B
        .byte   $44,$F6,$02,$20,$80,$20,$0B,$24,$04,$00,$09,$1A,$04,$80,$20,$0B
        .byte   $F2,$03,$00,$09,$24,$04,$27,$FE,$62,$9B,$67,$00,$37,$2E,$04,$AA
        .byte   $20,$A1,$28,$46,$29,$F6,$8A,$20,$28,$12,$0E,$00,$2C,$4B,$44,$2C
        .byte   $02,$2C,$05,$2C,$01,$2C,$04,$29,$3A,$A7,$20,$46,$2C,$4B,$58,$FD
        .byte   $09,$F9,$12,$0C,$00,$2C,$4B,$59,$FD,$09,$F9,$12,$10,$00,$3A,$A1
        .byte   $20,$46,$00,$0C,$38,$04,$27,$FE,$A8,$71,$65,$96,$6F,$3F,$00,$23
        .byte   $42,$04,$92,$20,$E3,$28,$08,$29,$F4,$01,$20,$85,$20,$A3,$20,$22
        .byte   $4F,$31,$42,$32,$41,$47,$46,$45,$44,$43,$22,$3A,$80,$20,$0B,$60
        .byte   $04,$00,$07,$4C,$04,$27,$FE,$FE,$00,$09,$56,$04,$80,$20,$0B,$D6
        .byte   $01,$00,$0B,$60,$04,$27,$FE,$FE,$FE,$64,$8B,$87,$00,$21,$6A,$04
        .byte   $8C,$20,$4A,$58,$F6,$07,$20,$88,$20,$09,$3A,$93,$20,$53,$24,$3A
        .byte   $AE,$20,$0A,$2C,$4A,$58,$3A,$8B,$20,$53,$24,$3A,$8D,$00,$07,$74
        .byte   $04,$8A,$20,$9B,$00,$12,$7E,$04,$91,$20,$22,$FD,$FE,$FE,$FE,$FE
        .byte   $FE,$FE,$FE,$FE,$FD,$22,$00,$12,$88,$04,$91,$20,$22,$FE,$FD,$20
        .byte   $64,$6B,$7E,$61,$20,$FD,$FE,$22,$00,$12,$92,$04,$91,$20,$22,$FD
        .byte   $FE,$FE,$FE,$FE,$FE,$FE,$FE,$FE,$FD,$22,$00,$16,$9C,$04,$53,$F6
        .byte   $D6,$28,$01,$29,$3A,$92,$20,$53,$F6,$01,$20,$80,$20,$0B,$9C,$04
        .byte   $00,$0D,$A6,$04,$92,$20,$53,$F6,$02,$20,$85,$20,$82,$00,$05,$B0
        .byte   $04,$A2,$00,$00,$F6,$58,$4C,$FD,$09,$F9,$12,$0C,$00,$3A,$59,$F6
        .byte   $59,$4C,$FD,$09,$F9,$08,$3A,$A7,$20,$08,$2C,$58,$2C,$59,$3A,$83
        .byte   $00,$32,$70,$03,$AE,$20,$58,$42,$2C,$59,$4C,$FA,$02,$3A,$8B,$20
        .byte   $22,$20,$22,$3A,$52,$F6,$52,$F9,$02,$3A,$57,$F6,$57,$F9,$12,$0F
        .byte   $00,$3A,$AE,$20,$12,$0E,$00,$2C,$12,$17,$00,$3A,$8B,$20,$57,$3B
        .byte   $3A,$83,$00,$5D,$7A,$03,$A0,$3A,$49,$F6,$01,$3A,$46,$F6,$01,$3A
        .byte   $4B,$F6,$01,$3A,$43,$F6,$01,$3A,$52,$F6,$01,$3A,$5A,$F6,$12,$30
        .byte   $00,$3A,$AE,$20,$12,$0A,$00,$2C,$12,$15,$00,$3A,$8B,$22,$54,$49
        .byte   $4D,$45,$22,$3B,$54,$3A,$AE,$20,$07,$2C,$12,$16,$00,$3A,$8B,$22
        .byte   $48,$49,$2D,$53,$43,$4F,$52,$45,$22,$3B,$48,$3A,$AE,$20,$0A,$2C
        .byte   $12,$17,$00,$3A,$8B,$22,$53,$43,$4F,$52,$45,$22,$3B,$57,$3B,$00
        .byte   $05,$84,$03,$83,$00,$14,$8E,$03,$27,$63,$86,$2D,$20,$6B,$95,$87
        .byte   $20,$78,$20,$AD,$8A,$99,$86,$80,$00,$2C,$98,$03,$92,$20,$E5,$F6
        .byte   $12,$30,$02,$20,$85,$20,$58,$4C,$F6,$FA,$12,$1B,$00,$FD,$28,$58
        .byte   $4C,$F8,$02,$29,$3A,$81,$20,$0B,$66,$03,$3A,$A1,$20,$08,$3A,$BD
        .byte   $20,$0B,$76,$02,$00,$2D,$A2,$03,$92,$20,$E5,$F6,$12,$86,$01,$20
        .byte   $85,$20,$58,$4C,$F6,$FA,$12,$1B,$00,$FD,$28,$58,$4C,$F8,$01,$29
        .byte   $3A,$81,$20,$0B,$66,$03,$3A,$53,$F6,$53,$31,$3A,$BD,$20,$0B,$26
        .byte   $02,$00,$14,$AC,$03,$92,$20,$59,$4C,$F7,$12,$17,$00,$20,$85,$20
        .byte   $BD,$20,$0B,$94,$02,$00,$1B,$B6,$03,$92,$20,$58,$4C,$F7,$12,$1B
        .byte   $00,$20,$85,$20,$58,$4C,$F6,$01,$3A,$81,$20,$0B,$66,$03,$3A,$BD
        .byte   $00,$1B,$C0,$03,$92,$20,$58,$4C,$F8,$01,$20,$85,$20,$58,$4C,$F6
        .byte   $12,$1B,$00,$3A,$81,$20,$0B,$66,$03,$3A,$BD,$00,$18,$CA,$03,$92
        .byte   $20,$58,$42,$F7,$12,$1B,$00,$20,$85,$20,$58,$42,$F6,$01

        .segment "BANK7"
; ---- $DBFE-$E681（数据 2692 字节）
LDBFE:
        .byte   $15,$64,$00,$99,$3A,$AB,$20,$03,$3A,$9F,$20,$02,$2C,$02,$3A,$BA
        .byte   $20,$01,$2C,$02,$00,$17,$6E,$00,$8A,$20,$9A,$3A,$A3,$20,$22,$56
        .byte   $31,$35,$54,$31,$3A,$56,$31,$35,$54,$31,$22,$00,$16,$78,$00,$92
        .byte   $20,$E1,$28,$02,$2C,$03,$29,$F3,$22,$20,$22,$20,$80,$20,$0B,$F0
        .byte   $00,$00,$09,$82,$00,$27,$FE,$81,$61,$8A,$00,$25,$8C,$00,$8C,$20
        .byte   $4A,$58,$F6,$02,$20,$88,$20,$12,$15,$00,$20,$89,$20,$05,$3A,$8C
        .byte   $20,$4A,$59,$F6,$03,$20,$88,$20,$12,$14,$00,$20,$89,$20,$04,$00
        .byte   $24,$96,$00,$AE,$20,$4A,$58,$2C,$4A,$59,$3A,$8B,$20,$22,$E8,$ED
        .byte   $ED,$ED,$E8,$22,$3A,$92,$20,$4A,$59,$F7,$12,$13,$00,$20,$80,$20
        .byte   $0B,$B4,$00,$00,$17,$A0,$00,$AE,$20,$4A,$58,$2C,$4A,$59,$F9,$02
        .byte   $3A,$8B,$20,$22,$EE,$20,$20,$20,$EE,$22,$00,$17,$AA,$00,$AE,$20
        .byte   $4A,$58,$2C,$4A,$59,$F9,$03,$3A,$8B,$20,$22,$EE,$20,$20,$20,$EE
        .byte   $22,$00,$07,$B4,$00,$8D,$3A,$8D,$00,$2F,$BE,$00,$8C,$20,$53,$F6
        .byte   $06,$20,$88,$20,$12,$11,$00,$20,$89,$20,$04,$3A,$AE,$20,$02,$2C
        .byte   $53,$3A,$8B,$20,$22,$EC,$22,$3A,$AE,$20,$12,$19,$00,$2C,$53,$3A
        .byte   $8B,$20,$22,$EB,$22,$3A,$8D,$00,$10,$C8,$00,$AE,$20,$02,$2C,$03
        .byte   $3A,$8B,$20,$22,$EF,$22,$3B,$00,$22,$D2,$00,$8C,$20,$53,$F6,$01
        .byte   $20,$88,$20,$05,$3A,$8B,$20,$22,$ED,$ED,$ED,$EA,$22,$3B,$3A,$8D
        .byte   $3A,$8B,$20,$22,$ED,$ED,$ED,$F0,$22,$00,$12,$DC,$00,$AE,$20,$02
        .byte   $2C,$12,$14,$00,$3A,$8B,$20,$22,$F1,$22,$3B,$00,$22,$E6,$00,$8C
        .byte   $20,$53,$F6,$01,$20,$88,$20,$05,$3A,$8B,$20,$22,$ED,$ED,$ED,$E9
        .byte   $22,$3B,$3A,$8D,$3A,$8B,$20,$22,$ED,$ED,$ED,$F2,$22,$00,$0A,$F0
        .byte   $00,$27,$FE,$73,$88,$82,$78,$00,$27,$FA,$00,$BA,$20,$01,$2C,$01
        .byte   $3A,$A0,$3A,$8B,$20,$22,$65,$61,$A0,$62,$6C,$88,$3F,$20,$20,$20
        .byte   $41,$3D,$6E,$78,$7E,$7E,$20,$20,$20,$42,$3D,$6C,$88,$22,$00,$16
        .byte   $04,$01,$4B,$F6,$D6,$28,$01,$29,$3A,$92,$20,$4B,$F6,$01,$20,$80
        .byte   $20,$0B,$04,$01,$00,$0D,$0E,$01,$92,$20,$4B,$F6,$05,$20,$85,$20
        .byte   $B4,$00,$15,$18,$01,$A3,$20,$22,$4F,$33,$43,$31,$44,$45,$3A,$4F
        .byte   $33,$45,$31,$46,$47,$22,$00,$22,$22,$01,$50,$53,$24,$F6,$22,$31
        .byte   $32,$33,$34,$35,$36,$37,$38,$39,$22,$3A,$50,$4C,$F6,$CE,$28,$50
        .byte   $53,$24,$29,$3A,$50,$53,$F6,$02,$00,$0E,$2C,$01,$8C,$20,$53,$F6
        .byte   $02,$20,$88,$20,$50,$4C,$00,$28,$36,$01,$20,$53,$58,$F6,$03,$F9
        .byte   $D0,$28,$12,$18,$00,$29,$3A,$53,$59,$F6,$03,$F9,$D0,$28,$12,$12
        .byte   $00,$29,$3A,$53,$24,$F6,$E1,$28,$53,$58,$2C,$53,$59,$29,$00,$21
        .byte   $40,$01,$20,$92,$20,$53,$24,$F6,$22,$20,$22,$20,$F0,$20,$E2,$28
        .byte   $50,$53,$24,$2C,$53,$24,$29,$F7,$01,$20,$80,$20,$0B,$36,$01,$00
        .byte   $18,$4A,$01,$AE,$20,$53,$58,$2C,$53,$59,$3A,$8B,$20,$D4,$28,$50
        .byte   $53,$24,$2C,$53,$2C,$02,$29,$00,$05,$54,$01,$8D,$00,$0A,$5E,$01
        .byte   $27,$FE,$7C,$8C,$6C,$62,$00,$2C,$68,$01,$50,$58,$F6,$12,$0D,$00
        .byte   $FD,$09,$F9,$12,$0C,$00,$3A,$50,$59,$F6,$03,$FD,$09,$F9,$12,$10
        .byte   $00,$3A,$43,$F6,$04,$3A,$54,$F6,$01,$3A,$46,$57,$F6,$01,$3A,$46
        .byte   $F6,$01,$00,$1D,$72,$01,$4B,$4E,$F6,$03,$3A,$57,$F6,$12,$DB,$00
        .byte   $3A,$4B,$58,$F6,$01,$3A,$4B,$59,$F6,$01,$3A,$4B,$44,$F6,$01,$00
        .byte   $28,$7C,$01,$AE,$20,$01,$2C,$01,$3A,$8B,$20,$22,$20,$20,$20,$8B
        .byte   $74,$78,$65,$9E,$22,$3B,$43,$3B,$22,$20,$20,$20,$20,$20,$20,$20
        .byte   $73,$67,$72,$8C,$22,$3B,$54,$00,$28,$86,$01,$AA,$20,$A1,$28,$08
        .byte   $29,$F6,$8A,$20,$28,$05,$2C,$01,$2C,$03,$2C,$05,$2C,$01,$2C,$01
        .byte   $29,$3A,$A7,$20,$08,$2C,$50,$58,$2C,$50,$59,$3A,$A1,$20,$08,$00
        .byte   $0D,$90,$01,$94,$20,$45,$28,$4B,$4E,$FA,$02,$29,$00,$0A,$9A,$01
        .byte   $27,$FE,$65,$75,$A4,$88,$00,$10,$A4,$01,$8C,$20,$46,$F6,$01,$20
        .byte   $88,$20,$4B,$4E,$FA,$02,$00,$1B,$AE,$01,$AA,$20,$A1,$28,$46,$29
        .byte   $F6,$8A,$28,$12,$0E,$00,$2C,$02,$2C,$03,$2C,$05,$2C,$01,$2C,$04
        .byte   $29,$00,$43,$B8,$01,$53,$58,$F6,$02,$F9,$D0,$28,$12,$1A,$00,$29
        .byte   $3A,$53,$59,$F6,$12,$0A,$00,$F9,$D0,$28,$12,$0A,$00,$29,$3A,$92
        .byte   $20,$E1,$28,$53,$58,$2C,$53,$59,$29,$F6,$22,$20,$22,$20,$F0,$20
        .byte   $E1,$28,$53,$58,$2C,$53,$59,$FA,$02,$29,$F6,$22,$20,$22,$20,$80
        .byte   $20,$0B,$B8,$01,$00,$1D,$C2,$01,$A7,$20,$46,$2C,$53,$58,$FD,$09
        .byte   $F9,$12,$0C,$00,$2C,$53,$59,$FD,$09,$F9,$12,$18,$00,$3A,$A1,$20
        .byte   $46,$00,$05,$CC,$01,$8D,$00,$0E,$D6,$01,$27,$FE,$FE,$FE,$81,$61
        .byte   $8C,$FE,$FE,$FE,$00,$0E,$E0,$01,$27,$FE,$FE,$FE,$FE,$FE,$AE,$8C
        .byte   $98,$8C,$00,$0E,$F4,$01,$27,$FE,$66,$2D,$61,$8C,$2F,$62,$9B,$67
        .byte   $00,$24,$FE,$01,$53,$F6,$01,$3A,$53,$58,$F6,$01,$3A,$53,$59,$F6
        .byte   $01,$3A,$4B,$F6,$D5,$28,$01,$29,$3A,$92,$20,$4B,$F6,$01,$20,$80
        .byte   $20,$0B,$4E,$02,$00,$14,$08,$02,$92,$20,$4B,$F6,$02,$20,$85,$20
        .byte   $53,$F6,$04,$3A,$53,$58,$F6,$02,$00,$15,$12,$02,$92,$20,$4B,$F6
        .byte   $03,$20,$85,$20,$53,$F6,$08,$3A,$53,$58,$F6,$FA,$02,$00,$14,$1C
        .byte   $02,$92,$20,$4B,$F6,$05,$20,$85,$20,$53,$F6,$06,$3A,$53,$59,$F6
        .byte   $02,$00,$15,$26,$02,$92,$20,$4B,$F6,$09,$20,$85,$20,$53,$F6,$02
        .byte   $3A,$53,$59,$F6,$FA,$02,$00,$23,$2B,$02,$50,$58,$F6,$28,$D7,$28
        .byte   $08,$29,$FA,$12,$0C,$00,$29,$FC,$09,$3A,$50,$59,$F6,$28,$D8,$28
        .byte   $08,$29,$FA,$12,$10,$00,$29,$FC,$09,$00,$15,$30,$02,$53,$24,$F6
        .byte   $E1,$28,$50,$58,$F9,$53,$58,$2C,$50,$59,$F9,$53,$59,$29,$00,$18
        .byte   $3A,$02,$92,$20,$53,$F6,$01,$20,$F0,$20,$53,$24,$F6,$22,$20,$22
        .byte   $20,$80,$20,$0B,$4E,$02,$00,$34,$44,$02,$AA,$20,$A1,$28,$08,$29
        .byte   $F6,$8A,$20,$28,$05,$2C,$53,$2C,$03,$2C,$05,$2C,$01,$2C,$01,$29
        .byte   $3A,$A7,$20,$08,$2C,$50,$58,$FD,$09,$F9,$12,$0C,$00,$2C,$50,$59
        .byte   $FD,$09,$F9,$12,$10,$00,$3A,$A1,$20,$08,$00,$0C,$4E,$02,$27,$FE
        .byte   $6C,$62,$9D,$7A,$8A,$62,$00,$1A,$58,$02,$92,$20,$53,$24,$F3,$D4
        .byte   $28,$50,$53,$24,$2C,$50,$53,$2C,$02,$29,$20,$80,$20,$0B,$8A,$02
        .byte   $00,$18,$62,$02,$A3,$20,$22,$4F,$34,$43,$30,$23,$43,$44,$23,$44
        .byte   $45,$46,$23,$46,$47,$23,$47,$22,$00,$1B,$6C,$02,$AE,$20,$50,$58
        .byte   $F9,$53,$58,$2C,$50,$59,$F9,$53,$59,$3A,$8B,$20,$DC,$28,$12,$C7
        .byte   $00,$29,$3B,$00,$17,$76,$02,$54,$F6,$54,$F9,$12,$0A,$00,$3A,$AE
        .byte   $20,$12,$15,$00,$2C,$01,$3A,$8B,$20,$54,$00,$31,$80,$02,$50,$53
        .byte   $F6,$50,$53,$F9,$02,$3A,$92,$20,$50,$53,$F7,$50,$4C,$20,$85,$20
        .byte   $A3,$20,$22,$4F,$34,$43,$33,$44,$45,$46,$47,$3A,$4F,$34,$45,$33
        .byte   $46,$47,$41,$42,$22,$3A,$80,$20,$0B,$60,$04,$00,$0A,$8A,$02,$27
        .byte   $FE,$8B,$74,$64,$67,$00,$2D,$94,$02,$92,$20,$D6,$28,$01,$29,$F3
        .byte   $09,$20,$F0,$20,$E2,$28,$50,$53,$24,$2C,$53,$24,$29,$F3,$01,$20
        .byte   $F0,$20,$43,$F8,$02,$20,$F0,$20,$46,$57,$F6,$02,$20,$80,$20,$0B
        .byte   $B2,$02,$00,$28,$9E,$02,$A3,$20,$22,$4F,$32,$45,$32,$43,$22,$3A
        .byte   $57,$24,$F6,$53,$24,$3A,$AE,$20,$50,$58,$F9,$53,$58,$2C,$50,$59
        .byte   $F9,$53,$59,$3A,$8B,$20,$DC,$28,$57,$29,$00,$18,$A8,$02,$46,$57
        .byte   $F6,$02,$3A,$43,$F6,$43,$FA,$02,$3A,$AE,$20,$09,$2C,$01,$3A,$8B
        .byte   $20,$43,$00,$0C,$B2,$02,$27,$FE,$FE,$FE,$FE,$FE,$65,$75,$00,$18
        .byte   $BC,$02,$46,$F6,$46,$F9,$02,$3A,$92,$20,$46,$F7,$4B,$4E,$FA,$02
        .byte   $20,$85,$20,$46,$F6,$01,$00,$0C,$C6,$02,$27,$FE,$6B,$A7,$89,$72
        .byte   $88,$3F,$00,$18,$D0,$02,$4B,$44,$F6,$E6,$28,$46,$29,$3A,$92,$20
        .byte   $4B,$44,$F7,$01,$20,$80,$20,$0B,$F8,$02,$00,$22,$DA,$02,$92,$20
        .byte   $A1,$28,$46,$29,$F3,$01,$20,$85,$20,$A3,$20,$22,$4F,$30,$43,$30
        .byte   $45,$47,$43,$45,$47,$22,$3A,$80,$20,$0B,$38,$04,$00,$0C,$E4,$02
        .byte   $27,$FE,$6B,$A7,$89,$64,$8B,$87,$00,$0B,$EE,$02,$4B,$44,$F6,$45
        .byte   $28,$46,$29,$00,$0A,$F8,$02,$27,$FE,$9C,$7A,$95,$62,$00,$1F,$02
        .byte   $03,$4B,$58,$F6,$28,$D7,$28,$46,$29,$FA,$12,$0C,$00,$29,$FC,$09
        .byte   $3A,$4A,$58,$F6,$D1,$28,$50,$58,$FA,$4B,$58,$29,$00,$1F,$0C,$03
        .byte   $4B,$59,$F6,$28,$D8,$28,$46,$29,$FA,$12,$10,$00,$29,$FC,$09,$3A
        .byte   $4A,$59,$F6,$D1,$28,$50,$59,$FA,$4B,$59,$29,$00,$4C,$16,$03,$53
        .byte   $4B,$24,$F6,$E1,$28,$4B,$58,$2C,$4B,$59,$29,$3A,$53,$52,$24,$F6
        .byte   $E1,$28,$4B,$58,$F9,$02,$2C,$4B,$59,$29,$3A,$53,$4C,$24,$F6,$E1
        .byte   $28,$4B,$58,$FA,$02,$2C,$4B,$59,$29,$3A,$53,$44,$24,$F6,$E1,$28
        .byte   $4B,$58,$2C,$4B,$59,$F9,$02,$29,$3A,$53,$55,$24,$F6,$E1,$28,$4B
        .byte   $58,$2C,$4B,$59,$FA,$02,$29,$00,$09,$20,$03,$27,$FE,$8B,$74,$3F
        .byte   $00,$14,$2A,$03,$92,$20,$53,$4B,$24,$F3,$DC,$28,$57,$29,$20,$80
        .byte   $20,$0B,$52,$03,$00,$38,$34,$03,$AA,$20,$A1,$28,$46,$29,$F6,$8A
        .byte   $20,$28,$12,$0E,$00,$2C,$01,$2C,$02,$2C,$12,$C8,$00,$2C,$02,$2C
        .byte   $03,$29,$3A,$A7,$20,$46,$2C,$4B,$58,$FD,$09,$F9,$12,$0C,$00,$2C
        .byte   $4B,$59,$FD,$09,$F9,$12,$10,$00,$3A,$A1,$20,$46,$00,$1D,$3E,$03
        .byte   $45,$28,$46,$29,$F6,$4B,$44,$3A,$AE,$20,$4B,$58,$2C,$4B,$59,$3A
        .byte   $8B,$20,$57,$24,$3A,$46,$57,$F6,$01,$00,$09,$48,$03,$80,$20,$0B
        .byte   $38,$04,$00,$0D,$52,$03,$27,$FE,$FE,$FE,$64,$61,$65,$68,$88,$00
        .byte   $28,$5C,$03,$92,$28,$53,$52,$24,$F9,$53,$4C,$24,$F3,$22,$20,$20
        .byte   $22,$20,$F1,$20,$53,$44,$24,$F9,$53,$55,$24,$F3,$22,$20,$20,$22
        .byte   $29,$20,$80,$20,$0B,$8E,$03,$00,$0C,$66,$03,$27,$FE,$FE,$70,$95
        .byte   $67,$6B,$8C,$00,$44,$70,$03,$92,$28,$4B,$44,$F6,$04,$20,$F1,$20
        .byte   $4A,$58,$F6,$02,$29,$F0,$28,$4B,$44,$F6,$08,$20,$F1,$20,$4A,$58
        .byte   $F6,$FA,$02,$29,$F0,$28,$4B,$44,$F6,$06,$20,$F1,$20,$4A,$59,$F6
        .byte   $02,$29,$F0,$28,$4B,$44,$F6,$02,$20,$F1,$20,$4A,$59,$F6,$FA,$02
        .byte   $29,$20,$80,$20,$0B,$24,$04,$00,$0A,$7A,$03,$27,$FE,$79,$8C,$72
        .byte   $8C,$00,$1C,$84,$03,$4B,$44,$F6,$28,$4B,$44,$F9,$05,$29,$F9,$09
        .byte   $FD,$28,$4B,$44,$F7,$04,$29,$3A,$80,$20,$0B,$24,$04,$00,$0A,$8E
        .byte   $03,$27,$FE,$FE,$A8,$8C,$66,$00,$09,$98,$03,$27,$FE,$61,$68,$88
        .byte   $00,$1C,$A2,$03,$92,$20,$4A,$58,$FD,$4A,$59,$F3,$01,$20,$F1,$20
        .byte   $D0,$28,$03,$29,$F6,$02,$20,$80,$20,$0B,$C0,$03,$00,$21,$AC,$03
        .byte   $92,$20,$4A,$58,$F6,$02,$20,$F1,$20,$53,$52,$24,$F3,$22,$20,$22
        .byte   $20,$85,$20,$4B,$44,$F6,$04,$3A,$80,$20,$0B,$24,$04,$00,$22,$B6
        .byte   $03,$92,$20,$4A,$58,$F6,$FA,$02,$20,$F1,$20,$53,$4C,$24,$F3,$22
        .byte   $20,$22,$20,$85,$20,$4B,$44,$F6,$08,$3A,$80,$20,$0B,$24,$04,$00
        .byte   $21,$C0,$03,$92,$20,$4A,$59,$F6,$02,$20,$F1,$20,$53,$44,$24,$F3
        .byte   $22,$20,$22,$20,$85,$20,$4B,$44,$F6,$06,$3A,$80,$20,$0B,$24,$04
        .byte   $00,$22,$CA,$03,$92,$20,$4A,$59,$F6,$FA,$02,$20,$F1,$20,$53,$55
        .byte   $24,$F3,$22,$20,$22,$20,$85,$20,$4B,$44,$F6,$02,$3A,$80,$20,$0B
        .byte   $24,$04,$00,$0A,$D4,$03,$27,$FE,$61,$68,$74,$61,$00,$0C,$DE,$03
        .byte   $53,$F6,$D0,$28,$05,$29,$F9,$02,$00,$19,$E8,$03,$9A,$20,$53,$20
        .byte   $80,$20,$0B,$F2,$03,$2C,$0B,$FC,$03,$2C,$0B,$06,$04,$2C,$0B,$10
        .byte   $04,$00,$1A,$F2,$03,$92,$20,$53,$52,$24,$F3,$22,$20,$22,$20,$85
        .byte   $20,$4B,$44,$F6,$04,$20,$80,$20,$0B,$24,$04,$00,$1A,$FC,$03,$92
        .byte   $20,$53,$4C,$24,$F3,$22,$20,$22,$20,$85,$20,$4B,$44,$F6,$08,$20
        .byte   $80,$20,$0B,$24,$04,$00,$1A,$06,$04,$92,$20,$53,$44,$24,$F3,$22
        .byte   $20,$22,$20,$85,$20,$4B,$44,$F6,$06,$20,$80,$20,$0B,$24,$04,$00
        .byte   $1A,$10,$04,$92,$20,$53,$55,$24,$F3,$22,$20,$22,$20,$85,$20,$4B
        .byte   $44,$F6,$02,$20,$80,$20,$0B,$24,$04,$00,$09,$1A,$04,$80,$20,$0B
        .byte   $F2,$03,$00,$09,$24,$04,$27,$FE,$62,$9B,$67,$00,$37,$2E,$04,$AA
        .byte   $20,$A1,$28,$46,$29,$F6,$8A,$20,$28,$12,$0E,$00,$2C,$4B,$44,$2C
        .byte   $02,$2C,$05,$2C,$01,$2C,$04,$29,$3A,$A7,$20,$46,$2C,$4B,$58,$FD
        .byte   $09,$F9,$12,$0C,$00,$2C,$4B,$59,$FD,$09,$F9,$12,$10,$00,$3A,$A1
        .byte   $20,$46,$00,$0C,$38,$04,$27,$FE,$A8,$71,$65,$96,$6F,$3F,$00,$23
        .byte   $42,$04,$92,$20,$E3,$28,$08,$29,$F4,$01,$20,$85,$20,$A3,$20,$22
        .byte   $4F,$31,$42,$32,$41,$47,$46,$45,$44,$43,$22,$3A,$80,$20,$0B,$60
        .byte   $04,$00,$07,$4C,$04,$27,$FE,$FE,$00,$09,$56,$04,$80,$20,$0B,$D6
        .byte   $01,$00,$0B,$60,$04,$27,$FE,$FE,$FE,$64,$8B,$87,$00,$21,$6A,$04
        .byte   $8C,$20,$4A,$58,$F6,$07,$20,$88,$20,$09,$3A,$93,$20,$53,$24,$3A
        .byte   $AE,$20,$0A,$2C,$4A,$58,$3A,$8B,$20,$53,$24,$3A,$8D,$00,$07,$74
        .byte   $04,$8A,$20,$9B,$00,$12,$7E,$04,$91,$20,$22,$FD,$FE,$FE,$FE,$FE
        .byte   $FE,$FE,$FE,$FE,$FD,$22,$00,$12,$88,$04,$91,$20,$22,$FE,$FD,$20
        .byte   $64,$6B,$7E,$61,$20,$FD,$FE,$22,$00,$12,$92,$04,$91,$20,$22,$FD
        .byte   $FE,$FE,$FE,$FE,$FE,$FE,$FE,$FE,$FD,$22,$00,$16,$9C,$04,$53,$F6
        .byte   $D6,$28,$01,$29,$3A,$92,$20,$53,$F6,$01,$20,$80,$20,$0B,$9C,$04
        .byte   $00,$0D,$A6,$04,$92,$20,$53,$F6,$02,$20,$85,$20,$82,$00,$05,$B0
        .byte   $04,$A2,$00,$00

        .segment "BANK8"
; ---- $E682-$F307（数据 3206 字节）
LE682:
        .byte   $09,$0A,$00,$9F,$20,$02,$2C,$01,$00,$0D,$14,$00,$9A,$20,$BC,$20
        .byte   $80,$20,$0B,$8E,$03,$00,$48,$1E,$00,$57,$F6,$46,$F6,$4B,$F6,$54
        .byte   $F6,$52,$F6,$5A,$F6,$4E,$49,$F6,$4E,$4E,$F6,$58,$45,$F6,$59,$45
        .byte   $F6,$58,$4E,$F6,$59,$4E,$F6,$58,$42,$F6,$58,$4C,$F6,$59,$4C,$F6
        .byte   $53,$32,$F6,$53,$31,$F6,$53,$F6,$47,$F6,$56,$F6,$58,$F6,$59,$F6
        .byte   $49,$F6,$44,$3A,$48,$F6,$01,$3A,$53,$24,$F6,$47,$24,$00,$1C,$28
        .byte   $00,$AF,$42,$20,$04,$2C,$12,$0D,$00,$2C,$12,$30,$00,$2C,$12,$36
        .byte   $00,$2C,$12,$16,$00,$3A,$8A,$20,$9A,$00,$2B,$32,$00,$4B,$24,$F6
        .byte   $DC,$28,$12,$18,$00,$29,$3A,$52,$24,$F6,$DC,$28,$12,$19,$00,$29
        .byte   $3A,$48,$24,$F6,$DC,$28,$12,$1A,$00,$29,$3A,$55,$24,$F6,$DC,$28
        .byte   $12,$1B,$00,$29,$00,$21,$3C,$00,$AA,$20,$8A,$20,$01,$2C,$28,$01
        .byte   $2C,$02,$2C,$01,$2C,$02,$2C,$01,$29,$F6,$52,$24,$F9,$4B,$24,$F9
        .byte   $55,$24,$F9,$48,$24,$00,$2B,$46,$00,$4B,$24,$F6,$DC,$28,$12,$0C
        .byte   $00,$29,$3A,$52,$24,$F6,$DC,$28,$12,$0D,$00,$29,$3A,$48,$24,$F6
        .byte   $DC,$28,$12,$0E,$00,$29,$3A,$55,$24,$F6,$DC,$28,$12,$0F,$00,$29
        .byte   $00,$21,$50,$00,$AA,$20,$8A,$20,$02,$2C,$28,$01,$2C,$02,$2C,$01
        .byte   $2C,$02,$2C,$01,$29,$F6,$52,$24,$F9,$4B,$24,$F9,$55,$24,$F9,$48
        .byte   $24,$00,$21,$5A,$00,$AA,$20,$8A,$20,$03,$2C,$28,$01,$2C,$02,$2C
        .byte   $01,$2C,$01,$2C,$01,$29,$F6,$4B,$24,$F9,$52,$24,$F9,$48,$24,$F9
        .byte   $55,$24,$00,$2B,$64,$00,$4B,$24,$F6,$DC,$28,$12,$14,$00,$29,$3A
        .byte   $52,$24,$F6,$DC,$28,$12,$15,$00,$29,$3A,$48,$24,$F6,$DC,$28,$12
        .byte   $16,$00,$29,$3A,$55,$24,$F6,$DC,$28,$12,$17,$00,$29,$00,$21,$6E
        .byte   $00,$AA,$20,$8A,$20,$04,$2C,$28,$01,$2C,$02,$2C,$01,$2C,$01,$2C
        .byte   $01,$29,$F6,$4B,$24,$F9,$52,$24,$F9,$48,$24,$F9,$55,$24,$00,$37
        .byte   $78,$00,$4B,$24,$F6,$DC,$28,$12,$E3,$00,$29,$3A,$52,$24,$F6,$DC
        .byte   $28,$12,$D7,$00,$29,$3A,$48,$24,$F6,$DC,$28,$12,$D2,$00,$29,$3A
        .byte   $55,$24,$F6,$DC,$28,$12,$C0,$00,$29,$3A,$94,$20,$51,$28,$04,$29
        .byte   $2C,$44,$28,$04,$29,$00,$0D,$82,$00,$27,$6B,$95,$66,$20,$6D,$96
        .byte   $72,$61,$00,$36,$8C,$00,$54,$F6,$12,$20,$03,$3A,$57,$F6,$01,$3A
        .byte   $4E,$4E,$F6,$02,$3A,$58,$F6,$12,$74,$00,$3A,$59,$F6,$12,$7F,$00
        .byte   $3A,$A7,$20,$08,$2C,$58,$2C,$59,$3A,$81,$20,$0B,$7A,$03,$3A,$56
        .byte   $F6,$01,$3A,$80,$20,$0B,$1C,$02,$00,$16,$96,$00,$27,$6F,$61,$80
        .byte   $20,$73,$20,$73,$67,$72,$8C,$20,$78,$20,$79,$8C,$72,$61,$00,$36
        .byte   $A0,$00,$54,$F6,$54,$FA,$02,$3A,$92,$20,$54,$20,$FB,$20,$12,$0A
        .byte   $00,$F6,$01,$20,$85,$20,$AE,$20,$12,$0E,$00,$2C,$12,$15,$00,$3A
        .byte   $8B,$20,$54,$3B,$22,$20,$20,$22,$3A,$92,$20,$54,$F6,$01,$20,$85
        .byte   $20,$0B,$94,$02,$00,$1D,$AA,$00,$92,$20,$5A,$F6,$12,$3A,$00,$20
        .byte   $85,$20,$92,$20,$52,$F7,$12,$0F,$00,$20,$85,$20,$81,$20,$0B,$7A
        .byte   $03,$00,$11,$B4,$00,$27,$75,$6F,$75,$6F,$20,$78,$20,$AD,$8A,$99
        .byte   $86,$80,$00,$13,$BE,$00,$49,$F6,$FA,$28,$49,$F9,$02,$29,$FD,$28
        .byte   $49,$F3,$4E,$4E,$29,$00,$13,$C8,$00,$58,$4E,$F6,$D7,$28,$49,$29
        .byte   $3A,$59,$4E,$F6,$D8,$28,$49,$29,$00,$50,$D2,$00,$92,$20,$58,$4E
        .byte   $F8,$12,$0C,$00,$20,$F0,$20,$58,$4E,$F7,$12,$DE,$00,$20,$F0,$20
        .byte   $59,$4E,$F7,$12,$BC,$00,$20,$85,$20,$58,$4E,$F6,$D0,$28,$12,$1B
        .byte   $00,$29,$FD,$09,$F9,$12,$0C,$00,$3A,$59,$4E,$F6,$08,$3A,$A7,$20
        .byte   $49,$2C,$58,$4E,$2C,$59,$4E,$3A,$44,$F6,$01,$3A,$51,$28,$49,$29
        .byte   $F6,$06,$3A,$80,$20,$0B,$18,$01,$00,$1B,$DC,$00,$58,$45,$F6,$28
        .byte   $58,$4E,$FA,$09,$29,$FC,$09,$3A,$59,$45,$F6,$28,$59,$4E,$FA,$03
        .byte   $29,$FC,$09,$00,$0F,$E6,$00,$47,$24,$F6,$E1,$28,$58,$45,$2C,$59
        .byte   $45,$29,$00,$0A,$F0,$00,$44,$F6,$51,$28,$49,$29,$00,$39,$FA,$00
        .byte   $92,$20,$47,$24,$F3,$55,$24,$20,$85,$20,$92,$20,$59,$4C,$F3,$59
        .byte   $45,$20,$F0,$20,$47,$24,$F3,$48,$24,$20,$85,$20,$44,$F6,$06,$3A
        .byte   $51,$28,$49,$29,$F6,$04,$FA,$05,$FD,$28,$58,$F8,$58,$4E,$29,$3A
        .byte   $80,$20,$0B,$0E,$01,$00,$12,$04,$01,$92,$20,$44,$28,$49,$29,$F6
        .byte   $44,$20,$85,$20,$0B,$2C,$01,$00,$23,$0E,$01,$58,$4E,$F6,$58,$45
        .byte   $FD,$09,$F9,$12,$0C,$00,$3A,$59,$4E,$F6,$59,$45,$FD,$09,$F9,$08
        .byte   $3A,$A7,$20,$49,$2C,$58,$4E,$2C,$59,$4E,$00,$2E,$18,$01,$AA,$20
        .byte   $A1,$28,$49,$29,$F6,$8A,$28,$12,$0B,$00,$2C,$44,$2C,$03,$2C,$12
        .byte   $FF,$00,$F9,$12,$FB,$00,$FD,$28,$44,$F6,$06,$29,$29,$3A,$A1,$20
        .byte   $49,$3A,$44,$28,$49,$29,$F6,$44,$00,$10,$22,$01,$27,$A8,$71,$65
        .byte   $87,$20,$78,$20,$79,$8C,$72,$61,$00,$43,$2C,$01,$4E,$49,$F6,$E3
        .byte   $28,$08,$29,$3A,$92,$20,$4E,$49,$F7,$FA,$02,$20,$85,$20,$92,$20
        .byte   $CA,$28,$D8,$28,$4E,$49,$29,$FA,$D8,$28,$08,$29,$29,$F8,$12,$0E
        .byte   $00,$20,$F1,$20,$CA,$28,$D7,$28,$4E,$49,$29,$FA,$D7,$28,$08,$29
        .byte   $29,$F8,$12,$0E,$00,$20,$85,$20,$0B,$94,$02,$00,$0F,$36,$01,$92
        .byte   $20,$46,$F6,$02,$20,$85,$20,$0B,$0C,$03,$00,$10,$40,$01,$27,$7E
        .byte   $87,$64,$20,$78,$20,$AD,$8A,$99,$86,$80,$00,$21,$4A,$01,$53,$31
        .byte   $F6,$53,$3A,$53,$F6,$D5,$28,$01,$29,$3A,$92,$20,$53,$F7,$03,$20
        .byte   $85,$20,$53,$F6,$04,$FA,$28,$53,$F7,$08,$29,$00,$11,$54,$01,$58
        .byte   $F6,$D7,$28,$08,$29,$3A,$59,$F6,$D8,$28,$08,$29,$00,$17,$5E,$01
        .byte   $58,$4C,$F6,$58,$FC,$09,$FA,$02,$3A,$59,$4C,$F6,$28,$59,$FA,$03
        .byte   $29,$FC,$09,$00,$19,$68,$01,$9A,$20,$53,$20,$80,$20,$0B,$26,$02
        .byte   $2C,$0B,$26,$02,$2C,$0B,$E0,$01,$2C,$0B,$FE,$01,$00,$05,$72,$01
        .byte   $27,$00,$0F,$7C,$01,$56,$F6,$01,$3A,$4B,$F6,$01,$3A,$9C,$20,$08
        .byte   $00,$32,$86,$01,$92,$20,$E1,$28,$58,$4C,$2C,$59,$4C,$29,$F6,$22
        .byte   $20,$22,$20,$85,$20,$92,$20,$E1,$28,$58,$4C,$2C,$59,$4C,$FA,$02
        .byte   $29,$F3,$48,$24,$20,$85,$20,$81,$20,$0B,$66,$03,$3A,$80,$20,$0B
        .byte   $EE,$02,$00,$2F,$90,$01,$58,$42,$F6,$58,$4C,$F9,$28,$53,$31,$F6
        .byte   $02,$29,$FA,$28,$53,$31,$F6,$03,$29,$3A,$92,$20,$E1,$28,$58,$42
        .byte   $2C,$59,$4C,$FA,$02,$29,$F6,$52,$24,$20,$85,$20,$81,$20,$0B,$70
        .byte   $03,$00,$22,$9A,$01,$92,$20,$E1,$28,$58,$4C,$2C,$59,$4C,$FA,$02
        .byte   $29,$F6,$4B,$24,$20,$85,$20,$47,$F6,$FA,$12,$0F,$00,$3A,$80,$20
        .byte   $0B,$02,$03,$00,$44,$A4,$01,$92,$20,$CB,$28,$E1,$28,$58,$4C,$2C
        .byte   $59,$4C,$FA,$02,$29,$29,$F6,$5A,$20,$85,$20,$AE,$20,$58,$4C,$2C
        .byte   $59,$4C,$FA,$02,$3A,$8B,$20,$22,$20,$22,$3A,$5A,$F6,$5A,$F9,$02
        .byte   $3A,$57,$F6,$57,$F9,$12,$1E,$00,$3A,$AE,$20,$12,$0E,$00,$2C,$12
        .byte   $17,$00,$3A,$8B,$20,$57,$3B,$00,$12,$AE,$01,$92,$20,$D6,$28,$01
        .byte   $29,$F3,$09,$20,$85,$20,$0B,$D6,$01,$00,$19,$B8,$01,$92,$20,$E1
        .byte   $28,$58,$4C,$FA,$02,$2C,$59,$4C,$29,$F6,$55,$24,$20,$85,$20,$0B
        .byte   $F8,$02,$00,$17,$C2,$01,$92,$20,$E1,$28,$58,$4C,$2C,$59,$4C,$29
        .byte   $F6,$55,$24,$20,$85,$20,$0B,$F8,$02,$00,$19,$CC,$01,$92,$20,$E1
        .byte   $28,$58,$4C,$F9,$02,$2C,$59,$4C,$29,$F6,$55,$24,$20,$85,$20,$0B
        .byte   $F8,$02,$00,$09,$D6,$01,$80,$20,$0B,$A0,$00,$00,$13,$E0,$01,$56
        .byte   $F6,$06,$3A,$47,$24,$F6,$E1,$28,$58,$4C,$2C,$59,$4C,$29,$00,$25
        .byte   $EA,$01,$92,$20,$47,$24,$F3,$48,$24,$20,$85,$20,$92,$20,$47,$24
        .byte   $F3,$55,$24,$20,$85,$20,$53,$F6,$01,$3A,$56,$F6,$01,$3A,$80,$20
        .byte   $0B,$EE,$02,$00,$09,$F4,$01,$80,$20,$0B,$08,$02,$00,$15,$FE,$01
        .byte   $56,$F6,$02,$3A,$47,$24,$F6,$E1,$28,$58,$4C,$2C,$59,$4C,$FA,$02
        .byte   $29,$00,$17,$08,$02,$92,$20,$47,$24,$F3,$48,$24,$20,$85,$20,$53
        .byte   $F6,$01,$3A,$80,$20,$0B,$7C,$01,$00,$1C,$12,$02,$92,$20,$53,$31
        .byte   $F6,$01,$20,$F0,$20,$4B,$F7,$01,$20,$85,$20,$81,$20,$0B,$66,$03
        .byte   $3A,$4B,$F6,$01,$00,$28,$1C,$02,$AA,$20,$A1,$28,$08,$29,$F6,$8A
        .byte   $28,$01,$2C,$56,$2C,$02,$2C,$05,$29,$3A,$A1,$20,$08,$3A,$8A,$20
        .byte   $53,$32,$3A,$8E,$20,$06,$3A,$80,$20,$0B,$A0,$00,$00,$20,$26,$02
        .byte   $56,$F6,$05,$FD,$53,$FA,$02,$3A,$4B,$F6,$02,$FA,$28,$4B,$F9,$28
        .byte   $4B,$F7,$04,$29,$29,$FD,$28,$53,$F6,$53,$31,$29,$00,$25,$30,$02
        .byte   $47,$24,$F6,$E1,$28,$58,$4C,$F9,$02,$F9,$03,$FD,$28,$53,$F6,$03
        .byte   $29,$2C,$59,$4C,$29,$3A,$53,$24,$F6,$E1,$28,$58,$4C,$2C,$59,$4C
        .byte   $29,$00,$33,$3A,$02,$58,$42,$F6,$58,$4C,$F9,$02,$F9,$03,$FD,$28
        .byte   $53,$F6,$02,$29,$3A,$92,$20,$4B,$F7,$03,$20,$85,$20,$92,$20,$E1
        .byte   $28,$58,$42,$2C,$59,$4C,$FA,$02,$29,$F6,$52,$24,$20,$85,$20,$81
        .byte   $20,$0B,$70,$03,$00,$21,$44,$02,$92,$20,$E1,$28,$58,$4C,$2C,$59
        .byte   $4C,$FA,$02,$29,$F6,$52,$24,$20,$85,$20,$58,$42,$F6,$58,$4C,$3A
        .byte   $81,$20,$0B,$70,$03,$00,$38,$4E,$02,$92,$20,$47,$24,$F3,$55,$24
        .byte   $20,$F1,$20,$47,$24,$F3,$48,$24,$20,$85,$20,$58,$F6,$58,$F9,$07
        .byte   $FD,$28,$53,$24,$F3,$22,$20,$22,$29,$FD,$28,$28,$53,$F6,$02,$29
        .byte   $FA,$28,$53,$F6,$03,$29,$29,$3A,$80,$20,$0B,$EE,$02,$00,$19,$58
        .byte   $02,$92,$20,$53,$24,$F6,$22,$20,$22,$20,$85,$20,$53,$F6,$53,$31
        .byte   $3A,$80,$20,$0B,$EE,$02,$00,$32,$62,$02,$92,$20,$D6,$28,$01,$29
        .byte   $F6,$09,$20,$85,$20,$92,$20,$47,$24,$F6,$55,$24,$20,$F0,$20,$53
        .byte   $24,$F6,$55,$24,$20,$F0,$20,$E1,$28,$58,$42,$2C,$59,$4C,$29,$F6
        .byte   $55,$24,$20,$85,$20,$0B,$F8,$02,$00,$2F,$6C,$02,$4E,$49,$F6,$E3
        .byte   $28,$08,$29,$3A,$92,$20,$4E,$49,$F7,$FA,$02,$20,$85,$20,$92,$20
        .byte   $CA,$28,$D7,$28,$4E,$49,$29,$FA,$D7,$28,$08,$29,$29,$F8,$12,$0E
        .byte   $00,$20,$85,$20,$0B,$94,$02,$00,$2A,$76,$02,$92,$20,$4B,$F8,$05
        .byte   $20,$85,$20,$AA,$20,$A1,$28,$08,$29,$F6,$8A,$28,$01,$2C,$56,$2C
        .byte   $05,$FA,$4B,$2C,$12,$FF,$00,$29,$3A,$A1,$20,$08,$3A,$8A,$20,$53
        .byte   $32,$00,$09,$80,$02,$80,$20,$0B,$A0,$00,$00,$10,$8A,$02,$27,$64
        .byte   $8B,$87,$20,$78,$20,$AD,$8A,$99,$86,$80,$00,$15,$94,$02,$8A,$20
        .byte   $53,$3A,$8A,$20,$53,$31,$3A,$8A,$20,$53,$32,$3A,$B0,$20,$08,$00
        .byte   $23,$9E,$02,$8C,$20,$49,$F6,$01,$20,$88,$20,$4E,$4E,$3A,$B0,$20
        .byte   $49,$3A,$A7,$20,$49,$2C,$01,$2C,$01,$3A,$44,$28,$49,$29,$F6,$01
        .byte   $3A,$8D,$00,$1A,$A8,$02,$AE,$20,$09,$2C,$0A,$3A,$8B,$22,$2A,$2A
        .byte   $2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$22,$00,$1C,$B2,$02
        .byte   $AE,$20,$09,$2C,$12,$0A,$00,$3A,$8B,$22,$2A,$20,$47,$41,$4D,$45
        .byte   $20,$4F,$56,$45,$52,$20,$2A,$22,$00,$1C,$BC,$02,$AE,$20,$09,$2C
        .byte   $12,$0B,$00,$3A,$8B,$22,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A,$2A
        .byte   $2A,$2A,$2A,$22,$00,$1E,$C6,$02,$92,$20,$57,$F7,$48,$20,$85,$20
        .byte   $48,$F6,$57,$3A,$AE,$20,$12,$0E,$00,$2C,$12,$16,$00,$3A,$8B,$20
        .byte   $48,$3B,$00,$12,$D0,$02,$92,$20,$D6,$28,$01,$29,$F6,$02,$20,$85
        .byte   $20,$0B,$8C,$00,$00,$09,$DA,$02,$80,$20,$0B,$D0,$02,$00,$11,$E4
        .byte   $02,$27,$9D,$93,$8C,$AD,$20,$78,$20,$AD,$8A,$99,$86,$80,$00,$13
        .byte   $EE,$02,$47,$F6,$03,$3A,$59,$F6,$59,$F9,$04,$3A,$80,$20,$0B,$02
        .byte   $03,$00,$2A,$F8,$02,$47,$F6,$FA,$0A,$3A,$58,$F6,$58,$F9,$28,$03
        .byte   $FD,$4B,$F9,$07,$29,$FD,$28,$28,$53,$F6,$03,$29,$FA,$28,$53,$F6
        .byte   $02,$29,$29,$3A,$59,$F6,$59,$FA,$12,$0C,$00,$00,$3D,$02,$03,$46
        .byte   $F6,$02,$3A,$8A,$20,$53,$2C,$58,$2C,$59,$3A,$B0,$20,$08,$3A,$3A
        .byte   $8A,$20,$04,$3A,$A7,$20,$08,$2C,$58,$2C,$59,$3A,$53,$32,$F6,$53
        .byte   $3A,$4B,$F6,$4B,$FD,$28,$28,$53,$F6,$03,$29,$FA,$28,$53,$F6,$02
        .byte   $29,$29,$3A,$80,$20,$0B,$A0,$00,$00,$2C,$0C,$03,$4E,$49,$F6,$E3
        .byte   $28,$08,$29,$3A,$92,$20,$4E,$49,$F7,$FA,$02,$20,$85,$20,$92,$20
        .byte   $CA,$28,$D8,$28,$4E,$49,$29,$FA,$59,$29,$F8,$12,$0C,$00,$20,$85
        .byte   $20,$0B,$94,$02,$00,$2D,$16,$03,$47,$F6,$47,$F9,$04,$F9,$04,$FD
        .byte   $28,$47,$F7,$07,$29,$3A,$58,$F6,$58,$F9,$4B,$3A,$59,$F6,$59,$F9
        .byte   $47,$3A,$8A,$20,$53,$2C,$58,$2C,$59,$3A,$A7,$20,$08,$2C,$58,$2C
        .byte   $59,$00,$31,$20,$03,$58,$4C,$F6,$58,$FC,$09,$FA,$02,$3A,$59,$4C
        .byte   $F6,$28,$59,$FA,$03,$29,$FC,$09,$3A,$47,$24,$F6,$E1,$28,$58,$4C
        .byte   $2C,$59,$4C,$29,$3A,$53,$24,$F6,$E1,$28,$58,$4C,$2C,$59,$4C,$FA
        .byte   $02,$29,$00,$1D,$2A,$03,$92,$20,$47,$24,$F6,$55,$24,$20,$85,$20
        .byte   $81,$20,$0B,$5C,$03,$3A,$4B,$F6,$02,$3A,$80,$20,$0B,$1C,$02,$00
        .byte   $2B,$34,$03,$92,$20,$53,$24,$F6,$48,$24,$20,$85,$20,$81,$20,$0B
        .byte   $5C,$03,$3A,$8A,$20,$04,$2C,$58,$2C,$59,$3A,$8A,$20,$53,$32,$3A
        .byte   $53,$32,$F6,$04,$3A,$80,$20,$0B,$A0,$00,$00,$1D,$3E,$03,$92,$20
        .byte   $47,$24,$F6,$48,$24,$20,$85,$20,$81,$20,$0B,$5C,$03,$3A,$56,$F6
        .byte   $01,$3A,$80,$20,$0B,$1C,$02,$00,$09,$48,$03,$80,$20,$0B,$0C,$03
        .byte   $00,$10,$52,$03,$27,$53,$55,$42,$20,$78,$20,$AD,$8A,$99,$86,$80
        .byte   $00,$1B,$5C,$03,$81,$20,$0B,$66,$03,$3A,$46,$F6,$01,$3A,$4B,$F6
        .byte   $01,$3A,$8A,$20,$53,$2C,$58,$2C,$59,$3A,$83,$00,$25,$66,$03,$9C
        .byte   $20,$08,$3A,$58,$F6,$58,$4C,$FD,$09,$F9,$12,$0C,$00,$3A,$59,$F6
        .byte   $59,$4C,$FD,$09,$F9,$08,$3A,$A7,$20,$08,$2C,$58,$2C,$59,$3A,$83
        .byte   $00,$32,$70,$03,$AE,$20,$58,$42,$2C,$59,$4C,$FA,$02,$3A,$8B,$20
        .byte   $22,$20,$22,$3A,$52,$F6,$52,$F9,$02,$3A,$57,$F6,$57,$F9,$12,$0F
        .byte   $00,$3A,$AE,$20,$12,$0E,$00,$2C,$12,$17,$00,$3A,$8B,$20,$57,$3B
        .byte   $3A,$83,$00,$5D,$7A,$03,$A0,$3A,$49,$F6,$01,$3A,$46,$F6,$01,$3A
        .byte   $4B,$F6,$01,$3A,$43,$F6,$01,$3A,$52,$F6,$01,$3A,$5A,$F6,$12,$30
        .byte   $00,$3A,$AE,$20,$12,$0A,$00,$2C,$12,$15,$00,$3A,$8B,$22,$54,$49
        .byte   $4D,$45,$22,$3B,$54,$3A,$AE,$20,$07,$2C,$12,$16,$00,$3A,$8B,$22
        .byte   $48,$49,$2D,$53,$43,$4F,$52,$45,$22,$3B,$48,$3A,$AE,$20,$0A,$2C
        .byte   $12,$17,$00,$3A,$8B,$22,$53,$43,$4F,$52,$45,$22,$3B,$57,$3B,$00
        .byte   $05,$84,$03,$83,$00,$14,$8E,$03,$27,$63,$86,$2D,$20,$6B,$95,$87
        .byte   $20,$78,$20,$AD,$8A,$99,$86,$80,$00,$2C,$98,$03,$92,$20,$E5,$F6
        .byte   $12,$30,$02,$20,$85,$20,$58,$4C,$F6,$FA,$12,$1B,$00,$FD,$28,$58
        .byte   $4C,$F8,$02,$29,$3A,$81,$20,$0B,$66,$03,$3A,$A1,$20,$08,$3A,$BD
        .byte   $20,$0B,$76,$02,$00,$2D,$A2,$03,$92,$20,$E5,$F6,$12,$86,$01,$20
        .byte   $85,$20,$58,$4C,$F6,$FA,$12,$1B,$00,$FD,$28,$58,$4C,$F8,$01,$29
        .byte   $3A,$81,$20,$0B,$66,$03,$3A,$53,$F6,$53,$31,$3A,$BD,$20,$0B,$26
        .byte   $02,$00,$14,$AC,$03,$92,$20,$59,$4C,$F7,$12,$17,$00,$20,$85,$20
        .byte   $BD,$20,$0B,$94,$02,$00,$1B,$B6,$03,$92,$20,$58,$4C,$F7,$12,$1B
        .byte   $00,$20,$85,$20,$58,$4C,$F6,$01,$3A,$81,$20,$0B,$66,$03,$3A,$BD
        .byte   $00,$1B,$C0,$03,$92,$20,$58,$4C,$F8,$01,$20,$85,$20,$58,$4C,$F6
        .byte   $12,$1B,$00,$3A,$81,$20,$0B,$66,$03,$3A,$BD,$00,$18,$CA,$03,$92
        .byte   $20,$58,$42,$F7,$12,$1B,$00,$20,$85,$20,$58,$42,$F6,$01,$3A,$A1
        .byte   $08,$3A,$BD,$00,$18,$D4,$03,$92,$20,$58,$42,$F8,$01,$20,$85,$20
        .byte   $58,$42,$F6,$12,$1B,$00,$3A,$A1,$08,$3A,$BD,$00,$1D,$DE,$03,$92
        .byte   $20,$58,$4C,$F9,$02,$F7,$12,$1B,$00,$20,$85,$20,$58,$4C,$F6,$01
        .byte   $3A,$81,$20,$0B,$66,$03,$3A,$BD,$00,$1D,$E8,$03,$92,$20,$58,$4C
        .byte   $FA,$02,$F8,$01,$20,$85,$20,$58,$4C,$F6,$12,$1B,$00,$3A,$81,$20
        .byte   $0B,$66,$03,$3A,$BD,$00,$15,$F2,$03,$92,$20,$58,$F8,$12,$0C,$00
        .byte   $20,$85,$20,$58,$F6,$12,$E4,$00,$3A,$BD,$00,$15,$FC,$03,$92,$20
        .byte   $58,$F7,$12,$E4,$00,$20,$85,$20,$58,$F6,$12,$0C,$00,$3A,$BD,$00
        .byte   $05,$06,$04,$A2,$00,$00

        .segment "BANK9"
; ---- $F308-$FFF9（数据 3314 字节）
LF308:
        .byte   $13,$0A,$00,$27,$22,$41,$52,$45,$41,$20,$38,$38,$20,$56,$37,$2E
        .byte   $33,$22,$00,$37,$14,$00,$54,$F6,$4F,$F6,$55,$F6,$43,$F6,$48,$F6
        .byte   $44,$F6,$49,$F6,$4B,$F6,$4C,$F6,$4D,$F6,$4E,$F6,$50,$F6,$53,$F6
        .byte   $56,$F6,$58,$F6,$59,$F6,$4D,$41,$F6,$58,$30,$F6,$58,$31,$F6,$59
        .byte   $30,$F6,$59,$31,$F6,$50,$50,$F6,$51,$00,$1F,$1E,$00,$94,$20,$44
        .byte   $28,$02,$29,$2C,$56,$28,$02,$29,$2C,$46,$28,$09,$29,$2C,$53,$43
        .byte   $28,$02,$29,$2C,$53,$28,$02,$29,$00,$26,$28,$00,$4C,$F6,$0A,$3A
        .byte   $4D,$41,$F6,$03,$3A,$4E,$F6,$04,$3A,$56,$F6,$08,$3A,$43,$F6,$08
        .byte   $3A,$53,$28,$01,$29,$F6,$05,$3A,$53,$28,$02,$29,$F6,$05,$00,$1F
        .byte   $32,$00,$58,$30,$F6,$12,$46,$00,$3A,$59,$30,$F6,$12,$50,$00,$3A
        .byte   $58,$31,$F6,$12,$6E,$00,$3A,$59,$31,$F6,$12,$3C,$00,$00,$3D,$3C
        .byte   $00,$44,$28,$01,$29,$F6,$04,$3A,$56,$28,$01,$29,$F6,$03,$3A,$44
        .byte   $28,$02,$29,$F6,$08,$3A,$56,$28,$02,$29,$F6,$03,$3A,$46,$28,$01
        .byte   $29,$F6,$04,$3A,$46,$28,$02,$29,$F6,$04,$3A,$4D,$41,$F6,$4D,$41
        .byte   $F9,$02,$3A,$55,$F6,$4E,$3A,$4D,$F6,$02,$00,$32,$46,$00,$8A,$20
        .byte   $9A,$3A,$9F,$20,$02,$2C,$02,$3A,$A3,$22,$54,$31,$59,$32,$4D,$31
        .byte   $56,$39,$4F,$33,$52,$31,$22,$3A,$AF,$53,$20,$02,$2C,$12,$0D,$00
        .byte   $2C,$12,$30,$00,$2C,$12,$26,$00,$2C,$12,$12,$00,$00,$2A,$50,$00
        .byte   $8C,$20,$49,$F6,$12,$0F,$00,$20,$88,$20,$12,$12,$00,$3A,$41,$24
        .byte   $F6,$41,$24,$F9,$E1,$28,$49,$2C,$12,$16,$00,$29,$3A,$8D,$3A,$48
        .byte   $F6,$D9,$28,$41,$24,$29,$00,$09,$5A,$00,$81,$20,$0B,$2E,$04,$00
        .byte   $2C,$64,$00,$A0,$3A,$8C,$50,$F6,$01,$88,$12,$C8,$00,$3A,$AE,$D0
        .byte   $28,$12,$1B,$00,$29,$2C,$D0,$28,$12,$17,$00,$29,$3A,$8B,$DC,$28
        .byte   $12,$CD,$00,$29,$3B,$3A,$8D,$3A,$50,$F6,$01,$00,$05,$6E,$00,$27
        .byte   $00,$2D,$78,$00,$49,$F6,$50,$F9,$07,$3A,$92,$20,$46,$28,$50,$29
        .byte   $20,$85,$20,$9A,$20,$46,$28,$50,$29,$20,$81,$20,$0B,$9E,$02,$2C
        .byte   $0B,$BC,$02,$2C,$0B,$A8,$02,$3A,$80,$20,$0B,$96,$00,$00,$17,$82
        .byte   $00,$92,$20,$4D,$F6,$50,$20,$85,$20,$81,$20,$0B,$86,$01,$3A,$80
        .byte   $20,$0B,$96,$00,$00,$09,$8C,$00,$81,$20,$0B,$D2,$00,$00,$12,$96
        .byte   $00,$92,$20,$A1,$28,$49,$29,$F6,$01,$20,$85,$20,$B0,$20,$49,$00
        .byte   $09,$A0,$00,$81,$20,$0B,$12,$02,$00,$09,$AA,$00,$81,$20,$0B,$20
        .byte   $03,$00,$16,$B4,$00,$9A,$20,$46,$28,$09,$29,$F9,$02,$20,$81,$20
        .byte   $0B,$AC,$03,$2C,$0B,$10,$04,$00,$0F,$BE,$00,$50,$F6,$02,$FA,$50
        .byte   $3A,$80,$20,$0B,$78,$00,$00,$05,$C8,$00,$27,$00,$11,$D2,$00,$53
        .byte   $F6,$D5,$28,$50,$29,$3A,$54,$F6,$D6,$28,$50,$29,$00,$12,$DC,$00
        .byte   $92,$20,$54,$F3,$05,$20,$85,$20,$46,$28,$49,$29,$F6,$02,$00,$0F
        .byte   $E6,$00,$92,$20,$53,$F9,$54,$F6,$01,$20,$85,$20,$83,$00,$0C,$F0
        .byte   $00,$92,$53,$F6,$01,$85,$0B,$5E,$01,$00,$48,$FA,$00,$53,$F6,$FA
        .byte   $02,$FD,$28,$53,$F6,$09,$29,$FA,$04,$FD,$28,$53,$F6,$02,$29,$FA
        .byte   $06,$FD,$28,$53,$F6,$05,$29,$FA,$08,$FD,$28,$53,$F6,$03,$29,$FA
        .byte   $03,$FD,$28,$53,$F6,$0A,$29,$FA,$05,$FD,$28,$53,$F6,$06,$29,$FA
        .byte   $07,$FD,$28,$53,$F6,$07,$29,$FA,$09,$FD,$28,$53,$F6,$12,$0A,$00
        .byte   $29,$00,$0A,$04,$01,$51,$F6,$44,$28,$50,$29,$00,$0F,$0E,$01,$92
        .byte   $20,$53,$F6,$51,$20,$85,$20,$0B,$5E,$01,$00,$0F,$18,$01,$92,$20
        .byte   $51,$F7,$05,$20,$85,$20,$0B,$36,$01,$00,$19,$22,$01,$92,$20,$28
        .byte   $53,$F7,$51,$20,$F1,$20,$53,$F5,$51,$F9,$05,$29,$20,$85,$20,$0B
        .byte   $40,$01,$00,$09,$2C,$01,$80,$20,$0B,$4A,$01,$00,$18,$36,$01,$92
        .byte   $20,$28,$53,$F4,$51,$FA,$05,$20,$F1,$20,$53,$F8,$51,$29,$85,$20
        .byte   $0B,$4A,$01,$00,$0F,$40,$01,$51,$F6,$51,$F9,$02,$3A,$80,$20,$0B
        .byte   $54,$01,$00,$09,$4A,$01,$51,$F6,$51,$FA,$02,$00,$2D,$54,$01,$50
        .byte   $50,$F6,$FA,$02,$FD,$28,$51,$F7,$09,$29,$FA,$09,$FD,$28,$51,$F8
        .byte   $02,$29,$3A,$51,$F6,$FA,$51,$FD,$28,$50,$50,$F6,$01,$29,$F9,$50
        .byte   $50,$3A,$44,$28,$50,$29,$F6,$51,$00,$28,$5E,$01,$92,$20,$54,$20
        .byte   $F1,$05,$20,$85,$20,$92,$20,$46,$28,$49,$29,$20,$85,$20,$56,$28
        .byte   $50,$29,$F6,$56,$FA,$56,$28,$50,$29,$3A,$46,$28,$49,$29,$F6,$01
        .byte   $00,$26,$68,$01,$AA,$20,$A1,$28,$50,$29,$F6,$8A,$20,$28,$50,$F9
        .byte   $09,$2C,$44,$28,$50,$29,$2C,$56,$28,$50,$29,$2C,$12,$FF,$00,$2C
        .byte   $02,$29,$3A,$A1,$20,$50,$00,$4A,$72,$01,$92,$20,$54,$F7,$08,$20
        .byte   $85,$20,$AA,$20,$A1,$28,$49,$29,$F6,$8A,$28,$12,$0C,$00,$2C,$44
        .byte   $28,$50,$29,$2C,$02,$2C,$12,$50,$00,$2C,$01,$2C,$50,$29,$3A,$A7
        .byte   $20,$49,$2C,$28,$D7,$28,$50,$29,$F9,$05,$29,$FB,$12,$00,$01,$2C
        .byte   $28,$D8,$28,$50,$29,$F9,$05,$29,$FB,$12,$00,$01,$3A,$A1,$20,$49
        .byte   $00,$05,$7C,$01,$83,$00,$05,$86,$01,$27,$00,$13,$90,$01,$4B,$F6
        .byte   $4B,$FA,$02,$3A,$92,$20,$4B,$F7,$01,$20,$85,$20,$83,$00,$33,$9A
        .byte   $01,$54,$F6,$01,$3A,$44,$F6,$01,$3A,$58,$F6,$28,$D7,$28,$02,$FA
        .byte   $50,$29,$FA,$D7,$28,$50,$29,$29,$FC,$12,$0D,$00,$3A,$59,$F6,$28
        .byte   $D8,$28,$02,$FA,$50,$29,$FA,$D8,$28,$50,$29,$29,$FC,$12,$0D,$00
        .byte   $00,$0F,$A4,$01,$92,$20,$58,$F7,$01,$20,$85,$20,$44,$F6,$04,$00
        .byte   $0F,$AE,$01,$92,$20,$58,$F8,$01,$20,$85,$20,$44,$F6,$08,$00,$11
        .byte   $B8,$01,$92,$20,$59,$F7,$01,$20,$85,$20,$44,$F6,$44,$F9,$06,$00
        .byte   $1F,$C2,$01,$92,$20,$59,$F8,$01,$20,$85,$20,$44,$F6,$44,$F9,$02
        .byte   $3A,$92,$20,$58,$F8,$01,$20,$85,$20,$44,$F6,$12,$10,$00,$00,$13
        .byte   $CC,$01,$92,$20,$58,$20,$F1,$20,$59,$20,$85,$20,$44,$F6,$44,$FC
        .byte   $03,$00,$29,$D6,$01,$53,$F6,$03,$3A,$4F,$F6,$44,$FA,$44,$28,$50
        .byte   $29,$3A,$92,$28,$01,$F8,$4F,$20,$F1,$20,$4F,$F8,$06,$29,$F0,$20
        .byte   $4F,$F8,$FA,$04,$20,$85,$20,$53,$F6,$02,$00,$0F,$E0,$01,$92,$20
        .byte   $4F,$F6,$01,$20,$85,$20,$53,$F6,$01,$00,$17,$EA,$01,$92,$20,$56
        .byte   $28,$02,$FA,$50,$29,$F3,$56,$28,$50,$29,$20,$85,$20,$54,$F6,$05
        .byte   $00,$1F,$F4,$01,$92,$20,$A1,$28,$49,$29,$F6,$01,$20,$F0,$20,$D0
        .byte   $28,$12,$0A,$00,$29,$F6,$01,$20,$85,$20,$54,$F6,$54,$F9,$09,$00
        .byte   $15,$FE,$01,$92,$20,$58,$FD,$58,$F9,$59,$FD,$59,$F7,$12,$C8,$00
        .byte   $20,$85,$20,$83,$00,$10,$08,$02,$4B,$F6,$D0,$28,$4C,$29,$3A,$80
        .byte   $20,$0B,$DC,$00,$00,$05,$12,$02,$27,$00,$22,$1C,$02,$44,$F6,$E3
        .byte   $28,$01,$29,$3A,$92,$20,$44,$F7,$01,$20,$F1,$20,$44,$F8,$07,$20
        .byte   $85,$20,$4F,$F6,$01,$3A,$81,$20,$0B,$4E,$02,$00,$22,$26,$02,$44
        .byte   $F6,$E3,$28,$02,$29,$3A,$92,$20,$44,$F7,$02,$20,$F1,$20,$44,$F8
        .byte   $07,$20,$85,$20,$4F,$F6,$02,$3A,$81,$20,$0B,$4E,$02,$00,$22,$30
        .byte   $02,$44,$F6,$E3,$28,$07,$29,$3A,$92,$20,$44,$F7,$01,$20,$F1,$20
        .byte   $44,$F8,$07,$20,$85,$20,$4F,$F6,$07,$3A,$81,$20,$0B,$62,$02,$00
        .byte   $29,$3A,$02,$44,$F6,$E3,$28,$08,$29,$3A,$92,$20,$44,$F6,$01,$20
        .byte   $F0,$20,$28,$44,$F7,$02,$20,$F1,$20,$44,$F8,$07,$29,$85,$20,$4F
        .byte   $F6,$08,$3A,$81,$20,$0B,$62,$02,$00,$05,$44,$02,$83,$00,$6F,$4E
        .byte   $02,$58,$F6,$CA,$28,$D7,$28,$4F,$29,$FA,$D7,$28,$44,$29,$29,$3A
        .byte   $59,$F6,$CA,$28,$D8,$28,$4F,$29,$FA,$D8,$28,$44,$29,$29,$3A,$92
        .byte   $20,$58,$F8,$12,$0A,$00,$20,$F1,$20,$59,$F8,$12,$0A,$00,$20,$85
        .byte   $20,$92,$20,$A1,$28,$4F,$29,$20,$F1,$20,$A1,$28,$44,$29,$20,$F1
        .byte   $20,$46,$28,$44,$29,$F6,$01,$20,$F1,$20,$46,$28,$4F,$29,$F6,$01
        .byte   $20,$85,$20,$46,$28,$4F,$29,$F6,$02,$3A,$46,$28,$44,$29,$F6,$02
        .byte   $3A,$92,$20,$44,$F7,$02,$20,$85,$20,$0B,$8A,$02,$00,$05,$58,$02
        .byte   $83,$00,$5D,$62,$02,$58,$F6,$CA,$28,$D7,$28,$4F,$29,$FA,$D7,$28
        .byte   $44,$29,$FA,$05,$29,$3A,$59,$F6,$CA,$28,$D8,$28,$4F,$29,$FA,$D8
        .byte   $28,$44,$29,$FA,$05,$29,$3A,$92,$20,$58,$F8,$09,$20,$F1,$20,$59
        .byte   $F8,$09,$20,$85,$20,$92,$20,$A1,$28,$4F,$29,$20,$F1,$20,$A1,$28
        .byte   $44,$29,$20,$F1,$20,$46,$28,$44,$29,$F6,$01,$20,$85,$20,$B0,$20
        .byte   $4F,$3A,$46,$28,$44,$29,$F6,$02,$3A,$80,$20,$0B,$76,$02,$00,$05
        .byte   $6C,$02,$83,$00,$21,$76,$02,$92,$20,$44,$F8,$03,$20,$85,$20,$53
        .byte   $43,$28,$4F,$FA,$07,$29,$F6,$53,$43,$28,$4F,$FA,$07,$29,$F9,$12
        .byte   $0A,$00,$3A,$83,$00,$1C,$80,$02,$53,$43,$28,$4F,$FA,$07,$29,$F6
        .byte   $53,$43,$28,$4F,$FA,$07,$29,$F9,$02,$F9,$46,$28,$09,$29,$FD,$05
        .byte   $00,$2E,$8A,$02,$AA,$20,$A1,$28,$44,$29,$F6,$8A,$28,$12,$0A,$00
        .byte   $2C,$D0,$28,$0A,$29,$2C,$D0,$28,$04,$29,$F9,$05,$2C,$12,$0A,$00
        .byte   $29,$3A,$A1,$20,$44,$3A,$A3,$22,$4F,$34,$43,$22,$3A,$83,$00,$05
        .byte   $94,$02,$27,$00,$50,$9E,$02,$46,$28,$50,$29,$F6,$03,$3A,$AA,$20
        .byte   $A1,$28,$50,$29,$F6,$8A,$28,$12,$0A,$00,$2C,$44,$28,$50,$29,$2C
        .byte   $56,$28,$50,$29,$2C,$12,$46,$00,$FC,$56,$28,$50,$29,$2C,$01,$29
        .byte   $3A,$A1,$20,$50,$3A,$A3,$22,$4F,$22,$F9,$DD,$28,$50,$F9,$04,$29
        .byte   $F9,$22,$47,$47,$22,$3A,$53,$28,$50,$29,$F6,$53,$28,$50,$29,$FA
        .byte   $02,$3A,$83,$00,$32,$A8,$02,$92,$20,$50,$F6,$4D,$20,$F0,$20,$D6
        .byte   $28,$50,$29,$F9,$D5,$28,$50,$29,$20,$85,$20,$46,$28,$50,$29,$F6
        .byte   $01,$3A,$A7,$20,$50,$2C,$58,$30,$F9,$50,$FD,$58,$31,$2C,$59,$30
        .byte   $F9,$50,$FD,$59,$31,$00,$05,$B2,$02,$83,$00,$0E,$BC,$02,$92,$20
        .byte   $A1,$28,$50,$29,$20,$85,$20,$83,$00,$0E,$C6,$02,$46,$28,$50,$29
        .byte   $F6,$04,$3A,$B0,$20,$50,$00,$1D,$D0,$02,$92,$20,$53,$43,$28,$01
        .byte   $29,$F7,$48,$20,$F1,$20,$4D,$F3,$01,$20,$85,$20,$48,$F6,$53,$43
        .byte   $28,$01,$29,$00,$1D,$DA,$02,$92,$20,$53,$43,$28,$02,$29,$F7,$48
        .byte   $20,$F1,$20,$4D,$F3,$02,$20,$85,$20,$48,$F6,$53,$43,$28,$02,$29
        .byte   $00,$0F,$E4,$02,$92,$20,$4C,$F8,$02,$20,$85,$20,$4C,$F6,$06,$00
        .byte   $51,$EE,$02,$AE,$20,$01,$2C,$12,$14,$00,$3A,$8B,$22,$53,$43,$4F
        .byte   $52,$45,$20,$22,$2C,$53,$43,$28,$01,$29,$2C,$53,$43,$28,$02,$29
        .byte   $2C,$22,$4C,$45,$56,$45,$4C,$22,$3A,$8B,$22,$53,$48,$49,$50,$53
        .byte   $22,$2C,$53,$28,$01,$29,$2C,$53,$28,$02,$29,$2C,$4C,$3B,$22,$20
        .byte   $22,$3A,$8B,$22,$48,$49,$2D,$53,$43,$4F,$52,$45,$22,$2C,$48,$2C
        .byte   $00,$10,$F8,$02,$92,$20,$53,$28,$50,$29,$F7,$01,$20,$85,$20,$83
        .byte   $00,$1E,$02,$03,$92,$20,$4D,$F6,$50,$20,$85,$20,$4C,$F6,$4C,$FA
        .byte   $02,$3A,$53,$28,$50,$29,$F6,$05,$3A,$80,$20,$0B,$E4,$02,$00,$23
        .byte   $0C,$03,$92,$20,$4D,$F6,$FA,$02,$20,$85,$20,$4D,$F6,$50,$3A,$8B
        .byte   $20,$50,$3B,$22,$20,$43,$4F,$4D,$22,$3A,$A3,$22,$45,$45,$22,$3A
        .byte   $83,$00,$34,$16,$03,$AE,$20,$12,$0A,$00,$2C,$12,$0A,$00,$3A,$8B
        .byte   $20,$22,$47,$41,$4D,$45,$20,$4F,$56,$45,$52,$22,$3A,$A3,$22,$4F
        .byte   $31,$43,$35,$43,$44,$45,$44,$36,$43,$34,$43,$35,$22,$3A,$8E,$20
        .byte   $12,$C8,$00,$3A,$82,$00,$05,$20,$03,$27,$00,$19,$2A,$03,$58,$F6
        .byte   $D7,$28,$50,$29,$FC,$09,$FA,$02,$3A,$59,$F6,$D8,$28,$50,$29,$FC
        .byte   $09,$FA,$03,$00,$25,$34,$03,$92,$20,$58,$F8,$01,$20,$F0,$20,$58
        .byte   $F7,$12,$1B,$00,$20,$F0,$20,$59,$F8,$01,$20,$F0,$20,$59,$F7,$12
        .byte   $17,$00,$20,$85,$20,$0B,$70,$03,$00,$15,$3E,$03,$92,$20,$E1,$28
        .byte   $58,$2C,$59,$29,$F3,$22,$CF,$22,$85,$20,$0B,$70,$03,$00,$11,$48
        .byte   $03,$53,$F6,$CB,$28,$E1,$28,$58,$2C,$59,$2C,$02,$29,$29,$00,$11
        .byte   $52,$03,$92,$20,$53,$F6,$50,$F9,$02,$20,$85,$20,$0B,$70,$03,$00
        .byte   $15,$5C,$03,$92,$20,$46,$28,$50,$29,$F6,$01,$20,$85,$20,$46,$28
        .byte   $50,$29,$F6,$02,$00,$05,$66,$03,$27,$00,$10,$70,$03,$92,$20,$A1
        .byte   $28,$49,$29,$F6,$01,$20,$85,$20,$83,$00,$21,$7A,$03,$58,$F6,$28
        .byte   $D7,$28,$49,$29,$FA,$12,$0C,$00,$29,$FC,$09,$3A,$59,$F6,$28,$D8
        .byte   $28,$49,$29,$FA,$12,$14,$00,$29,$FC,$09,$00,$23,$84,$03,$92,$20
        .byte   $58,$F8,$01,$20,$F0,$20,$58,$F7,$12,$1B,$00,$20,$F0,$20,$59,$F8
        .byte   $01,$20,$F0,$20,$59,$F7,$12,$17,$00,$20,$85,$20,$83,$00,$14,$8E
        .byte   $03,$92,$20,$E1,$28,$58,$2C,$59,$29,$F3,$22,$CF,$22,$20,$85,$20
        .byte   $83,$00,$1D,$98,$03,$53,$F6,$CB,$28,$E1,$28,$58,$2C,$59,$2C,$02
        .byte   $29,$29,$3A,$92,$20,$53,$F6,$50,$F9,$02,$20,$85,$20,$83,$00,$16
        .byte   $A2,$03,$B0,$20,$49,$3A,$AE,$20,$58,$2C,$59,$3A,$8B,$20,$22,$20
        .byte   $22,$3B,$3A,$83,$00,$05,$AC,$03,$27,$00,$23,$B6,$03,$8C,$20,$49
        .byte   $F6,$03,$20,$88,$20,$4D,$41,$3A,$92,$20,$A1,$28,$49,$29,$F0,$20
        .byte   $46,$28,$49,$29,$F6,$03,$20,$85,$20,$0B,$D4,$03,$00,$1A,$C0,$03
        .byte   $92,$20,$46,$28,$49,$29,$F6,$02,$20,$85,$20,$81,$20,$0B,$DE,$03
        .byte   $3A,$80,$20,$0B,$D4,$03,$00,$28,$CA,$03,$AA,$20,$A1,$28,$49,$29
        .byte   $F6,$8A,$20,$28,$43,$2C,$D0,$28,$0A,$29,$2C,$D0,$28,$04,$29,$F9
        .byte   $43,$FC,$03,$2C,$12,$50,$00,$2C,$01,$29,$3A,$A1,$20,$49,$00,$07
        .byte   $D4,$03,$8D,$3A,$83,$00,$05,$DE,$03,$27,$00,$2A,$E8,$03,$46,$28
        .byte   $49,$29,$F6,$01,$3A,$B0,$20,$49,$3A,$A7,$20,$49,$2C,$12,$78,$00
        .byte   $2C,$12,$78,$00,$3A,$92,$20,$55,$F8,$4D,$41,$20,$85,$20,$46,$28
        .byte   $49,$29,$F6,$03,$00,$21,$F2,$03,$55,$F6,$55,$FA,$02,$3A,$92,$20
        .byte   $55,$F6,$01,$20,$85,$20,$46,$28,$03,$29,$F6,$01,$3A,$46,$28,$09
        .byte   $29,$F6,$02,$3A,$83,$00,$63,$FC,$03,$92,$20,$55,$F8,$01,$20,$85
        .byte   $20,$55,$F6,$4E,$3A,$46,$28,$03,$29,$F6,$01,$3A,$46,$28,$04,$29
        .byte   $F6,$01,$3A,$46,$28,$05,$29,$F6,$01,$3A,$46,$28,$06,$29,$F6,$01
        .byte   $3A,$46,$28,$09,$29,$F6,$01,$3A,$43,$F6,$43,$FA,$03,$3A,$4C,$F6
        .byte   $4C,$FA,$02,$3A,$92,$20,$43,$F8,$03,$20,$85,$20,$43,$F6,$08,$3A
        .byte   $92,$20,$4D,$F7,$FA,$02,$20,$85,$20,$53,$28,$02,$FA,$4D,$29,$F6
        .byte   $53,$28,$02,$FA,$4D,$29,$F9,$02,$00,$05,$06,$04,$83,$00,$15,$10
        .byte   $04,$92,$20,$46,$28,$03,$29,$20,$85,$20,$49,$F6,$03,$3A,$80,$0B
        .byte   $DE,$03,$00,$32,$1A,$04,$92,$20,$A1,$28,$03,$29,$F6,$01,$20,$85
        .byte   $20,$AA,$20,$A1,$28,$03,$29,$F6,$8A,$28,$03,$2C,$D0,$28,$09,$29
        .byte   $F9,$02,$2C,$D0,$28,$04,$29,$F9,$02,$2C,$12,$0A,$00,$2C,$01,$29
        .byte   $3A,$A1,$20,$03,$00,$05,$24,$04,$83,$00,$05,$2E,$04,$27,$00,$0B
        .byte   $38,$04,$A0,$3A,$84,$20,$0B,$2E,$04,$00,$25,$42,$04,$8C,$50,$F6
        .byte   $01,$88,$12,$C8,$00,$3A,$AE,$D0,$28,$12,$1B,$00,$29,$2C,$D0,$28
        .byte   $12,$17,$00,$29,$3A,$8B,$DC,$28,$12,$CD,$00,$29,$3A,$8D,$00,$17
        .byte   $4C,$04,$AE,$09,$2C,$04,$3A,$8B,$22,$53,$54,$41,$52,$20,$4B,$49
        .byte   $4C,$4C,$45,$52,$22,$00,$4C,$56,$04,$8C,$20,$50,$F6,$01,$20,$88
        .byte   $20,$02,$3A,$AE,$20,$12,$0A,$00,$2C,$12,$0A,$00,$F9,$50,$FD,$09
        .byte   $3A,$AA,$20,$A1,$28,$50,$29,$F6,$8A,$28,$50,$F9,$09,$2C,$44,$28
        .byte   $50,$29,$2C,$03,$2C,$12,$1E,$00,$29,$3A,$A7,$20,$50,$2C,$58,$30
        .byte   $F9,$50,$FD,$58,$31,$2C,$59,$30,$F9,$50,$FD,$59,$31,$3A,$A1,$20
        .byte   $50,$00,$10,$60,$04,$92,$20,$A1,$28,$50,$29,$20,$85,$20,$0B,$60
        .byte   $04,$00,$2C,$6A,$04,$93,$20,$41,$24,$3A,$8C,$20,$49,$F6,$02,$20
        .byte   $88,$20,$CE,$28,$41,$24,$29,$3A,$8B,$20,$D4,$28,$41,$24,$2C,$49
        .byte   $2C,$02,$29,$3B,$3A,$A3,$22,$43,$22,$3A,$8D,$3A,$8D,$00,$64,$74
        .byte   $04,$8C,$20,$50,$F6,$03,$20,$88,$20,$04,$3A,$AE,$20,$12,$0A,$00
        .byte   $2C,$12,$11,$00,$F9,$50,$3A,$93,$20,$41,$24,$3A,$8C,$20,$49,$F6
        .byte   $02,$20,$88,$20,$CE,$28,$41,$24,$29,$3A,$8B,$20,$D4,$28,$41,$24
        .byte   $2C,$49,$2C,$02,$29,$3B,$3A,$A3,$22,$43,$22,$3A,$8D,$3A,$8D,$3A
        .byte   $AE,$02,$2C,$12,$16,$00,$3A,$8B,$22,$7B,$6F,$87,$20,$A4,$20,$60
        .byte   $6E,$A7,$7E,$6C,$65,$28,$59,$2F,$4E,$29,$22,$3B,$3A,$4D,$F6,$FA
        .byte   $02,$00,$17,$7E,$04,$41,$24,$F6,$DE,$28,$01,$29,$3A,$92,$41,$24
        .byte   $F6,$22,$4E,$22,$85,$4D,$F6,$02,$00,$0A,$88,$04,$8E,$12,$50,$00
        .byte   $3A,$83,$00,$2E,$92,$04,$91,$20,$22,$50,$4C,$41,$59,$45,$52,$20
        .byte   $31,$22,$2C,$22,$50,$4C,$41,$59,$45,$52,$20,$32,$22,$2C,$22,$20
        .byte   $20,$20,$4F,$52,$22,$2C,$22,$43,$4F,$4D,$50,$55,$54,$45,$52,$22
        .byte   $00,$00

        .segment "BANK10"
; ---- 现场缓冲：7 页（零页与 $0200-$07FF），由 platform.s 里的现场换入换出使用
        .res    1792, $00

