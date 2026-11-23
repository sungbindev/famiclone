; ------------------------------------------------------------------------
; 名称：bbg-bios-get-cursor.s
; 说明：BIOS 调用 30：取光标所在列。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_bios_get_cursor
;   说明：BIOS 调用 30：取光标所在列。
;   入口：无
;   出口：Y = 光标列
bbg_bios_get_cursor:
        lda     #BIOS_GET_CURSOR
        jmp     BIOS_ENTRY
