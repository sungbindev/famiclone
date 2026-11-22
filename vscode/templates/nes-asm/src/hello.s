; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：NES 最小汇编程序：设置调色板，在屏幕中间显示 HELLO, NES!。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-10
; 备注：入口是导出的 _main，由 cc65 的启动代码调用；PPU 寄存器的符号在 fcnes.inc 里；
;       文字用 fcdev 附带的 ASCII 字库，图块号等于字符码；写显存前画面要关闭。
; ------------------------------------------------------------------------
        .include "fcnes.inc"
        .export _main

        .segment "RODATA"

; 32 字节调色板：每组 4 色，颜色 0 是背景（黑），颜色 1 是文字（白）
palette:
        .byte $0F, $30, $10, $00, $0F, $30, $10, $00
        .byte $0F, $30, $10, $00, $0F, $30, $10, $00
        .byte $0F, $30, $10, $00, $0F, $30, $10, $00
        .byte $0F, $30, $10, $00, $0F, $30, $10, $00

message:
        .asciiz "HELLO, NES!"

        .segment "CODE"

_main:
        lda #$00
        sta PPUMASK             ; 关闭画面，才能写显存

        lda PPUSTATUS           ; 复位 PPUADDR 的写入顺序
        lda #$3F
        sta PPUADDR
        lda #$00
        sta PPUADDR             ; 显存地址 $3F00：调色板
        ldx #0
@palette:
        lda palette,x
        sta PPUDATA
        inx
        cpx #32
        bne @palette

        lda #$21
        sta PPUADDR
        lda #$CA
        sta PPUADDR             ; 名称表 (10, 14)：$2000 + 14*32 + 10 = $21CA
        ldx #0
@text:  lda message,x
        beq @done
        sta PPUDATA
        inx
        bne @text
@done:
@vblank:
        bit PPUSTATUS           ; 等待垂直消隐，再打开画面
        bpl @vblank

        lda #$00
        sta PPUSCROLL
        sta PPUSCROLL           ; 滚动复位到 (0, 0)
        sta PPUCTRL
        lda #$1E
        sta PPUMASK             ; 显示背景与精灵

@forever:
        jmp @forever            ; NES 程序没有“返回”，主循环一直运行
