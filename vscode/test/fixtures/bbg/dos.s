; ------------------------------------------------------------------------
; 名称：dos.s
; 说明：测试 bbg.lib 的 DOS 调用封装：版本、显示方式、内存页、键盘测试，以及显示与按键输入（由脚本配合）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：检查编号与脚本里的期望一一对应；需要按键的停靠点见 bbt_sync 的调用处。
; ------------------------------------------------------------------------

        .include        "bbg-test.inc"
        .export         _main

        .bss

line:           .res    20
page:           .res    1
mode:           .res    1

        .rodata

hello:          .byte   "HELLO$"
name_in:        .byte   "A:HELLO.TXT", 0
name_out:       .byte   "HELLO   TXT"

        .code

_main:
        jsr     bbt_init

        ; DOS 版本
        jsr     bbg_dos_version
        SAVE
        CHECK_RA 1, 3
        CHECK_RX 2, 0
        CHECK_RC 3, 0

        ; 显示方式：读出后再设回原值，应保持不变
        jsr     bbg_dos_display_mode_get
        sta     mode
        and     #$7F
        CHECK_A 4, 0                    ; 只有位 7 有意义
        lda     mode
        jsr     bbg_dos_display_mode_set
        jsr     bbg_dos_display_mode_get
        eor     mode
        CHECK_A 5, 0
        ; 键盘缓冲区应为空
        jsr     bbg_dos_key_ready
        php
        pla
        and     #$02                    ; Z 标志
        CHECK_A 6, 2

        ; 申请内存页：申请 1 页成功，释放后再次查询
        lda     #0
        jsr     bbg_dos_pages_alloc
        SAVE
        CHECK_RC 7, 0
        lda     bbt_ra
        sta     page
        lda     #1
        jsr     bbg_dos_pages_alloc
        SAVE
        CHECK_RC 8, 0
        lda     bbt_ra
        cmp     page
        beq     @same
        lda     #1
        jmp     @cmp
@same:  lda     #0
@cmp:   CHECK_A 9, 0                    ; 申请到的页就是查询到的空闲页
        lda     page
        jsr     bbg_dos_pages_free
        lda     #0
        jsr     bbg_dos_pages_alloc
        eor     page
        CHECK_A 18, 0                   ; 释放后再查询，空闲页又是那一页

        ; 显示字符与字符串
        jsr     bbt_sync                ; 停靠点 1：脚本截图（显示之前）
        lda     #'A'
        jsr     bbg_dos_put_char
        lda     #<hello
        ldx     #>hello
        jsr     bbg_dos_put_string
        jsr     bbt_sync                ; 停靠点 2：脚本截图（显示之后）

        ; 读键：脚本在停靠点按下 A
        jsr     bbt_sync                ; 停靠点 3：脚本按键 A
        jsr     bbg_dos_read_key
        CHECK_A 10, 'a'
        jsr     bbt_sync                ; 停靠点 4：脚本按键 B
        jsr     bbg_dos_read_key_echo
        CHECK_A 11, 'b'

        ; 输入一行：脚本输入 hi 回车
        lda     #10
        sta     line
        jsr     bbt_sync                ; 停靠点 5：脚本输入 h i 回车
        lda     #<line
        ldx     #>line
        jsr     bbg_dos_get_line
        lda     line + 1
        CHECK_A 12, 2
        lda     line + 2
        CHECK_A 13, 'h'
        lda     line + 3
        CHECK_A 14, 'i'

        ; 文件名转换：A:HELLO.TXT -> "HELLO   TXT"，盘号 1
        ldx     #11
@name:  lda     name_in,x
        sta     line,x
        dex
        bpl     @name
        lda     #<line
        ldx     #>line
        jsr     bbg_dos_convert_name
        SAVE
        CHECK_RA 15, 1
        CHECK_MEM 16, line, name_out, 11

        ; 清缓冲后读键：脚本先按 Z（进缓冲），再按 Q；读到的应该是 Q
        jsr     bbt_sync                ; 停靠点 6：脚本按 Z
        lda     #DOS_READ_KEY_NO_ECHO
        jsr     bbg_dos_read_key_clear  ; 停靠点 6 之后脚本按 Q
        CHECK_A 17, 'q'

        ; 清屏
        jsr     bbg_dos_clear_screen
        jsr     bbt_sync                ; 停靠点 7：脚本截图（清屏之后）

        jsr     bbt_done
