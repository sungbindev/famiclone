/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-fill-rect.c
 * 说明：sb_image_fill_rect 的实现：按效果铺满矩形。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $60：参数同 $5C，没有映射流。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_image_fill_rect(const sb_map_t *map)
{
    SB_BYTE_AT(SB_PARAM_0648) = map->col;
    SB_BYTE_AT(SB_PARAM_0649) = map->row;
    SB_BYTE_AT(SB_PARAM_064B) = map->width;
    SB_BYTE_AT(SB_PARAM_064A) = map->height;
    SB_BYTE_AT(SB_PARAM_064C) = (unsigned char)map->base;
    SB_BYTE_AT(SB_PARAM_064D) = (unsigned char)(map->base >> 8);
    SB_BYTE_AT(SB_PARAM_0651) = map->speed;
    SB_BYTE_AT(SB_PARAM_0652) = map->effect;
    SB_BYTE_AT(SB_PARAM_0653) = 4;
    SB_BYTE_AT(SB_PARAM_0654) = map->palette;
    sb_syscall(SB_FN_FILL_RECT, map->table & 3);
}
