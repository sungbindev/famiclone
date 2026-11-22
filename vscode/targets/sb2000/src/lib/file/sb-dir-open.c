/*
 * ------------------------------------------------------------------------
 * 名称：sb-dir-open.c
 * 说明：sb_dir_open 的实现：开始遍历一个目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：当前目录的第一个扇区号在 FS_CURRENT_DIR_LBA；根目录从扇区 19 起；子目录先以方式 4 取得目录项，
 *       再把它的起始簇经 $7B/$03 换算成扇区号；第一个扇区读进 DISK_BUFFER。
 * ------------------------------------------------------------------------
 */
#include "../sb-internal.h"

/** 1.44MB 软盘上根目录的第一个扇区：引导扇区 1 个加两份 FAT 各 9 个 */
#define ROOT_DIR_LBA 19

/** 判断路径是不是根目录：可带驱动器号，其余只有一个反斜杠 */
static unsigned char is_root(const char *path)
{
    if (path[0] != 0 && path[1] == ':') {
        path += 2;
    }
    return path[0] == '\\' && path[1] == 0;
}

unsigned char sb_dir_open(const char *path)
{
    unsigned char handle;
    unsigned char error;
    unsigned int lba;
    unsigned int cluster;

    sbi_dir_ended = 1;
    if (path == 0 || path[0] == 0) {
        lba = SBI_GET16(SB_FS_CURRENT_DIR_LBA);
    } else if (is_root(path)) {
        lba = ROOT_DIR_LBA;
    } else {
        error = sb_file_open(path, SB_MODE_ENTRY, &handle);
        if (error != 0) {
            return error;
        }
        cluster = SBI_GET16(SB_FILE_HANDLES + handle * SBI_HANDLE_SIZE + SB_FH_CLUSTER_OFFSET);
        sb_file_close(handle);
        SB_BYTE_AT(SB_FS_CLUSTER) = (unsigned char)cluster;
        SB_BYTE_AT(SB_FS_CLUSTER + 1) = (unsigned char)(cluster >> 8);
        error = sbi_dos_call(SB_FN_FILE, SB_SUB_FS_CLUSTER_LBA);
        if (error != 0) {
            return error;
        }
        lba = SBI_GET16(SB_FS_LBA);
    }
    error = sbi_load_sector(lba);
    if (error != 0) {
        return error;
    }
    sbi_dir_lba = lba;
    sbi_dir_index = 0;
    sbi_dir_ended = 0;
    return 0;
}
