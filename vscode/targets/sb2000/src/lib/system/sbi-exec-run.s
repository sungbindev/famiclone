; ------------------------------------------------------------------------
; 名称：sbi-exec-run.s
; 说明：库内部的汇编小例程：运行 PATH_BUFFER 里的程序，运行前后保存、恢复 C 运行库的零页。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：子程序的启动代码会重新设置 C 软件栈指针，并改写 C 运行库的零页（$0060-$007F，见 link.cfg 的 ZP），
;       不保存的话返回后调用者的软件栈就乱了；恢复只用绝对地址与 A、Y，不依赖软件栈。
;       保存区放在调用者自己的 BSS 里，子程序装在别的页，不会改到它。
; ------------------------------------------------------------------------
        .export         _sbi_exec_run

        .include        "sb2000.inc"

ZP_BASE = $60                                   ; 零页起点，与 link.cfg 的 __ZPSTART__ 一致
ZP_LAST = $1F                                   ; 零页大小 $20 的最后一个偏移

        .code

; unsigned char sbi_exec_run(void)：返回 DOS 出错码
_sbi_exec_run:
        ldy     #ZP_LAST
@save:  lda     ZP_BASE,y
        sta     zpsave,y
        dey
        bpl     @save
        lda     #FN_CLOCK
        sta     SYS_FUNCTION
        lda     #SUB_CLOCK_EXEC
        sta     SYS_SUBFUNCTION
        jsr     SYSCALL
        ldy     #ZP_LAST
@back:  lda     zpsave,y
        sta     ZP_BASE,y
        dey
        bpl     @back
        lda     DOS_ERROR
        ldx     #$00
        rts

        .bss

zpsave: .res    ZP_LAST + 1
