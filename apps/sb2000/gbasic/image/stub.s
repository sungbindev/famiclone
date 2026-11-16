; ------------------------------------------------------------------------
; 名称：stub.s
; 说明：GBASIC 映像在 ERAM 里的常驻小段：进入（launch）、调用 SB-DOS 服务的入口门（dos_call）、退出（leave）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：运行时窗口 0-7 全是映像自己的页，这段代码在窗口之外的 ERAM（$7E00 起）里，窗口换来换去时它始终可见。
;       零页与 $0100-$07FF 共 8 页在“DOS 现场”和“GBASIC 现场”之间就地交换：缓冲 BUFFER 里放着当前不在用的那一份。
;       交换过程中栈页也在换，所以交换代码里不能有 jsr、pha 这类动栈的指令。
;       变量区在 $7FC0 起（段 STUBVAR），由加载器在启动前填写窗口页号表。
;       入口门要求调用时 I 标志已置位；DOS 服务返回后重新置位。
; ------------------------------------------------------------------------
        .export         stub_launch, stub_dos_call, stub_leave
        .export         stub_request_function, stub_request_sub, stub_irq_mask, stub_saved_ppu_ctrl
        .export         stub_image_window, stub_dos_window
        .import         image_entry

        .include        "sb2000.inc"

; 缓冲在映像最后 2K（窗口 7 内），与 image.cfg 里的 BUFFER 区一致
BUFFER_HIGH = $F7

        .segment        "STUBVAR"

stub_image_window:      .res 8          ; 映像运行时窗口 0-7 的页号，由加载器填写
stub_dos_window:        .res 8          ; 返回 DOS 时窗口 0-7 的页号：程序文件的 4 页与 ROM 页 4-7，由加载器填写
stub_request_function:  .res 1          ; 入口门的请求：功能号、子功能号
stub_request_sub:       .res 1
stub_irq_mask:          .res 1          ; 进入时 DOS 的中断屏蔽影子，映像轮询键盘时用
stub_saved_ppu_ctrl:    .res 1          ; 进入时 DOS 的 PPU 控制影子（返回 DOS 时由 DOS 现场自带，这里留给映像读取）
dos_stack:              .res 1          ; DOS 现场的栈指针
image_stack:            .res 1          ; 映像调用入口门时的栈指针
exchange_byte:          .res 1          ; 交换时的暂存

; 交换 DOS 与映像的现场：系统页 0-7 与缓冲 BUFFER 的 8 页互换。只用 A、X、Y 与 exchange_byte，不动栈
.macro  exchange_pages
        .local  page_loop, byte_loop, system_read, system_write, buffer_read, buffer_write
        ldx     #$07
page_loop:
        txa
        sta     system_read + 2
        sta     system_write + 2
        clc
        adc     #BUFFER_HIGH
        sta     buffer_read + 2
        sta     buffer_write + 2
        ldy     #$00
byte_loop:
system_read:
        lda     $0000,y
        sta     exchange_byte
buffer_read:
        lda     $0000,y
system_write:
        sta     $0000,y
        lda     exchange_byte
buffer_write:
        sta     $0000,y
        iny
        bne     byte_loop
        dex
        bpl     page_loop
.endmacro

; 把 8 个窗口设为 table 里的页号
.macro  set_windows table
        .local  window_loop
        ldx     #$07
window_loop:
        lda     table,x
        sta     HW_WINDOW_0,x
        dex
        bpl     window_loop
.endmacro

        .segment        "STUB"

; 进入映像：由加载器 jsr；返回地址留在 DOS 的栈里，stub_leave 用它回到加载器
stub_launch:
        tsx
        stx     dos_stack
        lda     IRQ_MASK_SHADOW
        sta     stub_irq_mask
        lda     PPU_CTRL_SHADOW
        sta     stub_saved_ppu_ctrl
        lda     #$00
        sta     HW_PPU_CTRL                     ; 关 NMI，换窗口时不能有中断
        set_windows stub_image_window
        exchange_pages                          ; 此后系统页是映像的初始现场（缓冲原来的内容），DOS 现场在缓冲里
        ldx     #$FF
        txs
        jmp     image_entry

; 入口门：调用一个 SB-DOS 服务。输入 stub_request_function、stub_request_sub；结果由调用者在调用后读 ERAM 里的结果单元
stub_dos_call:
        tsx
        stx     image_stack
        lda     #$00
        sta     HW_PPU_CTRL
        exchange_pages                          ; 系统页换成 DOS 现场，映像现场在缓冲里
        set_windows stub_dos_window
        ldx     dos_stack
        txs
        lda     IRQ_MASK_SHADOW
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        lda     stub_request_function
        sta     SYS_FUNCTION
        lda     stub_request_sub
        sta     SYS_SUBFUNCTION
        cli
        jsr     SYSCALL
        sei
        lda     #$00
        sta     HW_PPU_CTRL
        set_windows stub_image_window
        exchange_pages                          ; DOS 现场（服务可能改过）回到缓冲，映像现场回到系统页
        ldx     image_stack
        txs
        rts

; 退出映像：还原 DOS 现场与窗口，回到加载器
stub_leave:
        sei
        lda     #$00
        sta     HW_PPU_CTRL
        exchange_pages
        set_windows stub_dos_window
        ldx     dos_stack
        txs
        lda     IRQ_MASK_SHADOW
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        rts
