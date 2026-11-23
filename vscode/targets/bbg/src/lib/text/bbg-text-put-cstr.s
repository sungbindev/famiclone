; ------------------------------------------------------------------------
; 名称：bbg-text-put-cstr.s
; 说明：显示以 0 结尾的字符串。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：逐个字符调用 DOS 的显示字符功能，$0D、$0A 会换行；不保留 A、X、Y。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr

        .code

; bbg_text_put_cstr
;   说明：显示以 0 结尾的字符串。
;   入口：A（低）、X（高）= 字符串地址
;   出口：无
bbg_text_put_cstr:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
@next:  ldy     #0
        lda     (bbgi_ptr),y
        beq     @done
        tax
        lda     #DOS_DISPLAY_CHAR
        jsr     DOS_ENTRY
        inc     bbgi_ptr
        bne     @next
        inc     bbgi_ptr + 1
        bne     @next
@done:  rts
