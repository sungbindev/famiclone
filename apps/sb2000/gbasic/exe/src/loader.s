; ------------------------------------------------------------------------
; 名称：loader.s
; 说明：GBASIC 的 SB-DOS 程序外壳：程序文件里放着 32K 映像，启动时把 ERAM 小段（stub）复制到 ERAM，填好窗口页号表，进入映像；映像退出后回到 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：映像由 tools/build-image.mjs 生成 build/image.bin，按 4K 一页依次放进 BANK1、BANK2...BANK8；
;       文件页序：0、1 常驻（本程序），2 BANK1，3 栈页（不放内容），4 起 BANK2...，所以映像第 k 页在文件的 2、4、5、6、7、8、9、10 页。
;       fcdev 把文件整个装入连续的扩展 RAM 页，起始页号取自文件头偏移 5，映像页号 = 起始页号 + 文件页序。
;       启动时窗口 2 正好指向 BANK1（映像第 0 页），ERAM 小段的装载区就在那里。
;       ERAM 小段的运行地址与变量地址要与 image/image.cfg 一致。
; ------------------------------------------------------------------------
        .export         _main
        .include        "sb2000.inc"

STUB_RUN        = $7E00                         ; 小段的运行地址，第一项是 stub_launch
STUB_SIZE       = $01C0                         ; 小段的大小
STUB_IMAGE_WINDOW = $7FC0                       ; 映像窗口页号表，8 项
STUB_DOS_WINDOW = $7FC8                         ; 返回 DOS 的窗口页号表，8 项
STUB_LOAD       = $A000                         ; 小段的装载区：窗口 2 看到的映像第 0 页开头

        .segment        "CODE"

_main:
        php
        pla
        sta     saved_flags
        sei
        ldx     #$00                            ; 复制小段：第一页 256 字节，再 $C0 字节
@page0: lda     STUB_LOAD,x
        sta     STUB_RUN,x
        inx
        bne     @page0
@page1: lda     STUB_LOAD + $100,x
        sta     STUB_RUN + $100,x
        inx
        cpx     #<(STUB_SIZE - $100)
        bne     @page1
        ldx     #$07                            ; 窗口页号表
@tables:
        lda     image_file_page,x
        clc
        adc     PROGRAM_START_PAGE
        sta     STUB_IMAGE_WINDOW,x
        lda     dos_page_offset,x
        bmi     @rom
        clc
        adc     PROGRAM_START_PAGE
        bcc     @store                          ; 起始页号加页序不会进位，只是为了跳过下一条
@rom:   and     #$7F                            ; ROM 页号 4-7 以 $84-$87 记，去掉标志位
@store: sta     STUB_DOS_WINDOW,x
        dex
        bpl     @tables
        jsr     STUB_RUN                        ; 映像退出时回到这里
        lda     saved_flags
        pha
        plp
        rts

        .segment        "RODATA"

; 映像第 k 页在文件里的页序
image_file_page:
        .byte   2, 4, 5, 6, 7, 8, 9, 10

; 返回 DOS 时窗口 k 的页：窗口 0-3 是程序文件的第 0-3 页（加起始页号），窗口 4-7 是 ROM 页 4-7（位 7 置位表示不加起始页号）
dos_page_offset:
        .byte   0, 1, 2, 3, $84, $85, $86, $87

        .segment        "BSS"

saved_flags:    .res    1

        .segment        "BANK1"
        .incbin "../../build/image.bin", 0 * $1000, $1000

        .segment        "BANK2"
        .incbin "../../build/image.bin", 1 * $1000, $1000

        .segment        "BANK3"
        .incbin "../../build/image.bin", 2 * $1000, $1000

        .segment        "BANK4"
        .incbin "../../build/image.bin", 3 * $1000, $1000

        .segment        "BANK5"
        .incbin "../../build/image.bin", 4 * $1000, $1000

        .segment        "BANK6"
        .incbin "../../build/image.bin", 5 * $1000, $1000

        .segment        "BANK7"
        .incbin "../../build/image.bin", 6 * $1000, $1000

        .segment        "BANK8"
        .incbin "../../build/image.bin", 7 * $1000, $1000
