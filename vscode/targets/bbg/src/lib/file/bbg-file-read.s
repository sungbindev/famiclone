; ------------------------------------------------------------------------
; 名称：bbg-file-read.s
; 说明：文件调用 2：从读写位置读文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_read
;   说明：文件调用 2：从读写位置读文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_PAGE、FH_BUFFER 为缓冲区，FH_BYTES 为读入字节数，FH_POSITION 为读写位置
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_BYTES 为实际读入字节数（到文件末尾时少于请求），FH_BUFFER 为缓冲区结束地址，FH_POSITION 前进
bbg_file_read:
        ldy     #FILE_READ
        jmp     bbgi_file_call
