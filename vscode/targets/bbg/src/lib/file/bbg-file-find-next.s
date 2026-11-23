; ------------------------------------------------------------------------
; 名称：bbg-file-find-next.s
; 说明：文件调用 7：继续查找下一个文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_find_next
;   说明：文件调用 7：继续查找下一个文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块沿用 bbg_file_find_first 的结果，不能改动
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码（没有更多文件为 ERR_SEARCH_ENDING）；结果区同 bbg_file_find_first
bbg_file_find_next:
        ldy     #FILE_NEXT
        jmp     bbgi_file_call
