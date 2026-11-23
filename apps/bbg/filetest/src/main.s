; ------------------------------------------------------------------------
; 名称：main.s
; 说明：BBGDOS 文件查找（FILE_FIRST、FILE_NEXT）验证程序：列出默认盘根目录的文件，每行显示属性、11 字节文件名与大小低字节。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：通道块布局来自对 BBGDOS.SYS 的反汇编：偏移 5 起 11 字节为文件名，偏移 $16 为属性掩码，
;       查找结果在偏移 $17 起（属性、时间、日期、大小、文件名）；文件名取自调用者页里 FH_BUFFER 指向的 11 字节。
; ------------------------------------------------------------------------

        .setcpu "6502"
        .export _main

BIOS_ENTRY       = $5800
FILE_ENTRY       = $5803
DOS_ENTRY        = $580C
BIOS_GET_PAGE    = 2
DOS_DISPLAY_CHAR = 2
FILE_FIRST       = 6
FILE_NEXT        = 7

FH_COMMAND       = 0
FH_DRIVE         = 1
FH_PAGE          = 2
FH_BUFFER        = 3
FH_DTA_ATTRIBUTE = $17
FH_DTA_SIZE      = $1C
FH_DTA_NAME      = $20
FH_SIZE          = 48

        .segment "BSS"

cur_page:       .res 1
fh:             .res FH_SIZE
saved_y:        .res 1

        .segment "RODATA"

; 查找用的文件名：11 个通配符
pattern_any:    .byte "???????????"

        .segment "CODE"

_main:
        lda #BIOS_GET_PAGE
        jsr BIOS_ENTRY
        sta cur_page
        ldx #FH_SIZE - 1
        lda #0
@clear:
        sta fh,x
        dex
        bpl @clear
        lda #FILE_FIRST
        sta fh + FH_COMMAND
        lda cur_page
        sta fh + FH_PAGE
        lda #<pattern_any
        sta fh + FH_BUFFER
        lda #>pattern_any
        sta fh + FH_BUFFER + 1
        jsr call_file
        bcs @done
@entry:
        lda fh + FH_DTA_ATTRIBUTE
        jsr print_hex
        ldx #' '
        jsr print_char
        ldy #0
@name:
        sty saved_y
        ldx fh + FH_DTA_NAME,y
        jsr print_char
        ldy saved_y
        iny
        cpy #11
        bne @name
        ldx #' '
        jsr print_char
        lda fh + FH_DTA_SIZE + 1
        jsr print_hex
        lda fh + FH_DTA_SIZE
        jsr print_hex
        jsr print_newline
        lda #FILE_NEXT
        sta fh + FH_COMMAND
        jsr call_file
        bcc @entry
@done:
        pha
        ldx #'E'
        jsr print_char
        ldx #'='
        jsr print_char
        pla
        jsr print_hex
        jsr print_newline
        rts

; 调用文件功能，通道块在 fh
call_file:
        ldx #>fh
        ldy #<fh
        jmp FILE_ENTRY

; 显示 A 的两位十六进制
print_hex:
        pha
        lsr a
        lsr a
        lsr a
        lsr a
        jsr print_nibble
        pla
        and #$0F
print_nibble:
        cmp #10
        bcc @digit
        adc #6
@digit:
        adc #'0'
        tax

; 显示字符 X
print_char:
        lda #DOS_DISPLAY_CHAR
        jmp DOS_ENTRY

print_newline:
        ldx #$0D
        jsr print_char
        ldx #$0A
        jmp print_char
