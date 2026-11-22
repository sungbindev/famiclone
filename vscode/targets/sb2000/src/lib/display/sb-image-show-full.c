/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-show-full.c
 * 说明：sb_image_show_full 的实现：显示整屏图片。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $5E：图片 $0C/$0D（页组取程序起始页），图块号 $064C/$064D，名称表在子功能。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_image_show_full(const void *image, unsigned int tile, unsigned char table)
{
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)tile;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(tile >> 8);
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)image);
    sb_syscall(SB_FN_IMAGE_FULL, table & 3);
}
