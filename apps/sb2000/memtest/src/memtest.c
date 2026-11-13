/*
 * ------------------------------------------------------------------------
 * 名称：memtest.c
 * 说明：在 SB-DOS 下运行：memtest；在真机上实测扩展 RAM、ROM 页、扩展显存、内部 RAM 与 DMA 的可用写法，结果分两屏显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-12
 * 备注：测试写入的单元都先保存、测完按相反顺序还原；关 NMI、关中断之后不调用 DOS 服务，直接写窗口 2 的页寄存器 $4042。
 *       扩展 RAM 的测试单元在窗口 2 页内偏移 CELL_OFFSET 处：先只读比较各页与本程序自己的页（代码页）的内容，
 *       内容相同的页是别名，不写；本程序占的 4 页也不写，按“4 页互不相同”计入。
 *       DMA 的写法组合：模式 $A0 与 $E0、$404B 位 7 为 0 与 1、目标 ERAM $7C00 与窗口 2；开着 NMI 与显示时只试目标 ERAM，
 *       关着 NMI 与中断时全试。ERAM $7C00 的 16 字节先保存后还原。
 *       模式 $A0 与 $E0 的目的位（位 5）都是 1，目标都是工作 RAM；$E0 只多置了位 6（传输速率）。
 *       显示全部用 ASCII，每行不超过 31 个字符（屏幕 32 列），共两屏。
 * ------------------------------------------------------------------------
 */
#include <string.h>
#include <sb2000.h>

#define MSG_SIZE "Bad program size"
#define MSG_MEMORY "No memory"
#define MSG_WAIT "Testing, please wait..."

/* 寄存器：含义见 MapperSB2K.cpp 与 machines/sb2000/disassembly/docs/research/NES兼容模式研究.md */
#define REG_PPU_CTRL 0x2000
#define REG_PPU_STATUS 0x2002
#define REG_PPU_ADDRESS 0x2006
#define REG_PPU_DATA 0x2007
#define REG_WINDOW2 0x4042
#define REG_DMA_MODE 0x4048
#define REG_DMA_SOURCE_BANK 0x4049
#define REG_DMA_SOURCE_LOW 0x404A
#define REG_DMA_SOURCE_MID 0x404B
#define REG_DMA_DEST_LOW 0x404C
#define REG_DMA_DEST_HIGH 0x404D
#define REG_DMA_LENGTH_LOW 0x404E
#define REG_DMA_LENGTH_HIGH 0x404F
#define REG_VRAM_BANK 0x4300
#define DMA_SOURCE_EXT_RAM 0x10

/* 窗口 2 与测试单元 */
#define WINDOW_ADDRESS 0xA000
#define CELL_OFFSET 0x0800
#define PROGRAM_LIMIT WINDOW_ADDRESS
#define PROGRAM_START_PAGE 0x8005 /* DOS 装入时写入的起始页号 */
#define PROGRAM_PAGES 4

/* 与别的页比较内容时取本程序的静态字节：第 1 页 $100-$7FF 的代码段（每隔 7 字节取 1 个）与第 2 页开头 256 字节；程序文件要大于 PROGRAM_MIN_END */
#define OWN_HEAD_OFFSET 0x0100
#define OWN_HEAD_STEP 7
#define OWN_SAMPLES 256
#define PROGRAM_MIN_END 0x9100

/* 测试范围 */
#define EXT_FIRST_BANK 0x80
#define EXT_BANKS 128
#define ROM_BANKS 128
#define VRAM_BANKS 32
#define VRAM_CELL 0xFFF8 /* 扩展显存 32K 窗口的末尾附近 */
#define DMA_BYTES 16
#define DMA_MODES 2
#define DMA_DEST_ERAM 0x7C00
#define DMA_MID_FLAG 0x80 /* 原厂 BIOS 每次都把 $404B 的位 7 置 1 */

/* 公共工作区：各项测试依次使用；扩展 RAM 测试用 3 段，每段 128 字节 */
static unsigned char work[3 * EXT_BANKS];

static unsigned char ext_distinct;
static unsigned char ext_alias;
static unsigned char ext_unexpected;
static unsigned char rom_distinct;
static unsigned char rom_blank;
static unsigned int rom_period;
static unsigned char vram_distinct;
static unsigned char ram_distinct;
static unsigned char eram_distinct;
static unsigned char dma_ok[2][DMA_MODES][2][2]; /* [0 开着 NMI 与显示，1 关着][模式][$404B 位 7][0 目标 ERAM，1 目标窗口 2] */

static unsigned char scratch_page;
static unsigned char dma_pattern[DMA_BYTES];
static const unsigned char dma_modes[DMA_MODES] = { 0xA0, 0xE0 };

extern unsigned char _BSS_RUN__;
extern unsigned char _BSS_SIZE__;

/* 页寄存器直接选窗口 2，不经 DOS */
#define MAP_WINDOW2(bank) (SB_BYTE_AT(REG_WINDOW2) = (bank))
#define CELL (*(volatile unsigned char *)(WINDOW_ADDRESS + CELL_OFFSET))

/* 统计 work[first .. first+count) 里有多少个非 0 */
static unsigned char count_set(unsigned char first, unsigned char count)
{
    unsigned char i;
    unsigned char total = 0;

    for (i = 0; i < count; ++i) {
        if (work[first + i] != 0) {
            ++total;
        }
    }
    return total;
}

static unsigned char hash_uniform;

/* 取窗口 2 里 bank 页从 offset 起每隔 step 字节的 count 个字节算摘要；hash_uniform 记这些字节是否全相同 */
static unsigned int hash_page(unsigned char bank, unsigned int offset, unsigned int count, unsigned char step)
{
    volatile unsigned char *p;
    unsigned int h = 0;
    unsigned char value;
    unsigned char first;

    MAP_WINDOW2(bank);
    p = (volatile unsigned char *)(WINDOW_ADDRESS + offset);
    first = *p;
    hash_uniform = 1;
    while (count-- != 0) {
        value = *p;
        p += step;
        h = ((h << 3) | (h >> 13)) ^ value;
        if (value != first) {
            hash_uniform = 0;
        }
    }
    return h;
}

/* 扩展 RAM 页 $80-$FF：先只读排除本程序的页与它们的别名，再往其余页各写页号，写完读回，不重复的值的个数就是独立的页数 */
static void test_ext_ram(void)
{
    unsigned char start_page = SB_BYTE_AT(PROGRAM_START_PAGE);
    unsigned int own_head = hash_page(start_page, OWN_HEAD_OFFSET, OWN_SAMPLES, OWN_HEAD_STEP);
    unsigned int own_tail = hash_page(start_page + 1, 0, OWN_SAMPLES, 1);
    unsigned char *saved = work;
    unsigned char *seen = work + EXT_BANKS;
    unsigned char *skip = work + 2 * EXT_BANKS;
    unsigned char bank;
    unsigned char k;
    unsigned char value;

    memset(seen, 0, EXT_BANKS);
    ext_alias = 0;
    ext_unexpected = 0;
    for (k = 0; k < EXT_BANKS; ++k) {
        bank = EXT_FIRST_BANK + k;
        skip[k] = 0;
        if (bank >= start_page && bank < start_page + PROGRAM_PAGES) {
            skip[k] = 1;
        } else if (hash_page(bank, OWN_HEAD_OFFSET, OWN_SAMPLES, OWN_HEAD_STEP) == own_head
                   || hash_page(bank, 0, OWN_SAMPLES, 1) == own_tail) {
            skip[k] = 1;
            ++ext_alias;
        }
    }
    for (k = 0; k < EXT_BANKS; ++k) {
        if (skip[k] == 0) {
            MAP_WINDOW2(EXT_FIRST_BANK + k);
            saved[k] = CELL;
            CELL = k;
        }
    }
    for (k = 0; k < EXT_BANKS; ++k) {
        if (skip[k] == 0) {
            MAP_WINDOW2(EXT_FIRST_BANK + k);
            value = CELL;
            if (value < EXT_BANKS) {
                seen[value] = 1;
            } else {
                ++ext_unexpected;
            }
        }
    }
    for (k = EXT_BANKS; k-- > 0;) {
        if (skip[k] == 0) {
            MAP_WINDOW2(EXT_FIRST_BANK + k);
            CELL = saved[k];
        }
    }
    ext_distinct = count_set(EXT_BANKS, EXT_BANKS) + PROGRAM_PAGES;
}

/* ROM 页 $00-$7F：只读，每页取 256 个采样字节算摘要，比较摘要得出不同页数、空白页数与重复周期 */
static void test_rom(void)
{
    unsigned int *sig = (unsigned int *)work;
    unsigned int period;
    unsigned char k;
    unsigned char j;
    unsigned char ok;

    rom_blank = 0;
    rom_distinct = 0;
    for (k = 0; k < ROM_BANKS; ++k) {
        sig[k] = hash_page(k, 0, 256, 16);
        rom_blank += hash_uniform;
    }
    for (k = 0; k < ROM_BANKS; ++k) {
        ok = 1;
        for (j = 0; j < k; ++j) {
            if (sig[j] == sig[k]) {
                ok = 0;
                break;
            }
        }
        rom_distinct += ok;
    }
    for (period = 1; period < ROM_BANKS; period <<= 1) {
        ok = 1;
        for (k = 0; k < ROM_BANKS; ++k) {
            if (sig[k] != sig[k & (period - 1)]) {
                ok = 0;
                break;
            }
        }
        if (ok) {
            break;
        }
    }
    rom_period = period;
}

static void vram_set_address(void)
{
    unsigned char sink = SB_BYTE_AT(REG_PPU_STATUS);

    (void)sink;
    SB_BYTE_AT(REG_PPU_ADDRESS) = (unsigned char)(VRAM_CELL >> 8);
    SB_BYTE_AT(REG_PPU_ADDRESS) = (unsigned char)VRAM_CELL;
}

/* 读一个字节：第一次读可能只是刷新缓冲，重设地址后第二次读才是内容 */
static unsigned char vram_read(void)
{
    unsigned char sink;

    vram_set_address();
    sink = SB_BYTE_AT(REG_PPU_DATA);
    (void)sink;
    vram_set_address();
    return SB_BYTE_AT(REG_PPU_DATA);
}

static void vram_write(unsigned char value)
{
    vram_set_address();
    SB_BYTE_AT(REG_PPU_DATA) = value;
}

/* 扩展显存 bank 0-31（$4300）：每个 bank 写入 bank 号，读回后数不同的值 */
static void test_ext_vram(void)
{
    unsigned char k;
    unsigned char value;

    memset(work + VRAM_BANKS, 0, VRAM_BANKS);
    for (k = 0; k < VRAM_BANKS; ++k) {
        SB_BYTE_AT(REG_VRAM_BANK) = k;
        work[k] = vram_read();
        vram_write(k);
    }
    for (k = 0; k < VRAM_BANKS; ++k) {
        SB_BYTE_AT(REG_VRAM_BANK) = k;
        value = vram_read();
        if (value < VRAM_BANKS) {
            work[VRAM_BANKS + value] = 1;
        }
    }
    for (k = VRAM_BANKS; k-- > 0;) {
        SB_BYTE_AT(REG_VRAM_BANK) = k;
        vram_write(work[k]);
    }
    vram_distinct = count_set(VRAM_BANKS, VRAM_BANKS);
}

/* CPU 地址空间里 count 个相隔 step 的单元：各写不同的值，读回后数不同的值（镜像会让它们合并） */
static unsigned char test_mirror(unsigned int base, unsigned int step, unsigned char count)
{
    unsigned char i;
    unsigned char value;
    unsigned int address;

    memset(work + 8, 0, 8);
    for (i = 0; i < count; ++i) {
        address = base + (unsigned int)i * step;
        work[i] = SB_BYTE_AT(address);
        SB_BYTE_AT(address) = 0xA0 + i;
    }
    for (i = 0; i < count; ++i) {
        address = base + (unsigned int)i * step;
        value = (unsigned char)(SB_BYTE_AT(address) - 0xA0);
        if (value < count) {
            work[8 + value] = 1;
        }
    }
    for (i = count; i-- > 0;) {
        address = base + (unsigned int)i * step;
        SB_BYTE_AT(address) = work[i];
    }
    return count_set(8, count);
}

/* 用 DMA 把扩展 RAM 里的 DMA_BYTES 字节复制到 CPU 地址 dest，模式字节取 mode，$404B 的位 7 取 mid_flag */
static void dma_copy(unsigned char mode, unsigned int dest, unsigned char mid_flag)
{
    unsigned long source = (unsigned long)(unsigned char)(scratch_page - EXT_FIRST_BANK) << 12;
    unsigned int wait;

    SB_BYTE_AT(REG_DMA_SOURCE_BANK) = DMA_SOURCE_EXT_RAM | (unsigned char)((source >> 15) & 0x0F);
    SB_BYTE_AT(REG_DMA_SOURCE_LOW) = (unsigned char)source;
    SB_BYTE_AT(REG_DMA_SOURCE_MID) = ((unsigned char)(source >> 8) & 0x7F) | mid_flag;
    SB_BYTE_AT(REG_DMA_DEST_LOW) = (unsigned char)dest;
    SB_BYTE_AT(REG_DMA_DEST_HIGH) = (unsigned char)(dest >> 8);
    SB_BYTE_AT(REG_DMA_LENGTH_LOW) = DMA_BYTES;
    SB_BYTE_AT(REG_DMA_LENGTH_HIGH) = 0;
    SB_BYTE_AT(REG_DMA_MODE) = mode;
    for (wait = 0; wait != 20000; ++wait) {
        __asm__("nop");
    }
}

/* 试一次 DMA：目标清零，复制后看内容是否等于源页里的样板 */
static unsigned char dma_try(unsigned char mode, unsigned char to_window, unsigned char mid_flag)
{
    unsigned char *target = (unsigned char *)(to_window ? WINDOW_ADDRESS : DMA_DEST_ERAM);

    if (to_window) {
        MAP_WINDOW2(scratch_page + 1);
    }
    memset(target, 0, DMA_BYTES);
    dma_copy(mode, (unsigned int)target, mid_flag);
    return memcmp(target, dma_pattern, DMA_BYTES) == 0;
}

/* 开着 NMI 与显示（DOS 正常状态）时试 DMA：样板经 DOS 服务写入源页，只试目标 ERAM */
static void test_dma_normal(void)
{
    unsigned char saved[DMA_BYTES];
    unsigned char mode;
    unsigned char flag;

    memcpy(saved, (void *)DMA_DEST_ERAM, DMA_BYTES);
    sb_far_write(scratch_page, 0, dma_pattern, DMA_BYTES);
    for (mode = 0; mode < DMA_MODES; ++mode) {
        for (flag = 0; flag < 2; ++flag) {
            dma_ok[0][mode][flag][0] = dma_try(dma_modes[mode], 0, flag ? DMA_MID_FLAG : 0);
        }
    }
    memcpy((void *)DMA_DEST_ERAM, saved, DMA_BYTES);
}

/* 关着 NMI 与中断时试 DMA 的 8 种写法（模式 x $404B 位 7 x 目标） */
static void test_dma(void)
{
    unsigned char saved[DMA_BYTES];
    unsigned char mode;
    unsigned char flag;
    unsigned char window;

    memcpy(saved, (void *)DMA_DEST_ERAM, DMA_BYTES);
    MAP_WINDOW2(scratch_page);
    memcpy((void *)WINDOW_ADDRESS, dma_pattern, DMA_BYTES);
    for (mode = 0; mode < DMA_MODES; ++mode) {
        for (flag = 0; flag < 2; ++flag) {
            for (window = 0; window < 2; ++window) {
                dma_ok[1][mode][flag][window] = dma_try(dma_modes[mode], window, flag ? DMA_MID_FLAG : 0);
            }
        }
    }
    memcpy((void *)DMA_DEST_ERAM, saved, DMA_BYTES);
}

static void run_tests(void)
{
    unsigned char start_page = SB_BYTE_AT(PROGRAM_START_PAGE);

    test_dma_normal();
    sb_display_off();
    SB_BYTE_AT(REG_PPU_CTRL) = 0;
    __asm__("php");
    __asm__("sei");

    test_ext_ram();
    test_rom();
    test_dma();
    ram_distinct = test_mirror(0x07F8, 0x0800, 4);
    eram_distinct = test_mirror(0x63F8, 0x0400, 8);
    test_ext_vram();

    MAP_WINDOW2(start_page + 2);
    SB_BYTE_AT(REG_VRAM_BANK) = 0;
    __asm__("plp");
    sb_display_on();
}

static void print_count(const char *label, unsigned int value, const char *tail)
{
    sb_print(label);
    sb_print_uint(value);
    sb_print(tail);
    sb_newline();
}

static void show_results(void)
{
    sb_print("Ext RAM: ");
    sb_print_uint(ext_distinct);
    sb_print("/128 banks = ");
    sb_print_uint((unsigned int)ext_distinct * 4);
    sb_print("K");
    sb_newline();
    sb_print("  alias ");
    sb_print_uint(ext_alias);
    sb_print(", odd ");
    sb_print_uint(ext_unexpected);
    sb_newline();
    sb_print("ROM: ");
    sb_print_uint(rom_distinct);
    sb_print("/128 distinct, ");
    sb_print_uint(rom_blank);
    sb_print(" blank");
    sb_newline();
    print_count("  repeats every ", rom_period, " banks");
    sb_print("Ext VRAM: ");
    sb_print_uint(vram_distinct);
    sb_print("/32 banks = ");
    sb_print_uint((unsigned int)vram_distinct * 32);
    sb_print("K");
    sb_newline();
    print_count("RAM $0000 x4 distinct: ", ram_distinct, "");
    print_count("ERAM $6000 x8 distinct: ", eram_distinct, "");
    sb_print("Press any key (1/2)");
    sb_getkey();
}

/* 第二屏：DMA 各种写法的结果，每行“阶段 模式 E:位7=0位7=1 W:位7=0位7=1” */
static void show_dma(void)
{
    unsigned char phase;
    unsigned char mode;
    unsigned char window;
    unsigned char flag;

    for (mode = 0; mode < 12; ++mode) {
        sb_newline();
    }
    sb_puts("DMA ok? Y/N");
    sb_puts("N/F=NMI on/off E=ERAM W=win");
    sb_puts("pairs: $404B bit7 = 0, 1");
    for (phase = 0; phase < 2; ++phase) {
        for (mode = 0; mode < DMA_MODES; ++mode) {
            sb_print(phase ? "F " : "N ");
            sb_print_hex(dma_modes[mode]);
            for (window = 0; window < 2; ++window) {
                sb_print(window ? " W:" : " E:");
                for (flag = 0; flag < 2; ++flag) {
                    if (phase == 0 && window == 1) {
                        sb_print("-");
                    } else {
                        sb_print(dma_ok[phase][mode][flag][window] ? "Y" : "N");
                    }
                }
            }
            sb_newline();
        }
    }
    sb_print("Press any key (2/2)");
}

int main(void)
{
    unsigned char i;

    if ((unsigned)&_BSS_RUN__ < PROGRAM_MIN_END || (unsigned)&_BSS_RUN__ + (unsigned)&_BSS_SIZE__ > PROGRAM_LIMIT) {
        sb_puts(MSG_SIZE);
        sb_getkey();
        return 0;
    }
    if (sb_page_alloc(2, &scratch_page) != 0) {
        sb_puts(MSG_MEMORY);
        sb_getkey();
        return 0;
    }
    for (i = 0; i < DMA_BYTES; ++i) {
        dma_pattern[i] = 0x31 + i;
    }
    sb_puts(MSG_WAIT);
    run_tests();
    sb_page_free(scratch_page, 2);
    show_results();
    show_dma();
    sb_getkey();
    return 0;
}
