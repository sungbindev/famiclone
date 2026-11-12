/*
 * ------------------------------------------------------------------------
 * 名称：sb-clock-set-date.c
 * 说明：sb_clock_set_date 的实现：设置系统日期。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7F 子功能 1，日期以 FAT 格式放在 FILE_DATE，合法性由系统检查。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_clock_set_date(const sb_date_t *date)
{
    unsigned char error = sbi_date_to_fat(date);

    if (error != 0) {
        return error;
    }
    return sbi_dos_call(SB_FN_CLOCK, SB_SUB_CLOCK_SET_DATE);
}
