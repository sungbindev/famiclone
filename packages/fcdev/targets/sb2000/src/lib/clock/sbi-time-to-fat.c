/*
 * ------------------------------------------------------------------------
 * 名称：sbi-time-to-fat.c
 * 说明：库内部函数：把时间写成 FAT 格式放进 FILE_TIME。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：FAT 时间：位 0-4 秒的一半，位 5-10 分，位 11-15 时。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sbi_time_to_fat(const sb_time_t *time)
{
    unsigned int value = ((unsigned int)time->hour << 11) | ((unsigned int)time->minute << 5) | (time->second >> 1);

    SB_BYTE_AT(SB_FILE_TIME_LO) = (unsigned char)value;
    SB_BYTE_AT(SB_FILE_TIME_HI) = (unsigned char)(value >> 8);
}
