/*
 * ------------------------------------------------------------------------
 * 名称：sb-clock-get-date.c
 * 说明：sb_clock_get_date 的实现：取系统日期。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7F 子功能 2，日期以 FAT 格式放在 FILE_DATE。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_clock_get_date(sb_date_t *date)
{
    sb_syscall(SB_FN_CLOCK, SB_SUB_CLOCK_GET_DATE);
    sbi_date_from_fat(date);
}
