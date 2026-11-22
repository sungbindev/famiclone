; ------------------------------------------------------------------------
; 名称：tvdef.inc
; 说明：步步高 BIOS/DOS 源码使用的符号定义：系统内存地址、硬件端口与 BIOS 功能号常量。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：由 Bios.asm、Command.asm、Smcdos.asm 里的未定义符号反推，取值依据：
;       ①《步步高多媒体学生电脑 BIOS 系统 DOS 系统使用手册》系统篇第二章（BIOS 功能号）、
;         第四章（文件通道结构）；②源码里被注释掉的同名定义（固定地址）；
;       ③Bios.asm 的功能号分发表（BiosSetSEndVector）。
;       宏 orgrun、orgreal 由汇编器 gasm.mjs 内置实现：orgrun 只改标号的运行地址，
;       字节仍按顺序存放；orgreal 恢复为存放地址。
;       “推测”标注的符号手册与源码都没有给出取值，需要有人在真机或模拟器里核对。
; ------------------------------------------------------------------------

;---------------------------------------; 系统调用入口（手册：系统篇第一章 2.）
ROMBiosEntry = $5800	; BIOS 调用
FileIOEntry = $5803	; 磁盘文件调用
MonitorEntry = $5806	; 系统冷启动入口
HardResetEntry = $5806	;
DeviceEntry = $5809	; 外设调用
DosIOEntry = $580c	; DOS 调用
SoftResetEntry = $580f	; 系统热启动入口
CtrlBreakEntry = $5812	; Ctrl/Break 处理入口
CtrlCProcEntry = $5815	; Ctrl/C 处理入口（手册：JMP 指令，应用可改写）
FaultErrorEntry = $5818	; 严重错误处理入口（手册：第六章）
RunCommandLine = $0100	; 运行程序的参数：$0100 为长度，$0101 起为字符串

;---------------------------------------; 固定地址的 BIOS 变量（Bios.asm 注释里的定义）
Mouse1X = $5feb	;
Mouse1Y = $5fec	;
KeyStatus1 = $5fed	;
KeyStatus2 = $5fee	;
KeyStatus3 = $5fef	;
KeyStatus4 = $5ff0	;
Mouse0X = $5ff2	;
Mouse0Y = $5ff3	;
CurColumn = $5ff6	;
CurLine = $5ff7	;

;---------------------------------------; 文件通道结构（手册：系统篇第四章）
FILECMD = 0	; 文件操作命令（功能号）
CURDRIVE = 1	; 盘号
HPAGE = 2	; 缓冲区页号
HBUF = 3	; 缓冲区地址（2 字节）
BYTES = 5	; 读写长度（2 字节）
CURP = 7	; 读写位置指针
HandleSize = 48	; 文件通道长度（手册：不小于 48 字节）

;---------------------------------------; 文件调用功能号（手册：第四章 1.）
FileReset = 0
FileOpen = 1
FileRead = 2
FileWrite = 3
FileSeek = 4
FileClose = 5
FileFirst = 6
FileNext = 7
FileCreate = 8
FileDrive = 9	; 手册写作 FileDriver
FileDelete = 10
FileDriveSize = 11	; 手册写作 DriveSize
FileAttribute = 12
FileRename = 13
FileReadAbs = 14
FileWriteAbs = 15
FileChangeDir = 16
FileMakeDir = 17
FileRemoveDir = 18
FileGetDir = 19
FilePathOpen = 20
FilePathCreate = 21
FilePathFirst = 22
FilePathDelete = 23

;---------------------------------------; DOS 调用功能号（手册：第五章 1.）
DosProgramReturn = 0
DosReadKeyEcho = 1
DosDisplayChar = 2
DosInquirePages = 3
DosDisplayString = 4
DosRunProgram = 5
DosGetKBString = 6
DosReadKeyNoEcho = 7
DosReadKBClear = 8
DosInitialScreen = 9
DosReleasePages = 10
DosGetSetDispMode = 11
DosFileNameAccs = 12
DosGetSetInitScrn = 13	; 手册在 (13)/(14) 处编号有错位，按功能号表顺序取 13
DosCheckKeyBoard = 14
DosRunPathProgram = 15
DosGetDOSVersion = 16	; 推测：手册未列，Command.asm 用它核对 DOS 版本

HZDisplayMode = $80	; 中文显示方式（手册：DosGetSetDispMode 的 Y 参数）
ASDisplayMode = 0	; 英文显示方式

;---------------------------------------; 出错返回码（手册：第四章、第五章）
FileOK = $00
DosOK = $00
ErrorCommand = $01
FileNotExist = $02
PathNotFound = $03
AccessDenied = $05
FormatError = $05
TooSmallDRAM = $08	; 手册写作 TooSmallDRAM（OCR 有误）
DriveInvalid = $0f
RemoveCurtDir = $10	; 试图删除当前目录
SearchEnding = $12	; 没有更多文件
DriveNotReady = $15
CRCError = $17
UnknownMType = $1a
SectorNoFound = $1b
ErrRead = $1e
GeneralError = $1f
DispModeError = $28	; 手册写作 DisplayModeError
TooSmallVRAM = $29
FatError = $2a
StackTooDepth = $2b	; 推测：程序驻留层数太多，手册未列

;---------------------------------------; BIOS 功能号（手册：系统篇第二章 1.）
BiosSetPage = 1
BiosGetPage = 2
BiosSetAV10 = 3	; 手册未列，由 ROMBios 的分发表推出
BiosRunInitial = 4
BiosLowRAMFree = 5
BiosDiskIO = 6
BiosGetDRAMSize = 7
BiosReadKey = 11
BiosMoveBlock = 12
BiosWaitVSynCnt = 14
BiosLowRAMAlloc = 15
BiosExchBlock = 17
BiosSplitScreen = 18	; 手册未列，由 ROMBios 的分发表推出
BiosGetDriveMsg = 20
BiosSetNMIVector = 22
BiosGetNMIVector = 23
BiosDevice = 24
BiosDisplayTTY = 27
BiosSetCursor = 28
BiosDisplayChars = 29
BiosGetCursor = 30
BiosGetPagedChar = 32
BiosSetPagedChar = 33
BiosWaitDisplay = 34
BiosUnzipBlock = 37
BiosSetSEndVector = 39	; 手册未列，由 ROMBios 的分发表推出
BiosDispChAtTTY = 40
BiosDispCharsAt = 41
BiosScreenScroll = 42
BiosSetPalette = 43
BiosDisplayChAt = 44
BiosDispRefChar = 45
BiosDispFGCharAt = 47
BiosSetCursorType = 48
BiosGetSetExtFont = 49
BiosDispStringAt = 50
BiosGetSystemFont = 51
BiosSetScreenSW = 52	; Smcdos.asm 的注释标为 $34
BiosMouseOperate = 53
BiosSetBGFont = 54
BiosGetBGFont = 55
BiosSetFGFont = 56
BiosGetFGFont = 57
BiosDispHalfChAt = 58
BiosMoveFGChar = 59
BiosDispInitial = 60
BiosDispOpenClose = 61
BiosChgBGColor = 62
BiosGetDateTime = 63
BiosSetSystemDate = 64
BiosSetSystemTime = 65
BiosDispGraphics = 66
BiosGetSplitePoint = 67
BiosSetLineWidth = 68
BiosSetScreenPan = 69
BiosGetVersion = 255
