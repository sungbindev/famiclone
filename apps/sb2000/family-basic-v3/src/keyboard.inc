; ------------------------------------------------------------------------
; 名称：keyboard.inc
; 说明：FBASIC 移植版的键盘：IRQ 里读 SB2000 键盘的扫描码（码集 1），维护按着的键与修饰键；前台的 LB958 把按着的键译成原版内部键码。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-17
; 备注：被 fbasic.s 包含，和解释器在同一个翻译单元里，用它的标签与零页单元（$6F 有 Ctrl、$70 有 Shift、$71 有 GRPH、$72 假名模式、$50 假名键锁、$18 修饰键用过）。
;       按键用 SB2000 键盘的位置与字符，不用 Family BASIC 键盘的矩阵：字母、数字、标点按 PC 美式键盘；CapsLock 切换假名模式，Alt 是 GRPH，
;       F1-F8 是功能键 1-8，F9 与 Ctrl+C 是 STOP，Esc 是 ESC，Backspace 与 Delete 是 DEL，Insert 是 INS，Home 是 CLR/HOME，Shift+Home 是清屏，F12 退出回 DOS。
;       假名模式与 GRPH 的字符沿用原版按键字符的对应（字母键对应原版同字母键上的假名与图形）。
; ------------------------------------------------------------------------

; ---- 键盘状态（原版键盘矩阵用的 $0634-$063C）----
KB_KEY          = $0634                 ; 按着的键的扫描码，0 为没有
KB_EXT          = $0635                 ; 按着的键是否带 $E0 前缀
KB_PREFIX       = $0636                 ; 前缀状态：0 无，1 收到 $E0，$80 以上是暂停键后面还要忽略的字节数
KB_MODS         = $0637                 ; 修饰键位，见下
KB_RAW          = $0638                 ; 正在处理的原始码
KB_EXT_NOW      = $0639                 ; 正在处理的码是否带 $E0 前缀
KB_ANY          = $063A                 ; 译码时有没有修饰键
KB_CODE         = $063B                 ; 译出的键码

KB_SHIFT_BITS   = $03                   ; 左 Shift $01，右 Shift $02
KB_CTRL_BITS    = $0C                   ; 左 Ctrl $04，右 Ctrl $08
KB_ALT_BITS     = $30                   ; 左 Alt $10，右 Alt $20

KEY_C           = $2E
KEY_CAPS        = $3A
KEY_F9          = $43
KEY_F1          = $3B
KEY_F8          = $42
KEY_F12         = $58
KEY_TABLE_SIZE  = $59                   ; key_plain、key_shift 的项数（扫描码 $00-$58）
KANA_LIMIT      = $36                   ; 假名与 GRPH 表的项数（扫描码 $00-$35）
MOD_COUNT       = 6                     ; 修饰键表的项数
EXT_COUNT       = 9                     ; 带 $E0 前缀的键的项数

; ---- IRQ：只开键盘，读走扫描码 ----
irq_handler:
        pha
        txa
        pha
        tsx
        lda     $0103,x                         ; 栈里的状态寄存器：位 4 为 1 是 BRK，原版的 BRK 向量与 RESET 相同（热启动）
        and     #$10
        beq     irq_keyboard
        ldx     #$FD
        txs
        jmp     L80BA
irq_keyboard:
        tya
        pha
        lda     HW_IRQ_PENDING
        and     #HW_IRQ_KEYBOARD
        beq     irq_exit
        ora     SHELL_IRQ_MASK
        sta     HW_IRQ_MASK                     ; 屏蔽并清掉键盘中断
        lda     #$00
        sta     HW_KEY_CONTROL                  ; 键盘到系统，时钟由硬件驱动
        lda     HW_KEY_DATA
        jsr     kbd_receive
irq_exit:
        lda     SHELL_IRQ_MASK
        sta     HW_IRQ_MASK
        pla
        tay
        pla
        tax
        pla
        rti

; 处理一个扫描码：A = 码（位 7 为 1 是松开）
kbd_receive:
        ldx     KB_PREFIX
        bmi     @skip
        cmp     #$E0
        beq     @prefix
        cmp     #$E1
        beq     @pause
        sta     KB_RAW
        stx     KB_EXT_NOW
        lda     #$00
        sta     KB_PREFIX
        lda     KB_RAW
        and     #$7F
        tay
        ldx     #MOD_COUNT - 1
@find:  tya
        cmp     mod_scan,x
        bne     @next
        lda     mod_ext,x
        cmp     KB_EXT_NOW
        beq     @modifier
@next:  dex
        bpl     @find
        lda     KB_EXT_NOW                      ; 带前缀的 $2A、$36 是方向键附带的假 Shift，不是按键
        beq     @key
        cpy     #$2A
        beq     @done
        cpy     #$36
        beq     @done
@key:   bit     KB_RAW
        bmi     @release
        sty     KB_KEY
        lda     KB_EXT_NOW
        sta     KB_EXT
        rts
@release:
        cpy     KB_KEY
        bne     @done
        lda     KB_EXT_NOW
        cmp     KB_EXT
        bne     @done
        lda     #$00
        sta     KB_KEY
@done:  rts
@modifier:
        lda     mod_bit,x
        bit     KB_RAW
        bmi     @mod_release
        ora     KB_MODS
        bne     @mod_store
@mod_release:
        eor     #$FF
        and     KB_MODS
@mod_store:
        sta     KB_MODS
        rts
@prefix:
        lda     #$01
        sta     KB_PREFIX
        rts
@pause:
        lda     #$85                            ; 暂停键 $E1 后面还有 5 个字节
        sta     KB_PREFIX
        rts
@skip:
        dex
        cpx     #$80
        bne     @skip_store
        ldx     #$00
@skip_store:
        stx     KB_PREFIX
        rts

; 扫描键盘：A = 按着的键的原版键码，0 为没有键；同时设 $6F、$70、$71 与 $18、$50
; 原版每次扫描要 1.4 毫秒，BASIC 的按键重复与光标闪烁都按扫描次数计时，所以补足这段时间
LB958:
        ldy     #$03
@wait_outer:
        ldx     #$A6
@wait_inner:
        dex
        bne     @wait_inner
        dey
        bne     @wait_outer
        lda     #$00
        sta     $6F
        sta     $70
        sta     $71
        lda     KB_MODS
        and     #KB_CTRL_BITS
        beq     @no_ctrl
        dec     $6F
@no_ctrl:
        lda     KB_MODS
        and     #KB_SHIFT_BITS
        beq     @no_shift
        dec     $70
@no_shift:
        lda     KB_MODS
        and     #KB_ALT_BITS
        beq     @no_alt
        dec     $71
@no_alt:
        lda     $6F
        ora     $70
        ora     $71
        sta     KB_ANY
        beq     @check_key
        lda     #$FF
        sta     $18
@check_key:
        lda     KB_KEY
        bne     @held
        sta     $50
        lda     KB_ANY
        bne     @none
        sta     $18
@none:  lda     #$00
        rts
@held:  ldx     KB_EXT
        bne     @translate
        cmp     #KEY_F12
        bne     @translate
        jmp     exit_to_dos
@translate:
        jsr     kbd_translate
        sta     KB_CODE
        lda     KB_ANY
        bne     @give
        lda     $18
        ora     $50
        bne     @none
@give:  lda     KB_CODE
        rts

; 把按着的键译成原版键码：A = 键码，0 为没有
kbd_translate:
        ldx     KB_KEY
        lda     KB_EXT
        bne     @extended
        cpx     #KEY_CAPS
        beq     @kana_key
        cpx     #KEY_TABLE_SIZE
        bcs     @none
        lda     KB_MODS
        and     #KB_CTRL_BITS
        bne     @ctrl
        lda     KB_MODS
        and     #KB_SHIFT_BITS
        bne     @shift
        lda     KB_MODS
        and     #KB_ALT_BITS
        bne     @grph
        lda     $72
        beq     @base
        cpx     #KANA_LIMIT
        bcs     @base
        lda     kana_plain,x
        bne     @done
@base:  lda     key_plain,x
@done:  rts
@shift: lda     $72
        beq     @shift_ascii
        cpx     #KANA_LIMIT
        bcs     @shift_ascii
        lda     kana_shift,x
        bne     @done
@shift_ascii:
        lda     key_shift,x
        rts
@grph:  lda     $72
        beq     @none
        cpx     #KANA_LIMIT
        bcs     @base
        lda     kana_grph,x
        bne     @done
        beq     @base
@ctrl:  lda     key_plain,x                     ; Ctrl+字母得 $01-$1A，$00-$1F 的键原样
        cmp     #$20
        bcc     @done
        sec
        sbc     #$40
        bcc     @none
        cmp     #$20
        bcc     @done
@none:  lda     #$00
        rts
@kana_key:
        lda     $50                             ; 按着不放只翻转一次
        bne     @none
        lda     #$FF
        sta     $50
        lda     $72
        beq     @kana_on
        jsr     LB766
        jmp     @none
@kana_on:
        jsr     LB75E
        jmp     @none
@extended:
        ldy     #EXT_COUNT - 1
@ext_find:
        txa
        cmp     ext_scan,y
        beq     @ext_hit
        dey
        bpl     @ext_find
        bmi     @none
@ext_hit:
        lda     KB_MODS
        and     #KB_SHIFT_BITS
        bne     @ext_shift
        lda     ext_plain,y
        rts
@ext_shift:
        lda     ext_shift,y
        rts

; 按着的键是 F1-F8 时进位置位（它们要展开成功能键字符串）
LBA99:  lda     KB_EXT
        bne     @no
        lda     KB_KEY
        cmp     #KEY_F1
        bcc     @no
        cmp     #KEY_F8 + 1
        bcs     @no
        sec
        rts
@no:    clc
        rts

; STOP 键（F9 或 Ctrl+C）按着时返回 Z=1
L883A:  lda     KB_EXT
        bne     @not
        lda     KB_KEY
        cmp     #KEY_F9
        beq     @pressed
        cmp     #KEY_C
        bne     @not
        lda     KB_MODS
        and     #KB_CTRL_BITS
        beq     @not
@pressed:
        lda     #$00
        rts
@not:   lda     #$01
        rts

; ---- 修饰键：扫描码、是否带 $E0 前缀、位 ----
mod_scan:
        .byte   $2A,$36,$1D,$1D,$38,$38
mod_ext:
        .byte   $00,$00,$00,$01,$00,$01
mod_bit:
        .byte   $01,$02,$04,$08,$10,$20

; ---- 带 $E0 前缀的键：Home、上、左、右、下、Insert、Delete、小键盘回车、小键盘除号 ----
ext_scan:
        .byte   $47,$48,$4B,$4D,$50,$52,$53,$1C,$35
ext_plain:
        .byte   $0B,$1E,$1D,$1C,$1F,$12,$08,$0D,$2F
ext_shift:
        .byte   $0C,$1E,$1D,$1C,$1F,$12,$08,$0D,$3F

; ---- 按扫描码（$00-$58）查键码：无修饰键 ----
key_plain:
        .byte   $00,$1B,$31,$32,$33,$34,$35,$36,$37,$38,$39,$30,$2D,$3D,$08,$00
        .byte   $51,$57,$45,$52,$54,$59,$55,$49,$4F,$50,$5B,$5D,$0D,$00,$41,$53
        .byte   $44,$46,$47,$48,$4A,$4B,$4C,$3B,$27,$00,$00,$5C,$5A,$58,$43,$56
        .byte   $42,$4E,$4D,$2C,$2E,$2F,$00,$2A,$00,$20,$00,$F0,$F1,$F2,$F3,$F4
        .byte   $F5,$F6,$F7,$03,$00,$00,$00,$37,$38,$39,$2D,$34,$35,$36,$2B,$31
        .byte   $32,$33,$30,$2E,$00,$00,$00,$00,$00

; ---- 按着 Shift ----
key_shift:
        .byte   $00,$1B,$21,$40,$23,$24,$25,$5E,$26,$2A,$28,$29,$5F,$2B,$08,$00
        .byte   $51,$57,$45,$52,$54,$59,$55,$49,$4F,$50,$00,$00,$0D,$00,$41,$53
        .byte   $44,$46,$47,$48,$4A,$4B,$4C,$3A,$22,$00,$00,$00,$5A,$58,$43,$56
        .byte   $42,$4E,$4D,$3C,$3E,$3F,$00,$2A,$00,$20,$00,$F0,$F1,$F2,$F3,$F4
        .byte   $F5,$F6,$F7,$03,$00,$00,$00,$37,$38,$39,$2D,$34,$35,$36,$2B,$31
        .byte   $32,$33,$30,$2E,$00,$00,$00,$00,$00

; ---- 假名模式（扫描码 $00-$35）：键码是 0 的键改用上面的表 ----
kana_plain:
        .byte   $00,$00,$60,$61,$62,$63,$64,$74,$75,$76,$77,$78,$86,$00,$00,$00
        .byte   $65,$66,$67,$68,$69,$79,$7A,$7B,$7C,$7D,$00,$00,$00,$00,$6A,$6B
        .byte   $6C,$6D,$6E,$7E,$7F,$80,$81,$82,$00,$00,$00,$88,$6F,$70,$71,$72
        .byte   $73,$83,$84,$85,$8B,$8D
kana_shift:
        .byte   $00,$00,$8E,$8F,$90,$91,$92,$00,$00,$00,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$AB,$AC,$AD,$AE,$AF,$00,$00,$00,$00,$00,$00
        .byte   $00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$00,$96,$00
        .byte   $00,$93,$94,$95,$00,$00
kana_grph:
        .byte   $00,$00,$E8,$E9,$EA,$EB,$EC,$ED,$EE,$EF,$F0,$F1,$F2,$00,$00,$00
        .byte   $97,$98,$99,$9A,$9B,$A6,$A7,$A8,$A9,$AA,$00,$00,$00,$00,$9C,$9D
        .byte   $9E,$9F,$A0,$00,$00,$00,$00,$00,$00,$00,$00,$FE,$A1,$A2,$A3,$A4
        .byte   $A5,$00,$00,$00,$00,$00
