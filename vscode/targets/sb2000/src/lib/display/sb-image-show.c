/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-show.c
 * 说明：sb_image_show 的实现：显示压缩图片。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $5F：图片 $0C/$0D（页组取程序起始页），图块号 $064C/$064D，调色板组号 $0653，原点 $0648/$0649，效果 $0652，速度 $0651，名称表选择在子功能。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_image_show(const void *image, const sb_map_t *map, unsigned char palette_group)
{
    SB_BYTE_AT(SB_PARAM_0648) = map->col;
    SB_BYTE_AT(SB_PARAM_0649) = map->row;
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)map->base;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(map->base >> 8);
    SB_BYTE_AT(SB_PARAM_0651) = map->speed;
    SB_BYTE_AT(SB_PARAM_0652) = map->effect;
    SB_BYTE_AT(SB_PARAM_0653) = palette_group;
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)image);
    sb_syscall(SB_FN_IMAGE_SHOW, map->table & 3);
}
