/*
 * ------------------------------------------------------------------------
 * 名称：sb-mouse-buttons.c
 * 说明：sb_mouse_buttons 的实现：取两个鼠标键的状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6C 子功能 3，结果在 $0360-$0367。子功能 3 返回前会拿残留的寄存器当子功能号继续比较，
 *       若恰好是 4 就会把 $0652、$0653 当作坐标写入，所以调用前先把它们设成当前坐标，使这种情况下坐标不变。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "../sb-internal.h"

void sb_mouse_buttons(sb_mouse_buttons_t *buttons)
{
    SB_BYTE_AT(SB_PARAM_0652) = SB_BYTE_AT(SB_MOUSE_X);
    SB_BYTE_AT(SB_PARAM_0653) = SB_BYTE_AT(SB_MOUSE_Y);
    sb_syscall(SB_FN_MOUSE, SB_SUB_MOUSE_GET_BUTTONS);
    memcpy(buttons, (const void *)SB_MOUSE_BTN_BASE, sizeof(sb_mouse_buttons_t));
}
