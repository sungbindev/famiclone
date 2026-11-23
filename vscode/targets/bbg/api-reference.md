# 步步高汇编运行库接口参考

本文由 `vscode/tools/gen/gen-bbg-api-doc.mjs` 从运行库源码（`src/lib`）里每个函数的说明注释生成，不手工修改；改了注释或 `api-unwrapped.json` 后重新生成。符号与功能号见 `asminc/bbg.inc`（由 `machines/bbg/tools/gen-bbg-inc.mjs` 生成）。

## 调用约定

- 汇编程序用 `.include "bbg.inc"` 引入符号，`JSR` 调用库函数；库放在 `bbg.lib`，链接时只带入用到的函数。
- 一个字节的参数放 A；一个 16 位参数（地址、数值）低字节放 A、高字节放 X；再多的参数写进零页单元 `bbg_arg0`、`bbg_arg1`（各 2 字节，低字节在前）；盘号等第二个字节参数放 Y。
- 返回值：一个字节在 A；16 位在 A（低）、X（高）；更多的返回值在 `bbg_arg0`、`bbg_arg1`。DOS 与文件调用的封装保持系统的约定：C=0 成功，C=1 出错且 A 为出错码（文件调用成功时 A=0）。
- 不保留 A、X、Y；零页只使用链接配置划给程序的 $0020-$007F 里的 `bbg_arg0`、`bbg_arg1` 与库内部几个单元（`bbgi_` 开头）。
- 文件调用的参数在 48 字节（`FH_SIZE`）的文件通道块里，由调用者提供；`bbg_file_clear` 清零并设置缓冲区页，`bbg_file_set_*` 与 `bbg_file_get_*` 读写其中的字段，其余字段可直接按 `FH_` 偏移读写。

## BIOS 调用（BIOS_ENTRY，$5800）

### bbg_bios_dram_size

BIOS 调用 7：取内存页数。

- 入口：无
- 出口：A、X = 最后一页的页号加 1

### bbg_bios_get_cursor

BIOS 调用 30：取光标所在列。

- 入口：无
- 出口：Y = 光标列

### bbg_bios_get_page

BIOS 调用 2：取 $8000-$BFFF 窗口当前的页号。

- 入口：无
- 出口：A = 页号；系统默认页返回 $FE

### bbg_bios_put_char

BIOS 调用 27：以电传方式显示一个字符。

- 入口：A = 字符码
- 出口：无

### bbg_bios_version

BIOS 调用 255：取 BIOS 版本。

- 入口：无
- 出口：A=$40，X=$61，Y=1（bbg1 的 BIOS 1.0）

## DOS 调用（DOS_ENTRY，$580C）

### bbg_dos_clear_screen

DOS 调用 9：初始化屏幕并清屏。

- 入口：无
- 出口：无

### bbg_dos_convert_name

DOS 调用 12：把 "D:FFFFFFFF.XXX" 形式、以 0 结尾的文件名转换成 11 字节的内部格式，结果写回原地址。

- 入口：A（低）、X（高）= 文件名地址，缓冲区至少 12 字节
- 出口：A = 盘号（0 默认盘，1 为 A 盘），盘符非法时为 $0F；缓冲区前 11 字节为主名 8 字节加扩展名 3 字节，空格补齐，* 展开成 ?

### bbg_dos_display_mode_get

DOS 调用 11：取显示方式。

- 入口：无
- 出口：A = DISP_MODE_ENGLISH（$00）或 DISP_MODE_CHINESE（$80）

### bbg_dos_display_mode_set

DOS 调用 11：设置显示方式。

- 入口：A = DISP_MODE_ENGLISH 或 DISP_MODE_CHINESE
- 出口：无

### bbg_dos_exit

DOS 调用 0：程序结束，回到命令行，不返回。

- 入口：A = 返回码
- 出口：不返回

### bbg_dos_get_line

DOS 调用 6：输入一行并回显。

- 入口：A（低）、X（高）= 缓冲区地址，缓冲区第 0 字节为最大字符数
- 出口：缓冲区第 1 字节为实际输入的字符数，文本从第 2 字节起

### bbg_dos_key_ready

DOS 调用 14：测试键盘，不等待。

- 入口：无
- 出口：Z=1 没有按键；Z=0 有按键，A 为字符码

### bbg_dos_pages_alloc

DOS 调用 3：申请内存页，页数为 0 时只查询。

- 入口：A = 页数（0 表示查询空闲页号）
- 出口：C=0 成功，A 为起始页号；C=1 出错，A 为最多可申请的页数

### bbg_dos_pages_free

DOS 调用 10：释放内存页，从起始页起的页全部释放。

- 入口：A = 起始页号
- 出口：无

### bbg_dos_put_char

DOS 调用 2：显示一个字符，$0D、$0A 换行。

- 入口：A = 字符码
- 出口：无

### bbg_dos_put_string

DOS 调用 4：显示以 '$' 结尾的字符串。

- 入口：A（低）、X（高）= 字符串地址
- 出口：无
- 备注：地址在 DOS 里按 X 为高字节、Y 为低字节传入，与手册的文字相反。

### bbg_dos_read_key

DOS 调用 7：等待按键，不回显。

- 入口：无
- 出口：A = 字符码，0 表示功能键

### bbg_dos_read_key_clear

DOS 调用 8：先清空键盘缓冲区，再执行指定的键盘功能。

- 入口：A = 要执行的 DOS 功能号（1、6 或 7）；功能 6 时 X 为缓冲区地址高字节、Y 为低字节，缓冲区格式同 bbg_dos_get_line
- 出口：同所执行的功能
- 备注：功能号经零页 $FF 传给 DOS，$FF 属 DOS 的零页区，库不另行保存。

### bbg_dos_read_key_echo

DOS 调用 1：等待按键并回显。

- 入口：无
- 出口：A = 字符码，0 表示功能键

### bbg_dos_run

DOS 调用 5：运行一个程序（11 字节文件名的扩展名须为 CMD、EXC 或 GAM），返回时已回到本程序。

- 入口：A（低）、X（高）= 11 字节文件名地址（主名 8 加扩展名 3，空格补齐，扩展名必须是 CMD、EXC 或 GAM）；Y = 程序所在盘号（0 默认盘，1 为 A 盘）；传给程序的参数先写在 $0100（长度）与 $0101 起（文本）
- 出口：C=0 成功，A 为被运行程序的退出码；扩展名不是 CMD、EXC、GAM 时 C=1（实测 Y=$0B）
- 备注：嵌套层数有限（手册写最多 4 层，BBGDOS 代码上限 8 层，未实测）；DOS 返回后寄存器内容被破坏；盘号经零页 $FF 传给 DOS，$FF 属 DOS 的零页区。

### bbg_dos_run_path

DOS 调用 15：运行一个程序，文件名可以带路径。

- 入口：A（低）、X（高）= 以 0 结尾的路径与文件名地址（目录之间用 \ 分隔，文件名必须带扩展名 CMD、EXC 或 GAM）；Y = 程序所在盘号（0 默认盘，1 为 A 盘）；传给程序的参数先写在 $0100（长度）与 $0101 起（文本）
- 出口：同 bbg_dos_run
- 备注：嵌套层数有限（手册写最多 4 层，BBGDOS 代码上限 8 层，未实测）；DOS 返回后寄存器内容被破坏；盘号经零页 $FF 传给 DOS，$FF 属 DOS 的零页区。

### bbg_dos_version

DOS 调用 16：取 DOS 版本。

- 入口：无
- 出口：A=3，X=0，C=0（BBGDOS 3.0）

## 文件调用（FILE_ENTRY，$5803）

### bbg_file_change_dir

文件调用 16：改变当前目录。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向以 0 结尾的路径串，目录之间用 \ 分隔
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码（路径不存在为 ERR_PATH_NOT_FOUND）

### bbg_file_clear

清空文件通道块，并把缓冲区页设成本程序所在页。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
- 出口：块全部清零，FH_PAGE 为当前页，FH_DRIVE 为 0（默认盘）

### bbg_file_close

文件调用 5：关闭文件，把长度与日期写回目录项。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_create

文件调用 8：在当前目录建立文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向 11 字节文件名
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；成功时文件已打开，长度为 0

### bbg_file_delete

文件调用 10：删除文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向 11 字节文件名
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码（不存在为 ERR_FILE_NOT_EXIST）

### bbg_file_drive

文件调用 9：读取或设置默认驱动器。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为 0 表示读取，否则为要设置的盘号（1 为 A 盘）
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；读取时 FH_DRIVE 为默认盘号，FH_DRIVE_COUNT 为逻辑驱动器数

### bbg_file_drive_size

文件调用 11：取驱动器容量。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_SECTORS_PER_CLUSTER、FH_BYTES_PER_SECTOR、FH_FREE_CLUSTERS、FH_TOTAL_CLUSTERS 为容量信息

### bbg_file_find_first

文件调用 6：在当前目录查找第一个文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向 11 字节文件名（? 为通配符）
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码（没有文件为 ERR_SEARCH_ENDING）；成功时 FH_DTA_ATTRIBUTE、FH_DTA_TIME、FH_DTA_DATE、FH_DTA_SIZE、FH_DTA_NAME 为找到的文件

### bbg_file_find_next

文件调用 7：继续查找下一个文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块沿用 bbg_file_find_first 的结果，不能改动
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码（没有更多文件为 ERR_SEARCH_ENDING）；结果区同 bbg_file_find_first

### bbg_file_get_dir

文件调用 19：取当前目录。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_PAGE、FH_BUFFER 指向 DOS 页里以 0 结尾的路径串（根目录为空串），内容不能改写

### bbg_file_get_length

读出通道块里的读写长度（FH_BYTES）。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
- 出口：A（低）、X（高）= FH_BYTES

### bbg_file_get_position

读出通道块里的读写位置（FH_POSITION，4 字节）。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
- 出口：A（低）、X（高）= 位置的低 16 位；bbg_arg0 = 位置的高 16 位

### bbg_file_get_size

读出已打开文件的长度（FH_FILE_SIZE，4 字节）。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）
- 出口：A（低）、X（高）= 长度的低 16 位；bbg_arg0 = 长度的高 16 位

### bbg_file_make_dir

文件调用 17：建立目录。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；参数同 bbg_file_change_dir
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_open

文件调用 1：打开文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号（0 默认盘），FH_PAGE、FH_BUFFER 指向 11 字节文件名（主名 8 加扩展名 3，大写，空格补齐）
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码（不存在为 ERR_FILE_NOT_EXIST）；成功时块里 FH_OPEN_ATTRIBUTE、FH_OPEN_DATE、FH_OPEN_TIME、FH_FILE_SIZE 为文件的属性、日期、时间与长度，读写位置为 0

### bbg_file_path_create

文件调用 21：按路径建立文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；参数同 bbg_file_path_open
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_path_delete

文件调用 23：按路径删除文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；参数同 bbg_file_path_open
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_path_find_first

文件调用 22：按路径查找第一个文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向以 0 结尾的路径与文件名（可带 * 与 ?），FH_BYTES 低字节为属性掩码（0 只找一般文件）
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；结果区同 bbg_file_find_first；之后用 bbg_file_find_next 继续

### bbg_file_path_open

文件调用 20：按路径打开文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 指向以 0 结尾的路径与文件名（如 SUB\A.TXT）
- 出口：同 bbg_file_open，另有 ERR_PATH_NOT_FOUND

### bbg_file_read

文件调用 2：从读写位置读文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_PAGE、FH_BUFFER 为缓冲区，FH_BYTES 为读入字节数，FH_POSITION 为读写位置
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_BYTES 为实际读入字节数（到文件末尾时少于请求），FH_BUFFER 为缓冲区结束地址，FH_POSITION 前进

### bbg_file_read_sectors

文件调用 14：按绝对扇区号读扇区。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号，FH_PAGE、FH_BUFFER 为缓冲区，FH_BYTES 为扇区数，FH_POSITION 为起始扇区号（从 0 起）
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_remove_dir

文件调用 18：删除目录。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；参数同 bbg_file_change_dir
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_rename

文件调用 13：给已打开的文件改名。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_PAGE、FH_BUFFER 指向 11 字节新文件名
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_reset

文件调用 0：复位驱动器。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_DRIVE 为盘号
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_seek

文件调用 4：修改读写位置。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_POSITION 为新位置
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

### bbg_file_set_attribute

文件调用 12：设置已打开文件的属性、日期与时间。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_OPEN_ATTRIBUTE、FH_OPEN_DATE、FH_OPEN_TIME 为新值
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；关闭文件后写入目录项

### bbg_file_set_buffer

设置通道块里的缓冲区（页取当前页）。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；bbg_arg0 = 缓冲区地址或文件名地址
- 出口：无

### bbg_file_set_drive

设置通道块里的盘号。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；Y = 盘号（0 默认盘，1 为 A 盘）
- 出口：无

### bbg_file_set_length

设置通道块里的读写长度（FH_BYTES）。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；bbg_arg0 = 字节数（读写扇区时为扇区数）
- 出口：无

### bbg_file_set_position

设置通道块里的读写位置（FH_POSITION，4 字节）。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；bbg_arg0 = 位置的低 16 位，bbg_arg1 = 位置的高 16 位
- 出口：无

### bbg_file_write

文件调用 3：从读写位置写文件。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；块里 FH_PAGE、FH_BUFFER 为数据，FH_BYTES 为写入字节数（0 表示在读写位置截断文件），FH_POSITION 为读写位置
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码；FH_BYTES 为实际写入字节数，FH_POSITION 前进

### bbg_file_write_sectors

文件调用 15：按绝对扇区号写扇区。

- 入口：A（低）、X（高）= 文件通道块地址（FH_SIZE 字节，由调用者提供）；参数同 bbg_file_read_sectors
- 出口：C=0 成功，A=0；C=1 出错，A 为出错码

## 文字输出

### bbg_text_put_cstr

显示以 0 结尾的字符串。

- 入口：A（低）、X（高）= 字符串地址
- 出口：无
- 备注：逐个字符调用 DOS 的显示字符功能，$0D、$0A 会换行；不保留 A、X、Y。

### bbg_text_put_dec16

以十进制显示一个无符号 16 位数，不补前导零，0 显示为 "0"。

- 入口：A（低）、X（高）= 数值
- 出口：无
- 备注：用 10000、1000、100、10、1 逐位相减；每显示一位都要调用 DOS，会破坏 X、Y，所以位置与数值放在零页。

### bbg_text_put_hex16

以四位大写十六进制显示一个 16 位数。

- 入口：A（低）、X（高）= 数值
- 出口：无
- 备注：先显示高字节再显示低字节；不保留 A、X、Y。

### bbg_text_put_hex8

以两位大写十六进制显示一个字节。

- 入口：A = 字节
- 出口：无
- 备注：逐个字符调用 DOS 的显示字符功能；不保留 A、X、Y。

### bbg_text_put_newline

显示回车与换行。

- 入口：无
- 出口：无
- 备注：调用 DOS 的显示字符功能依次显示 $0D、$0A；不保留 A、X、Y。

## 功能号覆盖表

每个功能号被哪些库函数封装；没有封装的写明原因。BIOS 只列 bbg1 的 BIOS 1.0 实现的功能号。

### DOS 调用

| 功能号 | 符号 | 封装 |
| --- | --- | --- |
| 0 | `DOS_PROGRAM_RETURN` | `bbg_dos_exit` |
| 1 | `DOS_READ_KEY_ECHO` | `bbg_dos_read_key_echo` |
| 2 | `DOS_DISPLAY_CHAR` | `bbg_dos_put_char`、`bbg_text_put_cstr`、`bbg_text_put_dec16`、`bbg_text_put_hex8`、`bbg_text_put_newline` |
| 3 | `DOS_INQUIRE_PAGES` | `bbg_dos_pages_alloc` |
| 4 | `DOS_DISPLAY_STRING` | `bbg_dos_put_string` |
| 5 | `DOS_RUN_PROGRAM` | `bbg_dos_run` |
| 6 | `DOS_GET_KB_STRING` | `bbg_dos_get_line` |
| 7 | `DOS_READ_KEY_NO_ECHO` | `bbg_dos_read_key` |
| 8 | `DOS_READ_KB_CLEAR` | `bbg_dos_read_key_clear` |
| 9 | `DOS_INITIAL_SCREEN` | `bbg_dos_clear_screen` |
| 10 | `DOS_RELEASE_PAGES` | `bbg_dos_pages_free` |
| 11 | `DOS_GET_SET_DISP_MODE` | `bbg_dos_display_mode_get`、`bbg_dos_display_mode_set` |
| 12 | `DOS_FILE_NAME_ACCESS` | `bbg_dos_convert_name` |
| 13 | `DOS_GET_SET_INIT_SCREEN` | 未封装：取并设置初始化显示屏幕处理向量，只供 DOS 自己的显示初始化使用，新向量要放在指定页且地址减 1，没有用例，没有验证。 |
| 14 | `DOS_CHECK_KEYBOARD` | `bbg_dos_key_ready` |
| 15 | `DOS_RUN_PATH_PROGRAM` | `bbg_dos_run_path` |
| 16 | `DOS_GET_DOS_VERSION` | `bbg_dos_version` |

### 文件调用

| 功能号 | 符号 | 封装 |
| --- | --- | --- |
| 0 | `FILE_RESET` | `bbg_file_reset` |
| 1 | `FILE_OPEN` | `bbg_file_open` |
| 2 | `FILE_READ` | `bbg_file_read` |
| 3 | `FILE_WRITE` | `bbg_file_write` |
| 4 | `FILE_SEEK` | `bbg_file_seek` |
| 5 | `FILE_CLOSE` | `bbg_file_close` |
| 6 | `FILE_FIRST` | `bbg_file_find_first` |
| 7 | `FILE_NEXT` | `bbg_file_find_next` |
| 8 | `FILE_CREATE` | `bbg_file_create` |
| 9 | `FILE_DRIVE` | `bbg_file_drive` |
| 10 | `FILE_DELETE` | `bbg_file_delete` |
| 11 | `FILE_DRIVE_SIZE` | `bbg_file_drive_size` |
| 12 | `FILE_ATTRIBUTE` | `bbg_file_set_attribute` |
| 13 | `FILE_RENAME` | `bbg_file_rename` |
| 14 | `FILE_READ_ABS` | `bbg_file_read_sectors` |
| 15 | `FILE_WRITE_ABS` | `bbg_file_write_sectors` |
| 16 | `FILE_CHANGE_DIR` | `bbg_file_change_dir` |
| 17 | `FILE_MAKE_DIR` | `bbg_file_make_dir` |
| 18 | `FILE_REMOVE_DIR` | `bbg_file_remove_dir` |
| 19 | `FILE_GET_DIR` | `bbg_file_get_dir` |
| 20 | `FILE_PATH_OPEN` | `bbg_file_path_open` |
| 21 | `FILE_PATH_CREATE` | `bbg_file_path_create` |
| 22 | `FILE_PATH_FIRST` | `bbg_file_path_find_first` |
| 23 | `FILE_PATH_DELETE` | `bbg_file_path_delete` |
| 24 | `FILE_SET_ATTRIBUTE` | 未封装：手册没有列出这个功能；对已打开的文件调用返回 ERR_ACCESS_DENIED（$05），用法没有查清。 |

### BIOS 调用

| 功能号 | 符号 | 封装 |
| --- | --- | --- |
| 0 | `BIOS_NO_OPERATION` | 未封装：空操作，没有封装的意义。 |
| 1 | `BIOS_SET_PAGE` | 未封装：改变 $8000-$BFFF 窗口的页：程序自己就在这个窗口里运行，换走自己所在的页会使程序消失，只能由放在低端内存的代码调用，没有通用的封装。 |
| 2 | `BIOS_GET_PAGE` | `bbg_bios_get_page`、`bbg_file_clear`、`bbg_file_set_buffer` |
| 3 | `BIOS_SET_AV10` | 未封装：设置显存 AV10 连接方式，直接写系统控制寄存器映像，没有验证。 |
| 4 | `BIOS_RUN_INITIAL` | 未封装：重新初始化系统资源，会清掉 DOS 设置的向量，程序里用不到。 |
| 5 | `BIOS_LOW_RAM_FREE` | 未封装：取低端内存（$5800-$5FFF 区）的剩余量，参数与返回值只有手册的说明，没有验证。 |
| 6 | `BIOS_DISK_IO` | 未封装：磁盘 I/O 块直接读写扇区，绕过 DOS 的文件系统；需要读写扇区时用 bbg_file_read_sectors 与 bbg_file_write_sectors。 |
| 7 | `BIOS_GET_DRAM_SIZE` | `bbg_bios_dram_size` |
| 8 | `BIOS_FN_8` | 未封装：手册没有列出，只是读写变量 $5FF9，用途没有查清。 |
| 9 | `BIOS_FN_9` | 未封装：手册没有列出，没有核对。 |
| 10 | `BIOS_FN_10` | 未封装：手册没有列出，只读变量 $5FF5，用途没有查清。 |
| 11 | `BIOS_READ_KEY` | 未封装：读键盘与手柄，手册的参数说明不完整，没有验证；读键盘用 bbg_dos_read_key 与 bbg_dos_key_ready。 |
| 12 | `BIOS_MOVE_BLOCK` | 未封装：在页之间复制内存块，块描述放在零页，要写在 DOS 使用的零页区，没有通用的封装，没有验证。 |
| 13 | `BIOS_FN_13` | 未封装：手册没有列出，向 16 字节的块发送命令，用途没有查清。 |
| 14 | `BIOS_WAIT_VSYNC_COUNT` | 未封装：等待若干个垂直同步，参数没有验证。 |
| 15 | `BIOS_LOW_RAM_ALLOC` | 未封装：申请低端内存是 DOS 启动时的动作，程序用不到，没有验证。 |
| 16 | `BIOS_FN_16` | 未封装：手册没有列出，没有核对。 |
| 17 | `BIOS_EXCHANGE_BLOCK` | 未封装：在页之间交换内存块，块描述放在零页，没有通用的封装，没有验证。 |
| 18 | `BIOS_SPLIT_SCREEN` | 未封装：屏幕分割，bbg1 的 BIOS 1.0 里的行为没有验证。 |
| 19 | `BIOS_FN_19` | 未封装：手册没有列出，没有核对。 |
| 20 | `BIOS_GET_DRIVE_MESSAGE` | 未封装：取驱动器信息，参数没有验证。 |
| 21 | `BIOS_FN_21` | 未封装：手册没有列出，没有核对。 |
| 22 | `BIOS_SET_NMI_VECTOR` | 未封装：设置 NMI 后续处理向量，会影响 DOS 的屏幕刷新，没有验证。 |
| 23 | `BIOS_GET_NMI_VECTOR` | 未封装：取 NMI 后续处理向量，没有验证。 |
| 24 | `BIOS_DEVICE` | 未封装：外设调用，子功能没有分析；与 DEVICE_ENTRY（$5809）功能相同。 |
| 25 | `BIOS_FN_25` | 未封装：手册没有列出，没有核对。 |
| 26 | `BIOS_FN_26` | 未封装：手册没有列出，只返回 0。 |
| 27 | `BIOS_DISPLAY_TTY` | `bbg_bios_put_char` |
| 28 | `BIOS_SET_CURSOR` | 未封装：设置光标位置，参数只有手册的说明，没有验证。 |
| 29 | `BIOS_DISPLAY_CHARS` | 未封装：显示若干个相同字符，没有验证；可用 bbg_bios_put_char 或 bbg_dos_put_char 循环。 |
| 30 | `BIOS_GET_CURSOR` | `bbg_bios_get_cursor` |
| 31 | `BIOS_FN_31` | 未封装：手册没有列出，没有核对。 |
| 32 | `BIOS_GET_PAGED_CHAR` | 未封装：读其他页的一个字节，页号与地址放在零页，没有验证。 |
| 33 | `BIOS_SET_PAGED_CHAR` | 未封装：写其他页的一个字节，页号、值与地址放在零页，没有验证。 |
| 34 | `BIOS_WAIT_DISPLAY` | 未封装：等待显示完成，没有验证。 |
| 35 | `BIOS_FN_35` | 未封装：手册没有列出，没有核对。 |
| 36 | `BIOS_FN_36` | 未封装：手册没有列出，没有核对。 |
| 37 | `BIOS_UNZIP_BLOCK` | 未封装：解压内存块，参数没有验证。 |
| 38 | `BIOS_FN_38` | 未封装：手册没有列出，没有核对。 |
| 39 | `BIOS_SET_END_VECTOR` | 未封装：设置 NMI 结束时调用的处理，用途没有查清，没有验证。 |
| 40 | `BIOS_GET_DATE_TIME` | 未封装：取日期时间，bbg1 的 BIOS 1.0 只读软件变量，没有实时钟，日期时间由 DOS 启动时询问；零页返回值没有验证。 |
| 41 | `BIOS_SET_SYSTEM_DATE` | 未封装：设置日期，参数没有验证。 |
| 42 | `BIOS_SET_SYSTEM_TIME` | 未封装：设置时间，参数没有验证。 |
| 60 | `BIOS_DISPLAY_INITIAL` | 未封装：显示初始化，只供 DOS 内部使用，会破坏屏幕状态。 |
| 63 | `BIOS_GET_DATE_TIME_63` | 未封装：与功能 40 相同。 |
| 64 | `BIOS_SET_SYSTEM_DATE_64` | 未封装：与功能 41 相同。 |
| 65 | `BIOS_SET_SYSTEM_TIME_65` | 未封装：与功能 42 相同。 |
| 255 | `BIOS_GET_VERSION` | `bbg_bios_version` |
