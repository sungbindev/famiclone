; ------------------------------------------------------------------------
; 名称：bbg-file-drive-size.s
; 说明：文件调用 11：取驱动器容量。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_drive_size
;   说明：文件调用 11：取驱动器容量。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_SECTORS_PER_CLUSTER、FH_BYTES_PER_SECTOR、FH_FREE_CLUSTERS、FH_TOTAL_CLUSTERS 为容量信息
bbg_file_drive_size:
        ldy     #FILE_DRIVE_SIZE
        jmp     bbgi_file_call
