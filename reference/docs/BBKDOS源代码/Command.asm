DOSVersion	equ	3
DOSSubVersion	equ	0
	include	tvdef.asm
	org	$8000
	jmp	Initial
;-------------------------------------------------------;
FileIOTmp	equ	$0400			; ds HandleSize
;-----------------------------------------------;
SizeCount	equ	$e0			; ds 4
 FDate		equ	 $e0			;  ds 2
 FTime		equ	 $e2			;  ds 2
FileCount	equ	$e4			; ds 2
FileNameFlag	equ	$e6			; ds 1
PathNameTmpEnd	equ	$e7			; ds 1
PathNameEndingC	equ	$e8			; ds 1
FileNameEndingC	equ	$e9			; ds 1
PathNameEnding	equ	$ea			; ds 1
PathNameTarEnd	equ	$eb			; ds 1
PathNameSrcEnd	equ	$ec			; ds 1
SegmentEnd	equ	$ed			; ds 1
TP		equ	$ee			; ds 1
MP		equ	$ef			; ds 1

;-----------------------------------------------;
FileIOSrc	equ	$6100			; ds HandleSize
FileIOTar	equ	FileIOSrc+HandleSize	; ds HandleSize
FileIODsk	equ	FileIOTar+HandleSize	; ds HandleSize
TypeBuffer	equ	FileIODsk+HandleSize	; ds 256
CommandLineBeg	equ	TypeBuffer+256		;|ds 2
CommandLine	equ	CommandLineBeg+2	;|ds 80
CmdLLen		equ	80			;
CommandLineP	equ	CommandLine+CmdLLen	; ds 1
StringLength	equ	CommandLineP+1		; ds 1
CommandSaveLP	equ	StringLength+1		; ds 1
InsertModeFlag	equ	CommandSaveLP+1		; ds 1
CommandCurBegin	equ	InsertModeFlag+1	; ds 1
PathFileNameTmp	equ	CommandCurBegin+1	; ds 80
TempDrive	equ	PathFileNameTmp+CmdLLen	; ds 1
PathFileNameSrc	equ	TempDrive+1		; ds 80
SourceDrive	equ	PathFileNameSrc+CmdLLen	; ds 1
PathFileNameTar	equ	SourceDrive+1		; ds 80
TargetDrive	equ	PathFileNameTar+CmdLLen	; ds 1
RunBATBuffer	equ	TargetDrive+1		; ds 1
SwitchHelpIndex	equ	RunBATBuffer+1		; ds 1
FreePage	equ	SwitchHelpIndex+1	; ds 1 Program free page.
DisplayLower	equ	FreePage+1		; ds 1 Display with lower case
TempSave	equ	DisplayLower+1		; ds 1
DisplayMode	equ	TempSave+1		; ds 1 ==0: ASCII, <0: Hanzi
SwitchHelp	equ	DisplayMode+1		; ds 1 Is /? switch.
SwitchPoint	equ	SwitchHelp+1		; ds 2
EchoFlagLine	equ	SwitchPoint+2		; ds 1
CmdPermanentTMP	equ	EchoFlagLine+1		; ds 1
CmdRunProgTMP	equ	CmdPermanentTMP+1	; ds 1
CmdStepThTMP	equ	CmdRunProgTMP+1		; ds 1
RunProgramTypes	equ	4			;
RunProgramFlag	equ	CmdStepThTMP+1		; ds RunProgramTypes
;-----------------------------------------------;
CtrlCCommand	equ	RunProgramFlag+RunProgramTypes

StrFiles	db	" file(s) $"
StrBytes	db	" bytes$"
StrFree		db	" bytes free$"
StrNoFile	db	"File not found$"
StrVolumn	db	"Drive $"
StrIS		db	" volumn is $"
StrNoLabel	db	" volumn has no label$"
StrInvdDrive	db	"Invalid drive specification$"
StrTooPara	db	"Too many parameters$"
StrErrSwitch	db	"Invalid switch$"
StrBadCmd	db	"Bad command or file name$"
StrDelAll	db	"All files in directory will be",$0d,$0a
		db	"deleted! Are you sure (Y/N)?$"
StrParaMiss	db	"Required parameter missing$"
StrAccDeny	db	"Access denied$"
StrOvWr		db	"Overwrite $"
StrYNA		db	13,10,"(Yes/No/All)?$"
StrCopied	db	"copied$"
StrNoDiskSp	db	"Insufficient disk space$"
StrCopySelf	db	"File can't be copied to itself$"
StrDataLost	db	"Dest. content lost before copy$"
StrInvdName	db	"Error filename / file no found$"
StrInvdParam	db	"Invalid parameter$"
StrDupFName	db	"Duplicate file name$"
StrDRAMSmall	db	"Program too big to fit memory$"
StrVRAMSmall	db	"Video memory too small$"
StrExcProgEr	db	"Program format error$"
StrProgRdErr	db	"Program read error$"
StrProAbsent	db	"Program not exist$"
StrProgRet	db	10,"Return from running program$"
StrDispMode	db	"Program must run at HANZI mode$"
StrPressKey	db	"Press any key to continue ...$"
StrDeleteSign	db	", Delete(Y/N)?$"
StrBBGDOSVer	db	$0a,"BBGDOS Version "
		db	DOSVersion+'0'
		db	"."
		db	DOSSubVersion+'0'
		db	"$"
StrParaError	db	"Parameter not correct$"
StrNoMemory	db	"No enough memory to work.$"
StrCurrentDate	db	"Current date is $"
StrEnterDate	db	"Enter new date(mm-dd-yy):$"
StrInvalidDate	db	"Invalid date$"
StrCurrentTime	db	"Current time is $"
StrEnterTime	db	"Enter new time: $"
StrInvalidTime	db	"Invalid time$"
StrDir		db	" <DIR>    $"
StrInvdDir	db	"Invalid directory$"
StrErrMakeDir	db	"Unable to create directory$"
StrErrRemoveDir	db	"Not found this directory,",$0d,$0a
		db	"or directory not empty.$"
StrDirectory	db	"Directory of $"
StrRDCurtDir	db	"Attemp to remove current direc",$0d,$0a
		db	"tory - $"
StrPathNoFound	db	"Path not found$"
StrDirContinue	db	"(continuing $"
StrTotalFile	db	"Total files listed:$"
StrTermBatJob	db	$0d,$0a
		db	"Terminate batch job (Y/N)?$"
StrGenError	db	"General error$"
StrDriveNoValid	db	"Current drive is not valid$"
StrCmdNotExit	db	"COMMAND is resident, Exit deny$"
StrRunPgTooDep	db	"Program is too depth to runing$"
StrEcho		db	"ECHO is $"
StrOn		db	"on$"
StrOff		db	"off$"
StrErrNumInp	db	"Error screen mode input!$"
StrCDOSHpSign	db	"$"
;---------------------------------------;
SthFiles	db	" 个文件$"
SthBytes	db	" 字节$"
SthFree		db	" 剩余字节$"
SthNoFile	db	"文件没有找到$"
SthVolumn	db	"磁盘$"
SthIS		db	"的卷标是 $"
SthNoLabel	db	"没有卷标$"
SthInvdDrive	db	"磁盘不存在$"
SthTooPara	db	"参数太多$"
SthErrSwitch	db	"非法开关$"
SthBadCmd	db	"命令错误,未找到执行程序$"
SthDelAll	db	"目录中的所有文件都删除!",$0d,$0a
		db	"是否确定(Y/N)?$"
SthParaMiss	db	"缺少必要的参数$"
SthAccDeny	db	"禁止访问$"
SthOvWr		db	"覆盖掉 $"
SthYNA		db	13,10,"(是Y/否N/全A)$"
SthCopied	db	"被复制$"
SthNoDiskSp	db	"磁盘空间不够$"
SthCopySelf	db	"文件不能自我复制$"
SthDataLost	db	"目标文件在复制前内容被破坏$"
SthInvdName	db	"文件名非法或文件未找到$"
SthInvdParam	db	"参数非法$"
SthDupFName	db	"重复相同的文件名$"
SthDRAMSmall	db	"程序太大不能装入到内存中执行$"
SthVRAMSmall	db	"显示内存太小程序不能执行$"
SthExcProgEr	db	"程序格式错误$"
SthProgRdErr	db	"程序读错误$"
SthProAbsent	db	"程序不存在$"
SthProgRet	db	10,"从执行程序中返回$"
SthDispMode	db	"本程序必须在西文DOS环境下执行$"
SthPressKey	db	"按任意键继续...$"
SthDeleteSign	db	",是否删除(Y/N)?$"
SthBBGDOSVer	db	$0a,"BBGDOS 第"
		db	DOSVersion+'0'
		db	"."
		db	DOSSubVersion+'0'
		db	"版$"
SthParaError	db	"参数格式不正确$"
SthNoMemory	db	"没有足够的内容运行这一命令$"
SthCurrentDate	db	"当前日期是$"
SthEnterDate	db	"输入新日期(月-日-年):$"
SthInvalidDate	db	"无效日期$"
SthCurrentTime	db	"当前的时间是$"
SthEnterTime	db	"输入新时间: $"
SthInvalidTime	db	"无效时间$"
SthDir		db	" <目录>   $"
SthInvdDir	db	"目录无效$"
SthErrMakeDir	db	"不能建立目录$"
SthErrRemoveDir	db	"找不到目录或目录有内容$"
SthDirectory	db	"目录是 $"
SthRDCurtDir	db	"试图删除当前目录 - $"
SthPathNoFound	db	"路径未找到$"
SthDirContinue	db	"(接着 $"
SthTotalFile	db	"列出的所有文件:$"
SthTermBatJob	db	$0d,$0a
		db	"中止执行批处理命令(Y/N)?$"
SthGenError	db	"一般错误$"
SthDriveNoValid	db	"当前盘不再有效$"
SthCmdNotExit	db	"当前COMMAND已驻留,不能退出!$"
SthRunPgTooDep	db	"程序驻留层数太多,不能再执行!$"
SthEcho		db	"ECHO 是 $"
SthOn		db	"开$"
SthOff		db	"关$"
SthErrNumInp	db	"所输入的屏幕方式错误!$"

;=======================================;
ErrorProgram:				;
	orgrun	CtrlCCommand		;
CtrlCProgram:				;
	lda	RunBATEnter		;
	bne	necpm04			;
	lda	#BiosSetPage		;
	ldx	#$fe			;
	jsr	ROMBiosEntry		;
	jmp	RunProgramAbort		;
necpm04:jsr	CheckCommand		;
	jmp	RunBATCtrlCReturn	;
CtrlCProgLen	equ	@-CtrlCProgram	;
;---------------------------------------;
FaultProgram:				;
	jsr	FaultErrorCall		; Call original fault error process.
	cpy	#2			; Is Abort?
	beq	nefpm04			;
	rts				; No.
nefpm04:lda	#BiosSetPage		; Yes.
	ldx	#$fe			;
	jsr	ROMBiosEntry		;
	jmp	FaultAbortProc		;
FaultErrorCall:				;
	jmp	FaultErrorEntS		;
FaultProgLen	equ	@-FaultProgram	;
;---------------------------------------;
FaultCommand	equ	CtrlCCommand+CtrlCProgLen
CommandDataEnd	equ	FaultCommand+FaultProgLen
	orgreal				;
ErrorProgLen	equ	@-ErrorProgram	;
;---------------------------------------;
SetErrorProgram:			;
	ldy	#ErrorProgLen-1		;
lpscp04:lda	ErrorProgram,y		;
	sta	CtrlCCommand,y		;
	dey				;
	bpl	lpscp04			;
	lda	#$4c			; JMP CtrlCProcEntryCL to vector.
	ldx	#<CtrlCCommand		;
	ldy	#>CtrlCCommand		;
	sta	CtrlCProcEntry		;
	stx	CtrlCProcEntry+2	;
	sty	CtrlCProcEntry+1	;
	ldx	#<FaultCommand		;
	ldy	#>FaultCommand		;
	sta	FaultErrorEntry		;
	stx	FaultErrorEntry+2	;
	sty	FaultErrorEntry+1	;
	rts
;---------------------------------------;
RestoreErrorEntry:			;
	lda	CtrlCProcEntryS		; Restore original vector.
	ldx	CtrlCProcEntryS+2	;
	ldy	CtrlCProcEntryS+1	;
	sta	CtrlCProcEntry		;
	stx	CtrlCProcEntry+2	;
	sty	CtrlCProcEntry+1	;
	lda	FaultErrorEntS		;
	ldx	FaultErrorEntS+2	;
	ldy	FaultErrorEntS+1	;
	sta	FaultErrorEntry		;
	stx	FaultErrorEntry+2	;
	sty	FaultErrorEntry+1	;
	rts				;
;---------------------------------------;
StringFiles		equ	0
StringBytes		equ	1
StringNoFile		equ	2	;|
StringPathNoFound	equ	3	;|
StringRenHpSign		equ	4
StringAccDeny		equ	5	;|
StringIS		equ	6
StringNoLabel		equ	7
StringOvWr		equ	8
StringTooPara		equ	9
StringErrSwitch		equ	10
StringBadCmd		equ	11
StringDelAll		equ	12
StringParaMiss		equ	13
StringVolumn		equ	14
StringInvdDrive		equ	15	;|
StringYNA		equ	16
StringCopied		equ	17
StringNoDiskSp		equ	18
StringCopySelf		equ	19
StringDataLost		equ	20
StringInvdName		equ	21
StringInvdParam		equ	22
StringDupFName		equ	23
StringDRAMSmall		equ	24
StringVRAMSmall		equ	25
StringExcProgEr		equ	26
StringProgRdErr		equ	27
StringProAbsent		equ	28
StringProgRet		equ	29
StringNoLowRAM		equ	30
StringGenError		equ	31	;|
StringPressKey		equ	32
StringDirHpSign		equ	33
StringCopyHpSign	equ	34
StringCmdHpSign		equ	35
StringCopyright		equ	36
StringDelHpSign		equ	37
StringDeleteSign	equ	38
StringVerHpSign		equ	39
StringBBGDOSVer		equ	40
StringVOLHpSign		equ	41
StringParaError		equ	42
StringInCHpsign		equ	43
StringRemHpSign		equ	44
StringClsHpSign		equ	45
StringTypeHpSign	equ	46
StringNoMemory		equ	47
StringExitHpSign	equ	48
StringDateHpSign	equ	49
StringCurrentDate	equ	50
StringEnterDate		equ	51
StringInvalidDate	equ	52
StringTimeHpSign	equ	53
StringCurrentTime	equ	54
StringEnterTime		equ	55
StringInvalidTime	equ	56
StringCDHpSign		equ	57
StringMDHpSign		equ	58
StringRDHpSign		equ	59
StringDir		equ	60
StringInvdDir		equ	61
StringErrMakeDir	equ	62
StringErrRemoveDir	equ	63
StringDirectory		equ	64
StringRDCurtDir		equ	65
StringFree		equ	66
StringDirContinue	equ	67
StringTotalFile		equ	68
StringTermBatJob	equ	69
StringDispMode		equ	70
StringDriveNoValid	equ	71
StringCmdNotExit	equ	72
StringRunPgTooDep	equ	73
StringEcho		equ	74
StringEchoHpSign	equ	75
StringOn		equ	76
StringOff		equ	77
StringScrHpSign		equ	78
StringErrNumInp		equ	79
StringErrorVersion	equ	80
StringCDOSHpSign	equ	81
StrASC	dw	StrFiles,	StrBytes,	StrNoFile,	StrPathNoFound
	dw	StrRenHpSign,	StrAccDeny,	StrIS,		StrNoLabel
	dw	StrOvWr,	StrTooPara,	StrErrSwitch,	StrBadCmd	
	dw	StrDelAll,	StrParaMiss,	StrVolumn,	StrInvdDrive
	dw	StrYNA,		StrCopied,	StrNoDiskSp,	StrCopySelf
	dw	StrDataLost,	StrInvdName,	StrInvdParam,	StrDupFName
	dw	StrDRAMSmall,	StrVRAMSmall,	StrExcProgEr,	StrProgRdErr
	dw	StrProAbsent,	StrProgRet,	StrNoLowRAM,	StrGenError
	dw	StrPressKey,	StrDirHpSign,	StrCopyHpSign,	StrCmdHpSign
	dw	StrCopyright,	StrDelHpSign,	StrDeleteSign,	StrVerHpSign
	dw	StrBBGDOSVer,	StrVOLHpSign,	StrParaError,	StrInCHpsign
	dw	StrRemHpSign,	StrClsHpSign,	StrTypeHpSign,	StrNoMemory
	dw	StrExitHpSign,	StrDateHpSign,	StrCurrentDate,	StrEnterDate
	dw	StrInvalidDate,	StrTimeHpSign,	StrCurrentTime,	StrEnterTime
	dw	StrInvalidTime,	StrCDHpSign,	StrMDHpSign,	StrRDHpSign
	dw	StrDir,		StrInvdDir,	StrErrMakeDir,	StrErrRemoveDir
	dw	StrDirectory,	StrRDCurtDir,	StrFree,	StrDirContinue
	dw	StrTotalFile,	StrTermBatJob,	StrDispMode,	StrDriveNoValid
	dw	StrCmdNotExit,	StrRunPgTooDep,	StrEcho,	StrEchoHpSign
	dw	StrOn,		StrOff,		StrScrHpSign,	StrErrNumInp
	dw	StrErrorVersion,StrCDOSHpSign
;-----------------------------------------------------------------------------
StrHAN	dw	SthFiles,	SthBytes,	SthNoFile,	SthPathNoFound
	dw	SthRenHpSign,	SthAccDeny,	SthIS,		SthNoLabel
	dw	SthOvWr,	SthTooPara,	SthErrSwitch,	SthBadCmd	
	dw	SthDelAll,	SthParaMiss,	SthVolumn,	SthInvdDrive
	dw	SthYNA,		SthCopied,	SthNoDiskSp,	SthCopySelf
	dw	SthDataLost,	SthInvdName,	SthInvdParam,	SthDupFName
	dw	SthDRAMSmall,	SthVRAMSmall,	SthExcProgEr,	SthProgRdErr
	dw	SthProAbsent,	SthProgRet,	SthNoLowRAM,	SthGenError
	dw	SthPressKey,	SthDirHpSign,	SthCopyHpSign,	SthCmdHpSign
	dw	SthCopyright,	SthDelHpSign,	SthDeleteSign,	SthVerHpSign
	dw	SthBBGDOSVer,	SthVOLHpSign,	SthParaError,	SthInCHpsign
	dw	SthRemHpSign,	SthClsHpSign,	SthTypeHpSign,	SthNoMemory
	dw	SthExitHpSign,	SthDateHpSign,	SthCurrentDate,	SthEnterDate
	dw	SthInvalidDate,	SthTimeHpSign,	SthCurrentTime,	SthEnterTime
	dw	SthInvalidTime,	SthCDHpSign,	SthMDHpSign,	SthRDHpSign
	dw	SthDir,		SthInvdDir,	SthErrMakeDir,	SthErrRemoveDir
	dw	SthDirectory,	SthRDCurtDir,	SthFree,	SthDirContinue
	dw	SthTotalFile,	SthTermBatJob,	SthDispMode,	SthDriveNoValid
	dw	SthCmdNotExit,	SthRunPgTooDep,	SthEcho,	SthEchoHpSign
	dw	SthOn,		SthOff,		SthScrHpSign,	SthErrNumInp
	dw	SthErrorVersion,StrCDOSHpSign
ReturnErrString	db	StringDRAMSmall,	StringDispMode
		db	StringExcProgEr,	StringProAbsent
		db	StringProgRdErr,	StringVRAMSmall
		db	StringRunPgTooDep
ReturnErrType	db	TooSmallDRAM,	DispModeError
		db	FormatError,	FileNotExist
		db	ErrRead,	TooSmallVRAM
		db	StackTooDepth
ReturnErrTypeL	equ	@-ReturnErrType

;---------------------------------------;
DisplayCtrlC:				;
	ldx	#'^'			;
	jsr	DisplayASCII		;
	ldx	#'C'			;
	jsr	DisplayASCII		;
	ldx	#$0d			;
	jsr	DisplayASCII		;
	ldx	#$0a			;
;	jmp	DisplayASCII		;
;---------------------------------------;
DisplayASCII:				;
	lda	#BiosDisplayTTY		;
	ldy	#0			;
	jmp	ROMBiosEntry		;
;---------------------------------------;
CharToUpper:				;
	cmp	#'a'			;
	bcc	nectu04			;
	cmp	#'z'+1			;
	bcs	nectu04			;
	and	#$df			;
nectu04:rts				;
;---------------------------------------;
DisplayString:				;
	asl				;
	tax				;
	lda	StrASC+1,x		;
	ldy	StrASC,x		;
	bit	DisplayMode		; Current is HANZI mode?
	bpl	nedsg04			;
	lda	StrHAN+1,x		;
	ldy	StrHAN,x		;
nedsg04:tax				;
	lda	#DosDisplayString	;
	jmp	DosIOEntry		;
;---------------------------------------;
;    Display two characters.		;
;	X|A = Two characters.		;
;---------------------------------------;
DisplayCharTwo:				;
	pha				;
	jsr	DisplayChar		;
	pla				;
;---------------------------------------;
DisplayCharA:				;
	tax				;
;---------------------------------------;
;	Save	(Y).			;
;---------------------------------------;
DisplayChar:				;
	tya				;
	pha				;
	lda	#DosDisplayChar		;
	jsr	DosIOEntry		;
	pla				;
	tay				;
	rts				;
;---------------------------------------;
DisplayStringCRLF:			; Return with Cy=0.
	jsr	DisplayString		;
;	jmp	DisplayCRLF		;
;---------------------------------------;
DisplayCRLF:				;
	ldx	#$0d			;
	lda	#$0a			;
	jsr	DisplayCharTwo		;
	clc				;
	rts				;
;---------------------------------------;
;	Input	X|Y= DWord base pointer.;
;		(A)= Pointer index	;
;	Output	SizeCount+=DWord.	;
;---------------------------------------;
CountPlused:				;
	stx	$ff			;
	sty	$fe			;
	tay				;
	lda	($fe),y			;
	sta	$00			;
	clc				;
	adc	SizeCount		;
	sta	SizeCount		;
	iny				;
	lda	($fe),y			;
	sta	$01			;
	adc	SizeCount+1		;
	sta	SizeCount+1		;
	iny				;
	lda	($fe),y			;
	sta	$02			;
	adc	SizeCount+2		;
	sta	SizeCount+2		;
	iny				;
	lda	($fe),y			;
	sta	$03			;
	adc	SizeCount+3		;
	sta	SizeCount+3		;
	rts				;
;---------------------------------------;
;	Input	X|Y= DWord base pointer.;
;		(A)= Pointer index	;
;		(TempSave)		;
;	Output	SizeCount+=DWord.	;
;		Display DWord		;
;---------------------------------------;
DisplayDWord:				;
	jsr	CountPlused		;
	ldx	#11			;
lpddw16:ldy	#32			; Div 10.
	lda	#0			;
lpddw20:rol	$00			;
	rol	$01			;
	rol	$02			;
	rol	$03			;
	dey				;
	bmi	neddw12			;
	rol				;
	cmp	#10			;
	bcc	lpddw20			;
	sbc	#10			;
	bcs	lpddw20			;
neddw12:ora	#'0'			;
	sta	$80-1,x			;
	dex				;
	beq	neddw24			;
	lda	$00			;
	ora	$01			;
	ora	$02			;
	ora	$03			;
	beq	neddw20			;
	cpx	#8			; Next [']
	beq	neddw16			; Yes.
	cpx	#4			; First [']
	bne	lpddw16			;
	lda	TempSave		; Yes. Display 10 character length?
	beq	neddw16			;
	lda	$01			; Yes. Data > 9M?
	bne	lpddw16			; Yes.
	lda	$00			;
	cmp	#10			;
	bcs	lpddw16			;
neddw16:lda	#','			; Insert ","
	sta	$80-1,x			;
	dex				;
	bne	lpddw16	;(jmp)		;
neddw20:lda	#' '			;
lpddw24:sta	$80-1,x			;
	dex				;
	bne	lpddw24			;
neddw24:ldy	TempSave		; Display filesize.
lpddw28:ldx	$0080,y			;
	iny				;
	jsr	DisplayChar		;
	cpy	#11			;
	bne	lpddw28			;
	rts				;
;---------------------------------------;
;	Input	X|Y= Dividend.		;
;		A=   Divesor.		;
;	Output	A=   Quotient.		;
;		X=   Remainer.		;
;		Y=   0.			;
;		Zy= (A)'s.		;
;		Cy=  Original C.	;
;---------------------------------------;
DivXY_A:sty	$fd			;
	sta	$fc			;
	txa				;
	ldy	#8			;
lpxya04:rol	$fd			;
	rol	a			;
	cmp	$fc			;
	bcc	nexya04			;
	sbc	$fc			;
nexya04:dey				;
	bne	lpxya04			;
	tax				;
	lda	$fd			;
	rol	a			;
	rts				;
;---------------------------------------;
DisplayTwo:				;
	cmp	#100			;
	bcc	nedto04			;
	sbc	#100			;
nedto04:tay				;
	ldx	#0			;
	lda	#10			;
	jsr	DivXY_A			;
	stx	TempSave		;
	ora	#'0'			;
	tax				; high.
	lda	TempSave		;
	ora	#'0'			;
	jmp	DisplayCharTwo		; low low.
;---------------------------------------;
;    Pass all space character($09,$20)	;
;	Output	Zy=1: Last char=$0d.	;
;	Save	(X).			;
;---------------------------------------;
PassSpace:				;
lppse04:lda	CommandLine,y		;
	cmp	#$20			;
	beq	nepse04			;
	cmp	#$09			;
	beq	nepse04			;
	cmp	#','			;
	beq	nepse04			;
	cmp	#';'			;
	beq	nepse04			;
	cmp	#']'			;
	beq	nepse04			;
	cmp	#'['			;
	bne	nepse08			;
nepse04:iny				;
	bne	lppse04		;(jmp)	;
nepse08:cmp	#$0d			;
	rts				;
;========================================================; Command
DTA_Begin	equ	0		;|---- Search file information returned.
DTA_FileAttrib	equ	DTA_Begin+0	;| ds 1
DTA_FileTime	equ	DTA_FileAttrib+1;| ds 2
DTA_FileDate	equ	DTA_FileTime+2	;| ds 2
DTA_FileSize	equ	DTA_FileDate+2	;| ds 4
DTA_Filename	equ	DTA_FileSize+4	;| ds 11
 DTA_DirSize	equ	DTA_Filename+11-DTA_Begin
DTA_FilePath	equ	DTA_Filename+11	;| ds 3
DTA_DirFstClus	equ	DTA_FilePath+3	;| ds 2
 DTA_Size	equ	DTA_DirFstClus+2-DTA_Begin
;---------------------------------------;
PCheckPhaseBreakS:			;
	lda	CommandLine,y		;
	cmp	#'+'			;
	beq	necpb04			;
;---------------------------------------;
CCheckPhaseBreakS:			;
	lda	CommandLine,y		;
;---------------------------------------;
CheckPhaseBreakS:			;
	cmp	#'/'			;
	beq	necpb04			;
;---------------------------------------;
CheckPhaseBreak:			;
	cmp	#$0d			;
	beq	necpb04			;
	cmp	#$20 			;
	beq	necpb04			;
	cmp	#$09			;
	beq	necpb04			;
	cmp	#','			;
	beq	necpb04			;
	cmp	#';'			;
	beq	necpb04			;
	cmp	#']'			;
	beq	necpb04			;
	cmp	#'['			;
necpb04:rts				;
;---------------------------------------;
;	Output	(Cy)=1: Has '*' or '?'	;
;---------------------------------------;
CheckNameMark				;
	ldx	PathNameEnding		;
	ldy	#0			;
lpcnm04:lda	PathFileNameTmp,x	;
	beq	necnm08			;
	cpy	#8			;
	beq	necnm04			;
	cmp	#'*'			;
	beq	necnm12			;
	cmp	#'?'			;
	beq	necnm12			;
	iny				;
necnm04:inx				;
	cmp	#'.'			;
	bne	lpcnm04			;
	cpy	#1			; First is '.'?
	beq	necnm12			; Yes (Cy)=0.
necnm08:clc				;
necnm12:rts				;
;---------------------------------------;
CheckNameExtMark:			;
	tya				;
	pha				;
	jsr	CheckNameMark		;
	bcs	necne08			;
	ldy	#8			;
lpcne04:lda	PathFileNameTmp,x	;
	beq	necne04			;
	cmp	#'*'			;
	beq	necne08			;
	cmp	#'?'			;
	beq	necne08			;
	inx				;
	iny				;
	cpy	#11			;
	bne	lpcne04			;
necne04:clc				;
necne08:pla				;
	tay				;
	rts				;
;---------------------------------------;
AccessPathName:				;
	ldx	#0			;
lpapn04:jsr	PCheckPhaseBreakS	;
	beq	neapn04			;
	sta	PathFileNameTmp,x	;
	iny				;
	inx				;
	cmp	#'\'			;
	bne	lpapn04			;
	stx	PathNameEnding		;
	sty	PathNameEndingC		;
	beq	lpapn04		;(jmp)	;
neapn04:lda	#0			;
	sta	PathFileNameTmp,x	;
	rts				;
;---------------------------------------;
IOTmpHBUFToFE:				;
	ldx	FileIOTmp+HBUF+1	;
	ldy	FileIOTmp+HBUF		;
	stx	$fe+1			;
	sty	$fe			;
	rts				;
;---------------------------------------;
;	Save	(Y).			;
;---------------------------------------;
SearchTmpEnding:			;
	ldx	PathNameEnding		;
lpste04:inx				;
	lda	PathFileNameTmp,x	;
	bne	lpste04			;
	rts				;
;---------------------------------------;
;	Input	(Y)= FileName index	;
;		(X)= Drive		;
;		(A):			;
;	  Bit 7: Enable PATH filename.	;
;	  Bit 6: .x->*.x, x->x.*	;
;	  Bit 0: Empty -> *.*		;
;	Output	(Cy)=0: No error.	;
;		 (Y)= OK.		;
;		(Cy)=1: Error.		;
;---------------------------------------;
AccessPathFileName:			;
	sta	FileNameFlag		;
	stx	FileIOTmp+CURDRIVE	;
	sty	FileNameEndingC		;
	ldx	#0			; Search last '\' position in filepath. 
	stx	PathNameEnding		;
	jsr	PCheckPhaseBreakS	; Empty filepath string?
	beq	neapf08			;
	sty	PathNameEndingC		; No.
	jsr	AccessPathName		;
	sty	FileNameEndingC		;
	ldx	PathNameEnding		; Has any '\' character in filepath?
	beq	neapf04			;
	ldy	PathNameEndingC		; Yes. The '\' is last character of filepath?
	jsr	PCheckPhaseBreakS	;
	bne	neapf04			;
	lda	#PathNotFound		;
	cpx	#1			; Yes. Only '\' character of filepath?
	bne	neapf16			;
	jmp	neapf44		;(jmp)	; Yes. Set to \*.*
neapf04:bit	FileNameFlag		; Can access filename with whole path?
	bpl	neapf24			;
	jsr	CheckNameExtMark	; Yes.
	bcs	neapf24			;
	lda	#$17			; Attribute.
	sta	FileIOTmp+BYTES		;
	lda	#FilePathFirst		; Yes.
lpapf12:jsr	TempFileOperP		; Search the path of same filename.
	bcs	neapf20			;
	jsr	IOTmpHBUFToFE		; DTA.
	ldy	#DTA_FileAttrib		;
	lda	($fe),y			; Yes. Attribute SET ok?
	and	#$10			; Is directory item?
	bne	neapf40			;
	lda	#FileNext		;
	bne	lpapf12		;(jmp)	;
neapf08:lda	FileNameFlag		;
	lsr				;
	bcs	neapf44			;
	lda	#FileNotExist		;
neapf16:sec				;
	rts				;
neapf20:cmp	#SearchEnding		;
	bne	neapf16			;
neapf24:ldx	PathNameEnding		;
	ldy	PathNameEndingC		;
	jsr	PCheckPhaseBreakS	; Access file name.
	cmp	#'.'			;
	bne	neapf36			;
	iny				;
	jsr	PCheckPhaseBreakS	; Only a '.'?
	beq	neapf44			;
	cmp	#'.'			;
	bne	neapf32			;
	iny				;
	jsr	PCheckPhaseBreakS	; Only a ".."?
	bne	neapf28			;
	inx				; Yes. Change to ..\*.*
	inx				;
	bne	neapf42		;(jmp)	;
neapf28:lda	#'*'			; No.  Change to *.
	sta	PathFileNameTmp,x	;
	inx				;
	bne	neapf52		;(jmp)	;
;---------------------------------------;
neapf32:bit	FileNameFlag		; No.
	bvc	neapf60			;
	dey				;
	lda	#'*'			;
	sta	PathFileNameTmp,x	;
	inx				;
neapf36:bit	FileNameFlag		; No.
	bvc	neapf60			;
lpapf16:jsr	PCheckPhaseBreakS	;
	beq	neapf48			;
	iny				;
	sta	PathFileNameTmp,x	;
	inx				;
	cmp	#'.'			;
	bne	lpapf16			;
lpapf20:jsr	PCheckPhaseBreakS	;
	beq	neapf56			;
	iny				;
	sta	PathFileNameTmp,x	;
	inx				;
	bne	lpapf20		;(jmp)	;
neapf40:jsr	SearchTmpEnding		; Yes. Path "\*.*" to filename ending.
neapf42:lda	#'\'			;
	sta	PathFileNameTmp,x	;
	inx				;
	stx	PathNameEnding		;
;---------------------------------------;
SetTmpAllFile:				;
neapf44:lda	#'*'			;
	sta	PathFileNameTmp,x	;
	inx				;
neapf48:lda	#'.'			;
	sta	PathFileNameTmp,x	;
	inx				;
	lda	#'*'			;
	sta	PathFileNameTmp,x	;
neapf52:inx				;
neapf56:lda	#0			;
	sta	PathFileNameTmp,x	;
neapf60:ldy	FileNameEndingC		;
	clc				;
	rts				;
;---------------------------------------;
CommandLineError:			;
	tya				; Error too many parameters.
	pha				;
	txa				;
	jsr	DisplayString		;
	ldx	#' '			;
	jsr	DisplayChar		;
	ldx	#'-'			;
	lda	#' '			;
	jsr	DisplayCharTwo		;
	pla				;
	tay				;
	lda	CommandLine,y		; Can display first '/'
	jsr	CheckPhaseBreak		;
	beq	necle04			;
lpcle04:iny				;
	jsr	DisplayCharA		;
	jsr	CCheckPhaseBreakS	;
	bne	lpcle04			;
necle04:jsr	DisplayCRLF		;
	clc				;
	rts				;

;---------------------------------------;
DisplayWord:				;
	lda	#' '			;
	sta	$80			;
	sta	$80+1			;
	sta	$80+2			;
	lda	#100			;
	clc				;
	jsr	DivXY_A			;
	beq	nedwd12			;
	stx	TempSave		;
	ldx	#0			;
	tay				;
	lda	#10			;
	jsr	DivXY_A			;
	beq	nedwd04			;
	ora	#'0'			;
	sta	$80			;
	sec				;
nedwd04:txa				;
	bne	nedwd08			;
	bcc	nedwd12			;
nedwd08:ora	#'0'			;
	sta	$80+1			;
	sec				;
	ldx	TempSave		;
nedwd12:txa				;
	tay				;
	ldx	#0			;
	lda	#10			;
	jsr	DivXY_A			;
	bne	nedwd16			;
	bcc	nedwd20			;
nedwd16:ora	#'0'			;
	sta	$80+2			;
nedwd20:txa				;
	ora	#'0'			;
	sta	$80+3			;
	lda	#'$'			; Files.
	sta	$80+4			;
	ldx	#<$0080			;
	ldy	#>$0080			;
	lda	#DosDisplayString	;
	jmp	DosIOEntry		;
;---------------------------------------;
DisplayFileCount:			;
	ldx	FileCount+1		;
	ldy	FileCount		;
DisplayFileCountIN:			;
	jsr	DisplayWord		;
	lda	#StringFiles		;
	jmp	DisplayString		;
;---------------------------------------;
;	Output	FileIOTmp+CURP: Size.	;
;---------------------------------------;
GetDiskFreeSize:			;
	lda	#FileDriveSize		;
	jsr	TempFileOper		;
	bcs	negdf12			;
	lda	FileIOTmp+HBUF		; sectors per cluster.
lpgdf04:lsr				;
	bcs	negdf04			;
	asl	FileIOTmp+BYTES		; bytes pre secotrs.
	rol	FileIOTmp+BYTES+1	;
	jmp	lpgdf04			;
negdf04:lda	#0			;
	sta	FileIOTmp+CURP+2	;
	sta	FileIOTmp+CURP+3	;
lpgdf08:lsr	FileIOTmp+BYTES+1	;
	ror	FileIOTmp+BYTES		;
	bcs	negdf08			;
	asl	FileIOTmp+CURP		;
	rol	FileIOTmp+CURP+1	;
	rol	FileIOTmp+CURP+2	;
	rol	FileIOTmp+CURP+3	;
	jmp	lpgdf08			;
negdf08:clc				;
negdf12:rts				;
;---------------------------------------;
TempFileOperFP:				;
	sta	FileIOTmp+BYTES		;
	lda	#FilePathFirst		;
;---------------------------------------;
TempFileOperP:				;
	ldx	#<PathFileNameTmp	;
	ldy	#>PathFileNameTmp	;
TempFileOperHBUF:			;
	stx	FileIOTmp+HBUF+1	;
	sty	FileIOTmp+HBUF		;
;---------------------------------------;
TempFileOper:				;
	sta	FileIOTmp+FILECMD	;
	ldx	#<FileIOTmp		;
	ldy	#>FileIOTmp		;
	jmp	FileIOEntry		;
;---------------------------------------;
SourceFileOperTmp:			;
	ldx	#<PathFileNameTmp	;
	ldy	#>PathFileNameTmp	;
	jmp	SourceFileOperHBUF	;
;---------------------------------------;
SourceFileOperP:			;
	ldx	#<PathFileNameSrc	;
	ldy	#>PathFileNameSrc	;
SourceFileOperHBUF:			;
	stx	FileIOSrc+HBUF+1	;
	sty	FileIOSrc+HBUF		;
;---------------------------------------;
SourceFileOper:				;
	sta	FileIOSrc+FILECMD	;
	ldx	#<FileIOSrc		;
	ldy	#>FileIOSrc		;
	jmp	FileIOEntry		;
;---------------------------------------;
TargetFileOperP:			;
	ldx	#<PathFileNameTar	;
	ldy	#>PathFileNameTar	;
	stx	FileIOTar+HBUF+1	;
	sty	FileIOTar+HBUF		;
TargetFileOper:				;
	sta	FileIOTar+FILECMD	;
	ldx	#<FileIOTar		;
	ldy	#>FileIOTar		;
	jmp	FileIOEntry		;

;---------------------------------------;
;	Input	(Y)= Switch string point;
;		(A)= First switch char.	;
;	Output	RTS&RTS when error	;
;		Zy=1 CommandLine end.	;
;		Zy=0 Commandline noend.	;
;		  (Y)= Next.		;
;---------------------------------------;
SwitchBranch:				;
	iny				;
	iny				;
	lda	($fe),y			;
	pha				;
	dey				;
	lda	($fe),y			;
	pha				;
	txa				;
	tay				;
	rts				;
;---------------------------------------;
CheckSwitch:				;
lpcsh04:jsr	PassSpace		; Analyze switchs
	beq	necsh16			;
	cmp	#'/'			; Is switch leading?
	bne	necsh16			;
	iny				; Yes. Read switch character.
	tya				;
	tax				;
	lda	CommandLine,y		;
	cmp	#$0d			; Only '/' in line last?
	beq	necsh12			; Yes.
	cmp	#'?'			;
	beq	necsh20			;
	and	#$df			;
	sta	$fd			;
	lda	SwitchPoint+1		; Switch structure pointer.
	sta	$fe+1			;
	lda	SwitchPoint		;
	sta	$fe			;
	ldy	#0			;
	lda	CommandLine+1,x		; Only one switch char?
	jsr	CheckPhaseBreakS	;
	beq	lpcsh12			;
lpcsh08:lda	($fe),y			; No. Search two or more swtich beginning.
	beq	necsh12			;
	bmi	necsh04			;
	iny				;
	iny				;
	iny				;
	bne	lpcsh08			;
necsh04:iny				;
lpcsh12:lda	($fe),y			; Yes.
	beq	necsh12			;
	bmi	necsh12			;
	cmp	$fd			;
	beq	necsh08			;
	iny				;
	iny				;
	iny				;
	bne	lpcsh12		;(jmp)	;
necsh08:inx				;
	clc				;
	jsr	SwitchBranch		;
	bcc	lpcsh04			;
	tya				;
	tax				;
	dex				;
necsh12:dex				;
	pla				; POP stack.
	pla				;
	txa				;
	tay				;
	ldx	#StringErrSwitch	; Error switch
	jmp	CommandLineError	;
necsh16:rts				;
necsh20:sta	SwitchHelp		;
	pla				;
	pla				;
	lda	SwitchHelpIndex		;
	jmp	DisplayStringCRLF	;

;---------------------------------------;
;	Output	(X)= Drive.		;
;		(A)=0/Drive.		;
;---------------------------------------;
CheckDriveCmd:				;
	lda	CommandLine,y		; Is "X:" stirng?
	cmp	#$0d			;
	beq	necdc12			;
	ldx	CommandLine+1,y		;
	cmp	#$0d			;
	beq	necdc12			;
	cpx	#':'			;
	bne	necdc12			;
	and	#$df			; driver character valid?
	cmp	#'Z'+1			;
	bcs	necdc04			;
	cmp	#'A'			;
	bcs	necdc08			;
necdc04:pla				;
	pla				;
	lda	#StringInvdDrive	;
	sec				;
	rts				;
necdc08:sbc	#'A'-1			;
	iny				;
	iny				;
	tax				;
	clc				;
	rts				;
necdc12:tya				;
	pha				;
	lda	#FileDrive		; Get default driver.
	sta	FileIODsk+FILECMD	;
	ldx	#0			;
	stx	FileIODsk+CURDRIVE	;
	ldx	#<FileIODsk		;
	ldy	#>FileIODsk		;
	jsr	FileIOEntry		;
	pla				;
	tay				;
	ldx	FileIODsk+CURDRIVE	;
	lda	#0			;
	rts				;
;---------------------------------------;
PathNameTarToTmp:			;
	ldx	#>-1			; Move Tar PathFileName to Tmp.
lpptt04:inx				;
	lda	PathFileNameTar,x	;
	sta	PathFileNameTmp,x	;
	bne	lpptt04			;
	rts				;
;---------------------------------------;
PathNameTmpToTar:			;
	ldx	#>-1			; Move Tmp PathFileName to Tar.
lpptt08:inx				;
	lda	PathFileNameTmp,x	;
	sta	PathFileNameTar,x	;
	bne	lpptt08			;
	rts				;
;---------------------------------------;
MoveTmpFNToPath:			;
	jsr	IOTmpHBUFToFE		; DTA.
MoveTmpFNToPathX:			;
	ldx	PathNameTmpEnd		;
MoveTmpFNToPathFE:			;
	ldy	#DTA_Filename		;
lpmfp04:lda	($fe),y			;
	cmp	#' '			;
	beq	nemfp04			;
	sta	PathFileNameTmp,x	;
	inx				;
	iny				;
	cpy	#DTA_Filename+8		;
	bne	lpmfp04			;
nemfp04:ldy	#DTA_Filename+8		;
	lda	($fe),y			;
	cmp	#' '			;
	beq	nemfp08			;
	lda	#'.'			;
	sta	PathFileNameTmp,x	;
	inx				;
lpmfp08:lda	($fe),y			;
	cmp	#' '			;
	beq	nemfp08			;
	sta	PathFileNameTmp,x	;
	inx				;
	iny				;
	cpy	#DTA_Filename+11	;
	bne	lpmfp08			;
nemfp08:lda	#0			;
	sta	PathFileNameTmp,x	;
	rts				;
;---------------------------------------;
DisplayPrompt:				;
	ldx	#0			;
	stx	FileIOTmp+CURDRIVE	;
	jsr	DisplayCurtPath		;
	bcc	nedpt04			;
	lda	#StringDriveNoValid	;
	jsr	DisplayString		;
nedpt04:ldx	#'>'			;
	jmp	DisplayChar		;

;---------------------------------------;
;	Return	(Zy)=0: Step over.	;
;---------------------------------------;
Y_N	db	" [Y/N]?$"
StepThroughAsk:				;
	lda	CmdStepThrough		; Step through flag on?
	beq	nesta12			;
	lda	CommandLine		; Yes. Is a empty line?
	cmp	#$0d			;
	beq	nesta12			; Yes.
	jsr	DisplayCRLF		;
	ldy	#0			;
lpsta04:ldx	CommandLine,y		;
	cpx	#$0d			;
	beq	nesta04			;
	jsr	DisplayChar		;
	iny				;
	bne	lpsta04		;(jmp)	;
nesta04:lda	#DosDisplayString	;
	ldx	#<Y_N			;
	ldy	#>Y_N			;
	jsr	DosIOEntry		;
lpsta08:lda	#DosReadKeyNoEcho	; Read key.
	jsr	DosIOEntry		;
	and	#$df			;
	cmp	#'Y'			;
	beq	nesta08			;
	cmp	#'N'			;
	bne	lpsta08			;
nesta08:pha				;
	jsr	DisplayCharA		;
	jsr	DisplayCRLF		;
	pla				;
	cmp	#'Y'			;
nesta12:rts				;
;---------------------------------------;
ReadOneBATChar:				;
	ldx	#<RunBATBuffer		;
	ldy	#>RunBATBuffer		;
	stx	FileIOInter+HBUF+1	;
	sty	FileIOInter+HBUF	;
	ldx	#<0001			;
	ldy	#>0001			;
	stx	FileIOInter+BYTES+1	;
	sty	FileIOInter+BYTES	;
	lda	#FileRead		;
	sta	FileIOInter+FILECMD	;
	ldx	#<FileIOInter		;
	ldy	#>FileIOInter		;
	jsr	FileIOEntry		;
	lda	FileIOInter+BYTES	;
	rts				;
;---------------------------------------;
RunBATCtrlCReturn:			;
	ldx	#$ff-2			; 1 depth CALL remained.
	txs				;
	lda	#<ProgramEnd-$8000+$ff	;
	sta	CheckSumPages		;
	jsr	DisplayCtrlC		;
lpbcr04:lda	#StringTermBatJob	;
	jsr	DisplayString		;
	lda	#DosReadKeyEcho		; Read key.
	jsr	DosIOEntry		;
	and	#$df			;
	cmp	#'Y'			; Terminate?
	beq	nebcr04			; Yes.
	cmp	#'N'			; No terminate?
	bne	lpbcr04			;
	jsr	DisplayCRLF		;
lpbcr08:jsr	ReadOneBATChar		; Yes. Search next line.
	bcs	nebcr04			;
	beq	nebcr04			;
	lda	RunBATBuffer		;
	cmp	#$0d			;
	beq	RunBATProgramLoop	;
	cmp	#$1a			;
	bne	lpbcr08			;
nebcr04:jsr	DisplayCRLF		;
	jmp	RunBATProgramEnd	;
;=======================================;
RunBATProgram:				; Run XXXXXXXX.BAT program
	lda	RunBATEnter		; Call from RunBATProgram?
	beq	nerbp04			;
	pla				; Yes.
	pla				; Pass JSR from RunBATProgram
	lda	#FileClose		; Ignore last BAT handle.
	sta	FileIOInter+FILECMD	;
	ldx	#<FileIOInter		;
	ldy	#>FileIOInter		;
	jsr	FileIOEntry		;
nerbp04:inc	RunBATEnter		;
	jsr	DisplayCRLF		;
	lda	EchoFlagCommand		;
	sta	EchoFlagBAT		;
	lda	FileIOTmp+CURDRIVE	;
	sta	FileIOInter+CURDRIVE	;
	ldx	#<PathFileNameTmp	;
	ldy	#>PathFileNameTmp	;
	stx	FileIOInter+HBUF+1	;
	sty	FileIOInter+HBUF	;
	lda	#FilePathOpen		; Open file.
	sta	FileIOInter+FILECMD	;
	ldx	#<FileIOInter		;
	ldy	#>FileIOInter		;
	jsr	FileIOEntry		;
	bcc	RunBATProgramLoop	;
	lda	#StringNoFile		;
	rts				;
;---------------------------------------;
RunBATProgramLoop:			;
	lda	EchoFlagBAT		;
	sta	EchoFlagLine		;
lprbl04:jsr	ReadOneBATChar		;
	bcs	RunBATProgramEnd	;
	beq	nerbl28			;
	lda	RunBATBuffer		;
	cmp	#' '			;
	beq	lprbl04			;
	cmp	#'	'		;
	beq	lprbl04			;
	cmp	#$0a			;
	beq	lprbl04			;
	cmp	#'@'			; Command first character is '@'?
	bne	nerbl04			;
	dec	EchoFlagLine		; Yes.
	jsr	ReadOneBATChar		;
	bcs	RunBATProgramEnd	;
	beq	nerbl28			;
nerbl04:bit	EchoFlagLine		; Echo off?
	bmi	nerbl08			;
	jsr	DisplayPrompt		; No.
nerbl08:ldy	#0			; Command line point.
	beq	nerbl12		;(jmp)	;
lprbl08:tya				;
	pha				;
lprbl12:jsr	ReadOneBATChar		;
	pla				;
	tay				;
	bcs	RunBATProgramEnd	;
	lda	FileIOInter+BYTES	; Read BAT all?
	beq	nerbl28			;
nerbl12:lda	RunBATBuffer		;
	cmp	#$0d			; No.
	beq	nerbl16			;
	cmp	#$1a			;
	beq	nerbl28			;
	cpy	#CmdLLen-1		;
	bcs	lprbl08			;
	sta	CommandLine,y		;
	iny				;
	tax				;
	tya				;
	pha				;
	bit	EchoFlagLine		; Echo off?
	bmi	lprbl12			;
	jsr	DisplayChar		; No.
	jmp	lprbl12			;
nerbl16:sta	CommandLine,y		;
	jsr	StepThroughAsk		;
	bne	RunBATProgramLoop	;
	jsr	CommandLineAccs		;
	jsr	CommandReturnAccs	;
	jmp	RunBATProgramLoop	;
nerbl28:lda	#$0d			; BAT file end.
	sta	CommandLine,y		;
	jsr	StepThroughAsk		;
	bne	RunBATProgramEnd	;
	jsr	CommandLineAccs		;
	jsr	CommandReturnAccs	;
RunBATProgramEnd:			;
	lda	#FileClose		;
	sta	FileIOInter+FILECMD	;
	ldx	#<FileIOInter		;
	ldy	#>FileIOInter		;
	jsr	FileIOEntry		;
	ldy	#0			;
	sty	RunBATEnter		;
	clc				;
	rts				;

;---------------------------------------;
CDSwitch:				;
ClsSwitch:				;
DateSwitch:				;
EchoSwitch:				;
ExitSwitch:				;
IntCmdSwitch:				;
MDSwitch:				;
RDSwitch:				;
RemarksSwitch:				;
RenameSwitch:				;
TimeSwitch:				;
TypeSwitch:				;
VOLSwitch:				;
VersionSwitch:				;
ScreenSwitch:				;
	db	0			;

StrExitHpSign	db	"Quits the COMMAND.CMD program.",$0d,$0a
		db	$0a
		db	"EXIT$"
SthExitHpSign	db	"退出命令解释器程序COMMAND.CMD",$0d,$0a
		db	"EXIT$"
;=======================================;
ExitCommand:				;
	jsr	CheckSwitch		;
	beq	neexc04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
neexc04:lda	CmdPermanent		;
	beq	ExitCommandIN		;
	lda	#StringCmdNotExit	;
	sec				;
	rts				;
ExitCommandIN:				;
	lda	#BiosLowRAMFree		; Free low RAM
	ldx	#<RunProgramDataL	; Low RAM beginning
	ldy	#>RunProgramDataL	;
	jsr	ROMBiosEntry		;
	ldx	RunProgramDataP+1	; Restore RunPorgramData area from local.
	ldy	RunProgramDataP		;
	stx	$fe+1			;
	sty	$fe			;
	ldy	#RunProgramDataL-1	;
lpexc04:lda	($fe),y			;
	sta	RunProgramDataB,y	;
	dey				;
	bpl	lpexc04			;
	lda	#0			;
	sta	KBStringLen		;
	jsr	RestoreErrorEntry	;
	lda	#DosProgramReturn	;
	ldy	#0			;
	jmp	DosIOEntry		;

StrRemHpSign	db	"Remarks in batch or CONFIG.SYS",$0d,$0a
		db	$0a
		db	"REM [comment]$"
SthRemHpSign	db	"批处理或CONFIG.SYS中的注释命令",$0d,$0a
		db	"REM [comment]$"
;=======================================;
REMCommand:				;
	jsr	PassSpace		; Analyze switchs
	beq	nermc04			;
	cmp	#'/'			; Is switch leading?
	bne	nermc04			;
	lda	CommandLine+1,y		;
	cmp	#'?'			;
	bne	nermc04			;
	lda	CommandLine+2,y		;
	cmp	#$0d			;
	bne	nermc04			;
	lda	#StringRemHpSign	;
	jmp	DisplayStringCRLF	;
nermc04:clc				;
	rts				;

StrTypeHpSign	db	"Display the contents of a file",$0d,$0a
		db	$0a
		db	"TYPE [drive:][path]filename$"
SthTypeHpSign	db	"显示一个文本文件的内容",$0d,$0a
		db	"TYPE [盘:][路径]文件名$"
;=======================================;
TypeReadSize	equ	$f8		; ds 2
TypeReadPointH	equ	$fa		; ds 1
FileNamePoint	equ	$fb		; ds 1
TypeCommand:				;
	jsr	CheckSwitch		;
	bne	netpf04			;
	lda	#StringParaMiss		;
	sec				;
	rts				;
netpf04:jsr	CheckDriveCmd		;
	sta	TempDrive		;
	sty	FileNamePoint		;
	lda	#0			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	netpf12			;
	jsr	CheckSwitch		;
	beq	netpf08			;
	ldx	#StringTooPara		;
	bne	netpf28		;(jmp)	;
;---------------------------------------;
netpf08:jsr	CheckNameExtMark	;
	bcs	netpf16			;
	lda	#FilePathOpen		;
	jsr	TempFileOperP		;
	bcc	netpf20			;
netpf12:tax				;
	bne	netpf24		;(jmp)	;
netpf16:lda	#StringInvdName		;
	sec				;
	rts				;
netpf20:lda	FileIOTmp+HPAGE		; File attribute.
	and	#$18			; Is <DIR> or VOLUMN?
	beq	netpf32			;
	ldx	#StringAccDeny		;
netpf24:ldy	FileNamePoint		;
netpf28:jmp	CommandLineError	;
netpf32:ldx	FreePage		;
	cpx	#$fe			;
	bcc	netpf36			;
	lda	#StringNoMemory		;
	sec				;
	rts				;
netpf36:stx	FileIOTmp+HPAGE		;
	ldx	#<$4000			;
	ldy	#>$4000			;
	stx	FileIOTmp+BYTES+1	;
	sty	FileIOTmp+BYTES		;
lptpf08:lda	#FileRead		; Read one page file
	ldx	#<$8000			;
	ldy	#>$8000			;
	stx	TypeReadPointH		;
	jsr	TempFileOperHBUF	;
	bcs	netpf52			;
	ldx	FileIOTmp+BYTES+1	; Read ending?
	ldy	FileIOTmp+BYTES		;
	txa				;
	ora	FileIOTmp+BYTES		;
	beq	netpf48			;
	stx	TypeReadSize+1		; No.
	sty	TypeReadSize		;
lptpf12:ldx	FreePage		;
	stx	$80			;
	ldx	TypeReadPointH		;
	ldy	#0			;
	stx	$82			;
	sty	$81			;
	inc	TypeReadPointH		;
	ldx	#<TypeBuffer		;
	ldy	#>TypeBuffer		;
	stx	$85			;
	sty	$84			;
	ldx	#<256			;
	ldy	#>256			;
	stx	$87			;
	sty	$86			;
	lda	#BiosMoveBlock		;
	ldx	#$80			;
	jsr	ROMBiosEntry		;
	ldy	#0			;
lptpf16:ldx	TypeBuffer,y		; Display one character.
	cpx	#$1a			; Text file end?
	beq	netpf48			;
	jsr	DisplayChar		; Save Y.
	lda	TypeReadSize		; SizeCount--;
	sec				;
	sbc	#1			;
	sta	TypeReadSize		;
	bcs	netpf40			;
	dec	TypeReadSize+1		;
netpf40:ora	TypeReadSize+1		;
	beq	netpf44			;
	iny				;
	bne	lptpf16			;
	jmp	lptpf12			;
netpf44:lda	FileIOTmp+BYTES+1	; Last read file block full?
	cmp	#<$4000			;
	beq	lptpf08			;
netpf48:lda	#FileClose		; No.
	jsr	TempFileOper		;
netpf52:clc				;
	rts				;

;---------------------------------------;
StrRenHpSign	db	"Renames a file or files",$0d,$0a
		db	$0a
		db	"RENAME or",$0d,$0a
		db	"REN [drive:][path]filename1",$0d,$0a
		db	"     filename2$"
SthRenHpSign	db	"对一个文件或多个文件改名",$0d,$0a
		db	"RENAME 或",$0d,$0a
		db	"REN [盘:][路径]文件名1 文件名2$"
RenAnyFileFlag	equ	$fb		;
;=======================================;
RenameCommand:				;
	jsr	CheckSwitch		;
	beq	nernf04			;
	jsr	CheckDriveCmd		; Analyze source filename
	sta	FileIOSrc+CURDRIVE	;
	sta	FileIOTar+CURDRIVE	;
	lda	#0			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	nernf06			;
	jsr	CheckSwitch		;
	bne	nernf08			;
nernf04:lda	#StringParaMiss		;
	sec				;
nernf06:rts				;
nernf08:jsr	PathNameTmpToTar	;
	lda	PathNameEnding		;
	sta	PathNameTarEnd		;
	sta	PathNameTmpEnd		;
	jsr	CheckDriveCmd		; Analyze target filename
	tax				; Target filename has D:
	beq	nernf12			;
	lda	#StringInvdParam	; Yes. StringInvdName
	sec				;
	rts				;
nernf12:lda	#0			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	nernf18			;
	jsr	CheckSwitch		;
	beq	nernf16			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
nernf16:ldx	PathNameEnding		;
	beq	nernf20			;
	lda	#StringInvdName		; Yes. 
	sec				;
nernf18:rts				;
nernf20:stx	PathNameSrcEnd		;
	dex		;(X)=-1		; Move Tmp PathFileName to Src.
lprnf08:inx				;
	lda	PathFileNameTmp,x	;
	sta	PathFileNameSrc,x	;
	bne	lprnf08			;
	jsr	PathNameTarToTmp	;
;---------------------------------------;
	lda	#1			; Attribute.
	sta	FileIOTmp+BYTES		;
	lda	FileIOTar+CURDRIVE	;
	sta	FileIOTmp+CURDRIVE	;
	lda	#FilePathFirst		;
lprnf16:jsr	TempFileOperP		; Search file.
	bcs	nernf36			;
	lda	#1			; Find one file at least.
	sta	RenAnyFileFlag		;
	jsr	MoveTmpFNToPath		; Move filename to PathFileNameTmp.
	jsr	MoveTmpSrcTar		;
	lda	#FilePathOpen		; Target file exist?
	jsr	TargetFileOperP		;
	bcc	nernf32			;
	cmp	#FileNotExist		; No.
	bne	nernf44			;
	lda	#FilePathOpen		; Open source file.
	jsr	SourceFileOperTmp	;
	bcs	nernf44			;
	ldx	#<PathFileNameTar	;
	lda	#>PathFileNameTar	;
	clc				;
	adc	PathNameTarEnd		;
	bcc	nernf24			;
	inx				;
nernf24:stx	$fe+1			;
	sta	$fe			;
	tay				;
	lda	#DosFileNameAccs	;
	jsr	DosIOEntry		;
	lda	#FileRename		; Rename.
	ldx	$fe+1			;
	ldy	$fe			;
	jsr	SourceFileOperHBUF	;
	bcs	nernf40			;
	lda	#FileClose		;
	jsr	SourceFileOper		;
	bcs	nernf44			;
	lda	#FileNext		;
	jmp	lprnf16			;
nernf32:lda	#FileClose		; File exist.
	jsr	TargetFileOper		;
	lda	#StringDupFName		;
	sec				;
	rts				;
nernf36:lda	RenAnyFileFlag		; Has rename any file?
	bne	nernf44			;
	lda	#StringNoFile		;
	sec				;
	rts				;
nernf40:lda	#FileClose		;
	jsr	SourceFileOper		;
nernf44:clc				;
	rts				;

StrClsHpSign	db	"Clears the screen",$0d,$0a
		db	$0a
		db	"CLS$"
SthClsHpSign	db	"清除显示屏幕",$0d,$0a
		db	"CLS$"
;=======================================;
ClsCommand:				;
	jsr	CheckSwitch		;
	lda	#DosInitialScreen	;
	jsr	DosIOEntry		;
	lda	#BiosSetCursor		;
	ldx	#1			;
	ldy	#0			;
	jsr	ROMBiosEntry		;
	clc				;
	rts				;
	
StrScrHpSign	db	"Set new screen display mode",$0d,$0a
		db	$0a
		db	"SCREEN Num",$0d,$0a
		db	$0a
		db	"num: New screen mode number.",$0d,$0a
		db	"     0: ASCII, 1: HANZI$"
SthScrHpSign	db	"设置新的屏幕显示方式",$0d,$0a
		db	$0a
		db	"SCREEN 数值",$0d,$0a
		db	$0a
		db	"数值: 新的屏幕显示方式",$0d,$0a
		db	"    0: 英文, 1: 中/英文$"
;=======================================;
ScreenCommand:				;
	jsr	CheckSwitch		;
	bne	nescd04			;
	lda	#StringParaMiss		;
	sec				;
	rts				;
nescd04:jsr	GetDigitalString	;
	bne	nescd12			;
	cmp	#$0d			;
	bne	nescd12			;
	ldy	$ff			;
	cpy	#1			;
	bcc	SetDisplayMode		;
	bne	nescd12			;
	ldy	#$80			;
;---------------------------------------;
SetDisplayMode:				;
	lda	#DosGetSetDispMode	; Set display mode.
	ldx	#1			;
	jsr	DosIOEntry		;
	bcs	nescd12			;
	lda	#DosGetSetDispMode	; Get display mode.
	ldx	#0			;
	jsr	DosIOEntry		;
	sta	DisplayMode		;
	lda	#StringBBGDOSVer	;
	jmp	DisplayStringCRLF	;
nescd12:lda	#StringErrNumInp	;
	sec				;
	rts				;

StrDateHpSign	db	"Displays or sets the date.",$0d,$0a
		db	$0a
		db	"DATE [mm-dd-yy]",$0d,$0a
		db	$0a
		db	"mm-dd-yy The date that you set",$0d,$0a
		db	$0a
		db	"Type DATE only to show date and"
		db	"wait for a new one. Press ENTER"
		db	"to keep the same date.$"
SthDateHpSign	db	"显示或设置日期",$0d,$0a
		db	"DATE [mm-dd-yy]",$0d,$0a
		db	$0a
		db	"mm-dd-yy 要设置的日期",$0d,$0a
		db	$0a
		db	"只输入DATE显示当前日期并等待输",$0d,$0a
		db	"入新日期，按回车则保持日期不变$"
;---------------------------------------;
DisplayYear:				;
	lsr				;
	clc				;
	adc	#>1980			; 1980 base
	tay				;
	lda	#0			;
	adc	#<1980			;
	tax				;
	lda	#100			;
	jsr	DivXY_A			;
	stx	$fe			;
	jsr	DisplayTwo		;
	lda	$fe			;
	jmp	DisplayTwo		;
;---------------------------------------;
DisplayMonth:				;
	lsr				;
	lda	$ff			;
	ror				;
	lsr				;
	lsr				;
	lsr				;
	lsr				; Month.
	jmp	DisplayTwo		;
;---------------------------------------;
CheckDigital:				;
	lda	CommandLine,y		;
	cmp	#'9'+1			;
	bcs	necdl04			;
	cmp	#'0'			;
	bcs	necdl08			;
	sec				;
necdl04:rts				;
necdl08:sbc	#'0'			;
	clc				;
	rts				;
StarTableHZ	db	"二三四五六日一";
StarTableAS	db	"TueWenThuFriSatSunMon"
StarMonthEnd	db	0,3,3,6,1,4,6,2,5,0,3,5
;---------------------------------------;
;	Input	A|$ff= Data.		;
;---------------------------------------;
GetStar:lsr				;
	sta	$fc			; Year(80-...).
	lda	$ff			;
	ror				;
	lsr				;
	lsr				;
	lsr				;
	lsr				; Month(1-12).
	tay				;
	lda	$fc			;
	clc				;
	adc	#3			;
	pha				;
	and	#3			;
	tax				;
	pla				;
	lsr				;
	lsr				;
	clc				;
	adc	$fc			;
	sta	$fc			; (Cy)=0
	lda	$ff			;
	and	#$1f			; Date(1-60).
	sbc	#0			;
	cpx	#3			; In YunYue?
	bcc	negsr04			;
	cpy	#2+1			; Yes. Month >=2(Yunn perhaps)?
negsr04:adc	$fc			;
	adc	StarMonthEnd-1,y	;
	tay				;
	ldx	#0			;
	lda	#7			;
	jsr	DivXY_A			;
	txa				;
	rts				;
;---------------------------------------;
GetDigitalString:			;
	ldx	#0			;
	stx	$ff			;
lpgds04:jsr	CheckDigital		;
	bcs	negds04			;
	iny				;
	sta	$fe			;
	lda	#10			;
	jsr	MulXFF_A		; (X|A)=(X|$ff)*(A)
	clc				;
	adc	$fe			;
	sta	$ff			;
	bcc	lpgds04			;
	inx				;
	bcs	lpgds04		;(jmp)	;
negds04:cpx	#0			;
	rts				;
;---------------------------------------;
NewMonth	equ	$f9		;
NewDate		equ	$fa		;
DateCommand:				;
	jsr	CheckSwitch		;
	beq	DateCommandIN		;
	jmp	nedec12			;
DateCommandIN:				;
	lda	#StringCurrentDate	;
	jsr	DisplayString		;
	lda	#BiosGetDateTime	; Get system date and uime.
	jsr	ROMBiosEntry		;
	bit	DisplayMode		;
	bpl	nedec04			;
	pha				;
	jsr	DisplayYear		; Year.
	ldx	#<'年'			;
	lda	#>'年'			;
	jsr	DisplayCharTwo		;
	pla				;
	pha
	jsr	DisplayMonth		; Month.
	ldx	#<'月'			;
	lda	#>'月'			;
	jsr	DisplayCharTwo		;
	lda	$ff			; Data.
	and	#$1f			;
	jsr	DisplayTwo		;
	ldx	#<'日'			;
	lda	#>'日'			;
	jsr	DisplayCharTwo		;
	ldx	#<'星'			;
	lda	#>'星'			;
	jsr	DisplayCharTwo		;
	ldx	#<'期'			;
	lda	#>'期'			;
	jsr	DisplayCharTwo		;
	pla				;
	jsr	GetStar			;
	asl				;
	tay				;
	ldx	StarTableHZ,y		;
	lda	StarTableHZ+1,y		;
	jsr	DisplayCharTwo		;
	jmp	lpdec04			;
nedec04:pha				;
	jsr	GetStar			;
	sta	$fe			; Star index.
	asl				;
	adc	$fe			;
	tay				;
	lda	StarTableAS+2,y		;
	pha				;
	ldx	StarTableAS,y		;
	lda	StarTableAS+1,y		;
	jsr	DisplayCharTwo		;
	pla				;
	tax				;
	lda	#' '			;
	jsr	DisplayCharTwo		;
	pla				;
	pha				;
	jsr	DisplayMonth		; Month.
	ldx	#'-'			;
	jsr	DisplayChar		;
	lda	$ff			; Data.
	and	#$1f			;
	jsr	DisplayTwo		;
	ldx	#'-'			;
	jsr	DisplayChar		;
	pla				;
	jsr	DisplayYear		; Year.
lpdec04:jsr	DisplayCRLF		;
	lda	#StringEnterDate	;
	jsr	DisplayStringCRLF	;
	lda	#31			;
	sta	CommandLineBeg		;
	lda	#DosGetKBString		;
	ldx	#<CommandLineBeg	;
	ldy	#>CommandLineBeg	;
	jsr	DosIOEntry		;
	bcc	nedec08			;
	jsr	DisplayCtrlC		; Ctrl/C input.
	clc				;
	rts				;
nedec08:ldy	#0			;
	jsr	PassSpace		;
	beq	nedec24			;
	tya				;
	pha				;
	jsr	DisplayCRLF		;
	pla				;
	tay				;
nedec12:jsr	GetDigitalString	; Month.
	bne	nedec28			;
	cmp	#'-'			;
	bne	nedec28			;
	lda	$ff			;
	beq	nedec28			;
	cmp	#12+1			; 1-12?
	bcs	nedec28			;
	sta	NewMonth		;
	iny				;
	jsr	GetDigitalString	; Date.
	bne	nedec28			;
	cmp	#'-'			;
	bne	nedec28			;
	lda	$ff			;
	beq	nedec28			;
	cmp	#31+1			;
	bcs	nedec28			;
	sta	NewDate			;
	iny				;
	jsr	GetDigitalString	; Year
	jsr	PassSpace		;
	bne	nedec28			;
	ldy	$ff			;
	txa				;
	bne	nedec16			;
	cpy	#99+1			; 80-99?
	bcs	nedec28			;
	tya				; (Cy)=0.
	sbc	#80-1			;
	bcc	nedec28			;
	bcs	nedec20		;(jmp)	;
nedec16:cpy	#>1980			; 1980-2099?
	sbc	#<1980			;
	bcc	nedec28			;
	txa				;
	cpy	#>2099+1		;
	sbc	#<2099+1		;
	bcs	nedec28			;
	tya				;
	sbc	#>1980-1		;
nedec20:ldy	NewMonth		;
	cpy	#8			; Month bit 3 to Cy.
	rol				;
	tax				;
	tya				; Cy=0
	ror				;
	ror				;
	ror				;
	ror				;
	ora	NewDate			; Date
	tay				;
	lda	#BiosSetSystemDate	;
	jsr	ROMBiosEntry		;
nedec24:clc				;
	rts				;
nedec28:jsr	DisplayCRLF		;
	lda	#StringInvalidDate	;
	jsr	DisplayString		;
	jmp	lpdec04			;

StrTimeHpSign	db	"Displays or sets the time.",$0d,$0a
		db	$0a
		db	"TIME [time]",$0d,$0a
		db	$0a
		db	"Type TIME only to show time and"
		db	"wait for a new one. Press ENTER"
		db	"to keep the same time.$"
SthTimeHpSign	db	"显示或设置时间",$0d,$0a
		db	"TIME [time]",$0d,$0a
		db	$0a
		db	"只输入TIME显示当前时间并等待输",$0d,$0a
		db	"入新时间，按回车则保持时间不变$"
;---------------------------------------;
;	(X|A) = (X|$ff)*(A)		;
;---------------------------------------;
MulXFF_A:				;
	sta	$fb			;
	stx	$fd			;
	ldx	#8			;
	lda	#0			; $fc|A
	sta	$fc			;
lpmax04:ror	$fb			;
	bcc	nemax04			;
	clc				;
	adc	$ff			;
	pha				;
	lda	$fc			;
	adc	$fd			;
	sta	$fc			;
	pla				;
nemax04:ror	$fc			;
	ror				;
	dex				;
	bne	lpmax04			;
	tax				;
	lda	$fb			;
	ror				;
	rts				;
;---------------------------------------;
CheckTimeEnd:				;
	stx	$fe			;
	tax				;
	jsr	CheckPhaseBreak		;
	beq	necte04			;
	ora	#$20			;
	cmp	#'a'			;
	beq	necte04			;
	cmp	#'p'			;
	beq	necte04			;
	cpx	$fe			;
	bne	necte04			;
	iny				;
	ldx	#0			;
necte04:rts				;
TimeAPm		equ	$fa		;
NewHour		equ	$fa		;
NewMinite	equ	$f9		;
NewSecond	equ	$f8		;
NewSecond50	equ	$f7		;
;=======================================;
TimeCommand:				;
	jsr	CheckSwitch		;
	beq	TimeCommandIN		;
	jmp	netec20			;
TimeCommandIN:				;
	lda	#StringCurrentTime	;
	jsr	DisplayString		;
	lda	#BiosGetDateTime	; Get system date and time.
	jsr	ROMBiosEntry		;
	txa				;
	pha				;
	sty	$ff			;
	txa				; Hour.
	ldx	#'a'			;
	lsr				;
	lsr				;
	lsr				;
	cmp	#12			;
	bcc	netec04			;
	sbc	#12			;
	ldx	#'p'			;
netec04:stx	TimeAPm			;
	tax				;
	bne	netec08			;
	lda	#12			;
netec08:jsr	DisplayTwo		;
	ldx	#':'			;
	jsr	DisplayChar		;
	pla				;
	sta	$fd			;
	lda	$ff			; Minite.
	asl				;
	rol	$fd			;
	asl				;
	rol	$fd			;
	asl				;
	rol	$fd			;
	lda	$fd			;
	and	#$3f			;
	jsr	DisplayTwo		;
	ldx	#':'			;
	jsr	DisplayChar		;
	lda	$fe			; 1/50 second.
	cmp	#50			;
	lda	$ff			; Second.
	and	#$1f			;
	rol				;
	jsr	DisplayTwo		;
	ldx	#'.'			;
	jsr	DisplayChar		;
	lda	$fe			;
	cmp	#50			;
	bcc	netec12			;
	sbc	#50			;
netec12:jsr	DisplayTwo		;
	ldx	TimeAPm			;
	jsr	DisplayChar		;
lptec04:jsr	DisplayCRLF		;
	lda	#StringEnterTime	;
	jsr	DisplayString		;
	lda	#14			;
	sta	CommandLineBeg		;
	lda	#DosGetKBString		;
	ldx	#<CommandLineBeg	;
	ldy	#>CommandLineBeg	;
	jsr	DosIOEntry		;
	bcc	netec14			;
	jsr	DisplayCtrlC		; Ctrl/C input.
	clc				;
	rts				;
netec14:jsr	DisplayCRLF		;
	ldy	#0			;
	jsr	PassSpace		;
	bne	netec20			;
netec16:clc				;
	rts				;
netec20:jsr	GetDigitalString	; Hour.
	bne	netec32			;
	ldx	#':'			;
	jsr	CheckTimeEnd		;
	bne	netec32			;
	lda	$ff			;
	sta	NewHour			;
	jsr	GetDigitalString	; Minite.
	bne	netec32			;
	ldx	#':'			;
	jsr	CheckTimeEnd		;
	bne	netec32			;
	lda	$ff			;
	cmp	#59+1			; 0-59
	bcs	netec32			;
	sta	NewMinite		;
	jsr	GetDigitalString	; Second.
	bne	netec32			;
	ldx	#'.'			;
	jsr	CheckTimeEnd		;
	bne	netec32			;
	lda	$ff			;
	cmp	#59+1			;
	bcs	netec32			;
	sta	NewSecond		;
	jsr	GetDigitalString	; 1/100 second.
	bne	netec32			;
	ldx	#$0d			;
	jsr	CheckTimeEnd		;
	bne	netec32			;
	tax				;
	lda	$ff			;
	cmp	#99+1			; 0-99
	bcs	netec32			;
	sta	NewSecond50		;
	lda	NewHour			;
	cpx	#'a'			;
	beq	netec24			;
	cpx	#'p'			;
	bne	netec28			;
	adc	#12-1			; (Cy)=1
netec24:iny				; Pass 'a' or 'p'.
netec28:cmp	#23+1			; 0-23?
	bcs	netec32			;
	asl	NewMinite		;
	asl	NewMinite		;
	asl	NewMinite		;
	rol				;
	asl	NewMinite		;
	rol				;
	asl	NewMinite		;
	rol				;
	tax				;
	jsr	PassSpace		;
	beq	netec36			;
netec32:jsr	DisplayCRLF		;
	lda	#StringInvalidTime	;
	jsr	DisplayString		;
	jmp	lptec04			;
netec36:lda	NewSecond		;
	lsr				;
	ora	NewMinite		;
	tay				;
	lda	NewSecond50		;
	bcc	netec40			;
	adc	#100-1			; (Cy)=1.
netec40:lsr				;
	sta	$ff			;
	lda	#BiosSetSystemTime	;
	jsr	ROMBiosEntry		;
	clc				;
	rts				;

CDOSSwitch	db	'U'		;
		dw	CDOSSwitchU-1	;
		db	'Z'		;
		dw	CDOSSwitchZOne-1;
		db	$ff		;
		db	'Z'		;
		dw	CDOSSwitchZ-1	;
		db	0		;
;---------------------------------------;
CDOSUninstall	equ	$fb		;
CDOSSwitchU:				;
	inc	CDOSUninstall		;
CDOSSwitchZOne:				;
	clc				;
	rts				;
;---------------------------------------;
CDOSSwitchZ:				;
	lda	CommandLine,y		;
	iny				;
	cmp	#'0'			;
	bcc	necsz04			;
	cmp	#'9'+1			;
	bcc	necsz08			;
necsz04:sec				;
necsz08:rts				;

;=======================================;
CDOSCommand:				;
	jsr	CheckSwitch		;
	beq	nebbd04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
nebbd04:ldy	#$80			; HANZI mode.
	lda	CDOSUninstall		;
	beq	nebbd08			;
	ldy	#0			; ASCII mode.
nebbd08:jmp	SetDisplayMode		; Set display mode.

StrCDHpSign	db	"Displays or changes directory",$0d,$0a
		db	$0a
		db	"CHDIR|CD [drive:][path]",$0d,$0a
		db	"CHDIR|CD[..]",$0d,$0a
		db	$0a
		db	"Type CD without directory will",$0d,$0a
		db	"display the current directory$"
SthCDHpSign	db	"显示当前目录名或改变当前目录",$0d,$0a
		db	"CHDIR|CD [盘:][路径]",$0d,$0a
		db	"CHDIR|CD[..]",$0d,$0a
		db	$0a
		db	"输入 CD 盘: 显示指定盘当前目录",$0d,$0a
		db	"只输入CD显示当前盘及当前目录$"
;---------------------------------------;
GetPagedChar:				;	
	lda	#BiosGetPagedChar	;
	ldx	$fd			;
	jmp	ROMBiosEntry		;
;---------------------------------------;
DisplayCurtDrive:			;
	lda	FileIOTmp+CURDRIVE	; Driver name.
DisplayDrive:				;
	ora	#'A'-1			;
	tax				;
	lda	#':'			;
	jmp	DisplayCharTwo		;
;=======================================;
CDCommand:				;
	jsr	CheckSwitch		;
	jsr	CheckDriveCmd		;
	stx	FileIOTmp+CURDRIVE	;
	jsr	CCheckPhaseBreakS	;
	beq	necdd04			;
	jsr	AccessPathName		;
	jsr	CheckSwitch		;
	beq	necdd08			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
necdd04:jsr	DisplayCurtPath		;
	lda	#StringInvdDrive	;
	bcs	necdd12			;
	jmp	DisplayCRLF		;
necdd08:lda	#FileChangeDir		;
	jsr	TempFileOperP		;
	bcc	necdd12			;
	lda	#StringInvdDir		;
necdd12:rts				;
;---------------------------------------; Display current directory.
DisplayCurtPath:			;
	lda	#FileGetDir		;
	jsr	TempFileOper		;
	bcs	necdd12			;
	jsr	DisplayCurtDrive	;
;---------------------------------------;
DisplayCurtDir:				;
	lda	FileIOTmp+HPAGE		;
	ldx	FileIOTmp+HBUF+1	; DTA.
	ldy	FileIOTmp+HBUF		;
;	jmp	DisplayDirString	;
;---------------------------------------;
;	Input	A|X|Y= String pointer.	;
;---------------------------------------;
DisplayDirString:			;
	sta	$fd			;
	stx	$ff			;
	sty	$fe			;
	ldy	#0			; In root directory?
	jsr	GetPagedChar		;
	tax				;
	bne	lpdds04			;
	ldx	#'\'			; Yes.
	jsr	DisplayChar		;
	clc				;
	rts				;
lpdds04:jsr	GetPagedChar		;
	tax				;
	beq	nedds04			;
	tya				;
	pha				;
	jsr	DisplayChar		;
	pla				;
	tay				;
	iny				;
	bne	lpdds04		;(jmp)	;
nedds04:clc				;
	rts				;

StrMDHpSign	db	"Create a directory",$0d,$0a
		db	$0a
		db	"MKDIR or",$0d,$0a
		db	"MD [drive:][path]Directory$"
SthMDHpSign	db	"建立一个目录",$0d,$0a
		db	$0a
		db	"MKDIR|MD [盘:][路径]目录名$"
;=======================================;
MDCommand:				;
	jsr	CheckSwitch		;
	beq	nemcd16			;
	jsr	CheckDriveCmd		;
	stx	FileIOTmp+CURDRIVE	;
	jsr	CCheckPhaseBreakS	;
	beq	nemcd08			;
	jsr	AccessPathName		;
	jsr	CheckSwitch		;
	beq	nemcd04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
nemcd04:lda	#FileMakeDir		;
	jsr	TempFileOperP		;
	bcc	nemcd12			;
nemcd08:lda	#StringErrMakeDir	;
	sec				;
nemcd12:rts				;
nemcd16:lda	#StringParaMiss		;
	sec				;
	rts				;

StrRDHpSign	db	"Removes (deletes) a directory",$0d,$0a
		db	$0a
		db	"RMDIR or",$0d,$0a
		db	"RD [drive:][path]Directory$"
SthRDHpSign	db	"删除一个目录",$0d,$0a
		db	$0a
		db	"RMDIR|RD [盘:][路径]目录名$"
;=======================================;
RDCommand:				;
	jsr	CheckSwitch		;
	beq	nemcd16			;
	jsr	CheckDriveCmd		;
	stx	FileIOTmp+CURDRIVE	;
	jsr	CCheckPhaseBreakS	;
	beq	nercd08			;
	jsr	AccessPathName		;
	jsr	CheckSwitch		;
	beq	nercd04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
nercd04:lda	#FileRemoveDir		;
	jsr	TempFileOperP		;
	bcc	nercd12			;
	cmp	#RemoveCurtDir		;
	beq	nercd16			;
nercd08:lda	#StringErrRemoveDir	;
	sec				;
nercd12:rts				;
nercd16:lda	#FileGetDir		;
	jsr	TempFileOper		;
	bcs	nercd12			;
	lda	#StringRDCurtDir	;
	jsr	DisplayString		;
	jsr	DisplayCurtDir		;
	jmp	DisplayCRLF		;

StrDirHpSign	db	"Displays  a list of  files and",$0d,$0a
		db	"subdirectories in a directory.",$0d,$0a
		db	$0a
		db	"DIR [drive:][path][filename]",$0d,$0a
		db	"  [/P][/W][/B][/A[:][attribs]]",$0d,$0a
		db	"  [/L][/S]    [/O[:][sortord]]",$0d,$0a
		db	$0a
		db	"/P Pauses after each screen",$0d,$0a
		db	"/W Uses wide list format",$0d,$0a
		db	"/B Use short format",$0d,$0a
		db	"/A Displays attributes files",$0d,$0a
		db	"attribs S System,R Read-only",$0d,$0a
		db	"        H Hidden,A Archive",$0d,$0a
		db	"        - <not>, D Directory",$0d,$0a
		db	"/L Use lowercase",$0d,$0a
		db	"/S include all subdirectories",$0d,$0a
		db	"/O List in sorted order",$0d,$0a
		db	"sortord N Name,  E Extension",$0d,$0a
		db	"        S Size,  D Date&Time",$0d,$0a
		db	"        - <not>, G Directory$"
SthDirHpSign	db	"显示目录文件及子目录",$0d,$0a
		db	"DIR [盘:][路径][文件名]",$0d,$0a
		db	"  [/P] [/W] [/B] [/A[:][属性]]",$0d,$0a
		db	"  [/L] [/S]      [/O[:][排序]]",$0d,$0a
		db	"/P 满一屏停下  /W 宽列方式显示",$0d,$0a
		db	"/B 简短格式    /L 小写字符显示",$0d,$0a
		db	"/S 显示目录和所有子目录的文件",$0d,$0a
		db	"/A 显示指定属性的文件目录",$0d,$0a
		db	"属性 S 系统文件 R 只读文件",$0d,$0a
		db	"     H 隐藏文件 A 档案文件",$0d,$0a
		db	"     D 目录     - 反属性前缀",$0d,$0a
		db	"/O 按指定排序方式显示目录",$0d,$0a
		db	"排序 N 文件名   E 文件扩展名",$0d,$0a
		db	"     S 文件大小 D 日期和时间",$0d,$0a
		db	"     G 目录优先 - 逆排序前缀$"
;---------------------------------------;
DirSwitch	db	'P'
		dw	DirSwitchP-1
		db	'W'
		dw	DirSwitchW-1
		db	'B'
		dw	DirSwitchB-1
		db	'L'
		dw	DirSwitchL-1
		db	'S'
		dw	DirSwitchS-1
		db	'A'
		dw	DirSwitchAOne-1
		db	'O'
		dw	DirSwitchO-1
		db	$ff
		db	'A'
		dw	DirSwitchA-1
		db	'O'
		dw	DirSwitchO-1
		db	0
;---------------------------------------;
DirFileOddEven	equ	$df		; db
FileHandleStack	equ	$ea		; dw
FileCountTotal	equ	$ec		; dw
SizeCountTotal	equ	$ee		; dd
DirSubDir	equ	$f2		;
DirPaged	equ	$f3		; Dir in screen paged.
DirCount	equ	$f4		;
DirSortSize	equ	$f5		;
DirBare		equ	$f6		;
DirWidthed	equ	$f7		;
DirAttribR	equ	$f8		;
DirAttribS	equ	$f9		;
DirBufferPoint	equ	$fa		;
;---------------------------------------;
DirSwitchP:				;
	lda	#30			; ASCII screen lines.
	bit	DisplayMode		;
	bpl	nedrs04			;
	lda	#14			; HANZI screen lines.
nedrs04:sta	DirPaged		;
nedrs08:rts				;
;---------------------------------------;
DirSwitchW:				;
	inc	DirWidthed		;
	rts				;
;---------------------------------------;
DirSwitchB:				;
	inc	DirBare			;
	rts				;
;---------------------------------------;
DirSwitchS:				;
	inc	DirSubDir		;
	rts				;
;---------------------------------------;
DirSwitchL:				;
	dec	DisplayLower		;
	rts				;
;---------------------------------------;
DirSwitchAOne:				;
	lda	#0			;
	sta	DirAttribS		;
	lda	#8			; V attrib reset.
	sta	DirAttribR		;
	rts				;
;---------------------------------------;
SwitchAChars	db	"ADxSHR"	;
SwitchACharsLen	equ	@-SwitchAChars	;
DirSwitchA:				;
	sty	$fd			;
	lda	#0			;
	sta	DirAttribS		;
	sta	DirAttribR		;
	lda	CommandLine,y		;
	cmp	#':'			;
	bne	lpdsa04			;
	iny				;
	jsr	CCheckPhaseBreakS	; ':' is the last character?
	beq	nedsa16			; Yes.
lpdsa04:lda	#0			;
lpdsa08:sta	$fc			;
	jsr	CCheckPhaseBreakS	;
	beq	nedsa12			;
	iny				;
	cmp	#'-'			;
	beq	lpdsa08			;
	and	#$df			;
	ldx	#SwitchACharsLen	;
lpdsa12:dex				;
	bmi	nedsa16			;
	cmp	SwitchAChars,x		;
	bne	lpdsa12			;
	lda	#$20*2			;
lpdsa16:lsr				;
	dex				;
	bpl	lpdsa16			;
	ldx	$fc			; '-' prefix?
	bne	nedsa08			;
	ora	DirAttribS		; No.
	sta	DirAttribS		;
	bne	lpdsa04		;(jmp)	;
nedsa08:ora	DirAttribR		; Yes.
	sta	DirAttribR		;
	bne	lpdsa04			;
nedsa12:lda	$fc			; '-' is the last character?
	bne	nedsa16			; Yes.
	clc				;
	rts				;
nedsa16:ldy	$fd			;
	sec				;
	rts				;
;---------------------------------------;
SwitchOChars	db	"NESDG"		;
SwitchOCharsLen	equ	@-SwitchOChars	;
;---------------------------------------;
DirSwitchO:				;
	sty	$fd			;
	jsr	CCheckPhaseBreakS	;
	bne	nedso04			; Only /O switch?
	lda	#0			; Yes. As: /ONE
	sta	CommandLine+0		;
	lda	#1			;
	sta	CommandLine+1		;
	ldx	#2			;
	bne	nedso16		;(jmp)	;
nedso04:cmp	#':'			;
	bne	nedso08			;
	iny				;
	lda	CommandLine,y		;
nedso08:ldx	#0			;
lpdso04:stx	$fe			;
	ldx	#0			;
	cmp	#'-'			;
	bne	nedso12			;
	iny				;
	jsr	CCheckPhaseBreakS	; Last is '-'?
	beq	nesdo20			; Yes.
	ldx	#$80			; Reverse sort flag.
nedso12:stx	$ff			;
	and	#$df			;
	ldx	#SwitchOCharsLen	;
lpdso08:dex				;
	bmi	nesdo20			;
	cmp	SwitchOChars,x		;
	bne	lpdso08			;
	txa				;
	ora	$ff			;
	ldx	$fe			;
	sta	CommandLine,x		;
	inx				;
	iny				;
	jsr	CCheckPhaseBreakS	;
	bne	lpdso04			;
nedso16:stx	DirSortSize		;
	clc				;
	rts				;
nesdo20:ldy	$fd			;
	sec				;
	rts				;
;---------------------------------------;
;	Input	($ff|fe)= Compare target;
;		($fd|fc)= Compare source;
;	Output	(Zy)= 1: Equal.		;
;		(Cy)= 0: <		;
;		(Cy)= 1: >=		;
;---------------------------------------;
DTACompBegin	db	DTA_Filename	; N
		db	DTA_Filename+8	; E
		db	DTA_FileSize+3	; S
		db	DTA_FileTime+3	; D
DTACompLength	db	8		; N
		db	3		; E
		db	4		; S
		db	4		; D
SortOrderComp:				;
	ldx	#0			;
lpsoc04:txa				;
	pha				;
	lda	CommandLine,x		;
	and	#$7f			;
	cmp	#4			; Sort order G?
	beq	nesoc08			;
	tax				;
	ldy	DTACompBegin,x		;
	lda	DTACompLength,x		;
	cpx	#1+1			;
	bcs	nesoc04			;
;---------------------------------------;
	tax				; N,E
lpsoc08:lda	($fc),y			;
	cmp	($fe),y			;
	bcc	nesoc16			;
	bne	nesoc16			;
	iny				;
	dex				;
	bne	lpsoc08			;
	beq	nesoc12		;(jmp)	;
;---------------------------------------;
nesoc04:tax				; S,D
lpsoc12:lda	($fc),y			;
	cmp	($fe),y			;
	bcc	nesoc16			;
	bne	nesoc16			;
	dey				;
	dex				;
	bne	lpsoc12			;
	beq	nesoc12		;(jmp)	;
;---------------------------------------;
nesoc08:ldy	DTA_FileAttrib		; G
	lda	($fc),y			;
	and	#$10			;
	sta	$de			;
	lda	($fe),y			;
	and	#$10			;
	cmp	$de			;
	bne	nesoc16			;
nesoc12:pla				;
	tax				;
	inx				;
	cpx	DirSortSize		;
	bne	lpsoc04			;
	rts				;
nesoc16:pla				;
	tax				;
	ror				; Cy->A'Bit7
	eor	CommandLine,x		; Reverse to change A'Bit7
	sec				;
	rol				; A'Bit7->Cy and !0->A.
	rts				;
;---------------------------------------;
DisplayPath:				;
	jsr	CheckDirPaged		;
	lda	#StringDirectory	; Display dir path.
	jsr	DisplayString		;
	jsr	DisplayPathIN		;
	jsr	DisplayCRLF		;
	jmp	CheckDirPaged		;
DisplayPathIN:				;
	jsr	DisplayCurtDrive	;
	jsr	IOTmpHBUFToFE		; DTA.
	ldy	#DTA_FilePath		;
	lda	($fe),y			; Page.
	pha				;
	ldy	#DTA_FilePath+2		;
	lda	($fe),y			; High.
	tax				;
	dey				;
	lda	($fe),y			; Low.
	tay				;
	pla				;
	jmp	DisplayDirString	;
;---------------------------------------;
CheckDirPaged:				;
	lda	DirPaged		;
	beq	necdp08			;
	inc	DirCount		;
	cmp	DirCount		;
	bne	necdp08			;
	lda	$ff			;
	pha				;
	lda	$fe			;
	pha				;
	lda	#StringPressKey		;
	jsr	DisplayString		;
	lda	#DosReadKeyNoEcho	; Read key.
	jsr	DosIOEntry		;
	tax				; Function key?
	bne	necdp04			;
	lda	#DosReadKeyNoEcho	; Yes.
	jsr	DosIOEntry		;
necdp04:jsr	DisplayCRLF		;
	lda	#StringDirContinue	;
	jsr	DisplayString		;
	jsr	DisplayPathIN		;
	ldx	#')'			;
	jsr	DisplayChar		;
	jsr	DisplayCRLF		;
	lda	#2			;
	sta	DirCount		;
	pla				;
	sta	$fe			;
	pla				;
	sta	$ff			;
necdp08:rts				;
;---------------------------------------;
DisplayCharCKLow:			;
	iny				;
	bit	DisplayLower		;
	bpl	nedcl04			;
	ora	#$20			; Lower case
nedcl04:jmp	DisplayCharA		;
;---------------------------------------;
DirItemFCPlus:				;
	lda	$fc			;
	clc				;
	adc	#DTA_DirSize		;
	sta	$fc			;
	bcc	nedip04			;
	inc	$fc+1			;
nedip04:rts				;
;---------------------------------------;
CheckDirPointEnd:			;
	lda	$fc+1			;
	cmp	DirBufferPoint+1	;
	bne	necde04			;
	lda	$fc			;
	cmp	DirBufferPoint		;
necde04:rts				;
;---------------------------------------;
CheckDirAttribs:			;
	jsr	IOTmpHBUFToFE		; DTA.
	ldy	#DTA_FileAttrib		;
	lda	DirAttribS		; Has attribute SET?
	beq	necda04			;
	and	($fe),y			; Yes. Attribute SET ok?
	beq	necda08			;
necda04:lda	DirAttribR		; Yes.
	and	($fe),y			; Attribute RESET ok?
	beq	necda12			; Yes.
necda08:sec				;
	rts				;
necda12:clc				;
	rts				;
;---------------------------------------;
AccsOneDirItem:				;
	inc	FileCount		; No.
	bne	neaoi04			;
	inc	FileCount+1		;
neaoi04:ldy	#DTA_DirSize-1		;
lpaoi04:lda	($fe),y			;
	sta	(DirBufferPoint),y	;
	dey				;
	bpl	lpaoi04			;
	lda	DirSortSize		; Sort order switch set?
	beq	DispOneDirItem		;
	lda	DirBufferPoint+1	; Yes. Move dir item to dir buffer.
	cmp	#<$fe00			; Dir buffer full?
	bcs	neaoi08			;
	lda	DirBufferPoint		; No.
	clc				;
	adc	#DTA_DirSize		;
	sta	DirBufferPoint		;
	bcc	neaoi08			;
	inc	DirBufferPoint+1	;
neaoi08:rts				;
;---------------------------------------;
;	Input	($ff|fe)= DTA item.	;
;---------------------------------------;
DispOneDirItem:				;
	lda	DirBare			; Display DIR with bare format?
	bne	neddi08			;
	lda	DirWidthed		; No.
	beq	neddi28			;
;---------------------------------------; Display with widthed format.
	lsr	DirFileOddEven		;
	bcc	neddi04			;
	lda	#BiosGetCursor		;
	jsr	ROMBiosEntry		;
	lda	#BiosSetCursor		; Even
	ldy	#15			;
	jsr	ROMBiosEntry		;
	jmp	neddi12			;
neddi04:inc	DirFileOddEven		;
neddi08:jsr	DisplayCRLF		;
	jsr	CheckDirPaged		;
neddi12:ldy	#DTA_FileAttrib		;
	lda	($fe),y			;
	and	#$10			; Dir item?
	php				;
	beq	neddi16			;
	ldx	#'['			; Yes.
	jsr	DisplayChar		;
neddi16:jsr	MoveTmpFNToPathX	;
	ldy	PathNameTmpEnd		;
lpddi04:lda	PathFileNameTmp,y	;
	beq	neddi20			;
	jsr	DisplayCharCKLow	;
	jmp	lpddi04		;(jmp)	;
neddi20:plp				;
	beq	neddi24			;
	ldx	#']'			;
	jsr	DisplayChar		;
neddi24:ldx	$ff			;
	ldy	$fe			;
	lda	#DTA_FileSize		; File size.
	jmp	CountPlused		;
;---------------------------------------; Display width normal format.
neddi28:jsr	DisplayCRLF		;
	jsr	CheckDirPaged		;
	ldy	#DTA_Filename		; Display filename
lpddi08:lda	($fe),y			;
	jsr	DisplayCharCKLow	;
	cpy	#DTA_Filename+8		;
	bne	lpddi08			;
	ldx	#$20			; Space.
	jsr	DisplayChar		;
lpddi12:lda	($fe),y			; Display fileext.
	jsr	DisplayCharCKLow	;
	cpy	#DTA_Filename+11	;
	bne	lpddi12			;
	ldy	#DTA_FileAttrib		; Is DIR attribute?
	lda	($fe),y			;
	and	#$10			;
	beq	neddi32			;
	lda	#StringDir		; Yes.
	jsr	DisplayString		;
	jmp	neddi36			;
neddi32:lda	#1			; 
	sta	TempSave		; Display 10 character limit.
	ldx	$fe+1			;
	ldy	$fe			;
	lda	#DTA_FileSize		; File size.
	jsr	DisplayDWord		;
neddi36:ldx	#$20			; Space.
	jsr	DisplayChar		;
	ldy	#DTA_FileDate+1		; Dates.
	lda	($fe),y			;
	lsr				;
	dey				;
	lda	($fe),y			;
	rol				;
	rol				;
	rol				;
	rol				;
	and	#15			; Month.
	jsr	DisplayTwo		;
	ldx	#'-'			;
	jsr	DisplayChar		;
	ldy	#DTA_FileDate		; Date.
	lda	($fe),y			;
	and	#31			;
	jsr	DisplayTwo		;
	ldx	#'-'			; Data low.
	jsr	DisplayChar		;
	ldy	#DTA_FileDate+1		; Year.
	lda	($fe),y			;
	lsr				;
	clc				;
	adc	#80			; Base on 1980.
	jsr	DisplayTwo		;
	ldx	#$08			;
	jmp	DisplayChar		;
;---------------------------------------;
;	Output	(Zy)=1: No file listed.	;
;---------------------------------------;
DirOneDirFiles:				;
	lda	#<ProgramAllEnd+$ff	; Dir buffer from paged align.
	sta	DirBufferPoint+1	;
	lda	#0			;
	sta	DirFileOddEven		;
	sta	DirBufferPoint		;
	sta	FileCount+1		;
	sta	FileCount		;
	sta	SizeCount+3		;
	sta	SizeCount+2		;
	sta	SizeCount+1		;
	sta	SizeCount		;
lpdof04:jsr	CheckDirAttribs		; Yes. This file's attribute match?
	bcs	nedof08			;
	lda	FileCount		; Yes. Is the first match file?
	ora	FileCount+1		;
	bne	nedof04			;
	jsr	DisplayPath		; Yes. Display the path.
nedof04:jsr	IOTmpHBUFToFE		;
	jsr	AccsOneDirItem		;
nedof08:lda	#FileNext		;
	jsr	TempFileOper		; Has file match?
	bcc	lpdof04			;
	lda	DirSortSize		; Sort order switch set?
	beq	nedof28			;
lpdof12:ldx	#<ProgramAllEnd+$ff	; Dir buffer from paged align.
	ldy	#0			;
	stx	$fc+1			;
	sty	$fc			; Search first valid item.
lpdof16:jsr	CheckDirPointEnd	; Search all?
	beq	nedof28			;
	ldy	#DTA_FileAttrib		; No. This dir item valid?
	lda	($fc),y			;
	bpl	nedof16			;
	jsr	DirItemFCPlus		; No. Next dir item?
	jmp	lpdof16			;
lpdof20:jsr	CheckDirPointEnd	; Compare all?
	beq	nedof24			;
	ldy	#DTA_FileAttrib		; No. This dir item valid?
	lda	($fc),y			;
	bmi	nedof20			;
	jsr	SortOrderComp		; No. Compare one. Is dir item small order?
	beq	nedof20			;
	bcs	nedof20			;
nedof16:ldx	$fc+1			; Yes. Save this dir item as compare base.
	ldy	$fc			;
	stx	$fe+1			;
	sty	$fe			;
nedof20:jsr	DirItemFCPlus		;
	jmp	lpdof20			;
nedof24:jsr	DispOneDirItem		;
	ldy	#DTA_FileAttrib		; Set this dir item invalid.
	lda	#$80			;
	sta	($fe),y			;
	jmp	lpdof12			;
nedof28:lda	FileCount+1		;
	ora	FileCount		;
	beq	nedof40			;
	clc				;
	lda	FileCountTotal		;
	adc	FileCount		;
	sta	FileCountTotal		;
	lda	FileCountTotal+1	;
	adc	FileCount+1		;
	sta	FileCountTotal+1	;
	ldy	#4			;
	ldx	#0			;
lpdof24:lda	SizeCountTotal,x	;
	adc	SizeCount,x		;
	sta	SizeCountTotal,x	;
	inx				;
	dey				;
	bne	lpdof24			;
	lda	DirBare			;
	bne	nedof40			;
	jsr	DisplayCRLF		;
	jsr	CheckDirPaged		;
	jsr	DisplayFileCount	;
	ldx	#<SizeCount		; Bytes.
	ldy	#>SizeCount		;
	lda	#0			;
	sta	TempSave		;
	jsr	DisplayDWord		;
	lda	#StringBytes		;
	jsr	DisplayStringCRLF	;
	jsr	CheckDirPaged		;
	lda	#1			;
nedof40:rts				;
;---------------------------------------;
;    Display match files and subdir's.	;
;---------------------------------------;
DirOneSubDir:				;
	lda	#$17			; Attribute.
	jsr	TempFileOperFP		; Has file match?
	bcs	nedod04			;
	jsr	DirOneDirFiles		; Yes. Display all match files.
	beq	nedod04			; Has file list?
	jsr	DisplayCRLF		; Yes.
nedod04:ldx	PathNameTmpEnd		; Search *.* in current dir
	lda	#'*'			; (Search DIR item).
	sta	PathFileNameTmp,x	;
	sta	PathFileNameTmp+2,x	;
	lda	#'.'			;
	sta	PathFileNameTmp+1,x	;
	lda	#0			;
	sta	PathFileNameTmp+3,x	;
	lda	#$17			;
	jsr	TempFileOperFP		; Search subdir itemfiles.
lpdod04:jsr	IOTmpHBUFToFE		; DTA.
	ldy	#DTA_FileAttrib		;
	lda	($fe),y			;
	and	#$10			; <DIR> item?
	beq	nedod16			;
	ldy	#DTA_Filename		;
	lda	($fe),y			;
	cmp	#'.'			; Is <DIR> leading?
	beq	nedod16			;
;---------------------------------------; Find a subdir item.
	ldy	#HandleSize		; Save PathNameTmpEnd
	lda	PathNameTmpEnd		;
	sta	(FileHandleStack),y	;
	dey				; Save FileIOTmp.
lpdod08:lda	FileIOTmp,y		;
	sta	(FileHandleStack),y	;
	dey				;
	bpl	lpdod08			;
	lda	FileHandleStack		; Stack plused.
	clc				;
	adc	#HandleSize+1		;
	sta	FileHandleStack		;
	bcc	nedod08			;
	inc	FileHandleStack+1	;
nedod08:jsr	MoveTmpFNToPath		; No. is DIR item. Save DIR name to path.
	lda	#'\'			;
	sta	PathFileNameTmp,x	; Save DIR ending.
	inx				;
	stx	PathNameTmpEnd		;
	ldy	#0			; Move searching filename to path.
lpdod12:lda	PathFileNameSrc,y	;
	sta	PathFileNameTmp,x	;
	inx				;
	iny				;
	cmp	#0			;
	bne	lpdod12			;
	jsr	DirOneSubDir		; --------
	lda	FileHandleStack		;
	sec				;
	sbc	#HandleSize+1		;
	sta	FileHandleStack		;
	bcs	nedod12			;
	dec	FileHandleStack+1	;
nedod12:ldy	#HandleSize		; Restore PathNameTmpEnd
	lda	(FileHandleStack),y	;
	sta	PathNameTmpEnd		;
	dey				; Restore FileIOTmp.
lpdod16:lda	(FileHandleStack),y	;
	sta	FileIOTmp,y		;
	dey				;
	bpl	lpdod16			;
nedod16:lda	#FileNext		;
	jsr	TempFileOper		;
	bcc	lpdod04			;
	rts				;
;=======================================;
DirCommand:				;
	lda	#$0e			; Is V/S/H attribute?
	sta	DirAttribR		; Yes.
	jsr	CheckSwitch		;
	jsr	CheckDriveCmd		;
	sta	FileIOTmp+CURDRIVE	;
	lda	#$c1			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	nedrc08			;
	jsr	CheckSwitch		;
	beq	nedrc04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
;---------------------------------------;
nedrc04:lda	PathNameEnding		;
	sta	PathNameTmpEnd		;
	jsr	PathNameTmpToTar	; Save PathFilenameTmp
	jsr	DisplayVolumn		;
	bcs	nedrc08			;
	jsr	PathNameTarToTmp	; Restore PathFilenameTmp
	inc	DirCount		;
	lda	#$17			; Attribute.
	jsr	TempFileOperFP		; Check path valid.
	bcc	nedrc12			;
	cmp	#SearchEnding		;
	beq	nedrc12			;
nedrc08:sec				; Path not found(Display it).
	rts				;
nedrc12:ldx	#<ProgramAllEnd+$ff	; Dir buffer from paged align.
	stx	DirBufferPoint+1	;
	ldx	#0			;
	stx	DirBufferPoint		;
	ldx	DirSubDir		; /S switch?
	bne	nedrc20			;
;---------------------------------------;
	tax				; No. DIR only one directory
	bne	nedrc16			; Has file match?
	jsr	DirOneDirFiles		; Yes. Has file list?
	bne	nedrc24			;
nedrc16:jsr	DisplayPath		; No. Display path.
	jmp	nedrc28			;
;---------------------------------------; DIR with /S switch.
nedrc20:ldx	PathNameTmpEnd		; Save filename.
	ldy	#0			;
lpdrc04:lda	PathFileNameTmp,x	;
	sta	PathFileNameSrc,y	;
	inx				;
	iny				;
	cmp	#0			;
	bne	lpdrc04			;
	ldx	#<CommandDataEnd	;
	ldy	#>CommandDataEnd	;
	stx	FileHandleStack+1	;
	sty	FileHandleStack		;
	jsr	DirOneSubDir		; --------
	lda	FileCountTotal+1	;
	ora	FileCountTotal		;
	beq	nedrc28			;
	lda	DirBare			;
	bne	nedrc32			;
	jsr	CheckDirPaged		;
	lda	#StringTotalFile	;
	jsr	DisplayStringCRLF	;
	jsr	CheckDirPaged		;
	ldx	FileCountTotal+1	;
	ldy	FileCountTotal		;
	jsr	DisplayFileCountIN	;
	ldx	#<SizeCountTotal	; Bytes.
	ldy	#>SizeCountTotal	;
	lda	#0			;
	sta	TempSave		;
	jsr	DisplayDWord		;
	lda	#StringBytes		;
	jsr	DisplayStringCRLF	;
nedrc24:lda	DirBare			;
	bne	nedrc32			;
	jsr	CheckDirPaged		;
	ldx	#9			; Disk free size.
	jsr	DisplayChar		;
	jsr	GetDiskFreeSize		;
	ldx	#<FileIOTmp+CURP	;
	ldy	#>FileIOTmp+CURP	;
	lda	#0			;
	sta	TempSave		;
	jsr	DisplayDWord		;
	lda	#StringFree		;
	jmp	DisplayStringCRLF	;
nedrc28:lda	#StringNoFile		;
	sec				;
	rts				;
nedrc32:jmp	DisplayCRLF		;

StrEchoHpSign	db	"Displays messages, or turns",$0d,$0a
		db	"command-echoing on or off.",$0d,$0a
		db	$0a
		db	"ECHO [ON | OFF]",$0d,$0a
		db	"ECHO [message]$"
SthEchoHpSign	db	"显示信息或打开/关闭命令行回显",$0d,$0a
		db	$0a
		db	"ECHO [ON | OFF]",$0d,$0a
		db	"ECHO [信息]$"
;=======================================;
EchoCommand:				;
	jsr	CheckSwitch		;
	jsr	PassSpace		;
	bne	neecd08			;
	lda	#StringEcho		; Only ECHO command
	jsr	DisplayString		;
	lda	#StringOn		;
	bit	EchoFlagCommand		;
	bpl	neecd04			;
	lda	#StringOff		;
neecd04:jmp	DisplayStringCRLF	;
neecd08:sty	$ff			; is ECHO ON/OFF
	lda	CommandLine,y		;
	iny				;
	and	#$df			;
	cmp	#'O'			;
	bne	neecd24			;
	lda	CommandLine,y		;
	iny				;
	and	#$df			;
	cmp	#'N'			;
	beq	neecd12			;
	cmp	#'F'			;
	bne	neecd24			;
	lda	CommandLine,y		;
	iny				;
	and	#$df			;
	cmp	#'F'			;
	bne	neecd24			;
neecd12:tax				;
	jsr	PassSpace		;
	bne	neecd24			;
	ldy	#0			; Yes.
	cpx	#'N'			;
	beq	neecd16			;
	dey				;
neecd16:sty	EchoFlagBAT		;
	lda	RunBATEnter		; Current in BAT running?
	bne	neecd20			;
	sty	EchoFlagCommand		; No.
neecd20:clc				;
	rts				;
;---------------------------------------;
neecd24:ldy	$ff			;
lpecd04:ldx	CommandLine,y		;
	cpx	#$0d			;
	beq	neecd20			;
	jsr	DisplayChar		;
	iny				;
	bne	lpecd04		;(jmp)	;

StrCopyHpSign	db	"Copy file(s) to other location",$0d,$0a
		db	$0a
		db	"COPY [drive:][path]source",$0d,$0a
		db	"   [+[drive:][path]source]",$0d,$0a
		db	"     ...",$0d,$0a
		db	"   [drive:][path][destination]",$0d,$0a
		db	"   [/Y]",$0d,$0a	;[/A][/V]
		db	$0a
;		db	"/A Indicate an ASCII text file",$0d,$0a
;		db	"/V Verifies that new files OK",$0d,$0a
		db	"/Y No confirm to overwrite all$"
SthCopyHpSign	db	"复制一个或多个文件到新的位置",$0d,$0a
		db	"COPY [盘:][路径]源文件",$0d,$0a
		db	"   [+[盘:][路径]源文件]",$0d,$0a
		db	"     ...",$0d,$0a
		db	"     [盘:][路径][目的文件][/Y]",$0d,$0a
		db	$0a			;[/A] [/V] 
;		db	"/A 指定为ASCII文本文件",$0d,$0a
;		db	"/V 校验新写的文件是否正确",$0d,$0a
		db	"/Y 文件已存在时不用再确认$"
FAttribute	equ	$f0		; ds 1
CopyAnyFileFlag	equ	$f1		; ds 1
FileNameCmpCnt	equ	$f2		; ds 1
TargetNameMark	equ	$f3		; ds 1
SourceNameMark	equ	$f4		; ds 1
CopyASCIIFlag	equ	$f5		; ds 1
CopyVerifyFlag	equ	$f6		; ds 1
AskYNAType	equ	$f7		; ds 1
SourceLenLast	equ	$f8		; ds 2
SourcePageLast	equ	$fa		; ds 1
WriteAll	equ	$fb		; ds 1
;---------------------------------------;
CopySwitch	db	'A'		;
		dw	CopySwitchA-1	;
		db	'V'		;
		dw	CopySwitchV-1	;
		db	'Y'		;
		dw	CopySwitchY-1	;
		db	0		;
;---------------------------------------;
CopySwitchA:				;
	inc	CopyASCIIFlag		;
	rts				;
;---------------------------------------;
CopySwitchV:				;
	inc	CopyVerifyFlag		;
	rts				;
;---------------------------------------;
CopySwitchY:				;
	inc	WriteAll		;
	rts				;
;---------------------------------------;
;	Input	(Y)=Tar point.		;
;---------------------------------------;
MoveTmpSrcTarSeg:			;
lpmss04:ldx	MP			;
	lda	PathFileNameTmp,x	;
	beq	nemss16			;
	inc	MP			;
	cmp	SegmentEnd		;
	beq	nemss16			;
	lda	PathFileNameSrc,y	;
	beq	nemss10			;
	cmp	#'*'			;
	beq	nemss04			;
	iny				;
	cmp	SegmentEnd		;
	beq	nemss10			;
	cmp	#'?'			;
	bne	nemss08			;
nemss04:lda	PathFileNameTmp,x	; Target is '*' or '?'.
nemss08:ldx	TP			;
	sta	PathFileNameTar,x	;
	inc	TP			;
	bne	lpmss04		;(jmp)	;
nemss10:ldx	TP			;
	sta	PathFileNameTar,x	;
	inc	TP			;
	ldx	MP			;
lpmss08:lda	PathFileNameTmp,x	; Target reach '.' or 0.
	beq	nemss12			;
	inx				;
	cmp	SegmentEnd		;
	bne	lpmss08			;
nemss12:stx	MP			;
	rts				;
nemss16:ldx	TP			; Source reach '.' or 0.
lpmss12:lda	PathFileNameSrc,y	;
	beq	nemss32			;
	iny				;
	cmp	#'?'			;
	beq	lpmss12			;
	cmp	#'*'			;
	beq	lpmss16			;
	cmp	SegmentEnd		;
	beq	nemss24			;
	sta	PathFileNameTar,x	;
	inx				;
	bne	lpmss12		;(jmp)	;
lpmss16:lda	PathFileNameSrc,y	;
	beq	nemss32			;
	iny				;
	cmp	SegmentEnd		;
	bne	lpmss16			;
nemss24:sta	PathFileNameTar,x	;
	inx				;
nemss32:stx	TP			;
	rts				;
;---------------------------------------;
;   Move Tmp to Tar by Src.		;
;		Tmp: Real SF.		;
;		Src: Wild TF.		;
;		Tar: Real TF.		;
;---------------------------------------;
MoveTmpSrcTar:				;
	lda	PathNameTmpEnd		;
	sta	MP			;
	lda	PathNameTarEnd		;
	sta	TP			;
	lda	#'.'			;
	sta	SegmentEnd		;
	ldy	PathNameSrcEnd		;
	jsr	MoveTmpSrcTarSeg	; Move file name
	lda	#0			;
	sta	SegmentEnd		;
	jsr	MoveTmpSrcTarSeg	; Move file extent.
	ldx	TP			;
	lda	#0			;
	sta	PathFileNameTar,x	;
	rts				;
;---------------------------------------;
; Comp PathFileNameTmp & PathFileNameTar;
;	Output	(Zy)=1: Same.		;
;---------------------------------------;
CopyCompMFTF:				;
	ldx	#0			;
	ldy	#8			;
lpcmt04:lda	PathFileNameTmp,x	; Filename same?
	cmp	PathFileNameTar,x	;
	beq	necmt08			;
	ldy	PathFileNameTar,x	; No. Is x.-x or x-x.?
	cmp	#0			;
	beq	necmt04			;
	cmp	#'.'			;
	bne	necmt20			; No.
	tya				;
	bne	necmt20			; Yes.
	lda	PathFileNameTmp+1,x	;
	beq	necmt16			;
	rts				;
necmt04:cpy	#'.'			;
	bne	necmt20			; Yes.
	lda	PathFileNameTar+1,x	;
	beq	necmt16			;
	rts				;
necmt08:cmp	#0			;
	beq	necmt16			;
	cmp	#'.'			;
	beq	necmt10			;
	inx				;
	dey				;
	bne	lpcmt04			;
	txa				;
	tay				;
lpcmt08:lda	PathFileNameTmp,x	;
	beq	lpcmt12			;
	inx				;
	cmp	#'.'			;
	bne	lpcmt08			;
lpcmt12:lda	PathFileNameTar,y	;
	beq	necmt12			;
	iny				;
	cmp	#'.'			;
	bne	lpcmt12			;
	bne	necmt12		;(jmp)	;
necmt10:txa				;
	tay				;
necmt12:lda	#3			;
	sta	FileNameCmpCnt		;
lpcmt16:lda	PathFileNameTmp,x	;
	cmp	PathFileNameTar,y	;
	bne	necmt20			;
	cmp	#0			;
	beq	necmt16			;
	inx				;
	iny				;
	dec	FileNameCmpCnt		;
	bne	lpcmt16			;
necmt16:lda	FileIOSrc+CURDRIVE	; Yes. Drive same also?
	cmp	FileIOTar+CURDRIVE	;
necmt20:rts				;
;---------------------------------------;
DisplayDFNTmpCRLF:			;
	jsr	DisplayDFNTmp		;
	jmp	DisplayCRLF		;
DisplayDFNTmp:				;
	ldx	#<PathFileNameTmp	; Yes. Error
	ldy	#>PathFileNameTmp	;
	lda	SourceDrive		;
DisplayDFN:				;
	stx	$fe+1			;
	sty	$fe			;
DisplayDFNIN:				;
	beq	neddf04			;
	jsr	DisplayDrive		;
neddf04:ldy	#0			;
lpddf04:lda	($fe),y			;
	beq	neddf08			;
	sty	FileNameCmpCnt		;
	jsr	DisplayCharA		;
	ldy	FileNameCmpCnt		;
	iny				;
	bne	lpddf04		;(jmp)	;
neddf08:rts				;
;---------------------------------------;
DisplayDFNCRLF:				;
	stx	$fe+1			;
	sty	$fe			;
	pha				;
	ldx	#'-'			;
	lda	#' '			;
	jsr	DisplayCharTwo		;
	pla				;
	jsr	DisplayDFNIN		;
	jmp	DisplayCRLF		;
;---------------------------------------;
ReadSourceFile:				;
	ldx	FreePage		; Read source to pages from FreePage
	stx	FileIOSrc+HPAGE		;
lprsf04:ldx	#<$8000			; Page beginning address
	ldy	#>$8000			;
	stx	FileIOSrc+HBUF+1	;
	sty	FileIOSrc+HBUF		;
	ldx	#<$4000			; Page length
	ldy	#>$4000			;
	stx	FileIOSrc+BYTES+1	;
	sty	FileIOSrc+BYTES		;
	lda	#FileRead		;
	jsr	SourceFileOper		;
	bcs	nersf12			;
	lda	CopyASCIIFlag		;
	beq	nersf08			;
;---------------------------------------;
comment	%
	ldx	FileIOSrc+BYTES+1	;
	ldy	FileIOSrc+BYTES		;
???
lprsf08:lda	FileIOSrc+HPAGE		;
	ldx	FileIOSrc+HBUF+1	;
	ldy	FileIOSrc+HBUF		;
	sta	$80			;
	stx	$82			;
	sty	$81			;
	ldx	#<TypeBuffer		;
	ldy	#>TypeBuffer		;
	stx	$85			;
	sty	$84			;
	stx	$ff			;
	sty	$fe			;
	ldx	#<256			;
	ldy	#>256			;
	stx	$87			;
	sty	$86			;
	lda	#BiosMoveBlock		;
	ldx	#$80			;
	jsr	ROMBiosEntry		;
	ldy	#0			;
lprsf12:lda	($fe),y			;
	cmp	#$1a			;
	beq	nersf16			;
	iny				;
	cpy	
	bne	lprsf12			;
	%
nersf08:lda	FileIOSrc+BYTES+1	;
	cmp	#<$4000			; Read page full?
	bne	nersf20			;
	inc	FileIOSrc+HPAGE		; Yes.
	lda	FileIOSrc+HPAGE		;
	cmp	#$fe			; page full?
	bne	lprsf04			;
	sta	SourcePageLast		; Yes.
	clc				;
nersf12:rts				;
nersf16:
nersf20:lda	FileIOSrc+HPAGE		;
	sta	SourcePageLast		;
	ldx	FileIOSrc+BYTES+1	;
	ldy	FileIOSrc+BYTES		;
	stx	SourceLenLast+1		;
	sty	SourceLenLast		;
	rts				;
;---------------------------------------;
CopyVerify:				;
	clc				;
	lda	CopyVerifyFlag		;
	beq	necvy08			;
	sec				;
	lda	FileIOTar+CURP		;
	sbc	FileIOTar+BYTES		;
	sta	FileIOTar+CURP		;
	lda	FileIOTar+CURP+1	;
	sbc	FileIOTar+BYTES+1	;
	sta	FileIOTar+CURP+1	;
	lda	FileIOTar+CURP+2	;
	sbc	#0			;
	sta	FileIOTar+CURP+2	;
	bcs	necvy04			;
	dec	FileIOTar+CURP+3	;
necvy04:ldx	#<$8000			;
	ldy	#>$8000			;
	stx	FileIOTar+HBUF+1	;
	sty	FileIOTar+HBUF		;
	lda	#FileRead		;
	jsr	TargetFileOper		;
necvy08:rts				;	
;---------------------------------------;
WriteTargetFile:			;
	ldx	FreePage		;
	stx	FileIOTar+HPAGE		;
lpwsf04:ldx	#<$8000			;
	ldy	#>$8000			;
	stx	FileIOTar+HBUF+1	;
	sty	FileIOTar+HBUF		;
	ldx	#<$4000			;
	ldy	#>$4000			;
	lda	FileIOTar+HPAGE		;
	cmp	SourcePageLast		;
	bne	newsf04			;
	ldx	SourceLenLast+1		;
	ldy	SourceLenLast		;
newsf04:stx	FileIOTar+BYTES+1	;
	sty	FileIOTar+BYTES		;
	lda	#FileWrite		;
	jsr	TargetFileOper		;
	bcs	newsf20			;
	ldx	FileIOTar+BYTES+1	;
	ldy	FileIOTar+BYTES		;
	lda	FileIOTar+HPAGE		;
	cmp	SourcePageLast		;
	bne	newsf12			;
	cpx	SourceLenLast+1		;
	bne	newsf16			;
	cpy	SourceLenLast		;
	bne	newsf16			;
	jmp	CopyVerify		;
newsf12:cpx	#<$4000			;
	bne	newsf16			;
	jsr	CopyVerify		;
	bcs	newsf20			;
	inc	FileIOTar+HPAGE		;
	lda	FileIOTar+HPAGE		;
	cmp	#$fe			;
	bne	lpwsf04			;
	clc				;
	rts				;
newsf16:lda	#StringNoDiskSp		;
	jsr	DisplayStringCRLF	;
newsf20:sec				;
	rts				;
;---------------------------------------;
;	Output	Zy==1: Over write OK.	;
;---------------------------------------;
AskOverWrite:				;
	lda	#StringOvWr		;
	jsr	DisplayString		;
	ldx	#<PathFileNameTar	;
	ldy	#>PathFileNameTar	;
	lda	TargetDrive		;
	jsr	DisplayDFN		;
	lda	#StringYNA		;
	jsr	DisplayString		;
	lda	#0			;
	sta	AskYNAType		;
lpaow04:lda	#DosReadKeyNoEcho	; Read key.
	jsr	DosIOEntry		;
	cmp	#$0d			; Accept?
	beq	neaow08			;
	tax				;
	and	#$df			; No. is Y/N/A?
	cmp	#'Y'			;
	beq	neaow04			;
	cmp	#'N'			;
	beq	neaow04			;
	cmp	#'A'			;
	bne	lpaow04			;
neaow04:sta	AskYNAType		;
	lda	#$08			; Yes.
	jsr	DisplayCharTwo		;
	jmp	lpaow04			;
neaow08:lda	AskYNAType		; Has key in Y/N/A?
	beq	lpaow04			;
	jsr	DisplayCRLF		;
	lda	AskYNAType		;
	cmp	#'Y'			;
	beq	neaow12			;
	cmp	#'A'			;
	bne	neaow12			;
	ldx	#1			;
	stx	WriteAll		;
	dex				; Zy=1.
neaow12:rts				; Return Zy.
;---------------------------------------;
CloseTarFile:				;
	lda	FAttribute		;
	sta	FileIOTar+HPAGE		;
	ldx	FDate+1			;
	ldy	FDate			;
	stx	FileIOTar+HBUF+1	;
	sty	FileIOTar+HBUF		;
	ldx	FTime+1			;
	ldy	FTime			;
	stx	FileIOTar+BYTES+1	;
	sty	FileIOTar+BYTES		;
	lda	#FileAttribute		;
	jsr	TargetFileOper		;
	bcs	neofe04			;
	lda	#FileClose		;
	jmp	TargetFileOper		;
;---------------------------------------;
OpenFile:				;
	lda	#FilePathOpen		;
	jsr	SourceFileOperTmp	;
	bcs	neofe04			;
	lda	FileIOSrc+HPAGE		;
	sta	FAttribute		;
	lda	FileIOSrc+HBUF		;
	sta	FDate			;
	lda	FileIOSrc+HBUF+1	;
	sta	FDate+1			;
	lda	FileIOSrc+BYTES		;
	sta	FTime			;
	lda	FileIOSrc+BYTES+1	;
	sta	FTime+1			;
neofe04:rts				;
;---------------------------------------;
DisplayNoCopyFile:			;
	lda	#StringNoFile		;
	jsr	DisplayString		;
	ldx	#<PathFileNameTmp	;
	ldy	#>PathFileNameTmp	;
	lda	SourceDrive		;
	jsr	DisplayDFNCRLF		;
	jmp	DisplayCopyFiles	;
;---------------------------------------;
DisplayCopyAccDeny:			;
	lda	#StringAccDeny		;
	jsr	DisplayString		;
	ldx	#<PathFileNameTar	;
	ldy	#>PathFileNameTar	;
	lda	TargetDrive		;
	jsr	DisplayDFNCRLF		;
	jmp	DisplayCopyFiles	;
;---------------------------------------;
;	Output	(Zy)==1: Is '+'.	;
;---------------------------------------;
CheckCopyPlus:				;
	lda	CommandLine,y		; Is '+' copy?
	cmp	#'+'			;
	bne	neccp04			;
lpccp04:iny				;
	jsr	PassSpace		;
	lda	CommandLine,y		;
	cmp	#'+'			;
	beq	lpccp04			;
	lda	#0			;
neccp04:rts				;
;---------------------------------------;
;	Output	(Cy)=0: Target ok.	;
;		    =1: Target error.	;
;		 (Zy)=0: Target denies.	;
;		 (Zy)=1: Target error.	;
;---------------------------------------;
CheckCopyTarget:			;
	lda	#FilePathOpen		; Open target file.
	jsr	TargetFileOperP		;
	bcc	necct04			;
	cmp	#FileNotExist		; Target exist?
	beq	necct12			;
	bne	necct16		;(jmp)	;
necct04:lda	#FileClose		; Yes
	jsr	TargetFileOper		;
	bcs	necct16			;
	lda	WriteAll		; All over write?
	bne	necct08			; Yes.
	jsr	AskOverWrite		; This file is exist.
	bne	necct16			;
necct08:lda	FileIOTar+HPAGE		;
	and	#$1f			; Is this file D/V/H/S/R?
	bne	necct20			;
necct12:lda	#FilePathCreate		; Create target file.
	jsr	TargetFileOperP		;
	bcc	necct24			;
necct16:lda	#0			;
necct20:sec				;
necct24:rts				;
;---------------------------------------;
;  Copy files to target.		;
;	Output	(Cy)=1: Error.		;
;		(Cy)=0: Copy ending.	;
;---------------------------------------;
CopySrcsToTar:				;
lpcst04:jsr	MoveTmpFNToPath		; Move filename to PathFileNameTmp.
	jsr	DisplayDFNTmpCRLF	;
	lda	#FilePathOpen		;
	jsr	SourceFileOperTmp	;
	bcs	necst16			;
	jsr	CopyCompMFTF		;
	bne	lpcst08			;
	lda	#StringDataLost		;
	jsr	DisplayStringCRLF	;
	jmp	necst08			;
lpcst08:jsr	ReadSourceFile		;
	bcs	necst12			;
CopySrcsToTarIN:			;
	jsr	WriteTargetFile		;
	bcs	necst12			;
	lda	FileIOSrc+BYTES+1	;
	cmp	#<$4000			;
	beq	lpcst08			;
	lda	#FileClose		;
	jsr	SourceFileOper		;
necst08:lda	#FileNext		;
	jsr	TempFileOper		;
	bcc	lpcst04			;
	cmp	#SearchEnding		;
	bne	necst16			;
	clc				;
	rts				;
necst12:lda	#FileClose		;
	jsr	SourceFileOper		;
necst16:sec				;
	rts				;
;=======================================;
CopyCommand:				; COPY command
	jsr	CheckSwitch		;
	bne	necyf04			;
	lda	#StringParaMiss		;
	sec				;
	rts				;
necyf04:jsr	CheckDriveCmd		; Analyze source filename
	sta	SourceDrive		;
	stx	FileIOSrc+CURDRIVE	;
	sta	CommandDataEnd+CURDRIVE	;
	lda	#$80			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	necyf12			;
	jsr	CheckNameExtMark	; Source filename has '*' or '?'
	bcc	necyf08			;
	inc	SourceNameMark		;
necyf08:ldx	#>-1			; Move Tmp PathFileName to Src.
lpcyf04:inx				;
	lda	PathFileNameTmp,x	;
	sta	CommandDataEnd+HandleSize,x
	bne	lpcyf04			;
	lda	PathNameEnding		;
	sta	PathNameTmpEnd		;
	jsr	CheckSwitch		;
	jsr	CheckCopyPlus		;
	bne	necyf20			;
;---------------------------------------; Copy files with '+'.
	lda	#<CommandDataEnd+HandleSize+40
	sta	$fc+1			;
	lda	#>CommandDataEnd+HandleSize+40
	sta	$fc			;
	inc	SizeCount		;
	inc	SourceNameMark		;
lpcyf08:inc	SizeCount		; '+' copy flag and counter.
	jsr	CheckDriveCmd		; Analyze source filename
	sty	$ff			;
	ldy	#CURDRIVE		;
	sta	($fc),y			; (X)= Drive
	ldy	$ff			;
	lda	#$80			;
	jsr	AccessPathFileName	;
necyf12:bcs	necyf32			;
	sty	$ff			; Save (Y).
	ldy	#HandleSize-1		; Move Tmp PathFileName to Src.
lpcyf12:iny				;
	lda	PathFileNameTmp-HandleSize,y
	sta	($fc),y			;
	bne	lpcyf12			;
	lda	$fc			;
	clc				;
	adc	#HandleSize+40		;
	bcc	necyf16			;
	inc	$fc+1			;
necyf16:sta	$fc			;
	ldy	$ff			;
	jsr	CheckSwitch		;
	beq	necyf20			;
	jsr	CheckCopyPlus		;
	beq	lpcyf08			;
;---------------------------------------;
necyf20:jsr	CheckDriveCmd		; Analyze target filename
	sta	TargetDrive		;
	stx	FileIOTar+CURDRIVE	;
	lda	#$81			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	necyf32			;
	jsr	CheckNameExtMark	; Target filename has '*' or '?'
	bcc	necyf24			;
	inc	TargetNameMark		;
necyf24:ldx	#>-1			;
lpcyf16:inx				;
	lda	PathFileNameTmp,x	; Move Tmp PathFileName to Src & Tar.
	sta	PathFileNameSrc,x	;
	sta	PathFileNameTar,x	;
	bne	lpcyf16			;
	ldx	#>-1			;
lpcyf20:inx				; Move Src PathFileName to Tmp.
	lda	CommandDataEnd+HandleSize,x
	sta	PathFileNameTmp,x	;
	bne	lpcyf20			;
	lda	PathNameEnding		;
	sta	PathNameSrcEnd		;
	sta	PathNameTarEnd		;
	jsr	CheckSwitch		;
	beq	necyf28			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
;---------------------------------------;
necyf28:lda	FreePage		;
	cmp	#$fe			;
	bcc	necyf36			;
	lda	#StringNoMemory		;
necyf32:sec				;
	rts				;
necyf36:lda	SourceNameMark		; Source file name has any "*" or "?".
	bne	necfs04			;
	jmp	necof04			; No.
;---------------------------------------; Copy files.
necfs04:lda	TargetNameMark		; Target file name has any "*" or "?".
	bne	necfs08			;
	jmp	necfo04			;
necfs08:lda	SizeCount		; Yes.
	beq	necfs12			;
	jmp	necfo08			; With '+'
;=======================================; Copy files to files.
necfs12:lda	FileIOSrc+CURDRIVE	;
	sta	FileIOTmp+CURDRIVE	;
	lda	#1			; Attribute.
	sta	FileIOTmp+BYTES		;
	lda	#FilePathFirst		;
lpcfs04:jsr	TempFileOperP		; Search file.
	bcs	necfs40			;
	lda	#1			; Find one file at least.
	sta	CopyAnyFileFlag		;
	jsr	MoveTmpFNToPath		; Move filename to PathFileNameTmp.
	jsr	MoveTmpSrcTar		;
	jsr	CopyCompMFTF		; Source name same as target one?
	beq	necfs52			;
	jsr	OpenFile		;
	bcs	necfs40			;
	lda	#FilePathOpen		; Open target file.
	jsr	TargetFileOperP		;
	bcs	necfs16			;
	lda	#FileClose		;
	jsr	TargetFileOper		;
	bcs	necfs44			;
	lda	WriteAll		; All over write?
	bne	necfs24			; Yes.
	jsr	AskOverWrite		;
	beq	necfs28			;
	bne	necfs36		;(jmp)	; No next file.
necfs16:ldx	#0			;
	stx	FileIOTar+HPAGE		;
	cmp	#FileNotExist		;
	bne	necfs44			;
necfs24:jsr	DisplayDFNTmpCRLF	;
necfs28:lda	FileIOTar+HPAGE		;
	and	#$17			; Is this file H/S/R?
	beq	necfs32			;
	jmp	DisplayCopyAccDeny	;
necfs32:lda	#FilePathCreate		; Create target file.
	jsr	TargetFileOperP		;
	bcs	necfs44			;
lpcfs08:jsr	ReadSourceFile		;
	bcs	necfs48			;
	jsr	WriteTargetFile		;
	bcs	necfs48			;
	lda	FileIOSrc+BYTES+1	;
	cmp	#<$4000			;
	beq	lpcfs08			;
	lda	#FileClose		;
	jsr	SourceFileOper		;
	jsr	CloseTarFile		;
	bcs	necfs48			;
	inc	FileCount		;
	bne	necfs36			;
	inc	FileCount+1		;
necfs36:lda	#FileNext		;
	bne	lpcfs04		;(jmp)	;
necfs40:lda	CopyAnyFileFlag		; Has find any file?
	bne	necfs44			;
	jmp	DisplayNoCopyFile	; No file.
necfs44:jmp	DisplayCopyFiles	;
necfs48:jmp	CopyAccessError		;
necfs52:jsr	DisplayDFNTmpCRLF	;
	jmp	DisplayCopySelf		;
;=======================================;
necfo04:lda	SizeCount		;
	bne	necfo08			; Copy files to one "with '+'".
	jmp	necfe04			;
necfo08:lda	SizeCount		;
	sta	SizeCount+1		; Files items
	lda	#<CommandDataEnd	; Source files pointer.
	sta	$fc+1			;
	lda	#>CommandDataEnd	;
	sta	$fc			;
lpcfo04:ldy	#CURDRIVE		;
	lda	($fc),y			;
	sta	FileIOTmp+CURDRIVE	;
	sta	FileIOSrc+CURDRIVE	;
	sta	SourceDrive		;
	ldx	$fc+1			;
	lda	$fc			;
	clc				;
	adc	#HandleSize		;
	bcc	necfo12			;
	inx				;
necfo12:tay				;
	stx	$fc+1			;
	sty	$fc			;
	stx	FileIOTmp+HBUF+1	;
	sty	FileIOTmp+HBUF		;
	ldy	#0			; Move pathfilename string to Tmp.
lpcfo08:sty	PathNameTmpEnd		; Search path ending.
lpcfo12:lda	($fc),y			;
	sta	PathFileNameTmp,y	;
	iny				;
	cmp	#'\'			;
	beq	lpcfo08			;
	tax				;
	bne	lpcfo12			;
	lda	#1			; Attribute.
	sta	FileIOTmp+BYTES		;
	lda	#FilePathFirst		; X|Y=Source filename pointer.
	jsr	TempFileOper		; Search a source filename first item.
	bcc	necfo24			;
	cmp	#SearchEnding		;
	beq	necfo44			;
	jmp	necfe20		;(jmp)	;
necfo24:lda	SizeCount+2		; Has target create flag?
	bne	necfo40			;
	inc	SizeCount+2		; No.
	jsr	MoveTmpFNToPath		; Move filename to PathFileNameTmp.
	lda	TargetNameMark		;
	beq	necfo28			;
	jsr	MoveTmpSrcTar		; Source With '+' and target with '*' or '?'.
necfo28:jsr	DisplayDFNTmpCRLF	;
	lda	#FilePathOpen		;
	jsr	SourceFileOperTmp	;
	bcs	necfo36			;
	jsr	ReadSourceFile		;
	bcs	necfo36			;
	jsr	CheckCopyTarget		;
	bcs	necfo32			;
	jsr	CopySrcsToTarIN		;
	bcs	necfe20			;
	bcc	necfo44		;(jmp)	;
necfo32:beq	necfe28			;
	jmp	DisplayCopyAccDeny	;
necfo36:lda	#FileClose		;
	jsr	SourceFileOper		;
	jmp	necfe20			;
necfo40:jsr	CopySrcsToTar		; Copy a source filename's all items.
	bcs	necfe20			;
necfo44:lda	$fc			;
	clc				;
	adc	#40			;
	sta	$fc			;
	bcc	necfo48			;
	inc	$fc+1			;
necfo48:dec	SizeCount+1		;
	beq	necfo52			;
	jmp	lpcfo04			;
necfo52:lda	SizeCount+2		;
	bne	necfe16			;
	jmp	DisplayNoCopyFile	; No file.
;---------------------------------------; Copy files to one.
necfe04:lda	FileIOSrc+CURDRIVE	;
	sta	FileIOTmp+CURDRIVE	;
	lda	#1			; Attribute.
	jsr	TempFileOperFP		; Search first file.
	bcc	necfe08			;
	cmp	#SearchEnding		;
	bne	necfe28			;
	jmp	DisplayNoCopyFile	; No file.
necfe08:jsr	CheckCopyTarget		;
	bcc	necfe12			;
	beq	necfe28			;
	jmp	DisplayCopyAccDeny	;
necfe12:jsr	CopySrcsToTar		;
	bcs	necfe20			;
necfe16:lda	#FileClose		; Close target file.
	jsr	TargetFileOper		;
	bcs	necfe24			;
	inc	FileCount		;
	jmp	DisplayCopyFiles	;
necfe20:lda	#FileClose		; Close target file.
	jsr	TargetFileOper		;
necfe24:lda	#FilePathDelete		;
	jsr	TargetFileOperP		;
necfe28:jmp	DisplayCopyFiles	;
;=======================================; Copy one file.
necof04:jsr	OpenFile		;
	bcs	necof06			;
	lda	#$1e			; Can not copied file?
	and	FAttribute		;
	beq	necof08			;
	lda	#FileClose		; No.
	jsr	SourceFileOper		;
necof06:jmp	DisplayNoCopyFile	;
necof08:jsr	MoveTmpSrcTar		;
	jsr	CopyCompMFTF		; Source name same as target one?
	beq	necof20			;
	jsr	ReadSourceFile		; Read source file.
	bcs	necof36			;
	lda	#FilePathOpen		; Open target file.
	jsr	TargetFileOperP		;
	bcs	necof12			;
	lda	#FileClose		; Target file exist.
	jsr	TargetFileOper		;
	lda	WriteAll		; All over write?
	bne	necof10			; Yes.
	jsr	AskOverWrite		; This file is exist.
	bne	necof36			;
necof10:lda	FileIOTar+HPAGE		;
	and	#$17			; Is this file H/S/R?
	beq	necof16			;
	lda	#FileClose		;
	jsr	SourceFileOper		;
	jmp	DisplayCopyAccDeny	;
necof12:cmp	#FileNotExist		;
	bne	necof36			;
necof16:lda	#FilePathCreate		; Create target file.
	jsr	TargetFileOperP		;
	bcs	necof36			;
	jsr	WriteTargetFile		;
	bcs	CopyAccessError		;
lpcof04:lda	FileIOSrc+BYTES+1	;
	cmp	#<$4000			;
	bne	necof24			;
	jsr	ReadSourceFile		;
	bcs	CopyAccessError		;
	jsr	WriteTargetFile		;
	bcc	lpcof04			;
;---------------------------------------;
CopyAccessError:			;
	lda	#FileClose		;
	jsr	TargetFileOper		;
	jmp	necof28			;
necof20:lda	#FileClose		;
	jsr	SourceFileOper		;
DisplayCopySelf:			;
	lda	#StringCopySelf		; Yes. copy error.
	jsr	DisplayStringCRLF	;
	jmp	DisplayCopyFiles	;
necof24:jsr	CloseTarFile		; Close target file and chnage attributes.
	bcc	necof32			;
necof28:lda	#FilePathDelete		;
	jsr	TargetFileOperP		;
	jmp	necof36			;
necof32:inc	FileCount		;
necof36:lda	#FileClose		;
	jsr	SourceFileOper		;
;---------------------------------------;
DisplayCopyFiles:			;
	jsr	DisplayFileCount	;
	lda	#StringCopied		;
	jmp	DisplayStringCRLF	;

;---------------------------------------;
StrVerHpSign	db	"Display the BBGDOS version",$0d,$0a
		db	$0a
		db	"VER$"
SthVerHpSign	db	"显示BBGDOS版本号",$0d,$0a
		db	"VER$"
;=======================================;
VersionCommand:				;
	jsr	CheckSwitch		;
	beq	nevsc04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
nevsc04:lda	#StringBBGDOSVer	;
	jmp	DisplayStringCRLF	;

StrVOLHpSign	db	"Displays the disk volume label",$0d,$0a
		db	$0a
		db	"VOL [drive:]$"
SthVOLHpSign	db	"显示磁盘的卷标",$0d,$0a
		db	$0a
		db	"VOL [盘:]$"
;=======================================;
VOLCommand:				;
	jsr	CheckSwitch		;
	jsr	CheckDriveCmd		;
	bcs	nevlc12			;
	sta	FileIOTmp+CURDRIVE	;
	jsr	CheckSwitch		;
	beq	DisplayVolumn		;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
;---------------------------------------;
DisplayVolumn:				;
	ldx	#0			;
	jsr	SetTmpAllFile		; Set "*.*" to PathFileNameTmp.
	lda	#8			;
	jsr	TempFileOperFP		; Search volumn.
	bcc	nevlc04			;
	cmp	#SearchEnding		;
	bne	nevlc12			;
nevlc04:php				;
	lda	#StringVolumn		;
	jsr	DisplayString		;
	lda	FileIOTmp+CURDRIVE	;
	ora	#'A'-1			;
	jsr	DisplayCharA		;
	plp				;
	bcs	nevlc08			;
	lda	#StringIS		;
	jsr	DisplayString		;
	jsr	IOTmpHBUFToFE		;
	ldy	#DTA_Filename		;
lpvlc04:lda	($fe),y			;
	jsr	DisplayCharA		;
	iny				;
	cpy	#DTA_Filename+11	;
	bne	lpvlc04			;
	jmp	DisplayCRLF		;
nevlc08:lda	#StringNoLabel		;
	jmp	DisplayStringCRLF	;
nevlc12:sec				;
	rts				;

StrInCHpsign	db	"Display DOS internal command",$0d,$0a
		db	$0a
		db	"?$"
SthInCHpsign	db	"显示DOS内部命令",$0d,$0a
		db	"?$"
;---------------------------------------;	
DisplayOneCommand:			;
	tya				;
	pha				;
	lda	#BiosGetCursor		;
	jsr	ROMBiosEntry		;
	tya				;
	beq	nedcd08			;
	ldy	#10			;
	cmp	#10			;
	bcc	nedcd04			;
	ldy	#20			;
	cmp	#20			;
	bcc	nedcd04			;
	jsr	DisplayCRLF		;
	bcc	nedcd08		;(jmp)	;
nedcd04:lda	#BiosSetCursor		;
	jsr	ROMBiosEntry		;
nedcd08:pla				;
	tay				;
lpdcd04:ldx	CommandStringDP,y	;
	beq	nedcd12			;
	jsr	DisplayChar		;
	iny				;
	bne	lpdcd04		;(jmp)	;
nedcd12:rts				;
;=======================================;
IntCmdCommand:				;
	jsr	CheckSwitch		;
	beq	neicd04			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
neicd04:jsr	DisplayCRLF		;
	ldy	#0			;
lpicd04:jsr	DisplayOneCommand	;
	iny				;
	lda	CommandStringDP,y	; Has multiple command name?
	beq	neicd08			;
	jsr	DisplayOneCommand	; Yes.
neicd08:tya				;
	clc				; Next command
	adc	#5+1			;
	tay				;
	lda	CommandStringDP,y	;
	bne	lpicd04			;
	jmp	DisplayCRLF		;

;---------------------------------------;
StrDelHpSign	db	"Deletes one or more files",$0d,$0a
		db	$0a
		db	"ERASE or",$0d,$0a
		db	"DEL [drive:][path]filename[/P]",$0d,$0a
		db	$0a
		db	"More files by useing wildcards",$0d,$0a
		db	$0a
		db	"/P Prompts for DEL each file$"
SthDelHpSign	db	"删除一个或多个文件",$0d,$0a
		db	"ERASE 或",$0d,$0a
		db	"DEL [盘:][路径]文件名 [/P]",$0d,$0a
		db	$0a
		db	"删除多个文件可用通配符(*)或(?)",$0d,$0a
		db	$0a
		db	"/P 每删除一个文件前要确认$"
;---------------------------------------;
HasReachROFile	equ	$f9		;
HasDelAnyFile	equ	$fa		;
DelPromptFlag	equ	$fb		;
DelSwitch	db	'P'		;
		dw	DelSwitchP-1	;
		db	0		;
;---------------------------------------;
DelSwitchP:				;
	inc	DelPromptFlag		;
	rts				;
;=======================================;
DelCommand:				; DEL(ERASE) command
	jsr	CheckSwitch		;
	bne	nedlf04			;
	lda	#StringParaMiss		;
	sec				;
nedlf02:rts				;
nedlf04:jsr	CheckDriveCmd		;
	sta	SourceDrive		;
	lda	#$80			;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	nedlf02			;
	lda	PathNameEnding		;
	sta	PathNameTmpEnd		;
	jsr	CheckSwitch		;
	beq	nedlf08			;
	ldx	#StringTooPara		;
	jmp	CommandLineError	;
;---------------------------------------;
nedlf08:lda	DelPromptFlag		; Prompt for delete each file?
	bne	nedlf12			;
	ldx	PathNameTmpEnd		; Is del *.*?
	ldy	#8			;
lpdlf04:lda	PathFileNameTmp,x	;
	beq	nedlf12			;
	inx				;
	cmp	#'*'			;
	beq	lpdlf12			;
	cmp	#'?'			;
	bne	nedlf12			;
	dey				;
	bne	lpdlf04			;
lpdlf12:lda	PathFileNameTmp,x	;
	beq	nedlf12			;
	inx				;
	cmp	#'.'			;
	bne	lpdlf12			;
lpdlf16:lda	PathFileNameTmp,x	;
	beq	nedlf12			;
	inx				;
	cmp	#'*'			;
	beq	lpdlf20			;
	cmp	#'?'			;
	bne	nedlf12			;
	dey				;
	bne	lpdlf16			;
lpdlf20:lda	#StringDelAll		; Yes. Conform.
	jsr	DisplayString		;
	lda	#3			; Read keyboard string.
	sta	$00e0			;
	lda	#DosGetKBString		;
	ldx	#<$00e0			;
	ldy	#>$00e0			;
	jsr	DosIOEntry		;
	jsr	DisplayCRLF		;
	lda	$00e0+2			; First character.
	and	#$df			;
	cmp	#'Y'			;
	beq	nedlf12			;
	cmp	#'N'			;
	beq	nedlf44			;
	jmp	lpdlf20			;
nedlf12:lda	#1			; Attribute.
	sta	FileIOTmp+BYTES		;
	lda	#FilePathFirst		; Search file first.
;---------------------------------------;
lpdlf24:jsr	TempFileOperP		;
	bcs	nedlf32			;
	jsr	IOTmpHBUFToFE		; Search ok.
	ldy	#DTA_FileAttrib		;
	lda	($fe),y			;
	and	#1			; This file ReadOnly one?
	beq	nedlf16			;
	sta	HasReachROFile		; Yes.
	bne	nedlf28		;(jmp)	;
nedlf16:jsr	MoveTmpFNToPath		; Move filename to PathFileNameTmp.
	lda	DelPromptFlag		;
	bne	lpdlf28			;
	lda	#DosCheckKeyBoard	; Check CTRL/C(Break)
	jsr	DosIOEntry		;
	jmp	nedlf20			;
lpdlf28:jsr	DisplayDFNTmp		;
	lda	#StringDeleteSign	;
	jsr	DisplayString		;
	lda	#DosReadKeyEcho		; Read key.
	jsr	DosIOEntry		;
	pha				;
	jsr	DisplayCRLF		;
	pla				;
	and	#$df			;
	cmp	#'N'			;
	beq	nedlf24			; No delete
	tax				;
	cmp	#'Y'			;
	bne	lpdlf28			;
;---------------------------------------;
nedlf20:lda	SourceDrive		;
	sta	FileIOSrc+CURDRIVE	;
	lda	#FilePathDelete		; Delete file.
	jsr	SourceFileOperTmp	;
	bcs	nedlf40			;
nedlf24:lda	#1			;
	sta	HasDelAnyFile		;
nedlf28:lda	#FileNext		;
	jmp	lpdlf24			;
nedlf32:cmp	#SearchEnding		;
	bne	nedlf40			;
	lda	HasDelAnyFile		; Has delete any file?
	bne	nedlf44			;
	lda	HasReachROFile		; No. Has reach ReadOnly file?
	beq	nedlf36			;
	lda	#StringAccDeny		; Yes.
	sec				;
	rts				;
nedlf36:lda	#StringNoFile		;
nedlf40:sec				;
	rts				;
nedlf44:clc				;
	rts				;

;=======================================;
CommandString	db	"BBGCDOS",0
BBGCDOSPoint	db		0
		dw	CDOSCommand-1,	CDOSSwitch
			db	StringCDOSHpSign
CommandStringDP	db	"CD"	,0
		db	"CHDIR"	,0
			dw	CDCommand-1,	CDSwitch
			db	StringCDHpSign
		db	"CLS"	,0
		db		 0
			dw	ClsCommand-1,	ClsSwitch
			db	StringClsHpSign
		db	"COPY"	,0	
		db		 0
			dw	CopyCommand-1,	CopySwitch
			db	StringCopyHpSign
		db	"DATE"	,0		
		db		 0
			dw	DateCommand-1,	DateSwitch
			db	StringDateHpSign
		db	"DEL"	,0		
		db	"ERASE"	,0
			dw	DelCommand-1,	DelSwitch
			db	StringDelHpSign
		db	"DIR"	,0
		db		 0
			dw	DirCommand-1,	DirSwitch
			db	StringDirHpSign
		db	"ECHO"	,0
		db		 0
			dw	EchoCommand-1,	EchoSwitch
			db	StringEchoHpSign
		db	"EXIT"	,0
		db		 0
			dw	ExitCommand-1,	ExitSwitch
			db	StringExitHpSign
		db	"MD"	,0
		db	"MKDIR"	,0
			dw	MDCommand-1,	MDSwitch
			db	StringMDHpSign
		db	"REM"	,0
		db		 0
			dw	REMCommand-1,	RemarksSwitch
			ds	1
		db	"RD"	,0
		db	"RMDIR"	,0
			dw	RDCommand-1,	RDSwitch
			db	StringRDHpSign
		db	"RENAME",0
		db	"REN"	,0
			dw	RenameCommand-1,RenameSwitch
			db	StringRenHpSign
		db	"SCREEN",0
		db		 0
			dw	ScreenCommand-1,ScreenSwitch
			db	StringScrHpSign
		db	"TIME"	,0		
		db		 0
			dw	TimeCommand-1,	TimeSwitch
			db	StringTimeHpSign
		db	"TYPE"	,0
		db		 0
			dw	TypeCommand-1,	TypeSwitch
			db	StringTypeHpSign
		db	"VER"	,0
		db		 0
			dw	VersionCommand-1,VersionSwitch
			db	StringVerHpSign
		db	"VOL"	,0
		db		 0
			dw	VOLCommand-1,	VOLSwitch
			db	StringVOLHpSign
		db	"?"	,0
		db		 0
			dw	IntCmdCommand-1,IntCmdSwitch
			db	StringInCHpsign
		db	0
ProgramString	db	"CMD"
		db	"EXC"
		db	"GAM"
		db	"BAT"
BatProgram	equ	@-ProgramString
;---------------------------------------;
InternalReturn:				;
	bcc	neirn04			;
	jsr	DisplayStringCRLF	;
neirn04:ldy	#0			; No initial screen flag.
	rts				;
;---------------------------------------;
lpccs04:inx				; Yes. next character.
	iny				;
CheckCommandStg:			;
	lda	CommandString,x		; Compare one character.
	beq	neccs04			;
	lda	CommandLine,y		; Change to Uppercase.
	jsr	CharToUpper		;
	cmp	CommandString,x		; This character same?
	beq	lpccs04			;
neccs04:rts				;
;---------------------------------------;
CommandReturnAccs:			;
	bcs	necra08			;
	tya				; Must initial screen?
	beq	necra12			;
	lda	#StringProgRet		;
necra08:pha				;
	lda	#DosGetSetDispMode	; Get display mode.
	ldx	#0			;
	jsr	DosIOEntry		;
	sta	DisplayMode		;
	pla				;
	jsr	DisplayStringCRLF	;
necra12:rts				;
;---------------------------------------;
;	Input	CommandLine		;
;	Output	(Y)!=0: Initial screen.	;
;		(A)= Return code.	;
;	 Internal: Return to caller.	;
;	 External: To ProgramReturn:.	;
;---------------------------------------;
CommandLineAccs:			;
	lda	#DosGetSetDispMode	; Get display mode.
	ldx	#0			;
	jsr	DosIOEntry		;
	sta	DisplayMode		;
	lda	#DosInquirePages	;
	ldx	#0			;
	jsr	DosIOEntry		;
	sta	FreePage		;
	ldy	#0			;
	sty	DisplayLower		;
	jsr	PassSpace		;
	beq	necla04			;
	tya				;
	pha				;
	jsr	DisplayCRLF		;
	pla				;
	tay				;
	jsr	CheckDriveCmd		; Has X: string?
	stx	FileIOTmp+CURDRIVE	;
	tax				;
	beq	necla12			; No.
	ldx	CommandLine,y		; Yes. Only X: string?
	cpx	#$0d			;
	beq	necla02			;
	jmp	necla26			; No. Search extend program.
;---------------------------------------; Yes. Change current drive command.
necla02:lda	FileIOTmp+CURDRIVE	;
	lda	#FileDrive		; Change to new driver.
	jsr	TempFileOper		;
	lda	#StringInvdDrive	;
	bcs	necla08			;
necla04:clc				;
necla08:jmp	InternalReturn		;
;=======================================; Search internal program.
necla12:ldx	#0			;
lpcla04:tya				;
	pha				; Save command line pointer.
	jsr	CheckCommandStg		; Search first command string.
	php				;
lpcla06:inx				; Point to next command if first unsame.
	lda	CommandString,x		; Point to parameters if first same.
	bne	lpcla06			;
	plp				;
	beq	necla16			;
	inx				;
	lda	CommandString,x		; Has second command string?
	beq	necla24			;
	pla				; Yes.
	pha				;
	tay				;
	jsr	CheckCommandStg		; Search second command string.
	bne	necla24			;
necla16:jsr	CCheckPhaseBreakS	; Command string same.
	beq	necla20			;
	cmp	#'.'			;
	beq	necla20			;
	cmp	#'\'			;
	bne	necla24			;
;---------------------------------------; Yes.
necla20:pla				; S--.
	cpx	#>BBGCDOSPoint-CommandString
	bne	necla22			; Is BBGCDOS command?
	sta	$fe			;
	sty	$ff			;
	lda	#BiosGetVersion		; Yes.
	jsr	ROMBiosEntry		;
	ldy	$fe			;
	cmp	#$40			; Is BBG new bios?
	bne	necla26			; No.
	cpx	#98			;
	bcc	necla26			;
	ldx	#>BBGCDOSPoint-CommandString
	ldy	$ff			;
necla22:lda	CommandString+3,x	;
	sta	SwitchPoint		;
	lda	CommandString+4,x	;
	sta	SwitchPoint+1		;
	lda	CommandString+5,x	;
	sta	SwitchHelpIndex		;
	lda	#<InternalReturn-1	; Set return address.
	pha				;
	lda	#>InternalReturn-1	;
	pha				;
	lda	CommandString+2,x	; Get command access address.
	pha				;
	lda	CommandString+1,x	;
	pha				;
	lda	#0			; Clear $e0-$fb.
	ldx	#$fc-$e0-1		;
lpcla08:sta	$e0,x			;
	dex				;
	bpl	lpcla08			;
	rts				; branch to command access.
;---------------------------------------;
necla24:pla				;
	tay				;
lpcla10:inx				; No. point to next command.
	lda	CommandString-1,x	;
	bne	lpcla10			;
	txa				; Has next command string?
	clc				;
	adc	#5			;
	tax				;
	lda	CommandString,x		;
	beq	necla26			;
	jmp	lpcla04			;
;=======================================; No. Search extend program.
necla26:lda	#$40			;
	ldx	FileIOTmp+CURDRIVE	;
	jsr	AccessPathFileName	; (X)= Drive
	bcs	necla32			;
	sty	CommandLineP		; Save command line ending.
	ldx	#RunProgramTypes-1	; Clear searched flag.
	lda	#0			;
lpcla12:sta	RunProgramFlag,x	;
	dex				;
	bpl	lpcla12			;
	jsr	CheckNameMark		; Has any ? in name?
	bcs	necla32			;
	lda	#3			; No. Hidden/RO/Normal file attribute.
	jsr	TempFileOperFP		; Search file.
	bcs	necla32			;
lpcla16:jsr	IOTmpHBUFToFE		;
	ldx	#0			;
	stx	$fd			; Type counter
lpcla20:ldy	#DTA_Filename+8		; Check if .EXT same as .RUN
lpcla24:lda	($fe),y			;
	cmp	ProgramString,x		;
	bne	lpcla28			;
	inx				;
	iny				;
	cpy	#DTA_Filename+11	;
	bne	lpcla24			;
	lda	#$ff			;
	ldx	$fd			; Yes.
	sta	RunProgramFlag,x	; Set same flag.
	bne	necla28			; Is .CMD ext?
	beq	necla30		;(jmp)	; Yes. Run it.
lpcla28:inx				; No.
	iny				;
	cpy	#DTA_Filename+11	;
	bne	lpcla28			; Compare next.
	inc	$fd			;
	cpx	#BatProgram		; Compare all?
	bne	lpcla20			;
necla28:lda	#FileNext		; Yes. Search next.
	jsr	TempFileOper		;
	bcc	lpcla16			;
;---------------------------------------;
necla30:ldy	#0			; Has any .RUN file?
lpcla32:lda	RunProgramFlag,y	;
	bne	necla36			; Yes.
	iny				;
	cpy	#RunProgramTypes	;
	bne	lpcla32			;
necla32:lda	#StringBadCmd		; No. Error comand.
	sec				;
	jmp	InternalReturn		;
necla36:tya				; Y*3
	sta	$fd			;
	asl				;
	adc	$fd			;
	tay				;
	jsr	SearchTmpEnding		;
lpcla34:dex				;
	lda	PathFileNameTmp,x	;
	cmp	#'.'			;
	bne	lpcla34			;
	lda	#3			;
	sta	$fd			;
lpcla36:lda	ProgramString,y		; Move .EXT to filename.
	sta	PathFileNameTmp+1,x	;
	inx				;
	iny				;
	dec	$fd			;
	bne	lpcla36			;
	lda	#0			;
	sta	PathFileNameTmp+1,x	;
	cpy	#BatProgram		;
	bne	necla40			;
	jmp	RunBATProgram		;
;---------------------------------------;
necla40:ldy	CommandLineP		; Pass command line string.
	ldx	#0			;
lpcla40:lda	CommandLine,y		;
	sta	RunCommandLine+1,x	;
	iny				;
	inx				;
	cmp	#$0d			;
	bne	lpcla40			;
	dex				;
	stx	RunCommandLine		; String length.
	tsx				;
	ldy	#0			; Save return stack.
lpcla44:lda	$101,x			;
	sta	RunProgramStack,y	; 2 JSR deep.
	inx				;
	iny				;
	cpy	#4			;
	bne	lpcla44			;
	jsr	RestoreErrorEntry	;
	lda	#BiosWaitDisplay	;
	jsr	ROMBiosEntry		;
	lda	FileIOTmp+CURDRIVE	;
	sta	$ff			;
	lda	#DosRunPathProgram	;
	ldx	#<PathFileNameTmp	;
	ldy	#>PathFileNameTmp	;
	jmp	RunProgramRelat		;
;---------------------------------------;
RunProgramReturn:			;
	jsr	SetErrorProgram		;
	pla				; Restore (Cy),(Y)
	tay				;
	plp				;
	ldx	#$ff			; Restore JSR stack.
	txs				;
	lda	#<ProgramEnd-$8000+$ff	;
	sta	CheckSumPages		;
	ldx	#3			;
lpcla48:lda	RunProgramStack,x	;
	pha				;
	dex				;
	bpl	lpcla48			;
	bcc	necla44			;
	tya				;
	ldx	#ReturnErrTypeL		; (A)= Error index.
	clc				;
lpcla52:dex				;
	bmi	necla44			; (Cy)=0.
	cmp	ReturnErrType,x		;
	bne	lpcla52			;
	lda	ReturnErrString,x	;
	sec				;
necla44:rts				;

;=======================================;
DisplayCursor:				;
	lda	DisplayMode		;
	bne	neccr08			;
	ldx	#$db			; Display block cursor.
	bne	neccr04		;(jmp)	;
;---------------------------------------;
ClearCursor:				;
	lda	DisplayMode		;
	bne	neccr08			;
DisplaySpace:				;
	ldx	#$20			;
neccr04:lda	#BiosDisplayChars	; Clear block cursor.
	ldy	#1			;
	jsr	ROMBiosEntry		;
neccr08:rts				;
;---------------------------------------;
;	Input	(A)= Character.		;
;---------------------------------------;
InsertCommandChar:			;
	ldx	StringLength		; Buffer full?
	inx				;
	cpx	#CmdLLen		;
	beq	neicc08			; Yes.
	sta	CommandLine-1,x		;
	inc	StringLength		;
	cmp	#$20			; Is Ctrl/Key?
	bcs	neicc04			;
	pha				;
	ldx	#'^'			;
	jsr	DisplayChar		;
	pla				;
	ora	#'A'-1			;
neicc04:tax				;
	jsr	DisplayChar		;
	clc				;
neicc08:rts				;
;---------------------------------------;
;	Output	(Zy)==1: Ending.	;
;---------------------------------------;
GetOneCommandSave:			;
	lda	#0			;
	sta	InsertModeFlag		;
	ldx	CommandSaveLP		;
	cpx	KBStringLen		;
	beq	negos04			; (Zy)=1.
	lda	KBStringSave,x		;
	jsr	InsertCommandChar	;
	inc	CommandSaveLP		; (Zy)=0.
negos04:rts				;
;---------------------------------------;
DelOneCommand:				;
	lda	#0			;
	sta	InsertModeFlag		;
	ldy	StringLength		; Yes. Has character in string?
	beq	nedoc08			;
	dey				;
	sty	StringLength		;
	lda	CommandLine,y		; Yes. Get last character.
	cmp	#$20			; Is Ctrl/Key?
	bcs	nedoc04			;
	jsr	ClearCursor		; Yes.
	ldx	#$08			;
	jsr	DisplayChar		;
	jsr	DisplaySpace		;
nedoc04:jsr	ClearCursor		; Clear Cursor.
	ldx	#$08			;
	jsr	DisplayChar		;
	jsr	DisplaySpace		;
nedoc08:dec	CommandSaveLP		;
	bpl	nedoc12			;
	inc	CommandSaveLP		;
nedoc12:rts				;
;---------------------------------------;
;	Output	(A)= Character.		;
;		(Zy)=1: Function key.	;
;---------------------------------------;
CommandGetKeyCur:			;
	jsr	DisplayCursor		; Display block cursor.
CommandGetKey:				;
	lda	#DosReadKeyNoEcho	;
	jsr	DosIOEntry		;
	tax				;
	bne	necgk04			;
	lda	#DosReadKeyNoEcho	;
	jsr	DosIOEntry		;
	ldx	#0			;
necgk04:rts				;
;---------------------------------------;
F2Key:	lda	#0			;
	sta	InsertModeFlag		;
	jsr	CommandGetKeyCur	; F2. Copy until CH.
	beq	nef2k08			; Function key.
	ldx	CommandSaveLP		;
nef2k04:cpx	KBStringLen		;
	beq	nef2k08			;
lpf2k04:inx				;
	cpx	KBStringLen		;
	beq	nef2k08			;
	cmp	KBStringSave,x		;
	bne	lpf2k04			;
	ldx	CommandSaveLP		;
lpf2k08:pha				;
	lda	KBStringSave,x		;
	jsr	InsertCommandChar	;
	pla				;
	inc	CommandSaveLP		;
	ldx	CommandSaveLP		;
	cmp	KBStringSave,x		;
	bne	lpf2k08			;
nef2k08:rts				;
;---------------------------------------;
F3Key:	lda	#0			;
	sta	InsertModeFlag		;
lpf3k04:jsr	GetOneCommandSave	; Yes. Copy LineSave->Line
	bne	lpf3k04			;
	rts				;
;---------------------------------------;
F4Key:	lda	#0			;
	sta	InsertModeFlag		;
	jsr	CommandGetKeyCur	; F4. Skip until CH.
	beq	nef4k08			; Function key.
	ldx	CommandSaveLP		;
lpf4k04:cpx	KBStringLen		;
	beq	nef4k08			;
	cmp	KBStringSave,x		;
	beq	nef4k04			;
	inx				;
	bne	lpf4k04		;(jmp)	;
nef4k04:stx	CommandSaveLP		;
nef4k08:rts				;
;---------------------------------------;
InsertMode:				;
	inc	InsertModeFlag		;
	rts				;
;---------------------------------------;
DelOneSave:				;
	ldx	CommandSaveLP		; No.
	cpx	KBStringLen		;
	beq	nedos04			;
	inc	CommandSaveLP		;
nedos04:rts				;
;---------------------------------------;
CommandFuncChar	db	$3b,			$3c		; F1,F2
		db	$3d,			$3e		; F3,F4
		db	$4b,			$4d		; <-,->
		db	$52,			$53		; Ins,Del
CommandFuncChL	equ	@-CommandFuncChar
CommandFuncHigh	db	<GetOneCommandSave-1,	<F2Key-1
		db	<F3Key-1,		<F4Key-1	
		db	<DelOneCommand-1,	<GetOneCommandSave-1
		db	<InsertMode-1,		<DelOneSave-1
CommandFuncLow	db	>GetOneCommandSave-1,	>F2Key-1
		db	>F3Key-1,		>F4Key-1	
		db	>DelOneCommand-1,	>GetOneCommandSave-1
		db	>InsertMode-1,		>DelOneSave-1
;---------------------------------------;
CommandBranch:				;
	lda	CommandFuncHigh,x	;
	pha				;
	lda	CommandFuncLow,x	;
	pha				;
	rts				;
;=======================================;
;	DOS function (AH=0A)		;
;	Input	X|Y= String.		;
;		(+0)= String length Lim.;
;	Output	Cy=1: Ctrl/C input.	;
;		(+1)= String length.	;
;		(+2-)= String.		;
;---------------------------------------;
GetKBString:				;
	lda	#BiosGetCursor		;
	jsr	ROMBiosEntry		;
	sty	CommandCurBegin		;
lpgks04:lda	#0			; String point.
	sta	StringLength		;
	sta	CommandSaveLP		;
	sta	InsertModeFlag		;
lpgks08:jsr	CommandGetKeyCur	;
	beq	negks24			; Function key.
	cmp	#$08			; Backspace?
	bne	negks12			;
	jsr	DelOneCommand		;
	jmp	lpgks08			;
negks12:cmp	#$1b			; ESC key?
	bne	negks28			;
	ldx	#'\'			; Yes.
	jsr	DisplayChar		;
	jsr	DisplayCRLF		;
	lda	CommandCurBegin		;
	beq	lpgks04			;
	sta	$fd			;
lpgks12:ldx	#' '			;
	jsr	DisplayChar		;
	dec	$fd			;
	bne	lpgks12			;
	beq	lpgks04		;(jmp)	;
;---------------------------------------;
negks24:ldx	#CommandFuncChL		; Function key.
lpgks16:dex				;
	bmi	lpgks08			;
	cmp	CommandFuncChar,x	;
	bne	lpgks16			;
	jsr	CommandBranch		;
	jmp	lpgks08			;
;---------------------------------------;
negks28:cmp	#$0d			; String ending?
	beq	negks36			;
	cmp	#$09			; No. Tab code?
	bne	negks32			;
	lda	#$20			;
negks32:jsr	InsertCommandChar	;
	bcs	lpgks08			; Buffer full?
	lda	InsertModeFlag		;
	lsr				;
	bcs	lpgks08			;
	ldx	CommandSaveLP		; No.
	cpx	KBStringLen		;
	beq	lpgks08			;
	inc	CommandSaveLP		;
	bne	lpgks08		;(jmp)	;
;---------------------------------------;
negks36:jsr	ClearCursor		; Clear Cursor.
	ldx	#$0d			;
	jsr	DisplayChar		;
	lda	#$0d			;
	ldy	StringLength		;
	sta	CommandLine,y		; Save CR.
	sty	KBStringLen		;
	beq	negks40			;
lpgks20:lda	CommandLine-1,y		;
	sta	KBStringSave-1,y	;
	dey				;
	bne	lpgks20			;
negks40:rts				;

;---------------------------------------;
RunProgramAbort:			;
	jsr	DisplayCtrlC		;
;=======================================;
CommandMain:				;
	lda	#0			;
	sta	EchoFlagCommand		;
	sta	KBStringLen		;
lpcmn04:lda	EchoFlagCommand		;
	sta	EchoFlagLine		;
	ldx	#$ff			;
	txs				;
	lda	#DosGetSetDispMode	; Get display mode.
	ldx	#0			;
	jsr	DosIOEntry		;
	sta	DisplayMode		;
	jsr	DisplayCRLF		;
	jsr	DisplayPrompt		;
	jsr	GetKBString		;
	jsr	CommandLineAccs		;
	jsr	CommandReturnAccs	;
	jmp	lpcmn04			;
;---------------------------------------;
ProgramEnd:				;
KBStringSave	equ	(ProgramEnd+255)/256*256
ProgramAllEnd	equ	KBStringSave+CmdLLen
	
;---------------------------------------;
InitialNext:				;
	jsr	ROMBiosEntry		;
	tya				; Has '/K','/Kp' or '/C'?
	beq	neint08			;
	cpy	#3			; Yes. '/Kp' switch?
	beq	neint04			;
	jsr	StepThroughAsk		; No.
	bne	neint08			;
neint04:jsr	CommandLineAccs		;
;---------------------------------------;
FaultAbortProc:				;
neint08:lda	CmdRunProgram		; /C switch?
	cmp	#2			;
	bne	neint12			;
	jmp	ExitCommandIN		; Yes. Exit.
neint12:lda	#0			;
	sta	CmdStepThrough		;
	jmp	CommandMain		;
;---------------------------------------;
ProgramMoveEnd:

AutoexecFile	db	"AUTOEXECBAT"	;
AutoexecBat	db	"AUTOEXEC.BAT",$0d
AutoexecBatLen	equ	@-AutoexecBat	;
;---------------------------------------;
StrNoLowRAM	db	"No enough low RAM to work.$"
SthNoLowRAM	db	"低端内存不足,不能执行$"
StrErrorVersion	db	"DOS version incorrect.$"
SthErrorVersion	db	"DOS版本号不对!$"
StrCopyright	db	$0a
		db	"BBG(R) BBGDOS(R) Version "
		db	DOSVersion+'0'
		db	"."
		db	DOSSubVersion+'0'
		db	$0d,$0a
		db	"(C)Copyright R.C.Tang 1998",$0d,$0a,"$"
SthCopyright	db	$0a
		db	"『步步高』 BBGDOS(R) 第"
		db	DOSVersion+'0'
		db	"."
		db	DOSSubVersion+'0'
		db	"版",$0d,$0a
		db	"版权所有 清华大学 唐瑞春 1998.",$0d,$0a,"$"
StrCmdHpSign	db	$0a
		db	"Start new BBGDOS command shell",$0d,$0a
		db	$0a
		db	"COMMAND [/P]",$0d,$0a
		db	"        [/Y [/K Cmd|/C Cmd]]",$0d,$0a
		db	$0a
		db	"/P Command shell permanent",$0d,$0a
		db	"/Y Steps through Cmd of /C,/K",$0d,$0a
		db	"/K Cmd: Execute Cmd & resident",$0d,$0a
		db	"/C Cmd: Execute Cmd & exit$"
SthCmdHpSign	db	"启动一个新的BBGDOS命令解释程序",$0d,$0a
		db	"COMMAND [/P]",$0d,$0a
		db	"        [/Y [/K 命令|/C 命令]]",$0d,$0a
		db	$0a
		db	"/P COMMAND 驻留不许退出(EXIT)",$0d,$0a
		db	"/Y 单个执行/C和/K指定的命令",$0d,$0a
		db	"/K 命令:执行“命令”并驻留内存",$0d,$0a
		db	"/C 命令:执行“命令”然后退出$"
;---------------------------------------;
CommandSwitch	db	'P'		;
		dw	CommandSwitchP-1;
		db	'Y'		;
		dw	CommandSwitchY-1;
		db	'K'		;
		dw	CommandSwitchK-1;
		db	'C'		;
		dw	CommandSwitchC-1;
		db	0		;
;---------------------------------------;
CommandSwitchP:				;
	inc	CmdPermanentTMP		;
	rts				;
CommandSwitchY:				;
	inc	CmdStepThTMP		;
	rts				;
;---------------------------------------;
CommandSwitchC:				;
	lda	#2			;
	bne	necdk04		;(jmp)	;
;---------------------------------------;
CommandSwitchK:				;
	lda	#1			;
necdk04:sta	CmdRunProgTMP		;
	jsr	PassSpace		;
	ldx	#0			;
lpcdk04:lda	CommandLine,y		;
	sta	CommandLine,x		;
	iny				;
	inx				;
	cmp	#$0d			;
	bne	lpcdk04			;
	dey				;
	clc				;
	rts				;
;=======================================;
CommandCommand:				;
	jsr	CheckSwitch		;
	beq	neccd04			;
	ldx	#StringTooPara		;
	jsr	CommandLineError	;
neccd04:rts				;

CommandReturnB:				;
;---------------------------------------;
FileIOZero	equ	$0040		;
;-----------------------------------------------;
CheckSum	equ	$6018			; ds 1
;-----------------------------------------------; Save to low when another COMMAND shell.
RunProgramDataB	equ	CheckSum+2		;
RunProgramStack	equ	RunProgramDataB		;|ds 4
CheckSumPages	equ	RunProgramStack+4	;|ds 2
CmdPermanent	equ	CheckSumPages+1		;|ds 1 COMMAND.CMD permanent.(can't exit)
CmdRunProgram	equ	CmdPermanent+1		;|ds 1 1:/K ,2:/C 3:/P(only)
CmdStepThrough	equ	CmdRunProgram+1		;|ds 1 /Y
RunBATEnter	equ	CmdStepThrough+1	;|ds 1 Has enter RUN BAT flag.
RunProgramDataP	equ	RunBATEnter+1		;|ds 2
CtrlCProcEntryS	equ	RunProgramDataP+2	;|ds 3
FaultErrorEntS	equ	CtrlCProcEntryS+3	;|ds 3
EchoFlagCommand	equ	FaultErrorEntS+3	;|ds 1  0:on; !0:off.
EchoFlagBAT	equ	EchoFlagCommand+1	;|ds 1
KBStringLen	equ	EchoFlagBAT+1		;|ds 1
FileIOInter	equ	KBStringLen+1		;|ds HandleSize($20 only)
RunProgramDataL	equ	FileIOInter+$20-RunProgramDataB
CommandReturnBL	equ	FileIOInter+$20		;
;=======================================;
	orgrun	CommandReturnBL		;
;---------------------------------------;
RunProgramRelat:			;
	jsr	DosIOEntry		;
	php				; Save (Y),(Cy)
	tya				;
	pha				;
	jsr	CheckCommand		;
	jmp	RunProgramReturn	;
;---------------------------------------;
CheckCommand:				;
	lda	#BiosSetPage		;
	ldx	#$fe			;
	jsr	ROMBiosEntry		;
	ldx	#<$8000			; Get checksum
	ldy	#>$8000			;
	stx	$fe+1			;
	sty	$fe			;
;	ldy	#0			;
	tya				;
	tax				;
lprpm04:pha				;
	clc				;
	txa				;
	adc	($fe),y			;
	tax				;
	iny				;
	pla				;
	adc	($fe),y			;
	iny				;
	bne	lprpm04			;
	inc	$fe+1			;
	dec	CheckSumPages		;
	bne	lprpm04			;
	cmp	CheckSum+1		;
	bne	nerpm04			;
	cpx	CheckSum		;
	beq	nerpm12			;
;---------------------------------------;
nerpm04:sty	KBStringLen		; (Y)=0.
lprpm08:lda	#$ff			; Open COMMAND.CMD
	sta	FileIOZero+CURDRIVE	; From boot drive.
	lda	#FilePathOpen		;
	ldx	#<COMMANDFile		;
	ldy	#>COMMANDFile		;
	jsr	TempFileOperHBUFL	;
	bcc	nerpm08			;
	lda	#DosDisplayString	;
	ldx	#<InsertCommand		;
	ldy	#>InsertCommand		;
	jsr	DosIOEntry		;
	lda	#DosReadKeyNoEcho	;
	sta	$ff			;
	lda	#DosReadKBClear		;
	jsr	DosIOEntry		;
	jmp	lprpm08			;
nerpm08:lda	#$fe			; Read COMMAND.CMD
	sta	FileIOZero+HPAGE	;
	ldx	#<ProgramEnd-$8000+$ff	;
	ldy	#2			;$Pass CMD header.
	stx	FileIOZero+BYTES+1	;
	sty	FileIOZero+BYTES	;
	lda	#FileRead		;
	ldx	#<$8000-2		;
	ldy	#>$8000-2		;
	jsr	TempFileOperHBUFL	;
	lda	#FileClose		;
	jsr	TempFileOperL		;
nerpm12:rts				;
;---------------------------------------;
TempFileOperHBUFL:			;
	stx	FileIOZero+HBUF+1	;
	sty	FileIOZero+HBUF		;
TempFileOperL:				;
	sta	FileIOZero+FILECMD	;
	ldx	#<FileIOZero		;
	ldy	#>FileIOZero		;
	jmp	FileIOEntry		;
InsertCommand	db	$0d,$0a
		db	"Insert system disk in A:$"
COMMANDFile	db	"\COMMAND.CMD",0;
	orgreal				;
CommandReturnL	equ	@-CommandReturnB;

;---------------------------------------;
SaveErrorProcEntry:			;
	lda	CtrlCProcEntry		; Save original CtrlCProcEntry vector.
	ldx	CtrlCProcEntry+2	;
	ldy	CtrlCProcEntry+1	;
	sta	CtrlCProcEntryS		;
	stx	CtrlCProcEntryS+2	;
	sty	CtrlCProcEntryS+1	;
	lda	FaultErrorEntry		;
	ldx	FaultErrorEntry+2	;
	ldy	FaultErrorEntry+1	;
	sta	FaultErrorEntS		;
	stx	FaultErrorEntS+2	;
	sty	FaultErrorEntS+1	;
	rts				;
;=======================================;
Initial:ldx	#$ff			;
	txs				;
	lda	#DosGetSetDispMode	; Get display mode.
	ldx	#0			;
	jsr	DosIOEntry		;
	sta	DisplayMode		;
	lda	#DosGetDOSVersion	;
	jsr	DosIOEntry		;
	bcs	neini02			;
	cmp	#DOSVersion		;
	bne	neini02			;
	cpx	#DOSSubVersion		;
	beq	neini04			;
neini02:lda	#StringErrorVersion	;
	jmp	neini14			;
neini04:ldx	#0			;
lpini04:lda	RunCommandLine+1,x	;
	sta	CommandLine,x		;
	inx				;
	cmp	#$0d			;
	bne	lpini04			;
	ldx	#<CommandSwitch		;
	ldy	#>CommandSwitch		;
	stx	SwitchPoint+1		;
	sty	SwitchPoint		;
	lda	#StringCmdHpSign	;
	sta	SwitchHelpIndex		;
	ldy	#0			;
	sty	SwitchHelp		;
	sty	CmdPermanentTMP		;
	sty	CmdRunProgTMP		;
	sty	CmdStepThTMP		;
	jsr	CommandCommand		; ---
	lda	SwitchHelp		; /? switch?
	beq	neini08			;
	lda	#DosProgramReturn	; Yes
	ldx	#0			;
	ldy	#0			;
	jmp	DosIOEntry		;
;---------------------------------------;
neini08:lda	CmdPermanentTMP		; '/P' switch?
	beq	neini12			;
;---------------------------------------; Has '/P' switch.
	lda	CmdRunProgTMP		; Has /C or /K switch?
	beq	neini20			;
	cmp	#2			; Yes. is '/C' switch?(1:/K,2:/C).
	bne	neini24			;
	lda	#0			; Yes. Has /P and /C. 
	sta	CmdPermanentTMP		;  Clear '/P' switch.
;---------------------------------------; No '/P' switch.
neini12:lda	#BiosLowRAMAlloc	; (Can exit) Allocate low memory.
	ldx	#<RunProgramDataL	;
	ldy	#>RunProgramDataL	;
	jsr	ROMBiosEntry		;
	cmp	#0			;
	beq	neini16			;
	lda	#StringNoLowRAM		; (No enough low memory)
neini14:jsr	DisplayStringCRLF	;
	lda	#DosProgramReturn	;
	ldx	#>-1			; Error exit.
	ldy	#0			;
	jmp	DosIOEntry		;
neini16:stx	$fa+1			; Low ram beginning.
	sty	$fa			;
	ldy	#>RunProgramDataL-1	; Save RunPorgramData area to low.
lpini08:lda	RunProgramDataB,y	;  (Last COMMAND.CMD used)
	sta	($fa),y			;
	dey				;
	bpl	lpini08			;
	ldx	$fa+1			;
	ldy	$fa			;
	stx	RunProgramDataP+1	; Set RunProgramDataP for EXIT restore.
	sty	RunProgramDataP		;
	jmp	neini24		;(jmp)	;
;---------------------------------------; Has '/P' and No '/K' or '/C';
neini20:ldy	#CommandReturnL		; Move CommandReturnB to RunProgram.
lpini12:lda	CommandReturnB-1,y	;
	sta	CommandReturnBL-1,y	;
	dey				;
	bne	lpini12			;
	lda	#BiosGetPage		;
	jsr	ROMBiosEntry		;
	sta	FileIOTmp+HPAGE		; Check AUTOEXEC.BAT file is exist?
	lda	#0			;
	sta	FileIOTmp+CURDRIVE	;
	lda	#FileFirst		;
	ldx	#<AutoexecFile		;
	ldy	#>AutoexecFile		;
	jsr	TempFileOperHBUF	;
	bcs	neini24			;
	ldx	#AutoexecBatLen-1	; Yes.
lpini16:lda	AutoexecBat,x		; Move "AUTOEXEC.BAT",$0d to CommandLine
	sta	CommandLine,x		;
	dex				;
	bpl	lpini16			;
	lda	#3			;
	sta	CmdRunProgTMP	;(=1)	; Set '/Kp' switch flag.
;---------------------------------------;
neini24:ldx	#<$8000			; Set CheckSum.
	ldy	#>$8000			;
	stx	$fe+1			;
	sty	$fe			;
	lda	#<ProgramEnd-$8000+$ff	;
	sta	CheckSumPages		;
	sta	$fc			;
	lda	#0			;
	tax				;
;	tay				;
lpini24:pha				;
	clc				;
	txa				;
	adc	($fe),y			;
	tax				;
	iny				;
	pla				;
	adc	($fe),y			;
	iny				;
	bne	lpini24			;
	inc	$fe+1			;
	dec	$fc			;
	bne	lpini24			;
	sta	CheckSum+1		;
	stx	CheckSum		;
	jsr	SaveErrorProcEntry	;
;---------------------------------------;
	lda	CmdRunProgTMP		; Has /C or /K switch?
	bne	neini28			;
	lda	CmdPermanentTMP		; No. Has /P switch?
	beq	neini28			;
	lda	#$60			; RTS
	sta	CtrlCProcEntry		; No process Ctrl/C
	jsr	DateCommandIN		; No. run DATE
	jsr	TimeCommandIN		; and run TIME.
	lda	#StringCopyright	; 
	jsr	DisplayString		;
neini28:jsr	SetErrorProgram		;
;---------------------------------------; Move program to $FE page.
	lda	#BiosGetPage		;
	jsr	ROMBiosEntry		;
	sta	$f8+0			;
	ldx	#<$8000			;
	ldy	#>$8000			;
	stx	$f8+2			;
	sty	$f8+1			;
	lda	#$fe			;
	sta	$f8+3			;
	stx	$f8+5			;
	sty	$f8+4			;
	ldx	#<ProgramMoveEnd-$8000+$ff; 256 align for checksum.
	ldy	#0			;
	stx	$f8+7			;
	sty	$f8+6			;
	lda	#BiosMoveBlock		;
	ldx	#>$f8			;
	jsr	ROMBiosEntry		;
	lda	#0			;
	sta	RunBATEnter		;
	sta	EchoFlagCommand		;
	lda	CmdPermanentTMP		;
	sta	CmdPermanent		;
	lda	#0			;
	ldy	CmdRunProgTMP		; Has '/K' or '/C'?
	beq	neini32			;
	lda	CmdStepThTMP		; Yes.
neini32:sta	CmdStepThrough		;
	sty	CmdRunProgram		; "/K or /C switch" flag.
	lda	#BiosSetPage		; JMP to $FE page.
	ldx	#$fe			;
	jmp	InitialNext		;

	end
