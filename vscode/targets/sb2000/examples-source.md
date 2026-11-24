# SB2000 示例程序源码

本文由示例项目的源码生成。
共 11 个示例，每个是一个 FC Dev 项目：目录里有 `fcdev.json`（`target` 是 `sb2000`，`name` 是 DOS 里的程序名）和 `src/` 下的一个源文件，下面给出完整源码。每个示例做什么、怎么用见 [SB2000 开发指南](../../docs/sb2000/01-开发指南.md) 第 10 节，源码开头的注释也有说明。

把源码放进新建的项目后，用 FC Dev 任务构建（`command` 为 `build`）、另外生成放有程序的软盘镜像（`disk`）、在 Ember 里运行（`run`），见 [构建](../../docs/01-构建.md) 与 [模拟器与调试](../../docs/02-模拟器与调试.md)。

## 目录

- [hello](#hello)
- [say](#say)
- [show](#show)
- [copy](#copy)
- [peek](#peek)
- [poke](#poke)
- [pad](#pad)
- [mouse](#mouse)
- [play](#play)
- [newcall](#newcall)
- [syscall](#syscall)

## hello

程序名 `HELLO`。

`src/hello.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：hello.c
 * 说明：最小的 SB-DOS 程序：显示英文与中文各一行，等待按键后返回 DOS。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：源文件用 UTF-8 保存，字符串里的汉字构建时自动转成学习机用的 GB2312；
 *       返回 DOS 后 DOS 会清屏，所以显示后先等待按键。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

int main(void)
{
    sb_puts("Hello, SB2000!");

    sb_puts("你好，小霸王！");

    sb_getkey();
    return 0;
}
```

## say

程序名 `SAY`。

`src/say.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：say.c
 * 说明：显示命令行参数的 SB-DOS 程序：在 DOS 下输入“say hello world”，屏幕上显示 HELLO WORLD。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：DOS 把程序名之后的部分转成大写，以零结尾放在扩展 RAM 的 SB_CMD_TAIL 起，第一个字节是分隔用的空格；
 *       没有参数时第一个字节为 0，之后的内容是上一条命令的残留，不能显示。
 *       字符串要在程序窗口里才能交给 sb_puts，所以先复制进程序自己的缓冲。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 参数缓冲，最多显示 39 个字符 */
static char text[40];

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char count = 0;

    if (tail[0] != 0) {
        while (count < sizeof(text) - 1 && tail[count + 1] != 0) {
            text[count] = tail[count + 1];
            ++count;
        }
        text[count] = 0;
        sb_puts(text);
    }

    sb_getkey();
    return 0;
}
```

## show

程序名 `SHOW`。

`src/show.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：show.c
 * 说明：显示文件开头内容的 SB-DOS 程序：在 DOS 下输入“show hello.txt”，显示文件的前 90 个字节（每行 30 个字符，共 3 行）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：演示文件接口的标准顺序：取命令行参数 → 打开 → 取大小 → 读入 → 关闭；每个调用都检查出错码，出错时先关闭文件再显示“ERROR 码”。
 *       控制字符显示成“.”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 一次最多显示的字节数 */
#define SHOW_BYTES 90
/** 每行显示的字符数 */
#define LINE_CHARS 30

static char path[29];
static unsigned char data[SHOW_BYTES];
static char line[LINE_CHARS + 1];

/** 从命令行取下一个以空格分隔的词放进 out，没有词时返回 0 */
static unsigned char next_word(const char **cursor, char *out, unsigned char size)
{
    const char *p = *cursor;
    unsigned char count = 0;

    while (*p == ' ') {
        ++p;
    }
    *cursor = p;
    if (*p == 0) {
        return 0;
    }
    while (*p != 0 && *p != ' ') {
        if (count < size - 1) {
            out[count++] = *p;
        }
        ++p;
    }
    out[count] = 0;
    *cursor = p;
    return 1;
}

/** 显示“ERROR 码”并等待按键 */
static void report_error(unsigned char code)
{
    sb_cursor_set_column(1);
    sb_print("ERROR ");
    sb_print_hex(code);
    sb_newline();
    sb_getkey();
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char handle;
    unsigned char error;
    unsigned long size;
    unsigned int count = 0;
    unsigned int start;
    unsigned int n;

    if (!next_word(&tail, path, sizeof(path))) {
        sb_puts("USAGE: SHOW FILENAME");
        sb_getkey();
        return 0;
    }

    error = sb_file_open(path, SB_MODE_OPEN, &handle);
    if (error != 0) {
        report_error(error);
        return 0;
    }

    error = sb_file_size(handle, &size);
    if (error == 0 && size > 0) {
        count = size > SHOW_BYTES ? SHOW_BYTES : (unsigned int)size;
        error = sb_file_read(handle, 0, data, count);
    }
    sb_file_close(handle);
    if (error != 0) {
        report_error(error);
        return 0;
    }

    if (count == 0) {
        sb_puts("(EMPTY FILE)");
    }
    for (start = 0; start < count; start += LINE_CHARS) {
        n = count - start < LINE_CHARS ? count - start : LINE_CHARS;
        for (error = 0; error < n; ++error) {
            line[error] = data[start + error] < 0x20 ? '.' : data[start + error];
        }
        line[n] = 0;
        sb_puts(line);
    }

    sb_getkey();
    return 0;
}
```

## copy

程序名 `CP`。

`src/copy.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：copy.c
 * 说明：复制文件的 SB-DOS 程序：在 DOS 下输入“cp 源文件 目标文件”，把源文件整个复制成目标文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：程序名取 CP，避开 DOS 内部的 COPY 命令。
 *       演示整个文件的加载与保存：sb_file_load 把源文件读入新分配的扩展 RAM 页，sb_file_save 把这些页写成目标文件（已存在则覆盖），
 *       最后释放页。出错时也要释放页，再显示“ERROR 码”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

static char source[29];
static char target[29];

/** 从命令行取下一个以空格分隔的词放进 out，没有词时返回 0 */
static unsigned char next_word(const char **cursor, char *out, unsigned char size)
{
    const char *p = *cursor;
    unsigned char count = 0;

    while (*p == ' ') {
        ++p;
    }
    *cursor = p;
    if (*p == 0) {
        return 0;
    }
    while (*p != 0 && *p != ' ') {
        if (count < size - 1) {
            out[count++] = *p;
        }
        ++p;
    }
    out[count] = 0;
    *cursor = p;
    return 1;
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char page = 0;
    unsigned char pages;
    unsigned char error;
    unsigned long size = 0;

    if (!next_word(&tail, source, sizeof(source)) || !next_word(&tail, target, sizeof(target))) {
        sb_puts("USAGE: CP SRC DST");
        sb_getkey();
        return 0;
    }

    error = sb_file_load(source, &page, &size);
    if (error == 0) {
        error = sb_file_save(target, page, size);
    }
    /* 空文件没有分配页，页号为 0 */
    pages = (unsigned char)((size + 4095UL) / 4096UL);
    if (page != 0 && pages != 0) {
        sb_page_free(page, pages);
    }

    if (error == 0) {
        sb_puts("COPY OK");
    } else {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
    }
    sb_getkey();
    return 0;
}
```

## peek

程序名 `PEEK`。

`src/peek.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：peek.c
 * 说明：按偏移读文件的 SB-DOS 程序：在 DOS 下输入“peek 文件名 十六进制偏移”，显示文件里从该偏移起的 30 个字节。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：演示按偏移读取 sb_file_read：偏移不小于文件大小、或偏移加长度超过文件大小时出错码为 SB_ERR_BAD_POSITION（0x14）。
 *       省略偏移时从 0 开始；控制字符显示成“.”；出错时先关闭文件再显示“ERROR 码”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 显示的字节数 */
#define SHOW_BYTES 30

static char path[29];
static char offset_text[9];
static unsigned char data[SHOW_BYTES];
static char line[SHOW_BYTES + 1];

/** 从命令行取下一个以空格分隔的词放进 out，没有词时返回 0 */
static unsigned char next_word(const char **cursor, char *out, unsigned char size)
{
    const char *p = *cursor;
    unsigned char count = 0;

    while (*p == ' ') {
        ++p;
    }
    *cursor = p;
    if (*p == 0) {
        return 0;
    }
    while (*p != 0 && *p != ' ') {
        if (count < size - 1) {
            out[count++] = *p;
        }
        ++p;
    }
    out[count] = 0;
    *cursor = p;
    return 1;
}

/** 解析大写十六进制数，遇到非十六进制字符停止 */
static unsigned long parse_hex(const char *text)
{
    unsigned long value = 0;
    unsigned char digit;

    for (; *text != 0; ++text) {
        if (*text >= '0' && *text <= '9') {
            digit = *text - '0';
        } else if (*text >= 'A' && *text <= 'F') {
            digit = *text - 'A' + 10;
        } else {
            break;
        }
        value = (value << 4) | digit;
    }
    return value;
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char handle;
    unsigned char error;
    unsigned char i;
    unsigned long offset = 0;

    if (!next_word(&tail, path, sizeof(path))) {
        sb_puts("USAGE: PEEK FILE HEXOFFSET");
        sb_getkey();
        return 0;
    }
    if (next_word(&tail, offset_text, sizeof(offset_text))) {
        offset = parse_hex(offset_text);
    }

    error = sb_file_open(path, SB_MODE_OPEN, &handle);
    if (error != 0) {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
        sb_getkey();
        return 0;
    }
    error = sb_file_read(handle, offset, data, SHOW_BYTES);
    sb_file_close(handle);

    if (error != 0) {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
    } else {
        for (i = 0; i < SHOW_BYTES; ++i) {
            line[i] = data[i] < 0x20 ? '.' : data[i];
        }
        line[SHOW_BYTES] = 0;
        sb_puts(line);
    }
    sb_getkey();
    return 0;
}
```

## poke

程序名 `POKE`。

`src/poke.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：poke.c
 * 说明：按偏移写文件的 SB-DOS 程序：在 DOS 下输入“poke 文件名 十六进制偏移 文字”，把文字（大写）写进已有文件的该偏移处。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：演示按偏移写入 sb_file_write：以 SB_MODE_UPDATE 打开已有文件，写入范围不能超出文件当前大小（否则出错码 0x14），
 *       关闭时才把改动写回目录项。文字最长 60 个字符；出错时先关闭文件再显示“ERROR 码”。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 文字的最大长度 */
#define TEXT_MAX 60

static char path[29];
static char offset_text[9];
static char text[TEXT_MAX + 1];

/** 从命令行取下一个以空格分隔的词放进 out，没有词时返回 0 */
static unsigned char next_word(const char **cursor, char *out, unsigned char size)
{
    const char *p = *cursor;
    unsigned char count = 0;

    while (*p == ' ') {
        ++p;
    }
    *cursor = p;
    if (*p == 0) {
        return 0;
    }
    while (*p != 0 && *p != ' ') {
        if (count < size - 1) {
            out[count++] = *p;
        }
        ++p;
    }
    out[count] = 0;
    *cursor = p;
    return 1;
}

/** 解析大写十六进制数，遇到非十六进制字符停止 */
static unsigned long parse_hex(const char *digits)
{
    unsigned long value = 0;
    unsigned char digit;

    for (; *digits != 0; ++digits) {
        if (*digits >= '0' && *digits <= '9') {
            digit = *digits - '0';
        } else if (*digits >= 'A' && *digits <= 'F') {
            digit = *digits - 'A' + 10;
        } else {
            break;
        }
        value = (value << 4) | digit;
    }
    return value;
}

int main(void)
{
    const char *tail = (const char *)SB_CMD_TAIL;
    unsigned char handle;
    unsigned char error;
    unsigned char length = 0;

    if (!next_word(&tail, path, sizeof(path)) || !next_word(&tail, offset_text, sizeof(offset_text))) {
        sb_puts("USAGE: POKE FILE HEXOFF TEXT");
        sb_getkey();
        return 0;
    }
    while (*tail == ' ') {
        ++tail;
    }
    while (*tail != 0 && length < TEXT_MAX) {
        text[length++] = *tail++;
    }
    text[length] = 0;
    if (length == 0) {
        sb_puts("USAGE: POKE FILE HEXOFF TEXT");
        sb_getkey();
        return 0;
    }

    error = sb_file_open(path, SB_MODE_UPDATE, &handle);
    if (error == 0) {
        error = sb_file_write(handle, parse_hex(offset_text), text, length);
        sb_file_close(handle);
    }

    if (error == 0) {
        sb_puts("POKE OK");
    } else {
        sb_cursor_set_column(1);
        sb_print("ERROR ");
        sb_print_hex(error);
        sb_newline();
    }
    sb_getkey();
    return 0;
}
```

## pad

程序名 `PAD`。

`src/pad.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：pad.c
 * 说明：测试手柄服务的 SB-DOS 程序：初始化后等待手柄按键，显示键码与两个手柄的状态，再等键盘按键后返回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：显示格式：KEY xx P1 xx P2 xx，三个值依次是键码、1 号手柄与 2 号手柄的原始状态（位 7 为 A，位 0 为右）。
 *       在模拟器里用调试命令“pad 0 01”按下 1 号手柄 A，松开用“pad 0 clear”。
 *       从键盘启动程序时，回车键在启动后的一小段时间里还按着，模拟器把它当成手柄的开始键，所以先等 120 帧再初始化。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

int main(void)
{
    unsigned char key;

    sb_wait_frames(120);
    sb_pad_init();
    sb_puts("PRESS A PAD BUTTON");

    key = sb_pad_wait();
    sb_print("KEY ");
    sb_print_hex(key);
    sb_print(" P1 ");
    sb_print_hex(sb_pad_state(0));
    sb_print(" P2 ");
    sb_print_hex(sb_pad_state(1));
    sb_newline();

    sb_getkey();
    return 0;
}
```

## mouse

程序名 `MOUSE`。

`src/mouse.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：mouse.c
 * 说明：测试鼠标服务的 SB-DOS 程序：复位鼠标、设置坐标范围与初始坐标，等 250 帧后取坐标并显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：在模拟器里等程序显示“MOVE THE MOUSE”后，用调试命令“mouse move dx dy”移动鼠标，“mouse btn 掩码”注入按键。
 *       显示格式：X xx Y xx（十六进制），初始坐标是 X=80、Y=40；没有检测到鼠标时显示 NO MOUSE。
 *       第二行 B 后面依次是两个键的状态、双击、单击、拖动共 8 个字节。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

int main(void)
{
    unsigned char x;
    unsigned char y;
    unsigned char i;
    sb_mouse_buttons_t buttons;

    if (!sb_mouse_reset()) {
        sb_puts("NO MOUSE");
        sb_getkey();
        return 0;
    }
    /* 鼠标中断要先允许，再设范围与初始坐标 */
    sb_irq_enable(SB_IRQ_SOURCE_MOUSE);
    sb_mouse_set_limits(0, 0, 0xFF, 0xFF);
    sb_mouse_set_position(0x80, 0x40);

    sb_puts("MOVE THE MOUSE");
    sb_wait_frames(250);

    sb_mouse_position(&x, &y);
    sb_mouse_buttons(&buttons);
    sb_print("X ");
    sb_print_hex(x);
    sb_print(" Y ");
    sb_print_hex(y);
    sb_newline();

    sb_print("B");
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.state[i]);
    }
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.double_click[i]);
    }
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.click[i]);
    }
    for (i = 0; i < 2; ++i) {
        sb_print(" ");
        sb_print_hex(buttons.drag[i]);
    }
    sb_newline();

    sb_getkey();
    return 0;
}
```

## play

程序名 `PLAY`。

`src/play.s`：

```text
; ------------------------------------------------------------------------
; 名称：play.s
; 说明：在 DOS 下运行 .nes 游戏：play 文件名.nes，支持 mapper 0 与 mapper 4（MMC3），PRG、CHR 各不超过 128K。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-11
; 备注：用 FC Dev 构建任务构建 src 目录（程序入口是导出的 _main，由启动代码调用）；游戏文件名放在命令行，如 play mario.nes
;       流程：取参数里的文件名 -> 打开 -> 取大小（超过 256K+16 就按 256K+16 读，文件末尾可能带说明文字）-> 分配页 -> 读入 -> 关闭
;       -> 检查 iNES 头 -> 关 NMI -> PRG 先用 DMA 复制到 ERAM 缓冲 $6800-$77FF，再把目的页映射到 $B000 后用 CPU 复制过去，CHR 用 DMA 复制到扩展 RAM、扩展显存的第 3 个 128K 窗口（$5006=$FF）
;       -> 把 PRG 里的 RESET 向量改为 ERAM $6100 的启动桩 -> 在 ERAM $6000 写 $5006、$4301 进入 FC 兼容模式。
;       放置方式：MMC3 仿真下 $C000-$FFFF 固定为窗口末尾 16K，$8000-$BFFF 由 R6/R7 选 8K 页（R6/R7 只取低 4 位）。
;       mapper 0：PRG 放在窗口末尾，启动桩把 R6/R7 设为末尾前的两个 8K（32K 为 12、13，16K 为 14、15）；
;       mapper 4：PRG 放在窗口开头（银行号直接对应），PRG 不足 128K 时再把最后 16K 复制到窗口末尾，R6/R7 设为 0、1，之后游戏自己设置。
;       CHR 放在扩展显存同一窗口开头，R0-R5 设为 0、2、4、5、6、7（与 CHR 顺序一致）。
;       DMA 寄存器（$4048-$404F）：$4049 低 4 位与 bit4 为源（bit4=1 表示扩展 RAM），$404A/$404B 为源偏移（$404B 位 7 必须为 1，否则 DMA 不传输），
;       $404C/$404D 为目的，$404E/$404F 为长度（字节数减 1）；$4048 写 $E0 为复制到 CPU 地址（目的在 $8000 以上写不进去，所以只用来写 ERAM 缓冲），$C0 为复制到显存（目的 bit15 置位表示扩展显存，再加 $4300 选的 32K bank）。
;       运行变量（ERAM）：$6700 文件页数、$6701 文件首页、$6702 PRG 个数（16K）、$6703 CHR 个数（8K）、$6704 iNES 标志 6、
;       $6705 标志 7 高 4 位、$6706 PRG 页数（4K）、$6707 复制序号、$6709/$670A DMA 目的、$670B/$670C DMA 长度、
;       $670F 当前目的页、$6710/$6711 游戏原 RESET 入口、$6712 mapper、$6713 CHR 4K 块数、$6714 PRG 首个目的页、$6715-$6717 DMA 源偏移。
; ------------------------------------------------------------------------
    .export _main
    .segment "ZEROPAGE"
copy_src:
    .res 2
copy_dst:
    .res 2
    .segment "CODE"

_main:
start:
    LDA #$01
    STA $630C
    JSR copy_name
    LDA $6543
    BNE name_ok
    LDX #<msg_usage
    LDY #>msg_usage
    JMP fail
name_ok:
    LDA #$00
    STA $6560
    LDA #$7B
    LDX #$09
    JSR sys
    LDA $6300
    BEQ opened
    LDX #<msg_open
    LDY #>msg_open
    JMP fail
opened:
    LDA #$7C
    LDX #$0D
    JSR sys
    JSR clamp_size
    ; 页数 = (大小 + 4095) / 4096，大小在 $6568-$656A（小端）
    LDA $656A
    ASL A
    ASL A
    ASL A
    ASL A
    STA $6700
    LDA $6569
    LSR A
    LSR A
    LSR A
    LSR A
    ORA $6700
    STA $6700
    LDA $6569
    AND #$0F
    ORA $6568
    BEQ whole_pages
    INC $6700
whole_pages:
    LDA $6700
    STA $65A6
    LDA #$7E
    LDX #$00
    JSR sys
    LDA $6300
    BEQ allocated
no_memory:
    LDX #<msg_mem
    LDY #>msg_mem
    JMP fail
allocated:
    LDA $65A7
    STA $656B
    STA $6701
    ; 文件所在页必须全部低于目的窗口（页 $E0 起）
    CLC
    ADC $6700
    CMP #$E1
    BCS no_memory
    LDA #$7C
    LDX #$0F
    JSR sys
    LDA $6300
    PHA
    LDA #$7B
    LDX #$0A
    JSR sys
    PLA
    BEQ loaded
    LDX #<msg_read
    LDY #>msg_read
    JMP fail
loaded:
    ; 把文件第一页映射到 $B000，检查 iNES 头
    LDA $6701
    JSR map_b000
    LDA $B000
    CMP #$4E
    BNE bad_header
    LDA $B001
    CMP #$45
    BNE bad_header
    LDA $B002
    CMP #$53
    BNE bad_header
    LDA $B003
    CMP #$1A
    BEQ header_ok
bad_header:
    LDX #<msg_header
    LDY #>msg_header
    JMP fail
header_ok:
    LDA $B004
    STA $6702
    LDA $B005
    STA $6703
    LDA $B006
    STA $6704
    LDA $B007
    AND #$F0
    STA $6705
    LDA $6704
    LSR A
    LSR A
    LSR A
    LSR A
    ORA $6705
    STA $6712
    JSR check_header
    BCC size_ok
    LDX $6720
    LDY $6721
    JMP fail
size_ok:
    ; PRG 页数（每 16K 为 4 页）与首个目的页：mapper 0 放窗口末尾，mapper 4 放窗口开头
    LDA $6702
    ASL A
    ASL A
    STA $6706
    LDA #$E0
    LDX $6712
    BNE prg_start_set
    LDA #$00
    SEC
    SBC $6706
prg_start_set:
    STA $6714
    ; 关 NMI 并禁止中断：此后不再返回 DOS，DOS 的显示与键盘不应再运行
    SEI
    LDA #$00
    STA $2000
    STA $6707
prg_loop:
    LDA $6701
    SEC
    SBC #$80
    CLC
    ADC $6707
    LDY $6714
    JSR copy_page_k
    INC $6707
    LDA $6707
    CMP $6706
    BNE prg_loop
    ; mapper 4 的 PRG 不足 128K 时，把最后 16K 再复制到窗口末尾（页 $FC-$FF）
    LDA $6712
    BEQ prg_done
    LDA $6706
    CMP #$20
    BEQ prg_done
    LDA #$00
    STA $6707
tail_loop:
    LDA $6701
    SEC
    SBC #$80
    CLC
    ADC $6706
    SEC
    SBC #$04
    CLC
    ADC $6707
    LDY #$FC
    JSR copy_page_k
    INC $6707
    LDA $6707
    CMP #$04
    BNE tail_loop
prg_done:
    ; 窗口最后一页（$FF）里有向量：保存原 RESET 入口，再改为 $6100
    LDA #$FF
    JSR map_b000
    LDA $BFFC
    STA $6710
    LDA $BFFD
    STA $6711
    LDA #$00
    STA $BFFC
    LDA #$61
    STA $BFFD
    ; CHR 紧接在 PRG 之后：每 4K 一块，源页序号 = 起始页 - $80 + PRG 页数 + 块号
    LDA $6703
    ASL A
    STA $6713
    LDA #$00
    STA $6707
chr_loop:
    LDA $6701
    SEC
    SBC #$80
    CLC
    ADC $6706
    CLC
    ADC $6707
    JSR set_source
    ; 目的 = $8000 + (块号 mod 8) * $1000，显存 bank = 12 + 块号 / 8（第 3 个窗口从 32K 单位 12 开始）
    LDA $6707
    LSR A
    LSR A
    LSR A
    CLC
    ADC #$0C
    STA $4300
    LDA $6707
    AND #$07
    ASL A
    ASL A
    ASL A
    ASL A
    ORA #$80
    STA $670A
    LDA #$00
    STA $6709
    LDA #$FF
    STA $670B
    LDA #$0F
    STA $670C
    LDA #$C0
    JSR dma
    INC $6707
    LDA $6707
    CMP $6713
    BNE chr_loop
    ; 把启动桩复制到 ERAM
    LDX #$00
copy_trampoline:
    LDA trampoline_image,X
    STA $6000,X
    INX
    CPX #$0C
    BNE copy_trampoline
    LDX #$00
copy_stub:
    LDA stub_image,X
    STA $6100,X
    INX
    CPX #$30
    BNE copy_stub
    ; 启动桩的参数：原 RESET 入口、镜像（iNES 标志位 0 为 1 表示垂直，MMC3 的 $A000 写 0 为垂直）与 PRG 银行
    LDA $6710
    STA $611B
    LDA $6711
    STA $611C
    LDA $6704
    AND #$01
    EOR #$01
    STA $6116
    LDA $6712
    BNE regs_mapper4
    LDA #$0C
    LDX $6702
    CPX #$01
    BNE prg_regs
    LDA #$0E
    JMP prg_regs
regs_mapper4:
    LDA #$00
prg_regs:
    STA $612A
    CLC
    ADC #$01
    STA $612C
    JMP $6000

; 文件大小超过 $040010 时按 $040010 读取（128K PRG + 128K CHR + 头）
clamp_size:
    LDA $656A
    CMP #$04
    BCC clamp_done
    BNE clamp_set
    LDA $6569
    BNE clamp_set
    LDA $6568
    CMP #$11
    BCC clamp_done
clamp_set:
    LDA #$10
    STA $6568
    LDA #$00
    STA $6569
    LDA #$04
    STA $656A
clamp_done:
    RTS

; 检查 iNES 头：可运行则清进位返回；否则置进位，信息指针放在 $6720/$6721
check_header:
    LDA $6704
    AND #$04
    BNE check_size_bad
    LDA $6703
    BEQ check_size_bad
    CMP #$11
    BCS check_size_bad
    LDA $6702
    BEQ check_size_bad
    CMP #$09
    BCS check_size_bad
    LDA $6712
    BEQ check_mapper0
    CMP #$04
    BEQ check_ok
    LDA #<msg_mapper
    STA $6720
    LDA #>msg_mapper
    STA $6721
    SEC
    RTS
check_mapper0:
    LDA $6703
    CMP #$01
    BNE check_size_bad
    LDA $6702
    CMP #$03
    BCS check_size_bad
check_ok:
    CLC
    RTS
check_size_bad:
    LDA #<msg_size
    STA $6720
    LDA #>msg_size
    STA $6721
    SEC
    RTS

; 复制一个 4K 页：A 为源页序号（相对 $80），Y 为目的起始页，$6707 为序号（目的页 = Y + 序号）
copy_page_k:
    PHA
    TYA
    CLC
    ADC $6707
    STA $670F
    PLA
copy_page_do:
    JSR set_source
    ; 先 DMA 到 ERAM $6800 的缓冲：目的在 $8000 以上（程序窗口）时 DMA 写不进去
    LDA #$00
    STA $6709
    LDA #$68
    STA $670A
    LDA #$FF
    STA $670B
    LDA #$0F
    STA $670C
    LDA #$E0
    JSR dma
    ; 再把目的页映射到 $B000，用 CPU 从缓冲复制 16 个 256 字节
    LDA $670F
    JSR map_b000
    LDA #$00
    STA copy_src
    STA copy_dst
    LDA #$68
    STA copy_src+1
    LDA #$B0
    STA copy_dst+1
    LDX #$10
    LDY #$00
copy_loop:
    LDA (copy_src),Y
    STA (copy_dst),Y
    INY
    BNE copy_loop
    INC copy_src+1
    INC copy_dst+1
    DEX
    BNE copy_loop
    RTS

; 取参数里的第一个词作为文件路径，放到 $6543；$6364 为 0 表示没有参数（$6365 起可能有上一条命令的残留）
copy_name:
    LDX #$00
skip_blank:
    LDA $6364,X
    BEQ copy_name_none
    CMP #$20
    BNE copy_name_start
    INX
    BNE skip_blank
copy_name_none:
    LDA #$00
    STA $6543
    RTS
copy_name_start:
    LDY #$00
copy_name_loop:
    LDA $6364,X
    BEQ copy_name_end
    CMP #$20
    BEQ copy_name_end
    STA $6543,Y
    INX
    INY
    CPY #$30
    BNE copy_name_loop
copy_name_end:
    LDA #$00
    STA $6543,Y
    RTS

; 把页 A 映射到窗口 3（$B000）
map_b000:
    STA $0641
    LDA #$03
    STA $0653
    LDA #$29
    LDX #$00
    JMP sys

; 设置 DMA 源偏移 = A * $1000 + 16（A 是以 4K 为单位的页序号）
set_source:
    PHA
    ASL A
    ASL A
    ASL A
    ASL A
    STA $6716
    PLA
    LSR A
    LSR A
    LSR A
    LSR A
    STA $6717
    LDA #$10
    STA $6715
    RTS

; 启动 DMA：A 为 $4048 的值，源偏移在 $6715-$6717，目的在 $6709/$670A，长度在 $670B/$670C
dma:
    PHA
    LDA $6716
    ASL A
    LDA $6717
    ROL A
    AND #$0F
    ORA #$10
    STA $4049
    LDA $6715
    STA $404A
    LDA $6716
    AND #$7F
    ORA #$80
    STA $404B
    LDA $6709
    STA $404C
    LDA $670A
    STA $404D
    LDA $670B
    STA $404E
    LDA $670C
    STA $404F
    PLA
    STA $4048
    RTS

; 失败：显示 Y:X 指向的信息，等待按键后返回 DOS
fail:
    STX $4B
    STY $4C
    LDA #$80
    STA $6308
    LDA #$62
    LDX #$82
    JSR rom_call
    LDA #$01
    STA $630C
    LDA #$E2
    LDX #$80
    JSR rom_call
    LDA #$6B
    LDX #$04
    JSR sys
    CLC
    RTS

; 经功能 $25 调用 ROM 页组 0 的 X:A（返回后回到调用者）
rom_call:
    STA $0C
    STX $0D
    LDA #$25
    STA $063E
    LDA #$00
    STA $0642
    JMP $FFF0

; 调用系统功能：A 为功能号，X 为子功能号
sys:
    STA $063E
    STX $063F
    JMP $FFF0

; Usage: PLAY FILE.NES
msg_usage:
    .byte $55,$73,$61,$67,$65,$3A,$20,$50,$4C,$41,$59,$20,$46,$49,$4C,$45,$2E,$4E,$45,$53,$00
; Can't open file
msg_open:
    .byte $43,$61,$6E,$27,$74,$20,$6F,$70,$65,$6E,$20,$66,$69,$6C,$65,$00
; No memory
msg_mem:
    .byte $4E,$6F,$20,$6D,$65,$6D,$6F,$72,$79,$00
; Read error
msg_read:
    .byte $52,$65,$61,$64,$20,$65,$72,$72,$6F,$72,$00
; Not an iNES file
msg_header:
    .byte $4E,$6F,$74,$20,$61,$6E,$20,$69,$4E,$45,$53,$20,$66,$69,$6C,$65,$00
; Mapper not supported
msg_mapper:
    .byte $4D,$61,$70,$70,$65,$72,$20,$6E,$6F,$74,$20,$73,$75,$70,$70,$6F,$72,$74,$65,$64,$00
; Size not supported
msg_size:
    .byte $53,$69,$7A,$65,$20,$6E,$6F,$74,$20,$73,$75,$70,$70,$6F,$72,$74,$65,$64,$00

; 以下两段在运行时复制到 ERAM（$6000 的跳板、$6100 的启动桩）；它们按 ERAM 地址取值，ca65 没有 .rorg，所以写成字节并在注释里给出汇编
trampoline_image:
    .byte $A9,$FF            ; LDA #$FF
    .byte $8D,$06,$50        ; STA $5006
    .byte $A9,$55            ; LDA #$55
    .byte $8D,$01,$43        ; STA $4301
    .byte $EA                ; NOP
stub_image:                  ; 汇编地址 $6100
    .byte $78                ; $6100 SEI
    .byte $A2,$00            ; $6101 LDX #$00
    .byte $BD,$1D,$61        ; $6103 stub_loop: LDA stable,X（stable 在 $611D）
    .byte $8D,$00,$80        ; $6106 STA $8000
    .byte $BD,$1E,$61        ; $6109 LDA stable+1,X
    .byte $8D,$01,$80        ; $610C STA $8001
    .byte $E8,$E8            ; $610F INX、INX
    .byte $E0,$10            ; $6111 CPX #$10
    .byte $D0,$EE            ; $6113 BNE stub_loop
    .byte $A9,$00            ; $6115 stub_mirror: LDA #$00（运行时改写第 2 字节）
    .byte $8D,$00,$A0        ; $6117 STA $A000
    .byte $4C,$00,$00        ; $611A stub_jmp: JMP $0000（运行时改写后两个字节）
    .byte $00,$00,$01,$02,$02,$04,$03,$05,$04,$06,$05,$07,$06,$0C,$07,$0D   ; $611D stable：MMC3 寄存器序号与初值
```

## newcall

程序名 `NEWCALL`。

`src/newcall.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：newcall.c
 * 说明：调用 BIOS 新增的系统调用 $35，显示它返回的扩展版本号；BIOS 不支持时显示 NOT SUPPORTED。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：给自己修改过的 BIOS 做功能检测的写法：新系统调用只能经内存返回结果，约定它把主版本写入 SYS_PAGE、次版本写入 SYS_GROUP。
 *       调用前先把这两个单元置 0xFF；原始 BIOS 里功能 $35 是空桩，不会改写它们，据此判断是否支持。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 新增的系统调用号 */
#define FN_BIOS_VERSION 0x35

#define SYS_PAGE (*(volatile unsigned char *)SB_SYS_PAGE)
#define SYS_GROUP (*(volatile unsigned char *)SB_SYS_GROUP)

static char text[] = "BIOS EXT VER 0.0";

/** 低 4 位转成十六进制字符 */
static char hex_digit(unsigned char value)
{
    value &= 0x0F;
    return value < 10 ? '0' + value : 'A' + value - 10;
}

int main(void)
{
    unsigned char major;
    unsigned char minor;

    SYS_PAGE = 0xFF;
    SYS_GROUP = 0xFF;
    sb_syscall(FN_BIOS_VERSION, 0);
    major = SYS_PAGE;
    minor = SYS_GROUP;

    if (major == 0xFF) {
        sb_puts("NOT SUPPORTED");
    } else {
        text[14] = hex_digit(major);
        text[16] = hex_digit(minor);
        sb_puts(text);
    }
    sb_getkey();
    return 0;
}
```

## syscall

程序名 `SYSCALL`。

`src/syscall.c`：

```c
/*
 * ------------------------------------------------------------------------
 * 名称：syscall.c
 * 说明：系统调用实验程序：由调试器把功能号与参数写进 ERAM 的请求块，程序代为调用，用来在模拟器里逐个核对没有示例的系统服务。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：DOS 下输入 syscall 运行，之后一直等待请求，不会退出。
 *       请求块在 $7E00：+0 请求标志（调试器写 1，程序执行完清 0）、+1 功能号、+2 子功能、
 *       +3 起 20 字节写入 $0640-$0653、+$1F 与 +$20 写入 $0C、$0D；调用后 $0640-$0653 复制回 +$30 起，+$44 放 DOS_ERROR。
 * ------------------------------------------------------------------------
 */
#include <sb2000.h>

/** 请求块的起始地址 */
#define BLOCK ((volatile unsigned char *)0x7E00)

/** 参数区 $0640-$0653 的字节数 */
#define PARAM_BYTES 20

#define PARAM_AREA ((volatile unsigned char *)0x0640)
#define PROG_ENTRY ((volatile unsigned char *)0x000C)

int main(void)
{
    unsigned char i;

    BLOCK[0] = 0;
    for (;;) {
        while (BLOCK[0] == 0) {
        }
        for (i = 0; i < PARAM_BYTES; ++i) {
            PARAM_AREA[i] = BLOCK[3 + i];
        }
        PROG_ENTRY[0] = BLOCK[0x1F];
        PROG_ENTRY[1] = BLOCK[0x20];
        sb_syscall(BLOCK[1], BLOCK[2]);
        for (i = 0; i < PARAM_BYTES; ++i) {
            BLOCK[0x30 + i] = PARAM_AREA[i];
        }
        BLOCK[0x44] = *(volatile unsigned char *)SB_DOS_ERROR;
        BLOCK[0] = 0;
    }
}
```
