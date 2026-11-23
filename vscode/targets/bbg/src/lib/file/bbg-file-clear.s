; ------------------------------------------------------------------------
; 名称：bbg-file-clear.s
; 说明：清空文件通道块，并把缓冲区页设成本程序所在页。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr

        .code

; bbg_file_clear
;   说明：清空文件通道块，并把缓冲区页设成本程序所在页。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
;   出口：块全部清零，FH_PAGE 为当前页，FH_DRIVE 为 0（默认盘）
bbg_file_clear:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        ldy     #FH_SIZE - 1
        lda     #0
@clear: sta     (bbgi_ptr),y
        dey
        bpl     @clear
        lda     #BIOS_GET_PAGE
        jsr     BIOS_ENTRY
        ldy     #FH_PAGE
        sta     (bbgi_ptr),y
        rts
