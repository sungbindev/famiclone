/*
 * ------------------------------------------------------------------------
 * 名称：sbi-date-to-fat.c
 * 说明：库内部函数：把日期写成 FAT 格式放进 FILE_DATE。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：FAT 日期：位 0-4 日，位 5-8 月，位 9-15 年（自 1980）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_date_to_fat(const sb_date_t *date)
{
    unsigned int value;

    if (date->year < 1980) {
        return SB_ERR_BAD_PARAMETER;
    }
    value = ((date->year - 1980) << 9) | ((unsigned int)date->month << 5) | date->day;
    SB_BYTE_AT(SB_FILE_DATE_LO) = (unsigned char)value;
    SB_BYTE_AT(SB_FILE_DATE_HI) = (unsigned char)(value >> 8);
    return 0;
}
