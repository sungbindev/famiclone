/*
 * ------------------------------------------------------------------------
 * 名称：sb-keyboard-reset.c
 * 说明：sb_keyboard_reset 的实现：复位并识别键盘。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $17，结果在 $0635。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_keyboard_reset(void)
{
    sb_syscall(SB_FN_KEYBOARD_INIT, 0);
    return SB_BYTE_AT(SB_KBD_SELF_TEST_RESULT);
}
