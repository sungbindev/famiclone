/*
 * ------------------------------------------------------------------------
 * 名称：sbi-select-handle.c
 * 说明：库内部函数：选定后续句柄服务作用的句柄。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：句柄服务作用在 FILE_HANDLE_OFFSET 指定的句柄上，偏移是句柄号乘 $2C。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_select_handle(unsigned char handle)
{
    if (handle > 3) {
        return SB_ERR_BAD_PARAMETER;
    }
    SB_BYTE_AT(SB_FILE_HANDLE_OFFSET) = (unsigned char)(handle * SBI_HANDLE_SIZE);
    return 0;
}
