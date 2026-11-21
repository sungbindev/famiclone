; ------------------------------------------------------------------------
; 名称：screen.s
; 说明：出错时的提示：用 DOS 字体（图案表 1，ASCII 码）显示标题、错误原因与操作说明，等待按键后经 BIOS 重新开机。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-22
; 备注：字体在字符 RAM 的图案表 1 里，只在 CHR 写入之前使用；键盘扫描协议见 ember 的 keyboard-sb97（位置 1-13，每个位置两半，位置 10 后半的第 1 个键恒为按下）。
; ------------------------------------------------------------------------
.setcpu "6502"
.include "defs.inc"

.export show_error
.import ram_handoff, delay

.segment "CODE"

; 显示错误码 X 的提示，然后等待按键并经 BIOS 重新开机（不返回）
show_error:
    stx ERRIDX
    ldx #$FF
    txs
    lda #$00
    sta PPU_CTRL
    sta PPU_MASK
    lda #$04
    sta FDC_DOR
    lda #$3F
    sta PPU_ADDR
    lda #$00
    sta PPU_ADDR
    lda #$0F
    sta PPU_DATA
    lda #$30
    sta PPU_DATA
    sta PPU_DATA
    sta PPU_DATA
    lda #$20
    sta PPU_ADDR
    lda #$00
    sta PPU_ADDR
    lda #$20
    ldx #$04
@clear_page:
    ldy #$00
@clear_byte:
    sta PPU_DATA
    iny
    bne @clear_byte
    dex
    bne @clear_page
    lda #$23
    sta PPU_ADDR
    lda #$C0
    sta PPU_ADDR
    lda #$00
    ldx #$40
@attr:
    sta PPU_DATA
    dex
    bne @attr
    lda #$00
    sta PPU_SCROLL
    sta PPU_SCROLL
    lda #<msg_title
    sta ZS
    lda #>msg_title
    sta ZS+1
    lda #$03
    sta ROW
    lda #$02
    sta COL
    jsr put_text
    lda ERRIDX
    asl
    tax
    lda err_table,x
    sta ZS
    lda err_table+1,x
    sta ZS+1
    lda #$08
    sta ROW
    lda #$02
    sta COL
    jsr put_text
    lda #<msg_press
    sta ZS
    lda #>msg_press
    sta ZS+1
    lda #$0E
    sta ROW
    lda #$02
    sta COL
    jsr put_text
    lda #$00
    sta PPU_SCROLL
    sta PPU_SCROLL
    lda #$10
    sta PPU_CTRL
    lda #$0A
    sta PPU_MASK
@release:
    jsr scan_key
    bne @release
    lda #$02
    jsr delay
@press:
    jsr scan_key
    beq @press
    lda #$00
    sta HAND_MIRROR
    sta HAND_PAGE
    jmp ram_handoff

; 在 ROW 行、COL 列写入 ZS 指向的以 0 结尾的文字（名称表 0，每行 32 格）
put_text:
    lda ROW
    lsr
    lsr
    lsr
    ora #$20
    sta T1
    lda ROW
    asl
    asl
    asl
    asl
    asl
    clc
    adc COL
    sta T0
    bcc @addr
    inc T1
@addr:
    lda T1
    sta PPU_ADDR
    lda T0
    sta PPU_ADDR
    ldy #$00
@char:
    lda (ZS),y
    beq @done
    sta PPU_DATA
    iny
    bne @char
@done:
    rts

; 扫描键盘：有键按下时 A 为 1，否则为 0
scan_key:
    lda #$05
    sta JOY1
    ldy #$01
@pos:
    lda #$04
    sta JOY1
    lda KEYBOARD
    and #$1E
    cmp #$1E
    bne @hit
    lda #$06
    sta JOY1
    lda KEYBOARD
    and #$1E
    cpy #$0A
    bne @full
    and #$1C
    cmp #$1C
    bne @hit
    jmp @next
@full:
    cmp #$1E
    bne @hit
@next:
    iny
    cpy #$0E
    bne @pos
    lda #$00
    rts
@hit:
    lda #$01
    rts

.segment "RODATA"

err_table:
    .word msg_title
    .word msg_usage
    .word msg_open
    .word msg_header
    .word msg_mapper
    .word msg_size
    .word msg_trunc
    .word msg_read
    .word msg_disk

msg_title:
    .byte "RUNNES - NES ON SB97", $00
msg_usage:
    .byte "Usage: RUNNES FILE.NES", $00
msg_open:
    .byte "Can't open file", $00
msg_header:
    .byte "Not an iNES file", $00
msg_mapper:
    .byte "Mapper not supported", $00
msg_size:
    .byte "Size not supported", $00
msg_trunc:
    .byte "File is truncated", $00
msg_read:
    .byte "Read error", $00
msg_disk:
    .byte "Disk format not supported", $00
msg_press:
    .byte "Press any key to reboot", $00
