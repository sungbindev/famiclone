/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-status.c
 * 说明：sb_sound_status 的实现：取播放状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $65 子功能 3：结果取到 $0653。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_sound_status(void)
{
    sb_syscall(SB_FN_SOUND, SB_SUB_SOUND_STATUS);
    return SB_BYTE_AT(SB_PARAM_0653);
}
