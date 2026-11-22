/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-load.c
 * 说明：sb_file_load 的实现：把整个文件读入新分配的扩展 RAM 页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 $0F 要求方式 0，页由调用者先分配，起始页号写 FILE_START_PAGE。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_load(const char *path, unsigned char *first_page, unsigned long *size)
{
    unsigned char handle;
    unsigned char error;
    unsigned char close_error;
    unsigned char page;
    unsigned char pages;
    unsigned long total;

    *first_page = 0;
    *size = 0;
    error = sb_file_open(path, SB_MODE_OPEN, &handle);
    if (error != 0) {
        return error;
    }
    error = sb_file_size(handle, &total);
    if (error == 0 && total != 0) {
        if (total > 255UL * 4096UL) {
            error = SB_ERR_NO_MEMORY;
        } else {
            pages = (unsigned char)((total + 4095UL) >> 12);
            error = sb_page_alloc(pages, &page);
            if (error == 0) {
                sbi_select_handle(handle);
                SB_BYTE_AT(SB_FILE_START_PAGE) = page;
                error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_READ_FILE);
                if (error != 0) {
                    sb_page_free(page, pages);
                } else {
                    *first_page = page;
                    *size = total;
                }
            }
        }
    }
    close_error = sb_file_close(handle);
    return error != 0 ? error : close_error;
}
