; ------------------------------------------------------------------------
; 名称：nes.s
; 说明：在裕兴学习机的 DOS 下运行 NES 游戏：GAME.EXE 列出软盘根目录里的 .NES 文件，手柄选择后运行，支持 mapper 0 与 mapper 4（MMC3）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：裕兴的 DOS 不接受命令行参数，所以程序自己在根目录里列出扩展名是 NES 的文件，由手柄选择。
;       流程：探测 DOS 给 $6000 窗口选的是哪一块 RAM（$5500 位 0-1）-> 软驱读根目录列出 NES 文件 -> 手柄选择 -> 沿簇链读文件
;       -> 读第一个扇区检查头 -> PRG 按 16K 一页写入程序 RAM 的第 0 页起，CHR 写入字符 RAM
;       -> mapper 0：改成 32K 模式（16K 的 PRG 先复制成两份）、设镜像，JMP ($FFFC)；
;       mapper 4：把 PRG 最后的 16K 复制到程序 RAM 第 31 页（MMC3 的固定页在 8K 页 62、63），切到 MMC3 模式，JMP ($FFFC)。
;       程序运行在 $6000 窗口，窗口里的 RAM 不随程序窗口的模式改变；复制 PRG 与切换 MMC3 的代码放在内部 RAM $0300 运行。
;       MMC3 的图案页号经字符页重排进字符 RAM：游戏的第 f 个 8K 图案放进第 swap(f) 页，swap 把页号的位 3-2 与位 1-0 互换。
;       只有程序 RAM 有 1MB 的型号（V9.0-98）支持 mapper 4；程序 RAM 只有 128K 的型号（V8.x）只支持 mapper 0。
;       出错时用 ROM 的显示功能显示英文提示并停住，需要复位；进入游戏后不能回到 DOS。
; ------------------------------------------------------------------------

        .setcpu "6502"
        .include "yuxing.inc"
        .export _main

; 长距离条件分支：条件不满足时短跳过，满足时用 JMP 去远处的标签
.macro JCS target
        bcc :+
        jmp target
:
.endmacro

.macro JNE target
        beq :+
        jmp target
:
.endmacro

.macro JEQ target
        bne :+
        jmp target
:
.endmacro

; 软驱控制器的寄存器（ember/docs/02-硬件模块.md 第 6.12 节）
FDC_DCR         = $4200         ; 数据率（写）
FDC_DOR         = $4201         ; 数字输出（写）
FDC_DATAW       = $4205         ; 数据（写）
FDC_MSR         = $4304         ; 主状态（读）
FDC_DATAR       = $4305         ; 数据（读）

; 整机寄存器
REG_ROM         = $4800         ; 位 0-4 ROM 页，位 5 为 1 时写 $8000 起是程序 RAM 的页寄存器，位 7 汉字模式
REG_MODE        = $5500         ; 位 0-1 $6000 窗口，位 2 程序 RAM 模式，位 3 水平镜像，位 6 为 1 时 32K 页
REG_CHR         = $5501         ; 位 0-3 字符页，位 7 MMC3 模式

; DOS 运行程序时 $4800 的值（V8.2-D、V9.0-98 都是 $90）：出错显示前恢复 ROM 模式用
DOS_REG_ROM     = $90

; 菜单
MENU_FIRST      = 3             ; 第一项的行号，每项占两行（汉字是 16 点高）
MENU_VIS        = 10            ; 一屏显示的项数
MENU_HELP_ROW   = 24            ; 操作提示的行号
PAD_START       = $10           ; 手柄读数的位：A 在位 7，依次到右键在位 0
PAD_UP          = $08
PAD_DOWN        = $04

; 内部 RAM 里运行的代码的位置
RAM_BASE        = $0300

; 出错码（与 msg_table 的顺序一致）
E_DISK          = 1
E_NOTFOUND      = 2
E_INES          = 3
E_MAPPER        = 4
E_SIZE          = 5
E_TRUNC         = 6
E_TRAINER       = 7

        .segment "ZEROPAGE"

ptr:            .res 2          ; 源指针
ptr2:           .res 2          ; 目录项指针
dstp:           .res 2          ; 程序 RAM 的写入指针，用 (dstp,X) 写
cnt:            .res 2
tmo:            .res 2          ; 等待软驱的超时计数
track:          .res 1          ; 软驱当前磁道
curc:           .res 2          ; 当前簇号
lbaLo:          .res 1          ; 要读的扇区号
lbaHi:          .res 1
headv:          .res 1
secv:           .res 1
st0:            .res 1
st1:            .res 1
st2:            .res 1
dirn:           .res 1          ; 根目录剩余扇区数
fatSec:         .res 1          ; 当前 FAT 缓冲里的扇区号
destp:          .res 2          ; 读扇区的目的地址
rdp:            .res 2          ; 读数据时的写入指针
trkn:           .res 1
oLo:            .res 1          ; FAT 字节偏移
oHi:            .res 1
plo:            .res 1          ; FAT 项在缓冲里的偏移
phi:            .res 1
fsec:           .res 1
b0:             .res 1          ; FAT 项的两个字节
b1:             .res 1
pass:           .res 1          ; 扇区内两遍处理的计数
hdrflag:        .res 1          ; 头部是否已经解析
tmp:            .res 1
tmp2:           .res 1
err:            .res 1          ; 出错码
sel:            .res 1          ; DOS 给 $6000 窗口选的 RAM（$5500 位 0-1）
sig:            .res 8          ; 程序开头 8 个字节，探测窗口时用来认出自己
dsize:          .res 4          ; 目录项里的文件大小
req:            .res 2          ; 头里写的数据需要的 256 字节页数
skipL:          .res 1          ; 头部剩余要跳过的字节
prg_units:      .res 1          ; PRG 的 16K 个数
chr_units:      .res 1          ; CHR 的 8K 个数
flags6:         .res 1
mapper:         .res 1
page16:         .res 1          ; 正在写的程序 RAM 页（16K）
prg_active:     .res 1          ; 正在写 PRG
chr_active:     .res 1          ; 正在写 CHR
all_done:       .res 1          ; 全部数据写完
chr_blk:        .res 1          ; 正在写的 8K 图案块
chr_cnt:        .res 2          ; 当前图案块剩余字节
src_page:       .res 1          ; 复制 16K 的来源页与目的页
dst_page:       .res 1
chunk:          .res 1
eptr:           .res 2          ; 游戏表项的指针
padacc:         .res 1          ; 手柄读数的累加
pressed:        .res 1          ; 刚按下的键
bidx:           .res 1          ; 正在写入菜单行的表项序号

        .segment "BSS"

; 工作 RAM 里的缓冲：SECTBUF 放一个扇区，FATBUF 放 FAT 的一个扇区
SECTBUF:        .res 512
FATBUF:         .res 512

        .segment "MENU"

; 游戏表：每项 16 字节，主名 8、首簇 2、大小 4、备用 2；MENU 段不进文件，放在程序窗口末尾
ENTRIES:        .res 224 * 16
ROWBUF:         .res 16         ; 菜单一行的显示字符串
mv_count:       .res 1
mv_sel:         .res 1
mv_top:         .res 1
mv_prev:        .res 1
mv_pad:         .res 1
mv_k:           .res 1

        .segment "RODATA"

msg_table:
        .word msg_disk, msg_notfound, msg_ines, msg_mapper
        .word msg_size, msg_trunc, msg_trainer

; 字符串用 ROM 显示功能的格式：定位到第 10 行第 2 列，以 STR_END 结束
msg_disk:       .byte STR_LOCATE, 2, 10, "Disk read error", STR_END
msg_notfound:   .byte STR_LOCATE, 2, 10, "No .NES file on the disk", STR_END
msg_ines:       .byte STR_LOCATE, 2, 10, "Not an iNES file", STR_END
msg_mapper:     .byte STR_LOCATE, 2, 10, "Mapper not supported", STR_END
msg_size:       .byte STR_LOCATE, 2, 10, "Size not supported", STR_END
msg_trunc:      .byte STR_LOCATE, 2, 10, "File is truncated", STR_END
msg_trainer:    .byte STR_LOCATE, 2, 10, "Trainer not supported", STR_END
msg_title:      .byte STR_LOCATE, 2, 1, "NES GAMES", STR_END
msg_help:       .byte STR_LOCATE, 2, MENU_HELP_ROW, "UP/DOWN  START=RUN", STR_END

; 目录项里首簇（26-27）与大小（28-31）的位置，按这个顺序复制进游戏表
copy_src:       .byte 26, 27, 28, 29, 30, 31

        .segment "CODE"

; 入口：由启动代码调用（已经 SEI）；成功时不会返回，失败时显示提示后停住
_main:
        lda #0
        sta $2000                       ; 关 NMI 与显示，之后不再有中断打扰
        sta $2001
        sta $4015
        lda #$40
        sta $4017                       ; 关 APU 帧中断
        jsr probe_window
        jsr fdc_init
        JCS disk_err
        jsr scan_nes
        JCS fail_set
        jsr menu
        jsr select_entry
        jsr probe_window                ; 菜单的 BIOS 调用会改写 $0380 一带，装载前重新复制 RAM 代码
        jsr load_file
        JCS fail_set
        lda mapper
        bne @mmc3
        jmp start_nrom
@mmc3:
        jmp start_mmc3
disk_err:
        lda #E_DISK
        sta err
fail_set:
        jmp fail

; 把内部 RAM 里运行的代码复制到 RAM_BASE，再探测 $6000 窗口：DOS 给它选的 RAM 里放着本程序，
; 逐个试 $5500 位 0-1 的 4 种取值，窗口开头与程序开头一致的就是；结果放进 sel
probe_window:
        ldx #(ram_end - ram_start - 1)
@copy:
        lda ram_start,x
        sta RAM_BASE,x
        dex
        cpx #$FF
        bne @copy
        ldx #7
@sig:
        lda $6000,x
        sta sig,x
        dex
        bpl @sig
        jsr RAM_BASE + (ram_probe - ram_start)
        sta sel
        rts

; ------------------------------------------------------------------------
; 在内部 RAM 里运行的代码（位置无关，复制到 RAM_BASE）
; ------------------------------------------------------------------------
ram_start:

; 探测 $6000 窗口：返回 A 是与程序一致的窗口选择（0-3），都不一致时返回 $FF
ram_probe:
        ldx #0
@try:
        stx REG_MODE
        ldy #7
@cmp:
        lda $6000,y
        cmp sig,y
        bne @next
        dey
        bpl @cmp
        txa
        rts
@next:
        inx
        cpx #4
        bne @try
        lda #$FF
        rts

; 把程序 RAM 的 src_page 页（16K）复制到 dst_page 页，每次 256 字节，用 SECTBUF 当中转
ram_copy16:
        lda #0
        sta chunk
@chunk:
        lda #$20
        sta REG_ROM
        lda src_page
        sta $8000
        lda #0
        sta REG_ROM
        lda chunk
        clc
        adc #$80
        sta ptr + 1
        lda #0
        sta ptr
        tay
@read:
        lda (ptr),y
        sta SECTBUF,y
        iny
        bne @read
        lda #$20
        sta REG_ROM
        lda dst_page
        sta $8000
        lda #0
        sta REG_ROM
@write:
        lda SECTBUF,y
        sta (ptr),y
        iny
        bne @write
        inc chunk
        lda chunk
        cmp #64
        bne @chunk
        rts

; mapper 4：PRG 最后的 16K 复制到第 31 页（MMC3 的固定页在 8K 页 62、63），切到 MMC3 模式，进入游戏
ram_mmc3:
        ldx prg_units
        dex
        stx src_page
        lda #31
        sta dst_page
        jsr RAM_BASE + (ram_copy16 - ram_start)
        lda #0
        sta REG_ROM
        lda #$04
        ora sel
        sta REG_MODE                    ; 程序 RAM 模式，16K
        lda #$80
        sta REG_CHR                     ; MMC3 模式
        jmp ($FFFC)

ram_end:

; ------------------------------------------------------------------------
; 启动游戏
; ------------------------------------------------------------------------

; mapper 0：16K 的 PRG 镜像成两份，改成 32K 页，设镜像，进入游戏
start_nrom:
        lda prg_units
        cmp #1
        bne @map
        lda #0
        sta src_page
        lda #1
        sta dst_page
        jsr RAM_BASE + (ram_copy16 - ram_start)
@map:
        lda #0
        jsr set_page                    ; 32K 页 0 就是 16K 页 0、1
        lda flags6
        and #$01
        eor #$01                        ; 头里 1 为垂直镜像；$5500 位 3 为 1 是水平镜像
        asl a
        asl a
        asl a
        ora #$44                        ; 位 2：程序 RAM 模式；位 6：32K 页
        ora sel
        sta REG_MODE
        lda #0
        sta REG_ROM
        sta REG_CHR
        jmp ($FFFC)

start_mmc3:
        jmp RAM_BASE + (ram_mmc3 - ram_start)

; 选程序 RAM 的 16K 页 A（页寄存器）；返回时 $8000 起的写入是写程序 RAM
set_page:
        pha
        lda #$20
        sta REG_ROM
        pla
        sta $8000
        lda #0
        sta REG_ROM
        rts

; 恢复 DOS 的显示状态，显示出错码 err 对应的提示，然后停住
fail:
        lda sel
        sta REG_MODE
        lda #DOS_REG_ROM
        sta REG_ROM
        lda #0
        sta REG_CHR
        jsr BIOS_INIT_TEXT
        jsr BIOS_CLEAR
        ldx err
        dex
        txa
        asl a
        tax
        lda msg_table,x
        ldy msg_table + 1,x
        jsr BIOS_PRINT
halt:
        jmp halt

; ------------------------------------------------------------------------
; 软驱：初始化、等待、命令字节
; ------------------------------------------------------------------------
fdc_init:
        lda #$00
        sta FDC_DOR             ; 复位控制器
        jsr delay
        lda #$1C
        sta FDC_DOR             ; 电机开，DMA 与中断开，取消复位，驱动器 A
        lda #$00
        sta FDC_DCR             ; 500 kbps
        jsr delay
        jsr delay
        jsr delay
        jsr delay
        lda #$03                ; SPECIFY
        jsr fdc_put
        JCS fdc_fail
        lda #$DF
        jsr fdc_put
        JCS fdc_fail
        lda #$03
        jsr fdc_put
        JCS fdc_fail
        lda #$07                ; RECALIBRATE，回到 0 磁道
        jsr fdc_put
        JCS fdc_fail
        lda #$00
        jsr fdc_put
        JCS fdc_fail
        jsr fdc_wait_idle
        JCS fdc_fail
        jsr sense_int
        JCS fdc_fail
        lda #$00
        sta track
        clc
        rts
fdc_fail:
        sec
        rts

; 把 A 写入数据寄存器（等控制器可以接收）；超时 C 置位
fdc_put:
        pha
        lda #$FF
        sta tmo
        sta tmo+1
@w:
        lda FDC_MSR
        and #$C0
        cmp #$80
        beq @go
        jsr dec_tmo
        bne @w
        pla
        sec
        rts
@go:
        pla
        sta FDC_DATAW
        clc
        rts

; 从数据寄存器读一个字节到 A（等控制器有数据）；超时 C 置位
fdc_get:
        lda #$FF
        sta tmo
        sta tmo+1
@w:
        lda FDC_MSR
        and #$C0
        cmp #$C0
        beq @go
        jsr dec_tmo
        bne @w
        sec
        rts
@go:
        lda FDC_DATAR
        clc
        rts

; 等待驱动器空闲（主状态寄存器的 DB0 清 0）；超时 C 置位
fdc_wait_idle:
        lda #$FF
        sta tmo
        sta tmo+1
@w:
        lda FDC_MSR
        and #$0F
        beq @ok
        jsr dec_tmo
        bne @w
        sec
        rts
@ok:
        clc
        rts

; 读中断状态（清除寻道或校准的中断）
sense_int:
        lda #$08
        jsr fdc_put
        JCS fdc_fail
        jsr fdc_get
        JCS fdc_fail
        sta st0
        jsr fdc_get
        JCS fdc_fail
        clc
        rts

; 把磁头移到 A 磁道，已在该磁道时什么也不做
seek_to:
        cmp track
        beq @same
        sta tmp
        lda #$0F
        jsr fdc_put
        JCS fdc_fail
        lda #$00
        jsr fdc_put
        JCS fdc_fail
        lda tmp
        jsr fdc_put
        JCS fdc_fail
        jsr fdc_wait_idle
        JCS fdc_fail
        jsr sense_int
        JCS fdc_fail
        lda tmp
        sta track
@same:
        clc
        rts

; 超时计数减一，结果为 0 时 Z 置位
dec_tmo:
        lda tmo
        bne @s
        dec tmo+1
@s:
        dec tmo
        lda tmo
        ora tmo+1
        rts

delay:
        ldx #$00
@o:
        ldy #$00
@i:
        dey
        bne @i
        dex
        bne @o
        rts

; 读一个逻辑扇区（lbaLo/lbaHi）到 destp 指向的 512 字节；C 置位表示失败
read_sector:
        lda lbaLo
        sta cnt
        lda lbaHi
        sta cnt+1
        lda #0
        sta trkn
@dv:
        lda cnt+1
        bne @sb
        lda cnt
        cmp #36
        bcc @dd
@sb:
        lda cnt
        sec
        sbc #36
        sta cnt
        lda cnt+1
        sbc #0
        sta cnt+1
        inc trkn
        jmp @dv
@dd:
        lda #0
        sta headv
        lda cnt
        cmp #18
        bcc @h0
        lda #1
        sta headv
        lda cnt
        sec
        sbc #18
        sta cnt
@h0:
        lda cnt
        clc
        adc #1
        sta secv
        lda trkn
        jsr seek_to
        JCS rd_fail
        lda #$66                ; READ DATA，MFM、跳过删除数据
        jsr fdc_put
        JCS rd_fail
        lda headv
        asl a
        asl a
        jsr fdc_put             ; 磁头号左移 2 位，驱动器 0
        JCS rd_fail
        lda trkn
        jsr fdc_put
        JCS rd_fail
        lda headv
        jsr fdc_put
        JCS rd_fail
        lda secv
        jsr fdc_put
        JCS rd_fail
        lda #2                  ; N = 2，512 字节
        jsr fdc_put
        JCS rd_fail
        lda secv                ; EOT：只读这一个扇区
        jsr fdc_put
        JCS rd_fail
        lda #$1B                ; GPL
        jsr fdc_put
        JCS rd_fail
        lda #$FF                ; DTL
        jsr fdc_put
        JCS rd_fail
        lda destp
        sta rdp
        lda destp+1
        sta rdp+1
        ldy #0
@rd1:
        jsr fdc_get
        JCS rd_fail
        sta (rdp),y
        iny
        bne @rd1
        inc rdp+1
@rd2:
        jsr fdc_get
        JCS rd_fail
        sta (rdp),y
        iny
        bne @rd2
        jsr fdc_get             ; ST0
        JCS rd_fail
        sta st0
        jsr fdc_get             ; ST1
        JCS rd_fail
        sta st1
        jsr fdc_get             ; ST2
        JCS rd_fail
        sta st2
        jsr fdc_get             ; C、H、R、N
        JCS rd_fail
        jsr fdc_get
        JCS rd_fail
        jsr fdc_get
        JCS rd_fail
        jsr fdc_get
        JCS rd_fail
        lda st0
        and #$80
        bne rd_fail             ; IC 为 10 或 11：异常结束
        lda st0
        and #$40
        beq @ic0
        lda st1                 ; IC 为 01：只有 EN 或 OR（读到末尾）才算正常
        and #$90
        beq rd_fail
@ic0:
        lda st1
        and #$25
        bne rd_fail
        lda st2
        bne rd_fail
        clc
        rts
rd_fail:
        sec
        rts

; ------------------------------------------------------------------------
; 目录与 FAT12（1.44MB 格式：根目录在扇区 19-32，数据从扇区 33 开始）
; ------------------------------------------------------------------------

; 扫描根目录，把每个扩展名是 NES 的文件记入游戏表（add_entry），个数记在 mv_count；没有文件时 err 置为出错码，C 置位
scan_nes:
        lda #0
        sta mv_count
        lda #19
        sta lbaLo
        lda #0
        sta lbaHi
        lda #14
        sta dirn
@sec:
        lda #<SECTBUF
        sta destp
        lda #>SECTBUF
        sta destp+1
        jsr read_sector
        JCS fn_diskerr
        lda #<SECTBUF
        sta ptr2
        lda #>SECTBUF
        sta ptr2+1
        ldx #16
@ent:
        ldy #0
        lda (ptr2),y
        JEQ scan_done         ; 0：目录结束
        cmp #$E5
        beq @next               ; 已删除
        ldy #11
        lda (ptr2),y
        and #$18
        bne @next               ; 子目录、卷标与长文件名项
        ldy #8
        lda (ptr2),y
        cmp #'N'
        bne @next
        iny
        lda (ptr2),y
        cmp #'E'
        bne @next
        iny
        lda (ptr2),y
        cmp #'S'
        bne @next
        jsr add_entry
@next:
        lda ptr2
        clc
        adc #32
        sta ptr2
        bcc @nc
        inc ptr2+1
@nc:
        dex
        bne @ent
        dec dirn
        bne @sec
scan_done:
        lda mv_count
        JEQ fn_notfound
        clc
        rts
fn_notfound:
        lda #E_NOTFOUND
        sta err
        sec
        rts
fn_diskerr:
        lda #E_DISK
        sta err
        sec
        rts

; 把 ptr2 指向的目录项记入游戏表的第 mv_count 项（X 是调用方的循环计数，这里保存）
add_entry:
        txa
        pha
        lda mv_count
        jsr make_eptr
        ldy #0
@name:
        lda (ptr2),y
        sta (eptr),y
        iny
        cpy #8
        bne @name
        ldx #0
@copy:
        ldy copy_src,x
        lda (ptr2),y
        sta tmp
        txa
        clc
        adc #8
        tay
        lda tmp
        sta (eptr),y
        inx
        cpx #6
        bne @copy
        inc mv_count
        pla
        tax
        rts

; eptr = ENTRIES + A * 16（A 不超过 223）
make_eptr:
        pha
        and #$0F
        asl a
        asl a
        asl a
        asl a
        clc
        adc #<ENTRIES
        sta eptr
        lda #0
        adc #0
        sta eptr+1              ; 低字节相加的进位
        pla
        lsr a
        lsr a
        lsr a
        lsr a
        clc
        adc eptr+1
        clc
        adc #>ENTRIES
        sta eptr+1
        rts

; 取第 A 项的首簇与大小到 curc、dsize
select_entry:
        jsr make_eptr
        ldy #8
        lda (eptr),y
        sta curc
        iny
        lda (eptr),y
        sta curc+1
        ldy #10
        lda (eptr),y
        sta dsize
        iny
        lda (eptr),y
        sta dsize+1
        iny
        lda (eptr),y
        sta dsize+2
        iny
        lda (eptr),y
        sta dsize+3
        rts

; 显示游戏列表，手柄上下选择、Start 运行；返回 A 为所选项的序号，C 清零
menu:
        jsr ui_setup
        jsr read_pad
        sta mv_prev
        lda #0
        sta mv_sel
        sta mv_top
        jsr menu_draw
@loop:
        jsr read_pad
        sta mv_pad
        lda mv_prev
        eor #$FF
        and mv_pad
        sta pressed
        lda mv_pad
        sta mv_prev
        lda pressed
        and #PAD_START
        bne @run
        lda pressed
        and #PAD_UP
        beq @down
        lda mv_sel
        beq @loop
        sec
        sbc #1
        sta mv_sel
        cmp mv_top
        bcs @redraw
        sta mv_top
        jmp @redraw
@down:
        lda pressed
        and #PAD_DOWN
        beq @loop
        lda mv_sel
        clc
        adc #1
        cmp mv_count
        bcs @loop
        sta mv_sel
        sec
        sbc mv_top
        cmp #MENU_VIS
        bcc @redraw
        lda mv_sel
        sec
        sbc #MENU_VIS - 1
        sta mv_top
@redraw:
        jsr menu_draw
        jmp @loop
@run:
        lda #0
        sta $2000               ; BIOS 的文字屏会重新打开显示与 NMI，加载 CHR 前要关掉
        sta $2001
        sta $4015
        lda #$40
        sta $4017
        sei
        lda mv_sel
        clc
        rts

; 恢复 DOS 的显示状态并初始化文字屏（与出错时相同）
ui_setup:
        lda sel
        sta REG_MODE
        lda #DOS_REG_ROM
        sta REG_ROM
        lda #0
        sta REG_CHR
        jsr BIOS_INIT_TEXT
        rts

; 读手柄 1：返回 A，位 7 为 A 键，依次到位 0 为右键，按下为 1
read_pad:
        lda #1
        sta $4016
        lda #0
        sta $4016
        sta padacc
        ldx #8
@r:
        lda $4016
        lsr a
        rol padacc
        dex
        bne @r
        lda padacc
        rts

; 清屏并显示标题、游戏列表与操作提示
menu_draw:
        jsr BIOS_CLEAR
        lda #<msg_title
        ldy #>msg_title
        jsr BIOS_PRINT
        lda #0
        sta mv_k
@row:
        lda mv_k
        cmp #MENU_VIS
        bcs @help
        clc
        adc mv_top
        cmp mv_count
        bcs @help
        jsr build_row
        lda #<ROWBUF
        ldy #>ROWBUF
        jsr BIOS_PRINT
        inc mv_k
        jmp @row
@help:
        lda #<msg_help
        ldy #>msg_help
        jsr BIOS_PRINT
        rts

; 把第 A 项写进 ROWBUF：定位、光标（选中的为 >）、主名、结束符；行号由 mv_k 决定
build_row:
        sta bidx
        jsr make_eptr
        lda #STR_LOCATE
        sta ROWBUF
        lda #2
        sta ROWBUF+1
        lda mv_k
        asl a
        clc
        adc #MENU_FIRST
        sta ROWBUF+2
        lda bidx
        cmp mv_sel
        beq @cur
        lda #' '
        jmp @put
@cur:
        lda #'>'
@put:
        sta ROWBUF+3
        ldy #0
@name:
        lda (eptr),y
        sta ROWBUF+4,y
        iny
        cpy #8
        bne @name
        ldy #7
@trim:
        lda ROWBUF+4,y
        cmp #' '
        bne @end
        dey
        bpl @trim
@end:
        lda #STR_END
        sta ROWBUF+5,y
        rts

; 沿簇链读文件：第一个扇区检查头，之后逐扇区写入程序 RAM 与字符 RAM；全部写完时 C 清零
load_file:
        lda #$FF
        sta fatSec
        lda #$00
        sta hdrflag
        sta all_done
@cl:
        lda curc+1
        bne @hi
        lda curc
        cmp #2
        bcc lf_eof
        jmp @go
@hi:
        cmp #$0F
        bcc @go
        bne lf_eof
        lda curc
        cmp #$F8
        bcs lf_eof
@go:
        lda curc
        clc
        adc #31                 ; 扇区 = 簇号 + 31
        sta lbaLo
        lda curc+1
        adc #0
        sta lbaHi
        lda #<SECTBUF
        sta destp
        lda #>SECTBUF
        sta destp+1
        jsr read_sector
        JCS lf_disk
        lda hdrflag
        bne @hdrok
        jsr parse_header
        JCS lf_fail
        lda #1
        sta hdrflag
@hdrok:
        jsr process_sector
        lda all_done
        bne @done
        jsr next_cluster
        JCS lf_disk
        jmp @cl
@done:
        clc
        rts
lf_eof:
        lda #E_TRUNC
        sta err
        sec
        rts
lf_disk:
        lda #E_DISK
        sta err
        sec
        rts
lf_fail:
        sec
        rts

; 检查 iNES 头（在 SECTBUF 里）：失败时 err 置为出错码，C 置位；通过时进入程序 RAM 模式，准备好写入
parse_header:
        lda SECTBUF
        cmp #'N'
        JNE ph_ines
        lda SECTBUF+1
        cmp #'E'
        JNE ph_ines
        lda SECTBUF+2
        cmp #'S'
        JNE ph_ines
        lda SECTBUF+3
        cmp #$1A
        JNE ph_ines
        lda SECTBUF+6
        sta flags6
        and #$04
        JNE ph_trainer
        lda SECTBUF+6
        lsr a
        lsr a
        lsr a
        lsr a
        sta mapper
        lda SECTBUF+7
        and #$F0
        ora mapper
        sta mapper
        beq @m0
        cmp #4
        JNE ph_mapper
        jmp @m4
@m0:
        lda SECTBUF+4           ; mapper 0：PRG 16K 或 32K，CHR 8K
        sta prg_units
        JEQ ph_size
        cmp #3
        JCS ph_size
        lda SECTBUF+5
        sta chr_units
        cmp #1
        JNE ph_size
        jmp @sized
@m4:
        lda SECTBUF+4           ; mapper 4：PRG 不超过 30 个 16K（480K），CHR 不超过 16 个 8K（128K）
        sta prg_units
        JEQ ph_size
        cmp #31
        JCS ph_size
        lda SECTBUF+5
        sta chr_units
        JEQ ph_size
        cmp #17
        JCS ph_size
@sized:
        lda #0                  ; 头里写的数据需要的 256 字节页数 = PRG * 64 + CHR * 32
        sta req+1
        lda prg_units
        sta req
        ldx #6
@mul64:
        asl req
        rol req+1
        dex
        bne @mul64
        lda chr_units
        sta tmp
        lda #0
        sta tmp2
        ldx #5
@mul32:
        asl tmp
        rol tmp2
        dex
        bne @mul32
        lda req
        clc
        adc tmp
        sta req
        lda req+1
        adc tmp2
        sta req+1
        lda dsize+3             ; 目录项里的文件大小至少要有头 16 字节加这么多页
        bne @large
        lda dsize+2
        cmp req+1
        bcc ph_trunc
        bne @large
        lda dsize+1
        cmp req
        bcc ph_trunc
        bne @large
        lda dsize
        cmp #16
        bcc ph_trunc
@large:
        lda mapper
        beq @ready
        jsr detect_small        ; mapper 4 需要 1MB 的程序 RAM
        JCS ph_mapper
@ready:
        lda #$04
        ora sel
        sta REG_MODE            ; 程序 RAM 模式，16K 页
        lda #0
        sta page16
        jsr set_page
        lda #$00
        sta dstp
        lda #$80
        sta dstp+1
        lda #16
        sta skipL
        lda #1
        sta prg_active
        lda #0
        sta chr_active
        clc
        rts
ph_ines:
        lda #E_INES
        bne ph_err
ph_size:
        lda #E_SIZE
        bne ph_err
ph_trainer:
        lda #E_TRAINER
        bne ph_err
ph_trunc:
        lda #E_TRUNC
        bne ph_err
ph_mapper:
        lda #E_MAPPER
ph_err:
        sta err
        sec
        rts

; 程序 RAM 只有 128K 的型号，页寄存器只取低 3 位，选第 8 页与选第 0 页是同一页：
; 在第 0 页开头写 0，选第 8 页写 $5A，再回第 0 页读，读到 $5A 就是小容量，C 置位
detect_small:
        lda #$04
        ora sel
        sta REG_MODE
        lda #0
        jsr set_page
        lda #0
        sta $8000
        lda #8
        jsr set_page
        lda #$5A
        sta $8000
        lda #0
        jsr set_page
        lda $8000
        cmp #$5A
        beq @small
        clc
        rts
@small:
        sec
        rts

; 处理 SECTBUF 的 512 字节：头部跳过，PRG 写程序 RAM，CHR 写 $2007
process_sector:
        lda #<SECTBUF
        sta ptr
        lda #>SECTBUF
        sta ptr+1
        lda #2
        sta pass
@pass:
        ldy #0
@byte:
        lda all_done
        bne @ret
        lda (ptr),y
        sta tmp
        lda skipL
        beq @nskip
        dec skipL
        jmp @next
@nskip:
        lda prg_active
        beq @chr
        lda tmp
        ldx #0
        sta (dstp,x)
        inc dstp
        bne @next
        inc dstp+1
        lda dstp+1
        cmp #$C0
        bne @next
        tya                     ; 写满一页（16K），换下一页
        pha
        jsr next_prg_page
        pla
        tay
        jmp @next
@chr:
        lda chr_active
        beq @next
        lda tmp
        sta $2007
        lda chr_cnt
        bne @cl
        dec chr_cnt+1
@cl:
        dec chr_cnt
        lda chr_cnt
        ora chr_cnt+1
        bne @next
        tya
        pha
        jsr next_chr_block
        pla
        tay
@next:
        iny
        bne @byte
        inc ptr+1
        dec pass
        bne @pass
@ret:
        rts

; PRG 的 16K 页写满了：还有 PRG 就选下一页，否则开始写 CHR
next_prg_page:
        inc page16
        lda page16
        cmp prg_units
        beq @chr
        jsr set_page
        lda #$00
        sta dstp
        lda #$80
        sta dstp+1
        rts
@chr:
        lda #0
        sta prg_active
        sta chr_blk
        lda #1
        sta chr_active
        jmp start_chr_block

; CHR 的 8K 块写满了：还有就写下一块，否则全部写完
next_chr_block:
        inc chr_blk
        lda chr_blk
        cmp chr_units
        bne start_chr_block
        lda #0
        sta chr_active
        lda #1
        sta all_done
        rts

; 开始写第 chr_blk 个 8K 图案块：字符页选 swap(chr_blk)，显存地址回到 $0000
start_chr_block:
        lda chr_blk
        and #$03
        asl a
        asl a
        sta tmp2
        lda chr_blk
        lsr a
        lsr a
        ora tmp2
        sta REG_CHR
        bit $2002
        lda #0
        sta $2006
        sta $2006
        sta chr_cnt
        lda #$20
        sta chr_cnt+1
        rts

; 下一个簇：FAT12，off = c + c/2，项在 FAT 第 off/512 扇区的 off%512 处
next_cluster:
        lda curc+1
        lsr a
        sta oHi
        lda curc
        ror a
        sta oLo
        lda curc
        clc
        adc oLo
        sta oLo
        lda curc+1
        adc oHi
        sta oHi
        lda oHi
        lsr a
        sta fsec
        lda oHi
        and #1
        sta phi
        lda oLo
        sta plo
        lda fatSec
        cmp fsec
        beq @have
        lda fsec
        jsr load_fat_sector
        JCS nc_fail
@have:
        lda #<FATBUF
        clc
        adc plo
        sta ptr
        lda #>FATBUF
        adc phi
        sta ptr+1
        ldy #0
        lda (ptr),y
        sta b0
        lda plo
        cmp #$FF
        bne @b1same
        lda phi
        beq @b1same
        lda fsec
        clc
        adc #1
        jsr load_fat_sector
        JCS nc_fail
        lda FATBUF
        sta b1
        jmp @calc
@b1same:
        ldy #1
        lda (ptr),y
        sta b1
@calc:
        lda curc
        and #1
        bne @odd
        lda b0
        sta curc
        lda b1
        and #$0F
        sta curc+1
        clc
        rts
@odd:
        lda b0
        lsr a
        lsr a
        lsr a
        lsr a
        sta tmp
        lda b1
        asl a
        asl a
        asl a
        asl a
        ora tmp
        sta curc
        lda b1
        lsr a
        lsr a
        lsr a
        lsr a
        sta curc+1
        clc
        rts
nc_fail:
        sec
        rts

; 读 FAT 的第 A 个扇区（FAT1 从扇区 1 开始）到 FATBUF
load_fat_sector:
        sta tmp
        clc
        adc #1
        sta lbaLo
        lda #0
        sta lbaHi
        lda #<FATBUF
        sta destp
        lda #>FATBUF
        sta destp+1
        jsr read_sector
        bcs lfs_fail
        lda tmp
        sta fatSec
        clc
        rts
lfs_fail:
        sec
        rts
