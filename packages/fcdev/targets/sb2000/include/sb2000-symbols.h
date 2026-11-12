/*
 * ------------------------------------------------------------------------
 * 名称：sb2000-symbols.h
 * 说明：SB-DOS 程序接口的符号常量：系统调用变量地址、功能号、子功能号、出错码、文件打开方式、运行库入口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-10
 * 备注：由 tools/gen-sb2000-header.mjs 从 asminc/sb2000.inc 生成，不要手工修改（DO NOT EDIT）；注释里的符号名省略 SB_ 前缀。
 * ------------------------------------------------------------------------
 */
#ifndef SB2000_SYMBOLS_H
#define SB2000_SYMBOLS_H

/* 系统调用：JSR SYSCALL，功能号写 SYS_FUNCTION */
/** 系统调用入口，JSR 到这里 */
#define SB_SYSCALL 0xFFF0
/** 功能号，调用前写入 */
#define SB_SYS_FUNCTION 0x063E
/** 子功能号，有子功能的服务才用 */
#define SB_SYS_SUBFUNCTION 0x063F
/** 状态，调用开头清 0，子功能越界时为 $FF */
#define SB_SYS_STATUS 0x0640
/** 页号：功能 $29 要映射的页，或远指针的页 */
#define SB_SYS_PAGE 0x0641
/** 页组的首页号，连续 4 页 */
#define SB_SYS_GROUP 0x0642
/** 窗口号 0-3，功能 $29 用 */
#define SB_SYS_WINDOW 0x0653

/* 零页：系统调用与运行库使用的单元 */
/** 调用地址低字节，功能 $25、$26 用 */
#define SB_PROG_ENTRY_LO 0x0C
/** 调用地址高字节 */
#define SB_PROG_ENTRY_HI 0x0D
/** 远指针的页内偏移低字节 */
#define SB_FAR_OFFSET_LO 0x14
/** 远指针的页内偏移高字节 */
#define SB_FAR_OFFSET_HI 0x15
/** 字符串地址低字节，LIB_PRINT_STRING 用 */
#define SB_STR_PTR_LO 0x4B
/** 字符串地址高字节 */
#define SB_STR_PTR_HI 0x4C

/* 不经 SYSCALL、直接 JSR 的内核入口 */
/** 把页组窗口地址处的数据复制到 $0700 缓冲 */
#define SB_FAR_COPY_IN 0xFFEA
/** 把 $0700 缓冲的数据复制到页组窗口地址 */
#define SB_FAR_COPY_OUT 0xFFED
/** 读远指针加 Y 处的一个字节，结果在 A */
#define SB_FAR_READ_BYTE 0xFFF3
/** 把 A 写到远指针加 Y 处 */
#define SB_FAR_WRITE_BYTE 0xFFF6

/* 功能号（写入 SYS_FUNCTION） */
/** 显示选项：把值按位并入显示相关的硬件寄存器 */
#define SB_FN_VIDEO_OPTION 0x10
/** 显示模式与字库区域登记 */
#define SB_FN_DISPLAY_MODE 0x11
/** 画面控制：开关 NMI 及其中的各项服务 */
#define SB_FN_PPU_CONTROL 0x12
/** 中断屏蔽：允许或屏蔽各中断源 */
#define SB_FN_IRQ_MASK 0x13
/** 鼠标复位，是否接入放在 $0633 */
#define SB_FN_MOUSE_RESET 0x14
/** 把 $0636 置 1，用途未确认 */
#define SB_FN_USER_FLAG 0x15
/** 键盘复位与识别 */
#define SB_FN_KEYBOARD_INIT 0x17
/** 系统硬件与状态初始化 */
#define SB_FN_SYSTEM_INIT 0x18
/** 关闭 NMI，等到垂直消隐后关闭背景与精灵显示 */
#define SB_FN_DISPLAY_OFF 0x19
/** 等到垂直消隐，开启 NMI 并恢复显示 */
#define SB_FN_DISPLAY_ON 0x1A
/** 等待 X 个垂直消隐周期（帧） */
#define SB_FN_WAIT_FRAMES 0x1B
/** 保存上下文，类似 setjmp */
#define SB_FN_SAVE_CONTEXT 0x1C
/** 恢复上下文并跳转，不返回，类似 longjmp */
#define SB_FN_RESTORE_CONTEXT 0x1D
/** 查询显示与系统状态，结果放在 $0652 */
#define SB_FN_QUERY_STATE 0x1E
/** 把系统时钟的时、分、秒复制到 $069A 起 */
#define SB_FN_CLOCK_COPY 0x1F
/** 按远指针读一个字节到 $0653，指针后移 */
#define SB_FN_FAR_READ_NEXT 0x24
/** 映射页组并调用 PROG_ENTRY 处的例程 */
#define SB_FN_CALL_BANKED 0x25
/** 同上，但直接跳转，不再返回 */
#define SB_FN_JUMP_BANKED 0x26
/** 把页组的 4 页映射到 $8000-$BFFF，调用返回后保持 */
#define SB_FN_MAP_GROUP 0x27
/** 页号换算成 DMA 源地址的高字节 */
#define SB_FN_PAGE_CONVERT 0x28
/** 把页 SYS_PAGE 映射到窗口 SYS_WINDOW */
#define SB_FN_MAP_WINDOW 0x29
/** 把 SYS_GROUP 写到显存 bank 寄存器 */
#define SB_FN_VRAM_BANK 0x2A
/** 把 $0653 写到远指针处，指针后移 */
#define SB_FN_FAR_WRITE_NEXT 0x2B
/** 把窗口地址换算成远指针 */
#define SB_FN_ADDR_TO_FAR 0x2C
/** 声音服务 */
#define SB_FN_SOUND 0x65
/** 键盘输入服务 */
#define SB_FN_INPUT 0x6B
/** 鼠标服务 */
#define SB_FN_MOUSE 0x6C
/** 手柄服务 */
#define SB_FN_GAMEPAD 0x6A
/** 文件服务：打开、关闭、设置当前目录 */
#define SB_FN_FILE 0x7B
/** 句柄服务：读写、大小、日期、属性 */
#define SB_FN_FILE_HANDLE 0x7C
/** 目录服务：创建、删除、整理名称 */
#define SB_FN_DIR 0x7D
/** 内存页服务：分配、释放 */
#define SB_FN_PAGE 0x7E
/** 日期、时间与运行程序 */
#define SB_FN_CLOCK 0x7F
/** 磁盘服务：读写扇区、软驱停转 */
#define SB_FN_DISK 0x80

/* 子功能号（写入 SYS_SUBFUNCTION） */

/* 子功能：键盘输入（FN_INPUT） */
/** 复位键盘与鼠标，清空按键队列 */
#define SB_SUB_KEY_INIT 0x00
/** 之后按 ESC 会中断程序 */
#define SB_SUB_KEY_ESC_ON 0x01
/** 按 ESC 不再中断程序 */
#define SB_SUB_KEY_ESC_OFF 0x02
/** 取一个按键，没有按键时结果为 0 */
#define SB_SUB_KEY_GET_NOWAIT 0x03
/** 等到有按键再取 */
#define SB_SUB_KEY_GET_WAIT 0x04
/** 清空按键队列 */
#define SB_SUB_KEY_FLUSH 0x05
/** 队列里的按键数，放在 KEY_ASCII */
#define SB_SUB_KEY_COUNT 0x06

/* 子功能：画面与中断控制（FN_PPU_CONTROL、FN_IRQ_MASK） */
/** FN_PPU_CONTROL：打开 NMI 中读手柄 */
#define SB_SUB_NMI_PAD_ON 0x85
/** FN_PPU_CONTROL：关闭 NMI 中读手柄 */
#define SB_SUB_NMI_PAD_OFF 0x05
/** FN_IRQ_MASK：允许鼠标中断 */
#define SB_SUB_IRQ_MOUSE_ON 0x81

/* 子功能：手柄（FN_GAMEPAD） */
/** 初始化手柄状态 */
#define SB_SUB_PAD_INIT 0x00
/** 取按键码，不等待，没有键时为 $FF */
#define SB_SUB_PAD_GET 0x01
/** 等到有按键再返回 */
#define SB_SUB_PAD_WAIT 0x02

/* 子功能：鼠标（FN_MOUSE） */
/** 设置坐标范围，上下限在 $0646-$0649 */
#define SB_SUB_MOUSE_SET_LIMITS 0x00
/** 取坐标：X 在 $0646，Y 在 $0647 */
#define SB_SUB_MOUSE_GET_POS 0x02
/** 整理按键状态到 MOUSE_BTN_BASE 起 */
#define SB_SUB_MOUSE_GET_BUTTONS 0x03
/** 设置坐标：X 取自 $0652，Y 取自 $0653 */
#define SB_SUB_MOUSE_SET_POS 0x04

/* 子功能：文件服务（FN_FILE） */
/** 按 FILE_MODE 打开 PATH_BUFFER 里的路径 */
#define SB_SUB_FS_OPEN 0x09
/** 关闭 FILE_HANDLE_OFFSET 指定的句柄 */
#define SB_SUB_FS_CLOSE 0x0A
/** 设置当前目录，相当于 CD */
#define SB_SUB_FS_FIND_ENTRY 0x0B
/** 把 4 个句柄全部置为空闲 */
#define SB_SUB_FS_RESET 0x13
/** 前进到下一扇区，结果在 FS_NEXT_LBA，区域结束时 FS_REGION_END 为 1 */
#define SB_SUB_FS_NEXT_SECTOR 0x00
/** 读 FS_LBA 号扇区到 DISK_BUFFER，必要时先把 FAT 装入缓存 */
#define SB_SUB_FS_FAT_LOAD 0x01
/** 把 FS_CLUSTER 的简号换算成该簇第一个扇区 FS_LBA */
#define SB_SUB_FS_CLUSTER_LBA 0x03
/** 从 FS_FREE_START 起统计空闲空间，字节数累加到 ARG_069A */
#define SB_SUB_FS_FREE_SPACE 0x06

/* 子功能：句柄服务（FN_FILE_HANDLE） */
/** 关闭句柄，并让软驱停转 */
#define SB_SUB_FH_CLOSE_FLUSH 0x02
/** 按偏移与长度读到扩展 RAM 页 */
#define SB_SUB_FH_READ_AT 0x05
/** 按偏移与长度从扩展 RAM 页写入 */
#define SB_SUB_FH_WRITE_AT 0x06
/** 取文件大小到 FILE_SIZE */
#define SB_SUB_FH_GET_SIZE 0x0D
/** 把整个文件读入扩展 RAM 页 */
#define SB_SUB_FH_READ_FILE 0x0F
/** 把扩展 RAM 页的内容整个写入文件 */
#define SB_SUB_FH_WRITE_FILE 0x10
/** 删除 PATH_BUFFER 里的文件 */
#define SB_SUB_FH_DELETE 0x03
/** 把句柄的文件改名为 PATH_BUFFER 里的新名称 */
#define SB_SUB_FH_RENAME 0x04
/** 设置句柄的日期（FILE_DATE） */
#define SB_SUB_FH_SET_DATE 0x07
/** 取得句柄的日期到 FILE_DATE */
#define SB_SUB_FH_GET_DATE 0x08
/** 设置句柄的时间（FILE_TIME） */
#define SB_SUB_FH_SET_TIME 0x09
/** 取得句柄的时间到 FILE_TIME */
#define SB_SUB_FH_GET_TIME 0x0A
/** 设置句柄的属性（FILE_ATTR） */
#define SB_SUB_FH_SET_ATTR 0x0B
/** 取得句柄的属性到 FILE_ATTR */
#define SB_SUB_FH_GET_ATTR 0x0C

/* 子功能：目录服务（FN_DIR） */
/** 把当前目录名整理成 \NAME.EXT 放入 PATH_BUFFER */
#define SB_SUB_DIR_FORMAT_NAME 0x01
/** 创建目录 */
#define SB_SUB_DIR_MAKE 0x02
/** 删除目录，目录必须为空 */
#define SB_SUB_DIR_DELETE 0x03

/* 子功能：内存页服务（FN_PAGE） */
/** 按 ALLOC_COUNT 分配连续页，起始页号在 ALLOC_START */
#define SB_SUB_PAGE_ALLOC 0x00
/** 释放 ALLOC_START 起的 ALLOC_COUNT 个页 */
#define SB_SUB_PAGE_FREE 0x01
/** 求最长连续空闲页数，放在 ALLOC_COUNT */
#define SB_SUB_PAGE_LARGEST 0x02

/* 子功能：日期、时间与运行程序（FN_CLOCK） */
/** 运行 PATH_BUFFER 里的程序 */
#define SB_SUB_CLOCK_EXEC 0x00
/** 用 FILE_DATE 设置系统日期 */
#define SB_SUB_CLOCK_SET_DATE 0x01
/** 取系统日期到 FILE_DATE */
#define SB_SUB_CLOCK_GET_DATE 0x02
/** 用 FILE_TIME 设置系统时间 */
#define SB_SUB_CLOCK_SET_TIME 0x03
/** 取系统时间到 FILE_TIME */
#define SB_SUB_CLOCK_GET_TIME 0x04

/* 子功能：磁盘服务（FN_DISK） */
/** 按 DISK_ 参数连续读扇区 */
#define SB_SUB_DISK_READ 0x04
/** 按 DISK_ 参数连续写扇区 */
#define SB_SUB_DISK_WRITE 0x05
/** 软驱停转，每次文件操作之后都要调用 */
#define SB_SUB_DISK_STOP 0x07
/** 启动软驱、复位到 0 磁道，读出状态后停转 */
#define SB_SUB_DISK_INIT 0x00
/** 格式化一个磁道 */
#define SB_SUB_DISK_FORMAT 0x01
/** 读一个扇区到 DISK_BUFFER */
#define SB_SUB_DISK_READ_ONE 0x02
/** 把 DISK_BUFFER 写到一个扇区 */
#define SB_SUB_DISK_WRITE_ONE 0x03
/** 启动软驱 */
#define SB_SUB_DISK_START 0x06
/** 逐个读一条磁道的 18 个扇区，读不出的在位图里置位 */
#define SB_SUB_DISK_SCAN 0x08

/* 子功能：声音服务（FN_SOUND） */
/** 播放内存里的 WAV，播完才返回 */
#define SB_SUB_SOUND_PLAY_WAV 0x0C

/* 变量：出错码、命令行与显示（扩展 RAM） */
/** 出错码，0 为成功，每次调用开头清 0 */
#define SB_DOS_ERROR 0x6300
/** 运行库显示字符串所在的页组，高 6 位有效 */
#define SB_LIB_GROUP 0x6308
/** 光标列，从 1 起，每行 32 列 */
#define SB_CURSOR_COL 0x630C
/** 光标所在的显示行 */
#define SB_CURSOR_ROW 0x630D
/** 字形缓冲行，由运行库维护，程序不要改 */
#define SB_GLYPH_ROW 0x630E
/** 命令行参数的起点，大写、0 结尾，首字节是空格 */
#define SB_CMD_TAIL 0x6364
/** 字号 */
#define SB_FONT_SIZE 0x6428

/* 变量：文件与目录（扩展 RAM） */
/** 当前目录的 11 字节名称，不要随便改写 */
#define SB_DIR_NAME83 0x651D
/** 路径，0 结尾的 ASCII，可带 A: 与 \ */
#define SB_PATH_BUFFER 0x6543
/** 打开方式，0-5 */
#define SB_FILE_MODE 0x6560
/** 当前句柄在句柄表里的偏移 */
#define SB_FILE_HANDLE_OFFSET 0x6561
/** 文件内偏移，3 字节，低字节在前 */
#define SB_FILE_OFFSET_0 0x6565
/** 文件内偏移第 2 字节 */
#define SB_FILE_OFFSET_1 0x6566
/** 文件内偏移第 3 字节 */
#define SB_FILE_OFFSET_2 0x6567
/** 文件大小或读写长度，3 字节，低字节在前 */
#define SB_FILE_SIZE_0 0x6568
/** 文件大小或读写长度第 2 字节 */
#define SB_FILE_SIZE_1 0x6569
/** 文件大小或读写长度第 3 字节 */
#define SB_FILE_SIZE_2 0x656A
/** 读写整个文件或一段数据时的扩展 RAM 起始页号 */
#define SB_FILE_START_PAGE 0x656B
/** 日期低字节，FAT 格式：年（自 1980）、月、日 */
#define SB_FILE_DATE_LO 0x656E
/** 日期高字节 */
#define SB_FILE_DATE_HI 0x656F
/** 时间低字节，FAT 格式：时、分、秒的一半 */
#define SB_FILE_TIME_LO 0x6570
/** 时间高字节 */
#define SB_FILE_TIME_HI 0x6571
/** 属性：位 0 只读，位 3 卷标，位 4 目录，位 5 存档 */
#define SB_FILE_ATTR 0x6572

/* 变量：文件系统内部（扩展 RAM） */
/** 扇区缓冲，512 字节 */
#define SB_DISK_BUFFER 0x6000
/** 当前目录的第一个扇区号，16 位，根目录为 19 */
#define SB_FS_CURRENT_DIR_LBA 0x6528
/** 要读写的扇区号，16 位，低字节在前 */
#define SB_FS_LBA 0x652E
/** 统计空闲空间的起始簇号，16 位 */
#define SB_FS_FREE_START 0x6530
/** 簇号，16 位 */
#define SB_FS_CLUSTER 0x6536
/** 句柄表起点，共 4 项，每项 $2C 字节 */
#define SB_FILE_HANDLES 0x643B
/** 句柄表项里起始簇号的偏移，16 位 */
#define SB_FH_CLUSTER_OFFSET 0x15
/** 句柄表项里文件大小的偏移，3 字节，低字节在前 */
#define SB_FH_SIZE_OFFSET 0x0E
/** 前进到下一扇区的结果，16 位 */
#define SB_FS_NEXT_LBA 0x65A9
/** 下一扇区越出区域或簇链到尾时为 1 */
#define SB_FS_REGION_END 0x65C5
/** 数学与空闲空间统计的参数区起点 */
#define SB_ARG_069A 0x069A

/* 变量：内存页分配（扩展 RAM） */
/** 要分配或释放的页数 */
#define SB_ALLOC_COUNT 0x65A6
/** 分配得到的起始页号，或要释放的起始页号 */
#define SB_ALLOC_START 0x65A7

/* 变量：磁盘参数（扩展 RAM） */
/** 面，0 或 1 */
#define SB_DISK_HEAD 0x6200
/** 起始扇区，1-18 */
#define SB_DISK_SECTOR 0x6201
/** 磁道，0-79 */
#define SB_DISK_TRACK 0x6202
/** 缓冲所在的扩展 RAM 页号 */
#define SB_DISK_PAGE 0x6203
/** 缓冲从页内第几个 512 字节起，0-7 */
#define SB_DISK_PAGE_SECTOR 0x6204
/** 扇区数低字节 */
#define SB_DISK_COUNT_LO 0x6205
/** 扇区数高字节 */
#define SB_DISK_COUNT_HI 0x6206
/** 方式：$80 出错时立即返回 */
#define SB_DISK_MODE 0x6207
/** 结果状态，0 为成功 */
#define SB_DISK_STATUS 0x6208
/** 扫描磁道的坏扇区位图，3 字节，位 7 起对应扇区 1-8、9-16、17-18 */
#define SB_DISK_BAD_MAP 0x6237

/* 变量：键盘、手柄、鼠标（内核 RAM） */
/** 取到的按键的 ASCII 码，没有字符的键为 0 */
#define SB_KEY_ASCII 0x034B
/** 取到的按键的扫描码 */
#define SB_KEY_SCAN 0x034C
/** 手柄按键码，$FF 为没有键 */
#define SB_PAD_KEY 0x034D
/** 1 号手柄的原始状态，位 7 是 A，位 0 是右 */
#define SB_PAD1_BITS 0x034E
/** 2 号手柄的原始状态 */
#define SB_PAD2_BITS 0x034F
/** 鼠标光标的 X 坐标 */
#define SB_MOUSE_X 0x0354
/** 鼠标光标的 Y 坐标 */
#define SB_MOUSE_Y 0x0355
/** 鼠标双击的间隔参数，默认 $0A */
#define SB_MOUSE_PARAM 0x035E
/** 键盘修饰键状态：位 0 右 Shift，1 左 Shift，2 Ctrl，3 Alt，4 ScrollLock，5 NumLock，6 CapsLock，7 Insert */
#define SB_KBD_FLAGS 0x0344
/** 键盘指示灯状态：位 0 ScrollLock，1 NumLock，2 CapsLock */
#define SB_KBD_LEDS 0x0348
/** 键盘复位与识别（FN_KEYBOARD_INIT）的结果，1 为通过 */
#define SB_KBD_SELF_TEST_RESULT 0x0635
/** 鼠标按键状态的起点，共 8 个字节 */
#define SB_MOUSE_BTN_BASE 0x0360
/** 鼠标复位后非 0 表示检测到鼠标，0 为没有（BIOS 里它是鼠标数据包的字节序号） */
#define SB_MOUSE_PRESENT 0x0633

/* 常量：NMI 服务号、中断源号（传给 FN_PPU_CONTROL、FN_IRQ_MASK 的子功能） */
/** NMI 服务：条带动画（卡拉 OK 歌词滚动） */
#define SB_NMI_SERVICE_BAR_ANIM 0x01
/** NMI 服务：声音引擎每帧更新 */
#define SB_NMI_SERVICE_SOUND 0x02
/** NMI 服务：调用登记的钩子 5 */
#define SB_NMI_SERVICE_HOOK5 0x03
/** NMI 服务：调用登记的钩子 4 */
#define SB_NMI_SERVICE_HOOK4 0x04
/** NMI 服务：读手柄 */
#define SB_NMI_SERVICE_PAD 0x05
/** NMI 服务：把 $0700 缓冲上传到调色板 */
#define SB_NMI_SERVICE_PALETTE 0x06
/** 中断源：定时器 */
#define SB_IRQ_SOURCE_TIMER 0x00
/** 中断源：鼠标 */
#define SB_IRQ_SOURCE_MOUSE 0x01
/** 中断源：键盘 */
#define SB_IRQ_SOURCE_KEYBOARD 0x02
/** 中断源：中断 4（只有钩子） */
#define SB_IRQ_SOURCE_IRQ4 0x03
/** 中断源：软驱 */
#define SB_IRQ_SOURCE_FLOPPY 0x04
/** 中断源：中断 2（只有钩子） */
#define SB_IRQ_SOURCE_IRQ2 0x05
/** 中断源：中断 1（只有钩子） */
#define SB_IRQ_SOURCE_IRQ1 0x06

/* 程序文件头里由 DOS 写入的单元 */
/** 程序装入的起始扩展 RAM 页号，也就是程序所在页组的首页 */
#define SB_PROGRAM_START_PAGE 0x8005
/** 程序占用的页数 */
#define SB_PROGRAM_PAGE_COUNT 0x8006

/* 变量：显示、中断与窗口的影子（内核 RAM） */
/** PPU 控制寄存器（$2000）的影子，位 7 是 NMI 开关 */
#define SB_PPU_CTRL_SHADOW 0x062A
/** PPU 显示寄存器（$2001）的影子，位 3 背景，位 4 精灵 */
#define SB_PPU_MASK_SHADOW 0x062B
/** 扩展控制寄存器（$2008）的影子 */
#define SB_PPU_EXT_CTRL_SHADOW 0x062E
/** 中断屏蔽（$4032）的影子，位为 1 表示屏蔽 */
#define SB_IRQ_MASK_SHADOW 0x062F
/** NMI 里各项服务的开关 */
#define SB_NMI_FLAGS 0x0630
/** 字形模式，高 2 位是显示模式 */
#define SB_GLYPH_MODE 0x0638
/** 窗口 0（$8000）页号的影子 */
#define SB_WINDOW_SAVE_0 0x0639
/** 窗口 1（$9000）页号的影子 */
#define SB_WINDOW_SAVE_1 0x063A
/** 窗口 2（$A000）页号的影子 */
#define SB_WINDOW_SAVE_2 0x063B
/** 窗口 3（$B000）页号的影子 */
#define SB_WINDOW_SAVE_3 0x063C
/** 跨页复制用的中转缓冲 */
#define SB_COPY_BUFFER 0x0700
/** NMI 钩子 5 的地址（低），地址在零页，钩子由 NMI_FLAGS 位 5 打开 */
#define SB_NMI_HOOK5_VEC_LO 0x1E
/** NMI 钩子 4 的地址（低），钩子由 NMI_FLAGS 位 4 打开 */
#define SB_NMI_HOOK4_VEC_LO 0x20
/** 定时器钩子的地址（低） */
#define SB_TIMER_HOOK_VEC_LO 0x24
/** 中断 3 钩子的地址（低），系统不会调用它 */
#define SB_IRQ3_HOOK_VEC_LO 0x26
/** 中断 1 钩子的地址（低） */
#define SB_IRQ1_HOOK_VEC_LO 0x28
/** 中断 2 钩子的地址（低） */
#define SB_IRQ2_HOOK_VEC_LO 0x2A
/** 中断 4 钩子的地址（低） */
#define SB_IRQ4_HOOK_VEC_LO 0x2C
/** 键盘钩子的地址（低） */
#define SB_KBD_HOOK_VEC_LO 0x2E
/** 鼠标钩子的地址（低） */
#define SB_MOUSE_HOOK_VEC_LO 0x30
/** NMI 钩子 5 所在页组的首页号 */
#define SB_NMI_HOOK5_PAGE 0x0306
/** NMI 钩子 4 所在页组的首页号 */
#define SB_NMI_HOOK4_PAGE 0x0307
/** 键盘钩子所在页组的首页号 */
#define SB_KBD_HOOK_PAGE 0x068E
/** 鼠标钩子所在页组的首页号 */
#define SB_MOUSE_HOOK_PAGE 0x068F
/** 定时器钩子所在页组的首页号 */
#define SB_TIMER_HOOK_PAGE 0x0690
/** 中断 3 钩子所在页组的首页号 */
#define SB_IRQ3_HOOK_PAGE 0x0691
/** 中断 1 钩子所在页组的首页号 */
#define SB_IRQ1_HOOK_PAGE 0x0692
/** 中断 2 钩子所在页组的首页号 */
#define SB_IRQ2_HOOK_PAGE 0x0693
/** 中断 4 钩子所在页组的首页号 */
#define SB_IRQ4_HOOK_PAGE 0x0694
/** 每次 NMI 加 1 的计数 */
#define SB_NMI_COUNTER_UP 0x0304
/** 鼠标不可用标志：允许鼠标中断后，在线清 0，否则 $FF */
#define SB_MOUSE_ABSENT 0x0634
/** 按 ESC 中断开关：非 0 时按 ESC 会恢复上下文栈顶的恢复点 */
#define SB_ESC_BREAK 0x063D
/** 上下文栈里的恢复点个数减 1，空栈为 $FF */
#define SB_CONTEXT_COUNT 0x0655
/** 软件时钟：秒 */
#define SB_CLOCK_SECONDS 0x068A
/** 软件时钟：分 */
#define SB_CLOCK_MINUTES 0x068B
/** 软件时钟：时 */
#define SB_CLOCK_HOURS 0x068C
/** 软件时钟过午夜标志：设置日期时清 0，非 0 时取日期要先加 1 天 */
#define SB_CLOCK_MIDNIGHT_FLAG 0x068D

/* 变量：系统调用的参数单元，含义随服务而定（内核 RAM） */
/** 鼠标 X 下限，或显存地址高字节 */
#define SB_PARAM_0646 0x0646
/** 鼠标 Y 下限，或显存地址低字节 */
#define SB_PARAM_0647 0x0647
/** 鼠标 X 上限，或屏幕缓冲的列 */
#define SB_PARAM_0648 0x0648
/** 鼠标 Y 上限，或屏幕缓冲的行 */
#define SB_PARAM_0649 0x0649
/** 个数、长度或字节数，含义随服务而定 */
#define SB_PARAM_064F 0x064F
/** 通用参数，多数服务从这里取输入 */
#define SB_PARAM_0652 0x0652
/** 通用参数或返回值，如读到的字节 */
#define SB_PARAM_0653 0x0653

/* 文件打开方式（FILE_MODE） */
/** 打开已有文件 */
#define SB_MODE_OPEN 0x00
/** 创建文件，已存在则覆盖 */
#define SB_MODE_CREATE 0x01
/** 打开已有文件，可按偏移写，关闭时写回目录项 */
#define SB_MODE_UPDATE 0x02
/** 删除文件：标为已删除并释放簇链 */
#define SB_MODE_DELETE_ENTRY 0x03
/** 取得目录项，只对根目录与子目录成功 */
#define SB_MODE_ENTRY 0x04
/** 创建目录项 */
#define SB_MODE_MAKE_DIR 0x05

/* 出错码（DOS_ERROR） */
/** 无错 */
#define SB_ERR_NONE 0x00
/** 无效的功能调用号 */
#define SB_ERR_BAD_FUNCTION 0x01
/** 文件未找到 */
#define SB_ERR_FILE_NOT_FOUND 0x02
/** 路径未找到 */
#define SB_ERR_PATH_NOT_FOUND 0x03
/** 打开的文件过多，4 个句柄都在用 */
#define SB_ERR_TOO_MANY_FILES 0x04
/** 访问被拒绝，如写只读文件 */
#define SB_ERR_ACCESS_DENIED 0x05
/** 无足够内存，没有足够的连续空闲页 */
#define SB_ERR_NO_MEMORY 0x08
/** 无效的内存地址，页号不合法 */
#define SB_ERR_BAD_ADDRESS 0x09
/** 驱动器未准备好 */
#define SB_ERR_NOT_READY 0x15
/** 磁盘操作参数错，面、磁道超出范围 */
#define SB_ERR_BAD_DISK_PARAM 0x10
/** 磁盘写保护，写操作失败 */
#define SB_ERR_WRITE_PROTECT 0x13
/** 文件读写位置错误 */
#define SB_ERR_BAD_POSITION 0x14
/** 循环冗余校验错 */
#define SB_ERR_CRC 0x17
/** 驱动器中没有磁盘 */
#define SB_ERR_NO_DISK 0x18
/** 未知磁盘类型 */
#define SB_ERR_UNKNOWN_DISK 0x1A
/** 文件已经存在 */
#define SB_ERR_FILE_EXISTS 0x20
/** 参数无效 */
#define SB_ERR_BAD_PARAMETER 0x25
/** 目录已经存在 */
#define SB_ERR_DIR_EXISTS 0x26
/** 文件名或目录名错误 */
#define SB_ERR_BAD_NAME 0x27
/** 文件打开方式错误 */
#define SB_ERR_BAD_OPEN_MODE 0x28
/** 企图删除非空目录 */
#define SB_ERR_DIR_NOT_EMPTY 0x29
/** 目录嵌套层数过多 */
#define SB_ERR_DIR_TOO_DEEP 0x2A
/** 磁盘空间不够 */
#define SB_ERR_NO_SPACE 0x2C
/** 文件名重复，如要建的目录与文件同名 */
#define SB_ERR_DUPLICATE_NAME 0x2D
/** 文件未打开 */
#define SB_ERR_NOT_OPEN 0x2E

/* 运行库入口（ROM 页组 0，经 FN_CALL_BANKED 且 SYS_GROUP=0 调用） */
/** 显示 A 的十六进制，经功能 $25 调用时 A 传不进去，不能用 */
#define SB_LIB_PRINT_HEX 0x8047
/** 发一声提示音 */
#define SB_LIB_BEEP 0x802D
/** 光标左移一列，到行首移到上一行末尾 */
#define SB_LIB_CURSOR_LEFT 0x8081
/** 光标移到上一行末尾，已在第一行时不动 */
#define SB_LIB_CURSOR_UP 0x808F
/** 光标右移一列，到第 32 列换行 */
#define SB_LIB_CURSOR_RIGHT 0x80CF
/** 换行，行满滚屏；调用前 CURSOR_COL 要置 1 */
#define SB_LIB_NEWLINE 0x80E2
/** 等待按键，结果在 KEY_ASCII、KEY_SCAN */
#define SB_LIB_GET_KEY 0x81F4
/** 显示消息号为 LIB_GROUP 值的内置消息 */
#define SB_LIB_PRINT_MESSAGE 0x8202
/** 在光标处显示 STR_PTR 指向的字符串，不移动光标 */
#define SB_LIB_PRINT_STRING 0x8262
/** 伪随机数：32 位移位寄存器 RANDOM_STATE */
#define SB_FN_RANDOM 0x4A
/** 16 位除以 8 位 */
#define SB_FN_DIV16 0x4B
/** 时、分、秒转成 HH:MM:SS 文字 */
#define SB_FN_TIME_TEXT 0x4C
/** 十进制文字转数值 */
#define SB_FN_DECIMAL_PARSE 0x4D
/** 数值转十进制文字 */
#define SB_FN_DECIMAL_FORMAT 0x4E
/** 浮点乘 */
#define SB_FN_FLOAT_MUL 0x4F
/** 浮点除 */
#define SB_FN_FLOAT_DIV 0x50
/** 浮点加 */
#define SB_FN_FLOAT_ADD 0x51
/** 浮点减 */
#define SB_FN_FLOAT_SUB 0x52
/** 直线步进（布雷森汉姆算法） */
#define SB_FN_LINE_STEP 0x53
/** 3 字节整数的加、减、乘、除 */
#define SB_FN_INT24 0x54
/** 伪随机数的 32 位寄存器，低字节在前 */
#define SB_RANDOM_STATE 0x0696
/** 第二个 3 字节整数的起点 */
#define SB_ARG_069E 0x069E
/** 第二个浮点数的起点 */
#define SB_ARG_06A1 0x06A1
/** 浮点运算结果的起点 */
#define SB_ARG_06A8 0x06A8
/** 40 位整数结果与时间文字的起点 */
#define SB_ARG_06AA 0x06AA
/** 40 位整数转文字的输入起点 */
#define SB_ARG_06AF 0x06AF
/** 零页：直线步进参数块的指针，低字节在前 */
#define SB_LINE_BLOCK_PTR 0x0008
/** 扩展 RAM 高端的 18 字节，放直线步进的参数块：功能 $53 换页后程序窗口里的内容不可见，参数块必须放在任何映射下都可见的地方 */
#define SB_LINE_BLOCK_BUFFER 0x7EE0
/** 载入并播放曲子，先关声音更新，播完前再打开 */
#define SB_SUB_SOUND_PLAY 0x00
/** 停止播放 */
#define SB_SUB_SOUND_STOP 0x02
/** 把 SOUND_STATUS 取到 PARAM_0653 */
#define SB_SUB_SOUND_STATUS 0x03
/** 把 PARAM_0653 加到 4 个声道的移调值 */
#define SB_SUB_SOUND_TRANSPOSE 0x04
/** 把 PARAM_0653 加到拍速 */
#define SB_SUB_SOUND_TEMPO 0x05
/** 播放内存里的 WAV */
#define SB_SUB_SOUND_WAV 0x0C
/** 重复播放 WAV，次数在 PARAM_0652 */
#define SB_SUB_SOUND_WAV_LOOP 0x0D
/** 先复位再按曲目号播放音效曲子 */
#define SB_SUB_SOUND_EFFECT 0x0B
/** 播放状态：停止后为 $FF */
#define SB_SOUND_STATUS 0x03A1
/** 4 个声道的移调值，连续 4 个字节 */
#define SB_SOUND_TRANSPOSE 0x0447
/** 拍速 */
#define SB_SOUND_TEMPO 0x03D4
/** 语音控制标志的开关 */
#define SB_FN_SPEECH_FLAGS 0x74
/** 发送语音数据 */
#define SB_FN_SPEECH_SEND 0x75
/** 语音控制标志，位 0 为 0 同步发送、为 1 交给 NMI 发送 */
#define SB_SPEECH_FLAGS 0x032B
/** 显示与语音服务的参数：字符码高字节、行距、词条序号高字节等 */
#define SB_PARAM_064A 0x064A
/** 显示与语音服务的参数：字符码低字节、宽、词条序号低字节等 */
#define SB_PARAM_064B 0x064B
/** 打印机发送变换模式等参数 */
#define SB_PARAM_064E 0x064E
/** 打印机服务，$76-$7A 是同一个入口，只看子功能 */
#define SB_FN_PRINTER 0x76
/** 中文输入法服务 */
#define SB_FN_IME 0x6D
/** 本页剩余走纸量，16 位，高字节在前（单位 1/216 英寸） */
#define SB_PRN_PAGE_LEFT 0x04BC
/** 子功能 7 要发送的字节 */
#define SB_PRN_BYTE 0x04A4
/** 子功能 5、6 的位置参数低字节 */
#define SB_PRN_POS_LO 0x04B3
/** 子功能 5、6 的位置参数高字节 */
#define SB_PRN_POS_HI 0x04B4
/** 输入法状态区起点：低 4 位是输入法序号 */
#define SB_IME_STATE 0x6900
/** 输入法允许位 */
#define SB_IME_ALLOW 0x6901
/** 输入法标志位：位 1 全角、位 2 暂停、位 4 自定义输入法可选 */
#define SB_IME_FLAGS 0x6902
/** 处理一个键的结果：0 无输出、2 输出汉字内码、$82 按键原样传回 */
#define SB_IME_KEY_RESULT 0x6922
/** 输入得到的字符或汉字，从这里起 */
#define SB_IME_KEY_OUT 0x6923
/** 已输入的字符个数 */
#define SB_IME_INPUT_COUNT 0x6904
/** 输入缓冲，最多 6 个字符 */
#define SB_IME_INPUT_BUFFER 0x6905
/** 显示参数：单元值或显示地址低字节 */
#define SB_PARAM_064C 0x064C
/** 显示参数：单元值或显示地址高字节 */
#define SB_PARAM_064D 0x064D
/** 单元个数或长度的高字节 */
#define SB_PARAM_0650 0x0650
/** 刷新方式或等待帧数 */
#define SB_PARAM_0651 0x0651
/** 调色板号 */
#define SB_PARAM_0654 0x0654
/** 第二个页组：解压到内存的目的页组、文字缓冲的目的页组 */
#define SB_SYS_GROUP_2 0x0643
/** 零页：目的窗口地址低字节 */
#define SB_DEST_ADDR_LO 0x0012
/** 零页：目的窗口地址高字节 */
#define SB_DEST_ADDR_HI 0x0013
/** 字模所在的页，功能 $3B 的结果 */
#define SB_GLYPH_PAGE 0x0644
/** 零页：字模页内偏移低字节，功能 $3B 的结果 */
#define SB_GLYPH_OFFSET_LO 0x000E
/** 零页：字模页内偏移高字节（高 4 位是窗口位），功能 $3B 的结果 */
#define SB_GLYPH_OFFSET_HI 0x000F
/** 显示模式影子：低 3 位是字模形式，位 7 为 1 取 8×16 字模 */
#define SB_DISPLAY_MODE 0x0638
/** 名称表 DMA 与屏幕缓冲服务 */
#define SB_FN_DMA_NAMETABLE 0x30
/** 用一个字节填充显存的一段 */
#define SB_FN_VRAM_FILL 0x32
/** 把扩展 RAM 的一段 DMA 到显存 */
#define SB_FN_VRAM_LOAD 0x33
/** 查字模的页与地址 */
#define SB_FN_GLYPH_ADDR 0x3B
/** 画二平面字符串到显存瓦片（汉字与西文） */
#define SB_FN_PUT_STRING_C 0x3C
/** 画四平面字符串到显存瓦片（汉字与西文） */
#define SB_FN_PUT_STRING_D 0x3D
/** 画二平面字符串到显存瓦片（只处理单字节字符） */
#define SB_FN_PUT_ASCII_C 0x3E
/** 画四平面字符串到显存瓦片（只处理单字节字符） */
#define SB_FN_PUT_ASCII_D 0x3F
/** 把单元写进名称表缓冲并刷新 */
#define SB_FN_CELLS 0x40
/** 在屏幕上显示一行文字（汉字与西文） */
#define SB_FN_TEXT_FIELD 0x45
/** 在屏幕上显示一行文字（只处理单字节字符） */
#define SB_FN_TEXT_FIELD_ASCII 0x46
/** 把字符串画成点阵瓦片写进内存（汉字与西文） */
#define SB_FN_TEXT_BUFFER 0x47
/** 把字符串画成点阵瓦片写进内存（只处理单字节字符） */
#define SB_FN_TEXT_BUFFER_ASCII 0x48
/** 窗口管理：保存与还原屏幕矩形 */
#define SB_FN_WINDOW 0x59
/** 不解压直接把一段图案 DMA 进显存 */
#define SB_FN_IMAGE_RAW 0x5A
/** 解压图案流进显存 */
#define SB_FN_UNPACK_TILES 0x5B
/** 解压映射流并写到屏幕 */
#define SB_FN_UNPACK_MAP 0x5C
/** 写调色板 */
#define SB_FN_PALETTE 0x5D
/** 显示整屏图片 */
#define SB_FN_IMAGE_FULL 0x5E
/** 显示压缩图片 */
#define SB_FN_IMAGE_SHOW 0x5F
/** 用一个单元值按效果铺满矩形 */
#define SB_FN_FILL_RECT 0x60
/** 解压到内存 */
#define SB_FN_UNPACK_MEMORY 0x61

#endif
