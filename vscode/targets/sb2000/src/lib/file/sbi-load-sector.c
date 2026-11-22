/*
 * ------------------------------------------------------------------------
 * 名称：sbi-load-sector.c
 * 说明：库内部函数：读一个扇区到 DISK_BUFFER。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：功能 $7B 子功能 1；扇区号写在 FS_LBA（16 位，低字节在前）；FAT 缓存还没装入时系统先把它装入。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

unsigned char sbi_load_sector(unsigned int lba)
{
    SB_BYTE_AT(SB_FS_LBA) = (unsigned char)lba;
    SB_BYTE_AT(SB_FS_LBA + 1) = (unsigned char)(lba >> 8);
    return sbi_dos_call(SB_FN_FILE, SB_SUB_FS_FAT_LOAD);
}
