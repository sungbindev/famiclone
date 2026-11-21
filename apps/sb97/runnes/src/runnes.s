; ------------------------------------------------------------------------
; 名称：runnes.s
; 说明：sb97 上运行 NES 游戏的入口：解析命令行的文件名、加载游戏、写入 CHR 与 PRG 镜像，然后进入 NES 模式；出错时显示提示并返回 BIOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-22
; 备注：DOS 用 JMP 进入程序（栈里没有返回地址），所以游戏与出错两条路都不返回 DOS，只能经 BIOS 重新开机。
; ------------------------------------------------------------------------
.setcpu "6502"
.include "defs.inc"

.import fdc_init, fdc_stop, fdc_read_sector
.import load_game
.import show_error
.import ram_copy, ram_handoff
.import __RAMCODE_LOAD__, __RAMCODE_RUN__, __RAMCODE_SIZE__

.export copy_bytes, delay

.segment "CODE"

; 复位入口：DOS 从程序映像的复位向量进入
reset:
    sei
    cld
    ldx #$FF
    txs
    lda #$00
    sta PPU_CTRL
    sta PPU_MASK
    lda #$40
    sta KEYBOARD                    ; 关闭 APU 帧中断，软驱中断不接进来
    jsr copy_ramcode
    jsr copy_tail
    jsr parse_name
    bne @have_name
    ldx #ERR_USAGE
    jmp show_error
@have_name:
    jsr fdc_init
    jsr load_game
    beq @loaded
    tax
    jmp show_error
@loaded:
    jmp finish_game

nmi:
    rti

irq:
    rti

; 把换页例程从映像复制到 RAM（$0200）
copy_ramcode:
    lda #<__RAMCODE_LOAD__
    sta ZS
    lda #>__RAMCODE_LOAD__
    sta ZS+1
    lda #<__RAMCODE_RUN__
    sta ZD
    lda #>__RAMCODE_RUN__
    sta ZD+1
    lda #<__RAMCODE_SIZE__
    sta ZN
    lda #>__RAMCODE_SIZE__
    sta ZN+1
    jmp copy_bytes

; 把 DOS 的命令行复制到 TAIL，最多 62 字节
copy_tail:
    ldx #$00
@loop:
    lda TAIL_SRC,x
    sta TAIL,x
    beq @done
    inx
    cpx #62
    bne @loop
    lda #$00
    sta TAIL,x
@done:
    rts

; 从命令行取第二个词（第一个词是程序名）作为 8.3 文件名，放进 NAME83；有文件名时 A 为 1，否则为 0
parse_name:
    ldx #$00
    jsr skip_spaces
@program:
    lda TAIL,x
    beq @none
    cmp #' '
    beq @args
    inx
    bne @program
@none:
    lda #$00
    rts
@args:
    jsr skip_spaces
    lda TAIL,x
    beq @none
    ldy #10
    lda #' '
@blank:
    sta NAME83,y
    dey
    bpl @blank
    ldy #$00
@base:
    lda TAIL,x
    beq @ok
    cmp #' '
    beq @ok
    cmp #'.'
    beq @ext
    jsr to_upper
    cpy #8
    bcs @base_next
    sta NAME83,y
    iny
@base_next:
    inx
    bne @base
@ext:
    inx
    ldy #8
@ext_loop:
    lda TAIL,x
    beq @ok
    cmp #' '
    beq @ok
    jsr to_upper
    cpy #11
    bcs @ext_next
    sta NAME83,y
    iny
@ext_next:
    inx
    bne @ext_loop
@ok:
    lda #$01
    rts

; 跳过 X 指向的空格
skip_spaces:
@loop:
    lda TAIL,x
    cmp #' '
    bne @done
    inx
    bne @loop
@done:
    rts

; 把 A 中的小写字母改成大写
to_upper:
    cmp #'a'
    bcc @done
    cmp #'z'+1
    bcs @done
    and #$DF
@done:
    rts

; 从 ZS 复制 ZN 个字节到 ZD（16 位计数，结束时 ZS、ZD 指向复制后的位置）
copy_bytes:
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
    rts

; 延时：A 为次数，每次约 0.18 秒
delay:
    sta T1
@outer:
    ldy #$00
@middle:
    ldx #$00
@inner:
    dex
    bne @inner
    dey
    bne @middle
    dec T1
    bne @outer
    rts

; 把 CHR 写进字符 RAM（没有 CHR 时清零）
write_chr:
    lda #$00
    sta PPU_ADDR
    sta PPU_ADDR
    lda #<CHR_BUF
    sta ZS
    lda #>CHR_BUF
    sta ZS+1
    ldx #$20
    lda CHR
    ora CHR+1
    beq @zero
@page:
    ldy #$00
@byte:
    lda (ZS),y
    sta PPU_DATA
    iny
    bne @byte
    inc ZS+1
    dex
    bne @page
    rts
@zero:
    ldy #$00
@zbyte:
    lda #$00
    sta PPU_DATA
    iny
    bne @zbyte
    dex
    bne @zero
    rts

; 16K 的 PRG 在程序 RAM 里复制一份到 $C000，使 $C000-$FFFF 与 $8000-$BFFF 相同
mirror_prg:
    lda PRG+1
    cmp #$40
    bne @done
    lda #$81
    sta ZPAGE
    lda #$00
    sta ZS
    sta ZD
    sta ZN
    lda #$80
    sta ZS+1
    lda #$C0
    sta ZD+1
    lda #$40
    sta ZN+1
    jsr ram_copy
@done:
    rts

; 游戏的收尾：停转软驱、写 CHR、镜像 PRG、清内存，然后切到游戏的程序页并跳到它的复位向量
finish_game:
    jsr fdc_stop
    jsr write_chr
    jsr mirror_prg
    lda MIRROR_VAL
    sta HAND_MIRROR
    lda #$81
    sta HAND_PAGE
    jsr clear_for_game
    ldx #$FF
    txs
    jmp ram_handoff

; 清零零页、$0300-$07FF 与 WRAM（栈与 $0200-$02FF 保留）
clear_for_game:
    lda #$00
    ldx #$00
@zp:
    sta $00,x
    inx
    bne @zp
    lda #$03
    ldx #$05
    jsr clear_pages
    lda #$60
    ldx #$20
    jsr clear_pages
    rts

; A 为起始页的高字节，X 为页数，全部清零
clear_pages:
    sta ZD+1
    lda #$00
    sta ZD
@page:
    ldy #$00
    lda #$00
@byte:
    sta (ZD),y
    iny
    bne @byte
    inc ZD+1
    dex
    bne @page
    rts

.segment "VECTORS"
    .byte $AA, $55
    .word nmi, reset, irq
