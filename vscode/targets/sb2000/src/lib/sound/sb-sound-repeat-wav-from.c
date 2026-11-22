/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-repeat-wav-from.c
 * 说明：sb_sound_repeat_wav_from 的实现：重复播放 WAV。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $65 子功能 $0D：重复次数写 $0652。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_sound_repeat_wav_from(unsigned char group, unsigned int address, unsigned char times)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SBI_SET_ENTRY(address);
    SB_BYTE_AT(SB_PARAM_0652) = times;
    sb_syscall(SB_FN_SOUND, SB_SUB_SOUND_WAV_LOOP);
}
