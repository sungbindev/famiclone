/*
 * ------------------------------------------------------------------------
 * 名称：sb-dir-change.c
 * 说明：sb_dir_change 的实现：设置当前目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7B 子功能 $0B，路径放 PATH_BUFFER。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_dir_change(const char *path)
{
    unsigned char error = sbi_set_path(path);

    if (error != 0) {
        return error;
    }
    return sbi_dos_call(SB_FN_FILE, SB_SUB_FS_FIND_ENTRY);
}
