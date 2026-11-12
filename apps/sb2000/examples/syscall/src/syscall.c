/*
 * ------------------------------------------------------------------------
 * 名称：syscall.c
 * 说明：系统调用实验程序：由调试器把功能号与参数写进 ERAM 的请求块，程序代为调用，用来在模拟器里逐个核对没有示例的系统服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：DOS 下输入 syscall 运行，之后一直等待请求，不会退出。
 *       请求块在 $7E00：+0 请求标志（调试器写 1，程序执行完清 0）、+1 功能号、+2 子功能、
 *       +3 起 20 字节写入 $0640-$0653、+$1F 与 +$20 写入 $0C、$0D；调用后 $0640-$0653 复制回 +$30 起，+$44 放 DOS_ERROR。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 请求块的起始地址 */
#define BLOCK ((volatile unsigned char *)0x7E00)

/** 参数区 $0640-$0653 的字节数 */
#define PARAM_BYTES 20

#define PARAM_AREA ((volatile unsigned char *)0x0640)
#define PROG_ENTRY ((volatile unsigned char *)0x000C)

int main(void)
{
    unsigned char i;

    BLOCK[0] = 0;
    for (;;) {
        while (BLOCK[0] == 0) {
        }
        for (i = 0; i < PARAM_BYTES; ++i) {
            PARAM_AREA[i] = BLOCK[3 + i];
        }
        PROG_ENTRY[0] = BLOCK[0x1F];
        PROG_ENTRY[1] = BLOCK[0x20];
        sb_syscall(BLOCK[1], BLOCK[2]);
        for (i = 0; i < PARAM_BYTES; ++i) {
            BLOCK[0x30 + i] = PARAM_AREA[i];
        }
        BLOCK[0x44] = *(volatile unsigned char *)SB_DOS_ERROR;
        BLOCK[0] = 0;
    }
}
