; ------------------------------------------------------------------------
; 名称：file.s
; 说明：测试 bbg.lib 的文件调用封装：打开、读、写、定位、关闭、建立、改名、删除、属性、查找、驱动器、目录、路径与绝对扇区。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：在步步高系统盘 BBGDOS.IMG 的副本上运行，根目录里有 BBGDOS.SYS、COMMAND.CMD 与本测试程序 BBGTEST.CMD 三个文件；
;       检查编号与脚本里的期望一一对应；测试用的文件与目录在测试结束前都删除。
; ------------------------------------------------------------------------

        .include        "bbg-test.inc"
        .export         _main

        .bss

fh:             .res    FH_SIZE
buf:            .res    600
sector:         .res    512
count:          .res    1

        .rodata

name_sys:       .byte   "BBGDOS  SYS"
name_cmd:       .byte   "COMMAND CMD"
name_none:      .byte   "NOFILE  TXT"
name_test1:     .byte   "TEST1   TXT"
name_test2:     .byte   "TEST2   TXT"
name_attr:      .byte   "ATTR    TXT"
name_any:       .byte   "???????????"
dir_sub1:       .byte   "SUB1", 0
dir_sub2:       .byte   "SUB2", 0
dir_none:       .byte   "NODIR", 0
dir_up:         .byte   "..", 0
path_in:        .byte   "SUB1\IN.TXT", 0
path_find:      .byte   "SUB1\*.*", 0
sys_head:       .byte   $4C, $96, $7C, $0A, $53, $74, $61, $72
sys_at100:      .byte   $68, $AA, $68, $28
cmd_at506:      .byte   "mory too sma"
hello:          .byte   "HELLO"
boot_name:      .byte   "SMDOS1.0"
dta_sys:        .byte   "BBGDOS  SYS"
dta_in:         .byte   "IN      TXT"

        .code

.macro FILE fn
        lda     #<fh
        ldx     #>fh
        jsr     fn
.endmacro

.macro INIT
        FILE    bbg_file_clear
.endmacro

.macro SETBUF addr
        lda     #<(addr)
        sta     bbg_arg0
        lda     #>(addr)
        sta     bbg_arg0 + 1
        FILE    bbg_file_set_buffer
.endmacro

.macro SETLEN n
        lda     #<(n)
        sta     bbg_arg0
        lda     #>(n)
        sta     bbg_arg0 + 1
        FILE    bbg_file_set_length
.endmacro

.macro SETPOS n
        lda     #<(n)
        sta     bbg_arg0
        lda     #>(n)
        sta     bbg_arg0 + 1
        lda     #0
        sta     bbg_arg1
        sta     bbg_arg1 + 1
        FILE    bbg_file_set_position
.endmacro

.macro SETDRIVE n
        lda     #<fh
        ldx     #>fh
        ldy     #n
        jsr     bbg_file_set_drive
.endmacro

_main:
        jsr     bbt_init

        ; ---- 打开 BBGDOS.SYS，读文件信息
        INIT
        SETBUF  name_sys
        FILE    bbg_file_open
        CHECK_OK 1
        FILE    bbg_file_get_size
        CHECK16 2, $39E6
        lda     bbg_arg0
        ora     bbg_arg0 + 1
        CHECK_A 3, 0                    ; 长度的高 16 位为 0
        lda     fh + FH_OPEN_ATTRIBUTE
        CHECK_A 4, $20
        lda     fh + FH_OPEN_DATE
        ldx     fh + FH_OPEN_DATE + 1
        CHECK16 5, $242B                ; 1998-01-11

        ; ---- 读 8 字节
        SETBUF  buf
        SETLEN  8
        SETPOS  0
        FILE    bbg_file_read
        CHECK_OK 6
        FILE    bbg_file_get_length
        CHECK16 7, 8
        CHECK_MEM 8, buf, sys_head, 8
        FILE    bbg_file_get_position
        CHECK16 9, 8

        ; ---- 定位到 100 再读 4 字节
        SETPOS  100
        FILE    bbg_file_seek
        CHECK_OK 10
        SETBUF  buf
        SETLEN  4
        FILE    bbg_file_read
        CHECK_OK 11
        CHECK_MEM 12, buf, sys_at100, 4
        FILE    bbg_file_get_position
        CHECK16 13, 104

        ; ---- 到文件末尾：从 14820 读 10 字节，只能读到 2 字节
        SETPOS  14820
        SETBUF  buf
        SETLEN  10
        FILE    bbg_file_read
        CHECK_OK 14
        FILE    bbg_file_get_length
        CHECK16 15, 2
        FILE    bbg_file_close
        CHECK_OK 16

        ; ---- COMMAND.CMD：跨扇区读 12 字节
        INIT
        SETBUF  name_cmd
        FILE    bbg_file_open
        CHECK_OK 17
        SETBUF  buf
        SETLEN  12
        SETPOS  506
        FILE    bbg_file_read
        CHECK_OK 18
        CHECK_MEM 19, buf, cmd_at506, 12
        FILE    bbg_file_close
        CHECK_OK 20

        ; ---- 打开不存在的文件
        INIT
        SETBUF  name_none
        FILE    bbg_file_open
        CHECK_ERR 21, ERR_FILE_NOT_EXIST

        ; ---- 建立 TEST1.TXT，写入 HELLO，关闭
        INIT
        SETBUF  name_test1
        FILE    bbg_file_create
        CHECK_OK 22
        SETBUF  hello
        SETLEN  5
        SETPOS  0
        FILE    bbg_file_write
        CHECK_OK 23
        FILE    bbg_file_get_length
        CHECK16 24, 5
        FILE    bbg_file_close
        CHECK_OK 25

        ; ---- 重新打开：长度为 5，内容为 HELLO
        INIT
        SETBUF  name_test1
        FILE    bbg_file_open
        CHECK_OK 26
        FILE    bbg_file_get_size
        CHECK16 27, 5
        SETBUF  buf
        SETLEN  10
        SETPOS  0
        FILE    bbg_file_read
        CHECK_OK 28
        FILE    bbg_file_get_length
        CHECK16 29, 5
        CHECK_MEM 30, buf, hello, 5

        ; ---- 改名为 TEST2.TXT
        SETBUF  name_test2
        FILE    bbg_file_rename
        CHECK_OK 31
        FILE    bbg_file_close
        CHECK_OK 32
        INIT
        SETBUF  name_test1
        FILE    bbg_file_open
        CHECK_ERR 33, ERR_FILE_NOT_EXIST
        INIT
        SETBUF  name_test2
        FILE    bbg_file_open
        CHECK_OK 34
        FILE    bbg_file_close
        CHECK_OK 35

        ; ---- 删除 TEST2.TXT
        INIT
        SETBUF  name_test2
        FILE    bbg_file_delete
        CHECK_OK 36
        INIT
        SETBUF  name_test2
        FILE    bbg_file_open
        CHECK_ERR 37, ERR_FILE_NOT_EXIST

        ; ---- 设置属性、日期与时间
        INIT
        SETBUF  name_attr
        FILE    bbg_file_create
        CHECK_OK 38
        lda     #$01
        sta     fh + FH_OPEN_ATTRIBUTE
        lda     #$00
        sta     fh + FH_OPEN_DATE
        lda     #$28
        sta     fh + FH_OPEN_DATE + 1
        lda     #$34
        sta     fh + FH_OPEN_TIME
        lda     #$12
        sta     fh + FH_OPEN_TIME + 1
        FILE    bbg_file_set_attribute
        CHECK_OK 39
        FILE    bbg_file_close
        CHECK_OK 40
        INIT
        SETBUF  name_attr
        FILE    bbg_file_open
        CHECK_OK 41
        lda     fh + FH_OPEN_ATTRIBUTE
        CHECK_A 42, $01
        lda     fh + FH_OPEN_DATE
        ldx     fh + FH_OPEN_DATE + 1
        CHECK16 43, $2800
        lda     fh + FH_OPEN_TIME
        ldx     fh + FH_OPEN_TIME + 1
        CHECK16 44, $1234
        FILE    bbg_file_close
        CHECK_OK 45
        INIT
        SETBUF  name_attr
        FILE    bbg_file_delete
        CHECK_OK 46

        ; ---- 查找：根目录里有 3 个文件，第一个是 BBGDOS.SYS，查完时返回 ERR_SEARCH_ENDING
        INIT
        SETBUF  name_any
        FILE    bbg_file_find_first
        CHECK_MEM 47, fh + FH_DTA_NAME, dta_sys, 11
        INIT
        SETBUF  name_any
        lda     #0
        sta     count
        FILE    bbg_file_find_first
@scan:  bcs     @scanned
        inc     count
        FILE    bbg_file_find_next
        jmp     @scan
@scanned:

        SAVE
        CHECK_RA 48, ERR_SEARCH_ENDING
        CHECK_RC 49, 1
        lda     count
        CHECK_A 50, 3

        ; ---- 驱动器：默认盘与容量
        INIT
        FILE    bbg_file_drive
        CHECK_OK 51
        lda     fh + FH_DRIVE
        CHECK_A 52, 1
        lda     fh + FH_DRIVE_COUNT
        CHECK_A 53, 2
        INIT
        SETDRIVE 1
        FILE    bbg_file_drive_size
        CHECK_OK 54
        lda     fh + FH_BYTES_PER_SECTOR
        ldx     fh + FH_BYTES_PER_SECTOR + 1
        CHECK16 55, 512
        lda     fh + FH_TOTAL_CLUSTERS
        ldx     fh + FH_TOTAL_CLUSTERS + 1
        CHECK16 56, 715

        ; ---- 目录与路径：建立 SUB1，在里面建立文件、打开、查找、删除，再删除目录
        INIT
        SETBUF  dir_sub1
        FILE    bbg_file_make_dir
        CHECK_OK 57
        INIT
        SETBUF  path_in
        FILE    bbg_file_path_create
        CHECK_OK 58
        SETBUF  hello
        SETLEN  5
        SETPOS  0
        FILE    bbg_file_write
        CHECK_OK 59
        FILE    bbg_file_close
        CHECK_OK 60
        INIT
        SETBUF  path_in
        FILE    bbg_file_path_open
        CHECK_OK 61
        FILE    bbg_file_get_size
        CHECK16 62, 5
        FILE    bbg_file_close
        CHECK_OK 63
        INIT
        SETBUF  path_find
        FILE    bbg_file_path_find_first
        CHECK_OK 64
        CHECK_MEM 65, fh + FH_DTA_NAME, dta_in, 11
        INIT
        SETBUF  path_in
        FILE    bbg_file_path_delete
        CHECK_OK 66
        INIT
        SETBUF  dir_sub1
        FILE    bbg_file_remove_dir
        CHECK_OK 67

        ; ---- 当前目录：进入 SUB2，进入不存在的目录出错，取当前目录，回到上级，再删除 SUB2
        INIT
        SETBUF  dir_sub2
        FILE    bbg_file_make_dir
        CHECK_OK 68
        SETBUF  dir_sub2
        FILE    bbg_file_change_dir
        CHECK_OK 69
        SETBUF  dir_none
        FILE    bbg_file_change_dir
        CHECK_ERR 70, ERR_PATH_NOT_FOUND
        FILE    bbg_file_get_dir
        CHECK_OK 71
        SETBUF  dir_up
        FILE    bbg_file_change_dir
        CHECK_OK 72
        SETBUF  dir_sub2
        FILE    bbg_file_remove_dir
        CHECK_OK 73

        ; ---- 绝对扇区：读 0 号扇区（引导扇区）并原样写回
        INIT
        SETDRIVE 1
        SETBUF  sector
        SETLEN  1
        SETPOS  0
        FILE    bbg_file_read_sectors
        CHECK_OK 74
        CHECK_MEM 75, sector + 3, boot_name, 8
        SETBUF  sector
        SETLEN  1
        SETPOS  0
        FILE    bbg_file_write_sectors
        CHECK_OK 76

        ; ---- 复位驱动器
        INIT
        FILE    bbg_file_reset
        CHECK_OK 77

        jsr     bbt_done
