; ------------------------------------------------------------------------
; 名称：platform.s
; 说明：FBASIC 的 SB-DOS 程序外壳（阶段 4.1）：多页程序，接管零页与 $0200-$07FF，内核 NMI 每帧调用钩子 5 去运行解释器的 NMI 例程，
;       库页切换与跨页调用（跳板进入例程 far_enter），需要调用 SB-DOS 服务时经“DOS 入口门”换回 DOS 现场。解释器本体在 core.s（tools/port.mjs 生成）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：文件按页排列（见 fcdev 的 link-banked.cfg）：页 0、1 常驻（窗口 0、1），页 2 是库页 1（窗口 2，启动时就在），
;       页 3 是窗口 3，这里当程序文本页用（4K，对应原版的 $6000-$6FFF），页 4 起是库页 2。
;       库页切换改窗口 2：先写内核的窗口影子 WINDOW_SAVE_2，再写硬件寄存器；顺序不能反，否则切换中途来一次 NMI，
;       NMI 结束时按影子还原窗口，会把刚切好的库页换回去。NMI 钩子 5 由内核把窗口 0-3 换成程序的前 4 页后调用，所以钩子与它用到的代码都放在常驻页里。
;       NMI 里也会经跳板进入库页，所以钩子入口先把跳板的临时变量存到栈里，结束时还原。
;       DOS 现场（零页 + $0200-$07FF，共 7 页）保存在现场缓冲库页里，入口门用“交换”：缓冲与内存互换，再互换回来。
; ------------------------------------------------------------------------
        .export         _main, far_enter, frame_count, bank_current, fb_page6
        .import         L80BA_far, L8971_far
        .include        "sb2000.inc"
        .include        "layout.inc"

; NMI 里只开钩子 5（位 5）；原版的 NMI 例程自己做精灵 DMA
FBASIC_NMI_FLAGS = $20

NMI_HOOK5_VEC_HI = NMI_HOOK5_VEC_LO + 1

; 现场里的页：零页 + $0200-$07FF，共 7 页；硬件栈页 $0100 不换
IMAGE_PAGES     = 7
IMAGE_WINDOW    = $A000

; 跳板的临时变量个数（far_info 3 字节，far_a、far_x、far_y、far_p 各 1 字节）
FAR_STATE_SIZE  = 7

        .segment        "CODE"

; 程序入口，由 crt0 调用。解释器的初始化不会返回
_main:
        php                                     ; 保存 I 标志（目前没有退出的路径）
        pla
        sta     saved_p
        sei                                     ; 之后不接受 IRQ
        jsr     nmi_off
        jsr     save_image                      ; DOS 现场存进缓冲库页
        lda     #1                              ; 启动时窗口 2 里是库页 1
        sta     bank_current
        lda     #<nmi_hook                      ; 钩子 5：地址、页组、开关
        sta     NMI_HOOK5_VEC_LO
        lda     #>nmi_hook
        sta     NMI_HOOK5_VEC_HI
        lda     PROGRAM_START_PAGE
        sta     NMI_HOOK5_PAGE
        lda     #FBASIC_NMI_FLAGS
        sta     NMI_FLAGS
        jsr     L80BA_far                       ; 原版的复位入口：初始化后进入主循环
        rts

; NMI 钩子 5：内核 NMI 在写 PPU 控制/滚动寄存器之前调用，窗口 0-3 已换成本程序的前 4 页（窗口 2 是库页 1）；以 RTS 结束。
; 先把跳板的临时变量、跳板里自修改的两个操作数和当前库页号存到栈里，再运行解释器的 NMI 例程
nmi_hook:
        ldx     #FAR_STATE_SIZE - 1
@save:  lda     far_state,x
        pha
        dex
        bpl     @save
        lda     far_read + 1
        pha
        lda     far_read + 2
        pha
        lda     bank_current
        pha
        lda     #1                              ; 钩子里窗口 2 就是库页 1
        sta     bank_current
        inc     frame_count
        jsr     L8971_far
        pla
        sta     bank_current
        pla
        sta     far_read + 2
        pla
        sta     far_read + 1
        ldx     #0
@load:  pla
        sta     far_state,x
        inx
        cpx     #FAR_STATE_SIZE
        bne     @load
        rts

; 选择库页（窗口 2）：A = 库页号。库页 1 是文件第 2 页，库页 n（不小于 2）是第 n+2 页；只改 A 与标志
bank_select:
        sta     bank_current
        cmp     #1
        beq     @first
        clc
        adc     #2
        bne     @map
@first: lda     #2
@map:   clc
        adc     PROGRAM_START_PAGE
        sta     WINDOW_SAVE_2                   ; 先写影子，再写硬件
        sta     HW_WINDOW_2
        rts

; 跳板的进入例程：跳板是 jsr far_enter，后面跟 3 个字节：目标地址（低、高）与库页号。
; 进入时 A、X、Y 与标志都是调用者给目标例程的参数，返回时是目标例程的结果；调用前后窗口 2 里的库页不变。
; 跳板的返回地址不回到跳板，所以目标例程 RTS 之后直接回到跳板的调用者
far_enter:
        sta     far_a
        stx     far_x
        sty     far_y
        php
        pla
        sta     far_p
        pla                                     ; 跳板返回地址（指向跳板 jsr 的最后一个字节），低字节
        sta     far_read + 1                    ; 自修改：下面 lda 的操作数
        pla
        sta     far_read + 2
        ldx     #1
far_read:
        lda     $FFFF,x
        sta     far_info - 1,x
        inx
        cpx     #4
        bne     far_read
        lda     bank_current
        pha
        lda     far_info + 2
        jsr     bank_select
        lda     far_p
        pha
        ldx     far_x
        ldy     far_y
        lda     far_a
        plp
        jsr     far_jump
        php
        sta     far_a
        stx     far_x
        sty     far_y
        pla
        sta     far_p
        pla                                     ; 调用前的库页
        jsr     bank_select
        lda     far_p
        pha
        lda     far_a
        ldx     far_x
        ldy     far_y
        plp
        rts

far_jump:
        jmp     (far_info)

        .assert (far_info & $FF) <> $FF, error, "far_info 不能跨页，否则 jmp (far_info) 会取错高字节"

; 窗口 0-3 的硬件寄存器按影子还原（入口门换入现场后用）
restore_windows:
        lda     WINDOW_SAVE_0
        sta     HW_WINDOW_0
        lda     WINDOW_SAVE_1
        sta     HW_WINDOW_1
        lda     WINDOW_SAVE_2
        sta     HW_WINDOW_2
        lda     WINDOW_SAVE_3
        sta     HW_WINDOW_3
        rts

; 关 NMI：硬件的 NMI 位清 0，影子不动
nmi_off:
        lda     PPU_CTRL_SHADOW
        and     #<~HW_CTRL_NMI
        sta     HW_PPU_CTRL
        rts

; DOS 入口门：调用一个 SB-DOS 服务。程序占用着零页与 $0200-$07FF，内核的服务要在 DOS 现场里运行，
; 所以先停 NMI，把现场缓冲与内存互换（DOS 现场换入，含窗口影子，硬件窗口随之还原），调用，再换回。
; 调用后 DOS 现场被服务改写过，换回时它自然留在缓冲里，下次进来用的是最新的 DOS 现场。
; 本段代码在常驻页里，窗口 0、1 在换入前后都不变。
; 输入：A = 功能号，X = 子功能号，Y = 页数（只给页分配类服务用，写入 ALLOC_COUNT）
; 输出：gate_error = 出错码（DOS_ERROR），gate_value = ALLOC_START
dos_call:
        sta     req_function
        stx     req_subfunction
        sty     req_count
        jsr     nmi_off
        jsr     swap_image
        jsr     restore_windows
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
        jsr     swap_image
        jsr     restore_windows                 ; 回到程序的窗口设置（含库页）
        lda     IRQ_MASK_SHADOW                 ; 回到程序的设置
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL
        rts

; 把现场缓冲库页映射到窗口 2（只写硬件，不写影子，用完由 restore_windows 或调用者还原）
map_image:
        lda     PROGRAM_START_PAGE
        clc
        adc     #BANK_IMAGE + 2
        sta     HW_WINDOW_2
        rts

; 把零页与 $0200-$07FF 复制到现场缓冲（启动时保存 DOS 现场用）
save_image:
        jsr     map_image
        ldy     #IMAGE_PAGES - 1
@page:  lda     image_pages,y
        sta     @read + 2
        tya
        clc
        adc     #>IMAGE_WINDOW
        sta     @write + 2
        ldx     #0
@read:  lda     a:$0000,x
@write: sta     IMAGE_WINDOW,x
        inx
        bne     @read
        dey
        bpl     @page
        lda     WINDOW_SAVE_2
        sta     HW_WINDOW_2
        rts

; 把零页与 $0200-$07FF 同现场缓冲互换。零页本身在被换，所以这里不用零页，只用 A、X、Y 与栈
swap_image:
        jsr     map_image
        ldy     #IMAGE_PAGES - 1
@page:  lda     image_pages,y
        sta     @read + 2
        sta     @store + 2
        tya
        clc
        adc     #>IMAGE_WINDOW
        sta     @load + 2
        sta     @keep + 2
        ldx     #0
@read:  lda     a:$0000,x
        pha
@load:  lda     IMAGE_WINDOW,x
@store: sta     a:$0000,x
        pla
@keep:  sta     IMAGE_WINDOW,x
        inx
        bne     @read
        dey
        bpl     @page
        rts

; 现场里各页的高字节
image_pages:
        .byte   $00, $02, $03, $04, $05, $06, $07

        .segment        "BSS"

; 以下变量都在常驻页里，不属于被换入换出的现场
far_state:                                      ; 钩子入口要存取的跳板临时变量，必须连在一起
far_info:       .res    3                       ; 跳板里的目标地址（低、高）与库页号
far_a:          .res    1
far_x:          .res    1
far_y:          .res    1
far_p:          .res    1
saved_p:        .res    1                       ; 进入时的处理器状态
frame_count:    .res    1                       ; 钩子 5 每帧加 1
bank_current:   .res    1                       ; 窗口 2 里当前的库页号
req_function:   .res    1                       ; 入口门的请求：功能号、子功能号、页数
req_subfunction: .res   1
req_count:      .res    1
gate_error:     .res    1                       ; 入口门的结果：出错码、ALLOC_START
gate_value:     .res    1
fb_page6:       .res    256                     ; 原版 $0600-$06FF 的替身（那一页在学习机里是内核的变量）
