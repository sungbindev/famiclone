/*
 * ------------------------------------------------------------------------
 * 名称：sbi-date-from-fat.c
 * 说明：库内部函数：把 FILE_DATE 里的 FAT 日期取成年月日。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：FAT 日期：位 0-4 日，位 5-8 月，位 9-15 年（自 1980）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sbi_date_from_fat(sb_date_t *date)
{
    unsigned int value = SB_BYTE_AT(SB_FILE_DATE_LO) | ((unsigned int)SB_BYTE_AT(SB_FILE_DATE_HI) << 8);

    date->year = 1980 + (value >> 9);
    date->month = (unsigned char)((value >> 5) & 0x0F);
    date->day = (unsigned char)(value & 0x1F);
}
