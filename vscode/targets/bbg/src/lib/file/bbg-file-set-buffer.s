; ------------------------------------------------------------------------
; 名称：bbg-file-set-buffer.s
; 说明：设置通道块里的缓冲区（页取当前页）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr
        .importzp       bbg_arg0

        .code

; bbg_file_set_buffer
;   说明：设置通道块里的缓冲区（页取当前页）。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；bbg_arg0 = 缓冲区地址或文件名地址
;   出口：无
bbg_file_set_buffer:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        lda     #BIOS_GET_PAGE
        jsr     BIOS_ENTRY
        ldy     #FH_PAGE
        sta     (bbgi_ptr),y
        iny
        lda     bbg_arg0
        sta     (bbgi_ptr),y
        iny
        lda     bbg_arg0 + 1
        sta     (bbgi_ptr),y
        rts
