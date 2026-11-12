/*
 * ------------------------------------------------------------------------
 * 名称：sb-speech-send-entry-from.c
 * 说明：sb_speech_send_entry_from 的实现：按序号发送词条。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $75 子功能 1：序号高字节写 $064A、低字节写 $064B。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_speech_send_entry_from(unsigned char group, unsigned int table, unsigned int index)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SBI_SET_ENTRY(table);
    SB_BYTE_AT(SB_PARAM_064A) = (unsigned char)(index >> 8);
    SB_BYTE_AT(SB_PARAM_064B) = (unsigned char)index;
    SB_BYTE_AT(SB_SPEECH_FLAGS) &= 0xFE;
    sb_syscall(SB_FN_SPEECH_SEND, 1);
}
