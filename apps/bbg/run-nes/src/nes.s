; ------------------------------------------------------------------------
; 名称：nes.s
; 说明：在步步高学生电脑（BBGDOS）下运行 NES 游戏：RUN 游戏名（或 RUN 游戏名.NES），读软盘上的文件，只支持 mapper 0。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：流程：取命令行（$0100 长度、$0101 起是参数）-> 拼出文件名 -> 用 DOS 的文件调用打开文件 -> 读头、检查
;       -> PRG 读入当前页后面的两页、CHR 读入第三页（程序运行时，当前页之后的分页内存都归程序）-> 复制跳板
;       -> 跳板运行在内部 RAM $0300：把 CHR 写入字符 RAM、改掉读手柄 2 的指令、映射 PRG 并 JMP ($FFFC)。
;       出错时在命令行显示英文提示并回到 DOS；进入游戏后不能回到 DOS，只能复位。
; ------------------------------------------------------------------------

        .setcpu "6502"
        .include "bbg.inc"
        .export _main
        .import __TRAMP_LOAD__, __TRAMP_RUN__, __TRAMP_SIZE__
        .import __STUBS_LOAD__, __STUBS_RUN__, __STUBS_SIZE__

; 系统调用入口与功能号（手册：系统篇第一章、第二章、第四章）
BIOS_ENTRY      = $5800
FILE_ENTRY      = $5803
BIOS_GET_PAGE   = 2
FILE_READ       = 2
FILE_CLOSE      = 5
FILE_PATH_OPEN  = 20

; 文件通道的偏移（共 48 字节）
FH_CMD          = 0
FH_DRIVE        = 1
FH_PAGE         = 2
FH_BUF          = 3
FH_BYTES        = 5
FH_SIZE         = 48

; 命令行参数：$0100 是长度，$0101 起是参数（含程序名后的空格）
CMD_LENGTH      = $0100
CMD_TEXT        = $0101

; 出错码（与 msg_offsets 的顺序一致）
E_USAGE         = 1
E_BADNAME       = 2
E_MEMORY        = 3
E_NOTFOUND      = 4
E_READ          = 5
E_INES          = 6
E_MAPPER        = 7
E_SIZE          = 8
E_TRUNC         = 9
E_TRAINER       = 10

        .segment "ZEROPAGE"

; 零页变量：只在最后一步（不再调用 DOS）使用
zp_src:         .res 2
zp_dst:         .res 2
zp_len:         .res 2
zp_ptr:         .res 2

        .segment "BSS"

cur_page:       .res 1                  ; 程序所在的页
page_a:         .res 1                  ; PRG 的第一个 16K
page_b:         .res 1                  ; PRG 的第二个 16K
page_c:         .res 1                  ; CHR 的 8K
prg_units:      .res 1                  ; PRG 的 16K 个数
flags6:         .res 1                  ; 头里的标志 6
has_dot:        .res 1                  ; 参数里有没有扩展名
file_open:      .res 1                  ; 文件是否已打开
expect_high:    .res 1                  ; 读文件时期望读到的长度的高字节
err_code:       .res 1
msg_index:      .res 1
hdrbuf:         .res 16
pathbuf:        .res 14
fh:             .res FH_SIZE

        .segment "RODATA"

msgs:
msg_usage:      .byte "Usage: RUN FILE.NES", 0
msg_badname:    .byte "Bad file name", 0
msg_memory:     .byte "No memory", 0
msg_notfound:   .byte "Can't open file", 0
msg_read:       .byte "Read error", 0
msg_ines:       .byte "Not an iNES file", 0
msg_mapper:     .byte "Mapper not supported", 0
msg_size:       .byte "Size not supported", 0
msg_trunc:      .byte "File is truncated", 0
msg_trainer:    .byte "Trainer not supported", 0

msg_offsets:
        .byte msg_usage - msgs, msg_badname - msgs, msg_memory - msgs, msg_notfound - msgs
        .byte msg_read - msgs, msg_ines - msgs, msg_mapper - msgs, msg_size - msgs
        .byte msg_trunc - msgs, msg_trainer - msgs

ext_nes:        .byte ".NES"

        .segment "CODE"

; 入口：由启动代码调用；成功时不会返回，失败时显示提示后返回（启动代码回到 DOS）
_main:
        lda #0
        sta file_open
        lda #BIOS_GET_PAGE
        jsr BIOS_ENTRY
        sta cur_page
        clc
        adc #3
        bcs @nomem
        cmp #$FF                        ; 要用到当前页后面的 3 页，不能到 $FF（与 $C000 公共区重叠）
        bcc @pages
@nomem:
        lda #E_MEMORY
        jmp fail
@pages:
        ldx cur_page
        inx
        stx page_a
        inx
        stx page_b
        inx
        stx page_c
        jsr make_path
        bcc @named
        jmp fail
@named:
        jsr open_file
        bcc @opened
        lda #E_NOTFOUND
        jmp fail
@opened:
        lda #1
        sta file_open
        jsr read_header
        bcc @headed
        jmp fail
@headed:
        jsr check_header
        bcc @checked
        jmp fail
@checked:
        lda page_a
        ldx #$40
        jsr read_pages
        bcc @prg1
        jmp fail
@prg1:
        lda prg_units
        cmp #2
        bne @chr
        lda page_b
        ldx #$40
        jsr read_pages
        bcc @chr
        jmp fail
@chr:
        lda page_c
        ldx #$20
        jsr read_pages
        bcc @loaded
        jmp fail
@loaded:
        jsr close_file
        jmp launch

; 显示出错码 A 对应的提示，必要时关闭文件，然后返回启动代码
fail:
        sta err_code
        lda file_open
        beq @show
        jsr close_file
@show:
        ldx err_code
        lda msg_offsets - 1,x
        sta msg_index
@next:
        ldx msg_index
        lda msgs,x
        beq @crlf
        tax
        lda #DOS_DISPLAY_CHAR
        jsr DOS_ENTRY
        inc msg_index
        jmp @next
@crlf:
        ldx #$0D
        lda #DOS_DISPLAY_CHAR
        jsr DOS_ENTRY
        ldx #$0A
        lda #DOS_DISPLAY_CHAR
        jsr DOS_ENTRY
        rts

; 调用文件功能：文件通道在 fh
call_file:
        ldx #>fh
        ldy #<fh
        jmp FILE_ENTRY

close_file:
        lda #FILE_CLOSE
        sta fh + FH_CMD
        jsr call_file
        lda #0
        sta file_open
        rts

; 取命令行里的第一个词，转大写，没有扩展名时加 .NES，放进 pathbuf（以 0 结尾）；
; 失败时 C 置位，A 是出错码
make_path:
        ldx #0
@skip:
        cpx CMD_LENGTH
        bcs @usage
        lda CMD_TEXT,x
        cmp #' '
        beq @blank
        cmp #$09
        bne @begin
@blank:
        inx
        jmp @skip
@usage:
        lda #E_USAGE
        sec
        rts
@begin:
        ldy #0
        sty has_dot
@copy:
        cpx CMD_LENGTH
        bcs @end
        lda CMD_TEXT,x
        cmp #' '
        beq @end
        cmp #$09
        beq @end
        cmp #$0D
        beq @end
        cmp #'.'
        bne @name
        inc has_dot
@name:
        cmp #'a'
        bcc @store
        cmp #'z' + 1
        bcs @store
        and #$DF
@store:
        cpy #12
        bcs @bad
        sta pathbuf,y
        iny
        inx
        jmp @copy
@end:
        lda has_dot
        bne @term
        cpy #9                          ; 主名最多 8 个字符
        bcs @bad
        ldx #0
@ext:
        lda ext_nes,x
        sta pathbuf,y
        iny
        inx
        cpx #4
        bne @ext
@term:
        lda #0
        sta pathbuf,y
        clc
        rts
@bad:
        lda #E_BADNAME
        sec
        rts

; 打开 pathbuf 里的文件；失败时 C 置位
open_file:
        lda #FILE_PATH_OPEN
        sta fh + FH_CMD
        lda #0
        sta fh + FH_DRIVE
        lda cur_page
        sta fh + FH_PAGE
        lda #<pathbuf
        sta fh + FH_BUF
        lda #>pathbuf
        sta fh + FH_BUF + 1
        jmp call_file

; 读 16 字节的头到 hdrbuf；失败时 C 置位，A 是出错码
read_header:
        lda #FILE_READ
        sta fh + FH_CMD
        lda cur_page
        sta fh + FH_PAGE
        lda #<hdrbuf
        sta fh + FH_BUF
        lda #>hdrbuf
        sta fh + FH_BUF + 1
        lda #16
        sta fh + FH_BYTES
        lda #0
        sta fh + FH_BYTES + 1
        jsr call_file
        bcs @error
        lda fh + FH_BYTES
        cmp #16
        bne @short
        lda fh + FH_BYTES + 1
        bne @short
        clc
        rts
@short:
        lda #E_INES
        sec
        rts
@error:
        lda #E_READ
        sec
        rts

; 检查 iNES 头；失败时 C 置位，A 是出错码
check_header:
        lda hdrbuf
        cmp #'N'
        bne @ines
        lda hdrbuf + 1
        cmp #'E'
        bne @ines
        lda hdrbuf + 2
        cmp #'S'
        bne @ines
        lda hdrbuf + 3
        cmp #$1A
        bne @ines
        lda hdrbuf + 6
        sta flags6
        and #$04
        bne @trainer
        lda hdrbuf + 6
        ora hdrbuf + 7
        and #$F0
        bne @mapper
        lda hdrbuf + 4
        sta prg_units
        beq @size
        cmp #3
        bcs @size
        lda hdrbuf + 5
        cmp #1
        bne @size
        clc
        rts
@ines:
        lda #E_INES
        sec
        rts
@size:
        lda #E_SIZE
        sec
        rts
@trainer:
        lda #E_TRAINER
        sec
        rts
@mapper:
        lda #E_MAPPER
        sec
        rts

; 把文件的下一段读入页 A 的 $8000 起，长度是 X * 256；失败时 C 置位，A 是出错码
read_pages:
        sta fh + FH_PAGE
        stx expect_high
        stx fh + FH_BYTES + 1
        lda #0
        sta fh + FH_BYTES
        sta fh + FH_BUF
        lda #$80
        sta fh + FH_BUF + 1
        lda #FILE_READ
        sta fh + FH_CMD
        jsr call_file
        bcs @error
        lda fh + FH_BYTES
        bne @short
        lda fh + FH_BYTES + 1
        cmp expect_high
        bne @short
        clc
        rts
@short:
        lda #E_TRUNC
        sec
        rts
@error:
        lda #E_READ
        sec
        rts

; 最后一步：不再调用 DOS。复制跳板与小桩，把参数交给跳板，进入跳板
launch:
        lda #<__TRAMP_LOAD__
        sta zp_src
        lda #>__TRAMP_LOAD__
        sta zp_src + 1
        lda #<__TRAMP_RUN__
        sta zp_dst
        lda #>__TRAMP_RUN__
        sta zp_dst + 1
        lda #<__TRAMP_SIZE__
        sta zp_len
        lda #>__TRAMP_SIZE__
        sta zp_len + 1
        jsr copy_block
        lda #<__STUBS_LOAD__
        sta zp_src
        lda #>__STUBS_LOAD__
        sta zp_src + 1
        lda #<__STUBS_RUN__
        sta zp_dst
        lda #>__STUBS_RUN__
        sta zp_dst + 1
        lda #<__STUBS_SIZE__
        sta zp_len
        lda #>__STUBS_SIZE__
        sta zp_len + 1
        jsr copy_block
        lda page_a
        sta t_page_a
        lda page_b
        sta t_page_b
        lda page_c
        sta t_page_c
        lda prg_units
        sta t_units
        lda flags6
        and #$01
        eor #$01                        ; 头里 1 为垂直镜像，主板的视频控制位 1-0：0 垂直、1 水平
        sta t_mirror
        jmp go

; 复制 zp_len 字节：zp_src -> zp_dst
copy_block:
        ldy #0
@loop:
        lda zp_len
        ora zp_len + 1
        beq @done
        lda (zp_src),y
        sta (zp_dst),y
        inc zp_src
        bne @dst
        inc zp_src + 1
@dst:
        inc zp_dst
        bne @len
        inc zp_dst + 1
@len:
        lda zp_len
        bne @dec
        dec zp_len + 1
@dec:
        dec zp_len
        jmp @loop
@done:
        rts

; ------------------------------------------------------------------------
; 跳板：运行在内部 RAM $0300，切换页之后仍在；游戏启动后这一块由游戏自己使用
; ------------------------------------------------------------------------
        .segment "TRAMP"

t_page_a:       .byte 0
t_page_b:       .byte 0
t_page_c:       .byte 0
t_units:        .byte 0
t_mirror:       .byte 0

go:
        sei
        cld
        lda #0
        sta $2000                       ; 关 NMI 与显示
        sta $2001
        sta $4015                       ; 关声音
        lda #$40
        sta $4017                       ; 关 APU 帧中断
        sta $FF12                       ; 清分屏队列
        lda t_mirror
        ora #$08                        ; 位 3：$C000 起用 DRAM；位 2、5、6 为 0：不用扫描线 IRQ、MMC3 与分屏
        sta $FF01
        ldx #0                          ; 8 个 1K 字符页寄存器 $FF23、$FF2B ... 依次指向字符 RAM 的 0 到 7 页
@chars:
        txa
        lsr a
        lsr a
        lsr a
        sta $FF23,x
        txa
        clc
        adc #8
        tax
        cpx #64
        bcc @chars
        lda t_page_c                    ; CHR 页映射到 $8000，写进图案表 $0000-$1FFF
        sta $FF04
        lda $2002
        lda #0
        sta $2006
        sta $2006
        sta zp_ptr
        lda #$80
        sta zp_ptr + 1
        ldx #32
        ldy #0
@copy:
        lda (zp_ptr),y
        sta $2007
        iny
        bne @copy
        inc zp_ptr + 1
        dex
        bne @copy
        lda t_page_a                    ; 改掉读手柄 2 的指令（见 STUBS）
        jsr patch_page
        lda t_units
        cmp #2
        bne @map
        lda t_page_b
        jsr patch_page
@map:
        lda t_page_a                    ; $8000-$BFFF 是 PRG 的第一个 16K
        sta $FF04
        lda t_units
        cmp #2
        bne @upper
        lda t_page_b
        jmp @cd
@upper:
        lda t_page_a                    ; 只有 16K 时 $C000 起镜像同一页
@cd:
        asl a                           ; 16K 页号换成 8K 页号：$C000-$DFFF 用偶数页、$E000-$FFFF 用奇数页
        sta $FF24
        ora #1
        sta $FF2C
        jmp ($FFFC)

; 把页 A 映射到 $8000，在里面找 LDA $4017（AD 17 40）与 LDA $4016,X（BD 16 40），改成 JSR 到小桩
patch_page:
        sta $FF04
        lda #$00
        sta zp_ptr
        lda #$80
        sta zp_ptr + 1
@scan:
        ldy #0
        lda (zp_ptr),y
        cmp #$AD
        bne @notabs
        ldy #1
        lda (zp_ptr),y
        cmp #$17
        bne @adv
        iny
        lda (zp_ptr),y
        cmp #$40
        bne @adv
        lda #<pad2_abs
        ldx #>pad2_abs
        jmp @patch
@notabs:
        cmp #$BD
        bne @adv
        ldy #1
        lda (zp_ptr),y
        cmp #$16
        bne @adv
        iny
        lda (zp_ptr),y
        cmp #$40
        bne @adv
        lda #<pad_x
        ldx #>pad_x
@patch:
        ldy #1
        sta (zp_ptr),y
        txa
        iny
        sta (zp_ptr),y
        ldy #0
        lda #$20                        ; JSR
        sta (zp_ptr),y
        lda #3                          ; 跳过改好的 3 个字节
        bne @step
@adv:
        lda #1
@step:
        clc
        adc zp_ptr
        sta zp_ptr
        bcc @check
        inc zp_ptr + 1
@check:
        lda zp_ptr + 1                  ; 页内最后一个能放下 3 字节指令的位置是 $BFFD
        cmp #$BF
        bcc @scan
        bne @done
        lda zp_ptr
        cmp #$FD
        bcc @scan
@done:
        rts

; ------------------------------------------------------------------------
; 小桩：运行在公共 RAM $7E00，游戏运行时被改过的指令 JSR 到这里
; 步步高只有手柄口 1，$4017 的位 1-4 接键盘，没按键时读到 1，游戏读手柄 2 会以为一直按着全部按键
; ------------------------------------------------------------------------
        .segment "STUBS"

; 替换 LDA $4017：没有手柄 2，读到 0
pad2_abs:
        lda #$00
        rts

; 替换 LDA $4016,X：X 为 1 时读手柄 2，读到 0；X 为 0 时照常读，进位标志不变
pad_x:
        php
        cpx #$01
        beq @none
        plp
        lda $4016,x
        rts
@none:
        plp
        lda #$00
        rts
