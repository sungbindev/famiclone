/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-printer.h
 * 说明：SB-DOS 程序的打印机服务：24 针点阵打印机（Epson ESC/P）的复位、走纸、定位、发送字节、页长、打印文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。功能 $76-$7A 是同一个入口，函数统一用 $76。打印机经并口驱动，模拟器里是虚拟的 ESC/P 打印机。
 *       文字打印按 GB2312：$20-$7E 是西文（8×16，宽 8 列），$A1 以上两个字节组成一个汉字（16×16，宽 16 列）；
 *       每行 1144 列，非位图模式每行打两遍。文字里可以夹控制码，见 sb_printer_text 的说明。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_PRINTER_H
#define SB2000_PRINTER_H

/** 打印格式。 */
typedef struct {
    /** 字宽倍率，0 到 5，每一列重复 1 到 6 次。 */
    unsigned char width;
    /** 点数模式：0 每个点上下各重复 2 次，1 每个点重复 3 次，2 位图模式（只打一遍）。 */
    unsigned char dots;
    /** 每个字符后附加的空白列数。 */
    unsigned char gap;
    /** 换行后的进纸量（ESC J 的 n，单位 1/216 英寸）。 */
    unsigned char feed;
    /** 发送变换模式，0 不变。 */
    unsigned char transform;
} sb_printer_style_t;

/**
 * 打印一段文字，到 0 结束。每行先发 ESC @ 与图形命令，再逐字符取字模发送，行满换行，行末进纸并让打印头回到左端。
 * 文字里的控制码：$0D $0A 换行；$92 n 设置字宽与点数模式；$95 n 设置变换模式；$99 n 设置附加空白列；$9B n 设置换行后的进纸量，具体见系统调用手册（docs/sb2000/02-系统调用手册.md）5.11 节。
 *
 * @syscall $76/$00
 * @param text 文字，零结尾，在程序窗口 $8000-$BFFF 里。
 * @param style 打印格式；传 0 时用字宽 0、点数模式 0、无附加空白列、进纸量 $18。
 */
void sb_printer_text(const char *text, const sb_printer_style_t *style);

/**
 * 发送复位命令 ESC @。
 *
 * @syscall $76/$01
 */
void sb_printer_reset(void);

/**
 * 发送 ESC <，让打印头回到左端。
 *
 * @syscall $76/$02
 */
void sb_printer_home(void);

/**
 * 复位打印机并发送回车。
 *
 * @syscall $76/$03
 */
void sb_printer_carriage_return(void);

/**
 * 进纸，发送 ESC J n。
 *
 * @syscall $76/$04
 * @param units 进纸量，单位 1/216 英寸。
 */
void sb_printer_feed(unsigned char units);

/**
 * 横向定位：绝对定位发送 ESC $ 加两个字节，相对定位发送 ESC \ 加两个字节。
 *
 * @syscall $76/$05、$76/$06
 * @param relative 0 绝对定位，非 0 相对定位。
 * @param position 位置，单位 1/60 英寸（绝对）或 1/120 英寸（相对），16 位。
 */
void sb_printer_position(unsigned char relative, unsigned int position);

/**
 * 把一个字节原样发给打印机，不做变换。
 *
 * @syscall $76/$07
 * @param value 要发送的字节。
 */
void sb_printer_send_byte(unsigned char value);

/**
 * 发送 ESC * 39 1144，进入 24 针三倍密度位图模式，每行 1144 列。
 *
 * @syscall $76/$08
 */
void sb_printer_graphics_mode(void);

/**
 * 初始化并口并等待。只初始化一次，之后的调用什么都不做；其他打印机函数每次都会先调用它，所以一般不必单独调用。
 *
 * @syscall $76/$0E
 */
void sb_printer_init_port(void);

/**
 * 按纸张类型设置页长计数（单位 1/216 英寸，进纸时从中扣除）：0 是 1900，1 是 2523（11.68 英寸，A4），2 是 2106，3 是 1488，4 是 2077，5 是 1821；
 * 6 以上只把参数存进计数的高字节。
 *
 * @syscall $76/$0F
 * @param paper 纸张类型，0 到 5。
 */
void sb_printer_page_length(unsigned char paper);

/**
 * 取本页剩余走纸量（单位 1/216 英寸）。剩余不足 $58 时打印文字会自动收尾进纸并按纸张类型重新装入页长。
 *
 * @return 剩余走纸量。
 */
unsigned int sb_printer_page_left(void);

#endif
