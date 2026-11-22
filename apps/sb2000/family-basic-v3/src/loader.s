; ------------------------------------------------------------------------
; 名称：loader.s
; 说明：FBASIC 的 SB-DOS 程序外壳：启动时把 ERAM 小段复制到 ERAM、填好窗口页号表，进入整机映像；映像退出后回到 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：文件页序（链接配置 link.cfg 决定）：0、1 是常驻的本程序，2-9 是映像（窗口 0-7），10、11 是 CHR，12-14 是 GAME 0-3 的数据。
;       页号 = 起始页号（文件头偏移 5）加页序。
; ------------------------------------------------------------------------
        .export         _main
        .import         __STUB_LOAD__, __STUB_SIZE__    ; 链接器生成：小段在程序页里的位置与大小
        .include        "sb2000.inc"
        .include        "shell.inc"

        .segment        "CODE"

_main:
        php
        pla
        sta     saved_flags
        sei
        ldx     #$00                            ; 复制小段到 ERAM：先 256 字节，再余下的部分
@first: lda     __STUB_LOAD__,x
        sta     SHELL_RUN,x
        inx
        bne     @first
@rest:  lda     __STUB_LOAD__ + $100,x
        sta     SHELL_RUN + $100,x
        inx
        cpx     #<__STUB_SIZE__
        bne     @rest
        ldx     #$07                            ; 窗口页号表
@tables:
        lda     image_file_page,x
        clc
        adc     PROGRAM_START_PAGE
        sta     SHELL_IMAGE_WINDOW,x
        lda     dos_page,x
        bmi     @rom
        clc
        adc     PROGRAM_START_PAGE
        bcc     @store                          ; 起始页号加页序不会进位，只是为了跳过下一条
@rom:   and     #$7F                            ; ROM 页号 4-7 以 $84-$87 记，去掉标志位
@store: sta     SHELL_DOS_WINDOW,x
        dex
        bpl     @tables
        lda     PROGRAM_START_PAGE              ; CHR 的两页在文件的第 10、11 页
        clc
        adc     #10
        sta     SHELL_CHR_PAGE
        lda     PROGRAM_START_PAGE              ; GAME 数据的三页在文件的第 12-14 页
        clc
        adc     #12
        sta     SHELL_GAME_PAGE
        jsr     SHELL_LAUNCH                    ; 映像退出时回到这里
        lda     saved_flags
        pha
        plp
        rts

        .segment        "RODATA"

; 映像 8 页在文件里的页序（窗口 0-7）
image_file_page:
        .byte   2, 3, 4, 5, 6, 7, 8, 9

; 返回 DOS 时窗口 k 的页：窗口 0-3 是程序文件的第 0-3 页（加起始页号），窗口 4-7 是 ROM 页 4-7（位 7 置位表示不加起始页号）
dos_page:
        .byte   0, 1, 2, 3, $84, $85, $86, $87

        .segment        "BSS"

saved_flags:    .res    1
