; ------------------------------------------------------------------------
; 名称：cec1-put-cstr.s
; 说明：输出以 0 结尾的字符串。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-26
; 备注：逐个字符调用监控程序的 COUT（位 7 置 1），$0D 是回车换行；字符串长度不限，指针会进位到高字节；
;       不保留 A、X、Y。
; ------------------------------------------------------------------------
        .include        "cec1.inc"

        .code

; cec1_put_cstr
;   说明：输出以 0 结尾的字符串。
;   入口：A（低）、X（高）= 字符串地址
;   出口：无
cec1_put_cstr:
        sta     cec1i_ptr
        stx     cec1i_ptr + 1
@next:  ldy     #0
        lda     (cec1i_ptr),y
        beq     @done
        ora     #$80
        jsr     MON_COUT
        inc     cec1i_ptr
        bne     @next
        inc     cec1i_ptr + 1
        bne     @next
@done:  rts
