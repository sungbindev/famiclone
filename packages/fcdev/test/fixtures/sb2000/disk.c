/*
 * ------------------------------------------------------------------------
 * 名称：disk.c
 * 说明：测试 sb2000-disk.h 里的软驱函数：启动停转、读写单个扇区、成批读写（跨面、页内起始扇区）、扫描磁道、格式化磁道、空闲字节数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：读写的位置都在镜像的空闲区，不碰文件：单扇区 面 1 磁道 70 扇区 5（逻辑扇区 2542，第 j 个字节是 j*5+3），
 *       成批 面 0 磁道 71 扇区 17 起 20 个扇区（逻辑扇区 2572-2591，第 n 个扇区的第 j 个字节是 n*7+j*3+1），
 *       格式化 面 1 磁道 79（逻辑扇区 2862-2879，应全是 $F6）；脚本在结束后读回镜像核对这些位置。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>
#include "sb-test.h"

/** 一个扇区的缓冲 */
static unsigned char sector[SB_DISK_SECTOR_SIZE];
/** 读回比较用的缓冲 */
static unsigned char other[SB_DISK_SECTOR_SIZE];
/** 写文件用的数据 */
static unsigned char data[1100];
/** 扫描磁道的坏扇区位图 */
static unsigned char bad[3];

/** 成批读写数据里第 n 个扇区的第 j 个字节 */
#define BATCH_BYTE(n, j) ((unsigned char)((n) * 7 + (j) * 3 + 1))

/** 把第 n 个扇区的内容填进 buffer */
static void fill_batch(unsigned char *buffer, unsigned char n)
{
    unsigned int j;

    for (j = 0; j < SB_DISK_SECTOR_SIZE; ++j) {
        buffer[j] = BATCH_BYTE(n, j);
    }
}

int main(void)
{
    unsigned char write_page;
    unsigned char read_page;
    unsigned char n;
    unsigned char all_match;
    unsigned long free_before;
    unsigned long free_after;
    unsigned int j;

    /* 启动、复位、停转 */
    SB_CHECK_EQ(sb_disk_init(), 0);
    SB_CHECK_EQ(sb_disk_start(), 0);
    sb_disk_stop();

    /* 引导扇区：每扇区字节数 512（小端 00 02），末尾标志 55 AA */
    SB_CHECK_EQ(sb_disk_read_sector(0, 0, 1, sector), 0);
    SB_CHECK(sector[11] == 0 && sector[12] == 2);
    SB_CHECK_EQ((unsigned int)sector[510] * 256 + sector[511], 0x55AA);

    /* 单个扇区：写一个空闲区的扇区，读回一致；再读一次引导扇区，说明缓冲没有被串用 */
    for (j = 0; j < SB_DISK_SECTOR_SIZE; ++j) {
        sector[j] = (unsigned char)(j * 5 + 3);
    }
    SB_CHECK_EQ(sb_disk_write_sector(1, 70, 5, sector), 0);
    memset(other, 0, sizeof(other));
    SB_CHECK_EQ(sb_disk_read_sector(1, 70, 5, other), 0);
    SB_CHECK(memcmp(sector, other, SB_DISK_SECTOR_SIZE) == 0);
    SB_CHECK_EQ(sb_disk_read_sector(0, 0, 1, other), 0);
    SB_CHECK(other[12] == 2 && other[510] == 0x55);

    /* 成批写：20 个扇区从面 0 磁道 71 扇区 17 起，跨到面 1；数据放在 3 个连续页里 */
    SB_CHECK_EQ(sb_page_alloc(3, &write_page), 0);
    for (n = 0; n < 20; ++n) {
        fill_batch(sector, n);
        sb_far_write(write_page, (unsigned int)n * SB_DISK_SECTOR_SIZE, sector, SB_DISK_SECTOR_SIZE);
    }
    SB_CHECK_EQ(sb_disk_write_sectors(0, 71, 17, write_page, 0, 20, SB_DISK_STOP_ON_ERROR), 0);

    /* 成批读：读到另一组页里，从页内第 3 个扇区的位置开始放，逐个扇区核对 */
    SB_CHECK_EQ(sb_page_alloc(3, &read_page), 0);
    SB_CHECK_EQ(sb_disk_read_sectors(0, 71, 17, read_page, 3, 20, SB_DISK_STOP_ON_ERROR), 0);
    all_match = 1;
    for (n = 0; n < 20; ++n) {
        fill_batch(sector, n);
        sb_far_read(read_page, (3 + (unsigned int)n) * SB_DISK_SECTOR_SIZE, other, SB_DISK_SECTOR_SIZE);
        if (memcmp(sector, other, SB_DISK_SECTOR_SIZE) != 0) {
            all_match = 0;
        }
    }
    SB_CHECK(all_match);
    /* 同样的扇区用单扇区读：面 1 磁道 71 扇区 1 是第 2 个（序号 2）扇区 */
    SB_CHECK_EQ(sb_disk_read_sector(1, 71, 1, other), 0);
    fill_batch(sector, 2);
    SB_CHECK(memcmp(sector, other, SB_DISK_SECTOR_SIZE) == 0);
    sb_page_free(write_page, 3);
    sb_page_free(read_page, 3);

    /* 扫描磁道：完好的磁道没有坏扇区，面、磁道超出范围被拒绝 */
    memset(bad, 0xFF, sizeof(bad));
    SB_CHECK_EQ(sb_disk_scan_track(0, 0, bad), 0);
    SB_CHECK(bad[0] == 0 && bad[1] == 0 && bad[2] == 0);
    SB_CHECK_EQ(sb_disk_scan_track(2, 0, bad), SB_ERR_BAD_DISK_PARAM);
    SB_CHECK_EQ(sb_disk_scan_track(0, 80, bad), SB_ERR_BAD_DISK_PARAM);

    /* 格式化磁道：整个磁道变成 $F6，参数超出范围被拒绝 */
    SB_CHECK_EQ(sb_disk_format_track(1, 79), 0);
    memset(other, 0, sizeof(other));
    SB_CHECK_EQ(sb_disk_read_sector(1, 79, 1, other), 0);
    SB_CHECK(other[0] == 0xF6 && other[511] == 0xF6);
    SB_CHECK_EQ(sb_disk_format_track(2, 0), SB_ERR_BAD_DISK_PARAM);

    /* 空闲字节数：在 1.44MB 以内，写一个 1100 字节的文件（3 个扇区的簇）后正好少 1536 */
    SB_CHECK_EQ(sb_disk_free_bytes(&free_before), 0);
    SB_CHECK(free_before > 1000000UL && free_before < 1474560UL);
    memset(data, 0x5A, sizeof(data));
    SB_CHECK_EQ(sb_file_write_all("FREE.BIN", data, sizeof(data)), 0);
    SB_CHECK_EQ(sb_disk_free_bytes(&free_after), 0);
    SB_CHECK_EQ(free_before - free_after, 1536);

    sb_test_done();
    return 0;
}
