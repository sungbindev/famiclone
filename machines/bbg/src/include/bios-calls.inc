; ------------------------------------------------------------------------
; 名称：bios-calls.inc
; 说明：步步高 BIOS 调用的功能号。调用方法：功能号放在 A，JSR BIOS_ENTRY（$5800），其余参数见各功能。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：功能号与处理例程来自对 ember/roms/bbg1_bios.rom（BIOS 1.0）的反汇编：分发表在 $D0A5，
;       共 43 项（下标 0-42）。功能号 $3F-$41 先减 $17 再查表，因此 63、64、65 与 40、41、42 是同一个功能；
;       功能号 60 直接转到显示初始化；功能号 $FF 返回版本。其他功能号被忽略，A、X、Y 不变。
;       名称取自使用手册；手册没有的功能号用 BIOS_FN_ 加十进制编号。
;       “验证”一栏：反汇编 = 已读代码核对；手册 = 只依据手册；DOS = 由 BBGDOS.SYS 的调用样例核对。
; ------------------------------------------------------------------------

;---------------------------------------; BIOS 1.0 实现的功能号（分发表下标 = 功能号）
BIOS_NO_OPERATION       = 0     ; 空操作，直接返回（反汇编）
BIOS_SET_PAGE           = 1     ; 设置 $8000-$BFFF 窗口的页：X 为页号，0 取 $FE，$FE 取系统默认页 [$5FF9]（反汇编）
BIOS_GET_PAGE           = 2     ; 取窗口当前页：A 为页号，系统默认页返回 $FE，$FE 页返回 0（反汇编）
BIOS_SET_AV10           = 3     ; 设置显存 AV10 连接方式：X 的位 0、1 写入系统控制寄存器映像 [$5FFF] 的位 0、1（反汇编）
BIOS_RUN_INITIAL        = 4     ; 重新初始化系统资源：清变量、重设 NMI 与 IRQ 向量（反汇编）
BIOS_LOW_RAM_FREE       = 5     ; 取低端内存（$5800-$5FFF 区）的剩余量（手册）
BIOS_DISK_IO            = 6     ; 软盘读写：X:Y 指向磁盘 I/O 块（X 高、Y 低），A 返回 0 表示成功（反汇编、DOS）
BIOS_GET_DRAM_SIZE      = 7     ; 取内存页数：A、X 返回最后一页的页号加 1（反汇编）
BIOS_FN_8               = 8     ; 手册未列：A 返回 [$5FF9]，并把 X 写入 [$5FF9]（反汇编）
BIOS_FN_9               = 9     ; 手册未列，未核对
BIOS_FN_10              = 10    ; 手册未列：A 返回 [$5FF5]（反汇编）
BIOS_READ_KEY           = 11    ; 读键盘与手柄（手册）
BIOS_MOVE_BLOCK         = 12    ; 在页之间复制内存块：X 为零页里块描述的地址（DOS）
BIOS_FN_13              = 13    ; 手册未列：向 16 字节的块发送命令（反汇编，用途未确认）
BIOS_WAIT_VSYNC_COUNT   = 14    ; 等待若干个垂直同步（手册）
BIOS_LOW_RAM_ALLOC      = 15    ; 申请低端内存：X:Y 为字节数（X 高、Y 低），A=0 成功，X:Y 返回地址（X 高、Y 低）（DOS）
BIOS_FN_16              = 16    ; 手册未列，未核对
BIOS_EXCHANGE_BLOCK     = 17    ; 在页之间交换内存块（DOS）
BIOS_SPLIT_SCREEN       = 18    ; 屏幕分割（手册）
BIOS_FN_19              = 19    ; 手册未列，未核对
BIOS_GET_DRIVE_MESSAGE  = 20    ; 取驱动器信息（手册）
BIOS_FN_21              = 21    ; 手册未列，未核对
BIOS_SET_NMI_VECTOR     = 22    ; 设置 NMI 后续处理向量：X:Y 为地址（DOS）
BIOS_GET_NMI_VECTOR     = 23    ; 取 NMI 后续处理向量：X:Y 返回地址（DOS）
BIOS_DEVICE             = 24    ; 外设调用，与 DEVICE_ENTRY（$5809）功能相同，X 为子功能（反汇编）
BIOS_FN_25              = 25    ; 手册未列，未核对
BIOS_FN_26              = 26    ; 手册未列：A 返回 0（反汇编）
BIOS_DISPLAY_TTY        = 27    ; 以电传方式显示一个字符：X 为字符码，Y 为 0（DOS）
BIOS_SET_CURSOR         = 28    ; 设置光标位置（手册）
BIOS_DISPLAY_CHARS      = 29    ; 显示若干个相同字符：X 为字符码，Y 为个数（DOS）
BIOS_GET_CURSOR         = 30    ; 取光标位置：Y 返回列（DOS）
BIOS_FN_31              = 31    ; 手册未列，未核对
BIOS_GET_PAGED_CHAR     = 32    ; 读其他页的一个字节：X 为页号，$FE/$FF 为地址，Y 为偏移，A 返回字节（反汇编、DOS）
BIOS_SET_PAGED_CHAR     = 33    ; 写其他页的一个字节：X 为页号，$FD 为要写的值，$FE/$FF 为地址，Y 为偏移（反汇编）
BIOS_WAIT_DISPLAY       = 34    ; 等待显示完成（手册）
BIOS_FN_35              = 35    ; 手册未列，未核对
BIOS_FN_36              = 36    ; 手册未列，未核对
BIOS_UNZIP_BLOCK        = 37    ; 解压内存块（手册）
BIOS_FN_38              = 38    ; 手册未列，未核对
BIOS_SET_END_VECTOR     = 39    ; 设置 NMI 结束时调用的处理：X:Y 存入 $597F、$597E，并记下当前页（反汇编，用途细节未确认）
BIOS_GET_DATE_TIME      = 40    ; 取日期时间（与 63 相同）：A=[$5FE7]，X=[$5FE9]，Y=[$5FE8]，零页 $FF=[$5FE6]，$FE=[$5FEA]（反汇编）
BIOS_SET_SYSTEM_DATE    = 41    ; 设置日期（与 64 相同）：X 存入 [$5FE7]，Y 存入 [$5FE6]
BIOS_SET_SYSTEM_TIME    = 42    ; 设置时间（与 65 相同）：X 存入 [$5FE9]，Y 存入 [$5FE8]，零页 $FF 存入 [$5FEA]
BIOS_DISPLAY_INITIAL    = 60    ; 显示初始化（反汇编：直接转到 $EA0E）
BIOS_GET_DATE_TIME_63   = 63    ; 同 40
BIOS_SET_SYSTEM_DATE_64 = 64    ; 同 41
BIOS_SET_SYSTEM_TIME_65 = 65    ; 同 42
BIOS_GET_VERSION        = 255   ; 取 BIOS 版本：A=$40，X=$61，Y=1（反汇编）

;---------------------------------------; 手册列出、但 bbg1 的 BIOS 1.0 不实现的功能号（调用时被忽略）
BIOS_SET_PALETTE        = 43    ; 设置调色板
BIOS_DISPLAY_CH_AT      = 44    ; 在指定位置显示字符
BIOS_DISP_REF_CHAR      = 45    ; 显示参考字符
BIOS_DISP_FG_CHAR_AT    = 47    ; 在指定位置显示前景字符
BIOS_SET_CURSOR_TYPE    = 48    ; 设置光标形状
BIOS_GET_SET_EXT_FONT   = 49    ; 取或设置扩展字库
BIOS_DISP_STRING_AT     = 50    ; 在指定位置显示字符串
BIOS_GET_SYSTEM_FONT    = 51    ; 取系统字库
BIOS_SET_SCREEN_SW      = 52    ; 设置屏幕开关
BIOS_MOUSE_OPERATE      = 53    ; 鼠标操作
BIOS_SET_BG_FONT        = 54    ; 设置背景字库
BIOS_GET_BG_FONT        = 55    ; 取背景字库
BIOS_SET_FG_FONT        = 56    ; 设置前景字库
BIOS_GET_FG_FONT        = 57    ; 取前景字库
BIOS_DISP_HALF_CH_AT    = 58    ; 显示半角字符
BIOS_MOVE_FG_CHAR       = 59    ; 移动前景字符
BIOS_DISP_OPEN_CLOSE    = 61    ; 打开或关闭显示
BIOS_CHG_BG_COLOR       = 62    ; 改变背景色
BIOS_DISP_GRAPHICS      = 66    ; 显示图形
BIOS_GET_SPLIT_POINT    = 67    ; 取分割点
BIOS_SET_LINE_WIDTH     = 68    ; 设置行宽
BIOS_SET_SCREEN_PAN     = 69    ; 设置屏幕平移

;---------------------------------------; 磁盘 I/O 块（BIOS_DISK_IO 的参数，由引导扇区与 DOS 的调用核对）
DISK_BLOCK_COMMAND      = 0     ; 命令：0 复位，2 读
DISK_BLOCK_DRIVE        = 1     ; 驱动器号
DISK_BLOCK_CYLINDER     = 2     ; 柱面低 8 位
DISK_BLOCK_HEAD         = 3     ; 磁头
DISK_BLOCK_SECTOR       = 4     ; 扇区号（从 1 起），位 6、7 是柱面的高 2 位
DISK_BLOCK_BUFFER_PAGE  = 5     ; 缓冲区所在页，$FE 表示当前页
DISK_BLOCK_BUFFER_LOW   = 6     ; 缓冲区地址低字节
DISK_BLOCK_BUFFER_HIGH  = 7     ; 缓冲区地址高字节
DISK_BLOCK_SECTOR_COUNT = 8     ; 扇区数
DISK_COMMAND_RESET      = 0     ; 复位磁盘
DISK_COMMAND_READ       = 2     ; 读扇区
