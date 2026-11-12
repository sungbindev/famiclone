; ------------------------------------------------------------------------
; 名称：sb-jump-banked.s
; 说明：sb_jump_banked 的实现：映射页组后跳转到其中的例程。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：功能 $26 会丢弃系统调用的返回地址与保存的寄存器，所以不会回到本例程；入口例程的 RTS 直接返回到本函数调用者的位置。
;       C 的参数由被调用者弹出，所以先在这里弹出页组，再发起系统调用，否则软件栈会少弹 3 个字节。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .export         _sb_jump_banked
        .import         popa

        .code

; void __fastcall__ sb_jump_banked(unsigned char group, unsigned int entry)
_sb_jump_banked:
        sta     PROG_ENTRY_LO
        stx     PROG_ENTRY_HI
        jsr     popa
        sta     SYS_GROUP
        lda     #FN_JUMP_BANKED
        sta     SYS_FUNCTION
        jsr     SYSCALL
        rts
