; ------------------------------------------------------------------------
; 名称：bbg-file-find-first.s
; 说明：文件调用 6：在当前目录查找第一个文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_find_first
;   说明：文件调用 6：在当前目录查找第一个文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向 11 字节文件名（? 为通配符）
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码（没有文件为 ERR_SEARCH_ENDING）；成功时 FH_DTA_ATTRIBUTE、FH_DTA_TIME、FH_DTA_DATE、FH_DTA_SIZE、FH_DTA_NAME 为找到的文件
bbg_file_find_first:
        ldy     #FILE_FIRST
        jmp     bbgi_file_call
