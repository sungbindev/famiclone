/*
 * ------------------------------------------------------------------------
 * 名称：bank.c
 * 说明：换页测试程序的常驻部分：经蹦床调用库页函数，核对参数与返回值、换回原库页、嵌套换页、系统调用后窗口 2 不变。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：需要 targetOptions.banks 不小于 9；库页函数在 bank-pages.c。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "bank-pages.h"
#include "sb-test.h"

unsigned char resident_double(unsigned char n)
{
    return (unsigned char)(n * 2);
}

int main(void)
{
    unsigned char data[16];

    /* 启动时窗口 2 里是库页 1 */
    SB_CHECK_EQ(sb_bank_current(), 1);
    SB_CHECK_EQ(bank1_tag, 1);

    /* 调用库页函数：返回值正确，调用后换回原来的库页 */
    SB_CHECK_EQ(bank1_value(), 0x11);
    SB_CHECK_EQ(bank2_value(), 0x12);
    SB_CHECK_EQ(bank9_value(), 0x19);
    SB_CHECK_EQ(sb_bank_current(), 1);

    /* 每个库页的整块数据都读得到（16 位返回值） */
    SB_CHECK_EQ(bank1_sum(), 3072);
    SB_CHECK_EQ(bank9_sum(), 27648);
    SB_CHECK_EQ(sb_bank_current(), 1);

    /* 多个参数：C 栈上的 16 位参数与 A、X 里的最后一个参数 */
    SB_CHECK_EQ(bank2_mix(300, 7, 5000), 6000);
    SB_CHECK_EQ(bank9_mix(1, 255, 40000u), 40000u + 25500u + 1u);
    SB_CHECK_EQ(sb_bank_current(), 1);

    /* 嵌套：库页 2 的函数调用库页 9 与库页 1，回来后库页 2 的数据仍然对 */
    SB_CHECK_EQ(bank2_nested(), 1230 + 0x19 + 0x11 + 2);
    SB_CHECK_EQ(sb_bank_current(), 1);

    /* 库页里的代码调用常驻函数 */
    SB_CHECK_EQ(bank9_call_resident(21), 42 + 9);

    /* 手动换页：系统调用（等帧、写文件）之后，窗口 2 里仍是换入的库页 */
    sb_bank_select(9);
    SB_CHECK_EQ(sb_bank_current(), 9);
    SB_CHECK_EQ(bank9_tag, 9);
    sb_wait_frames(30);
    SB_CHECK_EQ(bank9_tag, 9);
    memset(data, 0x5A, sizeof(data));
    SB_CHECK_EQ(sb_file_write_all("X.TMP", data, sizeof(data)), 0);
    SB_CHECK_EQ(bank9_tag, 9);

    /* 当前库页是 2 时调用库页 9 的函数，返回后仍是库页 2 */
    sb_bank_select(2);
    SB_CHECK_EQ(bank2_tag, 2);
    SB_CHECK_EQ(bank9_value(), 0x19);
    SB_CHECK_EQ(sb_bank_current(), 2);
    SB_CHECK_EQ(bank2_tag, 2);

    sb_bank_select(1);
    sb_test_done();
    return 0;
}
