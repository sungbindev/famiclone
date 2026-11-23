; ------------------------------------------------------------------------
; 名称：bbg-test.s
; 说明：步步高运行库测试程序的公共实现：记录检查结果、停靠点与结束。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：变量 bbg_test_stop 在结束时写 1、在 bbt_sync 里写 2 并等脚本清零（脚本轮询它，不设断点，
;       避免输入程序名时的时序竞争）；bbg_test_count、bbg_test_ok、bbg_test_line（检查编号）、
;       bbg_test_value（实际值）是检查结果，格式与 SB2000 的测试程序一致。
; ------------------------------------------------------------------------

        .include        "bbg.inc"
        .export         bbg_test_stop, bbg_test_count, bbg_test_ok, bbg_test_line, bbg_test_value
        .export         bbt_init, bbt_done, bbt_sync
        .export         bbt_check, bbt_check_ok, bbt_check_cc, bbt_check16, bbt_compare
        .export         bbt_actual16, bbt_expected16, bbt_ra, bbt_rx, bbt_ry, bbt_rc
        .exportzp       bbt_ptr1, bbt_ptr2

MAX_CHECKS      = 80

        .zeropage

bbt_ptr1:       .res    2
bbt_ptr2:       .res    2

        .bss

bbg_test_stop:  .res    1
bbg_test_count: .res    1
bbg_test_ok:    .res    MAX_CHECKS
bbg_test_line:  .res    MAX_CHECKS * 2
bbg_test_value: .res    MAX_CHECKS * 2
bbt_actual16:   .res    2
bbt_expected16: .res    2
bbt_ra:         .res    1
bbt_rx:         .res    1
bbt_ry:         .res    1
bbt_rc:         .res    1
tmp_actual:     .res    1
tmp_id:         .res    1
tmp_expected:   .res    1

        .code

; 清空检查记录；BSS 不会被启动代码清零
bbt_init:
        lda     #0
        sta     bbg_test_stop
        sta     bbg_test_count
        rts

; 通知脚本全部检查结束，不返回
bbt_done:
        lda     #1
        sta     bbg_test_stop
@halt:  jmp     @halt

; 停靠点：写 2 后等脚本把 bbg_test_stop 清零，脚本在这里截图、注入按键，再继续运行
bbt_sync:
        lda     #2
        sta     bbg_test_stop
@wait:  lda     bbg_test_stop
        cmp     #2
        beq     @wait
        rts

; 记录一次检查：A = 实际值，X = 检查编号，Y = 期望值
bbt_check:
        sta     tmp_actual
        stx     tmp_id
        sty     tmp_expected
        lda     bbg_test_count
        cmp     #MAX_CHECKS
        bcs     @full
        asl     a
        tay
        lda     tmp_id
        sta     bbg_test_line,y
        lda     #0
        sta     bbg_test_line + 1,y
        lda     tmp_actual
        sta     bbg_test_value,y
        lda     #0
        sta     bbg_test_value + 1,y
        ldx     bbg_test_count
        ldy     tmp_actual
        lda     #0
        cpy     tmp_expected
        bne     @store
        lda     #1
@store: sta     bbg_test_ok,x
        inc     bbg_test_count
@full:  rts

; 检查调用结果：C 与 A 合并成一个值（出错时位 7 为 1）；X = 检查编号，Y = 期望值
bbt_check_cc:
        bcc     @plain
        ora     #$80
@plain: jmp     bbt_check

; 检查调用成功：C=0 且 A=0；X = 检查编号
bbt_check_ok:
        ldy     #0
        jmp     bbt_check_cc

; 检查 16 位值：bbt_actual16 与 bbt_expected16；X = 检查编号
bbt_check16:
        stx     tmp_id
        lda     bbg_test_count
        cmp     #MAX_CHECKS
        bcs     @full
        asl     a
        tay
        lda     tmp_id
        sta     bbg_test_line,y
        lda     #0
        sta     bbg_test_line + 1,y
        lda     bbt_actual16
        sta     bbg_test_value,y
        lda     bbt_actual16 + 1
        sta     bbg_test_value + 1,y
        ldx     bbg_test_count
        lda     #0
        ldy     bbt_actual16
        cpy     bbt_expected16
        bne     @store
        ldy     bbt_actual16 + 1
        cpy     bbt_expected16 + 1
        bne     @store
        lda     #1
@store: sta     bbg_test_ok,x
        inc     bbg_test_count
@full:  rts

; 比较 bbt_ptr1 与 bbt_ptr2 指向的两段内存，Y = 长度（1-255）；相同返回 A=0，否则 A=1
bbt_compare:
        dey
@loop:  lda     (bbt_ptr1),y
        cmp     (bbt_ptr2),y
        bne     @differ
        dey
        bpl     @loop
        lda     #0
        rts
@differ:
        lda     #1
        rts
