/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-file.h
 * 说明：SB-DOS 程序的文件与目录服务：打开、关闭、读写、整个文件加载与保存、删除、改名、属性、目录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。磁盘是 1.44MB 的 FAT12 软盘，文件名是 8.3 格式（大写），
 *       路径最长 28 个字符，可带驱动器 A: 与反斜杠，目录只支持一级子目录；同时最多打开 4 个文件，句柄号 0 到 3。
 *       所有函数返回 DOS 出错码，0 表示成功，结果经指针参数带回。路径字符串必须在程序窗口 $8000-$BFFF 里。
 *       文件数据在扩展 RAM 页里中转：sb_file_read、sb_file_write 内部临时借用 1 页，sb_file_load、sb_file_save 的数据直接放在页里。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_FILE_H
#define SB2000_FILE_H

/** 文件属性：只读。 */
#define SB_ATTR_READONLY 0x01
/** 文件属性：隐藏。 */
#define SB_ATTR_HIDDEN 0x02
/** 文件属性：系统。 */
#define SB_ATTR_SYSTEM 0x04
/** 文件属性：卷标。 */
#define SB_ATTR_VOLUME 0x08
/** 文件属性：目录。 */
#define SB_ATTR_DIRECTORY 0x10
/** 文件属性：存档。 */
#define SB_ATTR_ARCHIVE 0x20

/** sb_dir_read 返回它表示已经没有更多的项。 */
#define SB_DIR_END 0xFF

/** 目录项。 */
typedef struct {
    /** 名称，格式 NAME.EXT，没有扩展名时没有点，零结尾。 */
    char name[13];
    /** 属性，SB_ATTR_ 开头的常量按位组合。 */
    unsigned char attr;
    /** 修改日期。 */
    sb_date_t date;
    /** 修改时间。 */
    sb_time_t time;
    /** 文件大小，字节。 */
    unsigned long size;
} sb_dir_entry_t;

/**
 * 打开文件或目录项。打开方式用 SB_MODE_ 开头的常量：
 * SB_MODE_OPEN 打开已有文件（只读）；SB_MODE_CREATE 创建文件，已存在则覆盖；
 * SB_MODE_UPDATE 打开已有文件，可按偏移写，关闭时写回目录项；SB_MODE_ENTRY 取得根目录或子目录的目录项。
 * 相对路径在当前目录里找。
 *
 * @syscall $7B/$09
 * @param path 路径，零结尾，最长 28 个字符。
 * @param mode 打开方式。
 * @param handle 返回句柄号，0 到 3。
 * @return 0 成功；SB_ERR_FILE_NOT_FOUND 文件不存在；SB_ERR_PATH_NOT_FOUND 目录不存在；SB_ERR_TOO_MANY_FILES 4 个句柄都在用；
 *         SB_ERR_BAD_NAME 名称含非法字符或路径过长；SB_ERR_BAD_OPEN_MODE 打开方式不合法；SB_ERR_ACCESS_DENIED 名称与卷标同名。
 */
unsigned char sb_file_open(const char *path, unsigned char mode, unsigned char *handle);

/**
 * 关闭文件，写回目录项，并让软驱停转。关闭空闲的句柄什么都不做，也不出错。
 *
 * @syscall $7C/$02
 * @param handle 句柄号。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 句柄号超出 0-3。
 */
unsigned char sb_file_close(unsigned char handle);

/**
 * 取文件大小。
 *
 * @syscall $7C/$0D
 * @param handle 句柄号。
 * @param size 返回文件大小，字节。
 * @return 0 成功。
 */
unsigned char sb_file_size(unsigned char handle, unsigned long *size);

/**
 * 从文件的指定位置读数据到内存，长度不限（内部分块读）。
 *
 * @syscall $7C/$05
 * @param handle 句柄号，打开方式要是 SB_MODE_OPEN、SB_MODE_CREATE 或 SB_MODE_UPDATE。
 * @param offset 文件内偏移。
 * @param dest 目的地，必须在程序窗口 $8000-$BFFF 内，且不在窗口 3（$B000-$BFFF）。
 * @param length 字节数；偏移不小于文件大小或偏移加长度超过文件大小时出错，数据不会被截短读入。
 * @return 0 成功；$14 读写位置超出文件；SB_ERR_BAD_OPEN_MODE 打开方式不允许读；SB_ERR_NO_MEMORY 没有空闲页做中转。
 */
unsigned char sb_file_read(unsigned char handle, unsigned long offset, void *dest, unsigned int length);

/**
 * 改写文件里已有的一段数据，文件大小不变（内部分块写）。不能用它扩大文件或新建文件，那些用 sb_file_write_all 或 sb_file_save。
 *
 * @syscall $7C/$06
 * @param handle 句柄号，打开方式要是 SB_MODE_CREATE 或 SB_MODE_UPDATE。
 * @param offset 文件内偏移。
 * @param src 数据来源，位置的要求同 sb_file_read 的 dest。
 * @param length 字节数。
 * @return 0 成功；$14 写入范围超出文件当前大小；SB_ERR_BAD_OPEN_MODE 打开方式不允许写；SB_ERR_NO_MEMORY 没有空闲页做中转。
 */
unsigned char sb_file_write(unsigned char handle, unsigned long offset, const void *src, unsigned int length);

/**
 * 把内存里的数据写成一个文件（已存在则覆盖），文件大小就是数据长度。数据先放进临时分配的扩展 RAM 页，再整个写出。
 *
 * @syscall $7C/$10
 * @param path 路径。
 * @param data 数据，位置的要求同 sb_file_read 的 dest。
 * @param length 字节数，0 只创建空文件。
 * @return 0 成功；SB_ERR_NO_MEMORY 没有足够的页；SB_ERR_NO_SPACE 磁盘空间不够；其他是创建与写入的出错码。
 */
unsigned char sb_file_write_all(const char *path, const void *data, unsigned int length);

/**
 * 把整个文件读入新分配的扩展 RAM 页，适合比内存大的文件：数据在页里，用 sb_far_read 按需取出。
 * 用完要用 sb_page_free 释放页。空文件不分配页，页号返回 0。
 *
 * @syscall $7C/$0F
 * @param path 路径。
 * @param first_page 返回第一个页号。
 * @param size 返回文件大小，字节。
 * @return 0 成功；SB_ERR_NO_MEMORY 没有足够的连续空闲页；其他是打开与读取的出错码。
 */
unsigned char sb_file_load(const char *path, unsigned char *first_page, unsigned long *size);

/**
 * 把扩展 RAM 页里的数据整个写成一个文件（已存在则覆盖）。数据长度为 0 时只创建空文件。
 *
 * @syscall $7C/$10
 * @param path 路径。
 * @param first_page 数据的第一个页号。
 * @param size 数据长度，字节，不超过 16777215。
 * @return 0 成功；SB_ERR_NO_SPACE 磁盘空间不够，已写入的其他文件完好；其他是创建与写入的出错码。
 */
unsigned char sb_file_save(const char *path, unsigned char first_page, unsigned long size);

/**
 * 删除文件：目录项标为已删除并释放空间，之后软驱停转。
 *
 * @syscall $7C/$03
 * @param path 路径。
 * @return 0 成功；SB_ERR_ACCESS_DENIED 文件是只读的；SB_ERR_FILE_NOT_FOUND 文件不存在。
 */
unsigned char sb_file_delete(const char *path);

/**
 * 给文件改名。新名称是 NAME.EXT 格式，不含目录。
 *
 * @syscall $7C/$04
 * @param path 原路径。
 * @param new_name 新名称。
 * @return 0 成功；SB_ERR_FILE_EXISTS 新名称已存在；SB_ERR_BAD_NAME 新名称含非法字符。
 */
unsigned char sb_file_rename(const char *path, const char *new_name);

/**
 * 取已打开文件的修改日期。
 *
 * @syscall $7C/$08
 * @param handle 句柄号。
 * @param date 返回日期。
 * @return 0 成功。
 */
unsigned char sb_file_get_date(unsigned char handle, sb_date_t *date);

/**
 * 设置已打开文件的修改日期，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。
 *
 * @syscall $7C/$07
 * @param handle 句柄号。
 * @param date 新日期。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 日期不合法。
 */
unsigned char sb_file_set_date(unsigned char handle, const sb_date_t *date);

/**
 * 取已打开文件的修改时间。
 *
 * @syscall $7C/$0A
 * @param handle 句柄号。
 * @param time 返回时间。
 * @return 0 成功。
 */
unsigned char sb_file_get_time(unsigned char handle, sb_time_t *time);

/**
 * 设置已打开文件的修改时间，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。
 *
 * @syscall $7C/$09
 * @param handle 句柄号。
 * @param time 新时间。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 时间不合法。
 */
unsigned char sb_file_set_time(unsigned char handle, const sb_time_t *time);

/**
 * 取已打开文件的属性。
 *
 * @syscall $7C/$0C
 * @param handle 句柄号。
 * @param attr 返回属性，SB_ATTR_ 开头的常量按位组合。
 * @return 0 成功。
 */
unsigned char sb_file_get_attr(unsigned char handle, unsigned char *attr);

/**
 * 设置已打开文件的属性，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。
 * 属性大于 $3F 或带卷标位时出错。只读文件不能删除，方式 SB_MODE_CREATE 打开则直接覆盖并恢复成存档属性。
 *
 * @syscall $7C/$0B
 * @param handle 句柄号。
 * @param attr 新属性。
 * @return 0 成功；SB_ERR_BAD_PARAMETER 属性不合法。
 */
unsigned char sb_file_set_attr(unsigned char handle, unsigned char attr);

/**
 * 创建子目录。只支持一级子目录，在子目录里再建目录出错。
 *
 * @syscall $7D/$02
 * @param path 路径。
 * @return 0 成功；SB_ERR_DIR_EXISTS 目录已存在；SB_ERR_DUPLICATE_NAME 同名文件已存在；SB_ERR_PATH_NOT_FOUND 父目录不存在；SB_ERR_DIR_TOO_DEEP 目录嵌套层数过多。
 */
unsigned char sb_dir_make(const char *path);

/**
 * 删除空的子目录。
 *
 * @syscall $7D/$03
 * @param path 路径。
 * @return 0 成功；SB_ERR_DIR_NOT_EMPTY 目录非空；SB_ERR_ACCESS_DENIED 目录是只读的；SB_ERR_DUPLICATE_NAME 路径是文件；SB_ERR_PATH_NOT_FOUND 路径不存在。
 */
unsigned char sb_dir_remove(const char *path);

/**
 * 设置当前目录，相对路径都以它为起点。
 *
 * @syscall $7B/$0B
 * @param path 路径，根目录写 "\"。
 * @return 0 成功；SB_ERR_PATH_NOT_FOUND 路径不存在或是文件，当前目录不变。
 */
unsigned char sb_dir_change(const char *path);

/**
 * 取当前目录自己的名称，格式 \NAME.EXT，根目录是 \。
 *
 * @syscall $7D/$01
 * @param name 返回名称，缓冲区至少 16 个字节。
 * @return 0 成功。
 */
unsigned char sb_dir_current(char *name);

/**
 * 开始遍历一个目录。遍历期间不要做其他磁盘操作，它们会占用同一个扇区缓冲。
 *
 * @syscall $7B/$01、$7B/$03、$7B/$09
 * @param path 目录路径；写 NULL 或空字符串表示当前目录，"\" 表示根目录。
 * @return 0 成功；SB_ERR_PATH_NOT_FOUND 目录不存在；其他是磁盘出错码。
 */
unsigned char sb_dir_open(const char *path);

/**
 * 取下一个目录项，跳过已删除的项；卷标和子目录也会返回，用属性区分。
 *
 * @syscall $7B/$00、$7B/$01
 * @param entry 返回目录项。
 * @return 0 取到一项；SB_DIR_END 已经没有更多的项；其他是磁盘出错码。
 */
unsigned char sb_dir_read(sb_dir_entry_t *entry);

#endif
