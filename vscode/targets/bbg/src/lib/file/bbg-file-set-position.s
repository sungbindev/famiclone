; ------------------------------------------------------------------------
; 名称：bbg-file-set-position.s
; 说明：设置通道块里的读写位置（FH_POSITION，4 字节）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr
        .importzp       bbg_arg0
        .importzp       bbg_arg1

        .code

; bbg_file_set_position
;   说明：设置通道块里的读写位置（FH_POSITION，4 字节）。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；bbg_arg0 = 位置的低 16 位，bbg_arg1 = 位置的高 16 位
;   出口：无
bbg_file_set_position:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        ldy     #FH_POSITION
        lda     bbg_arg0
        sta     (bbgi_ptr),y
        iny
        lda     bbg_arg0 + 1
        sta     (bbgi_ptr),y
        iny
        lda     bbg_arg1
        sta     (bbgi_ptr),y
        iny
        lda     bbg_arg1 + 1
        sta     (bbgi_ptr),y
        rts
