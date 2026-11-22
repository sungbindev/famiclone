; ------------------------------------------------------------------------
; 名称：sbi-far-copy-out.s
; 说明：库内部的汇编小例程：把 $0700 缓冲里的数据复制到扩展 RAM 页。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：调用者先放好页组 SYS_GROUP、窗口地址 PROG_ENTRY 与字节数 $064F；缓冲从 $0700 起，X 为 0。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .export         _sbi_far_copy_out

        .code

; void sbi_far_copy_out(void)
_sbi_far_copy_out:
        ldx     #$00
        jmp     FAR_COPY_OUT
