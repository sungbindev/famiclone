/*
 * ------------------------------------------------------------------------
 * 名称：sb-clock-get-time.c
 * 说明：sb_clock_get_time 的实现：取系统时间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7F 子功能 4，时间以 FAT 格式放在 FILE_TIME。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_clock_get_time(sb_time_t *time)
{
    sb_syscall(SB_FN_CLOCK, SB_SUB_CLOCK_GET_TIME);
    sbi_time_from_fat(time);
}
