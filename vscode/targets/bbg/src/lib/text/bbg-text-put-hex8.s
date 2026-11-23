; ------------------------------------------------------------------------
; 名称：bbg-text-put-hex8.s
; 说明：以两位大写十六进制显示一个字节。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：逐个字符调用 DOS 的显示字符功能；不保留 A、X、Y。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_text_put_hex8
;   说明：以两位大写十六进制显示一个字节。
;   入口：A = 字节
;   出口：无
bbg_text_put_hex8:
        pha
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        jsr     put_nibble
        pla
        and     #$0F

; 显示 A 的低 4 位对应的十六进制字符
put_nibble:
        cmp     #10
        bcc     @digit
        adc     #6                      ; C=1，实际加 7，从 '9' 之后跳到 'A'
@digit: adc     #'0'
        tax
        lda     #DOS_DISPLAY_CHAR
        jmp     DOS_ENTRY
