/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-write-sector.c
 * 说明：sb_disk_write_sector 的实现：写一个扇区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 3，先把数据复制到 ERAM 的扇区缓冲 DISK_BUFFER（$6000）再写盘。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "../sb-internal.h"

unsigned char sb_disk_write_sector(unsigned char head, unsigned char track, unsigned char sector, const void *src)
{
    memcpy((void *)SB_DISK_BUFFER, src, SB_DISK_SECTOR_SIZE);
    sbi_disk_set_position(head, track, sector);
    return sbi_disk_call(SB_SUB_DISK_WRITE_ONE);
}
