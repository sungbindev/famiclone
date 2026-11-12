/*
 * ------------------------------------------------------------------------
 * 名称：sb-dir-read.c
 * 说明：sb_dir_read 的实现：取目录里的下一项。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：每个扇区 16 项，每项 32 字节：0-7 名称，8-10 扩展名，11 属性，22-23 时间，24-25 日期，28-31 大小；
 *       首字节为 0 表示目录结束，$E5 表示已删除；扇区用完后经 $7B/$00 前进到下一扇区（沿簇链或到根目录区域末尾）。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include "../sb-internal.h"

/** 把名称的一个字段复制到输出，去掉结尾的空格，返回复制的字符数 */
static unsigned char copy_field(char *out, const unsigned char *field, unsigned char length)
{
    unsigned char count = length;
    unsigned char index;

    while (count != 0 && field[count - 1] == ' ') {
        --count;
    }
    for (index = 0; index < count; ++index) {
        out[index] = (char)field[index];
    }
    return count;
}

unsigned char sb_dir_read(sb_dir_entry_t *entry)
{
    unsigned char raw[32];
    unsigned char error;
    unsigned char length;
    unsigned int value;

    for (;;) {
        if (sbi_dir_ended) {
            return SB_DIR_END;
        }
        if (sbi_dir_index == 16) {
            SB_BYTE_AT(SB_FS_LBA) = (unsigned char)sbi_dir_lba;
            SB_BYTE_AT(SB_FS_LBA + 1) = (unsigned char)(sbi_dir_lba >> 8);
            error = sbi_dos_call(SB_FN_FILE, SB_SUB_FS_NEXT_SECTOR);
            if (error != 0) {
                return error;
            }
            if (SB_BYTE_AT(SB_FS_REGION_END) != 0) {
                sbi_dir_ended = 1;
                return SB_DIR_END;
            }
            sbi_dir_lba = SBI_GET16(SB_FS_NEXT_LBA);
            error = sbi_load_sector(sbi_dir_lba);
            if (error != 0) {
                return error;
            }
            sbi_dir_index = 0;
        }
        memcpy(raw, (const void *)(SB_DISK_BUFFER + sbi_dir_index * 32), 32);
        ++sbi_dir_index;
        if (raw[0] == 0) {
            sbi_dir_ended = 1;
            return SB_DIR_END;
        }
        if (raw[0] == 0xE5 || raw[11] == 0x0F) {
            continue;
        }
        length = copy_field(entry->name, raw, 8);
        if (raw[8] != ' ') {
            entry->name[length++] = '.';
            length += copy_field(entry->name + length, raw + 8, 3);
        }
        entry->name[length] = 0;
        entry->attr = raw[11];
        value = raw[22] | ((unsigned int)raw[23] << 8);
        entry->time.hour = (unsigned char)(value >> 11);
        entry->time.minute = (unsigned char)((value >> 5) & 0x3F);
        entry->time.second = (unsigned char)((value & 0x1F) << 1);
        value = raw[24] | ((unsigned int)raw[25] << 8);
        entry->date.year = 1980 + (value >> 9);
        entry->date.month = (unsigned char)((value >> 5) & 0x0F);
        entry->date.day = (unsigned char)(value & 0x1F);
        entry->size = (unsigned long)raw[28] | ((unsigned long)raw[29] << 8)
            | ((unsigned long)raw[30] << 16) | ((unsigned long)raw[31] << 24);
        return 0;
    }
}
