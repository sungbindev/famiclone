/*
 * ------------------------------------------------------------------------
 * 名称：sb-timer-start.c
 * 说明：sb_timer_start 的实现：设置并启动定时器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：顺序与 BIOS 一致：先控制字节，再预置值。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_timer_start(unsigned char control, unsigned char preset)
{
    SB_BYTE_AT(SB_HW_TIMER_CONTROL) = control;
    SB_BYTE_AT(SB_HW_TIMER_PRESET) = preset;
}
