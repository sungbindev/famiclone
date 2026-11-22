/*
 * ------------------------------------------------------------------------
 * 名称：sb-window-map.c
 * 说明：sb_window_map 的实现：把一个扩展 RAM 页映射到一个窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $29，页号写 $0641，窗口号写 $0653；调用返回后映射保持。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_window_map(unsigned char window, unsigned char page)
{
    SB_BYTE_AT(SB_SYS_PAGE) = page;
    SB_BYTE_AT(SB_SYS_WINDOW) = window;
    sb_syscall(SB_FN_MAP_WINDOW, 0);
}
