/*
 * ------------------------------------------------------------------------
 * 名称：sb-ppu-wait-vblank.c
 * 说明：sb_ppu_wait_vblank 的实现：等垂直消隐。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：轮询 $2002，读一次就会清掉垂直消隐位；最多读 65535 次，避免显示关闭时一直等。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_ppu_wait_vblank(void)
{
    unsigned int n;
    unsigned char status;

    for (n = 0; n != 0xFFFFu; ++n) {
        status = SB_BYTE_AT(SB_HW_PPU_STATUS);
        if (status & SB_HW_STATUS_VBLANK) {
            return status;
        }
    }
    return 0;
}
