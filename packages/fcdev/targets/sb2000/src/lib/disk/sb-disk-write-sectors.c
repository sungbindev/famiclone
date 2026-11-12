/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-write-sectors.c
 * 说明：sb_disk_write_sectors 的实现：连续写多个扇区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 5。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_disk_write_sectors(unsigned char head, unsigned char track, unsigned char sector,
    unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode)
{
    return sbi_disk_batch(SB_SUB_DISK_WRITE, head, track, sector, page, page_sector, count, mode);
}
