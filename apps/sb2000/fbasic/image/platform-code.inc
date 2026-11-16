; ------------------------------------------------------------------------
; 名称：platform-code.inc
; 说明：FBASIC 移植版的平台代码：画面初始化（图案 DMA 进显存、清名称表）、PPU 控制寄存器的写入、背景图案表切换。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-17
; 备注：被 fbasic.s 包含，和解释器在同一个翻译单元里，用它的标签（LB559、LB562 等）。
; ------------------------------------------------------------------------

; 画面初始化：关显示与 NMI，扩展显存用第 0 页，把 CHR 的两张 4K 图案表各 DMA 两份进显存
; （背景用：扩展显存 $D000、$E000；精灵用：字符 RAM $2000、$3000），再清名称表 0、1
platform_init:
        lda     #$00
        sta     HW_PPU_CTRL
        sta     HW_PPU_MASK
        sta     HW_VRAM_BANK
        lda     #$08
        sta     HW_PPU_EXT                      ; 4 色背景，精灵图案表 0（在字符 RAM $2000）
        lda     #$0E
        sta     V_TILE_HIGH                     ; 背景图案表 1（原版开机用 CHR 的 $1000-$1FFF 作背景字库）
        ldx     #$03
dma_loop:
        lda     SHELL_CHR_PAGE
        clc
        adc     dma_page,x
        sec
        sbc     #$80                            ; 扩展 RAM 页号从 $80 起
        pha
        lsr     a
        lsr     a
        lsr     a
        ora     #HW_DMA_SOURCE_RAM
        sta     HW_DMA_SOURCE_BANK
        pla
        and     #$07
        asl     a
        asl     a
        asl     a
        asl     a
        ora     #HW_DMA_SOURCE_A15
        sta     HW_DMA_SOURCE_HIGH
        lda     #$00
        sta     HW_DMA_SOURCE_LOW
        sta     HW_DMA_DEST_LOW
        lda     dma_dest,x
        sta     HW_DMA_DEST_HIGH
        lda     #$FF
        sta     HW_DMA_LENGTH_LOW
        lda     #$0F
        sta     HW_DMA_LENGTH_HIGH              ; 4096 字节
        lda     #HW_DMA_START | HW_DMA_FAST
        sta     HW_DMA_MODE
        lda     #$10                            ; 等 DMA 做完（890KB/s，4096 字节约 8000 个周期）
        sta     V_TMP2
dma_wait:
        ldy     #$00
dma_wait_inner:
        dey
        bne     dma_wait_inner
        dec     V_TMP2
        bne     dma_wait
        dex
        bpl     dma_loop
        bit     HW_PPU_STATUS                   ; 清名称表 0、1：2048 个图块，空格、调色板组 0
        lda     #$00
        sta     HW_PPU_ADDRESS
        sta     HW_PPU_ADDRESS
        ldy     #$08
        ldx     #$00
clear_names:
        lda     #$20
        sta     HW_PPU_DATA
        lda     V_TILE_HIGH
        sta     HW_PPU_DATA
        dex
        bne     clear_names
        dey
        bne     clear_names
        lda     #$00                            ; 键盘状态清零，只开键盘中断
        sta     KB_KEY
        sta     KB_EXT
        sta     KB_PREFIX
        sta     KB_MODS
        sta     SHELL_DISK_PAGE
        lda     #$FF & ~HW_IRQ_KEYBOARD
        sta     SHELL_IRQ_MASK
        sta     HW_IRQ_MASK
        cli
        rts

; DMA 的 4 次传输：CHR 页 0、1（各 4K）送到背景用与精灵用的位置
dma_page:
        .byte   0, 0, 1, 1
dma_dest:
        .byte   $D0, $20, $E0, $30

; GAME n：把内置程序搬进程序区，从 $19/$1A 一直搬到 $7000。数据在扩展 RAM 的 GAME 数据里（3 页），$1B/$1C 是它在数据里的偏移。
; 通用 DMA 的源地址不能越过 4K 页边界后继续用同一组寄存器，所以按页分段搬（最多两段）。
game_copy:
        sec
        lda     #$00
        sbc     $19
        sta     V_GAME_LEN_LO
        lda     #$70
        sbc     $1A
        sta     V_GAME_LEN_HI                             ; V_GAME_LEN_LO/V_GAME_LEN_HI = 要搬的字节数
        lda     $1C
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     SHELL_GAME_PAGE
        sec
        sbc     #$80                            ; 扩展 RAM 页号从 $80 起
        sta     V_GAME_PAGE                             ; V_GAME_PAGE = 源所在的扩展 RAM 页（0 起）
        lda     $1C
        and     #$0F
        sta     $1C                             ; $1C:$1B = 页内偏移
game_segment:
        sec
        lda     #$00
        sbc     $1B
        sta     V_TMP1
        lda     #$10
        sbc     $1C
        sta     V_TMP2                          ; V_TMP1/V_TMP2 = 本页还剩的字节数
        lda     V_GAME_LEN_LO
        cmp     V_TMP1
        lda     V_GAME_LEN_HI
        sbc     V_TMP2
        bcc     game_last                       ; 要搬的不超过本页剩下的
        jsr     dma_piece
        sec
        lda     V_GAME_LEN_LO
        sbc     V_TMP1
        sta     V_GAME_LEN_LO
        lda     V_GAME_LEN_HI
        sbc     V_TMP2
        sta     V_GAME_LEN_HI
        clc
        lda     $19
        adc     V_TMP1
        sta     $19
        lda     $1A
        adc     V_TMP2
        sta     $1A
        inc     V_GAME_PAGE
        lda     #$00
        sta     $1B
        sta     $1C
        lda     V_GAME_LEN_LO
        ora     V_GAME_LEN_HI
        bne     game_segment
        rts
game_last:
        lda     V_GAME_LEN_LO
        sta     V_TMP1
        lda     V_GAME_LEN_HI
        sta     V_TMP2
; 搬一段：源是扩展 RAM 页 V_GAME_PAGE 的 $1C:$1B，目的是 $19/$1A，长度 V_TMP1/V_TMP2；通用 DMA 在 4K 页内连续
dma_piece:
        lda     V_GAME_PAGE
        lsr     a
        lsr     a
        lsr     a
        ora     #HW_DMA_SOURCE_RAM
        sta     HW_DMA_SOURCE_BANK
        lda     V_GAME_PAGE
        and     #$07
        asl     a
        asl     a
        asl     a
        asl     a
        ora     #HW_DMA_SOURCE_A15
        ora     $1C
        sta     HW_DMA_SOURCE_HIGH
        lda     $1B
        sta     HW_DMA_SOURCE_LOW
        lda     $19
        sta     HW_DMA_DEST_LOW
        lda     $1A
        sta     HW_DMA_DEST_HIGH
        sec
        lda     V_TMP1
        sbc     #$01
        sta     HW_DMA_LENGTH_LOW
        lda     V_TMP2
        sbc     #$00
        sta     HW_DMA_LENGTH_HIGH
        lda     #HW_DMA_START | HW_DMA_FAST | HW_DMA_TO_CPU
        sta     HW_DMA_MODE
        ldx     #$10                            ; 等 DMA 做完（4096 字节约 8000 个周期）
dma_piece_wait:
        ldy     #$00
dma_piece_inner:
        dey
        bne     dma_piece_inner
        dex
        bne     dma_piece_wait
        rts

; 写 PPU 控制寄存器：A = 原版的 NES 控制值。NES 的位 3（精灵图案表）在 6576 上是 $2008 位 3-2（精灵图案表 0、1 在字符 RAM $2000、$3000），
; 位 4（背景图案表）要写进图块条目的高字节（V_TILE_HIGH），表变了时把已经在屏幕上的图块也改过去
video_write_ctrl:
        pha
        and     #$10
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     #$0D
        cmp     V_TILE_HIGH
        beq     table_same
        sta     V_TILE_HIGH
        jsr     video_retable
table_same:
        pla
        pha
        and     #$08
        lsr     a
        ora     #$08
        sta     HW_PPU_EXT
        pla
        and     #$A7                            ; NMI、8x16、地址增量、名称表位 1-0
        sta     HW_PPU_CTRL
        rts

; 把名称表 0、1（CRAM $0000-$0FFF，2048 个图块）每个图块条目高字节的图案表位改成 V_TILE_HIGH，调色板组不变；显示关闭期间做
video_retable:
        jsr     LB559
        lda     #$01                            ; V_TMP1、V_TMP2 = 第一个条目高字节的 CRAM 地址（低、高）
        sta     V_TMP1
        lda     #$00
        sta     V_TMP2
retable_loop:
        jsr     retable_address
        lda     HW_PPU_DATA                     ; 读缓冲
        lda     HW_PPU_DATA
        and     #$F0
        ora     V_TILE_HIGH
        tay
        jsr     retable_address
        sty     HW_PPU_DATA
        lda     V_TMP1
        clc
        adc     #$02
        sta     V_TMP1
        bcc     retable_next
        inc     V_TMP2
retable_next:
        lda     V_TMP2
        cmp     #$10
        bcc     retable_loop
        jmp     LB562

retable_address:
        bit     HW_PPU_STATUS
        lda     V_TMP2
        sta     HW_PPU_ADDRESS
        lda     V_TMP1
        sta     HW_PPU_ADDRESS
        rts
