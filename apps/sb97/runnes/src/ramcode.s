; ------------------------------------------------------------------------
; 名称：ramcode.s
; 说明：在 RAM 里运行的换页例程：把数据复制进程序 RAM 的任意页，以及切换到游戏的程序页并跳转。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-22
; 备注：切换程序页后程序映像不再可见，所以这些例程只能放在 RAM（$0200），由 runnes.s 启动时复制过来。
; ------------------------------------------------------------------------
.setcpu "6502"
.include "defs.inc"

.export ram_copy, ram_handoff

.segment "RAMCODE"

; 切到 ZPAGE 指定的程序页，从 ZS 复制 ZN 个字节到 ZD，再切回程序页 0
ram_copy:
    lda ZPAGE
    sta PRAM_PAGE
    ldy #$00
@loop:
    lda ZN
    ora ZN+1
    beq @done
    lda (ZS),y
    sta (ZD),y
    inc ZS
    bne @s
    inc ZS+1
@s:
    inc ZD
    bne @d
    inc ZD+1
@d:
    lda ZN
    bne @n
    dec ZN+1
@n:
    dec ZN
    jmp @loop
@done:
    lda #$80
    sta PRAM_PAGE
    rts

; 写 $5200 与 $5000，再经新映射里的复位向量跳转
ram_handoff:
    lda HAND_MIRROR
    sta MIRROR_REG
    lda HAND_PAGE
    sta PRAM_PAGE
    jmp (VEC_RESET)
