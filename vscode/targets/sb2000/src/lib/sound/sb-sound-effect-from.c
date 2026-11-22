/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-effect-from.c
 * 说明：sb_sound_effect_from 的实现：播放音效曲子。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $65 子功能 $0B：曲目号 $0653 写 $FF，表示直接播放页组与窗口地址指向的曲子。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_sound_effect_from(unsigned char group, unsigned int address)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SBI_SET_ENTRY(address);
    SB_BYTE_AT(SB_PARAM_0653) = 0xFF;
    sb_syscall(SB_FN_SOUND, SB_SUB_SOUND_EFFECT);
}
