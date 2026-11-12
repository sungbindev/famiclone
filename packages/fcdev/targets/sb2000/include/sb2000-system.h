/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-system.h
 * 说明：SB-DOS 程序的系统基础服务：等待帧、NMI 与中断控制、显示选项、状态查询、跨页调用、上下文保存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。程序最大 16K，装在 $8000-$BFFF 的 4 个窗口里；
 *       页组指连续的 4 个扩展 RAM 页，程序自己的页组首页号是 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_SYSTEM_H
#define SB2000_SYSTEM_H

/**
 * 等待指定数量的垂直消隐周期（帧）。
 *
 * @syscall $1B
 * @param frames 帧数；0 按 256 计。
 */
void __fastcall__ sb_wait_frames(unsigned char frames);

/**
 * 打开 NMI（垂直消隐中断）。NMI 里的各项服务（声音更新、读手柄等）要它开着才工作。
 *
 * @syscall $12/$80
 */
void sb_nmi_enable(void);

/**
 * 关闭 NMI。磁盘读写这类要独占硬件的操作前后常成对使用。
 *
 * @syscall $12/$00
 */
void sb_nmi_disable(void);

/**
 * 打开或关闭 NMI 里的一项服务。服务号用 SB_NMI_SERVICE_ 开头的常量：
 * SB_NMI_SERVICE_BAR_ANIM 条带动画，SB_NMI_SERVICE_SOUND 声音引擎每帧更新，
 * SB_NMI_SERVICE_HOOK5、SB_NMI_SERVICE_HOOK4 调用登记的钩子，SB_NMI_SERVICE_PAD 读手柄，
 * SB_NMI_SERVICE_PALETTE 上传 $0700 缓冲到调色板。
 * 打开 SB_NMI_SERVICE_PAD 时系统还把 $0634 置 $FF。服务号超出 1-6 时什么都不做。
 *
 * @syscall $12/$81-$86、$12/$01-$06
 * @param service 服务号，1 到 6。
 * @param enable 非 0 打开，0 关闭。
 */
void sb_nmi_service(unsigned char service, unsigned char enable);

/**
 * 登记一个 NMI 钩子：对应的 NMI 服务（SB_NMI_SERVICE_HOOK4、SB_NMI_SERVICE_HOOK5）打开后，
 * 每次 NMI 都会映射页组并调用入口例程。钩子是汇编例程，在中断里运行，以 RTS 返回，不能调用 C 函数。
 * 钩子号不是 4 或 5 时什么都不做。
 *
 * @param hook 钩子号，4 或 5。
 * @param group 钩子所在页组的首页号，自己程序里的例程传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
 * @param entry 入口例程的地址。
 */
void sb_nmi_hook_set(unsigned char hook, unsigned char group, unsigned int entry);

/**
 * 把一个 NMI 钩子恢复成空钩子。钩子号不是 4 或 5 时什么都不做。
 *
 * @syscall $12/$88
 * @param hook 钩子号，4 对应 SB_NMI_SERVICE_HOOK4，5 对应 SB_NMI_SERVICE_HOOK5。
 */
void sb_nmi_hook_reset(unsigned char hook);

/**
 * 屏蔽一个中断源（不再响应它）。中断源用 SB_IRQ_SOURCE_ 开头的常量：
 * 0 定时器，1 鼠标，2 键盘，3 中断 4，4 软驱，5 中断 2，6 中断 1。
 * 屏蔽键盘时系统还向 $4022 写 $40。中断源超出 0-6 时什么都不做。
 *
 * @syscall $13/$00-$06
 * @param source 中断源，0 到 6。
 */
void sb_irq_mask(unsigned char source);

/**
 * 允许一个中断源。中断源编号同 sb_irq_mask。
 * 允许定时器时系统设置定时器寄存器；允许鼠标时按鼠标是否接入更新 $0634。
 * 注意：中断源 5、6 的允许，系统用的屏蔽值取自表后相邻字节（原程序的疏漏）：
 * 允许中断源 5 实际清掉的是中断源 6 的屏蔽位，允许中断源 6 会把除定时器外的全部屏蔽位清 0。
 * 不建议对这两个中断源使用。
 *
 * @syscall $13/$80-$86
 * @param source 中断源，0 到 6。
 */
void sb_irq_enable(unsigned char source);

/**
 * 登记一个中断钩子：对应的中断发生后，系统映射页组并调用入口例程。钩子是汇编例程，在中断里运行，以 RTS 返回，不能调用 C 函数。
 * 钩子号：0 定时器，1 中断 3（系统不会调用它），2 中断 1，3 中断 4，4 鼠标，5 键盘，6 中断 2。
 * 钩子号超出 0-6 时什么都不做。
 *
 * @param hook 钩子号，0 到 6。
 * @param group 钩子所在页组的首页号，自己程序里的例程传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
 * @param entry 入口例程的地址。
 */
void sb_irq_hook_set(unsigned char hook, unsigned char group, unsigned int entry);

/**
 * 把一个中断钩子恢复成空钩子。钩子号：0 定时器，1 中断 3，2 中断 1，3 中断 4，4 鼠标，5 键盘。
 * 钩子号超出 0-5 时什么都不做；系统不提供中断 2 钩子的复位。
 *
 * @syscall $13/$88
 * @param hook 钩子号，0 到 5。
 */
void sb_irq_hook_reset(unsigned char hook);

/**
 * 把值按位并入显示相关的硬件寄存器并写入硬件。选项与它修改的位：
 * 0 扩展控制 $2008 的位 7；1 $2008 的位 0；2 $2008 的位 2、3；3 $2000 的低 2 位（名称表选择）；
 * 4 精灵显示：$2001 的位 4，同时值非 0 时打开 NMI 里的精灵 DMA；5 $2000 的位 5（8x16 精灵）；
 * 6 $2001 只保留位 1-4 再并入；7 $2001 的位 3（背景显示）。
 * 值要已经放在寄存器相应的位上，如选项 7 打开背景显示传 0x08。
 *
 * @syscall $10
 * @param option 选项，0 到 7。
 * @param value 要并入的位。
 * @return 0 成功；选项超出范围时为 $FF，寄存器不变。
 */
unsigned char sb_display_option(unsigned char option, unsigned char value);

/**
 * 关闭 NMI，等到垂直消隐后关闭背景与精灵显示，用于大批量改显存前。
 *
 * @syscall $19
 */
void sb_display_off(void);

/**
 * 等到垂直消隐后打开 NMI，并恢复背景与精灵显示。
 *
 * @syscall $1A
 */
void sb_display_on(void);

/**
 * 查询显示与系统状态。查询号与结果（位类的结果保持在原来的位上，不移位）：
 * 0 $2008 的位 7；1 $2008 的位 0；2 $2008 的位 2、3；3 $2000 的低 2 位（名称表选择）；
 * 4 $2001 的位 4（精灵显示）；5 $2000 的位 5（精灵尺寸）；
 * 6 显示模式号 0-3，同时把当前字库的位置放进系统的字库参数单元；
 * 7 NMI 服务开关 $0630 的位 1-7，位 0 换成“NMI 是否开着”；8 中断屏蔽影子；
 * 9 鼠标是否在线（1 在线）；10 恒为 1；11 键盘自检结果；12 $2001 的位 3、4（背景与精灵是否显示）。
 * 查询号超出 0-12 时不调用，返回 0。
 *
 * @syscall $1E
 * @param query 查询号，0 到 12。
 * @return 查询结果。
 */
unsigned char sb_system_state(unsigned char query);

/**
 * 系统硬件与状态初始化：重设显示、中断屏蔽、定时器、各钩子、键盘手柄鼠标状态、系统时钟（12:00:00）、DOS 内部状态等。
 * 程序一般不需要调用，调用后程序此前的显示与输入设置都会丢失。
 *
 * @syscall $18
 */
void sb_system_init(void);

/**
 * 映射页组到 $8000-$BFFF 后调用其中的例程，例程以 RTS 返回，返回后窗口恢复成调用前的样子。
 * 调用 ROM 里的运行库时页组为 0。要调用自己程序里的函数，页组传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
 *
 * @syscall $25
 * @param group 页组的首页号，连续 4 页。
 * @param entry 例程在窗口里的地址。
 */
void sb_call_banked(unsigned char group, unsigned int entry);

/**
 * 映射页组到 $8000-$BFFF 后跳转到其中的例程，不再回到本函数。
 * 例程的 RTS 直接返回到调用本函数的位置，相当于尾调用。
 *
 * @syscall $26
 * @param group 页组的首页号，连续 4 页。
 * @param entry 例程在窗口里的地址。
 */
void __fastcall__ sb_jump_banked(unsigned char group, unsigned int entry);

/**
 * 运行另一个程序，运行完返回。系统按文件大小分配扩展 RAM 页、读入文件、检查文件头魔数 AA 55，
 * 运行完关闭文件并释放页；子程序以 RTS 返回，用的窗口与页都是它自己的，不影响本程序。
 *
 * @syscall $7F/$00
 * @param path 程序的路径，零结尾，最长 28 个字符。
 * @return 0 成功；SB_ERR_FILE_NOT_FOUND 文件不存在；$3E 文件头魔数不符；SB_ERR_NO_MEMORY 没有足够的连续空闲页。
 */
unsigned char sb_exec(const char *path);

/**
 * 取硬件栈指针。返回的是本函数被调用之后的值，即调用者的栈指针减 2。
 *
 * @return 硬件栈指针的低字节（高字节固定为 $01）。
 */
unsigned char sb_stack_pointer(void);

/**
 * 把一个恢复点压入系统的上下文栈（最多 10 项，满了不保存），类似 setjmp。
 * 之后 sb_context_restore 或按 ESC 中断（sb_break_enable）会取出它，把硬件栈指针设成保存的值并跳到入口例程。
 * 入口例程是新的执行起点，不要用 RTS 返回。
 *
 * @syscall $1C
 * @param group 入口例程所在页组的首页号，自己程序里的例程传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
 * @param entry 入口例程的地址。
 * @param stack_pointer 恢复时要设的硬件栈指针，通常取 sb_stack_pointer() 的返回值。
 */
void sb_context_save(unsigned char group, unsigned int entry, unsigned char stack_pointer);

/**
 * 弹出上下文栈顶的恢复点并跳过去，类似 longjmp：关闭 NMI 里的各项服务与声音，恢复保存的栈指针后跳转。
 * 栈里有恢复点时不会返回；栈空时直接返回。
 *
 * @syscall $1D
 */
void sb_context_restore(void);

#endif
