/*
 * ------------------------------------------------------------------------
 * 名称：sb-test.c
 * 说明：SB2000 运行库测试程序的公共实现：记录检查结果，提供脚本用的停靠点。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：sb_test_done、sb_test_sync 往停靠标志 sb_test_stop 写入 1、2，测试脚本对它设写断点。
 *       不对函数入口设执行断点：DOS 外壳与 BIOS 也在 $8000 窗口里跑，程序里的函数入口地址可能正好是它们等待循环的某一条指令，
 *       每转一圈就停一次，模拟器几乎不前进。
 * ------------------------------------------------------------------------
 */
#include "sb-test.h"

/** 停靠标志：1 表示结束（sb_test_done），2 表示等待脚本操作（sb_test_sync） */
volatile unsigned char sb_test_stop;
unsigned char sb_test_count;
unsigned char sb_test_ok[SB_TEST_MAX];
unsigned int sb_test_line[SB_TEST_MAX];
unsigned int sb_test_value[SB_TEST_MAX];

void sb_test_record(unsigned char ok, unsigned int line, unsigned int value)
{
    if (sb_test_count >= SB_TEST_MAX) {
        return;
    }
    sb_test_ok[sb_test_count] = ok ? 1 : 2;
    sb_test_line[sb_test_count] = line;
    sb_test_value[sb_test_count] = value;
    sb_test_count++;
}

void sb_test_done(void)
{
    sb_test_stop = 1;
}

void sb_test_sync(void)
{
    sb_test_stop = 2;
}
