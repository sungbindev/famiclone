/*
 * ------------------------------------------------------------------------
 * 名称：sb-disk-free-bytes.c
 * 说明：sb_disk_free_bytes 的实现：统计磁盘空闲字节数。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：先用 $7B/$01 保证 FAT 缓存已装入（它读扇区 0），再从簇 2 起用 $7B/$06 统计，结果累加在 ARG_069A 起的 3 字节；
 *       $7B/$01 装入缓存失败时 DOS_ERROR 仍为 0，所以还要看 DISK_STATUS；最后让软驱停转。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sb_disk_free_bytes(unsigned long *bytes)
{
    unsigned char error;

    SB_BYTE_AT(SB_DISK_STATUS) = 0;
    error = sbi_load_sector(0);
    if (error == 0) {
        error = SB_BYTE_AT(SB_DISK_STATUS);
    }
    if (error == 0) {
        SB_BYTE_AT(SB_FS_FREE_START) = 2;
        SB_BYTE_AT(SB_FS_FREE_START + 1) = 0;
        SB_BYTE_AT(SB_ARG_069A) = 0;
        SB_BYTE_AT(SB_ARG_069A + 1) = 0;
        SB_BYTE_AT(SB_ARG_069A + 2) = 0;
        error = sbi_dos_call(SB_FN_FILE, SB_SUB_FS_FREE_SPACE);
    }
    if (error == 0) {
        *bytes = (unsigned long)SB_BYTE_AT(SB_ARG_069A)
            | ((unsigned long)SB_BYTE_AT(SB_ARG_069A + 1) << 8)
            | ((unsigned long)SB_BYTE_AT(SB_ARG_069A + 2) << 16);
    }
    sb_disk_stop();
    return error;
}
