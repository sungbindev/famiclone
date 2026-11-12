/*
 * ------------------------------------------------------------------------
 * 名称：sb-speech-send-from.c
 * 说明：sb_speech_send_from 的实现：同步发送语音数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $75 子功能 0：语音控制标志 $032B 的位 0 为 0 时同步发送。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_speech_send_from(unsigned char group, unsigned int address)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SBI_SET_ENTRY(address);
    SB_BYTE_AT(SB_SPEECH_FLAGS) &= 0xFE;
    sb_syscall(SB_FN_SPEECH_SEND, 0);
}
