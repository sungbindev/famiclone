/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-save.c
 * 说明：sb_file_save 的实现：把扩展 RAM 页里的数据整个写成一个文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 $10 要求方式 1，长度写 FILE_SIZE、起始页号写 FILE_START_PAGE；这两个单元要在打开之后才写，否则会被打开服务改掉。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_save(const char *path, unsigned char first_page, unsigned long size)
{
    unsigned char handle;
    unsigned char error;
    unsigned char close_error;

    error = sb_file_open(path, SB_MODE_CREATE, &handle);
    if (error != 0) {
        return error;
    }
    if (size != 0) {
        sbi_select_handle(handle);
        SBI_SET24(SB_FILE_SIZE_0, size);
        SB_BYTE_AT(SB_FILE_START_PAGE) = first_page;
        error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_WRITE_FILE);
    }
    close_error = sb_file_close(handle);
    return error != 0 ? error : close_error;
}
