; ------------------------------------------------------------------------
; 名称：bbg-text-put-dec16.s
; 说明：以十进制显示一个无符号 16 位数，不补前导零。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：用 10000、1000、100、10、1 逐位相减；每显示一位都要调用 DOS，会破坏 X、Y，所以位置与数值放在零页。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr, bbgi_flag, bbgi_index

        .rodata

power_low:      .byte   <10000, <1000, <100, <10, <1
power_high:     .byte   >10000, >1000, >100, >10, >1

        .code

; bbg_text_put_dec16
;   说明：以十进制显示一个无符号 16 位数，不补前导零，0 显示为 "0"。
;   入口：A（低）、X（高）= 数值
;   出口：无
bbg_text_put_dec16:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        lda     #0
        sta     bbgi_flag               ; 已经显示过非零位
        sta     bbgi_index
@digit: ldy     #0                      ; Y = 当前位的值
        ldx     bbgi_index
@sub:   lda     bbgi_ptr
        sec
        sbc     power_low,x
        pha
        lda     bbgi_ptr + 1
        sbc     power_high,x
        bcc     @undo
        sta     bbgi_ptr + 1
        pla
        sta     bbgi_ptr
        iny
        bne     @sub
@undo:  pla
        tya
        ora     bbgi_flag
        bne     @show                   ; 前面已有非零位或本位非零
        ldx     bbgi_index
        cpx     #4
        bne     @skip                   ; 前导零不显示，最后一位总要显示
@show:  sta     bbgi_flag
        tya
        clc
        adc     #'0'
        tax
        lda     #DOS_DISPLAY_CHAR
        jsr     DOS_ENTRY
@skip:  inc     bbgi_index
        lda     bbgi_index
        cmp     #5
        bne     @digit
        rts
