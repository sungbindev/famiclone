/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-play-wav-from.c
 * 说明：sb_sound_play_wav_from 的实现：播放页组里的 WAV。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $65 子功能 $0C。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_sound_play_wav_from(unsigned char group, unsigned int address)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SBI_SET_ENTRY(address);
    sb_syscall(SB_FN_SOUND, SB_SUB_SOUND_WAV);
}
