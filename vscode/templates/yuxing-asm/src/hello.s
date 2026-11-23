; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：裕兴学习机 DOS 最小汇编程序：用 ROM 的显示功能在屏幕上显示两行英文并停住。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：入口是导出的 _main，由启动代码调用；程序不能返回 DOS，要退出只能复位。
;       显示用 ROM 的三个功能：初始化文字屏、清屏、显示字符串，见 yuxing.inc。
; ------------------------------------------------------------------------
        .include "yuxing.inc"
        .export _main

        .segment "RODATA"

message:
        .byte STR_LOCATE, 2, 10
        .byte "Hello, Yuxing!"
        .byte STR_LOCATE, 2, 12
        .byte "Reset the machine to exit."
        .byte STR_END

        .segment "CODE"

_main:
        jsr BIOS_INIT_TEXT
        jsr BIOS_CLEAR
        lda #<message
        ldy #>message
        jsr BIOS_PRINT
        rts                             ; 返回启动代码，它会停在原地
