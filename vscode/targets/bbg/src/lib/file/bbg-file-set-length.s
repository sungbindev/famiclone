; ------------------------------------------------------------------------
; 名称：bbg-file-set-length.s
; 说明：设置通道块里的读写长度（FH_BYTES）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr
        .importzp       bbg_arg0

        .code

; bbg_file_set_length
;   说明：设置通道块里的读写长度（FH_BYTES）。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；bbg_arg0 = 字节数（读写扇区时为扇区数）
;   出口：无
bbg_file_set_length:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        ldy     #FH_BYTES
        lda     bbg_arg0
        sta     (bbgi_ptr),y
        iny
        lda     bbg_arg0 + 1
        sta     (bbgi_ptr),y
        rts
