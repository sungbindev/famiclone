/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-play-wav.c
 * 说明：sb_sound_play_wav 的实现：播放程序窗口里的 WAV。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：页组取程序的起始页。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_sound_play_wav(const void *wav)
{
    sb_sound_play_wav_from(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)wav);
}
