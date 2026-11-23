; ------------------------------------------------------------------------
; 名称：bios.s
; 说明：步步高学生电脑 BBG 的 BIOS 源码，使用 ca65 语法，各 org 对应的段由 link.cfg 指定地址。
; 作者：sungbin
; 邮箱：sungbin@3578.cn
; 日期：2026-11-23
; 备注：段 S0 至 S7 对应原版的各个 org，地址由 link.cfg 指定；汇编链接通过，结果不与 ROM
; 逐字节比对。
; ------------------------------------------------------------------------
        .include "tvdef.inc"            ; 符号定义，见 src/include/tvdef.inc
        .segment "S0"                   ; 段 S0，对应原版 org $C000
BiosYear = 97                           ; BIOS 版本：年（97 即 1997 年）
BiosMonth = 1                           ; BIOS 版本：月
BiosDate = 0                            ; BIOS 版本：日
;---------------------------------------;
;    软驱控制器端口。                   ;
;---------------------------------------;
FDCDMADackIO = $ff80                    ; DMA 应答端口（3F1H）
FDCDMATcIO = $ff88                      ; DMA 终止计数端口（3F1H）
;---------------------------------------;
FDCDRQPortI = $ff90                     ; 读端口（3F2H）
;       位 6：FDC 的 DRQ 引脚。
FDCCtrlPortO = $ff90                    ; 写端口（3F2H）
;       位 5：驱动器 B 电机使能。
;       位 4：驱动器 A 电机使能。
;       位 3：允许 INT 与 DMA 信号。
;       位 2：FDC 复位（低有效）。
;       位 10：驱动器选择。
;---------------------------------------;
FDCIRQPortI = $ff98                     ; 中断状态读端口（3F3H）
;       位 6：FDC 的 IRQ 引脚。
;---------------------------------------;
FDCResetPortO = $ffa0                   ; 复位写端口（3F4H、3F6H）
;       位 6：FDC 复位引脚。
FDCStatPortI = $ffa0                    ; 状态读端口（3F4H）
;       位 7：FDC 就绪，可以通信。
;       位 6：通信方向。
;       位 5：FDC 正在执行命令。
;       位 4：未注明。
;---------------------------------------;
FDCDataPortIO = $ffa8                   ; 数据读写端口（3F5H）
;---------------------------------------;
FDCChangePortI = $ffb8                  ; 磁盘更换状态读端口（3F7H）
;       位 7：磁盘已更换。
FDCSpeedPortO = $ffb8                   ; 数据率写端口（3F7H）
;       位 1-0：00 为 500kbps（1.2M、1.44M）。
;       01 为 300kbps（360K）。
;       10 为 250kbps（720K）。
;---------------------------------------;
SpeakInitPort = $ff10                   ; 声音初始化端口
SpeakDataPort = $ff18                   ; 声音数据端口
;---------------------------------------;
;    PC 控制器（并口）端口。            ;
;---------------------------------------;
PCDaCtPortO = $ff40                     ; PC 数据与控制写端口
PCCDataPort = $ff40                     ; PCC 端口基址加 0：数据
;---------------------------------------;
;       位 4：向 PC 发出 ERROR 信号。
;       位 3-0：输出给 PC 的数据。
PCCtrlPortI = $ff48                     ; PCC 控制读端口
PCCStatsPortI = $ff48                   ; PCC 端口基址加 1：状态
;       位 3：PC 的 STB 引脚信号。
PCDataPortIH = $ff48                    ; PC 数据输入高 4 位
;       位 7-4：PC 输入的数据。
PCDataPortIL = $ff50                    ; PC 数据输入低 4 位
PCCCtrlPoutO = $ff50                    ; PCC 端口基址加 2：控制输出
;       位 3-0：PC 输入的数据。
;---------------------------------------;
;    游戏卡控制器端口。                 ;
;---------------------------------------;
VideoCtrlPort = $ff01                   ; 视频控制端口
;       位 1-0：6116 的 AV10 连接。
;       00：AV10。
;       01：AV11。
;       10：0。
;       11：1。
;       位 2：INT 计数与加载（低有效）。
;       位 3：$C000-$FFFF 映射到 DRAM 或 ROM。
IntCountPortL = $ff02                   ; 中断计数低字节端口
;       位 4-0：计数器（低 4 位）。
VideoDataPort = $ff03                   ; 视频寄存器数据端口
;       位 7-0：寄存器文件数据。
DRAMPagePort = $ff04                    ; DRAM 页选择端口
;       位 3-0：页号。
DRAMPagePortCD = $ff24                  ; DRAM 页选择端口（C、D 页）
DRAMPagePortEF = $ff2c                  ; DRAM 页选择端口（E、F 页）
;=======================================;
SoundPort0 = $ff10                      ; 声音端口 0（与 SpeakInitPort 同地址）
SoundPort1 = $ff18                      ; 声音端口 1（与 SpeakDataPort 同地址）
KebBoardLEDPort = $ff00                 ; 键盘指示灯端口
;---------------------------------------;
Addr2000 = $6000                        ; 未确认：$2000 寄存器的影子单元
Addr2001 = $6001                        ; 未确认：$2001 寄存器的影子单元
Addr2005 = $6005                        ; 未确认：$2005 寄存器的影子单元
SMDOS_Begin = $7c00                     ; 未确认：SB-DOS 程序区起始地址
;===============================================;
;   键盘、鼠标、打印机工作缓冲区。              ;
;-----------------------------------------------;
KBDBufferSize = 64                      ; 键盘缓冲区大小（字节）
KBDBegin = 0                            ; 缓冲区起始偏移
KBDEnd = KBDBufferSize                  ; 缓冲区结束偏移（不含）
KBDBuffer = $5c00                       ; 键盘缓冲区首地址，占 KBDBufferSize 字节
NMICheckFlag = KBDBuffer+KBDBufferSize  ; NMI 键盘检查标志，1 字节
;       位 7：置 1 不做 NMI 键盘检查，清 0 做检查。
;       位 6：置 1 在缓冲区无键时做键盘检查，清 0 不做。
TempSave1 = NMICheckFlag+1              ; 临时保存单元 1，1 字节
TempSave2 = TempSave1+1                 ; 临时保存单元 2，1 字节
BiosKeyCode = TempSave2+1               ; BIOS 键码，2 字节
DelayTimes = BiosKeyCode+2              ; 按键延迟计数，1 字节
RateTime = DelayTimes+1                 ; 按键重复速率计数，1 字节
MaximumS = RateTime+1                   ; 鼠标坐标上限组的起始地址，共 6 字节
MaximumX = MaximumS                     ; 上限 X，2 字节，与 MaximumS 同地址
MaximumY = MaximumX+2                   ; 上限 Y，2 字节
MouseMovePlus = MaximumY+2              ; 鼠标移动增量，2 字节
KeyboardDataLen = 12                    ; 按键映射表的长度（字节）
CurrentKeyMap = MouseMovePlus+2         ; 当前按键映射表，KeyboardDataLen 字节
PressCount0 = CurrentKeyMap+KeyboardDataLen ; 0 号手柄按键计数组的起始地址
PressACount0 = PressCount0              ; 0 号手柄 A 键按下计数，1 字节
PressBCount0 = PressACount0+1           ; 0 号手柄 B 键按下计数，1 字节
PressSlCount0 = PressBCount0+1          ; 0 号手柄 Select 键按下计数，1 字节
PressStCount0 = PressSlCount0+1         ; 0 号手柄 Start 键按下计数，1 字节
PressCount1 = PressStCount0+1           ; 1 号手柄按键计数组的起始地址
PressACount1 = PressCount1              ; 1 号手柄 A 键按下计数，1 字节
PressBCount1 = PressACount1+1           ; 1 号手柄 B 键按下计数，1 字节
PressSlCount1 = PressBCount1+1          ; 1 号手柄 Select 键按下计数，1 字节
PressStCount1 = PressSlCount1+1         ; 1 号手柄 Start 键按下计数，1 字节
MinimumS = PressStCount1+1              ; 鼠标坐标下限组的起始地址，共 6 字节
MinimumX = MinimumS                     ; 下限 X，2 字节，与 MinimumS 同地址
MinimumY = MinimumX+2                   ; 下限 Y，2 字节
LastRlMoveKey = MinimumY+2              ; 上一次的左右移动键，2 字节
MoveRlKeyCount = LastRlMoveKey+2        ; 左右移动键的连续计数，2 字节
MovePrKeyCount = MoveRlKeyCount+2       ; 上下移动键的连续计数，2 字节
MouseMovePlusC = MovePrKeyCount+2       ; 鼠标移动增量的计数，1 字节
;---------------------------------------; 以下由 KeyboardReset 初始化
KBResetClearBeg = MouseMovePlusC+1      ; KBResetClear 段的起始地址，复位时清零
LastKeyMap = KBResetClearBeg            ; 上一次的按键映射表，KeyboardDataLen 字节
KBDHead = LastKeyMap+KeyboardDataLen    ; 键盘缓冲区写入位置（头），1 字节
KBDTail = KBDHead+1                     ; 键盘缓冲区读出位置（尾），1 字节
KeyAltCompose = KBDTail+1               ; Alt 加小键盘数字的组合输入，1 字节
AutoRepeatCount = KeyAltCompose+1       ; 自动重复计数，1 字节
AutoRepeatDelay = AutoRepeatCount+1     ; 自动重复延迟计数，1 字节
KBResetClearLen = AutoRepeatDelay+1-KBResetClearBeg ; KBResetClear 段的长度（字节）
;===============================================;
;    BIOS 与监视器工作缓冲区。                  ;
;-----------------------------------------------;
DisplayBuffer = $5d00                   ; 显示缓冲区首地址
SplitScreenDt = DisplayBuffer+$100      ; 分屏数据区，$5E00
MoveBuffer = SplitScreenDt+$0a0         ; 移动缓冲区，$5EA0
StringBuffer = MoveBuffer               ; 字符串缓冲区，与 MoveBuffer 同地址
DiskIOBuffer = MoveBuffer               ; 磁盘 I/O 缓冲区，与 MoveBuffer 同地址
DiskIOZB = $00                          ; 磁盘 I/O 零页块的起始地址
Command = $00                           ; 磁盘命令号，1 字节
CMD_RESET = 0                           ; 复位软驱系统
CMD_STATUS = 1                          ; 读状态：A 返回状态，X 返回驱动器号
CMD_READ = 2                            ; 读扇区
CMD_WRITE = 3                           ; 写扇区
CMD_VERIFY = 4                          ; 校验扇区
CMD_FORMAT = 5                          ; 格式化磁道
CMD_SETPARAM = 6                        ; 设置磁盘介质参数
Drive = 1                               ; 驱动器号，1 字节
Track = 2                               ; 磁道号，1 字节
Head = 3                                ; 磁头号，1 字节
Sector = 4                              ; 扇区号，1 字节
BPage = 5                               ; 缓冲区页号，1 字节
Buffer = 6                              ; 缓冲区地址，2 字节；不能跨过 $C000
Sectors = 8                             ; 扇区数，1 字节
DiskIOCmdSize = Sectors+1-Command       ; 磁盘命令块的长度（字节）
DiskIOPageSave = DiskIOZB+DiskIOCmdSize ; 保存的缓冲区页号，1 字节
SectorBfPoint0 = DiskIOPageSave+1       ; 扇区缓冲区指针 0，2 字节
SectorBfPoint1 = SectorBfPoint0+2       ; 扇区缓冲区指针 1，2 字节
DiskIOZB_Size = SectorBfPoint1+2-DiskIOZB ; 磁盘 I/O 零页块的长度（字节）
;=======================================;
;    设备工作缓冲区。                   ;
;---------------------------------------;
MotorOnCount = MoveBuffer+256           ; 电机开启计数，1 字节
DiskOutStatus = MotorOnCount+1          ; 磁盘输出状态，1 字节
DiskOK = $00                            ; 磁盘状态码：正常
BadCommand = $01                        ; 磁盘状态码：命令错误
BadAddressMark = $02                    ; 磁盘状态码：地址标记错误
WriteProtect = $03                      ; 磁盘状态码：写保护
RecordNotFound = $04                    ; 磁盘状态码：找不到记录
BadReset = $05                          ; 磁盘状态码：复位错误
DriveEmpty = $06                        ; 磁盘状态码：驱动器无盘

BadDMA = $08                            ; 磁盘状态码：DMA 错误
BadSector = $0a                         ; 磁盘状态码：扇区错误

BadCRC = $10                            ; 磁盘状态码：CRC 错误
DataCorrected = $11                     ; 磁盘状态码：数据已校正
BadCntlr = $20                          ; 磁盘状态码：控制器错误
BadSeek = $40                           ; 磁盘状态码：寻道错误
TimeOut = $80                           ; 磁盘状态码：超时
NotRdy = $aa                            ; 磁盘状态码：驱动器未就绪
UndefErr = $bb                          ; 磁盘状态码：未定义错误
WriteFault = $cc                        ; 磁盘状态码：写故障
NoError = $e0                           ; 磁盘状态码：无错误
SeekStatus = DiskOutStatus+1            ; 寻道状态，1 字节
;       位 1：1 号驱动器已寻道。
;       位 0：0 号驱动器已寻道。
MotorStatus = SeekStatus+1              ; 电机状态，1 字节
;       位 5：1 号驱动器电机已开启。
;       位 4：0 号驱动器电机已开启。
SDriveMessage = MotorStatus+1           ; 驱动器存在标志，1 字节
;       位 7：软驱控制器存在。
;       位 1：1 号驱动器存在。
;       位 0：0 号驱动器存在。
MediaMessage = SDriveMessage+1          ; 介质信息，2 字节，分别对应 0、1 号驱动器
DriveMedia = MediaMessage+2             ; 驱动器介质类型，2 字节，分别对应 0、1 号驱动器
CurrentTrack = DriveMedia+2             ; 当前磁道号，2 字节，分别对应 0、1 号驱动器
NECStatus = CurrentTrack+2              ; NEC 控制器状态块，7 字节
;       写保护，1 字节。
;       读写扇区数，1 字节。
;       读写起始扇区，1 字节。
;       磁道号，2 字节。
;       ECC、字节数、驱动器与磁头，1 字节。
;       命令号，1 字节。
;=======================================;
BiosTemp1 = NECStatus+7                 ; BIOS 临时单元 1，1 字节
BiosTemp2 = BiosTemp1+1                 ; BIOS 临时单元 2，1 字节
BiosTemp3 = BiosTemp2+1                 ; BIOS 临时单元 3，1 字节
BiosTemp4 = BiosTemp3+1                 ; BIOS 临时单元 4，1 字节
BiosTemp5 = BiosTemp4+1                 ; BIOS 临时单元 5，1 字节
BiosTemp6 = BiosTemp5+1                 ; BIOS 临时单元 6，1 字节
GetByteSave = BiosTemp6+1               ; 读取字节的临时保存，1 字节
LowRAMBegin = GetByteSave+1             ; 低地址 RAM 的起始单元，2 字节；$5800-$6000 之间可用
SplitPoint = LowRAMBegin+2              ; 分屏参数指针，2 字节
;       位 7：PC 卡存在。
;       位 0：PC 卡已连接。
VectorSet = SplitPoint+1                ; 中断向量设置标志，1 字节
NMIDRAMPage = VectorSet+1               ; NMI 向量所在的 DRAM 页号，1 字节
INTDRAMPage = NMIDRAMPage+1             ; IRQ 向量所在的 DRAM 页号，1 字节
DiskDisplayOff = INTDRAMPage+1          ; 磁盘操作期间关闭显示的标志，1 字节
;---------------------------------------;
DispBfHead = DiskDisplayOff+1           ; 显示缓冲区写入位置（头），1 字节
DispBfTail = DispBfHead+1               ; 显示缓冲区读出位置（尾），1 字节
Addr2000N = DispBfTail+1                ; 未确认：$2000 寄存器的 RAM 影子，1 字节
ScrollPanning = Addr2000N+1             ; 滚动平移值，1 字节
ScrollPanningN = ScrollPanning+1        ; 滚动平移值的新值，1 字节
BRKASave = ScrollPanningN+1             ; BRK 时保存的 A 寄存器，1 字节
;---------------------------------------;
;    监视器模式参数。                   ;
MonitorMode = BRKASave+1                ; 监视器模式，1 字节
RBlockPoint = MonitorMode+1             ; 读块指针，2 字节
SBlockPoint = RBlockPoint+2             ; 保存块指针，2 字节
BlockLength = SBlockPoint+2             ; 块长度，2 字节
StringLength = BlockLength+2            ; 字符串长度，1 字节
BiosParamEnd = StringLength+1           ; BIOS 参数区的结束地址
;    监视器参数区结束。                 ;
;---------------------------------------; BIOS 功能命令区。
;    标注 "*" 的单元位置不能改变，应用程序直接使用这些单元。
;---------------------------------------;
;    键盘状态 KeyStatus1 的位定义，单元位于 $5FED（未在此处定义）。
;       位 7：[Insert] 状态。
;       位 6：[Caps Lock] 状态。
;       位 5：[Num Lock] 状态。
;       位 4：[Scroll Lock] 状态。
;       位 3：[Alt] 键按下。
;       位 2：[Ctrl] 键按下。
	;    键盘状态 KeyStatus1 的位 1、0。
;       位 1：左 [Shift] 键按下。
;       位 0：右 [Shift] 键按下。
;    键盘状态 KeyStatus2，单元位于 $5FEE。
;       位 7：[Insert] 键按下。
;       位 6：[Caps Lock] 键按下。
;       位 5：[Num Lock] 键按下。
;       位 4：[Scroll Lock] 键按下。
;       位 3：右 [Alt] 键按下。
;       位 2：右 [Ctrl] 键按下。
;       位 1：左 [Alt] 键按下。
;       位 0：左 [Ctrl] 键按下。
;    键盘状态 KeyStatus3，单元位于 $5FEF。
;       位 7：键盘已安装。
;       位 6：垂直消隐后检查键盘。
;       位 4：101/102 键盘标志。
;       位 3：暂停状态。
;       位 2：SysRq 键按下。
;       位 1：上一个 $E0 键的标志。
;       位 0：上一个 $E1 键的标志。
;    键盘状态 KeyStatus4，单元位于 $5FF0。
;       位 7：键盘发送错误。
;       位 2：Caps Lock 指示灯。
;       位 1：Num Lock 指示灯。
;       位 0：Scroll Lock 指示灯。
;    鼠标 0 的当前坐标，X 位于 $5FF2，Y 位于 $5FF3。
BiosUsePage = $5ff4                     ; BIOS 使用的 DRAM 页，取决于硬件
VideoPageNumber = $5ff5                 ; 视频页号，单位为 2K
NMIEnterFlag = $5ff8                    ; 是否已进入 NMI，1 字节
DRAMPageNotUse = $5ff9                  ; 禁止切换到 $5800-$7FFF 的 DRAM 页
DRAMPageSave = $5ffa                    ; 当前 DRAM 页的实际值
DRAMPageHigh = $5ffb                    ; DRAM 页的标志位
CurrentJoyKey = $5ffc                   ; 最近的手柄按键，0 或 1
Joystick1Key = $5ffc                    ; 手柄 1 的最近按键，与 CurrentJoyKey 同地址
Joystick2Key = $5ffd                    ; 手柄 2 的最近按键
PCCardMessage = $5ffe                   ; PC 卡状态
VideoCtrlSave = $5fff                   ; 视频控制寄存器的保存值
;---------------------------------------;
BRKOperation = 0                        ; 操作号：BRK
FileOperation = 1                       ; 操作号：文件
BootOperation = 2                       ; 操作号：启动
DebugOperation = 3                      ; 操作号：调试
;======= 临时段 ========================; 压缩长度 $1800，保存到 $F000
;=======================================;
;    以下为 BIOS 入口跳转表，每项 3 字节，地址固定。
;    入口 $5800：BIOS 功能入口。
	jmp	ROMBiosEntryJMP-$C000+$5800
;---------------------------------------;
;    入口 $5803：文件 I/O 入口。
	jmp	FileIOEntryJMP-$C000+$5800
;---------------------------------------;
;    入口 $5806：监视器与硬复位入口。
	jmp	MonitorEntryJMP-$C000+$5800
;---------------------------------------;
;    入口 $5809：设备入口。
	jmp	DeviceEntryJMP-$C000+$5800
;---------------------------------------;
;    入口 $580C：DOS 的 I/O 入口，与文件 I/O 入口共用代码。
	jmp	FileIOEntryJMP-$C000+$5800
;---------------------------------------;
;    入口 $580F：软复位入口。
	jmp	SoftResetEntryJMP-$C000+$5800
;---------------------------------------;
;    入口 $5812：Ctrl+Break，直接返回。
	rts	;
	.res 16	; 填充到下一个入口之前
;---------------------------------------;
;    软复位：把 $C000-$FFFF 切到 ROM，再经复位向量 $FFFC 重新启动。
SoftResetEntryJMP:	;
	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jmp	($fffc)	;
;=======================================;
;    把 $C000-$FFFF 切到 ROM。
;    入口与出口：A、P 保持不变。
MapToROM:	;
	pha	;
	php	;
	lda     #$7f	;
	and     Addr2000	;
	sta     $2008	;
	lda     #$f7	; 视频控制位 3 清 0：$C000-$FFFF 映射到 ROM
	sei	;
	and     VideoCtrlSave	;
	sta     VideoCtrlSave	;
	sta     VideoCtrlPort	;
	plp	;
	lda     Addr2000	;
	sta     $2000	;
	pla	;
	rts	;
;---------------------------------------;
;    把 $C000-$FFFF 切到 DRAM。
;    入口与出口：A、P 保持不变。
MapToRAM:	;
	pha	;
	php	;
	lda     #$7f	;
	and     Addr2000	;
	sta     $2008	;
	lda     #$08	; 视频控制位 3 置 1：$C000-$FFFF 映射到 DRAM
	sei	;
	ora     VideoCtrlSave	;
	sta     VideoCtrlSave	;
	sta     VideoCtrlPort	;
	plp	;
	lda     Addr2000	;
	sta     $2000	;
	pla	;
	rts	;
;---------------------------------------;
;    BIOS 入口分派：页号的设置与读取、设备功能直接处理，其余切到 ROM 后调用 ROMBios。
;    入口：A 为功能号。
ROMBiosEntryJMP:	;
	php	; 保存 D 与 I 标志
	cmp	#BiosSetPage	;
	beq	nebej04	;
	cmp	#BiosGetPage	;
	beq	nebej08	;
	cmp	#BiosDevice	;
	beq	nebej12	;
	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jsr	ROMBios	;
	jsr     MapToRAM-$C000+$5800	; 把 $C000-$FFFF 切回 DRAM
	plp	; 恢复 D 与 I 标志
	rts	;
nebej04:	jsr	SetPage-$C000+$5800	; 设置页号
	plp	;
	rts	;
nebej08:	jsr	GetPage-$C000+$5800	; 读取页号
	plp	;
	rts	;
nebej12:	jsr	DeviceEntry	; 设备功能
	plp	;
	rts	;
;---------------------------------------;
;    文件 I/O 服务不存在：返回 ServerAbsent 错误码，并置位进位标志。
ServerAbsent = $42	; 文件服务不存在的错误码
FileIOEntryJMP:	;
	lda	#ServerAbsent	;
	sec	;
RTSInstruction:	;
	rts	;
;---------------------------------------;
;    监视器入口：把 $C000-$FFFF 切到 ROM，再跳到 ROM 中的监视器。
MonitorEntryJMP:	;
	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jmp	MonitorEntryROM	; 进入 ROM 中的监视器
;---------------------------------------;
;    NMI 向量的间接地址，初始指向 RTSInstruction（经 $5800 偏移换算）。
NMIVectorAddr:	.word RTSInstruction-$C000+$5800
;    经间接跳转进入 NMI 向量指向的代码。
NMIVectorBranch:	;
	jmp	(NMIVectorAddr-$C000+$5800)
;    NMI 返回：恢复 DRAM 页与视频映射后返回。
NMIReturn:	;
	pla	;
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	lda	VideoCtrlSave	; 恢复视频映射
	sta	VideoCtrlPort	;
	pla	;
	rti	;
;    NMI 入口：递减 NMI 计数，计数到 0 时关闭软驱电机；Addr2000 位 7 为 0 时直接退出。
;    否则刷新显示缓冲区、处理分屏，再经 NMI 向量分派。
NMIEntry:	;
	pha	;
	dec	NMIEnterFlag	; 标记已进入 NMI
	dec	MotorOnCount	; 电机开启计数减 1
	bne	nenas04	; 计数未到 0 时保留电机状态
	lda	#$0c	;
	sta	FDCCtrlPortO	; 关闭软驱电机（位 4、5 清 0）
	lda	#0	;
	sta	MotorStatus	; 清除电机状态
nenas04:	bit	$2002	; 读 $2002 状态寄存器
	bit	Addr2000	; Addr2000 位 7 为 0 时 NMI 未启用
	bpl	NMIExit	;
;    监视器模式下不刷新显示，直接退出。
	lda	MonitorMode	;
	bne	NMIExit	;
	lda	DispBfHead	;
	cmp	DispBfTail	;
	beq	nenas08	;
;    显示缓冲区有待刷新的数据：映射到 ROM 后刷新显示，再映射回 DRAM。
	lda	VideoCtrlSave	; 映射到 ROM
	and	#$f7	;
	sta	VideoCtrlPort	;
	jsr	DisplayNMIROM	; 在 ROM 中刷新显示
	lda	VideoCtrlSave	; 映射回 DRAM
	sta	VideoCtrlPort	;
nenas08:	lda	SplitPoint	; 是否有分屏操作
	beq	nenas12	;
;    有分屏操作：清除分屏标志，映射到 ROM 后切换背景。
	lda	#0	;
	sta	SplitPoint	;
	lda	VideoCtrlSave	; 映射到 ROM
	and	#$f7	;
	sta	VideoCtrlPort	;
	jsr	SwitchBG	; 在 ROM 中切换背景
	lda	VideoCtrlSave	; 映射回 DRAM
	sta	VideoCtrlPort	;
nenas12:	lda	Addr2001	; 恢复 $2001 显示控制寄存器
	sta	$2001	;
	jsr	NMIVectorBranch-$C000+$5800	; 经 NMI 向量分派
;    VectorSet 位 6 为 1 时，改在 NMI 所在的 DRAM 页中执行 NMI 的后续处理。
	bit	VectorSet	;
	bvc	nenas16	;
	lda	DRAMPageSave	; 保存当前 DRAM 页
	pha	;
;    构造 RTI 的返回地址：NMIReturn 的低字节与高字节压栈。
	lda	#>(NMIReturn-$C000+$5800)	;
	pha	;
	lda	#<(NMIReturn-$C000+$5800)	;
	pha	;
	php	;
	lda	NMIDRAMPage	; NMI 所在的 DRAM 页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	lda	VideoCtrlSave	; 映射到 DRAM（置位 3）
	ora	#8	;
	sta	VideoCtrlPort	;
	bne	NMIAddress	; 结果非零，等同于无条件跳转
nenas16:	pla	; 恢复入口时保存的 A
;    NMIAddressSave 是下一条 jmp 的操作数低字节，运行时可被修改（自修改代码）。
NMIAddress:	;
NMIAddressSave = NMIAddress+1	;
	jmp	RTIReturn-$C000+$5800	;
;---------------------------------------;
;    分屏中断入口：保存 A，调用分屏结束时的子程序。
;    SEndAddressSave 是下一条 jsr 的操作数，运行时可被修改（自修改代码）。
SplitScreenINT:	;
	pha	; 保存 A
SEndAddressAdd:	;
SEndAddressSave = SEndAddressAdd+1	;
	jsr	RTSInstruction-$C000+$5800
NMIExit:	pla	; 恢复 A
RTIReturn:	; 此处必须位于 $5800-$58FF，原因未确认
	rti	;
;---------------------------------------;
;    分屏结束时访问 DRAM：切换到 INT 所在的 DRAM 页，调用后恢复原映射与页号。
;    出口：Y 与 A 恢复原值。
SEndAccessRAM:	;
	tya	;
	pha	;
	lda	DRAMPageSave	; 保存当前 DRAM 页
	pha	;
	lda	INTDRAMPage	; INT 所在的 DRAM 页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	lda	VideoCtrlSave	; 映射到 DRAM（置位 3）
	pha	;
	ora	#8	;
	sta	VideoCtrlSave	;
	sta	VideoCtrlPort	;
;    SEndAccessSave 是下一条 jsr 的操作数，运行时可被修改（自修改代码）。
SendAccessAdd:	;
SEndAccessSave = SendAccessAdd+1	;
	jsr	RTSInstruction-$C000+$5800
	lda	#$f7	;
	and	VideoCtrlSave	; 映射回原来的状态
	sta	VideoCtrlSave	;
	pla	;
	and	#8	;
	ora	VideoCtrlSave	;
	sta	VideoCtrlSave	;
	sta	VideoCtrlPort	;
	pla	;
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	pla	;
	tay	;
	rts	;
;---------------------------------------;
;    BRK 与 IRQ 共用的入口：根据状态寄存器的位 4 区分 BRK 与 IRQ。
;    IRQ 时恢复 A 并返回 RTIReturn；BRK 时进入监视器。
BRKINTEntry:	;
	sta	BRKASave	; 保存 A
	pla	; 取出中断时的状态寄存器
	pha	; 放回栈中
	and	#$10	; 位 4 为 1 是 BRK，为 0 是 IRQ
	bne	nenir08	;
	lda     BRKASave	; IRQ：恢复 A
;    BRKAddressSave 是下一条 jmp 的操作数，运行时可被修改（自修改代码）。
BRKAddress:	;
BRKAddressSave = BRKAddress+1	;
	jmp	RTIReturn-$C000+$5800	; 返回 RTIReturn 执行 RTI
;---------------------------------------;
;    硬件中断进入监视器的路径，当前没有已启用的入口跳转到这里（未确认）。
nenir04:	lda	#$80	;
	sta	MonitorMode	;
	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jmp	PCINTAccess	;
;    BRK：置监视器模式为 $80，映射到 ROM 后进入 BRKAccess。
nenir08:	lda	#$80	;
	sta	MonitorMode	;
	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jmp	BRKAccess	;
	brk	; 原版的 BRK 指令，保持字节不变
;---------------------------------------;
;    目标程序返回入口：映射到 DRAM，清除监视器模式后返回。
TargetGoExit:	;
	jsr	MapToRAM-$C000+$5800	; 把 $C000-$FFFF 切回 DRAM
	lda	#0	;
	sta	MonitorMode	; 清除监视器模式
	pla	;
	rti	;
;---------------------------------------;
;    在 DRAM 中搜索字符串：$FE/$FF 为搜索指针，BlockLength 为剩余长度。
;    找到时把地址发送出去，然后返回 SearchStringRET。
SearchStringRAM:	;
	lda	Addr2000	;
	pha	;
	and	#$7f	; 清 NMI 允许位（位 7）后搜索
	sta	Addr2000	;
	jsr	MapToRAM-$C000+$5800	; 把 $C000-$FFFF 切到 DRAM
	lda	StringBuffer	; 字符串首字符
lpssr08:	cmp	($fe),y	; 比较首字符
	bne	nessr20	;
	ldx	StringLength	;
	dex	;
	beq	nessr12	; 字符串只有 1 个字符，直接命中
	ldx	#1	; 从第 2 个字符开始比较
lpssr12:	iny	;
	lda	StringBuffer,x	;
	cmp	($fe),y	;
	bne	nessr16	;
	inx	;
	cpx	StringLength	;
	bne	lpssr12	;
;    命中：映射到 ROM，发送命中地址，再映射回 DRAM。
nessr12:	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	lda	 $ff	; 发送地址的高字节
	jsr	SendByte	;
	lda	 $fe	; 发送地址的低字节
	jsr	SendByte	;
	jsr	MapToRAM-$C000+$5800	; 把 $C000-$FFFF 切回 DRAM
nessr16:	lda	StringBuffer	; 未命中，重新取首字符
	ldy	#0	;
nessr20:	inc	 $fe	; 搜索指针加 1
	bne	nessr24	;
	inc	 $ff	;
nessr24:	dec	BlockLength	; 剩余长度减 1
	bne	lpssr08	;
	dec	BlockLength+1	;
	bne	lpssr08	;
	pla	;
	sta	Addr2000	; 恢复 Addr2000
	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jmp	SearchStringRET	;
;---------------------------------------;
;    映射到 DRAM 后调用 CallAddress，返回时映射到 ROM。
CallToRAM:	;
	jsr	MapToRAM-$C000+$5800	; 把 $C000-$FFFF 切到 DRAM
;    CallAddress 是下一条 jsr 的操作数，运行时可被修改（自修改代码）。
CallAddress:	;
	jsr	RTSInstruction-$C000+$5800
	jmp	MapToROM-$C000+$5800	; 返回时把 $C000-$FFFF 切到 ROM
;=======================================;
;    设置 DRAM 页。
;    入口：X 为页号。出口：A、Y 不改变。
SetPage:	cpx	BiosUsePage	;
	bne	nespe04	;
	ldx	DRAMPageNotUse	; 页号等于 BiosUsePage 时改用 DRAMPageNotUse
nespe04:	cpx	#0	;
	bne	nespe08	;
	ldx	BiosUsePage	; 页号为 0 时改用 BiosUsePage
nespe08:	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
	rts	;
;=======================================;
;    读取 DRAM 页号。
;    出口：A 为页号。X、Y 不改变。
GetPage:	lda	DRAMPageSave	;
	cmp	BiosUsePage	;
	bne	negpe04	;
	lda	#0	; 当前页为 BiosUsePage 时返回 0
negpe04:	cmp	DRAMPageNotUse	;
	bne	negpe08	;
	lda	BiosUsePage	; 当前页为 DRAMPageNotUse 时返回 BiosUsePage
negpe08:	rts	;
;=======================================;
;    设备入口：X=1 时测试键盘，其他功能切到 ROM 中的 DeviceEntryROM 处理。
DeviceEntryJMP:	;
	cpx	#1	; X=1 时测试键盘
	bne	nekbj04	;
;---------------------------------------; 测试键盘（X=1）
;    键盘缓冲区有键时取出一个键：A 为键码，X 为扩展码，Y=1；无键时 Y=0。
TestKeyBoardDev:	;
lptkd04:	ldy	KBDHead	; 缓冲区是否有键
	cpy	KBDTail	;
	beq	netkd04	;
	lda	KBDBuffer,y	; 有键：取出键码与扩展码
	ldx	KBDBuffer+1,y	;
	ldy	#1	; Y=1 表示有键
	rts	;
netkd04:	bit	NMICheckFlag	;
	bvs	nekbj04	; 位 6 为 1 时交给 ROM 检查键盘
	ldy	#0	; 无键返回 Y=0
	rts	;
nekbj04:	jsr	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
	jsr     DeviceEntryROM	; 在 ROM 中处理设备功能
	jmp	MapToRAM-$C000+$5800	; 把 $C000-$FFFF 切回 DRAM
;---------------------------------------;
;    键盘 NMI：NMICheckFlag 位 7 为 1 时不做检查，否则映射到 ROM 执行 KeyBoardNMIROM。
KeyBoardNMI:	;
	bit	NMICheckFlag	;
	bmi	nekbn04	;
KeyBoardNMIDo:	;
	lda	VideoCtrlSave	; 映射到 ROM
	and	#$f7	;
	sta	VideoCtrlPort	;
	jsr	KeyBoardNMIROM	; 在 ROM 中检查键盘
	lda	VideoCtrlSave	; 映射回 DRAM
	sta	VideoCtrlPort	;
nekbn04:	rts	;
;---------------------------------------;
;    暂停：循环等待 KeyStatus3 位 3（暂停状态）清零，然后把 $C000-$FFFF 切到 ROM。
PauseRunningRAM:	;
	lda	#8	;
lpprg04:	bit	KeyStatus3	;
	bne	lpprg04	;
	jmp	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
;---------------------------------------;
;    调用 CtrlBreakEntry，返回后把 $C000-$FFFF 切到 ROM。
CtrlBreakEntryRAM:	;
	jsr	CtrlBreakEntry	;
	jmp	MapToROM-$C000+$5800	; 把 $C000-$FFFF 切到 ROM
MoveRAMEnding:	;

	.byte	$cf,$b5,$cd,$b3,$d6,$c6,$d7,$f7,$3a,$c7,$e5,$bb,$aa,$b4,$f3,$d1	; 制作单位字符串
	.byte	$a7,$bc,$c6,$cb,$e3,$bb,$fa,$cf,$b5,$cc,$c6,$c8,$f0,$b4,$ba
	.byte "written by Tang ruichun Tsinghua University Computer Department"	; 英文版权字符串
;---------------------------------------;
;    连接 PC 卡：经并口握手交换字节，成功返回 A=1，超时或校验失败返回 A=0。
ConnectPCCard:	;
	jsr	EnableNMI	; 打开 NMI 作为超时计时
lpccc04:	lda	#$1a	; 置 P-ERROR（接收结束）
	sta	PCDaCtPortO	; 输出标志字节
lpccc08:	ldx	#20	; 等待码 $FF 连续出现 20 次
lpccc12:	lda	#25	; 设置超时计数
	sta	NMIEnterFlag	;
lpccc16:	lda	NMIEnterFlag	; 超时检查
	beq	neccc08	;
	lda	PCCtrlPortI	;
	and	#8	; P-STB 为低表示数据就绪
	bne	lpccc16	;
	lda	PCDataPortIH	; 读取高 4 位
	and	#$f0	;
	sta	GetByteSave	;
	lda	#$0a	; 复位 P-ERROR（数据已接收）
	sta	PCDaCtPortO	;
	lda	PCDataPortIL	;
	and	#$0f	;
	ora	GetByteSave	;
	eor	#$8b	;
	tay	; 保存接收到的字节
lpccc20:	lda	NMIEnterFlag	;
	beq	neccc08	;
	lda	PCCtrlPortI	; 等待 P-STB 变高（发送结束）
	and	#8	;
	beq	lpccc20	;
	lda	#$1a	; 置 P-ERROR（接收结束）
	sta	PCDaCtPortO	;
	tya	;
	cmp	#$aa	; 接收到的字节必须为 $AA，否则重新等待
	bne	lpccc08	;
	dex	; 等待码计数减 1
	bne	lpccc12	;
	lda	#$55	; 发送结束字符 $55
	jsr	SendByte	;
	jsr	GetByte	;
	cmp	#$55	; 读取对方的结束字节，应为 $55
	bne	lpccc04	;
	lda	#$5a	; 回复 $5A
	jsr	SendByte	;
	lda	#1	; 连接成功
	rts	;
neccc08:	lda	#$10	; 失败：置 P-ERROR（接收结束）
	sta	PCDaCtPortO	;
	lda	#0	; 返回 0
	rts	;
;---------------------------------------;
;    调色板初值，4 字节。
PaletteTable:	.byte $0e	; 调色板第 0 字节
	.byte $00	; 调色板第 1 字节
	.byte $12	; 背景色（蓝）
	.byte $20	; 普通显示色
PaletteLength = *-PaletteTable	; 调色板数据的长度（字节）
;    把调色板数据写入 $3F00 开始的 PPU 调色板。
InitialPalette:	;
	ldx	#>($3f00)	;
	ldy	#<($3f00)	;
	jsr	VRAMAddress	; 设置 PPU 地址为 $3F00
lpipe04:	lda	PaletteTable,y	; 逐字节写入调色板
	sta	$2007	;
	iny	;
	cpy	#PaletteLength	;
	bne	lpipe04	;
	lda	#0	; 把 PPU 地址复位为 0
	sta	$2006	;
	sta	$2006	;
	rts	;
;---------------------------------------;
;    等待一个垂直消隐：先等 VBLANK 结束，再等 VBLANK 开始。
WaitVSyn:	;
lpwvs04:	bit	$2002	;
	bmi	lpwvs04	;
lpwvs08:	bit	$2002	;
	bpl	lpwvs08	;
	rts	;
;---------------------------------------;
;    计算字符串显示的 VRAM 地址（行由 $FC 给出，列为 A），结果在 X|Y，
;    字符串指针保存在 $FE/$FF（具体位运算未逐位核对）。
DisplayStringInit:	;
	stx	$ff	;
	sta	$fe	;
	lda	$fc	;
	asl	;
	asl	;
	asl	;
	asl	;
	php	;
	asl	;
	ora	$fe	;
	sty	$fe	;
	tay	;
	lda	#$10	;
	ror	;
	plp	;
	rol	;
	rol	;
	tax	;
	rts	;
;---------------------------------------;
;    清除一行的显示内容（64 个单元写 $6A），再显示字符串。
CleanDisplayStg:	;
	jsr	DisplayStringInit	;
	pha	;
	tya	;
	pha	;
	and	#$e0	; 指向行首
	tay	;
	jsr	WaitVSyn	;
	jsr	VRAMAddress	;
	ldy	#64	; 清除 64 个单元
	lda	#$6a	;
lpcds04:	sta	$2007	;
	dey	;
	bne	lpcds04	;
	pla	; 恢复 Y
	pha	;
	tay	;
	jmp	DisplayStringINT	;
;---------------------------------------;
;    显示字符串，以 $FF 结束。
;    入口：X|Y 为字符串地址，$FC 为行号，A 为列号。
DisplayString:	;
	jsr	DisplayStringInit	; 得到 VRAM 地址
	pha	;
	tya	;
	pha	;
	jsr	WaitVSyn	;
DisplayStringINT:	;
	jsr	VRAMAddress	;
	ldy	#0	;
lpdsg04:	lda	($fe),y	; 第一行写入字符，遇 $FF 结束
	iny	;
	cmp	#$ff	;
	beq	nedsg04	;
	sta	$2007	;
	bne	lpdsg04	;
nedsg04:	pla	;
	clc	;
	adc	#$20	; 下一行的 VRAM 地址
	tay	;
	pla	;
	adc	#0	;
	tax	;
	jsr	VRAMAddress	;
	ldy	#0	;
;    第二行的图块号为字符码加 1（含进位）。
lpdsg08:	lda	($fe),y	;
	iny	;
	cmp	#$ff	;
	beq	nedsg08	;
	adc	#1	;
	sta	$2007	;
	bne	lpdsg08	;
nedsg08:
;---------------------------------------;
;    打开显示：滚动清零，显示控制寄存器设为 $0E。
OpenScreen:	;
	lda	#0	;
	sta	$2005	;
	sta	$2005	;
	lda	#$0e	;
	sta	$2001	;
	rts	;
;---------------------------------------;
;    16 位除以 8 位：被除数为 X|Y（X 为高字节），除数为 A。
;    出口：A 为商，X 为余数，Y 为 0，Z 标志与 A 相同，C 标志保持原值。
DivXY_A:	sty	$ff	;
	sta	$fe	;
	txa	;
	ldy	#8	; 8 位循环
lpdxy04:	rol	$ff	;
	rol	a	;
	cmp	$fe	;
	bcc	nedxy04	;
	sbc	$fe	;
nedxy04:	dey	;
	bne	lpdxy04	;
	tax	;
	lda	$ff	;
	rol	a	;
	rts	;
;---------------------------------------;
;    把 X|Y 除以 10 的一位余数与商写入显示缓冲区，图块号为数字乘 2；$80 为写入位置。
CheckRAMSub:	;
	ldx	#0	;
	lda	#10	;
	jsr	DivXY_A	; X|Y 除以 10：A 为商，X 为余数
	bne	necrs04	; 商不为 0 时先写商
	bcs	necrs08	;
necrs04:	ldy	$80	;
	asl	;
	sta	$81,y	; 写入商对应的图块号
	inc	$80	;
necrs08:	ldy	$80	;
	txa	;
	asl	;
	sta	$81,y	; 写入余数对应的图块号
	inc	$80	;
	rts	; C=0
;---------------------------------------;
;    显示 X|Y 表示的数值，单位为 K 或 M（A 为单位字符的图块号，从栈中取出）。
;    入口：X|Y 为数据。
DisplayChkRAM:	;
	pha	;
	lda	#100	;
	sec	;
	jsr	DivXY_A	; X|Y 除以 100：A 为商，X 为余数
	beq	nedcm04	; 商为 0 时只显示余数
	tay	;
	txa	; 保存 X（余数）
	pha	;
	jsr	CheckRAMSub	;
	pla	;
	bcc	nedcm08	;
nedcm04:	txa	;
nedcm08:	tay	;
	jsr	CheckRAMSub	;
	ldx	$80	;
	pla	; 单位字符（K 或 M）
	sta	$81,x	;
	lda	#$ff	; 字符串结束
	sta	$81+1,x	;
	ldx	#>($0081)	;
	ldy	#<($0081)	;
	lda	$fd	;
	jmp	DisplayString	;
;---------------------------------------;
;    8x8 字库图案数据，每行 16 字节为两个图块；注释给出偏移与对应字符。
FontTable:
	.byte $00,$00,$00,$00,$00,$7C,$C6,$C6,$CE,$D6,$D6,$E6,$C6,$C6,$7C,$00	; 偏移 $00：字符 "0"
	.byte $00,$00,$00,$00,$00,$18,$38,$78,$18,$18,$18,$18,$18,$18,$7E,$00	; 偏移 $02：字符 "1"
	.byte $00,$00,$00,$00,$00,$7C,$C6,$06,$0C,$18,$30,$60,$C0,$C6,$FE,$00	; 偏移 $04：字符 "2"
	.byte $00,$00,$00,$00,$00,$7C,$C6,$06,$06,$3C,$06,$06,$06,$C6,$7C,$00	; 偏移 $06：字符 "3"
	.byte $00,$00,$00,$00,$00,$0C,$1C,$3C,$6C,$CC,$FE,$0C,$0C,$0C,$1E,$00	; 偏移 $08：字符 "4"
	.byte $00,$00,$00,$00,$00,$FE,$C0,$C0,$C0,$FC,$0E,$06,$06,$C6,$7C,$00	; 偏移 $0a：字符 "5"
	.byte $00,$00,$00,$00,$00,$38,$60,$C0,$C0,$FC,$C6,$C6,$C6,$C6,$7C,$00	; 偏移 $0c：字符 "6"
	.byte $00,$00,$00,$00,$00,$FE,$C6,$06,$06,$0C,$18,$30,$30,$30,$30,$00	; 偏移 $0e：字符 "7"
	.byte $00,$00,$00,$00,$00,$7C,$C6,$C6,$C6,$7C,$C6,$C6,$C6,$C6,$7C,$00	; 偏移 $10：字符 "8"
	.byte $00,$00,$00,$00,$00,$7C,$C6,$C6,$C6,$7E,$06,$06,$06,$0C,$78,$00	; 偏移 $12：字符 "9"
	.byte $00,$00,$00,$00,$00,$10,$38,$6C,$C6,$C6,$FE,$C6,$C6,$C6,$C6,$00	; 偏移 $14：字符 "A"
	.byte $00,$00,$00,$00,$00,$FC,$66,$66,$66,$7C,$66,$66,$66,$66,$FC,$00	; 偏移 $16：字符 "B"
	.byte $00,$00,$00,$00,$00,$00,$00,$18,$18,$00,$00,$00,$18,$18,$00,$00	; 偏移 $18：字符 ":"
	.byte $00,$00,$00,$00,$00,$C6,$EE,$FE,$FE,$D6,$C6,$C6,$C6,$C6,$C6,$00	; 偏移 $1a：字符 "M"
	.byte $00,$00,$00,$00,$00,$E6,$66,$6C,$6C,$78,$78,$6C,$66,$66,$E6,$00	; 偏移 $1c：字符 "K"
	.byte $01,$01,$01,$01,$7F,$41,$41,$42,$42,$44,$48,$50,$40,$40,$40,$40	; 偏移 $1e：字符 "内"
	.byte $00,$00,$00,$04,$FE,$04,$04,$04,$84,$64,$34,$14,$04,$04,$14,$08	; 偏移 $20
	.byte $00,$1F,$10,$10,$1F,$10,$10,$1F,$14,$44,$34,$14,$04,$04,$FF,$00	; 偏移 $22：字符 "显"
	.byte $10,$F8,$10,$10,$F0,$10,$10,$F0,$50,$44,$4C,$50,$40,$44,$FE,$00	; 偏移 $24
	.byte $02,$02,$FF,$04,$08,$0B,$10,$10,$30,$5F,$90,$10,$10,$10,$11,$10	; 偏移 $26：字符 "存"
	.byte $00,$04,$FE,$00,$00,$F8,$10,$20,$44,$FE,$40,$40,$40,$40,$40,$80	; 偏移 $28
	.byte $10,$10,$10,$FE,$20,$29,$48,$7E,$08,$08,$0E,$F8,$48,$08,$09,$0A	; 偏移 $2a：字符 "软"
	.byte $40,$40,$40,$7C,$84,$08,$20,$20,$20,$20,$50,$50,$88,$88,$06,$04	; 偏移 $2c
	.byte $08,$FD,$09,$49,$49,$49,$49,$49,$7D,$05,$05,$1D,$E5,$45,$15,$08	; 偏移 $2e：字符 "驱"
	.byte $04,$FE,$00,$44,$44,$28,$28,$10,$10,$28,$28,$44,$84,$00,$FE,$00	; 偏移 $30
	.byte $02,$02,$03,$02,$02,$02,$FF,$02,$02,$02,$02,$02,$02,$02,$02,$02	; 偏移 $32：字符 "卡"
	.byte $00,$20,$F0,$00,$00,$04,$FE,$00,$00,$80,$60,$30,$10,$00,$00,$00	; 偏移 $34
	.byte $12,$12,$2A,$4A,$8A,$1F,$20,$7F,$A0,$2F,$29,$29,$29,$29,$30,$20	; 偏移 $36：字符 "微"
	.byte $10,$10,$90,$A0,$BE,$C4,$24,$A8,$28,$28,$10,$50,$A8,$28,$46,$84	; 偏移 $38
	.byte $10,$10,$11,$11,$FD,$11,$31,$39,$55,$51,$91,$11,$11,$12,$14,$18	; 偏移 $3a：字符 "机"
	.byte $00,$10,$F8,$10,$10,$10,$10,$10,$10,$10,$10,$10,$12,$12,$0E,$00	; 偏移 $3c
	.byte $03,$40,$30,$10,$03,$02,$F2,$13,$12,$12,$13,$12,$12,$2A,$44,$03	; 偏移 $3e：字符 "通"
	.byte $F8,$10,$A0,$40,$F8,$48,$48,$F8,$48,$48,$F8,$48,$68,$50,$00,$FE	; 偏移 $40
	.byte $00,$47,$30,$11,$01,$01,$F7,$11,$11,$11,$11,$11,$11,$29,$44,$03	; 偏移 $42：字符 "迅"
	.byte $10,$F8,$10,$10,$10,$10,$F0,$10,$10,$10,$12,$0A,$06,$00,$00,$FE	; 偏移 $44
	.byte $40,$31,$11,$01,$81,$62,$24,$03,$12,$21,$E1,$20,$20,$20,$23,$2C	; 偏移 $46：字符 "没"
	.byte $10,$F8,$10,$10,$10,$0E,$00,$F8,$08,$10,$10,$A0,$40,$A0,$1E,$08	; 偏移 $48
	.byte $00,$40,$2F,$21,$01,$02,$E7,$20,$20,$20,$2F,$20,$20,$50,$8F,$00	; 偏移 $4a：字符 "连"
	.byte $80,$88,$FC,$00,$40,$40,$F8,$40,$40,$48,$FC,$40,$40,$40,$FE,$00	; 偏移 $4c
	.byte $00,$7F,$02,$04,$08,$3F,$01,$06,$18,$3F,$11,$01,$11,$21,$45,$02	; 偏移 $4e：字符 "系"
	.byte $38,$C0,$00,$20,$40,$80,$00,$20,$10,$F8,$08,$00,$10,$08,$08,$00	; 偏移 $50
	.byte $10,$10,$20,$27,$48,$F9,$12,$27,$41,$F9,$41,$01,$1A,$E2,$44,$08	; 偏移 $52：字符 "统"
	.byte $80,$40,$48,$FC,$80,$10,$08,$FC,$24,$20,$20,$20,$22,$22,$1E,$00	; 偏移 $54
	.byte $10,$10,$10,$10,$FD,$12,$35,$30,$58,$54,$91,$11,$11,$10,$17,$10	; 偏移 $56：字符 "检"
	.byte $40,$40,$A0,$A0,$10,$08,$F6,$04,$00,$48,$48,$50,$10,$04,$FE,$00	; 偏移 $58
	.byte $00,$40,$37,$14,$85,$65,$25,$0D,$15,$15,$65,$21,$22,$24,$28,$20	; 偏移 $5a：字符 "测"
	.byte $04,$04,$C4,$54,$54,$54,$54,$54,$54,$54,$54,$04,$84,$44,$54,$08	; 偏移 $5c
	.byte $01,$00,$1F,$10,$10,$10,$1F,$10,$10,$10,$17,$24,$24,$44,$87,$04	; 偏移 $5e：字符 "启"
	.byte $00,$84,$FE,$04,$04,$04,$FC,$00,$00,$04,$FE,$04,$04,$04,$FC,$04	; 偏移 $60
	.byte $00,$00,$7C,$00,$01,$FE,$10,$10,$20,$24,$42,$FE,$42,$01,$02,$04	; 偏移 $62：字符 "动"
	.byte $40,$40,$40,$44,$FE,$44,$44,$44,$44,$44,$84,$84,$84,$04,$28,$10	; 偏移 $64
	.byte $00,$08,$08,$08,$08,$08,$08,$08,$08,$08,$14,$12,$21,$21,$42,$84	; 偏移 $66：字符 "从"
	.byte $00,$20,$20,$20,$20,$20,$20,$50,$50,$50,$50,$88,$88,$0C,$06,$04	; 偏移 $68
	.byte $00,$00,$00,$00,$00,$00,$00,$00,$18,$3c,$3c,$18,$00,$00,$00,$00	; 偏移 $6a：字符 "."
	.byte $01,$11,$11,$11,$1F,$21,$41,$01,$FF,$01,$02,$02,$04,$08,$10,$60	; 偏移 $6c：字符 "失"
	.byte $00,$00,$00,$10,$F8,$00,$00,$04,$FE,$00,$80,$80,$40,$30,$0E,$04	; 偏移 $6e
	.byte $00,$7C,$44,$54,$54,$55,$56,$54,$54,$54,$54,$10,$28,$24,$45,$82	; 偏移 $70：字符 "败"
	.byte $40,$40,$44,$FE,$88,$08,$88,$88,$88,$50,$50,$20,$60,$90,$0E,$04	; 偏移 $72
	.byte $04,$1F,$10,$12,$11,$FF,$10,$12,$21,$40,$3F,$24,$24,$24,$FF,$00	; 偏移 $74：字符 "盘"
	.byte $10,$F8,$10,$10,$10,$FE,$10,$10,$50,$20,$F8,$48,$48,$48,$FE,$00	; 偏移 $76
	.byte $10,$10,$10,$13,$FA,$14,$10,$1B,$30,$D0,$11,$10,$10,$10,$50,$23	; 偏移 $78：字符 "按"
	.byte $40,$20,$00,$FE,$02,$44,$40,$FE,$88,$88,$10,$90,$60,$50,$8C,$04	; 偏移 $7a
	.byte $08,$08,$0B,$10,$10,$30,$50,$9F,$10,$10,$10,$10,$10,$17,$10,$10	; 偏移 $7c：字符 "任"
	.byte $00,$18,$E0,$40,$40,$40,$44,$FE,$40,$40,$40,$40,$48,$FC,$00,$00	; 偏移 $7e
	.byte $01,$3F,$08,$04,$FF,$00,$1F,$10,$1F,$10,$1F,$02,$51,$50,$90,$0F	; 偏移 $80：字符 "意"
	.byte $00,$F8,$20,$44,$FE,$00,$F0,$10,$F0,$10,$F0,$00,$84,$92,$12,$F0	; 偏移 $82
	.byte $20,$27,$39,$41,$82,$7A,$24,$27,$F9,$21,$25,$22,$2A,$35,$28,$00	; 偏移 $84：字符 "键"
	.byte $10,$10,$7C,$14,$FE,$14,$7C,$10,$7C,$10,$10,$FE,$10,$10,$90,$7E	; 偏移 $86
	.byte $10,$10,$17,$10,$F8,$17,$10,$19,$36,$D4,$14,$17,$14,$14,$57,$24	; 偏移 $88：字符 "插"
	.byte $08,$3C,$C0,$40,$44,$FE,$40,$40,$5C,$44,$44,$5C,$44,$44,$FC,$04	; 偏移 $8a
	.byte $04,$02,$01,$01,$01,$02,$02,$02,$04,$04,$08,$08,$10,$20,$40,$80	; 偏移 $8c：字符 "入"
	.byte $00,$00,$00,$00,$00,$80,$80,$80,$40,$40,$20,$20,$10,$10,$0E,$04	; 偏移 $8e
	.byte $00,$00,$00,$FF,$00,$00,$00,$00	; 字库末尾的填充
;---------------------------------------;
;    程序标识字符串。
SMDOS_String:	.byte "SMDOS"
;    检测项所在的行号。
ExamineLine = 7
SysExamStg:	.byte $4e,$50,$52,$54,$56,$58,$5a,$5c,$ff	; 系统检测
MainMemoryStg:	.byte $1e,$20,$26,$28,$18	; 内存:
VideoMemoryStg:	.byte $22,$24,$26,$28,$18	; 显存:
FDCStg:	.byte $2a,$2c,$2e,$30,$32,$34,$ff	; 软驱卡
FDDAStg:	.byte $2a,$2c,$2e,$30,$18,$14,$ff	; 软驱:A
FDDBStg:	.byte $2a,$2c,$2e,$30,$18,$16,$ff	; 软驱:B
NOFDDStg:	.byte $46,$48,$2a,$2c,$2e,$30,$ff	; 没软驱
PCCStg:	.byte $36,$38,$3a,$3c,$3e,$40,$42,$44,$32,$34,$ff	; 微机通讯卡
PCCConnectStg:	.byte $46,$48,$4a,$4c,$3e,$40,$36,$38,$3a,$3c,$ff	; 没连通微机
;    启动信息所在的行号。
BootLine = 21
SysBootStg:	.byte $4e,$50,$52,$54,$5e,$60,$62,$64,$ff	; 系统启动
BootFDCAStg:	.byte $66,$68,$2a,$2c,$2e,$30,$14,$5e,$60,$62,$64	; 从软驱A启动...
	.byte $6a,$6a,$6a,$ff
BootFDCBStg:	.byte $66,$68,$2a,$2c,$2e,$30,$16,$5e,$60,$62,$64	; 从软驱B启动...
	.byte $6a,$6a,$6a,$ff
BootFDCErrStg:	.byte $2a,$2c,$2e,$30,$88,$8a,$8c,$8e,$4e,$50,$52,$54	; 软驱插入系统盘
	.byte $74,$76,$ff
PressKeyStg:	.byte $78,$7a,$7c,$7e,$80,$82,$84,$86,$ff	; 按任意键
BootPCCStg:	.byte $66,$68,$36,$38,$3a,$3c,$3e,$40,$42,$44,$32,$34	; 从微机通讯卡启动...
	.byte $5e,$60,$62,$64,$6a,$6a,$6a,$ff
BootFault:	.byte $4e,$50,$52,$54,$5e,$60,$62,$64	; 系统启动失败
	.byte $6c,$6e,$70,$72,$ff
;---------------------------------------;
;    初始化画面：把字库写入图案表，清空名称表与属性表。
InitialScreen:	;
	ldx	#>($0000)	;
	ldy	#<($0000)	;
	jsr	VRAMAddress	; 设置 VRAM 地址 $0000
;---------------------------------------; 装入背景字库
	ldx	#>(FontTable)	;
	ldy	#<(FontTable)	;
	stx	$ff	;
	sty	$fe	;
	lda	#<($91)	;
	sta	$fd	;
	ldy	#0	;
lpibf04:	ldx	#8	;
lpibf08:	lda	($fe),y	; 每组写入 8 字节字库数据
	iny	;
	sta	$2007	;
	dex	;
	bne	lpibf08	;
	lda	#$ff	;
	ldx	#8	; 再写入 8 字节 $FF
lpibf12:	sta	$2007	;
	dex	;
	bne	lpibf12	;
	tya	;
	bne	neibf04	;
	inc	$ff	; 跨页时指针加 1
neibf04:	dec	$fd	;
	bne	lpibf04	;
;---------------------------------------; 清空名称表与属性表
	ldx	#>($2000)	;
	ldy	#<($2000)	;
	jsr	VRAMAddress	;
	ldx	#4	; 清 $2000-$23BF，写空白图块 $6A
	lda	#$6a	;
	ldy	#$c0	;
lpivm04:	sta	$2007	;
	dey	;
	bne	lpivm04	;
	dex	;
	bne	lpivm04	;
	lda	#0	; 清 $23C0-$23FF 属性表
	ldy	#$40	;
lpivm08:	sta	$2007	;
	dey	;
	bne	lpivm08	;
	rts	;
;=======================================;
;    复位入口：关闭 NMI 与显示，初始化堆栈、并口与视频控制，然后检测显存。
Begin:	lda	#0	;
	sta	$2000	; 关闭 NMI
	sta	Addr2000	;
	sta	$2001	;
	sta	$4015	;
	sei	;
	cld	;
	ldx	#$ff	; 栈指针设为 $FF
	txs	;
	stx	DRAMPagePortEF	; 把最后一个 16K RAM 页映射到 $C000-$FFFF
	dex	;
	stx	DRAMPagePortCD	; 把页 $FE 映射到 $8000-$BFFF
	lda	#0	;
	sta	PCDaCtPortO	; 并口初始化
	sta	$ff50	; 打印机接口初始化
	sta	VideoCtrlPort	;
;---------------------------------------; 检测显存
	lda	#0	;
lpcvm04:	sta	VideoDataPort	; 依次设置视频页：00、80、C0、E0、F0、F8、FC、FE、FF
	ldx	#0	; 指向 VRAM $0000
	ldy	#0	;
	jsr	VRAMAddress	;
lpcvm08:	sta	$2007	;
	inx	;
	bne	lpcvm08	;
	sec	;
	ror	;
	bcc	lpcvm04	; 掩码右移补 1，直到 $FF
	lda	#0	; 视频页回到 00
lpcvm12:	sta	VideoDataPort	;
	ldx	#0	; 指向 VRAM $0000
	ldy	#0	;
	jsr	VRAMAddress	;
	bit	$2007	;
lpcvm16:	cmp	$2007	; 该页内容是否与页 00 相同
	bne	necvm04	;
	inx	;
	bne	lpcvm16	;
	beq	necvm08	; 相同：确定页大小
necvm04:	sec	; 不同：换下一个页掩码
	ror	;
	bcc	lpcvm12	;
necvm08:	eor	#$ff	;
	clc	;
	adc	#1	; 计算显存页大小
	pha	; 保存显存页大小
	ldx	#4	;
	stx	VideoDataPort	;
	inx	;
	stx	VideoDataPort+8	;
	jsr	WaitVSyn	; 等待垂直消隐后装入调色板与初始画面
	jsr	InitialPalette	;
	jsr	InitialScreen	;
	ldx	#>(ExamineLine*$20+$40+3+$2000)	; 检测项所在行的 VRAM 地址
	ldy	#<(ExamineLine*$20+$40+3+$2000)	;
	jsr	VRAMAddress	;
	ldx	#26	; 写 26 个分隔图块 $90
	lda	#$90	;
lpbeg12:	sta	$2007	;
	dex	;
	bne	lpbeg12	;
	jsr	OpenScreen	; 打开显示
	lda	#ExamineLine	;
	sta	$fc	; 设置显示行号
	ldx	#>(SysExamStg)	;
	ldy	#<(SysExamStg)	;
	lda	#12	; 显示“系统检测”标题（列 12）
	jsr	DisplayString	;
	lda	#ExamineLine+3	; 后续检测项的行号
	sta	$fc	;
;---------------------------------------;
	lda	#$40	; 硬复位软驱控制器
	sta	FDCResetPortO	;
;---------------------------------------; 检测主存（PRAM）容量
	ldx	#5-1	;
lpcpm04:	lda	MainMemoryStg,x	; 显示字符串“内存:”
	sta	$81,x	;
	dex	;
	bpl	lpcpm04	;
	lda	#5	; 显示列号 5
	sta	$fd	;
	lda	#0	;
lpcpm08:	sta	DRAMPagePort	; 依次设置 DRAM 页掩码：00、80、C0、E0、F0、F8、FC、FE、FF
	ldx	#0	;
lpcpm12:	sta	$9a00,x	;
	inx	;
	bne	lpcpm12	;
	sec	;
	ror	;
	bcc	lpcpm08	;
	lda	#0	; 页回到 00
lpcpm16:	ldx	#0	;
	sta	DRAMPagePort	;
lpcpm20:	cmp	$9a00,x	; 该页内容是否与页 00 相同
	bne	necpm04	;
	inx	;
	bne	lpcpm20	;
	beq	necpm08	; 相同：确定页数
necpm04:	sec	; 不同：换下一个页掩码
	ror	;
	bcc	lpcpm16	;
necpm08:	sta	$02	; 内存大小为 (~A+1) × 16K
	pha	; 保存 DRAM 页大小
	sta	$03	;
	lda	#0	;
	sta	FDCResetPortO	; 解除软驱硬复位
	sta	$00	; ($00) 指向 $8000
lpcpm24:	lda	#5	;
	sta	$80	;
	lda	#0	;
	sta	$ff	;
	lda	$02	;
	sec	;
	sbc	$03	;
	ldx	#4	;
lpcpm28:	asl	; 乘 16
	rol	$ff	;
	dex	;
	bne	lpcpm28	;
	clc	;
	adc	#16	;
	tay	;
	lda	$ff	;
	adc	#0	;
	tax	;
	lda	#$1c	; 单位字符 K
	jsr	DisplayChkRAM	;
	lda	#>($8000)	;
	sta	$01	;
	lda	$02	;
	sta	DRAMPagePort	;
	ldx	#$18	; 清除 $8000-$97FF
	ldy	#0	;
lpcpm32:	sta	($00),y	;
	iny	;
	bne	lpcpm32	;
	inc	$01	;
	dex	;
	bne	lpcpm32	;
	inc	$02	;
	bne	lpcpm24	;
	lda	#$08	; 软复位软驱控制器
	sta	FDCCtrlPortO	;
;---------------------------------------;
	ldx	#>($5800)	; 清除 $5800-$5FFF
	ldy	#<($5800)	;
	stx	$00+1	;
	sty	$00	;
	ldx	#>($6000-$5800)	;
	tya	;
lpmpg08:	sta	($00),y	;
	iny	;
	bne	lpmpg08	;
	inc	$01	;
	dex	;
	bne	lpmpg08	;
;    从后向前把程序复制到 $5800。
	lda	#>(MoveRAMEnding-$C000+$5800)
	sta	$01	;
	lda	#>(MoveRAMEnding)	;
	sta	$03	;
	lda	#0	;
	sta	$00	;
	sta	$02	;
	ldx	#>(MoveRAMEnding-$C000)	;
	ldy	#<(MoveRAMEnding)	;
lpmpg12:	dey	; Y 是否为 0
	cpy	#$ff	;
	bcc	nempg04	;
	dec	$01	;
	dec	$03	;
	dex	;
	bmi	nempg08	;
	clc	;
nempg04:	lda	($02),y	;
	sta	($00),y	;
	bcc	lpmpg12	;
nempg08:	ldx	#>(MoveRAMEnding-$C000+$5800)
	ldy	#<(MoveRAMEnding-$C000+$5800)
	stx	LowRAMBegin+1	; 程序在 $5800 的起始地址
	sty	LowRAMBegin	;
	pla	; 取出 DRAM 页号
	sta	DRAMPageHigh	;
	sta	DRAMPageNotUse	; DRAM 页号作为不使用的页（保护 $5800-$7FFF）
	pla	; 取出显存页号
	sta	$200	; 显存页号（同时写入 $200）
	sta	VideoPageNumber	;
	bit	FDCIRQPortI	; 读取 FDC 中断状态，影响 V 标志
	php	; 保存 V 标志，供 FDC 检测使用
	lda	#$0c	; 软驱解除复位
	sta	FDCCtrlPortO	;
	ldy	#0	;
lpppp:	dex	; 等待 FDC 复位完成
	bne	lpppp	;
	dey	;
	bne	lpppp	;
;---------------------------------------;
;    显示显存容量。
	ldx	#5-1	;
lpcrm04:	lda	VideoMemoryStg,x	; 显示字符串“显存:”
	sta	$81,x	;
	dex	;
	bpl	lpcrm04	;
	ldy	#19	; 显示列号 19
	sty	$fd	;
	lda	#0	;
lpcrm08:	pha	;
	ldx	#5	;
	stx	$80	;
	ldx	#0	;
	stx	$ff	;
	clc	;
	adc	#1	;
	asl	; 乘 2
	tay	;
	lda	#0	;
	rol	;
	tax	;
	cpy	#100	;
	bcc	necrm04	;
	lda	#19-1	;
	sta	$fd	;
necrm04:	lda	#$1c	; 单位字符 K
	jsr	DisplayChkRAM	;
	pla	;
	clc	;
	adc	#1	;
	cmp	$200	;
	bne	lpcrm08	;
	lda	#ExamineLine+6	;
	sta	$fc	;
;---------------------------------------;
	jsr	ResetKeyBoardDev	; 复位键盘
;---------------------------------------;
necfc04:	plp	; 复位期间是否有 IRQ
	bvs	necfc08	; 有 IRQ：没有软驱
	bit	FDCIRQPortI	; 复位后软驱是否产生 IRQ
	bvs	necfc12	;
necfc08:	jmp	necdc04	;
;    检测软驱：分别对 0、1 号驱动器寻道到 50 道再回到 0 道。
necfc12:	ldx	#>(FDCStg)	;
	ldy	#<(FDCStg)	;
	lda	#5	; 显示“软驱卡”
	jsr	DisplayString	;
	lda	#$80	;
	pha	;
	lda	#0	; 有软驱：0 号驱动器寻道
	sta	DiskIOZB+Drive	;
	jsr	OpenMotor	; 打开 0 号驱动器电机
	jsr	SoftDiskReset	;
	lda	#50	; 寻道到 50 道
	jsr	Seek	;
	bcs	necfc16	;
	jsr	WaitmSecond	; 等待 50 毫秒
	lda	#0	; 寻道回 0 道
	jsr	Seek	;
	bcs     necfc16	; 未确认
	ldx	#>(FDDAStg)	;
	ldy	#<(FDDAStg)	;
	lda	#14	;
	jsr	DisplayString	; 显示“软驱:A”
	pla	;
	ora	#1	; 记录 0 号驱动器存在
	pha	;
necfc16:	ldx	#1	; 1 号驱动器寻道
	stx	DiskIOZB+Drive	;
	dex	;
	stx	MotorStatus	;
	jsr	OpenMotor	; 打开 1 号驱动器电机
	lda	#50	; 寻道到 50 道
	jsr	Seek	;
	bcs	necfc20	;
	jsr	WaitmSecond	; 等待 50 毫秒
	lda	#0	; 寻道回 0 道
	jsr	Seek	;
	bcs	necfc20	;
	ldx	#>(FDDBStg)	;
	ldy	#<(FDDBStg)	;
	lda	#21	;
	jsr	DisplayString	; 显示“软驱:B”
	pla	;
	ora	#2	; 记录 1 号驱动器存在
	pha	;
necfc20:	pla	;
	sta	SDriveMessage	;
	and	#3	; 是否有软驱
	bne	necfc24	;
	ldx	#>(NOFDDStg)	;
	ldy	#<(NOFDDStg)	;
	lda	#21	;
	jsr	DisplayString	; 显示“没软驱”
necfc24:	inc	$fc	;
	inc	$fc	;
	inc	$fc	;
	lda	#0	;
	sta	MotorStatus	;
	lda	#$0c	; 关闭电机
	sta	FDCCtrlPortO	;
;---------------------------------------; 检测 PC 卡
necdc04:	lda	$ff50	; 检测 PC 卡是否存在
	ldx	#$0c	;
	stx	$ff50	;
	and	#$0f	;
	bne	necdc08	;
	lda	$ff50	;
	and	#$0f	;
	cmp	#$0c	;
necdc08:	lda	#$04	; 允许 PCDataPortIL 输入
	sta	$ff50	;
	ldx	#>(PCCStg)	; 存在：显示“微机通讯卡”
	ldy	#<(PCCStg)	;
	lda	#5	;
	jsr	DisplayString	;
	jsr	ConnectPCCard	; 返回 A=1 表示连接成功
	tax	;
	beq	necpc12	;
	ldx	#>(PCCConnectStg+2)	; 连接成功：显示“通讯”标题
	ldy	#<(PCCConnectStg+2)	;
	lda	#17+2	;
	jmp	necpc16	;
necpc12:	ldx	#>(PCCConnectStg)	; 连接失败：显示“没连通微机”
	ldy	#<(PCCConnectStg)	;
	lda	#17	;
necpc16:	jsr	DisplayString	;
	lda	#$80	; 记录 PC 卡已存在
	sta	PCCardMessage	;
	jmp	nebeg04	;
necpc20:	; 未被引用的标签
;---------------------------------------;
;    启动前的准备：关闭显示 DRAM 页保护，设置启动标记，显示“系统启动”标题，
;    启动 NMI 与键盘中断，然后进入启动流程。
nebeg04:	lda	#$ff	;
	sta	DRAMPagePort	;
	sta	DRAMPageSave	;
	lda	#$fe	;
	sta	BiosUsePage	;
	lda	#BootLine	;
	sta	$fc	;
	ldx	#>(SysBootStg)	;
	ldy	#<(SysBootStg)	;
	lda	#12	;
	jsr	DisplayString	;
	jsr	WaitVSyn	;
	ldx	#>(BootLine*$20+$40+3+$2000)	; 启动信息行的 VRAM 地址
	ldy	#<(BootLine*$20+$40+3+$2000)	;
	jsr	VRAMAddress	;
	ldx	#26	; 写 26 个分隔图块 $90
	lda	#$90	;
lpbeg16:	sta	$2007	;
	dex	;
	bne	lpbeg16	;
	stx	Addr2005	;
	jsr     RunInitial	; 修改 NMI、IRQ 向量并开启 NMI
	ldx	#>(KeyBoardNMI-$C000+$5800)	; NMI 中处理键盘
	ldy	#<(KeyBoardNMI-$C000+$5800)	;
	jsr	SetNMIVector	;
	jsr	OpenScreen	;
	lda	#$0e	;
	sta	Addr2001	;
;=======================================;
;    启动流程：依次尝试远程盘（PC 卡）、软驱，都失败时显示错误并等待按键后重试。
;---------------------------------------; 启动
SystemBoot:	;
	jsr	SoundBell	; 响铃提示
BootDiskIOBF = $0080	; 启动时磁盘 I/O 命令块的位置
BootDiskSBF = $0400	; 启动时读入扇区的缓冲区
;---------------------------------------; 从远程盘（PC 卡）启动
lpbot04:	lda	#$c0	; 不在 NMI 中检查键盘
	sta	NMICheckFlag	;
	lda	PCCardMessage	;
	bpl	nebot12	;
	lda	#BootLine+3	;
	sta	$fc	;
	ldx	#>(BootPCCStg)	; 显示“从微机通讯卡启动”
	ldy	#<(BootPCCStg)	;
	lda	#7	;
	jsr	CleanDisplayStg	;
	jsr	ConnectPCCard	; 返回 A=1 表示连接成功
	tax	;
	beq	nebot12	;
	ora	#$80	;
	sta	PCCardMessage	;
	lda	#BootOperation	; 执行启动功能
	jsr	SendByte	;
	jsr	PCCardServer	; 运行 PC 卡服务
	lda	#<(SMDOS_Begin)	; 调用地址指向 SMDOS_Begin
	sta	CallAddress-$C000+$5800+1
	lda	#>(SMDOS_Begin)	;
	sta	CallAddress-$C000+$5800+2
	lda	#$ff	; 远程盘启动标记
	jmp	nebot34	;
;---------------------------------------; 从软驱启动
nebot12:	lda	SDriveMessage	; 是否有软驱控制器
	bpl	nebot32	;
	and	#3	; 是否有驱动器
	beq	nebot32	;
	ldx	#>(BootFDCBStg)	;
	ldy	#<(BootFDCBStg)	;
	lsr	; 是否有 0 号驱动器
	bcc	nebot16	;
	lsr	; 有 0 号驱动器：优先从 0 号启动
	ldx	#>(BootFDCAStg)	;
	ldy	#<(BootFDCAStg)	;
nebot16:	and	#1	; 从 0 号或 1 号软驱启动
	pha	;
	lda	#BootLine+3	;
	sta	$fc	;
	lda	#9	;
	jsr	CleanDisplayStg	;
	pla	;
nebot24:	sta	BootDiskIOBF+Drive	;
	ldx	#0	;
	stx	BootDiskIOBF+Track	;
	stx	BootDiskIOBF+Head	;
	lda	#>(BootDiskSBF)	;
	sta	BootDiskIOBF+Buffer+1	;
	stx	BootDiskIOBF+Buffer	;
	inx	;
	stx	BootDiskIOBF+Sector	;
	stx	BootDiskIOBF+Sectors	;
	lda	#3	; 最多重试 3 次
	sta	BiosTemp5	;
lpbot12:	lda	#CMD_READ	; 读取启动扇区
	sta	BootDiskIOBF+Command	;
	ldx	#>(BootDiskIOBF)	;
	ldy	#<(BootDiskIOBF)	;
	jsr	DiskIO	;
	tax	; 读取是否成功
	beq	lpbot16	;
	dec	BiosTemp5	;
	beq	nebot32	;
	lda	#CMD_RESET	; 失败：复位软驱后重试
	sta	BootDiskIOBF+Command	;
	ldx	#>(BootDiskIOBF)	;
	ldy	#<(BootDiskIOBF)	;
	jsr	DiskIO	;
	jmp	lpbot12	;
;---------------------------------------; 无法启动
nebot32:	ldy	#<(BootFault)	;
	ldx	#>(BootFault)	;
	bne	nebot40	;
;    检查启动扇区的签名是否为 SMDOS。
lpbot16:	lda	BootDiskSBF+3,x	;
	cmp	SMDOS_String,x	;
	bne	nebot36	;
	inx	;
	cpx	#5	;
	bne	lpbot16	;
	lda	BootDiskSBF+1	; 签名正确：调用地址取启动扇区的起始地址
	clc	;
	adc	#2	;
	sta	CallAddress-$C000+$5800+1
	lda	#>(BootDiskSBF)	;
	sta	CallAddress-$C000+$5800+2
	lda	BootDiskIOBF+Drive	; 启动盘的驱动器号
nebot34:	pha	;
	jsr	DispInitial	;
	lda	#0	; 允许在 NMI 中检查键盘
	sta	NMICheckFlag	;
	pla	;
	jsr	CallToRAM-$C000+$5800	; 调用启动程序
	lda	#$c0	; 不在 NMI 中检查键盘
	sta	NMICheckFlag	;
nebot36:	ldy	#<(BootFDCErrStg)	; 显示“软驱插入系统盘”
	ldx	#>(BootFDCErrStg)	;
nebot40:	lda	#BootLine+3	;
	sta	$fc	;
	lda	#5	;
	jsr	CleanDisplayStg	;
	lda	#BootLine+3	;
	sta	$fc	;
	ldx	#>(PressKeyStg)	; 显示“按任意键”
	ldy	#<(PressKeyStg)	;
	lda	#19	;
	jsr	DisplayString	;
	lda	#0	; 允许在 NMI 中检查键盘
	sta	NMICheckFlag	;
	ldy	#0	; 读取键盘与手柄 1、2
	ldx	#$17	;
	jsr	ReadKey	; 等待任意键
	jmp	lpbot04	;

;======= 常驻部分 ======================================================;
;---------------------------------------;
;    设置 VRAM 地址。
VRAMAddress:	; 入口：X 为地址高字节，Y 为地址低字节
	bit	$2002	; 复位 PPU 地址锁存
	stx	$2006	;
	sty	$2006	;
	rts	;
;---------------------------------------; NMI 中输出显示缓冲区
; 缓冲区首字节的高两位决定类型：11 输出列表，10 输出端口，01 输出计数，00 输出单字符。
DisplayNMIROM:	;
	txa	; 保存 X、Y
	pha	;
	tya	;
	pha	;
	lda	#0	;
	sta	$2001	; 输出期间关闭显示
	ldy	DispBfHead	;
	ldx	#30	; 每次最多处理 30 个单元
	bit	$2002	;
	lda	$ff	;
	pha	;
lpdnr04:	lda	DisplayBuffer,y	;
	sta	$ff	; 暂存类型字节
	iny	;
	bit	$ff	;
	bpl	nednr32	; 最高位为 0：01 或 00 类型
;---------------------------------------; 11：输出列表
	bvc	nednr24	; 次高位为 0：10 类型
	pha	; 保存地址高字节
	and	#$3f	;
	sta	$2006	;
	lda	DisplayBuffer,y	;
	sta	$2006	;
	sta	DispBfHead	; 保存地址低字节
	iny	;
	lda	DisplayBuffer,y	; 字符个数
	sta	$ff	;
	pha	; 保存个数
lpdnr08:	iny	;
	lda	DisplayBuffer,y	; 每轮写 4 个字符，个数用完即退出
	sta	$2007	;
	dec	$ff	;
	beq	nednr20	;
	iny	;
	lda	DisplayBuffer,y	;
	sta	$2007	;
	dec	$ff	;
	beq	nednr20	;
	iny	;
	lda	DisplayBuffer,y	;
	sta	$2007	;
	dec	$ff	;
	beq	nednr20	;
	dex	;
	bne	lpdnr08	;
	lda	$ff	;
	sta	DisplayBuffer,y	;
	dey	;
	pla	; 取回个数
	sec	;
	sbc	$ff	; 已输出的字符数
	clc	;
	adc	DispBfHead	; 地址低字节加上已输出数
	sta	DisplayBuffer,y	;
	dey	;
	pla	; 取回地址高字节（不为 0）
	sta	DisplayBuffer,y	;
	jmp	nednr56	;
nednr20:	pla	; 跳过这一组的地址与个数
	pla	;
	jmp	nednr48	;
;---------------------------------------; 10：输出端口
nednr24:	lsr	;
	bcs	nednr28	;
	lda	Addr2000	; 10xxxxx0：修改 $2000 的低两位
	and	#$fc	;
	ora	DisplayBuffer,y	;
	sta	Addr2000	; 保存到 $2000 副本
	bcc	nednr48	; 总是跳转
nednr28:	lda	DisplayBuffer,y	; 10xxxxx1：设置滚动偏移
	sta	ScrollPanning	;
	bcs	nednr48	; 总是跳转
;---------------------------------------; 01：输出计数
nednr32:	bvc	nednr36	; 次高位为 0：00 类型
	and	#$3f	;
	sta	$2006	;
	lda	DisplayBuffer,y	;
	sta	$2006	;
	iny	;
	lda	DisplayBuffer,y	; 要写入的字符
	pha	;
	iny	;
	sty	$ff	; 保存计数的位置
	lda	DisplayBuffer,y	; 计数
	tay	;
	pla	;
lpdnr12:	sta	$2007	; 每轮写 4 个字符
	dey	;
	beq	nednr40	;
	sta	$2007	;
	dey	;
	beq	nednr40	;
	sta	$2007	;
	dey	;
	beq	nednr40	;
	sta	$2007	;
	dey	;
	beq	nednr40	;
	dex	;
	bne	lpdnr12	;
	tya	; 剩余计数
	ldy	$ff	; 恢复计数位置
	ldx	DisplayBuffer,y	;
	sta	DisplayBuffer,y	;
	txa	;
	sec	; 剩余的显示长度
	sbc	DisplayBuffer,y	;
	dey	;
	dey	;
	clc	;
	adc	DisplayBuffer,y	; 修改地址低字节
	sta	DisplayBuffer,y	;
	dey	;
	bcc	nednr56	; 总是跳转
;---------------------------------------; 00：输出单字符
nednr36:	sta	$2006	;
	lda	DisplayBuffer,y	; V 为 0
	sta	$2006	;
	iny	;
	lda	DisplayBuffer,y	;
	sta	$2007	;
	bvc	nednr48	; 总是跳转
nednr40:	ldy	$ff	;
nednr48:	iny	;
	cpy	DispBfTail	; 缓冲区是否已读完
	beq	nednr52	;
	dex	;
	beq	nednr56	;
	jmp	lpdnr04	;
nednr52:	lda	Addr2000	;
	and	#$fc	;
	ora	Addr2000N	;
	sta	Addr2000	;
nednr56:	sty	DispBfHead	; 保存显示位置
	pla	;
	sta	$ff	;
	lda	#0	;
	sta	$2005	;
	lda	ScrollPanning	;
	sta	$2005	;
	lda	Addr2000	;
	sta	$2000	;
	lda	Addr2001	;
	sta	$2001	;
	pla	;
	tay	;
	pla	;
	tax	;
	rts	;
;---------------------------------------; 关闭 NMI
DisableNMI:	;
	lda	Addr2000	;
	and	#$7f	;
	sta	$2000	;
	rts	;
;---------------------------------------; 开启 NMI
EnableNMI:	;
	lda	Addr2000	;
	ora	#$80	;
	sta	$2000	;
	rts	;
;---------------------------------------; 恢复 NMI 设置
RestoreNMI:	;
	lda	Addr2000	;
	sta	$2000	;
	rts	;

;---------------------------------------; 监视器功能跳转表（每项为入口地址减 1）
BranchAddress:	.word FunctionsReturn-1,	ReadByte-1	; 偏移 0
	.word SaveByte-1,		TargetGo-1	; 偏移 2
	.word ReadByteNext-1,		SaveByteNext-1	; 偏移 4
	.word GetBlock-1,		SaveBlock-1	; 偏移 6
	.word SetCurrentPage-1,	RunningInitial-1	; 偏移 8
	.word GetCurrentPage-1,	SaveBlockDirect-1	; 偏移 10
	.word IOOutByte-1,		IOInByte-1	; 偏移 12
	.word SaveVMEMBlock-1,	GetVMEMBlock-1	; 偏移 14
	.word ReadByteLast-1,		SaveByteLast-1	; 偏移 16
	.word DecodeVideoBlock-1,	SearchString-1	; 偏移 18
	.word CallROMBios-1,		CallDosIO-1	; 偏移 20
MonitorFuncs = *-BranchAddress	; 功能表字节数
;=============================================================================
;    按 A 中的偏移取出入口地址压栈，然后用 RTS 跳转。
JsrBranch:	;
	tax	;
	lda	BranchAddress+1,x	;
	pha	;
	lda	BranchAddress,x	;
	pha	;
	rts	;
;---------------------------------------; 进入监视器（ROM 中）
MonitorEntryROM:	;
	lda	#DebugOperation	; 通知 PC 进入调试模式
	jsr	SendByte	;
	lda	#$80	;
	sta	MonitorMode	;
	lda	#$80	; 开启 NMI
	sta	$2000	;
;=======================================;
;    循环接收 PC 发来的功能号并执行对应功能。
MainLoop:	;
	jsr	GetByte	;
	asl	;
	cmp	#MonitorFuncs	;
	bcs	MainLoop	; 功能号越界：忽略
	jsr	JsrBranch	;
	jmp	MainLoop	;
;=======================================;
;    弹出 MainLoop 的返回地址后返回，结束监视器循环。
FunctionsReturn:	;
	pla	;
	pla	;
	rts	;
;=======================================;
;    从 PC 读取块长度（高字节在前）和起始地址，把块内容逐字节发送给 PC。
GetBlock:	;
	jsr	GetByte	; 块长度高字节
	sta	BlockLength+1	;
	jsr	GetByte	; 块长度低字节
	sta	BlockLength	;
	jsr	ReadByte	;
lpgbk04:	ldx	BlockLength	;
	bne	negbk04	;
	dec	BlockLength+1	; 低字节为 0 时向高字节借位
negbk04:	dex	; 长度减一
	stx	BlockLength	;
	txa	;
	ora	BlockLength+1	;
	beq	negbk08	; 长度为 0：结束
	jsr	ReadByteNext	;
	jmp	lpgbk04	;
negbk08:	rts	;
;=======================================;
;    地址加一后读取下一个字节（低字节回绕时进位到高字节）。
ReadByteNext:	;
	ldx	RBlockPoint+1	;
	ldy	RBlockPoint	;
	iny	;
	bne	nerbn04	;
	inx	;
	stx	RBlockPoint+1	;
nerbn04:	sty	RBlockPoint	;
	jmp	ReadOne	;
;=======================================;
;    地址减一后读取（低字节为 0 时向高字节借位）。
ReadByteLast:	;
	ldx	RBlockPoint+1	;
	ldy	RBlockPoint	;
	bne	nerbl04	;
	dex	;
	stx	RBlockPoint+1	;
nerbl04:	dey	;
	sty	RBlockPoint	;
	jmp	ReadOne	;
;=======================================;
;    从 PC 读取地址（高字节在前），保存为块指针后读取该字节。
ReadByte:	;
	jsr	GetByte	;
	tax	;
	jsr	GetByte	;
	tay	;
	stx	RBlockPoint+1	;
	sty	RBlockPoint	;
;---------------------------------------;
;    入口：X 为地址高字节，Y 为地址低字节。
ReadOne:	cpx	#>($8000)	; 地址 >= $8000？
	bcs	neroe04	;
	cpy	#$fe	; 低于 $8000：低字节小于 $FE 时用 $FE 指针读取
	bcc	neroe16	;
	bcs	neroe12	; 低于 $8000 且低字节不小于 $FE：用 $FC 指针读取
neroe04:	cpx	#>($C000)	; 地址 >= $C000？
	bcc	neroe08	;
	txa	;
	and	#$bf	;
	tax	;
	lda	#$ff	; $C000 以上：映射到 $FF 页
	bne	neroe10	;
neroe08:	lda	DRAMPageSave	; $8000-$BFFF：映射到当前页
neroe10:	sta	DRAMPagePort	; 切换到当前页
;---------------------------------------;
neroe12:	lda	 $fc	; 保存 $FC、$FD
	pha	;
	lda	 $fd	;
	stx	 $fd	;
	tax	;
	sty	 $fc	;
	ldy	#0	;
	lda	($fc),y	; 读取该字节
	stx	 $fd	;
	jsr	SendByte	;
	pla	;
	sta	 $fc	;
	rts	;
neroe16:	lda	 $fe	; 保存 $FE、$FF
	pha	;
	lda	 $ff	;
	stx	 $ff	;
	tax	;
	sty	 $fe	;
	ldy	#0	;
	lda	($fe),y	; 读取该字节
	stx	 $ff	;
	jsr     SendByte	;
	pla	;
	sta	 $fe	;
	rts	;
;=======================================;
;    从 PC 读取块长度与起始地址，逐字节接收并写入内存。
SaveBlock:	;
	jsr	GetByte	; 块长度高字节
	sta	BlockLength+1	;
	jsr	GetByte	; 块长度低字节
	sta	BlockLength	;
	jsr	SaveByte	;
lpsbk04:	ldx	BlockLength	;
	bne	nesbk04	;
	dec	BlockLength+1	; 低字节为 0 时向高字节借位
nesbk04:	dex	; 长度减一
	stx	BlockLength	;
	txa	;
	ora	BlockLength+1	;
	beq	nesbk08	; 长度为 0：结束
	jsr	SaveByteNext	;
	jmp	lpsbk04	;
nesbk08:	rts	;
;=======================================;
;    地址加一后接收并保存下一个字节（低字节回绕时进位到高字节）。
SaveByteNext:	;
	ldx	SBlockPoint+1	;
	ldy	SBlockPoint	;
	iny	;
	bne	nesbn04	;
	inx	;
	stx	SBlockPoint+1	;
nesbn04:	sty	SBlockPoint	;
	jmp	SaveOne	;
;=======================================;
;    地址减一后接收并保存（低字节为 0 时向高字节借位）。
SaveByteLast:	;
	ldx	SBlockPoint+1	;
	ldy	SBlockPoint	;
	bne	nesbl04	;
	dex	;
	stx	SBlockPoint+1	;
nesbl04:	dey	;
	sty	SBlockPoint	;
	jmp	SaveOne	;
;=======================================;
;    从 PC 读取地址（高字节在前），保存为块指针后接收并保存一个字节。
SaveByte:	;
	jsr	GetByte	;
	tax	;
	jsr	GetByte	;
	tay	;
	stx	SBlockPoint+1	;
	sty	SBlockPoint	;
;---------------------------------------;
;    入口：X 为地址高字节，Y 为地址低字节。
SaveOne:	cpx	#>($8000)	; 地址 >= $8000？
	bcs	nesoe04	;
	cpy	#$fe	; 低于 $8000：低字节小于 $FE 时用 $FE 指针写入
	bcc	nesoe16	;
	bcs	nesoe12	; 低于 $8000 且低字节不小于 $FE：用 $FC 指针写入
nesoe04:	cpx	#>($C000)	; 地址 >= $C000？
	bcc	nesoe08	;
	txa	;
	and	#$bf	;
	tax	;
	lda	#$ff	; $C000 以上：映射到 $FF 页
	bne	nesoe10	;
nesoe08:	lda	DRAMPageSave	; $8000-$BFFF：映射到当前页
nesoe10:	sta	DRAMPagePort	; 切换到当前页
;---------------------------------------;
nesoe12:	lda	 $fc	; 保存 $FC、$FD
	pha	;
	lda	 $fd	;
	stx	 $fd	;
	tax	;
	sty	 $fc	;
	jsr	GetByte	; 接收该字节
	ldy	#0	;
	sta	($fc),y	;
	stx	 $fd	; 恢复 $FC、$FD
	pla	;
	sta	 $fc	;
	rts	;
nesoe16:	lda	 $fe	; 保存 $FE、$FF
	pha	;
	lda	 $ff	;
	stx	 $ff	;
	tax	;
	sty	 $fe	;
	jsr      GetByte	; 接收该字节
	ldy      #0	;
	sta      ($fe),y	;
	stx	 $ff	;
	pla	;
	sta	 $fe	;
	rts	;
;=======================================;
;    从 PC 接收数据写入显存（写入期间关闭显示，结束后恢复）。
SaveVMEMBlock:	;
	lda	#0	; 关闭显示，保证写入正确
	sta	$2001	;
	jsr	GetByte	;
	tax	;
	jsr	GetByte	;
	tay	;
	jsr	VRAMAddress	; 设置显存地址指针
	jsr	GetByte	; 块长度：X 为高字节，Y 为低字节
	tax	;
	jsr	GetByte	;
	tay	;
	beq	lpsvb04	;
	inx	; 低字节不为 0：外层循环加一次
lpsvb04:	jsr	GetByte	;
	sta	$2007	;
	dey	;
	bne	lpsvb04	;
	dex	;
	bne	lpsvb04	;
	lda	#0	;
	sta	$2005	;
	lda	Addr2005	;
	sta	$2005	;
	lda	Addr2001	;
	sta	$2001	;
	rts	;
;=======================================;
;    从显存读出数据发送给 PC（读出期间关闭显示，结束后恢复）。
GetVMEMBlock:	;
	lda	#0	; 关闭显示，保证读出正确
	sta	$2001	;
	jsr	GetByte	;
	tax	;
	jsr	GetByte	;
	tay	;
	jsr	VRAMAddress	; 设置显存地址指针
	jsr	GetByte	; 块长度：X 为高字节，Y 为低字节
	tax	;
	jsr	GetByte	;
	tay	;
	beq	negvb04	;
	inx	; 低字节不为 0：外层循环加一次
negvb04:	lda	$2007	; 空读一次，丢弃预读数据
lpgvb04:	lda	$2007	;
	jsr	SendByte	;
	dey	;
	bne	lpgvb04	;
	dex	;
	bne	lpgvb04	;
	lda	#0	;
	sta	$2005	;
	lda	Addr2005	;
	sta	$2005	;
	lda	Addr2001	;
	sta	$2001	;
	rts	;
;=======================================;
;    从 PC 读取页号并设置 DRAM 页；页号为 0 且保护页非 0 时直接使用保护页。
SetCurrentPage:	;
	jsr	GetByte	;
	tax	;
	bne	nescp04	;
	lda	BiosUsePage	;
	bne	nescp12	;
nescp04:	ora	DRAMPageHigh	;
	cmp	DRAMPageNotUse	; 与不使用的页相同？
	bne	nescp08	;
	ror	; 是：右移一位，C 置 1 表示禁用该页
nescp08:	cmp	BiosUsePage	; 与保护页相同？
	bne	nescp12	;
	lda	DRAMPageNotUse	; 是：改用不使用的页
nescp12:	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	rts	;
;=======================================;
;    发送当前 DRAM 页号。
GetCurrentPage:	;
	jsr	GetPage	;
	jsr	SendByte	;
	rts	;
;=======================================;
;    向 IO 端口写入一个字节（地址 $4800-$4803、$5800-$5807）。
IOOutByte:
	jsr	GetByte	; 地址高字节（未使用）
	tax	;
	jsr	GetByte	; 地址低字节
	tay	;
	jsr	GetByte	; 要写入的数据
	sta	$ff00,y	;
	rts	;
;=======================================;
;    从 IO 端口读取一个字节发送给 PC（地址 $4800-$4803、$5800-$5803）。
IOInByte:	;
	jsr	GetByte	; 地址高字节（未使用）
	tax	;
	jsr	GetByte	; 地址低字节
	tay	;
	lda	$ff00,y	;
	jmp	SendByte	;
;=======================================;
;    响应打印机接口中断：先清除 P-ERROR，等待 P-STB 为高后再置位。
PCINTAccess:	;
	lda	#$00	; 置 0 表示收到中断
	sta	PCDaCtPortO	;
lpnir04:	lda	PCCtrlPortI	; 等待 P-STB 为高（读取前半字节）
	and	#8	;
	beq	lpnir04	;
	lda	#$10	; 置 1，返回接收数据
	sta	PCDaCtPortO	;
;=======================================;
;    BRK 中断：把寄存器与堆栈内容发送给 PC，然后恢复向量并回到主循环。
BRKAccess:	;
	txa	;
	pha	; 保存 X
	sei	;
	cld	;
	jsr	DisableNMI	; 关闭 NMI
	lda	#BRKOperation	; BRK 标记
	jsr	SendByte	;
	tya	; Y
	jsr	SendByte	;
	pla	; X
	jsr	SendByte	;
	lda	BRKASave	; A
	jsr	SendByte	;
	tsx	;
	lda	$0101,x	; 标志寄存器
	jsr	SendByte	;
	lda	DRAMPageSave	; 页号：保护页发送 0，不使用页发送保护页号
	cmp	BiosUsePage	;
	bne	nebas04	;
	lda	#0	;
nebas04:	cmp	DRAMPageNotUse	;
	bne	nebas08	;
	lda	BiosUsePage	;
nebas08:	jsr	SendByte	;
	lda	$0102,x	; PC 低字节
	jsr	SendByte	;
	lda	$0103,x	; PC 高字节
	jsr	SendByte	;
	inx	; 跳过标志与 PC（共 3 字节）
	inx	;
	inx	;
	txs	;
	txa	;
	jsr	SendByte	; 堆栈指针 S
	lda	VectorSet	; NMI 向量已设置？
	beq	nebas12	;
	lda	#$ff	; 切换到最后一页
	sta	DRAMPagePort	;
	ldx	BRKAddressSave-$C000+$5800+1	; 恢复 BRK 向量
	ldy	BRKAddressSave-$C000+$5800
	stx	$bfff	;
	sty	$bffe	;
	ldx	NMIAddressSave-$C000+$5800+1	; 恢复 NMI 向量
	ldy	NMIAddressSave-$C000+$5800
	stx	$bffb	;
	sty	$bffa	;
nebas12:	jsr	EnableNMI	; 开启 NMI（用于关闭软驱马达）
	jmp	MainLoop	;
;=======================================;
;    从 PC 读取堆栈、程序计数器、页号与寄存器，恢复现场并跳转到目标程序。
TargetGo:	;
	jsr	GetByte	; 堆栈指针 S
	tax	;
	txs	;
	jsr	GetByte	; PC 高字节
	tax	;
	pha	;
	jsr	GetByte	; PC 低字节
	pha	;
	lda	#$ff	; 切换到最后一页
	sta	DRAMPagePort	;
	lda	VectorSet	; NMI 向量已设置？
	beq	netgo08	;
	ldx	$bfff	; 保存并设置 BRK 向量
	ldy	$bffe	;
	stx	BRKAddressSave-$C000+$5800+1
	sty	BRKAddressSave-$C000+$5800
	ldx	$bffb	; 保存并设置 NMI 向量
	ldy	$bffa	;
	stx	NMIAddressSave-$C000+$5800+1
	sty	NMIAddressSave-$C000+$5800
netgo08:	ldx	#>(BRKINTEntry-$C000+$5800)	; BRK 向量指向 BRKINTEntry
	ldy	#<(BRKINTEntry-$C000+$5800)	;
	stx	$bfff	;
	sty	$bffe	;
	ldx	#>(NMIEntry-$C000+$5800)	; NMI 向量指向 NMIEntry
	ldy	#<(NMIEntry-$C000+$5800)	;
	stx	$bffb	;
	sty	$bffa	;
	jsr	GetByte	; 页号
	cmp	BiosUsePage	;
	bne	netgo12	;
	lda	DRAMPageNotUse	;
netgo12:	tax	;
	bne	netgo16	;
	lda	BiosUsePage	; 页号为 0：使用保护页
netgo16:	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	jsr	GetByte	; 标志寄存器
	pha	;
;---------------------------------------;
	jsr	GetByte	; X
	tax	;
	jsr	GetByte	; Y
	tay	;
	jsr	GetByte	; A
	pha	;
	lda	#$40	; 关闭 CPU 内部中断
	sta	$4017	;
	lda	$2002	; 恢复 $2005、$2001
	sta	$2005	;
	lda	Addr2005	;
	sta	$2005	;
	lda	Addr2001	;
	sta	$2001	;
	jsr	RestoreNMI	;
	jmp	TargetGoExit-$C000+$5800	;
;=======================================;
;    从 PC 读取显存地址后发送显存比较结果。
DecodeVideoBlock:	;
	jsr	GetByte	; 地址高字节
	tax	;
	jsr	GetByte	; 地址低字节
	tay	;
	jsr	SendVRAMCmp	;
	rts	;
;=======================================;
;    运行初始化：从 PC 读取标志，按标志更新不使用页。
RunningInitial:	;
	jsr	GetByte	; 下一个标志
	pha	;
	tax	;
	jsr	RunInitial	;
	pla	;
	and	#4	; 是否带有不使用页的数据
	beq	neril04	;
	jsr	GetByte	; 是：读取不使用页号
	ora	DRAMPageHigh	;
	sta	DRAMPageNotUse	;
neril04:	rts	;
;=======================================;
;    从内存中搜索字符串。
;---------------------------------------;
SearchString:	;
	lda	DRAMPageSave	;
	sta	DRAMPagePort	;
	lda	 $ff	;
	pha	; 保存 $FF、$FE
	lda	 $fe	;
	pha	;
	jsr	GetByte	; 搜索串长度
	sta	StringLength	;
	tay	;
	ldx	#0	;
lpssg04:	jsr	GetByte	; 读入搜索串
	sta	StringBuffer,x	;
	inx	;
	dey	;
	bne	lpssg04	;
	jsr	GetByte	; 搜索起始地址：高字节为 $FF
	sta	 $ff	;
	jsr	GetByte	; 低字节为 $FE
	sta	 $fe	;
	jsr	GetByte	; 搜索范围高字节
	sta	BlockLength+1	;
	jsr	GetByte	; 搜索范围低字节
	tax	;
	beq	nessg04	;
	inc	BlockLength+1	; 低字节非 0 时高字节加一
nessg04:	sta	BlockLength	;
	jsr	DisableNMI	;
	ldy	#0	;
	jmp	SearchStringRAM-$C000+$5800	; 转入 RAM 中的搜索过程
SearchStringRET:	;
	jsr	RestoreNMI	;
	lda	#$ff	; 发送三个 $FF 作为搜索结束标记
	jsr	SendByte	;
	lda	#$ff	;
	jsr	SendByte	;
	lda	#$ff	;
	jsr	SendByte	;
	pla	;
	sta	 $fe	;
	pla	;
	sta	 $ff	;
	rts	;

;=======================================;
;    调试：调用 ROM BIOS 功能，并把 A、Y、X 发回 PC。
CallROMBios:	;
	jsr	GetByte	; X
	tax	;
	jsr	GetByte	; Y
	tay	;
	jsr	GetByte	; A
	jsr	ROMBios	;
	jsr	SendByte	; A
	tya	;
	jsr	SendByte	; Y
	txa	;
	jmp	SendByte	; X

;=======================================;
;    调试：调用 SB-DOS 的 IO 入口，并把标志、A、Y、X 发回 PC。
CallDosIO:	;
	jsr	GetByte	; X
	tax	;
	jsr	GetByte	; Y
	tay	;
	jsr	GetByte	; A
	pha	;
	lda	#<(DosIOEntry)	;
	sta	CallAddress-$C000+$5800+1
	lda	#>(DosIOEntry)	;
	sta	CallAddress-$C000+$5800+2
	pla	;
	jsr	CallToRAM-$C000+$5800	;
	pha	;
	php	;
	pla	;
	jsr	SendByte	; 标志
	pla	;
	jsr	SendByte	; A
	tya	;
	jsr	SendByte	; Y
	txa	;
	jmp	SendByte	; X

;=============================================================================
;    BIOS 功能号表，每项为入口地址减 1；注释为功能号（十六进制）。
BIOSBranchAddr:	.word SetPageMode-1,          SetPage-1-$C000+$5800	;1
	.word GetPage-1-$C000+$5800,  SetAV10-1	;3
	.word RunInitial-1,           LowRAMFree-1	;5
	.word DiskIO-1,               GetDRAMSize-1	;7
	.word SetPageNoUse-1,         SetVector-1	;9
	.word GetVRAMSize-1,          ReadKey-1	;b
	.word MoveBlock-1,            FileIOServer-1	;d
	.word WaitVSynCnt-1,          LowRAMAlloc-1	;f
	.word FaultError-1,           ExchBlock-1	;10、11
	.word SplitScreen-1,          SetVideoPage-1	;13
	.word GetDriveMessage-1,      LoadVideoData-1	;15
	.word SetNMIVector-1,         GetNMIVector-1	;17
	.word DeviceEntryROM-1,       RstVector-1	;19
	.word GetCardVer-1,           DisplayTTY-1	;1b
	.word SetCursor-1,            DisplayChars-1	;1d
	.word GetCursor-1,            SetVectorRTI-1	;1f
	.word GetPagedChar-1,         SetPagedChar-1	;20、21
	.word WaitDisplay-1,          SendPCByte-1	;23
	.word GetByte-1,              UnzipBlock-1	;25
	.word SetDiskDspOff-1,        SetSEndVector-1	;27
	.word DispInitial-1	;29
CommandLast2 = *-BIOSBranchAddr	; 功能表字节数

;=======================================;
;    按 A 中的功能号调用 BIOS 功能（功能号 255 返回版本与日期）。
;    入口：A 为功能号。
ROMBios:	cmp	#BiosDispInitial	;
	bne	nerbs08	;
	sec	;
	sbc	#BiosDispInitial-BiosSetSEndVector-1	; DispInitial 映射到表尾
nerbs08:	asl	; 功能号转为表偏移（每项两字节）
	bcs	nerbs12	; 功能号不小于 128：单独处理
	cmp	#CommandLast2	;
	bcs	nerbs16	;
	stx	BiosTemp1	; 保存 X
	tax	;
	lda	BIOSBranchAddr+1,x	;
	pha	; 压入目标地址，用 RTS 跳转
	lda	BIOSBranchAddr,x	;
	pha	;
	ldx	BiosTemp1	; 恢复 X
	rts	;
nerbs12:	cmp	#254	; 功能号 255 左移后为 254
	bne	nerbs16	;
;---------------------------------------;
;    功能号 255：输出 BIOS 版本与日期。
;    输出：A 为版本号，X 为年，Y 为月。
	lda	#$40	; 与 XNR 版本兼容
	ldx	#BiosYear	;
	ldy	#BiosMonth	;
nerbs16:	rts	;
;---------------------------------------;

;=======================================;
;    设置软驱显示开关（X 为开关值）。
SetDiskDspOff:	;
	stx	DiskDisplayOff	;
	rts	;
;=======================================;
;    设置 SEND 中断向量，并记录当前 DRAM 页。
SetSEndVector:	;
	stx	SEndAccessSave-$C000+$5800+1
	sty	SEndAccessSave-$C000+$5800
	ldx	DRAMPageSave	;
	stx	INTDRAMPage	;
	ldx	#>(SEndAccessRAM-$C000+$5800)
	ldy	#<(SEndAccessRAM-$C000+$5800)
	stx	SEndAddressSave-$C000+$5800+1
	sty	SEndAddressSave-$C000+$5800
	rts	;
;=======================================;
;    致命错误：持续响铃，不再返回。
FaultError:	;
lpfer04:	jsr	SoundBell	;
	jmp	lpfer04	;

;=======================================;
;    把 X 的内容作为字节发送给 PC。
SendPCByte:	;
	txa	;
;---------------------------------------;
;    入口：A 为要发送的字节。
SendByte:	;
	pha	;
lpsbe04:	lda	PCCtrlPortI	; 等待 P-STB 为低（对方可接收）
	and	#8	;
	bne	lpsbe04	;
	pla	; 取出字节，发送低半字节
	pha	;
	and	#$0f	;
	sta	PCDaCtPortO	; P-ERROR 置 0，数据就绪
lpsbe08:	lda	PCCtrlPortI	; 等待 P-STB 为高（对方已读取）
	and	#8	;
	beq	lpsbe08	;
	lda	#$10	; 置 1，准备发送高半字节
	sta	PCDaCtPortO	;
lpsbe12:	lda	PCCtrlPortI	; 等待 P-STB 为低（对方可接收）
	and	#8	;
	bne	lpsbe12	;
	pla	; 取出字节，发送高半字节
	lsr	;
	lsr	;
	lsr	;
	lsr	; 高半字节移到低 4 位
	sta	PCDaCtPortO	; P-ERROR 置 0，数据就绪
lpsbe16:	lda	PCCtrlPortI	; 等待 P-STB 为高（对方已接收）
	and	#8	;
	beq	lpsbe16	;
	lda	#$10	; 置 1，传输结束
	sta	PCDaCtPortO	;
	rts	;
;---------------------------------------;
;    出口：A 为接收到的字节。
;---------------------------------------;
GetByte:	;
lpgbe04:	lda	PCCtrlPortI	; 等待数据就绪（P-STB 为低）
	and	#8	;
	bne	lpgbe04	;
	lda	PCDataPortIH	; 读取高半字节
	and	#$f0	;
	sta	GetByteSave	;
	lda	PCDataPortIL	; 读取低半字节
	and	#$0f	;
	ora	GetByteSave	;
	eor	#$8b	; 与 PC 端约定的异或值
	pha	;
	lda	#$00	; 置 0，表示已收到数据
	sta	PCDaCtPortO	;
lpgbe08:	lda	PCCtrlPortI	; 等待 P-STB 为高（对方结束传输）
	and	#8	;
	beq	lpgbe08	;
	lda	#$10	; 置 1，表示接收结束
	sta	PCDaCtPortO	;
	pla	;
	rts	;
;=======================================;
;    从 PC 读取块长度与起始地址，把内存块逐字节发送给 PC（页为当前 DRAM 页）。
;    占用 $FE、$FF 作为指针。
GetBlockDirect:	;
	jsr	GetByte	; 块长度高字节
	tay	;
	jsr	GetByte	; 块长度低字节
	tax	;
	beq	negbd04	;
	iny	; 低字节非 0：高字节加一
negbd04:	lda	$fd	; 保存 $FD
	pha	;
	sty	$fd	; 外层计数
	jsr	GetByte	; 起始地址高字节
	sta	$ff	;
	jsr	GetByte	; 起始地址低字节
	tay	;
	lda	#0	;
	sta	$fe	;
	lda	DRAMPageSave	;
	sta	DRAMPagePort	; 切换到当前页
lpgbd04:	lda	($fe),y	;
	jsr	SendByte	;
	iny	;
	bne	negbd08	;
	inc	 $ff	; 跨页时高字节加一
negbd08:	dex	; 内层计数减一
	bne	lpgbd04	;
	dec	$fd	; 外层计数减一
	bne	lpgbd04	;
	pla	;
	sta	$fd	; 恢复 $FD
	rts	;
;=======================================;
;    从 PC 读取块长度与起始地址，逐字节接收并写入内存（页为当前 DRAM 页）。
;    占用 $FE、$FF 作为指针。
SaveBlockDirect:	;
	jsr	GetByte	; 块长度高字节
	tay	;
	jsr	GetByte	; 块长度低字节
	tax	;
	beq	nesbd04	;
	iny	; 低字节非 0：高字节加一
nesbd04:	lda	$fd	; 保存 $FD
	pha	;
	sty	$fd	; 外层计数
	jsr	GetByte	; 起始地址高字节
	sta	$ff	;
	jsr	GetByte	; 起始地址低字节
	tay	;
	lda	#0	;
	sta	$fe	;
	lda	DRAMPageSave	;
	sta	DRAMPagePort	; 切换到当前页
lpsbd04:	jsr	GetByte	;
	sta	($fe),y	;
	iny	;
	bne	nesbd08	;
	inc	$ff	; 跨页时高字节加一
nesbd08:	dex	; 内层计数减一
	bne	lpsbd04	;
	dec	$fd	; 外层计数减一
	bne	lpsbd04	;
	pla	;
	sta	$fd	; 恢复 $FD
	rts	;
;---------------------------------------;
;    PC 卡服务循环：按功能号执行请求，功能号为 0 时返回。
PCCardServer:	;
lppcs04:	jsr	GetByte	;
	cmp	#8	; 8：设置页
	bne	nepcs04	;
	jsr	GetByte	; 页号
	tax	;
	jsr	SetPage-$C000+$5800	;
	jmp	lppcs04	;
nepcs04:	cmp	#11	; 11：保存块（直接）
	bne	nepcs08	;
	jsr	SaveBlockDirect	;
	jmp	lppcs04	;
nepcs08:	cmp	#21	; 21：读取块（直接）
	bne	nepcs12	;
	jsr	GetBlockDirect	;
	jmp	lppcs04	;
nepcs12:	cmp	#10	; 10：读取页号
	bne	nepcs16	;
	jsr	GetPage	;
	jsr	SendByte	;
	jmp	lppcs04	;
nepcs16:	tax	; 功能号为 0：返回
	bne	lppcs04	;
	rts	;
;=======================================;
;    文件 IO 服务。输入：X 为参数高字节，Y 为参数低字节。
;    功能用法：
;    GetByte()、SendByte()；
;    08：SetCurrentPage()；11：SaveBlockDirect()；21：GetBlockDirect()。
;---------------------------------------;
FileIOServer:	;
	lda	$ff	; 保存 $FC-$FF
	pha	;
	lda	$fe	;
	pha	;
	lda	$fd	;
	pha	;
	lda	$fc	;
	pha	;
	stx	$fd	;
	sty	$fc	;
	lda	DRAMPageSave	; 保存原页号
	pha	;
	tax	;
	lda	$fd	;
	cmp	#>($c000)	;
	bcc	nefio04	; 高字节小于 $C0：不改变页
	and	#$bf	;
	sta	$fd	;
	ldx	#$ff	; 高字节不小于 $C0：映射到 $FF 页
	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
nefio04:	ldy	#HBUF+1	; 读取 HBUF 高字节
	lda	($fc),y	;
	cmp	#>($c000)	; 是否不小于 $C0
	php	;
	bcc	nefio08	;
	and	#$bf	; 是：修正高字节
	sta	($fc),y	;
	ldy	#HPAGE	;
	lda	#$ff	; 设为 $FF 页
	sta	($fc),y	;
nefio08:	txa	; 保存句柄页号
	pha	;
	lda	#FileOperation	; 文件操作标记
	jsr	SendByte	;
	ldy	#0	;
lpfio08:	lda	($fc),y	; 发送 16 字节参数
	jsr	SendByte	;
	iny	;
	cpy	#16	;
	bne	lpfio08	;
;---------------------------------------;
	jsr	PCCardServer	; 处理 PC 端的请求
	pla	; 取回句柄页号
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	ldy	#0	;
lpfio12:	jsr	GetByte	; 接收 14 字节结果
	sta	($fc),y	;
	iny	;
	cpy	#14	;
	bne	lpfio12	;
	plp	; 取回比较结果
	bcc	nefio12	;
	ldy	#HBUF+1	; 高字节加 $40，还原 $8000 以上的地址
	lda	($fc),y	;
	clc	;
	adc	#>($c000-$8000)	;
	sta	($fc),y	;
nefio12:	pla	; 恢复原页号
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	pla	;
	sta	$fc	;
	pla	;
	sta	$fd	;
	pla	;
	sta	$fe	;
	pla	;
	sta	$ff	;
SetPageMode:	; 入口仅返回
	rts	;
;---------------------------------------;
;    按 $FE/$FF 指向的地址修正页：高字节不小于 $C0 时映射到 $FF 页。
ModifyAddressFE:	;
	lda	$fe+1	;
	cmp	#>($c000)	;
	bcc	nemaf04	;
	and	#$bf	; 去掉 $40 位，映射到 $80-$BF 范围
	sta	$fe+1	;
	ldx	#$ff	; X 记录映射到 $FF 页
nemaf04:	lda	DRAMPageSave	;
	jsr	SetPage-$C000+$5800	;
	tax	; X 为要恢复的页号
	rts	;
;=======================================;
;    按页读取字符。
;    入口：X|($fe),y 为字符地址（X 为高字节）。
;    出口：A 为字符。保留 $fe、y。
GetPagedChar:	;
	lda	$fe+1	;
	sta	BiosTemp1	;
	jsr	ModifyAddressFE	;
	lda	($fe),y	;
	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
	ldx	BiosTemp1	;
	stx	$fe+1	;
	rts	;
;=======================================;
;    按页写字符。
;    入口：X|($fe),y 为字符地址（X 为高字节），$FD 为要写入的字符。
;    保留 $fe、y。
SetPagedChar:	;
	lda	$fe+1	;
	sta	BiosTemp1	;
	jsr	ModifyAddressFE	;
	lda	$fd	;
	sta	($fe),y	;
	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
	ldx	BiosTemp1	;
	stx	$fe+1	;
	rts	;
;=======================================;
;    设置页。入口：X 为页号。保留 A、Y。
SetPageROM:	;
	cpx	BiosUsePage	;
	bne	nespr04	;
	ldx	DRAMPageNotUse	; 与保护页相同：改用不使用的页
nespr04:	cpx	#0	;
	bne	nespr08	;
	ldx	BiosUsePage	; 页号为 0：使用保护页
nespr08:	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
	rts	;

;=======================================;
;    设置 AV10，入口 X 的位 1、0：
;    00：AV10 = AV10；01：AV10 = AV11；10：AV10 = 0；11：AV10 = 1。
SetAV10:	txa	;
	and	#3	;
	sta	BiosTemp1	;
	lda	VideoCtrlSave	;
	and	#$fc	; 清除 AV 位（低两位）
	ora	BiosTemp1	;
	sta	VideoCtrlSave	;
	rts	;
;---------------------------------------;
;    初始化四个显示页：VROM $0800-$0FFF 分别映射到显示页 4、5、6、7。
InitVideoPort:	;
	ldx	#4	; 页地址的 0、8、16、24 对应显示页 4-7
	stx	VideoDataPort	;
	inx	;
	stx	VideoDataPort+8	;
	inx	;
	stx	VideoDataPort+16	;
	inx	;
	stx	VideoDataPort+24	;
	rts	;
;=======================================;
;    程序运行初始化。入口：X 为标志，位 1、0 决定 AV10：
;    00：AV10 = AV10；01：AV10 = AV11；10：AV10 = 0；11：AV10 = 1。
RunInitial:	;
	lda	VideoCtrlSave	;
	and	#$f8	;
	sta	VideoCtrlSave	;
	txa	;
	and	#$03	; 取位 1、0
	ora	VideoCtrlSave	;
	sta	VideoCtrlSave	;
	lda	#0	;
	sta	VectorSet	; 没有用户的 NMI 与 INT 向量
	sta	SplitPoint	; 不使用分屏
	sta	CurColumn	;
	sta	CurLine	;
	sta	ScrollPanning	;
	sta	ScrollPanningN	;
	sta	Addr2000N	;
	sta	DispBfHead	;
	sta	DispBfTail	;
	sta	DiskDisplayOff	;
	jsr	DisableNMI	;
	sei	; 禁止中断
	lda	#$ff	; 切换到最后一页
	sta	DRAMPagePort	;
	ldx	#>(BRKINTEntry-$C000+$5800)	; BRK 向量指向 BRKINTEntry
	ldy	#<(BRKINTEntry-$C000+$5800)	;
	stx	$bfff	;
	sty	$bffe	;
	ldx	#>(NMIEntry-$C000+$5800)	; NMI 向量指向 NMIEntry
	ldy	#<(NMIEntry-$C000+$5800)	;
	stx	$bffb	;
	sty	$bffa	;
	lda	DRAMPageSave	;
	sta	DRAMPagePort	;
	ldx	#>(RTIReturn-$C000+$5800)	;
	ldy	#<(RTIReturn-$C000+$5800)	;
	stx	BRKAddressSave-$C000+$5800+1	; 保存 BRK 向量为 RTIReturn
	sty	BRKAddressSave-$C000+$5800
	stx	NMIAddressSave-$C000+$5800+1	; 保存 NMI 向量为 RTIReturn
	sty	NMIAddressSave-$C000+$5800
	ldx	#>(RTSInstruction-$C000+$5800)	; SEND 向量指向 RTSInstruction
	ldy	#<(RTSInstruction-$C000+$5800)	;
	stx	SEndAddressSave-$C000+$5800+1
	sty	SEndAddressSave-$C000+$5800
	jsr	SetNMIVector	;
	jsr	EnableNMI	;
	sta	Addr2000	;
	jsr	InitVideoPort	;
	lda	#0	; 关闭音频
	sta	$4015	;
	lda	#$c0	; 禁止 CPU 内部中断
	sta	$4017	;
	rts	;
;=======================================;
;    磁盘 BIOS。
;    入口：X|Y 为参数缓冲区地址。
;    出口：A 为状态，Y 为已读扇区数。
;    注意：不得修改参数缓冲区。
;---------------------------------------;
;    磁盘功能分支表：功能号 0 复位、1 状态、2 读、3 写、4 校验、5 格式化、6 设置介质。
SoftDiskBranch:	.word SoftDiskReset-1	;
	.word SoftDiskStatus-1	;
	.word SoftDiskRead-1	;
	.word SoftDiskWrite-1	;
	.word SoftDiskVerify-1	;
	.word SoftDiskFormat-1	;
	.word SoftSetDiskMedia-1
SoftCommandLast = *-SoftDiskBranch	; 功能表字节数
;    磁盘 I/O 入口。入口：X|Y 为命令缓冲区地址（X 为高字节）。
DiskIO:	txa	;
	pha	;
	ldx	#DiskIOZB_Size-1	;
lpdio04:	lda	DiskIOZB,x	;
	sta	DiskIOBuffer,x	; 保存原命令缓冲区
	dex	;
	bpl	lpdio04	;
	lda	DRAMPageSave	; 保存命令缓冲区的页
	sta	DiskIOPageSave	;
	pla	;
	cmp	#>($c000)	; 命令缓冲区是否不小于 $C000
	bcc	nedio04	;
	and	#$bf	;
	ldx	#$ff	; 映射到 $FF 页
	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
nedio04:	sta	SectorBfPoint0+1	;
	sty	SectorBfPoint0	;
	ldy	#DiskIOCmdSize-1	;
lpdio08:	lda	(SectorBfPoint0),y	; 复制命令块到 DiskIOZB
	sta	DiskIOZB+Command,y	;
	dey	;
	bpl	lpdio08	;
;---------------------------------------;
;    执行命令：功能号无效时返回 BadCommand，否则调用对应功能。
SoftDiskIO:	;
	lda	DiskIOZB+Command	;
	asl	;
	cmp	#SoftCommandLast	;
	bcc	nesdo04	;
	lda	#BadCommand	;
	sta	DiskOutStatus	;
	bne	nesdo08	;
nesdo04:	jsr	neadi12	;
nesdo08:	bit	$2002	; 暂不响应 NMI
	jsr	RestoreNMI	; 恢复 NMI 状态
	lda	#100	; 马达开启后的延时（约 2 秒）
	sta	MotorOnCount	;
	lda	DiskOutStatus	;
	jmp	DiskIOReturn	;
neadi12:	tax	;
	lda	SoftDiskBranch+1,x	;
	pha	; 压入功能地址，用 RTS 跳转
	lda	SoftDiskBranch,x	;
	pha	;
	rts	;
;---------------------------------------;
;    恢复命令缓冲区的页与内容，保持 A、X。
DiskIOReturn:	;
	pha	;
	lda	DiskIOPageSave	; 恢复命令缓冲区的页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	txa	;
	pha	;
	ldx	#DiskIOZB_Size-1	;
lpdio12:	lda	DiskIOBuffer,x	;
	sta	DiskIOZB,x	;
	dex	;
	bpl	lpdio12	;
	pla	;
	tax	;
	pla	;
	rts	;
;=======================================;
;    复位软驱：复位驱动器，并在 NEC 状态为 $C0 时设置 720K 参数。
SoftDiskReset:	;
	lda	MotorStatus	; 保持马达开启
	ora	DiskIOZB+Drive	; 选择要复位的驱动器
	ora	#8	;
	sta	FDCCtrlPortO	; 复位驱动器
	ldx	#0	;
	stx	DiskOutStatus	;
	stx	SeekStatus	; 所有磁盘都需要重新寻道
	ora	#4	; 置位本磁盘需寻道标志
	sta	FDCCtrlPortO	;
	jsr	ChkStat2	;
	lda	NECStatus	;
	cmp	#$c0	;
	bne	nedrt12	;
	lda	#2	; 720K 速率
	sta	FDCSpeedPortO	;
	lda	#3	;
	jsr	FDCOutCmdErr	;
	lda	#$df	; 720K 参数
	jsr	FDCOutCmdErr	;
	lda	#2	;
	jsr	FDCOutCmdErr	;
	rts	;
nedrt12:	lda	DiskOutStatus	; NEC 状态不为 $C0：记为控制器错误
	ora	#BadCntlr	;
	sta	DiskOutStatus	;
	sec	;
nedrt16:	rts	;
;=======================================;
;    软驱状态：有软驱时打开马达，返回软驱信息与换盘信号。
SoftDiskStatus:	;
	lda	DiskIOZB+Drive	;
	clc	;
	adc	#1	;
	bit	SDriveMessage	;
	beq	nedss04	;
	jsr	OpenMotor	;
nedss04:	ldx	SDriveMessage	;
	ldy	FDCChangePortI	;
	rts	;
;=======================================;
;    设置介质：18 扇区记为 1.44M（0），其余记为 720K（2）。
SoftSetDiskMedia:	;
	lda	#0	;
	ldx	DiskIOZB+Sector	;
	cpx	#18	;
	beq	nedsm04	;
	lda	#2	;
nedsm04:	ldx	DiskIOZB+Drive	; 驱动器号
	sta	DriveMedia,x	;
	lda	#1	; 标记该驱动器的介质已设置
	sta	MediaMessage,x	;
	rts	;
;=======================================;
;    格式化磁盘：准备参数后，向 NEC 发送格式化命令，再经 DMA 发送扇区 ID 数据。
SoftDiskFormat:	;
	jsr	DiskPrepare	;
	bcs	nedrt16	;
	ldx	DiskIOZB+Drive	; 驱动器号
	lda	DriveMedia,x	;
	sta	FDCSpeedPortO	;
	jsr	SendFDDParam	;
	bcs	nedrt16	;
	jsr	SetBfPoint0	;
	lda	#18	; 每磁道扇区数（720K 为 9，1.44M 为 18）
	ldx	DiskIOZB+Drive	; 驱动器号
	ldy	DriveMedia,x	;
	beq	nesdf04	;
	lda	#9	;
nesdf04:	sta	BiosTemp3	;
	asl	;
	asl	; 每扇区 4 字节的 ID 数据
	sta	BiosTemp2	;
	adc	SectorBfPoint0	;
	sta	SectorBfPoint0	;
	bcs	nesdf08	;
	dec	SectorBfPoint0+1	;
nesdf08:	lda	#1	;
	sec	;
	sbc	BiosTemp2	;
	sta	BiosTemp2	; 计算数据偏移
;---------------------------------------; 向 NEC 发送格式化命令
	lda	#$cd	; 操作命令
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Head	; 磁头与驱动器
	asl	;
	asl	;
	ora	DiskIOZB+Drive	;
	jsr	FDCOutCmdErr	;
	lda	#2	; 每扇区字节数（2 表示 512 字节）
	jsr	FDCOutCmdErr	;
	lda	BiosTemp3	;
	jsr	FDCOutCmdErr	;
	lda	#$6c	; GPL：1.44M 为 $6C
	ldx	DiskIOZB+Drive	; 驱动器号
	ldy	DriveMedia,x	;
	beq	nesdf28	;
	lda	#$50	; 720K 的 GPL 为 $50
nesdf28:	jsr	FDCOutCmdErr	;
	lda	#$f6	; 填充字节 $F6
	jsr	FDCOutCmdErr	;
;---------------------------------------;
	ldy	#$ff	;
	lda	(SectorBfPoint0),y	;
	pha	; 保存最后一个字节
	lda	SectorBfPoint0	; SectorBfPoint0 减一
	bne	newfd04	;
	dec	SectorBfPoint0+1	;
newfd04:	dec	SectorBfPoint0	;
	ldx	#0	;
	ldy	BiosTemp2	;
	lda	(SectorBfPoint0),y	;
lpwfd08:	bit	FDCDRQPortI	; 等待 DRQ
	bvs	newfd08	;
	dey	;
	bne	lpwfd08	;
	bit	FDCDRQPortI	;
	bvs	newfd08	;
	dex	;
	bne	lpwfd08	;
	beq	newfd16	;
newfd08:	ldy	BiosTemp2	;
lpwfd16:	sta	FDCDMADackIO	;
	iny	;
	beq	newfd12	;
	lda	(SectorBfPoint0),y	;
	ldx	#$40	;
	bit	FDCDRQPortI	;
	bvs	lpwfd16	;
	stx	BiosTemp3	;
	ldx	#0	;
lpwfd20:	bit	FDCDRQPortI	;
	bvs	lpwfd16	;
	dex	;
	bne	lpwfd20	;
	bit	FDCDRQPortI	;
	bvs	lpwfd16	;
	dec	BiosTemp3	;
	bne	lpwfd20	;
	beq	newfd16	;
newfd12:	pla	; 最后一个字节
	ldx	#0	;
lpwfd32:	bit	FDCDRQPortI	;
	bvs	newfd20	;
	dex	;
	bne	lpwfd32	;
newfd16:	jmp	DiskIODMAError	; DMA 超时
newfd20:	sta	FDCDMATcIO	; 发送 TC，结束传输
	jmp	RWOperEnd	;
;=======================================;
;    写扇区：命令 $C5，然后进入通用读写流程。
SoftDiskWrite:	;
	lda	#$c5	;
	bne	nesdr04	;
;=======================================;
;    读扇区与校验：共用读命令 $E6 的流程。
SoftDiskVerify:	;
SoftDiskRead:	;
	lda	#$e6	;
nesdr04:	sta	BiosTemp2	;
	jsr	RWOper	;
	lda	#0	;
	ldy	DiskIOZB+Head	;
	cpy	NECStatus+4	; 末磁头
	beq	nesdr12	;
	clc	;
	adc	#9	; 720K 每面 9 个扇区
	ldx	DiskIOZB+Drive	;
	ldy	DriveMedia,x	;
	bne	nesdr08	;
	adc	#9	; 1.44M 每面 18 个扇区
nesdr08:	ldy	DiskIOZB+Head	; 磁头是否从 1 变为 0
	beq	nesdr12	;
	eor	#$ff	; 是：取反加一，扣除本面扇区
	adc	#1	;
nesdr12:	ldy	NECStatus+3	; 末磁道
	cpy	DiskIOZB+Track	;
	beq	nesdr16	;
	clc	;
	adc	#18	; 720K 每磁道 18 个扇区
	ldx	DiskIOZB+Drive	;
	ldy	DriveMedia,x	;
	bne	nesdr16	;
	clc	;
	adc	#18	; 1.44M 每磁道 36 个扇区
nesdr16:	clc	;
	adc	NECStatus+5	; 末扇区
	sec	;
	sbc	DiskIOZB+Sector	;
	tay	; 返回已读扇区数（Y）
nesdr20:	rts	;
;---------------------------------------;
;    发送驱动器参数，并寻道到命令指定的磁道。
SendFDDParam:	;
	lda	#3	; 发送 FDD 参数命令
	jsr	FDCOutCmdErr	;
	lda	#$bf	; 1.44M 参数
	ldx	DiskIOZB+Drive	; 驱动器号
	ldy	DriveMedia,x	;
	beq	nerwo24	;
	lda	#$df	; 720K 参数
nerwo24:	jsr	FDCOutCmdErr	;
	lda	#2	;
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Track	; 目标磁道
	jmp	Seek	;
;---------------------------------------;
;    准备磁盘：打开马达，有换盘信号时复位并寻道，再检查盘片是否存在。
DiskPrepare:	;
	jsr	OpenMotor	;
	lda	#0	;
	sta	DiskOutStatus	;
	lda	FDCChangePortI	; 是否换盘
	bpl	nedpp16	;
	ldx	DiskIOZB+Drive	; 清除本驱动器的介质标记
	lda	#0	;
	sta	MediaMessage,x	;
	jsr	SoftDiskReset	; 复位磁盘
	bcc	nedpp04	;
	jsr	SoftDiskReset	;
	bcs	nedpp12	;
nedpp04:	lda	#1	; 寻道到 1 道
	jsr	Seek	;
	bcs	nedpp12	;
	lda	#0	; 寻道到 0 道
	jsr	Seek	;
	bcs	nedpp12	;
	ldx	FDCChangePortI	; 仍有换盘信号？
	bpl	nedpp16	;
	lda	#DriveEmpty	; 驱动器无盘
nedpp08:	sta	DiskOutStatus	;
	sec	;
nedpp12:	rts	;
nedpp16:	jsr	WaitDisplay	;
	ldx	#1	;
	jsr	WaitVSynCnt	;
	lda	DiskDisplayOff	;
	beq	nedpp20	;
	lda	#0	; 关闭显示
	sta	$2001	;
nedpp20:	sei	;
	jsr	DisableNMI	; 关闭 NMI，供软驱读写使用
	clc	;
	rts	;
;=======================================;
;    读写准备：检测介质（未设置时）、设置速率与参数、寻道并计算缓冲区指针。
RWOper:	jsr	DiskPrepare	;
	bcs	nedpp12	;
	ldx	DiskIOZB+Drive	; 本驱动器的介质是否已设置
	lda	MediaMessage,x	;
	bne	nerwo20	;
	lda	DiskIOZB+Drive	; 未设置：检测介质
	clc	;
	adc	#1	;
	ora	SeekStatus	;
	sta	SeekStatus	;
	jsr	HomeTrack	;
	bcc	nerwo04	;
	jsr	HomeTrack	;
	bcs	nedpp12	;
nerwo04:	lda	#0	; 500kbps 速率（1.44M）
	jsr	DetectMedia	;
	bcs	nedpp12	; 出错
	beq	nerwo08	; 介质匹配
	lda	#2	; 250kbps 速率（720K）
	jsr	DetectMedia	;
	bcs	nedpp12	; 出错
	beq	nerwo12	; 介质匹配
	lda	#BadAddressMark	; 介质不匹配
	bne	nedpp08	;
nerwo08:	lda	#0	;
	beq	nerwo16	;
nerwo12:	lda	#2	;
nerwo16:	ldx	DiskIOZB+Drive	; 驱动器号
	sta	DriveMedia,x	;
	lda	#1	; 标记该驱动器的介质已设置
	sta	MediaMessage,x	;
nerwo20:	lda	DriveMedia,x	; 设置速率
	sta	FDCSpeedPortO	;
	jsr	SendFDDParam	;
	bcs	nedpp12	;
	lda	#0	;
	sta	MotorOnCount	; 马达计数置为最大
	lda	DiskIOZB+Track	;
	jsr	Seek	;
	bcs	nedpp12	;
	jsr	SetBfPoint0	;
;---------------------------------------;
	ldy	DiskIOZB+Command	; 是否为写
	cpy	#CMD_WRITE	;
	beq	nerwo28	;
	clc	; 否
nerwo28:	adc	#$ff	;
	sta	SectorBfPoint1	;
	txa
	adc	#0	;
	sta	SectorBfPoint1+1	;
;---------------------------------------; 向 NEC 发送读写命令
	lda	BiosTemp2	; 操作命令
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Head	; 磁头与驱动器
	asl	;
	asl	;
	ora	DiskIOZB+Drive	;
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Track	; 磁道
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Head	; 磁头
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Sector	; 扇区
	jsr	FDCOutCmdErr	;
	lda	#2	; 每扇区字节数（2 表示 512 字节）
	jsr	FDCOutCmdErr	;
	lda	#18	; 每磁道扇区数（720K 为 9，1.44M 为 18）
	ldx	DiskIOZB+Drive	; 驱动器号
	ldy	DriveMedia,x	;
	beq	nerwo32	;
	lda	#9	;
nerwo32:	jsr	FDCOutCmdErr	;
	lda	#$1b	; GPL（720K 为 $2A，1.44M 为 $1B）
	ldx	DiskIOZB+Drive	; 驱动器号
	ldy	DriveMedia,x	;
	beq	nerwo36	;
	lda	#$2a	; 720K 的 GPL
nerwo36:	jsr	FDCOutCmdErr	;
	lda	#$ff	; DTL
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Sectors	;
	ldx	DiskIOZB+Command	; 是否为读操作
	cpx	#CMD_READ	;
	bne	VerifyOper	;
;---------------------------------------;
;    读操作：经 DMA 逐字节读入缓冲区。
ReadOper:	;
lpreo02:	pha	; 保存扇区计数
	ldx	#0	;
	ldy	#0	;
lpreo04:	bit	FDCDRQPortI	; 等待 DRQ，超时则报错
	bvs	nereo02	;
	dey	;
	bne	lpreo04	;
	bit	FDCDRQPortI	;
	bvs	nereo02	;
	dex	;
	bne	lpreo04	;
	beq	nereo12	;
nereo02:	ldy	#0	; 先读入前 256 字节
lpreo08:	lda	FDCDMADackIO	; 每字节约 16us（26.6 个时钟）
	sta	(SectorBfPoint0),y	; 6 个时钟
	iny	; 2 个时钟
	beq	nereo04	; 2 个时钟
	ldx	#0	; 超时计数
lpreo12:	bit	FDCDRQPortI	; 4 个时钟
	bvs	lpreo08	; 3-4 个时钟
	dex	;
	bne	lpreo12	;
	beq	nereo12	;
lpreo16:	lda	FDCDMADackIO	; 读入后 256 字节
	sta	(SectorBfPoint1),y	;
nereo04:	iny	;
	beq	nereo08	;
	ldx	#0	; 超时计数
lpreo20:	bit	FDCDRQPortI	;
	bvs	lpreo16	;
	dex	;
	bne	lpreo20	;
	beq	nereo12	;
nereo08:	ldx	#0	; 超时计数
lpreo24:	bit	FDCDRQPortI	;
	bvs	nereo16	;
	dex	;
	bne	lpreo24	;
nereo12:	jmp	DiskIODMAError	; DMA 超时
nereo16:	pla	; 取回扇区计数
	tax	;
	dex	; 是否为最后一个扇区
	bne	nereo20	;
	lda	FDCDMATcIO	; 是：产生 -DACK 与 TC 信号
	bvs	nereo24	;
nereo20:	lda	FDCDMADackIO	; 否：仅产生 -DACK 信号
nereo24:	inc	SectorBfPoint1+1	;
	sta	(SectorBfPoint1),y	;
	inc	SectorBfPoint1+1	; 指向下一扇区
	inc	SectorBfPoint0+1	;
	inc	SectorBfPoint0+1	;
	txa	;
	bne	lpreo02	;
	jmp	RWOperEnd	;
;---------------------------------------;
;    校验：经 DMA 读入扇区数据用于比较，不写回内存。
VerifyOper:	;
	cpx	#CMD_VERIFY	;
	bne	WriteOper	; 不是校验：转为写操作
lpveo02:	pha	; 保存扇区计数
	ldx	#0	;
	ldy	#0	;
lpveo04:	bit	FDCDRQPortI	;
	bvs	neveo02	;
	dey	;
	bne	lpveo04	;
	bit	FDCDRQPortI	;
	bvs	neveo02	;
	dex	;
	bne	lpveo04	;
	beq	neveo12	;
neveo02:	ldy	#0	; 先读入前 256 字节
lpveo08:	lda	FDCDMADackIO	;
	iny	;
	beq	neveo04	;
	ldx	#0	;
lpveo12:	bit	FDCDRQPortI	;
	bvs	lpveo08	;
	dex	;
	bne	lpveo12	;
	beq	neveo12	;
lpveo16:	lda	FDCDMADackIO	; 读入后 256 字节
neveo04:	iny	;
	beq	neveo08	;
	ldx	#0	; 超时计数
lpveo20:	bit	FDCDRQPortI	;
	bvs	lpveo16	;
	dex	;
	bne	lpveo20	;
	beq	neveo12	;
neveo08:	ldx	#0	; 超时计数
lpveo24:	bit	FDCDRQPortI	;
	bvs	neveo16	;
	dex	;
	bne	lpveo24	;
neveo12:	jmp	DiskIODMAError	;
neveo16:	pla	; 取回扇区计数
	tax	;
	dex	; 是否为最后一个扇区
	bne	neveo20	;
	lda	FDCDMATcIO	; 是：产生 -DACK 与 TC 信号
	bvs	neveo24	;
neveo20:	lda	FDCDMADackIO	; 否：仅产生 -DACK 信号
neveo24:	txa	;
	bne	lpveo02	;
	jmp	RWOperEnd	;
;---------------------------------------;
;    写操作：经 DMA 把缓冲区数据写入软驱，按扇区循环。
WriteOper:	;
lpweo04:	tax	; 是否为最后一个扇区
	dex	;
	beq	WriteLastSector	; 是：单独处理
	txa	;
	pha	; 保存扇区计数
	ldx	#0	;
	ldy	#0	;
	lda	(SectorBfPoint0),y	;
lpweo08:	bit	FDCDRQPortI	;
	bvs	neweo08	;
	dey	;
	bne	lpweo08	;
	bit	FDCDRQPortI	;
	bvs	neweo08	;
	dex	;
	bne	lpweo08	;
	beq	neweo20	;
neweo08:	ldy	#0	; 先写入前 256 字节（每字节约 16us）
lpweo16:	sta	FDCDMADackIO	;
	iny	;
	beq	neweo12	;
	lda	(SectorBfPoint0),y	;
	ldx	#0	; 超时计数
lpweo20:	bit	FDCDRQPortI	;
	bvs	lpweo16	;
	dex	;
	bne	lpweo20	;
	beq	neweo20	;
lpweo24:	sta	FDCDMADackIO	;
	iny	;
	beq	neweo24	;
neweo12:	lda	(SectorBfPoint1),y	; 读入后 256 字节
	bit	FDCDRQPortI	;
	bvs	lpweo24	;
	ldx	#0	; 超时计数
lpweo28:	bit	FDCDRQPortI	;
	bvs	lpweo24	;
	dex	;
	bne	lpweo28	;
neweo20:	jmp	DiskIODMAError	;
neweo24:	inc	SectorBfPoint1+1	;
	inc	SectorBfPoint1+1	; 指向下一扇区
	inc	SectorBfPoint0+1	;
	inc	SectorBfPoint0+1	;
	pla	; 取回扇区计数
	bne	lpweo04	;
	beq	RWOperEnd	;
;---------------------------------------;
;    写最后一个扇区：最后一个字节在末尾单独发送，并产生 TC 信号。
WriteLastSector:	;
	ldy	#$ff	;
	lda	(SectorBfPoint1),y	;
	pha	; 保存最后一个字节
	lda	SectorBfPoint1	;
	bne	newls04	;
	dec	SectorBfPoint1+1	;
newls04:	dec	SectorBfPoint1	;
	ldx	#0	;
	iny	;
	lda	(SectorBfPoint0),y	;
lpwls08:	bit	FDCDRQPortI	;
	bvs	newls08	;
	dey	;
	bne	lpwls08	;
	bit	FDCDRQPortI	;
	bvs	newls08	;
	dex	;
	bne	lpwls08	;
	beq	DiskIODMAError	;
newls08:	ldy	#0	; 先写入前 256 字节
lpwls16:	sta	FDCDMADackIO	;
	iny	;
	beq	newls12	;
	lda	(SectorBfPoint0),y	;
	ldx	#0	;
lpwls20:	bit	FDCDRQPortI	;
	bvs	lpwls16	;
	dex	;
	bne	lpwls20	;
	beq	DiskIODMAError	;
lpwls24:	sta	FDCDMADackIO	;
newls12:	iny	; 读入后 256 字节
	beq	newls16	;
	lda	(SectorBfPoint1),y	;
	ldx	#0	; 超时计数
lpwls28:	bit	FDCDRQPortI	;
	bvs	lpwls24	;
	dex	;
	bne	lpwls28	;
	beq	DiskIODMAError	;
newls16:	pla	; 取回最后一个字节
	ldx	#0	;
lpwls32:	bit	FDCDRQPortI	;
	bvs	newls28	;
	dex	;
	bne	lpwls32	;
;---------------------------------------;
;    DMA 错误：没有 IRQ 信号时记为超时。
DiskIODMAError:	;
	pla	; 丢弃保存的字节或扇区计数
	bit	FDCIRQPortI	; 是否有 IRQ 信号
	bvs	nerwe08	;
	lda	DiskOutStatus	; 没有 IRQ：超时
	ora	#TimeOut	;
	sta	DiskOutStatus	;
	sec	;
	rts	;
newls28:	sta	FDCDMATcIO	; 最后一个字节：发送 TC
;---------------------------------------;
;    读写结束：等待 FDC 中断，读取结果并转换为错误码。
RWOperEnd:	;
	jsr	WaitFDCInt	;
	bcs	nerwe20	;
nerwe08:	lda	#7	; 结果字节数 7
	jsr	Results	;
	bcs	nerwe20	;
	lda	NECStatus	; 读取 ST0
	and	#$c0	; 是否正常结束
	beq	nerwe20	; 是
	cmp	#$40	; 是否异常结束
	bne	nerwe12	; 否：NEC 错误
	lda	NECStatus+1	; 读取 ST1
	asl	; ST1 各位依次映射为错误码
	ldx	#RecordNotFound	;
	bcs	nerwe16	;
	asl	;
	asl	;
	ldx	#BadCRC	;
	bcs	nerwe16	;
	asl	;
	ldx	#BadDMA	;
	bcs	nerwe16	;
	asl	;
	asl	;
	ldx	#RecordNotFound	;
	bcs	nerwe16	;
	asl	;
	ldx	#WriteProtect	;
	bcs	nerwe16	;
	asl	;
	ldx	#BadAddressMark	;
	bcs	nerwe16	;
nerwe12:	ldx	#BadCntlr	; 其他 ST0 错误：控制器错误
nerwe16:	txa	;
	ora	DiskOutStatus	;
	sta	DiskOutStatus	;
nerwe20:	rts	;
;---------------------------------------;
;    向 FDC 输出一个命令字节。入口：A 为字节。超时时丢弃返回地址并返回错误（C=1）。
FDCOutCmdErr:	;
	pha	;
	ldx	#0	;
	ldy	#0	;
lpfoc04:	lda	FDCStatPortI	; DIO 为 1 时等待 FDC 输出结束
	and	#$40	;
	beq	nefoc04	;
	dey	;
	bne	lpfoc04	;
	dex	;
	bne	lpfoc04	;
	beq	nefoc08	;
nefoc04:	ldx	#0	;
	ldy	#0	;
lpfoc08:	lda	FDCStatPortI	; 等待 RQM 为 1 后写入
	bmi	nefoc12	;
	dey	;
	bne	lpfoc08	;
	dex	;
	bne	lpfoc08	;
nefoc08:	pla	;
	lda	DiskOutStatus	;
	ora	#TimeOut	;
	sta	DiskOutStatus	;
	pla	; 丢弃返回地址
	pla	;
	sec	; 错误返回
	rts	;
nefoc12:	pla	;
	sta	FDCDataPortIO	;
	clc	;
	rts	;
;---------------------------------------;
;    打开软驱马达，并等待 1 秒使马达稳定。
OpenMotor:	;
	lda	#0	;
	sta	MotorOnCount	; 马达计数置为最大
	lda	DiskIOZB+Drive	; 本驱动器的马达是否已开
	clc	;
	adc	#$1f	;
	and	#$30	;
	bit	MotorStatus	;
	bne	neomr04	;
	ora	MotorStatus	; 未开：选择马达
	sta	MotorStatus	;
	ora	DiskIOZB+Drive	;
	ora	#$0c	; 不复位，允许 DMA 与 INT
	sta	FDCCtrlPortO	; 开启马达
	jsr	Wait1Second	; 等待马达稳定
	lda	#0	;
	sta	MotorOnCount	; 马达计数置为最大
neomr04:	rts	;
;---------------------------------------;
;    寻道到指定磁道。入口：A 为目标磁道。
Seek:	sta	BiosTemp1	;
	lda	DiskIOZB+Drive	;
	clc	;
	adc	#1	;
	bit	SeekStatus	;
	bne	nesek08	; 已寻道过：跳过回零
	jsr	HomeTrack	;
	bcc	nesek04	;
	jsr	HomeTrack	;
	bcs	nesek12	;
nesek04:	lda	#0	;
	ldx	DiskIOZB+Drive	;
	sta	CurrentTrack,x	; 当前磁道记为 0
nesek08:	lda	BiosTemp1	; 目标磁道
	ldx	DiskIOZB+Drive	;
	cmp	CurrentTrack,x	;
	clc	;
	beq	nesek12	; 已在目标磁道
	lda	#$0f	; 寻道命令
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Drive	; 驱动器号（磁头 0）
	jsr	FDCOutCmdErr	;
	lda	BiosTemp1	; 目标磁道
	jsr	FDCOutCmdErr	;
	jsr	ChkStat2	;
	ldx	#$0f	; 磁头稳定时间 15 毫秒
lpsek04:	ldy	#138	; 约 1 毫秒
lpsek08:	pha	;
	pla	;
	dey	;
	bne	lpsek08	;
	dex	;
	bne	lpsek04	;
	bcs	nesek12	;
	lda	DiskIOZB+Drive	;
	tax	;
	clc	;
	adc	#1	;
	ora	SeekStatus	; 记录本驱动器已寻道
	sta	SeekStatus	;
	lda	NECStatus+1	;
	sta	CurrentTrack,x	; 更新当前磁道（C=0）
nesek12:	rts	;
;---------------------------------------;
;    检测介质：以指定速率读取扇区 ID，最多尝试 5 次。
;    出口：Z=1 表示介质匹配，C=1 表示出错。
DetectMedia:	;
	sta	FDCSpeedPortO	;
	lda	#5	; 最多尝试 5 次
	sta	BiosTemp1	;
lpdma04:	lda	#$4a	; 读扇区 ID 命令
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Drive	;
	jsr	FDCOutCmdErr	;
	jsr	WaitFDCInt	;
	bcs	nedma08	;
	lda	#7	;
	jsr	Results	;
	bcs	nedma08	;
	lda	NECStatus	; 读取 ST0
	and	#$c0	; 是否正常结束
	beq	nedma08	; 是：Z=1
	dec	BiosTemp1	;
	bne	lpdma04	;
	lda	#8	;
	bit	NECStatus	; 驱动器是否未就绪
	beq	nedma04	;
	lda	#TimeOut	; 未就绪
	sta	DiskOutStatus	;
	sec	;
	rts	;
nedma04:	lda	#1	; 不匹配（Z=0）
nedma08:	rts	;
;---------------------------------------;
;    回零道：发送回零命令并检查中断状态。
HomeTrack:	;
	lda	#$07	; 回零命令
	jsr	FDCOutCmdErr	;
	lda	DiskIOZB+Drive	;
	jsr	FDCOutCmdErr	;
	jsr	ChkStat2	;
	rts	;
;---------------------------------------;
;    检查中断状态 2。
ChkStat2:	;
	jsr	WaitFDCInt	;
	bcs	necs204	;
	lda	#8	; 检查中断状态命令
	jsr	FDCOutCmdErr	;
	lda	#2	;
	jsr	Results	; 返回两个结果字节
	bcs	necs204	;
	lda	NECStatus	;
	and	#$60	;
	cmp	#$60	;
	clc	;
	bne	necs204	;
	lda	DiskOutStatus	; 寻道错误
	ora	#BadSeek	;
	sta	DiskOutStatus	;
	sec	;
necs204:	rts	;
;-----------------------------------------------;
;    等待 FDC 产生中断（约 2 秒超时）。
WaitFDCInt:	;
	lda	#10	; 10 × 0.2 秒 = 2 秒超时
lpwfi04:	pha	;
	ldx	#118	;
lpwfi08:	ldy	#0	;
lpwfi12:	bit	FDCIRQPortI	;
	bvs	newfi04	;
	dey	;
	bne	lpwfi12	;
	dex	;
	bne	lpwfi08	;
	pla	;
	sec	;
	sbc	#1	;
	bne	lpwfi04	;
	lda	DiskOutStatus	;
	ora	#TimeOut	;
	sta	DiskOutStatus	;
	rts	;
newfi04:	pla	;
	clc	; 中断到达（C=0）
	rts	;
;---------------------------------------;
;    读取 FDC 命令的结果字节。入口：A 为结果字节数。
Results:	ldx	#0	;
lprst04:	pha	;
	txa	;
	pha	;
	ldx	#0	; 约 0.43 秒超时
	ldy	#0	;
lprst08:	lda	FDCStatPortI	; 检查是否有结果
	bmi	nerst04	;
	dey	;
	bne	lprst08	;
	dex	;
	bne	lprst08	;
	lda	DiskOutStatus	;
	ora	#TimeOut	;
	sta	DiskOutStatus	;
	pla	;
	pla	;
	sec	;
	rts	;
nerst04:	lda	FDCStatPortI	; 是否有结果
	and	#$40	;
	beq	nerst08	;
	pla	; 是：取回计数
	tax	;
	lda	FDCDataPortIO	;
	sta	NECStatus,x	; 保存结果字节
	inx	;
	pla	;
	sec	;
	sbc	#1	;
	bne	lprst04	;
	ldx	#5	; 延时约 15 微秒
lprst12:	dex	;
	bne	lprst12	;
	lda	#$10	; 读取全部结果
	bit	FDCStatPortI	; FDC 是否仍忙
	bne	nerst12	;
	clc	; 否
	rts	;
nerst08:	pla	; 无结果：控制器错误
	pla	;
nerst12:	lda	DiskOutStatus	;
	ora	#BadCntlr	;
	sta	DiskOutStatus	;
	sec	;
	rts	;
;---------------------------------------; 等待 1 秒
Wait1Second:	;
	lda	#50	; 50 次 × 20 毫秒 = 1 秒
lpw1s04:	jsr	WaitmSecond	;
	sec	;
	sbc	#1	;
	bne	lpw1s04	;
	rts	;
;---------------------------------------; 延时 20 毫秒
WaitmSecond:	;
	ldy	#28	;
lpwms08:	dex	;
	bne	lpwms08	;
	dey	;
	bne	lpwms08	;
	rts	;
;---------------------------------------;
;    设置缓冲区指针 SectorBfPoint0，并映射缓冲区所在的页。
SetBfPoint0:	;
	lda	DiskIOZB+Buffer+1	;
	ldx	DiskIOZB+BPage	;
	cmp	#$c0	; 缓冲区是否不小于 $C000
	bcc	nerwo40	;
	and	#$bf	; 是：映射到最后一页
	ldx	#$ff	;
nerwo40:	sta	SectorBfPoint0+1	;
	cpx	BiosUsePage	; 是否为保护页
	bne	nerwo44	;
	ldx	DRAMPageNotUse	; 是：改用不使用的页
nerwo44:	cpx	#0	;
	bne	nerwo46	;
	ldx	BiosUsePage	;
nerwo46:	stx	DRAMPageSave	;
	stx	DRAMPagePort	;
	tax	;
	lda	DiskIOZB+Buffer	; 缓冲区低字节
	sta	SectorBfPoint0	;
	rts	;

;=======================================;
;    取 DRAM 大小。出口：A 为大小（单位 16K）。
GetDRAMSize:	;
	ldx	DRAMPageHigh	;
	inx	;
	txa	;
	rts	;
;=======================================;
;    取出并设置不使用的 DRAM 页。入口：X 为页号。出口：A 为原来的页号。
SetPageNoUse:	;
	lda	DRAMPageNotUse	;
	stx	DRAMPageNotUse	;
	rts	;

;=======================================;
;    设置向量：记录用户修改过的 BRK、NMI 向量，并改为 BIOS 入口。
SetVector:	;
	jsr	DisableNMI	;
	lda	DRAMPageSave	; 保存当前页
	pha	;
	lda	#$ff	;
	sta	DRAMPagePort	; 切换到最后一页
	ldx	$bfff	; 保存用户的 BRK 向量
	ldy	$bffe	;
	stx	BRKAddressSave-$C000+$5800+1
	sty	BRKAddressSave-$C000+$5800
	ldx	$bffb	; 保存用户的 NMI 向量
	ldy	$bffa	;
	stx	NMIAddressSave-$C000+$5800+1
	sty	NMIAddressSave-$C000+$5800
	bit	MonitorMode	;
	bmi	nesvr04	; 监视器模式：不改写向量
	ldx	#>(BRKINTEntry-$C000+$5800)	; BRK 向量指向 BRKINTEntry
	ldy	#<(BRKINTEntry-$C000+$5800)	;
	stx	$bfff	;
	sty	$bffe	;
	ldx	#>(NMIEntry-$C000+$5800)	; NMI 向量指向 NMIEntry
	ldy	#<(NMIEntry-$C000+$5800)	;
	stx	$bffb	;
	sty	$bffa	;
nesvr04:	pla	;
	sta	DRAMPagePort	;
	jsr	RestoreNMI	;
	lda	#$80	; 设置向量已修改标志
	sta	VectorSet	;
	rts	;

;=======================================;
;    设置向量并标记 NMI 返回 RTI。入口：X 为页号。
SetVectorRTI:	;
	cpx	BiosUsePage	; 是否为保护页
	bne	nesvi04	;
	ldx	DRAMPageNotUse	; 是：改用不使用的页
nesvi04:	stx	NMIDRAMPage	;
	jsr	SetVector	;
	lda	#$c0	; 设置 NMI 返回标志
	sta	VectorSet	;
	rts	;

;=======================================;
;    恢复用户修改过的 BRK、NMI 向量。
RstVector:	;
	jsr	DisableNMI	;
	lda	DRAMPageSave	; 保存当前页
	pha	;
	lda	#$ff	;
	sta	DRAMPagePort	;
	ldx	BRKAddressSave-$C000+$5800+1	; 恢复 BRK 向量
	ldy	BRKAddressSave-$C000+$5800	;
	stx	$bfff	;
	sty	$bffe	;
	ldx	NMIAddressSave-$C000+$5800+1	; 恢复 NMI 向量
	ldy	NMIAddressSave-$C000+$5800	;
	stx	$bffb	;
	sty	$bffa	;
	pla	;
	sta	DRAMPagePort	;
	jmp	RestoreNMI	;

;=======================================;
;    取卡版本。出口：A 为版本号（0）。
GetCardVer:	;
	lda	#0	;
	rts	;

;=======================================;
;    取显存页大小。必须保留 X、Y。
GetVRAMSize:	;
	lda	VideoPageNumber	;
	rts	;
;===============================================;
;    读取按键（手柄或键盘）。
;    入口：
;        Y 为 0：读取按键；Y 为 1：测试按键。
;        X 为读取设备：
;            位 7：VSync 后再检查按键；
;            位 4：Press() 按键；
;            位 2：键盘；位 1：手柄 1；位 0：手柄 0。
;    出口：
;        A 位 1：1 为键盘，0 为手柄；
;        A 位 0：位 1 为 0 时，1 为手柄 2，0 为手柄 1；位 1 为 1 时，1 为有键，0 为无键。
;        X 为键码；Y 为变化的按键（手柄）或扩展码（键盘）。
;    手柄键码：
;        位 7 右，位 6 左，位 5 下，位 4 上，位 3 Start，位 2 Select，位 1 B，位 0 A。
;    使用 BiosTemp1-5。
;-----------------------------------------------;
ReadKey:	stx	BiosTemp2	; 保存参数
	sty	BiosTemp3	;
	txa	;
	and	#$07	; 是否读手柄或键盘
	beq	nerj112	;
	txa	;
	and	#$10	; 是否为 Press 按键
	beq	ReadKeys	;
	txa	; 是：清除原按键
	lsr	; 是否读手柄 1
	bcc	nerky04	;
	ldx	Joystick1Key	;
	stx	BiosTemp4	;
nerky04:	lsr	; 是否读手柄 2
	bcc	nerky08	;
	ldx	Joystick2Key	;
	stx	BiosTemp5	;
nerky08:	lsr	; 是否读键盘
	bcc	ReadKeys	;
lprky04:	jsr	TestKeyBoardDev-$C000+$5800	; 清空键盘缓冲区
	beq	ReadKeys	;
	jsr	ReadKeyBoardDev	;
	jmp	lprky04	;
;---------------------------------------;
ReadKeys:	;
	lda	BiosTemp2	;
	bpl	nerks04	;
	ldx	#1	; 是：等待 VSync
	jsr	WaitVSynCnt	;
nerks04:	lda	BiosTemp2	;
	lsr	;
	bcc	ReadJoystick2	;
ReadJoystick1:	;
	lda	Joystick1Key	; 取手柄 1 的键
	eor	BiosTemp4	; 与上次相同？
	bne	nerj104	;
	lda	BiosTemp3	; 是否为读键功能
	beq	ReadJoystick2	; 是：检查手柄 2
	lda	BiosTemp2	; 否：是否要测试其他设备
	and	#6	;
	bne	ReadJoystick2	; 是
	beq	nerj108	; 否：只测试手柄 1
nerj104:	tay	; 变化的键
	lda	BiosTemp2	; 是否为 Press 方式
	and	#$10	;
	beq	nerj108	;
	tya	;
	and	Joystick1Key	; 是否有按下的键
	beq	ReadJoystick2	; 否
nerj108:	lda	#0	; 手柄 1
nerj112:	rts	;
;---------------------------------------;
ReadJoystick2:	;
	lda	BiosTemp2	; 是否读手柄 2
	and	#2	;
	beq	ReadKeyBoard0	;
	lda	Joystick2Key	; 取手柄 2 的键
	eor	BiosTemp5	; 与上次相同？
	bne	nerj204	;
	lda	BiosTemp3	; 是否为读键功能
	beq	ReadKeyBoard0	; 是：检查键盘
	lda	BiosTemp2	; 否：是否要测试其他设备
	and	#4	;
	bne	ReadKeyBoard0	; 是
	beq	nerj208	; 否：只测试手柄 2
nerj204:	tay	; 变化的键
	lda	BiosTemp2	; 是否为 Press 方式
	and	#$10	;
	beq	nerj208	;
	tya	;
	and	Joystick2Key	; 是否有按下的键
	beq	ReadKeyBoard0	; 否
nerj208:	lda	#1	; 手柄 2
	rts	;
;---------------------------------------;
ReadKeyBoard0:	;
	lda	BiosTemp2	; 是否读键盘
	and	#4	;
	beq	nerkb16	; 否：只检查手柄
lprkb04:	jsr	TestKeyBoardDev-$C000+$5800	; 键盘缓冲区是否为空
	cpy	#0	;
	beq	neekb12	; 是：为空
	ldy	BiosTemp3	; 是否为测试功能
	bne	nerkb08	; 是：不读取键码
	jsr	ReadKeyBoardDev	; 否：读取键码
nerkb08:	pha	;
	txa	; X/A 转为 Y/X
	tay	;
	pla	;
	tax	;
	lda	#3	; 键盘输出
	rts	;
neekb12:	lda	BiosTemp3	; 是否为读键功能
	beq	nerkb16	;
	lda	#2	; 无键
	rts	;
nerkb16:	jmp	ReadKeys	; 继续检查手柄
;---------------------------------------;
;    读取手柄按键。入口：X 为手柄编号（0 为手柄 1，1 为手柄 2）。
;    手柄键码：位 7 右，位 6 左，位 5 下，位 4 上，位 3 Start，位 2 Select，位 1 B，位 0 A。
GetJoystickKey:	;
	lda	#1	; 锁存手柄（同时禁用键盘）
	sta	$4016	;
	lda	#0	;
	sta	$4016	;
	lda	#$80	; 哨兵位，移出后结束循环
lpgjk04:	pha	;
	lda	$4016,x	;
	lsr	;
	pla	;
	ror	;
	bcc	lpgjk04	;
	rts	;

;=======================================;
;    修正地址的页。入口：A 为地址高字节。出口：A 为修正后的高字节，Y 为页号。
StandardAddress:	;
	cmp	#>($c000)	;
	bcc	nesas04	;
	and	#$bf	;
	ldy	#$ff	;
nesas04:	cpy	BiosUsePage	; 是否为保护页
	bne	nesas08	;
	ldy	DRAMPageNotUse	; 是：改用不使用的页
nesas08:	cpy	#0	;
	bne	nesas12	;
	ldy	BiosUsePage	;
nesas12:	rts	;
;-----------------------------------------------;
MoveBlockStruct = $00	; 块结构（位于零页）
BkSrcPage = MoveBlockStruct	; 源页号
BkSrcAddress = MoveBlockStruct+1	; 源地址（低字节，高字节在其后）
BkTarPage = MoveBlockStruct+3	; 目标页号
BkTarAddress = MoveBlockStruct+4	; 目标地址
BkLength = MoveBlockStruct+6	; 长度
;-----------------------------------------------;
;    搬移内存块：按 X 指向的块结构，从源复制到目标（经 MoveBuffer 中转）。
MoveBlock:	;
	ldy	BkSrcPage,x	;
	lda	BkSrcAddress+1,x	; 修正源地址与页
	pha	;
	jsr	StandardAddress	;
	sta	BkSrcAddress+1,x	;
	sty	BiosTemp2	;
	ldy	BkTarPage,x	;
	lda	BkTarAddress+1,x	; 修正目标地址与页
	pha	;
	jsr	StandardAddress	;
	sta	BkTarAddress+1,x	;
	sty	BiosTemp3	;
	ldy	BkLength,x	;
	beq	nembk04	; 低字节为 0：整页复制
	inc	BkLength+1,x	; 否则高字节加一（末尾不足一页）
	ldy	#0	;
nembk04:	sty	BiosTemp1	;
	lda	DRAMPageSave	; 保存当前页
	pha	;
lpmbk04:	lda	BkLength+1,x	;
	cmp	#1	;
	bne	nembk08	;
	lda	BkLength,x	; 最后一页：只复制剩余字节
	sta	BiosTemp1	;
nembk08:	lda	BiosTemp2	; 源页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
lpmbk08:	lda	(BkSrcAddress,x)	; 读入源到 MoveBuffer
	sta	MoveBuffer,y	;
	inc	BkSrcAddress,x	;
	bne	nembk12	;
	inc	BkSrcAddress+1,x	;
nembk12:	iny	;
	cpy	BiosTemp1	;
	bne	lpmbk08	;
	ldy	#0	;
	lda	BiosTemp3	; 目标页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
lpmbk12:	lda	MoveBuffer,y	; 写入缓冲区内容到目标
	sta	(BkTarAddress,x)	;
	inc	BkTarAddress,x	;
	bne	nembk16	;
	inc	BkTarAddress+1,x	;
nembk16:	iny	;
	cpy	BiosTemp1	;
	bne	lpmbk12	;
	dec	BkLength+1,x	; 页数减一
	bne	lpmbk04	;
	jmp	neebk28	;
;---------------------------------------;
;    交换内存块：源与目标的内容互换（经 MoveBuffer 中转）。
ExchBlock:	;
	ldy	BkSrcPage,x	;
	lda	BkSrcAddress+1,x	; 修正源地址与页
	pha	;
	jsr	StandardAddress	;
	sta	BkSrcAddress+1,x	;
	sty	BiosTemp2	;
	ldy	BkTarPage,x	;
	lda	BkTarAddress+1,x	; 修正目标地址与页
	pha	;
	jsr	StandardAddress	;
	sta	BkTarAddress+1,x	;
	sty	BiosTemp3	;
	ldy	BkLength,x	;
	beq	neebk04	; 低字节为 0：整页交换
	inc	BkLength+1,x	; 否则高字节加一（末尾不足一页）
	ldy	#0	;
neebk04:	sty	BiosTemp1	;
	lda	DRAMPageSave	; 保存当前页
	pha	;
	lda	BiosTemp2	;
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
lpebk04:	lda	BkLength+1,x	;
	cmp	#1	;
	bne	neebk12	;
	lda	BkLength,x	; 最后一页：只交换剩余字节
	sta	BiosTemp1	;
neebk12:	lda	BkSrcAddress+1,x	; 保存源地址
	pha	;
	lda	BkSrcAddress,x	;
	pha	;
lpebk08:	lda	(BkSrcAddress,x)	; 读入源到 MoveBuffer
	sta	MoveBuffer,y	;
	inc	BkSrcAddress,x	;
	bne	neebk16	;
	inc	BkSrcAddress+1,x	;
neebk16:	iny	;
	cpy	BiosTemp1	;
	bne	lpebk08	;
	ldy	#0	;
	lda	BiosTemp3	; 目标页：与缓冲区互换
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
lpebk12:	lda	(BkTarAddress,x)	;
	pha	;
	lda	MoveBuffer,y	;
	sta	(BkTarAddress,x)	;
	pla	;
	sta	MoveBuffer,y	;
	inc	BkTarAddress,x	;
	bne	neebk20	;
	inc	BkTarAddress+1,x	;
neebk20:	iny	;
	cpy	BiosTemp1	;
	bne	lpebk12	;
	pla	; 恢复源地址
	sta	BkSrcAddress,x	;
	pla	;
	sta	BkSrcAddress+1,x	;
	lda	BiosTemp2	;
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	ldy	#0	;
lpebk16:	lda	MoveBuffer,y	; 把缓冲区内容写回源
	sta	(BkSrcAddress,x)	;
	inc	BkSrcAddress,x	;
	bne	neebk24	;
	inc	BkSrcAddress+1,x	;
neebk24:	iny	;
	cpy	BiosTemp1	;
	bne	lpebk16	;
	dec	BkLength+1,x	; 页数减一
	bne	lpebk04	;
neebk28:	pla	; 恢复页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	pla	; 原目标地址
	cmp	#>($c000)	;
	bcc	neebk32	;
	lda	BkTarAddress+1,x	; 高字节加回 $40（C 为 1）
	adc	#$40-1	;
	sta	BkTarAddress+1,x	;
neebk32:	pla	; 原源地址
	cmp	#>($c000)	;
	bcc	neebk48	;
	lda	BkSrcAddress+1,x	; 高字节加回 $40（C 为 1）
	adc	#$40-1	;
	sta	BkSrcAddress+1,x	;
neebk48:	rts	;

;=======================================;
;    等待 X 次 VSync。
WaitVSynCnt:	;
	stx	NMIEnterFlag	; 计数由 NMI 递减
	jsr     EnableNMI	;
lpwvc04:	lda	NMIEnterFlag	;
	bne	lpwvc04	;
	jmp	RestoreNMI	;

;=======================================;
;    分配低内存（$5800-$5FFF）。
;    入口：X|Y 为字节数。
;    出口：A=0 表示分配成功，X|Y 为起始位置；A=1 表示出错，X|Y 为最大可用长度。
;---------------------------------------;
LowRAMAlloc:	;
	stx	BiosTemp2	;
	sty	BiosTemp1	;
	lda	LowRAMBegin+1	;
	cmp	#>($5800)	;
	bcc	nelra04	;
	lda     LowRAMBegin	;
	clc	;
	adc	BiosTemp1	;
	tay	;
	lda	LowRAMBegin+1	;
	adc	BiosTemp2	;
	cmp	#>($5c00)	; 超出 $5C00：从 $4400 重新分配
	bcc	nelra08	;
	ldx	#>($4400)	;
	ldy	#<($4400)	;
	stx	LowRAMBegin+1	;
	sty	LowRAMBegin	;
nelra04:	lda	LowRAMBegin	;
	clc	;
	adc	BiosTemp1	;
	tay	;
	lda	LowRAMBegin+1	;
	adc	BiosTemp2	;
	tax	;
	cmp	#>($5800)	;
	bcc	nelra08	; 未超出 $5800
	lda	#>($5800)	; 超出：计算最大可用长度
	sec	;
	sbc	LowRAMBegin	;
	tay	;
	lda	#<($5800)	;
	sbc	LowRAMBegin+1	;
	tax	;
	lda	#1	; 分配错误
	rts	;
nelra08:	tax	; 成功：LowRAMBegin 移到末尾，X|Y 返回原起始位置
	lda	LowRAMBegin+1	;
	stx	LowRAMBegin+1	;
	tax	;
	lda	LowRAMBegin	;
	sty	LowRAMBegin	;
	tay	;
	lda	#0	; 分配成功
	rts	;
;=======================================;
;    释放低内存（$5800-$5FFF）。入口：X|Y 为字节数。
;---------------------------------------;
LowRAMFree:	;
	stx	BiosTemp2	;
	sty	BiosTemp1	;
	lda	LowRAMBegin	;
	sec	;
	sbc	BiosTemp1	;
	sta	LowRAMBegin	;
	lda	LowRAMBegin+1	;
	sbc	BiosTemp2	;
	sta	LowRAMBegin+1	;
	rts	;
;---------------------------------------;
;    切换背景：按分屏数据（SplitScreenDt）设置滚动与显示模式。
SwitchBG:	;
	txa	;
	pha	;
	tya	;
	pha	;
	ldx	SplitPoint	;
	lda	SplitScreenDt,x	;
	beq	nesbg08	; 无分屏
	lda	$2002	; 复位 PPU 地址锁存
	lda	SplitScreenDt+3,x	;
	sta	$2005	;
	lda	SplitScreenDt+4,x	;
	sta	$2005	;
	lda	VideoCtrlSave	;
	and	#$f3	; 清除 WORK 位
	ora	#$40	; 置 MODE 位
	sta	VideoCtrlPort	;
	sta	$ff12	; 排队 INIT
lpsbg04:	lda	SplitScreenDt,x	;
	beq	nesbg04	;
	sta	$ff0a	; 写入 SR 队列
	lda	SplitScreenDt+2,x	;
	asl	;
	asl	;
	asl	;
	asl	;
	ora	SplitScreenDt+1,x	;
	sta	$ff1a	; 写入 VR 队列
	txa	;
	clc	;
	adc	#5	; 指向下一项（每项 5 字节）
	tax	;
	jmp	lpsbg04	;
nesbg04:	stx	SplitPoint	;
	sta	$ff22	; 启动
	lda	VideoCtrlSave	;
	ora	#$44	; WORD=1，MODE=1
	sta	VideoCtrlSave	;
	and	#$f7	;
	sta	VideoCtrlPort	;
	bne	nesbg12	; 总是跳转
nesbg08:	lda	VideoCtrlSave	;
	and	#$bb	; WORK=0，MODE=0
	sta	VideoCtrlSave	;
	and	#$f3	;
	sta	VideoCtrlPort	;
nesbg12:	pla	;
	tay	;
	pla	;
	tax	;
	rts	;
;---------------------------------------;
;    设置分屏。入口：X|Y 为分屏数据地址。
;    数据：首字节为计数，每项 5 字节，项首字节为 0 时结束。
;    第 3、4 字节为水平、垂直滚动（$2005）。
SplitScreen:	;
	lda	$ff	; 保存 $FE、$FF
	pha	;
	lda	$fe	;
	pha	;
	stx	$ff	;
	sty	$fe	;
	jsr	DisableNMI	;
	ldy	#0	; 第一行没有 AV12D 设置
	lda	($fe),y	;
	sec	;
	sbc	#1	; 计数减一
	sta	($fe),y	;
	sta	SplitScreenDt,y	;
	beq	nessn08	;
lpssn04:	ldx	#5	;
lpssn08:	lda	($fe),y	;
	sta	SplitScreenDt,y	;
	iny	;
	dex	;
	bne	lpssn08	;
	lda	($fe),y	;
	bne	lpssn04	;
	tya	;
	tax	;
	dec     SplitScreenDt-5,x	;末项加一，供第 241 行的 AV12 计数用
nessn08:	sty	SplitPoint	; 指向最后一项
nessn16:	pla	; 恢复 $FE、$FF
	sta	$fe	;
	pla	;
	sta	$ff	;
	ldx	#>(SplitScreenINT-$C000+$5800)	; 保存 BRK 向量为 SplitScreenINT
	ldy	#<(SplitScreenINT-$C000+$5800)	;
	stx	BRKAddressSave-$C000+$5800+1
	sty	BRKAddressSave-$C000+$5800
	lda	SplitScreenDt	;
	bne	nessn20	;
	ldx	#>(RTSInstruction-$C000+$5800)	; 无分屏：SEND 向量指向 RTS
	ldy	#<(RTSInstruction-$C000+$5800)	;
	stx	SEndAddressSave-$C000+$5800+1
	sty	SEndAddressSave-$C000+$5800
	rts	;
nessn20:	inc	SplitScreenDt	; 计数加一
	rts	;
;=======================================;
;    设置一个显示页。入口：X 为页位置（0-3，对应 0000、0800、1000、1800），Y 为页号。
SetVideoPage:	;
	txa	;
	asl	;
	asl	;
	asl	;
	tax	; 显示页寄存器间隔 8
	tya	;
	sta	VideoDataPort,x	;
	rts	;
;---------------------------------------;
;    取全部驱动器信息。出口：A 为软驱信息，Y 为 PC 卡信息。
GetDriveMessage:	;
	lda	SDriveMessage	;
	ldx	#0	;
	ldy	PCCardMessage	;
	rts	;
;---------------------------------------;
;    指针加 A（$FE/$FF 为 16 位指针）。
PointAdjust:	;
	clc	;
	adc	 $fe	;
	sta	 $fe	;
	bcc	nepad04	;
	inc	 $ff	;
nepad04:	rts	;
;---------------------------------------;
;    发送显存比较数据。入口：X|Y 为地址（X 为高字节）。
;---------------------------------------;
SendVRAMCmp:	;
	lda	 $fe	; 保存 $FE、$FF
	pha	;
	lda	 $ff	;
	pha	;
	stx	 $ff	;
	sty	 $fe	;
	lda	$2002	; 复位 PPU 地址锁存
lpsvc02:	ldy	#1	;
	lda	($fe),y	; 显存地址高字节
	sta	$2006	;
	dey	;
	lda	($fe),y	; 显存地址低字节
	sta	$2006	;
	lda	#$02	; 跳过地址的两个字节
lpsvc04:	jsr	PointAdjust	;
	ldy	#0	;
	lda	($fe),y	;
	cmp	#$ff	; 结束
	beq	nesvc12	;
	cmp	#$7f	; 换一个显存地址
	beq	nesvc08	;
	tay	; 长度
	bpl	nesvc04	; 最高位为 0：重复一个字节
;---------------------------------------; 字符串
	and	#$7f	; 复制 A & $7F 个字节
	tax	;
	ldy	#1	;
lpsvc08:	lda	($fe),y	;
	sta	$2007	;
	iny	;
	dex	;
	bne	lpsvc08	;
	tya	;
	bne	lpsvc04	;
;---------------------------------------; 重复
nesvc04:	tax	; 长度
	ldy	#$01	;
	lda	($fe),y	; 要重复的数据
lpsvc12:	sta	$2007	;
	dex	;
	bne	lpsvc12	;
	lda	#2	; 跳过长度与数据
	bne	lpsvc04	;
nesvc08:	lda	#1	;
	jsr	PointAdjust	;
	jmp	lpsvc02	;
nesvc12:	pla	; 恢复 $FE、$FF
	sta	 $ff	;
	pla	;
	sta	 $fe	;
	rts	;
;---------------------------------------;
;    把数据装入显存（VRAM）或 VROM。入口：X 为零页指针位置。
;    偏移 0：数据页；1-2：数据地址；3-4：显存地址（Y 不为 0 时）；5-6：数据长度。
LoadVideoData:	;
	lda	DRAMPageSave	;
	pha	;
	lda	0,x	; 数据页
	cmp	BiosUsePage	; 是否为保护页
	bne	nelvd04	;
	lda	DRAMPageNotUse	; 是：改用不使用的页
nelvd04:	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	tya	;
	bne	nelvd08	; Y 不为 0：直接写入 VRAM
	ldy	1,x	;
	lda	2,x	;
	tax	;
	jsr	SendVRAMCmp	;
	jmp	nelvd20	;
nelvd08:	bit	$2002	; 复位 PPU 地址锁存
	lda	4,x	; 显存地址高字节
	sta	$2006	;
	lda	3,x	; 显存地址低字节
	sta	$2006	;
	lda	5,x	;
	beq	lplvd04	; 长度低字节为 0：高字节不加一
	inc	6,x	;
lplvd04:	lda	(1,x)	; 读入数据写入显存
	sta	$2007	;
	inc	1,x	;
	bne	nelvd16	;
	inc	2,x	;
nelvd16:	dec	5,x	;
	bne	lplvd04	;
	dec	6,x	;
	bne	lplvd04	;
nelvd20:	pla	; 恢复页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	rts	;
;---------------------------------------;
;    设置用户 NMI 向量（需先调用）。入口：X|Y 为新向量地址。
SetNMIVector:	;
	stx	NMIVectorAddr-$C000+$5800+1
	sty	NMIVectorAddr-$C000+$5800
	rts	;
;---------------------------------------;
;    取用户 NMI 向量。出口：X|Y 为向量地址。
GetNMIVector:	;
	ldx	NMIVectorAddr-$C000+$5800+1
	ldy	NMIVectorAddr-$C000+$5800
	rts	;

;=======================================;
;    发出提示音（方波通道）。
SoundBell:	;
	lda	#1	;
	sta	$4015	;
	lda	#$8f	;
	sta	$4000	;
	lda	#0	;
	sta	$4001	;
	lda	#$50	;
	sta	$4002	;
	lda	#$80	;
	sta	$4003	;
	rts	;

ScreenWidth = 31	; 屏幕宽度（字符数）
;---------------------------------------;
;    取光标所在的显存地址。出口：X|Y 为地址。
GetCurAddress:	;
	lda	CurLine	; 行号是否超出显存页
	asl	;
	asl	;
	asl	;
	ldx	#>($2000)	; 第一个显存页
	adc	ScrollPanningN	;
	bcs	negca04	;
	cmp	#240	;
	bcc	negca08	;
negca04:	sec	; 是：换到下一个显存页
	sbc	#240	;
	ldx	#>($2800)	;
negca08:	ldy	#0	;
	sty	BiosTemp1	;
	asl	;
	rol	BiosTemp1	;
	asl	;
	rol	BiosTemp1	;
	ora	CurColumn	; 加上列号
	tay	; 显存地址低字节
	iny	; 第一列对齐
	txa	;
	ora	BiosTemp1	;
	tax	; 显存地址高字节
	lda	Addr2000N	; 显存横向滚动的高位是否启用
	lsr	;
	lsr	;
	bcc	negca12	;
	txa	; 是：异或 8 切换显存页
	eor	#8	;
	tax	;
negca12:	rts	;
;=======================================;
;    等待显示缓冲区清空。
WaitDisplay:	;
	lda	DispBfHead	;
	cmp	DispBfTail	;
	bne	WaitDisplay	;
	rts	;
;=======================================;
;    以 TTY 方式显示字符。入口：X 为字符。
DisplayTTY:	;
	txa	;
	cmp	#$07	;
	bne	nedty04	;
	jmp	SoundBell	; 响铃（BEL）
nedty04:	cmp	#$08	;
	bne	nedty16	; 退格
	ldy	CurColumn	;
	bne	nedty08	;
	ldx	CurLine	; 行首
	beq	nedty12	;
	dec	CurLine	; 回到上一行末尾
	ldy	#ScreenWidth	;
nedty08:	dey	; 列减一
	sty	CurColumn	;
nedty12:	rts	;
nedty16:	cmp	#$0d	;
	bne	nedty20	;
	ldy	#0	; 回车：回到行首
	sty	CurColumn	;
	rts	;
nedty20:	cmp	#$0a	;
	beq	nedty40	; 换行
;---------------------------------------;
	pha	; 保存字符
	jsr	GetCurAddress	;
lpdty04:	lda	DispBfHead	; 等待显示缓冲区有空间
	sec	;
	sbc	DispBfTail	;
	beq	nedty32	;
	cmp	#4	;
	bcc	lpdty04	;
nedty32:	txa	; 写入地址高字节、地址低字节、字符（3 字节）
	ldx	DispBfTail	;
	sta	DisplayBuffer,x	;
	inx	;
	tya	;
	sta	DisplayBuffer,x	;
	inx	;
	pla	;
	sta	DisplayBuffer,x	;
	inx	;
	stx	DispBfTail	;
	ldy	CurColumn	; 光标列加一
	iny	;
	cpy	#ScreenWidth	; 是否到行尾
	bcs	nedty36	;
	sty	CurColumn	;
nedty34:	rts	;
nedty36:	ldy	#0	; 是：换行
	sty	CurColumn	;
nedty40:	inc	CurLine	; 光标行加一
	lda	CurLine	;
	cmp	#30	; 是否到窗口底部
	bcc	nedty34	;
;    到窗口底部：滚动一行，并设置滚动偏移与显存页。
	lda	#0	;
	sta	BiosTemp1	;
	lda	ScrollPanningN	;
	asl	;
	rol	BiosTemp1	;
	asl	;
	rol	BiosTemp1	;
	pha	; 显存地址低字节
	lda	BiosTemp1	;
	ora	#$20	;
	tax	; 显存地址高字节
	lda	Addr2000N	; 显存横向滚动的高位是否启用
	lsr	;
	lsr	;
	bcc	nedty44	;
	txa	; 是：异或 8 切换显存页
	eor	#8	;
	tax	;
nedty44:	txa	;
	pha	;
	lda	ScrollPanningN	; 下一行的显存地址
	clc	;
	adc	#8	; 进位为 1
	cmp	#240	;
	bcc	nedty52	;
	sbc	#240	; 进位为 1
	tay	;
lpdty08:	lda	DispBfHead	; 等待显示缓冲区有空间
	sec	;
	sbc	DispBfTail	;
	beq	nedty48	;
	cmp	#3	;
	bcc	lpdty08	;
nedty48:	ldx	DispBfTail	;
	lda	#$80	; 10 类型：修改 $2000 的名称表位
	sta	DisplayBuffer,x	;
	inx	;
	lda	Addr2000N	;
	eor	#2	;
	sta	Addr2000N	;
	sta	DisplayBuffer,x	;
	inx	;
	stx	DispBfTail	;
	tya	;
nedty52:	sta	ScrollPanningN	;
	tay	;
lpdty12:	lda	DispBfHead	; 等待显示缓冲区有空间
	sec	;
	sbc	DispBfTail	;
	beq	nedty56	;
	cmp	#3	;
	bcc	lpdty12	;
nedty56:	ldx	DispBfTail	;
	lda	#$81	; 10 类型：设置滚动偏移
	sta	DisplayBuffer,x	;
	inx	;
	tya	;
	sta	DisplayBuffer,x	;
	inx	;
	stx	DispBfTail	;
	pla	; 恢复显存地址
	tax	;
	pla	;
	tay	;
	iny	; 第一列对齐
	lda	#ScreenWidth	;
	sta	BiosTemp2	;
	lda	#$20	; 清空新行
	jsr	SetCharCount	;
	lda	#30-1	;
	sta	CurLine	; 光标回到窗口底行
	rts	;
;=======================================;
;    在光标处连续显示同一字符。入口：X 为字符，Y 为个数。
DisplayChars:	;
	txa	;
	pha	;
	sty	BiosTemp2	; 个数
	lda	#ScreenWidth	;
	sec	;
	sbc	CurColumn	; 行内剩余列数
	cmp	BiosTemp2	;
	bcs	nedcs04	;
	sta	BiosTemp2	; 不超过行尾
nedcs04:	jsr	GetCurAddress	;
	pla	; 取回字符
;---------------------------------------;
;    写入字符重复记录。入口：X|Y 为显存地址，A 为字符，BiosTemp2 为个数。
;---------------------------------------;
SetCharCount:	;
	pha	;
lpdcs04:	lda	DispBfHead	; 等待显示缓冲区有空间
	sec	;
	sbc	DispBfTail	;
	beq	nedcs08	;
	cmp	#5	;
	bcc	lpdcs04	;
nedcs08:	txa	; 地址高字节
	ora	#$40	; 01 类型：输出计数
	ldx	DispBfTail	;
	sta	DisplayBuffer,x	;
	inx	;
	tya	; 地址低字节
	sta	DisplayBuffer,x	;
	inx	;
	pla	; 字符
	sta	DisplayBuffer,x	;
	inx	;
	lda	BiosTemp2	; 个数
	sta	DisplayBuffer,x	;
	inx	;
	stx	DispBfTail	;
	rts	;
;=======================================;
;    设置光标。入口：X 为行，Y 为列。
SetCursor:	;
	stx	CurLine	;
	sty	CurColumn	;
	rts	;
;=======================================;
;    取光标。出口：X 为行，Y 为列，A 为屏幕行数。
GetCursor:	;
	ldx	CurLine	;
	ldy	CurColumn	;
	lda	#30	; 屏幕行数
	rts	;
;---------------------------------------;
ASCII8x8Font:	;
	.word 0,	0,	0,	0,	$817E,	$81A5,	$99BD,	$7E81,	$FF7E,	$FFDB,	$E7C3,	$7EFF,	$FE6C,	$FEFE,	$387C,	$10
	.word $3810,	$FE7C,	$387C,	$10,	$7C38,	$FE38,	$7CFE,	$7C38,	$1010,	$7C38,	$7CFE,	$7C38,	0,	$3C18,	$183C,	0
	.word $FFFF,	$C3E7,	$E7C3,	$FFFF,	$3C00,	$4266,	$6642,	$3C,	$C3FF,	$BD99,	$99BD,	$FFC3,	$70F,	$7D0F,	$CCCC,	$78CC
	.word $663C,	$6666,	$183C,	$187E,	$333F,	$303F,	$7030,	$E0F0,	$637F,	$637F,	$6763,	$C0E6,	$5A99,	$E73C,	$3CE7,	$995A
	.word $E080,	$FEF8,	$E0F8,	$80,	$E02,	$FE3E,	$E3E,	2,	$3C18,	$187E,	$7E18,	$183C,	$6666,	$6666,	$66,	$66
	.word $DB7F,	$7BDB,	$1B1B,	$1B,	$633E,	$6C38,	$386C,	$78CC,	0,	0,	$7E7E,	$7E,	$3C18,	$187E,	$3C7E,	$FF18
	.word $3C18,	$187E,	$1818,	$18,	$1818,	$1818,	$3C7E,	$18,	$1800,	$FE0C,	$180C,	0,	$3000,	$FE60,	$3060,	0
	.word 0,	$C0C0,	$FEC0,	0,	$2400,	$FF66,	$2466,	0,	$1800,	$7E3C,	$FFFF,	0,	$FF00,	$7EFF,	$183C,	0
	.word 0,	0,	0,	0,	$7830,	$7878,	$30,	$30,	$6C6C,	$6C,	0,	0,	$6C6C,	$6CFE,	$6CFE,	$6C
	.word $7C30,	$78C0,	$F80C,	$30,	$C600,	$18CC,	$6630,	$C6,	$6C38,	$7638,	$CCDC,	$76,	$6060,	$C0,	0,	0
	.word $3018,	$6060,	$3060,	$18,	$3060,	$1818,	$3018,	$60,	$6600,	$FF3C,	$663C,	0,	$3000,	$FC30,	$3030,	0
	.word 0,	0,	$3000,	$6030,	0,	$FC00,	0,	0,	0,	0,	$3000,	$30,	$C06,	$3018,	$C060,	$80
	.word $C67C,	$DECE,	$E6F6,	$7C,	$7030,	$3030,	$3030,	$FC,	$CC78,	$380C,	$CC60,	$FC,	$CC78,	$380C,	$CC0C,	$78
	.word $3C1C,	$CC6C,	$CFE,	$1E,	$C0FC,	$CF8,	$CC0C,	$78,	$6038,	$F8C0,	$CCCC,	$78,	$CCFC,	$180C,	$3030,	$30
	.word $CC78,	$78CC,	$CCCC,	$78,	$CC78,	$7CCC,	$180C,	$70,	$3000,	$30,	$3000,	$30,	$3000,	$30,	$3000,	$6030
	.word $3018,	$C060,	$3060,	$18,	0,	$FC,	$FC00,	0,	$3060,	$C18,	$3018,	$60,	$CC78,	$180C,	$30,	$30
	.word $C67C,	$DEDE,	$C0DE,	$78,	$7830,	$CCCC,	$CCFC,	$CC,	$66FC,	$7C66,	$6666,	$FC,	$663C,	$C0C0,	$66C0,	$3C
	.word $6CF8,	$6666,	$6C66,	$F8,	$607E,	$7860,	$6060,	$7E,	$607E,	$7860,	$6060,	$60,	$663C,	$C0C0,	$66CE,	$3E
	.word $CCCC,	$FCCC,	$CCCC,	$CC,	$3078,	$3030,	$3030,	$78,	$C1E,	$C0C,	$CCCC,	$78,	$66E6,	$786C,	$666C,	$E6
	.word $6060,	$6060,	$6060,	$7E,	$EEC6,	$FEFE,	$C6D6,	$C6,	$E6C6,	$DEF6,	$C6CE,	$C6,	$6C38,	$C6C6,	$6CC6,	$38
	.word $66FC,	$7C66,	$6060,	$F0,	$CC78,	$CCCC,	$78DC,	$1C,	$66FC,	$7C66,	$666C,	$E6,	$CC78,	$70E0,	$CC1C,	$78
	.word $30FC,	$3030,	$3030,	$30,	$CCCC,	$CCCC,	$CCCC,	$FC,	$CCCC,	$CCCC,	$78CC,	$30,	$C6C6,	$D6C6,	$EEFE,	$C6
	.word $C6C6,	$386C,	$6C38,	$C6,	$CCCC,	$78CC,	$3030,	$78,	$6FE,	$180C,	$6030,	$FE,	$6078,	$6060,	$6060,	$78
	.word $60C0,	$1830,	$60C,	$2,	$1878,	$1818,	$1818,	$78,	$3810,	$C66C,	0,	0,	0,	0,	0,	$FF00
	.word $3030,	$18,	0,	0,	0,	$C78,	$CC7C,	$76,	$60E0,	$7C60,	$6666,	$DC,	0,	$CC78,	$CCC0,	$78
	.word $C1C,	$7C0C,	$CCCC,	$76,	0,	$CC78,	$C0FC,	$78,	$6C38,	$F060,	$6060,	$F0,	0,	$CC76,	$7CCC,	$F80C
	.word $60E0,	$766C,	$6666,	$E6,	$30,	$3070,	$3030,	$78,	$C,	$C0C,	$CC0C,	$78CC,	$60E0,	$6C66,	$6C78,	$E6
	.word $3070,	$3030,	$3030,	$78,	0,	$FECC,	$D6FE,	$C6,	0,	$CCF8,	$CCCC,	$CC,	0,	$CC78,	$CCCC,	$78
	.word 0,	$66DC,	$7C66,	$F060,	0,	$CC76,	$7CCC,	$1E0C,	0,	$76DC,	$6066,	$F0,	0,	$C07C,	$C78,	$F8
	.word $3010,	$307C,	$3430,	$18,	0,	$CCCC,	$CCCC,	$76,	0,	$CCCC,	$78CC,	$30,	0,	$D6C6,	$FEFE,	$6C
	.word 0,	$6CC6,	$6C38,	$C6,	0,	$CCCC,	$7CCC,	$F80C,	0,	$98FC,	$6430,	$FC,	$301C,	$E030,	$3030,	$1C
	.word $1818,	$18,	$1818,	$18,	$30E0,	$1C30,	$3030,	$E0,	$DC76,	0,	0,	0,	$1000,	$6C38,	$C6C6,	$FE
	.word $CC78,	$CCC0,	$1878,	$780C,	$CC00,	$CC00,	$CCCC,	$7E,	$1C,	$CC78,	$C0FC,	$78,	$C37E,	$63C,	$663E,	$3F
	.word $CC,	$C78,	$CC7C,	$7E,	$E0,	$C78,	$CC7C,	$7E,	$3030,	$C78,	$CC7C,	$7E,	0,	$C078,	$78C0,	$380C
	.word $C37E,	$663C,	$607E,	$3C,	$CC,	$CC78,	$C0FC,	$78,	$E0,	$CC78,	$C0FC,	$78,	$CC,	$3070,	$3030,	$78
	.word $C67C,	$1838,	$1818,	$3C,	$E0,	$3070,	$3030,	$78,	$38C6,	$C66C,	$C6FE,	$C6,	$3030,	$7800,	$FCCC,	$CC
	.word $1C,	$60FC,	$6078,	$FC,	0,	$C7F,	$CC7F,	$7F,	$6C3E,	$FECC,	$CCCC,	$CE,	$CC78,	$7800,	$CCCC,	$78
	.word $CC00,	$7800,	$CCCC,	$78,	$E000,	$7800,	$CCCC,	$78,	$CC78,	$CC00,	$CCCC,	$7E,	$E000,	$CC00,	$CCCC,	$7E
	.word $CC00,	$CC00,	$7CCC,	$F80C,	$18C3,	$663C,	$3C66,	$18,	$CC,	$CCCC,	$CCCC,	$78,	$1818,	$C07E,	$7EC0,	$1818
	.word $6C38,	$F064,	$E660,	$FC,	$CCCC,	$FC78,	$FC30,	$3030,	$CCF8,	$FACC,	$CFC6,	$C7C6,	$1B0E,	$3C18,	$1818,	$70D8
	.word $1C,	$C78,	$CC7C,	$7E,	$38,	$3070,	$3030,	$78,	$1C00,	$7800,	$CCCC,	$78,	$1C00,	$CC00,	$CCCC,	$7E
	.word $F800,	$F800,	$CCCC,	$CC,	$FC,	$ECCC,	$DCFC,	$CC,	$6C3C,	$3E6C,	$7E00,	0,	$6C38,	$386C,	$7C00,	0
	.word $30,	$6030,	$CCC0,	$78,	0,	$FC00,	$C0C0,	0,	0,	$FC00,	$C0C,	0,	$C6C3,	$DECC,	$6633,	$FCC
	.word $C6C3,	$DBCC,	$6F37,	$3CF,	$1818,	$1800,	$1818,	$18,	$3300,	$CC66,	$3366,	0,	$CC00,	$3366,	$CC66,	0
	.word $8822,	$8822,	$8822,	$8822,	$AA55,	$AA55,	$AA55,	$AA55,	$77DB,	$EEDB,	$77DB,	$EEDB,	$1818,	$1818,	$1818,	$1818
	.word $1818,	$1818,	$18F8,	$1818,	$1818,	$18F8,	$18F8,	$1818,	$3636,	$3636,	$36F6,	$3636,	0,	0,	$36FE,	$3636
	.word 0,	$18F8,	$18F8,	$1818,	$3636,	$6F6,	$36F6,	$3636,	$3636,	$3636,	$3636,	$3636,	0,	$6FE,	$36F6,	$3636
	.word $3636,	$6F6,	$FE,	0,	$3636,	$3636,	$FE,	0,	$1818,	$18F8,	$F8,	0,	0,	0,	$18F8,	$1818
	.word $1818,	$1818,	$1F,	0,	$1818,	$1818,	$FF,	0,	0,	0,	$18FF,	$1818,	$1818,	$1818,	$181F,	$1818
	.word 0,	0,	$FF,	0,	$1818,	$1818,	$18FF,	$1818,	$1818,	$181F,	$181F,	$1818,	$3636,	$3636,	$3637,	$3636
	.word $3636,	$3037,	$3F,	0,	0,	$303F,	$3637,	$3636,	$3636,	$F7,	$FF,	0,	0,	$FF,	$36F7,	$3636
	.word $3636,	$3037,	$3637,	$3636,	0,	$FF,	$FF,	0,	$3636,	$F7,	$36F7,	$3636,	$1818,	$FF,	$FF,	0
	.word $3636,	$3636,	$FF,	0,	0,	$FF,	$18FF,	$1818,	0,	0,	$36FF,	$3636,	$3636,	$3636,	$3F,	0
	.word $1818,	$181F,	$1F,	0,	0,	$181F,	$181F,	$1818,	0,	0,	$363F,	$3636,	$3636,	$3636,	$36FF,	$3636
	.word $1818,	$18FF,	$18FF,	$1818,	$1818,	$1818,	$F8,	0,	0,	0,	$181F,	$1818,	$FFFF,	$FFFF,	$FFFF,	$FFFF
	.word 0,	0,	$FFFF,	$FFFF,	$F0F0,	$F0F0,	$F0F0,	$F0F0,	$F0F,	$F0F,	$F0F,	$F0F,	$FFFF,	$FFFF,	0,	0
	.word 0,	$DC76,	$DCC8,	$76,	$7800,	$F8CC,	$F8CC,	$C0C0,	$FC00,	$C0CC,	$C0C0,	$C0,	$FE00,	$6C6C,	$6C6C,	$6C
	.word $CCFC,	$3060,	$CC60,	$FC,	0,	$D87E,	$D8D8,	$70,	$6600,	$6666,	$7C66,	$C060,	$7600,	$18DC,	$1818,	$18
	.word $30FC,	$CC78,	$78CC,	$FC30,	$6C38,	$FEC6,	$6CC6,	$38,	$6C38,	$C6C6,	$6C6C,	$EE,	$301C,	$7C18,	$CCCC,	$78
	.word 0,	$DB7E,	$7EDB,	0,	$C06,	$DB7E,	$7EDB,	$C060,	$6038,	$F8C0,	$60C0,	$38,	$CC78,	$CCCC,	$CCCC,	$CC
	.word $FC00,	$FC00,	$FC00,	0,	$3030,	$30FC,	$30,	$FC,	$3060,	$3018,	$60,	$FC,	$3018,	$3060,	$18,	$FC
	.word $1B0E,	$181B,	$1818,	$1818,	$1818,	$1818,	$D818,	$70D8,	$3030,	$FC00,	$3000,	$30,	$7600,	$DC,	$DC76,	0
	.word $6C38,	$386C,	0,	0,	0,	$1800,	$18,	0,	0,	0,	$18,	0,	$C0F,	$C0C,	$6CEC,	$1C3C
	.word $6C78,	$6C6C,	$6C,	0,	$1870,	$6030,	$78,	0,	0,	$3C3C,	$3C3C,	0,	0,	0,	0,	0
;---------------------------------------;
;    调色板的 4 个颜色编号（从 $3F00 起写入）。
PaletteTableASC:	.byte $0e,	$20,	$20,	$30
PaletteASCLen = *-PaletteTableASC	; 调色板长度
;---------------------------------------;
;    设置 VRAM 地址为 X 的高字节、低字节 0。入口：X 为地址高字节。
VRAMBegin:	;
	ldy	#0	; 低字节为 0
	bit	$2002	;
	stx	$2006	;
	sty	$2006	;
	rts	;
;---------------------------------------;
;    向 $2007 写入 Y 个 0。
Send2007_0:	;
	lda	#0	;
;    向 $2007 写入 Y 个 A 的值。入口：A 为值，Y 为个数。
Send2007:	;
lps2704:	sta	$2007	;
	dey	;
	bne	lps2704	;
	rts	;
;=======================================;
;    初始化显示：关闭屏幕，装入字库、名称表、属性表与调色板，然后打开屏幕。
DispInitial:	;
	lda	#0	; 关闭屏幕
	sta	Addr2000	;
	sta	$2000	;
	sta	$2001	;
	lda	VideoCtrlSave	; 关闭计数中断
	and	#$bb	;
	sta	VideoCtrlSave	;
	sta	VideoCtrlPort	;
	lda	#0	;
	sta	SplitPoint	; 不使用分屏
	jsr	InitVideoPort	;
	ldx	#>($0000)	; 装入 256 个 ASCII 字库字符
	jsr	VRAMBegin	;
	ldx	#>(ASCII8x8Font)	; 字库地址（$FE/$FF 为指针）
	ldy	#<(ASCII8x8Font)	;
	stx	$fe+1	;
	sty	$fe	;
	ldx	#<(256)	;
	ldy	#0	;
lpcas04:	lda	($fe),y	;
	sta	$2007	;
	iny	;
	cpy	#8	;
	bne	lpcas04	;
	jsr	Send2007_0	; 另外 8 行写 0
	lda	$fe	; 指向下一个字符
	clc	;
	adc	#8	;
	sta	$fe	;
	bcc	necas08	;
	inc	$fe+1	;
necas08:	dex	;
	bne	lpcas04	;
;---------------------------------------; 初始化名称表 0
	ldx	#>($2000)	;
	jsr	VRAMBegin	;
	ldx	#>($03c0+255)	; 960 个空格：先写 192 个，再写 3 个整页
	ldy	#<($03c0)	;
	lda	#$20	;
lpcas08:	jsr	Send2007	;
	dex	;
	bne	lpcas08	;
	ldy	#$40	; 属性表（64 字节）清零
	jsr	Send2007_0	;
;---------------------------------------; 初始化名称表 2
	ldx	#>($2800)	;
	jsr	VRAMBegin	;
	ldx     #>($03c0+255)	; 960 个空格：先写 192 个，再写 3 个整页
	ldy	#<($03c0)	;
	lda	#$20	;
lpcas12:	jsr	Send2007	;
	dex	;
	bne	lpcas12	;
	ldy	#$40	; 属性表（64 字节）清零
	jsr	Send2007_0	;
;---------------------------------------; 初始化调色板
	ldx	#1	; 等待 1 次 VSync
	jsr	WaitVSynCnt	;
	ldx	#>($3f00)	;
	jsr	VRAMBegin	;
	ldy	#0	;
lpcas16:	lda	PaletteTableASC,y	;
	sta	$2007	;
	iny	;
	cpy	#PaletteASCLen	;
	bne	lpcas16	;
	ldx	#>($2000)	;
	jsr	VRAMBegin	;
	lda	#0	; 滚动归零
	sta	$2005	;
	sta	$2005	;
	sta	CurLine	; 光标归位
	sta	CurColumn	;
	lda	#$0c	; 打开背景显示
	sta	Addr2001	;
	sta	$2001	;
	bit	$2002	;
	lda	#$80	; 开启 NMI
	sta	Addr2000	;
	rts	;

;=======================================;
;    CRC 与解压寄存器（零页）。
CRCDWord = $d0	; CRC 寄存器（4 字节）
CRCSave = $d4	; 保存的 CRC（4 字节）
SourceLength = $d8	; 源数据剩余长度（4 字节）
SourceAddress = $dc	; 源地址（2 字节）
SourcePage = $de	; 源页号
TargetAddress = $df	; 目标地址（2 字节）
TargetPage = $e1	; 目标页号
ZipFlag = $e2	; 压缩标志
ZipType = $e3	; 压缩方法
ReadAll = $e4	; 读取状态
BackDistance = $e5	; 回溯距离（2 字节）
D5f68 = $e7	; 临时变量（用途待确认）
D5f5a = $e8	; 临时变量（用途待确认）
D5f5e = $e9	; 临时变量（用途待确认）
D527b = $ea	; 临时变量（用途待确认）
;		$EB-$FF：临时工作区。	;
;---------------------------------------;
;    解压用缓冲区与表的地址。
ZeroSave = $6100	; 解压时保存 $D0-$FF 的区域
SourceBuffer = $6200	; 源数据缓冲区（$100 字节）
TargetBuffer = $6300	; 目标缓冲区（$200 + $140 字节，需页对齐）
TargetBfLen = $0800	; 目标缓冲区长度
TargetBfTail = $6b00	; 与 D4da2 同地址
TargetBfEnd = $6b3f	; 表区末尾
;---------------------------------------;
TempBuffer = $6700	; 临时缓冲区（$10 + $100 + $100 × 2）
D4da2 = $6b00	; 表（$40 + $40 × 2 字节）
D4de2 = $6b40	; 表
D4e62 = $6bc0	; 表（$40 + $40 × 2 字节）
D4ea2 = $6c00	; 表
;---------------------------------------;
HasiTable = $6c00	; CRC-32 查表（4 段 $100 字节，需 4 字节对齐）
;---------------------------------------;
D4f22 = $6d00	; 表（$100 字节）
D5022 = $6e00	; 表（$200 字节）
;---------------------------------------;
D3da2 = $7000	; 表（$200 字节）
D3fa2 = $7200	; 表（$200 字节）
D41a2 = $7400	; 表（$200 字节）
D43a2 = $7600	; 表（$200 字节）
D45a2 = $7800	; 表（$200 字节）
D47a2 = $7a00	; 表（$200 字节）
D49a2 = $7c00	; 表（$200 字节）
D4ba2 = $7e00	; 表（$100 字节）
D4ca2 = $7f00	; 表（$100 字节）
; PKZIP 1.10 解压程序
;   PKZIP 1.10 文件头格式（$1E 字节）：
;	00-03：标记 "PK",3,4
;*	06：压缩参数
;*	08：压缩方法 0 存储，1 收缩（Shrunk），6 内爆（Implode），8 放气（Deflate，V2.04）
;*	0E-11：32 位 CRC
;*	12-15：压缩后长度
;	16-19：原始长度
;	1A：文件名长度
;	1E-：文件名（$1A 字节）
;   新文件头格式（10 字节）：
;	00：压缩参数
;	01：压缩方法 0 存储，1 收缩，6 内爆，8 放气
;	02-05：32 位 CRC
;	06-09：压缩后长度
;===============================================;
;    解压内存块。
;    入口：X 为块结构偏移（MoveBlockStruct）；
;          Y 的位 7 为 1 时校验 CRC；位 6 为 1 时源缓冲区需要分页（为 0 时不调用 SetPage）。
;    出口：Y=0 表示 CRC 校验通过。
;-----------------------------------------------;
UnzipBlock:	;
	sty	BiosTemp1	; CRC 校验标志
	ldy	#$30-1	;
lpubk04:	lda	$d0,y	; 保存 $D0-$FF
	sta	ZeroSave,y	;
	dey	;
	bpl	lpubk04	;
	lda	BkSrcPage,x	;
	sta	SourcePage	;
	lda	BkSrcAddress,x	;
	sta	SourceAddress	;
	lda	BkSrcAddress+1,x	;
	sta	SourceAddress+1	;
	lda	BkTarPage,x	;
	sta	TargetPage	;
	sta	BiosTemp2	; 目标页
	ldx	#>($8000)	;
	ldy	#<($8000)	;
	stx	TargetAddress+1	; 目标地址固定为 $8000
	sty	TargetAddress	;
;---------------------------------------;
	lda	DRAMPageSave	; 保存页
	pha	;
	jsr	ExplodeFile	;
	bit	BiosTemp1	;
	bpl	neubk16	;
	lda	#$ff	; CRC 初始值 $FFFFFFFF
	sta	CRCDWord	;
	sta	CRCDWord+1	;
	sta	CRCDWord+2	;
	sta	CRCDWord+3	;
	jsr	BuildHSTable	;
lpubk08:	ldx	#>($4000)	; 每次校验 16K（X|Y 为长度）
	ldy	#<($4000)	;
	lda	BiosTemp2	; 是否为最后一页
	cmp	TargetPage	;
	bcc	neubk04	;
	bne	neubk08	;
	lda	TargetAddress+1	; 是：计算最后一段的长度
	and	#>($7fff)	;
	tax	;
	ora	TargetAddress	;
	beq	neubk08	;
	ldy	TargetAddress	;
neubk04:	lda	BiosTemp2	;
	jsr	GetBlockCRC	; 使用 $F6-$FF
	inc	BiosTemp2	;
	jmp	lpubk08	;
neubk08:	ldx	#3	;
lpubk12:	lda	CRCDWord,x	; 比较 CRC（取反后与保存值比较）
	eor	#$ff	;
	cmp	CRCSave,x	;
	bne	neubk12	;
	dex	;
	bpl	lpubk12	;
	ldy	#0	; 一致：Y=0
	beq	neubk16	;
neubk12:	ldy	#1	; 不一致
neubk16:	ldx	#$30-1	;
lpubk16:	lda	ZeroSave,x	; 恢复 $D0-$FF
	sta	$d0,x	;
	dex	;
	bpl	lpubk16	;
	pla	; 恢复页
	sta	DRAMPageSave	;
	sta	DRAMPagePort	;
	rts	;
;---------------------------------------;
;    计算一段内存的 CRC。入口：X|Y 为长度，数据起于 $8000。占用 $F6-$FF。
GetBlockCRC:	;
	stx	$fd	;
	sty	$fc	;
	tax	; 页号（SetPage 用 X）
	jsr	SetPage-$C000+$5800	;
	tya	;
	beq	negbc04	;
	inc	$fd	; 低字节非 0：多一页
negbc04:	ldx	#>($8000)	;
	ldy	#<($8000)	;
	stx	$f6+1	;
	sty	$f6	;
	ldy	#0	;
lpgbc04:	lda	($f6),y	; 读入一字节
	eor	CRCDWord	;
	tax	;
	lda	CRCDWord+1	; CRC 右移一字节，再与查表值异或
	sta	CRCDWord	;
	lda	CRCDWord+2	;
	sta	CRCDWord+1	;
	lda	CRCDWord+3	;
	sta	CRCDWord+2	;
	lda	CRCDWord	;
	eor	HasiTable,x	;
	sta	CRCDWord	;
	lda	CRCDWord+1	;
	eor	HasiTable+$100,x	;
	sta	CRCDWord+1	;
	lda	CRCDWord+2	;
	eor	HasiTable+$200,x	;
	sta	CRCDWord+2	;
	lda	HasiTable+$300,x	;
	sta	CRCDWord+3	;
	iny	;
	bne	negbc08	;
	inc	$f7	; 跨页
negbc08:	dec	$fc	;
	bne	lpgbc04	;
	dec	$fd	;
	bne	lpgbc04	;
	rts	;
;---------------------------------------;
;    指针加 A（X 指向的 16 位指针）。
PointAdjustUnzip:	;
	clc	;
	adc	0,x	;
	sta	0,x	;
	bcc	nepau04	;
	inc	1,x	;
nepau04:	rts	;
;---------------------------------------;
;    解压程序使用的模拟寄存器（沿用原名）。
SI = $ee	;
BP = $ef	;
DI = $f0	;
DL = $f2	;
DH = $f3	;
CL = $f4	;
CH = $f5	;
AX = $f6	;
AL = $f6	;
AH = $f7	;
;---------------------------------------;
;    从源读取一块数据。入口：Y 为要读取的长度。出口：(BP)-1 为实际读取的长度。
;---------------------------------------;
ReadSourceBlock:	;
	sty	$ff	;
	lda	SourceLength	; 剩余长度减去读取长度
	sec	;
	sbc	$ff	;
	bcs	nersb16	; 剩余足够
	ldx	SourceLength+1	; 剩余不足：只读取剩余的全部数据
	bne	nersb12	;
	ldx	SourceLength+2	;
	bne	nersb08	;
	ldx	SourceLength+3	;
	bne	nersb04	;
	ldy	SourceLength	;
	lda	#0	;
	beq	nersb16	;
nersb04:	dec	SourceLength+3	; 多字节长度减一（借位）
nersb08:	dec	SourceLength+2	;
nersb12:	dec	SourceLength+1	;
nersb16:	sta	SourceLength	; 保存新的剩余长度低字节
	sty	BP	;
	inc	BP	; BP 为实际读取长度加一
	tya	; 本次搬移总长度
	ldx	#>(SourceBuffer)	; 目标：源缓冲区
	ldy	#<(SourceBuffer)	;
	stx	$fe+1	;
	sty	$fe	;
lpmse04:	sta	$fa	; 剩余长度
	sta	$fb	; 本次搬移长度
	ldx	SourceAddress+1	;
	ldy	SourceAddress	;
	stx	$fc+1	; 源指针 $FC/$FD
	sty	$fc	;
	bit	BiosTemp1	; 源是否分页（位 6）
	bvc	nemse08	;
	cpx	#>($c000-$0100)	; 是否接近 $C000 边界
	bcc	nemse04	;
	lda	$fc	; 页内剩余长度是否小于搬移长度
	clc	;
	adc	$fa	;
	bcc	nemse04	;
	lda	#<($c000)	; 是：只搬移到 $C000 为止
	sbc	$fc	;
	sta	$fb	;
nemse04:	ldx	SourcePage	; 源页
	jsr	SetPage-$C000+$5800	; 映射源页
nemse08:	ldy	#0	;
lpmse08:	lda	($fc),y	; 复制 $FB 个字节
	sta	($fe),y	;
	iny	;
	cpy	$fb	;
	bne	lpmse08	;
	tya	;
	clc	;
	adc	SourceAddress	; 源地址前进
	sta	SourceAddress	;
	bcc	nemse12	;
	inc	SourceAddress+1	;
	bit	BiosTemp1	; 源是否分页
	bvc	nemse12	;
	bit	SourceAddress+1	; 高字节到达 $C0 时转到下一页
	bvc	nemse12	;
	inc	SourcePage	; 下一页
	lda	#>($8000)	; 源地址回到 $8000
	sta	SourceAddress	;
nemse12:	lda	$fa	;
	cmp	$fb	;
	beq	nemse16	; 本次搬移完成
	sbc	$fb	;
	sta	$fa	;
	tya	;
	clc	;
	adc	$fe	; 目标指针前进
	sta	$fe	;
	bcc	lpmse04	;
	inc	$ff	;
	jmp	lpmse04	;
nemse16:	rts	;
;---------------------------------------;
;    读取码表并填入表。入口：$F3|$F2 为表地址，Y 为表长度。
ReadTable:	;
	sty	$f0	; 表长度
	ldy	#1	; 读取表头
	jsr	ReadSourceBlock	;
	ldy	SourceBuffer	;
	iny	;
	jsr	ReadSourceBlock	; 读取表数据到源缓冲区
	ldx	#0	;
lprbt04:	lda	SourceBuffer,x	;
	lsr	a	;
	lsr	a	;
	lsr	a	;
	lsr	a	; 高 4 位为步长（$F1）
	tay	;
	sty	$f1	;
	lda	SourceBuffer,x	;
	and	#$0f	; 低 4 位加一为填入的值
	clc	;
	adc	#1	;
lprbt08:	sta	($f2),y	;
	dey	;
	bpl	lprbt08	;
	inx	;
	lda	$f2	; 指针前进（步长加一）
	sec	;
	adc	$f1	;
	sta	$f2	;
	bcc	nerbt04	;
	inc	$f3	;
nerbt04:	dec	$f0	; 表长度减一
	beq	nerbt08	;
	lda	$f0	;
	sec	;
	sbc	$f1	;
	beq	nerbt08	;
	bcc	nerbt08	;
	sta	$f0	;
	bcs	lprbt04	;
nerbt08:	rts	;
;---------------------------------------;
;    位掩码表（逐位左移用）。
RoteLeft:	.byte $01,$02,$04,$08,$10,$20,$40,$80
;    建立码表（填充 D3da2、D3fa2、D41a2 等表）。用途细节待确认。
BuildTable:	;
	ldy	#$3f	; 64 项
lpbte04:	lda	D4da2,y	;
	sta	$ff	;
	cmp	#$10	;
	bcs	nebte12	; 位数不小于 16：转到后段
	tya	;
	asl	a	; 码值每项 2 字节
	tax	;
	lda	D4de2,x	;
	pha	;
	ldx	$ff	;
	cpx	#8	;
	bcs	nebte04	;
	lda	RoteLeft,x	; 位数小于 8：取低字节掩码
	tax	;
	lda	#0	;
	beq	nebte08	;
nebte04:	lda	RoteLeft-8,x	; 位数不小于 8：取高字节掩码
	ldx	#0	;
nebte08:	sta	$f1	; A|X 为步长（$F1|$F0）
	stx	$f0	;
	pla	;
	tax	;
lpbte08:	lda	$ff	; 按步长填入两个表
	sta	D4ca2,x	;
	tya	;
	sta	D4ba2,x	;
	txa	;
	clc	;
	adc	$f0	;
	tax	;
	lda	#0	;
	adc	$f1	;
	beq	lpbte08	;
nebte12:	tya	;
	asl	a	;
	tax	;
	lda	D4ea2+1,x	; 取 SI（$F3|$F2）
	sta	$f3	;
	lda	D4ea2,x	;
	sta	$f2	;
	lda	D4e62,y	;
	cmp	#9	; 位数是否大于 8
	bcs	nebte16	;
	ldx	#>(D3da2)	; $F1|$F0 指向 D3da2
	stx	$f1	;
	ldx	#<(D3da2)	;
	stx	$f0	;
	jmp	nebte28	;
nebte16:	ldx	$f2	; DI 是否为 0
	beq	nebte20	;
	lda	#$ff	;
	sta	D3da2,x	;
	lda	#>(D3fa2)	; $F1|$F0 指向 D3fa2
	sta	$f1	;
	lda	#<(D3fa2)	;
	sta	$f0	;
	ldx	#4	;
	lda	$f3	;
lpbte12:	lsr	 a	; SI 右移 4 位
	ror	$f2	;
	dex	;
	bne	lpbte12	;
	lda	#4	;
	bne	nebte24	;
nebte20:	lda	#>(D41a2)	; $F1|$F0 为 BP（指向 D41a2）
	sta	$f1	;
	lda	#<(D41a2)	;
	sta	$f0	;
	lda	$f3	; 高字节移到低字节（SI 右移 8 位）
	sta	$f2	;
	lda	 #0	;
	sta	$f3	;
	lda	#8	;
nebte24:	sta	$ff	;
	lda	D4e62,y	;
	sec	;
	sbc	$ff	; 剩余位数
nebte28:	tax	; CL 为 X
	tya	;
	clc	;
	adc	D5f68	; AL 为 Y + D5f68
	jsr	FillTable	;
	dey	;
	bmi	nebte32	;
	jmp	lpbte04	;
nebte32:	rts	;
;---------------------------------------;
;    建立长码表（用途细节待确认）。从 255 倒序处理 256 项。
BuildTableLarge:	;
	ldy	#0	;
lpbtl04:	dey	;
	tya	;
	asl	a	; 码值每项 2 字节
	tax	;
	bcs	nebtl04	;
	lda	D5022+1,x	; 取 SI（$F3|$F2）
	sta	$f3	;
	lda	D5022,x	;
	jmp	nebtl08	;
nebtl04:	lda	D5022+$101,x	;
	sta	$f3	;
	lda	D5022+$100,x	;
nebtl08:	sta	$f2	;
	ldx	D4f22,y	;
	cpx	#9	; 位数是否大于 8
	bcs	nebtl12	;
	lda	#>(D43a2)	; $F1|$F0 指向 D43a2
	sta	$f1	;
	lda	#<(D43a2)	;
	sta	$f0	;
	jmp	nebtl28	;
nebtl12:	ldx	$f2	; SI 低字节为 0 时转 D49a2
	beq	nebtl16	;
	lda	#$ff	;
	sta	D43a2,x	;
	lda	#$3f	;
	bit	$f2	; 按 $F2 的低 6 位选表
	beq	nebtl20	;
	lda	#>(D45a2)	; 选择 D45a2，移位 4 次
	pha	;
	lda	#<(D45a2)	;
	ldx	#4	;
	bne	nebtl24	;
nebtl16:	lda	#>(D49a2)	; 选择 D49a2，移位 8 次
	pha	;
	lda	#<(D49a2)	;
	ldx	#8	;
	bne	nebtl24	;
nebtl20:	lda	#>(D47a2)	; 选择 D47a2，移位 6 次
	pha	;
	lda	#<(D47a2)	;
	ldx	#6	;
nebtl24:	sta	$f0	;
	pla	;
	sta	$f1	;
	stx	$ff	; 移位次数
lpser08:	lsr	$f3	; SI 右移 $FF 位
	ror	$f2	;
	dex	;
	bne	lpser08	;
	lda	D4f22,y	;
	sec	;
	sbc	$ff	;
	tax	; 剩余位数（CL）
nebtl28:	jsr	FillTableA	;
	tya	;
	bne	lpbtl04	;
	rts	;
;---------------------------------------;
;	入口：$F1|$F0 为 BP，$F3:$F2 为 SI（$F3 为 0），A 为 AL，X 为 CL。
FillTableA:	;
	tya	;
FillTable:	;
	sty	$ff	; 保存 Y
	sta	$fe	;
	stx	$fd	;
	cpx	#8	;
	bcs	nefte04	;
	lda	RoteLeft,x	;
	tay	;
	lda	#0	;
	beq	nefte08	;
nefte04:	lda	RoteLeft-8,x	;
	ldy	#0	;
nefte08:	sta	$f5	; A|Y 为步长（$F5|$F4）
	sty	$f4	;
	ldy	$f2	;
lpfte04:	lda	$fe	; 写入 AL
	sta	($f0),y	;
	inc	$f1	;
	txa	;
	sta	($f0),y	; 写入 CL
	dec	$f1	;
	tya	;
	clc	;
	adc	$f4	; 前进一步
	tay	;
	lda	$f5	;
	adc	#0	;
	beq	lpfte04	;
	ldy	$ff	;
	rts	;
;---------------------------------------;
;    位掩码表（右移用，与 RoteLeft 顺序相反）。
RoteRight:	.byte $80,$40,$20,$10,$08,$04,$02,$01
;---------------------------------------;
;    统计码长（StructureData）。
;    入口：$F3|$F2 为源地址，$F1|$F0 为目标地址，Y 为数据长度。
;---------------------------------------;
;    按码长统计并生成规范 Huffman 编码。入口：$F2|$F3 为码长表，Y 为码长项数。
StructureData:	;
	sty	$fe	;
	ldy	#16	;
	lda	#0	;
lpsda04:	sta	TempBuffer-1,y	; 清零 16 个码长计数
	dey	;
	bne	lpsda04	;
	ldy	#0	;
lpsda08:	lda	($f2),y	; 统计各码长的个数
	tax	;
	inc	TempBuffer-1,x	;
	iny	;
	cpy	$fe	;
	bne	lpsda08	;
	lda	#0	;
	tax	;
lpsda12:	clc	;
	adc	TempBuffer,x	; 累加得到各码长的起始序号
	sta	TempBuffer,x	;
	inx	;
	cpx	#$10	;
	bne	lpsda12	;
	ldy	$fe	;
lpsda16:	dey	;
	lda	($f2),y	;
	tax	;
	dec	TempBuffer-1,x	;
	lda	TempBuffer-1,x	;
	tax	;
	lda	($f2),y	;
	sta	TempBuffer+$10,x	; 按码长排序后的符号表
	tya	;
	bne	lpsda16	;
	lda	#0	;
	sta	$ff	; 上一个码长（CH）
	sta	$f4	; 编码累加值（AX）：$f5|$f4
	sta	$f5	;
	sta	$f6	;
	sta	$f7	;
	ldy	$fe	;
lpsda20:	dey	;
	clc	;
	lda	$f4	;
	adc	$f6	;
	sta	$f4	;
	lda	$f5	;
	adc	$f7	;
	sta	$f5	;
	lda	TempBuffer+$10,y	;
	cmp	$ff	;
	beq	nesda12	;
	sta	$ff	;
	tax	;
	dex	; 码长减一（0-15）
	cpx	#8	;
	bcs	nesda04	;
	lda	RoteRight,x	;
	ldx	#0	;
	beq	nesda08	;
nesda04:	lda	RoteRight-8,x	;
	tax	;
	lda	#0	;
nesda08:	sta	$f7	; 步长（A|X 为 DX）
	stx	$f6	;
nesda12:	tya	;
	asl	a	;
	tax	;
	lda	$f4	;
	bcs	nesda16	;
	sta	TempBuffer+$110,x	; 保存编码（低 8 位码表）
	lda	$f5	;
	sta	TempBuffer+$111,x	;
	jmp	nesda20	;
nesda16:	sta	TempBuffer+$210,x	; 保存编码（高 8 位码表）
	lda	$f5	;
	sta	TempBuffer+$211,x	;
nesda20:	tya	;
	bne	lpsda20	;
	lda	$f0	; 目标地址（$F4|$F5）
	sta	$f4	;
	ldy	#0	;
lpsda24:	lda	($f2),y	;
	tax	;
	tya	;
	pha	; 保存 Y
	asl	a	;
	tay	;
	lda	$f1	;
	adc	#0	;
	sta	$f5	;
	lda	TempBuffer-1,x	;
	inc	TempBuffer-1,x	;
	asl	a	;
	tax	;
	bcs	nesda24	;
	lda	TempBuffer+$110,x	;
	pha	; 保存低字节
	lda	TempBuffer+$111,x	;
	jmp	nesda28	;
nesda24:	lda	TempBuffer+$210,x	;
	pha	;
	lda	TempBuffer+$211,x	;
nesda28:	ldx	#8	;
lpsda28:	asl	a	; 按位反转后写入
	ror	$ff	;
	dex	;
	bne	lpsda28	;
	lda	$ff	;
	sta	($f4),y	;
	iny	;
	pla	; 取回低字节
	ldx	#8	;
lpsda32:	asl	a	;
	ror	$ff	;
	dex	;
	bne	lpsda32	;
	lda	$ff	;
	sta	($f4),y	;
	pla	;
	tay	;
	iny	;
	cpy	$fe	;
	bne	lpsda24	;
	rts	;
;---------------------------------------;
;    生成 CRC-32 查表（多项式 $EDB88320），4 段，每段 $100 字节。
BuildHSTable:	;
	lda	#<(HasiTable)	; 表起始地址
	sta	$f0	;
	ldy	#0	;
lpbhs04:	dey	; 从 255 到 0
	lda	#0	;
	sta	$f2	; 4 字节 CRC 值：$F2 至 $F5
	sta	$f3	;
	sta	$f4	;
	sty	$f5	;
	ldx	#8	;
lpbhs08:	lsr	$f2	; 逐位计算
	ror	$f3	;
	ror	$f4	;
	ror	$f5	;
	bcc	nebhs04	;
	lda	$f2	; 移出位为 1：异或多项式
	eor	#$ed	;
	sta	$f2	;
	lda	$f3	;
	eor	#$b8	;
	sta	$f3	;
	lda	$f4	;
	eor	#$83	;
	sta	$f4	;
	lda	$f5	;
	eor	#$20	;
	sta	$f5	;
nebhs04:	dex	;
	bne	lpbhs08	;
	lda	#>(HasiTable)	; 4 字节分别写入 4 段表
	sta	$f1	;
	ldx	#3	;
lpbhs12:	lda	$f2,x	;
	sta	($f0),y	;
	inc	$f1	;
	dex	;
	bpl	lpbhs12	;
	tya	;
	bne	lpbhs04	;
	rts	;
;---------------------------------------;
;    保存解压结果到目标页。入口：X|Y 为保存长度。
SaveFileBlock:	;
	sty	$fe	;
	txa	;
	ldx	TargetPage	;
	jsr	SetPageROM	; 映射目标页
	ldx	#>(TargetBuffer)	;
	ldy	#<(TargetBuffer)	;
	stx	$fc+1	; 源为目标缓冲区
	sty	$fc	;
	ldy	#0	;
	tax	;
	beq	nesfb12	;
lpsfb04:	lda	($fc),y	; 复制整页
	sta	(TargetAddress),y	;
	iny	;
	bne	lpsfb04	;
	inc	$fc+1	;
	inc	TargetAddress+1	;
	dex	;
	bne	lpsfb04	;
nesfb12:	ldx	$fe	;
	beq	nesfb16	;
lpsfb08:	lda	($fc),y	; 复制剩余部分
	sta	(TargetAddress),y	;
	iny	;
	dex	;
	bne	lpsfb08	;
	sty	TargetAddress	; 更新目标低字节
nesfb16:	lda	TargetAddress+1	;
	cmp	#>($c000)	;
	bcc	nesfb20	;
	lda	#>($8000)	; 超过 $C000：回到 $8000 并换到下一页
	sta	TargetAddress+1	;
	inc	TargetPage	;
nesfb20:	rts	;
;---------------------------------------; 回溯复制（MoveBackTarget）。
MoveBackTarget:	;
	lda	#<(TargetBuffer)	;
	sec	;
	sbc	$f8	; $F9|$F8 为相对目标缓冲区的回溯距离，换算为源地址偏移
	sta	$f8	;
	lda	#>(TargetBuffer)	;
	sbc	$f9	;
	sta	$f9	;
	ldx	TargetPage	;
	lda	TargetAddress	;
	sec	;
	sbc	$f8	;
	sta	$eb	; 源地址低字节
	lda	TargetAddress+1	;
	sbc	$f9	;
	bcs	nembt04	; 是否回到上一页
lpmbt04:	cpx	BiosTemp2	; 是否超出目标缓冲区之前
	beq	nembt20	; 是：填零
	clc	;
	adc	#>($c000-$8000)	;
	dex	;
nembt04:	cmp	#>($8000)	;
	bcc	lpmbt04	;
	sta	$eb+1	; 源地址高字节
	stx	$eb+2	; 源页（$EB|$EC|$ED 为源地址）
lpmbt08:	lda	AL	; AX 为搬移长度
	sec	;
	sbc	$f8	;
	tay	;
	lda	AH	;
	sbc	$f9	;
	tax	; AX - ($F9|$F8) 为 X|Y
	bcs	nembt12	; 长度跨越页或缓冲区边界
	lda	AL	; 否：本次搬移 $F9|$F8 个字节
	sta	$f8	;
	lda	AH	;
	sta	$f9	;
	ldx	#0	;
	ldy	#0	;
nembt12:	stx	AH	; AX 为剩余待搬移长度
	sty	AL	;
lpmbt12:	ldx	$eb+2	; 映射源页
	jsr	SetPageROM	;
	lda	#$ff	;
	ldx	$eb+1	;
	cpx	#>($c000-0100)	; 是否接近 $C000 边界
	bcc	nembt14	;
	ldx	$eb	;
	beq	nembt14	;
	lda	#<($c000)	; 页内剩余长度小于搬移长度：只搬到 $C000
	sbc	$eb	;
nembt14:	sta	$fb	;
	tax	;
	lda	$f8	;
	sec	;
	sbc	$fb	; 搬移长度是否足够
	bcs	nembt16	;
	dec	$f9	;
	bpl	nembt16	;
	ldx	$f8	; 否：用剩余长度
	lda	#0	;
	sta	$f9	;
nembt16:	sta	$f8	;
	ldy	#0	;
lpmbt16:	lda	($eb),y	; 逐字节复制
	sta	(DI),y	;
	iny	;
	dex	;
	bne	lpmbt16	;
	ldx	#$eb	;
	tya	;
	jsr	PointAdjustUnzip	; 源指针前进
	ldx	#DI	;
	tya	;
	jsr	PointAdjustUnzip	; 目标指针前进
	lda	$f8	;
	ora	$f9	;
	beq	nembt18	; 无剩余：结束
	lda	$eb+1	;
	cmp	#>($c000)	;
	bcc	lpmbt12	;
	inc	$eb+2	; 换到下一页
	lda	#>($8000)	; 源地址回到 $8000
	sta	$eb+1	;
	bne	lpmbt12	;
nembt18:	rts	;
;---------------------------------------;
;    填零：回溯位置在目标缓冲区之前时，用 0 填充目标。
nembt20:	stx	$eb+2	;
	sta	$eb+1	;
	lda	#<($8000)	; 可填充的长度
	sec	;
	sbc	$eb	;
	tax	;
	lda	#>($8000)	;
	sbc	$eb+1	; 可填充长度是否不小于剩余长度
	tay	;
	txa	;
	cmp	AL	;
	tya	;
	sbc	AH	;
	bcc	nembt24	;
	ldx	AL	; 是：用剩余长度
	ldy	AH	;
nembt24:	stx	$eb	; 本次填充长度
	sty	$eb+1	;
	lda	AL	;
	sec	;
	sbc	$eb	;
	sta	AL	; AX 为剩余待搬移长度
	lda	AH	;
	sbc	$eb+1	;
	sta	AH	;
	txa	;
	bne	nembt28	;
	dey	; 长度低字节为 0：高字节减一
nembt28:	sty	$eb	; 长度高字节
	lda	#0	;
	tay	;
lpmbt20:	sta	(DI),y	;
	iny	;
	bne	nembt32	;
	inc	DI+1	;
nembt32:	dex	; 长度低字节
	bne	lpmbt20	;
	dec	$eb	;
	bpl	lpmbt20	;
	tya	;
	clc	;
	adc	DI	; 目标指针前进
	sta	DI	;
	bcc	nembt36	;
	inc	DI+1	;
nembt36:	lda	AL	; 是否还有剩余
	ora	AH	;
	beq	nembt18	;
	sty	$eb	;
	lda	$f8	; 是：修正回溯长度
	sec	;
	sbc	$eb	;
	sta	$f8	;
	bcs	nembt40	;
	dec	$f9	;
nembt40:	lda	#>($8000)	; 源地址回到 $8000
	sta	$eb+1	;
	lda	#<($8000)	;
	sta	$eb	;
	jmp	lpmbt08	;
;---------------------------------------;
;    解压主循环：每次读取一个标志位，选择输出字面字节或回溯复制。
RestoreFile:	;
	lda	#0	;
	sta	ReadAll	;
	ldy	#$ff	;
	jsr	ReadSourceBlock	; 读入一段源数据
	ldx	#>(TargetBuffer)	; DI 指向目标缓冲区
	ldy	#<(TargetBuffer)	;
	stx	DI+1	;
	sty	DI	;
	ldx	#0	;
	stx	CH	;
	lda	SourceBuffer,x	;
	sta	DL	; 当前标志字节
	stx	SI	;
	inc	SI	;
	dec	BP	;
	jmp	lprfe04	;
nerfe04:	ldx	#>(D47a2)	; 选择 D47a2 码表（6 位）
	ldy	#<(D47a2)	;
	lda	#6	;
nerfe08:	sta	CL	;
	stx	AH	;
	sty	AL	;
	jsr	RoteSourceByte	; 取 CL 位
	inc	AH	;
	lda	(AX),y	;
	sta	CL	;
	dec	AH	;
	lda	(AX),y	;
	sta	AL	;
	jmp	nerfe24	;
nerfe12:	ldx	#>(D49a2)	; 选择 D49a2 码表（8 位）
	ldy	#<(D49a2)	;
	lda	#8	;
	bne	nerfe08	;
nerfe16:	lda	#$3f	; DL 低 6 位为 0 时选 D47a2
	bit	DL	;
	beq	nerfe04	;
	ldx	#>(D45a2)	; 否则选 D45a2（4 位）
	ldy	#<(D45a2)	;
	lda	#4	;
	bne	nerfe08	;
nerfe20:	lda	DL	;
	beq	nerfe12	;
	lda	D43a2,y	; 取 D43a2 表项（Y 为 BX）
	cmp	#$ff	;
	beq	nerfe16	;
	sta	AL	;
	lda	D43a2+$100,y	;
	sta	CL	;
nerfe24:	jsr	RoteSourceByte	;
	lda	AL	;
	ldy	#0	;
	sta	(DI),y	; 输出一个字节
	inc	DI	;
	bne	nerfe28	;
	inc	DI+1	;
nerfe28:	jmp	nerfe72	;
nerfe32:	lda	ZipType	; 压缩方法非 0：查表
	bne	nerfe20	;
	lda	DL	; 存储方式：直接输出标志字节
	sta	AL	;
	lda	#8	;
	sta	CL	;
	bne	nerfe24	;
nerfe36:	ldx	#>(D41a2)	; 选择 D41a2 码表（8 位）
	ldy	#<(D41a2)	;
	lda	#8	;
nerfe40:	sta	CL	; 与 nerfe08 相同：按表项取 AL、CL
	stx	AH	;
	sty	AL	;
	jsr	RoteSourceByte	;
	inc	AH	;
	lda	(AX),y	;
	sta	CL	;
	dec	AH	;
	lda	(AX),y	;
	sta	AL	;
	jmp	nerfe48	;
nerfe42:	ldx	#>(D3fa2)	; 选择 D3fa2 码表（4 位）
	ldy	#<(D3fa2)	;
	lda	#4	;
	bne	nerfe40	;
lprfe04:	lda	#1	; 读取 1 位标志
	sta	CL	;
	jsr	RoteSourceByte	;
	bcs	nerfe32	; 标志为 1：字面量
	lda	DL	;
	and	D5f5a	;
	sta	AH	;
	lda	D5f5e	;
	sta	CL	;
	jsr	RoteSourceByte	; 读取位数（Y 为 BX）
	lda	D4ca2,y	;
	sta	CL	;
	lda	#0	;
	sta	BackDistance+1	;
	lda	D4ba2,y	;
	ldy	D5f5e	;
lprfe08:	asl	a	; 组合回溯距离（左移 D5f5e 位）
	rol	BackDistance+1	;
	dey	;
	bne	lprfe08	;
	ora	AH	;
	sta	BackDistance	;
	jsr	RoteSourceByte	; 读取（Y 为 BX）
	lda	DL	;
	beq	nerfe36	;
	lda	D3da2,y	;
	bmi	nerfe42	; 表项最高位为 1：选 D3fa2
	sta	AL	;
	lda	D3da2+$100,y	;
	sta	CL	;
nerfe48:	jsr	RoteSourceByte	;
	lda	#0	;
	sta	AH	;
	lda	AL	;
	cmp	D527b	;
	bne	nerfe56	;
	clc	;
	adc	DL	;
	sta	AL	;
	bcc	nerfe52	;
	inc	AH	;
nerfe52:	lda	#8	; 块复制：AX 为长度
	sta	CL	;
	jsr	RoteSourceByte	;
nerfe56:	lda	DI	;
	clc	;
	sbc	BackDistance	; 源地址 = 目标地址 - 回溯距离
	sta	$f8	; $F9|$F8 为源地址
	lda	DI+1	;
	sbc	BackDistance+1	;
	sta	$f9	;
	bcc	nerfe60	;
	cmp	#>(TargetBuffer)	; 源地址是否在目标缓冲区之前
	bcs	lprfe12	; 否：直接复制
nerfe60:	jsr	MoveBackTarget	; 是：从已保存的页中复制
	ldx	#>(TargetBuffer)	;
	ldy	#<(TargetBuffer)	;
	stx	$f8+1	; 源指针回到目标缓冲区
	sty	$f8	;
lprfe12:	lda	AL	;
	ora	AH	;
	beq	nerfe72	; 剩余为 0：结束
	ldy	#$ff	;
	lda	AL	;
	sec	;
	sbc	#$ff	;
	bcs	nerfe64	;
	dec	AH	;
	bpl	nerfe64	;
	ldy	AL	; 最后一段：长度低字节
	lda	#0	;
	sta	AH	;
nerfe64:	sta	AL	;
	sty	$ff	; 本次复制长度
	ldy	#0	;
lprfe16:	lda	($f8),y	; 逐字节复制
	sta	(DI),y	;
	iny	;
	cpy	$ff	;
	bne	lprfe16	;
	ldx	#$f8	;
	tya	;
	jsr	PointAdjustUnzip	; 源指针前进
	ldx	#DI	;
	tya	;
	jsr	PointAdjustUnzip	; 目标指针前进
	jmp	lprfe12	;
nerfe72:	lda	DI+1	;
	cmp	#>(TargetBuffer+TargetBfLen)	; 目标缓冲区未满：继续
	bcs	nerfe80	;
	jmp	lprfe04	;
;    目标缓冲区满：写出一段，再把剩余数据移到缓冲区起始。
nerfe80:	ldx	#>(TargetBfLen)	;
	ldy	#<(TargetBfLen)	;
	jsr	SaveFileBlock	; 写出一段（$0800 字节）
	lda	DI	;
	sta	$ff	; 剩余数据长度低字节
	ldy	#0	;
	lda	DI+1	;
	sec	;
	sbc	#>(TargetBfLen)	;
	sta	DI+1	; 目标指针回到缓冲区起始
	cmp	#>(TargetBuffer)	;
	beq	lprfe28	; 剩余数据在第一页内：单页搬移
lprfe20:	lda	TargetBuffer+TargetBfLen,y	; 整页搬移（第二页到第一页）
	sta	TargetBuffer,y	;
	iny	;
	bne	lprfe20	;
lprfe24:	cpy	$ff	; 逐页搬移直到剩余长度
	beq	nerfe84	;
	lda	TargetBuffer+TargetBfLen+$100,y	;
	sta	TargetBuffer+$100,y	;
	iny	;
	jmp	lprfe24	;
lprfe28:	cpy	$ff	;
	beq	nerfe84	;
	lda	TargetBuffer+TargetBfLen,y	;
	sta	TargetBuffer,y	;
	iny	;
	bne	lprfe28	;
nerfe84:	jmp	lprfe04	;
nerfe88:	pla	; 结束：丢弃栈上的返回地址
	pla	;
	pla	;
	pla	;
	lda	DI	; 出口：X|Y 为已解压的长度
	sec	;
	sbc	#<(TargetBuffer)	;
	tay	;
	lda	DI+1	;
	sbc	#>(TargetBuffer)	;
	tax	;
	rts	;
;---------------------------------------;
;    读取下一块源数据；没有数据时置 ReadAll 标志。
ReadNextBlock:	;
	lda	ReadAll	;
	bne	nerfe88	; 已读完：结束
	ldy	#$ff	;
	jsr	ReadSourceBlock	;
	lda	#0	; 源指针复位
	sta	SI	;
	dec	BP	;
	bne	nernb04	;
	inc	BP	;
	inc	ReadAll	; 没有数据：置读完标志
nernb04:	rts	;
;---------------------------------------;
;    从位流取 CL 位。入口：CH 为当前字节剩余位数（$F5），CL 为需要的位数（$F4）。
;    出口：Y 为取得的位（BX）。
RoteSourceByte:	;
	lda	CH	;
	cmp	CL	;
	bcs	nesc412	; 当前字节位数足够
	tax	;
	beq	nesc404	;
lpsc404:	lsr	DH	; 移出当前字节已用的位
	ror	DL	;
	dex	;
	bne	lpsc404	;
nesc404:	dec	BP	;
	bne	nesc408	;
	jsr	ReadNextBlock	;
nesc408:	ldx	SI	; 取下一个源字节
	lda	SourceBuffer,x	;
	inc	SI	;
	sta	DH	;
	lda	CL	;
	sec	;
	sbc	CH	;
	sta	CL	; 还需取的位数
	lda	#8	;
	sec	;
nesc412:	sbc	CL	;
	sta	CH	; 更新当前字节剩余位数
	ldx	CL	;
lpsc408:	lsr	DH	; 把需要的位移入 DL
	ror	DL	;
	dex	;
	bne	lpsc408	;
	ldy	DL	;
	rts	;
;=======================================;
;    解压入口：读取文件头（压缩参数、CRC、长度），按参数建立码表，然后解压并保存。
ExplodeFile:	;
	lda	#$ff	;
	sta	SourceLength	; 源长度先置 $FF
	ldy	#10	; 读取 10 字节文件头
	jsr	ReadSourceBlock	;
	lda	SourceBuffer	;
	sta	ZipFlag	; 压缩参数
	ldx	#7	; 取 CRC 与源长度（8 字节）
lpefe04:	lda	SourceBuffer+2,x	;
	sta	CRCSave,x	;
	dex	;
	bpl	lpefe04	;
	ldx	#0	;
	lda	#$04	;
	bit	ZipFlag	; 位 2 为 1：有码表
	beq	neefe04	;
	ldx	#>(D4f22)	;
	ldy	#<(D4f22)	;
	stx	$f3	;
	sty	$f2	;
	ldy	#0	;
	jsr	ReadTable	; 读取码表 D4f22
	ldx	#>(D4f22)	;
	ldy	#<(D4f22)	;
	stx	$f2+1	;
	sty	$f2	;
	ldx	#>(D5022)	;
	ldy	#<(D5022)	;
	stx	$f0+1	;
	sty	$f0	;
	ldy	#$00	;
	jsr	StructureData	; 由码表生成 D5022
	jsr	BuildTableLarge	;
	ldx	#1	;
neefe04:	stx	ZipType	; 压缩方法
	lda	#$02	;
	bit	ZipFlag	; 位 1 为 1：参数组二
	bne	neefe08	;
	lda	#2	; 参数组一：D5f68=2，D5f5a=$3F，D5f5e=6
	sta	D5f68	;
	lda	#$3f	;
	sta	D5f5a	;
	lda	#6	;
	sta	D5f5e	;
	bne	neefe12	;
neefe08:	lda	#3	; 参数组二：D5f68=3，D5f5a=$7F，D5f5e=7
	sta	D5f68	;
	lda	#$7f	;
	sta	D5f5a	;
	lda	#7	;
	sta	D5f5e	;
neefe12:	lda	D5f68	;
	clc	;
	adc	#$3f	;
	sta	D527b	; 长度阈值（块复制判断）
	ldx	#>(D4e62)	;
	ldy	#<(D4e62)	;
	stx	$f2+1	;
	sty	$f2	;
	ldy	#$40	;
	jsr	ReadTable	; 读取 64 项表 D4e62
	ldx	#>(D4da2)	;
	ldy	#<(D4da2)	;
	stx	$f2+1	;
	sty	$f2	;
	ldy	#$40	;
	jsr	ReadTable	; 读取 64 项表 D4da2
	ldx	#>(D4e62)	;
	ldy	#<(D4e62)	;
	stx	$f2+1	;
	sty	$f2	;
	ldx	#>(D4ea2)	;
	ldy	#<(D4ea2)	;
	stx	$f0+1	;
	sty	$f0	;
	ldy	#$40	;
	jsr	StructureData	; 生成 D4ea2
	ldx	#>(D4da2)	;
	ldy	#<(D4da2)	;
	stx	$f2+1	;
	sty	$f2	;
	ldx	#>(D4de2)	;
	ldy	#<(D4de2)	;
	stx	$f0+1	;
	sty	$f0	;
	ldy	#$40	;
	jsr	StructureData	; 生成 D4de2
;---------------------------------------;
	jsr	BuildTable	;
	jsr	RestoreFile	; 解压
	jmp	SaveFileBlock	; 保存结果
;---------------------------------------;
;    键码表：每项对应键盘矩阵的一个位（行:位），注释为按键名称。
KeyCodeTable:	.byte $05	; 第 0 行 $01：4
	.byte $22	; 第 0 行 $02：G
	.byte $21	; 第 0 行 $04：F
	.byte $2e	; 第 0 行 $08：C
	.byte $3c	; 第 0 行 $10：F2
	.byte $12	; 第 0 行 $20：E
	.byte $06	; 第 0 行 $40：5
	.byte $2f	; 第 0 行 $80：V
	.byte $03	; 第 1 行 $01：2
	.byte $20	; 第 1 行 $02：D
	.byte $1f	; 第 1 行 $04：S
	.byte $4f+$80	; 第 1 行 $08：Alt/End
	.byte $3b	; 第 1 行 $10：F1
	.byte $11	; 第 1 行 $20：W
	.byte $04	; 第 1 行 $40：3
	.byte $2d	; 第 1 行 $80：X
	.byte $52+$80	; 第 2 行 $01：Ins
	.byte $0e	; 第 2 行 $02：退格
	.byte $51+$80	; 第 2 行 $04：PgDn
	.byte $4d+$80	; 第 2 行 $08：右
	.byte $42	; 第 2 行 $10：F8
	.byte $49+$80	; 第 2 行 $20：PgUp
	.byte $53+$80	; 第 2 行 $40：Del
	.byte $47+$80	; 第 2 行 $80：Home
	.byte $0a	; 第 3 行 $01：9
	.byte $17	; 第 3 行 $02：I
	.byte $26	; 第 3 行 $04：L
	.byte $33	; 第 3 行 $08：,
	.byte $3f	; 第 3 行 $10：F5
	.byte $18	; 第 3 行 $20：O
	.byte $0b	; 第 3 行 $40：0
	.byte $34	; 第 3 行 $80：.
	.byte $1b	; 第 4 行 $01：]
	.byte $1c	; 第 4 行 $02：回车
	.byte $48+$80	; 第 4 行 $04：上
	.byte $4b+$80	; 第 4 行 $08：左
	.byte $41	; 第 4 行 $10：F7
	.byte $1a	; 第 4 行 $20：[
	.byte $2b	; 第 4 行 $40：\
	.byte $50+$80	; 第 4 行 $80：下
	.byte $10	; 第 5 行 $01：Q
	.byte $3a	; 第 5 行 $02：大写锁定
	.byte $2c	; 第 5 行 $04：Z
	.byte $55	; 第 5 行 $08：Break/Pause
	.byte $01	; 第 5 行 $10：ESC
	.byte $1e	; 第 5 行 $20：A
	.byte $02	; 第 5 行 $40：1
	.byte $1d	; 第 5 行 $80：Ctrl
	.byte $08	; 第 6 行 $01：7
	.byte $15	; 第 6 行 $02：Y
	.byte $25	; 第 6 行 $04：K
	.byte $32	; 第 6 行 $08：M
	.byte $3e	; 第 6 行 $10：F4
	.byte $16	; 第 6 行 $20：U
	.byte $09	; 第 6 行 $40：8
	.byte $24	; 第 6 行 $80：J
	.byte $0c	; 第 7 行 $01：-
	.byte $27	; 第 7 行 $02：;
	.byte $28	; 第 7 行 $04：'
	.byte $35	; 第 7 行 $08：/
	.byte $40	; 第 7 行 $10：F6
	.byte $19	; 第 7 行 $20：P
	.byte $0d	; 第 7 行 $40：+
	.byte $2a	; 第 7 行 $80：Shift
	.byte $14	; 第 8 行 $01：T
	.byte $23	; 第 8 行 $02：H
	.byte $31	; 第 8 行 $04：N
	.byte $39	; 第 8 行 $08：空格/Tab
	.byte $3d	; 第 8 行 $10：F3
	.byte $13	; 第 8 行 $20：R
	.byte $07	; 第 8 行 $40：6
	.byte $30	; 第 8 行 $80：B
;---------------------------------------; SB486LC 表的最后一项；之后为 SB486D 表。
	.byte $ff	; 第 9 行 $01
	.byte $4b	; 第 9 行 $02：左
	.byte $47	; 第 9 行 $04：Home
	.byte $57	; 第 9 行 $08：F11
	.byte $58	; 第 9 行 $10：F12
	.byte $4f	; 第 9 行 $20：End
	.byte $50	; 第 9 行 $40：下
	.byte $48	; 第 9 行 $80：上
	.byte $4a	; 第 10 行 $01：小键盘 -
	.byte $4e	; 第 10 行 $02：小键盘 +
	.byte $37	; 第 10 行 $04：小键盘 *
	.byte $49	; 第 10 行 $08：PgUp
	.byte $44	; 第 10 行 $10：F10
	.byte $4c	; 第 10 行 $20：小键盘 5
	.byte $35+$80	; 第 10 行 $40：/
	.byte $45	; 第 10 行 $80：NumLock
	.byte $29	; 第 11 行 $01：`~
	.byte $4d	; 第 11 行 $02：右
	.byte $38	; 第 11 行 $04：Alt
	.byte $0f	; 第 11 行 $08：Tab
	.byte $43	; 第 11 行 $10：F9
	.byte $51	; 第 11 行 $20：PgDn
	.byte $53	; 第 11 行 $40：Del
	.byte $52	; 第 11 行 $80：Ins
;---------------------------------------;
;    读取按键扫描码。
;    出口：C=1 无按键（Z=0 无扫描码，Z=1 无输出）；
;          C=0 有变化：Y 为键码，X 为键码字节序号，A 为键码位。
CheckScanCode:	;
	lda	#5	; 选择键盘矩阵
	sta	$4016	;
	ldx	#0	;
lpcsc04:	lda	#4	;
	sta	$4016	;
	pha	; 延时
	pla	;
	lda	$4017	; 读取 D4-D1
	and	#$1e	;
	ldy	#6	;
	sty	$4016	;
	lsr	;
	sta	CurrentKeyMap,x	; 保存本行的低半字节
	lda	$4017	; 读取 D4-D1（另一半）
	and	#$1e	;
	asl	;
	asl	;
	asl	;
	ora	CurrentKeyMap,x	; 合并两次读取
	eor	#$ff	; 取反（低电平有效）
	sta	CurrentKeyMap,x	;
	inx	;
	cpx	#9	; 共 9 行
	bcc	lpcsc04	;
	bne	necsc04	;
	lda	#4	; 扫描结束：复位矩阵选择
	sta	$4016	;
	lda	#6	;
	sta	$4016	;
necsc04:	cpx	#KeyboardDataLen	;
	bne	lpcsc04	;
	ldx	#0	; 与上次扫描结果比较
lpcsc08:	lda	CurrentKeyMap,x	;
	cmp	LastKeyMap,x	;
	bne	necsc12	;
	inx	;
	cpx	#KeyboardDataLen	;
	bne	lpcsc08	;
	txa	; 无按键变化：A 为行数，C=1
	sec	;
	rts	;
necsc12:	eor	LastKeyMap,x	; 变化的位（不为 0）
	sta	TempSave1	;
	txa	;
	asl	;
	asl	;
	asl	;
	tay	; 该行的键码基址（每行 8 位）
	lda	#0	;
	sec	;
lpcsc16:	iny	; 找出变化的位
	rol	;
	bit	TempSave1	;
	beq	lpcsc16	;
	dey	;
	rts	; C=0
;---------------------------------------;
;    取按键输入（经两次扫描确认）。
;    出口：C=0 有键输出，X/A 为键码；
;          C=1：Z=0 无扫描码，Z=1 无输出。
;---------------------------------------;
CheckScanCodeInp:	;
	jsr	CheckScanCode	; 扫描码有变化？
	bcs	necsi12	;
	sty	TempSave2	; 是：再扫描一次确认
	jsr	CheckScanCode	;
	cpy	TempSave2	;
	bne	necsi20	; 两次结果不同：无扫描码
	tay	;
	eor	LastKeyMap,x	; 更新 LastKeyMap
	sta	LastKeyMap,x	;
	tya	;
	and	LastKeyMap,x	; 按下还是释放？
	tax	;
	ldy	#0	;
	sty	AutoRepeatDelay	; 清除自动重复延时
	ldy	TempSave2	;
	lda	KeyCodeTable,y	; 取键码
	bpl	necsi04	; 最高位为 1：扩展键（AT 101 键盘）
	tay	;
	lda	KeyStatus3	; 扩展键前导 $E0
	ora	#$12	;
	sta	KeyStatus3	;
	tya	;
	and	#$7f	;
necsi04:	dex	; 释放键？
	bpl	necsi08	;
	ora	#$80	; 是：释放标记
necsi08:	sta	TempSave2	;
	jsr	GetKeyCode	; 转换为键码
	bcc	necsi12	; 有输出
	ldy	#0	; C=1：无输出
necsi12:	rts	;
necsi20:	sec	; 无扫描码
	rts	;
;---------------------------------------;
;    检查 PrintScreen：小键盘 *（$37）且按住 Shift 时返回 C=0。
CheckPrintScreen:	;
	cpx	#$37	; 小键盘 *？
	bne	necps04	;
	lda	#3	; 是否按住 Shift
	bit	KeyStatus1	;
	beq	necps04	;
	clc	;
	rts	;
necps04:	sec	;
	rts	;

;=======================================;
;    打印输出（X 为 8）。入口：Y 为要输出的数据。
PrintDataDev:	;
	sty	$ff40	; 写入数据端口
	lda	#23	; 超时计数（约 10 秒）
	ldx	#0	;
lppda04:	bit	$ff48	; 检查打印机状态
	bmi	nepda04	;
	dey	;
	bne	lppda04	;
	dex	;
	bne	lppda04	;
	sec	;
	sbc	#1	;
	bne	lppda04	;
	sec	; 超时错误
	bcs	nepss04	;
nepda04:	lda	#$0d	; 选通信号 $0D
	sta	$ff50	;
	pha	; 延时
	pla	;
	bne	neipt04	;
;=======================================;
;    初始化打印机（X 为 9）。
InitialPrintDev:	;
	lda	#$08	; 复位打印机
	sta	$ff50	;
	ldy	#10	; 延时
lpipt04:	dex	;
	bne	lpipt04	;
	dey	;
	bne	lpipt04	;
neipt04:	lda	#$0c	; 结束复位
	sta	$ff50	;
	pha	; 延时
	pla	;
;=======================================;
;    取打印机状态（X 为 10）。
;    出口：A 为打印机状态，X 为端口状态；
;          X 位 7：打印端口存在；位 1：Autofeed 始终有效。
PrinterStatusDev:
	clc	;
nepss04:	lda	$ff48	; 读取打印机状态
	and	#$f8	;
	bcc	nepss08	;
	ora	#$01	; 超时（C=1）时位 0 置 1
nepss08:	eor	#$48	;
	ldx	#$80	;
	rts	;

;---------------------------------------;
;    鼠标移动：A 的位 7 为右/下，位 6 为左/上；X 为坐标偏移（$5F00 起），Y 为范围索引。
CheckMouse:	;
	asl	; 右/下
	bcc	neccr08	;
	pha	;
	lda	$5f00,x	;
	clc	;
	adc	MouseMovePlusC	;
	bcs	neccr02	;
	cmp	MaximumS,y	;
	bcc	neccr04	;
neccr02:	lda	MaximumS,y	; 超出最大值：限制到最大值
	sbc	#1	;
neccr04:	sta	$5f00,x	;
	pla	;
neccr08:	asl	; 左/上
	bcc	neccr20	;
	pha	;
	lda	$5f00,x	;
	sbc	MouseMovePlusC	;
	bcc	neccr12	;
	cmp	MinimumS,y	;
	bcs	neccr16	;
neccr12:	lda	MinimumS,y	; 低于最小值：限制到最小值
neccr16:	sta	$5f00,x	;
	pla	;
neccr20:	rts	;
;---------------------------------------;
;    检查两个手柄：先手柄 1，再手柄 2。
CheckJoystick:	;
	ldx	#0	;
	jsr	CheckJoystickOne	;
	ldx	#1	;
;---------------------------------------;
;    处理手柄 X 的按键：按下与释放的判断、按键计数，以及移动键的连发。
CheckJoystickOne:	;
	jsr	GetJoystickKey	; 读取手柄键码
	sta	TempSave1	; 当前键码
	eor	CurrentJoyKey,x	; 变化的键
	tay	;
	and	CurrentJoyKey,x	; 释放的键
	pha	;
	lda	TempSave1	;
	sta	CurrentJoyKey,x	;
;---------------------------------------;
	tya	;
	and	CurrentJoyKey,x	; 按下的键
	beq	necjo24	; 无按下的键
	stx	TempSave2	;
	cpx	#0	;
	bne	necjo06	;
	ldx	#0	; 统计次数
lpcjo04:	lsr	; 手柄 1 的 A、B、Sl、St 键
	bcc	necjo04	;
	inc	PressCount0,x	;
necjo04:	inx	;
	cpx	#4	;
	bne	lpcjo04	; 是否有移动键
	beq	necjo12	;
necjo06:	ldx	#0	;
lpcjo08:	lsr	; 手柄 2 的 A、B、Sl、St 键
	bcc	necjo08	;
	inc	PressCount1,x	;
necjo08:	inx	;
	cpx	#4	;
	bne	lpcjo08	; 是否有移动键
necjo12:	ldx	TempSave2	;
	asl	; 检查方向键（位 4-7）是否按下
	beq	necjo24	;
	asl	;
	asl	;
	asl	;
	tay	;
	eor	CurrentJoyKey,x	; 只有移动键按下？
	and	#$f0	;
	bne	necjo16	;
	lda	#6	;
	sta	MovePrKeyCount,x	; 是：设置 Press() 计数
necjo16:	tya	;
	and	LastRlMoveKey,x	; 是否紧跟在释放之后再按下
	beq	necjo20	;
	asl	MouseMovePlus,x	; 是：鼠标速率加倍
	bcc	necjo20	;
	ror	MouseMovePlus,x	;
necjo20:	lda	#0	;
	sta	LastRlMoveKey,x	;
	sta	MoveRlKeyCount,x	;
;---------------------------------------;
necjo24:	pla	; 取回释放的键
	and	#$f0	; 是否释放了移动键
	beq	necjo28	;
	sta	LastRlMoveKey,x	; 是：保存释放的移动键
	lda	#5	; 设置初始延时
	sta	MoveRlKeyCount,x	;
	bne	necjo36	;
necjo28:	lda	MoveRlKeyCount,x	;
	beq	necjo36	;
	dec	MoveRlKeyCount,x	; 延时结束？
	bne	necjo36	;
	lda	TempSave1	; 是
	and	#$f0	; 没有移动键按下？
	bne	necjo32	;
	lda	#1	;
	sta	MouseMovePlus,x	; 恢复速率
necjo32:	lda	#0	;
	sta	LastRlMoveKey,x	;
necjo36:	lda	MovePrKeyCount,x	; 是否处于 Press() 状态
	beq	necjo40	;
	ldy	MouseMovePlus,x	; 是：是否继续 Press()？
	dey	;
	bne	necjo40	;
	dec	MovePrKeyCount,x	; 否：结束 Press()
	cmp	#6	;
	bne	neskc12	;
necjo40:	lda	TempSave1	;
	and	#$f0	;
	beq	neskc12	; 无移动键：结束
	ldy	MouseMovePlus,x	;
	sty	MouseMovePlusC	;
	dex	;
	beq	necjo44	;
	ldx	#<(Mouse0X)	; 鼠标 0 左右
	ldy	#0	;
	jsr	CheckMouse	;
	ldx	#<(Mouse0Y)	; 鼠标 0 上下
	ldy	#2	;
	jmp	CheckMouse	;
necjo44:	ldx	#<(Mouse1X)	; 鼠标 1 左右
	ldy	#1	;
	jsr	CheckMouse	;
	ldx	#<(Mouse1Y)	; 鼠标 1 上下
	ldy	#3	;
	jmp	CheckMouse	;
;---------------------------------------;
;    把键码 X/A 保存到键盘缓冲区；缓冲区满时响铃。
SaveKeyCode:	;
	ldy	KBDTail	;
	sta	KBDBuffer,y	;
	iny	;
	txa	;
	sta	KBDBuffer,y	;
	iny	; 移动尾指针
	cpy	#KBDEnd	;
	bne	neskc04	;
	ldy	#KBDBegin	; 绕回缓冲区起始
neskc04:	cpy	KBDHead	; 缓冲区满？
	bne	neskc08	;
	lda	#1	; 响铃
	sta	$4015	;
	lda	#$8f	;
	sta	$4000	;
	lda	#0	;
	sta	$4001	;
	lda	#$50	;
	sta	$4002	;
	lda	#$80	;
	sta	$4003	;
	rts	;
neskc08:	sty	KBDTail	;
neskc12:	rts	;
;=======================================;
;    键盘中断服务（NMI）：读取键码，处理自动重复，最后检查手柄。
KeyBoardNMIROM:	;
	txa	; 保存 X、Y
	pha	;
	tya	;
	pha	;
	jsr	CheckScanCodeInp	; 是否有键码输出？
	bcs	nenip04	;
	sta	BiosKeyCode	; 是：保存键码
	stx	BiosKeyCode+1	;
	bit	TempSave2	; 最后是否为释放键（用于组合键重复）
	bmi	nenip08	;
	inc	AutoRepeatDelay	;
	bne	nenip08	;
;---------------------------------------; 自动重复
nenip04:	beq	nenip24	; 键码有变化？
	lda	AutoRepeatDelay	; 否：上次是否有键码输出？
	beq	nenip24	;
	cmp	DelayTimes	; 是：是否超过长按延时
	bcc	nenip12	;
	ldx	AutoRepeatCount	;
	inx	;
	cpx	RateTime	; 是否达到重复速率
	bcc	nenip20	;
	lda	BiosKeyCode	; 重复输出键码
	ldx	BiosKeyCode+1	;
nenip08:	jsr	SaveKeyCode	; 保存键码
	jmp	nenip16	;
nenip12:	inc	AutoRepeatDelay	;
nenip16:	ldx	#0	;
nenip20:	stx	AutoRepeatCount	;
nenip24:	jsr	CheckJoystick	; 检查手柄
	pla	;
	tay	;
	pla	;
	tax	;
	rts	;
;===============================================;
;    设备功能表（X 为功能号）：
;    功能 0：读取键盘。出口：X/A 为扩展/基础键码。
;    功能 1：测试键盘。出口：Y=0 无键；Y=1 时 X/A 为 E/B 键码。
;    功能 2：取键盘状态。出口：X/A 为状态 2/状态 1。
;    功能 3：设置击键速率与延时。Y 为速率与延时值。
;    功能 4：释放键盘（保留）。
;    功能 5：把键码放回键盘缓冲区。$FF/Y 为 E/B 键码。
;    功能 6：修改中断检查方式。Y 为 NMICheckFlag；出口：X|Y 为 KeyBoardNMI 地址。
;    功能 7：复位键盘。出口：A=0 复位成功。
;    功能 8：打印输出。Y 为字符。
;    功能 9：复位打印机。出口：A 为打印机状态，X 为端口状态。
;    功能 10：取打印机状态。出口：A 为打印机状态，X 为端口状态。
;    功能 11：取/设置 CtrlBreak 向量。Y|$FF|$FE 为新向量；出口：A|X|Y 为原向量。
;    功能 12：鼠标初始化。Y 位 0：1 为鼠标 1，0 为鼠标 0。出口：A=0 鼠标正常。
;    功能 13：读取鼠标。Y 位 0 选择鼠标。
;        出口：A 为当前按键，位 7 右、位 6 左、位 5 下、位 4 上、位 3 Start、
;              位 2 Select、位 1 B、位 0 A；X 为鼠标 X，Y 为鼠标 Y；
;              $FC 为 A 键按下次数（原注释此处又写作 D 键，待确认）；
;              $FE 为 Select 键按下次数；$FF 为 Start 键按下次数。
;    功能 14：设置鼠标。Y 位 0 选择鼠标；$FE/$FF 为鼠标 X/Y。
;    功能 15：设置鼠标范围。Y 位 0 选择鼠标；$FC/$FD 为左上角 X/Y；
;             $FE/$FF 为右下角 X/Y（原注释写作 X/T，疑为笔误，待确认）。
;-----------------------------------------------;
KBEBranch:	.word ReadKeyBoardDev-1,	TestKeyBoardROM-1
	.word ReadStatusDev-1,	SetTypeRateDelayDev-1
	.word ReleaseKeyBoardDev-1,	SaveBackKeyCodeDev-1
	.word SetINICheckModeDev-1,	ResetKeyBoardDev-1
	.word PrintDataDev-1,		InitialPrintDev-1
	.word PrinterStatusDev-1,	GetSetBreakVectorDev-1
	.word MouseResetDev-1,	GetMouseDev-1
	.word SetMouseDev-1,		SetMouseRangeDev-1
	.word SpeakInitDev-1,		SpeakWordDev-1
	.word SpeakNoSoundDev-1
FunctionLimit = *-KBEBranch	; 功能表字节数
;    设备入口：按 X 中的功能号跳转到 KBEBranch 中的例程，超出范围直接返回。
DeviceEntryROM:	;
	txa	;
	asl	; 功能号转为表偏移
	cmp	#FunctionLimit	;
	bcs	nekbe04	;
	tax	;
	lda	KBEBranch+1,x	;
	pha	;
	lda	KBEBranch,x	;
	pha	;
nekbe04:	rts	;
;=======================================; 功能 0：读取键盘。
ReadKeyBoardDev:	;
lprkd04:	ldy	KBDHead	; 键盘缓冲区是否有键？
	cpy	KBDTail	;
	bne	nerkd04	;
	bit	NMICheckFlag	;
	bvc	lprkd04	; 不在中断中检查：继续等待
	jsr	CheckScanCodeInp	;
	bcs	lprkd04	;
	jsr	SaveKeyCode	;
	jmp	lprkd04	;
nerkd04:	lda	KBDBuffer,y	; 有：取出键码
	iny	;
	ldx	KBDBuffer,y	;
	iny	;
	cpy	#KBDEnd	; 移动头指针
	bcc	nerdk08	;
	ldy	#KBDBegin	;
nerdk08:	sty	KBDHead	;
	rts	;
;---------------------------------------; 功能 1：测试键盘。
TestKeyBoardROM:	;
lptkr04:	ldy	KBDHead	; 缓冲区是否有键？
	cpy	KBDTail	;
	beq	netkr04	;
	lda	KBDBuffer,y	; 有：返回键码
	ldx	KBDBuffer+1,y	;
	ldy	#1	;
	rts	;
netkr04:	bit	NMICheckFlag	;
	bvc	netkr08	;
	jsr	CheckScanCodeInp	;
	bcs	netkr08	;
	jsr	SaveKeyCode	;
	jmp	lptkr04	;
netkr08:	ldy	#0	; 无键
	rts	;
;=======================================; 功能 2：取键盘状态。
ReadStatusDev:	;
	lda	KeyStatus1	; 取状态
	ldx	KeyStatus2	;
	ldy	KeyStatus3	;
	rts	;
;---------------------------------------; 功能 3：设置击键速率与延时。
RateTable:	.byte 2,  3,  4,  5,  8,  11, 16, 22	; 击键速率表
SetTypeRateDelayDev:	;
	tya	;
	and	#$60	; 位 6-5：延时档位
	asl	;
	rol	;
	rol	;
	rol	;
	tax	;
	lda	DelayTeble,x	;
	sta	DelayTimes	;
	tya	;
	and	#$1c	; 位 4-2：速率档位
	lsr	;
	lsr	;
	tax	;
	lda	RateTable,x	;
	sta	RateTime	;
	rts	;
;---------------------------------------; 功能 5：把键码放回键盘缓冲区（$FF/Y 为 E/B 键码）。
SaveBackKeyCodeDev:	;
	ldx	KBDHead	; 头指针减两字节
	dex	;
	dex	;
	bpl	nesbb04	;
	ldx	#KBDEnd-2	;
nesbb04:	lda	Addr2000	; 关闭 NMI
	pha	;
	and	#$7f	;
	sta	$2000	;
	sta	Addr2000	;
	cpx	KBDTail	; 缓冲区是否已满
	bne	nesbb12	;
	txa	; 是：忽略最后一个键
	sec	;
	sbc	#2	;
	bpl	nesbb08	;
	lda	#KBDEnd-2	;
nesbb08:	sta	KBDTail	;
nesbb12:	stx	KBDHead	;
	pla	;
	sta	Addr2000	;
	sta	$2000	; 恢复 NMI
	tya	;
	sta	KBDBuffer,x	;
	lda	$ff	;
	sta	KBDBuffer+1,x	;
	rts	;
;---------------------------------------; 功能 6：启用或禁用 NMI 键盘检查。出口：X|Y 为 KeyBoardNMI 地址。
SetINICheckModeDev:	;
	sty	NMICheckFlag	;
	ldx	#>(KeyBoardNMIDo-$C000+$5800)
	ldy	#<(KeyBoardNMIDo-$C000+$5800)
	rts	;
;---------------------------------------; 功能 7：复位键盘。出口：A=0 复位成功。
ResetKeyBoardDev:	;
	lda	#0	;
	sta	KeyStatus1	;
	sta	KeyStatus2	;
	sta	KeyStatus3	;
	sta	KeyStatus4	;
	ldx	#KBResetClearLen-1	;
lpkbd08:	sta	KBResetClearBeg,x	; 清除键盘状态区
	dex	;
	bpl	lpkbd08	;
	ldx	#26	; 默认延时
	stx	DelayTimes	;
	ldx	#3	; 默认速率
	stx	RateTime	;
	jsr	SendKBDLED	; 发送 LED 设置
	lda	#0	;
	rts	;
;---------------------------------------; 功能 11：取/设置 CtrlBreak 向量（此处仅返回）。
GetSetBreakVectorDev:	;
	rts	;
;---------------------------------------; 功能 12：鼠标初始化。Y 位 0 选择鼠标。出口：A=0 鼠标正常。
MouseResetDev:	;
	tya	;
	and	#1	; 鼠标编号（位 0）
	tay	;
	lda	#0	;
	sta	CurrentJoyKey,y	;
	sta	MinimumX,y	;
	sta	MinimumY,y	;
	lda	#246	;
	sta	MaximumX,y	; 坐标范围 246 × 239
	lda	#239	;
	sta	MaximumY,y	;
	lsr	; 纵坐标取中点（239 / 2）
	dey	;
	beq	nemrt04	;
	sta	Mouse0Y	; 鼠标 0 的初始位置
	lda	#246/3	;
	sta	Mouse0X	;
	lda	#0	;
	beq	negme04	;
nemrt04:	sta	Mouse1Y	; 鼠标 1 的初始位置
	lda	#246/3*2	;
	sta	Mouse1X	;
	lda	#0	;
	beq	negme08	;
;---------------------------------------; 功能 13：读取鼠标。
GetMouseDev:	;
	tya	;
	ldx	#4-1	;
	lsr	; 鼠标编号（位 0）
	bcs	lpgme08	;
lpgme04:	lda	PressCount0,x	; 鼠标 0 的按键计数
	sta	$fc,x	;
	dex	;
	bpl	lpgme04	;
	lda	CurrentJoyKey	;
	ldx	Mouse0X	;
	ldy	Mouse0Y	;
negme04:	pha	;
	lda	#0	;
	sta	PressACount0	; 清除按键计数
	sta	PressBCount0	;
	sta	PressSlCount0	;
	sta	PressStCount0	;
	pla	;
	rts	;
lpgme08:	lda	PressCount1,x	; 鼠标 1 的按键计数
	sta	$fc,x	;
	dex	;
	bpl	lpgme08	;
	lda	CurrentJoyKey+1	;
	ldx	Mouse1X	;
	ldy	Mouse1Y	;
negme08:	pha	;
	lda	#0	;
	sta	PressACount1	;
	sta	PressBCount1	;
	sta	PressSlCount1	;
	sta	PressStCount1	;
	pla	;
	rts	;
;---------------------------------------; 功能 14：设置鼠标位置（$FE/$FF 为 X/Y）。
SetMouseDev:	;
	tya	;
	ldx	$fe	;
	ldy	$ff	;
	lsr	;
	bcs	nesme04	;
	stx	Mouse0X	;
	sty	Mouse0Y	;
	rts	;
nesme04:	stx	Mouse1X	;
	sty	Mouse1Y	;
	rts	;
;---------------------------------------; 功能 15：设置鼠标范围（$FC/$FD 为左上角，$FE/$FF 为右下角）。
SetMouseRangeDev:	;
	tya	;
	and	#1	; 鼠标编号（位 0）
	tay	;
	lda	$fc	;
	sta	MinimumX,y	;
	lda	$fd	;
	sta	MinimumY,y	;
	lda	$fe	;
	sta	MaximumX,y	;
	lda	$ff	;
	sta	MaximumY,y	;
	rts	;

;=======================================; 功能 16：语音初始化。
;    名称：SpeakInit。说明：初始化语音芯片。入口：无。出口：无。
;******************************************************
SpeakInitDev:
	lda #$ff
	sta SpeakInitPort
	pha
	pla
	jsr SpeakNoSoundDev
	pha
	pla
	lda #$ff
	sta SpeakInitPort
	rts

;=======================================; 功能 17：播放一个词。
;    名称：SpeakWord。说明：播放一个词。
;    入口：Y 为词所在页，$FE/$FF 为词地址（低字节、高字节）。
;    出口：无。改写：A、X、Y，以及 BiosTemp1、BiosTemp4、BiosTemp5。
;    调用：SpeakInit；ROMBiosEntry（BiosGetPage、BiosSetPage）；DeviceEntry（TestKeyBoard）。
;*******************************************************
SpeakWordLenLow = BiosTemp4	; 词长度低字节
SpeakWordLenHi = BiosTemp5	; 词长度高字节
SpeakWordAddrLo = $fe	; 词地址低字节
SpeakWordAddrHi = $ff	; 词地址高字节

SpeakWordDev:
	tya	;
	pla	;
	jsr SpeakInitDev	;
	jsr GetPage-$C000+$5800	; 保存当前页
	sta BiosTemp1	;
	pla	;
	tax	;
	jsr SetPageROM	; 映射词所在页
	ldy #$0	;
	lda (SpeakWordAddrLo),y	; 词长度高字节
	sta SpeakWordLenHi	;
	iny	;
	lda (SpeakWordAddrLo),y	; 词长度低字节
	sta SpeakWordLenLow	;
	iny	;
SpeakWordLp1:
	lda SpeakDataPort	; 等待语音芯片就绪
	bpl SpeakWordLp1	;
	cmp #$ff	;
	beq SpeakWordLp1	;
	tya	;
	pha	;
	jsr TestKeyBoardDev-$C000+$5800	; 检查按键
	pla	;
	cpy #0	;
	bne SpeakWordOver	; 有按键：停止播放
	tay	;
	lda (SpeakWordAddrLo),y	;
	sta SpeakDataPort	; 输出一个字节
	iny	;
	bne SpeakWordNe1	;
	inc SpeakWordAddrHi	; 地址跨页
SpeakWordNe1:
	sec	;
	lda SpeakWordLenLow	;
	sbc #$1	; 剩余长度减一
	sta SpeakWordLenLow	;
	bcs SpeakWordNe2	;
	dec SpeakWordLenHi	; 借位
SpeakWordNe2:
	lda SpeakWordLenLow	;
	ora SpeakWordLenHi	;
	bne SpeakWordLp1	;
	beq SpeakWordOk	;
SpeakWordOver:
	ldx #$F0	; 停止：补发 $F0 次 0
SpeakWordLp10:
	lda SpeakDataPort	;
	bpl SpeakWordLp10	;
	lda #$00	;
	sta SpeakDataPort	;
	inx	;
	bne SpeakWordLp10	;
SpeakWordOk:
	ldx BiosTemp1	; 恢复页
	jmp SetPageROM	;

;=======================================; 功能 18：停止发声。
SpeakNoSoundDev:
	lda #$0	;
	sta SpeakInitPort	;
	rts	;
;---------------------------------------;
;    判断 A 是否为锁定键：大写锁定 X=$40，数字锁定 X=$20，滚动锁定 X=$10。Z=1 表示是锁定键。
CheckLockKey:	;
	ldx	#$40	;
	cmp	#$3a	; 大写锁定（$3A）
	beq	neclk04	;
	ldx	#$20	;
	cmp	#$45	; 数字锁定（$45）
	beq	neclk04	;
	ldx	#$10	;
	cmp	#$46	; 滚动锁定（$46）
neclk04:	rts	;
;---------------------------------------;
;    Pause 键之后的 NumLock 处理：清除 Pause 标记，丢弃返回地址并跳过键码。
CheckPauseNL:	;
	lda	#8	; 上一个键是否为 Pause
	bit	KeyStatus3	;
	beq	necpl08	;
	cpx	#$45	; 是：当前键是否为 NumLock
	beq	necpl04	;
	lda	KeyStatus3	;
	and	#$f7	; 清除 Pause 标记
	sta	KeyStatus3	;
necpl04:	pla	;
	pla	;
	jmp	NoKeyCodeClr	;
necpl08:	rts	;
;---------------------------------------;
;    处理锁定键：按下时切换锁定状态并更新 LED；不是锁定键时返回原键码。
CheckLockKeyAc:	;
	txa	;
	jsr	CheckLockKey	; 是否为锁定键？
	bne	necla04	;
	txa	; 是
	bit	KeyStatus2	; 锁定键是否仍按住
	bne	necpl04	;
	ora	KeyStatus2	; 否：记录按下
	sta	KeyStatus2	;
	txa	;
	eor	KeyStatus1	; 切换状态
	sta	KeyStatus1	;
	jsr	ChangeMode	; 更新模式与 LED
	jmp	necpl04	;
necla04:	tax	;
	rts	;
;---------------------------------------;
;    取 Alt 组合键码：A 为 $FF 表示没有对应码，丢弃该键。
ExcAltCode:	;
	lda	AltCode-1,x	;
	cmp	#$ff	;
	beq	necpl04	;
	rts	;
;---------------------------------------;
;    处理 Ins 键：按下时切换插入状态。
CheckInsKey:	;
	cpx	#$52	; 是否为 Ins 键
	bne	nepst04	;
	lda	#$80	;
	bit	KeyStatus2	; Ins 是否仍按住
	bne	necpl04	;
	ora	KeyStatus2	;
	sta	KeyStatus2	;
	lda	#$80	;
	eor	KeyStatus1	; 切换插入状态
	sta	KeyStatus1	;
nepst04:	rts	;
;---------------------------------------;
;    判断修饰键：X 为对应的状态位，Z=1 表示是修饰键。
CheckHoldKey:	;
	ldx	#8	;
	cmp	#$38	; Alt
	beq	nechk04	;
	ldx	#4	;
	cmp	#$1d	; Ctrl
	beq	nechk04	;
;    判断 Shift 键：左 Shift $2A，右 Shift $36。
CheckShiftKey:	;
	ldx	#2	;
	cmp	#$2a	; 左 Shift
	beq	nechk04	;
	ldx	#1	;
	cmp	#$36	; 右 Shift
nechk04:	rts	;
;---------------------------------------;
;    检查是否为 101 键盘：Z=0 表示 101 键盘（KeyStatus3 位 4）。
Check101KB:	;
	lda	#$10	;
	bit	KeyStatus3	;
	rts	;
;---------------------------------------;
;    C 为 KeyStatus3 位 0，即上一个扫描码为 E1 前导。
CheckLastE1:	;
	pha	;
	lda	KeyStatus3	;
	lsr	;
	pla	;
	rts	;
;---------------------------------------;
;    C 为 KeyStatus3 位 1，即上一个扫描码为 E0 前导。
CheckLastE0:	;
	pha	;
	lda	KeyStatus3	;
	lsr	;
	lsr	;
	pla	;
	rts	;
;---------------------------------------;
;    切换模式：按 KeyStatus1 的状态位更新模式标志与 LED。
ChangeMode:	;
	lda	#$40	;
	bit	KeyStatus4	;
	bne	necme12	; 正在切换：忽略
	ora	KeyStatus4	; 置切换标志
	sta	KeyStatus4	;
	lda	KeyStatus1	;
	lsr	;
	lsr	;
	lsr	;
	lsr	;
	and	#7	;
	tax	; 新的 LED 值（X）
	eor	KeyStatus4	;
	and	#7	;
	lda	KeyStatus4	;
	and	#$f8	; 保留 Mode Flag97 的 LED 位
	stx	KeyStatus4	;
	ora	KeyStatus4	;
	sta	KeyStatus4	;
	txa	;
	jsr	SendKBDLED	; 发送 LED
	lda	#$3f	;
	and	KeyStatus4	; 清除切换标志
	sta	KeyStatus4	;
necme12:	rts	;
;=======================================;
;    键码转换：A 为 BIOS 键码，最高位为 1 表示释放。
;    出口：C=1 无键；C=0 时 X/A 为扩展/基础键码。
;---------------------------------------;
GetKeyCode:	;
	tax	;
	bmi	ReleaseKey	;
	jmp	PressKey	;
;---------------------------------------;
;    释放键：清除修饰键与锁定键状态，必要时产生 Alt 组合码。
ReleaseKey:	;
	and	#$7f	;
	jsr	CheckLastE1	; 上一个是否为 E1 前导
	bcc	nerly04	;
	jsr	CheckHoldKey	; 是：是否为 Ctrl、Alt、Shift
	bne	nerly16	;
	jmp	NoKeyCode	;
nerly04:	jsr	CheckLastE0	; 上一个是否为 E0 前导
	bcc	nerly08	;
	jsr	CheckShiftKey	; 是：是否为 Shift
	beq	nerly16	;
nerly08:	jsr	CheckLockKey	;
	beq	nerly12	;
	ldx	#$80	;
	cmp	#$52	; 释放 Ins 键？
	bne	nerly20	;
nerly12:	txa	;
	eor	#$ff	;
	and	KeyStatus2	;
	sta	KeyStatus2	;
nerly16:	jmp	NoKeyCodeClr	;
nerly20:	cmp	#$54	; SysRq 键？
	bne	nerly24	;
	lda	KeyStatus3	; 清除 SysRq 状态
	and	#$fb	;
	sta	KeyStatus3	;
	jmp	NoKeyCode	;
nerly24:	jsr	CheckHoldKey	; 是否为 Ctrl、Alt、Shift
	bne	nerly16	;
	pha	;
	txa	;
	eor	#$ff	;
	tay	; 保存反码
	and	KeyStatus1	;
	sta	KeyStatus1	;
	pla	;
	jsr	CheckShiftKey	; 是否为 Ctrl、Alt
	beq	nerly16	;
	tax	; 是：保存键码
	jsr	CheckLastE0	; 是否为右侧键
	bcc	nerly28	;
	tya	; 是：清除右 Ctrl、右 Alt
	and	KeyStatus2	;
	sta	KeyStatus2	;
	bcs	nerly32	;
nerly28:	tya	; 否：清除左 Ctrl、左 Alt
	lsr	;
	lsr	;
	ora	#$c0	;
	and	KeyStatus2	;
	sta	KeyStatus2	;
nerly32:	lda	KeyStatus2	; 取 Ctrl、Alt 状态
	asl	;
	asl	;
	ora	KeyStatus2	;
	and	#$0c	;
	ora	KeyStatus1	;
	sta	KeyStatus1	;
	cpx	#$38	; 释放 Alt 键？
	bne	nerly16	;
	lda	KeyAltCompose	; 是：取 Alt 加小键盘的组合码
	beq	nerly16	;
	ldx	#0	;
	stx	KeyAltCompose	;
	jmp	CreateKeyCode	;
;    Shift 状态的字符表（名称中的 62 为原名）。$FF 表示没有字符。
ShiftCode62:	.byte $1b,$21,$40,$23,$24,$25,$5e,$26,$2a,$28,$29,$5f,$2b,$08,$00,$51
	.byte $57,$45,$52,$54,$59,$55,$49,$4f,$50,$7b,$7d,$0d,$ff,$41,$53,$44
	.byte $46,$47,$48,$4a,$4b,$4c,$3a,$22,$7e,$ff,$7c,$5a,$58,$43,$56,$42
	.byte $4e,$4d,$3c,$3e,$3f,$ff,$2a,$ff,$20,$ff,$54,$55,$56,$57,$58,$59
	.byte $5a,$5b,$5c,$5d,$ff,$ff,$37,$38,$39,$2d,$34,$35,$36,$2b,$31,$32
	.byte $33,$30,$2e,$ff,$03,$7c,$87,$88
;    不按 Shift 的字符表（名称中的 E2 为原名）。
ShiftCodeE2:	.byte $1b,$31,$32,$33,$34,$35,$36,$37,$38,$39,$30,$2d,$3d,$08,$09,$71
	.byte $77,$65,$72,$74,$79,$75,$69,$6f,$70,$5b,$5d,$0d,$ff,$61,$73,$64
	.byte $66,$67,$68,$6a,$6b,$6c,$3b,$27,$60,$ff,$5c,$7a,$78,$63,$76,$62
	.byte $6e,$6d,$2c,$2e,$2f,$ff,$2a,$ff,$20,$ff,$3b,$3c,$3d,$3e,$3f,$40
	.byte $41,$42,$43,$44,$ff,$ff,$47,$48,$49,$2d,$4b,$f0,$4d,$2b,$4f,$50
	.byte $51,$52,$53,$ff,$03,$5c,$85,$86
;---------------------------------------;
;    按下键：处理 E0 前导、修饰键、系统请求键与 Pause 键，其余键转换为字符或扩展码。
PressKey:	;
nepky08:	jsr	CheckLastE0	; 上一个是否为 E0 前导
	bcc	nepky12	;
	jsr	CheckShiftKey	; 是：是否为 Shift
	bne	nepky12	;
nepky04:	jmp	NoKeyCodeClr	;
nepky12:	jsr	CheckHoldKey	; 是否为修饰键
	bne	nepky20	;
	pha	;
	txa	;
	tay	; 设置修饰键状态
	ora	KeyStatus1	;
	sta	KeyStatus1	;
	pla	;
	jsr	CheckShiftKey	; 是否为 Ctrl、Alt
	beq	nepky04	;
	tya	;
	jsr	CheckLastE0	; 是否为右侧键
	bcc	nepky16	;
	ora	KeyStatus2	; 设置右 Alt 或右 Ctrl
	sta	KeyStatus2	;
	bne	nepky04	;
nepky16:	lsr	;
	lsr	;
	ora	KeyStatus2	; 设置左 Alt 或左 Ctrl
	sta	KeyStatus2	;
	bne	nepky04	;
nepky20:	tax	;
	beq	nepky04	;
	cmp	#$58+1	; 超出键码范围：忽略
	bcs	nepky04	;
	cmp	#$54	; 系统请求键（SysReq）？
	bne	nepky28	;
	lda	#4	; 是
	bit	KeyStatus3	; SysReq 是否已按下
	bne	nepky04	;
	ora	KeyStatus3	;
	sta	KeyStatus3	;
nepky24:	jmp	NoKeyCode	;
nepky28:	lda	#8	; Alt 键是否按下
	bit	KeyStatus1	;
	beq	nepky36	;
	jsr	Check101KB	; 是否为 101/102 键盘
	beq	nepky32	;
	lda	#4	; 是
	bit	KeyStatus3	; SysReq 是否按下
	bne	nepky36	;
nepky32:	jmp	PressAlt	;
nepky36:	lda	#4	;
	bit	KeyStatus1	; Ctrl 是否按下
	beq	nepky38	;
	jmp	PressCtrl	;
nepky38:	cpx	#$55	; 是否为 Pause 键
	bne	nepky40	;
	lda	#8	;
	bit	KeyStatus3	; Pause 状态
	bne	nepky04	;
	jmp	PressPause	;
nepky40:	jsr	CheckLockKeyAc	; 锁定键处理
	cpx	#$52	; 是否为 Ins 键
	beq	nepky44	;
	jsr	CheckPauseNL	;
nepky44:	jsr	CheckPrintScreen	;
	bcs	nepky56	;
	jmp	NoKeyCodeClr	;
nepky56:	jsr	CheckLastE0	;
	bcs	nepky60	;
	lda	#3	; Shift 是否按下
	bit	KeyStatus1	;
	beq	PressNoShift	;
	bne	PressShift	;
nepky60:	cpx	#$1c	; 小键盘回车？
	bne	nepky64	;
	lda	#$0d	;
	bne	nepky68	;
nepky64:	cpx	#$35	; 小键盘 /？
	bne	PressExtend	;
	lda	#$2f	;
nepky68:	ldx	#$e0	; 扩展前缀
	jmp	CreateKeyCode	;
;---------------------------------------;
;    按 Shift 时的字符：NumLock 开启时小键盘键按 Shift 仍输出字符。
PressShift:	;
	lda	#$20	; NumLock 是否开启
	bit	KeyStatus1	;
	beq	nepst08	;
	jsr	CheckInsKey	;
	cpx	#$46+1	;
	bcc	nepst08	;
	cpx	#$56	;
	bcs	nepst08	;
	bcc	nepst16	;
;---------------------------------------;
;    不按 Shift 时的字符：NumLock 开启时小键盘键输出数字。
PressNoShift:	;
	lda	#$20	; NumLock 是否开启
	bit	KeyStatus1	;
	beq	nepst12	;
	cpx	#$46+1	;
	bcc	nepst16	;
	cpx	#$56	;
	bcs	nepst16	;
nepst08:	lda	ShiftCode62-1,x	; 取 Shift 字符表项
	jmp	nepst20	;
PressExtend:	;
nepst12:	jsr	CheckInsKey	; 非小键盘：处理 Ins 键
nepst16:	lda	ShiftCodeE2-1,x	; 取不按 Shift 的字符表项
nepst20:	cmp	#$ff	; 无字符：忽略
	bne	nepst24	;
	jmp	NoKeyCodeClr	;
nepst24:	cpx	#$3a+1	; 小键盘键？
	bcc	nepst28	;
	cmp	#$3a+1	;
	bcc	nepst28	;
	cmp	#$f0	;
	beq	nepst28	;
	cpx	#$56	;
	beq	nepst28	;
	tax	;
	lda	#0	;
	jsr	CheckLastE0	; 是否为 E0 前导
	bcc	nepst28	;
	lda	#$e0	; 是：扩展码 $E0
nepst28:	tay	;
	lda	#$40	; 大写锁定是否开启
	bit	KeyStatus1	;
	beq	nepst36	;
	lda	#3	;
	bit	KeyStatus1	; Shift 是否按下
	bne	nepst32	;
	cpy	#'z'+1	; 未按 Shift：小写字母转为大写
	bcs	nepst36	;
	cpy	#'a'	;
	bcc	nepst36	;
	tya	;
	and	#$df	;
	bne	CreateKeyCode	;
nepst32:	cpy	#'Z'+1	; 按 Shift：大写字母转为小写
	bcs	nepst36	;
	cpy	#'A'	;
	bcc	nepst36	;
	tya	;
	ora	#$20	;
	bne	CreateKeyCode	;
nepst36:	tya	;
;---------------------------------------;
;    产生键码。入口：A 为 ASCII 码，X 为扩展前缀（$E0 或 0）。
;---------------------------------------;
CreateKeyCode:	;
	tay	;
	lda	KeyStatus3	; 清除 E0、E1 前导标记
	and	#$fc	;
	sta	KeyStatus3	;
	tya	;
	cmp	#$f0	;
	bne	neckc04	;
	lda	#0	; $F0 表示无字符：清零
neckc04:	clc	; C=0 表示有键码输出
	rts	;
;---------------------------------------;
NoKeyCodeClr:	;
	lda	KeyStatus3	; 清除 E0、E1 前导标记
	and	#$fc	;
	sta	KeyStatus3	;
NoKeyCode:	;
	sec	; C=1 表示无键码
	rts	;
;    Ctrl 组合的控制码表（$FF 表示没有控制码）。
CtrlCode:	.byte $1b,$ff,$00,$ff,$ff,$ff,$1e,$ff,$ff,$ff,$ff,$1f,$ff,$7f,$94,$11
	.byte $17,$05,$12,$14,$19,$15,$09,$0f,$10,$1b,$1d,$0a,$ff,$01,$13,$04
	.byte $06,$07,$08,$0a,$0b,$0c,$ff,$ff,$ff,$ff,$1c,$1a,$18,$03,$16,$02
	.byte $0e,$0d,$ff,$ff,$95,$ff,$96,$ff,$20,$ff,$5e,$5f,$60,$61,$62,$63
	.byte $64,$65,$66,$67,$45,$46,$77,$8d,$84,$8e,$73,$8f,$74,$90,$75,$91
	.byte $76,$92,$93,$ff,$03,$ff,$89,$8a
;---------------------------------------;
;    Ctrl 按下：产生控制码，或处理 Ctrl+Break、Ctrl+Pause、Ctrl+PrtSc 等组合。
PressCtrl:	;
	jsr	CheckPauseNL	;
	lda	CtrlCode-1,x	;
	cmp	#$ff	; 无控制码：忽略
	beq	nepcl08	;
	cpx	#$45	; Ctrl+NumLock？
	bne	nepcl16	;
PressPause:	;
	lda	#8	; 设置 Pause 状态
	ora	KeyStatus3	;
	sta	KeyStatus3	;
	lda	NMIEnterFlag	; 保存 VSync 等待计数
	pha	;
	jsr	PauseRunningRAM-$C000+$5800	; 暂停（在 RAM 中执行）
	pla	;
	sta	NMIEnterFlag	; 恢复 VSync 等待计数
	jmp	NoKeyCode	;
nepcl08:	jmp	NoKeyCodeClr	;
nepcl16:	cpx	#$55	; Ctrl+Break？
	beq	nepcl20	;
	cpx	#$46	; Ctrl+ScrollLock？
	bne	nepcl24	;
nepcl20:	lda	KBDHead	; 清空键盘缓冲区
	sta	KBDTail	;
	jsr	CtrlBreakEntryRAM-$C000+$5800	; CtrlBreak 入口（在 RAM 中执行）
	lda	#0	;
	tax	;
	jmp	CreateKeyCode	;
nepcl24:	cpx	#$37	; Ctrl+小键盘 * 或 PrtSc？
	bne	nepcl32	;
	tax	;
	jsr	Check101KB	; 是否为 101/102 键盘
	beq	nepcl28	;
	txa	;
	jsr	CheckLastE0	; 是：上一个是否为 E0
	bcc	nepcl44	;
nepcl28:	lda	#$72	; Ctrl+PrtSc
	bne	nepcl44	;
nepcl32:	jsr	CheckLastE0	; 上一个是否为 E0
	bcs	nepcl36	;
	cpx	#$0f	; 否：Ctrl+Tab？
	beq	nepcl44	;
	cpx	#$39	; Ctrl+空格？
	beq	nepcl48	;
	cpx	#$35	; Ctrl+/？
	beq	nepcl08	;
	bcc	nepcl48	;
	bcs	nepcl44	;
nepcl36:	cpx	#$35	; Ctrl+小键盘 /
	beq	nepcl44	;
	cpx	#$1c	; Ctrl+回车？
	beq	nepcl40	;
	tax	;
	lda	#$e0	; 否：ASCII 为 $E0
	bne	nepcl48	;
nepcl40:	ldx	#$e0	;
	bne	nepcl48	;
nepcl44:	tax	; 扩展码
	lda	#0	; ASCII 码为 0
nepcl48:	jmp	CreateKeyCode	;
;    Alt 组合的小键盘数字码表（$FF 表示没有对应码）。
AltCode:	.byte $f0,$78,$79,$7a,$7b,$7c,$7d,$7e,$7f,$80,$81,$82,$83,$f0,$a5,$10
	.byte $11,$12,$13,$14,$15,$16,$17,$18,$19,$f0,$f0,$f0,$ff,$1e,$1f,$20
	.byte $21,$22,$23,$24,$25,$26,$f0,$f0,$f0,$ff,$f0,$2c,$2d,$2e,$2f,$30
	.byte $31,$32,$f0,$f0,$f0,$ff,$f0,$ff,$20,$ff,$68,$69,$6a,$6b,$6c,$6d
	.byte $6e,$6f,$70,$71,$ff,$ff,$07,$08,$09,$f0,$04,$05,$06,$f0,$01,$02
	.byte $03,$00,$53,$ff,$03,$ff,$8b,$8c
;---------------------------------------;
;    Alt 按下：Ctrl+Alt+Del 软复位，否则处理小键盘输入的 Alt 组合码。
PressAlt:	;
	lda	#4	; Ctrl 是否也按下
	bit	KeyStatus1	;
	beq	nepat04	;
	cpx	#$53	; 是否为 Ctrl+Alt+Del
	bne	nepat08	;
	jmp	SoftResetEntry	; 软复位
nepat04:	jsr	CheckLockKeyAc	;
nepat08:	jsr	CheckPauseNL	;
	jsr	ExcAltCode	;
	jsr	CheckLastE0	; 上一个是否为 E0
	bcc	nepat24	;
	ldy	#0	; 是：清除组合码
	sty	KeyAltCompose	;
	cpx	#$1c	; 小键盘回车？
	bne	nepat16	;
	lda	#$a6	;
	bne	nepat36	;
nepat16:	cpx	#$35	; 小键盘 /？
	bne	nepat20	;
	lda	#$a4	;
	bne	nepat36	;
nepat20:	txa	;
	clc	;
	adc	#$50	; 扩展码
	bne	nepat36	;
nepat24:	cmp	#$0a	; 是否为小键盘数字？
	bcs	nepat32	;
	tay	;
	lda	KeyAltCompose	; 组合码 = 组合码 × 10 + 数字
	asl	;
	sta	KeyAltCompose	;
	asl	;
	asl	;
	clc	;
	adc	KeyAltCompose	;
	sty	KeyAltCompose	;
	clc	;
	adc	KeyAltCompose	;
	sta	KeyAltCompose	;
nepat28:	jmp	NoKeyCodeClr	;
nepat32:	ldy	#0	;
	sty	KeyAltCompose	; 清除组合码
	cpx	#$53	; Del 键？
	beq	nepat28	;
	cpx	#$39	; 空格键？
	beq	nepat40	;
	cmp	#$f0	;
	beq	nepat40	;
nepat36:	tax	; 扩展码
	lda	#0	; ASCII 码为 0
nepat40:	jmp	CreateKeyCode	;

;=======================================;
	.segment "S1"	; 预留给 I/O 空间
	.segment "S2"	; 预留给 I/O
DelayTeble:	.byte 13, 26, 48, 51	; 击键延时表（DelayTimes 取值）
	.segment "S3"	;
	.segment "S4"	; 预留给 I/O
;---------------------------------------;
;    发送键盘 LED：A 的位 2 为 CapsLock 灯，位 1 为 NumLock 灯（低电平有效）。
SendKBDLED:	;
	eor	#7	; 取反（低电平有效）
	sta	KebBoardLEDPort	;
	rts	;
;---------------------------------------; 功能 4：释放键盘（保留，直接返回）。
ReleaseKeyBoardDev:	;
	rts	;
	.segment "S5"	;
	.segment "S6"	; 预留给 I/O

	.segment "S7"	;
	.word NMIEntry-$C000+$5800	; 向量：NMI
	.word Begin	; 向量：RESET
	.word BRKINTEntry-$C000+$5800	; 向量：IRQ/BRK

