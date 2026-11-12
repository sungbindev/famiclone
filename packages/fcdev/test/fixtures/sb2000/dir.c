/*
 * ------------------------------------------------------------------------
 * 名称：dir.c
 * 说明：测试 sb2000-file.h 里的目录函数与 sb2000-clock.h、sb_exec：创建、遍历、切换、删除目录，运行子程序，系统日期时间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：镜像里预先放了 HELLO.TXT（15 字节）、BIG.BIN（10000 字节）与 CHILD.EXE；脚本在结束后核对留下的目录 KEEPDIR。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

int main(void)
{
    sb_date_t date;
    sb_time_t time;
    sb_date_t saved_date;
    sb_time_t saved_time;
    sb_dir_entry_t entry;
    char name[16];
    unsigned char found_hello = 0;
    unsigned char found_big = 0;
    unsigned char found_sub = 0;
    unsigned char found_dot = 0;
    unsigned char found_in = 0;

    /* 创建、重复创建、遍历根目录 */
    SB_CHECK_EQ(sb_dir_make("SUB"), 0);
    SB_CHECK_EQ(sb_dir_make("SUB"), SB_ERR_DIR_EXISTS);
    SB_CHECK_EQ(sb_dir_open("\\"), 0);
    while (sb_dir_read(&entry) == 0) {
        if (strcmp(entry.name, "HELLO.TXT") == 0 && entry.size == 15) {
            found_hello = 1;
        }
        if (strcmp(entry.name, "BIG.BIN") == 0 && entry.size == 10000) {
            found_big = 1;
        }
        if (strcmp(entry.name, "SUB") == 0 && (entry.attr & SB_ATTR_DIRECTORY) != 0) {
            found_sub = 1;
        }
    }
    SB_CHECK(found_hello && found_big && found_sub);
    SB_CHECK_EQ(sb_dir_read(&entry), SB_DIR_END);

    /* 进入子目录：当前目录名，里面创建文件，遍历当前目录能看到 . 与新文件 */
    SB_CHECK_EQ(sb_dir_change("SUB"), 0);
    SB_CHECK(sb_dir_current(name) == 0 && strcmp(name, "\\SUB") == 0);
    SB_CHECK_EQ(sb_file_write_all("IN.TXT", "inside", 6), 0);
    SB_CHECK_EQ(sb_dir_open(0), 0);
    while (sb_dir_read(&entry) == 0) {
        if (strcmp(entry.name, ".") == 0) {
            found_dot = 1;
        }
        if (strcmp(entry.name, "IN.TXT") == 0 && entry.size == 6) {
            found_in = 1;
        }
    }
    SB_CHECK(found_dot && found_in);
    SB_CHECK_EQ(sb_dir_change("\\"), 0);
    SB_CHECK(sb_dir_current(name) == 0 && strcmp(name, "\\") == 0);

    /* 非空目录不能删；删掉里面的文件才能删；再建一个留给脚本核对的目录；不存在的目录不能进入 */
    SB_CHECK_EQ(sb_dir_remove("SUB"), SB_ERR_DIR_NOT_EMPTY);
    SB_CHECK_EQ(sb_file_delete("SUB\\IN.TXT"), 0);
    SB_CHECK_EQ(sb_dir_remove("SUB"), 0);
    SB_CHECK_EQ(sb_dir_make("KEEPDIR"), 0);
    SB_CHECK_EQ(sb_dir_change("NOWHERE"), SB_ERR_PATH_NOT_FOUND);

    /* 运行子程序：它往 $7F00 写标记 */
    SB_BYTE_AT(0x7F00) = 0;
    SB_CHECK_EQ(sb_exec("CHILD.EXE"), 0);
    SB_CHECK_EQ(SB_BYTE_AT(0x7F00), 0xA5);
    SB_CHECK_EQ(sb_exec("NOFILE.EXE"), SB_ERR_FILE_NOT_FOUND);

    /* 系统日期与时间：设置后读回，不合法的被拒绝，最后恢复 */
    sb_clock_get_date(&saved_date);
    sb_clock_get_time(&saved_time);
    date.year = 2024;
    date.month = 2;
    date.day = 29;
    SB_CHECK_EQ(sb_clock_set_date(&date), 0);
    memset(&date, 0, sizeof(date));
    sb_clock_get_date(&date);
    SB_CHECK(date.year == 2024 && date.month == 2 && date.day == 29);
    date.year = 2023;
    SB_CHECK_EQ(sb_clock_set_date(&date), SB_ERR_BAD_PARAMETER);
    time.hour = 13;
    time.minute = 45;
    time.second = 50;
    SB_CHECK_EQ(sb_clock_set_time(&time), 0);
    sb_clock_get_time(&time);
    SB_CHECK(time.hour == 13 && time.minute == 45 && time.second >= 50 && time.second <= 56);
    time.hour = 24;
    SB_CHECK_EQ(sb_clock_set_time(&time), SB_ERR_BAD_PARAMETER);
    sb_clock_set_date(&saved_date);
    sb_clock_set_time(&saved_time);

    sb_test_done();
    return 0;
}
