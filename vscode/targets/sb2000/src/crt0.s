; ------------------------------------------------------------------------
; 名称：crt0.s
; 说明：SB-DOS 程序的启动代码：程序文件头，占住窗口的全部 4 页，初始化 C 运行库，调用 main，返回 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：程序从 $8000 起装入，文件头为 4C lo hi AA 55，偏移 5、6 由 DOS 写成起始页号与页数，偏移 7 保留。
;       DOS 只按文件大小分配页，而 C 的数据（BSS）与软件栈在 $8000-$BFFF 的 4 个窗口里，文件之外的页没有人占用，
;       会被页分配服务分给别人，所以启动时把不足 4 页的部分紧接着程序的页再申请下来，返回 DOS 前释放；
;       申请不到连续的页时以“内存不足”返回 DOS。
;       C 软件栈在窗口的顶端（link.cfg 里的 __STACKSTART__ 与 __STACKSIZE__）；
;       C 运行库的零页放在 $0060 起，这段零页在 BIOS 的符号表里没有任何引用，开机到提示符期间也没有写入，所以返回 DOS 前不需要恢复。
;       main 的返回值被忽略，程序总是以 CLC、RTS 返回 DOS。
; ------------------------------------------------------------------------
        .export         _exit
        .export         __STARTUP__ : absolute = 1      ; 标记本文件是启动代码
        .import         zerobss, _main
        .import         initlib, donelib
        .import         __STACKSTART__                  ; 链接器生成

        .include        "zeropage.inc"
        .include        "sb2000.inc"

; 窗口里的页数：$8000-$BFFF 共 4 个窗口
WINDOW_PAGES = 4

        .segment        "HEADER"

        jmp     start
        .byte   $AA, $55                                ; 魔数
        .byte   $00, $00, $00                           ; 偏移 5、6 由 DOS 改写，偏移 7 保留

        .segment        "STARTUP"

start:  jsr     claim_pages
        bcc     @ready
        lda     #ERR_NO_MEMORY                          ; 出错码放在 $6308，再带进位返回 DOS
        sta     LIB_GROUP
        sec
        rts
@ready: lda     #<__STACKSTART__
        ldx     #>__STACKSTART__
        sta     c_sp
        stx     c_sp+1
        jsr     zerobss
        jsr     initlib
        jsr     _main
_exit:  jsr     donelib
        jsr     release_pages
        clc
        rts

; 把窗口里文件之外的页申请下来：每次申请 1 页，必须正好是紧接着程序的页，否则释放已申请的并失败
; 输出：进位清 0 成功，置 1 失败（已释放）
claim_pages:
        lda     PROGRAM_PAGE_COUNT
        cmp     #WINDOW_PAGES
        bcs     @done
        sta     have
        lda     #WINDOW_PAGES
        sec
        sbc     have
        sta     missing
        ldy     #$00
@loop:  lda     #$01
        sta     ALLOC_COUNT
        lda     #FN_PAGE
        sta     SYS_FUNCTION
        lda     #SUB_PAGE_ALLOC
        sta     SYS_SUBFUNCTION
        jsr     SYSCALL
        lda     DOS_ERROR
        bne     @fail
        tya                                             ; 预期的页号 = 起始页 + 文件占的页数 + 已申请的页数
        clc
        adc     have
        clc
        adc     PROGRAM_START_PAGE
        cmp     ALLOC_START
        bne     @mismatch
        iny
        cpy     missing
        bcc     @loop
        sty     claimed
@done:  clc
        rts
@mismatch:
        lda     #$01                                    ; 申请到的页位置不对：释放它
        sta     ALLOC_COUNT
        lda     #FN_PAGE
        sta     SYS_FUNCTION
        lda     #SUB_PAGE_FREE
        sta     SYS_SUBFUNCTION
        jsr     SYSCALL
@fail:  sty     claimed                                 ; 已申请的 Y 个页交给 release_pages 释放
        jsr     release_pages
        sec
        rts

; 释放启动时申请的页
release_pages:
        lda     claimed
        beq     @done
        sta     ALLOC_COUNT
        lda     have
        clc
        adc     PROGRAM_START_PAGE
        sta     ALLOC_START
        lda     #FN_PAGE
        sta     SYS_FUNCTION
        lda     #SUB_PAGE_FREE
        sta     SYS_SUBFUNCTION
        jsr     SYSCALL
        lda     #$00
        sta     claimed
@done:  rts

        .segment        "DATA"

have:       .byte   $00                                 ; DOS 按文件大小给程序的页数
missing:    .byte   $00                                 ; 还差的页数
claimed:    .byte   $00                                 ; 已申请下来的页数
