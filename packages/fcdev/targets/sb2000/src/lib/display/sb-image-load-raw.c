/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-load-raw.c
 * 说明：sb_image_load_raw 的实现：直接装入图案。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $5A：数据 $0C/$0D（页组取程序起始页），长度 $0650/$064F，图块号 $064C/$064D。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_image_load_raw(const void *data, unsigned int length, unsigned int tile)
{
    SB_BYTE_AT(SB_PARAM_064F) = (unsigned char)length;
    SB_BYTE_AT(SB_PARAM_0650) = (unsigned char)(length >> 8);
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)tile;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(tile >> 8);
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)data);
    sb_syscall(SB_FN_IMAGE_RAW, 0);
}
