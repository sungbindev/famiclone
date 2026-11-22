/*
 * ------------------------------------------------------------------------
 * 名称：sbi-disk-call.c
 * 说明：库内部函数：调用磁盘服务并返回软驱状态码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80；服务开头先把 DISK_STATUS 清 0，结果状态在 DISK_STATUS。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_disk_call(unsigned char sub)
{
    sb_syscall(SB_FN_DISK, sub);
    return SB_BYTE_AT(SB_DISK_STATUS);
}
