; ------------------------------------------------------------------------
; 名称：bbg-file-remove-dir.s
; 说明：文件调用 18：删除目录。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：参数与返回值见下方函数说明；不保留 A、X、Y，不使用库以外的零页单元。
; ------------------------------------------------------------------------
        .include        "bbg.inc"
        .import         bbgi_file_call

        .code

; bbg_file_remove_dir
;   说明：文件调用 18：删除目录。
;   入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；参数同 bbg_file_change_dir
;   出口：C=0 成功，A=0；C=1 出错，A 为出错码
bbg_file_remove_dir:
        ldy     #FILE_REMOVE_DIR
        jmp     bbgi_file_call
