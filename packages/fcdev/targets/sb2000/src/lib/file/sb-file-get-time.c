/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-get-time.c
 * 说明：sb_file_get_time 的实现：取已打开文件的修改时间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 $0A，时间以 FAT 格式放在 FILE_TIME。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_get_time(unsigned char handle, sb_time_t *time)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    error = sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_GET_TIME);
    if (error == 0) {
        sbi_time_from_fat(time);
    }
    return error;
}
