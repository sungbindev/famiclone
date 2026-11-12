/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-set-date.c
 * 说明：sb_file_set_date 的实现：设置已打开文件的修改日期。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 7，日期以 FAT 格式放在 FILE_DATE，合法性由系统检查。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_set_date(unsigned char handle, const sb_date_t *date)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    error = sbi_date_to_fat(date);
    if (error != 0) {
        return error;
    }
    return sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_SET_DATE);
}
