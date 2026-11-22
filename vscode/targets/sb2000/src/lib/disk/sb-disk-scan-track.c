/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-scan-track.c
 * 说明：sb_disk_scan_track 的实现：扫描一个磁道的坏扇区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 8，结果位图在 DISK_BAD_MAP 起的 3 字节。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_disk_scan_track(unsigned char head, unsigned char track, unsigned char *bad_map)
{
    unsigned char status;
    unsigned char index;

    SB_BYTE_AT(SB_DISK_HEAD) = head;
    SB_BYTE_AT(SB_DISK_TRACK) = track;
    status = sbi_disk_call(SB_SUB_DISK_SCAN);
    if (status == 0) {
        for (index = 0; index < 3; ++index) {
            bad_map[index] = SB_BYTE_AT(SB_DISK_BAD_MAP + index);
        }
    }
    return status;
}
