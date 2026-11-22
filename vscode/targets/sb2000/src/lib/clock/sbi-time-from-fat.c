/*
 * ------------------------------------------------------------------------
 * 名称：sbi-time-from-fat.c
 * 说明：库内部函数：把 FILE_TIME 里的 FAT 时间取成时分秒。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：FAT 时间：位 0-4 秒的一半，位 5-10 分，位 11-15 时。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sbi_time_from_fat(sb_time_t *time)
{
    unsigned int value = SB_BYTE_AT(SB_FILE_TIME_LO) | ((unsigned int)SB_BYTE_AT(SB_FILE_TIME_HI) << 8);

    time->hour = (unsigned char)(value >> 11);
    time->minute = (unsigned char)((value >> 5) & 0x3F);
    time->second = (unsigned char)((value & 0x1F) << 1);
}
