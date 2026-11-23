; ------------------------------------------------------------------------
; 名称：bbg-dos-read-key-clear.s
; 说明：DOS 调用 8：先清空键盘缓冲区，再执行指定的键盘功能。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：功能号经零页 $FF 传给 DOS，$FF 属 DOS 的零页区，库不另行保存。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_dos_read_key_clear
;   说明：DOS 调用 8：先清空键盘缓冲区，再执行指定的键盘功能。
;   入口：A = 要执行的 DOS 功能号（1、6 或 7）；功能 6 时 X 为缓冲区地址高字节、Y 为低字节，缓冲区格式同 bbg_dos_get_line
;   出口：同所执行的功能
bbg_dos_read_key_clear:
        sta     $FF
        lda     #DOS_READ_KB_CLEAR
        jmp     DOS_ENTRY
