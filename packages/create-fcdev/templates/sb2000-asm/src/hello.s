; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：SB-DOS 最小汇编程序：显示一行英文与一行中文，等待按键后返回 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-10
; 备注：入口是导出的 _main，由启动代码调用，返回后启动代码负责回到 DOS；
;       _sb_puts、_sb_getkey 来自目标自带的 sb2000 库，参数用 cc65 的 fastcall：指针在 A（低）与 X（高）；
;       中文用 GB2312 编码，字符码不小于 $A1 的两个字节显示一个汉字。
; ------------------------------------------------------------------------
        .export _main
        .import _sb_puts, _sb_getkey

        .segment "RODATA"

message:
        .asciiz "Hello, SB2000!"

; GB2312：你好，小霸王！
message_cn:
        .byte $C4, $E3, $BA, $C3, $A3, $AC, $D0, $A1, $B0, $D4, $CD, $F5, $A3, $A1, $00

        .segment "CODE"

_main:
        lda #<message
        ldx #>message
        jsr _sb_puts            ; 从行首显示并换行

        lda #<message_cn
        ldx #>message_cn
        jsr _sb_puts

        jsr _sb_getkey          ; 等待按键，DOS 在程序返回后会清屏
        rts
