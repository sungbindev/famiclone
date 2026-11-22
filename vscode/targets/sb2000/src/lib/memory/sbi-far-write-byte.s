; ------------------------------------------------------------------------
; 名称：sbi-far-write-byte.s
; 说明：库内部的汇编小例程：把字节写到远指针处。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：调用者先把页号放进 SYS_PAGE，页内偏移放进 FAR_OFFSET；要写的字节由 C 调用约定放在 A。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .export         _sbi_far_write_byte

        .code

; void __fastcall__ sbi_far_write_byte(unsigned char value)
_sbi_far_write_byte:
        ldy     #$00
        jmp     FAR_WRITE_BYTE
