/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-open.c
 * 说明：sb_file_open 的实现：打开文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7B 子功能 9，路径放 PATH_BUFFER，方式放 FILE_MODE，成功后句柄偏移在 FILE_HANDLE_OFFSET。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_open(const char *path, unsigned char mode, unsigned char *handle)
{
    unsigned char error = sbi_set_path(path);

    if (error != 0) {
        return error;
    }
    SB_BYTE_AT(SB_FILE_MODE) = mode;
    error = sbi_dos_call(SB_FN_FILE, SB_SUB_FS_OPEN);
    if (error == 0) {
        *handle = SB_BYTE_AT(SB_FILE_HANDLE_OFFSET) / SBI_HANDLE_SIZE;
    }
    return error;
}
