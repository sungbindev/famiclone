/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-close.c
 * 说明：sb_file_close 的实现：关闭文件并让软驱停转。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 2。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_close(unsigned char handle)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    return sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_CLOSE_FLUSH);
}
