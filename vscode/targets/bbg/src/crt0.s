; ------------------------------------------------------------------------
; 名称：crt0.s
; 说明：步步高学生电脑 BBGDOS 的 CMD 程序的启动代码：文件前 2 个字节写装入地址，第一条指令是 JMP，
;       初始化后调用 main，main 返回时调用 DOS 的“程序结束返回”回到命令行。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：CMD 文件的前 2 个字节是装入地址，DOS 把后面的内容装到这个地址并从这里开始执行；不改堆栈与中断。
; ------------------------------------------------------------------------
        .setcpu "6502"
        .export         __STARTUP__ : absolute = 1      ; 标记本文件是启动代码
        .import         _main
        .import         __PROGRAM_START__
        .include        "bbg.inc"

        .segment        "LOADADDR"

        .addr   __PROGRAM_START__                       ; CMD 文件头：装入地址

        .segment        "STARTUP"

        jmp     reset                                   ; 装入地址上的第一条指令

reset:  cld
        jsr     _main
        ldx     #0                                      ; 返回码
        ldy     #0                                      ; 不要求初始化屏幕与系统资源
        lda     #DOS_PROGRAM_RETURN
        jmp     DOS_ENTRY                               ; 回到 DOS，不会返回
