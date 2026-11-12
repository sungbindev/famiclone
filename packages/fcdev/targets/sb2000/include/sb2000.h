/*
 * ------------------------------------------------------------------------
 * 名称：sb2000.h
 * 说明：SB-DOS（小霸王学习机 SB2000）程序的 C 接口：显示、按键、直接调用系统服务，以及常用内存变量的访问宏。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-10
 * 备注：程序从 $8000 起装入，最大 16K，main 返回后总是回到 DOS，返回值被忽略；返回 DOS 后 DOS 会清屏，所以显示后要先等待按键。
 *       只封装已在模拟器里验证过的系统调用；其他服务用 sb_syscall 加 sb2000-symbols.h 里的常量自行调用。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_H
#define SB2000_H

#include "sb2000-symbols.h"
#include "sb2000-system.h"
#include "sb2000-memory.h"
#include "sb2000-bank.h"
#include "sb2000-text.h"
#include "sb2000-input.h"
#include "sb2000-clock.h"
#include "sb2000-file.h"
#include "sb2000-disk.h"
#include "sb2000-display.h"
#include "sb2000-ime.h"
#include "sb2000-printer.h"
#include "sb2000-sound.h"
#include "sb2000-math.h"

/**
 * 读写一个固定地址的字节，如 SB_BYTE_AT(SB_KEY_ASCII)。
 *
 * @param address 字节的地址，可以用 sb2000-symbols.h 里的常量。
 */
#define SB_BYTE_AT(address) (*(volatile unsigned char *)(address))

/**
 * 调用系统服务，结果由调用者读各服务规定的变量。
 *
 * @param function 功能号，如 SB_FN_INPUT。
 * @param sub 子功能号，如 SB_SUB_KEY_GET_WAIT；没有子功能的服务可以写 0。
 */
void __fastcall__ sb_syscall(unsigned char function, unsigned char sub);

#endif
