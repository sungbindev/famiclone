/*
 * ------------------------------------------------------------------------
 * 名称：sb-window-map-group.c
 * 说明：sb_window_map_group 的实现：把页组的 4 个连续页映射到 4 个窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $27，页组写 $0642；调用返回后映射保持。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_window_map_group(unsigned char group)
{
    SB_BYTE_AT(SB_SYS_GROUP) = group;
    sb_syscall(SB_FN_MAP_GROUP, 0);
}
