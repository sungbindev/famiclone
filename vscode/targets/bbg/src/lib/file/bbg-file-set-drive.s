; ------------------------------------------------------------------------
; 名称：bbg-file-set-drive.s
; 说明：设置通道块里的盘号。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr

        .code

; bbg_file_set_drive
;   说明：设置通道块里的盘号。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；Y = 盘号（0 默认盘，1 为 A 盘）
;   出口：无
bbg_file_set_drive:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        tya
        ldy     #FH_DRIVE
        sta     (bbgi_ptr),y
        rts
