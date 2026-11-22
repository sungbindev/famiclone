/*
 * ------------------------------------------------------------------------
 * 名称：sb-clock-set-time.c
 * 说明：sb_clock_set_time 的实现：设置系统时间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7F 子功能 3，时间以 FAT 格式放在 FILE_TIME，合法性由系统检查。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_clock_set_time(const sb_time_t *time)
{
    sbi_time_to_fat(time);
    return sbi_dos_call(SB_FN_CLOCK, SB_SUB_CLOCK_SET_TIME);
}
