/*
 * ------------------------------------------------------------------------
 * 名称：sb-far-write.c
 * 说明：sb_far_write 的实现：把内存里的一段数据写到扩展 RAM。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：每次先复制最多 255 字节到 $0700 缓冲，再经内核的跨页复制入口写出。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "../sb-internal.h"

void sb_far_write(unsigned char page, unsigned int offset, const void *src, unsigned int length)
{
    const unsigned char *in = (const unsigned char *)src;
    unsigned char count;
    unsigned int address;

    page += (unsigned char)(offset >> 12);
    offset &= 0x0FFF;
    while (length != 0) {
        count = length > 255 ? 255 : (unsigned char)length;
        address = 0x8000 + offset;
        memcpy((void *)SB_COPY_BUFFER, in, count);
        SB_BYTE_AT(SB_SYS_GROUP) = page;
        SBI_SET_ENTRY(address);
        SB_BYTE_AT(SB_PARAM_064F) = count;
        sbi_far_copy_out();
        in += count;
        length -= count;
        offset += count;
        page += (unsigned char)(offset >> 12);
        offset &= 0x0FFF;
    }
}
