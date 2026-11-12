/*
 * ------------------------------------------------------------------------
 * 名称：sbi-peek.c
 * 说明：库内部函数：把固定地址的一段字节读给调用者。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：用来从系统服务的参数区取结果。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sbi_peek(unsigned char *dest, unsigned int address, unsigned char count)
{
    unsigned char index;

    for (index = 0; index < count; ++index) {
        dest[index] = SB_BYTE_AT(address + index);
    }
}
