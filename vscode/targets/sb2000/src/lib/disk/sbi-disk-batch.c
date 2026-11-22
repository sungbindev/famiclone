/*
 * ------------------------------------------------------------------------
 * 名称：sbi-disk-batch.c
 * 说明：库内部函数：设置成批读写的参数并调用磁盘服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：页号、页内起始扇区、扇区数与方式写在 DISK_ 参数块里。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_disk_batch(unsigned char sub, unsigned char head, unsigned char track, unsigned char sector,
    unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode)
{
    sbi_disk_set_position(head, track, sector);
    SB_BYTE_AT(SB_DISK_PAGE) = page;
    SB_BYTE_AT(SB_DISK_PAGE_SECTOR) = page_sector;
    SB_BYTE_AT(SB_DISK_COUNT_LO) = (unsigned char)count;
    SB_BYTE_AT(SB_DISK_COUNT_HI) = (unsigned char)(count >> 8);
    SB_BYTE_AT(SB_DISK_MODE) = mode;
    return sbi_disk_call(sub);
}
