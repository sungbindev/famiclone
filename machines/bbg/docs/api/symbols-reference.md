# 步步高系统符号参考

本文由 `tools/gen-symbols-doc.mjs` 从 `src/include` 下的符号定义文件生成，不手工修改；改符号文件后重新运行。

## system-entries.inc

步步高系统调用入口向量，位于 $5800-$5818 的公共 RAM，每个入口是一条 JMP。

### 符号

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `BIOS_ENTRY` | `$5800` | BIOS 调用：A 为功能号 |
| `FILE_ENTRY` | `$5803` | 磁盘文件调用：X 为通道块地址高字节，Y 为低字节，A 不用设置 |
| `COLD_START_ENTRY` | `$5806` | 系统冷启动，按复位键时执行 |
| `DEVICE_ENTRY` | `$5809` | 外设调用 |
| `DOS_ENTRY` | `$580C` | DOS 调用：A 为功能号 |
| `WARM_START_ENTRY` | `$580F` | 系统热启动，按 Ctrl+Alt+Del 时执行 |
| `CTRL_BREAK_ENTRY` | `$5812` | Ctrl+Break 处理，键盘硬中断里调用，应用可改写其中的 JMP 目标 |
| `CTRL_C_ENTRY` | `$5815` | Ctrl+C 处理，DOS 输出字符时检测到后调用，应用可改写其中的 JMP 目标 |
| `CRITICAL_ERROR_ENTRY` | `$5818` | 严重错误处理，DOS 遇到磁盘错误时调用 |

## bios-calls.inc

步步高 BIOS 调用的功能号。调用方法：功能号放在 A，JSR BIOS_ENTRY（$5800），其余参数见各功能。

### BIOS 1.0 实现的功能号（分发表下标 = 功能号）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `BIOS_NO_OPERATION` | `0` | 空操作，直接返回（反汇编） |
| `BIOS_SET_PAGE` | `1` | 设置 $8000-$BFFF 窗口的页：X 为页号，0 取 $FE，$FE 取系统默认页 [$5FF9]（反汇编） |
| `BIOS_GET_PAGE` | `2` | 取窗口当前页：A 为页号，系统默认页返回 $FE，$FE 页返回 0（反汇编） |
| `BIOS_SET_AV10` | `3` | 设置显存 AV10 连接方式：X 的位 0、1 写入系统控制寄存器映像 [$5FFF] 的位 0、1（反汇编） |
| `BIOS_RUN_INITIAL` | `4` | 重新初始化系统资源：清变量、重设 NMI 与 IRQ 向量（反汇编） |
| `BIOS_LOW_RAM_FREE` | `5` | 取低端内存（$5800-$5FFF 区）的剩余量（手册） |
| `BIOS_DISK_IO` | `6` | 软盘读写：X:Y 指向磁盘 I/O 块（X 高、Y 低），A 返回 0 表示成功（反汇编、DOS） |
| `BIOS_GET_DRAM_SIZE` | `7` | 取内存页数：A、X 返回最后一页的页号加 1（反汇编） |
| `BIOS_FN_8` | `8` | 手册未列：A 返回 [$5FF9]，并把 X 写入 [$5FF9]（反汇编） |
| `BIOS_FN_9` | `9` | 手册未列，未核对 |
| `BIOS_FN_10` | `10` | 手册未列：A 返回 [$5FF5]（反汇编） |
| `BIOS_READ_KEY` | `11` | 读键盘与手柄（手册） |
| `BIOS_MOVE_BLOCK` | `12` | 在页之间复制内存块：X 为零页里块描述的地址（DOS） |
| `BIOS_FN_13` | `13` | 手册未列：向 16 字节的块发送命令（反汇编，用途未确认） |
| `BIOS_WAIT_VSYNC_COUNT` | `14` | 等待若干个垂直同步（手册） |
| `BIOS_LOW_RAM_ALLOC` | `15` | 申请低端内存：X:Y 为字节数（X 高、Y 低），A=0 成功，X:Y 返回地址（X 高、Y 低）（DOS） |
| `BIOS_FN_16` | `16` | 手册未列，未核对 |
| `BIOS_EXCHANGE_BLOCK` | `17` | 在页之间交换内存块（DOS） |
| `BIOS_SPLIT_SCREEN` | `18` | 屏幕分割（手册） |
| `BIOS_FN_19` | `19` | 手册未列，未核对 |
| `BIOS_GET_DRIVE_MESSAGE` | `20` | 取驱动器信息（手册） |
| `BIOS_FN_21` | `21` | 手册未列，未核对 |
| `BIOS_SET_NMI_VECTOR` | `22` | 设置 NMI 后续处理向量：X:Y 为地址（DOS） |
| `BIOS_GET_NMI_VECTOR` | `23` | 取 NMI 后续处理向量：X:Y 返回地址（DOS） |
| `BIOS_DEVICE` | `24` | 外设调用，与 DEVICE_ENTRY（$5809）功能相同，X 为子功能（反汇编） |
| `BIOS_FN_25` | `25` | 手册未列，未核对 |
| `BIOS_FN_26` | `26` | 手册未列：A 返回 0（反汇编） |
| `BIOS_DISPLAY_TTY` | `27` | 以电传方式显示一个字符：X 为字符码，Y 为 0（DOS） |
| `BIOS_SET_CURSOR` | `28` | 设置光标位置（手册） |
| `BIOS_DISPLAY_CHARS` | `29` | 显示若干个相同字符：X 为字符码，Y 为个数（DOS） |
| `BIOS_GET_CURSOR` | `30` | 取光标位置：Y 返回列（DOS） |
| `BIOS_FN_31` | `31` | 手册未列，未核对 |
| `BIOS_GET_PAGED_CHAR` | `32` | 读其他页的一个字节：X 为页号，$FE/$FF 为地址，Y 为偏移，A 返回字节（反汇编、DOS） |
| `BIOS_SET_PAGED_CHAR` | `33` | 写其他页的一个字节：X 为页号，$FD 为要写的值，$FE/$FF 为地址，Y 为偏移（反汇编） |
| `BIOS_WAIT_DISPLAY` | `34` | 等待显示完成（手册） |
| `BIOS_FN_35` | `35` | 手册未列，未核对 |
| `BIOS_FN_36` | `36` | 手册未列，未核对 |
| `BIOS_UNZIP_BLOCK` | `37` | 解压内存块（手册） |
| `BIOS_FN_38` | `38` | 手册未列，未核对 |
| `BIOS_SET_END_VECTOR` | `39` | 设置 NMI 结束时调用的处理：X:Y 存入 $597F、$597E，并记下当前页（反汇编，用途细节未确认） |
| `BIOS_GET_DATE_TIME` | `40` | 取日期时间（与 63 相同）：A=[$5FE7]，X=[$5FE9]，Y=[$5FE8]，零页 $FF=[$5FE6]，$FE=[$5FEA]（反汇编） |
| `BIOS_SET_SYSTEM_DATE` | `41` | 设置日期（与 64 相同）：X 存入 [$5FE7]，Y 存入 [$5FE6] |
| `BIOS_SET_SYSTEM_TIME` | `42` | 设置时间（与 65 相同）：X 存入 [$5FE9]，Y 存入 [$5FE8]，零页 $FF 存入 [$5FEA] |
| `BIOS_DISPLAY_INITIAL` | `60` | 显示初始化（反汇编：直接转到 $EA0E） |
| `BIOS_GET_DATE_TIME_63` | `63` | 同 40 |
| `BIOS_SET_SYSTEM_DATE_64` | `64` | 同 41 |
| `BIOS_SET_SYSTEM_TIME_65` | `65` | 同 42 |
| `BIOS_GET_VERSION` | `255` | 取 BIOS 版本：A=$40，X=$61，Y=1（反汇编） |

### 手册列出、但 bbg1 的 BIOS 1.0 不实现的功能号（调用时被忽略）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `BIOS_SET_PALETTE` | `43` | 设置调色板 |
| `BIOS_DISPLAY_CH_AT` | `44` | 在指定位置显示字符 |
| `BIOS_DISP_REF_CHAR` | `45` | 显示参考字符 |
| `BIOS_DISP_FG_CHAR_AT` | `47` | 在指定位置显示前景字符 |
| `BIOS_SET_CURSOR_TYPE` | `48` | 设置光标形状 |
| `BIOS_GET_SET_EXT_FONT` | `49` | 取或设置扩展字库 |
| `BIOS_DISP_STRING_AT` | `50` | 在指定位置显示字符串 |
| `BIOS_GET_SYSTEM_FONT` | `51` | 取系统字库 |
| `BIOS_SET_SCREEN_SW` | `52` | 设置屏幕开关 |
| `BIOS_MOUSE_OPERATE` | `53` | 鼠标操作 |
| `BIOS_SET_BG_FONT` | `54` | 设置背景字库 |
| `BIOS_GET_BG_FONT` | `55` | 取背景字库 |
| `BIOS_SET_FG_FONT` | `56` | 设置前景字库 |
| `BIOS_GET_FG_FONT` | `57` | 取前景字库 |
| `BIOS_DISP_HALF_CH_AT` | `58` | 显示半角字符 |
| `BIOS_MOVE_FG_CHAR` | `59` | 移动前景字符 |
| `BIOS_DISP_OPEN_CLOSE` | `61` | 打开或关闭显示 |
| `BIOS_CHG_BG_COLOR` | `62` | 改变背景色 |
| `BIOS_DISP_GRAPHICS` | `66` | 显示图形 |
| `BIOS_GET_SPLIT_POINT` | `67` | 取分割点 |
| `BIOS_SET_LINE_WIDTH` | `68` | 设置行宽 |
| `BIOS_SET_SCREEN_PAN` | `69` | 设置屏幕平移 |

### 磁盘 I/O 块（BIOS_DISK_IO 的参数，由引导扇区与 DOS 的调用核对）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `DISK_BLOCK_COMMAND` | `0` | 命令：0 复位，2 读 |
| `DISK_BLOCK_DRIVE` | `1` | 驱动器号 |
| `DISK_BLOCK_CYLINDER` | `2` | 柱面低 8 位 |
| `DISK_BLOCK_HEAD` | `3` | 磁头 |
| `DISK_BLOCK_SECTOR` | `4` | 扇区号（从 1 起），位 6、7 是柱面的高 2 位 |
| `DISK_BLOCK_BUFFER_PAGE` | `5` | 缓冲区所在页，$FE 表示当前页 |
| `DISK_BLOCK_BUFFER_LOW` | `6` | 缓冲区地址低字节 |
| `DISK_BLOCK_BUFFER_HIGH` | `7` | 缓冲区地址高字节 |
| `DISK_BLOCK_SECTOR_COUNT` | `8` | 扇区数 |
| `DISK_COMMAND_RESET` | `0` | 复位磁盘 |
| `DISK_COMMAND_READ` | `2` | 读扇区 |

## dos-calls.inc

BBGDOS 3.0 的 DOS 调用（$580C）与文件调用（$5803）的功能号、文件通道块的布局与出错码。

### DOS 功能号（DOS_ENTRY = $580C）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `DOS_PROGRAM_RETURN` | `0` | 程序结束返回：X 返回码，Y 位 0=1 先初始化屏幕，位 1=1 先初始化系统资源（反汇编） |
| `DOS_READ_KEY_ECHO` | `1` | 读键盘并回显：A 返回字符码，0 表示功能键（反汇编） |
| `DOS_DISPLAY_CHAR` | `2` | 显示字符：X 为字符码（反汇编） |
| `DOS_INQUIRE_PAGES` | `3` | 申请内存页：X 为页数，0 取空闲页号；C=0 时 A 为起始页，C=1 时 A 为最多可申请页数（手册） |
| `DOS_DISPLAY_STRING` | `4` | 显示以 '$' 结尾的字符串：X 为地址高字节，Y 为低字节（反汇编） |
| `DOS_RUN_PROGRAM` | `5` | 运行程序：$FF 为盘号，X:Y 为文件名地址，$0100 起为参数（手册） |
| `DOS_GET_KB_STRING` | `6` | 输入一行并回显：X:Y 为缓冲区，第 0 字节最大个数，第 1 字节返回个数（反汇编） |
| `DOS_READ_KEY_NO_ECHO` | `7` | 读键盘不回显：A 返回字符码，0 表示功能键（反汇编） |
| `DOS_READ_KB_CLEAR` | `8` | 先清键盘缓冲，再执行 $FF 里指定的键盘功能 1、6、7（手册） |
| `DOS_INITIAL_SCREEN` | `9` | 初始化显示屏幕并清屏（反汇编） |
| `DOS_RELEASE_PAGES` | `10` | 释放内存页：X 为起始页号，从该页起的页都被释放（反汇编） |
| `DOS_GET_SET_DISP_MODE` | `11` | 取或设置显示方式：X 位 0=0 取，A 返回 0（英文）或 $80（中文）；位 0=1 设置，Y 为 0 或 $80（反汇编） |
| `DOS_FILE_NAME_ACCESS` | `12` | 文件名转换：X:Y 为文件名地址，转成 11 字节的内部格式，A 返回盘号（手册） |
| `DOS_GET_SET_INIT_SCREEN` | `13` | 取并设置初始化屏幕向量：$FF 为新向量所在页，X:Y 为地址减 1，返回原值（反汇编） |
| `DOS_CHECK_KEYBOARD` | `14` | 测试键盘：Z=1 无键，Z=0 有键且 A 为键码（反汇编） |
| `DOS_RUN_PATH_PROGRAM` | `15` | 运行程序，文件名可带路径（手册） |
| `DOS_GET_DOS_VERSION` | `16` | 取 DOS 版本：A=3，X=0，C=0（反汇编，手册未列） |

### 文件调用功能号（FILE_ENTRY = $5803），通道块第 0 字节

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `FILE_RESET` | `0` | 复位文件系统（手册） |
| `FILE_OPEN` | `1` | 打开文件（手册） |
| `FILE_READ` | `2` | 读文件（手册） |
| `FILE_WRITE` | `3` | 写文件（手册） |
| `FILE_SEEK` | `4` | 移动读写位置（手册） |
| `FILE_CLOSE` | `5` | 关闭文件（手册） |
| `FILE_FIRST` | `6` | 查找第一个文件（反汇编、Ember 实测） |
| `FILE_NEXT` | `7` | 查找下一个文件，没有更多时 A=$12（反汇编、Ember 实测） |
| `FILE_CREATE` | `8` | 建立文件（手册） |
| `FILE_DRIVE` | `9` | 设置默认驱动器（手册） |
| `FILE_DELETE` | `10` | 删除文件（手册） |
| `FILE_DRIVE_SIZE` | `11` | 取驱动器容量（手册） |
| `FILE_ATTRIBUTE` | `12` | 取或设置文件属性（手册） |
| `FILE_RENAME` | `13` | 文件改名（手册） |
| `FILE_READ_ABS` | `14` | 按绝对扇区读（手册） |
| `FILE_WRITE_ABS` | `15` | 按绝对扇区写（手册） |
| `FILE_CHANGE_DIR` | `16` | 改变当前目录（手册） |
| `FILE_MAKE_DIR` | `17` | 建立目录（手册） |
| `FILE_REMOVE_DIR` | `18` | 删除目录（手册） |
| `FILE_GET_DIR` | `19` | 取当前目录（手册） |
| `FILE_PATH_OPEN` | `20` | 按路径打开文件，与 FILE_OPEN 共用处理例程（反汇编） |
| `FILE_PATH_CREATE` | `21` | 按路径建立文件，与 FILE_CREATE 共用处理例程（反汇编） |
| `FILE_PATH_FIRST` | `22` | 按路径查找第一个文件（反汇编） |
| `FILE_PATH_DELETE` | `23` | 按路径删除文件，与 FILE_DELETE 共用处理例程（反汇编） |
| `FILE_SET_ATTRIBUTE` | `24` | 手册未列：按块偏移 $16 的值改写目录项属性（反汇编） |

### 文件通道块的布局（共 48 字节，FILE_ENTRY 的参数）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `FH_COMMAND` | `$00` | 功能号 |
| `FH_DRIVE` | `$01` | 盘号：0 默认盘，1 为 A 盘，$FF 为启动盘（反汇编） |
| `FH_PAGE` | `$02` | 缓冲区所在页 |
| `FH_BUFFER` | `$03` | 缓冲区地址（2 字节，低字节在前） |
| `FH_BYTES` | `$05` | 读写长度（2 字节）；FILE_FIRST 等功能里，$05 起的 11 字节是文件名 |
| `FH_NAME` | `$05` | 8 字节主名加 3 字节扩展名，空格补齐，'?' 为通配符 |
| `FH_POSITION` | `$07` | 读写位置 |
| `FH_ATTRIBUTE` | `$16` | 查找时的属性掩码；FILE_FIRST 先把它置为 $FF |
| `FH_DTA` | `$17` | 查找结果区（FILE_FIRST、FILE_NEXT 返回） |
| `FH_DTA_ATTRIBUTE` | `$17` | 找到的目录项属性 |
| `FH_DTA_TIME` | `$18` | 修改时间（2 字节） |
| `FH_DTA_DATE` | `$1A` | 修改日期（2 字节） |
| `FH_DTA_SIZE` | `$1C` | 文件大小（4 字节） |
| `FH_DTA_NAME` | `$20` | 文件名 11 字节 |
| `FH_SEARCH_STATE` | `$2B` | 查找状态（页号、$9BF5 与当前目录簇，供 FILE_NEXT 使用） |
| `FH_SIZE` | `48` | 通道块长度 |

### 出错码（返回在 A）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `ERR_OK` | `$00` | 成功 |
| `ERR_COMMAND` | `$01` | 功能号错误 |
| `ERR_FILE_NOT_EXIST` | `$02` | 文件不存在 |
| `ERR_PATH_NOT_FOUND` | `$03` | 路径不存在 |
| `ERR_ACCESS_DENIED` | `$05` | 拒绝访问（含通配符建文件、与目录或卷标同名） |
| `ERR_FORMAT` | `$05` | 格式错误（手册） |
| `ERR_TOO_SMALL_DRAM` | `$08` | 内存不够 |
| `ERR_DRIVE_INVALID` | `$0F` | 驱动器号出错 |
| `ERR_REMOVE_CURRENT_DIR` | `$10` | 试图删除当前目录 |
| `ERR_SEARCH_ENDING` | `$12` | 没有更多文件 |
| `ERR_DRIVE_NOT_READY` | `$15` | 驱动器未准备好 |
| `ERR_CRC` | `$17` | CRC 错误 |
| `ERR_UNKNOWN_MEDIA` | `$1A` | 未识别介质类型 |
| `ERR_SECTOR_NOT_FOUND` | `$1B` | 扇区未找到 |
| `ERR_READ` | `$1E` | 读文件错 |
| `ERR_GENERAL` | `$1F` | 一般错误 |
| `ERR_DISPLAY_MODE` | `$28` | 显示方式错误 |
| `ERR_TOO_SMALL_VRAM` | `$29` | 显存太小 |
| `ERR_FAT` | `$2A` | FAT 表错误 |

### 显示方式（DOS_GET_SET_DISP_MODE）

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `DISP_MODE_ENGLISH` | `$00` | 英文方式，30 行 |
| `DISP_MODE_CHINESE` | `$80` | 中文方式，14 加 1 行 |

### 程序文件

| 符号 | 值 | 说明 |
| --- | --- | --- |
| `RUN_COMMAND_LINE` | `$0100` | 运行程序的参数：$0100 为长度，$0101 起为文本 |
| `CMD_LOAD_ADDRESS_SIZE` | `2` | CMD 文件前 2 字节是装入地址，之后的内容装入该地址并从该地址开始执行 |
