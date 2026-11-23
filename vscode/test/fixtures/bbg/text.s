; ------------------------------------------------------------------------
; 名称：text.s
; 说明：测试 bbg.lib 的文字输出函数：用光标前进的列数核对长度，用屏幕截图核对内容（与 DOS 直接显示同样的字符串比较）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：每组测试先清屏，用库函数显示一次并停靠（脚本截图），再清屏，用 DOS 直接显示等价的字符串并停靠；
;       两张截图应相同。共 6 组，12 个停靠点。
; ------------------------------------------------------------------------

        .include        "bbg-test.inc"
        .export         _main

        .bss

column:         .res    1

        .rodata

str_1234:       .byte   "1234", 0
str_0:          .byte   "0", 0
str_65535:      .byte   "65535", 0
str_100:        .byte   "100", 0
str_ff:         .byte   "FF", 0
str_beef:       .byte   "BEEF", 0
str_lines:      .byte   "A", $0D, $0A, "B", 0
str_a:          .byte   "A", 0
str_b:          .byte   "B", 0

        .code

; 清屏并记下光标列
begin:
        jsr     bbg_dos_clear_screen
        jsr     bbg_bios_get_cursor
        sty     column
        rts

; 光标相对 begin 前进的列数
advanced:
        jsr     bbg_bios_get_cursor
        tya
        sec
        sbc     column
        rts

_main:
        jsr     bbt_init

        ; 第 1 组：十进制 1234，前进 4 列
        jsr     begin
        lda     #<1234
        ldx     #>1234
        jsr     bbg_text_put_dec16
        jsr     advanced
        CHECK_A 1, 4
        jsr     bbt_sync
        jsr     begin
        lda     #<str_1234
        ldx     #>str_1234
        jsr     bbg_text_put_cstr
        jsr     bbt_sync

        ; 第 2 组：十进制 0，只显示 "0"
        jsr     begin
        lda     #<0
        ldx     #>0
        jsr     bbg_text_put_dec16
        jsr     advanced
        CHECK_A 2, 1
        jsr     bbt_sync
        jsr     begin
        lda     #<str_0
        ldx     #>str_0
        jsr     bbg_text_put_cstr
        jsr     bbt_sync

        ; 第 3 组：十进制 65535，前进 5 列
        jsr     begin
        lda     #<65535
        ldx     #>65535
        jsr     bbg_text_put_dec16
        jsr     advanced
        CHECK_A 3, 5
        jsr     bbt_sync
        jsr     begin
        lda     #<str_65535
        ldx     #>str_65535
        jsr     bbg_text_put_cstr
        jsr     bbt_sync

        ; 第 4 组：十进制 100，中间的零要显示，前进 3 列
        jsr     begin
        lda     #<100
        ldx     #>100
        jsr     bbg_text_put_dec16
        jsr     advanced
        CHECK_A 4, 3
        jsr     bbt_sync
        jsr     begin
        lda     #<str_100
        ldx     #>str_100
        jsr     bbg_text_put_cstr
        jsr     bbt_sync

        ; 第 5 组：十六进制 FF 与 BEEF
        jsr     begin
        lda     #$FF
        jsr     bbg_text_put_hex8
        lda     #<$BEEF
        ldx     #>$BEEF
        jsr     bbg_text_put_hex16
        jsr     advanced
        CHECK_A 5, 6
        jsr     bbt_sync
        jsr     begin
        lda     #<str_ff
        ldx     #>str_ff
        jsr     bbg_text_put_cstr
        lda     #<str_beef
        ldx     #>str_beef
        jsr     bbg_text_put_cstr
        jsr     bbt_sync

        ; 第 6 组：换行
        jsr     begin
        lda     #<str_a
        ldx     #>str_a
        jsr     bbg_text_put_cstr
        jsr     bbg_text_put_newline
        lda     #<str_b
        ldx     #>str_b
        jsr     bbg_text_put_cstr
        jsr     bbt_sync
        jsr     begin
        lda     #<str_lines
        ldx     #>str_lines
        jsr     bbg_text_put_cstr
        jsr     bbt_sync

        jsr     bbt_done
