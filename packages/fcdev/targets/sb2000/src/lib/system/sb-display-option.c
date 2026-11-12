/*
 * ------------------------------------------------------------------------
 * 名称：sb-display-option.c
 * 说明：sb_display_option 的实现：把值按位并入显示相关的硬件寄存器。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $10，值写在 $0652；选项超出 0-7 时系统把状态 $0640 置 $FF。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

unsigned char sb_display_option(unsigned char option, unsigned char value)
{
    SB_BYTE_AT(SB_PARAM_0652) = value;
    sb_syscall(SB_FN_VIDEO_OPTION, option);
    return SB_BYTE_AT(SB_SYS_STATUS);
}
