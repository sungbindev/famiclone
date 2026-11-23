; ------------------------------------------------------------------------
; 名称：bbg-file-open.s
; 说明：文件调用 1：打开文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_open
;   说明：文件调用 1：打开文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号（0 默认盘），FH_PAGE、FH_BUFFER 指向 11 字节文件名（主名 8 加扩展名 3，大写，空格补齐）
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码（不存在为 ERR_FILE_NOT_EXIST）；成功时块里 FH_OPEN_ATTRIBUTE、FH_OPEN_DATE、FH_OPEN_TIME、FH_FILE_SIZE 为文件的属性、日期、时间与长度，读写位置为 0
bbg_file_open:
        ldy     #FILE_OPEN
        jmp     bbgi_file_call
