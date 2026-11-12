/*
 * ------------------------------------------------------------------------
 * 名称：sbi-set-path.c
 * 说明：库内部函数：把路径字符串复制到系统的路径缓冲。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：PATH_BUFFER 从 $6543 到 $655F 共 29 个字节，含结尾的 0，所以路径最长 28 个字符。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_set_path(const char *path)
{
    unsigned char index = 0;

    while (path[index] != 0) {
        if (index >= 28) {
            return SB_ERR_BAD_NAME;
        }
        SB_BYTE_AT(SB_PATH_BUFFER + index) = (unsigned char)path[index];
        ++index;
    }
    SB_BYTE_AT(SB_PATH_BUFFER + index) = 0;
    return 0;
}
