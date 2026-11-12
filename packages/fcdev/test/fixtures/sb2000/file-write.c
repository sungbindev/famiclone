/*
 * ------------------------------------------------------------------------
 * 名称：file-write.c
 * 说明：测试 sb2000-file.h 里写文件的函数：整个写出、日期时间属性、按偏移改写（含分块）、改名、整个保存、删除。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：镜像里预先放了 BIG.BIN（10000 字节，第 i 个字节是 i*7+1）；脚本在结束后把镜像读回，
 *       核对程序留下的 KEEP.DAT（700 字节，第 i 个字节是 5+i*3）与改写过的 BIG.BIN。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 第 index 个数据字节 */
#define PATTERN(index) ((unsigned char)(5 + (index) * 3))

/** 要写入的数据 */
static unsigned char data[4100];
/** 读回的一段数据，程序窗口放不下两个 4100 字节的缓冲，所以读回的内容分段核对 */
static unsigned char check[600];

/** 读文件里 offset 起的 count 字节，核对它们依次是 PATTERN(first)、PATTERN(first + 1)……；一致返回 1 */
static unsigned char matches(unsigned char handle, unsigned long offset, unsigned int count, unsigned int first)
{
    unsigned int done = 0;
    unsigned int part;
    unsigned int i;

    while (done < count) {
        part = count - done;
        if (part > sizeof(check)) {
            part = sizeof(check);
        }
        if (sb_file_read(handle, offset + done, check, part) != 0) {
            return 0;
        }
        for (i = 0; i < part; ++i) {
            if (check[i] != PATTERN(first + done + i)) {
                return 0;
            }
        }
        done += part;
    }
    return 1;
}

int main(void)
{
    unsigned char handle;
    unsigned char page;
    unsigned char attr;
    unsigned long size;
    unsigned int i;
    sb_date_t date;
    sb_time_t time;

    for (i = 0; i < sizeof(data); ++i) {
        data[i] = PATTERN(i);
    }

    /* 整个写成一个新文件，再用方式 2 设置日期时间 */
    SB_CHECK_EQ(sb_file_write_all("NEW.TXT", data, 2100), 0);
    SB_CHECK_EQ(sb_file_open("NEW.TXT", SB_MODE_UPDATE, &handle), 0);
    date.year = 2025;
    date.month = 11;
    date.day = 11;
    time.hour = 12;
    time.minute = 34;
    time.second = 56;
    SB_CHECK(sb_file_set_date(handle, &date) == 0 && sb_file_set_time(handle, &time) == 0);
    SB_CHECK_EQ(sb_file_close(handle), 0);

    /* 重新打开：大小、内容、日期时间、默认属性（存档），只读方式不能写 */
    SB_CHECK_EQ(sb_file_open("NEW.TXT", SB_MODE_OPEN, &handle), 0);
    SB_CHECK(sb_file_size(handle, &size) == 0 && size == 2100);
    SB_CHECK(matches(handle, 0, 2100, 0));
    memset(&date, 0, sizeof(date));
    memset(&time, 0, sizeof(time));
    SB_CHECK(sb_file_get_date(handle, &date) == 0 && date.year == 2025 && date.month == 11 && date.day == 11);
    SB_CHECK(sb_file_get_time(handle, &time) == 0 && time.hour == 12 && time.minute == 34 && time.second == 56);
    SB_CHECK(sb_file_get_attr(handle, &attr) == 0 && attr == SB_ATTR_ARCHIVE);
    SB_CHECK_EQ(sb_file_write(handle, 0, data, 4), SB_ERR_BAD_OPEN_MODE);
    sb_file_close(handle);

    /* 方式 2 在中间改写，文件大小不变；超出文件的写入、无效的日期与属性被拒绝 */
    SB_CHECK_EQ(sb_file_open("NEW.TXT", SB_MODE_UPDATE, &handle), 0);
    SB_CHECK_EQ(sb_file_write(handle, 5, "XYZ", 3), 0);
    SB_CHECK(sb_file_size(handle, &size) == 0 && size == 2100);
    SB_CHECK_EQ(sb_file_write(handle, 2099, "ab", 2), 0x14);
    date.month = 13;
    SB_CHECK_EQ(sb_file_set_date(handle, &date), SB_ERR_BAD_PARAMETER);
    SB_CHECK_EQ(sb_file_set_attr(handle, 0xFF), SB_ERR_BAD_PARAMETER);
    sb_file_close(handle);
    SB_CHECK_EQ(sb_file_open("NEW.TXT", SB_MODE_OPEN, &handle), 0);
    SB_CHECK(sb_file_size(handle, &size) == 0 && size == 2100);
    memset(check, 0, 16);
    SB_CHECK(sb_file_read(handle, 4, check, 5) == 0 && check[0] == data[4]
        && memcmp(check + 1, "XYZ", 3) == 0 && check[4] == data[8]);
    sb_file_close(handle);

    /* 在大文件中间改写 4100 字节（起始位置不在扇区开头，内部分成两块），大小不变，改写前后的字节不受影响 */
    SB_CHECK_EQ(sb_file_open("BIG.BIN", SB_MODE_UPDATE, &handle), 0);
    SB_CHECK_EQ(sb_file_write(handle, 3000, data, 4100), 0);
    SB_CHECK(sb_file_size(handle, &size) == 0 && size == 10000);
    SB_CHECK(matches(handle, 3000, 4100, 0));
    SB_CHECK(sb_file_read(handle, 2990, check, 10) == 0 && check[0] == (unsigned char)(2990 * 7 + 1) && check[9] == (unsigned char)(2999 * 7 + 1));
    SB_CHECK(sb_file_read(handle, 7100, check, 10) == 0 && check[0] == (unsigned char)(7100 * 7 + 1) && check[9] == (unsigned char)(7109 * 7 + 1));
    sb_file_close(handle);

    /* 改名：原名消失，新名存在 */
    SB_CHECK_EQ(sb_file_rename("NEW.TXT", "NEW2.TXT"), 0);
    SB_CHECK_EQ(sb_file_open("NEW.TXT", SB_MODE_OPEN, &handle), SB_ERR_FILE_NOT_FOUND);
    SB_CHECK_EQ(sb_file_open("NEW2.TXT", SB_MODE_OPEN, &handle), 0);
    sb_file_close(handle);

    /* 整个保存成 KEEP.DAT，留给脚本核对 */
    SB_CHECK_EQ(sb_page_alloc(1, &page), 0);
    sb_far_write(page, 0, data, 700);
    SB_CHECK_EQ(sb_file_save("KEEP.DAT", page, 700), 0);
    sb_page_free(page, 1);

    /* 删除文件，再删一次出错 */
    SB_CHECK_EQ(sb_file_delete("NEW2.TXT"), 0);
    SB_CHECK_EQ(sb_file_open("NEW2.TXT", SB_MODE_OPEN, &handle), SB_ERR_FILE_NOT_FOUND);
    SB_CHECK(sb_file_delete("NEW2.TXT") != 0);

    sb_test_done();
    return 0;
}
