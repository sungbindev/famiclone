; ------------------------------------------------------------------------
; 名称：bios.s
; 说明：测试 bbg.lib 的 BIOS 调用封装：版本、窗口页号、内存页数、光标列与电传显示。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：检查编号与脚本里的期望一一对应。
; ------------------------------------------------------------------------

        .include        "bbg-test.inc"
        .export         _main

        .bss

column:         .res    1
page:           .res    1

        .code

_main:
        jsr     bbt_init

        ; BIOS 版本
        jsr     bbg_bios_version
        SAVE
        CHECK_RA 1, $40
        CHECK_RX 2, $61
        CHECK_RY 3, 1

        ; 窗口页号：连续两次取得相同，且不是 0
        jsr     bbg_bios_get_page
        sta     page
        jsr     bbg_bios_get_page
        eor     page
        CHECK_A 4, 0
        lda     page
        beq     @zero
        lda     #1
@zero:  CHECK_A 5, 1

        ; 内存页数：A 与 X 相同，且大于程序所在页之前的页数
        jsr     bbg_bios_dram_size
        SAVE
        lda     bbt_ra
        eor     bbt_rx
        CHECK_A 6, 0
        lda     bbt_ra
        cmp     #$10
        lda     #0
        rol     a                       ; 页数不小于 $10 时 C=1
        CHECK_A 7, 1

        ; 光标列：用 DOS 显示 3 个字符后前进 3 列
        jsr     bbg_dos_clear_screen
        jsr     bbg_bios_get_cursor
        sty     column
        lda     #'a'
        jsr     bbg_dos_put_char
        lda     #'b'
        jsr     bbg_dos_put_char
        lda     #'c'
        jsr     bbg_dos_put_char
        jsr     bbg_bios_get_cursor
        tya
        sec
        sbc     column
        CHECK_A 8, 3

        ; 电传显示：BIOS 显示一个字符后前进 1 列
        jsr     bbg_bios_get_cursor
        sty     column
        lda     #'d'
        jsr     bbg_bios_put_char
        jsr     bbg_bios_get_cursor
        tya
        sec
        sbc     column
        CHECK_A 9, 1

        jsr     bbt_done
