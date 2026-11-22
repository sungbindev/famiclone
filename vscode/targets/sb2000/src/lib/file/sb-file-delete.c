/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-delete.c
 * 说明：sb_file_delete 的实现：删除文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 3，以方式 3 打开再关闭，路径放 PATH_BUFFER。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_delete(const char *path)
{
    unsigned char error = sbi_set_path(path);

    if (error != 0) {
        return error;
    }
    return sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_DELETE);
}
