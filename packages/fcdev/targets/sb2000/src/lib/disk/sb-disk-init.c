/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-init.c
 * 说明：sb_disk_init 的实现：启动软驱并复位到 0 磁道。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $80 子功能 0。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_disk_init(void)
{
    return sbi_disk_call(SB_SUB_DISK_INIT);
}
