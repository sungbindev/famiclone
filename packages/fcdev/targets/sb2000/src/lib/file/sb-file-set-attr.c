/*
 * ------------------------------------------------------------------------
 * 名称：sb-file-set-attr.c
 * 说明：sb_file_set_attr 的实现：设置已打开文件的属性。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7C 子功能 $0B，属性放在 FILE_ATTR，合法性由系统检查。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_file_set_attr(unsigned char handle, unsigned char attr)
{
    unsigned char error = sbi_select_handle(handle);

    if (error != 0) {
        return error;
    }
    SB_BYTE_AT(SB_FILE_ATTR) = attr;
    return sbi_dos_call(SB_FN_FILE_HANDLE, SB_SUB_FH_SET_ATTR);
}
