; ------------------------------------------------------------------------
; 名称：bbg-dos-display-mode-set.s
; 说明：DOS 调用 11：设置显示方式。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_dos_display_mode_set
;   说明：DOS 调用 11：设置显示方式。
;   入口：A = DISP_MODE_ENGLISH 或 DISP_MODE_CHINESE
;   出口：无
bbg_dos_display_mode_set:
        tay
        ldx     #1
        lda     #DOS_GET_SET_DISP_MODE
        jmp     DOS_ENTRY
