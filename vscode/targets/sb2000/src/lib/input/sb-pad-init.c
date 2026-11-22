/*
 * ------------------------------------------------------------------------
 * 名称：sb-pad-init.c
 * 说明：sb_pad_init 的实现：初始化手柄状态，并打开 NMI 里读手柄的服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $12/$85 打开读手柄服务，再用功能 $6A 子功能 0 初始化。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

void sb_pad_init(void)
{
    sb_nmi_service(SB_NMI_SERVICE_PAD, 1);
    sb_syscall(SB_FN_GAMEPAD, SB_SUB_PAD_INIT);
}
