/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-write.c
 * 说明：sb_file_write 的实现：改写文件里已有的一段数据。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 6 从扩展 RAM 页写出，数据先用 sb_far_write 放进借用的页。实测的行为：
 *       ① 每次写完，系统把文件大小改成“偏移加长度”（可能把文件截短，也可能放大）；② 偏移不小于当前文件大小时出错 $14；
 *       ③ 一页最多装 8 个扇区，起始偏移在扇区内的位置也占用这个空间。
 *       所以这里只允许改写已有的范围，每块写完都把句柄里的文件大小恢复成原来的值，再写下一块，每块最多写 4096 减去该位置个字节。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_write(unsigned char handle, unsigned long offset, const void *src, unsigned int length)
{
    const unsigned char *in = (const unsigned char *)src;
    unsigned char page;
    unsigned char error;
    unsigned long old_size;
    unsigned long chunk;
    unsigned int size_cell;

    if (length == 0) {
        return 0;
    }
    error = sb_file_size(handle, &old_size);
    if (error != 0) {
        return error;
    }
    if (offset + length > old_size) {
        return 0x14;
    }
    error = sb_page_alloc(1, &page);
    if (error != 0) {
        return error;
    }
    size_cell = SB_FILE_HANDLES + handle * SBI_HANDLE_SIZE + SB_FH_SIZE_OFFSET;
    while (length != 0 && error == 0) {
        chunk = 4096 - (unsigned int)(offset & 511);
        if (chunk > length) {
            chunk = length;
        }
        sb_far_write(page, 0, in, (unsigned int)chunk);
        sbi_select_handle(handle);
        SBI_SET24(SB_FILE_OFFSET_0, offset);
        SBI_SET24(SB_FILE_SIZE_0, chunk);
        SB_BYTE_AT(SB_FILE_START_PAGE) = page;
        error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_WRITE_AT);
        SBI_SET24(size_cell, old_size);
        if (error == 0) {
            in += chunk;
            offset += chunk;
            length -= (unsigned int)chunk;
        }
    }
    sb_page_free(page, 1);
    return error;
}
