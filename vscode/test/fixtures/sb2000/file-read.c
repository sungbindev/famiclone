/*
 * ------------------------------------------------------------------------
 * 名称：file-read.c
 * 说明：测试 sb2000-file.h 里读文件的函数：打开、大小、按偏移读（含跨过 4096 字节分块）、整个读入扩展 RAM 页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：镜像里预先放了 HELLO.TXT（15 字节）与 BIG.BIN（10000 字节，第 i 个字节是 i*7+1）。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 读用的缓冲，要大于 4096，才能测到分块读 */
static unsigned char buffer[4100];

int main(void)
{
    unsigned char handle;
    unsigned char error;
    unsigned char page;
    unsigned char attr;
    unsigned long size;
    unsigned int i;
    unsigned char ok;

    /* 读小文件：大小、按偏移读、越界读出错、属性、关闭 */
    error = sb_file_open("HELLO.TXT", SB_MODE_OPEN, &handle);
    SB_CHECK_EQ(error, 0);
    SB_CHECK(sb_file_size(handle, &size) == 0 && size == 15);
    SB_CHECK(sb_file_read(handle, 0, buffer, 5) == 0 && memcmp(buffer, "Hello", 5) == 0);
    SB_CHECK(sb_file_read(handle, 7, buffer, 7) == 0 && memcmp(buffer, "SB2000!", 7) == 0);
    SB_CHECK_EQ(sb_file_read(handle, 10, buffer, 6), 0x14);
    SB_CHECK_EQ(sb_file_read(handle, 0, buffer, 0), 0);
    SB_CHECK(sb_file_get_attr(handle, &attr) == 0 && (attr & SB_ATTR_DIRECTORY) == 0);
    SB_CHECK_EQ(sb_file_close(handle), 0);

    /* 出错：文件不存在，句柄号非法 */
    SB_CHECK_EQ(sb_file_open("NOFILE.TXT", SB_MODE_OPEN, &handle), SB_ERR_FILE_NOT_FOUND);
    SB_CHECK_EQ(sb_file_close(7), SB_ERR_BAD_PARAMETER);
    SB_CHECK_EQ(sb_file_size(7, &size), SB_ERR_BAD_PARAMETER);

    /* 大文件按偏移读 4100 字节，长度超过一页，内部分成两块 */
    error = sb_file_open("BIG.BIN", SB_MODE_OPEN, &handle);
    SB_CHECK_EQ(error, 0);
    error = sb_file_read(handle, 3000, buffer, 4100);
    ok = 1;
    for (i = 0; i < 4100; ++i) {
        if (buffer[i] != (unsigned char)((3000 + i) * 7 + 1)) {
            ok = 0;
        }
    }
    SB_CHECK(error == 0 && ok);
    sb_file_close(handle);

    /* 整个读入扩展 RAM 页：大小、页号，抽样核对内容，再释放 */
    error = sb_file_load("BIG.BIN", &page, &size);
    SB_CHECK(error == 0 && size == 10000 && page >= 0x80);
    SB_CHECK(sb_far_read_byte(page, 0) == 1 && sb_far_read_byte(page, 4095) == (unsigned char)(4095 * 7 + 1)
        && sb_far_read_byte(page, 4096) == (unsigned char)(4096 * 7 + 1) && sb_far_read_byte(page, 9999) == (unsigned char)(9999 * 7 + 1));
    SB_CHECK_EQ(sb_page_free(page, 3), 0);
    SB_CHECK_EQ(sb_file_load("NOFILE.BIN", &page, &size), SB_ERR_FILE_NOT_FOUND);

    sb_test_done();
    return 0;
}
