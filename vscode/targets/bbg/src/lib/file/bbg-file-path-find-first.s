; ------------------------------------------------------------------------
; 名称：bbg-file-path-find-first.s
; 说明：文件调用 22：按路径查找第一个文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_path_find_first
;   说明：文件调用 22：按路径查找第一个文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向以 0 结尾的路径与文件名（可带 * 与 ?），FH_BYTES 低字节为属性掩码（0 只找一般文件）
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码；结果区同 bbg_file_find_first；之后用 bbg_file_find_next 继续
bbg_file_path_find_first:
        ldy     #FILE_PATH_FIRST
        jmp     bbgi_file_call
