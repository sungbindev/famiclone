/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-unpack-to-memory.c
 * 说明：sb_image_unpack_to_memory 的实现：解压到内存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $61：流 $0C/$0D（页组取程序起始页），目的地址 $12/$13，目的页组 $0643，解出的字节数返回在 $064F/$0650。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned int sb_image_unpack_to_memory(const void *stream, unsigned char group, unsigned int address)
{
    SB_BYTE_AT(SB_DEST_ADDR_LO) = (unsigned char)address;
    SB_BYTE_AT(SB_DEST_ADDR_HI) = (unsigned char)(address >> 8);
    SB_BYTE_AT(SB_SYS_GROUP_2) = group;
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)stream);
    sb_syscall(SB_FN_UNPACK_MEMORY, 0);
    return SBI_GET16(SB_PARAM_064F);
}
