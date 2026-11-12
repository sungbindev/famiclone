/*
 * ------------------------------------------------------------------------
 * 名称：sb-ime-init.c
 * 说明：sb_ime_init 的实现：初始化输入法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $6D 子功能 0：参数放 $069A-$06A1。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ime_init(const sb_ime_config_t *config)
{
    SB_BYTE_AT(SB_ARG_069A) = config->row;
    SB_BYTE_AT(SB_ARG_069A + 1) = (unsigned char)config->tile;
    SB_BYTE_AT(SB_ARG_069A + 2) = (unsigned char)(config->tile >> 8);
    SB_BYTE_AT(SB_ARG_069A + 3) = config->subfunction;
    SB_BYTE_AT(SB_ARG_069A + 4) = config->color;
    SB_BYTE_AT(SB_ARG_069A + 5) = config->method;
    SB_BYTE_AT(SB_ARG_069A + 6) = config->allow;
    SB_BYTE_AT(SB_ARG_069A + 7) = config->flags;
    SB_BYTE_AT(SB_SYS_STATUS) = 0;
    sb_syscall(SB_FN_IME, 0);
    return SB_BYTE_AT(SB_SYS_STATUS);
}
