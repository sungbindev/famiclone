; ------------------------------------------------------------------------
; 名称：crt0.s
; 说明：裕兴学习机 DOS 程序的启动代码：文件最前面的 JMP，初始化后调用 main。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：DOS 把文件装在 $6000 并从 $6000 开始执行，没有签名与文件头，所以第一条指令是 JMP。
;       程序不能返回 DOS：main 返回后停在原地。
; ------------------------------------------------------------------------
        .setcpu "6502"
        .export         __STARTUP__ : absolute = 1      ; 标记本文件是启动代码
        .import         _main

        .segment        "STARTUP"

        jmp     reset                                   ; DOS 从文件第一个字节开始执行

reset:  sei
        cld
        ldx     #$FF
        txs
        jsr     _main
halt:   jmp     halt                                    ; main 返回后停住，不能回 DOS
