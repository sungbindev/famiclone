; ------------------------------------------------------------------------
; 名称：sbi-interrupts-off.s
; 说明：库内部的汇编小例程：关中断并返回原来的状态寄存器。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：与 sbi_interrupts_restore 成对使用，保护不能被中断打断的多字节写入。
; ------------------------------------------------------------------------
        .export         _sbi_interrupts_off

        .code

; unsigned char sbi_interrupts_off(void)
_sbi_interrupts_off:
        php
        sei
        pla
        ldx     #$00
        rts
