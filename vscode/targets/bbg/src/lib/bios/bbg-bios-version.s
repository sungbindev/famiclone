; ------------------------------------------------------------------------
; 名称：bbg-bios-version.s
; 说明：BIOS 调用 255：取 BIOS 版本。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"

        .code

; bbg_bios_version
;   说明：BIOS 调用 255：取 BIOS 版本。
;   入口：无
;   出口：A=$40，X=$61，Y=1（bbg1 的 BIOS 1.0）
bbg_bios_version:
        lda     #BIOS_GET_VERSION
        jmp     BIOS_ENTRY
