/*
 * ------------------------------------------------------------------------
 * 名称：sb-sound-transpose.c
 * 说明：sb_sound_transpose 的实现：调整声道移调。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $65 子功能 4：把 $0653 加到 $0447-$044A。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_sound_transpose(unsigned char delta)
{
    SB_BYTE_AT(SB_PARAM_0653) = delta;
    sb_syscall(SB_FN_SOUND, SB_SUB_SOUND_TRANSPOSE);
}
