/*
 * ------------------------------------------------------------------------
 * 名称：sbi-dos-call.c
 * 说明：库内部函数：调用系统服务并返回 DOS 出错码。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：文件、目录、内存页等服务出错时把出错码写在 $6300，0 表示成功。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_dos_call(unsigned char function, unsigned char sub)
{
    sb_syscall(function, sub);
    return SB_BYTE_AT(SB_DOS_ERROR);
}
