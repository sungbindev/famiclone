; ------------------------------------------------------------------------
; 名称：bbg-dos-display-mode-get.s
; 说明：DOS 调用 11：取显示方式。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_dos_display_mode_get
;   说明：DOS 调用 11：取显示方式。
;   入口：无
;   出口：A = DISP_MODE_ENGLISH（$00）或 DISP_MODE_CHINESE（$80）
bbg_dos_display_mode_get:
        ldx     #0
        lda     #DOS_GET_SET_DISP_MODE
        jmp     DOS_ENTRY
