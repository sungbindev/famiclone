/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-stop.c
 * 说明：sb_sound_stop 的实现：停止播放。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $65 子功能 2。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_sound_stop(void)
{
    sb_syscall(SB_FN_SOUND, SB_SUB_SOUND_STOP);
}
