/*
 * ------------------------------------------------------------------------
 * 名称：sb-speech-send.c
 * 说明：sb_speech_send 的实现：同步发送程序窗口里的语音数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：页组取程序的起始页。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_speech_send(const void *data)
{
    sb_speech_send_from(SB_BYTE_AT(SB_PROGRAM_START_PAGE), (unsigned int)data);
}
