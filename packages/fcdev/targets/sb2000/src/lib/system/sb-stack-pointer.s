; ------------------------------------------------------------------------
; 名称：sb-stack-pointer.s
; 说明：sb_stack_pointer 的实现：取硬件栈指针。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：返回的是本函数被调用之后的栈指针，即调用者的栈指针减 2（JSR 压入的返回地址）。
; ------------------------------------------------------------------------
        .export         _sb_stack_pointer

        .code

; unsigned char sb_stack_pointer(void)
_sb_stack_pointer:
        tsx
        txa
        ldx     #$00
        rts
