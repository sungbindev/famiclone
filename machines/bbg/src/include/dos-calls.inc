; ------------------------------------------------------------------------
; 名称：dos-calls.inc
; 说明：BBGDOS 3.0 的 DOS 调用（$580C）与文件调用（$5803）的功能号、文件通道块的布局与出错码。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-24
; 备注：功能号与分发表来自对 BBGDOS.IMG 里 BBGDOS.SYS 的反汇编：DOS 分发表在 $A664（17 项），
;       文件调用分发表在 $900F、$9028（25 项）。“验证”：反汇编 = 已读处理代码；手册 = 只依据手册。
;       调用方法：
;       DOS 调用：A 为功能号，其余参数见各功能，JSR DOS_ENTRY；返回时 C=0 成功，C=1 出错且 A 为出错码。
;       文件调用：X 为通道块地址高字节，Y 为低字节（A 不用设置），JSR FILE_ENTRY；通道块第 0 字节是功能号；
;                 返回时 A 为出错码（0 成功），C=1 表示出错。
; ------------------------------------------------------------------------

;---------------------------------------; DOS 功能号（DOS_ENTRY = $580C）
DOS_PROGRAM_RETURN      = 0     ; 程序结束返回：X 返回码，Y 位 0=1 先初始化屏幕，位 1=1 先初始化系统资源（反汇编、Ember 实测）
DOS_READ_KEY_ECHO       = 1     ; 读键盘并回显：A 返回字符码，0 表示功能键（反汇编、Ember 实测）
DOS_DISPLAY_CHAR        = 2     ; 显示字符：X 为字符码（反汇编、Ember 实测）
DOS_INQUIRE_PAGES       = 3     ; 申请内存页：X 为页数，0 取空闲页号；C=0 时 A 为起始页，C=1 时 A 为最多可申请页数（手册、Ember 实测）
DOS_DISPLAY_STRING      = 4     ; 显示以 '$' 结尾的字符串：X 为地址高字节，Y 为低字节（反汇编、Ember 实测）
DOS_RUN_PROGRAM         = 5     ; 运行程序：$FF 为盘号，X:Y（X 高）为 11 字节文件名（扩展名必须是 CMD、EXC、GAM），$0100 起为参数；C=0 时 A 为退出码（Ember 实测）
DOS_GET_KB_STRING       = 6     ; 输入一行并回显：X:Y 为缓冲区，第 0 字节最大个数，第 1 字节返回个数（反汇编、Ember 实测）
DOS_READ_KEY_NO_ECHO    = 7     ; 读键盘不回显：A 返回字符码，0 表示功能键（反汇编、Ember 实测）
DOS_READ_KB_CLEAR       = 8     ; 先清键盘缓冲，再执行 $FF 里指定的键盘功能 1、6、7（手册、Ember 实测）
DOS_INITIAL_SCREEN      = 9     ; 初始化显示屏幕并清屏（反汇编）
DOS_RELEASE_PAGES       = 10    ; 释放内存页：X 为起始页号，从该页起的页都被释放（反汇编）
DOS_GET_SET_DISP_MODE   = 11    ; 取或设置显示方式：X 位 0=0 取，A 返回 0（英文）或 $80（中文）；位 0=1 设置，Y 为 0 或 $80（反汇编、Ember 实测）
DOS_FILE_NAME_ACCESS    = 12    ; 文件名转换：X:Y 为文件名地址，转成 11 字节的内部格式，A 返回盘号（手册、Ember 实测）
DOS_GET_SET_INIT_SCREEN = 13    ; 取并设置初始化屏幕向量：$FF 为新向量所在页，X:Y 为地址减 1，返回原值（反汇编）
DOS_CHECK_KEYBOARD      = 14    ; 测试键盘：Z=1 无键，Z=0 有键且 A 为键码（反汇编、Ember 实测）
DOS_RUN_PATH_PROGRAM    = 15    ; 运行程序，文件名可带路径：$FF 为盘号，X:Y 为以 0 结尾的路径加文件名（必须带扩展名）；返回同功能 5（Ember 实测）
DOS_GET_DOS_VERSION     = 16    ; 取 DOS 版本：A=3，X=0，C=0（反汇编，手册未列、Ember 实测）

;---------------------------------------; 文件调用功能号（FILE_ENTRY = $5803），通道块第 0 字节
FILE_RESET              = 0     ; 复位文件系统（手册）
FILE_OPEN               = 1     ; 打开文件：出口 FH_OPEN_ATTRIBUTE、FH_OPEN_DATE、FH_OPEN_TIME、FH_FILE_SIZE，读写位置清零；不存在时 A=$02（Ember 实测）
FILE_READ               = 2     ; 读文件：入口 FH_PAGE、FH_BUFFER、FH_BYTES、FH_POSITION；出口 FH_BYTES 为实际读入字节数，FH_BUFFER 为结束地址，FH_POSITION 前进（Ember 实测，可跨扇区）
FILE_WRITE              = 3     ; 写文件：FH_BYTES 为 0 表示在 FH_POSITION 处截断；出口同读；不检查只读属性（Ember 实测）
FILE_SEEK               = 4     ; 移动读写位置：入口 FH_POSITION（Ember 实测）
FILE_CLOSE              = 5     ; 关闭文件，把长度与日期写回目录项（Ember 实测）
FILE_FIRST              = 6     ; 查找第一个文件（反汇编、Ember 实测）
FILE_NEXT               = 7     ; 查找下一个文件，没有更多时 A=$12（反汇编、Ember 实测）
FILE_CREATE             = 8     ; 建立文件（Ember 实测）
FILE_DRIVE              = 9     ; 读取或设置默认驱动器：FH_DRIVE 为 0 时读取，出口 FH_DRIVE 为默认盘号、FH_DRIVE_COUNT 为逻辑驱动器数（Ember 实测）
FILE_DELETE             = 10    ; 删除文件，不检查只读属性（Ember 实测）
FILE_DRIVE_SIZE         = 11    ; 取驱动器容量：出口 FH_SECTORS_PER_CLUSTER、FH_BYTES_PER_SECTOR、FH_FREE_CLUSTERS、FH_TOTAL_CLUSTERS（Ember 实测）
FILE_ATTRIBUTE          = 12    ; 设置已打开文件的属性、日期、时间：入口 FH_OPEN_ATTRIBUTE、FH_OPEN_DATE、FH_OPEN_TIME，关闭后写入目录项（Ember 实测）
FILE_RENAME             = 13    ; 给已打开的文件改名：新文件名取自 FH_PAGE、FH_BUFFER 指向的 11 字节（Ember 实测）
FILE_READ_ABS           = 14    ; 按绝对扇区读：FH_POSITION 为起始扇区号（从 0 起），FH_BYTES 为扇区数（Ember 实测）
FILE_WRITE_ABS          = 15    ; 按绝对扇区写：参数同 FILE_READ_ABS（Ember 实测）
FILE_CHANGE_DIR         = 16    ; 改变当前目录：路径是以 0 结尾的字符串（Ember 实测）
FILE_MAKE_DIR           = 17    ; 建立目录：路径是以 0 结尾的字符串（Ember 实测）
FILE_REMOVE_DIR         = 18    ; 删除目录：路径不存在 A=$03（Ember 实测）；删除当前目录 A=$10（手册）
FILE_GET_DIR            = 19    ; 取当前目录：出口 FH_PAGE、FH_BUFFER 指向 DOS 页里以 0 结尾的路径串，根目录为空串（Ember 实测）
FILE_PATH_OPEN          = 20    ; 按路径打开文件，路径是以 0 结尾的字符串（Ember 实测）
FILE_PATH_CREATE        = 21    ; 按路径建立文件（Ember 实测）
FILE_PATH_FIRST         = 22    ; 按路径查找第一个文件，通配符 * 与 ?，FH_BYTES 低字节为属性掩码；结果区同 FILE_FIRST（Ember 实测）
FILE_PATH_DELETE        = 23    ; 按路径删除文件（Ember 实测）
FILE_SET_ATTRIBUTE      = 24    ; 手册未列：按块偏移 $16 的值改写目录项属性（反汇编）；对已打开文件调用返回 A=$05，用法未查清

;---------------------------------------; 文件通道块的布局（共 48 字节，FILE_ENTRY 的参数）
FH_COMMAND              = $00   ; 功能号
FH_DRIVE                = $01   ; 盘号：0 默认盘，1 为 A 盘，$FF 为启动盘（反汇编）
FH_PAGE                 = $02   ; 缓冲区所在页
FH_BUFFER               = $03   ; 缓冲区地址（2 字节，低字节在前）
FH_BYTES                = $05   ; 读写长度（2 字节）；FILE_FIRST 等功能里，$05 起的 11 字节是文件名
FH_NAME                 = $05   ; 8 字节主名加 3 字节扩展名，空格补齐，'?' 为通配符
FH_POSITION             = $07   ; 读写位置
FH_ATTRIBUTE            = $16   ; 查找时的属性掩码；FILE_FIRST 先把它置为 $FF
FH_DTA                  = $17   ; 查找结果区（FILE_FIRST、FILE_NEXT 返回）
FH_DTA_ATTRIBUTE        = $17   ; 找到的目录项属性
FH_DTA_TIME             = $18   ; 修改时间（2 字节）
FH_DTA_DATE             = $1A   ; 修改日期（2 字节）
FH_DTA_SIZE             = $1C   ; 文件大小（4 字节）
FH_DTA_NAME             = $20   ; 文件名 11 字节
FH_SEARCH_STATE         = $2B   ; 查找状态（页号、$9BF5 与当前目录簇，供 FILE_NEXT 使用）
FH_SIZE                 = 48    ; 通道块长度
FH_OPEN_ATTRIBUTE       = $02   ; FILE_OPEN、FILE_ATTRIBUTE：文件属性（与 FH_PAGE 同一单元，出口覆盖页号）
FH_OPEN_DATE            = $03   ; FILE_OPEN、FILE_ATTRIBUTE：日期（2 字节，位 15-9 年减 1980，位 8-5 月，位 4-0 日）
FH_OPEN_TIME            = $05   ; FILE_OPEN、FILE_ATTRIBUTE：时间（2 字节，位 15-11 时，位 10-5 分，位 4-0 秒除 2）
FH_FILE_CLUSTER         = $0B   ; 已打开文件的起始簇号（2 字节，DOS 内部使用）
FH_FILE_SIZE            = $0D   ; 已打开文件的长度（4 字节，写入时随之增长）
FH_DIR_ENTRY            = $11   ; 目录项在目录扇区里的偏移除以 2（DOS 内部使用）
FH_DIR_SECTOR           = $12   ; 目录项所在扇区号（DOS 内部使用）
FH_DRIVE_COUNT          = $02   ; FILE_DRIVE 出口：逻辑驱动器数
FH_SECTORS_PER_CLUSTER  = $03   ; FILE_DRIVE_SIZE 出口：每簇扇区数（2 字节）
FH_BYTES_PER_SECTOR     = $05   ; FILE_DRIVE_SIZE 出口：每扇区字节数（2 字节）
FH_FREE_CLUSTERS        = $07   ; FILE_DRIVE_SIZE 出口：空闲簇数（2 字节）
FH_TOTAL_CLUSTERS       = $09   ; FILE_DRIVE_SIZE 出口：磁盘总簇数（2 字节）

;---------------------------------------; 出错码（返回在 A）
ERR_OK                  = $00   ; 成功
ERR_COMMAND             = $01   ; 功能号错误
ERR_FILE_NOT_EXIST      = $02   ; 文件不存在
ERR_PATH_NOT_FOUND      = $03   ; 路径不存在
ERR_ACCESS_DENIED       = $05   ; 拒绝访问（含通配符建文件、与目录或卷标同名）
ERR_FORMAT              = $05   ; 格式错误（手册）
ERR_TOO_SMALL_DRAM      = $08   ; 内存不够
ERR_DRIVE_INVALID       = $0F   ; 驱动器号出错
ERR_REMOVE_CURRENT_DIR  = $10   ; 试图删除当前目录
ERR_SEARCH_ENDING       = $12   ; 没有更多文件
ERR_DRIVE_NOT_READY     = $15   ; 驱动器未准备好
ERR_CRC                 = $17   ; CRC 错误
ERR_UNKNOWN_MEDIA       = $1A   ; 未识别介质类型
ERR_SECTOR_NOT_FOUND    = $1B   ; 扇区未找到
ERR_READ                = $1E   ; 读文件错
ERR_GENERAL             = $1F   ; 一般错误
ERR_DISPLAY_MODE        = $28   ; 显示方式错误
ERR_TOO_SMALL_VRAM      = $29   ; 显存太小
ERR_FAT                 = $2A   ; FAT 表错误

;---------------------------------------; 显示方式（DOS_GET_SET_DISP_MODE）
DISP_MODE_ENGLISH       = $00   ; 英文方式，30 行
DISP_MODE_CHINESE       = $80   ; 中文方式，14 加 1 行

;---------------------------------------; 程序文件
RUN_COMMAND_LINE        = $0100 ; 运行程序的参数：$0100 为长度，$0101 起为文本
CMD_LOAD_ADDRESS_SIZE   = 2     ; CMD 文件前 2 字节是装入地址，之后的内容装入该地址并从该地址开始执行
