/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-start.c
 * 说明：sb_disk_start 的实现：启动软驱。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 6。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_disk_start(void)
{
    return sbi_disk_call(SB_SUB_DISK_START);
}
