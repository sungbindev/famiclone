/*
 * ------------------------------------------------------------------------
 * 名称：sb-palette-write.c
 * 说明：sb_palette_write 的实现：写调色板。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $5D：偏移 $0647，个数 $064F，数据 $0C/$0D（页组取程序起始页）。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_palette_write(unsigned char offset, const unsigned char *colors, unsigned char count)
{
    SB_BYTE_AT(SB_PARAM_0647) = offset;
    SB_BYTE_AT(SB_PARAM_064F) = count;
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)colors);
    sb_syscall(SB_FN_PALETTE, 0);
}
