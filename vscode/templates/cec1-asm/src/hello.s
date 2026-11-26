; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：中华学习机 CEC-I 最小汇编程序：清屏，显示两行英文，按任意键后回到 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-26
; 备注：入口是导出的 _main，由启动代码调用，返回后启动代码跳到 DOS 的热启动入口回到命令行；
;       显示与键盘用运行库 cec1.lib 的函数，符号在 cec1.inc。
; ------------------------------------------------------------------------
        .include "cec1.inc"
        .export _main

        .segment "RODATA"

message:
        .byte "HELLO, CEC-I!", $0D
        .byte "PRESS ANY KEY TO EXIT.", $0D
        .byte 0

        .segment "CODE"

_main:
        jsr cec1_home
        lda #<message
        ldx #>message
        jsr cec1_put_cstr
        jsr cec1_get_key
        rts                             ; 返回启动代码，它会回到 DOS 的命令行
