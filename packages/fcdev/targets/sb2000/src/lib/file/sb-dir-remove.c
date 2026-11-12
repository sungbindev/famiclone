/*
 * ------------------------------------------------------------------------
 * 名称：sb-dir-remove.c
 * 说明：sb_dir_remove 的实现：删除空的子目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7D 子功能 3，路径放 PATH_BUFFER。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_dir_remove(const char *path)
{
    unsigned char error = sbi_set_path(path);

    if (error != 0) {
        return error;
    }
    return sbi_dos_call(SB_FN_DIR, SB_SUB_DIR_DELETE);
}
