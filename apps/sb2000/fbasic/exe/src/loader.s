; ------------------------------------------------------------------------
; 名称：loader.s
; 说明：FBASIC 的 SB-DOS 程序外壳：程序文件里放着 32K 映像与 ERAM 小段，启动时把小段复制到 ERAM、填好窗口页号表，进入映像；映像退出后回到 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：映像（build/image.bin）按 4K 一页依次放进 BANK1-BANK8，小段（build/stub.bin）放在本程序的常驻页里。
;       fcdev 的多页程序文件按页排列：0、1 常驻（本程序），2 是 BANK1，3 是栈页（不放内容），4 起是 BANK2...，
;       所以映像第 k 页在文件里的页序是 2、4、5、6、7、8、9、10，页号 = 起始页号（文件头偏移 5）加页序。
; ------------------------------------------------------------------------
        .export         _main
        .include        "sb2000.inc"
        .include        "../../image/shell.inc"

        .segment        "CODE"

_main:
        php
        pla
        sta     saved_flags
        sei
        ldx     #$00                            ; 复制小段到 ERAM：先 256 字节，再余下的部分
@first: lda     stub_image,x
        sta     SHELL_RUN,x
        inx
        bne     @first
@rest:  lda     stub_image + $100,x
        sta     SHELL_RUN + $100,x
        inx
        cpx     #<(stub_image_end - stub_image)
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
        jsr     SHELL_LAUNCH                    ; 映像退出时回到这里
        lda     saved_flags
        pha
        plp
        rts

        .segment        "RODATA"

; 映像第 k 页在文件里的页序
image_file_page:
        .byte   2, 4, 5, 6, 7, 8, 9, 10

; 返回 DOS 时窗口 k 的页：窗口 0-3 是程序文件的第 0-3 页（加起始页号），窗口 4-7 是 ROM 页 4-7（位 7 置位表示不加起始页号）
dos_page:
        .byte   0, 1, 2, 3, $84, $85, $86, $87

stub_image:
        .incbin "../../build/stub.bin"
stub_image_end:

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
