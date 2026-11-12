/*
 * ------------------------------------------------------------------------
 * 名称：sb-internal.h
 * 说明：运行库内部使用的声明：几个只供库里的 C 函数调用的汇编小例程。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：不对外发布；库内部的辅助函数一律以 sbi_ 开头，不在接口检查的范围内。
 * ------------------------------------------------------------------------
 */
#ifndef SB_INTERNAL_H
#define SB_INTERNAL_H

#include <sb2000.h>

/** 设置远指针：页号放 $0641，页内偏移放 $14/$15（偏移已不超过 $0FFF）。 */
#define SBI_SET_FAR(page, offset) \
    do { \
        SB_BYTE_AT(SB_SYS_PAGE) = (page); \
        SB_BYTE_AT(SB_FAR_OFFSET_LO) = (unsigned char)(offset); \
        SB_BYTE_AT(SB_FAR_OFFSET_HI) = (unsigned char)((offset) >> 8); \
    } while (0)

/** 设置 $0C/$0D 里的调用地址或窗口地址。 */
#define SBI_SET_ENTRY(address) \
    do { \
        SB_BYTE_AT(SB_PROG_ENTRY_LO) = (unsigned char)(address); \
        SB_BYTE_AT(SB_PROG_ENTRY_HI) = (unsigned char)((address) >> 8); \
    } while (0)

/** 读远指针处的字节，指针事先放好。 */
unsigned char sbi_far_read_byte(void);

/** 把 value 写到远指针处，指针事先放好。 */
void __fastcall__ sbi_far_write_byte(unsigned char value);

/** 把页组 $0642、窗口地址 $0C/$0D 起的 $064F 个字节复制到 $0700 缓冲。 */
void sbi_far_copy_in(void);

/** 把 $0700 缓冲里的 $064F 个字节复制到页组 $0642、窗口地址 $0C/$0D 起的位置。 */
void sbi_far_copy_out(void);

/** 关中断，返回原来的状态寄存器。 */
unsigned char sbi_interrupts_off(void);

/** 恢复 sbi_interrupts_off 返回的状态寄存器。 */
void __fastcall__ sbi_interrupts_restore(unsigned char status);

/** 运行 PATH_BUFFER 里的程序，运行前后保存、恢复 C 运行库的零页；返回 DOS 出错码。 */
unsigned char sbi_exec_run(void);

/** 一个句柄在句柄表里占的字节数。 */
#define SBI_HANDLE_SIZE 0x2C

/** 把 24 位的值写进 3 个相邻的单元（低字节在前）。 */
#define SBI_SET24(address, value) \
    do { \
        SB_BYTE_AT(address) = (unsigned char)(value); \
        SB_BYTE_AT((address) + 1) = (unsigned char)((value) >> 8); \
        SB_BYTE_AT((address) + 2) = (unsigned char)((value) >> 16); \
    } while (0)

/** 读 16 位的值（低字节在前）。 */
#define SBI_GET16(address) (SB_BYTE_AT(address) | ((unsigned int)SB_BYTE_AT((address) + 1) << 8))

/** 目录遍历的状态：当前扇区号、扇区内下一项的序号、是否已结束。 */
extern unsigned int sbi_dir_lba;
extern unsigned char sbi_dir_index;
extern unsigned char sbi_dir_ended;

/** 读 lba 号扇区到 DISK_BUFFER，返回 DOS 出错码。 */
unsigned char sbi_load_sector(unsigned int lba);

/** 调用系统服务，返回调用后的 DOS 出错码。 */
unsigned char sbi_dos_call(unsigned char function, unsigned char sub);

/** 把路径复制到 PATH_BUFFER，最长 28 个字符；过长时返回 SB_ERR_BAD_NAME，否则返回 0。 */
unsigned char sbi_set_path(const char *path);

/** 选定当前句柄；句柄号超出 0-3 时返回 SB_ERR_BAD_PARAMETER，否则返回 0。 */
unsigned char sbi_select_handle(unsigned char handle);

/** 把日期写成 FAT 格式放进 FILE_DATE；年份早于 1980 时返回 SB_ERR_BAD_PARAMETER，否则返回 0。 */
unsigned char sbi_date_to_fat(const sb_date_t *date);

/** 把 FILE_DATE 里的 FAT 日期取成年月日。 */
void sbi_date_from_fat(sb_date_t *date);

/** 把时间写成 FAT 格式放进 FILE_TIME；秒数取一半。 */
void sbi_time_to_fat(const sb_time_t *time);

/** 把 FILE_TIME 里的 FAT 时间取成时分秒。 */
void sbi_time_from_fat(sb_time_t *time);

/** 调用磁盘服务（功能 $80），返回软驱状态码 DISK_STATUS。 */
unsigned char sbi_disk_call(unsigned char sub);

/** 设置磁盘服务的起始面、磁道、扇区。 */
void sbi_disk_set_position(unsigned char head, unsigned char track, unsigned char sector);

/** 设置成批读写的全部参数并调用磁盘服务，返回软驱状态码。 */
unsigned char sbi_disk_batch(unsigned char sub, unsigned char head, unsigned char track, unsigned char sector,
    unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode);

/** 把 count 个字节从 source 写到固定地址 address 起。 */
void sbi_poke(unsigned int address, const unsigned char *source, unsigned char count);

/** 把固定地址 address 起的 count 个字节读到 dest。 */
void sbi_peek(unsigned char *dest, unsigned int address, unsigned char count);

#endif
