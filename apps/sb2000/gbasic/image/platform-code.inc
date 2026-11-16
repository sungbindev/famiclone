; ------------------------------------------------------------------------
; 名称：platform.s
; 说明：GBASIC 移植版的平台层：启动、退出，以及 SB2000 的画面、键盘、声音、文件的原生实现。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：与 gbasic.s 合并汇编：本文件用 gbasic.s 里的标签（L8000 等），gbasic.s 也调用这里导出的例程。
; ------------------------------------------------------------------------
        .export         image_entry, irq_handler
        .import         stub_leave, stub_irq_mask

        .segment        "CODE"

; 映像入口：由 ERAM 小段 stub_launch 跳入，此时窗口 0-7 是本映像的页，零页与 $0100-$07FF 是干净的
image_entry:
        sei
        cld
        ldx     #$FF
        txs
        jsr     video_init
        cli                                     ; 键盘由 IRQ 驱动
        jmp     L8000

; 退出到 DOS：由菜单项 3 进入
L7D04:  lda     #$00
        sta     HW_PPU_CTRL
        sta     HW_PPU_MASK
        jmp     stub_leave

; 画面初始化：关显示与 NMI，4 色背景模式，把图案与字库放进扩展显存
; 写 PPU 控制寄存器：A = 原版的 NES 控制值。NES 的位 3（精灵图案表）在 6576 上是 $2008 位 2，位 4（背景图案表）要写进图块条目的高字节（video_tile_high），
; 文字引擎按 $4F 选名称表，显示的名称表跟着它（$20 对应名称表 0，$24 对应名称表 1）
video_write_ctrl:
        pha
        and     #$10
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        ora     #$08
        cmp     video_tile_high
        beq     @table_same
        sta     video_tile_high
        jsr     video_retable                   ; 背景图案表变了，已经在屏幕上的图块也要跟着变
@table_same:
        pla
        pha
        and     #$08                            ; 精灵图案表：NES 的表 0、1 在字符 RAM $2000、$3000，$2008 位 3-2 = 2、3
        lsr     a
        ora     #$08
        sta     HW_PPU_EXT                      ; 位 7 = 0：4 色背景
        pla
        and     #$A7                            ; NMI、8x16、地址增量、名称表位 1-0
        sta     $F6
        lda     $4F
        lsr     a
        lsr     a
        and     #$01
        ora     $F6
        sta     HW_PPU_CTRL
        rts

; 把名称表 0、1（CRAM $0000-$0FFF，共 2048 个图块）每个图块条目高字节的图案表位改成 video_tile_high，调色板组不变；
; 显示关闭期间做（CGSET 语句切换背景图案表时调用，主程序里，不在 NMI 里）
video_retable:
        jsr     LABF1
        lda     #$01                            ; $FE/$FF = 第一个条目高字节的 CRAM 地址
        sta     $FE
        lda     #$00
        sta     $FF
L_retable_loop:
        jsr     L_retable_address
        lda     HW_PPU_DATA                     ; 读缓冲
        lda     HW_PPU_DATA
        and     #$F0
        ora     video_tile_high
        tay
        jsr     L_retable_address
        sty     HW_PPU_DATA
        lda     $FE
        clc
        adc     #$02
        sta     $FE
        bcc     L_retable_next
        inc     $FF
L_retable_next:
        lda     $FF
        cmp     #$10
        bcc     L_retable_loop
        jmp     LABFA

L_retable_address:
        bit     HW_PPU_STATUS
        lda     $FF
        sta     HW_PPU_ADDRESS
        lda     $FE
        sta     HW_PPU_ADDRESS
        rts

video_init:
        lda     #$00
        sta     HW_PPU_CTRL
        sta     HW_PPU_MASK
        sta     HW_PPU_EXT                      ; 4 色背景，精灵图案表 0，名称表在 CRAM
        sta     $4300                           ; 扩展显存 bank 0
        lda     #VIDEO_TILE_HIGH
        sta     video_tile_high
        lda     #$10
        sta     HW_PPU_CTRL                     ; 背景用图案表 1，NMI 仍关
        lda     #VIDEO_PATTERN_VRAM_HIGH        ; 背景图案：扩展显存 $8000 起
        jsr     video_copy_patterns
        lda     #VIDEO_SPRITE_PATTERN_HIGH      ; 精灵图案：字符 RAM $2000 起
        jsr     video_copy_patterns
        lda     #$00                            ; 清 4 个名称表：4096 个图块，空格、调色板组 0
        sta     HW_PPU_ADDRESS
        sta     HW_PPU_ADDRESS
        ldx     #$10
        ldy     #$00
@clear: lda     #$20
        sta     HW_PPU_DATA
        lda     video_tile_high
        sta     HW_PPU_DATA
        dey
        bne     @clear
        dex
        bne     @clear
        lda     #GK_CAPS                        ; 字母默认大写
        sta     gk_flags
        rts

; NMI 命令 16：读 $4A/$4B 处图块的调色板组（0-3）到 $73
L_nmi_group_read:
        jsr     LABD8
        lda     HW_PPU_DATA                     ; 读缓冲
        lda     HW_PPU_DATA                     ; 图案号
        lda     HW_PPU_DATA                     ; 高字节
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        and     #$03
        sta     $73
        jmp     L8867

; NMI 命令 17：把 $9C（0-3）写成 $4A/$4B 起 2x2 个图块的调色板组
L_nmi_group_write:
        jsr     L_group_write_block
        jmp     L8867

; 把 $9C（0-3）写成 $4A/$4B 起 2x2 个图块的调色板组，只改高字节，图案表位不变；改 $4A/$4B、A、Y
L_group_write_block:
        jsr     L_group_high
        tay
        jsr     LABD8
        bit     HW_PPU_DATA
        sty     HW_PPU_DATA
        bit     HW_PPU_DATA
        sty     HW_PPU_DATA
        lda     $4A                             ; 下一行：名称表地址加 32
        clc
        adc     #$20
        sta     $4A
        bcc     L_group_second_row
        inc     $4B
L_group_second_row:
        jsr     LABD8
        bit     HW_PPU_DATA
        sty     HW_PPU_DATA
        bit     HW_PPU_DATA
        sty     HW_PPU_DATA
        rts

; ---- 名称表 1 的存取（图形编辑器的 FILE 存取）：文件里仍是原版的 1K 格式：960 个图块号，后面 64 个属性字节；
; 属性字节每个管 4x4 个图块，4 个 2x2 块的调色板组各占 2 位（左上、右上、左下、右下由低位到高位）。
; 6576 里调色板组在每个图块条目的高字节，所以读时从每个块左上图块的条目取，写时写进块里 4 个图块 ----

; 读名称表 1 的第 A 页（0-3，每页 256 字节）到 $0700；页 3 是最后 192 个图块号加 64 个属性字节
video_read_nt_page:
        cmp     #$03
        beq     L_nt_read_last
        clc
        adc     #$24
        ldx     #$00
        jsr     L81F8
        lda     HW_PPU_DATA                     ; 读缓冲
        ldx     #$00
L_nt_read_tiles:
        lda     HW_PPU_DATA
        sta     $0700,x
        bit     HW_PPU_DATA
        inx
        bne     L_nt_read_tiles
        rts
L_nt_read_last:
        lda     #$27
        ldx     #$00
        jsr     L81F8
        lda     HW_PPU_DATA
        ldx     #$00
L_nt_read_last_tiles:
        lda     HW_PPU_DATA
        sta     $0700,x
        bit     HW_PPU_DATA
        inx
        cpx     #$C0
        bne     L_nt_read_last_tiles
        lda     #$00
        sta     $EB
L_nt_attr_read_loop:
        jsr     L_nt_attr_read
        ldx     $EB
        sta     $07C0,x
        inc     $EB
        lda     $EB
        cmp     #$40
        bne     L_nt_attr_read_loop
        rts

; 把 $0700 的 256 个文件字节写进名称表 1 的第 A 页（0-3），格式同上
video_write_nt_page:
        cmp     #$03
        beq     L_nt_write_last
        clc
        adc     #$24
        ldx     #$00
        jsr     L81F8
        ldx     #$00
L_nt_write_tiles:
        lda     $0700,x
        put_tile
        inx
        bne     L_nt_write_tiles
        rts
L_nt_write_last:
        lda     #$27
        ldx     #$00
        jsr     L81F8
        ldx     #$00
L_nt_write_last_tiles:
        lda     $0700,x
        put_tile
        inx
        cpx     #$C0
        bne     L_nt_write_last_tiles
        lda     #$00
        sta     $EB
L_nt_attr_write_loop:
        jsr     L_nt_attr_write
        inc     $EB
        lda     $EB
        cmp     #$40
        bne     L_nt_attr_write_loop
        rts

; 第 $EB 个属性字节管的 4x4 图块的左上角行、列 -> $E3、$E4
L_nt_attr_origin:
        lda     $EB
        lsr     a
        lsr     a
        lsr     a
        asl     a
        asl     a
        sta     $E3
        lda     $EB
        and     #$07
        asl     a
        asl     a
        sta     $E4
        rts

; 第 $EA 个 2x2 块（0 左上、1 右上、2 左下、3 右下）的左上图块的行、列 -> $E6、$E7
L_nt_quad_row_col:
        lda     $EA
        and     #$02
        clc
        adc     $E3
        sta     $E6
        lda     $EA
        and     #$01
        asl     a
        clc
        adc     $E4
        sta     $E7
        rts

; 名称表 1 的 $E6 行 $E7 列图块的 NES 地址：A = 高字节，X = 低字节
L_nt_tile_address:
        lda     $E6
        and     #$07
        asl     a
        asl     a
        asl     a
        asl     a
        asl     a
        ora     $E7
        tax
        lda     $E6
        lsr     a
        lsr     a
        lsr     a
        clc
        adc     #$24
        rts

; 读第 $EB 个属性字节 -> A
L_nt_attr_read:
        jsr     L_nt_attr_origin
        lda     #$00
        sta     $E5
        lda     #$03
        sta     $EA
L_nt_attr_read_quad:
        jsr     L_nt_quad_row_col
        jsr     L_nt_tile_address
        jsr     L81F8
        lda     HW_PPU_DATA                     ; 读缓冲
        lda     HW_PPU_DATA                     ; 图案号
        lda     HW_PPU_DATA                     ; 高字节
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        and     #$03
        sta     $E8
        asl     $E5
        asl     $E5
        lda     $E5
        ora     $E8
        sta     $E5
        dec     $EA
        bpl     L_nt_attr_read_quad
        lda     $E5
        rts

; 把 $07C0 + $EB 处的属性字节写成 4 个 2x2 块的调色板组
L_nt_attr_write:
        jsr     L_nt_attr_origin
        ldx     $EB
        lda     $07C0,x
        sta     $E5
        lda     #$00
        sta     $EA
L_nt_attr_write_quad:
        lda     $E5
        and     #$03
        sta     $9C
        lsr     $E5
        lsr     $E5
        jsr     L_nt_quad_row_col
        jsr     L_nt_tile_address
        sta     $4B
        stx     $4A
        jsr     L_group_write_block
        inc     $EA
        lda     $EA
        cmp     #$04
        bne     L_nt_attr_write_quad
        rts

; NMI 命令 18：把 $9C（0-3）写成 $4A/$4B 起连续 16 个图块的调色板组
L_nmi_group_row:
        jsr     L_group_high
        tay
        jsr     LABD8
        ldx     #$10
L_group_row_loop:
        bit     HW_PPU_DATA
        sty     HW_PPU_DATA
        dex
        bne     L_group_row_loop
        jmp     L8867

; 图块条目的新高字节：调色板组（$9C 的低 2 位）在高 4 位，低 4 位是背景图案表位
L_group_high:
        lda     $9C
        and     #$03
        asl     a
        asl     a
        asl     a
        asl     a
        ora     video_tile_high
        rts

; 把映像里的 8K 图案数据（两个 4K 图案表）复制到显存：A = 目的地址高字节，低字节为 0
video_copy_patterns:
        bit     HW_PPU_STATUS
        sta     HW_PPU_ADDRESS
        lda     #$00
        sta     HW_PPU_ADDRESS
        lda     #<LC410
        sta     $FE
        lda     #>LC410
        sta     $FF
        ldx     #$20
        ldy     #$00
@copy:  lda     ($FE),y
        sta     HW_PPU_DATA
        iny
        bne     @copy
        inc     $FF
        dex
        bne     @copy
        rts

; ---- 临时：文件层改写（4.6）之前，保留原版文件服务用到的参数单元，调用一律失败 ----
L7B03:  .res    2
L7B05:  .res    1
L7B06:  .res    1
L7B07:  .res    1
L7B08:  .res    1
L7B09:  .res    1
L7B0A:  .res    1
L7B0B:  .res    1
L7B0C:  .res    1
L7B0D:  .res    1
L7D03:  .res    1
L7EEA:  .res    2
L7EEC:  .res    1
L7EED:  .res    1
L7EEE:  .res    1
L7EEF:  .res    1
L7EF0:  .res    1
L7B33:  sec
        rts
L7EE0:  sec
        rts

; 直接方式执行完后“下一行”指针指向的空行（长度字节为 0 表示程序结束）；原版借用了键盘码表的第一个字节，码表删除后单独放一个
LB144:  .byte   $00, $00, $00

; ---- 键盘：AT 键盘的扫描码集 1（释放码 = 按下码 | $80，扩展键前导 $E0），维护修饰键与当前按着的键 ----
; IRQ 只开键盘（IMAGE_IRQ_MASK）：每收到一个扫描码进一次，读走码、清除请求，交给 gk_scan_code；协议与内核的 KBD_HANDLER 相同
irq_handler:
        pha
        txa
        pha
        tya
        pha
        lda     HW_IRQ_PENDING
        and     #HW_IRQ_KEYBOARD
        beq     L_irq_done
        lda     #$00
        sta     HW_KEY_CONTROL
        lda     HW_KEY_DATA
        pha
        lda     #$FF                            ; 先全部屏蔽以清除请求，再恢复映像的屏蔽
        sta     HW_IRQ_MASK
        lda     #IMAGE_IRQ_MASK
        sta     HW_IRQ_MASK
        lda     #$80
        sta     HW_KEY_CONTROL
        pla
        jsr     gk_scan_code
L_irq_done:
        pla
        tay
        pla
        tax
        pla
        rti

; 按键是电平的（按着就一直返回），按键重复、去抖由调用者 LB263 处理；按键状态由 IRQ 处理程序维护
; 等到下一帧再取键：LB263 的重复、闪烁与去抖都以帧为单位计时
gk_poll:
        lda     gk_frame
L_gk_wait:
        cmp     gk_frame
        beq     L_gk_wait
        jmp     LAF88
; 输出：A = 当前按着的键翻译出的内部码（原版的码域：ASCII、$08 退格、$0D 回车、$1C-$1F 光标、$F0-$F7 功能键），没有键时为 0（Z 置位）
; 按键是电平的（按着就一直返回），按键重复、去抖由调用者 LB263 处理；按键状态由 IRQ 处理程序维护
LAF9B:
        ldx     gk_held
        beq     L_gk_none
        bmi     L_gk_extended
        cpx     #$3A
        bcs     L_gk_function
        lda     gk_flags
        and     #GK_CTRL
        bne     L_gk_ctrl
        lda     gk_normal,x
        cmp     #'a'
        bcc     L_gk_symbol
        cmp     #'z' + 1
        bcs     L_gk_symbol
        pha                                     ; 字母：Shift 与 CapsLock 不同时为大写
        lda     gk_flags
        lsr     a
        lsr     a
        lsr     a
        eor     gk_flags
        and     #$01
        beq     L_gk_lower
        pla
        and     #$DF
        rts
L_gk_lower:
        pla
        rts
L_gk_symbol:
        pha
        lda     gk_flags
        and     #GK_SHIFT
        beq     L_gk_plain
        pla
        lda     gk_shifted,x
        rts
L_gk_plain:
        pla
        rts
L_gk_ctrl:
        lda     gk_normal,x                     ; Ctrl + 字母 = 控制码 $01-$1A
        cmp     #'a'
        bcc     L_gk_none
        cmp     #'z' + 1
        bcs     L_gk_none
        and     #$1F
        rts
L_gk_function:
        cpx     #$3B                            ; F1-F8 = $3B-$42 -> $F0-$F7
        bcc     L_gk_none
        cpx     #$43
        bcs     L_gk_none
        txa
        clc
        adc     #$F0 - $3B
        rts
L_gk_extended:
        txa
        and     #$7F
        ldx     #gk_extended_keys_end - gk_extended_keys - 2
L_gk_find:
        cmp     gk_extended_keys,x
        beq     L_gk_found
        dex
        dex
        bpl     L_gk_find
        bmi     L_gk_none
L_gk_found:
        lda     gk_extended_keys + 1,x
        rts
L_gk_none:
        lda     #$00
        rts

; 处理一个原始扫描码（A）：前缀、修饰键、按下与释放
gk_scan_code:
        cmp     #$E0
        bne     L_gk_not_e0
        lda     #$01
        sta     gk_extended
        rts
L_gk_not_e0:
        cmp     #$E1                            ; Pause 的前缀，忽略
        beq     L_gk_done
        sta     gk_raw
        and     #$7F
        tax
        cpx     #$2A
        beq     L_gk_shift
        cpx     #$36
        beq     L_gk_shift
        cpx     #$1D
        beq     L_gk_ctrl_key
        cpx     #$38
        beq     L_gk_alt
        cpx     #$3A
        beq     L_gk_caps
        lda     gk_extended                     ; 扩展键的位 7 置位
        beq     L_gk_ordinary
        txa
        ora     #$80
        tax
L_gk_ordinary:
        lda     #$00
        sta     gk_extended
        lda     gk_raw
        bmi     L_gk_release
        stx     gk_held                         ; 按下：成为按着的键
        rts
L_gk_release:
        cpx     gk_held                         ; 放开的是按着的那个键
        bne     L_gk_done
        lda     #$00
        sta     gk_held
L_gk_done:
        rts
L_gk_shift:
        lda     #GK_SHIFT
        bne     L_gk_modifier
L_gk_ctrl_key:
        lda     #GK_CTRL
        bne     L_gk_modifier
L_gk_alt:
        lda     #GK_ALT
L_gk_modifier:
        ldx     #$00
        stx     gk_extended
        ldx     gk_raw
        bmi     L_gk_clear
        ora     gk_flags
        sta     gk_flags
        rts
L_gk_clear:
        eor     #$FF
        and     gk_flags
        sta     gk_flags
        rts
L_gk_caps:
        lda     #$00
        sta     gk_extended
        lda     gk_raw
        bmi     L_gk_done                       ; CapsLock 只在按下时切换
        lda     gk_flags
        eor     #GK_CAPS
        sta     gk_flags
        rts

; 扫描码 $00-$39 的 ASCII：不带 Shift 与带 Shift；字母在不带 Shift 的表里用小写，大小写由 Shift 与 CapsLock 决定
gk_normal:
        .byte   $00, $1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', $08, $09
        .byte   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', $0D, $00, 'a', 's'
        .byte   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', $27, '`', $00, $5C, 'z', 'x', 'c', 'v'
        .byte   'b', 'n', 'm', ',', '.', '/', $00, '*', $00, ' '
gk_shifted:
        .byte   $00, $1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', $08, $09
        .byte   'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', $0D, $00, 'A', 'S'
        .byte   'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', $22, '~', $00, '|', 'Z', 'X', 'C', 'V'
        .byte   'B', 'N', 'M', '<', '>', '?', $00, '*', $00, ' '

; 扩展键（$E0 前缀）的扫描码、内部码：上 $1E、下 $1F、左 $1D、右 $1C、Home $0B、Insert $12、Delete $08、小键盘回车 $0D
gk_extended_keys:
        .byte   $48, $1E, $50, $1F, $4B, $1D, $4D, $1C, $47, $0B, $52, $12, $53, $08, $1C, $0D
gk_extended_keys_end:
