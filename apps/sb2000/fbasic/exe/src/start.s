; ------------------------------------------------------------------------
; 名称：start.s
; 说明：FBASIC 的 SB-DOS 程序骨架（阶段 3）：多页程序，保存 DOS 现场并接管零页与 $0200-$07FF，内核 NMI 每帧调用钩子 5，
;       自己轮询键盘，库页切换与跨页调用，程序文本页，需要调用 SB-DOS 服务时经“DOS 入口门”换回 DOS 现场，退出时还原现场并返回 DOS。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：文件按页排列（见 fcdev 的 link-banked.cfg）：页 0、1 常驻（窗口 0、1），页 2 是库页 1（窗口 2，启动时就在），
;       页 3 是窗口 3，这里当程序文本页用（4K，对应原版的 $6000-$6FFF），页 4 起是库页 2。
;       库页切换改窗口 2：先写内核的窗口影子 WINDOW_SAVE_2，再写硬件寄存器；顺序不能反，否则切换中途来一次 NMI，
;       NMI 结束时按影子还原窗口，会把刚切好的库页换回去。NMI 钩子 5 由内核把窗口 0-3 换成程序的前 4 页后调用，所以钩子与它用到的代码都放在常驻页里。
;       骨架启动后依次自检并不断重复：跨页调用库页 1、2 的例程并核对结果，往程序文本页写读，经入口门申请并释放 1 个扩展 RAM 页；
;       自检的计数与错误数放在 BSS 变量里，供调试时读取；按 ESC 退出。
; ------------------------------------------------------------------------
        .export         _main
        .include        "sb2000.inc"

; NMI 里只开钩子 5（位 5）与内核的精灵 DMA（位 2）
FBASIC_NMI_FLAGS = $24

; 键盘扫描码（XT 套，按下码）
SCAN_ESC        = $01

NMI_HOOK5_VEC_HI = NMI_HOOK5_VEC_LO + 1

; 现场里的页：零页 + $0200-$07FF，共 7 页；硬件栈页 $0100 不换
IMAGE_PAGES     = 7

; 程序文本页：窗口 3（$B000），4K
TEXT_WINDOW     = $B000
TEXT_LAST       = TEXT_WINDOW + $0FFF

; 复制一整页：源与目的都是绝对地址，只用 A、X，不用零页（现场换入换出时零页正在被覆盖）
.macro  copy_page src, dst
        .local  loop
        ldx     #$00
loop:   lda     a:src,x
        sta     a:dst,x
        inx
        bne     loop
.endmacro

; 跨页调用：bank 是库页号（1 起），target 是库页里的例程；返回时 A 是例程的结果
.macro  far_call bank, target
        lda     #<target
        sta     call_target
        lda     #>target
        sta     call_target + 1
        ldx     #bank
        jsr     bank_call
.endmacro

        .segment        "CODE"

; 程序入口，由 crt0 调用
_main:
        php                                     ; 保存 I 标志，退出时还原
        pla
        sta     saved_p
        sei                                     ; 之后不接受 IRQ，键盘自己轮询
        jsr     nmi_off
        jsr     save_dos_image                  ; DOS 现场：零页 + $0200-$07FF，含窗口影子
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
        jsr     nmi_on

        jsr     gate_test                       ; 先经入口门走一遍，之后每圈自检
main_loop:
        jsr     self_test
        jsr     kbd_poll
        cmp     #SCAN_ESC
        bne     main_loop

        jsr     nmi_off
        jsr     load_dos_image                  ; 还原进入时的零页与 $0200-$07FF（含影子与钩子地址）
        jsr     restore_windows                 ; 硬件窗口回到 DOS 的设置
        lda     IRQ_MASK_SHADOW
        sta     HW_IRQ_MASK
        lda     PPU_CTRL_SHADOW
        sta     HW_PPU_CTRL                     ; 硬件的 NMI 开关回到 DOS 的设置
        lda     saved_p
        pha
        plp
        rts

; 一圈自检：跨页调用两个库页、写读程序文本页，三项任一不符就把对应的错误数加 1
self_test:
        inc     rounds
        lda     rounds
        sta     TEXT_WINDOW
        sta     TEXT_LAST
        far_call 1, bank1_probe
        cmp     #$A1
        beq     @bank2
        inc     bank_errors
@bank2: far_call 2, bank2_probe
        cmp     #$B2
        beq     @text
        inc     bank_errors
@text:  lda     TEXT_WINDOW
        cmp     rounds
        bne     @text_error
        lda     TEXT_LAST
        cmp     rounds
        beq     @window
@text_error:
        inc     text_errors
@window:
        lda     bank_current                    ; 窗口 2 的影子要等于当前库页对应的页
        cmp     #1
        beq     @first
        clc
        adc     #2
        bne     @compare
@first: lda     #2
@compare:
        sta     expected_rel
        lda     WINDOW_SAVE_2
        sec
        sbc     PROGRAM_START_PAGE
        cmp     expected_rel
        beq     @done
        inc     window_errors
@done:  rts

; 经入口门申请 1 页并释放，验证现场换入换出
gate_test:
        lda     #FN_PAGE
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
        rts

; NMI 钩子 5：内核 NMI 在写 PPU 控制/滚动寄存器之前调用，窗口 0-3 已换成本程序的前 4 页；以 RTS 结束
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

; 选择库页（窗口 2）：A = 库页号。库页 1 是文件第 2 页，库页 n（不小于 2）是第 n+2 页
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

; 跨页调用：X = 库页号，call_target 是库页里的例程；调用前后窗口 2 里的库页保持不变
bank_call:
        lda     bank_current
        pha
        txa
        jsr     bank_select
        jsr     call_thunk
        sta     call_result
        pla
        jsr     bank_select
        lda     call_result
        rts

call_thunk:
        jmp     (call_target)

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

; DOS 入口门：调用一个 SB-DOS 服务。程序占用着零页与 $0200-$07FF，内核的服务要在 DOS 现场里运行，
; 所以先停 NMI，把程序的现场存起来，换入 DOS 现场（含窗口影子，硬件窗口随之还原），调用，再换回。
; 调用后 DOS 现场被服务改写过，换回前先把它存回 dos_image，下次进来用的是最新的 DOS 现场。
; 本段代码在常驻页里，窗口 0、1 在换入前后都不变。
; 输入：A = 功能号，X = 子功能号，Y = 页数（只给页分配类服务用，写入 ALLOC_COUNT）
; 输出：gate_error = 出错码（DOS_ERROR），gate_value = ALLOC_START
dos_call:
        sta     req_function
        stx     req_subfunction
        sty     req_count
        jsr     nmi_off
        jsr     save_gbasic_image
        jsr     load_dos_image
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
        jsr     save_dos_image
        jsr     load_gbasic_image
        jsr     restore_windows                 ; 回到程序的窗口设置（含库页）
        lda     IRQ_MASK_SHADOW                 ; 回到程序的设置
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

        .segment        "BANK1"

; 库页 1 的测试例程
bank1_probe:
        lda     #$A1
        rts

        .segment        "BANK2"

; 库页 2 的测试例程
bank2_probe:
        lda     #$B2
        rts

        .segment        "BSS"

; 以下变量都在常驻页里，不属于被换入换出的现场
dos_image:      .res    IMAGE_PAGES * 256       ; 进入时（以及每次调用服务后）的 DOS 现场
gbasic_image:   .res    IMAGE_PAGES * 256       ; 调用服务期间存放的程序现场
saved_p:        .res    1                       ; 进入时的处理器状态
frame_count:    .res    1                       ; 钩子 5 每帧加 1
bank_current:   .res    1                       ; 窗口 2 里当前的库页号
call_target:    .res    2                       ; 跨页调用的目标
call_result:    .res    1                       ; 跨页调用的结果
expected_rel:   .res    1
req_function:   .res    1                       ; 入口门的请求：功能号、子功能号、页数
req_subfunction: .res   1
req_count:      .res    1
gate_error:     .res    1                       ; 入口门的结果：出错码、ALLOC_START
gate_value:     .res    1
probe_page:     .res    1                       ; 最近一次申请到的页号
probe_error:    .res    1                       ; 最近一次申请的出错码
probe_count:    .res    1                       ; 入口门成功往返的次数
rounds:         .res    1                       ; 自检圈数（低 8 位）
bank_errors:    .res    1                       ; 跨页调用结果不符的次数
text_errors:    .res    1                       ; 程序文本页读回不符的次数
window_errors:  .res    1                       ; 窗口 2 影子与库页号不符的次数
