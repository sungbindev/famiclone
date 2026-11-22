/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-format-track.c
 * 说明：sb_disk_format_track 的实现：格式化一个磁道。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_disk_format_track(unsigned char head, unsigned char track)
{
    SB_BYTE_AT(SB_DISK_HEAD) = head;
    SB_BYTE_AT(SB_DISK_TRACK) = track;
    return sbi_disk_call(SB_SUB_DISK_FORMAT);
}
