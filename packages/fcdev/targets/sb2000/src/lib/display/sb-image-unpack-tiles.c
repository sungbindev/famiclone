/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-unpack-tiles.c
 * 说明：sb_image_unpack_tiles 的实现：解压图案流进显存。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $5B：流 $0C/$0D（页组取程序起始页），图块号 $064C/$064D。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_image_unpack_tiles(const void *stream, unsigned int tile)
{
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)tile;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(tile >> 8);
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)stream);
    sb_syscall(SB_FN_UNPACK_TILES, 0);
}
