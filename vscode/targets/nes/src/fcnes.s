; ------------------------------------------------------------------------
; 名称：fcnes.s
; 说明：fcnes.h 里各函数的实现：画面开关、调色板、文字显示、等待垂直消隐。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-10
; 备注：C 调用约定为 cc65 的 fastcall：最后一个参数在 A（低字节）与 X（高字节），其余参数在 C 栈上，由被调用者弹出。
; ------------------------------------------------------------------------
        .include "fcnes.inc"
        .export _nes_screen_off, _nes_screen_on, _nes_set_palette, _nes_print, _nes_wait_vblank
        .import popa
        .importzp ptr1, tmp1, tmp2

; 背景与精灵都显示，包括最左 8 列
SHOW_ALL = $1E

        .code

; void nes_screen_off(void)
_nes_screen_off:
        lda #0
        sta PPUMASK
        rts

; void nes_screen_on(void)：先复位滚动，再打开显示
_nes_screen_on:
        lda PPUSTATUS
        lda #0
        sta PPUSCROLL
        sta PPUSCROLL
        sta PPUCTRL
        lda #SHOW_ALL
        sta PPUMASK
        rts

; void nes_set_palette(const unsigned char *colors)：把 32 字节写到 $3F00
_nes_set_palette:
        sta ptr1
        stx ptr1+1
        lda PPUSTATUS
        lda #$3F
        sta PPUADDR
        lda #$00
        sta PPUADDR
        ldy #0
@loop:  lda (ptr1),y
        sta PPUDATA
        iny
        cpy #32
        bne @loop
        rts

; void nes_print(unsigned char x, unsigned char y, const char *s)
; 名称表地址 = $2000 + y*32 + x
_nes_print:
        sta ptr1
        stx ptr1+1
        jsr popa
        sta tmp1                ; y
        jsr popa
        sta tmp2                ; x
        lda PPUSTATUS
        lda tmp1
        lsr a
        lsr a
        lsr a
        clc
        adc #$20
        sta PPUADDR             ; 高字节：$20 加上 y 除以 8
        lda tmp1
        and #$07
        asl a
        asl a
        asl a
        asl a
        asl a
        ora tmp2
        sta PPUADDR             ; 低字节：y 的低 3 位乘 32 再加 x
        ldy #0
@next:  lda (ptr1),y
        beq @done
        sta PPUDATA
        iny
        bne @next
@done:  rts

; void nes_wait_vblank(void)
_nes_wait_vblank:
@wait:  bit PPUSTATUS
        bpl @wait
        rts
