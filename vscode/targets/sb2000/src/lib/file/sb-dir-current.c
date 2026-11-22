/*
 * ------------------------------------------------------------------------
 * 名称：sb-dir-current.c
 * 说明：sb_dir_current 的实现：取当前目录自己的名称。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7D 子功能 1 把名称整理成 \NAME.EXT 放进 PATH_BUFFER，这里复制出来。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_dir_current(char *name)
{
    unsigned char error = sbi_dos_call(SB_FN_DIR, SB_SUB_DIR_FORMAT_NAME);
    unsigned char index = 0;

    if (error != 0) {
        return error;
    }
    while (index < 15 && SB_BYTE_AT(SB_PATH_BUFFER + index) != 0) {
        name[index] = (char)SB_BYTE_AT(SB_PATH_BUFFER + index);
        ++index;
    }
    name[index] = 0;
    return 0;
}
