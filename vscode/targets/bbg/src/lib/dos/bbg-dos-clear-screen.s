; ------------------------------------------------------------------------
; 名称：bbg-dos-clear-screen.s
; 说明：DOS 调用 9：初始化屏幕并清屏。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_dos_clear_screen
;   说明：DOS 调用 9：初始化屏幕并清屏。
;   入口：无
;   出口：无
bbg_dos_clear_screen:
        lda     #DOS_INITIAL_SCREEN
        jmp     DOS_ENTRY
