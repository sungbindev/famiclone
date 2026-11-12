; ------------------------------------------------------------------------
; 名称：sbi-bank-trampoline.s
; 说明：库内部的汇编小例程：库页函数的调用蹦床，换入函数所在的库页，调用函数，返回后换回原来的库页。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：供 cc65 的 #pragma wrapped-call 使用：编译器在每次直接调用库页函数时，把库页号放进 tmp4、函数地址放进 ptr4，再 JSR 到本例程；
;       函数的参数（最后一个在 A、X，其余在 C 软件栈）原样传给函数，返回值原样带回。
;       原来的库页压在硬件栈里，所以库页函数之间可以嵌套调用；函数指针调用和变参函数不经过蹦床。
; ------------------------------------------------------------------------
        .export         _sbi_bank_trampoline
        .import         _sb_bank_select, _sbi_bank_current

        .include        "zeropage.inc"

        .code

_sbi_bank_trampoline:
        sta     value                           ; 参数（或上一步的返回值）暂存，换页时会被改写
        stx     value+1
        lda     ptr4
        sta     target
        lda     ptr4+1
        sta     target+1
        lda     _sbi_bank_current
        pha                                     ; 原来的库页
        lda     tmp4
        jsr     _sb_bank_select
        lda     value
        ldx     value+1
        jsr     call
        sta     value                           ; 函数的返回值
        stx     value+1
        pla
        jsr     _sb_bank_select
        lda     value
        ldx     value+1
        rts

call:   jmp     (target)

        .bss

        .align  2                               ; 间接跳转不能跨页读地址，保证 target 不落在页尾
target: .res    2
value:  .res    2
