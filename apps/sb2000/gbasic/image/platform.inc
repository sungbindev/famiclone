; ------------------------------------------------------------------------
; 名称：platform.inc
; 说明：GBASIC 移植版的平台符号：显存布局、调色板与宏。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：6576（SB2000）的显示布局，见 apps/sb2000/bbk-bios/src/hal-ppu.asm 与 packages/fcdev/docs/sb2000/03-硬件说明.md：
;       图案放在扩展显存 $8000 起（NES 的图案表 0、1 依次在 $8000、$9000），名称表在 CRAM，每个图块 2 字节：
;       低字节 = 图案号，高字节 = 调色板组 << 4 | $8 | 图案表号（图案表 1 时为 $9）。
; ------------------------------------------------------------------------

; 背景图案表号：原版 PPUCTRL 位 4 为 1，背景用图案表 1
VIDEO_TILE_HIGH = $09                   ; 图块高字节的低 4 位：$8 | 图案表号 1
VIDEO_PATTERN_VRAM_HIGH = $80           ; 扩展显存里图案区的 $2006 高字节（$8000 起）

; 写一个图块条目到名称表当前地址（2 字节：图案号、高字节）；输入 A = 图案号，改 A
.macro  put_tile
        sta     HW_PPU_DATA
        lda     #VIDEO_TILE_HIGH
        sta     HW_PPU_DATA
.endmacro

; 键盘（AT 键盘，扫描码集 1）：状态放在 $04E8 起（原版的键盘矩阵区，已不用）
gk_flags        = $04E8                 ; 修饰键状态
gk_held         = $04E9                 ; 正按着的键：扫描码，扩展键的位 7 置位，0 为没有
gk_extended     = $04EA                 ; 刚收到 $E0 前缀
gk_raw          = $04EB                 ; 刚收到的原始扫描码
GK_SHIFT        = $01                   ; gk_flags：Shift
GK_CTRL         = $02                   ; Ctrl
GK_ALT          = $04                   ; Alt
GK_CAPS         = $08                   ; CapsLock
