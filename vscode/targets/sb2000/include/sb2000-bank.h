/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-bank.h
 * 说明：大程序（超过 16K）的换页支持：把代码与只读数据放进库页，调用时自动换入窗口 2。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含；只有在 fcdev.json 的 targetOptions 里写了 "banks": N（N 个库页）时才有库页可用。
 *       程序文件的布局：第 0、1 页是常驻部分（窗口 0、1，放 main、运行库、没放进库页的全部代码与数据，共 8K），
 *       第 2 页是库页 1（程序启动时就在窗口 2 里），第 3 页是栈页（窗口 3，放 C 软件栈，不能当库页），第 4 页起是库页 2、3、……。
 *       库页 4K，起始地址 $A000；库页里放代码与只读数据（const），可写的变量放在常驻部分。
 *       用法：库页里的函数在 .c 文件里用 SB_BANK_BEGIN(n) 与 SB_BANK_END() 包起来，声明在头文件里用 SB_BANK_CALLS_BEGIN(n) 与
 *       SB_BANK_CALLS_END() 包起来；之后从任何地方直接调用这些函数，编译器会经蹦床换入库页、调用、再换回来，库页函数之间可以嵌套调用。
 *       限制：只有直接按函数名的调用经过蹦床，用函数指针调用库页函数、变参函数都不行；
 *       库页里函数返回指向库页数据的指针，调用者拿到时窗口 2 已换回原来的库页，指针指向的内容可能已被换走；
 *       库页里的代码调用运行库函数时，传给系统服务的地址如果在窗口 2 里，指的是当时换入的库页。
 *       一个库页 4K，多个文件的库页段会按库页号合并到同一页，页放不下时链接失败。
 *       程序最大约 468K（113 个库页）：DOS 一次把整个文件装入内存，空闲的连续页不够时程序不能启动。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_BANK_H
#define SB2000_BANK_H

/** 蹦床：库页函数的调用都经它换页，库内部使用，程序不要直接调用。 */
void sbi_bank_trampoline(void);

/** 把字面量转成字符串，库内部使用。 */
#define SBI_BANK_STR(x) #x
#define SBI_BANK_STRING(x) SBI_BANK_STR(x)

/** 生成一条 #pragma，库内部使用。 */
#define SBI_BANK_PRAGMA_(x) _Pragma(#x)
#define SBI_BANK_PRAGMA(x) SBI_BANK_PRAGMA_(x)

/**
 * 在 .c 文件里标出库页 n 的开始：之后定义的函数与只读数据放进库页 n（段 BANKn），直到 SB_BANK_END()。
 * n 是 1 到 fcdev.json 里 banks 之间的整数字面量。
 */
#define SB_BANK_BEGIN(n) \
    SBI_BANK_PRAGMA(code-name (push, SBI_BANK_STRING(BANK##n))) \
    SBI_BANK_PRAGMA(rodata-name (push, SBI_BANK_STRING(BANK##n)))

/** 结束 SB_BANK_BEGIN 标出的库页。 */
#define SB_BANK_END() \
    SBI_BANK_PRAGMA(rodata-name (pop)) \
    SBI_BANK_PRAGMA(code-name (pop))

/**
 * 在声明库页 n 里的函数之前使用：之后声明的函数，调用时经蹦床换入库页 n，直到 SB_BANK_CALLS_END()。
 * 要放在函数所在 .c 文件与调用者都包含的头文件里。
 */
#define SB_BANK_CALLS_BEGIN(n) \
    SBI_BANK_PRAGMA(wrapped-call (push, sbi_bank_trampoline, n))

/** 结束 SB_BANK_CALLS_BEGIN 标出的声明。 */
#define SB_BANK_CALLS_END() \
    SBI_BANK_PRAGMA(wrapped-call (pop))

/**
 * 把一个库页换进窗口 2，窗口 2 里原来的库页看不到了。调用蹦床时会自动换页，只有需要直接读库页里的数据时才自己调用。
 *
 * @syscall $29
 * @param bank 库页号，1 到 fcdev.json 里 banks 的值。
 */
void sb_bank_select(unsigned char bank);

/**
 * 取窗口 2 当前的库页号。
 *
 * @return 库页号；程序刚启动时是 1。
 */
unsigned char sb_bank_current(void);

#endif
