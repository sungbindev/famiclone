/*
 * ------------------------------------------------------------------------
 * 名称：sb-break-enable.c
 * 说明：sb_break_enable 的实现：打开按 ESC 中断程序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6B 子功能 1，把 $063D 置 $FF。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_break_enable(void)
{
    sb_syscall(SB_FN_INPUT, SB_SUB_KEY_ESC_ON);
}
