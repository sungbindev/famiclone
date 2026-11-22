/*
 * ------------------------------------------------------------------------
 * 名称：sb-pad-state.c
 * 说明：sb_pad_state 的实现：取一个手柄的原始按键状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：读 $034E（1 号手柄）、$034F（2 号手柄），它们由 sb_pad_poll、sb_pad_wait 更新。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_pad_state(unsigned char pad)
{
    return pad == 0 ? SB_BYTE_AT(SB_PAD1_BITS) : SB_BYTE_AT(SB_PAD2_BITS);
}
