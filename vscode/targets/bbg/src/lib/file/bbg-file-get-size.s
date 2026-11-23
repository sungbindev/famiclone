; ------------------------------------------------------------------------
; 名称：bbg-file-get-size.s
; 说明：读出已打开文件的长度（FH_FILE_SIZE，4 字节）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr
        .importzp       bbg_arg0

        .code

; bbg_file_get_size
;   说明：读出已打开文件的长度（FH_FILE_SIZE，4 字节）。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
;   出口：A（低）、X（高）= 长度的低 16 位；bbg_arg0 = 长度的高 16 位
bbg_file_get_size:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        ldy     #FH_FILE_SIZE + 2
        lda     (bbgi_ptr),y
        sta     bbg_arg0
        iny
        lda     (bbgi_ptr),y
        sta     bbg_arg0 + 1
        ldy     #FH_FILE_SIZE + 1
        lda     (bbgi_ptr),y
        tax
        dey
        lda     (bbgi_ptr),y
        rts
