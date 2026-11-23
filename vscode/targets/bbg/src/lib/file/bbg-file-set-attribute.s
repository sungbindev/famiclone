; ------------------------------------------------------------------------
; 名称：bbg-file-set-attribute.s
; 说明：文件调用 12：设置已打开文件的属性、日期与时间。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_set_attribute
;   说明：文件调用 12：设置已打开文件的属性、日期与时间。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_OPEN_ATTRIBUTE、FH_OPEN_DATE、FH_OPEN_TIME 为新值
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码；关闭文件后写入目录项
bbg_file_set_attribute:
        ldy     #FILE_ATTRIBUTE
        jmp     bbgi_file_call
