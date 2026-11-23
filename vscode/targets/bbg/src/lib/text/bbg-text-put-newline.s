; ------------------------------------------------------------------------
; 名称：bbg-text-put-newline.s
; 说明：显示回车与换行。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：调用 DOS 的显示字符功能依次显示 $0D、$0A；不保留 A、X、Y。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_text_put_newline
;   说明：显示回车与换行。
;   入口：无
;   出口：无
bbg_text_put_newline:
        ldx     #$0D
        lda     #DOS_DISPLAY_CHAR
        jsr     DOS_ENTRY
        ldx     #$0A
        lda     #DOS_DISPLAY_CHAR
        jmp     DOS_ENTRY
