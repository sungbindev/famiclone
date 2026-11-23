; ------------------------------------------------------------------------
; 名称：run.s
; 说明：测试 bbg.lib 的运行程序封装（bbg_dos_run、bbg_dos_run_path）与程序退出（bbg_dos_exit）：测试程序运行自己的副本作为子程序。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：命令行参数长度（$0100）不为 0 时是子程序：建立 CHILD.TXT 后以退出码 7 返回；父程序检查返回值与 CHILD.TXT，
;       再用路径版本运行一次。
; ------------------------------------------------------------------------

        .include        "bbg-test.inc"
        .export         _main
        .import         bbg_test_stop

        .bss

fh:             .res    FH_SIZE

        .rodata

self_name:      .byte   "BBGTEST CMD"
self_path:      .byte   "BBGTEST.CMD", 0
child_name:     .byte   "CHILD   TXT"

        .code

; 设置通道块缓冲区为子程序要建立的文件名
set_name:
        lda     #<child_name
        sta     bbg_arg0
        lda     #>child_name
        sta     bbg_arg0 + 1
        lda     #<fh
        ldx     #>fh
        jmp     bbg_file_set_buffer

; 清空通道块并设置文件名
prepare:
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_clear
        jmp     set_name

_main:
        lda     RUN_COMMAND_LINE
        beq     parent
        jmp     child
parent:
        jsr     bbt_init

        ; ---- 运行子程序：参数 "-c"
        lda     #2
        sta     RUN_COMMAND_LINE
        lda     #'-'
        sta     RUN_COMMAND_LINE + 1
        lda     #'c'
        sta     RUN_COMMAND_LINE + 2
        lda     #<self_name
        ldx     #>self_name
        ldy     #0
        jsr     bbg_dos_run
        SAVE
        CHECK_RC 1, 0
        CHECK_RA 2, 7
        jsr     prepare
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_open
        CHECK_OK 3
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_close
        jsr     prepare
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_delete
        CHECK_OK 4

        ; ---- 用路径版本再运行一次
        lda     #2
        sta     RUN_COMMAND_LINE
        lda     #'-'
        sta     RUN_COMMAND_LINE + 1
        lda     #'c'
        sta     RUN_COMMAND_LINE + 2
        lda     #<self_path
        ldx     #>self_path
        ldy     #0
        jsr     bbg_dos_run_path
        SAVE
        CHECK_RC 5, 0
        CHECK_RA 6, 7
        jsr     prepare
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_open
        CHECK_OK 7
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_close
        jsr     prepare
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_delete
        CHECK_OK 8

        jsr     bbt_done

; 子程序：不让脚本把本页里的停靠标志当成父程序的，建立 CHILD.TXT 后以退出码 7 返回
child:
        lda     #0
        sta     bbg_test_stop           ; 子程序的页里没有被初始化的停靠标志，先清零
        jsr     prepare
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_create
        lda     #<fh
        ldx     #>fh
        jsr     bbg_file_close
        lda     #7
        jmp     bbg_dos_exit
