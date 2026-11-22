; ------------------------------------------------------------------------
; 名称：sbi-far-read-byte.s
; 说明：库内部的汇编小例程：读远指针处的字节。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：调用者先把页号放进 SYS_PAGE，页内偏移放进 FAR_OFFSET；内核入口 FAR_READ_BYTE 把页临时映射到窗口 3 读取，读完恢复。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .export         _sbi_far_read_byte

        .code

; unsigned char sbi_far_read_byte(void)
_sbi_far_read_byte:
        ldy     #$00
        jsr     FAR_READ_BYTE
        ldx     #$00
        rts
