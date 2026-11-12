/*
 * ------------------------------------------------------------------------
 * 名称：start.c
 * 说明：在 SB-DOS 下运行：start；读入盘上的 BIOS.BBK（步步高 BIOS 映像），换掉小霸王的 BIOS 与 SB-DOS，启动步步高系统。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-12
 * 备注：映像格式见 apps/sb2000/bbk-bios/docs/计划与进度.md：16 字节头（"BBKB"、版本 1、页数 8、入口地址）加 8 个 16K 页。
 *       流程：读入文件到扩展 RAM 的低页 -> 检查文件头 -> 关 NMI 与中断
 *       -> 对每个 4K 块：用 DMA 复制到 ERAM 缓冲 $6800-$77FF，再用 CPU 复制到窗口 2 映射的目标页，
 *          目标是扩展 RAM 的页 $E0-$FF（4K 页，共 128K，页 r 在 $E0+4r 起）
 *          （真机实测（memtest）：DMA 只能写 ERAM，不能写窗口；$404B 的位 7 必须置 1）
 *       -> 把跳板复制到 ERAM $6000 运行：窗口 4-7（$C000-$FFFF）指向映像的第 7 页（BIOS 代码与向量），
 *          窗口 0-3（$8000-$BFFF）指向第 0 页，屏蔽 SB2000 的全部中断，设置 PPU，然后跳到入口。
 *       始终留在 6576 模式，不用 $4301；步步高系统起来后不再返回 DOS，只能硬复位。
 *       复制时借用窗口 2（$A000），窗口 3 里有 C 软件栈不能换页，所以程序（含数据）必须在 $A000 以下；
 *       窗口直接写页寄存器 $4042，不再调用 DOS 服务。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>

#define MSG_OPEN "Can't open BIOS.BBK"
#define MSG_MEMORY "No memory"
#define MSG_READ "Read error"
#define MSG_HEADER "Not a BBK BIOS file"
#define MSG_TOO_BIG "Program too big"

#define BIOS_FILE "BIOS.BBK"

/* 映射器与 DMA 寄存器，含义见 MapperSB2K.cpp */
#define REG_PPU_CTRL 0x2000
#define REG_WINDOW2 0x4042
#define REG_DMA_MODE 0x4048
#define REG_DMA_SOURCE_BANK 0x4049
#define REG_DMA_SOURCE_LOW 0x404A
#define REG_DMA_SOURCE_MID 0x404B
#define REG_DMA_DEST_LOW 0x404C
#define REG_DMA_DEST_HIGH 0x404D
#define REG_DMA_LENGTH_LOW 0x404E
#define REG_DMA_LENGTH_HIGH 0x404F
#define DMA_SOURCE_ERAM 0x10
#define DMA_TO_CPU 0xA0
#define DMA_MID_FLAG 0x80
#define DMA_BUFFER 0x6800
#define DMA_WAIT 3000U /* 约 2.5 帧；原厂 BIOS 在复制 3840 字节后等 1 帧 */

/* 文件头与映像大小 */
#define HEADER_SIZE 16
#define PAGE_COUNT 8
#define IMAGE_SIZE 0x020000UL /* 8 个 16K 页 */
#define FILE_SIZE (HEADER_SIZE + IMAGE_SIZE)
#define BLOCK_SIZE 0x1000

/* 扩展 RAM 的 4K 页号：从 $80 起共 128 页；映像放在最后 32 页 */
#define FIRST_ERAM_PAGE 0x80
#define IMAGE_FIRST_PAGE 0xE0
#define IMAGE_BLOCKS 32
#define COPY_ADDRESS 0xA000

/* ERAM $6000 的跳板 */
#define TRAMPOLINE_ADDRESS 0x6000
#define TRAMPOLINE_ENTRY 0x40 /* 偏移：JMP 入口 的操作数，2 字节 */

static unsigned char header[HEADER_SIZE];
static unsigned char first_page;
static unsigned char page_count;

/*
 * 跳板：
 *   SEI
 *   窗口 4-7 = 页 $FC-$FF（映像第 7 页），窗口 0-3 = 页 $E0-$E3（映像第 0 页）
 *   $4032 写 $FF 屏蔽 SB2000 的全部中断源；$2000、$2001、$2008、$4300 清 0（NMI 关、显示关、4 色模式、显存 bank 0）
 *   LDX #$FF : TXS，JMP 入口
 */
static const unsigned char trampoline[] = {
    0x78,
    0xA9, 0xFC, 0x8D, 0x44, 0x40,
    0xA9, 0xFD, 0x8D, 0x45, 0x40,
    0xA9, 0xFE, 0x8D, 0x46, 0x40,
    0xA9, 0xFF, 0x8D, 0x47, 0x40,
    0xA9, 0xE0, 0x8D, 0x40, 0x40,
    0xA9, 0xE1, 0x8D, 0x41, 0x40,
    0xA9, 0xE2, 0x8D, 0x42, 0x40,
    0xA9, 0xE3, 0x8D, 0x43, 0x40,
    0xA9, 0xFF, 0x8D, 0x32, 0x40,
    0xA9, 0x00, 0x8D, 0x00, 0x20, 0x8D, 0x01, 0x20, 0x8D, 0x08, 0x20, 0x8D, 0x00, 0x43,
    0xA2, 0xFF, 0x9A,
    0x4C, 0x00, 0x00
};

/* 程序末尾（BSS 的结束地址）由链接器给出 */
extern unsigned char _BSS_RUN__;
extern unsigned char _BSS_SIZE__;

/* 读入 BIOS.BBK 并检查文件头，成功返回 0，失败返回提示信息；文件占用的页记在 first_page、page_count */
static const char *load_image(void)
{
    unsigned char handle;
    unsigned char pages;
    unsigned char read_error;
    unsigned long size;

    if ((unsigned)&_BSS_RUN__ + (unsigned)&_BSS_SIZE__ > COPY_ADDRESS) {
        return MSG_TOO_BIG;
    }
    if (sb_file_open(BIOS_FILE, SB_MODE_OPEN, &handle) != 0) {
        return MSG_OPEN;
    }
    sb_file_size(handle, &size);
    if (size != FILE_SIZE) {
        sb_file_close(handle);
        return MSG_HEADER;
    }
    pages = (unsigned char)((size + BLOCK_SIZE - 1) >> 12);
    if (sb_page_alloc(pages, &first_page) != 0) {
        sb_file_close(handle);
        return MSG_MEMORY;
    }
    page_count = pages;
    if ((unsigned)first_page + pages > IMAGE_FIRST_PAGE) {
        sb_file_close(handle);
        return MSG_MEMORY;
    }
    SB_BYTE_AT(SB_FILE_START_PAGE) = first_page;
    sb_syscall(SB_FN_FILE_HANDLE, SB_SUB_FH_READ_FILE);
    read_error = SB_BYTE_AT(SB_DOS_ERROR);
    sb_file_close(handle);
    if (read_error != 0) {
        return MSG_READ;
    }
    sb_far_read(first_page, 0, header, HEADER_SIZE);
    if (memcmp(header, "BBKB", 4) != 0 || header[4] != 1 || header[5] != PAGE_COUNT) {
        return MSG_HEADER;
    }
    return 0;
}

/* 把扩展 RAM 里 source 处的 4K 经 DMA 、ERAM 缓冲复制到页 page；源地址不要求对齐 */
static void copy_block(unsigned char page, unsigned long source)
{
    unsigned int wait;

    SB_BYTE_AT(REG_DMA_SOURCE_BANK) = DMA_SOURCE_ERAM | (unsigned char)((source >> 15) & 0x0F);
    SB_BYTE_AT(REG_DMA_SOURCE_LOW) = (unsigned char)source;
    SB_BYTE_AT(REG_DMA_SOURCE_MID) = ((unsigned char)(source >> 8) & 0x7F) | DMA_MID_FLAG;
    SB_BYTE_AT(REG_DMA_DEST_LOW) = (unsigned char)DMA_BUFFER;
    SB_BYTE_AT(REG_DMA_DEST_HIGH) = (unsigned char)(DMA_BUFFER >> 8);
    SB_BYTE_AT(REG_DMA_LENGTH_LOW) = (unsigned char)BLOCK_SIZE;
    SB_BYTE_AT(REG_DMA_LENGTH_HIGH) = (unsigned char)(BLOCK_SIZE >> 8);
    SB_BYTE_AT(REG_DMA_MODE) = DMA_TO_CPU;
    for (wait = 0; wait != DMA_WAIT; ++wait) {
        __asm__("nop");
    }
    SB_BYTE_AT(REG_WINDOW2) = page;
    memcpy((void *)COPY_ADDRESS, (void *)DMA_BUFFER, BLOCK_SIZE);
}

/* 把映像放到页 $E0-$FF 并跳进步步高 BIOS，不会返回 */
static void start_bios(void)
{
    /* 文件在扩展 RAM 里的起始偏移，DMA 的源地址以它为基准 */
    unsigned long base = (unsigned long)(unsigned)(first_page - FIRST_ERAM_PAGE) << 12;
    unsigned char k;

    /* 此后 DOS 的显示与键盘不再运行 */
    SB_BYTE_AT(REG_PPU_CTRL) = 0;
    __asm__("sei");

    for (k = 0; k < IMAGE_BLOCKS; ++k) {
        copy_block(IMAGE_FIRST_PAGE + k, base + HEADER_SIZE + ((unsigned long)k << 12));
    }

    memcpy((void *)TRAMPOLINE_ADDRESS, trampoline, sizeof(trampoline));
    ((unsigned char *)TRAMPOLINE_ADDRESS)[TRAMPOLINE_ENTRY] = header[6];
    ((unsigned char *)TRAMPOLINE_ADDRESS)[TRAMPOLINE_ENTRY + 1] = header[7];
    ((void (*)(void))TRAMPOLINE_ADDRESS)();
}

int main(void)
{
    const char *message = load_image();

    if (message == 0) {
        start_bios();
    }
    if (page_count != 0) {
        sb_page_free(first_page, page_count);
    }
    sb_puts(message);
    sb_getkey();
    return 0;
}
