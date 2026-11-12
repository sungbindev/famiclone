/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-disk.h
 * 说明：SB-DOS 程序的软驱底层服务：按面、磁道、扇区读写，成批读写，扫描坏扇区，格式化磁道，空闲空间。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。软驱是 1.44MB：2 个面、80 个磁道、每道 18 个扇区，扇区 512 字节；
 *       面从 0 起，磁道从 0 起，扇区号从 1 起。直接读写扇区会绕过文件系统，写错位置会破坏磁盘上的文件，
 *       要读写文件请用 sb2000-file.h。所有函数返回软驱状态码，0 表示成功（常见的有 SB_ERR_BAD_DISK_PARAM 参数超出范围、
 *       SB_ERR_WRITE_PROTECT 写保护、SB_ERR_NOT_READY 驱动器未准备好、SB_ERR_CRC 校验错、SB_ERR_NO_DISK 没有磁盘、SB_ERR_UNKNOWN_DISK 未知磁盘类型）。
 *       缓冲区指针必须在程序窗口 $8000-$BFFF 里；成批读写的数据放在扩展 RAM 页里（页用 sb_page_alloc 申请，用 sb_far_read、sb_far_write 存取）。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_DISK_H
#define SB2000_DISK_H

/** 一个扇区的字节数。 */
#define SB_DISK_SECTOR_SIZE 512
/** 每道的扇区数。 */
#define SB_DISK_SECTORS_PER_TRACK 18
/** 磁头（面）数。 */
#define SB_DISK_HEADS 2
/** 磁道数。 */
#define SB_DISK_TRACKS 80

/** 成批读写时遇到出错的扇区立即返回，状态码是该扇区的出错码。 */
#define SB_DISK_STOP_ON_ERROR 0x80
/** 成批读写时跳过出错的扇区继续，最后状态码为 0，没读到的扇区缓冲保持原样（出错被吞掉）。 */
#define SB_DISK_SKIP_ERRORS 0x00

/**
 * 启动软驱并复位到 0 磁道，读出状态后停转。可用来检查驱动器是否就绪。
 *
 * @syscall $80/$00
 * @return 0 成功；SB_ERR_NOT_READY 驱动器没有响应。
 */
unsigned char sb_disk_init(void);

/**
 * 启动软驱。文件服务会自己启动与停转软驱，只有直接读写扇区时才需要。
 *
 * @syscall $80/$06
 * @return 0 成功；SB_ERR_NOT_READY 驱动器未准备好。
 */
unsigned char sb_disk_start(void);

/**
 * 让软驱停转。直接读写扇区之后调用，文件服务每次操作之后都会自己调用。
 *
 * @syscall $80/$07
 */
void sb_disk_stop(void);

/**
 * 读一个扇区，先检测盘片。
 *
 * @syscall $80/$02
 * @param head 面，0 或 1。
 * @param track 磁道，0 到 79。
 * @param sector 扇区号，1 到 18。
 * @param dest 接收 512 字节的缓冲区，在程序窗口里。
 * @return 0 成功；SB_ERR_NO_DISK 驱动器中没有磁盘；SB_ERR_UNKNOWN_DISK 未知磁盘类型；SB_ERR_CRC 扇区校验错。
 */
unsigned char sb_disk_read_sector(unsigned char head, unsigned char track, unsigned char sector, void *dest);

/**
 * 写一个扇区，先检测盘片。
 *
 * @syscall $80/$03
 * @param head 面，0 或 1。
 * @param track 磁道，0 到 79。
 * @param sector 扇区号，1 到 18。
 * @param src 要写入的 512 字节，在程序窗口里。
 * @return 0 成功；SB_ERR_WRITE_PROTECT 磁盘写保护；SB_ERR_NO_DISK 驱动器中没有磁盘；SB_ERR_UNKNOWN_DISK 未知磁盘类型。
 */
unsigned char sb_disk_write_sector(unsigned char head, unsigned char track, unsigned char sector, const void *src);

/**
 * 从起始扇区连续读若干扇区到扩展 RAM 页里，扇区号递增，满 18 换面，两面满换磁道。先检测盘片。
 *
 * @syscall $80/$04
 * @param head 起始面，0 或 1。
 * @param track 起始磁道，0 到 79。
 * @param sector 起始扇区号，1 到 18。
 * @param page 接收数据的扩展 RAM 页号。
 * @param page_sector 数据从页内第几个 512 字节开始放，0 到 7；每页放 8 个扇区，页要有足够的连续页。
 * @param count 扇区数。
 * @param mode SB_DISK_STOP_ON_ERROR 或 SB_DISK_SKIP_ERRORS，见这两个常量的说明。
 * @return 0 成功；SB_ERR_NO_DISK 驱动器中没有磁盘；SB_ERR_CRC 等扇区出错码（mode 为 SB_DISK_STOP_ON_ERROR 时）。
 */
unsigned char sb_disk_read_sectors(unsigned char head, unsigned char track, unsigned char sector,
    unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode);

/**
 * 把扩展 RAM 页里的数据连续写到若干扇区，扇区号递增的规则同 sb_disk_read_sectors。先检测盘片，写保护时立即停止。
 *
 * @syscall $80/$05
 * @param head 起始面，0 或 1。
 * @param track 起始磁道，0 到 79。
 * @param sector 起始扇区号，1 到 18。
 * @param page 数据所在的扩展 RAM 页号。
 * @param page_sector 数据从页内第几个 512 字节开始，0 到 7。
 * @param count 扇区数。
 * @param mode SB_DISK_STOP_ON_ERROR 或 SB_DISK_SKIP_ERRORS。
 * @return 0 成功；SB_ERR_WRITE_PROTECT 磁盘写保护；SB_ERR_NO_DISK 驱动器中没有磁盘。
 */
unsigned char sb_disk_write_sectors(unsigned char head, unsigned char track, unsigned char sector,
    unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode);

/**
 * 逐个读一个磁道的 18 个扇区，读不出的在位图里置位。不检测盘片，直接读每个扇区。
 *
 * @syscall $80/$08
 * @param head 面，0 或 1。
 * @param track 磁道，0 到 79。
 * @param bad_map 返回 3 字节位图：第 0 字节的位 7 到位 0 依次对应扇区 1 到 8，第 1 字节对应扇区 9 到 16，第 2 字节的位 7、位 6 对应扇区 17、18；置位表示读不出。
 * @return 0 成功；SB_ERR_BAD_DISK_PARAM 面或磁道超出范围。
 */
unsigned char sb_disk_scan_track(unsigned char head, unsigned char track, unsigned char *bad_map);

/**
 * 格式化一个磁道：磁道 0 的 18 个扇区填 $00，其他磁道填 $F6。先检测盘片。会清掉这个磁道上的全部数据。
 *
 * @syscall $80/$01
 * @param head 面，0 或 1。
 * @param track 磁道，0 到 79。
 * @return 0 成功；SB_ERR_BAD_DISK_PARAM 面或磁道超出范围；SB_ERR_WRITE_PROTECT 磁盘写保护；SB_ERR_NO_DISK 驱动器中没有磁盘。
 */
unsigned char sb_disk_format_track(unsigned char head, unsigned char track);

/**
 * 取磁盘上的空闲字节数（空闲簇数乘每簇字节数）。
 *
 * @syscall $7B/$01、$7B/$06
 * @param bytes 返回空闲字节数。
 * @return 0 成功；其他是读 FAT 时的磁盘出错码。
 */
unsigned char sb_disk_free_bytes(unsigned long *bytes);

#endif
