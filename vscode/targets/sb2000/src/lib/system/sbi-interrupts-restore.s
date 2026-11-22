; ------------------------------------------------------------------------
; 名称：sbi-interrupts-restore.s
; 说明：库内部的汇编小例程：恢复 sbi_interrupts_off 返回的状态寄存器。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：状态寄存器的值由 C 调用约定放在 A。
; ------------------------------------------------------------------------
        .export         _sbi_interrupts_restore

        .code

; void __fastcall__ sbi_interrupts_restore(unsigned char status)
_sbi_interrupts_restore:
        pha
        plp
        rts
