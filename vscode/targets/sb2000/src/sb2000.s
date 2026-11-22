; ------------------------------------------------------------------------
; 名称：sb2000.s
; 说明：sb2000.h 里各函数的实现：显示字符串、换行、等待按键、直接调用系统服务。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-10
; 备注：C 调用约定为 cc65 的 fastcall：最后一个参数在 A（低字节）与 X（高字节），其余参数在 C 栈上，由被调用者弹出。
;       显示借用 ROM 页组 0 里的运行库，经功能 $25 调用；LIB_PRINT_STRING 在光标当前列显示字符串，不移动光标，
;       所以 sb_puts 先把光标列置 1（行首）再显示，显示后换行。字符串在程序所在的页组里，页组首页号取自文件头偏移 5（DOS 装入时写入）。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .export         _sb_puts, _sb_newline, _sb_getkey, _sb_syscall
        .import         popa

        .code

; void sb_puts(const char *s)：从行首显示字符串并换行
_sb_puts:
        pha
        lda     #$01
        sta     CURSOR_COL
        pla
        sta     STR_PTR_LO
        stx     STR_PTR_HI
        lda     PROGRAM_START_PAGE
        sta     LIB_GROUP
        lda     #<LIB_PRINT_STRING
        ldx     #>LIB_PRINT_STRING
        jsr     rom_call

; void sb_newline(void)：回到行首并换行，行满时由运行库滚动屏幕
_sb_newline:
        lda     #$01
        sta     CURSOR_COL
        lda     #<LIB_NEWLINE
        ldx     #>LIB_NEWLINE

; 经功能 FN_CALL_BANKED 调用 ROM 页组 0 里 X:A 处的例程，返回后回到调用者
rom_call:
        sta     PROG_ENTRY_LO
        stx     PROG_ENTRY_HI
        lda     #FN_CALL_BANKED
        sta     SYS_FUNCTION
        lda     #$00
        sta     SYS_GROUP
        jmp     SYSCALL

; unsigned char sb_getkey(void)：等待按键，返回它的 ASCII 码
_sb_getkey:
        lda     #FN_INPUT
        sta     SYS_FUNCTION
        lda     #SUB_KEY_GET_WAIT
        sta     SYS_SUBFUNCTION
        jsr     SYSCALL
        lda     KEY_ASCII
        ldx     #$00
        rts

; void sb_syscall(unsigned char function, unsigned char sub)：调用系统服务，结果由调用者读各服务规定的变量
_sb_syscall:
        sta     SYS_SUBFUNCTION
        jsr     popa
        sta     SYS_FUNCTION
        jmp     SYSCALL
