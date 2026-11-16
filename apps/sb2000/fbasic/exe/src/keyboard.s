; ------------------------------------------------------------------------
; 名称：keyboard.s
; 说明：SB2000 键盘服务：轮询键盘取扫描码（集 1），按“字符”映射表（keymap.s）翻成原版键盘矩阵里的键，填进原版的矩阵单元（$0634-$063C，现在在 fb_page6）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：代码在库页 12，由 platform.s 里的 kbd_matrix、kbd_stop_key 经跳板进入。F12 置退出请求；End 键对应原版的 STOP 键。
;       矩阵里每个键编号 = 行 × 8 + 位；第 7 行的位 4、3、2 是 Ctrl、Shift、GRPH。
; ------------------------------------------------------------------------
        .export         kbd_service, kbd_stop_service
        .import         fb_page6, keymap_normal, keymap_shifted, ext_scan, ext_normal, ext_shifted
        .import         kbd_ext, kbd_skip, kbd_mod, kbd_cur, kbd_id, kbd_need, kbd_exit, kbd_scan, kbd_brk, kbd_tmp
        .include        "sb2000.inc"
        .include        "keymap.inc"

MATRIX          = fb_page6 + $34
SCAN_F12        = $58
MOD_SHIFT       = $03
MOD_CTRL        = $04
MOD_ALT         = $08
; 原版扫描一次键盘矩阵约 1300 个周期，键盘里的重复延迟与防抖都按扫描次数计，所以这里补上这段耗时
SCAN_DELAY      = 200
; 每个按键至少让原版扫到 HOLD_SCANS 次再松开，松开后至少空闲 GAP_SCANS 次再送下一个（原版要在两个键之间扫到“没有键”）
HOLD_SCANS     = 6
GAP_SCANS      = 6
QUEUE_MASK      = 7

        .segment        "BANK12"

; 读完待处理的扫描码，再按当前状态生成矩阵
kbd_service:
        jsr     kbd_drain
        jsr     kbd_advance
        ldx     #8
        lda     #0
@clear: sta     MATRIX,x
        dex
        bpl     @clear
        sta     kbd_tmp
        lda     kbd_mod
        and     #MOD_CTRL
        beq     @alt
        lda     #$10
        sta     kbd_tmp
@alt:   lda     kbd_mod
        and     #MOD_ALT
        beq     @key
        lda     kbd_tmp
        ora     #$04
        sta     kbd_tmp
@key:   lda     kbd_cur
        cmp     #$FF
        beq     @only_mod
        pha
        lsr     a
        lsr     a
        lsr     a
        tax
        pla
        and     #$07
        tay
        lda     #1
        cpy     #0
        beq     @set
@shift: asl     a
        dey
        bne     @shift
@set:   sta     MATRIX,x
        lda     kbd_need
        beq     @finish
        lda     kbd_tmp
        ora     #$08
        sta     kbd_tmp
        jmp     @finish
@only_mod:                                      ; 单独的 PC Shift 不送进矩阵：原版见到孤立的修饰键会让下一个键被忽略一次
@finish:
        lda     MATRIX + 7
        ora     kbd_tmp
        sta     MATRIX + 7
        ldy     #SCAN_DELAY
@wait:  dey
        bne     @wait
        rts

; 原版 $883A 的替身：A = 0 表示 STOP 键（矩阵第 0 行位 0）按下，否则 $10
kbd_stop_service:
        jsr     kbd_drain
        lda     kbd_cur
        beq     @down
        lda     #$10
        rts
@down:  lda     #0
        rts

; 取走键盘里所有待处理的扫描码
kbd_drain:
        lda     HW_IRQ_PENDING
        and     #HW_IRQ_KEYBOARD
        beq     @done
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
        jsr     kbd_code
        jmp     kbd_drain
@done:  rts

; 处理一个扫描码：前缀、修饰键、F12 与普通键
kbd_code:
        cmp     #$E0
        bne     @not_e0
        lda     #1
        sta     kbd_ext
        rts
@not_e0:
        cmp     #$E1
        bne     @not_e1
        lda     #5
        sta     kbd_skip
        rts
@not_e1:
        ldx     kbd_skip
        beq     @real
        dec     kbd_skip
        rts
@real:  tax
        and     #$80
        sta     kbd_brk
        txa
        and     #$7F
        sta     kbd_scan
        lda     kbd_ext
        beq     @plain_id
        lda     #$80
@plain_id:
        ora     kbd_scan
        sta     kbd_id
        lda     #0
        sta     kbd_ext
        lda     kbd_id
        cmp     #$AA                            ; 扩展前缀带来的假 Shift（E0 2A、E0 36）
        beq     @ignore
        cmp     #$B6
        beq     @ignore
        ldx     #3
@mod:   lda     kbd_scan
        cmp     mod_scan,x
        beq     @is_mod
        dex
        bpl     @mod
        lda     kbd_id
        cmp     #SCAN_F12
        bne     @map
        lda     kbd_brk
        bne     @ignore
        lda     #1
        sta     kbd_exit
@ignore:
        rts
@is_mod:
        lda     mod_bit,x
        ldy     kbd_brk
        bne     @mod_up
        ora     kbd_mod
        sta     kbd_mod
        rts
@mod_up:
        eor     #$FF
        and     kbd_mod
        sta     kbd_mod
        rts
@map:   lda     kbd_id
        bmi     @ext
        ldx     kbd_scan
        cpx     #KEYMAP_COUNT
        bcs     @ignore
        lda     kbd_mod
        and     #MOD_SHIFT
        beq     @plain_normal
        lda     keymap_shifted,x
        jmp     @have
@plain_normal:
        lda     keymap_normal,x
        jmp     @have
@ext:   ldx     #EXT_COUNT - 1
@find:  lda     kbd_scan
        cmp     ext_scan,x
        beq     @found
        dex
        bpl     @find
        lda     #$FF
        jmp     @have_none
@found: lda     kbd_mod
        and     #MOD_SHIFT
        beq     @ext_normal
        lda     ext_shifted,x
        jmp     @have
@ext_normal:
        lda     ext_normal,x
@have:  ldy     kbd_brk
        bne     @release
@have_none:
        cmp     #$FF
        beq     @ignore
        ldx     kbd_qt
        sta     kbd_q,x
        lda     kbd_id
        sta     kbd_qid,x
        inx
        txa
        and     #QUEUE_MASK
        cmp     kbd_qh
        beq     @rel_ret                        ; 队满就丢弃这个键
        sta     kbd_qt
        rts
@release:
        lda     kbd_phase
        cmp     #1
        bne     @queued
        lda     kbd_id
        cmp     kbd_cur_id
        bne     @queued
        lda     #0
        sta     kbd_phys
        rts
@queued:                                        ; 还在队里的键已经松开：标记，出队时就不再等它松开
        ldx     kbd_qh
@scan:  cpx     kbd_qt
        beq     @rel_ret
        lda     kbd_qid,x
        cmp     kbd_id
        bne     @next
        lda     #$FE
        sta     kbd_qid,x
@next:  inx
        txa
        and     #QUEUE_MASK
        tax
        jmp     @scan
@rel_ret:
        rts

; 每扫一次推进一步：空闲时从队里取一个键按下；按下时等满 HOLD_SCANS 次且 PC 键已松开才松开；松开后空闲 GAP_SCANS 次
kbd_advance:
        lda     kbd_phase
        beq     @idle
        cmp     #1
        beq     @down
        dec     kbd_cnt
        bne     @done
        lda     #0
        sta     kbd_phase
        rts
@down:  lda     kbd_cnt
        beq     @check
        dec     kbd_cnt
        rts
@check: lda     kbd_phys
        bne     @done
        lda     #$FF
        sta     kbd_cur
        lda     #GAP_SCANS
        sta     kbd_cnt
        lda     #2
        sta     kbd_phase
@done:  rts
@idle:  ldx     kbd_qh
        cpx     kbd_qt
        beq     @done
        lda     kbd_q,x
        pha
        and     #$7F
        sta     kbd_cur
        pla
        and     #$80
        sta     kbd_need
        lda     kbd_qid,x
        sta     kbd_cur_id
        ldy     #1
        cmp     #$FE
        bne     @phys
        ldy     #0
@phys:  sty     kbd_phys
        inx
        txa
        and     #QUEUE_MASK
        sta     kbd_qh
        lda     #HOLD_SCANS
        sta     kbd_cnt
        lda     #1
        sta     kbd_phase
        rts

; 修饰键：左 Shift、右 Shift、Ctrl、Alt（含扩展的右 Ctrl、右 Alt）
mod_scan:
        .byte   $2A, $36, $1D, $38
mod_bit:
        .byte   $01, $02, $04, $08

        .segment        "BSS"
kbd_cur_id:     .res    1
kbd_q:          .res    QUEUE_MASK + 1
kbd_qid:        .res    QUEUE_MASK + 1
kbd_qh:         .res    1
kbd_qt:         .res    1
kbd_phase:      .res    1
kbd_cnt:        .res    1
kbd_phys:       .res    1
