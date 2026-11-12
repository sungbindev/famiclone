/*
 * ------------------------------------------------------------------------
 * 名称：sbi-poke.c
 * 说明：库内部函数：把调用者的一段字节写到固定地址。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：用来把参数放进系统服务的参数区。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sbi_poke(unsigned int address, const unsigned char *source, unsigned char count)
{
    unsigned char index;

    for (index = 0; index < count; ++index) {
        SB_BYTE_AT(address + index) = source[index];
    }
}
