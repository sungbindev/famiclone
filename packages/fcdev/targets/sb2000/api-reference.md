# SB2000 C 接口参考

本文由头文件 `include/sb2000-*.h` 的文档注释生成，请不要手改；改注释后运行 `node tools/gen-sb2000-api-doc.mjs`。
共 204 个函数，17 个模块。随时查询用 `npx fcdev api`：`npx fcdev api <函数名>` 看完整文档，`npx fcdev api <模块名>` 看模块的函数列表，`npx fcdev api '$7B'` 看用到某个系统调用的函数，`npx fcdev api <词>` 按名称与说明搜索。

约定：

- 所有函数用 `#include <sb2000.h>` 引入；返回 DOS 出错码或软驱状态码的函数，0 表示成功，结果经指针参数带回。
- 字符串、缓冲区、结构体指针必须在程序窗口 `$8000-$BFFF` 里（系统服务换页后程序窗口不可见的，函数内部已经处理好，调用者不用管）。
- 程序最大 16K（窗口 `$8000-$BFFF`，C 软件栈占顶端 1K）。

## 目录

- [bank（sb2000-bank.h）](#bank)
- [日期与时间（sb2000-clock.h）](#clock)
- [软驱扇区（sb2000-disk.h）](#disk)
- [显示、窗口与图片（sb2000-display.h）](#display)
- [通用 DMA（sb2000-dma.h）](#dma)
- [文件与目录（sb2000-file.h）](#file)
- [中文输入法（sb2000-ime.h）](#ime)
- [键盘、手柄与鼠标（sb2000-input.h）](#input)
- [数学（sb2000-math.h）](#math)
- [内存页与远指针（sb2000-memory.h）](#memory)
- [图形端口（sb2000-ppu.h）](#ppu)
- [打印机（sb2000-printer.h）](#printer)
- [声音与语音（sb2000-sound.h）](#sound)
- [系统与中断（sb2000-system.h）](#system)
- [文字输出（sb2000-text.h）](#text)
- [硬件定时器（sb2000-timer.h）](#timer)
- [总头文件与通用调用（sb2000.h）](#sb2000)
- [系统调用覆盖表](#系统调用覆盖表)

<a id="bank"></a>
## bank（sb2000-bank.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_bank_select`](#sb_bank_select) | 把一个库页换进窗口 2，窗口 2 里原来的库页看不到了。调用蹦床时会自动换页，只有需要直接读库页里的数据时才自己调用。 | $29 |
| [`sb_bank_current`](#sb_bank_current) | 取窗口 2 当前的库页号。 |  |

<a id="sb_bank_select"></a>
### sb_bank_select

```c
void sb_bank_select(unsigned char bank);
```

把一个库页换进窗口 2，窗口 2 里原来的库页看不到了。调用蹦床时会自动换页，只有需要直接读库页里的数据时才自己调用。

系统调用：$29

参数：

- `bank`：库页号，1 到 fcdev.json 里 banks 的值。

<a id="sb_bank_current"></a>
### sb_bank_current

```c
unsigned char sb_bank_current(void);
```

取窗口 2 当前的库页号。

返回：库页号；程序刚启动时是 1。

<a id="clock"></a>
## 日期与时间（sb2000-clock.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_clock_set_date`](#sb_clock_set_date) | 设置系统日期。系统按年份能否被 4 整除判断闰年（不处理世纪年）。 | $7F/$01 |
| [`sb_clock_get_date`](#sb_clock_get_date) | 取系统日期。过了午夜后第一次取时，系统先把日期加一天。 | $7F/$02 |
| [`sb_clock_set_time`](#sb_clock_set_time) | 设置系统时间。 | $7F/$03 |
| [`sb_clock_get_time`](#sb_clock_get_time) | 取系统时间。 | $7F/$04 |

<a id="sb_clock_set_date"></a>
### sb_clock_set_date

```c
unsigned char sb_clock_set_date(const sb_date_t *date);
```

设置系统日期。系统按年份能否被 4 整除判断闰年（不处理世纪年）。

系统调用：$7F/$01

参数：

- `date`：新日期。

返回：0 成功；SB_ERR_BAD_PARAMETER 日期不合法，系统日期不变。

<a id="sb_clock_get_date"></a>
### sb_clock_get_date

```c
void sb_clock_get_date(sb_date_t *date);
```

取系统日期。过了午夜后第一次取时，系统先把日期加一天。

系统调用：$7F/$02

参数：

- `date`：返回当前日期。

<a id="sb_clock_set_time"></a>
### sb_clock_set_time

```c
unsigned char sb_clock_set_time(const sb_time_t *time);
```

设置系统时间。

系统调用：$7F/$03

参数：

- `time`：新时间。

返回：0 成功；SB_ERR_BAD_PARAMETER 时间不合法，系统时间不变。

<a id="sb_clock_get_time"></a>
### sb_clock_get_time

```c
void sb_clock_get_time(sb_time_t *time);
```

取系统时间。

系统调用：$7F/$04

参数：

- `time`：返回当前时间。

<a id="disk"></a>
## 软驱扇区（sb2000-disk.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_disk_init`](#sb_disk_init) | 启动软驱并复位到 0 磁道，读出状态后停转。可用来检查驱动器是否就绪。 | $80/$00 |
| [`sb_disk_start`](#sb_disk_start) | 启动软驱。文件服务会自己启动与停转软驱，只有直接读写扇区时才需要。 | $80/$06 |
| [`sb_disk_stop`](#sb_disk_stop) | 让软驱停转。直接读写扇区之后调用，文件服务每次操作之后都会自己调用。 | $80/$07 |
| [`sb_disk_read_sector`](#sb_disk_read_sector) | 读一个扇区，先检测盘片。 | $80/$02 |
| [`sb_disk_write_sector`](#sb_disk_write_sector) | 写一个扇区，先检测盘片。 | $80/$03 |
| [`sb_disk_read_sectors`](#sb_disk_read_sectors) | 从起始扇区连续读若干扇区到扩展 RAM 页里，扇区号递增，满 18 换面，两面满换磁道。先检测盘片。 | $80/$04 |
| [`sb_disk_write_sectors`](#sb_disk_write_sectors) | 把扩展 RAM 页里的数据连续写到若干扇区，扇区号递增的规则同 sb_disk_read_sectors。先检测盘片，写保护时立即停止。 | $80/$05 |
| [`sb_disk_scan_track`](#sb_disk_scan_track) | 逐个读一个磁道的 18 个扇区，读不出的在位图里置位。不检测盘片，直接读每个扇区。 | $80/$08 |
| [`sb_disk_format_track`](#sb_disk_format_track) | 格式化一个磁道：磁道 0 的 18 个扇区填 $00，其他磁道填 $F6。先检测盘片。会清掉这个磁道上的全部数据。 | $80/$01 |
| [`sb_disk_free_bytes`](#sb_disk_free_bytes) | 取磁盘上的空闲字节数（空闲簇数乘每簇字节数）。 | $7B/$01、$7B/$06 |

<a id="sb_disk_init"></a>
### sb_disk_init

```c
unsigned char sb_disk_init(void);
```

启动软驱并复位到 0 磁道，读出状态后停转。可用来检查驱动器是否就绪。

系统调用：$80/$00

返回：0 成功；SB_ERR_NOT_READY 驱动器没有响应。

<a id="sb_disk_start"></a>
### sb_disk_start

```c
unsigned char sb_disk_start(void);
```

启动软驱。文件服务会自己启动与停转软驱，只有直接读写扇区时才需要。

系统调用：$80/$06

返回：0 成功；SB_ERR_NOT_READY 驱动器未准备好。

<a id="sb_disk_stop"></a>
### sb_disk_stop

```c
void sb_disk_stop(void);
```

让软驱停转。直接读写扇区之后调用，文件服务每次操作之后都会自己调用。

系统调用：$80/$07

<a id="sb_disk_read_sector"></a>
### sb_disk_read_sector

```c
unsigned char sb_disk_read_sector(unsigned char head, unsigned char track, unsigned char sector, void *dest);
```

读一个扇区，先检测盘片。

系统调用：$80/$02

参数：

- `head`：面，0 或 1。
- `track`：磁道，0 到 79。
- `sector`：扇区号，1 到 18。
- `dest`：接收 512 字节的缓冲区，在程序窗口里。

返回：0 成功；SB_ERR_NO_DISK 驱动器中没有磁盘；SB_ERR_UNKNOWN_DISK 未知磁盘类型；SB_ERR_CRC 扇区校验错。

<a id="sb_disk_write_sector"></a>
### sb_disk_write_sector

```c
unsigned char sb_disk_write_sector(unsigned char head, unsigned char track, unsigned char sector, const void *src);
```

写一个扇区，先检测盘片。

系统调用：$80/$03

参数：

- `head`：面，0 或 1。
- `track`：磁道，0 到 79。
- `sector`：扇区号，1 到 18。
- `src`：要写入的 512 字节，在程序窗口里。

返回：0 成功；SB_ERR_WRITE_PROTECT 磁盘写保护；SB_ERR_NO_DISK 驱动器中没有磁盘；SB_ERR_UNKNOWN_DISK 未知磁盘类型。

<a id="sb_disk_read_sectors"></a>
### sb_disk_read_sectors

```c
unsigned char sb_disk_read_sectors(unsigned char head, unsigned char track, unsigned char sector, unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode);
```

从起始扇区连续读若干扇区到扩展 RAM 页里，扇区号递增，满 18 换面，两面满换磁道。先检测盘片。

系统调用：$80/$04

参数：

- `head`：起始面，0 或 1。
- `track`：起始磁道，0 到 79。
- `sector`：起始扇区号，1 到 18。
- `page`：接收数据的扩展 RAM 页号。
- `page_sector`：数据从页内第几个 512 字节开始放，0 到 7；每页放 8 个扇区，页要有足够的连续页。
- `count`：扇区数。
- `mode`：SB_DISK_STOP_ON_ERROR 或 SB_DISK_SKIP_ERRORS，见这两个常量的说明。

返回：0 成功；SB_ERR_NO_DISK 驱动器中没有磁盘；SB_ERR_CRC 等扇区出错码（mode 为 SB_DISK_STOP_ON_ERROR 时）。

<a id="sb_disk_write_sectors"></a>
### sb_disk_write_sectors

```c
unsigned char sb_disk_write_sectors(unsigned char head, unsigned char track, unsigned char sector, unsigned char page, unsigned char page_sector, unsigned int count, unsigned char mode);
```

把扩展 RAM 页里的数据连续写到若干扇区，扇区号递增的规则同 sb_disk_read_sectors。先检测盘片，写保护时立即停止。

系统调用：$80/$05

参数：

- `head`：起始面，0 或 1。
- `track`：起始磁道，0 到 79。
- `sector`：起始扇区号，1 到 18。
- `page`：数据所在的扩展 RAM 页号。
- `page_sector`：数据从页内第几个 512 字节开始，0 到 7。
- `count`：扇区数。
- `mode`：SB_DISK_STOP_ON_ERROR 或 SB_DISK_SKIP_ERRORS。

返回：0 成功；SB_ERR_WRITE_PROTECT 磁盘写保护；SB_ERR_NO_DISK 驱动器中没有磁盘。

<a id="sb_disk_scan_track"></a>
### sb_disk_scan_track

```c
unsigned char sb_disk_scan_track(unsigned char head, unsigned char track, unsigned char *bad_map);
```

逐个读一个磁道的 18 个扇区，读不出的在位图里置位。不检测盘片，直接读每个扇区。

系统调用：$80/$08

参数：

- `head`：面，0 或 1。
- `track`：磁道，0 到 79。
- `bad_map`：返回 3 字节位图：第 0 字节的位 7 到位 0 依次对应扇区 1 到 8，第 1 字节对应扇区 9 到 16，第 2 字节的位 7、位 6 对应扇区 17、18；置位表示读不出。

返回：0 成功；SB_ERR_BAD_DISK_PARAM 面或磁道超出范围。

<a id="sb_disk_format_track"></a>
### sb_disk_format_track

```c
unsigned char sb_disk_format_track(unsigned char head, unsigned char track);
```

格式化一个磁道：磁道 0 的 18 个扇区填 $00，其他磁道填 $F6。先检测盘片。会清掉这个磁道上的全部数据。

系统调用：$80/$01

参数：

- `head`：面，0 或 1。
- `track`：磁道，0 到 79。

返回：0 成功；SB_ERR_BAD_DISK_PARAM 面或磁道超出范围；SB_ERR_WRITE_PROTECT 磁盘写保护；SB_ERR_NO_DISK 驱动器中没有磁盘。

<a id="sb_disk_free_bytes"></a>
### sb_disk_free_bytes

```c
unsigned char sb_disk_free_bytes(unsigned long *bytes);
```

取磁盘上的空闲字节数（空闲簇数乘每簇字节数）。

系统调用：$7B/$01、$7B/$06

参数：

- `bytes`：返回空闲字节数。

返回：0 成功；其他是读 FAT 时的磁盘出错码。

<a id="display"></a>
## 显示、窗口与图片（sb2000-display.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_cells_fill`](#sb_cells_fill) | 从指定位置起线性地（先列后行，到行尾接下一行）把 count 个单元写成同一个值，写完刷新一次名称表到显存。 | $40 |
| [`sb_cells_rect`](#sb_cells_rect) | 把一个矩形区域的单元写成同一个值，每行刷新一次。 | $40 |
| [`sb_cells_copy`](#sb_cells_copy) | 把程序窗口里的一块单元数据逐行复制到屏幕，写完刷新一次名称表。 | $40 |
| [`sb_screen_clear`](#sb_screen_clear) | 清屏：把屏幕缓冲里指定名称表的全部单元填成同一个值（只写缓冲，不刷新显存）。 | $30/$0A |
| [`sb_screen_flush`](#sb_screen_flush) | 把一张名称表缓冲经 DMA 写到显存，并等一帧。 | $30/$00-$03 |
| [`sb_cell_address`](#sb_cell_address) | 求屏幕缓冲里一个单元的远地址，可以交给 sb_far_read、sb_far_write 读写。 | $30/$04-$07 |
| [`sb_text_field`](#sb_text_field) | 在屏幕上显示一行文字（可混排汉字与西文）：把字符串画成点阵写进显存的图块，再把上下两行单元写进名称表缓冲并刷新。 | $45 |
| [`sb_text_field_ascii`](#sb_text_field_ascii) | 同 sb_text_field，但只处理单字节字符，不补空白图块，只写一行单元：第 i 列的单元值是 cell + 步长 × i；步长在大字体（8×16 字模，DOS 的默认）下是 2，小字体下是 1。 | $46 |
| [`sb_text_draw`](#sb_text_draw) | 把字符串画成点阵图块，经 DMA 写进显存的图块区（目的地址 = 图块号 × 16），不写名称表。 | $3C、$3D、$3E、$3F |
| [`sb_text_to_memory`](#sb_text_to_memory) | 把字符串画成点阵图块，写进调用者指定的扩展 RAM 地址（不经显存）。每个图块二平面 16 字节、四平面 32 字节，按平面顺序各 8 字节； | $47、$48 |
| [`sb_display_mode`](#sb_display_mode) | 设置显示模式（显示模式影子 $0638 的高 2 位，决定文字字库区域的取法），低 6 位不变。 | $11/$00-$03 |
| [`sb_glyph_address`](#sb_glyph_address) | 查字符码对应的字模所在的页与页内地址，不绘制。字符码高字节为 0 时 $20 到 $7F 取西文字模（8×8 或 8×16，由显示模式决定）， | $3B |
| [`sb_window_init`](#sb_window_init) | 初始化窗口管理：30 个窗口号全部空闲，保存区末尾回到 $8060。程序开始用窗口之前调用一次。 | $59/$06 |
| [`sb_window_save`](#sb_window_save) | 把名称表缓冲里的一个矩形保存起来，以后用 sb_window_restore 原样写回。只保存缓冲，不碰显存。 | $59/$00-$03 |
| [`sb_window_restore`](#sb_window_restore) | 把保存的窗口内容写回名称表缓冲并释放窗口号。只写缓冲，不刷新显存，要用 sb_screen_flush 刷新。 | $59/$04 |
| [`sb_window_compact`](#sb_window_compact) | 整理窗口保存区：没有窗口时末尾回到 $8060，否则把后面的数据前移填掉空洞。 | $59/$05 |
| [`sb_palette_write`](#sb_palette_write) | 把一组颜色写进调色板，从调色板第 offset 项起。显示开着且 NMI 开着时由 NMI 处理程序写入。 | $5D |
| [`sb_vram_fill`](#sb_vram_fill) | 用一个字节填充显存的一段。长度为 0 什么都不做。 | $32 |
| [`sb_vram_load`](#sb_vram_load) | 把扩展 RAM 里的一段数据经 DMA 写进显存。长度为 0 什么都不做。 | $33 |
| [`sb_vram_bank`](#sb_vram_bank) | 设置显存 bank 寄存器（$4300）。 | $2A |
| [`sb_image_load_raw`](#sb_image_load_raw) | 不解压，直接把一段图案数据经 DMA 写进显存的图块区（目的地址 = 图块号 × 16），再等垂直消隐。长度为 0 直接返回。 | $5A |
| [`sb_image_unpack_tiles`](#sb_image_unpack_tiles) | 解压图案流并经 DMA 写进显存的图块区。流的格式：16 字节字典加指令流，指令字节 $00 结束； | $5B |
| [`sb_image_unpack_map`](#sb_image_unpack_map) | 解压映射流并按效果写进屏幕：流里的单元值加上 map->base 与调色板位，再按效果把矩形写进名称表缓冲并刷新到显存。 | $5C |
| [`sb_image_fill_rect`](#sb_image_fill_rect) | 用与 sb_image_unpack_map 相同的效果把一个单元值（map->base）铺满矩形，不加调色板位。 | $60 |
| [`sb_image_show`](#sb_image_show) | 显示压缩图片：10 字节头 [宽、高、图案数（16 位）、调色板组数、每组色数、映射流偏移（16 位）、调色板偏移（16 位）]（偏移相对头的起点）， | $5F |
| [`sb_image_show_full`](#sb_image_show_full) | 显示整屏图片：头格式同 sb_image_show；图案流解压到起始图块 tile，映射流直接按顺序解到名称表缓冲的开头（单元基础值是 tile，不加调色板位）， | $5E |
| [`sb_image_unpack_to_memory`](#sb_image_unpack_to_memory) | 把图案流解压到内存，格式同 sb_image_unpack_tiles。目的地址 address 是窗口地址（$8000 到 $FFFF，高 4 位选页，低 12 位是页内偏移）， | $61 |

<a id="sb_cells_fill"></a>
### sb_cells_fill

```c
void sb_cells_fill(unsigned char table, unsigned char col, unsigned char row, unsigned int count, unsigned int cell, unsigned char palette);
```

从指定位置起线性地（先列后行，到行尾接下一行）把 count 个单元写成同一个值，写完刷新一次名称表到显存。

系统调用：$40

参数：

- `table`：名称表号，0 到 3。
- `col`：起点列，0 到 31。
- `row`：起点行，0 到 29。
- `count`：单元个数，不要超过名称表里剩下的单元数；0 会被当作 65536，不要用。
- `cell`：单元值，低 12 位是图块号，高 4 位被调色板号替换。
- `palette`：调色板号，0 到 15。

<a id="sb_cells_rect"></a>
### sb_cells_rect

```c
void sb_cells_rect(unsigned char table, unsigned char col, unsigned char row, unsigned char width, unsigned char height, unsigned int cell, unsigned char palette);
```

把一个矩形区域的单元写成同一个值，每行刷新一次。

系统调用：$40

参数：

- `table`：名称表号，0 到 3。
- `col`：左上角列。
- `row`：左上角行。
- `width`：宽，列数，列加宽不超过 32。
- `height`：高，行数，行加高不超过 30。
- `cell`：单元值，低 12 位是图块号。
- `palette`：调色板号，0 到 15。

<a id="sb_cells_copy"></a>
### sb_cells_copy

```c
void sb_cells_copy(unsigned char table, unsigned char col, unsigned char row, unsigned char width, unsigned char height, const unsigned int *cells);
```

把程序窗口里的一块单元数据逐行复制到屏幕，写完刷新一次名称表。

系统调用：$40

参数：

- `table`：名称表号，0 到 3。
- `col`：左上角列。
- `row`：左上角行。
- `width`：宽，列数。
- `height`：高，行数。
- `cells`：单元数据，width × height 个，按行排列，在程序窗口里；单元值原样写入，不替换调色板位。

<a id="sb_screen_clear"></a>
### sb_screen_clear

```c
void sb_screen_clear(unsigned char tables, unsigned int cell);
```

清屏：把屏幕缓冲里指定名称表的全部单元填成同一个值（只写缓冲，不刷新显存）。

系统调用：$30/$0A

参数：

- `tables`：位掩码，位 0 到 3 依次对应名称表 0 到 3（缓冲里的名称表 4 到 7）。
- `cell`：单元值。

<a id="sb_screen_flush"></a>
### sb_screen_flush

```c
void sb_screen_flush(unsigned char table);
```

把一张名称表缓冲经 DMA 写到显存，并等一帧。

系统调用：$30/$00-$03

参数：

- `table`：名称表号，0 到 3。

<a id="sb_cell_address"></a>
### sb_cell_address

```c
void sb_cell_address(unsigned char table, unsigned char col, unsigned char row, unsigned char *page, unsigned int *offset);
```

求屏幕缓冲里一个单元的远地址，可以交给 sb_far_read、sb_far_write 读写。

系统调用：$30/$04-$07

参数：

- `table`：名称表号，0 到 3。
- `col`：列。
- `row`：行。
- `page`：返回页号。
- `offset`：返回页内偏移。

<a id="sb_text_field"></a>
### sb_text_field

```c
void sb_text_field(unsigned char table, unsigned char col, unsigned char row, const char *text, unsigned int cell, unsigned char palette, unsigned char color);
```

在屏幕上显示一行文字（可混排汉字与西文）：把字符串画成点阵写进显存的图块，再把上下两行单元写进名称表缓冲并刷新。
汉字占 2 列；宽度限制在 32 列以内，汉字起点离右边界不足 2 列时不画。
二平面模式下，上行第 k 列的单元值是 cell + 2k，下行再加 1。

系统调用：$45

参数：

- `table`：名称表号，0 到 3。
- `col`：起点列。
- `row`：起点行，文字占上下两行。
- `text`：以 0 结尾的字符串，在程序窗口里。
- `cell`：起始图块号（单元值的低 12 位），文字的字模写进显存从它起的图块。
- `palette`：调色板号，0 到 15。
- `color`：前景色 0 到 15：每一位决定一个平面里字模是否出现。

<a id="sb_text_field_ascii"></a>
### sb_text_field_ascii

```c
void sb_text_field_ascii(unsigned char table, unsigned char col, unsigned char row, const char *text, unsigned int cell, unsigned char palette, unsigned char color);
```

同 sb_text_field，但只处理单字节字符，不补空白图块，只写一行单元：第 i 列的单元值是 cell + 步长 × i；步长在大字体（8×16 字模，DOS 的默认）下是 2，小字体下是 1。

系统调用：$46

参数：

- `table`：名称表号，0 到 3。
- `col`：起点列。
- `row`：行。
- `text`：以 0 结尾的字符串，在程序窗口里。
- `cell`：起始图块号。
- `palette`：调色板号，0 到 15。
- `color`：前景色 0 到 15。

<a id="sb_text_draw"></a>
### sb_text_draw

```c
void sb_text_draw(unsigned int tile, const char *text, unsigned char terminator, unsigned char color, unsigned char four_planes, unsigned char ascii_only);
```

把字符串画成点阵图块，经 DMA 写进显存的图块区（目的地址 = 图块号 × 16），不写名称表。
返回后 $064C/$064D 被换算成字节地址（图块号 × 16）。

系统调用：$3C、$3D、$3E、$3F

参数：

- `tile`：起始图块号。
- `text`：字符串，在程序窗口里。
- `terminator`：结束符，读到该字节为止。
- `color`：前景色 0 到 15：每一位决定一个平面里字模是否出现。
- `four_planes`：0 画二平面图块（每个 16 字节），非 0 画四平面图块（每个 32 字节）。
- `ascii_only`：0 处理汉字与西文，非 0 只处理单字节字符。

<a id="sb_text_to_memory"></a>
### sb_text_to_memory

```c
unsigned int sb_text_to_memory(const char *text, unsigned char terminator, unsigned char group, unsigned int address, unsigned int capacity, unsigned char four_planes, unsigned char color, unsigned char ascii_only);
```

把字符串画成点阵图块，写进调用者指定的扩展 RAM 地址（不经显存）。每个图块二平面 16 字节、四平面 32 字节，按平面顺序各 8 字节；
汉字占两个图块宽。目的区装不下下一个字符时在该字符之前停止。目的区跨 4KB 页时窗口 2、3 的映射随之后移，输出线性连续。

系统调用：$47、$48

参数：

- `text`：字符串，在程序窗口里。
- `terminator`：结束符，读到该字节为止（汉字比较第一个字节）。
- `group`：目的页组的首页号。
- `address`：目的窗口地址（$8000 到 $FFFF，高 4 位选页，低 12 位是页内偏移），页号 = (高 4 位) - 8 + group。
- `capacity`：目的区还能容纳的字节数。
- `four_planes`：0 画二平面图块，非 0 画四平面图块。
- `color`：前景色 0 到 15。
- `ascii_only`：0 处理汉字与西文，非 0 只处理单字节字符。

返回：实际写入的字节数。

<a id="sb_display_mode"></a>
### sb_display_mode

```c
void sb_display_mode(unsigned char mode);
```

设置显示模式（显示模式影子 $0638 的高 2 位，决定文字字库区域的取法），低 6 位不变。

系统调用：$11/$00-$03

参数：

- `mode`：显示模式 0 到 3，对应高 2 位 $00、$40、$80、$C0。

<a id="sb_glyph_address"></a>
### sb_glyph_address

```c
unsigned char sb_glyph_address(unsigned int code, unsigned char *page, unsigned int *offset, unsigned char *shape);
```

查字符码对应的字模所在的页与页内地址，不绘制。字符码高字节为 0 时 $20 到 $7F 取西文字模（8×8 或 8×16，由显示模式决定），
高字节不小于 $A1 时按 GB2312 区位处理：$A1 到 $A9 区是符号，$B0 到 $F7 区是汉字（32 字节，前 16 字节是每行左半边，后 16 字节是右半边）。

系统调用：$3B

参数：

- `code`：字符码，西文放低字节，汉字是 GB2312 内码。
- `page`：返回字模所在的页。
- `offset`：返回字模在页内的偏移。
- `shape`：返回字模形式（低 3 位）。

返回：0 成功；$FF 字符码不合法。

<a id="sb_window_init"></a>
### sb_window_init

```c
void sb_window_init(void);
```

初始化窗口管理：30 个窗口号全部空闲，保存区末尾回到 $8060。程序开始用窗口之前调用一次。

系统调用：$59/$06

<a id="sb_window_save"></a>
### sb_window_save

```c
unsigned char sb_window_save(unsigned char table, unsigned char left, unsigned char top, unsigned char right, unsigned char bottom);
```

把名称表缓冲里的一个矩形保存起来，以后用 sb_window_restore 原样写回。只保存缓冲，不碰显存。

系统调用：$59/$00-$03

参数：

- `table`：名称表号，0 到 3。
- `left`：左上角列。
- `top`：左上角行。
- `right`：右下角列（含），小于 32。
- `bottom`：右下角行（含），小于 30。

返回：窗口号 1 到 30；SB_WINDOW_BAD_RECT 坐标不合法；SB_WINDOW_TOO_MANY 窗口用完；SB_WINDOW_NO_ROOM 保存区装不下，出错时窗口表与保存区不变。

<a id="sb_window_restore"></a>
### sb_window_restore

```c
void sb_window_restore(unsigned char id);
```

把保存的窗口内容写回名称表缓冲并释放窗口号。只写缓冲，不刷新显存，要用 sb_screen_flush 刷新。
窗口号不小于 $1F 或表里没有时什么都不做。只有释放的是表里最后一项时才回收保存区，否则留下空洞，要用 sb_window_compact 整理。

系统调用：$59/$04

参数：

- `id`：窗口号。

<a id="sb_window_compact"></a>
### sb_window_compact

```c
void sb_window_compact(void);
```

整理窗口保存区：没有窗口时末尾回到 $8060，否则把后面的数据前移填掉空洞。
注意：第一个窗口之后的空洞，系统修正表项的方式只在空洞大小恰等于前一个窗口长度时才正确，其他情况会让表项指向乱数据，不要在窗口之间有空洞时使用。

系统调用：$59/$05

<a id="sb_palette_write"></a>
### sb_palette_write

```c
void sb_palette_write(unsigned char offset, const unsigned char *colors, unsigned char count);
```

把一组颜色写进调色板，从调色板第 offset 项起。显示开着且 NMI 开着时由 NMI 处理程序写入。

系统调用：$5D

参数：

- `offset`：起始项，调色板共 64 项。
- `colors`：颜色字节，在程序窗口里。
- `count`：个数，0 什么都不做。

<a id="sb_vram_fill"></a>
### sb_vram_fill

```c
void sb_vram_fill(unsigned int address, unsigned int length, unsigned char value);
```

用一个字节填充显存的一段。长度为 0 什么都不做。

系统调用：$32

参数：

- `address`：显存起始地址。
- `length`：长度，字节。
- `value`：填充值。

<a id="sb_vram_load"></a>
### sb_vram_load

```c
void sb_vram_load(unsigned char page, unsigned int offset, unsigned int address, unsigned int length);
```

把扩展 RAM 里的一段数据经 DMA 写进显存。长度为 0 什么都不做。

系统调用：$33

参数：

- `page`：源数据所在的扩展 RAM 页。
- `offset`：源数据在页内的偏移。
- `address`：显存目的地址。
- `length`：长度，字节。

<a id="sb_vram_bank"></a>
### sb_vram_bank

```c
void sb_vram_bank(unsigned char bank);
```

设置显存 bank 寄存器（$4300）。

系统调用：$2A

参数：

- `bank`：要写入的值。

<a id="sb_image_load_raw"></a>
### sb_image_load_raw

```c
void sb_image_load_raw(const void *data, unsigned int length, unsigned int tile);
```

不解压，直接把一段图案数据经 DMA 写进显存的图块区（目的地址 = 图块号 × 16），再等垂直消隐。长度为 0 直接返回。

系统调用：$5A

参数：

- `data`：图案数据，在程序窗口里。
- `length`：长度，字节。
- `tile`：起始图块号。

<a id="sb_image_unpack_tiles"></a>
### sb_image_unpack_tiles

```c
void sb_image_unpack_tiles(const void *stream, unsigned int tile);
```

解压图案流并经 DMA 写进显存的图块区。流的格式：16 字节字典加指令流，指令字节 $00 结束；
指令字节高 4 位 H、低 4 位 L：H 为 1 到 12 把字典第 L 项重复 H 次，H=$D 后跟 L 个原样字节，H=$E 后跟 1 个字节重复 L 次，H=$F 后跟起始字节输出 L 个递增字节。

系统调用：$5B

参数：

- `stream`：图案流，在程序窗口里。
- `tile`：起始图块号。

<a id="sb_image_unpack_map"></a>
### sb_image_unpack_map

```c
void sb_image_unpack_map(const void *stream, const sb_map_t *map);
```

解压映射流并按效果写进屏幕：流里的单元值加上 map->base 与调色板位，再按效果把矩形写进名称表缓冲并刷新到显存。
指令字节高 3 位 T、低 5 位 N：$00 结束；T=0 取一个 16 位值填 N 个；T=1 取一个值，N 个单元依次加 1；T=2 取 N 个 16 位值；T=3 先取一个高字节再取 N 个低字节；T 不小于 4 取一个值，N 个单元依次加 2。
矩形限制在 32×32 内，起点列或行不小于 32 或宽高为 0 时什么都不写。

系统调用：$5C

参数：

- `stream`：映射流，在程序窗口里。
- `map`：写到屏幕的位置、效果与速度。

<a id="sb_image_fill_rect"></a>
### sb_image_fill_rect

```c
void sb_image_fill_rect(const sb_map_t *map);
```

用与 sb_image_unpack_map 相同的效果把一个单元值（map->base）铺满矩形，不加调色板位。

系统调用：$60

参数：

- `map`：矩形、效果与速度；base 是单元值。

<a id="sb_image_show"></a>
### sb_image_show

```c
void sb_image_show(const void *image, const sb_map_t *map, unsigned char palette_group);
```

显示压缩图片：10 字节头 [宽、高、图案数（16 位）、调色板组数、每组色数、映射流偏移（16 位）、调色板偏移（16 位）]（偏移相对头的起点），
先解压图案流（头后，图块号取 map->base），再写调色板（偏移 = 调色板组号 × 每组色数，长度 = 头里的组数 × 每组色数），最后按映射流、位置、效果与速度显示。

系统调用：$5F

参数：

- `image`：图片数据（头加图案流、映射流、调色板），在程序窗口里。
- `map`：位置、效果、速度；base 是图案的起始图块号。
- `palette_group`：调色板组号。

<a id="sb_image_show_full"></a>
### sb_image_show_full

```c
void sb_image_show_full(const void *image, unsigned int tile, unsigned char table);
```

显示整屏图片：头格式同 sb_image_show；图案流解压到起始图块 tile，映射流直接按顺序解到名称表缓冲的开头（单元基础值是 tile，不加调色板位），
解完整张名称表 DMA 一次，调色板从第 0 项起写 $40 字节。

系统调用：$5E

参数：

- `image`：图片数据，在程序窗口里。
- `tile`：起始图块号。
- `table`：名称表号，0 到 3。

<a id="sb_image_unpack_to_memory"></a>
### sb_image_unpack_to_memory

```c
unsigned int sb_image_unpack_to_memory(const void *stream, unsigned char group, unsigned int address);
```

把图案流解压到内存，格式同 sb_image_unpack_tiles。目的地址 address 是窗口地址（$8000 到 $FFFF，高 4 位选页，低 12 位是页内偏移），
页号 = (address 高 4 位) - 8 + group；目的页与下一页映射到窗口 2、3，所以目的地址线性连续。

系统调用：$61

参数：

- `stream`：图案流，在程序窗口里。
- `group`：目的页组的首页号。
- `address`：目的窗口地址。

返回：解出的字节数。

<a id="dma"></a>
## 通用 DMA（sb2000-dma.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_dma_page_address`](#sb_dma_page_address) | 把扩展 RAM 的页号与页内偏移换算成 DMA 的源地址。 |  |
| [`sb_dma_to_cpu`](#sb_dma_to_cpu) | 用 DMA 把扩展 RAM 里的数据复制到 CPU 地址。只有目的在 ERAM（$6000-$7FFF）时有效，目的范围不能越过 $8000。 |  |
| [`sb_dma_to_vram`](#sb_dma_to_vram) | 用 DMA 把扩展 RAM 里的数据复制到显存。目的地址不小于 $8000 时是扩展显存，再加 $4300 选的 32K bank，否则是内置字符 RAM。 |  |

<a id="sb_dma_page_address"></a>
### sb_dma_page_address

```c
unsigned long sb_dma_page_address(unsigned char page, unsigned int offset);
```

把扩展 RAM 的页号与页内偏移换算成 DMA 的源地址。

参数：

- `page`：扩展 RAM 页号，0x80 到 0xFF。
- `offset`：页内偏移，可以超过一页。

返回：源地址，传给 sb_dma_to_cpu、sb_dma_to_vram。

<a id="sb_dma_to_cpu"></a>
### sb_dma_to_cpu

```c
unsigned char sb_dma_to_cpu(unsigned long source, unsigned int dest, unsigned int length);
```

用 DMA 把扩展 RAM 里的数据复制到 CPU 地址。只有目的在 ERAM（$6000-$7FFF）时有效，目的范围不能越过 $8000。

参数：

- `source`：源地址，用 sb_dma_page_address 换算。
- `dest`：目的 CPU 地址。
- `length`：字节数，1 到 $8000。

返回：0 成功；SB_ERR_BAD_PARAMETER 长度为 0 或超过 $8000，或目的范围越过 $8000。

<a id="sb_dma_to_vram"></a>
### sb_dma_to_vram

```c
unsigned char sb_dma_to_vram(unsigned long source, unsigned int dest, unsigned int length);
```

用 DMA 把扩展 RAM 里的数据复制到显存。目的地址不小于 $8000 时是扩展显存，再加 $4300 选的 32K bank，否则是内置字符 RAM。
调用时要关着显示，或在垂直消隐期间。

参数：

- `source`：源地址，用 sb_dma_page_address 换算。
- `dest`：目的显存地址。
- `length`：字节数，1 到 $8000。

返回：0 成功；SB_ERR_BAD_PARAMETER 长度为 0 或超过 $8000。

<a id="file"></a>
## 文件与目录（sb2000-file.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_file_open`](#sb_file_open) | 打开文件或目录项。打开方式用 SB_MODE_ 开头的常量： | $7B/$09 |
| [`sb_file_close`](#sb_file_close) | 关闭文件，写回目录项，并让软驱停转。关闭空闲的句柄什么都不做，也不出错。 | $7C/$02 |
| [`sb_file_size`](#sb_file_size) | 取文件大小。 | $7C/$0D |
| [`sb_file_read`](#sb_file_read) | 从文件的指定位置读数据到内存，长度不限（内部分块读）。 | $7C/$05 |
| [`sb_file_write`](#sb_file_write) | 改写文件里已有的一段数据，文件大小不变（内部分块写）。不能用它扩大文件或新建文件，那些用 sb_file_write_all 或 sb_file_save。 | $7C/$06 |
| [`sb_file_write_all`](#sb_file_write_all) | 把内存里的数据写成一个文件（已存在则覆盖），文件大小就是数据长度。数据先放进临时分配的扩展 RAM 页，再整个写出。 | $7C/$10 |
| [`sb_file_load`](#sb_file_load) | 把整个文件读入新分配的扩展 RAM 页，适合比内存大的文件：数据在页里，用 sb_far_read 按需取出。 | $7C/$0F |
| [`sb_file_save`](#sb_file_save) | 把扩展 RAM 页里的数据整个写成一个文件（已存在则覆盖）。数据长度为 0 时只创建空文件。 | $7C/$10 |
| [`sb_file_delete`](#sb_file_delete) | 删除文件：目录项标为已删除并释放空间，之后软驱停转。 | $7C/$03 |
| [`sb_file_rename`](#sb_file_rename) | 给文件改名。新名称是 NAME.EXT 格式，不含目录。 | $7C/$04 |
| [`sb_file_get_date`](#sb_file_get_date) | 取已打开文件的修改日期。 | $7C/$08 |
| [`sb_file_set_date`](#sb_file_set_date) | 设置已打开文件的修改日期，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。 | $7C/$07 |
| [`sb_file_get_time`](#sb_file_get_time) | 取已打开文件的修改时间。 | $7C/$0A |
| [`sb_file_set_time`](#sb_file_set_time) | 设置已打开文件的修改时间，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。 | $7C/$09 |
| [`sb_file_get_attr`](#sb_file_get_attr) | 取已打开文件的属性。 | $7C/$0C |
| [`sb_file_set_attr`](#sb_file_set_attr) | 设置已打开文件的属性，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。 | $7C/$0B |
| [`sb_dir_make`](#sb_dir_make) | 创建子目录。只支持一级子目录，在子目录里再建目录出错。 | $7D/$02 |
| [`sb_dir_remove`](#sb_dir_remove) | 删除空的子目录。 | $7D/$03 |
| [`sb_dir_change`](#sb_dir_change) | 设置当前目录，相对路径都以它为起点。 | $7B/$0B |
| [`sb_dir_current`](#sb_dir_current) | 取当前目录自己的名称，格式 \NAME.EXT，根目录是 \。 | $7D/$01 |
| [`sb_dir_open`](#sb_dir_open) | 开始遍历一个目录。遍历期间不要做其他磁盘操作，它们会占用同一个扇区缓冲。 | $7B/$01、$7B/$03、$7B/$09 |
| [`sb_dir_read`](#sb_dir_read) | 取下一个目录项，跳过已删除的项；卷标和子目录也会返回，用属性区分。 | $7B/$00、$7B/$01 |

<a id="sb_file_open"></a>
### sb_file_open

```c
unsigned char sb_file_open(const char *path, unsigned char mode, unsigned char *handle);
```

打开文件或目录项。打开方式用 SB_MODE_ 开头的常量：
SB_MODE_OPEN 打开已有文件（只读）；SB_MODE_CREATE 创建文件，已存在则覆盖；
SB_MODE_UPDATE 打开已有文件，可按偏移写，关闭时写回目录项；SB_MODE_ENTRY 取得根目录或子目录的目录项。
相对路径在当前目录里找。

系统调用：$7B/$09

参数：

- `path`：路径，零结尾，最长 28 个字符。
- `mode`：打开方式。
- `handle`：返回句柄号，0 到 3。

返回：0 成功；SB_ERR_FILE_NOT_FOUND 文件不存在；SB_ERR_PATH_NOT_FOUND 目录不存在；SB_ERR_TOO_MANY_FILES 4 个句柄都在用； SB_ERR_BAD_NAME 名称含非法字符或路径过长；SB_ERR_BAD_OPEN_MODE 打开方式不合法；SB_ERR_ACCESS_DENIED 名称与卷标同名。

<a id="sb_file_close"></a>
### sb_file_close

```c
unsigned char sb_file_close(unsigned char handle);
```

关闭文件，写回目录项，并让软驱停转。关闭空闲的句柄什么都不做，也不出错。

系统调用：$7C/$02

参数：

- `handle`：句柄号。

返回：0 成功；SB_ERR_BAD_PARAMETER 句柄号超出 0-3。

<a id="sb_file_size"></a>
### sb_file_size

```c
unsigned char sb_file_size(unsigned char handle, unsigned long *size);
```

取文件大小。

系统调用：$7C/$0D

参数：

- `handle`：句柄号。
- `size`：返回文件大小，字节。

返回：0 成功。

<a id="sb_file_read"></a>
### sb_file_read

```c
unsigned char sb_file_read(unsigned char handle, unsigned long offset, void *dest, unsigned int length);
```

从文件的指定位置读数据到内存，长度不限（内部分块读）。

系统调用：$7C/$05

参数：

- `handle`：句柄号，打开方式要是 SB_MODE_OPEN、SB_MODE_CREATE 或 SB_MODE_UPDATE。
- `offset`：文件内偏移。
- `dest`：目的地，必须在程序窗口 $8000-$BFFF 内，且不在窗口 3（$B000-$BFFF）。
- `length`：字节数；偏移不小于文件大小或偏移加长度超过文件大小时出错，数据不会被截短读入。

返回：0 成功；$14 读写位置超出文件；SB_ERR_BAD_OPEN_MODE 打开方式不允许读；SB_ERR_NO_MEMORY 没有空闲页做中转。

<a id="sb_file_write"></a>
### sb_file_write

```c
unsigned char sb_file_write(unsigned char handle, unsigned long offset, const void *src, unsigned int length);
```

改写文件里已有的一段数据，文件大小不变（内部分块写）。不能用它扩大文件或新建文件，那些用 sb_file_write_all 或 sb_file_save。

系统调用：$7C/$06

参数：

- `handle`：句柄号，打开方式要是 SB_MODE_CREATE 或 SB_MODE_UPDATE。
- `offset`：文件内偏移。
- `src`：数据来源，位置的要求同 sb_file_read 的 dest。
- `length`：字节数。

返回：0 成功；$14 写入范围超出文件当前大小；SB_ERR_BAD_OPEN_MODE 打开方式不允许写；SB_ERR_NO_MEMORY 没有空闲页做中转。

<a id="sb_file_write_all"></a>
### sb_file_write_all

```c
unsigned char sb_file_write_all(const char *path, const void *data, unsigned int length);
```

把内存里的数据写成一个文件（已存在则覆盖），文件大小就是数据长度。数据先放进临时分配的扩展 RAM 页，再整个写出。

系统调用：$7C/$10

参数：

- `path`：路径。
- `data`：数据，位置的要求同 sb_file_read 的 dest。
- `length`：字节数，0 只创建空文件。

返回：0 成功；SB_ERR_NO_MEMORY 没有足够的页；SB_ERR_NO_SPACE 磁盘空间不够；其他是创建与写入的出错码。

<a id="sb_file_load"></a>
### sb_file_load

```c
unsigned char sb_file_load(const char *path, unsigned char *first_page, unsigned long *size);
```

把整个文件读入新分配的扩展 RAM 页，适合比内存大的文件：数据在页里，用 sb_far_read 按需取出。
用完要用 sb_page_free 释放页。空文件不分配页，页号返回 0。

系统调用：$7C/$0F

参数：

- `path`：路径。
- `first_page`：返回第一个页号。
- `size`：返回文件大小，字节。

返回：0 成功；SB_ERR_NO_MEMORY 没有足够的连续空闲页；其他是打开与读取的出错码。

<a id="sb_file_save"></a>
### sb_file_save

```c
unsigned char sb_file_save(const char *path, unsigned char first_page, unsigned long size);
```

把扩展 RAM 页里的数据整个写成一个文件（已存在则覆盖）。数据长度为 0 时只创建空文件。

系统调用：$7C/$10

参数：

- `path`：路径。
- `first_page`：数据的第一个页号。
- `size`：数据长度，字节，不超过 16777215。

返回：0 成功；SB_ERR_NO_SPACE 磁盘空间不够，已写入的其他文件完好；其他是创建与写入的出错码。

<a id="sb_file_delete"></a>
### sb_file_delete

```c
unsigned char sb_file_delete(const char *path);
```

删除文件：目录项标为已删除并释放空间，之后软驱停转。

系统调用：$7C/$03

参数：

- `path`：路径。

返回：0 成功；SB_ERR_ACCESS_DENIED 文件是只读的；SB_ERR_FILE_NOT_FOUND 文件不存在。

<a id="sb_file_rename"></a>
### sb_file_rename

```c
unsigned char sb_file_rename(const char *path, const char *new_name);
```

给文件改名。新名称是 NAME.EXT 格式，不含目录。

系统调用：$7C/$04

参数：

- `path`：原路径。
- `new_name`：新名称。

返回：0 成功；SB_ERR_FILE_EXISTS 新名称已存在；SB_ERR_BAD_NAME 新名称含非法字符。

<a id="sb_file_get_date"></a>
### sb_file_get_date

```c
unsigned char sb_file_get_date(unsigned char handle, sb_date_t *date);
```

取已打开文件的修改日期。

系统调用：$7C/$08

参数：

- `handle`：句柄号。
- `date`：返回日期。

返回：0 成功。

<a id="sb_file_set_date"></a>
### sb_file_set_date

```c
unsigned char sb_file_set_date(unsigned char handle, const sb_date_t *date);
```

设置已打开文件的修改日期，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。

系统调用：$7C/$07

参数：

- `handle`：句柄号。
- `date`：新日期。

返回：0 成功；SB_ERR_BAD_PARAMETER 日期不合法。

<a id="sb_file_get_time"></a>
### sb_file_get_time

```c
unsigned char sb_file_get_time(unsigned char handle, sb_time_t *time);
```

取已打开文件的修改时间。

系统调用：$7C/$0A

参数：

- `handle`：句柄号。
- `time`：返回时间。

返回：0 成功。

<a id="sb_file_set_time"></a>
### sb_file_set_time

```c
unsigned char sb_file_set_time(unsigned char handle, const sb_time_t *time);
```

设置已打开文件的修改时间，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。

系统调用：$7C/$09

参数：

- `handle`：句柄号。
- `time`：新时间。

返回：0 成功；SB_ERR_BAD_PARAMETER 时间不合法。

<a id="sb_file_get_attr"></a>
### sb_file_get_attr

```c
unsigned char sb_file_get_attr(unsigned char handle, unsigned char *attr);
```

取已打开文件的属性。

系统调用：$7C/$0C

参数：

- `handle`：句柄号。
- `attr`：返回属性，SB_ATTR_ 开头的常量按位组合。

返回：0 成功。

<a id="sb_file_set_attr"></a>
### sb_file_set_attr

```c
unsigned char sb_file_set_attr(unsigned char handle, unsigned char attr);
```

设置已打开文件的属性，关闭时写回目录项（方式 SB_MODE_CREATE、SB_MODE_UPDATE）。
属性大于 $3F 或带卷标位时出错。只读文件不能删除，方式 SB_MODE_CREATE 打开则直接覆盖并恢复成存档属性。

系统调用：$7C/$0B

参数：

- `handle`：句柄号。
- `attr`：新属性。

返回：0 成功；SB_ERR_BAD_PARAMETER 属性不合法。

<a id="sb_dir_make"></a>
### sb_dir_make

```c
unsigned char sb_dir_make(const char *path);
```

创建子目录。只支持一级子目录，在子目录里再建目录出错。

系统调用：$7D/$02

参数：

- `path`：路径。

返回：0 成功；SB_ERR_DIR_EXISTS 目录已存在；SB_ERR_DUPLICATE_NAME 同名文件已存在；SB_ERR_PATH_NOT_FOUND 父目录不存在；SB_ERR_DIR_TOO_DEEP 目录嵌套层数过多。

<a id="sb_dir_remove"></a>
### sb_dir_remove

```c
unsigned char sb_dir_remove(const char *path);
```

删除空的子目录。

系统调用：$7D/$03

参数：

- `path`：路径。

返回：0 成功；SB_ERR_DIR_NOT_EMPTY 目录非空；SB_ERR_ACCESS_DENIED 目录是只读的；SB_ERR_DUPLICATE_NAME 路径是文件；SB_ERR_PATH_NOT_FOUND 路径不存在。

<a id="sb_dir_change"></a>
### sb_dir_change

```c
unsigned char sb_dir_change(const char *path);
```

设置当前目录，相对路径都以它为起点。

系统调用：$7B/$0B

参数：

- `path`：路径，根目录写 "\"。

返回：0 成功；SB_ERR_PATH_NOT_FOUND 路径不存在或是文件，当前目录不变。

<a id="sb_dir_current"></a>
### sb_dir_current

```c
unsigned char sb_dir_current(char *name);
```

取当前目录自己的名称，格式 \NAME.EXT，根目录是 \。

系统调用：$7D/$01

参数：

- `name`：返回名称，缓冲区至少 16 个字节。

返回：0 成功。

<a id="sb_dir_open"></a>
### sb_dir_open

```c
unsigned char sb_dir_open(const char *path);
```

开始遍历一个目录。遍历期间不要做其他磁盘操作，它们会占用同一个扇区缓冲。

系统调用：$7B/$01、$7B/$03、$7B/$09

参数：

- `path`：目录路径；写 NULL 或空字符串表示当前目录，"\" 表示根目录。

返回：0 成功；SB_ERR_PATH_NOT_FOUND 目录不存在；其他是磁盘出错码。

<a id="sb_dir_read"></a>
### sb_dir_read

```c
unsigned char sb_dir_read(sb_dir_entry_t *entry);
```

取下一个目录项，跳过已删除的项；卷标和子目录也会返回，用属性区分。

系统调用：$7B/$00、$7B/$01

参数：

- `entry`：返回目录项。

返回：0 取到一项；SB_DIR_END 已经没有更多的项；其他是磁盘出错码。

<a id="ime"></a>
## 中文输入法（sb2000-ime.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_ime_init`](#sb_ime_init) | 初始化输入法：记下状态行的位置与颜色，选择初始输入法，登记说明文字。 | $6D/$00 |
| [`sb_ime_select`](#sb_ime_select) | 选择输入法。成功时清按键队列与输入缓冲，状态行换成该输入法的名称。 | $6D/$01 |
| [`sb_ime_set_state`](#sb_ime_set_state) | 设置输入法状态：允许位与标志位（只取低 7 位，位 7 保留）。 | $6D/$02 |
| [`sb_ime_get_state`](#sb_ime_get_state) | 取输入法状态。 | $6D/$08 |
| [`sb_ime_get_method`](#sb_ime_get_method) | 取当前输入法序号。 | $6D/$07 |
| [`sb_ime_resume`](#sb_ime_resume) | 恢复被暂停的输入法：暂停标志置位时清除它并刷新状态行，否则什么都不做。 | $6D/$03 |
| [`sb_ime_suspend`](#sb_ime_suspend) | 暂停输入法：清候选字，状态行清为空，暂停标志置位。已经暂停时什么都不做。暂停期间按键原样传回。 | $6D/$04 |
| [`sb_ime_set_width`](#sb_ime_set_width) | 设置半角或全角：状态行文字换成“半角”或“全角”，标志位里的全角位相应清除或置位。全角时输出的半角符号会换成全角内码。 | $6D/$05、$6D/$06 |
| [`sb_ime_key`](#sb_ime_key) | 处理一个按键（不等待）：从键盘队列取一个键交给输入法，没有键时直接返回。 | $6D/$0B |
| [`sb_ime_key_wait`](#sb_ime_key_wait) | 等待并处理按键，直到输入法有结果（结果标志不为 0）才返回，没有键时一直等下去。 | $6D/$0A |
| [`sb_ime_hex`](#sb_ime_hex) | 把一个 16 位内码转成 4 个小写十六进制字符。 | $6D/$0E |
| [`sb_ime_parse`](#sb_ime_parse) | 把 4 个输入字符转成汉字内码：全是数字当区位码（4 位十进制），第一个字符是 a 到 f 当 4 位十六进制内码。 | $6D/$0D |

<a id="sb_ime_init"></a>
### sb_ime_init

```c
unsigned char sb_ime_init(const sb_ime_config_t *config);
```

初始化输入法：记下状态行的位置与颜色，选择初始输入法，登记说明文字。

系统调用：$6D/$00

参数：

- `config`：初始化参数。

返回：0 成功；1 输入法序号不小于 10、没有权限或不可选，其余参数已写入。

<a id="sb_ime_select"></a>
### sb_ime_select

```c
unsigned char sb_ime_select(unsigned char method, unsigned char flags);
```

选择输入法。成功时清按键队列与输入缓冲，状态行换成该输入法的名称。

系统调用：$6D/$01

参数：

- `method`：输入法序号 0 到 9；序号 3 到 6 要 ERAM $0392 里对应的位允许，7 不可选。
- `flags`：0，或 SB_IME_CHECK_ALLOWED 表示同时核对允许位。

返回：0 成功；1 失败，状态不变。

<a id="sb_ime_set_state"></a>
### sb_ime_set_state

```c
void sb_ime_set_state(unsigned char allow, unsigned char flags);
```

设置输入法状态：允许位与标志位（只取低 7 位，位 7 保留）。

系统调用：$6D/$02

参数：

- `allow`：允许位。
- `flags`：标志位。

<a id="sb_ime_get_state"></a>
### sb_ime_get_state

```c
void sb_ime_get_state(unsigned char *allow, unsigned char *flags);
```

取输入法状态。

系统调用：$6D/$08

参数：

- `allow`：返回允许位。
- `flags`：返回标志位的低 7 位。

<a id="sb_ime_get_method"></a>
### sb_ime_get_method

```c
unsigned char sb_ime_get_method(void);
```

取当前输入法序号。

系统调用：$6D/$07

返回：序号 0 到 9。

<a id="sb_ime_resume"></a>
### sb_ime_resume

```c
void sb_ime_resume(void);
```

恢复被暂停的输入法：暂停标志置位时清除它并刷新状态行，否则什么都不做。

系统调用：$6D/$03

<a id="sb_ime_suspend"></a>
### sb_ime_suspend

```c
void sb_ime_suspend(void);
```

暂停输入法：清候选字，状态行清为空，暂停标志置位。已经暂停时什么都不做。暂停期间按键原样传回。

系统调用：$6D/$04

<a id="sb_ime_set_width"></a>
### sb_ime_set_width

```c
void sb_ime_set_width(unsigned char full);
```

设置半角或全角：状态行文字换成“半角”或“全角”，标志位里的全角位相应清除或置位。全角时输出的半角符号会换成全角内码。

系统调用：$6D/$05、$6D/$06

参数：

- `full`：0 半角，非 0 全角。

<a id="sb_ime_key"></a>
### sb_ime_key

```c
unsigned char sb_ime_key(sb_ime_result_t *result);
```

处理一个按键（不等待）：从键盘队列取一个键交给输入法，没有键时直接返回。
输入法的热键：Alt+F1 到 Alt+F10 选输入法 0 到 9，Ctrl+F1 暂停与恢复，Ctrl+F8、Ctrl+F9 切换全角半角，数字键 1 到 5 与空格选候选字。

系统调用：$6D/$0B

参数：

- `result`：返回处理结果。

返回：结果标志，同 result->flag：0 无输出，2 输出汉字内码，$82 按键原样传回。

<a id="sb_ime_key_wait"></a>
### sb_ime_key_wait

```c
unsigned char sb_ime_key_wait(sb_ime_result_t *result);
```

等待并处理按键，直到输入法有结果（结果标志不为 0）才返回，没有键时一直等下去。

系统调用：$6D/$0A

参数：

- `result`：返回处理结果。

返回：结果标志，同 result->flag。

<a id="sb_ime_hex"></a>
### sb_ime_hex

```c
void sb_ime_hex(unsigned int code, char *text);
```

把一个 16 位内码转成 4 个小写十六进制字符。

系统调用：$6D/$0E

参数：

- `code`：内码。
- `text`：返回 4 个字符与结尾的 0，要有 5 字节空间。

<a id="sb_ime_parse"></a>
### sb_ime_parse

```c
unsigned char sb_ime_parse(const char *chars, unsigned int *code);
```

把 4 个输入字符转成汉字内码：全是数字当区位码（4 位十进制），第一个字符是 a 到 f 当 4 位十六进制内码。

系统调用：$6D/$0D

参数：

- `chars`：4 个输入字符。
- `code`：返回内码。

返回：0 成功；1 不合法。

<a id="input"></a>
## 键盘、手柄与鼠标（sb2000-input.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_getkey`](#sb_getkey) | 等待按键并返回它的 ASCII 码。 |  |
| [`sb_key_wait`](#sb_key_wait) | 等到有按键，取出它的 ASCII 码与扫描码。 | $6B/$04 |
| [`sb_key_poll`](#sb_key_poll) | 不等待，取出队列里的一个按键。 | $6B/$03 |
| [`sb_key_count`](#sb_key_count) | 按键队列里的按键数。 | $6B/$06 |
| [`sb_key_flush`](#sb_key_flush) | 清空按键队列。 | $6B/$05 |
| [`sb_key_init`](#sb_key_init) | 复位键盘与鼠标，清空按键队列。约需几十帧。 | $6B/$00 |
| [`sb_break_enable`](#sb_break_enable) | 打开按 ESC 中断程序：之后按 ESC，系统取出上下文栈顶的恢复点（sb_context_save 保存的）并跳转过去。 | $6B/$01 |
| [`sb_break_disable`](#sb_break_disable) | 关闭按 ESC 中断程序。 | $6B/$02 |
| [`sb_key_modifiers`](#sb_key_modifiers) | 修饰键状态：位 0 右 Shift，位 1 左 Shift，位 2 Ctrl，位 3 Alt，位 4 ScrollLock，位 5 NumLock，位 6 CapsLock，位 7 Insert。 |  |
| [`sb_keyboard_reset`](#sb_keyboard_reset) | 复位并识别键盘。复位后键盘不再上报按键，需要再调用 sb_key_init 才恢复，一般用 sb_key_init 即可。 | $17 |
| [`sb_pad_init`](#sb_pad_init) | 初始化手柄状态，并打开 NMI 里读手柄的服务。 | $12/$85、$6A/$00 |
| [`sb_pad_poll`](#sb_pad_poll) | 不等待，取一个按下的手柄键。 | $6A/$01 |
| [`sb_pad_wait`](#sb_pad_wait) | 等到有手柄键按下。 | $6A/$02 |
| [`sb_pad_state`](#sb_pad_state) | 取一个手柄的原始按键状态，由 sb_pad_poll、sb_pad_wait 更新：位 7 是 A，位 6 是 B，位 5 选择，位 4 开始， |  |
| [`sb_mouse_reset`](#sb_mouse_reset) | 复位鼠标，返回鼠标是否接入。复位要尝试多次，约需几十帧；之后要用 sb_irq_enable(SB_IRQ_SOURCE_MOUSE) 允许鼠标中断， | $14 |
| [`sb_mouse_set_limits`](#sb_mouse_set_limits) | 设置鼠标光标坐标的范围，原有的双击间隔不变。 | $6C/$00 |
| [`sb_mouse_set_position`](#sb_mouse_set_position) | 设置鼠标光标的坐标。 | $6C/$04 |
| [`sb_mouse_position`](#sb_mouse_position) | 取鼠标光标的坐标。每收到一个数据包，坐标就加上位移，并限制在设置的范围内。 | $6C/$02 |
| [`sb_mouse_buttons`](#sb_mouse_buttons) | 取两个鼠标键的状态。单击、双击读走即清。 | $6C/$03 |
| [`sb_mouse_set_double_click`](#sb_mouse_set_double_click) | 设置鼠标双击的间隔：按下后在这个间隔内再次按下算双击，间隔由定时器中断计时，默认 10。 | $6C/$01 |

<a id="sb_getkey"></a>
### sb_getkey

```c
unsigned char sb_getkey(void);
```

等待按键并返回它的 ASCII 码。

返回：按键的 ASCII 码；没有字符的键（如功能键）为 0。

<a id="sb_key_wait"></a>
### sb_key_wait

```c
unsigned int sb_key_wait(void);
```

等到有按键，取出它的 ASCII 码与扫描码。

系统调用：$6B/$04

返回：低字节是 ASCII 码，高字节是扫描码。

<a id="sb_key_poll"></a>
### sb_key_poll

```c
unsigned int sb_key_poll(void);
```

不等待，取出队列里的一个按键。

系统调用：$6B/$03

返回：低字节是 ASCII 码，高字节是扫描码；队列为空时为 0。

<a id="sb_key_count"></a>
### sb_key_count

```c
unsigned char sb_key_count(void);
```

按键队列里的按键数。

系统调用：$6B/$06

返回：按键数，0 到 5。

<a id="sb_key_flush"></a>
### sb_key_flush

```c
void sb_key_flush(void);
```

清空按键队列。

系统调用：$6B/$05

<a id="sb_key_init"></a>
### sb_key_init

```c
void sb_key_init(void);
```

复位键盘与鼠标，清空按键队列。约需几十帧。

系统调用：$6B/$00

<a id="sb_break_enable"></a>
### sb_break_enable

```c
void sb_break_enable(void);
```

打开按 ESC 中断程序：之后按 ESC，系统取出上下文栈顶的恢复点（sb_context_save 保存的）并跳转过去。

系统调用：$6B/$01

<a id="sb_break_disable"></a>
### sb_break_disable

```c
void sb_break_disable(void);
```

关闭按 ESC 中断程序。

系统调用：$6B/$02

<a id="sb_key_modifiers"></a>
### sb_key_modifiers

```c
unsigned char sb_key_modifiers(void);
```

修饰键状态：位 0 右 Shift，位 1 左 Shift，位 2 Ctrl，位 3 Alt，位 4 ScrollLock，位 5 NumLock，位 6 CapsLock，位 7 Insert。

返回：修饰键状态。

<a id="sb_keyboard_reset"></a>
### sb_keyboard_reset

```c
unsigned char sb_keyboard_reset(void);
```

复位并识别键盘。复位后键盘不再上报按键，需要再调用 sb_key_init 才恢复，一般用 sb_key_init 即可。

系统调用：$17

返回：1 通过，0 失败。

<a id="sb_pad_init"></a>
### sb_pad_init

```c
void sb_pad_init(void);
```

初始化手柄状态，并打开 NMI 里读手柄的服务。

系统调用：$12/$85、$6A/$00

<a id="sb_pad_poll"></a>
### sb_pad_poll

```c
unsigned char sb_pad_poll(void);
```

不等待，取一个按下的手柄键。

系统调用：$6A/$01

返回：键码，$FF 表示没有键。

<a id="sb_pad_wait"></a>
### sb_pad_wait

```c
unsigned char sb_pad_wait(void);
```

等到有手柄键按下。

系统调用：$6A/$02

返回：键码。

<a id="sb_pad_state"></a>
### sb_pad_state

```c
unsigned char sb_pad_state(unsigned char pad);
```

取一个手柄的原始按键状态，由 sb_pad_poll、sb_pad_wait 更新：位 7 是 A，位 6 是 B，位 5 选择，位 4 开始，
位 3 上，位 2 下，位 1 左，位 0 右。

参数：

- `pad`：手柄号，0 是 1 号手柄，其他是 2 号手柄。

返回：原始状态。

<a id="sb_mouse_reset"></a>
### sb_mouse_reset

```c
unsigned char sb_mouse_reset(void);
```

复位鼠标，返回鼠标是否接入。复位要尝试多次，约需几十帧；之后要用 sb_irq_enable(SB_IRQ_SOURCE_MOUSE) 允许鼠标中断，
再设置坐标范围与初始坐标。

系统调用：$14

返回：非 0 表示鼠标已接入，0 表示没有鼠标。

<a id="sb_mouse_set_limits"></a>
### sb_mouse_set_limits

```c
void sb_mouse_set_limits(unsigned char x_min, unsigned char y_min, unsigned char x_max, unsigned char y_max);
```

设置鼠标光标坐标的范围，原有的双击间隔不变。

系统调用：$6C/$00

参数：

- `x_min`：X 下限。
- `y_min`：Y 下限。
- `x_max`：X 上限。
- `y_max`：Y 上限。

<a id="sb_mouse_set_position"></a>
### sb_mouse_set_position

```c
void sb_mouse_set_position(unsigned char x, unsigned char y);
```

设置鼠标光标的坐标。

系统调用：$6C/$04

参数：

- `x`：X 坐标。
- `y`：Y 坐标。

<a id="sb_mouse_position"></a>
### sb_mouse_position

```c
void sb_mouse_position(unsigned char *x, unsigned char *y);
```

取鼠标光标的坐标。每收到一个数据包，坐标就加上位移，并限制在设置的范围内。

系统调用：$6C/$02

参数：

- `x`：返回 X 坐标。
- `y`：返回 Y 坐标。

<a id="sb_mouse_buttons"></a>
### sb_mouse_buttons

```c
void sb_mouse_buttons(sb_mouse_buttons_t *buttons);
```

取两个鼠标键的状态。单击、双击读走即清。

系统调用：$6C/$03

参数：

- `buttons`：返回按键状态。

<a id="sb_mouse_set_double_click"></a>
### sb_mouse_set_double_click

```c
void sb_mouse_set_double_click(unsigned char interval);
```

设置鼠标双击的间隔：按下后在这个间隔内再次按下算双击，间隔由定时器中断计时，默认 10。
间隔为 0 时不产生双击。

系统调用：$6C/$01

参数：

- `interval`：间隔，单位是定时器中断的次数。

<a id="math"></a>
## 数学（sb2000-math.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_random_seed`](#sb_random_seed) | 设置伪随机数寄存器。寄存器的两个高字节全为 0 时，下一次步进前系统先把次高字节置成 $FF，所以种子 0 也能用。 | 直接写 $0696-$0699 |
| [`sb_random_next`](#sb_random_next) | 把伪随机数寄存器步进一次，返回新的寄存器值。 | $4A/$01 |
| [`sb_random_warm_up`](#sb_random_warm_up) | 把伪随机数寄存器连续步进 100 次，用来打散刚设置的种子。 | $4A/$00 |
| [`sb_math_div16`](#sb_math_div16) | 16 位数除以 8 位数。 | $4B |
| [`sb_math_time_text`](#sb_math_time_text) | 把时、分、秒转成 HH:MM:SS 文字。 | $4C |
| [`sb_float_from_text`](#sb_float_from_text) | 十进制文字转浮点数。 | $4D/$00 |
| [`sb_float_to_text`](#sb_float_to_text) | 浮点数转十进制文字。 | $4E/$00 |
| [`sb_float_add`](#sb_float_add) | 浮点加法：a + b。 | $51 |
| [`sb_float_sub`](#sb_float_sub) | 浮点减法：a - b。 | $52 |
| [`sb_float_mul`](#sb_float_mul) | 浮点乘法：a * b。 | $4F |
| [`sb_float_div`](#sb_float_div) | 浮点除法：a / b。除数的尾数为 0 时结果的第 1 个字节（阶码）为 $80。 | $50 |
| [`sb_int40_from_text`](#sb_int40_from_text) | 十进制数字文字转 40 位整数。前导空格当 0，超过 40 位时高位丢弃。 | $4D/$01 |
| [`sb_int40_to_text`](#sb_int40_to_text) | 40 位整数转十进制数字文字，数字左对齐、右边补字符 0。 | $4E/$01 |
| [`sb_line_step`](#sb_line_step) | 直线步进（布雷森汉姆算法）：从当前点向终点前进指定的步数，落点写回 x、y。步数不小于主轴长度时直接落在终点。 | $53 |
| [`sb_int24_add`](#sb_int24_add) | 3 字节整数加法：a + b，带符号。 | $54/$00 |
| [`sb_int24_sub`](#sb_int24_sub) | 3 字节整数减法：a - b，带符号。 | $54/$01 |
| [`sb_int24_mul`](#sb_int24_mul) | 3 字节整数乘法：a * b，乘积最多 48 位。 | $54/$02 |
| [`sb_int24_div`](#sb_int24_div) | 3 字节整数除法：a / b，得商与余数。 | $54/$03 |

<a id="sb_random_seed"></a>
### sb_random_seed

```c
void sb_random_seed(unsigned long seed);
```

设置伪随机数寄存器。寄存器的两个高字节全为 0 时，下一次步进前系统先把次高字节置成 $FF，所以种子 0 也能用。

系统调用：直接写 $0696-$0699

参数：

- `seed`：32 位种子。

<a id="sb_random_next"></a>
### sb_random_next

```c
unsigned long sb_random_next(void);
```

把伪随机数寄存器步进一次，返回新的寄存器值。

系统调用：$4A/$01

返回：步进后的 32 位寄存器值。

<a id="sb_random_warm_up"></a>
### sb_random_warm_up

```c
void sb_random_warm_up(void);
```

把伪随机数寄存器连续步进 100 次，用来打散刚设置的种子。

系统调用：$4A/$00

<a id="sb_math_div16"></a>
### sb_math_div16

```c
unsigned char sb_math_div16(unsigned int dividend, unsigned char divisor, unsigned int *quotient, unsigned char *remainder);
```

16 位数除以 8 位数。

系统调用：$4B

参数：

- `dividend`：被除数。
- `divisor`：除数。
- `quotient`：返回商。
- `remainder`：返回余数。

返回：0 成功；$FF 除数为 0，商与余数没有意义。

<a id="sb_math_time_text"></a>
### sb_math_time_text

```c
void sb_math_time_text(unsigned char hour, unsigned char minute, unsigned char second, char *text);
```

把时、分、秒转成 HH:MM:SS 文字。

系统调用：$4C

参数：

- `hour`：时。
- `minute`：分。
- `second`：秒。
- `text`：返回 8 个字符加结尾的 0，要有 9 字节空间。

<a id="sb_float_from_text"></a>
### sb_float_from_text

```c
void sb_float_from_text(const char *text, sb_float_t *result);
```

十进制文字转浮点数。

系统调用：$4D/$00

参数：

- `text`：13 个字符：第 1 个是符号（空格或 -），后面 12 个是带小数点的数字。
- `result`：返回浮点数。

<a id="sb_float_to_text"></a>
### sb_float_to_text

```c
void sb_float_to_text(const sb_float_t *value, char *text);
```

浮点数转十进制文字。

系统调用：$4E/$00

参数：

- `value`：浮点数。
- `text`：返回 13 个字符（符号加 12 位带小数点的数字）与结尾的 0，要有 14 字节空间。

<a id="sb_float_add"></a>
### sb_float_add

```c
void sb_float_add(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);
```

浮点加法：a + b。

系统调用：$51

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `result`：返回结果，可以与 a 或 b 是同一个变量。

<a id="sb_float_sub"></a>
### sb_float_sub

```c
void sb_float_sub(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);
```

浮点减法：a - b。

系统调用：$52

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `result`：返回结果，可以与 a 或 b 是同一个变量。

<a id="sb_float_mul"></a>
### sb_float_mul

```c
void sb_float_mul(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);
```

浮点乘法：a * b。

系统调用：$4F

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `result`：返回结果，可以与 a 或 b 是同一个变量。

<a id="sb_float_div"></a>
### sb_float_div

```c
void sb_float_div(const sb_float_t *a, const sb_float_t *b, sb_float_t *result);
```

浮点除法：a / b。除数的尾数为 0 时结果的第 1 个字节（阶码）为 $80。

系统调用：$50

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `result`：返回结果，可以与 a 或 b 是同一个变量。

<a id="sb_int40_from_text"></a>
### sb_int40_from_text

```c
void sb_int40_from_text(const char *digits, unsigned char count, unsigned char *value);
```

十进制数字文字转 40 位整数。前导空格当 0，超过 40 位时高位丢弃。

系统调用：$4D/$01

参数：

- `digits`：数字字符，不带符号与小数点。
- `count`：数字个数，0 到 12。
- `value`：返回 5 个字节的整数，高字节在前。

<a id="sb_int40_to_text"></a>
### sb_int40_to_text

```c
unsigned char sb_int40_to_text(const unsigned char *value, char *digits, unsigned char *length);
```

40 位整数转十进制数字文字，数字左对齐、右边补字符 0。

系统调用：$4E/$01

参数：

- `value`：5 个字节的整数，高字节在前。
- `digits`：返回 13 个数字字符与结尾的 0，要有 14 字节空间。
- `length`：返回去掉前导 0 后的有效位数，至少为 1。

返回：0 成功；$FF 数值不小于 10^12，放不下。

<a id="sb_line_step"></a>
### sb_line_step

```c
void sb_line_step(sb_line_t *line, unsigned char steps);
```

直线步进（布雷森汉姆算法）：从当前点向终点前进指定的步数，落点写回 x、y。步数不小于主轴长度时直接落在终点。

系统调用：$53

参数：

- `line`：参数块，调用前填好 x、y、end_x、end_y；系统服务换页后程序窗口不可见，所以函数把它复制到 ERAM 里再调用，返回后复制回来。
- `steps`：前进的步数，0 表示 256 步。

<a id="sb_int24_add"></a>
### sb_int24_add

```c
unsigned char sb_int24_add(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *result);
```

3 字节整数加法：a + b，带符号。

系统调用：$54/$00

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `result`：返回结果。

返回：0 没有溢出；1 结果超过 24 位，result 里是低 24 位。

<a id="sb_int24_sub"></a>
### sb_int24_sub

```c
unsigned char sb_int24_sub(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *result);
```

3 字节整数减法：a - b，带符号。

系统调用：$54/$01

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `result`：返回结果。

返回：0 没有溢出；1 结果超过 24 位，result 里是低 24 位。

<a id="sb_int24_mul"></a>
### sb_int24_mul

```c
void sb_int24_mul(const sb_int24_t *a, const sb_int24_t *b, unsigned char *sign, unsigned char *product);
```

3 字节整数乘法：a * b，乘积最多 48 位。

系统调用：$54/$02

参数：

- `a`：第一个操作数。
- `b`：第二个操作数。
- `sign`：返回乘积的符号，$00 正、$FF 负。
- `product`：返回 6 个字节的乘积，高字节在前。

<a id="sb_int24_div"></a>
### sb_int24_div

```c
unsigned char sb_int24_div(const sb_int24_t *a, const sb_int24_t *b, sb_int24_t *quotient, sb_int24_t *remainder);
```

3 字节整数除法：a / b，得商与余数。

系统调用：$54/$03

参数：

- `a`：被除数。
- `b`：除数。
- `quotient`：返回商，符号是商的符号。
- `remainder`：返回余数的数值，符号字节与商相同。

返回：0 成功；1 除数为 0，商与余数没有意义。

<a id="memory"></a>
## 内存页与远指针（sb2000-memory.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_page_alloc`](#sb_page_alloc) | 分配连续的扩展 RAM 页。页数为 0 时不分配，只返回第一个空闲页的页号。 | $7E/$00 |
| [`sb_page_free`](#sb_page_free) | 释放连续的扩展 RAM 页。 | $7E/$01 |
| [`sb_page_largest_free`](#sb_page_largest_free) | 求最长的连续空闲页数。 | $7E/$02 |
| [`sb_far_read_byte`](#sb_far_read_byte) | 读扩展 RAM 页里的一个字节。 |  |
| [`sb_far_write_byte`](#sb_far_write_byte) | 写扩展 RAM 页里的一个字节。 |  |
| [`sb_far_read`](#sb_far_read) | 把扩展 RAM 里的一段数据读到内存，可以跨页。数据经 $0700 缓冲中转，调用期间会改写 $0700-$07FF。 |  |
| [`sb_far_write`](#sb_far_write) | 把内存里的一段数据写到扩展 RAM，可以跨页。数据经 $0700 缓冲中转，调用期间会改写 $0700-$07FF。 |  |
| [`sb_address_to_far`](#sb_address_to_far) | 把窗口地址换算成远指针：页号 = 地址高 4 位 - 8 + 页组首页号（按 8 位回绕），页内偏移是地址的低 12 位。 | $2C |
| [`sb_window_map`](#sb_window_map) | 把一个扩展 RAM 页映射到一个窗口。调用返回后映射保持，直到再次映射。 | $29 |
| [`sb_window_map_group`](#sb_window_map_group) | 把页组的 4 个连续页映射到 4 个窗口（$8000-$BFFF），调用返回后映射保持。 | $27 |

<a id="sb_page_alloc"></a>
### sb_page_alloc

```c
unsigned char sb_page_alloc(unsigned char count, unsigned char *first_page);
```

分配连续的扩展 RAM 页。页数为 0 时不分配，只返回第一个空闲页的页号。

系统调用：$7E/$00

参数：

- `count`：页数，每页 4K。
- `first_page`：返回分配到的第一个页号；失败时为 0。

返回：0 成功；SB_ERR_NO_MEMORY 没有这么多连续的空闲页。

<a id="sb_page_free"></a>
### sb_page_free

```c
unsigned char sb_page_free(unsigned char first_page, unsigned char count);
```

释放连续的扩展 RAM 页。

系统调用：$7E/$01

参数：

- `first_page`：第一个页号，必须不小于 $80 且小于 $F0。
- `count`：页数；这个服务不检查页号加页数是否越过 $F0。

返回：0 成功；SB_ERR_BAD_ADDRESS 起始页号不合法，没有释放任何页。

<a id="sb_page_largest_free"></a>
### sb_page_largest_free

```c
unsigned char sb_page_largest_free(void);
```

求最长的连续空闲页数。

系统调用：$7E/$02

返回：页数；计数是 8 位，256 页全部空闲时回绕成 0。

<a id="sb_far_read_byte"></a>
### sb_far_read_byte

```c
unsigned char sb_far_read_byte(unsigned char page, unsigned int offset);
```

读扩展 RAM 页里的一个字节。

参数：

- `page`：页号。
- `offset`：页内偏移，超过 $0FFF 时折算到后面的页。

返回：读到的字节。

<a id="sb_far_write_byte"></a>
### sb_far_write_byte

```c
void sb_far_write_byte(unsigned char page, unsigned int offset, unsigned char value);
```

写扩展 RAM 页里的一个字节。

参数：

- `page`：页号。
- `offset`：页内偏移，超过 $0FFF 时折算到后面的页。
- `value`：要写的字节。

<a id="sb_far_read"></a>
### sb_far_read

```c
void sb_far_read(unsigned char page, unsigned int offset, void *dest, unsigned int length);
```

把扩展 RAM 里的一段数据读到内存，可以跨页。数据经 $0700 缓冲中转，调用期间会改写 $0700-$07FF。

参数：

- `page`：起始页号。
- `offset`：页内偏移，超过 $0FFF 时折算到后面的页。
- `dest`：目的地，必须在程序窗口的前 3 个窗口（$8000-$AFFF）、$6000-$7FFF 或内核 RAM 里，不能在窗口 3。
- `length`：字节数。

<a id="sb_far_write"></a>
### sb_far_write

```c
void sb_far_write(unsigned char page, unsigned int offset, const void *src, unsigned int length);
```

把内存里的一段数据写到扩展 RAM，可以跨页。数据经 $0700 缓冲中转，调用期间会改写 $0700-$07FF。

参数：

- `page`：起始页号。
- `offset`：页内偏移，超过 $0FFF 时折算到后面的页。
- `src`：数据来源，位置的要求同 sb_far_read 的 dest。
- `length`：字节数。

<a id="sb_address_to_far"></a>
### sb_address_to_far

```c
void sb_address_to_far(unsigned char group, unsigned int address, unsigned char *page, unsigned int *offset);
```

把窗口地址换算成远指针：页号 = 地址高 4 位 - 8 + 页组首页号（按 8 位回绕），页内偏移是地址的低 12 位。

系统调用：$2C

参数：

- `group`：页组的首页号。
- `address`：窗口地址，$8000-$FFFF。
- `page`：返回页号。
- `offset`：返回页内偏移。

<a id="sb_window_map"></a>
### sb_window_map

```c
void sb_window_map(unsigned char window, unsigned char page);
```

把一个扩展 RAM 页映射到一个窗口。调用返回后映射保持，直到再次映射。
窗口 0-2 可以用来临时访问别的页；程序自己占用的窗口被换走后，里面的代码与数据在恢复前看不到。
窗口 3 里有程序的软件栈，不要映射。

系统调用：$29

参数：

- `window`：窗口号 0-3，0 是 $8000，1 是 $9000，2 是 $A000，3 是 $B000。
- `page`：页号。

<a id="sb_window_map_group"></a>
### sb_window_map_group

```c
void sb_window_map_group(unsigned char group);
```

把页组的 4 个连续页映射到 4 个窗口（$8000-$BFFF），调用返回后映射保持。
调用者本身的代码所在的窗口也会换掉，所以新页组里同一地址要有同样的代码。

系统调用：$27

参数：

- `group`：页组的首页号。

<a id="ppu"></a>
## 图形端口（sb2000-ppu.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_ppu_address`](#sb_ppu_address) | 设置显存地址（$2006），之后读写 $2007 从这里开始。 |  |
| [`sb_ppu_write`](#sb_ppu_write) | 把一段数据写进显存。 |  |
| [`sb_ppu_fill`](#sb_ppu_fill) | 把显存的一段填成同一个值。 |  |
| [`sb_ppu_peek`](#sb_ppu_peek) | 读显存的一个字节。设地址后第一次读只刷新缓冲，所以函数设两次地址，取第二次读到的值。 |  |
| [`sb_ppu_read`](#sb_ppu_read) | 读一段显存。 |  |
| [`sb_ppu_palette_set`](#sb_ppu_palette_set) | 写调色板的一项（$2040 起共 64 项：背景 0-31，精灵 32-63）。 |  |
| [`sb_ppu_palette_get`](#sb_ppu_palette_get) | 读调色板的一项，只返回低 6 位（读出的位 6 恒为 1，已去掉）。 |  |
| [`sb_ppu_oam_clear`](#sb_ppu_oam_clear) | 把精灵 RAM 的 256 字节全部填成 $F0，使全部 64 个精灵都在屏幕之外。 |  |
| [`sb_ppu_oam_put`](#sb_ppu_oam_put) | 写一个精灵（4 字节）。读回时属性的位 4-2 总是 0。 |  |
| [`sb_ppu_wait_vblank`](#sb_ppu_wait_vblank) | 等垂直消隐：轮询 $2002 直到位 7 为 1，最多读 65535 次。 |  |
| [`sb_ppu_render`](#sb_ppu_render) | 设置控制寄存器与扩展控制寄存器、滚动回到 (0, 0)，最后写显示寄存器打开显示。 |  |

<a id="sb_ppu_address"></a>
### sb_ppu_address

```c
void sb_ppu_address(unsigned int address);
```

设置显存地址（$2006），之后读写 $2007 从这里开始。

参数：

- `address`：显存地址。

<a id="sb_ppu_write"></a>
### sb_ppu_write

```c
void sb_ppu_write(unsigned int address, const unsigned char *data, unsigned int count);
```

把一段数据写进显存。

参数：

- `address`：显存起始地址。
- `data`：数据。
- `count`：字节数。

<a id="sb_ppu_fill"></a>
### sb_ppu_fill

```c
void sb_ppu_fill(unsigned int address, unsigned char value, unsigned int count);
```

把显存的一段填成同一个值。

参数：

- `address`：显存起始地址。
- `value`：填充值。
- `count`：字节数。

<a id="sb_ppu_peek"></a>
### sb_ppu_peek

```c
unsigned char sb_ppu_peek(unsigned int address);
```

读显存的一个字节。设地址后第一次读只刷新缓冲，所以函数设两次地址，取第二次读到的值。

参数：

- `address`：显存地址。

返回：该地址的内容。

<a id="sb_ppu_read"></a>
### sb_ppu_read

```c
void sb_ppu_read(unsigned int address, unsigned char *dest, unsigned int count);
```

读一段显存。

参数：

- `address`：显存起始地址。
- `dest`：存放数据的缓冲。
- `count`：字节数。

<a id="sb_ppu_palette_set"></a>
### sb_ppu_palette_set

```c
void sb_ppu_palette_set(unsigned char index, unsigned char color);
```

写调色板的一项（$2040 起共 64 项：背景 0-31，精灵 32-63）。

参数：

- `index`：项号，0 到 63。
- `color`：颜色：低 4 位色相，位 5-4 亮度。

<a id="sb_ppu_palette_get"></a>
### sb_ppu_palette_get

```c
unsigned char sb_ppu_palette_get(unsigned char index);
```

读调色板的一项，只返回低 6 位（读出的位 6 恒为 1，已去掉）。

参数：

- `index`：项号，0 到 63。

返回：颜色。

<a id="sb_ppu_oam_clear"></a>
### sb_ppu_oam_clear

```c
void sb_ppu_oam_clear(void);
```

把精灵 RAM 的 256 字节全部填成 $F0，使全部 64 个精灵都在屏幕之外。

<a id="sb_ppu_oam_put"></a>
### sb_ppu_oam_put

```c
void sb_ppu_oam_put(unsigned char index, unsigned char y, unsigned char tile, unsigned char attr, unsigned char x);
```

写一个精灵（4 字节）。读回时属性的位 4-2 总是 0。

参数：

- `index`：精灵序号，0 到 63。
- `y`：垂直位置，实际显示在 y + 1 行。
- `tile`：图块号。
- `attr`：属性：位 7 垂直翻转，位 6 水平镜像，位 5 在背景之后，位 1-0 调色板组。
- `x`：水平位置。

<a id="sb_ppu_wait_vblank"></a>
### sb_ppu_wait_vblank

```c
unsigned char sb_ppu_wait_vblank(void);
```

等垂直消隐：轮询 $2002 直到位 7 为 1，最多读 65535 次。

返回：读到垂直消隐时 $2002 的值；超时返回 0。

<a id="sb_ppu_render"></a>
### sb_ppu_render

```c
void sb_ppu_render(unsigned char mask, unsigned char control, unsigned char ext);
```

设置控制寄存器与扩展控制寄存器、滚动回到 (0, 0)，最后写显示寄存器打开显示。

参数：

- `mask`：显示寄存器（$2001）的值，如 SB_HW_MASK_BG | SB_HW_MASK_SPRITE。
- `control`：控制寄存器（$2000）的值，不要带允许 NMI 的位，除非 NMI 已经准备好。
- `ext`：扩展控制寄存器（$2008）的值。

<a id="printer"></a>
## 打印机（sb2000-printer.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_printer_text`](#sb_printer_text) | 打印一段文字，到 0 结束。每行先发 ESC @ 与图形命令，再逐字符取字模发送，行满换行，行末进纸并让打印头回到左端。 | $76/$00 |
| [`sb_printer_reset`](#sb_printer_reset) | 发送复位命令 ESC @。 | $76/$01 |
| [`sb_printer_home`](#sb_printer_home) | 发送 ESC <，让打印头回到左端。 | $76/$02 |
| [`sb_printer_carriage_return`](#sb_printer_carriage_return) | 复位打印机并发送回车。 | $76/$03 |
| [`sb_printer_feed`](#sb_printer_feed) | 进纸，发送 ESC J n。 | $76/$04 |
| [`sb_printer_position`](#sb_printer_position) | 横向定位：绝对定位发送 ESC $ 加两个字节，相对定位发送 ESC \ 加两个字节。 | $76/$05、$76/$06 |
| [`sb_printer_send_byte`](#sb_printer_send_byte) | 把一个字节原样发给打印机，不做变换。 | $76/$07 |
| [`sb_printer_graphics_mode`](#sb_printer_graphics_mode) | 发送 ESC * 39 1144，进入 24 针三倍密度位图模式，每行 1144 列。 | $76/$08 |
| [`sb_printer_init_port`](#sb_printer_init_port) | 初始化并口并等待。只初始化一次，之后的调用什么都不做；其他打印机函数每次都会先调用它，所以一般不必单独调用。 | $76/$0E |
| [`sb_printer_page_length`](#sb_printer_page_length) | 按纸张类型设置页长计数（单位 1/216 英寸，进纸时从中扣除）：0 是 1900，1 是 2523（11.68 英寸，A4），2 是 2106，3 是 1488，4 是 2077，5 是 1821； | $76/$0F |
| [`sb_printer_page_left`](#sb_printer_page_left) | 取本页剩余走纸量（单位 1/216 英寸）。剩余不足 $58 时打印文字会自动收尾进纸并按纸张类型重新装入页长。 |  |

<a id="sb_printer_text"></a>
### sb_printer_text

```c
void sb_printer_text(const char *text, const sb_printer_style_t *style);
```

打印一段文字，到 0 结束。每行先发 ESC @ 与图形命令，再逐字符取字模发送，行满换行，行末进纸并让打印头回到左端。
文字里的控制码：$0D $0A 换行；$92 n 设置字宽与点数模式；$95 n 设置变换模式；$99 n 设置附加空白列；$9B n 设置换行后的进纸量，具体见系统调用手册（docs/sb2000/02-系统调用手册.md）5.11 节。

系统调用：$76/$00

参数：

- `text`：文字，零结尾，在程序窗口 $8000-$BFFF 里。
- `style`：打印格式；传 0 时用字宽 0、点数模式 0、无附加空白列、进纸量 $18。

<a id="sb_printer_reset"></a>
### sb_printer_reset

```c
void sb_printer_reset(void);
```

发送复位命令 ESC @。

系统调用：$76/$01

<a id="sb_printer_home"></a>
### sb_printer_home

```c
void sb_printer_home(void);
```

发送 ESC <，让打印头回到左端。

系统调用：$76/$02

<a id="sb_printer_carriage_return"></a>
### sb_printer_carriage_return

```c
void sb_printer_carriage_return(void);
```

复位打印机并发送回车。

系统调用：$76/$03

<a id="sb_printer_feed"></a>
### sb_printer_feed

```c
void sb_printer_feed(unsigned char units);
```

进纸，发送 ESC J n。

系统调用：$76/$04

参数：

- `units`：进纸量，单位 1/216 英寸。

<a id="sb_printer_position"></a>
### sb_printer_position

```c
void sb_printer_position(unsigned char relative, unsigned int position);
```

横向定位：绝对定位发送 ESC $ 加两个字节，相对定位发送 ESC \ 加两个字节。

系统调用：$76/$05、$76/$06

参数：

- `relative`：0 绝对定位，非 0 相对定位。
- `position`：位置，单位 1/60 英寸（绝对）或 1/120 英寸（相对），16 位。

<a id="sb_printer_send_byte"></a>
### sb_printer_send_byte

```c
void sb_printer_send_byte(unsigned char value);
```

把一个字节原样发给打印机，不做变换。

系统调用：$76/$07

参数：

- `value`：要发送的字节。

<a id="sb_printer_graphics_mode"></a>
### sb_printer_graphics_mode

```c
void sb_printer_graphics_mode(void);
```

发送 ESC * 39 1144，进入 24 针三倍密度位图模式，每行 1144 列。

系统调用：$76/$08

<a id="sb_printer_init_port"></a>
### sb_printer_init_port

```c
void sb_printer_init_port(void);
```

初始化并口并等待。只初始化一次，之后的调用什么都不做；其他打印机函数每次都会先调用它，所以一般不必单独调用。

系统调用：$76/$0E

<a id="sb_printer_page_length"></a>
### sb_printer_page_length

```c
void sb_printer_page_length(unsigned char paper);
```

按纸张类型设置页长计数（单位 1/216 英寸，进纸时从中扣除）：0 是 1900，1 是 2523（11.68 英寸，A4），2 是 2106，3 是 1488，4 是 2077，5 是 1821；
6 以上只把参数存进计数的高字节。

系统调用：$76/$0F

参数：

- `paper`：纸张类型，0 到 5。

<a id="sb_printer_page_left"></a>
### sb_printer_page_left

```c
unsigned int sb_printer_page_left(void);
```

取本页剩余走纸量（单位 1/216 英寸）。剩余不足 $58 时打印文字会自动收尾进纸并按纸张类型重新装入页长。

返回：剩余走纸量。

<a id="sound"></a>
## 声音与语音（sb2000-sound.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_sound_play_from`](#sb_sound_play_from) | 载入并播放一首曲子（音乐格式或音效格式）。先关闭声音更新，复位发声状态，起播后再打开声音更新。 | $65/$00 |
| [`sb_sound_stop`](#sb_sound_stop) | 停止播放：复位发声状态，关闭声音更新，播放状态变成 $FF。 | $65/$02 |
| [`sb_sound_status`](#sb_sound_status) | 取播放状态。 | $65/$03 |
| [`sb_sound_transpose`](#sb_sound_transpose) | 调整 4 个音乐声道的移调值：每个声道的移调值都加上 delta（8 位回绕）。 | $65/$04 |
| [`sb_sound_tempo`](#sb_sound_tempo) | 调整拍速：拍速值加上 delta（8 位回绕），同步写进备份单元。 | $65/$05 |
| [`sb_sound_effect_from`](#sb_sound_effect_from) | 复位后播放一首音效格式的曲子（音效声道 4-7）。音乐格式的曲子用它起不了播，要用 sb_sound_play_from。 | $65/$0B |
| [`sb_sound_play_wav_from`](#sb_sound_play_wav_from) | 播放内存里的 WAV（8 位采样）。屏蔽中断播放，播完才返回。 | $65/$0C |
| [`sb_sound_play_wav`](#sb_sound_play_wav) | 播放程序自己窗口里的 WAV，规则同 sb_sound_play_wav_from。 | $65/$0C |
| [`sb_sound_repeat_wav_from`](#sb_sound_repeat_wav_from) | 重复播放内存里的 WAV。次数不为 0 时先等 5 帧再开始；播完（含次数为 0）后声音寄存器恢复。 | $65/$0D |
| [`sb_speech_off`](#sb_speech_off) | 关闭语音：清语音芯片的使能位与语音控制标志的“已使用”位。 | $74/$00 |
| [`sb_speech_send_from`](#sb_speech_send_from) | 同步发送一段语音数据（LPC 码流）：先复位语音芯片，写命令字节 $0A，再把数据逐字节写给语音端口，读到结束帧后返回。 | $75/$00 |
| [`sb_speech_send`](#sb_speech_send) | 同步发送程序自己窗口里的语音数据，规则同 sb_speech_send_from。 | $75/$00 |
| [`sb_speech_send_entry_from`](#sb_speech_send_entry_from) | 按序号发送词条表里的一个词条。词条表头 2 字节是词条个数（低字节在前），之后每项 3 字节：页号增量、相对表起点的偏移低字节、偏移高字节。 | $75/$01 |

<a id="sb_sound_play_from"></a>
### sb_sound_play_from

```c
void sb_sound_play_from(unsigned char group, unsigned int address);
```

载入并播放一首曲子（音乐格式或音效格式）。先关闭声音更新，复位发声状态，起播后再打开声音更新。

系统调用：$65/$00

参数：

- `group`：曲子所在页组的首页号。
- `address`：曲子在页组窗口里的地址。

<a id="sb_sound_stop"></a>
### sb_sound_stop

```c
void sb_sound_stop(void);
```

停止播放：复位发声状态，关闭声音更新，播放状态变成 $FF。

系统调用：$65/$02

<a id="sb_sound_status"></a>
### sb_sound_status

```c
unsigned char sb_sound_status(void);
```

取播放状态。

系统调用：$65/$03

返回：播放状态，停止播放后是 $FF。

<a id="sb_sound_transpose"></a>
### sb_sound_transpose

```c
void sb_sound_transpose(unsigned char delta);
```

调整 4 个音乐声道的移调值：每个声道的移调值都加上 delta（8 位回绕）。

系统调用：$65/$04

参数：

- `delta`：要加的值，负数用补码。

<a id="sb_sound_tempo"></a>
### sb_sound_tempo

```c
void sb_sound_tempo(unsigned char delta);
```

调整拍速：拍速值加上 delta（8 位回绕），同步写进备份单元。

系统调用：$65/$05

参数：

- `delta`：要加的值，负数用补码。

<a id="sb_sound_effect_from"></a>
### sb_sound_effect_from

```c
void sb_sound_effect_from(unsigned char group, unsigned int address);
```

复位后播放一首音效格式的曲子（音效声道 4-7）。音乐格式的曲子用它起不了播，要用 sb_sound_play_from。

系统调用：$65/$0B

参数：

- `group`：曲子所在页组的首页号。
- `address`：曲子在页组窗口里的地址。

<a id="sb_sound_play_wav_from"></a>
### sb_sound_play_wav_from

```c
void sb_sound_play_wav_from(unsigned char group, unsigned int address);
```

播放内存里的 WAV（8 位采样）。屏蔽中断播放，播完才返回。
采样率取文件头偏移 $18 的低字节：$44 是 44100 Hz，$22 是 22050 Hz，其他按 11025 Hz；采样长度取偏移 $28 的 4 个字节，跳过 44 字节文件头；
实际会多播一个采样（总数是长度加 1）。

系统调用：$65/$0C

参数：

- `group`：WAV 所在页组的首页号。
- `address`：WAV 文件头在页组窗口里的地址。

<a id="sb_sound_play_wav"></a>
### sb_sound_play_wav

```c
void sb_sound_play_wav(const void *wav);
```

播放程序自己窗口里的 WAV，规则同 sb_sound_play_wav_from。

系统调用：$65/$0C

参数：

- `wav`：WAV 文件头的地址，在程序窗口 $8000-$BFFF 里。

<a id="sb_sound_repeat_wav_from"></a>
### sb_sound_repeat_wav_from

```c
void sb_sound_repeat_wav_from(unsigned char group, unsigned int address, unsigned char times);
```

重复播放内存里的 WAV。次数不为 0 时先等 5 帧再开始；播完（含次数为 0）后声音寄存器恢复。

系统调用：$65/$0D

参数：

- `group`：WAV 所在页组的首页号。
- `address`：WAV 文件头在页组窗口里的地址。
- `times`：重复的遍数。

<a id="sb_speech_off"></a>
### sb_speech_off

```c
void sb_speech_off(void);
```

关闭语音：清语音芯片的使能位与语音控制标志的“已使用”位。

系统调用：$74/$00

<a id="sb_speech_send_from"></a>
### sb_speech_send_from

```c
void sb_speech_send_from(unsigned char group, unsigned int address);
```

同步发送一段语音数据（LPC 码流）：先复位语音芯片，写命令字节 $0A，再把数据逐字节写给语音端口，读到结束帧后返回。
数据要以结束帧（SB_SPEECH_END_FRAME，低 4 位全 1）收尾，否则不会返回。

系统调用：$75/$00

参数：

- `group`：数据所在页组的首页号。
- `address`：数据在页组窗口里的地址。

<a id="sb_speech_send"></a>
### sb_speech_send

```c
void sb_speech_send(const void *data);
```

同步发送程序自己窗口里的语音数据，规则同 sb_speech_send_from。

系统调用：$75/$00

参数：

- `data`：语音数据的地址，在程序窗口 $8000-$BFFF 里。

<a id="sb_speech_send_entry_from"></a>
### sb_speech_send_entry_from

```c
void sb_speech_send_entry_from(unsigned char group, unsigned int table, unsigned int index);
```

按序号发送词条表里的一个词条。词条表头 2 字节是词条个数（低字节在前），之后每项 3 字节：页号增量、相对表起点的偏移低字节、偏移高字节。
序号不小于词条个数时什么都不发送。

系统调用：$75/$01

参数：

- `group`：词条表所在页组的首页号。
- `table`：词条表在页组窗口里的地址。
- `index`：词条序号，从 0 起。

<a id="system"></a>
## 系统与中断（sb2000-system.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_wait_frames`](#sb_wait_frames) | 等待指定数量的垂直消隐周期（帧）。 | $1B |
| [`sb_nmi_enable`](#sb_nmi_enable) | 打开 NMI（垂直消隐中断）。NMI 里的各项服务（声音更新、读手柄等）要它开着才工作。 | $12/$80 |
| [`sb_nmi_disable`](#sb_nmi_disable) | 关闭 NMI。磁盘读写这类要独占硬件的操作前后常成对使用。 | $12/$00 |
| [`sb_nmi_service`](#sb_nmi_service) | 打开或关闭 NMI 里的一项服务。服务号用 SB_NMI_SERVICE_ 开头的常量： | $12/$81-$86、$12/$01-$06 |
| [`sb_nmi_hook_set`](#sb_nmi_hook_set) | 登记一个 NMI 钩子：对应的 NMI 服务（SB_NMI_SERVICE_HOOK4、SB_NMI_SERVICE_HOOK5）打开后， |  |
| [`sb_nmi_hook_reset`](#sb_nmi_hook_reset) | 把一个 NMI 钩子恢复成空钩子。钩子号不是 4 或 5 时什么都不做。 | $12/$88 |
| [`sb_irq_mask`](#sb_irq_mask) | 屏蔽一个中断源（不再响应它）。中断源用 SB_IRQ_SOURCE_ 开头的常量： | $13/$00-$06 |
| [`sb_irq_enable`](#sb_irq_enable) | 允许一个中断源。中断源编号同 sb_irq_mask。 | $13/$80-$86 |
| [`sb_irq_hook_set`](#sb_irq_hook_set) | 登记一个中断钩子：对应的中断发生后，系统映射页组并调用入口例程。钩子是汇编例程，在中断里运行，以 RTS 返回，不能调用 C 函数。 |  |
| [`sb_irq_hook_reset`](#sb_irq_hook_reset) | 把一个中断钩子恢复成空钩子。钩子号：0 定时器，1 中断 3，2 中断 1，3 中断 4，4 鼠标，5 键盘。 | $13/$88 |
| [`sb_display_option`](#sb_display_option) | 把值按位并入显示相关的硬件寄存器并写入硬件。选项与它修改的位： | $10 |
| [`sb_display_off`](#sb_display_off) | 关闭 NMI，等到垂直消隐后关闭背景与精灵显示，用于大批量改显存前。 | $19 |
| [`sb_display_on`](#sb_display_on) | 等到垂直消隐后打开 NMI，并恢复背景与精灵显示。 | $1A |
| [`sb_system_state`](#sb_system_state) | 查询显示与系统状态。查询号与结果（位类的结果保持在原来的位上，不移位）： | $1E |
| [`sb_system_init`](#sb_system_init) | 系统硬件与状态初始化：重设显示、中断屏蔽、定时器、各钩子、键盘手柄鼠标状态、系统时钟（12:00:00）、DOS 内部状态等。 | $18 |
| [`sb_call_banked`](#sb_call_banked) | 映射页组到 $8000-$BFFF 后调用其中的例程，例程以 RTS 返回，返回后窗口恢复成调用前的样子。 | $25 |
| [`sb_jump_banked`](#sb_jump_banked) | 映射页组到 $8000-$BFFF 后跳转到其中的例程，不再回到本函数。 | $26 |
| [`sb_exec`](#sb_exec) | 运行另一个程序，运行完返回。系统按文件大小分配扩展 RAM 页、读入文件、检查文件头魔数 AA 55， | $7F/$00 |
| [`sb_stack_pointer`](#sb_stack_pointer) | 取硬件栈指针。返回的是本函数被调用之后的值，即调用者的栈指针减 2。 |  |
| [`sb_context_save`](#sb_context_save) | 把一个恢复点压入系统的上下文栈（最多 10 项，满了不保存），类似 setjmp。 | $1C |
| [`sb_context_restore`](#sb_context_restore) | 弹出上下文栈顶的恢复点并跳过去，类似 longjmp：关闭 NMI 里的各项服务与声音，恢复保存的栈指针后跳转。 | $1D |

<a id="sb_wait_frames"></a>
### sb_wait_frames

```c
void __fastcall__ sb_wait_frames(unsigned char frames);
```

等待指定数量的垂直消隐周期（帧）。

系统调用：$1B

参数：

- `frames`：帧数；0 按 256 计。

<a id="sb_nmi_enable"></a>
### sb_nmi_enable

```c
void sb_nmi_enable(void);
```

打开 NMI（垂直消隐中断）。NMI 里的各项服务（声音更新、读手柄等）要它开着才工作。

系统调用：$12/$80

<a id="sb_nmi_disable"></a>
### sb_nmi_disable

```c
void sb_nmi_disable(void);
```

关闭 NMI。磁盘读写这类要独占硬件的操作前后常成对使用。

系统调用：$12/$00

<a id="sb_nmi_service"></a>
### sb_nmi_service

```c
void sb_nmi_service(unsigned char service, unsigned char enable);
```

打开或关闭 NMI 里的一项服务。服务号用 SB_NMI_SERVICE_ 开头的常量：
SB_NMI_SERVICE_BAR_ANIM 条带动画，SB_NMI_SERVICE_SOUND 声音引擎每帧更新，
SB_NMI_SERVICE_HOOK5、SB_NMI_SERVICE_HOOK4 调用登记的钩子，SB_NMI_SERVICE_PAD 读手柄，
SB_NMI_SERVICE_PALETTE 上传 $0700 缓冲到调色板。
打开 SB_NMI_SERVICE_PAD 时系统还把 $0634 置 $FF。服务号超出 1-6 时什么都不做。

系统调用：$12/$81-$86、$12/$01-$06

参数：

- `service`：服务号，1 到 6。
- `enable`：非 0 打开，0 关闭。

<a id="sb_nmi_hook_set"></a>
### sb_nmi_hook_set

```c
void sb_nmi_hook_set(unsigned char hook, unsigned char group, unsigned int entry);
```

登记一个 NMI 钩子：对应的 NMI 服务（SB_NMI_SERVICE_HOOK4、SB_NMI_SERVICE_HOOK5）打开后，
每次 NMI 都会映射页组并调用入口例程。钩子是汇编例程，在中断里运行，以 RTS 返回，不能调用 C 函数。
钩子号不是 4 或 5 时什么都不做。

参数：

- `hook`：钩子号，4 或 5。
- `group`：钩子所在页组的首页号，自己程序里的例程传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
- `entry`：入口例程的地址。

<a id="sb_nmi_hook_reset"></a>
### sb_nmi_hook_reset

```c
void sb_nmi_hook_reset(unsigned char hook);
```

把一个 NMI 钩子恢复成空钩子。钩子号不是 4 或 5 时什么都不做。

系统调用：$12/$88

参数：

- `hook`：钩子号，4 对应 SB_NMI_SERVICE_HOOK4，5 对应 SB_NMI_SERVICE_HOOK5。

<a id="sb_irq_mask"></a>
### sb_irq_mask

```c
void sb_irq_mask(unsigned char source);
```

屏蔽一个中断源（不再响应它）。中断源用 SB_IRQ_SOURCE_ 开头的常量：
0 定时器，1 鼠标，2 键盘，3 中断 4，4 软驱，5 中断 2，6 中断 1。
屏蔽键盘时系统还向 $4022 写 $40。中断源超出 0-6 时什么都不做。

系统调用：$13/$00-$06

参数：

- `source`：中断源，0 到 6。

<a id="sb_irq_enable"></a>
### sb_irq_enable

```c
void sb_irq_enable(unsigned char source);
```

允许一个中断源。中断源编号同 sb_irq_mask。
允许定时器时系统设置定时器寄存器；允许鼠标时按鼠标是否接入更新 $0634。
注意：中断源 5、6 的允许，系统用的屏蔽值取自表后相邻字节（原程序的疏漏）：
允许中断源 5 实际清掉的是中断源 6 的屏蔽位，允许中断源 6 会把除定时器外的全部屏蔽位清 0。
不建议对这两个中断源使用。

系统调用：$13/$80-$86

参数：

- `source`：中断源，0 到 6。

<a id="sb_irq_hook_set"></a>
### sb_irq_hook_set

```c
void sb_irq_hook_set(unsigned char hook, unsigned char group, unsigned int entry);
```

登记一个中断钩子：对应的中断发生后，系统映射页组并调用入口例程。钩子是汇编例程，在中断里运行，以 RTS 返回，不能调用 C 函数。
钩子号：0 定时器，1 中断 3（系统不会调用它），2 中断 1，3 中断 4，4 鼠标，5 键盘，6 中断 2。
钩子号超出 0-6 时什么都不做。

参数：

- `hook`：钩子号，0 到 6。
- `group`：钩子所在页组的首页号，自己程序里的例程传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
- `entry`：入口例程的地址。

<a id="sb_irq_hook_reset"></a>
### sb_irq_hook_reset

```c
void sb_irq_hook_reset(unsigned char hook);
```

把一个中断钩子恢复成空钩子。钩子号：0 定时器，1 中断 3，2 中断 1，3 中断 4，4 鼠标，5 键盘。
钩子号超出 0-5 时什么都不做；系统不提供中断 2 钩子的复位。

系统调用：$13/$88

参数：

- `hook`：钩子号，0 到 5。

<a id="sb_display_option"></a>
### sb_display_option

```c
unsigned char sb_display_option(unsigned char option, unsigned char value);
```

把值按位并入显示相关的硬件寄存器并写入硬件。选项与它修改的位：
0 扩展控制 $2008 的位 7；1 $2008 的位 0；2 $2008 的位 2、3；3 $2000 的低 2 位（名称表选择）；
4 精灵显示：$2001 的位 4，同时值非 0 时打开 NMI 里的精灵 DMA；5 $2000 的位 5（8x16 精灵）；
6 $2001 只保留位 1-4 再并入；7 $2001 的位 3（背景显示）。
值要已经放在寄存器相应的位上，如选项 7 打开背景显示传 0x08。

系统调用：$10

参数：

- `option`：选项，0 到 7。
- `value`：要并入的位。

返回：0 成功；选项超出范围时为 $FF，寄存器不变。

<a id="sb_display_off"></a>
### sb_display_off

```c
void sb_display_off(void);
```

关闭 NMI，等到垂直消隐后关闭背景与精灵显示，用于大批量改显存前。

系统调用：$19

<a id="sb_display_on"></a>
### sb_display_on

```c
void sb_display_on(void);
```

等到垂直消隐后打开 NMI，并恢复背景与精灵显示。

系统调用：$1A

<a id="sb_system_state"></a>
### sb_system_state

```c
unsigned char sb_system_state(unsigned char query);
```

查询显示与系统状态。查询号与结果（位类的结果保持在原来的位上，不移位）：
0 $2008 的位 7；1 $2008 的位 0；2 $2008 的位 2、3；3 $2000 的低 2 位（名称表选择）；
4 $2001 的位 4（精灵显示）；5 $2000 的位 5（精灵尺寸）；
6 显示模式号 0-3，同时把当前字库的位置放进系统的字库参数单元；
7 NMI 服务开关 $0630 的位 1-7，位 0 换成“NMI 是否开着”；8 中断屏蔽影子；
9 鼠标是否在线（1 在线）；10 恒为 1；11 键盘自检结果；12 $2001 的位 3、4（背景与精灵是否显示）。
查询号超出 0-12 时不调用，返回 0。

系统调用：$1E

参数：

- `query`：查询号，0 到 12。

返回：查询结果。

<a id="sb_system_init"></a>
### sb_system_init

```c
void sb_system_init(void);
```

系统硬件与状态初始化：重设显示、中断屏蔽、定时器、各钩子、键盘手柄鼠标状态、系统时钟（12:00:00）、DOS 内部状态等。
程序一般不需要调用，调用后程序此前的显示与输入设置都会丢失。

系统调用：$18

<a id="sb_call_banked"></a>
### sb_call_banked

```c
void sb_call_banked(unsigned char group, unsigned int entry);
```

映射页组到 $8000-$BFFF 后调用其中的例程，例程以 RTS 返回，返回后窗口恢复成调用前的样子。
调用 ROM 里的运行库时页组为 0。要调用自己程序里的函数，页组传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。

系统调用：$25

参数：

- `group`：页组的首页号，连续 4 页。
- `entry`：例程在窗口里的地址。

<a id="sb_jump_banked"></a>
### sb_jump_banked

```c
void __fastcall__ sb_jump_banked(unsigned char group, unsigned int entry);
```

映射页组到 $8000-$BFFF 后跳转到其中的例程，不再回到本函数。
例程的 RTS 直接返回到调用本函数的位置，相当于尾调用。

系统调用：$26

参数：

- `group`：页组的首页号，连续 4 页。
- `entry`：例程在窗口里的地址。

<a id="sb_exec"></a>
### sb_exec

```c
unsigned char sb_exec(const char *path);
```

运行另一个程序，运行完返回。系统按文件大小分配扩展 RAM 页、读入文件、检查文件头魔数 AA 55，
运行完关闭文件并释放页；子程序以 RTS 返回，用的窗口与页都是它自己的，不影响本程序。

系统调用：$7F/$00

参数：

- `path`：程序的路径，零结尾，最长 28 个字符。

返回：0 成功；SB_ERR_FILE_NOT_FOUND 文件不存在；$3E 文件头魔数不符；SB_ERR_NO_MEMORY 没有足够的连续空闲页。

<a id="sb_stack_pointer"></a>
### sb_stack_pointer

```c
unsigned char sb_stack_pointer(void);
```

取硬件栈指针。返回的是本函数被调用之后的值，即调用者的栈指针减 2。

返回：硬件栈指针的低字节（高字节固定为 $01）。

<a id="sb_context_save"></a>
### sb_context_save

```c
void sb_context_save(unsigned char group, unsigned int entry, unsigned char stack_pointer);
```

把一个恢复点压入系统的上下文栈（最多 10 项，满了不保存），类似 setjmp。
之后 sb_context_restore 或按 ESC 中断（sb_break_enable）会取出它，把硬件栈指针设成保存的值并跳到入口例程。
入口例程是新的执行起点，不要用 RTS 返回。

系统调用：$1C

参数：

- `group`：入口例程所在页组的首页号，自己程序里的例程传 SB_BYTE_AT(SB_PROGRAM_START_PAGE)。
- `entry`：入口例程的地址。
- `stack_pointer`：恢复时要设的硬件栈指针，通常取 sb_stack_pointer() 的返回值。

<a id="sb_context_restore"></a>
### sb_context_restore

```c
void sb_context_restore(void);
```

弹出上下文栈顶的恢复点并跳过去，类似 longjmp：关闭 NMI 里的各项服务与声音，恢复保存的栈指针后跳转。
栈里有恢复点时不会返回；栈空时直接返回。

系统调用：$1D

<a id="text"></a>
## 文字输出（sb2000-text.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_puts`](#sb_puts) | 从行首显示零结尾字符串并换行。 |  |
| [`sb_newline`](#sb_newline) | 回到行首并换行，行满时自动滚动屏幕。 |  |
| [`sb_print`](#sb_print) | 在光标处显示字符串，光标移到字符串之后，行满时自动换行。 |  |
| [`sb_putc`](#sb_putc) | 在光标处显示一个字符，光标右移一格。换行符 \n 换行，回车符 \r 回到行首。 |  |
| [`sb_print_hex`](#sb_print_hex) | 在光标处显示一个字节的两位十六进制数（大写），光标移到它之后。 |  |
| [`sb_print_uint`](#sb_print_uint) | 在光标处显示一个无符号十进制数，光标移到它之后。 |  |
| [`sb_cursor_left`](#sb_cursor_left) | 光标左移一列，到行首时移到上一行的末尾。 |  |
| [`sb_cursor_right`](#sb_cursor_right) | 光标右移一列，到第 32 列时回到第 1 列并换行。 |  |
| [`sb_cursor_up`](#sb_cursor_up) | 光标移到上一行的末尾；已在第一行时不动。 |  |
| [`sb_cursor_set_column`](#sb_cursor_set_column) | 设置光标所在的列。列号超出 1-31 时不改。 |  |
| [`sb_cursor_column`](#sb_cursor_column) | 取光标所在的列。 |  |
| [`sb_cursor_row`](#sb_cursor_row) | 取光标所在的显示行。 |  |
| [`sb_beep`](#sb_beep) | 发一声提示音，用方波通道 2。 |  |

<a id="sb_puts"></a>
### sb_puts

```c
void __fastcall__ sb_puts(const char *s);
```

从行首显示零结尾字符串并换行。
字符码不小于 0xA1 的两个字节按 GB2312 显示一个汉字。

参数：

- `s`：零结尾的字符串。

<a id="sb_newline"></a>
### sb_newline

```c
void sb_newline(void);
```

回到行首并换行，行满时自动滚动屏幕。

<a id="sb_print"></a>
### sb_print

```c
void sb_print(const char *s);
```

在光标处显示字符串，光标移到字符串之后，行满时自动换行。

参数：

- `s`：零结尾的字符串。

<a id="sb_putc"></a>
### sb_putc

```c
void sb_putc(char c);
```

在光标处显示一个字符，光标右移一格。换行符 \n 换行，回车符 \r 回到行首。

参数：

- `c`：字符。

<a id="sb_print_hex"></a>
### sb_print_hex

```c
void sb_print_hex(unsigned char value);
```

在光标处显示一个字节的两位十六进制数（大写），光标移到它之后。

参数：

- `value`：要显示的字节。

<a id="sb_print_uint"></a>
### sb_print_uint

```c
void sb_print_uint(unsigned int value);
```

在光标处显示一个无符号十进制数，光标移到它之后。

参数：

- `value`：要显示的数，0 到 65535。

<a id="sb_cursor_left"></a>
### sb_cursor_left

```c
void sb_cursor_left(void);
```

光标左移一列，到行首时移到上一行的末尾。

<a id="sb_cursor_right"></a>
### sb_cursor_right

```c
void sb_cursor_right(void);
```

光标右移一列，到第 32 列时回到第 1 列并换行。

<a id="sb_cursor_up"></a>
### sb_cursor_up

```c
void sb_cursor_up(void);
```

光标移到上一行的末尾；已在第一行时不动。

<a id="sb_cursor_set_column"></a>
### sb_cursor_set_column

```c
void sb_cursor_set_column(unsigned char column);
```

设置光标所在的列。列号超出 1-31 时不改。

参数：

- `column`：列号，1 到 31。

<a id="sb_cursor_column"></a>
### sb_cursor_column

```c
unsigned char sb_cursor_column(void);
```

取光标所在的列。

返回：列号，从 1 起。

<a id="sb_cursor_row"></a>
### sb_cursor_row

```c
unsigned char sb_cursor_row(void);
```

取光标所在的显示行。

返回：行号，从 0 起。

<a id="sb_beep"></a>
### sb_beep

```c
void sb_beep(void);
```

发一声提示音，用方波通道 2。

<a id="timer"></a>
## 硬件定时器（sb2000-timer.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_timer_start`](#sb_timer_start) | 设置并启动定时器：先写控制字节，再写预置值（预置值同时装入当前值）。 |  |
| [`sb_timer_stop`](#sb_timer_stop) | 停止定时器：控制字节写 0。 |  |
| [`sb_timer_current`](#sb_timer_current) | 读定时器当前值（递减）。 |  |
| [`sb_timer_pending`](#sb_timer_pending) | 读定时器下溢标志（$4033 的位 7）。 |  |
| [`sb_timer_mask`](#sb_timer_mask) | 屏蔽定时器中断并清除下溢标志。 |  |
| [`sb_timer_unmask`](#sb_timer_unmask) | 允许定时器中断：之后下溢时置标志，并产生中断（中断没有被关掉时）。 |  |

<a id="sb_timer_start"></a>
### sb_timer_start

```c
void sb_timer_start(unsigned char control, unsigned char preset);
```

设置并启动定时器：先写控制字节，再写预置值（预置值同时装入当前值）。

参数：

- `control`：控制字节：位 7 允许，位 6 连续重载（0 只计一次），位 5 行消隐时钟（0 是 CPU 时钟），位 3-0 预分频。
- `preset`：预置值。

<a id="sb_timer_stop"></a>
### sb_timer_stop

```c
void sb_timer_stop(void);
```

停止定时器：控制字节写 0。

<a id="sb_timer_current"></a>
### sb_timer_current

```c
unsigned char sb_timer_current(void);
```

读定时器当前值（递减）。

返回：当前值。

<a id="sb_timer_pending"></a>
### sb_timer_pending

```c
unsigned char sb_timer_pending(void);
```

读定时器下溢标志（$4033 的位 7）。

返回：1 已下溢；0 没有。

<a id="sb_timer_mask"></a>
### sb_timer_mask

```c
void sb_timer_mask(void);
```

屏蔽定时器中断并清除下溢标志。

<a id="sb_timer_unmask"></a>
### sb_timer_unmask

```c
void sb_timer_unmask(void);
```

允许定时器中断：之后下溢时置标志，并产生中断（中断没有被关掉时）。

<a id="sb2000"></a>
## 总头文件与通用调用（sb2000.h）

| 函数 | 说明 | 系统调用 |
| --- | --- | --- |
| [`sb_syscall`](#sb_syscall) | 调用系统服务，结果由调用者读各服务规定的变量。 |  |

<a id="sb_syscall"></a>
### sb_syscall

```c
void __fastcall__ sb_syscall(unsigned char function, unsigned char sub);
```

调用系统服务，结果由调用者读各服务规定的变量。

参数：

- `function`：功能号，如 SB_FN_INPUT。
- `sub`：子功能号，如 SB_SUB_KEY_GET_WAIT；没有子功能的服务可以写 0。

## 系统调用覆盖表

BIOS 里有处理例程的每个功能号：被哪些函数封装，或者为什么不封装。需要没有封装的部分时，用 `sb_syscall` 加 `sb2000-symbols.h` 里的常量直接调用。

| 功能号 | 封装函数 | 没有封装的部分 |
| --- | --- | --- |
| $10 | sb_display_option |  |
| $11 | sb_display_mode |  |
| $12 | sb_pad_init、sb_nmi_enable、sb_nmi_disable、sb_nmi_service 等 5 个 |  |
| $13 | sb_irq_mask、sb_irq_enable、sb_irq_hook_reset |  |
| $14 | sb_mouse_reset |  |
| $15 | （不封装） | 只把 $0636 置 1，BIOS 里没有读取这个单元的指令，用途没有查清。需要时用 sb_syscall。 |
| $17 | sb_keyboard_reset |  |
| $18 | sb_system_init |  |
| $19 | sb_display_off |  |
| $1A | sb_display_on |  |
| $1B | sb_wait_frames |  |
| $1C | sb_context_save |  |
| $1D | sb_context_restore |  |
| $1E | sb_system_state |  |
| $1F | （不封装） | 把系统时钟的时、分、秒（原始计数）复制到 $069A 起，与 sb_clock_get_time（$7F/$04）取到的是同一个时钟，只是没有换算成 FAT 的 2 秒精度，没有单独封装的必要。 |
| $24 | （不封装） | 按远指针读一个字节并后移指针。sb_far_read、sb_far_read_byte 已经覆盖，它们直接调用内核的远读入口（$FFF3），比逐字节调用 $24 快。 |
| $25 | sb_call_banked |  |
| $26 | sb_jump_banked |  |
| $27 | sb_window_map_group |  |
| $28 | （不封装） | 把页号换算成 DMA 源地址的高字节，是 DMA 服务（sb_vram_load、sb_text_to_memory 等）的内部步骤，程序自己用不到。 |
| $29 | sb_bank_select、sb_window_map |  |
| $2A | sb_vram_bank |  |
| $2B | （不封装） | 按远指针写一个字节并后移指针。sb_far_write、sb_far_write_byte 已经覆盖，它们直接调用内核的远写入口（$FFF6）。 |
| $2C | sb_address_to_far |  |
| $30 | sb_screen_clear、sb_screen_flush、sb_cell_address | 子功能 $08（写单元）、$09（读单元）没有封装：屏幕缓冲在扩展 RAM 页里，用 sb_cell_address 取得地址后直接用 sb_far_read、sb_far_write 读写即可。 |
| $31 | （不封装） | 按位掩码一次填多块显存，内含分段 DMA 与调色板缓冲上传，分段细节没有实测。用 sb_vram_fill 分段填充可以得到同样的效果。需要时用 sb_syscall，参数见系统调用手册 5.7.2 节。 |
| $32 | sb_vram_fill |  |
| $33 | sb_vram_load |  |
| $34 | （不封装） | 同 $33（扩展 RAM 到显存的 DMA，已由 sb_vram_load 封装），区别是控制字不同、目的地址取自两个没有命名的单元，参数没有查清。 |
| $3B | sb_glyph_address |  |
| $3C | sb_text_draw |  |
| $3D | sb_text_draw |  |
| $3E | sb_text_draw |  |
| $3F | sb_text_draw |  |
| $40 | sb_cells_fill、sb_cells_rect、sb_cells_copy |  |
| $41 | （不封装） | 屏幕单元写入服务：每格的值依次加步长（用来排布成对的字模图块），一次写上下两格。常量填充用 sb_cells_fill、sb_cells_rect。主要给文字编辑软件整块排版用，语义复杂，需要时用 sb_syscall，参数见系统调用手册 5.12 节。 |
| $42 | （不封装） | 屏幕单元写入服务：按列写，每次连写左右两格后下移一行。理由同 $41。 |
| $43 | （不封装） | 屏幕单元写入服务：同 $41，每次只写一格。理由同 $41。 |
| $44 | （不封装） | 屏幕单元写入服务：按列写单格，下移一行。理由同 $41。 |
| $45 | sb_text_field |  |
| $46 | sb_text_field_ascii |  |
| $47 | sb_text_to_memory |  |
| $48 | sb_text_to_memory |  |
| $4A | sb_random_next、sb_random_warm_up |  |
| $4B | sb_math_div16 |  |
| $4C | sb_math_time_text |  |
| $4D | sb_float_from_text、sb_int40_from_text | 子功能 2（40 位小数与十进制小数位互转）没有封装，转换规则只有部分文字说明。 |
| $4E | sb_float_to_text、sb_int40_to_text | 子功能 2（同上）没有封装。 |
| $4F | sb_float_mul | 子功能非 0 时把结果换算成 40.40 定点数放 $06AA-$06B3，没有封装，需要时用 sb_syscall。 |
| $50 | sb_float_div | 子功能非 0 的定点数结果没有封装，理由同 $4F。 |
| $51 | sb_float_add |  |
| $52 | sb_float_sub |  |
| $53 | sb_line_step |  |
| $54 | sb_int24_add、sb_int24_sub、sb_int24_mul、sb_int24_div |  |
| $59 | sb_window_init、sb_window_save、sb_window_restore、sb_window_compact |  |
| $5A | sb_image_load_raw |  |
| $5B | sb_image_unpack_tiles |  |
| $5C | sb_image_unpack_map |  |
| $5D | sb_palette_write |  |
| $5E | sb_image_show_full |  |
| $5F | sb_image_show |  |
| $60 | sb_image_fill_rect |  |
| $61 | sb_image_unpack_to_memory |  |
| $65 | sb_sound_play_from、sb_sound_stop、sb_sound_status、sb_sound_transpose 等 9 个 | 子功能 1（与 0 只差一个单元的值）、9（起播后不重新打开声音更新，卡拉 OK 用）、$0A（不复位的音效起播）、6 到 8（原程序疑似漏写取下标的指令，不建议使用）没有封装。 |
| $66 | （不封装） | 卡拉 OK 歌词滚动：要特定格式的歌词文字流，整个流程（停声、起播、逐行排版、条带动画）只有 KARAOK.EXE 这类程序用，没有通用用途。需要时用 sb_syscall，文字流格式见 docs/sb2000/02-系统调用手册.md 第 10 节。 |
| $6A | sb_pad_init、sb_pad_poll、sb_pad_wait |  |
| $6B | sb_key_wait、sb_key_poll、sb_key_count、sb_key_flush 等 7 个 |  |
| $6C | sb_mouse_set_limits、sb_mouse_set_position、sb_mouse_position、sb_mouse_buttons 等 5 个 |  |
| $6D | sb_ime_init、sb_ime_select、sb_ime_set_state、sb_ime_get_state 等 12 个 | 子功能 $09（登记自定义输入法 6）、$0C 与 $0F 到 $12（词库查询、汉字反查读音）、$13 到 $24（转发给外挂输入法模块）没有封装；外挂模块与联想方式没有验证。 |
| $74 | sb_speech_off | 子功能 1 到 9（语音控制标志位的开关）、$0A 到 $0D（交给 NMI 逐字节发送语音数据）没有封装，需要时直接读写 $032B 与用 sb_syscall。 |
| $75 | sb_speech_send_from、sb_speech_send、sb_speech_send_entry_from | 子功能 2 及以上（按播放列表连续发送）没有封装。 |
| $76 | sb_printer_text、sb_printer_reset、sb_printer_home、sb_printer_carriage_return 等 10 个 | 子功能 9 到 $0C（字模转置、展开、逐列发送、发送空列）是 sb_printer_text 的内部步骤，没有封装。 |
| $77 | （不封装） | 与 $76 是同一个入口（分发器对 $76 到 $7A 做完全相同的事，只看子功能），统一用 sb_printer_* 系列。 |
| $78 | （不封装） | 与 $77 一样，是 $76 的同一个入口。 |
| $79 | （不封装） | 与 $77 一样，是 $76 的同一个入口。 |
| $7A | （不封装） | 与 $77 一样，是 $76 的同一个入口。 |
| $7B | sb_disk_free_bytes、sb_file_open、sb_dir_change、sb_dir_open 等 5 个 | 子功能 $00、$02、$04、$05、$07、$08、$0C 到 $12 是文件系统内部步骤（扇区定位、FAT 读写与缓存、簇链、分配与传输），由 sb_file_*、sb_dir_*、sb_disk_* 覆盖，没有单独封装；$13（复位全部句柄）每条命令开头由系统调用，程序一般不必调用。 |
| $7C | sb_file_close、sb_file_size、sb_file_read、sb_file_write 等 15 个 | 子功能 $00、$01 与 $0E（读引导扇区取磁盘参数）没有封装：磁盘参数在启动时已由 DOS 读好，更换磁盘后的重新读取没有验证。 |
| $7D | sb_dir_make、sb_dir_remove、sb_dir_current | 子功能 $00 没有封装（作用没有查清）。 |
| $7E | sb_page_alloc、sb_page_free、sb_page_largest_free |  |
| $7F | sb_clock_set_date、sb_clock_get_date、sb_clock_set_time、sb_clock_get_time 等 5 个 |  |
| $80 | sb_disk_init、sb_disk_start、sb_disk_stop、sb_disk_read_sector 等 9 个 |  |
| $81 | （不封装） | 只有子功能 0，什么都不做。 |
