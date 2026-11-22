; ------------------------------------------------------------------------
; 名称：platform-code.inc
; 说明：FBASIC 移植版的平台代码：画面初始化（图案 DMA 进显存、清名称表）、PPU 控制寄存器的写入、背景图案表切换。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-17
; 备注：被 fbasic.s 包含，和解释器在同一个翻译单元里，用它的标签（LB559、LB562 等）。
; ------------------------------------------------------------------------

; 画面初始化：关 NMI，扩展显存用第 0 页，把 CHR 的两张 4K 图案表各 DMA 两份进显存
; （背景用：扩展显存 $D000、$E000；精灵用：字符 RAM $2000、$3000），再用 DMA 把名称表 0-3 清成空白（环形滚动用 64 行，每个屏幕占 2 页）；
; 显示一直开着，调色板已经全黑（black_palette），所以这期间显存里的变化看不见，也不会出现关显示时的灰屏
platform_init:
        lda     #$00
        sta     HW_PPU_CTRL
        sta     HW_VRAM_BANK
        lda     #$08
        sta     HW_PPU_EXT                      ; 4 色背景，精灵图案表 0（在字符 RAM $2000）
        lda     #$0E
        sta     V_TILE_HIGH                     ; 背景图案表 1（原版开机用 CHR 的 $1000-$1FFF 作背景字库）
        ldx     #$03
init_chr:
        lda     SHELL_CHR_PAGE
        clc
        adc     dma_page,x
        ldy     dma_dest,x
        jsr     dma_to_video
        dex
        bpl     init_chr
        lda     #$10                            ; 映像在窗口 6 的页（没有代码）填成空格单元，DMA 两次盖住名称表 0-3：4096 个图块，空格、调色板组 0
        sta     V_COPY_LEN
        lda     #$20
        ldx     V_TILE_HIGH
        jsr     fill_window_page
        lda     SHELL_IMAGE_WINDOW + 6
        ldy     #$00
        jsr     dma_to_video
        lda     SHELL_IMAGE_WINDOW + 6
        ldy     #$10
        jsr     dma_to_video
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

; 通用 DMA 送 4096 字节进显存：A = 源在扩展 RAM 的页号（$80 起），Y = 目的地址高字节（低字节 0）；
; 显示开着时 DMA 只在垂直消隐里进行，所以启动后等两个垂直消隐再返回；不改 X
dma_to_video:
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
        sty     HW_DMA_DEST_HIGH
        lda     #$FF
        sta     HW_DMA_LENGTH_LOW
        lda     #$0F
        sta     HW_DMA_LENGTH_HIGH              ; 4096 字节
        lda     #HW_DMA_START | HW_DMA_FAST
        sta     HW_DMA_MODE
        jsr     LB42B
        jmp     LB42B

; 把映像在窗口 6 的页（DISK_STAGE_BASE 起）填上重复的两字节 A、X，共 V_COPY_LEN 个 256 字节块；会改 V_COPY_LEN、V_COPY_DST、V_TMP1、V_TMP2
fill_window_page:
        sta     V_TMP1
        stx     V_TMP2
        lda     #<DISK_STAGE_BASE
        sta     V_COPY_DST
        lda     #>DISK_STAGE_BASE
        sta     V_COPY_DST + 1
        ldy     #$00
fill_window_loop:
        lda     V_TMP1
        sta     (V_COPY_DST),y
        iny
        lda     V_TMP2
        sta     (V_COPY_DST),y
        iny
        bne     fill_window_loop
        inc     V_COPY_DST + 1
        dec     V_COPY_LEN
        bne     fill_window_loop
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
        and     #$A5                            ; NMI、8x16、地址增量、名称表位 0；位 1 由滚动值决定
        pha
        txa
        pha
        lda     $F0
        beq     ctrl_scroll_text
        lda     $E5
        jmp     ctrl_scroll_page
ctrl_scroll_text:
        lda     $E4
ctrl_scroll_page:
        asl     a                               ; 滚动值是行号乘 4（0-252），位 7 为 1 表示显示从名称表的下半页（页 +2）开始
        lda     #$00
        rol     a
        asl     a
        tsx
        ora     $0102,x
        sta     HW_PPU_CTRL
        pla
        tax
        pla
        rts

; 把名称表 0-3（CRAM $0000-$1FFF，4096 个图块）每个图块条目高字节的图案表位改成 V_TILE_HIGH，调色板组不变；显示关闭期间做
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
        cmp     #$20
        bcc     retable_loop
        jmp     LB562

; 把一页名称表（A = NES 地址高字节）所有图块的调色板组清成 0，只写高字节
clear_groups_page:
        ldx     #$00
        jsr     L81EC
        bit     HW_PPU_DATA
        ldy     #$04
clear_groups_loop:
        lda     V_TILE_HIGH
        sta     HW_PPU_DATA
        bit     HW_PPU_DATA
        dex
        bne     clear_groups_loop
        dey
        bne     clear_groups_loop
        rts

retable_address:
        bit     HW_PPU_STATUS
        lda     V_TMP2
        sta     HW_PPU_ADDRESS
        lda     V_TMP1
        sta     HW_PPU_ADDRESS
        rts

; ------------------------------------------------------------------------
; 背景图与调色板组：NES 的背景图是 1K（960 个图块号加 64 个属性字节，每 2x2 个图块共用一个调色板组），
; 6576 的名称表 1 在 CRAM $0800 起，每个图块 2 字节（图案号，高 4 位调色板组加图案表位），共 32 行；两者之间要转换。
; ------------------------------------------------------------------------
BG_ROW          = $F1                           ; 当前行（0-29）
BG_COL          = $F2                           ; 当前列（0-31）
BG_ATTR_ROW     = $F4                           ; 当前行所在属性行的起点（每行属性 8 个字节）

; 把 ($19) 起的 1K（NES 格式）写成名称表 1；第 30、31 行写空格。$EB/$EC 指向属性字节，$DE/$DF 是临时单元（NMI 不用）
bg_write_page:
        clc
        lda     $19
        adc     #$C0
        sta     $EB
        lda     $1A
        adc     #$03
        sta     $EC
        lda     #$24
        ldx     #$00
        jsr     L81EC
        lda     #$00
        sta     BG_ROW
bg_write_row:
        lda     BG_ROW
        lsr     a
        lsr     a
        asl     a
        asl     a
        asl     a
        sta     BG_ATTR_ROW
        lda     #$00
        sta     BG_COL
bg_write_col:
        ldy     BG_COL
        lda     ($19),y
        pha
        lda     BG_COL
        lsr     a
        lsr     a
        clc
        adc     BG_ATTR_ROW
        tay
        lda     ($EB),y
        sta     $DF
        lda     BG_COL                          ; 这个图块在 2x2 块里的位置决定属性字节里取哪两位
        and     #$02
        sta     $DE
        lda     BG_ROW
        and     #$02
        asl     a
        ora     $DE
        tax
        lda     $DF
        cpx     #$00
        beq     bg_write_group
bg_write_shift:
        lsr     a
        lsr     a
        dex
        dex
        bne     bg_write_shift
bg_write_group:
        and     #$03
        asl     a
        asl     a
        asl     a
        asl     a
        ora     V_TILE_HIGH
        sta     $DF
        pla
        sta     HW_PPU_DATA
        lda     $DF
        sta     HW_PPU_DATA
        inc     BG_COL
        lda     BG_COL
        cmp     #$20
        bne     bg_write_col
        clc
        lda     $19
        adc     #$20
        sta     $19
        bcc     bg_write_next
        inc     $1A
bg_write_next:
        inc     BG_ROW
        lda     BG_ROW
        cmp     #$1E
        bne     bg_write_row
        ldx     #$40
bg_write_blank:
        lda     #$20
        sta     HW_PPU_DATA
        lda     V_TILE_HIGH
        sta     HW_PPU_DATA
        dex
        bne     bg_write_blank
        rts

; 把名称表 1 读成 ($19) 起的 1K（NES 格式）：每个 2x2 块取左上角图块的调色板组
bg_read_page:
        clc
        lda     $19
        adc     #$C0
        sta     $EB
        lda     $1A
        adc     #$03
        sta     $EC
        ldy     #$3F
        lda     #$00
bg_read_clear:
        sta     ($EB),y
        dey
        bpl     bg_read_clear
        lda     #$24
        ldx     #$00
        jsr     L81EC
        lda     HW_PPU_DATA                     ; 读缓冲
        lda     #$00
        sta     BG_ROW
bg_read_row:
        lda     BG_ROW
        lsr     a
        lsr     a
        asl     a
        asl     a
        asl     a
        sta     BG_ATTR_ROW
        lda     #$00
        sta     BG_COL
bg_read_col:
        lda     HW_PPU_DATA
        ldy     BG_COL
        sta     ($19),y
        lda     HW_PPU_DATA
        sta     $DF
        lda     BG_ROW
        ora     BG_COL
        and     #$01
        bne     bg_read_next
        lda     $DF
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        and     #$03
        sta     $DF
        lda     BG_COL
        and     #$02
        sta     $DE
        lda     BG_ROW
        and     #$02
        asl     a
        ora     $DE
        tax
        lda     $DF
        cpx     #$00
        beq     bg_read_put
bg_read_shift:
        asl     a
        asl     a
        dex
        dex
        bne     bg_read_shift
bg_read_put:
        sta     $DF
        lda     BG_COL
        lsr     a
        lsr     a
        clc
        adc     BG_ATTR_ROW
        tay
        lda     ($EB),y
        ora     $DF
        sta     ($EB),y
bg_read_next:
        inc     BG_COL
        lda     BG_COL
        cmp     #$20
        bne     bg_read_col
        clc
        lda     $19
        adc     #$20
        sta     $19
        bcc     bg_read_advance
        inc     $1A
bg_read_advance:
        inc     BG_ROW
        lda     BG_ROW
        cmp     #$1E
        bne     bg_read_row
        rts

; NMI 命令 16：读 $4A/$4B 处图块的调色板组（0-3）到 $73
nmi_read_group:
        jsr     LB52E
        lda     HW_PPU_DATA
        lda     HW_PPU_DATA
        lda     HW_PPU_DATA
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        and     #$03
        sta     $73
        jmp     L89C9

; NMI 命令 17：把 $73 的调色板组写给 $4A/$4B 所在的 2x2 个图块（原版一个属性位对应 2x2 个图块），只改高字节
nmi_write_group:
        lda     $73
        asl     a
        asl     a
        asl     a
        asl     a
        ora     V_TILE_HIGH
        pha
        lda     $4A
        and     #$DE
        sta     $4A
        ldy     #$00
nmi_write_group_tile:
        lda     $4B
        ldx     $4A
        jsr     L81EC
        bit     HW_PPU_DATA
        pla
        pha
        sta     HW_PPU_DATA
        clc
        lda     $4A
        adc     nmi_write_group_step,y
        sta     $4A
        bcc     nmi_write_group_next
        inc     $4B
nmi_write_group_next:
        iny
        cpy     #$04
        bne     nmi_write_group_tile
        pla
        jmp     L89C9
nmi_write_group_step:
        .byte   $01, $1F, $01, $00

; 图形编辑器的颜色条：第 24、25 行的第 14-31 列的图块调色板组改成 $F4 里的高字节
bar_row:
        ldx     #$12
bar_row_tile:
        bit     HW_PPU_DATA
        lda     $F4
        sta     HW_PPU_DATA
        dex
        bne     bar_row_tile
        rts

; ------------------------------------------------------------------------
; 启动与退出时的画面处理：启动期间显示一直关着，退出前清屏，免得 DOS 重画之前露出 BASIC 的画面。
; ------------------------------------------------------------------------

; 写 $2001（A = 值，保持不变）：启动期间不关显示（显示关着时屏幕是灰色），免得启动时每次清屏都闪一下灰屏；启动时名称表已经是空白，清屏也跳过了
video_write_mask:
        pha
        lda     SHELL_BOOTING
        beq     video_mask_write
        pla
        pha
        and     #HW_MASK_BG
        beq     video_mask_skip
video_mask_write:
        pla
        sta     HW_PPU_MASK
        rts
video_mask_skip:
        pla
        rts

; 启动完成（进入主循环）：确保显示已打开；不改 A、X、Y
boot_done:
        pha
        lda     SHELL_BOOTING
        beq     boot_done_exit
        lda     #$00
        sta     SHELL_BOOTING
        txa
        pha
        tya
        pha
        jsr     LB562
        pla
        tay
        pla
        tax
boot_done_exit:
        pla
        rts

; 退出回 DOS：先把调色板全写成黑色，再还原 DOS 的空白名称表（页 0 是 $0080，其余页是 $0000，实测 DOS 清屏后如此）、精灵藏起来、滚动归零，
; 最后还原 DOS 的调色板再交给外壳；显示一直开着，所以没有关显示时的灰屏，DOS 重画之前也不会露出 BASIC 的画面
exit_to_dos:
        sei
        lda     #$00
        sta     HW_PPU_CTRL
        jsr     black_palette
        jsr     LAEEC
        lda     #$00
        sta     $2003
        lda     #$02
        sta     $4014
        lda     #$10                            ; 先清零整页，盖住页 2、3
        sta     V_COPY_LEN
        lda     #$00
        tax
        jsr     fill_window_page
        lda     SHELL_IMAGE_WINDOW + 6
        ldy     #$10
        jsr     dma_to_video
        lda     #$08                            ; 前半页（页 0 的 1024 个单元）填成 $0080，后半页（页 1）仍是 0
        sta     V_COPY_LEN
        lda     #$80
        ldx     #$00
        jsr     fill_window_page
        lda     SHELL_IMAGE_WINDOW + 6
        ldy     #$00
        jsr     dma_to_video
        bit     HW_PPU_STATUS
        lda     #$00
        sta     HW_PPU_SCROLL
        sta     HW_PPU_SCROLL
        jsr     LB42B
        ldx     #$3F
exit_palette:
        lda     dos_palette,x
        sta     $2040,x
        dex
        bpl     exit_palette
        jmp     SHELL_LEAVE

; 调色板全写成黑色（$0F）并打开背景显示：等垂直消隐再写；之后显存里的变化都看不见
black_palette:
        jsr     LB42B
        ldx     #$3F
        lda     #$0F
black_palette_loop:
        sta     $2040,x
        dex
        bpl     black_palette_loop
        lda     #$0E
        sta     HW_PPU_MASK
        rts

; 进入时的 DOS 调色板（映像的页是运行时的内存，可以写）；只在第一次进入时保存，热启动不再保存
palette_save:
        lda     dos_palette_saved
        bne     palette_save_exit
        ldx     #$3F
palette_save_loop:
        lda     $2040,x
        and     #$3F
        sta     dos_palette,x
        dex
        bpl     palette_save_loop
        inc     dos_palette_saved
palette_save_exit:
        rts
dos_palette_saved:
        .byte   $00
dos_palette:
        .res    64
