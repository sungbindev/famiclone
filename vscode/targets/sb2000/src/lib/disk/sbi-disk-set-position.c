/*
 * ------------------------------------------------------------------------
 * 名称：sbi-disk-set-position.c
 * 说明：库内部函数：设置磁盘服务的起始面、磁道、扇区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：参数块在 DISK_HEAD 起的 ERAM 单元。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sbi_disk_set_position(unsigned char head, unsigned char track, unsigned char sector)
{
    SB_BYTE_AT(SB_DISK_HEAD) = head;
    SB_BYTE_AT(SB_DISK_TRACK) = track;
    SB_BYTE_AT(SB_DISK_SECTOR) = sector;
}
