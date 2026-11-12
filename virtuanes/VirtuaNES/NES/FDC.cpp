/*******************************************************************************
 *  Floppy Dsik Controller for BBK/YuXing
 *
 *  Author:  fanoble <87430545@qq.com>
 *
 *  Create:   Nov 6, 2022, by fanoble
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdarg>
#include <vector>
#include <queue>
#include <algorithm>
#include "fdc.h"

// 调试服务的磁盘访问钩子（定义在 DebugServer.cpp）
extern volatile long g_dbgActive;
void DebugHook_OnDisk(int lba, int write);

//==============================================================================
static unsigned char	DOR_SELECT_DRIVE =0x03;
static unsigned char	DOR_NOT_RESET =0x04; // "Software" reset
static unsigned char	DOR_MOTOR_A =0x10;
static unsigned char	DOR_MOTOR_B =0x20;
static unsigned char	DOR_MOTOR_C =0x40;
static unsigned char	DOR_MOTOR_D =0x80;
static unsigned char	MSR_READY =0x80;
static unsigned char	MSR_BUSY =0x10;
static unsigned char	MSR_DIRECTION =0x40;
static unsigned char	MSR_NO_DMA =0x20;
static unsigned char	CMD_MULTI_TRACK =0x80;
static unsigned char	CMD_NUMBER =0;
static unsigned char	CMD_US_HD =1;
static unsigned char	CMD_CYLINDER =2;
static unsigned char	CMD_HEAD =3;
static unsigned char	CMD_SECTOR_NUMBER =4;
static unsigned char	CMD_SECTOR_SIZE =5;
static unsigned char	CMD_LAST_SECTOR =6;
static unsigned char	CMD_FORMAT_SECTOR_SIZE =2;
static unsigned char	CMD_FORMAT_LAST_SECTOR =3;
static unsigned char	CMD_FORMAT_FILL =5;
static unsigned char	CMD_NCN =2;
static unsigned char	ST0_NORMAL_TERMINATION =0x00;
static unsigned char	ST0_ABNORMAL_TERMINATION =0x40;
static unsigned char	ST0_INVALID_COMMAND =0x80;
static unsigned char	ST0_POLLING =0xC0;
static unsigned char	ST0_SEEK_COMPLETE =0x20;
static unsigned char	ST0_NOT_READY =0x08;
static unsigned char	ST1_END_OF_TRACK =0x80;
static unsigned char	ST1_OVERRUN =0x10;
static unsigned char	ST1_NO_DATA =0x04;
static unsigned char	ST1_NOT_WRITEABLE =0x02;
static unsigned char	ST1_MISSING_ADDRESS_MARK =0x01;
static unsigned char	ST2_WRONG_CYLINDER =0x10;
static unsigned char	ST2_DATA_ERROR =0x20;
static unsigned char	ST1_DATA_ERROR =0x20;
static unsigned char	ST0_EQUIPMENT_CHECK =0x10;
static unsigned char	SRA_IRQ =0x80;
static unsigned char	CFG_NOT_POLL =0x10;
const CommandInfo FDC::commandInfo[0x20] ={
	{ 1, &FDC::cmInvalid }, 		// 00
	{ 1, &FDC::cmInvalid }, 		// 01
	{ 9, &FDC::cmInvalid }, 		// 02
	{ 3, &FDC::cmSpecify }, 		// 03
	{ 2, &FDC::cmSenseDriveStatus },	// 04
	{ 9, &FDC::cmWriteData },		// 05
	{ 9, &FDC::cmReadData },		// 06
	{ 2, &FDC::cmRecalibrate },		// 07
	{ 1, &FDC::cmSenseInterruptStatus },	// 08
	{ 9, &FDC::cmInvalid },			// 09
	{ 2, &FDC::cmReadID },			// 0A
	{ 1, &FDC::cmInvalid },			// 0B
	{ 9, &FDC::cmInvalid },			// 0C
	{ 6, &FDC::cmFormatTrack },		// 0D
	{ 1, &FDC::cmInvalid },			// 0E
	{ 3, &FDC::cmSeek },			// 0F
	{ 1, &FDC::cmInvalid },			// 10
	{ 9, &FDC::cmInvalid },			// 11
	{ 2, &FDC::cmInvalid },			// 12
	{ 4, &FDC::cmConfigure },		// 13
	{ 1, &FDC::cmInvalid },			// 14
	{ 1, &FDC::cmInvalid },			// 15
	{ 9, &FDC::cmInvalid },			// 16
	{ 9, &FDC::cmInvalid },			// 19
	{ 9, &FDC::cmInvalid },			// 1D
	{ 1, &FDC::cmInvalid },			// 1E
	{ 1, &FDC::cmInvalid }			// 1F
};
static const int dataRates[4] = { 500, 300, 250, 1000 };
//==============================================================================

FDC::FDC()
{
	bDirty = 0;
	bDiskModified = false;
	nDiskSize = 0;
	pDiskImage = 0;

	isYuXing = false;
	isSB97 = false;
	isBBK = false;
	dbgNoIndex = false;
	dbgSeekFail = false;
	dbgDead = false;

//==============================================================================
	resetPin =false;
	DOR =CONFIG =pollTimer =pollDrive =0;
	dataRate =dataRates[0];
	for (int i =0; i <4; i++) {
		previouslyReady[i] =false;
		Drive *d =&drives[i];
		d->exists =true;
		d->ready =true;
		d->inserted =false;
		d->running =false;
		d->cylinder =0;
		d->cylinders =0;
		d->sides =0;
		d->sectors =0;
		d->correctDataRate =0;
		d->data =NULL;
	}
	useDMA =false; // not affected by Reset
	drive =&drives[0];
	reset();
//==============================================================================
}

FDC::~FDC()
{
//==============================================================================
	ejectDisk(0);
//==============================================================================
	if (pDiskImage) free(pDiskImage);
}

int FDC::LoadDiskImage(const char* fname)
{
//==============================================================================
	ejectDisk(0);
//==============================================================================

	FILE* fp;

	// try to load disk image
	if (!(fp = ::fopen(fname, "rb"))) {
		return 0;
	}

	::fseek(fp, 0, SEEK_END);
	nDiskSize = ::ftell(fp);
	::fseek(fp, 0, SEEK_SET);

	if (pDiskImage) free(pDiskImage);

	if (!(pDiskImage = (unsigned char*)malloc(nDiskSize))) {
		fclose(fp);
		return 0;
	}

	::fread(pDiskImage, nDiskSize, 1, fp);
	fclose(fp);

	bDirty = 0;
	strcpy(szDiskName, fname);
	dbgNoIndex = false;
	dbgSeekFail = false;
	dbgDead = false;
	dbgBadLba.clear();

	// 修改标志放在成员里，写扇区时控制器经指针置位
	bDiskModified = false;
	insertDisk(0, pDiskImage, nDiskSize, false, &bDiskModified);

	return 1;
}

int FDC::SaveDiskImage()
{
	FILE* fp;

	if (!(fp = ::fopen(szDiskName, "wb"))) {
		return 0;
	}
	
	if (!::fwrite(pDiskImage, nDiskSize, 1, fp)) {
		fclose(fp);
		return 0;
	}

	fclose(fp);

	bDirty = 0;
	bDiskModified = false;

	return 1;
}

//==============================================================================
//SimpleFDC code form NintendulatorNRS by NewRisingSun, used by YuXing and SB97.
//==============================================================================
void	FDC::run (void) {
	if (dbgDead) return;
	if (DOR &DOR_NOT_RESET && !resetPin) (this->*phaseHandler)();
}

void	FDC::reset () {
	phaseHandler =&FDC::phCommand;
	MSR =MSR_READY;
	SRA =ST0 =ST1 =ST2 =0x00;
	data =NULL;
	bytesLeft =0;
	if(isBBK)
		cylinder =head =sectorNumber =sectorSize =0;
	command.clear();
	while (!resultQueue.empty()) resultQueue.pop();
	dmaRequest =dmaTerminate =false;
	
	pollTimer =pollDrive =0;
	for (int i =0; i <4; i++) previouslyReady[i] =false;
}

void	FDC::phCommand (void) {
	pollDrives();
	if (~MSR &MSR_READY) {	// Data byte has been written to the latch
		MSR |=MSR_BUSY;
		command.push_back(latch);

		// If all required bytes of the desired command have been sent,
		// start the Execution Phase, otherwise wait for more bytes
		if (command.size() ==commandInfo[command[CMD_NUMBER] &0x1F].length) {
			char commandString[1024] ="Cmd: ";
			for (std::vector<unsigned char>::iterator it = command.begin(); it !=command.end(); ++it) {
				unsigned char c = *it;
				char single[32];
				_snprintf(single, sizeof(single)-1, "%02X ", c);
				single[sizeof(single)-1] = '\0';
				size_t remaining_size = sizeof(commandString) - strlen(commandString) - 1;
				if(remaining_size>0)
					strncat(commandString, single, remaining_size);
			}
			phaseHandler =commandInfo[command[CMD_NUMBER] &0x1F].handler;
			data =NULL;
			bytesLeft =0;
			if ((command[CMD_NUMBER] &0x1F) !=0x08) {
				ST0 =command[CMD_US_HD] &7;
				ST1 =ST2 =0x00;
			}
		} else
			MSR |=MSR_READY;
	}
}

void	FDC::phResult (void) {
	if (~MSR &MSR_READY) {
		// If all result bytes have been read, start the Command Phase
		// otherwise provide more bytes
		if (resultQueue.empty()) {
			MSR &=~MSR_DIRECTION;
			MSR &=~MSR_BUSY;
			command.clear();
			phaseHandler =&FDC::phCommand;
			dmaRequest =false;
		} else {
			MSR |= MSR_DIRECTION;
			latch =resultQueue.front();
			resultQueue.pop();
		}
		MSR |=MSR_READY;
	}
}

void	FDC::endCommand (bool raiseIRQ, int num, ...) {
	va_list list;
	va_start(list, num);

	char resultString[1024] ="Result: ";
	for (int i =0; i <num; i++) {
		int value =va_arg(list, int);
		resultQueue.push(value);
		char single[32];
		_snprintf(single, sizeof(single)-1, "%02X ", value);
		single[sizeof(single)-1] = '\0';
		size_t remaining_size = sizeof(resultString) - strlen(resultString) - 1;
		if(remaining_size>0)
			strncat(resultString, single, remaining_size);

	}
	va_end(list);
	phaseHandler =&FDC::phResult;
	if (raiseIRQ) SRA |=SRA_IRQ;
	MSR &=~MSR_NO_DMA;
}

void	FDC::nextSector (void) {
	bool firstSector =(data ==NULL);
	if (data ==NULL) {
		cylinder =command[CMD_CYLINDER];
		head =command[CMD_HEAD];
		sectorNumber =command[CMD_SECTOR_NUMBER];
		sectorSize =128 <<command[CMD_SECTOR_SIZE];
		if (sectorSize >8192) sectorSize =8192;
	} else
	if (sectorNumber ==command[CMD_LAST_SECTOR]) {
		if (command[CMD_NUMBER] &CMD_MULTI_TRACK && head ==0) {
			head ^=1;
			command[CMD_HEAD] ^=1;
			sectorNumber =1;
		} else
		if (useDMA && dmaTerminate) {
			cylinder++;
			head ^=1;
			sectorNumber =1;
		} else {
			ST0 |=ST0_ABNORMAL_TERMINATION;
			ST1 |=ST1_END_OF_TRACK;
			unsigned char sectorSizeCode = 2;
			if(command.size() > CMD_SECTOR_SIZE) {
				sectorSizeCode = command[CMD_SECTOR_SIZE];
			}
			endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, sectorSizeCode);
			return;
		}
	} else
		sectorNumber++;

	if (useDMA && dmaTerminate /* || SIGNAL_SH */) {
		ST0 |=ST0_NORMAL_TERMINATION;
		unsigned char sectorSizeCode = 2;
		if(command.size() > CMD_SECTOR_SIZE) {
			sectorSizeCode = command[CMD_SECTOR_SIZE];
		}
		endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, sectorSizeCode);
		dmaTerminate =false;
		return;
	}

	if (!drive->inserted || cylinder >=drive->cylinders || head >=drive->sides || !drive->running || dataRate !=drive->correctDataRate) {
//	if (!drive->inserted || cylinder >=drive->cylinders || !drive->running || dataRate !=drive->correctDataRate) {
		ST1 |=ST1_MISSING_ADDRESS_MARK;
		ST0 |=ST0_ABNORMAL_TERMINATION;
	} else
	if (cylinder !=drive->cylinder) {
		ST1 |=ST1_NO_DATA;
		ST2 |=ST2_WRONG_CYLINDER;
		ST0 |=ST0_ABNORMAL_TERMINATION;
	} else
	if (sectorNumber <1 || sectorNumber >drive->sectors || sectorSize !=512) {
		ST1 |=ST1_NO_DATA;
		ST0 |=ST0_ABNORMAL_TERMINATION;
	}

	if (ST0 &ST0_ABNORMAL_TERMINATION) {
		unsigned char sectorSizeCode = 2;
		if(command.size() > CMD_SECTOR_SIZE) {
			sectorSizeCode = command[CMD_SECTOR_SIZE];
		}
		endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, sectorSizeCode);
		return;
	} else {
		int lba =(cylinder *drive->sides + head) *drive->sectors +sectorNumber -1;
		// 调试服务的磁盘记录：一条读写命令只记录起始扇区
		if (firstSector && g_dbgActive) DebugHook_OnDisk(lba, (command[CMD_NUMBER] &0x1F) ==0x05);
		// 调试服务注入的坏扇区：只有读才出数据校验错误
		if ((command[CMD_NUMBER] &0x1F) ==0x06 && std::find(dbgBadLba.begin(), dbgBadLba.end(), lba) !=dbgBadLba.end()) {
			ST0 |=ST0_ABNORMAL_TERMINATION;
			ST1 |=ST1_DATA_ERROR;
			ST2 |=ST2_DATA_ERROR;
			unsigned char sizeCode = 2;
			if(command.size() > CMD_SECTOR_SIZE) sizeCode = command[CMD_SECTOR_SIZE];
			endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, sizeCode);
			return;
		}
		bytesLeft =sectorSize;
		data =drive->data +lba *sectorSize;
	}
}

void	FDC::cmReadID (void) {
	if (dbgNoIndex) return;
	if (!drive->inserted || cylinder >=drive->cylinders || head >=drive->sides || !drive->running || dataRate !=drive->correctDataRate) {
//	if (!drive->inserted || cylinder >=drive->cylinders || !drive->running || dataRate !=drive->correctDataRate) {
		ST1 |=ST1_MISSING_ADDRESS_MARK;
		ST0 |=ST0_ABNORMAL_TERMINATION;
	} else {
		cylinder =drive->cylinder;
		head =command[CMD_US_HD] >>2;;
		sectorSize =512;
		if (sectorNumber ==0 || sectorNumber >=drive->sectors) sectorNumber =1;
		ST0 |=ST0_NORMAL_TERMINATION;
	}
	unsigned char sectorSizeCode = 2;
	if(command.size() > CMD_SECTOR_SIZE) {
		sectorSizeCode = command[CMD_SECTOR_SIZE];
	}
	endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, sectorSizeCode);
}

void	FDC::cmReadData (void) {
	if (dbgNoIndex) return;
	if (~MSR &MSR_READY && !dmaRequest) { // Do nothing if we're waiting for CPU to collect a byte (don't emulate overrun)
		if (bytesLeft ==0) nextSector();
		if (bytesLeft) {
			latch =*data++;
			bytesLeft--;
			if (useDMA)
				dmaRequest =true;
			else {
				MSR |=MSR_READY;
				MSR |=MSR_NO_DMA;
				MSR |=MSR_DIRECTION;
				SRA |=SRA_IRQ;
			}
		}
	}
}

void	FDC::cmWriteData (void) {
	if (dbgNoIndex) return;
	if (drive->writeProtected) {
		ST0 |=ST0_ABNORMAL_TERMINATION;
		ST1 |=ST1_NOT_WRITEABLE;
		endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, command[CMD_FORMAT_SECTOR_SIZE]);
		return;
	}
	if (~MSR &MSR_READY && !dmaRequest) { // Do nothing if we're waiting for CPU to provide a byte (don't emulate overrun)
		if (bytesLeft ==0)
			nextSector();
		else {
			*data++ =latch;
			bytesLeft--;
			if (drive->modifiedFlag) *drive->modifiedFlag =true;
			bDirty = *drive->modifiedFlag;
		}
		if (bytesLeft) {
			if (useDMA)
				dmaRequest =true;
			else {
				MSR |= MSR_READY;
				MSR |= MSR_NO_DMA;
				MSR &=~MSR_DIRECTION;
				SRA |= SRA_IRQ;
			}
		}
	}
}

void	FDC::cmFormatTrack (void) {
	if (dbgNoIndex) return;
	if (drive->writeProtected) {
		ST0 |=ST0_ABNORMAL_TERMINATION;
		ST1 |=ST1_NOT_WRITEABLE;
		endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, command[CMD_FORMAT_SECTOR_SIZE]);
		return;
	}
	if (~MSR &MSR_READY) {	// Do nothing if we're waiting for CPU to provide a byte (don't emulate overrun)
		if (bytesLeft ==0) {
			if (data ==NULL) {
				cylinder =drive->cylinder;
				head =command[CMD_US_HD] >>2;
				sectorNumber =1;
			} else
			if (++sectorNumber >command[CMD_FORMAT_LAST_SECTOR]) {
				ST0 |=ST0_NORMAL_TERMINATION;
				endCommand(true, 7, ST0, ST1, ST2, cylinder, head, sectorNumber, command[CMD_FORMAT_SECTOR_SIZE]);
				return;
			}
			int lba =(cylinder *drive->sides + head) *drive->sectors +sectorNumber -1;
			bytesLeft =sectorSize;
			data =drive->data +lba *sectorSize;
		} else {
			*data++ =command[CMD_FORMAT_FILL];
			bytesLeft--;
			if (drive->modifiedFlag) *drive->modifiedFlag =true;
			bDirty = *drive->modifiedFlag;
		}
		if (bytesLeft > 512-4) { // request the sector ID while we are filling the first four bytes of the sector
			MSR |= MSR_READY;
			MSR |= MSR_NO_DMA;
			MSR &=~MSR_DIRECTION;
			SRA |= SRA_IRQ;
		}
	}
}

void	FDC::cmSeek (void) {
	if (!dbgSeekFail) {
		if (drive->cylinder !=command[CMD_NCN]) drive->changed =false;
		drive->cylinder =command[CMD_NCN];
	}
	ST0 |=ST0_SEEK_COMPLETE;
	if (!drive->exists) ST0 |=ST0_ABNORMAL_TERMINATION;
	endCommand(true, 0);
}

void	FDC::cmRecalibrate (void) {
	if (dbgSeekFail && drive->cylinder !=0) {
		// 退不回 0 磁道：寻道结束并报设备检查错误
		ST0 |=ST0_SEEK_COMPLETE | ST0_EQUIPMENT_CHECK | ST0_ABNORMAL_TERMINATION;
		endCommand(true, 0);
		return;
	}
	if (drive->cylinder !=0) drive->changed =false;
	drive->cylinder =0;
	ST0 |=ST0_SEEK_COMPLETE;
	if (!drive->exists) ST0 |=ST0_ABNORMAL_TERMINATION;
	endCommand(true, 0);
}

void	FDC::cmSenseInterruptStatus (void) {
	SRA &=~SRA_IRQ;
	endCommand(false, 2, ST0, drive->cylinder);
}

void	FDC::pollDrives (void) {
	if (CONFIG &CFG_NOT_POLL) return;
	if (!pollTimer) {
		pollTimer =394; // polling duration 220 μs
		if (previouslyReady[pollDrive] !=drives[pollDrive].ready) {
			previouslyReady[pollDrive] =drives[pollDrive].ready;
			ST0 =ST0 &~ST0_SEEK_COMPLETE | ST0_POLLING | pollDrive;
			SRA |=SRA_IRQ;
		} 
		pollDrive =(pollDrive +1) &3;
	} else
		pollTimer--;
}

void	FDC::cmSpecify (void) {
	useDMA =!(command [2] &1);
	endCommand(false, 0);
}

void	FDC::cmSenseDriveStatus (void) {
	endCommand(false, 1, (DOR &DOR_SELECT_DRIVE) |
	                     (head <<2) |
			     ((drive->cylinder ==0)? 0x10: 0x00) |
			     (drive->writeProtected? 0x40: 0x00) |
			     (drive->inserted? 0x20: 0x00) |
			     0x28 );
}

void	FDC::cmConfigure (void) {
	CONFIG =command[2];
	endCommand(false, 0);
}

void	FDC::cmInvalid (void) {
	ST0 |=ST0_INVALID_COMMAND;
	endCommand(false, 1, ST0);
}

int	FDC::readIO (int Addr) {
	int	result =0xFF;
	switch (Addr &7) {
		case 0:	result =SRA;
			break;
		case 2:	result =DOR;
			break;
		case 3: result =SRA &SRA_IRQ? 0x40: 0;
			break;
		case 4:	result =MSR;
			break;
		case 5:	if (MSR &MSR_READY && MSR &MSR_DIRECTION) {
				MSR &=~MSR_READY;
				SRA &=~SRA_IRQ;
				result =latch;
			}
			break;
		case 7:	result =drive->changed? 0x80: 0x00;
			drive->changed =false;
			break;
		default:
			break;
	}
	return result;
}

int	FDC::readIODebug (int Addr) {
	int	result =0xFF;
	switch (Addr &7) {
		case 0:	result =SRA;
			break;
		case 2:	result =DOR;
			break;
		case 4:	result =MSR;
			break;
		case 5:	if (MSR &MSR_READY && MSR &MSR_DIRECTION) result =latch;
			break;
	}
	return result;
}

void	FDC::writeIO (int Addr, int Val) {
	switch (Addr &7) {
		case 2:	if (DOR &DOR_NOT_RESET && ~Val &DOR_NOT_RESET) reset();
			DOR =Val;
			drive =&drives[DOR &DOR_SELECT_DRIVE];
			drives[0].running =!!(DOR &DOR_MOTOR_A && drives[0].exists);
			drives[1].running =!!(DOR &DOR_MOTOR_B && drives[1].exists);
			drives[2].running =!!(DOR &DOR_MOTOR_C && drives[2].exists);
			drives[3].running =!!(DOR &DOR_MOTOR_D && drives[3].exists);
			break;
		case 3:	SRA &=~SRA_IRQ;
			break;
		case 5:	if (MSR &MSR_READY && ~MSR &MSR_DIRECTION) {
				MSR &=~MSR_READY;
				SRA &=~SRA_IRQ;
				latch =Val;
			}
			break;
		case 7:	dataRate =dataRates[Val &3];
			break;
		default:
			break;
	}
}

void	FDC::insertDisk (unsigned char driveNum, unsigned char *_data, size_t _size, bool _writeProtected, bool *modifiedFlag) {
	Drive *d =&drives[driveNum &3];
	d->data =_data;
	d->writeProtected =_writeProtected;
	d->modifiedFlag =modifiedFlag;
	switch (_size) {
		case 160*1024:
			d->cylinders =40;
			d->sides =1;
			d->sectors =8;
			d->correctDataRate =250;
			break;
		case 180*1024:
			d->cylinders =40;
			d->sides =1;
			d->sectors =9;
			d->correctDataRate =250;
			break;
		case 320*1024:
			d->cylinders =40;
			d->sides =2;
			d->sectors =8;
			d->correctDataRate =250;
			break;
		case 360*1024:
			d->cylinders =40;
			d->sides =2;
			d->sectors =9;
			d->correctDataRate =250;
			break;
		case 720*1024:
			d->cylinders =80;
			d->sides =2;
			d->sectors =9;
			d->correctDataRate =250;
			break;
		case 1200*1024:
			d->cylinders =80;
			d->sides =2;
			d->sectors =15;
			d->correctDataRate =500;
			break;
		case 1440*1024:
			d->cylinders =80;
			d->sides =2;
			d->sectors =18;
			d->correctDataRate =500;
			break;
		case 2880*1024:
			d->cylinders =80;
			d->sides =2;
			d->sectors =36;
			d->correctDataRate =1000;
			break;
		default:	//Non-standard (For the moment use 1440*1024 floppy)
			d->cylinders =80;
			d->sides =2;
			d->sectors =18;
			d->correctDataRate =500;
			break;
	}
	d->inserted =true;
}

void	FDC::ejectDisk (unsigned char driveNum) {
	Drive *d =&drives[driveNum &3];
	d->data =NULL;
	if (d->inserted) d->changed =true;
	d->inserted =false;
}

bool	FDC::irqRaised (void) const {
	return !!(SRA &SRA_IRQ);
}

bool	FDC::haveDMARequest (void) const {
	return dmaRequest;
}

void	FDC::setTerminalCount (void) {
	dmaTerminate =true;
}

void	FDC::sendToDMA (unsigned char val) {
	latch =val;
	dmaRequest =false;
}

unsigned char	FDC::receiveFromDMA (void) {
	dmaRequest =false;
	return latch;
}

void	FDC::setDriveMask (unsigned char mask) {
	drive[0].exists =drive[0].ready =!!(mask &1);
	drive[1].exists =drive[1].ready =!!(mask &2);
	drive[2].exists =drive[2].ready =!!(mask &4);
	drive[3].exists =drive[3].ready =!!(mask &8);
}

void	FDC::setResetPin (bool pin) {
	if (!resetPin && pin) reset();
	resetPin =pin;
}

// ---- 调试服务的故障注入 ----
void	FDC::DbgEject() {
	ejectDisk(0);
}

int	FDC::DbgInsert() {
	if (!pDiskImage) return 0;
	insertDisk(0, pDiskImage, nDiskSize, drives[0].writeProtected, &bDiskModified);
	return 1;
}

void	FDC::DbgSetWriteProtect(bool protect) {
	drives[0].writeProtected =protect;
}

void	FDC::DbgClearFaults() {
	dbgNoIndex =false;
	dbgSeekFail =false;
	dbgDead =false;
	dbgBadLba.clear();
	drives[0].writeProtected =false;
	if (!drives[0].inserted) DbgInsert();
}

std::string	FDC::DbgStatus() {
	char buf[256];
	_snprintf(buf, sizeof(buf)-1, "inserted=%d wp=%d cylinder=%d noindex=%d seekfail=%d dead=%d bad=%d",
		drives[0].inserted ?1 :0, drives[0].writeProtected ?1 :0, drives[0].cylinder, dbgNoIndex ?1 :0, dbgSeekFail ?1 :0, dbgDead ?1 :0, (int)dbgBadLba.size());
	buf[sizeof(buf)-1] = '\0';
	return buf;
}
//==============================================================================

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
