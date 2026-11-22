; ------------------------------------------------------------------------
; 名称：crt0.s
; 说明：SB97 DOS 程序的启动代码：复位向量、中断向量与签名，初始化后调用 main。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：DOS 用 JMP ($FFFC) 启动程序，检查映像偏移 $7FF8、$7FF9 的签名 AA 55，所以向量段 VECTORS 放在 $FFF8。
;       程序不能返回 DOS：main 返回后停在原地。NMI 与 IRQ 都直接 RTI。
; ------------------------------------------------------------------------
        .setcpu "6502"
        .export         __STARTUP__ : absolute = 1      ; 标记本文件是启动代码
        .import         _main

        .segment        "CODE"

reset:  sei
        cld
        ldx     #$FF
        txs
        jsr     _main
halt:   jmp     halt                                    ; main 返回后停住，不能回 DOS

interrupt:
        rti

        .segment        "VECTORS"

        .byte   $AA, $55                                ; DOS 检查的签名
        .word   interrupt                               ; NMI
        .word   reset                                   ; 复位，DOS 从这里启动程序
        .word   interrupt                               ; IRQ
