; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：步步高学生电脑 BBGDOS 最小汇编程序：用 DOS 的显示字符功能显示两行英文，按任意键后回到 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：入口是导出的 _main，由启动代码调用，返回后启动代码调用 DOS 的程序结束功能回到命令行；
;       DOS 调用不保留寄存器，所以字符串的位置放在内存变量里，每显示一个字符就加 1。
; ------------------------------------------------------------------------
        .include "bbg.inc"
        .export _main

        .segment "RODATA"

message:
        .byte "Hello, BBGDOS!", $0D, $0A
        .byte "Press any key to exit.", $0D, $0A
        .byte 0

        .segment "BSS"

index:  .res 1                          ; 下一个要显示的字符在 message 里的位置

        .segment "CODE"

_main:
        lda #0
        sta index                       ; BSS 不会被清零，要自己赋初值
next:   ldy index
        lda message,y
        beq wait                        ; 0 结束
        tax
        lda #DOS_DISPLAY_CHAR
        jsr DOS_ENTRY
        inc index
        bne next
wait:   lda #DOS_READ_KEY_NO_ECHO
        jsr DOS_ENTRY
        rts                             ; 返回启动代码，它会回到 DOS
