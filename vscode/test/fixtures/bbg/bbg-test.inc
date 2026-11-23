; ------------------------------------------------------------------------
; 名称：bbg-test.inc
; 说明：步步高运行库测试程序的公共约定：把检查结果记在固定变量里，由测试脚本从模拟器内存读出。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：测试程序先调用 bbt_init，每做一次检查就用 CHECK_ 开头的宏，结束时调用 bbt_done；需要宿主（测试脚本）
;       配合操作（按键）时调用 bbt_sync，脚本在这里停下、注入按键等，再继续运行。检查编号 id 在同一个程序里
;       不要重复，失败时脚本按编号报告。
; ------------------------------------------------------------------------

        .include        "bbg.inc"

        .import         bbt_init, bbt_done, bbt_sync
        .import         bbt_check, bbt_check_ok, bbt_check_cc, bbt_check16, bbt_compare
        .import         bbt_actual16, bbt_expected16, bbt_ra, bbt_rx, bbt_ry, bbt_rc
        .importzp       bbt_ptr1, bbt_ptr2

; 检查 A 等于 expected（单字节）
.macro CHECK_A id, expected
        ldx     #id
        ldy     #expected
        jsr     bbt_check
.endmacro

; 检查 A、X（低、高）等于 expected（16 位）
.macro CHECK16 id, expected
        sta     bbt_actual16
        stx     bbt_actual16 + 1
        lda     #<(expected)
        sta     bbt_expected16
        lda     #>(expected)
        sta     bbt_expected16 + 1
        ldx     #id
        jsr     bbt_check16
.endmacro

; 保存刚才的调用结果：A、X、Y 与进位（用 CHECK_RA 等再检查）
.macro SAVE
        sta     bbt_ra
        stx     bbt_rx
        sty     bbt_ry
        php
        pla
        and     #1
        sta     bbt_rc
.endmacro

.macro CHECK_RA id, expected
        lda     bbt_ra
        CHECK_A id, expected
.endmacro

.macro CHECK_RX id, expected
        lda     bbt_rx
        CHECK_A id, expected
.endmacro

.macro CHECK_RY id, expected
        lda     bbt_ry
        CHECK_A id, expected
.endmacro

.macro CHECK_RC id, expected
        lda     bbt_rc
        CHECK_A id, expected
.endmacro

; 检查调用成功：C=0 且 A=0
.macro CHECK_OK id
        ldx     #id
        jsr     bbt_check_ok
.endmacro

; 检查调用出错：C=1 且 A 等于出错码 code
.macro CHECK_ERR id, code
        ldx     #id
        ldy     #(code) | $80
        jsr     bbt_check_cc
.endmacro

; 比较两段内存相同：len 个字节，相同时记通过
.macro CHECK_MEM id, first, second, len
        lda     #<(first)
        ldx     #>(first)
        sta     bbt_ptr1
        stx     bbt_ptr1 + 1
        lda     #<(second)
        ldx     #>(second)
        sta     bbt_ptr2
        stx     bbt_ptr2 + 1
        ldy     #len
        jsr     bbt_compare
        CHECK_A id, 0
.endmacro
