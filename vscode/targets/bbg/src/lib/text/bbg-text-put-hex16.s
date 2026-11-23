; ------------------------------------------------------------------------
; 名称：bbg-text-put-hex16.s
; 说明：以四位大写十六进制显示一个 16 位数。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：先显示高字节再显示低字节；不保留 A、X、Y。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbg_text_put_hex8

        .code

; bbg_text_put_hex16
;   说明：以四位大写十六进制显示一个 16 位数。
;   入口：A（低）、X（高）= 数值
;   出口：无
bbg_text_put_hex16:
        pha
        txa
        jsr     bbg_text_put_hex8
        pla
        jmp     bbg_text_put_hex8
