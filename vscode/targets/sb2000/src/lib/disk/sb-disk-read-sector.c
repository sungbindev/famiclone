/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-read-sector.c
 * 说明：sb_disk_read_sector 的实现：读一个扇区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 2，扇区先读到 ERAM 的扇区缓冲 DISK_BUFFER（$6000，任何窗口映射下都可见），再复制给调用者。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "../sb-internal.h"

unsigned char sb_disk_read_sector(unsigned char head, unsigned char track, unsigned char sector, void *dest)
{
    unsigned char status;

    sbi_disk_set_position(head, track, sector);
    status = sbi_disk_call(SB_SUB_DISK_READ_ONE);
    if (status == 0) {
        memcpy(dest, (const void *)SB_DISK_BUFFER, SB_DISK_SECTOR_SIZE);
    }
    return status;
}
