; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：SB-DOS 最小汇编程序：显示一行英文与一行中文，等待按键后返回 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-10
; 备注：入口是导出的 _main，由启动代码调用，返回后启动代码负责回到 DOS；
;       _sb_puts、_sb_getkey 来自目标自带的 sb2000 库，参数用 cc65 的 fastcall：指针在 A（低）与 X（高）；
;       源文件用 UTF-8 保存，字符串里的汉字构建时自动转成学习机用的 GB2312。
; ------------------------------------------------------------------------
        .export _main
        .import _sb_puts, _sb_getkey

        .segment "RODATA"

message:
        .asciiz "Hello, SB2000!"

message_cn:
        .asciiz "你好，小霸王！"

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
