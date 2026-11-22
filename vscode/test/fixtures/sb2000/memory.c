/*
 * ------------------------------------------------------------------------
 * 名称：memory.c
 * 说明：测试 sb2000-memory.h 里的内存函数：页的分配与释放、跨页读写、窗口映射、窗口地址换算。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000-library.test.mjs 在模拟器里运行；读写结果用两条独立的路径核对：远读写函数与窗口映射后的直接访问。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 要写入的数据，300 字节，跨过页尾，也跨过 255 字节的分块 */
static unsigned char pattern[300];
/** 读回的数据 */
static unsigned char readback[300];

int main(void)
{
    unsigned char program_page = SB_BYTE_AT(SB_PROGRAM_START_PAGE);
    unsigned char largest_before;
    unsigned char largest_after;
    unsigned char page;
    unsigned char probe;
    unsigned char again;
    unsigned char error;
    unsigned char out_page;
    unsigned int out_offset;
    unsigned int i;
    unsigned char ok;

    for (i = 0; i < sizeof(pattern); ++i) {
        pattern[i] = (unsigned char)(i * 7 + 3);
    }

    /* 测试要把窗口 2 换成别的页，程序的代码与数据必须全部在 $A000 之前 */
    SB_CHECK((unsigned int)&sb_test_value[SB_TEST_MAX] < 0xA000 && (unsigned int)&readback[sizeof(readback)] < 0xA000);
    /* 页数为 0 只返回第一个空闲页，不分配 */
    largest_before = sb_page_largest_free();
    SB_CHECK(largest_before > 0);
    error = sb_page_alloc(0, &probe);
    SB_CHECK(error == 0 && probe >= 0x80 && probe < 0xF0);

    /* 分配 3 页：页号在范围内，再看第一个空闲页就不是它了 */
    error = sb_page_alloc(3, &page);
    SB_CHECK(error == 0 && page >= 0x80 && page < 0xF0);
    sb_page_alloc(0, &probe);
    SB_CHECK(probe != page);
    largest_after = sb_page_largest_free();
    SB_CHECK(largest_after <= largest_before);

    /* 单字节读写，偏移超过一页时折算到后面的页 */
    sb_far_write_byte(page, 0x123, 0xA5);
    SB_CHECK_EQ(sb_far_read_byte(page, 0x123), 0xA5);
    sb_far_write_byte(page, 0x1010, 0x5A);
    SB_CHECK(sb_far_read_byte(page + 1, 0x010) == 0x5A && sb_far_read_byte(page, 0x1010) == 0x5A);

    /* 映射到窗口 2 直接访问，核对单字节写入 */
    sb_window_map(2, page);
    SB_CHECK((*(volatile unsigned char *)0xA123) == 0xA5 && SB_BYTE_AT(SB_WINDOW_SAVE_2) == page);

    /* 跨页的批量写入与读回 */
    sb_far_write(page, 0x0F00, pattern, sizeof(pattern));
    memset(readback, 0, sizeof(readback));
    sb_far_read(page, 0x0F00, readback, sizeof(readback));
    SB_CHECK(memcmp(readback, pattern, sizeof(pattern)) == 0);

    /* 逐字节读回，跟批量写入的数据比较 */
    ok = 1;
    for (i = 0; i < sizeof(pattern); ++i) {
        if (sb_far_read_byte(page, 0x0F00 + i) != pattern[i]) {
            ok = 0;
        }
    }
    SB_CHECK(ok);

    /* 窗口地址换算成远指针 */
    sb_address_to_far(0x80, 0x8123, &out_page, &out_offset);
    SB_CHECK(out_page == 0x80 && out_offset == 0x123);
    sb_address_to_far(0x80, 0xB456, &out_page, &out_offset);
    SB_CHECK(out_page == 0x83 && out_offset == 0x456);
    sb_address_to_far(0x90, 0x9ABC, &out_page, &out_offset);
    SB_CHECK(out_page == 0x91 && out_offset == 0xABC);
    sb_address_to_far(0x80, 0xC123, &out_page, &out_offset);
    SB_CHECK(out_page == 0x84 && out_offset == 0x123);

    /* 把自己的页组映射回来，4 个窗口的影子都恢复 */
    sb_window_map_group(program_page);
    SB_CHECK(SB_BYTE_AT(SB_WINDOW_SAVE_0) == program_page
        && SB_BYTE_AT(SB_WINDOW_SAVE_1) == program_page + 1
        && SB_BYTE_AT(SB_WINDOW_SAVE_2) == program_page + 2
        && SB_BYTE_AT(SB_WINDOW_SAVE_3) == program_page + 3);

    /* 释放后再分配同样的页数，得到同一个位置 */
    SB_CHECK_EQ(sb_page_free(page, 3), 0);
    error = sb_page_alloc(3, &again);
    SB_CHECK(error == 0 && again == page);
    sb_page_free(again, 3);

    /* 出错：要的页数太多；释放不合法的页号 */
    error = sb_page_alloc(255, &probe);
    SB_CHECK(error == SB_ERR_NO_MEMORY && probe == 0);
    SB_CHECK_EQ(sb_page_free(0x10, 1), SB_ERR_BAD_ADDRESS);
    SB_CHECK_EQ(sb_page_free(0xF0, 1), SB_ERR_BAD_ADDRESS);

    sb_test_done();
    return 0;
}
