/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-write-all.c
 * 说明：sb_file_write_all 的实现：把内存里的一段数据写成一个新文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：先把数据放进扩展 RAM 页，再用 sb_file_save 整个写出。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_write_all(const char *path, const void *data, unsigned int length)
{
    unsigned char page = 0;
    unsigned char pages;
    unsigned char error;

    if (length != 0) {
        pages = (unsigned char)(((unsigned long)length + 4095UL) >> 12);
        error = sb_page_alloc(pages, &page);
        if (error != 0) {
            return error;
        }
        sb_far_write(page, 0, data, length);
    }
    error = sb_file_save(path, page, length);
    if (length != 0) {
        sb_page_free(page, pages);
    }
    return error;
}
