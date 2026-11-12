; ------------------------------------------------------------------------
; 名称：hal-defs.asm
; 说明：步步高 BIOS 移植到小霸王学习机 SB2000（6576 模式）时用到的硬件寄存器与 HAL 变量定义。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-12
; 备注：符号定义，不占 ROM 空间。本工程的汇编语法是步步高 GABBG：'<' 取高字节，'>' 取低字节。
;       HAL 变量放在 ERAM $6010-$603F：步步高手册规定 $6000-$60FF 是系统保留区，只有 $6000、$6001、$6005 被使用。
; ------------------------------------------------------------------------

SBWindow0	equ	$4040		; 八个 4K 窗口的页寄存器，+n 对应 $8000+n*$1000，bank>=$80 是扩展 RAM
SBPalette	equ	$2040		; 6576 调色板，BG 16 组×4 色，精灵从 +16 起
HalVars		equ	$6010		; HAL 变量起点

vTog		equ	HalVars+0	; 虚拟 $2006 写入次序：0 下次写高字节，1 下次写低字节
vVAhi		equ	HalVars+1	; 虚拟 VRAM 地址高字节（已屏蔽到 $3f）
vVAlo		equ	HalVars+2	; 虚拟 VRAM 地址低字节
vBuf		equ	HalVars+3	; 虚拟 $2007 读缓冲
vReg2000	equ	HalVars+4	; 虚拟 $2000 的值
vMir		equ	HalVars+5	; 名称表镜像方式 0 竖 1 横 2/3 单屏
vSlot		equ	HalVars+6	; 四个 2K 图案槽的页号（4 字节）
vDummy		equ	HalVars+10	; 空端口：并口、PC 卡等没有接硬件的读写落在这里
tA		equ	HalVars+16	; 虚拟 PPU 过程的临时变量，不可重入
tX		equ	HalVars+17	;
tY		equ	HalVars+18	;
t0		equ	HalVars+19	;
t1		equ	HalVars+20	; 名称表号 j
t2		equ	HalVars+21	; 偏移高两位
t3		equ	HalVars+22	; 镜像掩码
t4		equ	HalVars+23	; 当前处理的名称表号 k
t5		equ	HalVars+24	;
t6		equ	HalVars+25	;
t7		equ	HalVars+26	;
t8		equ	HalVars+27	;
t9		equ	HalVars+28	;
tRow		equ	HalVars+29	; 图块行
tCol		equ	HalVars+30	; 图块列
tR		equ	HalVars+31	; 循环变量
tC		equ	HalVars+32	;
tRow0		equ	HalVars+33	;
tCol0		equ	HalVars+34	;
vKbHead		equ	HalVars+40	; 键盘扩描码队列的读指针（IRQ 写 vKbTail，主程序只改 vKbHead）
vKbTail		equ	HalVars+41	; 写指针
vKbBuf		equ	HalVars+42	; 队列，16 字节
HalVarsEnd	equ	HalVars+64	; 变量区结束，清零用
