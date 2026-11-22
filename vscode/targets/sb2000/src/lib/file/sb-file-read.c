/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-read.c
 * 说明：sb_file_read 的实现：从文件的指定位置读数据到内存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 5 把数据读进扩展 RAM 页，再用 sb_far_read 取出。
 *       系统从起始扇区的开头读起，一页最多装 8 个扇区，起始偏移在扇区内的位置也占用这个空间（实测超出部分读不到），
 *       所以每次最多读 4096 减去该位置个字节，借用的页读完释放。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_read(unsigned char handle, unsigned long offset, void *dest, unsigned int length)
{
    unsigned char *out = (unsigned char *)dest;
    unsigned char page;
    unsigned char error;
    unsigned long chunk;

    if (length == 0) {
        return 0;
    }
    error = sbi_select_handle(handle);
    if (error != 0) {
        return error;
    }
    error = sb_page_alloc(1, &page);
    if (error != 0) {
        return error;
    }
    while (length != 0 && error == 0) {
        chunk = 4096 - (unsigned int)(offset & 511);
        if (chunk > length) {
            chunk = length;
        }
        sbi_select_handle(handle);
        SBI_SET24(SB_FILE_OFFSET_0, offset);
        SBI_SET24(SB_FILE_SIZE_0, chunk);
        SB_BYTE_AT(SB_FILE_START_PAGE) = page;
        error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_READ_AT);
        if (error == 0) {
            sb_far_read(page, 0, out, (unsigned int)chunk);
            out += chunk;
            offset += chunk;
            length -= (unsigned int)chunk;
        }
    }
    sb_page_free(page, 1);
    return error;
}
