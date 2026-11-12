/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-set-width.c
 * 说明：sb_ime_set_width 的实现：设置半角或全角。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 5（半角）与 6（全角）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_ime_set_width(unsigned char full)
{
    sb_syscall(SB_FN_IME, full ? 6 : 5);
}
