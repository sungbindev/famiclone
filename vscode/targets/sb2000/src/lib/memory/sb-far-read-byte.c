/*
 * ------------------------------------------------------------------------
 * 名称：sb-far-read-byte.c
 * 说明：sb_far_read_byte 的实现：读扩展 RAM 页里的一个字节。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：偏移超过一页（$0FFF）时折算到后面的页。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_far_read_byte(unsigned char page, unsigned int offset)
{
    SBI_SET_FAR(page + (unsigned char)(offset >> 12), offset & 0x0FFF);
    return sbi_far_read_byte();
}
