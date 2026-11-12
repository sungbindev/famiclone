/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-size.c
 * 说明：sb_file_size 的实现：取文件大小。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 $0D，结果在 FILE_SIZE（3 字节，低字节在前）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_size(unsigned char handle, unsigned long *size)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_GET_SIZE);
    if (error == 0) {
        *size = (unsigned long)SB_BYTE_AT(SB_FILE_SIZE_0)
            | ((unsigned long)SB_BYTE_AT(SB_FILE_SIZE_1) << 8)
            | ((unsigned long)SB_BYTE_AT(SB_FILE_SIZE_2) << 16);
    }
    return error;
}
