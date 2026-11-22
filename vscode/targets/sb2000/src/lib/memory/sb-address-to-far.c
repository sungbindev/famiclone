/*
 * ------------------------------------------------------------------------
 * 名称：sb-address-to-far.c
 * 说明：sb_address_to_far 的实现：把窗口地址换算成远指针（页号与页内偏移）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $2C，页号 = 地址高 4 位 - 8 + 页组首页号（按 8 位回绕）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_address_to_far(unsigned char group, unsigned int address, unsigned char *page, unsigned int *offset)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    SBI_SET_ENTRY(address);
    sb_syscall(SB_FN_ADDR_TO_FAR, 0);
    *page = SB_BYTE_AT(SB_SYS_PAGE);
    *offset = SB_BYTE_AT(SB_FAR_OFFSET_LO) | ((unsigned int)SB_BYTE_AT(SB_FAR_OFFSET_HI) << 8);
}
