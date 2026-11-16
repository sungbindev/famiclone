; ------------------------------------------------------------------------
; 名称：start.s
; 说明：GBASIC 的 SB-DOS 程序骨架（阶段 3）：保存 DOS 现场，接管零页与 $0200-$07FF，让内核 NMI 调用钩子 5，
;       自己轮询键盘，需要调用 SB-DOS 服务时经“DOS 入口门”换回 DOS 现场，退出时还原现场并返回 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：策略见 docs/移植对照表.md 第 3.2 节。本文件只验证这套机制能运行，还没有 BASIC 解释器。
;       fcdev 的启动代码 crt0 已占好窗口 0-3 的 4 页、清好 BSS，然后调用 _main；_main 返回后它释放页并回到 DOS。
;       骨架的行为：背景色每帧由钩子 5 推进；按 ESC 退出；按 G 经入口门申请并释放 1 个扩展 RAM 页，
;       页号存在 probe_page，出错码存在 probe_error，供调试时读取。
; ------------------------------------------------------------------------
        .export         _main
        .include        "sb2000.inc"

; NMI 里只开钩子 5（位 5）与内核的精灵 DMA（位 2）
GBASIC_NMI_FLAGS = $24

; 键盘扫描码（XT 套，按下码）
SCAN_ESC        = $01
SCAN_G          = $22

NMI_HOOK5_VEC_HI = NMI_HOOK5_VEC_LO + 1

; 现场里的页：零页 + $0200-$07FF，共 7 页；硬件栈页 $0100 不换
IMAGE_PAGES     = 7

; 复制一整页：源与目的都是绝对地址，只用 A、X，不用零页（现场换入换出时零页正在被覆盖）
.macro  copy_page src, dst
        .local  loop
        ldx     #$00
loop:   lda     a:src,x
        sta     a:dst,x
        inx
        bne     loop
.endmacro

        .segment        "CODE"

; 程序入口，由 crt0 调用
_main:
        php                                     ; 保存 I 标志，退出时还原
        pla
        sta     saved_p
        sei                                     ; 之后不接受 IRQ，键盘自己轮询
        jsr     nmi_off
        jsr     save_dos_image                  ; DOS 现场：零页 + $0200-$07FF
        lda     #<nmi_hook                      ; 钩子 5：地址、页组、开关
        sta     NMI_HOOK5_VEC_LO
        lda     #>nmi_hook
        sta     NMI_HOOK5_VEC_HI
        lda     PROGRAM_START_PAGE
        sta     NMI_HOOK5_PAGE
        lda     #GBASIC_NMI_FLAGS
        sta     NMI_FLAGS
        jsr     nmi_on

main_loop:
        jsr     kbd_poll
        cmp     #SCAN_ESC
        beq     quit
        cmp     #SCAN_G
        bne     main_loop
        lda     #FN_PAGE                        ; 经入口门申请 1 页并释放，验证现场换入换出
        ldx     #SUB_PAGE_ALLOC
        ldy     #$01
        jsr     dos_call
        lda     gate_value
        sta     probe_page
        lda     gate_error
        sta     probe_error
        lda     #FN_PAGE
        ldx     #SUB_PAGE_FREE
        ldy     #$01
        jsr     dos_call                        ; ALLOC_START 仍是刚才的页号，直接释放
        inc     probe_count
        jmp     main_loop

quit:   jsr     nmi_off
        jsr     load_dos_image                  ; 还原进入时的零页与 $0200-$07FF（含影子与钩子地址）
        lda     IRQ_MASK_SHADOW
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL                     ; 硬件的 NMI 开关回到 DOS 的设置
        lda     saved_p
        pha
        plp
        rts

; NMI 钩子 5：内核 NMI 在写 PPU 控制/滚动寄存器之前调用，窗口 0-3 已换成本程序的页组；以 RTS 结束
nmi_hook:
        inc     frame_count
        lda     frame_count
        lsr     a
        lsr     a
        lsr     a
        and     #$0F
        ora     #$20
        sta     HW_PALETTE                      ; 背景色
        rts

; 关 NMI：硬件的 NMI 位清 0，影子不动
nmi_off:
        lda     PPU_CTRL_SHADOW
        and     #<~HW_CTRL_NMI
        sta     HW_PPU_CTRL
        rts

; 开 NMI：影子与硬件的 NMI 位都置 1
nmi_on: lda     PPU_CTRL_SHADOW
        ora     #HW_CTRL_NMI
        sta     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        rts

; 轮询键盘，不等待。键盘数据的接收协议与内核的 KBD_HANDLER 相同：先把 $4022 写 0，读 $4020，
; 写 $4032 清除键盘请求（并恢复屏蔽影子），再把 $4022 写 $80
; 输出：A = 按下或松开的扫描码，没有新码时为 0
kbd_poll:
        lda     HW_IRQ_PENDING
        and     #HW_IRQ_KEYBOARD
        beq     @none
        lda     #$00
        sta     HW_KEY_CONTROL
        lda     HW_KEY_DATA
        pha
        lda     IRQ_MASK_SHADOW
        ora     #HW_IRQ_KEYBOARD
        sta     HW_IRQ_MASK
        lda     IRQ_MASK_SHADOW
        sta     HW_IRQ_MASK
        lda     #$80
        sta     HW_KEY_CONTROL
        pla
        rts
@none:  lda     #$00
        rts

; DOS 入口门：调用一个 SB-DOS 服务。GBASIC 占用着零页与 $0200-$07FF，内核的服务要在 DOS 现场里运行，
; 所以先停 NMI，把 GBASIC 的现场存起来，换入 DOS 现场，调用，再换回。调用后 DOS 现场被服务改写过，
; 换回前先把它存回 dos_image，下次进来用的是最新的 DOS 现场。
; 输入：A = 功能号，X = 子功能号，Y = 页数（只给页分配类服务用，写入 ALLOC_COUNT）
; 输出：gate_error = 出错码（DOS_ERROR），gate_value = ALLOC_START
dos_call:
        sta     req_function
        stx     req_subfunction
        sty     req_count
        jsr     nmi_off
        jsr     save_gbasic_image
        jsr     load_dos_image
        lda     IRQ_MASK_SHADOW                 ; DOS 的中断屏蔽
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW                 ; DOS 的 NMI 设置
        sta     HW_PPU_CTRL
        lda     req_count
        sta     ALLOC_COUNT
        lda     req_function
        sta     SYS_FUNCTION
        lda     req_subfunction
        sta     SYS_SUBFUNCTION
        cli
        jsr     SYSCALL
        sei
        lda     DOS_ERROR
        sta     gate_error
        lda     ALLOC_START
        sta     gate_value
        jsr     nmi_off
        jsr     save_dos_image
        jsr     load_gbasic_image
        lda     IRQ_MASK_SHADOW                 ; 回到 GBASIC 的设置
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        rts

; 现场换入换出：零页与 $0200-$07FF（共 7 页）与缓冲之间复制
save_dos_image:
        copy_page $0000, dos_image + 0 * 256
        copy_page $0200, dos_image + 1 * 256
        copy_page $0300, dos_image + 2 * 256
        copy_page $0400, dos_image + 3 * 256
        copy_page $0500, dos_image + 4 * 256
        copy_page $0600, dos_image + 5 * 256
        copy_page $0700, dos_image + 6 * 256
        rts

load_dos_image:
        copy_page dos_image + 0 * 256, $0000
        copy_page dos_image + 1 * 256, $0200
        copy_page dos_image + 2 * 256, $0300
        copy_page dos_image + 3 * 256, $0400
        copy_page dos_image + 4 * 256, $0500
        copy_page dos_image + 5 * 256, $0600
        copy_page dos_image + 6 * 256, $0700
        rts

save_gbasic_image:
        copy_page $0000, gbasic_image + 0 * 256
        copy_page $0200, gbasic_image + 1 * 256
        copy_page $0300, gbasic_image + 2 * 256
        copy_page $0400, gbasic_image + 3 * 256
        copy_page $0500, gbasic_image + 4 * 256
        copy_page $0600, gbasic_image + 5 * 256
        copy_page $0700, gbasic_image + 6 * 256
        rts

load_gbasic_image:
        copy_page gbasic_image + 0 * 256, $0000
        copy_page gbasic_image + 1 * 256, $0200
        copy_page gbasic_image + 2 * 256, $0300
        copy_page gbasic_image + 3 * 256, $0400
        copy_page gbasic_image + 4 * 256, $0500
        copy_page gbasic_image + 5 * 256, $0600
        copy_page gbasic_image + 6 * 256, $0700
        rts

        .segment        "BSS"

; 以下变量都在程序窗口里，不属于被换入换出的现场
dos_image:      .res    IMAGE_PAGES * 256       ; 进入时（以及每次调用服务后）的 DOS 现场
gbasic_image:   .res    IMAGE_PAGES * 256       ; 调用服务期间存放的 GBASIC 现场
saved_p:        .res    1                       ; 进入时的处理器状态
frame_count:    .res    1                       ; 钩子 5 每帧加 1
req_function:   .res    1                       ; 入口门的请求：功能号、子功能号、页数
req_subfunction: .res   1
req_count:      .res    1
gate_error:     .res    1                       ; 入口门的结果：出错码、ALLOC_START
gate_value:     .res    1
probe_page:     .res    1                       ; 最近一次申请到的页号
probe_error:    .res    1                       ; 最近一次申请的出错码
probe_count:    .res    1                       ; 入口门成功往返的次数
