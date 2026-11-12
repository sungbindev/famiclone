        include tvdef.asm
	include	hal-defs.asm
	org	$c000			;
BiosYear        equ     97              ;
BiosMonth	equ	1		;
BiosDate	equ	0		;
;---------------------------------------;
;    Floppy Diskette Controller port.	;
;---------------------------------------;
FDCDMADackIO    equ     $4200           ; 3F1H
FDCDMATcIO	equ	$4201		; 3F1H
;---------------------------------------;
FDCDRQPortI     equ     $4202           ; 3F2H
;       Bit 6:  FDC DRQ pin.            ;
FDCCtrlPortO    equ     $4101           ; 3F2H
;       Bit 5:  Driver B motor enable   ;
;       Bit 4:  Driver A motor enable   ;
;	Bit 3:	Enable INT and DMA signal
;       Bit 2:  not FDC Reset.          ;
;       Bit 10: Driver select.          ;
;---------------------------------------;
FDCIRQPortI     equ     $4203           ; 3F3H
;       Bit 6:  FDC IRQ pin.            ;
;---------------------------------------;
FDCResetPortO   equ     $4204   ;ff30   ; 3F4H(3F6).
;	Bit 6:	FDC pin Reset		;
FDCStatPortI	equ	$4204		; 3F4H.
;	Bit 7:	FDC ready to communication.
;	Bit 6:	FDC comunication direction.
;	Bit 5:	FDC in excution process.
;	Bit 4:	
;---------------------------------------;
FDCDataPortIO   equ     $4205           ; 3F5H.
;---------------------------------------;
FDCChangePortI  equ     $4207           ; 3F7H.
;	Bit 7:	Disk change.
FDCSpeedPortO	equ	$4100		; 3F7H.
;	Bit 10:	00: 500kbps(1.2M,1.44M)
;		01: 300kbps(360K)
;		10: 250kbps(720K)
;---------------------------------------;
SpeakInitPort	equ	vDummy		;
SpeakDataPort	equ	vDummy		;
;---------------------------------------;
;    PC Controller(Parallel) port.	;
;---------------------------------------;
PCDaCtPortO	equ	vDummy		;
 PCCDataPort    equ     vDummy           ; PCC port base+0
;---------------------------------------;
;	   Bit 4: Singal to PC's ERROR.	;
;	   Bit 3-0: Output PC data.	;
PCCtrlPortI	equ	vDummy		;
 PCCStatsPortI	equ	vDummy		; PCC port base+1
;	   Bit 3: Signal of PC's pin STB
PCDataPortIH	equ	vDummy		;
;	   Bit7-4: PC input data.	;
PCDataPortIL	equ	vDummy		;
 PCCCtrlPoutO	equ	vDummy		; PCC port base+2
;	   Bit3-0: PC input data.	;
;---------------------------------------;
;    Game Card Controller port.		;
;---------------------------------------;
VideoCtrlPort	equ	$ff01		;
;	Bit 10: 6116's AV10 connection.	;
;		00: AV10		;
;		01: AV11		;
;		10: 0			;
;		11: 1			;
;	Bit 2:  INT count/-load.	;
;	Bit 3: 	c000-ffff map to DRAM/-ROM
IntCountPortL	equ	$ff02		;
;	Bit 4-0: Counter.		; Low 4 bits.
VideoDataPort	equ	$ff03		;
;	Bit 7-0: Registers file data	;
DRAMPagePort	equ	$ff04		;
;	Bit 3-0: Pages.			;
DRAMPagePortCD	equ	$ff24		;
DRAMPagePortEF	equ	$ff2c		;
;=======================================;
SoundPort0	equ	$ff10		;
SoundPort1	equ	$ff18		;
KebBoardLEDPort	equ	vDummy		;
;---------------------------------------;
Addr2000	equ	$6000		;
Addr2001	equ	$6001		;
Addr2005	equ	$6005		;
SMDOS_Begin	equ	$7c00		;
;===============================================;
;   Keyboard,Mouse,Printer work buffer.		;
;-----------------------------------------------;
KBDBufferSize	equ	64			;
KBDBegin	equ	0			;
KBDEnd		equ	KBDBufferSize		;
KBDBuffer	equ	 $5c00			; ds KBDBufferSize
NMICheckFlag	equ	KBDBuffer+KBDBufferSize	; ds 1
	;	Bit7: +No/-Do NMI keyboard check.
	;	Bit6: +Do/-No keyboard check if no key in buffer
TempSave1	equ	NMICheckFlag+1		; ds 1
TempSave2	equ	TempSave1+1		; ds 1
BiosKeyCode	equ	TempSave2+1		; ds 2
DelayTimes	equ	BiosKeyCode+2		; ds 1
RateTime	equ	DelayTimes+1		; ds 1
MaximumS	equ	RateTime+1		;|
MaximumX	equ	MaximumS		;|ds 2
MaximumY	equ	MaximumX+2		;|ds 2
MouseMovePlus	equ	MaximumY+2		; ds 2
KeyboardDataLen	equ	12			;
CurrentKeyMap	equ	MouseMovePlus+2		; ds KeyboardDataLen
PressCount0	equ	CurrentKeyMap+KeyboardDataLen
PressACount0	equ	PressCount0		;|ds 1
PressBCount0	equ	PressACount0+1		;|ds 1
PressSlCount0	equ	PressBCount0+1		;|ds 1
PressStCount0	equ	PressSlCount0+1		;|ds 1
PressCount1	equ	PressStCount0+1		;
PressACount1	equ	PressCount1		; |ds 1
PressBCount1	equ	PressACount1+1		; |ds 1
PressSlCount1	equ	PressBCount1+1		; |ds 1
PressStCount1	equ	PressSlCount1+1		; |ds 1
MinimumS	equ	PressStCount1+1		;|
MinimumX	equ	MinimumS		;|ds 2
MinimumY	equ	MinimumX+2		;|ds 2
LastRlMoveKey	equ	MinimumY+2		; ds 2
MoveRlKeyCount	equ	LastRlMoveKey+2		; ds 2
MovePrKeyCount	equ	MoveRlKeyCount+2	; ds 2
MouseMovePlusC	equ	MovePrKeyCount+2	; ds 1
;---------------------------------------; Initial by KeyboardReset
KBResetClearBeg	equ	MouseMovePlusC+1	;
LastKeyMap	equ	KBResetClearBeg		;  |ds KeyboardDataLen
KBDHead		equ	LastKeyMap+KeyboardDataLen;|ds 1
KBDTail		equ	KBDHead+1		;  |ds 1
KeyAltCompose	equ	KBDTail+1		;  |ds 1 Used for compose ALT/[num(keypad)].
AutoRepeatCount	equ	KeyAltCompose+1		;  |ds 1
AutoRepeatDelay	equ	AutoRepeatCount+1	;  |ds 1
KBResetClearLen	equ	AutoRepeatDelay+1-KBResetClearBeg
;===============================================;
;    Bios and Monitor work buffer.		;
;-----------------------------------------------;
DisplayBuffer	equ	$5d00			; $5d00
SplitScreenDt	equ	DisplayBuffer+$100	; $5e00
MoveBuffer	equ	SplitScreenDt+$0a0	; $5ea0
StringBuffer	equ	MoveBuffer		;
DiskIOBuffer	equ	MoveBuffer		;
DiskIOZB	equ	$00			;
 Command	equ	$00			; ds 1
  CMD_RESET	equ	0			; Reset diskette system.
  CMD_STATUS	equ	1			; (A)= Status. (X)= Driver.
  CMD_READ	equ	2			; Sectors read.
  CMD_WRITE	equ	3			; Sectors write.
  CMD_VERIFY	equ	4			; Sectors verify.
  CMD_FORMAT	equ	5			; Format track.
  CMD_SETPARAM	equ	6			; Set diskette media parameter.
 Drive		equ	1			; ds 1
 Track		equ	2			; ds 1
 Head		equ	3			; ds 1
 Sector		equ	4			; ds 1
 BPage		equ	5			; ds 1
 Buffer		equ	6			; ds 2 Buffer>$c000 || Buffer+Sectors*512<$c000
 Sectors	equ	8			; ds 1
 DiskIOCmdSize	equ	Sectors+1-Command	;
DiskIOPageSave	equ	DiskIOZB+DiskIOCmdSize	; ds 1
SectorBfPoint0	equ	DiskIOPageSave+1	; ds 2
SectorBfPoint1	equ	SectorBfPoint0+2	; ds 2
DiskIOZB_Size	equ	SectorBfPoint1+2-DiskIOZB
;=======================================;
;	Devices working buffer.		;
;---------------------------------------;
MotorOnCount	equ	MoveBuffer+256	; db 0
DiskOutStatus	equ	MotorOnCount+1	; db 0
 DiskOK		equ	$00		;
 BadCommand	equ	$01		;
 BadAddressMark	equ	$02		;
 WriteProtect	equ	$03		;
 RecordNotFound equ	$04		;
 BadReset	equ	$05		;
 DriveEmpty	equ	$06		;
 BadDMA		equ	$08		;
 BadSector	equ	$0a		;

 BadCRC         equ     $10             ;
 DataCorrected	equ	$11		;
 BadCntlr	equ	$20		;
 BadSeek	equ	$40		;
 TimeOut	equ	$80		;
 NotRdy		equ	$aa		;
 UndefErr	equ	$bb		;
 WriteFault	equ	$cc		;
 NoError	equ	$e0		;
SeekStatus	equ	DiskOutStatus+1	; db 0
 ;	Bit 1:	Disk 1 has seeked.	;
 ;	Bit 0:	Disk 0 has seeked.	;
MotorStatus	equ	SeekStatus+1	; db 0
 ;	Bit 5:	Disk 1 motor has turned on.
 ;	Bit 4:	Disk 0 motor has turned on.
SDriveMessage	equ	MotorStatus+1	; db 0
 ;	Bit 7:	Floppy disk drive controller is precent.
 ;	Bit 1:	Disk 1 precent.		;
 ;	Bit 0:	Disk 0 precent.		;
MediaMessage	equ	SDriveMessage+1	; ds 2		; Drive 0/1.
DriveMedia	equ	MediaMessage+2	; ds 2		; Drive 0/1.
CurrentTrack	equ	DriveMedia+2	; ds 2		; Drive 0/1.
NECStatus	equ	CurrentTrack+2	;|ds 7
 ;		db	0		; WP
 ;		db	0		; Sectors to read/write.
 ;		db	0		; Sector begin to read/write.
 ;		dw	0		; Track.
 ;		db	0		; ECC/Bytes/Disk/Head.
 ;		db	0		; Command.
;=======================================;
BiosTemp1       equ     NECStatus+7     ; db
BiosTemp2	equ	BiosTemp1+1	; db
BiosTemp3	equ	BiosTemp2+1	; db
BiosTemp4	equ	BiosTemp3+1	; db
BiosTemp5	equ	BiosTemp4+1	; db
BiosTemp6	equ	BiosTemp5+1	; db
GetByteSave	equ	BiosTemp6+1	; db
LowRAMBegin     equ     GetByteSave+1   ; dw; Can use low ram(5800-6000) beginning.
SplitPoint	equ	LowRAMBegin+2	; Splite parameter point.
 ;	Bit 7:	PC card precent.	;
 ;	Bit 0:	PC card has connected.	;
VectorSet	equ	SplitPoint+1	; db
NMIDRAMPage	equ	VectorSet+1	; db
INTDRAMPage	equ	NMIDRAMPage+1	; db
DiskDisplayOff	equ	INTDRAMPage+1	; db
;---------------------------------------;
DispBfHead	equ	DiskDisplayOff+1; db	
DispBfTail	equ	DispBfHead+1	; db
Addr2000N	equ	DispBfTail+1	; db
ScrollPanning	equ	Addr2000N+1	; db
ScrollPanningN	equ	ScrollPanning+1	; db
BRKASave	equ	ScrollPanningN+1; db
;---------------------------------------;
;---------------------------------------; BIOS function command.
;    "*": Position can not change.	;
;	  because used by application.	;
;---------------------------------------;
;Mouse1X	equ	$5feb		;* Mouse 1 current X coordination
;Mouse1Y	equ	$5fec		;* Mouse 1 current Y coordination
;KeyStatus1	equ	$5fed		;*
 ;	Bit 7:	[Insert] status		;
 ;	Bit 6:	[Caps Lock] status	;
 ;	Bit 5:	[Num Lock] status	;
 ;	Bit 4:	[Scroll Lock] status	;
 ;	Bit 3:	[Alt] key pressing.	;
 ;	Bit 2:	[Ctrl] key pressing.	;
 ;	Bit 1:	L-[Shift] key pressing.	;
 ;	Bit 0:	R-[Shift] key pressing.	;
;KeyStatus2	equ	$5fee		;*
 ;	Bit 7:	[Insert] key pressing	;
 ;	Bit 6:	[Caps Lock] key pressing;
 ;	Bit 5:	[Num Locl] key pressing	;
 ;	Bit 4:	[Scroll Lock] key pressing
 ;	Bit 3:	R-[Alt] key pressing	;
 ;	Bit 2:	R-[Ctrl] key pressing	;
 ;	Bit 1:	L-[Alt] key pressing	;
 ;	Bit 0:	L-[Ctrl] key pressing	;
;KeyStatus3	equ	$5fef		;*
 ;	Bit 7:	Keyboard installed.	;
 ;	Bit 6:	Check KB after VSyn.	;
 ;	Bit 4:	101/102 keyboard equit.	;
 ;	Bit 3:	Pause status.		;
 ;	Bit 2:	SysRq key pressing.	;
 ;	Bit 1:	Last $E0 key flag.	;
 ;	Bit 0:	Last $E1 key flag.	;
;KeyStatus4	equ	$5ff0		;*
 ;	Bit 7:	Keyboard transmit error.;
 ;	Bit 2:	Caps Lock LED.		;
 ;	Bit 1:	Num Lock LED.		;
 ;	Bit 0:	Scroll Lock LED.	;
 ;		equ	$5ff1		;
;Mouse0X	equ	$5ff2		;* Mouse 0 current X coordination
;Mouse0Y	equ	$5ff3		;* Mouse 0 current Y coordination
BiosUsePage	equ	$5ff4		;*Bios use DRAM page.(depend on hardware).
VideoPageNumber	equ	$5ff5		;*Video page number in 2K.
;CurColumn	equ	$5ff6		;*Cursor column.
;CurLine	equ	$5ff7		;*Cursor line.
NMIEnterFlag	equ	$5ff8		;*Indicate has enter NMI.
DRAMPageNotUse	equ	$5ff9		;*Never use page change to 5800-7FFF.
DRAMPageSave	equ	$5ffa		;*Current DRAM page real..
DRAMPageHigh	equ	$5ffb		;*Mark bits of DRAM page.
CurrentJoyKey	equ	$5ffc		;*Last joystick key 0/1
 Joystick1Key	equ	$5ffc		; db JostickKey 1 last.
 Joystick2Key	equ	$5ffd		; db JostickKey 2 last.
PCCardMessage	equ	$5ffe		;
VideoCtrlSave	equ	$5fff		;*
;---------------------------------------;
BRKOperation	equ	0		;
FileOperation	equ	1		;
BootOperation	equ	2		;
DebugOperation	equ	3		;
;======= Temparary part ================; Zip length $1800 save to $f000
;=======================================;
;ROMBiosEntry	equ	$5800		; ORG $5800
	jmp	ROMBiosEntryJMP-$C000+$5800
;---------------------------------------;
;FileIOEntry	equ	$5803		; ORG $5803
	jmp	FileIOEntryJMP-$C000+$5800
;---------------------------------------;
;MonitorEntry	equ	$5806		; ORG $5806
;HardResetEntry	equ	$5806		;
	jmp	MonitorEntryJMP-$C000+$5800
;---------------------------------------;
;DeviceEntry	equ	$5809		; ORG $5809
	jmp	DeviceEntryJMP-$C000+$5800
;---------------------------------------;
;DosIOEntry	equ	$580c		; ORG $580c
	jmp	FileIOEntryJMP-$C000+$5800
;---------------------------------------;
;SoftResetEntry	equ	$580f		; ORG $580f
	jmp	SoftResetEntryJMP-$C000+$5800
;---------------------------------------;
;CtrlBreakEntry	equ	$5812		; ORG $5812
	rts				;
	ds	16			;
;---------------------------------------;
SoftResetEntryJMP:			;
	jsr	MapToROM-$C000+$5800	; Set C000-FFFF to ROM.
	jmp	($fffc)			;
;=======================================;
MapToROM:				;
        pha                             ;
        php                             ;
        lda     #$7f                    ;
        and     Addr2000                ;
        jsr	HalW2000T-$C000+$5800                   ;
        lda     #$f7                    ;
        sei                             ;
        and     VideoCtrlSave           ;
        sta     VideoCtrlSave           ;
        jsr	HalVideoCtrl-$C000+$5800           ;
        plp                             ;
        lda     Addr2000                ;
        jsr	HalW2000-$C000+$5800                   ;
        pla                             ;
        rts                             ;
;---------------------------------------;
MapToRAM:				;
        pha                             ;
        php                             ;
        lda     #$7f                    ;
        and     Addr2000                ;
        jsr	HalW2000T-$C000+$5800                   ;
        lda     #$08                    ;
        sei                             ;
        ora     VideoCtrlSave           ;
        sta     VideoCtrlSave           ;
        jsr	HalVideoCtrl-$C000+$5800           ;
        plp                             ;
        lda     Addr2000                ;
        jsr	HalW2000-$C000+$5800                   ;
        pla                             ;
        rts                             ;
;---------------------------------------;
ROMBiosEntryJMP:			;
        php                             ; Save D and I flags.
	cmp	#BiosSetPage		;
	beq	nebej04			;
	cmp	#BiosGetPage		;
	beq	nebej08			;
	cmp	#BiosDevice		;
	beq	nebej12			;
	jsr	MapToROM-$C000+$5800	; Set C000-FFFF to ROM.
	jsr	ROMBios			;
        jsr     MapToRAM-$C000+$5800    ; Set C000-FFFF to RAM.
	plp				; Restore D and I flags.
	rts				;
nebej04:jsr	SetPage-$C000+$5800	;
	plp				;
	rts				;
nebej08:jsr	GetPage-$C000+$5800	;
	plp				;
	rts				;
nebej12:jsr	DeviceEntry		;
	plp				;
	rts				;
;---------------------------------------;
ServerAbsent	equ	$42		;
FileIOEntryJMP:				;
	lda	#ServerAbsent		;
	sec				;
RTSInstruction:				;
	rts				;
;---------------------------------------;
MonitorEntryJMP:			;
	jsr	MapToROM-$C000+$5800	; Set C000-FFFF to ROM.
	jmp	($fffc)			; ~
;---------------------------------------;
NMIVectorAddr	dw	RTSInstruction-$C000+$5800
NMIVectorBranch:			;
	jmp	(NMIVectorAddr-$C000+$5800)
NMIReturn:				;
	pla				;
	sta	DRAMPageSave		;
	jsr	HalPageA-$C000+$5800		;
	lda	VideoCtrlSave		; Map back.
	jsr	HalVideoCtrl-$C000+$5800		;
	pla				;
	rti				;
NMIEntry:				;
	pha				;
	dec	NMIEnterFlag		;
	dec	MotorOnCount		;
	bne	nenas04			;
	lda	#$0c			;
	sta	FDCCtrlPortO		;
	lda	#0			;
	sta	MotorStatus		;
nenas04:bit	$2002			;
	bit	Addr2000		; Original NMI enable?
	bpl	NMIExit			;
	lda	DispBfHead		;
	cmp	DispBfTail		;
	beq	nenas08			;
	lda	VideoCtrlSave		; Map to ROM.
	and	#$f7			;
	jsr	HalVideoCtrl-$C000+$5800		;
	jsr	DisplayNMIROM		;
	lda	VideoCtrlSave		; Map back.
	jsr	HalVideoCtrl-$C000+$5800		;
nenas08:lda	SplitPoint		; No. Has split screen operation?
	beq	nenas12			;
	lda	#0			;
	sta	SplitPoint		;
	lda	VideoCtrlSave		; Map to ROM.
	and	#$f7			;
	jsr	HalVideoCtrl-$C000+$5800		;
	jsr	SwitchBG		;
	lda	VideoCtrlSave		; Map back.
	jsr	HalVideoCtrl-$C000+$5800		;
nenas12:lda	Addr2001		;
	sta	$2001			;
	jsr	NMIVectorBranch-$C000+$5800
	bit	VectorSet		;
	bvc	nenas16			;
	lda	DRAMPageSave		; Save DRAM page.
	pha				;
	lda	#<NMIReturn-$C000+$5800	; Contruct RTI address.
	pha				;
	lda	#>NMIReturn-$C000+$5800	;
	pha				;
	php				;
	lda	NMIDRAMPage		; NMI page.
	sta	DRAMPageSave		;
	jsr	HalPageA-$C000+$5800		;
	lda	VideoCtrlSave		; Map to RAM.
	ora	#8			;
	jsr	HalVideoCtrl-$C000+$5800		;
	bne	NMIAddress	;(jmp)	;
nenas16:pla				;
NMIAddress:				;
NMIAddressSave	equ	NMIAddress+1	;
	jmp	RTIReturn-$C000+$5800	;
;---------------------------------------;
SplitScreenINT:				;
	pha				; Save A.
SEndAddressAdd:				;
SEndAddressSave	equ	SEndAddressAdd+1;
	jsr	RTSInstruction-$C000+$5800
NMIExit:pla				; Restore A.
RTIReturn:				; ??? This position must in 5800-58ff, WHY?
	rti				;
;---------------------------------------;
SEndAccessRAM:				;
	tya				;
	pha				;
	lda	DRAMPageSave		; Save DRAM page.
	pha				;
	lda	INTDRAMPage		; NMI page.
	sta	DRAMPageSave		;
	jsr	HalPageA-$C000+$5800		;
	lda	VideoCtrlSave		; Map to RAM.
	pha				;
	ora	#8			;
	sta	VideoCtrlSave		;
	jsr	HalVideoCtrl-$C000+$5800		;
SendAccessAdd:				;
SEndAccessSave	equ	SendAccessAdd+1	;
	jsr	RTSInstruction-$C000+$5800
	lda	#$f7			;
	and	VideoCtrlSave		; Map back.
	sta	VideoCtrlSave		;
	pla				;
	and	#8			;
	ora	VideoCtrlSave		;
	sta	VideoCtrlSave		;
	jsr	HalVideoCtrl-$C000+$5800		;
	pla				;
	sta	DRAMPageSave		;
	jsr	HalPageA-$C000+$5800		;
	pla				;
	tay				;
	rts				;
;---------------------------------------;
BRKINTEntry:				;
	sta	BRKASave		;|4 Save A.
	pla				;|4 Flag.
	pha				;|3
	and	#$10			;|2 Check if BRK or INT?
	jsr	HalKbdIrq-$C000+$5800	; SB2000 keyboard IRQ.
	bcc	nebrk04			;
	lda	BRKASave		;
	rti				;
nebrk04:        lda     BRKASave                ;|4 INT. Restore A.
BRKAddress:				;|0
BRKAddressSave	equ	BRKAddress+1	;|0
	jmp	RTIReturn-$C000+$5800	;|3 = 28.
;---------------------------------------;
;---------------------------------------;
CallToRAM:				;
	jsr	MapToRAM-$C000+$5800	; Set C000-FFFF to RAM.
CallAddress:				;
	jsr	RTSInstruction-$C000+$5800
	jmp	MapToROM-$C000+$5800	; Set C000-FFFF to ROM.
;=======================================;
;    Set page				;
;	(X)= Page number		;
;	Save	A,Y.			;
;---------------------------------------;
SetPage:cpx	BiosUsePage		;
	bne	nespe04			;
	ldx	DRAMPageNotUse		;
nespe04:cpx	#0			;
	bne	nespe08			;
	ldx	BiosUsePage		;
nespe08:stx	DRAMPageSave		;
	jsr	HalPageX-$C000+$5800		;
	rts				;
;=======================================;
;    Set page				;
;	(A)= Page number		;
;	Must save X,Y.			;
;---------------------------------------;
GetPage:lda	DRAMPageSave		;
	cmp	BiosUsePage		;
	bne	negpe04			;
	lda	#0			;
negpe04:cmp	DRAMPageNotUse		;
	bne	negpe08			;
	lda	BiosUsePage		;
negpe08:rts				;
;=======================================;
DeviceEntryJMP:                         ;
	cpx	#1			; Check keyboard?
	bne	nekbj04			;
;---------------------------------------; (X)=1. Test keyboard.
TestKeyBoardDev:			;
lptkd04:ldy	KBDHead			; Has any key in keyboard?
	cpy	KBDTail			;
	beq	netkd04			;
	lda	KBDBuffer,y		; Yes.
	ldx	KBDBuffer+1,y		;
	ldy	#1			;
	rts				;
netkd04:bit	NMICheckFlag		;
	bvs	nekbj04			;
	ldy	#0			;
	rts				;
nekbj04:jsr	MapToROM-$C000+$5800	;
        jsr     DeviceEntryROM          ;
	jmp	MapToRAM-$C000+$5800	;
;---------------------------------------;
KeyBoardNMI:				;
	bit	NMICheckFlag		;
	bmi	nekbn04			;
KeyBoardNMIDo:				;
	lda	VideoCtrlSave		; Map to ROM.
	and	#$f7			;
	jsr	HalVideoCtrl-$C000+$5800		;
	jsr	KeyBoardNMIROM		;
	lda	VideoCtrlSave		; Map back.
	jsr	HalVideoCtrl-$C000+$5800		;
nekbn04:rts				;
;---------------------------------------;
PauseRunningRAM:			;
	lda	#8			;
lpprg04:bit	KeyStatus3		;
	bne	lpprg04			;
	jmp	MapToROM-$C000+$5800	; Set C000-FFFF to ROM.
;---------------------------------------;
CtrlBreakEntryRAM			;
	jsr	CtrlBreakEntry		;
	jmp	MapToROM-$C000+$5800	; Set C000-FFFF to ROM.
	include	hal-resident.asm
MoveRAMEnding:				;

db	"系统制作:清华大学计算机系唐瑞春"
db	"written by Tang ruichun Tsinghua University Computer Department"
;---------------------------------------;
ConnectPCCard:				;
	lda	#0			; PC link not supported on SB2000.
	rts				;
lpccc04:lda	#$1a			; Set P-ERROR.(Receive end)
	sta	PCDaCtPortO		; and Output indecate byte($aa).
lpccc08:ldx	#20			; Wait code:$ff 20 times.
lpccc12:lda	#25			; Set timeout count.
	sta	NMIEnterFlag		;
lpccc16:lda	NMIEnterFlag		; Timeout?
	beq	neccc08			;
	lda	PCCtrlPortI		;
	and	#8			; P-STB is low?(Data ready)
	bne	lpccc16			;
	lda	PCDataPortIH		; Yes.
	and	#$f0			;
	sta	GetByteSave		;
	lda	#$0a			; Reset P-ERROR.(Data received)
	sta	PCDaCtPortO		;
	lda	PCDataPortIL		;
	and	#$0f			;
	ora	GetByteSave		;
	eor	#$8b			;
	tay				; Is indecate byte(aa).
lpccc20:lda	NMIEnterFlag		;
	beq	neccc08			;
	lda	PCCtrlPortI		; P-STB is high?(Transmission end)
	and	#8			;
	beq	lpccc20			;
	lda	#$1a			; Yes. Set P-ERROR.(Receive end)
	sta	PCDaCtPortO		;
	tya				;
	cmp	#$aa			;
	bne	lpccc08			;
	dex				;
	bne	lpccc12			;
	lda	#$55			; Send a end char.
	jsr	SendByte		;
	jsr	GetByte			;
	cmp	#$55			; Receive a end byte?
	bne	lpccc04			;
	lda	#$5a			;
	jsr	SendByte		;
	lda	#1			; Yes.
	rts				;
neccc08:lda	#$10			; Set P-ERROR.(Receive end)
	sta	PCDaCtPortO		;
	lda	#0			;
	rts				;
;---------------------------------------;
PaletteTable	db	$0e		;
		db	$00		;
		db	$12		; Background (Blue).
		db	$20		; Normal display.
PaletteLength	equ	@-PaletteTable	;
InitialPalette:                         ;
	ldx	#<$3f00			;
	ldy	#>$3f00			;
	jsr	VRAMAddress		;
  ;	ldy	#0			;
lpipe04:lda	PaletteTable,y		;
	jsr	HalW2007			;
	iny				;
	cpy	#PaletteLength		;
	bne	lpipe04			;
	lda	#0			;
	jsr	HalW2006			;
	jsr	HalW2006			;
	rts				;
;---------------------------------------;
WaitVSyn:				; Wait Vsyn.
lpwvs04:bit	$2002			;
	bmi	lpwvs04			;
lpwvs08:bit	$2002			;
	bpl	lpwvs08			;
	rts				;
;---------------------------------------;
DisplayStringInit:			;
	stx	$ff			;
	sta	$fe			;
	lda	$fc			;
	asl				;
	asl				;
	asl				;
	asl				;
	php				;
	asl				;
	ora	$fe			;
	sty	$fe			;
	tay				;
	lda	#$10			; $20/2
	ror				;
	plp				;
	rol				;
	rol				;
	tax				;
	rts				;
;---------------------------------------;
CleanDisplayStg:			;
	jsr	DisplayStringInit	;
	pha				;
	tya				;
	pha				;
	and	#$e0			; Point to line beginning.
	tay				;
	jsr	WaitVSyn		;
	jsr	VRAMAddress		;
	ldy	#64			; Clear to line.(8 bit high)
	lda	#$6a			;
lpcds04:jsr	HalW2007			;
	dey				;
	bne	lpcds04			;
	pla				; Restore Y.
	pha				;
	tay				;
	jmp	DisplayStringINT	;
;---------------------------------------;
;	Input	X|Y String address	;
;		($fc)= Line.		;
;		(A)=   Column.		;
;---------------------------------------;
DisplayString:				;
	jsr	DisplayStringInit	; X|Y= VRAM address.
	pha				;
	tya				;
	pha				;
	jsr	WaitVSyn		;
DisplayStringINT:			;
	jsr	VRAMAddress		;
	ldy	#0			;
lpdsg04:lda	($fe),y			;
	iny				;
	cmp	#$ff			;
	beq	nedsg04			;
	jsr	HalW2007			;
	bne	lpdsg04			;
nedsg04:pla				;
	clc				;
	adc	#$20			;
	tay				;
	pla				;
	adc	#0			;
	tax				;
	jsr	VRAMAddress		;
	ldy	#0			;
lpdsg08:lda	($fe),y			;
	iny				;
	cmp	#$ff			;
	beq	nedsg08			;
	adc	#1			;
	jsr	HalW2007			;
	bne	lpdsg08			;
nedsg08:
;	jsr	OpenScreen		;
;---------------------------------------;
OpenScreen:				;
	lda	#0			;
	sta	$2005			;
	sta	$2005			;
	lda	#$0e			;
	sta	$2001			;
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
DivXY_A:sty	$ff			;
	sta	$fe			;
	txa				;
	ldy	#8			;
lpdxy04:rol	$ff			;
	rol	a			;
	cmp	$fe			;
	bcc	nedxy04			;
	sbc	$fe			;
nedxy04:dey				;
	bne	lpdxy04			;
	tax				;
	lda	$ff			;
	rol	a			;
	rts				;
;---------------------------------------;
CheckRAMSub:				;
	ldx	#0			;
	lda	#10			;
	jsr	DivXY_A			;
	bne	necrs04			;
	bcs	necrs08			;
necrs04:ldy	$80			;
	asl				;
	sta	$81,y			;
	inc	$80			;
necrs08:ldy	$80			;
	txa				;
	asl				;
	sta	$81,y			;
	inc	$80			;
	rts				; C=0.	
;---------------------------------------;
;	Input	X|Y = Data.		;
;---------------------------------------;
DisplayChkRAM:				;
	pha				;
	lda	#100			;
	sec				;
	jsr	DivXY_A			;
	beq	nedcm04			;
	tay				;
	txa				; Save X.
	pha				;
	jsr	CheckRAMSub		;
	pla				;
	bcc	nedcm08			;
nedcm04:txa				;
nedcm08:tay				;
	jsr	CheckRAMSub		;
	ldx	$80			;
	pla				; "K,M"
	sta	$81,x			;
	lda	#$ff			; String ending.
	sta	$81+1,x			;
	ldx	#<$0081			;
	ldy	#>$0081			;
	lda	$fd			;
	jmp	DisplayString		;
;---------------------------------------;
FontTable:
db $00,$00,$00,$00,$00,$7C,$C6,$C6,$CE,$D6,$D6,$E6,$C6,$C6,$7C,$00 ; 00 "0"
db $00,$00,$00,$00,$00,$18,$38,$78,$18,$18,$18,$18,$18,$18,$7E,$00 ; 02 "1"
db $00,$00,$00,$00,$00,$7C,$C6,$06,$0C,$18,$30,$60,$C0,$C6,$FE,$00 ; 04 "2"
db $00,$00,$00,$00,$00,$7C,$C6,$06,$06,$3C,$06,$06,$06,$C6,$7C,$00 ; 06 "3"
db $00,$00,$00,$00,$00,$0C,$1C,$3C,$6C,$CC,$FE,$0C,$0C,$0C,$1E,$00 ; 08 "4"
db $00,$00,$00,$00,$00,$FE,$C0,$C0,$C0,$FC,$0E,$06,$06,$C6,$7C,$00 ; 0a "5"
db $00,$00,$00,$00,$00,$38,$60,$C0,$C0,$FC,$C6,$C6,$C6,$C6,$7C,$00 ; 0c "6"
db $00,$00,$00,$00,$00,$FE,$C6,$06,$06,$0C,$18,$30,$30,$30,$30,$00 ; 0e "7"
db $00,$00,$00,$00,$00,$7C,$C6,$C6,$C6,$7C,$C6,$C6,$C6,$C6,$7C,$00 ; 10 "8"
db $00,$00,$00,$00,$00,$7C,$C6,$C6,$C6,$7E,$06,$06,$06,$0C,$78,$00 ; 12 "9"
db $00,$00,$00,$00,$00,$10,$38,$6C,$C6,$C6,$FE,$C6,$C6,$C6,$C6,$00 ; 14 "A"
db $00,$00,$00,$00,$00,$FC,$66,$66,$66,$7C,$66,$66,$66,$66,$FC,$00 ; 16 "B"
db $00,$00,$00,$00,$00,$00,$00,$18,$18,$00,$00,$00,$18,$18,$00,$00 ; 18 ":"
db $00,$00,$00,$00,$00,$C6,$EE,$FE,$FE,$D6,$C6,$C6,$C6,$C6,$C6,$00 ; 1a "M"
db $00,$00,$00,$00,$00,$E6,$66,$6C,$6C,$78,$78,$6C,$66,$66,$E6,$00 ; 1c "K"
db $01,$01,$01,$01,$7F,$41,$41,$42,$42,$44,$48,$50,$40,$40,$40,$40 ; 1e 内 
db $00,$00,$00,$04,$FE,$04,$04,$04,$84,$64,$34,$14,$04,$04,$14,$08 ; 20    
db $00,$1F,$10,$10,$1F,$10,$10,$1F,$14,$44,$34,$14,$04,$04,$FF,$00 ; 22 显
db $10,$F8,$10,$10,$F0,$10,$10,$F0,$50,$44,$4C,$50,$40,$44,$FE,$00 ; 24 
db $02,$02,$FF,$04,$08,$0B,$10,$10,$30,$5F,$90,$10,$10,$10,$11,$10 ; 26 存
db $00,$04,$FE,$00,$00,$F8,$10,$20,$44,$FE,$40,$40,$40,$40,$40,$80 ; 28
db $10,$10,$10,$FE,$20,$29,$48,$7E,$08,$08,$0E,$F8,$48,$08,$09,$0A ; 2a 软
db $40,$40,$40,$7C,$84,$08,$20,$20,$20,$20,$50,$50,$88,$88,$06,$04 ; 2c
db $08,$FD,$09,$49,$49,$49,$49,$49,$7D,$05,$05,$1D,$E5,$45,$15,$08 ; 2e 驱
db $04,$FE,$00,$44,$44,$28,$28,$10,$10,$28,$28,$44,$84,$00,$FE,$00 ; 30
db $02,$02,$03,$02,$02,$02,$FF,$02,$02,$02,$02,$02,$02,$02,$02,$02 ; 32 卡
db $00,$20,$F0,$00,$00,$04,$FE,$00,$00,$80,$60,$30,$10,$00,$00,$00 ; 34
db $12,$12,$2A,$4A,$8A,$1F,$20,$7F,$A0,$2F,$29,$29,$29,$29,$30,$20 ; 36 微
db $10,$10,$90,$A0,$BE,$C4,$24,$A8,$28,$28,$10,$50,$A8,$28,$46,$84 ; 38 
db $10,$10,$11,$11,$FD,$11,$31,$39,$55,$51,$91,$11,$11,$12,$14,$18 ; 3a 机
db $00,$10,$F8,$10,$10,$10,$10,$10,$10,$10,$10,$10,$12,$12,$0E,$00 ; 3c 
db $03,$40,$30,$10,$03,$02,$F2,$13,$12,$12,$13,$12,$12,$2A,$44,$03 ; 3e 通
db $F8,$10,$A0,$40,$F8,$48,$48,$F8,$48,$48,$F8,$48,$68,$50,$00,$FE ; 40 
db $00,$47,$30,$11,$01,$01,$F7,$11,$11,$11,$11,$11,$11,$29,$44,$03 ; 42 迅
db $10,$F8,$10,$10,$10,$10,$F0,$10,$10,$10,$12,$0A,$06,$00,$00,$FE ; 44 
db $40,$31,$11,$01,$81,$62,$24,$03,$12,$21,$E1,$20,$20,$20,$23,$2C ; 46 没
db $10,$F8,$10,$10,$10,$0E,$00,$F8,$08,$10,$10,$A0,$40,$A0,$1E,$08 ; 48 
db $00,$40,$2F,$21,$01,$02,$E7,$20,$20,$20,$2F,$20,$20,$50,$8F,$00 ; 4a 连
db $80,$88,$FC,$00,$40,$40,$F8,$40,$40,$48,$FC,$40,$40,$40,$FE,$00 ; 4c 
db $00,$7F,$02,$04,$08,$3F,$01,$06,$18,$3F,$11,$01,$11,$21,$45,$02 ; 4e 系
db $38,$C0,$00,$20,$40,$80,$00,$20,$10,$F8,$08,$00,$10,$08,$08,$00 ; 50 
db $10,$10,$20,$27,$48,$F9,$12,$27,$41,$F9,$41,$01,$1A,$E2,$44,$08 ; 52 统
db $80,$40,$48,$FC,$80,$10,$08,$FC,$24,$20,$20,$20,$22,$22,$1E,$00 ; 54   
db $10,$10,$10,$10,$FD,$12,$35,$30,$58,$54,$91,$11,$11,$10,$17,$10 ; 56 检
db $40,$40,$A0,$A0,$10,$08,$F6,$04,$00,$48,$48,$50,$10,$04,$FE,$00 ; 58 
db $00,$40,$37,$14,$85,$65,$25,$0D,$15,$15,$65,$21,$22,$24,$28,$20 ; 5a 测
db $04,$04,$C4,$54,$54,$54,$54,$54,$54,$54,$54,$04,$84,$44,$54,$08 ; 5c 
db $01,$00,$1F,$10,$10,$10,$1F,$10,$10,$10,$17,$24,$24,$44,$87,$04 ; 5e 启
db $00,$84,$FE,$04,$04,$04,$FC,$00,$00,$04,$FE,$04,$04,$04,$FC,$04 ; 60 
db $00,$00,$7C,$00,$01,$FE,$10,$10,$20,$24,$42,$FE,$42,$01,$02,$04 ; 62 动
db $40,$40,$40,$44,$FE,$44,$44,$44,$44,$44,$84,$84,$84,$04,$28,$10 ; 64 
db $00,$08,$08,$08,$08,$08,$08,$08,$08,$08,$14,$12,$21,$21,$42,$84 ; 66 从
db $00,$20,$20,$20,$20,$20,$20,$50,$50,$50,$50,$88,$88,$0C,$06,$04 ; 68
db $00,$00,$00,$00,$00,$00,$00,$00,$18,$3c,$3c,$18,$00,$00,$00,$00 ; 6a "."
db $01,$11,$11,$11,$1F,$21,$41,$01,$FF,$01,$02,$02,$04,$08,$10,$60 ; 6c 失
db $00,$00,$00,$10,$F8,$00,$00,$04,$FE,$00,$80,$80,$40,$30,$0E,$04 ; 6e 
db $00,$7C,$44,$54,$54,$55,$56,$54,$54,$54,$54,$10,$28,$24,$45,$82 ; 70 败
db $40,$40,$44,$FE,$88,$08,$88,$88,$88,$50,$50,$20,$60,$90,$0E,$04 ; 72 
db $04,$1F,$10,$12,$11,$FF,$10,$12,$21,$40,$3F,$24,$24,$24,$FF,$00 ; 74 盘
db $10,$F8,$10,$10,$10,$FE,$10,$10,$50,$20,$F8,$48,$48,$48,$FE,$00 ; 76 
db $10,$10,$10,$13,$FA,$14,$10,$1B,$30,$D0,$11,$10,$10,$10,$50,$23 ; 78 按
db $40,$20,$00,$FE,$02,$44,$40,$FE,$88,$88,$10,$90,$60,$50,$8C,$04 ; 7a 
db $08,$08,$0B,$10,$10,$30,$50,$9F,$10,$10,$10,$10,$10,$17,$10,$10 ; 7c 任
db $00,$18,$E0,$40,$40,$40,$44,$FE,$40,$40,$40,$40,$48,$FC,$00,$00 ; 7e 
db $01,$3F,$08,$04,$FF,$00,$1F,$10,$1F,$10,$1F,$02,$51,$50,$90,$0F ; 80 意
db $00,$F8,$20,$44,$FE,$00,$F0,$10,$F0,$10,$F0,$00,$84,$92,$12,$F0 ; 82 
db $20,$27,$39,$41,$82,$7A,$24,$27,$F9,$21,$25,$22,$2A,$35,$28,$00 ; 84 键
db $10,$10,$7C,$14,$FE,$14,$7C,$10,$7C,$10,$10,$FE,$10,$10,$90,$7E ; 86 
db $10,$10,$17,$10,$F8,$17,$10,$19,$36,$D4,$14,$17,$14,$14,$57,$24 ; 88 插
db $08,$3C,$C0,$40,$44,$FE,$40,$40,$5C,$44,$44,$5C,$44,$44,$FC,$04 ; 8a 
db $04,$02,$01,$01,$01,$02,$02,$02,$04,$04,$08,$08,$10,$20,$40,$80 ; 8c 入
db $00,$00,$00,$00,$00,$80,$80,$80,$40,$40,$20,$20,$10,$10,$0E,$04 ; 8e 
db $00,$00,$00,$FF,$00,$00,$00,$00
SMDOS_String	db	"SMDOS"
ExamineLine	equ	7
SysExamStg	db	$4e,$50,$52,$54,$56,$58,$5a,$5c,$ff		; 系统检测
MainMemoryStg	db	$1e,$20,$26,$28,$18				; 内存:
VideoMemoryStg	db	$22,$24,$26,$28,$18				; 显存:
FDCStg		db	$2a,$2c,$2e,$30,$32,$34,$ff			; 软驱卡
FDDAStg		db	$2a,$2c,$2e,$30,$18,$14,$ff			; 软驱:A
FDDBStg		db	$2a,$2c,$2e,$30,$18,$16,$ff			; 软驱:B
NOFDDStg	db	$46,$48,$2a,$2c,$2e,$30,$ff			; 没软驱
PCCStg		db	$36,$38,$3a,$3c,$3e,$40,$42,$44,$32,$34,$ff	; 微机通讯卡
PCCConnectStg	db	$46,$48,$4a,$4c,$3e,$40,$36,$38,$3a,$3c,$ff	; 没连通微机
BootLine	equ	21
SysBootStg	db	$4e,$50,$52,$54,$5e,$60,$62,$64,$ff		; 系统启动
BootFDCAStg	db	$66,$68,$2a,$2c,$2e,$30,$14,$5e,$60,$62,$64	; 从软驱A启动...
		db	$6a,$6a,$6a,$ff
BootFDCBStg	db	$66,$68,$2a,$2c,$2e,$30,$16,$5e,$60,$62,$64	; 从软驱B启动...
		db	$6a,$6a,$6a,$ff
BootFDCErrStg	db	$2a,$2c,$2e,$30,$88,$8a,$8c,$8e,$4e,$50,$52,$54	; 软驱插入系统盘
		db	$74,$76,$ff
PressKeyStg	db	$78,$7a,$7c,$7e,$80,$82,$84,$86,$ff		; 按任意键
BootPCCStg	db	$66,$68,$36,$38,$3a,$3c,$3e,$40,$42,$44,$32,$34	; 从微机通讯卡启动...
		db	$5e,$60,$62,$64,$6a,$6a,$6a,$ff
BootFault	db	$4e,$50,$52,$54,$5e,$60,$62,$64			; 系统启动失败
		db	$6c,$6e,$70,$72,$ff
;---------------------------------------;
InitialScreen:				;
	ldx	#<$0000			;
	ldy	#>$0000			;
	jsr	VRAMAddress		;
;---------------------------------------; Initial Background font.
	ldx	#<FontTable		;
	ldy	#>FontTable		;
	stx	$ff			;
	sty	$fe			;
	lda	#>$91			;
	sta	$fd			;
	ldy	#0			;
lpibf04:ldx	#8			;
lpibf08:lda	($fe),y			;
	iny				;
	jsr	HalW2007			;
	dex				;
	bne	lpibf08			;
	lda	#$ff			;
	ldx	#8			;
lpibf12:jsr	HalW2007			;
	dex				;
	bne	lpibf12			;
	tya				;
	bne	neibf04			;
	inc	$ff			;
neibf04:dec	$fd			;
	bne	lpibf04			;
;---------------------------------------; Clear VRAM.
	ldx	#<$2000			;
	ldy	#>$2000			;
	jsr	VRAMAddress		;
	ldx	#4			; Clear 2000-23bf.
	lda	#$6a			;
	ldy	#$c0			;
lpivm04:jsr	HalW2007			;
	dey				;
	bne	lpivm04			;
	dex				;
	bne	lpivm04			;
	lda	#0			; Clear 23c0-23ff.
	ldy	#$40			;
lpivm08:jsr	HalW2007			;
	dey				;
	bne	lpivm08			;
	rts				;
;=======================================;
Begin:	lda	#0			; Disable NMI.
	jsr	HalInit			;
	sta	Addr2000		;
	sta	$2001			;
	sta	$4015			;
	sei				;
	cld				;
	ldx	#$ff			;
	txs				;
	nop
	nop
	nop
	dex				;
	nop
	nop
	nop
	lda	#0			;
	sta	PCDaCtPortO		; Parallel port initial.
	sta	vDummy			; For printer initial.
	jsr	HalVideoCtrl		;
;---------------------------------------; Check VRAM.(32K fixed)
	lda	#$f0			; 16 x 2K pages.
necvm08:eor	#$ff			;
	clc				;
	adc	#1			; Video page size.
	pha				; Save Video page size.
	ldx	#4			;
	jsr	HalSlot0X		;
	inx				;
	jsr	HalSlot1X		;
	jsr	WaitVSyn		;
	jsr	InitialPalette		;
	jsr	InitialScreen		;
	ldx	#<ExamineLine*$20+$40+3+$2000
	ldy	#>ExamineLine*$20+$40+3+$2000
	jsr	VRAMAddress		;
	ldx	#26			;
	lda	#$90			;
lpbeg12:jsr	HalW2007			;
	dex				;
	bne	lpbeg12			;
	jsr	OpenScreen		;
	lda	#ExamineLine		;
	sta	$fc			;
	ldx	#<SysExamStg		;
	ldy	#>SysExamStg		;
	lda	#12			;
	jsr	DisplayString		;
	lda	#ExamineLine+3		;
	sta	$fc			;
;---------------------------------------;
	lda	#$40			; Hard reset FDC chip.
	sta	FDCResetPortO		;
;---------------------------------------; Check PRAM.
	ldx	#5-1			;
lpcpm04:lda	MainMemoryStg,x		; Set string.
	sta	$81,x			;
	dex				;
	bpl	lpcpm04			;
	lda	#5			; Column.
	sta	$fd			;
	lda	#$80			;
lpcpm08:jsr	HalPageA		; Set page: 00,80,C0,E0,F0,F8,FC,FE,FF.
	ldx	#0			;
lpcpm12:sta	$9a00,x			;
	inx				;
	bne	lpcpm12			;
	sec				;
	ror				;
	bcc	lpcpm08			;
	lda	#0			; Set page to 00.
lpcpm16:ldx	#0			;
	jsr	HalPageA		;
lpcpm20:cmp	$9a00,x			; Page data same?
	bne	necpm04			;
	inx				;
	bne	lpcpm20			;
	beq	necpm08			; Yes.
necpm04:sec				;
	ror				;
	bcc	lpcpm16			;
;	bcs	DRAMCheckError		; Program Memory Error.
necpm08:sta	$02			; (!(A)+1)*16k= Memory size.
	pha				; DRAM page size.
	sta	$03			;
	lda	#0			;
	sta	FDCResetPortO		; Hard un-reset FDC chip.
;	lda	#>$8000			; Fill part of DRAM(256 Byte each).
	sta	$00			; ($00) point to $8000.
lpcpm24:lda	#5			;
	sta	$80			;
	lda	#0			;
	sta	$ff			;
	lda	$02			;
	sec				;
	sbc	$03			;
	ldx	#4			;
lpcpm28:asl				; * 16.
	rol	$ff			;
	dex				;
	bne	lpcpm28			;
	clc				;
	adc	#16			;
	tay				;
	lda	$ff			;
	adc	#0			;
	tax				;
	lda	#$1c			; "K"
	jsr	DisplayChkRAM		;
	lda	#<$8000			;
	sta	$01			;
	lda	$02			;
	jsr	HalPageA		;
	ldx	#$18			; Clear 8000-9800.
	ldy	#0			;
lpcpm32:sta	($00),y			;
	iny				;
	bne	lpcpm32			;
	inc	$01			;
	dex				;
	bne	lpcpm32			;
	inc	$02			;
	bne	lpcpm24			;
	lda	#$08			; Soft reset FDC.
	sta	FDCCtrlPortO		;
;---------------------------------------;
	ldx	#<$5800			; Clear $5800-5FFF.
	ldy	#>$5800			;
	stx	$00+1			;
	sty	$00			;
	ldx	#<$6000-$5800		;
;	ldy	#0			;
	tya				;
lpmpg08:sta	($00),y			;
	iny				;
	bne	lpmpg08			;
	inc	$01			;
	dex				;
	bne	lpmpg08			;
	lda	#<MoveRAMEnding-$C000+$5800
	sta	$01			; Move program to $5800.
	lda	#<MoveRAMEnding		;
	sta	$03			;
	lda	#0			;
	sta	$00			;
	sta	$02			;
	ldx	#<MoveRAMEnding-$C000	;
	ldy	#>MoveRAMEnding		;
lpmpg12:dey				; y==0?
	cpy	#$ff			;
	bcc	nempg04			;
	dec	$01			;
	dec	$03			;
	dex				;
	bmi	nempg08			;
	clc				;
nempg04:lda	($02),y			; No.
	sta	($00),y			;
	bcc	lpmpg12	;(jmp)		;
nempg08:ldx	#<MoveRAMEnding-$C000+$5800
	ldy	#>MoveRAMEnding-$C000+$5800
	stx	LowRAMBegin+1		;
	sty	LowRAMBegin		;
	pla				; DRAM page numbers.
	sta	DRAMPageHigh		;
	sta	DRAMPageNotUse		; Set 1th page to never used default.
					; (Set protect page for 5800-7FFF.)
	pla				; VRAM page numbers.
	sta	$200			; = VideoPageNumber.
	sta	VideoPageNumber		;
	bit	FDCIRQPortI		; Change V flag bit.
	php				;
	lda	#$0c			; Soft un-reset FDC.
	sta	FDCCtrlPortO		;
	ldy	#0
lpppp:	dex
	bne	lpppp
	dey
	bne	lpppp
;---------------------------------------;
	ldx	#5-1			; Display VRAM.
lpcrm04:lda	VideoMemoryStg,x	;
	sta	$81,x			;
	dex				;
	bpl	lpcrm04			;
	ldy	#19			; Column.
	sty	$fd			;
	lda	#0			;
lpcrm08:pha				;
	ldx	#5			;
	stx	$80			;
	ldx	#0			;
	stx	$ff			;
	clc				;
	adc	#1			;
	asl				; * 2.
	tay				;
	lda	#0			;
	rol				;
	tax				;
	cpy	#100			;
	bcc	necrm04			;
	lda	#19-1			;
	sta	$fd			;
necrm04:lda	#$1c			; "K"
	jsr	DisplayChkRAM		;
	pla				;
	clc				;
	adc	#1			;
	cmp	$200			;
	bne	lpcrm08			;
	lda	#ExamineLine+6		;
	sta	$fc			;
;---------------------------------------;
	jsr	ResetKeyBoardDev	; Reset keyboard.
;---------------------------------------;
necfc04:plp				; Has IRQ during FDC reset?
	bvs	necfc08			; Yes. No FDC.
	bit	FDCIRQPortI		; After FDC reset, FDC create IRQ?
	bvs	necfc12			;
necfc08:jmp	necdc04			;
necfc12:ldx	#<FDCStg		;
	ldy	#>FDCStg		;
	lda	#5			;
	jsr	DisplayString		;
	lda	#$80			;
	pha				;
	lda	#0			; Yes. FDC is precent.
	sta	DiskIOZB+Drive		; Disk 0 seek.
	jsr	OpenMotor		; Turn on disk 0 motor.
	jsr	SoftDiskReset		;
	lda	#50			; Seek to 50 track.
	jsr	Seek			;
	bcs	necfc16			;
	jsr	WaitmSecond		; Wait 50ms.
	lda	#0			; Seek to 0 track.
	jsr	Seek			;
        bcs     necfc16                 ; ????
	ldx	#<FDDAStg		;
	ldy	#>FDDAStg		;
	lda	#14			;
	jsr	DisplayString		;
	pla				;
	ora	#1			;
	pha				;
necfc16:ldx	#1			; Disk 1 seek.
	stx	DiskIOZB+Drive		;
	dex				;
	stx	MotorStatus		;
	jsr	OpenMotor		; Turn on disk 0 motor.
	lda	#50			; Seek to 50 track.
	jsr	Seek			;
	bcs	necfc20			;
	jsr	WaitmSecond		; Wait 50ms.
	lda	#0			; Seek to 0 track.
	jsr	Seek			;
	bcs	necfc20			;
	ldx	#<FDDBStg		;
	ldy	#>FDDBStg		;
	lda	#21			;
	jsr	DisplayString		;
	pla				;
	ora	#2			;
	pha				;
necfc20:pla				;
	sta	SDriveMessage		;
	and	#3			; Has any diskette drive?
	bne	necfc24			;
	ldx	#<NOFDDStg		;
	ldy	#>NOFDDStg		;
	lda	#21			;
	jsr	DisplayString		;
necfc24:inc	$fc			;
	inc	$fc			;
	inc	$fc			;
	lda	#0			;
	sta	MotorStatus		;
	lda	#$0c			; Turn off motor.
	sta	FDCCtrlPortO		;
;---------------------------------------; Check PC card.
necdc04:lda	vDummy			; PC card exist?
	ldx	#$0c			;
	stx	vDummy			;
	and	#$0f			;
	bne	necdc08			;
	lda	vDummy			;
	and	#$0f			;
	cmp	#$0c			;
;	beq	necpc20			;
necdc08:lda	#$04			; Enable PCDataPortIL input enable.
	sta	vDummy			;
	ldx	#<PCCStg		; Yes.
	ldy	#>PCCStg		;
	lda	#5			;
	jsr	DisplayString		;
	jsr	ConnectPCCard		; Return (A)=1: OK.
	tax				;
	beq	necpc12			;
	ldx	#<PCCConnectStg+2	;
	ldy	#>PCCConnectStg+2	;
	lda	#17+2			;
	jmp	necpc16			;
necpc12:ldx	#<PCCConnectStg		;
	ldy	#>PCCConnectStg		;
	lda	#17			;
necpc16:jsr	DisplayString		;
	lda	#$80			;
	sta	PCCardMessage		;
	jmp	nebeg04			;
necpc20:
;---------------------------------------;
nebeg04:lda	#$ff			;
	jsr	HalPageA		;
	sta	DRAMPageSave		;
	lda	#$fe			;
	sta	BiosUsePage		;
	lda	#BootLine		;
	sta	$fc			;
	ldx	#<SysBootStg		;
	ldy	#>SysBootStg		;
	lda	#12			;
	jsr	DisplayString		;
	jsr	WaitVSyn		;
	ldx	#<BootLine*$20+$40+3+$2000
	ldy	#>BootLine*$20+$40+3+$2000
	jsr	VRAMAddress		;
	ldx	#26			;
	lda	#$90			;
lpbeg16:jsr	HalW2007			;
	dex				;
	bne	lpbeg16			;
;	ldx	#0			; Set VROM paging.
	stx	Addr2005		;
        jsr     RunInitial              ; (Has change NMI,INT vector, enable NMI).
	ldx	#<KeyBoardNMI-$C000+$5800
	ldy	#>KeyBoardNMI-$C000+$5800
	jsr	SetNMIVector		;
	jsr	OpenScreen		;
	lda	#$0e			;
	sta	Addr2001		;
;=======================================;
;    Boot from RDisk-FDisk-HDisk.	;
;---------------------------------------; Boot.
SystemBoot:				;
	jsr	SoundBell		;
BootDiskIOBF	equ	$0080		;
BootDiskSBF	equ	$0400		;
;---------------------------------------; Boot from remote disk.
lpbot04:lda	#$c0			; No NMI keyboard access.
	sta	NMICheckFlag		;
	lda	PCCardMessage		;
	bpl	nebot12			;
	lda	#BootLine+3		;
	sta	$fc			;
	ldx	#<BootPCCStg		;
	ldy	#>BootPCCStg		;
	lda	#7			;
	jsr	CleanDisplayStg		;
	jsr	ConnectPCCard		; Return (A)=1: OK.
	tax				;
	beq	nebot12			;
	ora	#$80			;
	sta	PCCardMessage		;
	lda	#BootOperation		; Do boot function.
	jsr	SendByte		;
	jsr	PCCardServer		; Do PC card server.
	lda	#>SMDOS_Begin		;
	sta	CallAddress-$C000+$5800+1
	lda	#<SMDOS_Begin		;
	sta	CallAddress-$C000+$5800+2
	lda	#$ff			; Boot from remote disk.
	jmp	nebot34			;
;---------------------------------------; Boot from diskette.
nebot12:lda	SDriveMessage		; Has FDC?
	bpl	nebot32			;
	and	#3			; Yes. Has diskette driver?
	beq	nebot32			;
	ldx	#<BootFDCBStg		;
	ldy	#>BootFDCBStg		;
	lsr				; Has drive 0?
	bcc	nebot16			;
	lsr				; Yes.
	ldx	#<BootFDCAStg		;
	ldy	#>BootFDCAStg		;
nebot16:and	#1			; Boot from diskette 0(1).
	pha				;
	lda	#BootLine+3		;
	sta	$fc			;
	lda	#9			;
	jsr	CleanDisplayStg		;
	pla				;
nebot24:sta	BootDiskIOBF+Drive	;
	ldx	#0			;
	stx	BootDiskIOBF+Track	;
	stx	BootDiskIOBF+Head	;
	lda	#<BootDiskSBF		;
	sta	BootDiskIOBF+Buffer+1	;
;	ldx	#>BootDiskSBF		;
	stx	BootDiskIOBF+Buffer	;
	inx				;
	stx	BootDiskIOBF+Sector	;
	stx	BootDiskIOBF+Sectors	;
	lda	#3			;
	sta	BiosTemp5		;
lpbot12:lda	#CMD_READ		;
	sta	BootDiskIOBF+Command	;
	ldx	#<BootDiskIOBF		;
	ldy	#>BootDiskIOBF		;
	jsr	DiskIO			;
	tax				; Read secessiful?
	beq	lpbot16			;
	dec	BiosTemp5		;
	beq	nebot32			;
	lda	#CMD_RESET		;
	sta	BootDiskIOBF+Command	;
	ldx	#<BootDiskIOBF		;
	ldy	#>BootDiskIOBF		;
	jsr	DiskIO			;
	jmp	lpbot12			;
;---------------------------------------; Can not boot.
nebot32:ldy	#>BootFault		;
	ldx	#<BootFault		;
	bne	nebot40			;
lpbot16:lda	BootDiskSBF+3,x		; Yes. Check if SMDOS boot diskette.
	cmp	SMDOS_String,x		;
	bne	nebot36			;
	inx				;
	cpx	#5			;
	bne	lpbot16			;
	lda	BootDiskSBF+1		;
	clc				;
	adc	#2			;
	sta	CallAddress-$C000+$5800+1
	lda	#<BootDiskSBF		;
	sta	CallAddress-$C000+$5800+2
	lda	BootDiskIOBF+Drive	; Boot diskette transmit to BOOT.
nebot34:pha				;
	jsr	DispInitial		;
	lda	#0			; NMI keyboard access.
	sta	NMICheckFlag		;
	pla				;
	jsr	CallToRAM-$C000+$5800	;
	lda	#$c0			; No NMI keyboard access.
	sta	NMICheckFlag		;
nebot36:ldy	#>BootFDCErrStg		;
	ldx	#<BootFDCErrStg		;
nebot40:lda	#BootLine+3		;
	sta	$fc			;
	lda	#5			;
	jsr	CleanDisplayStg		;
	lda	#BootLine+3		;
	sta	$fc			;
	ldx	#<PressKeyStg		;
	ldy	#>PressKeyStg		;
	lda	#19			;
	jsr	DisplayString		;
	lda	#0			; NMI keyboard access.
	sta	NMICheckFlag		;
	ldy	#0			; Read Keyboard and Joystick 1/2.
	ldx	#$17			;
	jsr	ReadKey			; Read a any key.
	jmp	lpbot04			;

;======= Resident part ======================================================;
;---------------------------------------;
VRAMAddress:				;
	bit	$2002			;
	jsr	HalW2006X			;
	jsr	HalW2006Y			;
	rts				;
;---------------------------------------;
DisplayNMIROM:				;
	txa				; Save X,Y
	pha				;
	tya				;
	pha				;
	lda	#0			;
	sta	$2001			;
	ldy	DispBfHead		;
	ldx	#30			;
	bit	$2002			;
	lda	$ff			;
	pha				;
lpdnr04:lda	DisplayBuffer,y		;
	sta	$ff			; Tempare save.
	iny				;
	bit	$ff			;
	bpl	nednr32			;
;---------------------------------------; 11xx. Output List.
	bvc	nednr24			;
	pha				; Save High.
	and	#$3f			;
	jsr	HalW2006			;
	lda	DisplayBuffer,y		;
	jsr	HalW2006			;
	sta	DispBfHead		; Save Low.
	iny				;
	lda	DisplayBuffer,y		; Count
	sta	$ff			;
	pha				; Save Count.
lpdnr08:iny				;
	lda	DisplayBuffer,y		; Display 3 character one count.
	jsr	HalW2007			;
	dec	$ff			;
	beq	nednr20			;
	iny				;
	lda	DisplayBuffer,y		;
	jsr	HalW2007			;
	dec	$ff			;
	beq	nednr20			;
	iny				;
	lda	DisplayBuffer,y		;
	jsr	HalW2007			;
	dec	$ff			;
	beq	nednr20			;
	dex				;
	bne	lpdnr08			;
	lda	$ff			;
	sta	DisplayBuffer,y		;
	dey				;
	pla				; Restoe count.
	sec				;
	sbc	$ff			; Display passed.
	clc				;
	adc	DispBfHead		; Address low
	sta	DisplayBuffer,y		;
	dey				;
	pla				; Restore High.(!=0)
	sta	DisplayBuffer,y		;
	jmp	nednr56			;
nednr20:pla				; S+=2;
	pla				;
	jmp	nednr48			;
;---------------------------------------; 10xx. Output Port.
nednr24:lsr				;
	bcs	nednr28			;
	lda	Addr2000		; 10xxxxx0
	and	#$fc			;
	ora	DisplayBuffer,y		;
	sta	Addr2000		; Save to $2000.
	bcc	nednr48	;(jmp)		;
nednr28:lda	DisplayBuffer,y		; 10xxxxx1
	sta	ScrollPanning		; Save to ScrollPanning
	bcs	nednr48	;(jmp)		;
;---------------------------------------; 01xx. Output Count
nednr32:bvc	nednr36			;
	and	#$3f			;
	jsr	HalW2006			;
	lda	DisplayBuffer,y		;
	jsr	HalW2006			;
	iny				;
	lda	DisplayBuffer,y		; Character
	pha				;
	iny				;
	sty	$ff			; Save count position.
	lda	DisplayBuffer,y		; Count.
	tay				;
	pla				;
lpdnr12:jsr	HalW2007			; Display 4 character one count.
	dey				;
	beq	nednr40			;
	jsr	HalW2007			;
	dey				;
	beq	nednr40			;
	jsr	HalW2007			;
	dey				;
	beq	nednr40			;
	jsr	HalW2007			;
	dey				;
	beq	nednr40			;
	dex				;
	bne	lpdnr12			;
	tya				; Count remain.
	ldy	$ff			; Restore count position.
	ldx	DisplayBuffer,y		;
	sta	DisplayBuffer,y		;
	txa				;
	sec				; Dipslay length remain.
	sbc	DisplayBuffer,y		;
	dey				;
	dey				;
	clc				;
	adc	DisplayBuffer,y		; Address low modify.
	sta	DisplayBuffer,y		;
	dey				;
	bcc	nednr56	;(jmp)		;
;---------------------------------------; 00xx. Output One.
nednr36:jsr	HalW2006			;
	lda	DisplayBuffer,y		; V=0.
	jsr	HalW2006			;
	iny				;
	lda	DisplayBuffer,y		;
	jsr	HalW2007			;
	bvc	nednr48	;(jmp)		;
nednr40:ldy	$ff			;
nednr48:iny				;
	cpy	DispBfTail		;
	beq	nednr52			;
	dex				;
	beq	nednr56			;
	jmp	lpdnr04			;
nednr52:lda	Addr2000		;
	and	#$fc			;
	ora	Addr2000N		;
	sta	Addr2000		;
nednr56:sty	DispBfHead		;
	pla				;
	sta	$ff			;
	lda	#0			;
	sta	$2005			;
	lda	ScrollPanning		;
	sta	$2005			;
	lda	Addr2000		;
	jsr	HalW2000			;
	lda	Addr2001		;
	sta	$2001			;
	pla				;
	tay				;
	pla				;
	tax				;
	rts				;
;---------------------------------------;
DisableNMI:				;
	lda	Addr2000		;
	and	#$7f			;
	jsr	HalW2000			;
	rts				;
;---------------------------------------;
EnableNMI:				;
	lda	Addr2000		;
	ora	#$80			;
	jsr	HalW2000			;
	rts				;
;---------------------------------------;
RestoreNMI:				;
	lda	Addr2000		;
	jsr	HalW2000			;
	rts				;

;=============================================================================
BIOSBranchAddr  dw      SetPageMode-1,          SetPage-1-$C000+$5800    ;1
                dw      GetPage-1-$C000+$5800,  SetAV10-1                ;3
                dw      RunInitial-1,           LowRAMFree-1             ;5
                dw      DiskIO-1,               GetDRAMSize-1            ;7
                dw      SetPageNoUse-1,         SetVector-1              ;9
                dw      GetVRAMSize-1,          ReadKey-1                ;b
                dw      MoveBlock-1,            FileIOServer-1           ;d
                dw      WaitVSynCnt-1,          LowRAMAlloc-1            ;f
                dw      FaultError-1,           ExchBlock-1     ;10      ;11
                dw      SplitScreen-1,          SetVideoPage-1           ;13
                dw      GetDriveMessage-1,      LoadVideoData-1          ;15
                dw      SetNMIVector-1,         GetNMIVector-1           ;17
                dw      DeviceEntryROM-1,       RstVector-1              ;19
                dw      GetCardVer-1,           DisplayTTY-1             ;1b
                dw      SetCursor-1,            DisplayChars-1           ;1d
                dw      GetCursor-1,            SetVectorRTI-1           ;1f
                dw      GetPagedChar-1,         SetPagedChar-1  ;20       21
                dw      WaitDisplay-1,          SendPCByte-1             ;23
                dw      GetByte-1,              UnzipBlock-1             ;25
                dw      SetDiskDspOff-1,        SetSEndVector-1          ;27
                dw      DispInitial-1                                    ;29
CommandLast2	equ	@-BIOSBranchAddr

;=======================================;
;	(A)= Function numbers.		;
;---------------------------------------;
ROMBios:cmp	#BiosDispInitial	;
	bne	nerbs08			;
	sec				;
	sbc	#BiosDispInitial-BiosSetSEndVector-1
nerbs08:asl				;
	bcs	nerbs12			;
	cmp	#CommandLast2		;
	bcs	nerbs16			;
	stx	BiosTemp1		; Save (X).
	tax				;
	lda	BIOSBranchAddr+1,x	;
	pha				;
	lda	BIOSBranchAddr,x	;
	pha				;
	ldx	BiosTemp1		; Restore (X).
	rts				;
nerbs12:cmp	#254			;
	bne	nerbs16			;
;---------------------------------------;
;	(A)= 255: Get bios ver and date.;
;	Output	(A)= Version.		;
;		X: Year. Y: Month	;
;---------------------------------------;
	lda	#$40			; Compatiable with XNR version.
	ldx	#BiosYear		;
	ldy	#BiosMonth		;
nerbs16:rts				;
;---------------------------------------;

;=======================================;
SetDiskDspOff:				;
	stx	DiskDisplayOff		;
	rts				;
;=======================================;
SetSEndVector:				;
	stx	SEndAccessSave-$C000+$5800+1
	sty	SEndAccessSave-$C000+$5800
	ldx	DRAMPageSave		;
	stx	INTDRAMPage		;
	ldx	#<SEndAccessRAM-$C000+$5800
	ldy	#>SEndAccessRAM-$C000+$5800
	stx	SEndAddressSave-$C000+$5800+1
	sty	SEndAddressSave-$C000+$5800
	rts				;
;=======================================;
FaultError:				;
lpfer04:jsr	SoundBell		;
	jmp	lpfer04			;

;=======================================;
SendPCByte:				;
	txa				;
;---------------------------------------;
;	Input	(A)= Send byte.		;
;---------------------------------------;
SendByte:				;
	pha				;
lpsbe04:lda	PCCtrlPortI		; P-STB is low?(Read to receive)
	and	#8			;
	bne	lpsbe04			;
	pla				; Yes. send low half byte.
	pha				;
	and	#$0f			; Reset P-ERROR(Data ready).
	sta	PCDaCtPortO		;
lpsbe08:lda	PCCtrlPortI		; P-STB is high?(Read first half)
	and	#8			;
	beq	lpsbe08			;
	lda	#$10			; Yes. set P-ERROR(Next half).
	sta	PCDaCtPortO		;
lpsbe12:lda	PCCtrlPortI		; P-STB is low?(Read to receive)
	and	#8			;
	bne	lpsbe12			;
	pla				; Yes. send high half byte.
	lsr				;
	lsr				;
	lsr				;
	lsr				; Reset P-ERROR(Data ready)
	sta	PCDaCtPortO		;
lpsbe16:lda	PCCtrlPortI		; P-STB is high?(Data Received)
	and	#8			;
	beq	lpsbe16			;
	lda	#$10			; Yes. set P-ERROR(Transmission end)
	sta	PCDaCtPortO		;
	rts				;
;---------------------------------------;
;	Output	(A)= Get byte.		;
;---------------------------------------;
GetByte:				;
lpgbe04:lda	PCCtrlPortI		; P-STB is low?(Data ready)
	and	#8			;
	bne	lpgbe04			;
	lda	PCDataPortIH		;
	and	#$f0			;
	sta	GetByteSave		;
	lda	PCDataPortIL		;
	and	#$0f			;
	ora	GetByteSave		;
	eor	#$8b			;
	pha				;
	lda	#$00			; Yes. Reset P-ERROR.(Data received)
	sta	PCDaCtPortO		;
lpgbe08:lda	PCCtrlPortI		; P-STB is high?(Transmission end)
	and	#8			;
	beq	lpgbe08			;
	lda	#$10			; Yes. Set P-ERROR.(Receive end)
	sta	PCDaCtPortO		;
	pla				;
	rts				;
;=======================================;
;    Use $fe,$ff.			;
;---------------------------------------;
GetBlockDirect:				;
	jsr	GetByte			; Block length high.
	tay				;
	jsr	GetByte			; Block length low.
	tax				;
	beq	negbd04			;
	iny				;
negbd04:lda	$fd			; Save $fd.
	pha				;
	sty	$fd			; Save block length high.
	jsr	GetByte			; Address high
	sta	$ff			;
	jsr	GetByte			; Address low
	tay				;
	lda	#0			;
	sta	$fe			;
	lda	DRAMPageSave		;
	jsr	HalPageA		; Set to current page.
lpgbd04:lda	($fe),y			;
	jsr	SendByte		;
	iny				;
	bne	negbd08			;
	inc	 $ff			;
negbd08:dex				;
	bne	lpgbd04			;
	dec	$fd			;
	bne	lpgbd04			;
	pla				;
	sta	$fd			;
	rts				;
;=======================================;
;    Use $fe,$ff.			;
;---------------------------------------;
SaveBlockDirect:			;
	jsr	GetByte			; Block lenght high.
	tay				;
	jsr	GetByte			; Block length low.
	tax				;
	beq	nesbd04			;
	iny				;
nesbd04:lda	$fd			; Save $fd.
	pha				;
	sty	$fd			; Save block length high.
	jsr	GetByte			; Block address high
	sta	$ff			;
	jsr	GetByte			; Block address low.
	tay				;
	lda	#0			;
	sta	$fe			;
	lda	DRAMPageSave		;
	jsr	HalPageA		; Set to current page.
lpsbd04:jsr	GetByte			;
	sta	($fe),y			;
	iny				;
	bne	nesbd08			;
	inc	$ff			;
nesbd08:dex				;
	bne	lpsbd04			;
	dec	$fd			;
	bne	lpsbd04			;
	pla				;
	sta	$fd			;
	rts				;
;---------------------------------------;
PCCardServer:				;
lppcs04:jsr	GetByte			;
	cmp	#8			; Set page?
	bne	nepcs04			;
	jsr	GetByte			;
	tax				;
	jsr	SetPage-$C000+$5800	;
	jmp	lppcs04			;
nepcs04:cmp	#11			; Save block direct?
	bne	nepcs08			;
	jsr	SaveBlockDirect		;
	jmp	lppcs04			;
nepcs08:cmp	#21			;
	bne	nepcs12			;
	jsr	GetBlockDirect		;
	jmp	lppcs04			;
nepcs12:cmp	#10			; Get page?
	bne	nepcs16			;
	jsr	GetPage			;
	jsr	SendByte		;
	jmp	lppcs04			;
nepcs16:tax				; Function return?
	bne	lppcs04			;
	rts				;
;=======================================;
;	Input	X|Y= FileIO Parameters.	;
;	Function usage			;
;		GetByte()		;
;		SendByte()		;
;	 08:	SetCurrentPage()	;
;	 11:	SaveBlockDirect()	;
;	 21:	GetBlockDirect()	;
;---------------------------------------;
FileIOServer:				;
	lda	$ff			; Save $fc-$ff.
	pha				;
	lda	$fe			;
	pha				;
	lda	$fd			;
	pha				;
	lda	$fc			;
	pha				;
	stx	$fd			;
	sty	$fc			;
	lda	DRAMPageSave		; Save original page.
	pha				;
	tax				;
	lda	$fd			;
	cmp	#<$c000			;
	bcc	nefio04			;
	and	#$bf			;
	sta	$fd			;
	ldx	#$ff			;
	stx	DRAMPageSave		;
	jsr	HalPageX		;
nefio04:ldy	#HBUF+1			; No. Modify HBUF point.
	lda	($fc),y			;
	cmp	#<$c000			; Is > c000?
	php				;
	bcc	nefio08			;
	and	#$bf			; Yes.
	sta	($fc),y			;
	ldy	#HPAGE			;
	lda	#$ff			; Set to $ff page.
	sta	($fc),y			;
nefio08:txa				; Save handle's page.
	pha				;
	lda	#FileOperation		;
	jsr	SendByte		;
	ldy	#0			;
lpfio08:lda	($fc),y			;
	jsr	SendByte		;
	iny				;
	cpy	#16			;
	bne	lpfio08			;
;---------------------------------------;
	jsr	PCCardServer		;
	pla				; Restore handle's page.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	ldy	#0			;
lpfio12:jsr	GetByte			;
	sta	($fc),y			;
	iny				;
	cpy	#14			;
	bne	lpfio12			;
	plp				;
	bcc	nefio12			;
	ldy	#HBUF+1			; No. Modify HBUF point.
	lda	($fc),y			;
	clc				;
	adc	#<$c000-$8000		;
	sta	($fc),y			;	
nefio12:pla				; Restore original page.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	pla				;
	sta	$fc			;
	pla				;
	sta	$fd			;
	pla				;
	sta	$fe			;
	pla				;
	sta	$ff			;
SetPageMode:				;
	rts				;
;---------------------------------------;
ModifyAddressFE:			;
	lda	$fe+1			;
	cmp	#<$c000			;
	bcc	nemaf04			;
	and	#$bf			;
	sta	$fe+1			;
	ldx	#$ff			;
nemaf04:lda	DRAMPageSave		;
	jsr	SetPage-$C000+$5800	;
	tax				;
	rts				;
;=======================================;
;    Get character with page.		;
;	Input	X|($fe),y= &Character	;
;	Output	(A)= Character.		;
;	Save	$fe,y.			;
;---------------------------------------;
GetPagedChar:				;
	lda	$fe+1			;
	sta	BiosTemp1		;
	jsr	ModifyAddressFE		;
	lda	($fe),y			;
	stx	DRAMPageSave		;
	jsr	HalPageX		;
	ldx	BiosTemp1		;
	stx	$fe+1			;
	rts				;
;=======================================;
;    Save character with page.		;
;	Input	X|($fe),y= &Character	;
;	Output	($fd)= Character.	;
;	Save	$fe,y.			;
;---------------------------------------;
SetPagedChar:				;
	lda	$fe+1			;
	sta	BiosTemp1		;
	jsr	ModifyAddressFE		;
	lda	$fd			;
	sta	($fe),y			;
	stx	DRAMPageSave		;
	jsr	HalPageX		;
	ldx	BiosTemp1		;
	stx	$fe+1			;
	rts				;
;=======================================;
;    Set page				;
;	(X)= Page number		;
;	Save	A,Y.			;
;---------------------------------------;
SetPageROM:				;
	cpx	BiosUsePage		;
	bne	nespr04			;
	ldx	DRAMPageNotUse		;
nespr04:cpx	#0			;
	bne	nespr08			;
	ldx	BiosUsePage		;
nespr08:stx	DRAMPageSave		;
	jsr	HalPageX		;
	rts				;

;=======================================;
;	(X)= Bit 1,0:			;
;		00: AV10 = AV10.	;
;		01: AV10 = AV11.	;
;		10: AV10 = 0.		;
;		11: AV10 = 1.		;
;---------------------------------------;
SetAV10:txa				;
	and	#3			;
	sta	BiosTemp1		;
	lda	VideoCtrlSave		;
	and	#$fc			; Clear AV bits and set ROM to C000.
	ora	BiosTemp1		;
	sta	VideoCtrlSave		;
	rts				;
;---------------------------------------;
InitVideoPort:				;
	ldx	#4			; Initial four display Vpage. Page address's 0,8,16,24.
	jsr	HalSlot0X		; VROM 0800-0FFF to VPage 4.
	inx				;
	jsr	HalSlot1X		;      0800-0FFF to VPage 5.
	inx				;
	jsr	HalSlot2X	;      0800-0FFF to VPage 6.
	inx				;
	jsr	HalSlot3X	;      0800-0FFF to VPage 7.
	rts				;
;=======================================;
;    Program running initial.		;
;	(X)= Flags.			;
;	Bit 10: 00: AV10=AV10.		;
;		01: AV10=AV11.		;
;		10: AV10=0.		;
;		11: AV10=1.		;
;---------------------------------------;
RunInitial:				;
	lda	VideoCtrlSave		;
	and	#$f8			;
	sta	VideoCtrlSave		;
	txa				;
	and	#$03			; Pick Bit1,0.
	ora	VideoCtrlSave		;
	sta	VideoCtrlSave		;
	lda	#0			;
	sta	VectorSet		; No user NMI and INT vector.
	sta	SplitPoint		; No split screen operation.
	sta	CurColumn		;
	sta	CurLine			;
	sta	ScrollPanning		;
	sta	ScrollPanningN		;
	sta	Addr2000N		;
	sta	DispBfHead		;
	sta	DispBfTail		;
	sta	DiskDisplayOff		;
	jsr	DisableNMI		;
	sei				;
	lda	#$ff			;
	jsr	HalPageA		;
	ldx	#<BRKINTEntry-$C000+$5800
	ldy	#>BRKINTEntry-$C000+$5800
	stx	$bfff			;
	sty	$bffe			;
	ldx	#<NMIEntry-$C000+$5800	;
	ldy	#>NMIEntry-$C000+$5800	;
	stx	$bffb			;
	sty	$bffa			;
	lda	DRAMPageSave		;
	jsr	HalPageA		;
	ldx	#<RTIReturn-$C000+$5800	;
	ldy	#>RTIReturn-$C000+$5800	;
	stx	BRKAddressSave-$C000+$5800+1; Restore BRK vector.
	sty	BRKAddressSave-$C000+$5800
	stx	NMIAddressSave-$C000+$5800+1; Restore NMI vector.
	sty	NMIAddressSave-$C000+$5800
	ldx	#<RTSInstruction-$C000+$5800
	ldy	#>RTSInstruction-$C000+$5800
	stx	SEndAddressSave-$C000+$5800+1
	sty	SEndAddressSave-$C000+$5800
	jsr	SetNMIVector		;
	jsr	EnableNMI		;
	sta	Addr2000		;
	jsr	InitVideoPort		;
	lda	#0			; Close audio.
	sta	$4015			;
	lda	#$c0			; Disable CPU internal INT.
	sta	$4017			;
	rts				;
;=======================================;
;    Disk bios.				;
;	I: (X|Y)= Parameter buffer.	;
;	O: (A) = Status return.		;
;	   (Y) = Sectors has read.	;
;    Cautions:				;
;	Can't change paramter buffer.	;
;---------------------------------------;
SoftDiskBranch	dw	SoftDiskReset-1	;
		dw	SoftDiskStatus-1;
		dw	SoftDiskRead-1	;
		dw	SoftDiskWrite-1	;
		dw	SoftDiskVerify-1;
		dw	SoftDiskFormat-1;
		dw	SoftSetDiskMedia-1
SoftCommandLast	equ	@-SoftDiskBranch;
DiskIO:	txa				;
	pha				;
	ldx	#DiskIOZB_Size-1	;
lpdio04:lda	DiskIOZB,x		;
	sta	DiskIOBuffer,x		;
	dex				;
	bpl	lpdio04			;
	lda	DRAMPageSave		; Save command buffer's page.
	sta	DiskIOPageSave		;
	pla				;
	cmp	#<$c000			; Command buffer >= $c000?
	bcc	nedio04			;
	and	#$bf			;
	ldx	#$ff			;
	stx	DRAMPageSave		;
	jsr	HalPageX		;
nedio04:sta	SectorBfPoint0+1	;
	sty	SectorBfPoint0		;
	ldy	#DiskIOCmdSize-1	;
lpdio08:lda	(SectorBfPoint0),y	;
	sta	DiskIOZB+Command,y	;
	dey				;
	bpl	lpdio08			;
;---------------------------------------;
SoftDiskIO:				;
	lda	DiskIOZB+Command	;
	asl				;
	cmp	#SoftCommandLast	;
	bcc	nesdo04			;
	lda	#BadCommand		;
	sta	DiskOutStatus		;
	bne	nesdo08			;
nesdo04:jsr	neadi12			;
nesdo08:bit	$2002			; No respone NMI currently.
	jsr	RestoreNMI		; Restore NMI status.
	lda	#100			; 2 second motor on delay.
	sta	MotorOnCount		;
	lda	DiskOutStatus		;
	jmp	DiskIOReturn	;(jmp)	;
neadi12:tax				;
	lda	SoftDiskBranch+1,x	;
	pha				;
	lda	SoftDiskBranch,x	;
	pha				;
	rts				;
;---------------------------------------;
;	Save	A,X,Y.			;
;---------------------------------------;
DiskIOReturn:				;
	pha				;
	lda	DiskIOPageSave		; Restore command buffer's page.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	txa				;
	pha				;
	ldx	#DiskIOZB_Size-1	;
lpdio12:lda	DiskIOBuffer,x		;
	sta	DiskIOZB,x		;
	dex				;
	bpl	lpdio12			;
	pla				;
	tax				;
	pla				;
	rts				;
;=======================================;
SoftDiskReset:				;
	lda	MotorStatus		; Motor remain on.
	ora	DiskIOZB+Drive		; Select reset drive.
	ora	#8			;
	sta	FDCCtrlPortO		; Reset a drive.
	ldx	#0			;
	stx	DiskOutStatus		;
	stx	SeekStatus		; All disk must seek.
	ora	#4			; Set this disk seek require flag.
	sta	FDCCtrlPortO		;
	jsr	ChkStat2		;
	lda	NECStatus		;
	cmp	#$c0			;
	bne	nedrt12			;
	lda	#2			; 720K rate.
	sta	FDCSpeedPortO		;
	lda	#3			;
	jsr	FDCOutCmdErr		;
	lda	#$df			; 720k parameter.
	jsr	FDCOutCmdErr		;
	lda	#2			;
	jsr	FDCOutCmdErr		;
	rts				;
nedrt12:lda	DiskOutStatus		;
	ora	#BadCntlr		;
	sta	DiskOutStatus		;
	sec				;
nedrt16:rts				;
;=======================================;
SoftDiskStatus:				;
	lda	DiskIOZB+Drive		;
	clc				;
	adc	#1			;
	bit	SDriveMessage		;
	beq	nedss04			;
	jsr	OpenMotor		;
nedss04:ldx	SDriveMessage		;
	ldy	FDCChangePortI		;
	rts				;
;=======================================;
SoftSetDiskMedia:			;
	lda	#0			;
	ldx	DiskIOZB+Sector		;
	cpx	#18			;
	beq	nedsm04			;
	lda	#2			;
nedsm04:ldx	DiskIOZB+Drive		; Drive.
	sta	DriveMedia,x		;
	lda	#1			; This drive's media is set.
	sta	MediaMessage,x		;
	rts				;
;=======================================;
SoftDiskFormat:				;
	jsr	DiskPrepare		;
	bcs	nedrt16			;
	ldx	DiskIOZB+Drive		; Drive.
	lda	DriveMedia,x		;
	sta	FDCSpeedPortO		;
	jsr	SendFDDParam		;
	bcs	nedrt16			;
	jsr	SetBfPoint0		;
	lda	#18			; Sector/cylider (720K:9, 144M:18)
	ldx	DiskIOZB+Drive		; Drive.
	ldy	DriveMedia,x		;
	beq	nesdf04			;
	lda	#9			;
nesdf04:sta	BiosTemp3		;
	asl				;
	asl				;
	sta	BiosTemp2		;
	adc	SectorBfPoint0		;
	sta	SectorBfPoint0		;
	bcs	nesdf08			;
	dec	SectorBfPoint0+1	;
nesdf08:lda	#1			;
	sec				;
	sbc	BiosTemp2		;
	sta	BiosTemp2		;
;---------------------------------------; Send to NEC read or write command.
	lda	#$cd			; Operation command.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Head		; Head and drive.
	asl				;
	asl				;
	ora	DiskIOZB+Drive		;
	jsr	FDCOutCmdErr		;
	lda	#2			; Bytes*256/sector
	jsr	FDCOutCmdErr		;
	lda	BiosTemp3		;
	jsr	FDCOutCmdErr		;
	lda	#$6c			; GPL (720K:$2a, 144M:$1b)
	ldx	DiskIOZB+Drive		; Drive.
	ldy	DriveMedia,x		;
	beq	nesdf28			;
	lda	#$50			; 720K parameter.
nesdf28:jsr	FDCOutCmdErr		;
	lda	#$f6			; D.
	jsr	FDCOutCmdErr		;
;---------------------------------------;
	ldy	#$ff			;
	lda	(SectorBfPoint0),y	;
	pha				;
	lda	SectorBfPoint0		; SectorBfPoint0--;
	bne	newfd04			;
	dec	SectorBfPoint0+1	;
newfd04:dec	SectorBfPoint0		;
	ldx	#0			;
	ldy	BiosTemp2		;
	lda	(SectorBfPoint0),y	;
lpwfd08:bit	FDCDRQPortI		;
	bvs	newfd08			;
	dey				;
	bne	lpwfd08			;
	bit	FDCDRQPortI		;
	bvs	newfd08			;
	dex				;
	bne	lpwfd08			;
	beq	newfd16			;
newfd08:ldy	BiosTemp2		;
lpwfd16:sta	FDCDMADackIO		;
	iny				;
	beq	newfd12			;
	lda	(SectorBfPoint0),y	;
	ldx	#$40			;
	bit	FDCDRQPortI		;
	bvs	lpwfd16			;
	stx	BiosTemp3		;
	ldx	#0			;
lpwfd20:bit	FDCDRQPortI		;
	bvs	lpwfd16			;
	dex				;
	bne	lpwfd20			;
	bit	FDCDRQPortI		;
	bvs	lpwfd16			;
	dec	BiosTemp3		;
	bne	lpwfd20			;
	beq	newfd16			;
newfd12:pla				; Last byte.
	ldx	#0			;
lpwfd32:bit	FDCDRQPortI		;
	bvs	newfd20			;
	dex				;
	bne	lpwfd32			;
newfd16:jmp	DiskIODMAError		;
newfd20:sta	FDCDMATcIO		;
	jmp	RWOperEnd		;
;=======================================;
SoftDiskWrite:				;
	lda	#$c5			;
	bne	nesdr04			;
;=======================================;
SoftDiskVerify:				;
SoftDiskRead:				;
	lda	#$e6			;
nesdr04:sta	BiosTemp2		;
	jsr	RWOper			;
	lda	#0			;
	ldy	DiskIOZB+Head		;
	cpy	NECStatus+4		; End head.
	beq	nesdr12			;
	clc				;
	adc	#9			; 9 sector per side of 720K diskette.
	ldx	DiskIOZB+Drive		;
	ldy	DriveMedia,x		;
	bne	nesdr08			;
	adc	#9			; 18 sector per side of 720K diskette.
nesdr08:ldy	DiskIOZB+Head		; Head change from 1->0?
	beq	nesdr12			;
	eor	#$ff			; Yes.
	adc	#1			;
nesdr12:ldy	NECStatus+3		; End cylinder.
	cpy	DiskIOZB+Track		;
	beq	nesdr16			;
	clc				;
	adc	#18			; 18 sector per cylider of 720K diskette.
	ldx	DiskIOZB+Drive		;
	ldy	DriveMedia,x		;
	bne	nesdr16			;
	clc				;
	adc	#18			; 36 sector per cylider of 720K diskette.
nesdr16:clc				;
	adc	NECStatus+5		; End sector.
	sec				;
	sbc	DiskIOZB+Sector		;
	tay				; Return sectors.
nesdr20:rts				;
;---------------------------------------;
SendFDDParam:				;
	lda	#3			; Send FDD parameter
	jsr	FDCOutCmdErr		;
	lda	#$bf			; 1.44M parameter.
	ldx	DiskIOZB+Drive		; Drive.
	ldy	DriveMedia,x		;
	beq	nerwo24			;
	lda	#$df			; 720K parameter.
nerwo24:jsr	FDCOutCmdErr		;
	lda	#2			;
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Track		;
	jmp	Seek			;
;---------------------------------------;
DiskPrepare:				;
	jsr	OpenMotor		;
	lda	#0			;
	sta	DiskOutStatus		;
	lda	FDCChangePortI		; Disk change?
	bpl	nedpp16			;
	ldx	DiskIOZB+Drive		; Reset this drive's "MediaMessage"
	lda	#0			;
	sta	MediaMessage,x		;
	jsr	SoftDiskReset		; Reset diskette.
	bcc	nedpp04			;
	jsr	SoftDiskReset		;
	bcs	nedpp12			;
nedpp04:lda	#1			; Seek track 1.
	jsr	Seek			;
	bcs	nedpp12			;
	lda	#0			; Seek track 0.
	jsr	Seek			;
	bcs	nedpp12			;
	ldx	FDCChangePortI		; Disk change also?
	bpl	nedpp16			;
	lda	#DriveEmpty		;
nedpp08:sta	DiskOutStatus		;
	sec				;
nedpp12:rts				;
nedpp16:jsr	WaitDisplay		;
	ldx	#1			;
	jsr	WaitVSynCnt		;
	lda	DiskDisplayOff		;
	beq	nedpp20			;
	lda	#0			;
	sta	$2001			;
nedpp20:sei				;
	jsr	DisableNMI		; Disable NMI for soft disk r/w.
	clc				;
	rts				;
;=======================================;
RWOper:	jsr	DiskPrepare		;
	bcs	nedpp12			;
	ldx	DiskIOZB+Drive		; This drive media is set?
	lda	MediaMessage,x		;
	bne	nerwo20			;
	lda	DiskIOZB+Drive		; No. Detect media.
	clc				;
	adc	#1			;
	ora	SeekStatus		;
	sta	SeekStatus		;
	jsr	HomeTrack		;
	bcc	nerwo04			;
	jsr	HomeTrack		;
	bcs	nedpp12			;
nerwo04:lda	#0			; Set 500kbs(1.44M) speed.
	jsr	DetectMedia		;
	bcs	nedpp12			; Error.
	beq	nerwo08			; Media matched.
	lda	#2			; Set 250kbs(720K) speed.
	jsr	DetectMedia		;
	bcs	nedpp12			; Error.
	beq	nerwo12			; Media matched.
	lda	#BadAddressMark		;
	bne	nedpp08			;
nerwo08:lda	#0			;
	beq	nerwo16			;
nerwo12:lda	#2			;
nerwo16:ldx	DiskIOZB+Drive		; Drive.
	sta	DriveMedia,x		;
	lda	#1			; This drive's media is set.
	sta	MediaMessage,x		;
nerwo20:lda	DriveMedia,x		;
	sta	FDCSpeedPortO		;
	jsr	SendFDDParam		;
	bcs	nedpp12			;
	lda	#0			;
	sta	MotorOnCount		; Motor count to maximun.
	lda	DiskIOZB+Track		;
	jsr	Seek			;
	bcs	nedpp12			;
	jsr	SetBfPoint0		;
;---------------------------------------;
	ldy	DiskIOZB+Command	; Write?
	cpy	#CMD_WRITE		;
	beq	nerwo28			;
	clc				; No.
nerwo28:adc	#$ff			;
	sta	SectorBfPoint1		;
	txa
	adc	#0			;
	sta	SectorBfPoint1+1	;
;---------------------------------------; Send to NEC read or write command.
	lda	BiosTemp2		; Operation command.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Head		; Head and drive.
	asl				;
	asl				;
	ora	DiskIOZB+Drive		;
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Track		; Cylinder.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Head		; Head.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Sector		; Sector.
	jsr	FDCOutCmdErr		;
	lda	#2			; Bytes*256/sector
	jsr	FDCOutCmdErr		;
	lda	#18			; Sector/cylider (720K:9, 144M:18)
	ldx	DiskIOZB+Drive		; Drive.
	ldy	DriveMedia,x		;
	beq	nerwo32			;
	lda	#9			;
nerwo32:jsr	FDCOutCmdErr		;
	lda	#$1b			; GPL (720K:$2a, 144M:$1b)
	ldx	DiskIOZB+Drive		; Drive.
	ldy	DriveMedia,x		;
	beq	nerwo36			;
	lda	#$2a			; 720K parameter.
nerwo36:jsr	FDCOutCmdErr		;
	lda	#$ff			; DTL.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Sectors	;
	ldx	DiskIOZB+Command	; Read operation?
	cpx	#CMD_READ		;
	bne	VerifyOper		;
;---------------------------------------;
ReadOper:				; Yes.
lpreo02:pha				;
	ldx	#0			;
	ldy	#0			;
lpreo04:bit	FDCDRQPortI		;
	bvs	nereo02			;
	dey				;
	bne	lpreo04			;
	bit	FDCDRQPortI		;
	bvs	nereo02			;
	dex				;
	bne	lpreo04			;
	beq	nereo12			;
nereo02:ldy	#0			; First 256 byte.
lpreo08:lda	FDCDMADackIO		; 4 | 16us/byte(26.6 clk).
	sta	(SectorBfPoint0),y	; 6
	iny				; 2
	beq	nereo04			; 2
	ldx	#0			; 2 Check time out.
lpreo12:bit	FDCDRQPortI		; 4
	bvs	lpreo08			; 3-4 | 23-24
	dex				;
	bne	lpreo12			;
	beq	nereo12			;
lpreo16:lda	FDCDMADackIO		; Next 256 byte.
	sta	(SectorBfPoint1),y	;
nereo04:iny				;
	beq	nereo08			;
	ldx	#0			; Check time out.
lpreo20:bit	FDCDRQPortI		;
	bvs	lpreo16			;
	dex				;
	bne	lpreo20			;
	beq	nereo12			;
nereo08:ldx	#0			; Check time out.
lpreo24:bit	FDCDRQPortI		;
	bvs	nereo16			;
	dex				;
	bne	lpreo24			;
nereo12:jmp	DiskIODMAError		;
nereo16:pla				;
	tax				;
	dex				; Last sector?
	bne	nereo20			;
	lda	FDCDMATcIO		; Yes, create -DACK and T/C signal.
	bvs	nereo24			;
nereo20:lda	FDCDMADackIO		; No, create -DACK signal only.
nereo24:inc	SectorBfPoint1+1	;
	sta	(SectorBfPoint1),y	;
	inc	SectorBfPoint1+1	; Point to next sector.
	inc	SectorBfPoint0+1	;
	inc	SectorBfPoint0+1	;
	txa				;
	bne	lpreo02			;
	jmp	RWOperEnd		;
;---------------------------------------;
VerifyOper:				;
	cpx	#CMD_VERIFY		;
	bne	WriteOper		;
lpveo02:pha				;
	ldx	#0			;
	ldy	#0			;
lpveo04:bit	FDCDRQPortI		;
	bvs	neveo02			;
	dey				;
	bne	lpveo04			;
	bit	FDCDRQPortI		;
	bvs	neveo02			;
	dex				;
	bne	lpveo04			;
	beq	neveo12			;
neveo02:ldy	#0			; First 256 byte.
lpveo08:lda	FDCDMADackIO		;
	iny				;
	beq	neveo04			;
	ldx	#0			;
lpveo12:bit	FDCDRQPortI		;
	bvs	lpveo08			;
	dex				;
	bne	lpveo12			;
	beq	neveo12			;
lpveo16:lda	FDCDMADackIO		; Next 256 byte.
neveo04:iny				;
	beq	neveo08			;
	ldx	#0			; Check time out.
lpveo20:bit	FDCDRQPortI		;
	bvs	lpveo16			;
	dex				;
	bne	lpveo20			;
	beq	neveo12			;
neveo08:ldx	#0			; Check time out.
lpveo24:bit	FDCDRQPortI		;
	bvs	neveo16			;
	dex				;
	bne	lpveo24			;
neveo12:jmp	DiskIODMAError		;
neveo16:pla				;
	tax				;
	dex				; Last sector?
	bne	neveo20			;
	lda	FDCDMATcIO		; Yes, create -DACK and T/C signal.
	bvs	neveo24			;
neveo20:lda	FDCDMADackIO		; No, create -DACK signal only.
neveo24:txa				;
	bne	lpveo02			;
	jmp	RWOperEnd		;
;---------------------------------------;
WriteOper:				;
lpweo04:tax				; Is last sector?
	dex				;
	beq	WriteLastSector		; Yes.
	txa				;
	pha				;
	ldx	#0			;
	ldy	#0			;
	lda	(SectorBfPoint0),y	;
lpweo08:bit	FDCDRQPortI		;
	bvs	neweo08			;
	dey				;
	bne	lpweo08			;
	bit	FDCDRQPortI		;
	bvs	neweo08			;
	dex				;
	bne	lpweo08			;
	beq	neweo20			;
neweo08:ldy	#0			; First 256 byte.
lpweo16:sta	FDCDMADackIO		; 4   | 16us/byte(26.6 clk).
	iny				; 2
	beq	neweo12			; 2
	lda	(SectorBfPoint0),y	; 5-6
	ldx	#0			; 2   Check time out.
lpweo20:bit	FDCDRQPortI		; 4
	bvs	lpweo16			; 3-4 | 22-24
	dex				;
	bne	lpweo20			;
	beq	neweo20			;
lpweo24:sta	FDCDMADackIO		;
	iny				;
	beq	neweo24			;
neweo12:lda	(SectorBfPoint1),y	; Next 256 byte.
	bit	FDCDRQPortI		;
	bvs	lpweo24			;
	ldx	#0			; Check time out.
lpweo28:bit	FDCDRQPortI		;
	bvs	lpweo24			;
	dex				;
	bne	lpweo28			;
neweo20:jmp	DiskIODMAError		;
neweo24:inc	SectorBfPoint1+1	;
	inc	SectorBfPoint1+1	; Point to next sector.
	inc	SectorBfPoint0+1	;
	inc	SectorBfPoint0+1	;
	pla				;
	bne	lpweo04			;
	beq	RWOperEnd	;(jmp)	;
;---------------------------------------;
WriteLastSector:			;
	ldy	#$ff			;
	lda	(SectorBfPoint1),y	;
	pha				;
	lda	SectorBfPoint1		;
	bne	newls04			;
	dec	SectorBfPoint1+1	;
newls04:dec	SectorBfPoint1		;
	ldx	#0			;
	iny				;
	lda	(SectorBfPoint0),y	;
lpwls08:bit	FDCDRQPortI		;
	bvs	newls08			;
	dey				;
	bne	lpwls08			;
	bit	FDCDRQPortI		;
	bvs	newls08			;
	dex				;
	bne	lpwls08			;
	beq	DiskIODMAError		;
newls08:ldy	#0			; First 256 byte.
lpwls16:sta	FDCDMADackIO		; 4
	iny				; 2
	beq	newls12			; 2
	lda	(SectorBfPoint0),y	; 5-6 | 16us/byte(26.6 clk).
	ldx	#0			; 2
lpwls20:bit	FDCDRQPortI		; 4
	bvs	lpwls16			; 3-4 | 22-24
	dex				;
	bne	lpwls20			;
	beq	DiskIODMAError		;
lpwls24:sta	FDCDMADackIO		;
newls12:iny				; Next 256 byte.
	beq	newls16			;
	lda	(SectorBfPoint1),y	;
	ldx	#0			; Check time out.
lpwls28:bit	FDCDRQPortI		;
	bvs	lpwls24			;
	dex				;
	bne	lpwls28			;
	beq	DiskIODMAError		;
newls16:pla				; Last byte.
	ldx	#0			;
lpwls32:bit	FDCDRQPortI		;
	bvs	newls28			;
	dex				;
	bne	lpwls32			;
;---------------------------------------;
DiskIODMAError:				;
	pla				;
	bit	FDCIRQPortI		; Has IRQ signal?
	bvs	nerwe08			;
	lda	DiskOutStatus		; No.
	ora	#TimeOut		;
	sta	DiskOutStatus		;
	sec				;
	rts				;
newls28:sta	FDCDMATcIO		;
;---------------------------------------;
RWOperEnd:				;
	jsr	WaitFDCInt		;
	bcs	nerwe20			;
nerwe08:lda	#7			;
	jsr	Results			;
	bcs	nerwe20			;
	lda	NECStatus		; Get ST0.
	and	#$c0			; Normal ending?
	beq	nerwe20			; Yes.
	cmp	#$40			; No. Unusual ending?
	bne	nerwe12			; No. Bad NEC.
	lda	NECStatus+1		; Yes. Get ST1.
	asl				;
	ldx	#RecordNotFound		;
	bcs	nerwe16			;
	asl				;
	asl				;
	ldx	#BadCRC			;
	bcs	nerwe16			;
	asl				;
	ldx	#BadDMA			;
	bcs	nerwe16			;
	asl				;
	asl				;
	ldx	#RecordNotFound		;
	bcs	nerwe16			;
	asl				;
	ldx	#WriteProtect		;
	bcs	nerwe16			;
	asl				;
	ldx	#BadAddressMark		;
	bcs	nerwe16			;
nerwe12:ldx	#BadCntlr		;
nerwe16:txa				;
	ora	DiskOutStatus		;
	sta	DiskOutStatus		;
nerwe20:rts				;
;---------------------------------------;
FDCOutCmdErr:				;
	pha				;
	ldx	#0			;
	ldy	#0			;
lpfoc04:lda	FDCStatPortI		;
	and	#$40			;
	beq	nefoc04			;
	dey				;
	bne	lpfoc04			;
	dex				;
	bne	lpfoc04			;
	beq	nefoc08			;
nefoc04:ldx	#0			;
	ldy	#0			;
lpfoc08:lda	FDCStatPortI		;
	bmi	nefoc12			;
	dey				;
	bne	lpfoc08			;
	dex				;
	bne	lpfoc08			;
nefoc08:pla				;
	lda	DiskOutStatus		;
	ora	#TimeOut		;
	sta	DiskOutStatus		;
	pla				; Discard return address.
	pla				;
	sec				; Error exit.
	rts				;
nefoc12:pla				;
	sta	FDCDataPortIO		;
	clc				;
	rts				;
;---------------------------------------;
OpenMotor:				;
	lda	#0			;
	sta	MotorOnCount		; Motor count to maximun.
	lda	DiskIOZB+Drive		; This motor is on?
	clc				;
	adc	#$1f			;
	and	#$30			;
	bit	MotorStatus		;
	bne	neomr04			;
	ora	MotorStatus		; No. Motor select.
	sta	MotorStatus		;
	ora	DiskIOZB+Drive		;
	ora	#$0c			; Not reset and enable DMA/INT.
	sta	FDCCtrlPortO		; Turn on motor.
	jsr	Wait1Second		; Wait motor normal.
	lda	#0			;
	sta	MotorOnCount		; Motor count to maximun.
neomr04:rts				;
;---------------------------------------;
;    (A)= Seek track.			;
;---------------------------------------;
Seek:	sta	BiosTemp1		;
	lda	DiskIOZB+Drive		;
	clc				;
	adc	#1			;
	bit	SeekStatus		;
	bne	nesek08			;
	jsr	HomeTrack		;
	bcc	nesek04			;
	jsr	HomeTrack		;
	bcs	nesek12			;
nesek04:lda	#0			;
	ldx	DiskIOZB+Drive		;
	sta	CurrentTrack,x		;
nesek08:lda	BiosTemp1		; Seek track.
	ldx	DiskIOZB+Drive		;
	cmp	CurrentTrack,x		;
	clc				;
	beq	nesek12			;
	lda	#$0f			; Seek
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Drive		; Drive 0's head 0.
	jsr	FDCOutCmdErr		;
	lda	BiosTemp1		; Seek track.
	jsr	FDCOutCmdErr		;
	jsr	ChkStat2		;
	ldx	#$0f			; Head steady time 15ms.
lpsek04:ldy	#138			; Delay 1ms.
lpsek08:pha				; |3
	pla				; |4
	dey				; |2
	bne	lpsek08			; |3
	dex				;
	bne	lpsek04			;
	bcs	nesek12			;
	lda	DiskIOZB+Drive		;
	tax				;
	clc				;
	adc	#1			;
	ora	SeekStatus		;
	sta	SeekStatus		;
	lda	NECStatus+1		;
	sta	CurrentTrack,x		; Cy=0.
nesek12:rts				;
;---------------------------------------;
DetectMedia:				;
	sta	FDCSpeedPortO		;
	lda	#5			; Try 5 times.
	sta	BiosTemp1		;
lpdma04:lda	#$4a			; Read ID.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Drive		;
	jsr	FDCOutCmdErr		;
	jsr	WaitFDCInt		;
	bcs	nedma08			;
	lda	#7			;
	jsr	Results			;
	bcs	nedma08			;
	lda	NECStatus		; Get ST0.
	and	#$c0			; Normal ending?
	beq	nedma08			;
	dec	BiosTemp1		;
	bne	lpdma04			;
	lda	#8			;
	bit	NECStatus		; FDD not ready?
	beq	nedma04			;
	lda	#TimeOut		; Not ready.
	sta	DiskOutStatus		;
	sec				;
	rts				;
nedma04:lda	#1			; Set no eq.
nedma08:rts				;
;---------------------------------------;
HomeTrack:				;
	lda	#$07			; Home drive head.
	jsr	FDCOutCmdErr		;
	lda	DiskIOZB+Drive		;
	jsr	FDCOutCmdErr		;
	jsr	ChkStat2		;
	rts				;
;---------------------------------------;
;    Check interrupt status 2.		;
;---------------------------------------;
ChkStat2:				;
	jsr	WaitFDCInt		;
	bcs	necs204			;
	lda	#8			; Check interrupt status.
	jsr	FDCOutCmdErr		;
	lda	#2			;
	jsr	Results			; Return two result.
	bcs	necs204			;
	lda	NECStatus		;
	and	#$60			;
	cmp	#$60			;
	clc				;
	bne	necs204			;
	lda	DiskOutStatus		;
	ora	#BadSeek		;
	sta	DiskOutStatus		;
	sec				;
necs204:rts				;
;-----------------------------------------------;
;    Wait Floppy Disk Controller create INT.	;
;-----------------------------------------------;
WaitFDCInt:				;
	lda	#10			; 10 * 0.2s = 2 second time out.
lpwfi04:pha				;
	ldx	#118			;
lpwfi08:ldy	#0			;     |2
lpwfi12:bit	FDCIRQPortI		;|4
	bvs	newfi04			;|2
	dey				;|2
	bne	lpwfi12			;|3 = |2816
	dex				;     |2
	bne	lpwfi08			;     |3 = 333114.
	pla				;
	sec				;
	sbc	#1			;
	bne	lpwfi04			;
	lda	DiskOutStatus		;
	ora	#TimeOut		;
	sta	DiskOutStatus		;
	rts				;
newfi04:pla				;
	clc				;
	rts				;
;---------------------------------------;
;    Get FDC command's result.		;
;	Input	(A)= result bytes.	;
;---------------------------------------;
Results:ldx	#0			;
lprst04:pha				;
	txa				;
	pha				;
	ldx	#0			; 0.43 second TimeOut.
	ldy	#0			;
lprst08:lda	FDCStatPortI		; Check
	bmi	nerst04			;
	dey				;
	bne	lprst08			;
	dex				;
	bne	lprst08			;
	lda	DiskOutStatus		;
	ora	#TimeOut		;
	sta	DiskOutStatus		;
	pla				;
	pla				;
	sec				;
	rts				;
nerst04:lda	FDCStatPortI		; Has result?
	and	#$40			;
	beq	nerst08			;
	pla				; Yes.
	tax				;
	lda	FDCDataPortIO		;
	sta	NECStatus,x		;
	inx				;
	pla				;
	sec				;
	sbc	#1			;
	bne	lprst04			; wait 15us(25clk).
	ldx	#5			;
lprst12:dex				; 2
	bne	lprst12			; 3
	lda	#$10			; Read all.
	bit	FDCStatPortI		; FDC busy also?
	bne	nerst12			;
	clc				; No.
	rts				;
nerst08:pla				;
	pla				;
nerst12:lda	DiskOutStatus		;
	ora	#BadCntlr		;
	sta	DiskOutStatus		;
	sec				;
	rts				;
;---------------------------------------; Wait 1 second.
Wait1Second:				;
	lda	#50			; 8*1/8 second for motor delay.
lpw1s04:jsr	WaitmSecond		;
	sec				;
	sbc	#1			;
	bne	lpw1s04			;
	rts				;
;---------------------------------------; Wait 20ms.
WaitmSecond:				;
	ldy	#28			;
lpwms08:dex				;|2
	bne	lpwms08			;|3=|1280
	dey				;   |2
	bne	lpwms08			;   |3
	rts				;
;---------------------------------------;
SetBfPoint0:				;
	lda	DiskIOZB+Buffer+1	;
	ldx	DiskIOZB+BPage		;
	cmp	#$c0			; Disk buffer >= $c000
	bcc	nerwo40			;
	and	#$bf			; Yes. Last page.
	ldx	#$ff			;
nerwo40:sta	SectorBfPoint0+1	;
	cpx	BiosUsePage		; Set to protect page?
	bne	nerwo44			;
	ldx	DRAMPageNotUse		; Yes, change to replace page.
nerwo44:cpx	#0			;
	bne	nerwo46			;
	ldx	BiosUsePage		;
nerwo46:stx	DRAMPageSave		;
	jsr	HalPageX		;
	tax				;
	lda	DiskIOZB+Buffer		; Set buffer point to SectorBfPoint.
	sta	SectorBfPoint0		;
	rts				;

;=======================================;
;    Get DRAM size.			;
;	(A)= DRAM size.(in 16K).	;
;---------------------------------------;
GetDRAMSize:				;
	ldx	DRAMPageHigh		;
	inx				;
	txa				;
	rts				;
;=======================================;
;    Get and Set not use DRAM page.	;
;	(X)= Not use DRAM page.		;
;	(A)= Return original one.	;
;---------------------------------------;
SetPageNoUse:				;
	lda	DRAMPageNotUse		;
	stx	DRAMPageNotUse		;
	rts				;

;=======================================;
;    Set vector because last modify.	;
;---------------------------------------;
SetVector:				;
	jsr	DisableNMI		;
	lda	DRAMPageSave		; No.
	pha				;
	lda	#$ff			;
	jsr	HalPageA		;
	ldx	$bfff			; Change BRK Vector.
	ldy	$bffe			;
	stx	BRKAddressSave-$C000+$5800+1
	sty	BRKAddressSave-$C000+$5800
	ldx	$bffb			; Change NMI Vector.
	ldy	$bffa			;
	stx	NMIAddressSave-$C000+$5800+1
	sty	NMIAddressSave-$C000+$5800
	ldx	#<BRKINTEntry-$C000+$5800
	ldy	#>BRKINTEntry-$C000+$5800
	stx	$bfff			;
	sty	$bffe			;
	ldx	#<NMIEntry-$C000+$5800	;
	ldy	#>NMIEntry-$C000+$5800	;
	stx	$bffb			;
	sty	$bffa			;
nesvr04:pla				;
	jsr	HalPageA		;
	jsr	RestoreNMI		;
	lda	#$80			; Set Vector change flag.
	sta	VectorSet		;
	rts				;

;=======================================;
SetVectorRTI:				;
	cpx	BiosUsePage		; Set to protect page?
	bne	nesvi04			;
	ldx	DRAMPageNotUse		; Yes, change to replace page.
nesvi04:stx	NMIDRAMPage		;
	jsr	SetVector		;
	lda	#$c0			; Set NMI return flag.
	sta	VectorSet		;
	rts				;

;=======================================;
;    Restore vector because last modify.;
;---------------------------------------;
RstVector:				;
	jsr	DisableNMI		;
	lda	DRAMPageSave		; No.
	pha				;
	lda	#$ff			;
	jsr	HalPageA		; Restore BRK Vector.
	ldx	BRKAddressSave-$C000+$5800+1
	ldy	BRKAddressSave-$C000+$5800
	stx	$bfff			;
	sty	$bffe			; Restore NMI Vector.
	ldx	NMIAddressSave-$C000+$5800+1
	ldy	NMIAddressSave-$C000+$5800
	stx	$bffb			;
	sty	$bffa			;
	pla				;
	jsr	HalPageA		;
	jmp	RestoreNMI		;

;=======================================;
;    Get card version.			;
;	Output	(A)= Version.		;
;---------------------------------------;
GetCardVer:				;
	lda	#0			;
	rts				;

;=======================================;
;    Get Video RAM page size.		;
;	Must save X,Y.			;
;---------------------------------------;
GetVRAMSize:				;
	lda	VideoPageNumber		;
	rts				;
;===============================================;
;    Read key(joystick or keyboard)		;
;	Input	(Y)= 0: Read key.		;
;		     1: Test key.		;
;		(X)= Read device.		;
;		 Bit 7: Check key after VSyn.	;
;		 Bit 4: Press() key.		;
;		 Bit 2: Keyboard.		;
;		 Bit 1: Joystick 1.		;
;		 Bit 0: Joystick 0.		;
;	Return	(A) Bit 1: Keyb/-Joys.		;
;		    Bit 0: Joys2/-Joys1(Bit1=0).;
;			   Have/not key(Bit1=1).;
;		(X)= Keys code.			;
;		(Y)= Changed key.(Joys)		;
;		     Extend code.(KeyB).	;
;	Joystick code:				;
;		Bit 7: Right.			;
;		Bit 6: Left.			;
;		Bit 5: Down.			;
;		Bit 4: Up.			;
;		Bit 3: Start.			;
;		Bit 2: Select.			;
;		Bit 1: B.			;
;		Bit 0: A.			;
;	Use	BiosTemp1-5.			;
;-----------------------------------------------;
ReadKey:stx	BiosTemp2		;
	sty	BiosTemp3		;
	txa				;
	and	#$07			; Get keyboard or joystick?
	beq	nerj112			;
	txa				; Yes.
	and	#$10			; Check Press key?
	beq	ReadKeys		;
	txa				; Yes. Clear original key.
	lsr				; Get joystick 1?
	bcc	nerky04			;
	ldx	Joystick1Key		; Yes.
	stx	BiosTemp4		;
nerky04:lsr				; Get joystick 2?
	bcc	nerky08			;
	ldx	Joystick2Key		; Yes.
	stx	BiosTemp5		;
nerky08:lsr				; Get Keyboard?
	bcc	ReadKeys		;
lprky04:jsr	TestKeyBoardDev-$C000+$5800; Clear Keyboard.
	beq	ReadKeys		;
	jsr	ReadKeyBoardDev		;
	jmp	lprky04			;
;---------------------------------------;
ReadKeys:				;
	lda	BiosTemp2		;
	bpl	nerks04			;
	ldx	#1			; Yes. Wait VSyn.
	jsr	WaitVSynCnt		;
nerks04:lda	BiosTemp2		;
	lsr				;
	bcc	ReadJoystick2		;
ReadJoystick1:				;
	lda	Joystick1Key		; Get joystick 1's key.
	eor	BiosTemp4		;  (Key no change)?
	bne	nerj104			;
	lda	BiosTemp3		; Yes. Is read key function?
	beq	ReadJoystick2		; Yes.
	lda	BiosTemp2		; No. Test another device?
	and	#6			;
	bne	ReadJoystick2		; Yes.
	beq	nerj108		;(jmp)	; No. Test joystick1 only.
nerj104:tay				; Changed key.
	lda	BiosTemp2		; Is read press key?
	and	#$10			;
	beq	nerj108			;
	tya				; Yes.
	and	Joystick1Key		; Has key press
	beq	ReadJoystick2		; No.
nerj108:lda	#0			; Joystick 0.
nerj112:rts				;
;---------------------------------------;
ReadJoystick2:				;
	lda	BiosTemp2		; Joystick 2?
	and	#2			;
	beq	ReadKeyBoard0		;
	lda	Joystick2Key		; Get joystick 2's key.
	eor	BiosTemp5		;  (Key no change)?
	bne	nerj204			;
	lda	BiosTemp3		; Yes. Is read key function?
	beq	ReadKeyBoard0		; Yes.
	lda	BiosTemp2		; No. Test another device?
	and	#4			;
	bne	ReadKeyBoard0		; Yes.
	beq	nerj208		;(jmp)	; No. Test joystick2(1) only.
nerj204:tay				; Changed key.
	lda	BiosTemp2		; Is read press key?
	and	#$10			;
	beq	nerj208			;
	tya				; Yes.
	and	Joystick2Key		; Has key press
	beq	ReadKeyBoard0		; No.
nerj208:lda	#1			; Joystick 1.
	rts				;
;---------------------------------------;
ReadKeyBoard0:				;
	lda	BiosTemp2		; Read keyboard?
	and	#4			;
	beq	nerkb16			;
lprkb04:jsr	TestKeyBoardDev-$C000+$5800; Yes. Keyboard empty?
	cpy	#0			;
	beq	neekb12			; Yes.
	ldy	BiosTemp3		; No. Test keyboard function?
	bne	nerkb08			;
	jsr	ReadKeyBoardDev		; No. Read key from keyboard.
nerkb08:pha				;
	txa				; X/A -> Y/X.
	tay				;
	pla				;
	tax				;
	lda	#3			; Keyboard output.
	rts				;
neekb12:lda	BiosTemp3		; Read keyboard function?
	beq	nerkb16			;
	lda	#2			; No. No code return.
	rts				;
nerkb16:jmp	ReadKeys		; Yes.
;---------------------------------------;
;	Input	(X)= Joystick1/2	;
;	Joystick code:			;
;		Bit 7: Right.		;
;		Bit 6: Left.		;
;		Bit 5: Down.		;
;		Bit 4: Up.		;
;		Bit 3: Start.		;
;		Bit 2: Select.		;
;		Bit 1: B.		;
;		Bit 0: A.		;
;---------------------------------------;
GetJoystickKey:                         ;
	lda	#1			; Latch Joystick(and disable keyboard).
	sta	$4016			;
	lda	#0			;
	sta	$4016			;
	lda	#$80			; (A)=$80
lpgjk04:pha				;
	lda	$4016,x			;
	lsr				;
	pla				;
	ror				;
	bcc	lpgjk04			;
	rts				;

;=======================================;
;    Move block.			;
;---------------------------------------;
StandardAddress:			;
	cmp	#<$c000			;
	bcc	nesas04			;
	and	#$bf			;
	ldy	#$ff			;
nesas04:cpy	BiosUsePage		; Set to protect page?
	bne	nesas08			;
	ldy	DRAMPageNotUse		; Yes, change to replace page.
nesas08:cpy	#0			;
	bne	nesas12			;
	ldy	BiosUsePage		;
nesas12:rts                             ;
;-----------------------------------------------;
MoveBlockStruct	equ	$00			;
 BkSrcPage	equ	MoveBlockStruct		;
 BkSrcAddress	equ	MoveBlockStruct+1	;
 BkTarPage	equ	MoveBlockStruct+3	;
 BkTarAddress	equ	MoveBlockStruct+4	;
 BkLength	equ	MoveBlockStruct+6	;
;-----------------------------------------------;
MoveBlock:				;
	ldy	BkSrcPage,x		;
	lda	BkSrcAddress+1,x	; Modify source address and page.
	pha				;
	jsr	StandardAddress		;
	sta	BkSrcAddress+1,x	;
	sty	BiosTemp2		;
	ldy	BkTarPage,x		;
	lda	BkTarAddress+1,x	; Modify target address and page.
	pha				;
	jsr	StandardAddress		;
	sta	BkTarAddress+1,x	;
	sty	BiosTemp3		;
	ldy	BkLength,x		;
	beq	nembk04			;
	inc	BkLength+1,x		;
	ldy	#0			;
nembk04:sty	BiosTemp1		;
	lda	DRAMPageSave		;
	pha				;
lpmbk04:lda	BkLength+1,x		;
	cmp	#1			;
	bne	nembk08			;
	lda	BkLength,x		;
	sta	BiosTemp1		;
nembk08:lda	BiosTemp2		; Source page.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
lpmbk08:lda	(BkSrcAddress,x)	;
	sta	MoveBuffer,y		;
	inc	BkSrcAddress,x		;
	bne	nembk12			;
	inc	BkSrcAddress+1,x	;
nembk12:iny				;
	cpy	BiosTemp1		;
	bne	lpmbk08			;
	ldy	#0			;
	lda	BiosTemp3		; Target page.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
lpmbk12:lda	MoveBuffer,y		;
	sta	(BkTarAddress,x)	;
	inc	BkTarAddress,x		;
	bne	nembk16			;
	inc	BkTarAddress+1,x	;
nembk16:iny				;
	cpy	BiosTemp1		;
	bne	lpmbk12			;
	dec	BkLength+1,x		;
	bne	lpmbk04			;
	jmp	neebk28			;
;---------------------------------------;
ExchBlock:				;
	ldy	BkSrcPage,x		;
	lda	BkSrcAddress+1,x	; Modify source address and page.
	pha				;
	jsr	StandardAddress		;
	sta	BkSrcAddress+1,x	;
	sty	BiosTemp2		;
	ldy	BkTarPage,x		;
	lda	BkTarAddress+1,x	; Modify target address and page.
	pha				;
	jsr	StandardAddress		;
	sta	BkTarAddress+1,x	;
	sty	BiosTemp3		;
	ldy	BkLength,x		;
	beq	neebk04			;
	inc	BkLength+1,x		;
	ldy	#0			;
neebk04:sty	BiosTemp1		;
	lda	DRAMPageSave		;
	pha				;
	lda	BiosTemp2		;
	sta	DRAMPageSave		;
	jsr	HalPageA		;
lpebk04:lda	BkLength+1,x		;
	cmp	#1			;
	bne	neebk12			;
	lda	BkLength,x		;
	sta	BiosTemp1		;
neebk12:lda	BkSrcAddress+1,x	; Save source address.
	pha				;
	lda	BkSrcAddress,x		;
	pha				;
lpebk08:lda	(BkSrcAddress,x)	; Move source to MoveBuffer.
	sta	MoveBuffer,y		;
	inc	BkSrcAddress,x		;
	bne	neebk16			;
	inc	BkSrcAddress+1,x	;
neebk16:iny				;
	cpy	BiosTemp1		;
	bne	lpebk08			;
	ldy	#0			;
	lda	BiosTemp3		; Exchange target and MoveBuffer.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
lpebk12:lda	(BkTarAddress,x)	;
	pha				;
	lda	MoveBuffer,y		;
	sta	(BkTarAddress,x)	;
	pla				;
	sta	MoveBuffer,y		;
	inc	BkTarAddress,x		;
	bne	neebk20			;
	inc	BkTarAddress+1,x	;
neebk20:iny				;
	cpy	BiosTemp1		;
	bne	lpebk12			;
	pla				; Restore source.
	sta	BkSrcAddress,x		;
	pla				;
	sta	BkSrcAddress+1,x	;
	lda	BiosTemp2		;
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	ldy	#0			;
lpebk16:lda	MoveBuffer,y		;
	sta	(BkSrcAddress,x)	; Move source to MoveBuffer.
	inc	BkSrcAddress,x		;
	bne	neebk24			;
	inc	BkSrcAddress+1,x	;
neebk24:iny				;
	cpy	BiosTemp1		;
	bne	lpebk16			;
	dec	BkLength+1,x		;
	bne	lpebk04			;
neebk28:pla				;
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	pla				; Original target address.
	cmp	#<$c000			;
	bcc	neebk32			;
	lda	BkTarAddress+1,x	; Restore target address high byte.
	adc	#$40-1			;
	sta	BkTarAddress+1,x	;
neebk32:pla				; Original source address.
	cmp	#<$c000			;
	bcc	neebk48			;
	lda	BkSrcAddress+1,x	; Restore source address high byte.
	adc	#$40-1			;
	sta	BkSrcAddress+1,x	;
neebk48:rts				;

;=======================================;
;    Wait (X) times Vsyn.		;
;---------------------------------------;
WaitVSynCnt:				;
	stx	NMIEnterFlag		;
        jsr     EnableNMI               ;
lpwvc04:lda	NMIEnterFlag		;
	bne	lpwvc04			;
	jmp	RestoreNMI		;

;=======================================;
;    Allocate low RAM(5800-5fff).	;
;	Input	X|Y= Bytes.		;
;	Output	A=0: Allocate OK.	;
;		 X|Y= RAM Begin position;
;		A=1: Error		;
;		 X|Y= Maximum Length.	;
;---------------------------------------;
LowRAMAlloc:				;
	stx	BiosTemp2		;
	sty	BiosTemp1		;
	lda	LowRAMBegin+1		;
	cmp	#<$5800			;
	bcc	nelra04			;
        lda     LowRAMBegin             ;
	clc				;
	adc	BiosTemp1		;
	tay				;
	lda	LowRAMBegin+1		;
	adc	BiosTemp2		;
	cmp	#<$5c00			; Low memory overflow?
	bcc	nelra08			;
	ldx	#<$4400			;
	ldy	#>$4400			;
	stx	LowRAMBegin+1		;
	sty	LowRAMBegin		;
nelra04:lda	LowRAMBegin		;
	clc				;
	adc	BiosTemp1		;
	tay				;
	lda	LowRAMBegin+1		;
	adc	BiosTemp2		;
	tax				;
	cmp	#<$5800			;
	bcc	nelra08			; Low memory overflow?
	lda	#<$5800			;
	sec				;
	sbc	LowRAMBegin		;
	tay				;
	lda	#>$5800			; Yes.
	sbc	LowRAMBegin+1		;
	tax				;
	lda	#1			; Allocate Error!
	rts				;
nelra08:tax				; No.
	lda	LowRAMBegin+1		;
	stx	LowRAMBegin+1		;
	tax				;
	lda	LowRAMBegin		;
	sty	LowRAMBegin		;
	tay				;
	lda	#0			; Allocate OK!
	rts				;
;=======================================;
;    Free low RAM(5800-5fff).		;
;	Input	X|Y= Bytes.		;
;---------------------------------------;
LowRAMFree:				;
	stx	BiosTemp2		;
	sty	BiosTemp1		;
	lda	LowRAMBegin		;
	sec				;
	sbc	BiosTemp1		;
	sta	LowRAMBegin		;
	lda	LowRAMBegin+1		;
	sbc	BiosTemp2		;
	sta	LowRAMBegin+1		;
	rts				;
;---------------------------------------;
SwitchBG:				;
	txa				;
	pha				;
	tya				;
	pha				;
	ldx	SplitPoint		;
	lda	SplitScreenDt,x		;
	beq	nesbg08			;
	lda	$2002			;
	lda	SplitScreenDt+3,x	;
	sta	$2005			;
	lda	SplitScreenDt+4,x	;
	sta	$2005			;
	lda	VideoCtrlSave		;
	and	#$f3			; WORK=0.
	ora	#$40			; MODE=1.
	jsr	HalVideoCtrl		;
	sta	vDummy			; Queue INIT
lpsbg04:lda	SplitScreenDt,x		;
	beq	nesbg04			;
	sta	vDummy			; Write to SR queue
	lda	SplitScreenDt+2,x	;
	asl				;
	asl				;
	asl				;
	asl				;
	ora	SplitScreenDt+1,x	;
	sta	vDummy			; Write to VR queue
	txa				;
	clc				;
	adc	#5			;
	tax				;
	jmp	lpsbg04			;
nesbg04:stx	SplitPoint		;
	sta	vDummy			; START
	lda	VideoCtrlSave		;
	ora	#$44			; WORD=1,MODE=1
	sta	VideoCtrlSave		;
	and	#$f7			;
	jsr	HalVideoCtrl		;
	bne	nesbg12		;(jmp)	;
nesbg08:lda	VideoCtrlSave		;
	and	#$bb			; WORK=0,MODE=0.
	sta	VideoCtrlSave		;
	and	#$f3			;
	jsr	HalVideoCtrl		;
nesbg12:pla				;
	tay				;
	pla				;
	tax				;
	rts				;
;---------------------------------------;
;    SplitScreen.		 	;
;	Input	X|Y= Bytes.		;
;		0: Next count.		;
;		1: Page of 0000-07ff.	;
;		2: Page of 0800-0fff.	;
;		3: X panning.($2005).	;
;		4: Y panning.($2005).	;
;---------------------------------------;
SplitScreen:				;
	lda	$ff			;
	pha				;
	lda	$fe			;
	pha				;
	stx	$ff			;
	sty	$fe			;
	jsr	DisableNMI		;
	ldy	#0			; Because no AV12D at first row.
	lda	($fe),y			;
	sec				;
	sbc	#1			;
	sta	($fe),y			;
	sta	SplitScreenDt,y		;
	beq	nessn08			;
lpssn04:ldx	#5			;
lpssn08:lda	($fe),y			;
	sta	SplitScreenDt,y		;
	iny				;
	dex				;
	bne	lpssn08			;
	lda	($fe),y			;
	bne	lpssn04			;
	tya				;
	tax				;
        dec     SplitScreenDt-5,x       ;Last plus 1 for Line 241's AV12 count.
nessn08:sty	SplitPoint		; Point to last data.
nessn16:pla				;
	sta	$fe			;
	pla				;
	sta	$ff			;
	ldx	#<SplitScreenINT-$C000+$5800
	ldy	#>SplitScreenINT-$C000+$5800
	stx	BRKAddressSave-$C000+$5800+1
	sty	BRKAddressSave-$C000+$5800
	lda	SplitScreenDt		;
	bne	nessn20			;
	ldx	#<RTSInstruction-$C000+$5800
	ldy	#>RTSInstruction-$C000+$5800
	stx	SEndAddressSave-$C000+$5800+1
	sty	SEndAddressSave-$C000+$5800
	rts				;
nessn20:inc	SplitScreenDt		;
	rts				;
;=======================================;
;    Set one video page.	 	;
;	Input	(X)= Page position.	;
;		 0:0000-07ff		;
;		 1:0800-0fff		;
;		 2:1000-17ff		;
;		 3:1800-1fff		;
;		(Y)= Page.		;
;---------------------------------------;
SetVideoPage:				;
	txa				;
	asl				;
	asl				;
	asl				;
	tax				;
	tya				;
	jsr	HalSlotAX		;
	rts				;
;---------------------------------------;
;    Get All drive message.		;
;---------------------------------------;
GetDriveMessage:			;
	lda	SDriveMessage		; Diskette message.
	ldx	#0			;
	ldy	PCCardMessage		; PCCard message.
	rts				;
;---------------------------------------;
PointAdjust:				;
	clc				;
	adc	 $fe			;
	sta	 $fe			;
	bcc	nepad04			;
	inc	 $ff			;
nepad04:rts				;
;---------------------------------------;
;	Input	x|y= Address.		;
;---------------------------------------;
SendVRAMCmp:				;
	lda	 $fe			;
	pha				;
	lda	 $ff			;
	pha				;
	stx	 $ff			;
	sty	 $fe			;
	lda	$2002			; Initial send vram address.
lpsvc02:ldy	#1			;
	lda	($fe),y			; VRAM address.
	jsr	HalW2006			; High.
	dey				;
	lda	($fe),y			;
	jsr	HalW2006			; Low.
	lda	#$02			; pass first two point of vrom.
lpsvc04:jsr	PointAdjust		;
	ldy	#0			;
	lda	($fe),y			;
	cmp	#$ff			;
	beq	nesvc12			;
	cmp	#$7f			;
	beq	nesvc08			;
	tay				; length.
	bpl	nesvc04			;
;---------------------------------------; string.
	and	#$7f			; rep movsb.
	tax				;
	ldy	#1			;
lpsvc08:lda	($fe),y			;
	jsr	HalW2007			;
	iny				;
	dex				;
	bne	lpsvc08			;
	tya				;
	bne	lpsvc04			;
;---------------------------------------; pair.
nesvc04:tax				; Length.
	ldy	#$01			;
	lda	($fe),y			; Data.
lpsvc12:jsr	HalW2007			;
	dex				;
	bne	lpsvc12			;
	lda	#2			;
	bne	lpsvc04			;
nesvc08:lda	#1			;
	jsr	PointAdjust		;
	jmp	lpsvc02			;
nesvc12:pla				;
	sta	 $ff			;
	pla				;
	sta	 $fe			;
	rts				;
;---------------------------------------;
;    Load video data to VRAM/VROM.	;
;	Input	(X)= Point of zero page.;
;		 0-0: Data page.	;
;		 1-2: Data address.	;
;		 3-4: Video address.(Y!=0)
;		 5-6: Video data length.;
;---------------------------------------;
LoadVideoData:				;
	lda	DRAMPageSave		;
	pha				;
	lda	0,x			;
	cmp	BiosUsePage		; Set to protect page?
	bne	nelvd04			;
	lda	DRAMPageNotUse		; Yes, change to replace page.
nelvd04:sta	DRAMPageSave		;
	jsr	HalPageA		;
	tya				;
	bne	nelvd08			;
	ldy	1,x			;
	lda	2,x			;
	tax				;
	jsr	SendVRAMCmp		;
	jmp	nelvd20			;
nelvd08:bit	$2002			;
	lda	4,x			;
	jsr	HalW2006			;
	lda	3,x			;
	jsr	HalW2006			;
	lda	5,x			;
	beq	lplvd04			;
	inc	6,x			;
lplvd04:lda	(1,x)			;
	jsr	HalW2007			;
	inc	1,x			;
	bne	nelvd16			;
	inc	2,x			;
nelvd16:dec	5,x			;
	bne	lplvd04			;
	dec	6,x			;
	bne	lplvd04			;
nelvd20:pla				;
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	rts				;
;---------------------------------------;
;   Set User NMI vector.(Call this first)
;	X|Y = New vector address.	;
;---------------------------------------;
SetNMIVector:				;
	stx	NMIVectorAddr-$C000+$5800+1
	sty	NMIVectorAddr-$C000+$5800
	rts				;
;---------------------------------------;
;   Get User NMI vector.		;
;	X|Y = New vector address.	;
;---------------------------------------;
GetNMIVector:				;
	ldx	NMIVectorAddr-$C000+$5800+1
	ldy	NMIVectorAddr-$C000+$5800
	rts				;

;=======================================;
SoundBell:				;
	lda	#1			;
	sta	$4015			;
	lda	#$8f			;
	sta	$4000			;
	lda	#0			;
	sta	$4001			;
	lda	#$50			;
	sta	$4002			;
	lda	#$80			;
	sta	$4003			;
	rts				;

ScreenWidth	equ	31		;
;---------------------------------------;
;	Output	X|Y= Screen address.	;
;---------------------------------------;
GetCurAddress:				;
	lda	CurLine			; This line exceed VRAM page?
	asl				;
	asl				;
	asl				;
	ldx	#<$2000			; First VRAM.
	adc	ScrollPanningN		;
	bcs	negca04			;
	cmp	#240			;
	bcc	negca08			;
negca04:sec				; Yes.
	sbc	#240			;
	ldx	#<$2800			; Next VRAM.
negca08:ldy	#0			;
	sty	BiosTemp1		;
	asl				;
	rol	BiosTemp1		;
	asl				;
	rol	BiosTemp1		;
	ora	CurColumn		;
	tay				; VRAM address low.
	iny				; First column align.
	txa				;
	ora	BiosTemp1		;
	tax				; VRAM address high.
	lda	Addr2000N		; VRAM panning high enable?
	lsr				;
	lsr				;
	bcc	negca12			;
	txa				; Yes.
	eor	#8			;
	tax				;
negca12:rts				;
;=======================================;
WaitDisplay:				;
	lda	DispBfHead		;
	cmp	DispBfTail		;
	bne	WaitDisplay		;
	rts				;
;=======================================;
;    Display character in TTY mode.	;
;---------------------------------------;
DisplayTTY:				;
	txa				;
	cmp	#$07			;
	bne	nedty04			;
	jmp	SoundBell		;
nedty04:cmp	#$08			;
	bne	nedty16			; Backspace.
	ldy	CurColumn		;
	bne	nedty08			;
	ldx	CurLine			; In line begin.
	beq	nedty12			;
	dec	CurLine			; Not in screen begin.
	ldy	#ScreenWidth		;
nedty08:dey				; Column plus.
	sty	CurColumn		;
nedty12:rts				;
nedty16:cmp	#$0d			;
	bne	nedty20			;
	ldy	#0			; Return.
	sty	CurColumn		;
	rts				;
nedty20:cmp	#$0a			;
	beq	nedty40			; LineFeed.
;---------------------------------------;
	pha				; Save Character.
	jsr	GetCurAddress		;
lpdty04:lda	DispBfHead		; Wait display buffer has space.
	sec				;
	sbc	DispBfTail		;
	beq	nedty32			;
	cmp	#4			;
	bcc	lpdty04			;
nedty32:txa				;
	ldx	DispBfTail		;
	sta	DisplayBuffer,x		;
	inx				;
	tya				;
	sta	DisplayBuffer,x		;
	inx				;
	pla				;
	sta	DisplayBuffer,x		;
	inx				;
	stx	DispBfTail		;
	ldy	CurColumn		; Cursor column +1.
	iny				;
	cpy	#ScreenWidth		; Cursor column reach to line end?
	bcs	nedty36			;
	sty	CurColumn		;
nedty34:rts				;
nedty36:ldy	#0			; Yes.
	sty	CurColumn		;
nedty40:inc	CurLine			; Cursor line +1.
	lda	CurLine			;
	cmp	#30			; Cursor line reach to window end?
	bcc	nedty34			;
	lda	#0			;
	sta	BiosTemp1		;
	lda	ScrollPanningN		;
	asl				;
	rol	BiosTemp1		;
	asl				;
	rol	BiosTemp1		;
	pha				; VRAM address low.
	lda	BiosTemp1		;
	ora	#$20			;
	tax				; VRAM address high.
	lda	Addr2000N		; VRAM panning high enable?
	lsr				;
	lsr				;
	bcc	nedty44			;
	txa				; Yes.
	eor	#8			;
	tax				;
nedty44:txa				;
	pha				;
	lda	ScrollPanningN		; VRAM address accress
	clc				;
	adc	#8			; Cy = 1.
	cmp	#240			;
	bcc	nedty52			;
	sbc	#240			; Cy = 1.
	tay				;
lpdty08:lda	DispBfHead		; Wait display buffer has space.
	sec				;
	sbc	DispBfTail		;
	beq	nedty48			;
	cmp	#3			;
	bcc	lpdty08			;
nedty48:ldx	DispBfTail		;
	lda	#$80			;
	sta	DisplayBuffer,x		;
	inx				;
	lda	Addr2000N		;
	eor	#2			;
	sta	Addr2000N		;
	sta	DisplayBuffer,x		;
	inx				;
	stx	DispBfTail		;
	tya				;
nedty52:sta	ScrollPanningN		;
	tay				;
lpdty12:lda	DispBfHead		; Wait display buffer has space.
	sec				;
	sbc	DispBfTail		;
	beq	nedty56			;
	cmp	#3			;
	bcc	lpdty12			;
nedty56:ldx	DispBfTail		;
	lda	#$81			;
	sta	DisplayBuffer,x		;
	inx				;
	tya				;
	sta	DisplayBuffer,x		;
	inx				;
	stx	DispBfTail		;
	pla				;
	tax				;
	pla				;
	tay				;
	iny				; First column align.
	lda	#ScreenWidth		;
	sta	BiosTemp2		;
	lda	#$20			;
	jsr	SetCharCount		;
	lda	#30-1			;
	sta	CurLine			;
	rts				;
;=======================================;
DisplayChars:				;
	txa				;
	pha				;
	sty	BiosTemp2		; Count.
	lda	#ScreenWidth		;
	sec				;
	sbc	CurColumn		;
	cmp	BiosTemp2		;
	bcs	nedcs04			;
	sta	BiosTemp2		;
nedcs04:jsr	GetCurAddress		;
	pla				;
;---------------------------------------;
;	Input	X|Y= Screen address.	;
;		(A)= Character.		;
;		(BiosTemp2)= Count.	;
;---------------------------------------;
SetCharCount:				;
	pha				;
lpdcs04:lda	DispBfHead		; Wait display buffer has space.
	sec				;
	sbc	DispBfTail		;
	beq	nedcs08			;
	cmp	#5			;
	bcc	lpdcs04			;
nedcs08:txa				; Address high.
	ora	#$40			;
	ldx	DispBfTail		;
	sta	DisplayBuffer,x		;
	inx				;
	tya				; Address low.
	sta	DisplayBuffer,x		;
	inx				;
	pla				; Character
	sta	DisplayBuffer,x		;
	inx				;
	lda	BiosTemp2		; Count.
	sta	DisplayBuffer,x		;
	inx				;
	stx	DispBfTail		;
	rts				;
;=======================================;
;	Input	(X)= Cursor line.	;
;		(Y)= Cursor column.	;
;---------------------------------------;
SetCursor:				;
	stx	CurLine			;
	sty	CurColumn		;
	rts				;
;=======================================;
GetCursor:				;
	ldx	CurLine			;
	ldy	CurColumn		;
	lda	#30			; Screen lines.
	rts				;
;---------------------------------------;
ASCII8x8Font:				;
dw	0,	0,	0,	0,	$817E,	$81A5,	$99BD,	$7E81,	$FF7E,	$FFDB,	$E7C3,	$7EFF,	$FE6C,	$FEFE,	$387C,	$10
dw	$3810,	$FE7C,	$387C,	$10,	$7C38,	$FE38,	$7CFE,	$7C38,	$1010,	$7C38,	$7CFE,	$7C38,	0,	$3C18,	$183C,	0
dw	$FFFF,	$C3E7,	$E7C3,	$FFFF,	$3C00,	$4266,	$6642,	$3C,	$C3FF,	$BD99,	$99BD,	$FFC3,	$70F,	$7D0F,	$CCCC,	$78CC
dw	$663C,	$6666,	$183C,	$187E,	$333F,	$303F,	$7030,	$E0F0,	$637F,	$637F,	$6763,	$C0E6,	$5A99,	$E73C,	$3CE7,	$995A	
dw	$E080,	$FEF8,	$E0F8,	$80,	$E02,	$FE3E,	$E3E,	2,	$3C18,	$187E,	$7E18,	$183C,	$6666,	$6666,	$66,	$66	
dw	$DB7F,	$7BDB,	$1B1B,	$1B,	$633E,	$6C38,	$386C,	$78CC,	0,	0,	$7E7E,	$7E,	$3C18,	$187E,	$3C7E,	$FF18	
dw	$3C18,	$187E,	$1818,	$18,	$1818,	$1818,	$3C7E,	$18,	$1800,	$FE0C,	$180C,	0,	$3000,	$FE60,	$3060,	0	
dw	0,	$C0C0,	$FEC0,	0,	$2400,	$FF66,	$2466,	0,	$1800,	$7E3C,	$FFFF,	0,	$FF00,	$7EFF,	$183C,	0	
dw	0,	0,	0,	0,	$7830,	$7878,	$30,	$30,	$6C6C,	$6C,	0,	0,	$6C6C,	$6CFE,	$6CFE,	$6C	
dw	$7C30,	$78C0,	$F80C,	$30,	$C600,	$18CC,	$6630,	$C6,	$6C38,	$7638,	$CCDC,	$76,	$6060,	$C0,	0,	0	
dw	$3018,	$6060,	$3060,	$18,	$3060,	$1818,	$3018,	$60,	$6600,	$FF3C,	$663C,	0,	$3000,	$FC30,	$3030,	0	
dw	0,	0,	$3000,	$6030,	0,	$FC00,	0,	0,	0,	0,	$3000,	$30,	$C06,	$3018,	$C060,	$80	
dw	$C67C,	$DECE,	$E6F6,	$7C,	$7030,	$3030,	$3030,	$FC,	$CC78,	$380C,	$CC60,	$FC,	$CC78,	$380C,	$CC0C,	$78	
dw	$3C1C,	$CC6C,	$CFE,	$1E,	$C0FC,	$CF8,	$CC0C,	$78,	$6038,	$F8C0,	$CCCC,	$78,	$CCFC,	$180C,	$3030,	$30	
dw	$CC78,	$78CC,	$CCCC,	$78,	$CC78,	$7CCC,	$180C,	$70,	$3000,	$30,	$3000,	$30,	$3000,	$30,	$3000,	$6030	
dw	$3018,	$C060,	$3060,	$18,	0,	$FC,	$FC00,	0,	$3060,	$C18,	$3018,	$60,	$CC78,	$180C,	$30,	$30	
dw	$C67C,	$DEDE,	$C0DE,	$78,	$7830,	$CCCC,	$CCFC,	$CC,	$66FC,	$7C66,	$6666,	$FC,	$663C,	$C0C0,	$66C0,	$3C	
dw	$6CF8,	$6666,	$6C66,	$F8,	$607E,	$7860,	$6060,	$7E,	$607E,	$7860,	$6060,	$60,	$663C,	$C0C0,	$66CE,	$3E	
dw	$CCCC,	$FCCC,	$CCCC,	$CC,	$3078,	$3030,	$3030,	$78,	$C1E,	$C0C,	$CCCC,	$78,	$66E6,	$786C,	$666C,	$E6	
dw	$6060,	$6060,	$6060,	$7E,	$EEC6,	$FEFE,	$C6D6,	$C6,	$E6C6,	$DEF6,	$C6CE,	$C6,	$6C38,	$C6C6,	$6CC6,	$38	
dw	$66FC,	$7C66,	$6060,	$F0,	$CC78,	$CCCC,	$78DC,	$1C,	$66FC,	$7C66,	$666C,	$E6,	$CC78,	$70E0,	$CC1C,	$78	
dw	$30FC,	$3030,	$3030,	$30,	$CCCC,	$CCCC,	$CCCC,	$FC,	$CCCC,	$CCCC,	$78CC,	$30,	$C6C6,	$D6C6,	$EEFE,	$C6	
dw	$C6C6,	$386C,	$6C38,	$C6,	$CCCC,	$78CC,	$3030,	$78,	$6FE,	$180C,	$6030,	$FE,	$6078,	$6060,	$6060,	$78	
dw	$60C0,	$1830,	$60C,	$2,	$1878,	$1818,	$1818,	$78,	$3810,	$C66C,	0,	0,	0,	0,	0,	$FF00	
dw	$3030,	$18,	0,	0,	0,	$C78,	$CC7C,	$76,	$60E0,	$7C60,	$6666,	$DC,	0,	$CC78,	$CCC0,	$78	
dw	$C1C,	$7C0C,	$CCCC,	$76,	0,	$CC78,	$C0FC,	$78,	$6C38,	$F060,	$6060,	$F0,	0,	$CC76,	$7CCC,	$F80C	
dw	$60E0,	$766C,	$6666,	$E6,	$30,	$3070,	$3030,	$78,	$C,	$C0C,	$CC0C,	$78CC,	$60E0,	$6C66,	$6C78,	$E6	
dw	$3070,	$3030,	$3030,	$78,	0,	$FECC,	$D6FE,	$C6,	0,	$CCF8,	$CCCC,	$CC,	0,	$CC78,	$CCCC,	$78	
dw	0,	$66DC,	$7C66,	$F060,	0,	$CC76,	$7CCC,	$1E0C,	0,	$76DC,	$6066,	$F0,	0,	$C07C,	$C78,	$F8	
dw	$3010,	$307C,	$3430,	$18,	0,	$CCCC,	$CCCC,	$76,	0,	$CCCC,	$78CC,	$30,	0,	$D6C6,	$FEFE,	$6C	
dw	0,	$6CC6,	$6C38,	$C6,	0,	$CCCC,	$7CCC,	$F80C,	0,	$98FC,	$6430,	$FC,	$301C,	$E030,	$3030,	$1C	
dw	$1818,	$18,	$1818,	$18,	$30E0,	$1C30,	$3030,	$E0,	$DC76,	0,	0,	0,	$1000,	$6C38,	$C6C6,	$FE	
dw	$CC78,	$CCC0,	$1878,	$780C,	$CC00,	$CC00,	$CCCC,	$7E,	$1C,	$CC78,	$C0FC,	$78,	$C37E,	$63C,	$663E,	$3F	
dw	$CC,	$C78,	$CC7C,	$7E,	$E0,	$C78,	$CC7C,	$7E,	$3030,	$C78,	$CC7C,	$7E,	0,	$C078,	$78C0,	$380C	
dw	$C37E,	$663C,	$607E,	$3C,	$CC,	$CC78,	$C0FC,	$78,	$E0,	$CC78,	$C0FC,	$78,	$CC,	$3070,	$3030,	$78	
dw	$C67C,	$1838,	$1818,	$3C,	$E0,	$3070,	$3030,	$78,	$38C6,	$C66C,	$C6FE,	$C6,	$3030,	$7800,	$FCCC,	$CC	
dw	$1C,	$60FC,	$6078,	$FC,	0,	$C7F,	$CC7F,	$7F,	$6C3E,	$FECC,	$CCCC,	$CE,	$CC78,	$7800,	$CCCC,	$78	
dw	$CC00,	$7800,	$CCCC,	$78,	$E000,	$7800,	$CCCC,	$78,	$CC78,	$CC00,	$CCCC,	$7E,	$E000,	$CC00,	$CCCC,	$7E	
dw	$CC00,	$CC00,	$7CCC,	$F80C,	$18C3,	$663C,	$3C66,	$18,	$CC,	$CCCC,	$CCCC,	$78,	$1818,	$C07E,	$7EC0,	$1818	
dw	$6C38,	$F064,	$E660,	$FC,	$CCCC,	$FC78,	$FC30,	$3030,	$CCF8,	$FACC,	$CFC6,	$C7C6,	$1B0E,	$3C18,	$1818,	$70D8	
dw	$1C,	$C78,	$CC7C,	$7E,	$38,	$3070,	$3030,	$78,	$1C00,	$7800,	$CCCC,	$78,	$1C00,	$CC00,	$CCCC,	$7E	
dw	$F800,	$F800,	$CCCC,	$CC,	$FC,	$ECCC,	$DCFC,	$CC,	$6C3C,	$3E6C,	$7E00,	0,	$6C38,	$386C,	$7C00,	0	
dw	$30,	$6030,	$CCC0,	$78,	0,	$FC00,	$C0C0,	0,	0,	$FC00,	$C0C,	0,	$C6C3,	$DECC,	$6633,	$FCC	
dw	$C6C3,	$DBCC,	$6F37,	$3CF,	$1818,	$1800,	$1818,	$18,	$3300,	$CC66,	$3366,	0,	$CC00,	$3366,	$CC66,	0	
dw	$8822,	$8822,	$8822,	$8822,	$AA55,	$AA55,	$AA55,	$AA55,	$77DB,	$EEDB,	$77DB,	$EEDB,	$1818,	$1818,	$1818,	$1818	
dw	$1818,	$1818,	$18F8,	$1818,	$1818,	$18F8,	$18F8,	$1818,	$3636,	$3636,	$36F6,	$3636,	0,	0,	$36FE,	$3636	
dw	0,	$18F8,	$18F8,	$1818,	$3636,	$6F6,	$36F6,	$3636,	$3636,	$3636,	$3636,	$3636,	0,	$6FE,	$36F6,	$3636	
dw	$3636,	$6F6,	$FE,	0,	$3636,	$3636,	$FE,	0,	$1818,	$18F8,	$F8,	0,	0,	0,	$18F8,	$1818	
dw	$1818,	$1818,	$1F,	0,	$1818,	$1818,	$FF,	0,	0,	0,	$18FF,	$1818,	$1818,	$1818,	$181F,	$1818	
dw	0,	0,	$FF,	0,	$1818,	$1818,	$18FF,	$1818,	$1818,	$181F,	$181F,	$1818,	$3636,	$3636,	$3637,	$3636	
dw	$3636,	$3037,	$3F,	0,	0,	$303F,	$3637,	$3636,	$3636,	$F7,	$FF,	0,	0,	$FF,	$36F7,	$3636	
dw	$3636,	$3037,	$3637,	$3636,	0,	$FF,	$FF,	0,	$3636,	$F7,	$36F7,	$3636,	$1818,	$FF,	$FF,	0	
dw	$3636,	$3636,	$FF,	0,	0,	$FF,	$18FF,	$1818,	0,	0,	$36FF,	$3636,	$3636,	$3636,	$3F,	0	
dw	$1818,	$181F,	$1F,	0,	0,	$181F,	$181F,	$1818,	0,	0,	$363F,	$3636,	$3636,	$3636,	$36FF,	$3636	
dw	$1818,	$18FF,	$18FF,	$1818,	$1818,	$1818,	$F8,	0,	0,	0,	$181F,	$1818,	$FFFF,	$FFFF,	$FFFF,	$FFFF	
dw	0,	0,	$FFFF,	$FFFF,	$F0F0,	$F0F0,	$F0F0,	$F0F0,	$F0F,	$F0F,	$F0F,	$F0F,	$FFFF,	$FFFF,	0,	0	
dw	0,	$DC76,	$DCC8,	$76,	$7800,	$F8CC,	$F8CC,	$C0C0,	$FC00,	$C0CC,	$C0C0,	$C0,	$FE00,	$6C6C,	$6C6C,	$6C	
dw	$CCFC,	$3060,	$CC60,	$FC,	0,	$D87E,	$D8D8,	$70,	$6600,	$6666,	$7C66,	$C060,	$7600,	$18DC,	$1818,	$18	
dw	$30FC,	$CC78,	$78CC,	$FC30,	$6C38,	$FEC6,	$6CC6,	$38,	$6C38,	$C6C6,	$6C6C,	$EE,	$301C,	$7C18,	$CCCC,	$78	
dw	0,	$DB7E,	$7EDB,	0,	$C06,	$DB7E,	$7EDB,	$C060,	$6038,	$F8C0,	$60C0,	$38,	$CC78,	$CCCC,	$CCCC,	$CC	
dw	$FC00,	$FC00,	$FC00,	0,	$3030,	$30FC,	$30,	$FC,	$3060,	$3018,	$60,	$FC,	$3018,	$3060,	$18,	$FC	
dw	$1B0E,	$181B,	$1818,	$1818,	$1818,	$1818,	$D818,	$70D8,	$3030,	$FC00,	$3000,	$30,	$7600,	$DC,	$DC76,	0	
dw	$6C38,	$386C,	0,	0,	0,	$1800,	$18,	0,	0,	0,	$18,	0,	$C0F,	$C0C,	$6CEC,	$1C3C	
dw	$6C78,	$6C6C,	$6C,	0,	$1870,	$6030,	$78,	0,	0,	$3C3C,	$3C3C,	0,	0,	0,	0,	0	
;---------------------------------------;
PaletteTableASC	db	$0e,	$20,	$20,	$30
PaletteASCLen	equ	@-PaletteTableASC
;---------------------------------------;
VRAMBegin:				;
	ldy	#0			;
	bit	$2002			;
	jsr	HalW2006X			;
	jsr	HalW2006Y			;
	rts				;
;---------------------------------------;
Send2007_0:				;
	lda	#0			;
Send2007:				;
lps2704:jsr	HalW2007			;
	dey				;
	bne	lps2704			;
	rts				;
;=======================================;
DispInitial:				;
	lda	#0			; Close screen.
	sta	Addr2000		;
	jsr	HalW2000			;
	sta	$2001			;
	lda	VideoCtrlSave		; Disable count int.
	and	#$bb			;
	sta	VideoCtrlSave		;
	jsr	HalVideoCtrl		;
	lda	#0			;
	sta	SplitPoint		;
	jsr	InitVideoPort		;
	ldx	#<$0000			; Send 256 ASCII font.
	jsr	VRAMBegin		;
	ldx	#<ASCII8x8Font		;
	ldy	#>ASCII8x8Font		;
	stx	$fe+1			;
	sty	$fe			;
	ldx	#>256			;
	ldy	#0			;
lpcas04:lda	($fe),y			;
	jsr	HalW2007			;
	iny				;
	cpy	#8			;
	bne	lpcas04			;
	jsr	Send2007_0		;
	lda	$fe			;
	clc				;
	adc	#8			;
	sta	$fe			;
	bcc	necas08			;
	inc	$fe+1			;
necas08:dex				;
	bne	lpcas04			;
;---------------------------------------; Initial VRAM0.
	ldx	#<$2000			;
	jsr	VRAMBegin		;
	ldx	#<$03c0+255		; 960's space.
	ldy	#>$03c0			;
	lda	#$20			;
lpcas08:jsr	Send2007		;
	dex				;
	bne	lpcas08			;
	ldy	#$40			;
	jsr	Send2007_0		;
;---------------------------------------; Initial VRAM2.
	ldx	#<$2800			;
	jsr	VRAMBegin		;
        ldx     #<$03c0+255             ; 960's space.
	ldy	#>$03c0			;
	lda	#$20			;
lpcas12:jsr	Send2007		;
	dex				;
	bne	lpcas12			;
	ldy	#$40			;
	jsr	Send2007_0		;
;---------------------------------------; Initial Color palette.
	ldx	#1			; Wait Ysyn.
	jsr	WaitVSynCnt		;
	ldx	#<$3f00			;
	jsr	VRAMBegin		;
	ldy	#0			;
lpcas16:lda	PaletteTableASC,y	;
	jsr	HalW2007			;
	iny				;
	cpy	#PaletteASCLen		;
	bne	lpcas16			;
	ldx	#<$2000			;
	jsr	VRAMBegin		;
	lda	#0			;
	sta	$2005			;
	sta	$2005			;
	sta	CurLine			; Cursor.
	sta	CurColumn		;
	lda	#$0c			;
	sta	Addr2001		;
	sta	$2001			;
	bit	$2002			;
	lda	#$80			;
	sta	Addr2000		;
	rts				;

;=======================================;
CRCDWord	equ	$d0		; 4 byte.
CRCSave		equ	$d4		; |4 byte
SourceLength	equ	$d8		; |4 byte.
SourceAddress	equ	$dc		;|2
SourcePage	equ	$de		;|1
TargetAddress	equ	$df		; |2
TargetPage	equ	$e1		; |1
ZipFlag		equ	$e2		; 1
ZipType		equ	$e3		; 1
ReadAll		equ	$e4		; 1
BackDistance	equ	$e5		; 2
D5f68		equ	$e7		; 1
D5f5a		equ	$e8		; 1
D5f5e		equ	$e9		; 1
D527b		equ	$ea		; 1
;		$eb-ff:	Temp work unit.	;
;---------------------------------------;
ZeroSave	equ	$6100		;
SourceBuffer	equ	$6200		; $100 byte
TargetBuffer	equ	$6300		; $200+$140 byte.(Must is page align)
TargetBfLen	equ	$0800		;
TargetBfTail	equ	$6b00		;
TargetBfEnd	equ	$6b3f		;
;---------------------------------------;
TempBuffer	equ	$6700		; $10+$100+$100*2
D4da2		equ	$6b00		; $40+$40*2
D4de2		equ	$6b40		;
D4e62		equ	$6bc0		; $40+$40*2
D4ea2		equ	$6c00		;
;---------------------------------------;
HasiTable	equ	$6c00		; $400 byte.(Must is 4 byte align)
;---------------------------------------;
D4f22		equ	$6d00		; ds $100
D5022		equ	$6e00		; ds $200
;---------------------------------------;
D3da2		equ	$7000		; ds $200
D3fa2		equ	$7200		; ds $200
D41a2		equ	$7400		; ds $200
D43a2		equ	$7600		; ds $200
D45a2		equ	$7800		; ds $200
D47a2		equ	$7a00		; ds $200
D49a2		equ	$7c00		; ds $200
D4ba2		equ	$7e00		; ds $100
D4ca2		equ	$7f00		; ds $100
; PKZIP 1.10 zip file unzip program
;   PKZip 1.10 file head format: (1e bytes)
;	00-03:	DS 4	Tag		"PK",3,4
;*	06:	DB	Zip parameter
;*	08:	DB	Zip method	0: Store, 1: Shrunk, 6: Implode
;					8: Deflate(V2.04)
;*	0e-11:	DD	32 bit CRC
;*	12-15:	DD	Zip length
;	16-19:	DD	Original length
;	1a:	DB	File name length
;	1e-	DS (1a)	File name.
;   New Zip head format: (10 bytes)
;	00:	DB	Zip parameter
;	01:	DB	Zip method	0: Store, 1: Shrunk, 6: Implode
;					8: Deflate(V2.04)
;	02-05:	DD	32 bit CRC
;	06-09:	DD	Zip length
;===============================================;
;    UNZIP memory block.			;
;	Input	(X)= MoveBlockStruct		;
;		(Y)= Bit 7: Do CRC check.	;
;		     Bit 6: Source buffer paging;
;                    No use SetPage if no paging;
;	Output	(Y)= 0: CRC OK if check.	;
;-----------------------------------------------;
UnzipBlock:				;
	sty	BiosTemp1		; CRC check flag.
	ldy	#$30-1			;
lpubk04:lda	$d0,y			;
	sta	ZeroSave,y		;
	dey				;
	bpl	lpubk04			;
	lda	BkSrcPage,x		;
	sta	SourcePage		;
	lda	BkSrcAddress,x		;
	sta	SourceAddress		;
	lda	BkSrcAddress+1,x	;
	sta	SourceAddress+1		;
	lda	BkTarPage,x		;
	sta	TargetPage		;
	sta	BiosTemp2		; Target page.
	ldx	#<$8000			;
	ldy	#>$8000			;
	stx	TargetAddress+1		; BkTarAddress must be $8000.
	sty	TargetAddress		;
;---------------------------------------;
	lda	DRAMPageSave		; Save page
	pha				;
	jsr	ExplodeFile		;
	bit	BiosTemp1		;
	bpl	neubk16			;
	lda	#$ff			; Initial CRC
	sta	CRCDWord		;
	sta	CRCDWord+1		;
	sta	CRCDWord+2		;
	sta	CRCDWord+3		;
	jsr	BuildHSTable		;
lpubk08:ldx	#<$4000			;
	ldy	#>$4000			;
	lda	BiosTemp2		; Last page?
	cmp	TargetPage		;
	bcc	neubk04			;
	bne	neubk08			;
	lda	TargetAddress+1		; Yes.
	and	#<$7fff			;
	tax				;
	ora	TargetAddress		;
	beq	neubk08			;
	ldy	TargetAddress		;
neubk04:lda	BiosTemp2		;
	jsr	GetBlockCRC		; Use $f6-$ff.
	inc	BiosTemp2		;
	jmp	lpubk08			;
neubk08:ldx	#3			;
lpubk12:lda	CRCDWord,x		; Check CRC.
	eor	#$ff			;
	cmp	CRCSave,x		;
	bne	neubk12			;
	dex				;
	bpl	lpubk12			;
	ldy	#0			;
	beq	neubk16			;
neubk12:ldy	#1			;
neubk16:ldx	#$30-1			;
lpubk16:lda	ZeroSave,x		;
	sta	$d0,x			;
	dex				;
	bpl	lpubk16			;
	pla				; restore page.
	sta	DRAMPageSave		;
	jsr	HalPageA		;
	rts				;
;---------------------------------------;
;	Input	X|Y= Write length.	;
;		$8000.			;
;       Use     $f6-$ff                 ;
;---------------------------------------;
GetBlockCRC:				;
	stx	$fd			;
	sty	$fc			;
	tax				;
	jsr	SetPage-$C000+$5800	;
	tya				;
	beq	negbc04			;
	inc	$fd			;
negbc04:ldx	#<$8000			;
	ldy	#>$8000			;
	stx	$f6+1			;
	sty	$f6			;
	ldy	#0			;
lpgbc04:lda	($f6),y			;
	eor	CRCDWord		;
	tax				;
	lda	CRCDWord+1		;
	sta	CRCDWord		;
	lda	CRCDWord+2		;
	sta	CRCDWord+1		;
	lda	CRCDWord+3		;
	sta	CRCDWord+2		;
	lda	CRCDWord		;
	eor	HasiTable,x		;
	sta	CRCDWord		;
	lda	CRCDWord+1		;
	eor	HasiTable+$100,x	;
	sta	CRCDWord+1		;
	lda	CRCDWord+2		;
	eor	HasiTable+$200,x	;
	sta	CRCDWord+2		;
	lda	HasiTable+$300,x	;
	sta	CRCDWord+3		;
	iny				;
	bne	negbc08			;
	inc	$f7			;
negbc08:dec	$fc			;
	bne	lpgbc04			;
	dec	$fd			;
	bne	lpgbc04			;
	rts				;
;---------------------------------------;
PointAdjustUnzip:			;
	clc				;
	adc	0,x			;
	sta	0,x			;
	bcc	nepau04			;
	inc	1,x			;
nepau04:rts				;
;---------------------------------------;
SI	equ	$ee			;
BP	equ	$ef			;
DI	equ	$f0			;
DL	equ	$f2			;
DH	equ	$f3			;
CL	equ	$f4			;
CH	equ	$f5			;
AX	equ	$f6			;
AL	equ	$f6			;
AH	equ	$f7			;
;---------------------------------------;
;	Input	(Y)= Read length will.	;
;	Output	(BP)-1= Read length real;
;---------------------------------------;
ReadSourceBlock:			;
	sty	$ff			;
	lda	SourceLength		;
	sec				;
	sbc	$ff			;
	bcs	nersb16			;
	ldx	SourceLength+1		;
	bne	nersb12			;
	ldx	SourceLength+2		;
	bne	nersb08			;
	ldx	SourceLength+3		;
	bne	nersb04			;
	ldy	SourceLength		;
	lda	#0			;
	beq	nersb16			;
nersb04:dec	SourceLength+3		;
nersb08:dec	SourceLength+2		;
nersb12:dec	SourceLength+1		;
nersb16:sta	SourceLength		;
	sty	BP			;
	inc	BP			;
	tya				; Move total.
	ldx	#<SourceBuffer		;
	ldy	#>SourceBuffer		;
	stx	$fe+1			;
	sty	$fe			;
lpmse04:sta	$fa			;
	sta	$fb			;
	ldx	SourceAddress+1		;
	ldy	SourceAddress		;
	stx	$fc+1			;
	sty	$fc			;
	bit	BiosTemp1		;
	bvc	nemse08			;
	cpx	#<$c000-$0100		; Acceed to next perhaps?
	bcc	nemse04			;
	lda	$fc			; Remain length in page < move length?
	clc				;
	adc	$fa			;
	bcc	nemse04			;
	lda	#>$c000			;
	sbc	$fc			;
	sta	$fb			; Can move this time.
nemse04:ldx	SourcePage		; Page.
	jsr	SetPage-$C000+$5800	;
nemse08:ldy	#0			;
lpmse08:lda	($fc),y			;
	sta	($fe),y			;
	iny				;
	cpy	$fb			;
	bne	lpmse08			;
	tya				;
	clc				;
	adc	SourceAddress		;
	sta	SourceAddress		;
	bcc	nemse12			;
	inc	SourceAddress+1		;
	bit	BiosTemp1		; Source paged?
	bvc	nemse12			;
	bit	SourceAddress+1		; Yes. !(8000-bfff)?
	bvc	nemse12			;
	inc	SourcePage		; To next page.
	lda	#<$8000			;
	sta	SourceAddress		;
nemse12:lda	$fa			;
	cmp	$fb			;
	beq	nemse16			;
	sbc	$fb			;
	sta	$fa			;
	tya				;
	clc				;
	adc	$fe			;
	sta	$fe			;
	bcc	lpmse04			;
	inc	$ff			;
	jmp	lpmse04			;
nemse16:rts				;
;---------------------------------------;
;	Input	$f3|f2= Table address.	;
;---------------------------------------;
ReadTable:				;
	sty	$f0			; Table length.
	ldy	#1			;
	jsr	ReadSourceBlock		;
	ldy	SourceBuffer		;
	iny				;
	jsr	ReadSourceBlock		; Temp buffer.
	ldx	#0			;
lprbt04:lda	SourceBuffer,x		;
	lsr	a			;
	lsr	a			;
	lsr	a			;
	lsr	a			;
	tay				;
	sty	$f1			;
	lda	SourceBuffer,x		;
	and	#$0f			;
	clc				;
	adc	#1			;
lprbt08:sta	($f2),y			;
	dey				;
	bpl	lprbt08			;
	inx				;
	lda	$f2			;
	sec				; $f1+1
	adc	$f1			;
	sta	$f2			;
	bcc	nerbt04			;
	inc	$f3			;
nerbt04:dec	$f0			;
	beq	nerbt08			;
	lda	$f0			;
	sec				;
	sbc	$f1			;
	beq	nerbt08			;
	bcc	nerbt08			;
	sta	$f0			;
	bcs	lprbt04			;
nerbt08:rts				;
;---------------------------------------;
RoteLeft	db	$01,$02,$04,$08,$10,$20,$40,$80
BuildTable:				;
	ldy	#$3f			;
lpbte04:lda	D4da2,y			;
	sta	$ff			;
	cmp	#$10			;
	bcs	nebte12			;
	tya				;
	asl	a			;
	tax				;
	lda	D4de2,x			;
	pha				;
	ldx	$ff			;
	cpx	#8			;
	bcs	nebte04			;
	lda	RoteLeft,x		;
	tax				;
	lda	#0			;
	beq	nebte08			;
nebte04:lda	RoteLeft-8,x		;
	ldx	#0			;
nebte08:sta	$f1			; A|X= DI.
	stx	$f0			;
	pla				;
	tax				;
lpbte08:lda	$ff			;
	sta	D4ca2,x			;
	tya				;
	sta	D4ba2,x			;
	txa				;
	clc				;
	adc	$f0			;
	tax				;
	lda	#0			;
	adc	$f1			;
	beq	lpbte08			;
nebte12:tya				; ->0797
	asl	a			;
	tax				;
	lda	D4ea2+1,x		;
	sta	$f3			; $f3:f2= SI.
	lda	D4ea2,x			;
	sta	$f2			;
	lda	D4e62,y			;
	cmp	#9			;
	bcs	nebte16			;
	ldx	#<D3da2			;
	stx	$f1			;
	ldx	#>D3da2			;
	stx	$f0			;
	jmp	nebte28			;
nebte16:ldx	$f2			; DI
	beq	nebte20			;
	lda	#$ff			;
	sta	D3da2,x			;
	lda	#<D3fa2			;
	sta	$f1			;
	lda	#>D3fa2			;
	sta	$f0			;
	ldx	#4			;
	lda	$f3			;
lpbte12:lsr	 a			;
	ror	$f2			;
	dex				;
	bne	lpbte12			;
	lda	#4			;
	bne	nebte24			;
nebte20:lda	#<D41a2			; $f1|f0= BP.
	sta	$f1			;
	lda	#>D41a2			;
	sta	$f0			;
	lda	$f3			;
	sta	$f2			;
	lda	 #0			;
	sta	$f3			;
	lda	#8			;
nebte24:sta	$ff			;
	lda	D4e62,y			;
	sec				;
	sbc	$ff			;
nebte28:tax				; CL.
	tya				;
	clc				;
	adc	D5f68			; AL.
	jsr	FillTable		;
	dey				;
	bmi	nebte32			;
	jmp	lpbte04			;
nebte32:rts				;
;---------------------------------------;
BuildTableLarge:			;
	ldy	#0			;
lpbtl04:dey				;
	tya				; -0797
	asl	a			;
	tax				;
	bcs	nebtl04			;
	lda	D5022+1,x		;
	sta	$f3			;
	lda	D5022,x			;
	jmp	nebtl08			;
nebtl04:lda	D5022+$101,x		;
	sta	$f3			;
	lda	D5022+$100,x		;
nebtl08:sta	$f2			; $f3:f2= SI.
	ldx	D4f22,y			;
	cpx	#9			;
	bcs	nebtl12			;
	lda	#<D43a2			; $f1|f0= BP.
	sta	$f1			;
	lda	#>D43a2			;
	sta	$f0			;
	jmp	nebtl28			;
nebtl12:ldx	$f2			; ->080A
	beq	nebtl16			;
	lda	#$ff			;
	sta	D43a2,x			;
	lda	#$3f			;
	bit	$f2			;
	beq	nebtl20			;
	lda	#<D45a2			;
	pha				;
	lda	#>D45a2			;
	ldx	#4			;
	bne	nebtl24			;
nebtl16:lda	#<D49a2			;
	pha				;
	lda	#>D49a2			;
	ldx	#8			;
	bne	nebtl24			;
nebtl20:lda	#<D47a2			;
	pha				;
	lda	#>D47a2			;
	ldx	#6			;
nebtl24:sta	$f0			;
	pla				;
	sta	$f1			;
	stx	$ff			;
lpser08:lsr	$f3			;
	ror	$f2			;
	dex				;
	bne	lpser08			;
	lda	D4f22,y			;
	sec				;
	sbc	$ff			;
	tax				;
nebtl28:jsr	FillTableA		;
	tya				;
	bne	lpbtl04			;
	rts				;
;---------------------------------------;
;	Input:	$f1|f0= BP.		;
;		$f3:f2= SI.($f3=0)	;
;		A= AL.			;
;		X= CL.			;
;---------------------------------------;
FillTableA:				;
	tya				;
FillTable:				;
	sty	$ff			; Save Y.
	sta	$fe			;
	stx	$fd			;
	cpx	#8			;
	bcs	nefte04			;
	lda	RoteLeft,x		;
	tay				;
	lda	#0			;
	beq	nefte08			;
nefte04:lda	RoteLeft-8,x		;
	ldy	#0			;
nefte08:sta	$f5			; A|Y= DI.
	sty	$f4			;
	ldy	$f2			;
lpfte04:lda	$fe			;
	sta	($f0),y			;
	inc	$f1			;
	txa				;
	sta	($f0),y			;
	dec	$f1			;
	tya				;
	clc				;
	adc	$f4			;
	tay				;
	lda	$f5			;
	adc	#0			;
	beq	lpfte04			;
	ldy	$ff			;
	rts				;
;---------------------------------------;
;	Input	$f3|f2= Source.		;
;		$f1|f0= Target.		;
;		Y= Data length.		;
;---------------------------------------;
RoteRight	db	$80,$40,$20,$10,$08,$04,$02,$01
StructureData:				;
	sty	$fe			;
	ldy	#16			; ----
	lda	#0			;
lpsda04:sta	TempBuffer-1,y		;
	dey				;
	bne	lpsda04			;
	ldy	#0			; ----
lpsda08:lda	($f2),y			; a > 0.
	tax				;
	inc	TempBuffer-1,x		;
	iny				;
	cpy	$fe			;
	bne	lpsda08			;
	lda	#0			; ----
	tax				;
lpsda12:clc				;
	adc	TempBuffer,x		;
	sta	TempBuffer,x		;
	inx				;
	cpx	#$10			;
	bne	lpsda12			;
	ldy	$fe			; ----
lpsda16:dey				;
	lda	($f2),y			;
	tax				;
	dec	TempBuffer-1,x		;
	lda	TempBuffer-1,x		;
	tax				;
	lda	($f2),y			;
	sta	TempBuffer+$10,x	;
	tya				;
	bne	lpsda16			;
	lda	#0			; ----
	sta	$ff			; (CH)
	sta	$f4			; (AX): $f5|f4
	sta	$f5			;
	sta	$f6			;
	sta	$f7			;
	ldy	$fe			;
lpsda20:dey				;
	clc				;
	lda	$f4			;
	adc	$f6			;
	sta	$f4			;
	lda	$f5			;
	adc	$f7			;
	sta	$f5			;
	lda	TempBuffer+$10,y	;
	cmp	$ff			;
	beq	nesda12			;
	sta	$ff			;
	tax				;
	dex				; (X)= 0-15.
	cpx	#8			;
	bcs	nesda04			;
	lda	RoteRight,x		;
	ldx	#0			;
	beq	nesda08			;
nesda04:lda	RoteRight-8,x		;
	tax				;
	lda	#0			;
nesda08:sta	$f7			; A|X= DX.
	stx	$f6			;
nesda12:tya				;
	asl	a			;
	tax				;
	lda	$f4			;
	bcs	nesda16			;
	sta	TempBuffer+$110,x	;
	lda	$f5			;
	sta	TempBuffer+$111,x	;
	jmp	nesda20			;
nesda16:sta	TempBuffer+$210,x	;
	lda	$f5			;
	sta	TempBuffer+$211,x	;
nesda20:tya				;
	bne	lpsda20			;
	lda	$f0			; ----
	sta	$f4			;
	ldy	#0			;
lpsda24:lda	($f2),y			;
	tax				;
	tya				;
	pha				; Save Y.
	asl	a			;
	tay				;
	lda	$f1			;
	adc	#0			;
	sta	$f5			; $f4|f5 Target address.
	lda	TempBuffer-1,x		;
	inc	TempBuffer-1,x		;
	asl	a			;
	tax				;
	bcs	nesda24			;
	lda	TempBuffer+$110,x	;
	pha				; Save Low.
	lda	TempBuffer+$111,x	;
	jmp	nesda28			;
nesda24:lda	TempBuffer+$210,x	;
	pha				;
	lda	TempBuffer+$211,x	;
nesda28:ldx	#8			;
lpsda28:asl	a			;
	ror	$ff			;
	dex				;
	bne	lpsda28			;
	lda	$ff			;
	sta	($f4),y			;
	iny				;
	pla				; Restore Low.
	ldx	#8			;
lpsda32:asl	a			;
	ror	$ff			;
	dex				;
	bne	lpsda32			;
	lda	$ff			;
	sta	($f4),y			;
	pla				;
	tay				;
	iny				;
	cpy	$fe			;
	bne	lpsda24			;
	rts				;
;---------------------------------------;
BuildHSTable:				;
	lda	#>HasiTable		;
	sta	$f0			;
	ldy	#0			;
lpbhs04:dey				;
	lda	#0			;
	sta	$f2			; 2|3|4|5: AX|DX.
	sta	$f3			;
	sta	$f4			;
	sty	$f5			;
	ldx	#8			;
lpbhs08:lsr	$f2			;
	ror	$f3			;
	ror	$f4			;
	ror	$f5			;
	bcc	nebhs04			;
	lda	$f2			;
	eor	#$ed			;
	sta	$f2			;
	lda	$f3			;
	eor	#$b8			;
	sta	$f3			;
	lda	$f4			;
	eor	#$83			;
	sta	$f4			;
	lda	$f5			;
	eor	#$20			;
	sta	$f5			;
nebhs04:dex				;
	bne	lpbhs08			;
	lda	#<HasiTable		;
	sta	$f1			;
	ldx	#3			;
lpbhs12:lda	$f2,x			;
	sta	($f0),y			;
	inc	$f1			;
	dex				;
	bpl	lpbhs12			;
	tya				;
	bne	lpbhs04			;
	rts				;
;---------------------------------------;
;	Input	(X|Y)= Save length.	;
;---------------------------------------;
SaveFileBlock:				;
	sty	$fe			;
	txa				;
	ldx	TargetPage		;
	jsr	SetPageROM		;
	ldx	#<TargetBuffer		;
	ldy	#>TargetBuffer		;
	stx	$fc+1			;
	sty	$fc			;
	ldy	#0			;
	tax				;
	beq	nesfb12			;
lpsfb04:lda	($fc),y			;
	sta	(TargetAddress),y	;
	iny				;
	bne	lpsfb04			;
	inc	$fc+1			;
	inc	TargetAddress+1		;
	dex				;
	bne	lpsfb04			;
nesfb12:ldx	$fe			;
	beq	nesfb16			;
lpsfb08:lda	($fc),y			;
	sta	(TargetAddress),y	;
	iny				;
	dex				;
	bne	lpsfb08			;
	sty	TargetAddress		;
nesfb16:lda	TargetAddress+1		;
	cmp	#<$c000			;
	bcc	nesfb20			;
	lda	#<$8000			;
	sta	TargetAddress+1		;
	inc	TargetPage		;
nesfb20:rts				;
;---------------------------------------; SI output of TargetBuffer.
MoveBackTarget:				;
	lda	#>TargetBuffer		;
	sec				;
	sbc	$f8			; $f9|f8= Back distance from TargetBf.
	sta	$f8			;
	lda	#<TargetBuffer		;
	sbc	$f9			;
	sta	$f9			;
	ldx	TargetPage		;
	lda	TargetAddress		;
	sec				;
	sbc	$f8			;
	sta	$eb			; Low.
	lda	TargetAddress+1		;
	sbc	$f9			;
	bcs	nembt04			; Back to last page?
lpmbt04:cpx	BiosTemp2		; Yes. Back exceed to target buffer?
	beq	nembt20			; Yes. Fill with 0. 
	clc				;
	adc	#<$c000-$8000		;
	dex				;
nembt04:cmp	#<$8000			;
	bcc	lpmbt04			;
	sta	$eb+1			; High
	stx	$eb+2			; $ed|eb|ea= Back source buffer.
lpmbt08:lda	AL			; (AX)= Move length.
	sec				;
	sbc	$f8			;
	tay				;
	lda	AH			;
	sbc	$f9			;
	tax				; (AX)-($f9|f8) -> X|Y.
	bcs	nembt12			; Move length across TargetPage&TargetBf
	lda	AL			; No.
	sta	$f8			;
	lda	AH			;
	sta	$f9			; $f9|f8= Length of move from back source
	ldx	#0			;
	ldy	#0			;
nembt12:stx	AH			; AX= Move remain.
	sty	AL			;
lpmbt12:ldx	$eb+2			;
	jsr	SetPageROM		;
	lda	#$ff			;
	ldx	$eb+1			;
	cpx	#<$c000-0100		; Acceed to next perhaps?
	bcc	nembt14			;
	ldx	$eb			;
	beq	nembt14			;
	lda	#>$c000			; Remain length in page < move length?
	sbc	$eb			;
nembt14:sta	$fb			;
	tax				;
	lda	$f8			;
	sec				;
	sbc	$fb			; Move length >= 0xff?
	bcs	nembt16			;
	dec	$f9			;
	bpl	nembt16			;
	ldx	$f8			; No.
	lda	#0			;
	sta	$f9			;
nembt16:sta	$f8			;
	ldy	#0			;
lpmbt16:lda	($eb),y			;
	sta	(DI),y			;
	iny				;
	dex				;
	bne	lpmbt16			;
	ldx	#$eb			;
	tya				;
	jsr	PointAdjustUnzip	;
	ldx	#DI			;
	tya				;
	jsr	PointAdjustUnzip	;
	lda	$f8			;
	ora	$f9			;
	beq	nembt18			;
	lda	$eb+1			;
	cmp	#<$c000			;
	bcc	lpmbt12			;
	inc	$eb+2			;
	lda	#<$8000			;
	sta	$eb+1			;
	bne	lpmbt12			;
nembt18:rts				;
;---------------------------------------;
nembt20:stx	$eb+2			;
	sta	$eb+1			;
	lda	#>$8000			; Y|X= Length can fill.
	sec				;
	sbc	$eb			;
	tax				;
	lda	#<$8000			;
	sbc	$eb+1			; Y|X >= AX?
	tay				;
	txa				;
	cmp	AL			;
	tya				;
	sbc	AH			;
	bcc	nembt24			;
	ldx	AL			; Yes.
	ldy	AH			;
nembt24:stx	$eb			; Y|X= Length will fill.
	sty	$eb+1			;
	lda	AL			;
	sec				;
	sbc	$eb			;
	sta	AL			; AX= remain to move.
	lda	AH			;
	sbc	$eb+1			;
	sta	AH			;
	txa				;
	bne	nembt28			;	
	dey				;
nembt28:sty	$eb			; Length high
	lda	#0			;
	tay				;
lpmbt20:sta	(DI),y			;
	iny				;
	bne	nembt32			;
	inc	DI+1			;
nembt32:dex				; Length low.
	bne	lpmbt20			;
	dec	$eb			;
	bpl	lpmbt20			;
	tya				;
	clc				;
	adc	DI			;
	sta	DI			;
	bcc	nembt36			;
	inc	DI+1			;
nembt36:lda	AL			; Has any to move?
	ora	AH			;
	beq	nembt18			;
	sty	$eb			;
	lda	$f8			; Yes.
	sec				; Length of move from back source modify.
	sbc	$eb			;
	sta	$f8			;
	bcs	nembt40			;
	dec	$f9			;
nembt40:lda	#<$8000			;
	sta	$eb+1			; High
	lda	#>$8000			;
	sta	$eb			;
	jmp	lpmbt08			;
;---------------------------------------;
RestoreFile:				;
	lda	#0			;
	sta	ReadAll			;
	ldy	#$ff			;
	jsr	ReadSourceBlock		;
	ldx	#<TargetBuffer		; $f1|f0= DI.
	ldy	#>TargetBuffer		;
	stx	DI+1			;
	sty	DI			;
	ldx	#0			;
	stx	CH			;
	lda	SourceBuffer,x		;
	sta	DL			;
	stx	SI			;
	inc	SI			;
	dec	BP			;
	jmp	lprfe04			;
nerfe04:ldx	#<D47a2			; ->08fe
	ldy	#>D47a2			;
	lda	#6			;
nerfe08:sta	CL			;
	stx	AH			;
	sty	AL			;
	jsr	RoteSourceByte		; ->0903
	inc	AH			;
	lda	(AX),y			;
	sta	CL			;
	dec	AH			;
	lda	(AX),y			;
	sta	AL			;
	jmp	nerfe24			;
nerfe12:ldx	#<D49a2			; ->0910
	ldy	#>D49a2			;
	lda	#8			;
	bne	nerfe08			;
nerfe16:lda	#$3f			; ->0917
	bit	DL			;
	beq	nerfe04			;
	ldx	#<D45a2			;
	ldy	#>D45a2			;
	lda	#4			;
	bne	nerfe08			;
nerfe20:lda	DL			; ->0923
	beq	nerfe12			;
	lda	D43a2,y			; Y=BX?
	cmp	#$ff			;
	beq	nerfe16			;
	sta	AL			;
	lda	D43a2+$100,y		;
	sta	CL			;
nerfe24:jsr	RoteSourceByte		; ->0933
	lda	AL			;
	ldy	#0			;
	sta	(DI),y			;
	inc	DI			;
	bne	nerfe28			;
	inc	DI+1			;
nerfe28:jmp	nerfe72			;
nerfe32:lda	ZipType			; ->0939
	bne	nerfe20			;
	lda	DL			; DL.
	sta	AL			; AL.
	lda	#8			;
	sta	CL			;
	bne	nerfe24			;
nerfe36:ldx	#<D41a2			; ->0946
	ldy	#>D41a2			;
	lda	#8			;
nerfe40:sta	CL			; ->094b
	stx	AH			;
	sty	AL			;
	jsr	RoteSourceByte		;
	inc	AH			;
	lda	(AX),y			;
	sta	CL			;
	dec	AH			;
	lda	(AX),y			;
	sta	AL			;
	jmp	nerfe48			;
nerfe42:ldx	#<D3fa2			; ->0946
	ldy	#>D3fa2			;
	lda	#4			;
	bne	nerfe40		;(jmp)	;
lprfe04:lda	#1			; ->0958
	sta	CL			;
	jsr	RoteSourceByte		;
	bcs	nerfe32			;
	lda	DL			;
	and	D5f5a			;
	sta	AH			;
	lda	D5f5e			;
	sta	CL			;
	jsr	RoteSourceByte		; Y= BX.
	lda	D4ca2,y			;
	sta	CL			;
	lda	#0			;
	sta	BackDistance+1		;
	lda	D4ba2,y			;
	ldy	D5f5e			;
lprfe08:asl	a			;
	rol	BackDistance+1		;
	dey				;
	bne	lprfe08			;
	ora	AH			;
	sta	BackDistance		;
	jsr	RoteSourceByte		; Y= BX.
	lda	DL			;
	beq	nerfe36			;
	lda	D3da2,y			;
	bmi	nerfe42			;
	sta	AL			;
	lda	D3da2+$100,y		;
	sta	CL			;
nerfe48:jsr	RoteSourceByte		; ->0992
	lda	#0			;
	sta	AH			;
	lda	AL			;
	cmp	D527b			;
	bne	nerfe56			;
	clc				;
	adc	DL			;
	sta	AL			;
	bcc	nerfe52			;
	inc	AH			;
nerfe52:lda	#8			; Block copy (AX=Length).
	sta	CL			;
	jsr	RoteSourceByte		;
nerfe56:lda	DI			; ->09a6
	clc				;
	sbc	BackDistance		; Back Distance from target.
	sta	$f8			; $f9|f8= SI.
	lda	DI+1			;
	sbc	BackDistance+1		;
	sta	$f9			;
	bcc	nerfe60			;
	cmp	#<TargetBuffer		; Block source address before TargetBuffer?
	bcs	lprfe12			;
nerfe60:jsr	MoveBackTarget		; Yes. Copy from page buffer.
	ldx	#<TargetBuffer		;  Return AX=Remain in TargetBuffer
	ldy	#>TargetBuffer		;
	stx	$f8+1			;
	sty	$f8			;
lprfe12:lda	AL			;
	ora	AH			;
	beq	nerfe72			;
	ldy	#$ff			;
	lda	AL			;
	sec				;
	sbc	#$ff			;
	bcs	nerfe64			;
	dec	AH			;
	bpl	nerfe64			;
	ldy	AL			;
	lda	#0			;
	sta	AH			;
nerfe64:sta	AL			;
	sty	$ff			;
	ldy	#0			;
lprfe16:lda	($f8),y			;
	sta	(DI),y			;
	iny				;
	cpy	$ff			;
	bne	lprfe16			;
	ldx	#$f8			;
	tya				;
	jsr	PointAdjustUnzip	;
	ldx	#DI			;
	tya				;
	jsr	PointAdjustUnzip	;
	jmp	lprfe12			;
nerfe72:lda	DI+1			; ->09b3
	cmp	#<TargetBuffer+TargetBfLen
	bcs	nerfe80			;
	jmp	lprfe04			;
nerfe80:ldx	#<TargetBfLen		; ->9e4
	ldy	#>TargetBfLen		;
	jsr	SaveFileBlock		;
	lda	DI			;
	sta	$ff			;
	ldy	#0			;
	lda	DI+1			;
	sec				;
	sbc	#<TargetBfLen		;
	sta	DI+1			;
	cmp	#<TargetBuffer		;
	beq	lprfe28			;
lprfe20:lda	TargetBuffer+TargetBfLen,y
	sta	TargetBuffer,y		;
	iny				;
	bne	lprfe20			;
lprfe24:cpy	$ff			;
	beq	nerfe84			;
	lda	TargetBuffer+TargetBfLen+$100,y
	sta	TargetBuffer+$100,y	;
	iny				;
	jmp	lprfe24			;
lprfe28:cpy	$ff			;
	beq	nerfe84			;
	lda	TargetBuffer+TargetBfLen,y
	sta	TargetBuffer,y		;
	iny				;
	bne	lprfe28			;
nerfe84:jmp	lprfe04			;
nerfe88:pla				;
	pla				;
	pla				;
	pla				;
	lda	DI			;
	sec				;
	sbc	#>TargetBuffer		;
	tay				;
	lda	DI+1			;
	sbc	#<TargetBuffer		;
	tax				;
	rts				;
;---------------------------------------;
ReadNextBlock:				;
	lda	ReadAll			;
	bne	nerfe88			;
	ldy	#$ff			;
	jsr	ReadSourceBlock		;
	lda	#0			;
	sta	SI			;
	dec	BP			;
	bne	nernb04			;
	inc	BP			;
	inc	ReadAll			;
nernb04:rts				;
;---------------------------------------;
;	Input	($f5)= CH.		;
;		($f4)= CL.		;
;	Output	Cy.			;
;		(Y)= BX.		;
;---------------------------------------;
RoteSourceByte:				;
	lda	CH			;
	cmp	CL			;
	bcs	nesc412			;
	tax				;
	beq	nesc404			;
lpsc404:lsr	DH			;
	ror	DL			;
	dex				;
	bne	lpsc404			;
nesc404:dec	BP			;
	bne	nesc408			;
	jsr	ReadNextBlock		;
nesc408:ldx	SI			;
	lda	SourceBuffer,x		;
	inc	SI			;
	sta	DH			;
	lda	CL			;
	sec				;
	sbc	CH			;
	sta	CL			;
	lda	#8			;
	sec				;
nesc412:sbc	CL			;
	sta	CH			;
	ldx	CL			;
lpsc408:lsr	DH			;
	ror	DL			;
	dex				;
	bne	lpsc408			;
	ldy	DL			;
	rts				;
;=======================================;
ExplodeFile:				;
	lda	#$ff			;
	sta	SourceLength		;
	ldy	#10			; head.
	jsr	ReadSourceBlock		;
	lda	SourceBuffer		;
	sta	ZipFlag			;
	ldx	#7			; Get CRC and source length.
lpefe04:lda	SourceBuffer+2,x	;
	sta	CRCSave,x		;
	dex				;
	bpl	lpefe04			;
	ldx	#0			;
	lda	#$04			;
	bit	ZipFlag			;
	beq	neefe04			;
	ldx	#<D4f22			;
	ldy	#>D4f22			;
	stx	$f3			;
	sty	$f2			;
	ldy	#0			; ----
	jsr	ReadTable		;
	ldx	#<D4f22			; ----
	ldy	#>D4f22			;
	stx	$f2+1			;
	sty	$f2			;
	ldx	#<D5022			;
	ldy	#>D5022			;
	stx	$f0+1			;
	sty	$f0			;
	ldy	#$00			;
	jsr	StructureData		;
	jsr	BuildTableLarge		;
	ldx	#1			;
neefe04:stx	ZipType			;
	lda	#$02			;
	bit	ZipFlag			;
	bne	neefe08			;
	lda	#2			;
	sta	D5f68			;
	lda	#$3f			;
	sta	D5f5a			;
	lda	#6			;
	sta	D5f5e			;
	bne	neefe12			;
neefe08:lda	#3			;
	sta	D5f68			;
	lda	#$7f			;
	sta	D5f5a			;
	lda	#7			;
	sta	D5f5e			;
neefe12:lda	D5f68			;
	clc				;
	adc	#$3f			;
	sta	D527b			;
	ldx	#<D4e62			; ----
	ldy	#>D4e62			;
	stx	$f2+1			;
	sty	$f2			;
	ldy	#$40			;
	jsr	ReadTable		;
	ldx	#<D4da2			; ----
	ldy	#>D4da2			;
	stx	$f2+1			;
	sty	$f2			;
	ldy	#$40			;
	jsr	ReadTable		;
	ldx	#<D4e62			; ----
	ldy	#>D4e62			;
	stx	$f2+1			;
	sty	$f2			;
	ldx	#<D4ea2			;
	ldy	#>D4ea2			;
	stx	$f0+1			;
	sty	$f0			;
	ldy	#$40			;
	jsr	StructureData		;
	ldx	#<D4da2			; ----
	ldy	#>D4da2			;
	stx	$f2+1			;
	sty	$f2			;
	ldx	#<D4de2			;
	ldy	#>D4de2			;
	stx	$f0+1			;
	sty	$f0			;
	ldy	#$40			;
	jsr	StructureData		;
;---------------------------------------;
	jsr	BuildTable		;
	jsr	RestoreFile		;
	jmp	SaveFileBlock		;

;---------------------------------------;
KeyCodeTable	db	$05		; 0:01	4
		db	$22		; 0:02	G
		db	$21		; 0:04	F
		db	$2e		; 0:08	C
		db	$3c		; 0:10	F2
		db	$12		; 0:20	E
		db	$06		; 0:40	5
		db	$2f		; 0:80	V
		db	$03		; 1:01	2
		db	$20		; 1:02	D
		db	$1f		; 1:04	S
		db	$4f+$80		; 1:08	Alt/End
		db	$3b		; 1:10	F1
		db	$11		; 1:20	W
		db	$04		; 1:40	3
		db	$2d		; 1:80	X
		db	$52+$80		; 2:01	Ins
		db	$0e		; 2:02	BackSpace
		db	$51+$80		; 2:04	PgDn
		db	$4d+$80		; 2:08	Right
		db	$42		; 2:10	F8
		db	$49+$80		; 2:20	PgUp
		db	$53+$80		; 2:40	Del
		db	$47+$80		; 2:80	Home
		db	$0a		; 3:01	9
		db	$17		; 3:02	I
		db	$26		; 3:04	L
		db	$33		; 3:08	,
		db	$3f		; 3:10	F5
		db	$18		; 3:20	O
		db	$0b		; 3:40	0
		db	$34		; 3:80	.
		db	$1b		; 4:01	]
		db	$1c		; 4:02	Enter
		db	$48+$80		; 4:04	Up
		db	$4b+$80		; 4:08	Left
		db	$41		; 4:10	F7
		db	$1a		; 4:20	[
		db	$2b		; 4:40	\
		db	$50+$80		; 4:80	Down
		db	$10		; 5:01	Q
		db	$3a		; 5:02	Caps
		db	$2c		; 5:04	Z
		db	$55		; 5:08	Break/Pause
		db	$01		; 5:10	ESC
		db	$1e		; 5:20	A
		db	$02		; 5:40	1
		db	$1d		; 5:80	Ctrl
		db	$08		; 6:01	7
		db	$15		; 6:02	Y
		db	$25		; 6:04	K
		db	$32		; 6:08	M
		db	$3e		; 6:10	F4
		db	$16		; 6:20	U
		db	$09		; 6:40	8
		db	$24		; 6:80	J
		db	$0c		; 7:01	-
		db	$27		; 7:02	; 
		db	$28		; 7:04	'
		db	$35		; 7:08	/
		db	$40		; 7:10	F6
		db	$19		; 7:20	P
		db	$0d		; 7:40	+
		db	$2a		; 7:80	SHIFT
		db	$14		; 8:01	T
		db	$23		; 8:02	H
		db	$31		; 8:04	N
		db	$39		; 8:08	SpaceBar/Tab
		db	$3d		; 8:10	F3
		db	$13		; 8:20	R
		db	$07		; 8:40	6
		db	$30		; 8:80	B
;---------------------------------------; SB486LC last/SB486D next
		db	$ff		; 9:01
		db	$4b		; 9:02  <-(Left)
		db	$47		; 9:04  Home
		db	$57		; 9:08  F11
		db	$58		; 9:10  F12
		db	$4f		; 9:20  End
		db	$50		; 9:40  (Down)
		db	$48		; 9:80  (Up)
		db	$4a		; a:01  -(KeyPad)
		db	$4e		; a:02  +(KeyPad)
		db	$37		; a:04  $7c	*(KeyPad)
		db	$49		; a:08  PgUp
		db	$44		; a:10  F10
		db	$4c		; a:20  5(KeyPad)
		db	$35+$80		; a:40  /
		db	$45		; a:80  NumLock
		db	$29		; b:01  `~
		db	$4d		; b:02  ->(Right)
		db	$38		; b:04  Alt
		db	$0f		; b:08  Tab
		db	$43		; b:10  F9
		db	$51		; b:20  PgDn
		db	$53		; b:40  Del
		db	$52		; b:80  Ins
;---------------------------------------;
;	Output	Cy=1: No key.		;
;		  Zy=0: No scan code.	;
;		  Zy=1: No output.	;
;		Cy=0:			;
;		 (Y)= KeyCode.		;
;		 (C)= KeyCode byte.	;
;		 (A)= KeyCode bit.	;
;---------------------------------------;
CheckScanCode:				;
	lda	#5			;
	sta	$4016			;
	ldx	#0			;
lpcsc04:lda	#4			;
	sta	$4016			;
	pha				; Delay.
	pla				;
	lda	$4017			; D4-D1.
	and	#$1e			;
	ldy	#6			;
	sty	$4016			;
	lsr				;
	sta	CurrentKeyMap,x		;
	lda	$4017			; D4-D1.
	and	#$1e			;
	asl				;
	asl				;
	asl				;
	ora	CurrentKeyMap,x		;
	eor	#$ff			;
	sta	CurrentKeyMap,x		;
	inx				;
	cpx	#9			;
	bcc	lpcsc04			;
	bne	necsc04			;
	lda	#4			;
	sta	$4016			;
	lda	#6			;
	sta	$4016			;
;	lda	$4017			;
necsc04:cpx	#KeyboardDataLen	;
	bne	lpcsc04			;
	ldx	#0			; Key map same as last test?
lpcsc08:lda	CurrentKeyMap,x		;
	cmp	LastKeyMap,x		;
	bne	necsc12			;
	inx				;
	cpx	#KeyboardDataLen	;
	bne	lpcsc08			;
	txa				; Set Zy=0.
	sec				; Yes. No key press or release.
	rts				;
necsc12:eor	LastKeyMap,x		;
	sta	TempSave1		; (A)!=0.
	txa				;
	asl				;
	asl				;
	asl				;
	tay				;
	lda	#0			;
	sec				;
lpcsc16:iny				;
	rol				;
	bit	TempSave1		;
	beq	lpcsc16			;
	dey				;
	rts				; (Cy)=0.
;---------------------------------------;
;	Output	Cy=0: Has key output.	;
;		 X/A = E/B keycode.	;
;		Cy=1: Zy=0: No scan code;
;		      Zy=1: No output.	;
;---------------------------------------;
CheckScanCodeInp:				;
	jsr	HalKbdGet			; SB2000: XT scan code from the IRQ queue.
	bcs	necsi12			; No scan code.(Cy=1,Zy=0)
	cmp	#$e0			; Extend lead code?
	bne	necsi04			;
	lda	KeyStatus3		;
	ora	#$12			;
	sta	KeyStatus3		;
	lda	#0			; No output.(Cy=1,Zy=1)
	sec				;
	rts				;
necsi04:sta	TempSave2		;
	ldy	#0			;
	sty	AutoRepeatDelay		;
	jsr	GetKeyCode		;
	bcc	necsi12			;
	ldy	#0			; Cy=1 and No output.
necsi12:rts				;
;---------------------------------------;
CheckPrintScreen:			;
	cpx	#$37			; *(keypad) key?
	bne	necps04			;
	lda	#3			; Shift pressing?
	bit	KeyStatus1		;
	beq	necps04			;
	clc				;
	rts				;
necps04:sec				;
	rts				;
	
;=======================================; (X)=8. Print output.
PrintDataDev:				;
	sty	vDummy			; Port+1
	lda	#23			; Delay 10 second.
	ldx	#0			;
lppda04:bit	vDummy			; Port+1
	bmi	nepda04			;
	dey				;
	bne	lppda04			;
	dex				;
	bne	lppda04			;
	sec				;
	sbc	#1			;
	bne	lppda04			;
	sec				; Time out error.
	bcs	nepss04	;(jmp)		;
nepda04:lda	#$0d			;
	sta	vDummy			; Port+2
	pha				; Delay
	pla				;
	bne	neipt04		;(jmp)	;
;=======================================; (X)=9. Initial printer.
InitialPrintDev:				;
	lda	#$08			;
	sta	vDummy			;
	ldy	#10			; Delay.
lpipt04:dex				;
	bne	lpipt04			;
	dey				;
	bne	lpipt04			;
neipt04:lda	#$0c			;
	sta	vDummy			;
	pha				; Delay
	pla				;
;	bne	PrinterStatus	;(jmp)	;
;=======================================; (X)=10. Get printer status.
;	Output	(A)= Printer status.	;
;		(X)= Print port status.	;
;		 Bit 7: Print port precent.
;		 Bit 1: Autofeed always.;
;---------------------------------------;
PrinterStatusDev:
	clc				;
nepss04:lda	vDummy			;
	and	#$f8			;
	bcc	nepss08			;
	ora	#$01			;
nepss08:eor	#$48			;
	ldx	#$80			;
	rts				;

;---------------------------------------;
CheckMouse:				;
	asl				; Right/Down.
	bcc	neccr08			;
	pha				;
	lda	$5f00,x			;
	clc				;
	adc	MouseMovePlusC		;
	bcs	neccr02			;
	cmp	MaximumS,y		;
	bcc	neccr04			;
neccr02:lda	MaximumS,y		;
	sbc	#1			;
neccr04:sta	$5f00,x			;
	pla				;
neccr08:asl				; Left/Up
	bcc	neccr20			;
	pha				;
	lda	$5f00,x			;
	sbc	MouseMovePlusC		;
	bcc	neccr12			;
	cmp	MinimumS,y		;
	bcs	neccr16			;
neccr12:lda	MinimumS,y		;
neccr16:sta	$5f00,x			;
	pla				;
neccr20:rts				;
;---------------------------------------;
CheckJoystick:				;
	ldx	#0			;
	jsr	CheckJoystickOne	;
	ldx	#1			;
;	jmp	CheckJoystickOne	;
;---------------------------------------;
CheckJoystickOne:			;
	jsr	GetJoystickKey		; Check joystick X.
	sta	TempSave1		; Joystick code.
	eor	CurrentJoyKey,x		; (A)= Changed key.
	tay				;
	and	CurrentJoyKey,x		; (A)= Release  key.
	pha				;
	lda	TempSave1		;
	sta	CurrentJoyKey,x		; (Y)= Pressing key.
;---------------------------------------;
	tya				;
	and	CurrentJoyKey,x		; (A)= Press  key.
	beq	necjo24			;
	stx	TempSave2		;
	cpx	#0			;
	bne	necjo06			;
	ldx	#0			; Access  count.
lpcjo04:lsr				; Joystick 0. A/B/Sl/St.
	bcc	necjo04			;
	inc	PressCount0,x		;
necjo04:inx				;
	cpx	#4			;
	bne	lpcjo04			; Has motion key ?
	beq	necjo12		;(jmp)	;
necjo06:ldx	#0			;
lpcjo08:lsr				; Joystick 1. A/B/Sl/St.
	bcc	necjo08			;
	inc	PressCount1,x		;
necjo08:inx				;
	cpx	#4			;
	bne	lpcjo08			; Has motion key ?
necjo12:ldx	TempSave2		;
	asl				;
	beq	necjo24			;
	asl				;
	asl				;
	asl				;
	tay				;
	eor	CurrentJoyKey,x		; Yes. Motion key only ?
	and	#$f0			;
	bne	necjo16			;
	lda	#6			;
	sta	MovePrKeyCount,x	; Yes.
necjo16:tya				;
	and	LastRlMoveKey,x		; Yes. Is  immediately after 
	beq	necjo20			;
	asl	MouseMovePlus,x		; Yes. Mouse motion rate plus.
	bcc	necjo20			;
	ror	MouseMovePlus,x		;
necjo20:lda	#0			;
	sta	LastRlMoveKey,x		;
	sta	MoveRlKeyCount,x	;
;---------------------------------------;
necjo24:pla				; (A)= Release  key.
	and	#$f0			; Has motion key release?
	beq	necjo28			;
	sta	LastRlMoveKey,x		; Yes. Save the release key.
	lda	#5			; Set initial delay.
	sta	MoveRlKeyCount,x	;
	bne	necjo36		;(jmp)	;
necjo28:lda	MoveRlKeyCount,x	;
	beq	necjo36			;
	dec	MoveRlKeyCount,x	; Delay over?
	bne	necjo36			;
	lda	TempSave1		; Yes.
	and	#$f0			; Motion key no press?
	bne	necjo32			;
	lda	#1			;
	sta	MouseMovePlus,x		;
necjo32:lda	#0			;
	sta	LastRlMoveKey,x		;
necjo36:lda	MovePrKeyCount,x	; Press() last?
	beq	necjo40			;
	ldy	MouseMovePlus,x		; Yes. Continue press()?
	dey				;
	bne	necjo40			;
	dec	MovePrKeyCount,x	; No.
	cmp	#6			;
	bne	neskc12			;
necjo40:lda	TempSave1		;
	and	#$f0			;
	beq	neskc12			;
	ldy	MouseMovePlus,x		;
	sty	MouseMovePlusC		;
	dex				;
	beq	necjo44			;
	ldx	#>Mouse0X		; Mouse right/left
	ldy	#0			;
	jsr	CheckMouse		; 
	ldx	#>Mouse0Y		; Mouse down/up
	ldy	#2			;
	jmp	CheckMouse		;
necjo44:ldx	#>Mouse1X		; Mouse right/left
	ldy	#1			;
	jsr	CheckMouse		; 
	ldx	#>Mouse1Y		; Mouse down/up
	ldy	#3			;
	jmp	CheckMouse		;
;---------------------------------------;
SaveKeyCode:				;
	ldy	KBDTail			; And save to keyboard buffer.
	sta	KBDBuffer,y		;
	iny				;
	txa				;
	sta	KBDBuffer,y		;
	iny				; Modify buffer tail pointer.
	cpy	#KBDEnd			;
	bne	neskc04			;
	ldy	#KBDBegin		;
neskc04:cpy	KBDHead			; Keyboard full?
	bne	neskc08			;
	lda	#1			; Sound bell.
	sta	$4015			;
	lda	#$8f			;
	sta	$4000			;
	lda	#0			;
	sta	$4001			;
	lda	#$50			;
	sta	$4002			;
	lda	#$80			;
	sta	$4003			;
	rts				;
neskc08:sty	KBDTail			;
neskc12:rts				;
;=======================================;
KeyBoardNMIROM:				;
	txa				;
	pha				;
	tya				;
	pha				;
	jsr	CheckScanCodeInp	; Has keycode output?
	bcs	nenip04			;
	sta	BiosKeyCode		; Yes. Save bios key code.
	stx	BiosKeyCode+1		;
	bit	TempSave2		; Release key last?(for compose repeat)
	bmi	nenip08			;
	inc	AutoRepeatDelay		;
	bne	nenip08		;(jmp)	;
;---------------------------------------; Check auto repeat.
nenip04:beq	nenip24			; Has key change?
	lda	AutoRepeatDelay		; No. Last is keycode output?
	beq	nenip24			;
	cmp	DelayTimes		; Yes. Hold delay out?
	bcc	nenip12			;
	ldx	AutoRepeatCount		;
	inx				;
	cpx	RateTime		;
	bcc	nenip20			;
	lda	BiosKeyCode		; Save bios key code.
	ldx	BiosKeyCode+1		;
nenip08:jsr	SaveKeyCode		;
	jmp	nenip16			;
nenip12:inc	AutoRepeatDelay		;
nenip16:ldx	#0			;
nenip20:stx	AutoRepeatCount		;
nenip24:jsr	CheckJoystick		;
	pla				;
	tay				;
	pla				;
	tax				;
	rts				;
;===============================================;
;	Input	(X)=0: Read keyboard.		;
;		Output	X/A= Extend/Base key.	;
;	Input	(X)=1: Test keyeyboard.		;
;		Output	(Y)=0: No key.		;
;			(Y)=1: X/A= E/B key.	;
;	Input	(X)=2: Get keyboard status.	;
;		Output	X/A= Status2/Status1.	;
;	Input	(X)=3: Set type rate/Delay	;
;		(Y)= type rate/Delay.		;
;	Input	(X)=4: Release keyboard reident.;
;	Input	(X)=5: Save key back.		;
;		 $ff/Y= E/B key.		;
;	Input	(X)=6: Change INT check.	;
;		  (Y)= NMICheckFlag.		;
;		Output	X|Y= KeyBoardNMI point.	;
;	Input	(X)=7: Reset keyboard.		;
;		Output (A)=0: Reset OK.		;
;	Input	(X)=8: Print output.		;
;		(Y)= Character.			;
;	Input	(X)=9: Reset printer.		;
;		Output (A)= Printer status.	;
;		       (X)= Printor port status.;
;	Input	(X)=10:Get printer Status.	;
;		Output (A)= Printer status.	;
;		       (X)= Printor port status.;
;	Input	(X)=11: Get/Set CtrlBreak vector;
;		 Y|$ff|$fe= New vector.		;
;		Output A|X|Y= Original vector.	;
;	Input	(X)=12	Mouse initial.		;
;		 (Y)'Bit0: Mouse 1/-0		;
;		Output	(A)==0 Mouse Ok.	;
;	Input	(X)=13: Get mouse.		;
;		 (Y)'Bit0: Mouse 1/-0		;
;		Output	(A)= Current key pressing
;			Bit7: Right key pressing;
;			Bit6: Left key pressing.;
;			Bit5: Down key pressing.;
;			Bit4: Up key pressing.	;
;		 	Bit3: Start  key pressing
;		 	Bit2: Select key pressing
;		 	Bit1: B key pressing.	;
;		 	Bit0: A key pressing.	;
;			(X) = Mouse X.		;
;			(Y) = Mouse Y.		;
;			($FC)= A key press count;
;			($FC)= D key press count;
;			($FE)= Select key press count
;			($FF)= Start key press count
;	Input	(X)=14: Set mouse.		;
;		 (Y)'Bit0: Mouse 1/-0		;
;		 $FE/$FF: Mouse X/Y		;
;	Input	(X)=15: Set mouse range.	;
;		 (Y)'Bit0: Mouse 1/-0		;
;		 $FC/$FD: Mouse LU's X/Y.	;
;		 $FE/$FF: Mouse RD's X/T.	;
;-----------------------------------------------;
KBEBranch	dw	ReadKeyBoardDev-1,	TestKeyBoardROM-1
		dw	ReadStatusDev-1,	SetTypeRateDelayDev-1
		dw	ReleaseKeyBoardDev-1,	SaveBackKeyCodeDev-1
		dw	SetINICheckModeDev-1,	ResetKeyBoardDev-1
		dw	PrintDataDev-1,		InitialPrintDev-1
		dw	PrinterStatusDev-1,	GetSetBreakVectorDev-1
		dw	MouseResetDev-1,	GetMouseDev-1
		dw	SetMouseDev-1,		SetMouseRangeDev-1
		dw	SpeakInitDev-1,		SpeakWordDev-1
		dw	SpeakNoSoundDev-1
FunctionLimit	equ	@-KBEBranch	;
DeviceEntryROM:			;
	txa				;
	asl				;
	cmp	#FunctionLimit		;
	bcs	nekbe04			;
	tax				;
	lda	KBEBranch+1,x		;
	pha				;
	lda	KBEBranch,x		;
	pha				;
nekbe04:rts				;
;=======================================; (X)=0. Read keyboard.
ReadKeyBoardDev:				;
lprkd04:ldy	KBDHead			; Has any key in keyboard?
	cpy	KBDTail			;
	bne	nerkd04			;
	bit	NMICheckFlag		;
	bvc	lprkd04			;
	jsr	CheckScanCodeInp	;
	bcs	lprkd04			;
	jsr	SaveKeyCode		;
	jmp	lprkd04			;
nerkd04:lda	KBDBuffer,y		; Yes.
	iny				;
	ldx	KBDBuffer,y		;
	iny				;
	cpy	#KBDEnd			; Modify Keyboard head pointer.
	bcc	nerdk08			;
	ldy	#KBDBegin		;
nerdk08:sty	KBDHead			;
	rts				;
;---------------------------------------; (X)=1. Test keyboard.
TestKeyBoardROM:			;
lptkr04:ldy	KBDHead			; Has any key in keyboard?
	cpy	KBDTail			;
	beq	netkr04			;
	lda	KBDBuffer,y		; Yes.
	ldx	KBDBuffer+1,y		;
	ldy	#1			;
	rts				;
netkr04:bit	NMICheckFlag		;
	bvc	netkr08			;
	jsr	CheckScanCodeInp	;
	bcs	netkr08			;
	jsr	SaveKeyCode		;
	jmp	lptkr04			;
netkr08:ldy	#0			;
	rts				;
;=======================================; (X)=2
ReadStatusDev:				;
	lda	KeyStatus1		; Get status.
	ldx	KeyStatus2		;
	ldy	KeyStatus3		;
	rts				;
;---------------------------------------; (X)=3. Set Typematic rate/Delay.
RateTable	db	2,  3,  4,  5,  8,  11, 16, 22
SetTypeRateDelayDev:			;
	tya				;
	and	#$60			; (B65+1)*250ms/50ms
	asl				;
	rol				;
	rol				;
	rol				;
	tax				;
	lda	DelayTeble,x		;
	sta	DelayTimes		;
	tya				;
	and	#$1c			;
	lsr				;
	lsr				;
	tax				;
	lda	RateTable,x		;
	sta	RateTime		;
	rts				;
;---------------------------------------; (X)=5. Save keycode back to buffer.
SaveBackKeyCodeDev:			;
	ldx	KBDHead			; KBD head minus 2.
	dex				;
	dex				;
	bpl	nesbb04			;
	ldx	#KBDEnd-2		;
nesbb04:lda	Addr2000		; Disable NMI.
	pha				;
	and	#$7f			;
	jsr	HalW2000			;
	sta	Addr2000		;
	cpx	KBDTail			; Keyboard buffer full?
	bne	nesbb12			;
	txa				; Yes. Ignore last key.
	sec				;
	sbc	#2			;
	bpl	nesbb08			;
	lda	#KBDEnd-2		;
nesbb08:sta	KBDTail			;
nesbb12:stx	KBDHead			;
	pla				;
	sta	Addr2000		;
	jsr	HalW2000			; Restore NMI.
	tya				;
	sta	KBDBuffer,x		;
	lda	$ff			;
	sta	KBDBuffer+1,x		;
	rts				;
;---------------------------------------; (X)=6. Enable/Disable NMI check
SetINICheckModeDev:			;
	sty	NMICheckFlag		;
	ldx	#<KeyBoardNMIDo-$C000+$5800
	ldy	#>KeyBoardNMIDo-$C000+$5800
	rts				;
;---------------------------------------; (X)=7. Reset keyboard.
ResetKeyBoardDev:				;
	lda	#0			;
	sta	KeyStatus1		;
	sta	KeyStatus2		;
	sta	KeyStatus3		;
	sta	KeyStatus4		;
	ldx	#KBResetClearLen-1	;
lpkbd08:sta	KBResetClearBeg,x	;
	dex				;
	bpl	lpkbd08			;
	ldx	#26			;
	stx	DelayTimes		;
	ldx	#3			;
	stx	RateTime		;
;	lda	#0			;
	jsr	SendKBDLED		;
	lda	#0			;
	rts				;
;---------------------------------------; (X)=11
GetSetBreakVectorDev:			;
	rts				;
;---------------------------------------; (X)=12
MouseResetDev:				;
	tya				;
	and	#1			;
	tay				;
	lda	#0			;
	sta	CurrentJoyKey,y		;
	sta	MinimumX,y		;
	sta	MinimumY,y		;
	lda	#246			;
	sta	MaximumX,y		;
	lda	#239			;
	sta	MaximumY,y		;
	lsr				;
	dey				;
	beq	nemrt04			;
	sta	Mouse0Y			;
	lda	#246/3			;
	sta	Mouse0X			;
	lda	#0			;
	beq	negme04		;(jmp)	;
nemrt04:sta	Mouse1Y			;
	lda	#246/3*2		;
	sta	Mouse1X			;
	lda	#0			;
	beq	negme08		;(jmp)	;
;---------------------------------------; (X)=13
GetMouseDev:				;
	tya				;
	ldx	#4-1			;
	lsr				;
	bcs	lpgme08			;
lpgme04:lda	PressCount0,x		; Mouse0
	sta	$fc,x			;
	dex				;
	bpl	lpgme04			;
	lda	CurrentJoyKey		;
	ldx	Mouse0X			;
	ldy	Mouse0Y			;
negme04:pha				;
	lda	#0			;
	sta	PressACount0		;
	sta	PressBCount0		;
	sta	PressSlCount0		;
	sta	PressStCount0		;
	pla				;
	rts				;
lpgme08:lda	PressCount1,x		; Mouse0
	sta	$fc,x			;
	dex				;
	bpl	lpgme08			;
	lda	CurrentJoyKey+1		;
	ldx	Mouse1X			;
	ldy	Mouse1Y			;
negme08:pha				;
	lda	#0			;
	sta	PressACount1		;
	sta	PressBCount1		;
	sta	PressSlCount1		;
	sta	PressStCount1		;
	pla				;
	rts				;
;---------------------------------------; (X)=14
SetMouseDev:				;
	tya				;
	ldx	$fe			;
	ldy	$ff			;
	lsr				;
	bcs	nesme04			;
	stx	Mouse0X			;
	sty	Mouse0Y			;
	rts				;
nesme04:stx	Mouse1X			;
	sty	Mouse1Y			;
	rts				;
;---------------------------------------; (X)=15
SetMouseRangeDev:				;
	tya				;
	and	#1			;
	tay				;
	lda	$fc			;
	sta	MinimumX,y		;
	lda	$fd			;
	sta	MinimumY,y		;
	lda	$fe			;
	sta	MaximumX,y		;
	lda	$ff			;
	sta	MaximumY,y		;
	rts				;

;=======================================; (X)=16
; name:		SpeakInit
; description:  init
; input:	none
; output:       none
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

;=======================================; (X)=17
;	name:  SpeakWord
; description:  speak a word
;       input:  Y=SpeakWordPage
;	       $FE=SpeakWordAddrLo
;	       $FF=SpeakWordAddrHi
;      output:  none
;     changed:  A,X,Y
;	       BiosTemp1,BiosTemp4,BiosTemp5
;	call:  SpeakInit
;	       ROMBiosEntry (BiosGetPage,BiosSetPage)
;	       DeviceEntry(TestKeyBoard)
;*******************************************************
SpeakWordLenLow:	equ BiosTemp4
SpeakWordLenHi:         equ BiosTemp5
SpeakWordAddrLo:	equ $fe
SpeakWordAddrHi:	equ $ff

SpeakWordDev:
	tya
	pla
	jsr SpeakInitDev
	jsr GetPage-$C000+$5800
	sta BiosTemp1
	pla
	tax
	jsr SetPageROM
	ldy #$0
	lda (SpeakWordAddrLo),y
	sta SpeakWordLenHi
	iny
	lda (SpeakWordAddrLo),y
	sta SpeakWordLenLow
	iny
SpeakWordLp1:
	lda SpeakDataPort
	bpl SpeakWordLp1
	cmp #$ff
	beq SpeakWordLp1
	tya
	pha
	jsr TestKeyBoardDev-$C000+$5800
	pla
	cpy #0
	bne SpeakWordOver
	tay
	lda (SpeakWordAddrLo),y
	sta SpeakDataPort
	iny
	bne SpeakWordNe1
	inc SpeakWordAddrHi
SpeakWordNe1:
	sec
	lda SpeakWordLenLow
	sbc #$1
	sta SpeakWordLenLow
	bcs SpeakWordNe2
	dec SpeakWordLenHi
SpeakWordNe2:
	lda SpeakWordLenLow
	ora SpeakWordLenHi
	bne SpeakWordLp1
	beq SpeakWordOk
SpeakWordOver:
	ldx #$F0
SpeakWordLp10:
	lda SpeakDataPort
	bpl SpeakWordLp10
	lda #$00
	sta SpeakDataPort
	inx
	bne SpeakWordLp10
SpeakWordOk:
	ldx BiosTemp1
	jmp SetPageROM

;=======================================; (X)=18
SpeakNoSoundDev:
	lda #$0
	sta SpeakInitPort
	rts
;---------------------------------------;
CheckLockKey:				;
	ldx	#$40			;
	cmp	#$3a			; CapLock?
	beq	neclk04			;
	ldx	#$20			;
	cmp	#$45			; NumLock?
	beq	neclk04			;
	ldx	#$10			;
	cmp	#$46			; ScrollLock?
neclk04:rts				;
;---------------------------------------;
CheckPauseNL:				;
	lda	#8			; Pause key last?
	bit	KeyStatus3		;
	beq	necpl08			;
	cpx	#$45			; Yes. Current is NumLock
	beq	necpl04			;
	lda	KeyStatus3		;
	and	#$f7			;
	sta	KeyStatus3		;
necpl04:pla				;
	pla				;
	jmp	NoKeyCodeClr		;
necpl08:rts				;
;---------------------------------------;
CheckLockKeyAc:				;
	txa				;
	jsr	CheckLockKey		; Is Cap/Num/Scroll key?
	bne	necla04			;
	txa				; Yes.
	bit	KeyStatus2		; Lock key pressing?
	bne	necpl04			;
	ora	KeyStatus2		; No.
	sta	KeyStatus2		;
	txa				;
	eor	KeyStatus1		;
	sta	KeyStatus1		;
	jsr	ChangeMode		;
	jmp	necpl04			;
necla04:tax				;
	rts				;
;---------------------------------------;
ExcAltCode:				;
	lda	AltCode-1,x		;
	cmp	#$ff			;
	beq	necpl04			;
	rts				;
;---------------------------------------;
CheckInsKey:				;
	cpx	#$52			; Ins key?
	bne	nepst04			;
	lda	#$80			; Ins pressing?
	bit	KeyStatus2		;
	bne	necpl04			;
	ora	KeyStatus2		;
	sta	KeyStatus2		;
	lda	#$80			;
	eor	KeyStatus1		;
	sta	KeyStatus1		;
nepst04:rts				;
;---------------------------------------;
CheckHoldKey:				;
	ldx	#8			;
	cmp	#$38			; Alt
	beq	nechk04			;
	ldx	#4			;
	cmp	#$1d			; Ctrl
	beq	nechk04			;
CheckShiftKey:				;
	ldx	#2			;
	cmp	#$2a			; L-Shift
	beq	nechk04			;
	ldx	#1			;
	cmp	#$36			; R-Shift
nechk04:rts				;
;---------------------------------------;
Check101KB:				;
	lda	#$10			;
	bit	KeyStatus3		;
	rts				;
;---------------------------------------;
CheckLastE1:				;
	pha				;
	lda	KeyStatus3		;
	lsr				;
	pla				;
	rts				;
;---------------------------------------;
CheckLastE0:				;
	pha				;
	lda	KeyStatus3		;
	lsr				;
	lsr				;
	pla				;
	rts				;
;---------------------------------------;
ChangeMode:				;
	lda	#$40			;
	bit	KeyStatus4		;
	bne	necme12			;
	ora	KeyStatus4		; Set change mode flag.
	sta	KeyStatus4		;
	lda	KeyStatus1		;
	lsr				;
	lsr				;
	lsr				;
	lsr				;
	and	#7			;
	tax				; New LED in X.
	eor	KeyStatus4		;
	and	#7			;
	lda	KeyStatus4		; Mode Flag97's LEDs
	and	#$f8			;
	stx	KeyStatus4		;
	ora	KeyStatus4		;
	sta	KeyStatus4		;
	txa				;
	jsr	SendKBDLED		;
	lda	#$3f			;
	and	KeyStatus4		;
	sta	KeyStatus4		;
necme12:rts				;
;=======================================;
;	Input	(A)= Bios key code.	;
;	Output	Cy=1: No key.		;
;		Cy=0:			;
;		 X/A= Extend/Base key.	;
;---------------------------------------;
GetKeyCode:				;
	tax				;
	bmi	ReleaseKey		;
	jmp	PressKey		;
;---------------------------------------;
ReleaseKey:				;
	and	#$7f			;
	jsr	CheckLastE1		; Last lead $E1?
	bcc	nerly04			;
	jsr	CheckHoldKey		; Yes. Is Ctrl,Alt,Shift key?
	bne	nerly16			;
	jmp	NoKeyCode		;
nerly04:jsr	CheckLastE0		; Last lead $E0?
	bcc	nerly08			;
	jsr	CheckShiftKey		; Yes.
	beq	nerly16			;
nerly08:jsr	CheckLockKey		;
	beq	nerly12			;
	ldx	#$80			;
	cmp	#$52			; Release Ins key?
	bne	nerly20			;
nerly12:txa				;
	eor	#$ff			;
	and	KeyStatus2		;
	sta	KeyStatus2		;
nerly16:jmp	NoKeyCodeClr		;
nerly20:cmp	#$54			; SysRq key?
	bne	nerly24			;
	lda	KeyStatus3		; Clear SysRq key status.
	and	#$fb			;
	sta	KeyStatus3		;
;	lda	#1			;
;	jsr	SysReqKeyWait		;
	jmp	NoKeyCode		;
nerly24:jsr	CheckHoldKey		; Yes. Is Ctrl,Alt,Shift key?
	bne	nerly16			;
	pha				;
	txa				;
	eor	#$ff			;
	tay				; Save (X).
	and	KeyStatus1		;
	sta	KeyStatus1		;
	pla				;
	jsr	CheckShiftKey		; Is Ctrl,Atl?
	beq	nerly16			;
	tax				; Yes. Save Key.
	jsr	CheckLastE0		; Is Rights?
	bcc	nerly28			;
	tya				; Yes.
	and	KeyStatus2		; Clear R-Ctrl,R-Alt.
	sta	KeyStatus2		;
	bcs	nerly32			;
nerly28:tya				; No.
	lsr				;
	lsr				;
	ora	#$c0			;
	and	KeyStatus2		; Clear L-Ctrl,L-Alt.
	sta	KeyStatus2		;
nerly32:lda	KeyStatus2		; Get Ctrl,Alt status.
	asl				;
	asl				;
	ora	KeyStatus2		;
	and	#$0c			;
	ora	KeyStatus1		;
	sta	KeyStatus1		;
	cpx	#$38			; Release Alt key?
	bne	nerly16			;
	lda	KeyAltCompose		; Yes. Get Alt/keypad code.
	beq	nerly16			;
	ldx	#0			;
	stx	KeyAltCompose		;
	jmp	CreateKeyCode		;
ShiftCode62	db	$1b,$21,$40,$23,$24,$25,$5e,$26,$2a,$28,$29,$5f,$2b,$08,$00,$51
		db	$57,$45,$52,$54,$59,$55,$49,$4f,$50,$7b,$7d,$0d,$ff,$41,$53,$44
		db	$46,$47,$48,$4a,$4b,$4c,$3a,$22,$7e,$ff,$7c,$5a,$58,$43,$56,$42
		db	$4e,$4d,$3c,$3e,$3f,$ff,$2a,$ff,$20,$ff,$54,$55,$56,$57,$58,$59
		db	$5a,$5b,$5c,$5d,$ff,$ff,$37,$38,$39,$2d,$34,$35,$36,$2b,$31,$32
		db	$33,$30,$2e,$ff,$03,$7c,$87,$88
ShiftCodeE2	db	$1b,$31,$32,$33,$34,$35,$36,$37,$38,$39,$30,$2d,$3d,$08,$09,$71
		db	$77,$65,$72,$74,$79,$75,$69,$6f,$70,$5b,$5d,$0d,$ff,$61,$73,$64
		db	$66,$67,$68,$6a,$6b,$6c,$3b,$27,$60,$ff,$5c,$7a,$78,$63,$76,$62
		db	$6e,$6d,$2c,$2e,$2f,$ff,$2a,$ff,$20,$ff,$3b,$3c,$3d,$3e,$3f,$40
		db	$41,$42,$43,$44,$ff,$ff,$47,$48,$49,$2d,$4b,$f0,$4d,$2b,$4f,$50
		db	$51,$52,$53,$ff,$03,$5c,$85,$86
;---------------------------------------;
PressKey:				;
nepky08:jsr	CheckLastE0		; Last is $E0
	bcc	nepky12			;
	jsr	CheckShiftKey		; Yes.
	bne	nepky12			;
nepky04:jmp	NoKeyCodeClr		;
nepky12:jsr	CheckHoldKey		;
	bne	nepky20			;
	pha				;
	txa				;
	tay				;
	ora	KeyStatus1		;
	sta	KeyStatus1		;
	pla				;
	jsr	CheckShiftKey		;
	beq	nepky04			;
	tya				;
	jsr	CheckLastE0		; Is Alt or Ctrl.
	bcc	nepky16			;
	ora	KeyStatus2		; Set R-Alt or R-Ctrl
	sta	KeyStatus2		;
	bne	nepky04			;
nepky16:lsr				;
	lsr				;
	ora	KeyStatus2		; Set L-Alt or L-Ctrl
	sta	KeyStatus2		;
	bne	nepky04			;
nepky20:tax				;
	beq	nepky04			;
	cmp	#$58+1			;
	bcs	nepky04			;
	cmp	#$54			; SysReq key?
	bne	nepky28			;
	lda	#4			; Yes
	bit	KeyStatus3		; SysReq Pressing?
	bne	nepky04			;
	ora	KeyStatus3		;
	sta	KeyStatus3		;
;	lda	#0			;
;	jsr	SysReqKeyWait		;
nepky24:jmp	NoKeyCode		;
nepky28:lda	#8			; Alt key pressing?
	bit	KeyStatus1		;
	beq	nepky36			;
	jsr	Check101KB		; Yes. Is 101/102 Keyboard?
	beq	nepky32			;
	lda	#4			; Yes.
	bit	KeyStatus3		; SysReq Pressing?
	bne	nepky36			;
nepky32:jmp	PressAlt		;
nepky36:lda	#4			;
	bit	KeyStatus1		; Ctrl key pressing?
	beq	nepky38			;
	jmp	PressCtrl		;
nepky38:cpx	#$55			; No. Is Pause key?
	bne	nepky40			; Yes.
	lda	#8			;
	bit	KeyStatus3		; Pause status.
	bne	nepky04			; Yes.
	jmp	PressPause		;
nepky40:jsr	CheckLockKeyAc		;
	cpx	#$52			; Ins key?
	beq	nepky44			;
	jsr	CheckPauseNL		;
nepky44:jsr	CheckPrintScreen	;
	bcs	nepky56			;
;	jsr	PrintScreenAccess	;
	jmp	NoKeyCodeClr		;
nepky56:jsr	CheckLastE0		;
	bcs	nepky60			;
	lda	#3			; Shift pressing?
	bit	KeyStatus1		;
	beq	PressNoShift		;
	bne	PressShift		;
nepky60:cpx	#$1c			; Enter(keypad) key?
	bne	nepky64			;
	lda	#$0d			;
	bne	nepky68			;
nepky64:cpx	#$35			; /(keypad) key?
	bne	PressExtend		;
	lda	#$2f			;
nepky68:ldx	#$e0			;
	jmp	CreateKeyCode		;
;---------------------------------------;
PressShift:				;
	lda	#$20			; NumLock enable?
	bit	KeyStatus1		;
	beq	nepst08			;
	jsr	CheckInsKey		; Yes.
	cpx	#$46+1			;
	bcc	nepst08			;
	cpx	#$56			;
	bcs	nepst08			;
	bcc	nepst16			;
PressNoShift:				;
	lda	#$20			; NumLock enable?
	bit	KeyStatus1		;
	beq	nepst12			;
	cpx	#$46+1			; Yes.
	bcc	nepst16			;
	cpx	#$56			;
	bcs	nepst16			;
nepst08:lda	ShiftCode62-1,x		;
	jmp	nepst20			;
PressExtend:				;
nepst12:jsr	CheckInsKey		;
nepst16:lda	ShiftCodeE2-1,x		;
nepst20:cmp	#$ff			;
	bne	nepst24			;
	jmp	NoKeyCodeClr		;
nepst24:cpx	#$3a+1			; keypad key?
	bcc	nepst28			;
	cmp	#$3a+1			;
	bcc	nepst28			;
	cmp	#$f0			;
	beq	nepst28			;
	cpx	#$56			;
	beq	nepst28			;
	tax				;
	lda	#0			;
	jsr	CheckLastE0		;
	bcc	nepst28			;
	lda	#$e0			;
nepst28:tay				;
	lda	#$40			; CapLock enable?
	bit	KeyStatus1		;
	beq	nepst36			;
	lda	#3			;
	bit	KeyStatus1		; Shift pressing?
	bne	nepst32			;
	cpy	#'z'+1			; No. Change "a"-"z" -> "A"-"Z".
	bcs	nepst36			;
	cpy	#'a'			;
	bcc	nepst36			;
	tya				;
	and	#$df			;
	bne	CreateKeyCode		;
nepst32:cpy	#'Z'+1			;
	bcs	nepst36			;
	cpy	#'A'			;
	bcc	nepst36			;
	tya				;
	ora	#$20			;
	bne	CreateKeyCode		;
nepst36:tya				;
;---------------------------------------;
;	(A)= ASCII			;
;	(X)= Extend.			;
;---------------------------------------;
CreateKeyCode:				;
	tay				;
	lda	KeyStatus3		;
	and	#$fc			;
	sta	KeyStatus3		;
	tya				;
	cmp	#$f0			;
	bne	neckc04			;
	lda	#0			;
neckc04:clc				;
	rts				;
;---------------------------------------;
NoKeyCodeClr:				;
	lda	KeyStatus3		;
	and	#$fc			;
	sta	KeyStatus3		;
NoKeyCode:				;
	sec				;
	rts				;
CtrlCode	db	$1b,$ff,$00,$ff,$ff,$ff,$1e,$ff,$ff,$ff,$ff,$1f,$ff,$7f,$94,$11
		db	$17,$05,$12,$14,$19,$15,$09,$0f,$10,$1b,$1d,$0a,$ff,$01,$13,$04
		db	$06,$07,$08,$0a,$0b,$0c,$ff,$ff,$ff,$ff,$1c,$1a,$18,$03,$16,$02
		db	$0e,$0d,$ff,$ff,$95,$ff,$96,$ff,$20,$ff,$5e,$5f,$60,$61,$62,$63
		db	$64,$65,$66,$67,$45,$46,$77,$8d,$84,$8e,$73,$8f,$74,$90,$75,$91
		db	$76,$92,$93,$ff,$03,$ff,$89,$8a
;---------------------------------------;
PressCtrl:				;
	jsr	CheckPauseNL		;
	lda	CtrlCode-1,x		;
	cmp	#$ff			;
	beq	nepcl08			;
	cpx	#$45			; Ctrl/NumLock key?
	bne	nepcl16			;
;	jsr	Check101KB		; Yes. Is 101/102 Keyboard?
;	bne	nepcl08			;
PressPause:				;
	lda	#8			; No.
	ora	KeyStatus3		; Set Pause status.
	sta	KeyStatus3		;
	lda	NMIEnterFlag		; Save wait vsyn count.
	pha				;
	jsr	PauseRunningRAM-$C000+$5800
	pla				;
	sta	NMIEnterFlag		; Restore wait vsyn count.
	jmp	NoKeyCode		;
nepcl08:jmp	NoKeyCodeClr		;
nepcl16:cpx	#$55			; Ctrl/Break key?
	beq	nepcl20			;
	cpx	#$46			; Ctrl/Scroll key?
	bne	nepcl24			;
;	jsr	Check101KB		; Is 101/102 Keyboard?
;	beq	nepcl20			;
;	jsr	CheckLastE0		; Yes. Last is $E0
;	bcc	nepcl08			; No. Disable Ctrl/ScrollLock function.
nepcl20:lda	KBDHead			; Clear Keyboard Buffer
	sta	KBDTail			;
	jsr	CtrlBreakEntryRAM-$C000+$5800
	lda	#0			;
	tax				;
	jmp	CreateKeyCode		;
nepcl24:cpx	#$37			; Ctrl/*(keypad)or PrtSc key?
	bne	nepcl32			;
	tax				;
	jsr	Check101KB		; Yes. Is 101/102 keyboard
	beq	nepcl28			;
	txa				;
	jsr	CheckLastE0		; Yes. Last is $E0?
	bcc	nepcl44			;
nepcl28:lda	#$72			; Ctrl/PrtSc.
	bne	nepcl44			;
nepcl32:jsr	CheckLastE0		; Last is $E0?
	bcs	nepcl36			;
	cpx	#$0f			; No. Ctrl/Tab key?
	beq	nepcl44			;
	cpx	#$39			; Ctrl/Space key?
	beq	nepcl48			;
	cpx	#$35			; Ctrl// key?
	beq	nepcl08			;
	bcc	nepcl48			;
	bcs	nepcl44			;
nepcl36:cpx	#$35			; Ctrl//(keypad)
	beq	nepcl44			;
	cpx	#$1c			; Ctrl/Enter key?
	beq	nepcl40			;
	tax				;
	lda	#$e0			; No. ASCII= $E0.
	bne	nepcl48			;
nepcl40:ldx	#$e0			;
	bne	nepcl48			;
nepcl44:tax				; Extend code.
	lda	#0			; Ascii Code.
nepcl48:jmp	CreateKeyCode		;
;---------------------------------------;
AltCode		db	$f0,$78,$79,$7a,$7b,$7c,$7d,$7e,$7f,$80,$81,$82,$83,$f0,$a5,$10
		db	$11,$12,$13,$14,$15,$16,$17,$18,$19,$f0,$f0,$f0,$ff,$1e,$1f,$20
		db	$21,$22,$23,$24,$25,$26,$f0,$f0,$f0,$ff,$f0,$2c,$2d,$2e,$2f,$30
		db	$31,$32,$f0,$f0,$f0,$ff,$f0,$ff,$20,$ff,$68,$69,$6a,$6b,$6c,$6d
		db	$6e,$6f,$70,$71,$ff,$ff,$07,$08,$09,$f0,$04,$05,$06,$f0,$01,$02
		db	$03,$00,$53,$ff,$03,$ff,$8b,$8c
;---------------------------------------;
PressAlt:				;
	lda	#4			; Ctrl key pressing also?
	bit	KeyStatus1		;
	beq	nepat04			;
	cpx	#$53			; Is Ctrl/Alt/Del?
	bne	nepat08			;
	jmp	SoftResetEntry		; Reset.
nepat04:jsr	CheckLockKeyAc		;
nepat08:jsr	CheckPauseNL		;
	jsr	ExcAltCode		;
	jsr	CheckLastE0		; Last $E0?
	bcc	nepat24			;
	ldy	#0			; Yes.
	sty	KeyAltCompose		;
	cpx	#$1c			; Enter(keypad) key?
	bne	nepat16			;
	lda	#$a6			;
	bne	nepat36			;
nepat16:cpx	#$35			; /(keypad) key?
	bne	nepat20			;
	lda	#$a4			;
	bne	nepat36			;
nepat20:txa				;
	clc				;
	adc	#$50			;
	bne	nepat36			;
nepat24:cmp	#$0a			; Is Keypad number?
	bcs	nepat32			;
	tay				;
	lda	KeyAltCompose		; Flag19*10+(A)
	asl				;
	sta	KeyAltCompose		;
	asl				;
	asl				;
	clc				;
	adc	KeyAltCompose		;
	sty	KeyAltCompose		;
	clc				;
	adc	KeyAltCompose		;
	sta	KeyAltCompose		;
nepat28:jmp	NoKeyCodeClr		;
nepat32:ldy	#0			;
	sty	KeyAltCompose		;
	cpx	#$53			; Del key?
	beq	nepat28			;
	cpx	#$39			; Space key?
	beq	nepat40			;
	cmp	#$f0			;
	beq	nepat40			;
nepat36:tax				; Extend code.
	lda	#0			; Ascii Code.
nepat40:jmp	CreateKeyCode		;

;=======================================;
	include	hal-ppu.asm
	org	$ff00			; Reserve for I/O space.
	org	$ff00+1			; Reservrd for I/O
DelayTeble	db	13, 26, 48, 51	;
	org	$ff08			;
	org	$ff08+1			; Reservrd for I/O
;---------------------------------------;
SendKBDLED:				;
	eor	#7			; Bit 2: !CapsLock LED
	sta	KebBoardLEDPort		; Bit 1: !NumLock LED 
	rts				;
;---------------------------------------; (X)=4. Release keyboard.
ReleaseKeyBoardDev:			;
	rts				;
	org	$ff10			;
	org	$ff10+1			; Reservrd for I/O

	org	$fffa			;
	dw	NMIEntry-$C000+$5800	;
	dw	Begin			;
	dw	BRKINTEntry-$C000+$5800	;
        end
