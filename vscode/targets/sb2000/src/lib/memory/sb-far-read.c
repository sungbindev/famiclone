/*
 * ------------------------------------------------------------------------
 * 名称：sb-far-read.c
 * 说明：sb_far_read 的实现：把扩展 RAM 里的一段数据读到内存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：每次经内核的跨页复制入口读最多 255 字节到 $0700 缓冲，再复制到目的地；页组映射 4 页，所以一次读不会越出页组。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "../sb-internal.h"

void sb_far_read(unsigned char page, unsigned int offset, void *dest, unsigned int length)
{
    unsigned char *out = (unsigned char *)dest;
    unsigned char count;
    unsigned int address;

    page += (unsigned char)(offset >> 12);
    offset &= 0x0FFF;
    while (length != 0) {
        count = length > 255 ? 255 : (unsigned char)length;
        address = 0x8000 + offset;
        SB_BYTE_AT(SB_SYS_GROUP) = page;
        SBI_SET_ENTRY(address);
        SB_BYTE_AT(SB_PARAM_064F) = count;
        sbi_far_copy_in();
        memcpy(out, (const void *)SB_COPY_BUFFER, count);
        out += count;
        length -= count;
        offset += count;
        page += (unsigned char)(offset >> 12);
        offset &= 0x0FFF;
    }
}
