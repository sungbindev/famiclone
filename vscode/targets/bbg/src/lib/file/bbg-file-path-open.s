; ------------------------------------------------------------------------
; 名称：bbg-file-path-open.s
; 说明：文件调用 20：按路径打开文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_path_open
;   说明：文件调用 20：按路径打开文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向以 0 结尾的路径与文件名（如 SUB\A.TXT）
;   出口：同 bbg_file_open，另有 ERR_PATH_NOT_FOUND
bbg_file_path_open:
        ldy     #FILE_PATH_OPEN
        jmp     bbgi_file_call
