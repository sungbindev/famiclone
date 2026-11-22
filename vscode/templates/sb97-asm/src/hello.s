; ------------------------------------------------------------------------
; 名称：hello.s
; 说明：SB97 DOS 最小汇编程序：在屏幕上显示两行英文并停住。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：入口是导出的 _main，由启动代码调用；DOS 用 JMP 启动程序，程序不能返回 DOS，要退出只能复位。
;       直接写 PPU 显示文字：DOS 的字库在图案表 1，字符的 ASCII 码就是图块号，所以 PPU_CTRL 要选图案表 1。
; ------------------------------------------------------------------------
        .include "sb97.inc"
        .export _main

        .segment "ZEROPAGE"

text:   .res 2                          ; 要显示的字符串的地址

        .segment "RODATA"

message:
        .asciiz "Hello, SB97!"

message_exit:
        .asciiz "Reset the machine to exit."

        .segment "CODE"

_main:
        lda #$00
        sta PPU_CTRL                    ; 先关闭显示，才能安全写显存
        sta PPU_MASK

        lda #$20                        ; 名称表 0 起始地址 $2000
        sta PPU_ADDR
        lda #$00
        sta PPU_ADDR
        lda #' '                        ; 清屏：960 个空格
        ldy #15
@row:   ldx #64
@col:   sta PPU_DATA
        dex
        bne @col
        dey
        bne @row

        lda #<message
        sta text
        lda #>message
        sta text+1
        lda #$42                        ; 第 10 行第 2 列，地址 $2142
        jsr print

        lda #<message_exit
        sta text
        lda #>message_exit
        sta text+1
        lda #$82                        ; 第 12 行第 2 列，地址 $2182
        jsr print

        lda #$00
        sta PPU_SCROLL
        sta PPU_SCROLL
        lda #$10                        ; 背景用图案表 1，字库在那里
        sta PPU_CTRL
        lda #$08                        ; 显示背景
        sta PPU_MASK
forever:
        jmp forever

; 在名称表的指定位置显示字符串：A 为地址低字节（高字节固定 $21），text 指向以 0 结尾的字符串
print:
        ldx #$21
        stx PPU_ADDR
        sta PPU_ADDR
        ldy #0
@next:  lda (text),y
        beq @done
        sta PPU_DATA
        iny
        bne @next
@done:  rts
