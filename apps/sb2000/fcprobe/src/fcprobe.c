/*
 * ------------------------------------------------------------------------
 * 名称：fcprobe.c
 * 说明：SB-DOS 下运行：fcprobe；在 FC 兼容模式里测试内存写入、PPU、软驱与键盘，用来判断步步高系统能否在该模式下运行。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-12
 * 备注：做法与 runnes 相同：关 NMI 与中断 -> 把第 3 个 128K 窗口（扩展 RAM 页 $E0-$FF）预置成 $AA 并改写向量
 *       -> 把探测桩（probe-stub.s）复制到 ERAM $6100 -> 在 ERAM $6000 的跳板里写 $5006、$4301 进入 FC 兼容模式。
 *       CPU 复位后执行探测桩，结果写在 ERAM $7F00 起，由调试服务读出；桩运行后不会返回 DOS，只能硬复位。
 *       复制时借用窗口 2（$A000），窗口 3 里有 C 软件栈不能换页，所以程序（含数据）必须在 $A000 以下。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>

#define MSG_TOO_BIG "Program too big"

/* 第 3 个 128K 窗口：扩展 RAM 页 $E0-$FF，共 32 页，每页 4K */
#define WINDOW_FIRST_PAGE 0xE0
#define WINDOW_PAGES 32
#define LAST_PAGE 0xFF
#define PAGE_SIZE 0x1000
#define COPY_WINDOW 2
#define COPY_ADDRESS 0xA000
#define PATTERN 0xAA

/* 向量在最后一页的末尾 6 字节：NMI、RESET、IRQ */
#define VECTOR_OFFSET 0x0FFA

/* ERAM 里的跳板、探测桩与结果区 */
#define TRAMPOLINE_ADDRESS 0x6000
#define STUB_ADDRESS 0x6100
#define RESET_ENTRY (STUB_ADDRESS + 1)
#define RESULT_ADDRESS 0x7F00
#define RESULT_SIZE 0x100

/* 探测桩的起止，由 probe-stub.s 导出 */
extern unsigned char stub_start[];
extern unsigned char stub_end[];

/* 程序末尾（BSS 的结束地址）由链接器给出 */
extern unsigned char _BSS_RUN__;
extern unsigned char _BSS_SIZE__;

/* 跳板，复制到 ERAM $6000 运行：LDA #$FF : STA $5006 : LDA #$55 : STA $4301（写 $4301 会复位 CPU） */
static const unsigned char trampoline[] = {
    0xA9, 0xFF, 0x8D, 0x06, 0x50, 0xA9, 0x55, 0x8D, 0x01, 0x43
};

/* 写一个向量：低字节在前 */
static void set_vector(volatile unsigned char *vector, unsigned int address)
{
    vector[0] = (unsigned char)address;
    vector[1] = (unsigned char)(address >> 8);
}

/* 把第 3 个窗口预置成 $AA，并把最后一页的 NMI、RESET、IRQ 向量指向探测桩 */
static void prepare_window(void)
{
    unsigned char k;
    volatile unsigned char *vectors;

    for (k = 0; k < WINDOW_PAGES; ++k) {
        sb_window_map(COPY_WINDOW, WINDOW_FIRST_PAGE + k);
        memset((void *)COPY_ADDRESS, PATTERN, PAGE_SIZE);
    }
    sb_window_map(COPY_WINDOW, LAST_PAGE);
    vectors = (volatile unsigned char *)(COPY_ADDRESS + VECTOR_OFFSET);
    set_vector(vectors, STUB_ADDRESS);
    set_vector(vectors + 2, RESET_ENTRY);
    set_vector(vectors + 4, STUB_ADDRESS);
}

/* 进入 FC 兼容模式，不会返回 */
static void start_probe(void)
{
    SB_BYTE_AT(SB_HW_PPU_CTRL) = 0;
    __asm__("sei");

    prepare_window();
    memset((void *)RESULT_ADDRESS, 0, RESULT_SIZE);
    memcpy((void *)STUB_ADDRESS, stub_start, (unsigned int)(stub_end - stub_start));
    memcpy((void *)TRAMPOLINE_ADDRESS, trampoline, sizeof(trampoline));
    ((void (*)(void))TRAMPOLINE_ADDRESS)();
}

int main(void)
{
    if ((unsigned)&_BSS_RUN__ + (unsigned)&_BSS_SIZE__ > COPY_ADDRESS) {
        sb_puts(MSG_TOO_BIG);
        sb_getkey();
        return 0;
    }
    start_probe();
    return 0;
}
