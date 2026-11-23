; ------------------------------------------------------------------------
; 名称：bbgi-file-call.s
; 说明：文件调用的公共入口：把功能号写进通道块第 0 字节，再调用 FILE_ENTRY。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：库内部使用，调用者不要直接调用。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .importzp       bbgi_ptr
        .export         bbgi_file_call

        .code

; bbgi_file_call
;   入口：A（低）、X（高）= 文件通道块地址，Y = 功能号
;   出口：同 FILE_ENTRY：A 为出错码，C=1 出错
bbgi_file_call:
        sta     bbgi_ptr
        stx     bbgi_ptr + 1
        tya
        ldy     #FH_COMMAND
        sta     (bbgi_ptr),y
        ldy     bbgi_ptr                ; Y = 块地址低字节，X 已是高字节
        jmp     FILE_ENTRY
