; ------------------------------------------------------------------------
; 名称：nes.s
; 说明：在小霸王 97（SB97）的 DOS 下运行 NES 游戏：NES 参数名（mario 或 mario.nes），读软盘上的文件，只支持 mapper 0。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-22
; 备注：DOS 运行的程序是 32K 映像，映射在 $8000-$FFFF；签名 AA 55 与向量由 sb97 目标的启动代码提供，启动代码调用 _main。
;       装载程序（RAMCODE 段）复制到工作 RAM $6000 运行，切换程序 RAM 时执行的代码不会丢失。
;       流程：读参数 -> 软驱初始化 -> 在根目录找文件 -> 沿簇链读扇区 -> 检查 iNES 头
;       -> PRG 写入程序 RAM 第 1 个 32K 页（$5000=$81），CHR 写入图案区 -> 设置镜像 -> 改掉读手柄 2 的指令 -> JMP ($FFFC)。
;       出错时在画面第 10 行显示英文提示并停住，需要重新开机。
; ------------------------------------------------------------------------

    .setcpu "6502"
    .include "sb97.inc"
    .export _main
    .import __RAMCODE_LOAD__, __RAMCODE_RUN__, __RAMCODE_SIZE__

; 长距离条件分支：条件不满足时短跳过，满足时用 JMP 去远处的标签（用匿名标签，不切断 @ 局部标签的作用域）
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

; 零页变量（装载程序专用，游戏启动后不再使用）
ptr     = $00   ; 2 字节：源指针
ptr2    = $02   ; 2 字节：目的指针
dstp    = $04   ; 2 字节：程序 RAM 的写入指针，用 (dstp,X) 写
cnt     = $06   ; 2 字节：计数器
tmo     = $08   ; 2 字节：等待软驱的超时计数
track   = $0A   ; 软驱当前磁道
curc    = $0B   ; 2 字节：当前簇号
lbaLo   = $0D   ; 2 字节：要读的扇区号
lbaHi   = $0E
skipL   = $0F   ; 头部剩余要跳过的字节
prgL    = $10   ; 2 字节：PRG 剩余字节数
chrL    = $12   ; 2 字节：CHR 剩余字节数
mirr    = $14   ; 写入 $5200 的值（镜像位与 32K 模式位）
fatSec  = $15   ; 当前 FAT 缓冲里的扇区号
tmp     = $17
err     = $18   ; 出错码
destp   = $19   ; 2 字节：读扇区的目的地址
rdp     = $1B   ; 2 字节：读数据时的写入指针
headv   = $1D   ; 磁头号
secv    = $1E   ; 扇区号
st0     = $1F   ; 读命令的结果字节
st1     = $20
st2     = $21
dirn    = $22   ; 根目录剩余扇区数
hprg    = $25   ; PRG 个数（16K）
hchr    = $26   ; CHR 个数（8K）
hf6     = $27   ; 头部标志 6
pass    = $28   ; 扇区内两遍处理的计数
oLo     = $29   ; 2 字节：FAT 字节偏移
oHi     = $2A
plo     = $2B   ; FAT 项在缓冲里的偏移（0-511）
phi     = $2C
fsec    = $2D   ; FAT 扇区号
b0      = $2E   ; FAT 项的第一个字节
b1      = $2F   ; FAT 项的第二个字节
trkn    = $30   ; 读扇区时算出的磁道
hdrflag = $31   ; 头部是否已经解析

; 参数与 8.3 文件名放在零页
args    = $40   ; 16 字节：DOS 第一个参数（DOS_ARGUMENT 起，以 0 结尾）
name    = $50   ; 11 字节：8.3 文件名

; 出错码（与 msg_table 的顺序一致）
E_USAGE    = 1
E_BADNAME  = 2
E_DISK     = 3
E_NOTFOUND = 4
E_INES     = 5
E_MAPPER   = 6
E_SIZE     = 7
E_TRUNC    = 8
E_TRAINER  = 9

; ------------------------------------------------------------------------
; 入口：由启动代码调用；复制装载程序到工作 RAM，然后执行
; ------------------------------------------------------------------------
    .segment "CODE"
_main:
    ldx #$0F
@args:
    lda DOS_ARGUMENT,x      ; DOS 把第一个参数放在这里
    sta args,x
    dex
    bpl @args
    lda #<__RAMCODE_LOAD__
    sta ptr
    lda #>__RAMCODE_LOAD__
    sta ptr+1
    lda #<__RAMCODE_RUN__
    sta ptr2
    lda #>__RAMCODE_RUN__
    sta ptr2+1
    lda #<__RAMCODE_SIZE__
    sta cnt
    lda #>__RAMCODE_SIZE__
    sta cnt+1
@copy:
    ldy #0
    lda (ptr),y
    sta (ptr2),y
    inc ptr
    bne @c1
    inc ptr+1
@c1:
    inc ptr2
    bne @c2
    inc ptr2+1
@c2:
    lda cnt
    bne @c3
    dec cnt+1
@c3:
    dec cnt
    lda cnt
    ora cnt+1
    bne @copy
    jmp loader_start

; ------------------------------------------------------------------------
; 装载程序：运行在工作 RAM $6000，只能访问 RAM、寄存器与程序 RAM 窗口
; ------------------------------------------------------------------------
    .segment "RAMCODE"
loader_start:
    lda #$00
    sta PPU_CTRL
    sta PPU_MASK
    lda args
    bne @arg
    lda #E_USAGE
    jmp fail_code
@arg:
    jsr make_name
    bcc @named
    lda #E_BADNAME
    jmp fail_code
@named:
    jsr fdc_init
    JCS disk_err
    jsr find_file
    JCS fail_set
    jsr load_file
    JCS fail_set
    lda hprg
    cmp #1
    bne @go
    jsr mirror16
@go:
    jsr patch_pad
    jmp tramp
disk_err:
    lda #E_DISK
fail_code:
    sta err
fail_set:
    jmp fail

; 把 args 转成 8.3 文件名（name），没有扩展名时用 NES；C 置位表示名字不合法
make_name:
    lda #' '
    ldx #10
@fill:
    sta name,x
    dex
    bpl @fill
    ldx #0
    ldy #0
@base:
    lda args,x
    beq @noext
    cmp #'.'
    beq @dot
    cpy #8
    bcs @bad
    jsr upcase
    sta a:name,y
    iny
    inx
    jmp @base
@noext:
    jsr setnes
    jmp @ok
@dot:
    cpy #0
    beq @bad
    inx
    ldy #8
@ext:
    lda args,x
    beq @extend
    cpy #11
    bcs @bad
    jsr upcase
    sta a:name,y
    iny
    inx
    jmp @ext
@extend:
    cpy #8
    bne @ok
    jsr setnes
@ok:
    clc
    rts
@bad:
    sec
    rts

setnes:
    lda #'N'
    sta name+8
    lda #'E'
    sta name+9
    lda #'S'
    sta name+10
    rts

; A 为小写字母时转大写
upcase:
    cmp #'a'
    bcc @r
    cmp #'z'+1
    bcs @r
    and #$DF
@r:
    rts

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
    lda #$66                ; READ DATA，MFM、跳过删除数据（与 BIOS 相同）
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
    lda st1                 ; IC 为 01：只有 EN 或 OR（读到末尾）才算正常，与 BIOS 的判断一致
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

; 在根目录找 name，找到时 curc 为首簇，C 清零；否则 err 置为出错码，C 置位
find_file:
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
    JCS ff_diskerr
    lda #<SECTBUF
    sta ptr2
    lda #>SECTBUF
    sta ptr2+1
    ldx #16
@ent:
    ldy #0
    lda (ptr2),y
    JEQ ff_notfound
@cmp:
    lda (ptr2),y
    cmp a:name,y
    bne @next
    iny
    cpy #11
    bne @cmp
    ldy #26
    lda (ptr2),y
    sta curc
    iny
    lda (ptr2),y
    sta curc+1
    clc
    rts
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
ff_notfound:
    lda #E_NOTFOUND
    sta err
    sec
    rts
ff_diskerr:
    lda #E_DISK
    sta err
    sec
    rts

; 沿簇链读文件：第一个扇区检查头部，之后逐扇区写入程序 RAM 与图案区
load_file:
    lda #$FF
    sta fatSec
    lda #$00
    sta hdrflag
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
    JCS fail_set
    lda #1
    sta hdrflag
@hdrok:
    jsr process_sector
    lda prgL
    ora prgL+1
    ora chrL
    ora chrL+1
    ora skipL
    beq @done
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

; 检查 iNES 头（在 SECTBUF 里），设置程序 RAM 页、镜像与各段剩余字节数
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
    lda SECTBUF+4
    sta hprg
    JEQ ph_size
    cmp #3
    JCS ph_size
    lda SECTBUF+5
    sta hchr
    cmp #2
    JCS ph_size
    lda SECTBUF+6
    sta hf6
    and #$04
    JNE ph_trainer
    lda hf6
    and #$F0
    JNE ph_mapper
    lda SECTBUF+7
    and #$F0
    JNE ph_mapper
    lda #$00
    sta prgL
    lda hprg
    cmp #1
    beq @p16
    lda #$80
    bne @psz
@p16:
    lda #$40
@psz:
    sta prgL+1
    lda #$00
    sta chrL
    lda hchr
    beq @c0
    lda #$20
    bne @csz
@c0:
    lda #$00
@csz:
    sta chrL+1
    lda #16
    sta skipL
    lda hf6
    and #$01
    eor #$01
    ora #$04
    sta mirr
    lda #$00
    sta dstp
    lda #$80
    sta dstp+1
    lda #$81
    sta PRAM_SEL
    lda mirr
    sta MIRROR
    lda #$00
    sta PPU_ADDR
    sta PPU_ADDR
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
ph_mapper:
    lda #E_MAPPER
ph_err:
    sta err
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
    lda (ptr),y
    sta tmp
    lda skipL
    beq @nskip
    dec skipL
    jmp @next
@nskip:
    lda prgL
    ora prgL+1
    beq @chr
    lda tmp
    ldx #0
    sta (dstp,x)
    inc dstp
    bne @pd
    inc dstp+1
@pd:
    lda prgL
    bne @pl
    dec prgL+1
@pl:
    dec prgL
    jmp @next
@chr:
    lda chrL
    ora chrL+1
    beq @next
    lda tmp
    sta PPU_DATA
    lda chrL
    bne @cl
    dec chrL+1
@cl:
    dec chrL
@next:
    iny
    bne @byte
    inc ptr+1
    dec pass
    bne @pass
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

; 16K 的 PRG 镜像到 $C000-$FFFF
mirror16:
    lda #$00
    sta ptr
    sta ptr2
    lda #$80
    sta ptr+1
    lda #$C0
    sta ptr2+1
    ldx #64
@pg:
    ldy #0
@b:
    lda (ptr),y
    sta (ptr2),y
    iny
    bne @b
    inc ptr+1
    inc ptr2+1
    dex
    bne @pg
    rts

; ------------------------------------------------------------------------
; 手柄 2：SB97 的 $4017 接键盘，没按键时读到 1，游戏读它会当成手柄 2 一直按着全部按键
; （超级玛丽标题画面要求只按开始键，因此完全不响应）。
; 在 $8000-$FFF7 里找 LDA $4017（AD 17 40）与 LDA $4016,X（BD 16 40），改成 JSR 到下面的桩。
; ------------------------------------------------------------------------
patch_pad:
    lda #$00
    sta ptr
    lda #$80
    sta ptr+1
@scan:
    ldy #0
    lda (ptr),y
    cmp #$AD
    bne @notabs
    ldy #1
    lda (ptr),y
    cmp #$17
    bne @adv
    iny
    lda (ptr),y
    cmp #$40
    bne @adv
    lda #<pad2_abs
    ldx #>pad2_abs
    jmp @patch
@notabs:
    cmp #$BD
    bne @adv
    ldy #1
    lda (ptr),y
    cmp #$16
    bne @adv
    iny
    lda (ptr),y
    cmp #$40
    bne @adv
    lda #<pad_x
    ldx #>pad_x
@patch:
    ldy #1
    sta (ptr),y
    txa
    iny
    sta (ptr),y
    ldy #0
    lda #$20                ; JSR
    sta (ptr),y
    lda #3                  ; 跳过改好的 3 个字节
    bne @step
@adv:
    lda #1
@step:
    clc
    adc ptr
    sta ptr
    bcc @chk
    inc ptr+1
@chk:
    lda ptr+1
    cmp #$FF
    bcc @scan
    lda ptr
    cmp #$F8
    bcc @scan
    rts

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

; ------------------------------------------------------------------------
; 启动游戏：在工作 RAM 里切换程序页与镜像，然后跳到游戏的复位向量
; ------------------------------------------------------------------------
tramp:
    lda #$81
    sta PRAM_SEL
    lda mirr
    sta MIRROR
    lda #$00
    sta PPU_CTRL
    sta PPU_MASK
    sta PPU_SCROLL
    sta PPU_SCROLL
    jmp ($FFFC)

; 在画面第 10 行第 2 列显示出错码对应的英文提示，然后停住
fail:
    ldx err
    dex
    txa
    asl a
    tax
    lda msg_table,x
    sta ptr
    lda msg_table+1,x
    sta ptr+1
    lda #$00
    sta PPU_CTRL
    sta PPU_MASK
    lda #$21                ; 第 10 行第 2 列
    sta PPU_ADDR
    lda #$42
    sta PPU_ADDR
    ldy #0
@p:
    lda (ptr),y
    beq @show
    sta PPU_DATA
    iny
    bne @p
@show:
    lda #$00
    sta PPU_SCROLL
    sta PPU_SCROLL
    lda #$10                ; 字库在图案表 1
    sta PPU_CTRL
    lda #$08
    sta PPU_MASK
halt:
    jmp halt

msg_table:
    .word msg_usage, msg_badname, msg_disk, msg_notfound, msg_ines
    .word msg_mapper, msg_size, msg_trunc, msg_trainer
msg_usage:    .byte "Usage: NES FILE.NES", 0
msg_badname:  .byte "Bad file name", 0
msg_disk:     .byte "Disk read error", 0
msg_notfound: .byte "File not found", 0
msg_ines:     .byte "Not an iNES file", 0
msg_mapper:   .byte "Mapper not supported", 0
msg_size:     .byte "Size not supported", 0
msg_trunc:    .byte "File is truncated", 0
msg_trainer:  .byte "Trainer not supported", 0

; ------------------------------------------------------------------------
; 工作 RAM 的缓冲：SECTBUF 放一个扇区，FATBUF 放 FAT 的一个扇区
; ------------------------------------------------------------------------
    .segment "WRAM"
SECTBUF:
    .res 512
FATBUF:
    .res 512
