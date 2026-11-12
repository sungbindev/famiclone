/*
 * ------------------------------------------------------------------------
 * 名称：newcall.c
 * 说明：调用 BIOS 新增的系统调用 $35，显示它返回的扩展版本号；BIOS 不支持时显示 NOT SUPPORTED。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：给自己修改过的 BIOS 做功能检测的写法：新系统调用只能经内存返回结果，约定它把主版本写入 SYS_PAGE、次版本写入 SYS_GROUP。
 *       调用前先把这两个单元置 0xFF；原始 BIOS 里功能 $35 是空桩，不会改写它们，据此判断是否支持。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 新增的系统调用号 */
#define FN_BIOS_VERSION 0x35

#define SYS_PAGE (*(volatile unsigned char *)SB_SYS_PAGE)
#define SYS_GROUP (*(volatile unsigned char *)SB_SYS_GROUP)

static char text[] = "BIOS EXT VER 0.0";

/** 低 4 位转成十六进制字符 */
static char hex_digit(unsigned char value)
{
    value &= 0x0F;
    return value < 10 ? '0' + value : 'A' + value - 10;
}

int main(void)
{
    unsigned char major;
    unsigned char minor;

    SYS_PAGE = 0xFF;
    SYS_GROUP = 0xFF;
    sb_syscall(FN_BIOS_VERSION, 0);
    major = SYS_PAGE;
    minor = SYS_GROUP;

    if (major == 0xFF) {
        sb_puts("NOT SUPPORTED");
    } else {
        text[14] = hex_digit(major);
        text[16] = hex_digit(minor);
        sb_puts(text);
    }
    sb_getkey();
    return 0;
}
