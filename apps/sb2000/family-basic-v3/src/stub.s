; ------------------------------------------------------------------------
; 名称：stub.s
; 说明：FBASIC 外壳的 ERAM 小段：进入映像（launch）、调用 SB-DOS 服务的入口门（dos_call）、退出映像（leave）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：运行地址 SHELL_RUN（ERAM），窗口 0-7 全是映像自己的页时它仍可见；由加载器复制到 ERAM 后用 jsr 进入。
;       DOS 现场与映像现场共 14 页（零页到 $07FF、ERAM $6000-$65FF）就地交换：缓冲 SHELL_BUFFER 里放着当前不在用的那一份。
;       交换时栈页也在换，所以交换代码不用 jsr、pha 之类动栈的指令，返回地址放在变量里。
;       入口门返回时 PPU 控制寄存器（$2000）是 0（NMI 关），由映像自己按它的值写回。
; ------------------------------------------------------------------------
        .include        "sb2000.inc"
        .include        "shell.inc"

PAGE_COUNT      = SHELL_BUFFER_PAGES

; 小段自己的变量，在 shell.inc 的变量区之后、请求块之前
DOS_STACK       = $7FB1                 ; 进入时 DOS 的栈指针
IMAGE_STACK     = $7FB2                 ; 映像调用入口门时的栈指针
IMAGE_FLAGS     = $7FB3                 ; 映像调用入口门时的处理器状态
EXCHANGE_BYTE   = $7FB4                 ; 交换时的暂存
EXCHANGE_RETURN = $7FB5                 ; 交换后继续运行的地址（2 字节）

; 把 8 个窗口设为 table 里的页号
.macro  set_windows table
        .local  loop
        ldx     #$07
loop:   lda     table,x
        sta     HW_WINDOW_0,x
        dex
        bpl     loop
.endmacro

; 交换完成后从 target 继续
.macro  exchange_then target
        lda     #<target
        sta     EXCHANGE_RETURN
        lda     #>target
        sta     EXCHANGE_RETURN + 1
        jmp     exchange
.endmacro

        .segment        "STUB"

; 跳转表：地址固定，映像经它们进出
        jmp     launch
        jmp     dos_call
        jmp     leave

; 进入映像：返回地址留在 DOS 的栈里，leave 用它回到加载器
launch:
        tsx
        stx     DOS_STACK
        lda     #$00
        sta     HW_PPU_CTRL                     ; 关 NMI，换窗口时不能有中断
        ldx     #PAGE_COUNT - 1                 ; 缓冲清零：交换后映像看到的是干净的现场
clear_page:
        txa
        clc
        adc     #>SHELL_BUFFER
        sta     clear_byte + 2
        lda     #$00
        tay
clear_byte:
        sta     a:$0000,y
        iny
        bne     clear_byte
        dex
        bpl     clear_page
        lda     #$FF
        sta     SHELL_IRQ_MASK                  ; 映像默认屏蔽全部中断
        set_windows SHELL_IMAGE_WINDOW
        exchange_then launched
launched:
        lda     SHELL_IRQ_MASK
        sta     HW_IRQ_MASK
        ldx     #$FF
        txs
        jmp     ($FFFC)                         ; 和 CPU 复位一样，从映像的复位向量开始

; 入口门：调用一个 SB-DOS 服务。输入请求块（SHELL_REQ），结果在请求块的结果区
dos_call:
        php
        pla
        sta     IMAGE_FLAGS
        sei
        tsx
        stx     IMAGE_STACK
        lda     #$00
        sta     HW_PPU_CTRL
        exchange_then in_dos
in_dos:
        set_windows SHELL_DOS_WINDOW
        ldx     DOS_STACK
        txs
        jsr     apply_writes
        lda     IRQ_MASK_SHADOW                 ; 现在是 DOS 的现场，影子是 DOS 的值
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        lda     SHELL_REQ_FUNCTION
        sta     SYS_FUNCTION
        lda     SHELL_REQ_SUB
        sta     SYS_SUBFUNCTION
        cli
        jsr     SYSCALL
        sei
        lda     #$00
        sta     HW_PPU_CTRL
        jsr     collect_reads
        set_windows SHELL_IMAGE_WINDOW
        exchange_then back_in_image
back_in_image:
        ldx     IMAGE_STACK
        txs
        lda     SHELL_IRQ_MASK
        sta     HW_IRQ_MASK
        lda     IMAGE_FLAGS
        pha
        plp
        rts

; 退出映像：还原 DOS 现场、窗口与显示状态，回到加载器
leave:
        sei
        lda     #$00
        sta     HW_PPU_CTRL
        exchange_then back_in_dos
back_in_dos:
        set_windows SHELL_DOS_WINDOW
        ldx     DOS_STACK
        txs
        lda     #$00
        sta     HW_VRAM_BANK
        lda     PPU_EXT_CTRL_SHADOW
        sta     HW_PPU_EXT
        lda     PPU_MASK_SHADOW
        sta     HW_PPU_MASK
        lda     IRQ_MASK_SHADOW
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        rts

; 把请求块里列出的值写进 DOS 现场里的单元（用自修改的地址，不碰零页）
apply_writes:
        ldx     #$00
write_loop:
        cpx     SHELL_REQ_WRITE_COUNT
        beq     write_done
        lda     SHELL_REQ_WRITE_LO,x
        sta     write_target + 1
        lda     SHELL_REQ_WRITE_HI,x
        sta     write_target + 2
        lda     SHELL_REQ_WRITE_VALUE,x
write_target:
        sta     a:$0000
        inx
        bne     write_loop
write_done:
        rts

; 读请求块里列出的 DOS 现场单元，放进结果区
collect_reads:
        ldx     #$00
read_loop:
        cpx     SHELL_REQ_READ_COUNT
        beq     read_done
        lda     SHELL_REQ_READ_LO,x
        sta     read_source + 1
        lda     SHELL_REQ_READ_HI,x
        sta     read_source + 2
read_source:
        lda     a:$0000
        sta     SHELL_REQ_RESULT,x
        inx
        bne     read_loop
read_done:
        rts

; 交换：系统页（零页到 $07FF、ERAM $6000-$65FF）与缓冲逐页互换，然后从 EXCHANGE_RETURN 继续；只用 A、X、Y
exchange:
        ldx     #PAGE_COUNT - 1
page_loop:
        lda     system_pages,x
        sta     system_read + 2
        sta     system_write + 2
        txa
        clc
        adc     #>SHELL_BUFFER
        sta     buffer_read + 2
        sta     buffer_write + 2
        ldy     #$00
byte_loop:
system_read:
        lda     a:$0000,y
        sta     EXCHANGE_BYTE
buffer_read:
        lda     a:$0000,y
system_write:
        sta     a:$0000,y
        lda     EXCHANGE_BYTE
buffer_write:
        sta     a:$0000,y
        iny
        bne     byte_loop
        dex
        bpl     page_loop
        jmp     (EXCHANGE_RETURN)

system_pages:
        .byte   $00, $01, $02, $03, $04, $05, $06, $07
        .byte   $60, $61, $62, $63, $64, $65
