; ------------------------------------------------------------------------
; 名称：sb-wait-frames.s
; 说明：sb_wait_frames 的实现：等待指定数量的垂直消隐周期（帧）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：功能 $1B 的帧数在 X 寄存器里，C 调用约定的参数在 A，所以用汇编转一下。帧数 0 按 256 计。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .export         _sb_wait_frames

        .code

; void sb_wait_frames(unsigned char frames)
_sb_wait_frames:
        tax
        lda     #FN_WAIT_FRAMES
        sta     SYS_FUNCTION
        jmp     SYSCALL
