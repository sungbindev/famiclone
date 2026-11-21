; ------------------------------------------------------------------------
; 名称：tvdef.asm
; 说明：步步高 BIOS/DOS 源码缺失的符号定义文件（原厂 tvdef.asm 的还原版）。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-12
; 备注：由 Bios.asm、Command.asm、Smcdos.asm 里的未定义符号反推，取值依据：
;       ①《步步高多媒体学生电脑 BIOS 系统 DOS 系统使用手册》系统篇第二章（BIOS 功能号）、
;         第四章（文件通道结构）；②源码里被注释掉的同名定义（固定地址）；
;       ③Bios.asm 的功能号分发表（BiosSetSEndVector）。
;       宏 orgrun、orgreal 由汇编器 gasm.mjs 内置实现：orgrun 只改标号的运行地址，
;       字节仍按顺序存放；orgreal 恢复为存放地址。
;       “推测”标注的符号手册与源码都没有给出取值，需要有人在真机或模拟器里核对。
; ------------------------------------------------------------------------

;---------------------------------------; 系统调用入口（手册：系统篇第一章 2.）
ROMBiosEntry    equ     $5800           ; BIOS 调用
FileIOEntry     equ     $5803           ; 磁盘文件调用
MonitorEntry    equ     $5806           ; 系统冷启动入口
HardResetEntry  equ     $5806           ;
DeviceEntry     equ     $5809           ; 外设调用
DosIOEntry      equ     $580c           ; DOS 调用
SoftResetEntry  equ     $580f           ; 系统热启动入口
CtrlBreakEntry  equ     $5812           ; Ctrl/Break 处理入口
CtrlCProcEntry  equ     $5815           ; Ctrl/C 处理入口（手册：JMP 指令，应用可改写）
FaultErrorEntry equ     $5818           ; 严重错误处理入口（手册：第六章）
RunCommandLine  equ     $0100           ; 运行程序的参数：$0100 为长度，$0101 起为字符串

;---------------------------------------; 固定地址的 BIOS 变量（Bios.asm 注释里的定义）
Mouse1X         equ     $5feb           ;
Mouse1Y         equ     $5fec           ;
KeyStatus1      equ     $5fed           ;
KeyStatus2      equ     $5fee           ;
KeyStatus3      equ     $5fef           ;
KeyStatus4      equ     $5ff0           ;
Mouse0X         equ     $5ff2           ;
Mouse0Y         equ     $5ff3           ;
CurColumn       equ     $5ff6           ;
CurLine         equ     $5ff7           ;

;---------------------------------------; 文件通道结构（手册：系统篇第四章）
FILECMD         equ     0               ; 文件操作命令（功能号）
CURDRIVE        equ     1               ; 盘号
HPAGE           equ     2               ; 缓冲区页号
HBUF            equ     3               ; 缓冲区地址（2 字节）
BYTES           equ     5               ; 读写长度（2 字节）
CURP            equ     7               ; 读写位置指针
HandleSize      equ     48              ; 文件通道长度（手册：不小于 48 字节）

;---------------------------------------; 文件调用功能号（手册：第四章 1.）
FileReset       equ     0
FileOpen        equ     1
FileRead        equ     2
FileWrite       equ     3
FileSeek        equ     4
FileClose       equ     5
FileFirst       equ     6
FileNext        equ     7
FileCreate      equ     8
FileDrive       equ     9               ; 手册写作 FileDriver
FileDelete      equ     10
FileDriveSize   equ     11              ; 手册写作 DriveSize
FileAttribute   equ     12
FileRename      equ     13
FileReadAbs     equ     14
FileWriteAbs    equ     15
FileChangeDir   equ     16
FileMakeDir     equ     17
FileRemoveDir   equ     18
FileGetDir      equ     19
FilePathOpen    equ     20
FilePathCreate  equ     21
FilePathFirst   equ     22
FilePathDelete  equ     23

;---------------------------------------; DOS 调用功能号（手册：第五章 1.）
DosProgramReturn    equ     0
DosReadKeyEcho      equ     1
DosDisplayChar      equ     2
DosInquirePages     equ     3
DosDisplayString    equ     4
DosRunProgram       equ     5
DosGetKBString      equ     6
DosReadKeyNoEcho    equ     7
DosReadKBClear      equ     8
DosInitialScreen    equ     9
DosReleasePages     equ     10
DosGetSetDispMode   equ     11
DosFileNameAccs     equ     12
DosGetSetInitScrn   equ     13          ; 手册在 (13)/(14) 处编号有错位，按功能号表顺序取 13
DosCheckKeyBoard    equ     14
DosRunPathProgram   equ     15
DosGetDOSVersion    equ     16          ; 推测：手册未列，Command.asm 用它核对 DOS 版本

HZDisplayMode       equ     $80         ; 中文显示方式（手册：DosGetSetDispMode 的 Y 参数）
ASDisplayMode       equ     0           ; 英文显示方式

;---------------------------------------; 出错返回码（手册：第四章、第五章）
FileOK          equ     $00
DosOK           equ     $00
ErrorCommand    equ     $01
FileNotExist    equ     $02
PathNotFound    equ     $03
AccessDenied    equ     $05
FormatError     equ     $05
TooSmallDRAM    equ     $08             ; 手册写作 TooSmallDRAM（OCR 有误）
DriveInvalid    equ     $0f
RemoveCurtDir   equ     $10             ; 试图删除当前目录
SearchEnding    equ     $12             ; 没有更多文件
DriveNotReady   equ     $15
CRCError        equ     $17
UnknownMType    equ     $1a
SectorNoFound   equ     $1b
ErrRead         equ     $1e
GeneralError    equ     $1f
DispModeError   equ     $28             ; 手册写作 DisplayModeError
TooSmallVRAM    equ     $29
FatError        equ     $2a
StackTooDepth   equ     $2b             ; 推测：程序驻留层数太多，手册未列

;---------------------------------------; BIOS 功能号（手册：系统篇第二章 1.）
BiosSetPage         equ     1
BiosGetPage         equ     2
BiosSetAV10         equ     3           ; 手册未列，由 ROMBios 的分发表推出
BiosRunInitial      equ     4
BiosLowRAMFree      equ     5
BiosDiskIO          equ     6
BiosGetDRAMSize     equ     7
BiosReadKey         equ     11
BiosMoveBlock       equ     12
BiosWaitVSynCnt     equ     14
BiosLowRAMAlloc     equ     15
BiosExchBlock       equ     17
BiosSplitScreen     equ     18          ; 手册未列，由 ROMBios 的分发表推出
BiosGetDriveMsg     equ     20
BiosSetNMIVector    equ     22
BiosGetNMIVector    equ     23
BiosDevice          equ     24
BiosDisplayTTY      equ     27
BiosSetCursor       equ     28
BiosDisplayChars    equ     29
BiosGetCursor       equ     30
BiosGetPagedChar    equ     32
BiosSetPagedChar    equ     33
BiosWaitDisplay     equ     34
BiosUnzipBlock      equ     37
BiosSetSEndVector   equ     39          ; 手册未列，由 ROMBios 的分发表推出
BiosDispChAtTTY     equ     40
BiosDispCharsAt     equ     41
BiosScreenScroll    equ     42
BiosSetPalette      equ     43
BiosDisplayChAt     equ     44
BiosDispRefChar     equ     45
BiosDispFGCharAt    equ     47
BiosSetCursorType   equ     48
BiosGetSetExtFont   equ     49
BiosDispStringAt    equ     50
BiosGetSystemFont   equ     51
BiosSetScreenSW     equ     52          ; Smcdos.asm 的注释标为 $34
BiosMouseOperate    equ     53
BiosSetBGFont       equ     54
BiosGetBGFont       equ     55
BiosSetFGFont       equ     56
BiosGetFGFont       equ     57
BiosDispHalfChAt    equ     58
BiosMoveFGChar      equ     59
BiosDispInitial     equ     60
BiosDispOpenClose   equ     61
BiosChgBGColor      equ     62
BiosGetDateTime     equ     63
BiosSetSystemDate   equ     64
BiosSetSystemTime   equ     65
BiosDispGraphics    equ     66
BiosGetSplitePoint  equ     67
BiosSetLineWidth    equ     68
BiosSetScreenPan    equ     69
BiosGetVersion      equ     255
