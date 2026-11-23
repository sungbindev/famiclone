; ------------------------------------------------------------------------
; 名称：bbg-file-get-length.s
; 说明：读出通道块里的读写长度（FH_BYTES）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr

        .code

; bbg_file_get_length
;   说明：读出通道块里的读写长度（FH_BYTES）。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
;   出口：A（低）、X（高）= FH_BYTES
bbg_file_get_length:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        ldy     #FH_BYTES + 1
        lda     (bbgi_ptr),y
        tax
        dey
        lda     (bbgi_ptr),y
        rts
