/*
 * ------------------------------------------------------------------------
 * 名称：timer.c
 * 说明：测试 sb2000-timer.h 里的定时器函数：启动与停止、一次计时与连续计时的下溢标志、屏蔽与允许、当前值读回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：由 sb2000-library.test.mjs 在模拟器里运行；测试在关中断的状态下进行，测完用 DOS 开机时的值（控制 $EF、预置值 $3C）恢复定时器。
 *       用行消隐时钟（64 微秒一拍）和预置值 $20，约 2 毫秒下溢一次。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>
#include "sb-test.h"

/** 一次计时：允许、行消隐时钟、预分频 0 */
#define CONTROL_ONCE (SB_HW_TIMER_ENABLE | SB_HW_TIMER_LINE_CLOCK)
/** 连续计时：再加上重载 */
#define CONTROL_REPEAT (CONTROL_ONCE | SB_HW_TIMER_RELOAD)
/** 测试用的预置值 */
#define PRESET 0x20

/* 简单延时 */
static void delay(unsigned int count)
{
    while (count-- != 0) {
        __asm__("nop");
    }
}

/* 等下溢标志置位，等不到返回 0 */
static unsigned char wait_pending(void)
{
    unsigned int n;

    for (n = 0; n != 60000u; ++n) {
        if (sb_timer_pending()) {
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    unsigned char a;
    unsigned char b;
    unsigned char c;

    __asm__("sei");

    /* 屏蔽并清标志 */
    sb_timer_mask();
    SB_CHECK(!sb_timer_pending());
    sb_timer_unmask();

    /* 一次计时：启动后当前值等于预置值，下溢后标志置位、当前值保持预置值 */
    sb_timer_start(CONTROL_ONCE, PRESET);
    SB_CHECK_EQ(sb_timer_current(), PRESET);
    SB_CHECK(wait_pending());
    SB_CHECK_EQ(sb_timer_current(), PRESET);

    /* 屏蔽清掉标志；一次计时不再下溢，所以标志不会再置位 */
    sb_timer_mask();
    SB_CHECK(!sb_timer_pending());
    sb_timer_unmask();
    delay(20000);
    SB_CHECK(!sb_timer_pending());

    /* 连续计时：标志清掉后会再次置位 */
    sb_timer_start(CONTROL_REPEAT, PRESET);
    SB_CHECK(wait_pending());
    sb_timer_mask();
    SB_CHECK(!sb_timer_pending());
    sb_timer_unmask();
    SB_CHECK(wait_pending());

    /* 连续计时时当前值在变：三次读数不全相同 */
    a = sb_timer_current();
    delay(5);
    b = sb_timer_current();
    delay(37);
    c = sb_timer_current();
    SB_CHECK(a != b || b != c);

    /* 停止：控制字节为 0；控制字节与预置值可以读回 */
    sb_timer_stop();
    SB_CHECK_EQ(SB_BYTE_AT(SB_HW_TIMER_CONTROL), 0);
    sb_timer_start(0x21, 0x77);
    SB_CHECK(SB_BYTE_AT(SB_HW_TIMER_CONTROL) == 0x21 && SB_BYTE_AT(SB_HW_TIMER_PRESET) == 0x77);

    /* 恢复 DOS 开机时的设置 */
    sb_timer_mask();
    sb_timer_start(0xEF, 0x3C);
    sb_timer_unmask();
    __asm__("cli");

    sb_test_done();
    return 0;
}
