/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-set-time.c
 * 说明：sb_file_set_time 的实现：设置已打开文件的修改时间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 9，时间以 FAT 格式放在 FILE_TIME，合法性由系统检查。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_set_time(unsigned char handle, const sb_time_t *time)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    sbi_time_to_fat(time);
    return sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_SET_TIME);
}
