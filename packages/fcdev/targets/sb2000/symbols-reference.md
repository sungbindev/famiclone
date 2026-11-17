# SB2000 符号参考

本文由符号定义文件 `sb2000.inc` 生成。
共 433 个符号，32 节：系统调用的功能号、子功能号、系统变量的地址、出错码、硬件寄存器与位、运行库入口。

用法：

- 汇编程序用 `.include "sb2000.inc"` 引入，直接写“汇编名”；文件只含符号，不占空间，值不超过 `$FF` 的符号按零页寻址。
- C 程序 `#include <sb2000.h>` 已包含 `sb2000-symbols.h`，名字是“C 名”：汇编名前加 `SB_`，值是同一个数。
- 汇编程序调用运行库的 C 函数时，函数名前加下划线（如 `_sb_puts`），参数按 cc65 的 fastcall 约定传递；全部函数见 [SB2000 C 接口参考](api-reference.md)。

## 目录

- [系统调用：JSR SYSCALL，功能号写 SYS_FUNCTION](#s01)
- [零页：系统调用与运行库使用的单元](#s02)
- [不经 SYSCALL、直接 JSR 的内核入口](#s03)
- [功能号（写入 SYS_FUNCTION）](#s04)
- [子功能：键盘输入（FN_INPUT）](#s05)
- [子功能：画面与中断控制（FN_PPU_CONTROL、FN_IRQ_MASK）](#s06)
- [子功能：手柄（FN_GAMEPAD）](#s07)
- [子功能：鼠标（FN_MOUSE）](#s08)
- [子功能：文件服务（FN_FILE）](#s09)
- [子功能：句柄服务（FN_FILE_HANDLE）](#s10)
- [子功能：目录服务（FN_DIR）](#s11)
- [子功能：内存页服务（FN_PAGE）](#s12)
- [子功能：日期、时间与运行程序（FN_CLOCK）](#s13)
- [子功能：磁盘服务（FN_DISK）](#s14)
- [子功能：声音服务（FN_SOUND）](#s15)
- [变量：出错码、命令行与显示（扩展 RAM）](#s16)
- [变量：文件与目录（扩展 RAM）](#s17)
- [变量：文件系统内部（扩展 RAM）](#s18)
- [变量：内存页分配（扩展 RAM）](#s19)
- [变量：磁盘参数（扩展 RAM）](#s20)
- [变量：键盘、手柄、鼠标（内核 RAM）](#s21)
- [常量：NMI 服务号、中断源号（传给 FN_PPU_CONTROL、FN_IRQ_MASK 的子功能）](#s22)
- [程序文件头里由 DOS 写入的单元](#s23)
- [变量：显示、中断与窗口的影子（内核 RAM）](#s24)
- [变量：系统调用的参数单元，含义随服务而定（内核 RAM）](#s25)
- [硬件寄存器：图形（真机为 UM6576，行为以真机实测为准，见 docs/sb2000/03-硬件说明.md）](#s26)
- [硬件寄存器：外设与中断](#s27)
- [硬件寄存器：程序窗口、DMA 与显存 bank](#s28)
- [硬件寄存器的位](#s29)
- [文件打开方式（FILE_MODE）](#s30)
- [出错码（DOS_ERROR）](#s31)
- [运行库入口（ROM 页组 0，经 FN_CALL_BANKED 且 SYS_GROUP=0 调用）](#s32)

<a id="s01"></a>
## 系统调用：JSR SYSCALL，功能号写 SYS_FUNCTION

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SYSCALL` | `SB_SYSCALL` | `$FFF0` | 系统调用入口，JSR 到这里 |
| `SYS_FUNCTION` | `SB_SYS_FUNCTION` | `$063E` | 功能号，调用前写入 |
| `SYS_SUBFUNCTION` | `SB_SYS_SUBFUNCTION` | `$063F` | 子功能号，有子功能的服务才用 |
| `SYS_STATUS` | `SB_SYS_STATUS` | `$0640` | 状态，调用开头清 0，子功能越界时为 $FF |
| `SYS_PAGE` | `SB_SYS_PAGE` | `$0641` | 页号：功能 $29 要映射的页，或远指针的页 |
| `SYS_GROUP` | `SB_SYS_GROUP` | `$0642` | 页组的首页号，连续 4 页 |
| `SYS_WINDOW` | `SB_SYS_WINDOW` | `$0653` | 窗口号 0-3，功能 $29 用 |

<a id="s02"></a>
## 零页：系统调用与运行库使用的单元

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `PROG_ENTRY_LO` | `SB_PROG_ENTRY_LO` | `$0C` | 调用地址低字节，功能 $25、$26 用 |
| `PROG_ENTRY_HI` | `SB_PROG_ENTRY_HI` | `$0D` | 调用地址高字节 |
| `FAR_OFFSET_LO` | `SB_FAR_OFFSET_LO` | `$14` | 远指针的页内偏移低字节 |
| `FAR_OFFSET_HI` | `SB_FAR_OFFSET_HI` | `$15` | 远指针的页内偏移高字节 |
| `STR_PTR_LO` | `SB_STR_PTR_LO` | `$4B` | 字符串地址低字节，LIB_PRINT_STRING 用 |
| `STR_PTR_HI` | `SB_STR_PTR_HI` | `$4C` | 字符串地址高字节 |

<a id="s03"></a>
## 不经 SYSCALL、直接 JSR 的内核入口

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `FAR_COPY_IN` | `SB_FAR_COPY_IN` | `$FFEA` | 把页组窗口地址处的数据复制到 $0700 缓冲 |
| `FAR_COPY_OUT` | `SB_FAR_COPY_OUT` | `$FFED` | 把 $0700 缓冲的数据复制到页组窗口地址 |
| `FAR_READ_BYTE` | `SB_FAR_READ_BYTE` | `$FFF3` | 读远指针加 Y 处的一个字节，结果在 A |
| `FAR_WRITE_BYTE` | `SB_FAR_WRITE_BYTE` | `$FFF6` | 把 A 写到远指针加 Y 处 |

<a id="s04"></a>
## 功能号（写入 SYS_FUNCTION）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `FN_VIDEO_OPTION` | `SB_FN_VIDEO_OPTION` | `$10` | 显示选项：把值按位并入显示相关的硬件寄存器 |
| `FN_DISPLAY_MODE` | `SB_FN_DISPLAY_MODE` | `$11` | 显示模式与字库区域登记 |
| `FN_PPU_CONTROL` | `SB_FN_PPU_CONTROL` | `$12` | 画面控制：开关 NMI 及其中的各项服务 |
| `FN_IRQ_MASK` | `SB_FN_IRQ_MASK` | `$13` | 中断屏蔽：允许或屏蔽各中断源 |
| `FN_MOUSE_RESET` | `SB_FN_MOUSE_RESET` | `$14` | 鼠标复位，是否接入放在 $0633 |
| `FN_USER_FLAG` | `SB_FN_USER_FLAG` | `$15` | 把 $0636 置 1，用途未确认 |
| `FN_KEYBOARD_INIT` | `SB_FN_KEYBOARD_INIT` | `$17` | 键盘复位与识别 |
| `FN_SYSTEM_INIT` | `SB_FN_SYSTEM_INIT` | `$18` | 系统硬件与状态初始化 |
| `FN_DISPLAY_OFF` | `SB_FN_DISPLAY_OFF` | `$19` | 关闭 NMI，等到垂直消隐后关闭背景与精灵显示 |
| `FN_DISPLAY_ON` | `SB_FN_DISPLAY_ON` | `$1A` | 等到垂直消隐，开启 NMI 并恢复显示 |
| `FN_WAIT_FRAMES` | `SB_FN_WAIT_FRAMES` | `$1B` | 等待 X 个垂直消隐周期（帧） |
| `FN_SAVE_CONTEXT` | `SB_FN_SAVE_CONTEXT` | `$1C` | 保存上下文，类似 setjmp |
| `FN_RESTORE_CONTEXT` | `SB_FN_RESTORE_CONTEXT` | `$1D` | 恢复上下文并跳转，不返回，类似 longjmp |
| `FN_QUERY_STATE` | `SB_FN_QUERY_STATE` | `$1E` | 查询显示与系统状态，结果放在 $0652 |
| `FN_CLOCK_COPY` | `SB_FN_CLOCK_COPY` | `$1F` | 把系统时钟的时、分、秒复制到 $069A 起 |
| `FN_FAR_READ_NEXT` | `SB_FN_FAR_READ_NEXT` | `$24` | 按远指针读一个字节到 $0653，指针后移 |
| `FN_CALL_BANKED` | `SB_FN_CALL_BANKED` | `$25` | 映射页组并调用 PROG_ENTRY 处的例程 |
| `FN_JUMP_BANKED` | `SB_FN_JUMP_BANKED` | `$26` | 同上，但直接跳转，不再返回 |
| `FN_MAP_GROUP` | `SB_FN_MAP_GROUP` | `$27` | 把页组的 4 页映射到 $8000-$BFFF，调用返回后保持 |
| `FN_PAGE_CONVERT` | `SB_FN_PAGE_CONVERT` | `$28` | 页号换算成 DMA 源地址的高字节 |
| `FN_MAP_WINDOW` | `SB_FN_MAP_WINDOW` | `$29` | 把页 SYS_PAGE 映射到窗口 SYS_WINDOW |
| `FN_VRAM_BANK` | `SB_FN_VRAM_BANK` | `$2A` | 把 SYS_GROUP 写到显存 bank 寄存器 |
| `FN_FAR_WRITE_NEXT` | `SB_FN_FAR_WRITE_NEXT` | `$2B` | 把 $0653 写到远指针处，指针后移 |
| `FN_ADDR_TO_FAR` | `SB_FN_ADDR_TO_FAR` | `$2C` | 把窗口地址换算成远指针 |
| `FN_SOUND` | `SB_FN_SOUND` | `$65` | 声音服务 |
| `FN_INPUT` | `SB_FN_INPUT` | `$6B` | 键盘输入服务 |
| `FN_MOUSE` | `SB_FN_MOUSE` | `$6C` | 鼠标服务 |
| `FN_GAMEPAD` | `SB_FN_GAMEPAD` | `$6A` | 手柄服务 |
| `FN_FILE` | `SB_FN_FILE` | `$7B` | 文件服务：打开、关闭、设置当前目录 |
| `FN_FILE_HANDLE` | `SB_FN_FILE_HANDLE` | `$7C` | 句柄服务：读写、大小、日期、属性 |
| `FN_DIR` | `SB_FN_DIR` | `$7D` | 目录服务：创建、删除、整理名称 |
| `FN_PAGE` | `SB_FN_PAGE` | `$7E` | 内存页服务：分配、释放 |
| `FN_CLOCK` | `SB_FN_CLOCK` | `$7F` | 日期、时间与运行程序 |
| `FN_DISK` | `SB_FN_DISK` | `$80` | 磁盘服务：读写扇区、软驱停转 |

<a id="s05"></a>
## 子功能：键盘输入（FN_INPUT）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_KEY_INIT` | `SB_SUB_KEY_INIT` | `$00` | 复位键盘与鼠标，清空按键队列 |
| `SUB_KEY_ESC_ON` | `SB_SUB_KEY_ESC_ON` | `$01` | 之后按 ESC 会中断程序 |
| `SUB_KEY_ESC_OFF` | `SB_SUB_KEY_ESC_OFF` | `$02` | 按 ESC 不再中断程序 |
| `SUB_KEY_GET_NOWAIT` | `SB_SUB_KEY_GET_NOWAIT` | `$03` | 取一个按键，没有按键时结果为 0 |
| `SUB_KEY_GET_WAIT` | `SB_SUB_KEY_GET_WAIT` | `$04` | 等到有按键再取 |
| `SUB_KEY_FLUSH` | `SB_SUB_KEY_FLUSH` | `$05` | 清空按键队列 |
| `SUB_KEY_COUNT` | `SB_SUB_KEY_COUNT` | `$06` | 队列里的按键数，放在 KEY_ASCII |

<a id="s06"></a>
## 子功能：画面与中断控制（FN_PPU_CONTROL、FN_IRQ_MASK）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_NMI_PAD_ON` | `SB_SUB_NMI_PAD_ON` | `$85` | FN_PPU_CONTROL：打开 NMI 中读手柄 |
| `SUB_NMI_PAD_OFF` | `SB_SUB_NMI_PAD_OFF` | `$05` | FN_PPU_CONTROL：关闭 NMI 中读手柄 |
| `SUB_IRQ_MOUSE_ON` | `SB_SUB_IRQ_MOUSE_ON` | `$81` | FN_IRQ_MASK：允许鼠标中断 |

<a id="s07"></a>
## 子功能：手柄（FN_GAMEPAD）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_PAD_INIT` | `SB_SUB_PAD_INIT` | `$00` | 初始化手柄状态 |
| `SUB_PAD_GET` | `SB_SUB_PAD_GET` | `$01` | 取按键码，不等待，没有键时为 $FF |
| `SUB_PAD_WAIT` | `SB_SUB_PAD_WAIT` | `$02` | 等到有按键再返回 |

<a id="s08"></a>
## 子功能：鼠标（FN_MOUSE）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_MOUSE_SET_LIMITS` | `SB_SUB_MOUSE_SET_LIMITS` | `$00` | 设置坐标范围，上下限在 $0646-$0649 |
| `SUB_MOUSE_GET_POS` | `SB_SUB_MOUSE_GET_POS` | `$02` | 取坐标：X 在 $0646，Y 在 $0647 |
| `SUB_MOUSE_GET_BUTTONS` | `SB_SUB_MOUSE_GET_BUTTONS` | `$03` | 整理按键状态到 MOUSE_BTN_BASE 起 |
| `SUB_MOUSE_SET_POS` | `SB_SUB_MOUSE_SET_POS` | `$04` | 设置坐标：X 取自 $0652，Y 取自 $0653 |

<a id="s09"></a>
## 子功能：文件服务（FN_FILE）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_FS_OPEN` | `SB_SUB_FS_OPEN` | `$09` | 按 FILE_MODE 打开 PATH_BUFFER 里的路径 |
| `SUB_FS_CLOSE` | `SB_SUB_FS_CLOSE` | `$0A` | 关闭 FILE_HANDLE_OFFSET 指定的句柄 |
| `SUB_FS_FIND_ENTRY` | `SB_SUB_FS_FIND_ENTRY` | `$0B` | 设置当前目录，相当于 CD |
| `SUB_FS_RESET` | `SB_SUB_FS_RESET` | `$13` | 把 4 个句柄全部置为空闲 |
| `SUB_FS_NEXT_SECTOR` | `SB_SUB_FS_NEXT_SECTOR` | `$00` | 前进到下一扇区，结果在 FS_NEXT_LBA，区域结束时 FS_REGION_END 为 1 |
| `SUB_FS_FAT_LOAD` | `SB_SUB_FS_FAT_LOAD` | `$01` | 读 FS_LBA 号扇区到 DISK_BUFFER，必要时先把 FAT 装入缓存 |
| `SUB_FS_CLUSTER_LBA` | `SB_SUB_FS_CLUSTER_LBA` | `$03` | 把 FS_CLUSTER 的简号换算成该簇第一个扇区 FS_LBA |
| `SUB_FS_FREE_SPACE` | `SB_SUB_FS_FREE_SPACE` | `$06` | 从 FS_FREE_START 起统计空闲空间，字节数累加到 ARG_069A |

<a id="s10"></a>
## 子功能：句柄服务（FN_FILE_HANDLE）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_FH_CLOSE_FLUSH` | `SB_SUB_FH_CLOSE_FLUSH` | `$02` | 关闭句柄，并让软驱停转 |
| `SUB_FH_READ_AT` | `SB_SUB_FH_READ_AT` | `$05` | 按偏移与长度读到扩展 RAM 页 |
| `SUB_FH_WRITE_AT` | `SB_SUB_FH_WRITE_AT` | `$06` | 按偏移与长度从扩展 RAM 页写入 |
| `SUB_FH_GET_SIZE` | `SB_SUB_FH_GET_SIZE` | `$0D` | 取文件大小到 FILE_SIZE |
| `SUB_FH_READ_FILE` | `SB_SUB_FH_READ_FILE` | `$0F` | 把整个文件读入扩展 RAM 页 |
| `SUB_FH_WRITE_FILE` | `SB_SUB_FH_WRITE_FILE` | `$10` | 把扩展 RAM 页的内容整个写入文件 |
| `SUB_FH_DELETE` | `SB_SUB_FH_DELETE` | `$03` | 删除 PATH_BUFFER 里的文件 |
| `SUB_FH_RENAME` | `SB_SUB_FH_RENAME` | `$04` | 把句柄的文件改名为 PATH_BUFFER 里的新名称 |
| `SUB_FH_SET_DATE` | `SB_SUB_FH_SET_DATE` | `$07` | 设置句柄的日期（FILE_DATE） |
| `SUB_FH_GET_DATE` | `SB_SUB_FH_GET_DATE` | `$08` | 取得句柄的日期到 FILE_DATE |
| `SUB_FH_SET_TIME` | `SB_SUB_FH_SET_TIME` | `$09` | 设置句柄的时间（FILE_TIME） |
| `SUB_FH_GET_TIME` | `SB_SUB_FH_GET_TIME` | `$0A` | 取得句柄的时间到 FILE_TIME |
| `SUB_FH_SET_ATTR` | `SB_SUB_FH_SET_ATTR` | `$0B` | 设置句柄的属性（FILE_ATTR） |
| `SUB_FH_GET_ATTR` | `SB_SUB_FH_GET_ATTR` | `$0C` | 取得句柄的属性到 FILE_ATTR |

<a id="s11"></a>
## 子功能：目录服务（FN_DIR）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_DIR_FORMAT_NAME` | `SB_SUB_DIR_FORMAT_NAME` | `$01` | 把当前目录名整理成 \NAME.EXT 放入 PATH_BUFFER |
| `SUB_DIR_MAKE` | `SB_SUB_DIR_MAKE` | `$02` | 创建目录 |
| `SUB_DIR_DELETE` | `SB_SUB_DIR_DELETE` | `$03` | 删除目录，目录必须为空 |

<a id="s12"></a>
## 子功能：内存页服务（FN_PAGE）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_PAGE_ALLOC` | `SB_SUB_PAGE_ALLOC` | `$00` | 按 ALLOC_COUNT 分配连续页，起始页号在 ALLOC_START |
| `SUB_PAGE_FREE` | `SB_SUB_PAGE_FREE` | `$01` | 释放 ALLOC_START 起的 ALLOC_COUNT 个页 |
| `SUB_PAGE_LARGEST` | `SB_SUB_PAGE_LARGEST` | `$02` | 求最长连续空闲页数，放在 ALLOC_COUNT |

<a id="s13"></a>
## 子功能：日期、时间与运行程序（FN_CLOCK）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_CLOCK_EXEC` | `SB_SUB_CLOCK_EXEC` | `$00` | 运行 PATH_BUFFER 里的程序 |
| `SUB_CLOCK_SET_DATE` | `SB_SUB_CLOCK_SET_DATE` | `$01` | 用 FILE_DATE 设置系统日期 |
| `SUB_CLOCK_GET_DATE` | `SB_SUB_CLOCK_GET_DATE` | `$02` | 取系统日期到 FILE_DATE |
| `SUB_CLOCK_SET_TIME` | `SB_SUB_CLOCK_SET_TIME` | `$03` | 用 FILE_TIME 设置系统时间 |
| `SUB_CLOCK_GET_TIME` | `SB_SUB_CLOCK_GET_TIME` | `$04` | 取系统时间到 FILE_TIME |

<a id="s14"></a>
## 子功能：磁盘服务（FN_DISK）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_DISK_READ` | `SB_SUB_DISK_READ` | `$04` | 按 DISK_ 参数连续读扇区 |
| `SUB_DISK_WRITE` | `SB_SUB_DISK_WRITE` | `$05` | 按 DISK_ 参数连续写扇区 |
| `SUB_DISK_STOP` | `SB_SUB_DISK_STOP` | `$07` | 软驱停转，每次文件操作之后都要调用 |
| `SUB_DISK_INIT` | `SB_SUB_DISK_INIT` | `$00` | 启动软驱、复位到 0 磁道，读出状态后停转 |
| `SUB_DISK_FORMAT` | `SB_SUB_DISK_FORMAT` | `$01` | 格式化一个磁道 |
| `SUB_DISK_READ_ONE` | `SB_SUB_DISK_READ_ONE` | `$02` | 读一个扇区到 DISK_BUFFER |
| `SUB_DISK_WRITE_ONE` | `SB_SUB_DISK_WRITE_ONE` | `$03` | 把 DISK_BUFFER 写到一个扇区 |
| `SUB_DISK_START` | `SB_SUB_DISK_START` | `$06` | 启动软驱 |
| `SUB_DISK_SCAN` | `SB_SUB_DISK_SCAN` | `$08` | 逐个读一条磁道的 18 个扇区，读不出的在位图里置位 |

<a id="s15"></a>
## 子功能：声音服务（FN_SOUND）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `SUB_SOUND_PLAY_WAV` | `SB_SUB_SOUND_PLAY_WAV` | `$0C` | 播放内存里的 WAV，播完才返回 |

<a id="s16"></a>
## 变量：出错码、命令行与显示（扩展 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `DOS_ERROR` | `SB_DOS_ERROR` | `$6300` | 出错码，0 为成功，每次调用开头清 0 |
| `LIB_GROUP` | `SB_LIB_GROUP` | `$6308` | 运行库显示字符串所在的页组，高 6 位有效 |
| `CURSOR_COL` | `SB_CURSOR_COL` | `$630C` | 光标列，从 1 起，每行 32 列 |
| `CURSOR_ROW` | `SB_CURSOR_ROW` | `$630D` | 光标所在的显示行 |
| `GLYPH_ROW` | `SB_GLYPH_ROW` | `$630E` | 字形缓冲行，由运行库维护，程序不要改 |
| `CMD_TAIL` | `SB_CMD_TAIL` | `$6364` | 命令行参数的起点，大写、0 结尾，首字节是空格 |
| `FONT_SIZE` | `SB_FONT_SIZE` | `$6428` | 字号 |

<a id="s17"></a>
## 变量：文件与目录（扩展 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `DIR_NAME83` | `SB_DIR_NAME83` | `$651D` | 当前目录的 11 字节名称，不要随便改写 |
| `PATH_BUFFER` | `SB_PATH_BUFFER` | `$6543` | 路径，0 结尾的 ASCII，可带 A: 与 \ |
| `FILE_MODE` | `SB_FILE_MODE` | `$6560` | 打开方式，0-5 |
| `FILE_HANDLE_OFFSET` | `SB_FILE_HANDLE_OFFSET` | `$6561` | 当前句柄在句柄表里的偏移 |
| `FILE_OFFSET_0` | `SB_FILE_OFFSET_0` | `$6565` | 文件内偏移，3 字节，低字节在前 |
| `FILE_OFFSET_1` | `SB_FILE_OFFSET_1` | `$6566` | 文件内偏移第 2 字节 |
| `FILE_OFFSET_2` | `SB_FILE_OFFSET_2` | `$6567` | 文件内偏移第 3 字节 |
| `FILE_SIZE_0` | `SB_FILE_SIZE_0` | `$6568` | 文件大小或读写长度，3 字节，低字节在前 |
| `FILE_SIZE_1` | `SB_FILE_SIZE_1` | `$6569` | 文件大小或读写长度第 2 字节 |
| `FILE_SIZE_2` | `SB_FILE_SIZE_2` | `$656A` | 文件大小或读写长度第 3 字节 |
| `FILE_START_PAGE` | `SB_FILE_START_PAGE` | `$656B` | 读写整个文件或一段数据时的扩展 RAM 起始页号 |
| `FILE_DATE_LO` | `SB_FILE_DATE_LO` | `$656E` | 日期低字节，FAT 格式：年（自 1980）、月、日 |
| `FILE_DATE_HI` | `SB_FILE_DATE_HI` | `$656F` | 日期高字节 |
| `FILE_TIME_LO` | `SB_FILE_TIME_LO` | `$6570` | 时间低字节，FAT 格式：时、分、秒的一半 |
| `FILE_TIME_HI` | `SB_FILE_TIME_HI` | `$6571` | 时间高字节 |
| `FILE_ATTR` | `SB_FILE_ATTR` | `$6572` | 属性：位 0 只读，位 3 卷标，位 4 目录，位 5 存档 |

<a id="s18"></a>
## 变量：文件系统内部（扩展 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `DISK_BUFFER` | `SB_DISK_BUFFER` | `$6000` | 扇区缓冲，512 字节 |
| `FS_CURRENT_DIR_LBA` | `SB_FS_CURRENT_DIR_LBA` | `$6528` | 当前目录的第一个扇区号，16 位，根目录为 19 |
| `FS_LBA` | `SB_FS_LBA` | `$652E` | 要读写的扇区号，16 位，低字节在前 |
| `FS_FREE_START` | `SB_FS_FREE_START` | `$6530` | 统计空闲空间的起始簇号，16 位 |
| `FS_CLUSTER` | `SB_FS_CLUSTER` | `$6536` | 簇号，16 位 |
| `FILE_HANDLES` | `SB_FILE_HANDLES` | `$643B` | 句柄表起点，共 4 项，每项 $2C 字节 |
| `FH_CLUSTER_OFFSET` | `SB_FH_CLUSTER_OFFSET` | `$15` | 句柄表项里起始簇号的偏移，16 位 |
| `FH_SIZE_OFFSET` | `SB_FH_SIZE_OFFSET` | `$0E` | 句柄表项里文件大小的偏移，3 字节，低字节在前 |
| `FS_NEXT_LBA` | `SB_FS_NEXT_LBA` | `$65A9` | 前进到下一扇区的结果，16 位 |
| `FS_REGION_END` | `SB_FS_REGION_END` | `$65C5` | 下一扇区越出区域或簇链到尾时为 1 |
| `ARG_069A` | `SB_ARG_069A` | `$069A` | 数学与空闲空间统计的参数区起点 |

<a id="s19"></a>
## 变量：内存页分配（扩展 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `ALLOC_COUNT` | `SB_ALLOC_COUNT` | `$65A6` | 要分配或释放的页数 |
| `ALLOC_START` | `SB_ALLOC_START` | `$65A7` | 分配得到的起始页号，或要释放的起始页号 |

<a id="s20"></a>
## 变量：磁盘参数（扩展 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `DISK_HEAD` | `SB_DISK_HEAD` | `$6200` | 面，0 或 1 |
| `DISK_SECTOR` | `SB_DISK_SECTOR` | `$6201` | 起始扇区，1-18 |
| `DISK_TRACK` | `SB_DISK_TRACK` | `$6202` | 磁道，0-79 |
| `DISK_PAGE` | `SB_DISK_PAGE` | `$6203` | 缓冲所在的扩展 RAM 页号 |
| `DISK_PAGE_SECTOR` | `SB_DISK_PAGE_SECTOR` | `$6204` | 缓冲从页内第几个 512 字节起，0-7 |
| `DISK_COUNT_LO` | `SB_DISK_COUNT_LO` | `$6205` | 扇区数低字节 |
| `DISK_COUNT_HI` | `SB_DISK_COUNT_HI` | `$6206` | 扇区数高字节 |
| `DISK_MODE` | `SB_DISK_MODE` | `$6207` | 方式：$80 出错时立即返回 |
| `DISK_STATUS` | `SB_DISK_STATUS` | `$6208` | 结果状态，0 为成功 |
| `DISK_BAD_MAP` | `SB_DISK_BAD_MAP` | `$6237` | 扫描磁道的坏扇区位图，3 字节，位 7 起对应扇区 1-8、9-16、17-18 |

<a id="s21"></a>
## 变量：键盘、手柄、鼠标（内核 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `KEY_ASCII` | `SB_KEY_ASCII` | `$034B` | 取到的按键的 ASCII 码，没有字符的键为 0 |
| `KEY_SCAN` | `SB_KEY_SCAN` | `$034C` | 取到的按键的扫描码 |
| `PAD_KEY` | `SB_PAD_KEY` | `$034D` | 手柄按键码，$FF 为没有键 |
| `PAD1_BITS` | `SB_PAD1_BITS` | `$034E` | 1 号手柄的原始状态，位 7 是 A，位 0 是右 |
| `PAD2_BITS` | `SB_PAD2_BITS` | `$034F` | 2 号手柄的原始状态 |
| `MOUSE_X` | `SB_MOUSE_X` | `$0354` | 鼠标光标的 X 坐标 |
| `MOUSE_Y` | `SB_MOUSE_Y` | `$0355` | 鼠标光标的 Y 坐标 |
| `MOUSE_PARAM` | `SB_MOUSE_PARAM` | `$035E` | 鼠标双击的间隔参数，默认 $0A |
| `KBD_FLAGS` | `SB_KBD_FLAGS` | `$0344` | 键盘修饰键状态：位 0 右 Shift，1 左 Shift，2 Ctrl，3 Alt，4 ScrollLock，5 NumLock，6 CapsLock，7 Insert |
| `KBD_LEDS` | `SB_KBD_LEDS` | `$0348` | 键盘指示灯状态：位 0 ScrollLock，1 NumLock，2 CapsLock |
| `KBD_SELF_TEST_RESULT` | `SB_KBD_SELF_TEST_RESULT` | `$0635` | 键盘复位与识别（FN_KEYBOARD_INIT）的结果，1 为通过 |
| `MOUSE_BTN_BASE` | `SB_MOUSE_BTN_BASE` | `$0360` | 鼠标按键状态的起点，共 8 个字节 |
| `MOUSE_PRESENT` | `SB_MOUSE_PRESENT` | `$0633` | 鼠标复位后非 0 表示检测到鼠标，0 为没有（BIOS 里它是鼠标数据包的字节序号） |

<a id="s22"></a>
## 常量：NMI 服务号、中断源号（传给 FN_PPU_CONTROL、FN_IRQ_MASK 的子功能）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `NMI_SERVICE_BAR_ANIM` | `SB_NMI_SERVICE_BAR_ANIM` | `$01` | NMI 服务：条带动画（卡拉 OK 歌词滚动） |
| `NMI_SERVICE_SOUND` | `SB_NMI_SERVICE_SOUND` | `$02` | NMI 服务：声音引擎每帧更新 |
| `NMI_SERVICE_HOOK5` | `SB_NMI_SERVICE_HOOK5` | `$03` | NMI 服务：调用登记的钩子 5 |
| `NMI_SERVICE_HOOK4` | `SB_NMI_SERVICE_HOOK4` | `$04` | NMI 服务：调用登记的钩子 4 |
| `NMI_SERVICE_PAD` | `SB_NMI_SERVICE_PAD` | `$05` | NMI 服务：读手柄 |
| `NMI_SERVICE_PALETTE` | `SB_NMI_SERVICE_PALETTE` | `$06` | NMI 服务：把 $0700 缓冲上传到调色板 |
| `IRQ_SOURCE_TIMER` | `SB_IRQ_SOURCE_TIMER` | `$00` | 中断源：定时器 |
| `IRQ_SOURCE_MOUSE` | `SB_IRQ_SOURCE_MOUSE` | `$01` | 中断源：鼠标 |
| `IRQ_SOURCE_KEYBOARD` | `SB_IRQ_SOURCE_KEYBOARD` | `$02` | 中断源：键盘 |
| `IRQ_SOURCE_IRQ4` | `SB_IRQ_SOURCE_IRQ4` | `$03` | 中断源：中断 4（只有钩子） |
| `IRQ_SOURCE_FLOPPY` | `SB_IRQ_SOURCE_FLOPPY` | `$04` | 中断源：软驱 |
| `IRQ_SOURCE_IRQ2` | `SB_IRQ_SOURCE_IRQ2` | `$05` | 中断源：中断 2（只有钩子） |
| `IRQ_SOURCE_IRQ1` | `SB_IRQ_SOURCE_IRQ1` | `$06` | 中断源：中断 1（只有钩子） |

<a id="s23"></a>
## 程序文件头里由 DOS 写入的单元

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `PROGRAM_START_PAGE` | `SB_PROGRAM_START_PAGE` | `$8005` | 程序装入的起始扩展 RAM 页号，也就是程序所在页组的首页 |
| `PROGRAM_PAGE_COUNT` | `SB_PROGRAM_PAGE_COUNT` | `$8006` | 程序占用的页数 |

<a id="s24"></a>
## 变量：显示、中断与窗口的影子（内核 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `PPU_CTRL_SHADOW` | `SB_PPU_CTRL_SHADOW` | `$062A` | PPU 控制寄存器（$2000）的影子，位 7 是 NMI 开关 |
| `PPU_MASK_SHADOW` | `SB_PPU_MASK_SHADOW` | `$062B` | PPU 显示寄存器（$2001）的影子，位 3 背景，位 4 精灵 |
| `PPU_EXT_CTRL_SHADOW` | `SB_PPU_EXT_CTRL_SHADOW` | `$062E` | 扩展控制寄存器（$2008）的影子 |
| `IRQ_MASK_SHADOW` | `SB_IRQ_MASK_SHADOW` | `$062F` | 中断屏蔽（$4032）的影子，位为 1 表示屏蔽 |
| `NMI_FLAGS` | `SB_NMI_FLAGS` | `$0630` | NMI 里各项服务的开关 |
| `GLYPH_MODE` | `SB_GLYPH_MODE` | `$0638` | 字形模式，高 2 位是显示模式 |
| `WINDOW_SAVE_0` | `SB_WINDOW_SAVE_0` | `$0639` | 窗口 0（$8000）页号的影子 |
| `WINDOW_SAVE_1` | `SB_WINDOW_SAVE_1` | `$063A` | 窗口 1（$9000）页号的影子 |
| `WINDOW_SAVE_2` | `SB_WINDOW_SAVE_2` | `$063B` | 窗口 2（$A000）页号的影子 |
| `WINDOW_SAVE_3` | `SB_WINDOW_SAVE_3` | `$063C` | 窗口 3（$B000）页号的影子 |
| `COPY_BUFFER` | `SB_COPY_BUFFER` | `$0700` | 跨页复制用的中转缓冲 |
| `NMI_HOOK5_VEC_LO` | `SB_NMI_HOOK5_VEC_LO` | `$1E` | NMI 钩子 5 的地址（低），地址在零页，钩子由 NMI_FLAGS 位 5 打开 |
| `NMI_HOOK4_VEC_LO` | `SB_NMI_HOOK4_VEC_LO` | `$20` | NMI 钩子 4 的地址（低），钩子由 NMI_FLAGS 位 4 打开 |
| `TIMER_HOOK_VEC_LO` | `SB_TIMER_HOOK_VEC_LO` | `$24` | 定时器钩子的地址（低） |
| `IRQ3_HOOK_VEC_LO` | `SB_IRQ3_HOOK_VEC_LO` | `$26` | 中断 3 钩子的地址（低），系统不会调用它 |
| `IRQ1_HOOK_VEC_LO` | `SB_IRQ1_HOOK_VEC_LO` | `$28` | 中断 1 钩子的地址（低） |
| `IRQ2_HOOK_VEC_LO` | `SB_IRQ2_HOOK_VEC_LO` | `$2A` | 中断 2 钩子的地址（低） |
| `IRQ4_HOOK_VEC_LO` | `SB_IRQ4_HOOK_VEC_LO` | `$2C` | 中断 4 钩子的地址（低） |
| `KBD_HOOK_VEC_LO` | `SB_KBD_HOOK_VEC_LO` | `$2E` | 键盘钩子的地址（低） |
| `MOUSE_HOOK_VEC_LO` | `SB_MOUSE_HOOK_VEC_LO` | `$30` | 鼠标钩子的地址（低） |
| `NMI_HOOK5_PAGE` | `SB_NMI_HOOK5_PAGE` | `$0306` | NMI 钩子 5 所在页组的首页号 |
| `NMI_HOOK4_PAGE` | `SB_NMI_HOOK4_PAGE` | `$0307` | NMI 钩子 4 所在页组的首页号 |
| `KBD_HOOK_PAGE` | `SB_KBD_HOOK_PAGE` | `$068E` | 键盘钩子所在页组的首页号 |
| `MOUSE_HOOK_PAGE` | `SB_MOUSE_HOOK_PAGE` | `$068F` | 鼠标钩子所在页组的首页号 |
| `TIMER_HOOK_PAGE` | `SB_TIMER_HOOK_PAGE` | `$0690` | 定时器钩子所在页组的首页号 |
| `IRQ3_HOOK_PAGE` | `SB_IRQ3_HOOK_PAGE` | `$0691` | 中断 3 钩子所在页组的首页号 |
| `IRQ1_HOOK_PAGE` | `SB_IRQ1_HOOK_PAGE` | `$0692` | 中断 1 钩子所在页组的首页号 |
| `IRQ2_HOOK_PAGE` | `SB_IRQ2_HOOK_PAGE` | `$0693` | 中断 2 钩子所在页组的首页号 |
| `IRQ4_HOOK_PAGE` | `SB_IRQ4_HOOK_PAGE` | `$0694` | 中断 4 钩子所在页组的首页号 |
| `NMI_COUNTER_UP` | `SB_NMI_COUNTER_UP` | `$0304` | 每次 NMI 加 1 的计数 |
| `MOUSE_ABSENT` | `SB_MOUSE_ABSENT` | `$0634` | 鼠标不可用标志：允许鼠标中断后，在线清 0，否则 $FF |
| `ESC_BREAK` | `SB_ESC_BREAK` | `$063D` | 按 ESC 中断开关：非 0 时按 ESC 会恢复上下文栈顶的恢复点 |
| `CONTEXT_COUNT` | `SB_CONTEXT_COUNT` | `$0655` | 上下文栈里的恢复点个数减 1，空栈为 $FF |
| `CLOCK_SECONDS` | `SB_CLOCK_SECONDS` | `$068A` | 软件时钟：秒 |
| `CLOCK_MINUTES` | `SB_CLOCK_MINUTES` | `$068B` | 软件时钟：分 |
| `CLOCK_HOURS` | `SB_CLOCK_HOURS` | `$068C` | 软件时钟：时 |
| `CLOCK_MIDNIGHT_FLAG` | `SB_CLOCK_MIDNIGHT_FLAG` | `$068D` | 软件时钟过午夜标志：设置日期时清 0，非 0 时取日期要先加 1 天 |

<a id="s25"></a>
## 变量：系统调用的参数单元，含义随服务而定（内核 RAM）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `PARAM_0646` | `SB_PARAM_0646` | `$0646` | 鼠标 X 下限，或显存地址高字节 |
| `PARAM_0647` | `SB_PARAM_0647` | `$0647` | 鼠标 Y 下限，或显存地址低字节 |
| `PARAM_0648` | `SB_PARAM_0648` | `$0648` | 鼠标 X 上限，或屏幕缓冲的列 |
| `PARAM_0649` | `SB_PARAM_0649` | `$0649` | 鼠标 Y 上限，或屏幕缓冲的行 |
| `PARAM_064F` | `SB_PARAM_064F` | `$064F` | 个数、长度或字节数，含义随服务而定 |
| `PARAM_0652` | `SB_PARAM_0652` | `$0652` | 通用参数，多数服务从这里取输入 |
| `PARAM_0653` | `SB_PARAM_0653` | `$0653` | 通用参数或返回值，如读到的字节 |

<a id="s26"></a>
## 硬件寄存器：图形（真机为 UM6576，行为以真机实测为准，见 docs/sb2000/03-硬件说明.md）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `HW_PPU_CTRL` | `SB_HW_PPU_CTRL` | `$2000` | 控制：位 7 允许 NMI，位 5 精灵 8x16，位 2 地址增量，位 1-0 背景页 |
| `HW_PPU_MASK` | `SB_HW_PPU_MASK` | `$2001` | 显示：灰度、最左 8 像素、背景与精灵显示、颜色增强 |
| `HW_PPU_STATUS` | `SB_HW_PPU_STATUS` | `$2002` | 状态（读）：位 7 垂直消隐，位 6 精灵 0 碰撞，位 5 一行超过 8 个精灵 |
| `HW_OAM_ADDRESS` | `SB_HW_OAM_ADDRESS` | `$2003` | 精灵 RAM 地址 |
| `HW_OAM_DATA` | `SB_HW_OAM_DATA` | `$2004` | 精灵 RAM 数据 |
| `HW_PPU_SCROLL` | `SB_HW_PPU_SCROLL` | `$2005` | 滚动，连续写两次（X、Y） |
| `HW_PPU_ADDRESS` | `SB_HW_PPU_ADDRESS` | `$2006` | 显存地址，连续写两次（高、低） |
| `HW_PPU_DATA` | `SB_HW_PPU_DATA` | `$2007` | 显存数据，读要先读一次刷新缓冲 |
| `HW_PPU_EXT` | `SB_HW_PPU_EXT` | `$2008` | 扩展控制：位 7 背景 16 色，位 3-2 精灵图案表，位 0 名称表起始 |
| `HW_PALETTE` | `SB_HW_PALETTE` | `$2040` | 调色板，64 项（$2040-$207F） |
| `HW_OAM_DMA` | `SB_HW_OAM_DMA` | `$4014` | 精灵 DMA：写页号，把 $xx00 起的 256 字节送进精灵 RAM |

<a id="s27"></a>
## 硬件寄存器：外设与中断

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `HW_SOUND_ENABLE` | `SB_HW_SOUND_ENABLE` | `$4015` | 声音通道使能 |
| `HW_PAD_1` | `SB_HW_PAD_1` | `$4016` | 手柄 1 |
| `HW_PAD_2` | `SB_HW_PAD_2` | `$4017` | 手柄 2 |
| `HW_KEY_DATA` | `SB_HW_KEY_DATA` | `$4020` | 读：键盘扫描码；写：位 7 为 1 时 DMA 控制的位 6、5 才起作用 |
| `HW_KEY_SEND` | `SB_HW_KEY_SEND` | `$4021` | 发给键盘的数据 |
| `HW_KEY_CONTROL` | `SB_HW_KEY_CONTROL` | `$4022` | 键盘控制：位 7 键盘时钟，位 2 允许发送完成中断，位 0 方向 |
| `HW_MOUSE_SELECT` | `SB_HW_MOUSE_SELECT` | `$4023` | 位 0 选手柄 2 或鼠标 |
| `HW_MOUSE_DATA` | `SB_HW_MOUSE_DATA` | `$4024` | 鼠标数据 |
| `HW_PRINTER_DATA` | `SB_HW_PRINTER_DATA` | `$4025` | 打印机数据 |
| `HW_EXT_IO` | `SB_HW_EXT_IO` | `$4026` | 扩展 I/O |
| `HW_DAC` | `SB_HW_DAC` | `$4027` | 8 位 DAC |
| `HW_INIT` | `SB_HW_INIT` | `$4031` | 初始设置：依次写 $65、$76 进入 6576 模式 |
| `HW_IRQ_MASK` | `SB_HW_IRQ_MASK` | `$4032` | 中断屏蔽：写 1 的位被屏蔽，同时清除 HW_IRQ_PENDING 里对应的位 |
| `HW_IRQ_PENDING` | `SB_HW_IRQ_PENDING` | `$4033` | 待处理的中断（只读） |
| `HW_TIMER_CONTROL` | `SB_HW_TIMER_CONTROL` | `$4034` | 定时器控制：位 7 允许，位 6 连续重载，位 5 行消隐时钟，位 3-0 预分频 |
| `HW_TIMER_PRESET` | `SB_HW_TIMER_PRESET` | `$4035` | 定时器预置值，写入时同时装入当前值 |
| `HW_TIMER_CURRENT` | `SB_HW_TIMER_CURRENT` | `$4036` | 定时器当前值（递减） |
| `HW_RTC_ADDRESS` | `SB_HW_RTC_ADDRESS` | `$4080` | 实时时钟地址 |
| `HW_RTC_DATA` | `SB_HW_RTC_DATA` | `$4081` | 实时时钟数据 |

<a id="s28"></a>
## 硬件寄存器：程序窗口、DMA 与显存 bank

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `HW_WINDOW_0` | `SB_HW_WINDOW_0` | `$4040` | 窗口 0（$8000）的页号，4K 一页 |
| `HW_WINDOW_1` | `SB_HW_WINDOW_1` | `$4041` | 窗口 1（$9000）的页号 |
| `HW_WINDOW_2` | `SB_HW_WINDOW_2` | `$4042` | 窗口 2（$A000）的页号 |
| `HW_WINDOW_3` | `SB_HW_WINDOW_3` | `$4043` | 窗口 3（$B000）的页号 |
| `HW_WINDOW_4` | `SB_HW_WINDOW_4` | `$4044` | 窗口 4（$C000）的页号 |
| `HW_WINDOW_5` | `SB_HW_WINDOW_5` | `$4045` | 窗口 5（$D000）的页号 |
| `HW_WINDOW_6` | `SB_HW_WINDOW_6` | `$4046` | 窗口 6（$E000）的页号 |
| `HW_WINDOW_7` | `SB_HW_WINDOW_7` | `$4047` | 窗口 7（$F000）的页号 |
| `HW_DMA_MODE` | `SB_HW_DMA_MODE` | `$4048` | DMA 模式：位 7 启动，位 6 高速，位 5 目的是 CPU 地址 |
| `HW_DMA_SOURCE_BANK` | `SB_HW_DMA_SOURCE_BANK` | `$4049` | DMA 源 bank：位 4 为 1 取扩展 RAM，低 4 位是源地址的第 18-15 位 |
| `HW_DMA_SOURCE_LOW` | `SB_HW_DMA_SOURCE_LOW` | `$404A` | DMA 源地址的第 7-0 位 |
| `HW_DMA_SOURCE_HIGH` | `SB_HW_DMA_SOURCE_HIGH` | `$404B` | DMA 源地址：位 6-0 是第 14-8 位，位 7 要为 1 |
| `HW_DMA_DEST_LOW` | `SB_HW_DMA_DEST_LOW` | `$404C` | DMA 目的地址低字节 |
| `HW_DMA_DEST_HIGH` | `SB_HW_DMA_DEST_HIGH` | `$404D` | DMA 目的地址高字节，目的是显存时位 7 为 1 表示扩展显存 |
| `HW_DMA_LENGTH_LOW` | `SB_HW_DMA_LENGTH_LOW` | `$404E` | DMA 长度低字节，写“字节数 - 1” |
| `HW_DMA_LENGTH_HIGH` | `SB_HW_DMA_LENGTH_HIGH` | `$404F` | DMA 长度高字节 |
| `HW_VRAM_BANK` | `SB_HW_VRAM_BANK` | `$4300` | 扩展显存的 32K bank |
| `HW_RESET_6576` | `SB_HW_RESET_6576` | `$4301` | 写 $55 回到 FC 兼容模式 |
| `HW_VOICE_PORT` | `SB_HW_VOICE_PORT` | `$4302` | 语音端口 |

<a id="s29"></a>
## 硬件寄存器的位

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `HW_CTRL_NMI` | `SB_HW_CTRL_NMI` | `$80` | HW_PPU_CTRL：允许 NMI |
| `HW_CTRL_SPRITE_8X16` | `SB_HW_CTRL_SPRITE_8X16` | `$20` | HW_PPU_CTRL：精灵 8x16 |
| `HW_CTRL_STEP_32` | `SB_HW_CTRL_STEP_32` | `$04` | HW_PPU_CTRL：显存地址每次加 32 |
| `HW_MASK_GRAY` | `SB_HW_MASK_GRAY` | `$01` | HW_PPU_MASK：灰度 |
| `HW_MASK_BG_LEFT` | `SB_HW_MASK_BG_LEFT` | `$02` | HW_PPU_MASK：显示背景最左 8 像素 |
| `HW_MASK_SPRITE_LEFT` | `SB_HW_MASK_SPRITE_LEFT` | `$04` | HW_PPU_MASK：显示精灵最左 8 像素 |
| `HW_MASK_BG` | `SB_HW_MASK_BG` | `$08` | HW_PPU_MASK：显示背景 |
| `HW_MASK_SPRITE` | `SB_HW_MASK_SPRITE` | `$10` | HW_PPU_MASK：显示精灵 |
| `HW_MASK_TINT_GREEN` | `SB_HW_MASK_TINT_GREEN` | `$20` | HW_PPU_MASK：增强绿（PAL 顺序） |
| `HW_MASK_TINT_RED` | `SB_HW_MASK_TINT_RED` | `$40` | HW_PPU_MASK：增强红 |
| `HW_MASK_TINT_BLUE` | `SB_HW_MASK_TINT_BLUE` | `$80` | HW_PPU_MASK：增强蓝 |
| `HW_STATUS_VBLANK` | `SB_HW_STATUS_VBLANK` | `$80` | HW_PPU_STATUS：垂直消隐 |
| `HW_STATUS_SPRITE0` | `SB_HW_STATUS_SPRITE0` | `$40` | HW_PPU_STATUS：精灵 0 碰撞 |
| `HW_STATUS_OVERFLOW` | `SB_HW_STATUS_OVERFLOW` | `$20` | HW_PPU_STATUS：一行超过 8 个精灵 |
| `HW_EXT_16_COLOR` | `SB_HW_EXT_16_COLOR` | `$80` | HW_PPU_EXT：背景 16 色 |
| `HW_EXT_NAMES_2000` | `SB_HW_EXT_NAMES_2000` | `$01` | HW_PPU_EXT：名称表从显存 $2000 起 |
| `HW_KEY_DMA_ENABLE` | `SB_HW_KEY_DMA_ENABLE` | `$80` | HW_KEY_DATA（写）：使能 DMA 模式的位 6、5 |
| `HW_DMA_START` | `SB_HW_DMA_START` | `$80` | HW_DMA_MODE：启动 |
| `HW_DMA_FAST` | `SB_HW_DMA_FAST` | `$40` | HW_DMA_MODE：高速（1.78MB/s） |
| `HW_DMA_TO_CPU` | `SB_HW_DMA_TO_CPU` | `$20` | HW_DMA_MODE：目的是 CPU 地址（工作 RAM） |
| `HW_DMA_SOURCE_RAM` | `SB_HW_DMA_SOURCE_RAM` | `$10` | HW_DMA_SOURCE_BANK：源是扩展 RAM |
| `HW_DMA_SOURCE_A15` | `SB_HW_DMA_SOURCE_A15` | `$80` | HW_DMA_SOURCE_HIGH：源地址的第 15 位，真机要求为 1 |
| `HW_TIMER_ENABLE` | `SB_HW_TIMER_ENABLE` | `$80` | HW_TIMER_CONTROL：允许 |
| `HW_TIMER_RELOAD` | `SB_HW_TIMER_RELOAD` | `$40` | HW_TIMER_CONTROL：连续重载 |
| `HW_TIMER_LINE_CLOCK` | `SB_HW_TIMER_LINE_CLOCK` | `$20` | HW_TIMER_CONTROL：基准时钟用行消隐时钟 |
| `HW_IRQ_TIMER` | `SB_HW_IRQ_TIMER` | `$80` | HW_IRQ_MASK、HW_IRQ_PENDING：定时器 |
| `HW_IRQ_MOUSE` | `SB_HW_IRQ_MOUSE` | `$40` | 鼠标 |
| `HW_IRQ_KEYBOARD` | `SB_HW_IRQ_KEYBOARD` | `$20` | 键盘 |
| `HW_IRQ_4` | `SB_HW_IRQ_4` | `$10` | 中断 4 |
| `HW_IRQ_FLOPPY` | `SB_HW_IRQ_FLOPPY` | `$08` | 软驱 |
| `HW_IRQ_2` | `SB_HW_IRQ_2` | `$04` | 中断 2 |
| `HW_IRQ_1` | `SB_HW_IRQ_1` | `$02` | 中断 1 |

<a id="s30"></a>
## 文件打开方式（FILE_MODE）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `MODE_OPEN` | `SB_MODE_OPEN` | `$00` | 打开已有文件 |
| `MODE_CREATE` | `SB_MODE_CREATE` | `$01` | 创建文件，已存在则覆盖 |
| `MODE_UPDATE` | `SB_MODE_UPDATE` | `$02` | 打开已有文件，可按偏移写，关闭时写回目录项 |
| `MODE_DELETE_ENTRY` | `SB_MODE_DELETE_ENTRY` | `$03` | 删除文件：标为已删除并释放簇链 |
| `MODE_ENTRY` | `SB_MODE_ENTRY` | `$04` | 取得目录项，只对根目录与子目录成功 |
| `MODE_MAKE_DIR` | `SB_MODE_MAKE_DIR` | `$05` | 创建目录项 |

<a id="s31"></a>
## 出错码（DOS_ERROR）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `ERR_NONE` | `SB_ERR_NONE` | `$00` | 无错 |
| `ERR_BAD_FUNCTION` | `SB_ERR_BAD_FUNCTION` | `$01` | 无效的功能调用号 |
| `ERR_FILE_NOT_FOUND` | `SB_ERR_FILE_NOT_FOUND` | `$02` | 文件未找到 |
| `ERR_PATH_NOT_FOUND` | `SB_ERR_PATH_NOT_FOUND` | `$03` | 路径未找到 |
| `ERR_TOO_MANY_FILES` | `SB_ERR_TOO_MANY_FILES` | `$04` | 打开的文件过多，4 个句柄都在用 |
| `ERR_ACCESS_DENIED` | `SB_ERR_ACCESS_DENIED` | `$05` | 访问被拒绝，如写只读文件 |
| `ERR_NO_MEMORY` | `SB_ERR_NO_MEMORY` | `$08` | 无足够内存，没有足够的连续空闲页 |
| `ERR_BAD_ADDRESS` | `SB_ERR_BAD_ADDRESS` | `$09` | 无效的内存地址，页号不合法 |
| `ERR_NOT_READY` | `SB_ERR_NOT_READY` | `$15` | 驱动器未准备好 |
| `ERR_BAD_DISK_PARAM` | `SB_ERR_BAD_DISK_PARAM` | `$10` | 磁盘操作参数错，面、磁道超出范围 |
| `ERR_WRITE_PROTECT` | `SB_ERR_WRITE_PROTECT` | `$13` | 磁盘写保护，写操作失败 |
| `ERR_BAD_POSITION` | `SB_ERR_BAD_POSITION` | `$14` | 文件读写位置错误 |
| `ERR_CRC` | `SB_ERR_CRC` | `$17` | 循环冗余校验错 |
| `ERR_NO_DISK` | `SB_ERR_NO_DISK` | `$18` | 驱动器中没有磁盘 |
| `ERR_UNKNOWN_DISK` | `SB_ERR_UNKNOWN_DISK` | `$1A` | 未知磁盘类型 |
| `ERR_FILE_EXISTS` | `SB_ERR_FILE_EXISTS` | `$20` | 文件已经存在 |
| `ERR_BAD_PARAMETER` | `SB_ERR_BAD_PARAMETER` | `$25` | 参数无效 |
| `ERR_DIR_EXISTS` | `SB_ERR_DIR_EXISTS` | `$26` | 目录已经存在 |
| `ERR_BAD_NAME` | `SB_ERR_BAD_NAME` | `$27` | 文件名或目录名错误 |
| `ERR_BAD_OPEN_MODE` | `SB_ERR_BAD_OPEN_MODE` | `$28` | 文件打开方式错误 |
| `ERR_DIR_NOT_EMPTY` | `SB_ERR_DIR_NOT_EMPTY` | `$29` | 企图删除非空目录 |
| `ERR_DIR_TOO_DEEP` | `SB_ERR_DIR_TOO_DEEP` | `$2A` | 目录嵌套层数过多 |
| `ERR_NO_SPACE` | `SB_ERR_NO_SPACE` | `$2C` | 磁盘空间不够 |
| `ERR_DUPLICATE_NAME` | `SB_ERR_DUPLICATE_NAME` | `$2D` | 文件名重复，如要建的目录与文件同名 |
| `ERR_NOT_OPEN` | `SB_ERR_NOT_OPEN` | `$2E` | 文件未打开 |

<a id="s32"></a>
## 运行库入口（ROM 页组 0，经 FN_CALL_BANKED 且 SYS_GROUP=0 调用）

| 汇编名 | C 名 | 值 | 说明 |
| --- | --- | --- | --- |
| `LIB_PRINT_HEX` | `SB_LIB_PRINT_HEX` | `$8047` | 显示 A 的十六进制，经功能 $25 调用时 A 传不进去，不能用 |
| `LIB_BEEP` | `SB_LIB_BEEP` | `$802D` | 发一声提示音 |
| `LIB_CURSOR_LEFT` | `SB_LIB_CURSOR_LEFT` | `$8081` | 光标左移一列，到行首移到上一行末尾 |
| `LIB_CURSOR_UP` | `SB_LIB_CURSOR_UP` | `$808F` | 光标移到上一行末尾，已在第一行时不动 |
| `LIB_CURSOR_RIGHT` | `SB_LIB_CURSOR_RIGHT` | `$80CF` | 光标右移一列，到第 32 列换行 |
| `LIB_NEWLINE` | `SB_LIB_NEWLINE` | `$80E2` | 换行，行满滚屏；调用前 CURSOR_COL 要置 1 |
| `LIB_GET_KEY` | `SB_LIB_GET_KEY` | `$81F4` | 等待按键，结果在 KEY_ASCII、KEY_SCAN |
| `LIB_PRINT_MESSAGE` | `SB_LIB_PRINT_MESSAGE` | `$8202` | 显示消息号为 LIB_GROUP 值的内置消息 |
| `LIB_PRINT_STRING` | `SB_LIB_PRINT_STRING` | `$8262` | 在光标处显示 STR_PTR 指向的字符串，不移动光标 |
| `FN_RANDOM` | `SB_FN_RANDOM` | `$4A` | 伪随机数：32 位移位寄存器 RANDOM_STATE |
| `FN_DIV16` | `SB_FN_DIV16` | `$4B` | 16 位除以 8 位 |
| `FN_TIME_TEXT` | `SB_FN_TIME_TEXT` | `$4C` | 时、分、秒转成 HH:MM:SS 文字 |
| `FN_DECIMAL_PARSE` | `SB_FN_DECIMAL_PARSE` | `$4D` | 十进制文字转数值 |
| `FN_DECIMAL_FORMAT` | `SB_FN_DECIMAL_FORMAT` | `$4E` | 数值转十进制文字 |
| `FN_FLOAT_MUL` | `SB_FN_FLOAT_MUL` | `$4F` | 浮点乘 |
| `FN_FLOAT_DIV` | `SB_FN_FLOAT_DIV` | `$50` | 浮点除 |
| `FN_FLOAT_ADD` | `SB_FN_FLOAT_ADD` | `$51` | 浮点加 |
| `FN_FLOAT_SUB` | `SB_FN_FLOAT_SUB` | `$52` | 浮点减 |
| `FN_LINE_STEP` | `SB_FN_LINE_STEP` | `$53` | 直线步进（布雷森汉姆算法） |
| `FN_INT24` | `SB_FN_INT24` | `$54` | 3 字节整数的加、减、乘、除 |
| `RANDOM_STATE` | `SB_RANDOM_STATE` | `$0696` | 伪随机数的 32 位寄存器，低字节在前 |
| `ARG_069E` | `SB_ARG_069E` | `$069E` | 第二个 3 字节整数的起点 |
| `ARG_06A1` | `SB_ARG_06A1` | `$06A1` | 第二个浮点数的起点 |
| `ARG_06A8` | `SB_ARG_06A8` | `$06A8` | 浮点运算结果的起点 |
| `ARG_06AA` | `SB_ARG_06AA` | `$06AA` | 40 位整数结果与时间文字的起点 |
| `ARG_06AF` | `SB_ARG_06AF` | `$06AF` | 40 位整数转文字的输入起点 |
| `LINE_BLOCK_PTR` | `SB_LINE_BLOCK_PTR` | `$0008` | 零页：直线步进参数块的指针，低字节在前 |
| `LINE_BLOCK_BUFFER` | `SB_LINE_BLOCK_BUFFER` | `$7EE0` | 扩展 RAM 高端的 18 字节，放直线步进的参数块：功能 $53 换页后程序窗口里的内容不可见，参数块必须放在任何映射下都可见的地方 |
| `SUB_SOUND_PLAY` | `SB_SUB_SOUND_PLAY` | `$00` | 载入并播放曲子，先关声音更新，播完前再打开 |
| `SUB_SOUND_STOP` | `SB_SUB_SOUND_STOP` | `$02` | 停止播放 |
| `SUB_SOUND_STATUS` | `SB_SUB_SOUND_STATUS` | `$03` | 把 SOUND_STATUS 取到 PARAM_0653 |
| `SUB_SOUND_TRANSPOSE` | `SB_SUB_SOUND_TRANSPOSE` | `$04` | 把 PARAM_0653 加到 4 个声道的移调值 |
| `SUB_SOUND_TEMPO` | `SB_SUB_SOUND_TEMPO` | `$05` | 把 PARAM_0653 加到拍速 |
| `SUB_SOUND_WAV` | `SB_SUB_SOUND_WAV` | `$0C` | 播放内存里的 WAV |
| `SUB_SOUND_WAV_LOOP` | `SB_SUB_SOUND_WAV_LOOP` | `$0D` | 重复播放 WAV，次数在 PARAM_0652 |
| `SUB_SOUND_EFFECT` | `SB_SUB_SOUND_EFFECT` | `$0B` | 先复位再按曲目号播放音效曲子 |
| `SOUND_STATUS` | `SB_SOUND_STATUS` | `$03A1` | 播放状态：停止后为 $FF |
| `SOUND_TRANSPOSE` | `SB_SOUND_TRANSPOSE` | `$0447` | 4 个声道的移调值，连续 4 个字节 |
| `SOUND_TEMPO` | `SB_SOUND_TEMPO` | `$03D4` | 拍速 |
| `FN_SPEECH_FLAGS` | `SB_FN_SPEECH_FLAGS` | `$74` | 语音控制标志的开关 |
| `FN_SPEECH_SEND` | `SB_FN_SPEECH_SEND` | `$75` | 发送语音数据 |
| `SPEECH_FLAGS` | `SB_SPEECH_FLAGS` | `$032B` | 语音控制标志，位 0 为 0 同步发送、为 1 交给 NMI 发送 |
| `PARAM_064A` | `SB_PARAM_064A` | `$064A` | 显示与语音服务的参数：字符码高字节、行距、词条序号高字节等 |
| `PARAM_064B` | `SB_PARAM_064B` | `$064B` | 显示与语音服务的参数：字符码低字节、宽、词条序号低字节等 |
| `PARAM_064E` | `SB_PARAM_064E` | `$064E` | 打印机发送变换模式等参数 |
| `FN_PRINTER` | `SB_FN_PRINTER` | `$76` | 打印机服务，$76-$7A 是同一个入口，只看子功能 |
| `FN_IME` | `SB_FN_IME` | `$6D` | 中文输入法服务 |
| `PRN_PAGE_LEFT` | `SB_PRN_PAGE_LEFT` | `$04BC` | 本页剩余走纸量，16 位，高字节在前（单位 1/216 英寸） |
| `PRN_BYTE` | `SB_PRN_BYTE` | `$04A4` | 子功能 7 要发送的字节 |
| `PRN_POS_LO` | `SB_PRN_POS_LO` | `$04B3` | 子功能 5、6 的位置参数低字节 |
| `PRN_POS_HI` | `SB_PRN_POS_HI` | `$04B4` | 子功能 5、6 的位置参数高字节 |
| `IME_STATE` | `SB_IME_STATE` | `$6900` | 输入法状态区起点：低 4 位是输入法序号 |
| `IME_ALLOW` | `SB_IME_ALLOW` | `$6901` | 输入法允许位 |
| `IME_FLAGS` | `SB_IME_FLAGS` | `$6902` | 输入法标志位：位 1 全角、位 2 暂停、位 4 自定义输入法可选 |
| `IME_KEY_RESULT` | `SB_IME_KEY_RESULT` | `$6922` | 处理一个键的结果：0 无输出、2 输出汉字内码、$82 按键原样传回 |
| `IME_KEY_OUT` | `SB_IME_KEY_OUT` | `$6923` | 输入得到的字符或汉字，从这里起 |
| `IME_INPUT_COUNT` | `SB_IME_INPUT_COUNT` | `$6904` | 已输入的字符个数 |
| `IME_INPUT_BUFFER` | `SB_IME_INPUT_BUFFER` | `$6905` | 输入缓冲，最多 6 个字符 |
| `PARAM_064C` | `SB_PARAM_064C` | `$064C` | 显示参数：单元值或显示地址低字节 |
| `PARAM_064D` | `SB_PARAM_064D` | `$064D` | 显示参数：单元值或显示地址高字节 |
| `PARAM_0650` | `SB_PARAM_0650` | `$0650` | 单元个数或长度的高字节 |
| `PARAM_0651` | `SB_PARAM_0651` | `$0651` | 刷新方式或等待帧数 |
| `PARAM_0654` | `SB_PARAM_0654` | `$0654` | 调色板号 |
| `SYS_GROUP_2` | `SB_SYS_GROUP_2` | `$0643` | 第二个页组：解压到内存的目的页组、文字缓冲的目的页组 |
| `DEST_ADDR_LO` | `SB_DEST_ADDR_LO` | `$0012` | 零页：目的窗口地址低字节 |
| `DEST_ADDR_HI` | `SB_DEST_ADDR_HI` | `$0013` | 零页：目的窗口地址高字节 |
| `GLYPH_PAGE` | `SB_GLYPH_PAGE` | `$0644` | 字模所在的页，功能 $3B 的结果 |
| `GLYPH_OFFSET_LO` | `SB_GLYPH_OFFSET_LO` | `$000E` | 零页：字模页内偏移低字节，功能 $3B 的结果 |
| `GLYPH_OFFSET_HI` | `SB_GLYPH_OFFSET_HI` | `$000F` | 零页：字模页内偏移高字节（高 4 位是窗口位），功能 $3B 的结果 |
| `DISPLAY_MODE` | `SB_DISPLAY_MODE` | `$0638` | 显示模式影子：低 3 位是字模形式，位 7 为 1 取 8×16 字模 |
| `FN_DMA_NAMETABLE` | `SB_FN_DMA_NAMETABLE` | `$30` | 名称表 DMA 与屏幕缓冲服务 |
| `FN_VRAM_FILL` | `SB_FN_VRAM_FILL` | `$32` | 用一个字节填充显存的一段 |
| `FN_VRAM_LOAD` | `SB_FN_VRAM_LOAD` | `$33` | 把扩展 RAM 的一段 DMA 到显存 |
| `FN_GLYPH_ADDR` | `SB_FN_GLYPH_ADDR` | `$3B` | 查字模的页与地址 |
| `FN_PUT_STRING_C` | `SB_FN_PUT_STRING_C` | `$3C` | 画二平面字符串到显存瓦片（汉字与西文） |
| `FN_PUT_STRING_D` | `SB_FN_PUT_STRING_D` | `$3D` | 画四平面字符串到显存瓦片（汉字与西文） |
| `FN_PUT_ASCII_C` | `SB_FN_PUT_ASCII_C` | `$3E` | 画二平面字符串到显存瓦片（只处理单字节字符） |
| `FN_PUT_ASCII_D` | `SB_FN_PUT_ASCII_D` | `$3F` | 画四平面字符串到显存瓦片（只处理单字节字符） |
| `FN_CELLS` | `SB_FN_CELLS` | `$40` | 把单元写进名称表缓冲并刷新 |
| `FN_TEXT_FIELD` | `SB_FN_TEXT_FIELD` | `$45` | 在屏幕上显示一行文字（汉字与西文） |
| `FN_TEXT_FIELD_ASCII` | `SB_FN_TEXT_FIELD_ASCII` | `$46` | 在屏幕上显示一行文字（只处理单字节字符） |
| `FN_TEXT_BUFFER` | `SB_FN_TEXT_BUFFER` | `$47` | 把字符串画成点阵瓦片写进内存（汉字与西文） |
| `FN_TEXT_BUFFER_ASCII` | `SB_FN_TEXT_BUFFER_ASCII` | `$48` | 把字符串画成点阵瓦片写进内存（只处理单字节字符） |
| `FN_WINDOW` | `SB_FN_WINDOW` | `$59` | 窗口管理：保存与还原屏幕矩形 |
| `FN_IMAGE_RAW` | `SB_FN_IMAGE_RAW` | `$5A` | 不解压直接把一段图案 DMA 进显存 |
| `FN_UNPACK_TILES` | `SB_FN_UNPACK_TILES` | `$5B` | 解压图案流进显存 |
| `FN_UNPACK_MAP` | `SB_FN_UNPACK_MAP` | `$5C` | 解压映射流并写到屏幕 |
| `FN_PALETTE` | `SB_FN_PALETTE` | `$5D` | 写调色板 |
| `FN_IMAGE_FULL` | `SB_FN_IMAGE_FULL` | `$5E` | 显示整屏图片 |
| `FN_IMAGE_SHOW` | `SB_FN_IMAGE_SHOW` | `$5F` | 显示压缩图片 |
| `FN_FILL_RECT` | `SB_FN_FILL_RECT` | `$60` | 用一个单元值按效果铺满矩形 |
| `FN_UNPACK_MEMORY` | `SB_FN_UNPACK_MEMORY` | `$61` | 解压到内存 |
