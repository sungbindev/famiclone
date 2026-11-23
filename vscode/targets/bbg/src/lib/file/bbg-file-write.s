; ------------------------------------------------------------------------
; 名称：bbg-file-write.s
; 说明：文件调用 3：从读写位置写文件。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_write
;   说明：文件调用 3：从读写位置写文件。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_PAGE、FH_BUFFER 为数据，FH_BYTES 为写入字节数（0 表示在读写位置截断文件），FH_POSITION 为读写位置
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_BYTES 为实际写入字节数，FH_POSITION 前进
bbg_file_write:
        ldy     #FILE_WRITE
        jmp     bbgi_file_call
