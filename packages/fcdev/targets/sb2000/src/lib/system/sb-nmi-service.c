/*
 * ------------------------------------------------------------------------
 * 名称：sb-nmi-service.c
 * 说明：sb_nmi_service 的实现：打开或关闭 NMI 里的一项服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $12 子功能 $81-$86（打开）与 $01-$06（关闭）；服务号超出 1-6 时不调用。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

void sb_nmi_service(unsigned char service, unsigned char enable)
{
    if (service < 1 || service > 6) {
        return;
    }
    sb_syscall(SB_FN_PPU_CONTROL, enable ? (unsigned char)(0x80 | service) : service);
}
