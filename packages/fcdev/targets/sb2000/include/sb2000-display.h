/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-display.h
 * 说明：SB-DOS 程序的显示服务：屏幕单元（名称表缓冲）的读写与清屏、文字显示、字模查询、窗口保存与还原、调色板、显存 DMA 与填充、图片解压与显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-11
 * 备注：由 sb2000.h 引入，程序不必单独包含。屏幕是 32 列 × 30 行的单元，每个单元 2 字节（低字节在前，高 4 位是调色板），一行 64 字节；
 *       名称表缓冲在扩展 RAM 页 $FA（表 0、1，偏移 $000、$800）与页 $FB（表 2、3），写缓冲后再经 DMA 刷新到显存，所以屏幕要等刷新才变。
 *       字符串与图片数据的位置：带 group 的函数用“页组 + 窗口地址”表示（约定同 sb_call_banked）；不带 group 的函数要求数据在程序自己的窗口里。
 *       这些服务换页后程序窗口里放的是别的页，函数内部已用“页组 + 窗口地址”的方式把数据带给系统，调用者只需保证数据在程序窗口里。
 *       显示字模写进显存的图块区时，不要覆盖 DOS 屏幕正在用的图块（DOS 文字区从图块 0 起），程序要自己选用不冲突的图块号（如 $0300 以后）。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_DISPLAY_H
#define SB2000_DISPLAY_H

/** 屏幕的列数。 */
#define SB_SCREEN_COLUMNS 32
/** 屏幕的行数。 */
#define SB_SCREEN_ROWS 30

/** 窗口保存失败：坐标超出屏幕或右下角小于左上角。 */
#define SB_WINDOW_BAD_RECT 0xFD
/** 窗口保存失败：30 个窗口已用完。 */
#define SB_WINDOW_TOO_MANY 0xFE
/** 窗口保存失败：保存区装不下。 */
#define SB_WINDOW_NO_ROOM 0xFF

/** 映射流与铺满矩形的参数。 */
typedef struct {
    /** 名称表号，0 到 3。 */
    unsigned char table;
    /** 矩形左上角的列。 */
    unsigned char col;
    /** 矩形左上角的行。 */
    unsigned char row;
    /** 矩形宽，列数。 */
    unsigned char width;
    /** 矩形高，行数。 */
    unsigned char height;
    /** 效果号 0 到 8：0 逐行向下、1 逐列向右、2 逐列向左、3 逐行向下（带等待）、4 逐行向上、5 边框收缩后向外扩展、6 边框向内、7 从中线向两边、8 从两边向中线。 */
    unsigned char effect;
    /** 速度：不小于 5 时每步刷新并等 speed-5 帧，小于 5 时每 6-speed 步刷新一次。 */
    unsigned char speed;
    /** 调色板号 0 到 15。 */
    unsigned char palette;
    /** 单元基础值（图块号），映射流里的值加上它。 */
    unsigned int base;
} sb_map_t;

/**
 * 从指定位置起线性地（先列后行，到行尾接下一行）把 count 个单元写成同一个值，写完刷新一次名称表到显存。
 *
 * @syscall $40
 * @param table 名称表号，0 到 3。
 * @param col 起点列，0 到 31。
 * @param row 起点行，0 到 29。
 * @param count 单元个数，不要超过名称表里剩下的单元数；0 会被当作 65536，不要用。
 * @param cell 单元值，低 12 位是图块号，高 4 位被调色板号替换。
 * @param palette 调色板号，0 到 15。
 */
void sb_cells_fill(unsigned char table, unsigned char col, unsigned char row, unsigned int count, unsigned int cell, unsigned char palette);

/**
 * 把一个矩形区域的单元写成同一个值，每行刷新一次。
 *
 * @syscall $40
 * @param table 名称表号，0 到 3。
 * @param col 左上角列。
 * @param row 左上角行。
 * @param width 宽，列数，列加宽不超过 32。
 * @param height 高，行数，行加高不超过 30。
 * @param cell 单元值，低 12 位是图块号。
 * @param palette 调色板号，0 到 15。
 */
void sb_cells_rect(unsigned char table, unsigned char col, unsigned char row, unsigned char width, unsigned char height, unsigned int cell, unsigned char palette);

/**
 * 把程序窗口里的一块单元数据逐行复制到屏幕，写完刷新一次名称表。
 *
 * @syscall $40
 * @param table 名称表号，0 到 3。
 * @param col 左上角列。
 * @param row 左上角行。
 * @param width 宽，列数。
 * @param height 高，行数。
 * @param cells 单元数据，width × height 个，按行排列，在程序窗口里；单元值原样写入，不替换调色板位。
 */
void sb_cells_copy(unsigned char table, unsigned char col, unsigned char row, unsigned char width, unsigned char height, const unsigned int *cells);

/**
 * 清屏：把屏幕缓冲里指定名称表的全部单元填成同一个值（只写缓冲，不刷新显存）。
 *
 * @syscall $30/$0A
 * @param tables 位掩码，位 0 到 3 依次对应名称表 0 到 3（缓冲里的名称表 4 到 7）。
 * @param cell 单元值。
 */
void sb_screen_clear(unsigned char tables, unsigned int cell);

/**
 * 把一张名称表缓冲经 DMA 写到显存，并等一帧。
 *
 * @syscall $30/$00-$03
 * @param table 名称表号，0 到 3。
 */
void sb_screen_flush(unsigned char table);

/**
 * 求屏幕缓冲里一个单元的远地址，可以交给 sb_far_read、sb_far_write 读写。
 *
 * @syscall $30/$04-$07
 * @param table 名称表号，0 到 3。
 * @param col 列。
 * @param row 行。
 * @param page 返回页号。
 * @param offset 返回页内偏移。
 */
void sb_cell_address(unsigned char table, unsigned char col, unsigned char row, unsigned char *page, unsigned int *offset);

/**
 * 在屏幕上显示一行文字（可混排汉字与西文）：把字符串画成点阵写进显存的图块，再把上下两行单元写进名称表缓冲并刷新。
 * 汉字占 2 列；宽度限制在 32 列以内，汉字起点离右边界不足 2 列时不画。
 * 二平面模式下，上行第 k 列的单元值是 cell + 2k，下行再加 1。
 *
 * @syscall $45
 * @param table 名称表号，0 到 3。
 * @param col 起点列。
 * @param row 起点行，文字占上下两行。
 * @param text 以 0 结尾的字符串，在程序窗口里。
 * @param cell 起始图块号（单元值的低 12 位），文字的字模写进显存从它起的图块。
 * @param palette 调色板号，0 到 15。
 * @param color 前景色 0 到 15：每一位决定一个平面里字模是否出现。
 */
void sb_text_field(unsigned char table, unsigned char col, unsigned char row, const char *text, unsigned int cell, unsigned char palette, unsigned char color);

/**
 * 同 sb_text_field，但只处理单字节字符，不补空白图块，只写一行单元：第 i 列的单元值是 cell + 步长 × i；步长在大字体（8×16 字模，DOS 的默认）下是 2，小字体下是 1。
 *
 * @syscall $46
 * @param table 名称表号，0 到 3。
 * @param col 起点列。
 * @param row 行。
 * @param text 以 0 结尾的字符串，在程序窗口里。
 * @param cell 起始图块号。
 * @param palette 调色板号，0 到 15。
 * @param color 前景色 0 到 15。
 */
void sb_text_field_ascii(unsigned char table, unsigned char col, unsigned char row, const char *text, unsigned int cell, unsigned char palette, unsigned char color);

/**
 * 把字符串画成点阵图块，经 DMA 写进显存的图块区（目的地址 = 图块号 × 16），不写名称表。
 * 返回后 $064C/$064D 被换算成字节地址（图块号 × 16）。
 *
 * @syscall $3C、$3D、$3E、$3F
 * @param tile 起始图块号。
 * @param text 字符串，在程序窗口里。
 * @param terminator 结束符，读到该字节为止。
 * @param color 前景色 0 到 15：每一位决定一个平面里字模是否出现。
 * @param four_planes 0 画二平面图块（每个 16 字节），非 0 画四平面图块（每个 32 字节）。
 * @param ascii_only 0 处理汉字与西文，非 0 只处理单字节字符。
 */
void sb_text_draw(unsigned int tile, const char *text, unsigned char terminator, unsigned char color, unsigned char four_planes, unsigned char ascii_only);

/**
 * 把字符串画成点阵图块，写进调用者指定的扩展 RAM 地址（不经显存）。每个图块二平面 16 字节、四平面 32 字节，按平面顺序各 8 字节；
 * 汉字占两个图块宽。目的区装不下下一个字符时在该字符之前停止。目的区跨 4KB 页时窗口 2、3 的映射随之后移，输出线性连续。
 *
 * @syscall $47、$48
 * @param text 字符串，在程序窗口里。
 * @param terminator 结束符，读到该字节为止（汉字比较第一个字节）。
 * @param group 目的页组的首页号。
 * @param address 目的窗口地址（$8000 到 $FFFF，高 4 位选页，低 12 位是页内偏移），页号 = (高 4 位) - 8 + group。
 * @param capacity 目的区还能容纳的字节数。
 * @param four_planes 0 画二平面图块，非 0 画四平面图块。
 * @param color 前景色 0 到 15。
 * @param ascii_only 0 处理汉字与西文，非 0 只处理单字节字符。
 * @return 实际写入的字节数。
 */
unsigned int sb_text_to_memory(const char *text, unsigned char terminator, unsigned char group, unsigned int address, unsigned int capacity, unsigned char four_planes, unsigned char color, unsigned char ascii_only);

/**
 * 设置显示模式（显示模式影子 $0638 的高 2 位，决定文字字库区域的取法），低 6 位不变。
 *
 * @syscall $11/$00-$03
 * @param mode 显示模式 0 到 3，对应高 2 位 $00、$40、$80、$C0。
 */
void sb_display_mode(unsigned char mode);

/**
 * 查字符码对应的字模所在的页与页内地址，不绘制。字符码高字节为 0 时 $20 到 $7F 取西文字模（8×8 或 8×16，由显示模式决定），
 * 高字节不小于 $A1 时按 GB2312 区位处理：$A1 到 $A9 区是符号，$B0 到 $F7 区是汉字（32 字节，前 16 字节是每行左半边，后 16 字节是右半边）。
 *
 * @syscall $3B
 * @param code 字符码，西文放低字节，汉字是 GB2312 内码。
 * @param page 返回字模所在的页。
 * @param offset 返回字模在页内的偏移。
 * @param shape 返回字模形式（低 3 位）。
 * @return 0 成功；$FF 字符码不合法。
 */
unsigned char sb_glyph_address(unsigned int code, unsigned char *page, unsigned int *offset, unsigned char *shape);

/**
 * 初始化窗口管理：30 个窗口号全部空闲，保存区末尾回到 $8060。程序开始用窗口之前调用一次。
 *
 * @syscall $59/$06
 */
void sb_window_init(void);

/**
 * 把名称表缓冲里的一个矩形保存起来，以后用 sb_window_restore 原样写回。只保存缓冲，不碰显存。
 *
 * @syscall $59/$00-$03
 * @param table 名称表号，0 到 3。
 * @param left 左上角列。
 * @param top 左上角行。
 * @param right 右下角列（含），小于 32。
 * @param bottom 右下角行（含），小于 30。
 * @return 窗口号 1 到 30；SB_WINDOW_BAD_RECT 坐标不合法；SB_WINDOW_TOO_MANY 窗口用完；SB_WINDOW_NO_ROOM 保存区装不下，出错时窗口表与保存区不变。
 */
unsigned char sb_window_save(unsigned char table, unsigned char left, unsigned char top, unsigned char right, unsigned char bottom);

/**
 * 把保存的窗口内容写回名称表缓冲并释放窗口号。只写缓冲，不刷新显存，要用 sb_screen_flush 刷新。
 * 窗口号不小于 $1F 或表里没有时什么都不做。只有释放的是表里最后一项时才回收保存区，否则留下空洞，要用 sb_window_compact 整理。
 *
 * @syscall $59/$04
 * @param id 窗口号。
 */
void sb_window_restore(unsigned char id);

/**
 * 整理窗口保存区：没有窗口时末尾回到 $8060，否则把后面的数据前移填掉空洞。
 * 注意：第一个窗口之后的空洞，系统修正表项的方式只在空洞大小恰等于前一个窗口长度时才正确，其他情况会让表项指向乱数据，不要在窗口之间有空洞时使用。
 *
 * @syscall $59/$05
 */
void sb_window_compact(void);

/**
 * 把一组颜色写进调色板，从调色板第 offset 项起。显示开着且 NMI 开着时由 NMI 处理程序写入。
 *
 * @syscall $5D
 * @param offset 起始项，调色板共 64 项。
 * @param colors 颜色字节，在程序窗口里。
 * @param count 个数，0 什么都不做。
 */
void sb_palette_write(unsigned char offset, const unsigned char *colors, unsigned char count);

/**
 * 用一个字节填充显存的一段。长度为 0 什么都不做。
 *
 * @syscall $32
 * @param address 显存起始地址。
 * @param length 长度，字节。
 * @param value 填充值。
 */
void sb_vram_fill(unsigned int address, unsigned int length, unsigned char value);

/**
 * 把扩展 RAM 里的一段数据经 DMA 写进显存。长度为 0 什么都不做。
 *
 * @syscall $33
 * @param page 源数据所在的扩展 RAM 页。
 * @param offset 源数据在页内的偏移。
 * @param address 显存目的地址。
 * @param length 长度，字节。
 */
void sb_vram_load(unsigned char page, unsigned int offset, unsigned int address, unsigned int length);

/**
 * 设置显存 bank 寄存器（$4300）。
 *
 * @syscall $2A
 * @param bank 要写入的值。
 */
void sb_vram_bank(unsigned char bank);

/**
 * 不解压，直接把一段图案数据经 DMA 写进显存的图块区（目的地址 = 图块号 × 16），再等垂直消隐。长度为 0 直接返回。
 *
 * @syscall $5A
 * @param data 图案数据，在程序窗口里。
 * @param length 长度，字节。
 * @param tile 起始图块号。
 */
void sb_image_load_raw(const void *data, unsigned int length, unsigned int tile);

/**
 * 解压图案流并经 DMA 写进显存的图块区。流的格式：16 字节字典加指令流，指令字节 $00 结束；
 * 指令字节高 4 位 H、低 4 位 L：H 为 1 到 12 把字典第 L 项重复 H 次，H=$D 后跟 L 个原样字节，H=$E 后跟 1 个字节重复 L 次，H=$F 后跟起始字节输出 L 个递增字节。
 *
 * @syscall $5B
 * @param stream 图案流，在程序窗口里。
 * @param tile 起始图块号。
 */
void sb_image_unpack_tiles(const void *stream, unsigned int tile);

/**
 * 解压映射流并按效果写进屏幕：流里的单元值加上 map->base 与调色板位，再按效果把矩形写进名称表缓冲并刷新到显存。
 * 指令字节高 3 位 T、低 5 位 N：$00 结束；T=0 取一个 16 位值填 N 个；T=1 取一个值，N 个单元依次加 1；T=2 取 N 个 16 位值；T=3 先取一个高字节再取 N 个低字节；T 不小于 4 取一个值，N 个单元依次加 2。
 * 矩形限制在 32×32 内，起点列或行不小于 32 或宽高为 0 时什么都不写。
 *
 * @syscall $5C
 * @param stream 映射流，在程序窗口里。
 * @param map 写到屏幕的位置、效果与速度。
 */
void sb_image_unpack_map(const void *stream, const sb_map_t *map);

/**
 * 用与 sb_image_unpack_map 相同的效果把一个单元值（map->base）铺满矩形，不加调色板位。
 *
 * @syscall $60
 * @param map 矩形、效果与速度；base 是单元值。
 */
void sb_image_fill_rect(const sb_map_t *map);

/**
 * 显示压缩图片：10 字节头 [宽、高、图案数（16 位）、调色板组数、每组色数、映射流偏移（16 位）、调色板偏移（16 位）]（偏移相对头的起点），
 * 先解压图案流（头后，图块号取 map->base），再写调色板（偏移 = 调色板组号 × 每组色数，长度 = 头里的组数 × 每组色数），最后按映射流、位置、效果与速度显示。
 *
 * @syscall $5F
 * @param image 图片数据（头加图案流、映射流、调色板），在程序窗口里。
 * @param map 位置、效果、速度；base 是图案的起始图块号。
 * @param palette_group 调色板组号。
 */
void sb_image_show(const void *image, const sb_map_t *map, unsigned char palette_group);

/**
 * 显示整屏图片：头格式同 sb_image_show；图案流解压到起始图块 tile，映射流直接按顺序解到名称表缓冲的开头（单元基础值是 tile，不加调色板位），
 * 解完整张名称表 DMA 一次，调色板从第 0 项起写 $40 字节。
 *
 * @syscall $5E
 * @param image 图片数据，在程序窗口里。
 * @param tile 起始图块号。
 * @param table 名称表号，0 到 3。
 */
void sb_image_show_full(const void *image, unsigned int tile, unsigned char table);

/**
 * 把图案流解压到内存，格式同 sb_image_unpack_tiles。目的地址 address 是窗口地址（$8000 到 $FFFF，高 4 位选页，低 12 位是页内偏移），
 * 页号 = (address 高 4 位) - 8 + group；目的页与下一页映射到窗口 2、3，所以目的地址线性连续。
 *
 * @syscall $61
 * @param stream 图案流，在程序窗口里。
 * @param group 目的页组的首页号。
 * @param address 目的窗口地址。
 * @return 解出的字节数。
 */
unsigned int sb_image_unpack_to_memory(const void *stream, unsigned char group, unsigned int address);

#endif
