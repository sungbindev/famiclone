; ------------------------------------------------------------------------
; 名称：shell.inc
; 说明：FBASIC 外壳的共用常量：ERAM 小段（stub）的地址与入口、现场交换缓冲、请求块、窗口页号表。加载器、小段与映像都包含它。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-16
; 备注：ERAM 是 $6000-$7FFF，任何窗口映射下都可见，所以小段在窗口换来换去时一直可用。
;       $6000-$65FF 是 SB-DOS 的状态，同时是 Family BASIC 的程序文本区，所以入口门交换零页、$0100-$07FF 与它们。
; ------------------------------------------------------------------------

; ---- 小段的入口（跳转表，每项 3 字节）----
SHELL_RUN               = $7E00         ; 小段运行地址
SHELL_LAUNCH            = $7E00         ; 进入映像：由加载器 jsr，从映像的复位向量开始运行
SHELL_DOS_CALL          = $7E03         ; 入口门：调用一个 SB-DOS 服务，要求 I 标志已置位
SHELL_LEAVE             = $7E06         ; 退出映像：还原 DOS 现场与窗口，回到加载器

; ---- 现场交换缓冲：14 页，放当前不在用的那一份现场 ----
SHELL_BUFFER            = $7000
SHELL_BUFFER_PAGES      = 14            ; 零页到 $07FF 共 8 页，加 ERAM $6000-$65FF 共 6 页

; ---- 变量区（小段之后，不参与交换）----
SHELL_IMAGE_WINDOW      = $7FA0         ; 映像运行时窗口 0-7 的页号，由加载器填写
SHELL_DOS_WINDOW        = $7FA8         ; 返回 DOS 时窗口 0-7 的页号，由加载器填写
SHELL_IRQ_MASK          = $7FB0         ; 映像运行时的中断屏蔽（$4032），映像可以改，入口门返回时写回硬件
SHELL_CHR_PAGE          = $7FB8         ; CHR 图案表 0 所在的扩展 RAM 页号（表 1 在下一页），由加载器填写；CHR 不在窗口里，由映像用 DMA 送进显存
SHELL_GAME_PAGE         = $7FB9         ; GAME 0-3 数据（3 页，game.s）起始的扩展 RAM 页号，由加载器填写；运行 GAME 命令时由映像用 DMA 搬进程序区
SHELL_DISK_PAGE         = $7FBA         ; SAVE、LOAD 用的暂存页（扩展 RAM 页号，0 为没有），映像自己管理
SHELL_DISK_SIZE         = $7FBB         ; 暂存页里文件的字节数（2 字节）

; ---- 请求块：映像调用入口门前填写 ----
; 服务的参数与结果单元在 DOS 现场里（零页、$0200-$07FF 或 ERAM $6000-$65FF），入口门换入 DOS 现场后才能写、读，
; 所以映像在这里列出“要写的单元与值”和“要读的单元”，入口门代为执行；读到的值放在结果区。
SHELL_REQ               = $7FC0
SHELL_REQ_FUNCTION      = SHELL_REQ + 0 ; 功能号
SHELL_REQ_SUB           = SHELL_REQ + 1 ; 子功能号
SHELL_REQ_WRITE_COUNT   = SHELL_REQ + 2 ; 要写的单元个数，0-8
SHELL_REQ_READ_COUNT    = SHELL_REQ + 3 ; 要读的单元个数，0-8
SHELL_REQ_WRITE_LO      = SHELL_REQ + 4 ; 要写的地址，低字节，8 项
SHELL_REQ_WRITE_HI      = SHELL_REQ + 12 ; 高字节
SHELL_REQ_WRITE_VALUE   = SHELL_REQ + 20 ; 值
SHELL_REQ_READ_LO       = SHELL_REQ + 28 ; 要读的地址，低字节，8 项
SHELL_REQ_READ_HI       = SHELL_REQ + 36 ; 高字节
SHELL_REQ_RESULT        = SHELL_REQ + 44 ; 读到的值，8 项
