/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-rename.c
 * 说明：sb_file_rename 的实现：给文件改名。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 4 要求句柄以方式 2 打开，新名称放 PATH_BUFFER。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_rename(const char *path, const char *new_name)
{
    unsigned char handle;
    unsigned char error;
    unsigned char close_error;

    error = sb_file_open(path, SB_MODE_UPDATE, &handle);
    if (error != 0) {
        return error;
    }
    error = sbi_set_path(new_name);
    if (error == 0) {
        sbi_select_handle(handle);
        error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_RENAME);
    }
    close_error = sb_file_close(handle);
    return error != 0 ? error : close_error;
}
