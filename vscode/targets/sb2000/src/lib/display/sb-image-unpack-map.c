/*
 * ------------------------------------------------------------------------
 * 名称：sb-image-unpack-map.c
 * 说明：sb_image_unpack_map 的实现：解压映射流到屏幕。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $5C：基础值 $064C/$064D，效果 $0652，调色板位宽 $0653=4，原点 $0648/$0649，宽高 $064B/$064A，速度 $0651，调色板 $0654。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_image_unpack_map(const void *stream, const sb_map_t *map)
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
    SB_BYTE_AT(SB_SYS_GROUP) = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    SBI_SET_ENTRY((unsigned int)stream);
    sb_syscall(SB_FN_UNPACK_MAP, map->table & 3);
}
