; ------------------------------------------------------------------------
; 名称：bbg-dos-version.s
; 说明：DOS 调用 16：取 DOS 版本。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_dos_version
;   说明：DOS 调用 16：取 DOS 版本。
;   入口：无
;   出口：A=3，X=0，C=0（BBGDOS 3.0）
bbg_dos_version:
        lda     #DOS_GET_DOS_VERSION
        jmp     DOS_ENTRY
