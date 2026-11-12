/*
 * ------------------------------------------------------------------------
 * 名称：sb-text-to-memory.c
 * 说明：sb_text_to_memory 的实现：把字符串画成点阵写进内存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $47、$48：字符串 $0C/$0D（页组取程序起始页），目的 $12/$13 与页组 $0643，结束符 $0653，容量 $0650:$064F，平面数 $0652（4 画二平面，其他画四平面），颜色 $064E。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned int sb_text_to_memory(const char *text, unsigned char terminator, unsigned char group, unsigned int address, unsigned int capacity, unsigned char four_planes, unsigned char color, unsigned char ascii_only)
{
    SB_BYTE_AT(SB_DEST_ADDR_LO) = (unsigned char)address;
    SB_BYTE_AT(SB_DEST_ADDR_HI) = (unsigned char)(address >> 8);
    SB_BYTE_AT(SB_SYS_GROUP_2) = group;
    SB_BYTE_AT(SB_PARAM_0653) = terminator;
    SB_BYTE_AT(SB_PARAM_0652) = four_planes ? 8 : 4;
    SB_BYTE_AT(SB_PARAM_064E) = color;
    SB_BYTE_AT(SB_PARAM_064F) = (unsigned char)capacity;
    SB_BYTE_AT(SB_PARAM_0650) = (unsigned char)(capacity >> 8);
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)text);
    sb_syscall(ascii_only ? SB_FN_TEXT_BUFFER_ASCII : SB_FN_TEXT_BUFFER, 0);
    return SBI_GET16(SB_PARAM_064F);
}
