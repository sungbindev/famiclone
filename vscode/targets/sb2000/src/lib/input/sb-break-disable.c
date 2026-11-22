/*
 * ------------------------------------------------------------------------
 * 名称：sb-break-disable.c
 * 说明：sb_break_disable 的实现：关闭按 ESC 中断程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6B 子功能 2，把 $063D 清 0。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_break_disable(void)
{
    sb_syscall(SB_FN_INPUT, SB_SUB_KEY_ESC_OFF);
}
