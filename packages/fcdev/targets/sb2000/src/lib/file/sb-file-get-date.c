/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-get-date.c
 * 说明：sb_file_get_date 的实现：取已打开文件的修改日期。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 8，日期以 FAT 格式放在 FILE_DATE。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_get_date(unsigned char handle, sb_date_t *date)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_GET_DATE);
    if (error == 0) {
        sbi_date_from_fat(date);
    }
    return error;
}
