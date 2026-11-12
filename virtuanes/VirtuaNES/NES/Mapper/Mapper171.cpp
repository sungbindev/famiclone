/*******************************************************************************
 * NES Mapper for BBK
 *
 *  Author:  <fanoble@yeah.net>
 *
 *  Create:   2014-06-24, by fanoble
 *
 *	fix for bbk SC-98: 2025-07, code form NintendulatorNRS by NewRisingSun.
 *******************************************************************************
 */

void Mapper171::Reset()
{
	BBK_928 = FALSE;
	if (nes->rom->GetPROM_CRC() == 0xc48e161a) {
		BBK_928 = TRUE;
		nes->SetVideoMode(2);
		nes->pad->SetExController(PAD::EXCONTROLLER_BBK_FD1);
		SetPROM_32K_Bank(0, 0, 0, PROM_8K_SIZE - 1);
		return;
	}

	//	1.0( 128K) - crc(0xB3D11A3C)
	//	2.0(2048K) - crc(0x8C8FE800)
	bios_type = (nes->rom->GetPROM_SIZE() < 0x40) ? 0x00 : 0x01;
	PROM_mask = (nes->rom->GetPROM_SIZE() << 1) - 1;
	PRAM_mask = (nes->rom->GetPROM_SIZE() < 0x40) ? 0x3F : 0x3F;
	VRAM_mask = (nes->rom->GetPROM_SIZE() < 0x40) ? 0x1F : 0xFF;
	for (int i = 0; i < 0x100; i++) reg[i] = 0;
	bDiskAccess = 0;
	nes->fdc->isBBK = true;
	nes->fdc->reset();
	nes->fdc->setDriveMask(1);
	nRegSPInt = 0;
	nResetLPC = 0;
	nSPReadPos = 0;
	nSpWritePos = 0;
	bMapRam = 0;
	prev09 = 0;
	bSplitMode = 0;
	bEnableIRQ = 0;
	test_once = 1;
	CPU_MEM_BANK[2][0x1FF9] = 0;	// $5FF9
	YPRAM_mask = 0x10;
	YCRAM_mask = 0x7F;
	MMC3_mode = 0;
	SetBank();
	nes->SetVideoMode(2);
	nes->pad->SetExController(PAD::EXCONTROLLER_BBK_FD1);
}

BYTE Mapper171::ExCmdRead(EXCMDRD cmd)
{
	if (cmd == Mapper::EXCMDRD_DISKACCESS) return bDiskAccess;
	return 0;
}

BYTE Mapper171::ReadLow(WORD addr)
{
	if (BBK_928 || MMC3_mode)
		if (addr >= 0x4020)
			return CPU_MEM_BANK[addr >> 13][addr & 0x1FFF];

	if (addr < 0x4400)	return (addr >> 8); // empty
	else				return CPU_MEM_BANK[addr >> 13][addr & 0x1FFF];
}

BOOL Mapper171::ReadHigh(WORD addr, LPBYTE pdata)
{
	if (BBK_928 || MMC3_mode) return FALSE;

	//	if (bios_type) if(addr<0xFF00||addr>0xFFEF) return FALSE;

	int cs_type;
	DWORD phy_addr;
	phy_addr = MapAddr(addr, &cs_type, 0);
	if (INNO_CS_ROM == cs_type) {
		*pdata = nes->rom->GetPROM()[phy_addr];
		return TRUE;
	}
	else if (INNO_CS_DRAM == cs_type) {
		*pdata = YPRAM[phy_addr];
		return TRUE;
	}

	bDiskAccess = 0;
	switch (addr) {
	case 0xFF18:
		if (((nSpWritePos + 1) & 0x0F) == nSPReadPos)
			*pdata = 0x00;
		else
			*pdata = 0x8F;
		return TRUE;
	case 0xFF40:
		*pdata = 0;
		return TRUE;
	case 0xFF48: {
		unsigned status = 0;
		if (!nes->lpt->Error())   status |= 0x08;
		if (nes->lpt->Select())   status |= 0x10;
		if (nes->lpt->PaperOut()) status |= 0x20;
		if (!nes->lpt->Ack())     status |= 0x40;
		if (nes->lpt->BusyN())    status |= 0x80;
		*pdata = status;
	}
			   return TRUE;
	case 0xFF50:	// PC Card
		*pdata = 0;
		return TRUE;
	case 0xFFB0:
		nes->cpu->ClrIRQ(IRQ_MAPPER);
		*pdata = 0x06;
		return TRUE;
	case 0xFF80:
		bDiskAccess = 1;
		*pdata = nes->fdc->receiveFromDMA();
		return TRUE;
	case 0xFF88:
		bDiskAccess = 1;
		nes->fdc->setTerminalCount();
		*pdata = nes->fdc->receiveFromDMA();
		return TRUE;
	case 0xFF90:
		bDiskAccess = 1;
		*pdata = nes->fdc->haveDMARequest() ? 0x40 : 0x00;
		return TRUE;
	case 0xFF98: case 0xFFA0: case 0xFFA8: case 0xFFB8:
		bDiskAccess = 1;
		*pdata = nes->fdc->readIO((addr >> 3) & 7);
		return TRUE;
	default:
		return FALSE;
		//			DEBUGOUT("Read %02X\n", addr);
		break;
	}
}

void Mapper171::WriteLow(WORD addr, BYTE data)
{
	if (BBK_928 || MMC3_mode) {
		if (addr >= 0x4020)
			CPU_MEM_BANK[addr >> 13][addr & 0x1FFF] = data;
		return;
	}

	if (addr < 0x4000) {	// 2000-3FFF
		if ((addr & 7) == 0 || (addr & 7) == 1 || (addr & 7) == 5)
			if (bMapRam || reg[0x11] & 0x80)
				CPU_MEM_BANK[3][addr & 0x1FFF] = data; // Map to 6000-7FFF
	}
	else if (addr >= 0x4400) CPU_MEM_BANK[addr >> 13][addr & 0x1FFF] = data;

	if (test_once)	// temporary handling for BBK_bios2.0(SC-98)
		switch (CPU_MEM_BANK[2][0x1FF9]) {	// $5FF9
		case 0x80: PRAM_mask = 0xFF; test_once = FALSE; break;	// 2048K
		case 0xC0: PRAM_mask = 0x7F; test_once = FALSE; break;	// 1024K
		case 0xE0: PRAM_mask = 0x3F; test_once = FALSE; break;	//  512K
			//			case 0xF0: PRAM_mask=0x1F;test_once=FALSE;break;	//  256K
		}

}

void Mapper171::Write(WORD addr, BYTE data)
{
	if (BBK_928) {
		switch (addr) {
		case 0xffe0: SetPROM_8K_Bank(4, data); break;
		case 0xffe1: SetPROM_8K_Bank(5, data); break;
		case 0xffe2: SetPROM_8K_Bank(6, data); break;
		case 0xffe3: SetPROM_8K_Bank(7, data); break;
		case 0xffe4: break;//unknown
		}
		return;
	}

	//	if(addr>=0xFF00) DEBUGOUT("Write %02X -> [%04X]\n", data, addr);

	if (MMC3_mode) {
		MMC3_Write(addr, data);
		if (reg[0x11] & 0x80) return;
	}

	int cs_type;
	DWORD phy_addr;
	if (addr < 0xFF00) {
		//		if (bios_type) {
		//			if (!MMC3_mode) CPU_MEM_BANK[addr>>13][addr&0x1FFF]=data;
		//			return;
		//		}
		phy_addr = MapAddr(addr, &cs_type, 1);
		if (INNO_CS_DRAM == cs_type) {
			if (phy_addr == 0x7FFFE && data == 0x9B) data = 0x96; // fix IRQ handler for mine.cmd
			YPRAM[phy_addr] = data;
			return;
		}
	}
	else
		if (addr < 0xFF90) {
			reg[addr & 0xFF] = data;
			switch (addr & 0xFF) {
			case 0x00: // Keyboard LED
				break;
			case 0x01: // Video control port
				bEnableIRQ = !!(data & 0x04);
				bMapRam = !!(data & 0x08);
				MMC3_mode = !!(data & 0x20);
				bSplitMode = !!(data & 0x40);
				nes->cpu->ClrIRQ(IRQ_MAPPER);
				if (MMC3_mode) {
					YPRAM_mask = ~(reg[0x3C] & 0xF0) & 0x3F;		// 0x0F:128K / 0x1F:256K / 0x3F:512K
					YCRAM_mask = reg[0x3C] & 0x04 ? 0xFF : 0x7F;	// 256K / 128K
					MMC3_Reset();
				}
				break;
			case 0x02: // Scanline counter lower nibble
				scanlineCount = scanlineCount & 0xF0 | data & 0x0F;
				break;
			case 0x03: case 0x0B: case 0x13: case 0x1B: // 2 KiB CHR-RAM bank
				reg[0x23 + (addr << 1 & 0x30)] = data << 1 | 0;
				reg[0x2B + (addr << 1 & 0x30)] = data << 1 | 1;
				break;
			case 0x04: // 16 KiB PRG bank 8000-BFFF
				reg[0x14] = data << 1 | 0;
				reg[0x1C] = data << 1 | 1;
				break;
			case 0x05:	// Value 7 set on game start
				break;
			case 0x06: // Scanline counter upper nible
				scanlineCount = scanlineCount & 0x0F | data << 4;
				break;
			case 0x09:
				// D6 is highest RAM address bit
				// D5 is set by Floppy 1 BIOS.
				// D2 set on game start
				// D1 cleared on game start
				break;
			case 0x0A: // Add scanline to CHR bank switching queue
				tempSplitScanlines.push_back(data);
				break;
			case 0x0C: // 16 KiB PRG bank 8000-BFFF MSB?, mirrored at 5FE5
				break;
			case 0x10: // Reset LPC
				if (0 == nRegSPInt && (data & 1)) {
					nResetLPC = TRUE;
					::WaitForSingleObject(ackLPC, INFINITE);
				}
				nRegSPInt = data & 1;
				break;
			case 0x11: // decreased at start of BIOS call, increased and written here at end.
				break;
			case 0x12: // Reset the CHR bank switching queue
				splitScanlines.clear();
				splitBanks.clear();
				splitIndex = 0;
				break;
			case 0x14: // 8 KiB PRG bank 8000-9FFF
				reg[0x04] |= 0x80; // Enable RAM in 8000-BFFF. Needed by Crossword.
				break;
			case 0x1C: // 8 KiB PRG bank A000-BFFF
				reg[0x04] |= 0x80; // Enable RAM in 8000-BFFF. Needed by Crossword.
				break;
			case 0x18: // LPC data
				while (((nSpWritePos + 1) & 0x0F) == nSPReadPos)
					Sleep(10);
				nSPData[nSpWritePos] = data;
				nSpWritePos = (nSpWritePos + 1) & 0x0F;
				break;
			case 0x19: // Written-to at the start of a BIOS function with number
				reg[0x11] = 0;
				break;
			case 0x1A: // Add bank number to CHR bank switching queue
				tempSplitBanks.push_back(data);
				break;
			case 0x22: // Start screen plan
				splitScanlines = tempSplitScanlines;
				splitBanks = tempSplitBanks;
				tempSplitScanlines.clear();
				tempSplitBanks.clear();
				break;
			case 0x2A: // Related to the CHR bank switching queue. Only use by SC-98.
				splitIndex = 0;
				break;
			case 0x23: case 0x2B: case 0x33: case 0x3B:	// 1 KiB CHR-RAM bank
			case 0x43: case 0x4B: case 0x53: case 0x5B:	// 1 KiB CHR-RAM bank
				break;
			case 0x34: // PRAM size ???
				break;
			case 0x3C: // Game data size ???
				break;
			case 0x24: case 0x2C: // Bank CD/EF
				break;
			case 0x40:	// PCDaCtPortO/PCCDataPort
				nes->lpt->Write(data);
				break;
			case 0x48:	// PCCtrlPortI/PCCStatsPortI/PCDataPortIH
				data = data;
				break;
			case 0x50:	// PCDataPortIL/PCCCtrlPoutO
				nes->lpt->StrobeN(!(data & 1));
				nes->lpt->LineFeedN(!(data & 2));
				nes->lpt->Reset(!(data & 4));
				nes->lpt->SelectPrinterN(!(data & 8));
				break;
			case 0x80:
				bDiskAccess = 1;
				nes->fdc->sendToDMA(data);
				break;
			case 0x88:
				bDiskAccess = 1;
				nes->fdc->setTerminalCount();
				nes->fdc->sendToDMA(data);
				break;
			default:
				//				DEBUGOUT("Write %02X -> [%04X]\n", data, addr);
				break;
			}
			SetBank();
		}
		else
			if (addr == 0xFF98) { // IRQ ???
				reg[0x98] = data;
				reg[0x09] = prev09;
				SetBank();
				nes->cpu->ClrIRQ(IRQ_MAPPER);
			}
			else
				if (addr == 0xFFB0) { // IRQ Mask
					reg[0xB0] = data;
					nes->cpu->ClrIRQ(IRQ_MAPPER);
				}
				else
					if (addr == 0xFFA0) {
						reg[0xA0] = data;
						bDiskAccess = 1;
						nes->fdc->setResetPin(!!(data & 0x40));
					}
					else
						if (addr <= 0xFFB8) {
							reg[addr & 0xFF] = data;
							bDiskAccess = 1;
							nes->fdc->writeIO((addr >> 3) & 7, data);
						}
						else;

	bDiskAccess = 0;
}

void Mapper171::Clock(INT nCycles)
{
	nes->fdc->run();

	if (DirectInput.m_Sw[DIK_PAUSE]) {
		nes->Dump_YPRAM();
		nes->Dump_YCRAM();
	}
}

void Mapper171::HSync(INT nScanline)
{
	if (BBK_928) return;

	if (MMC3_mode) {
		if ((nScanline >= 0 && nScanline <= 239) && nes->ppu->IsDispON()) {
			MMC3_IRQSync();
		}
		return;
	}

	if (bSplitMode) {
		if (nScanline >= 240) {
			splitIndex = 0;
			scanlineCount = 0;
		}
		else
			if (nScanline < 240)
				scanlineCount++;
		if (!scanlineCount && splitIndex < splitBanks.size() && splitIndex < splitScanlines.size()) {
			SetYCRAM_2K_Bank(0, splitBanks[splitIndex] & 0x0F);
			SetYCRAM_2K_Bank(2, (splitBanks[splitIndex] >> 4) & 0x0F);
			scanlineCount = splitScanlines[splitIndex];
			splitIndex++;
		}
		if (nScanline == 239 && bEnableIRQ) {
			nes->cpu->SetIRQ(IRQ_MAPPER);
			prev09 = reg[0x09];
			reg[0x09] |= 0x02;
			SetBank();
		}
	}
	else
		if (!++scanlineCount && bEnableIRQ && (nScanline >= 0 && nScanline <= 239 && nes->ppu->IsDispON())) {
			nes->cpu->SetIRQ(IRQ_MAPPER);
			prev09 = reg[0x09];
			reg[0x09] |= 0x02;
			SetBank();
		}
}

void Mapper171::SetBank()
{
	//	SetYPRAM_16K_Bank( 2, 0x1E );
	SetYPRAM_16K_Bank(2, (PROM_mask < 0x40) ? 0x1E : 0x3E);
	if (reg[0x04] & 0x80) {
		SetYPRAM_8K_Bank(4, reg[0x14] & PRAM_mask);
		SetYPRAM_8K_Bank(5, reg[0x1C] & PRAM_mask);
	}
	else {
		SetPROM_8K_Bank(4, reg[0x14] & PROM_mask);
		SetPROM_8K_Bank(5, reg[0x1C] & PROM_mask);
	}
	if (reg[0x01] & 0x08 || reg[0x11] & 0x80) {
		SetYPRAM_8K_Bank(6, reg[0x24] & PRAM_mask);
		SetYPRAM_8K_Bank(7, reg[0x2C] & PRAM_mask);
	}
	else {
		SetPROM_16K_Bank(6, (PROM_mask - 1) >> 1);
	}
	if (!bSplitMode)
		for (int bank = 0; bank <= 7; bank++)
			SetYCRAM_1K_Bank(bank, reg[bank * 8 + 0x23] & VRAM_mask);
	switch (reg[0x01] & 3) {
	case 0: SetVRAM_Mirror(VRAM_VMIRROR);  break;
	case 1: SetVRAM_Mirror(VRAM_HMIRROR);  break;
	case 2: SetVRAM_Mirror(VRAM_MIRROR4L); break;
	case 3: SetVRAM_Mirror(VRAM_MIRROR4H); break;
	}
}

DWORD Mapper171::MapAddr(WORD addr, int* cs_type, BOOL is_write)
{
	DWORD phy_addr;
	int type;
	phy_addr = 0;
	type = INNO_CS_NONE;
	switch (addr >> 13) {
		//		case 0: // 01
	case 1: // 23
		if (((addr & 7) == 0 || (addr & 7) == 1 || (addr & 7) == 5)
			&& (bMapRam || reg[0x11] & 0x80) && is_write) {
			type = INNO_CS_DRAM;
			phy_addr = ((PROM_mask < 0x40) ? 0x7A000 : 0xFA000) + (addr & 0x1FFF);
		}
		break;
	case 2: // 45
	case 3: // 67
		type = INNO_CS_DRAM;
		phy_addr = ((PROM_mask < 0x40) ? 0x78000 : 0xF8000) + (addr & 0x3FFF);
		break;
	case 4: // 89
		if (reg[0x04] & 0x80) {
			type = INNO_CS_DRAM;
			phy_addr = (reg[0x14] & PRAM_mask) * 0x2000 + (addr & 0x1FFF);
		}
		else if (!is_write) {
			type = INNO_CS_ROM;
			phy_addr = (reg[0x14] & PROM_mask) * 0x2000 + (addr & 0x1FFF);
		}
		break;
	case 5: // AB
		if (reg[0x04] & 0x80) {
			type = INNO_CS_DRAM;
			phy_addr = (reg[0x1C] & PRAM_mask) * 0x2000 + (addr & 0x1FFF);
		}
		else if (!is_write) {
			type = INNO_CS_ROM;
			phy_addr = (reg[0x1C] & PROM_mask) * 0x2000 + (addr & 0x1FFF);
		}
		break;
	case 6: // CD
		if (reg[0x01] & 0x08 || reg[0x11] & 0x80) {
			type = INNO_CS_DRAM;
			phy_addr = (reg[0x24] & PRAM_mask) * 0x2000 + (addr & 0x1FFF);
		}
		else if (!is_write) {
			type = INNO_CS_ROM;
			phy_addr = (PROM_mask - 1) * 0x2000 + (addr & 0x1FFF);
		}
		break;
	case 7: // EF
		if (addr < 0xFF00) {
			if (reg[0x01] & 0x08 || reg[0x11] & 0x80) {
				type = INNO_CS_DRAM;
				phy_addr = (reg[0x2C] & PRAM_mask) * 0x2000 + (addr & 0x1FFF);
			}
			else if (!is_write) {
				type = INNO_CS_ROM;
				phy_addr = (PROM_mask - 0) * 0x2000 + (addr & 0x1FFF);
			}
		}
		else if (is_write) {	// FF00-FFFF
			type = INNO_CS_HOLTEK;
			phy_addr = addr & 0xFF;	// IO
		}
		else if (reg[0x01] & 0x08 || reg[0x11] & 0x80) {// Read
			type = INNO_CS_DRAM;
			phy_addr = (reg[0x2C] & PRAM_mask) * 0x2000 + (addr & 0x1FFF);
		}
		else if (0 == (addr & 7)) {	// Map to ROM
			type = INNO_CS_HOLTEK;
			phy_addr = addr & 0xFF;	// Read IO
		}
		else {
			type = INNO_CS_ROM;
			phy_addr = (PROM_mask - 0) * 0x2000 + (addr & 0x1FFF);
		}
		break;
	default:
		type = INNO_CS_NONE;
		break;
	}
	if (cs_type) *cs_type = type;
	return phy_addr;
}

#include "LPC_D6_SYNTH.H"
int Mapper171::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	Mapper171* thiz = (Mapper171*)host;
	ret = LPC_CMD_NONE;
	while (thiz->nSPReadPos == thiz->nSpWritePos) {
		if (!thiz->bRunLPC) {
			ret = LPC_CMD_STOP;
			break;
		}
		else if (thiz->nResetLPC) {
			ret = LPC_CMD_RESET;
			thiz->nResetLPC = FALSE;
			break;
		}
		Sleep(10);
	}
	if (LPC_CMD_NONE == ret) {
		*food = thiz->nSPData[thiz->nSPReadPos];
		thiz->nSPReadPos = (thiz->nSPReadPos + 1) & 0x0F;
		ret = LPC_CMD_PAYLOAD;
	}
	else ::SetEvent(thiz->ackLPC);
	return ret;
}
DWORD WINAPI Mapper171::LPCThreadProc(LPVOID lpParam)
{
	void* synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024];
	Mapper171* thiz = (Mapper171*)lpParam;
	wavLPC.Open(10000, 1);
	synLPC = lpc_d6_synth_new(FeedLPC, thiz, LPC_STD_VARIANT_BBK);
	while (thiz->bRunLPC) {
		int eos;
		int pcm_size;
		short* pcm;
		eos = lpc_d6_synth_do(synLPC, pcmBuf, &pcm_size, 0);
		pcm = pcmBuf;
		pcm_size *= sizeof(short);
		while (pcm_size > 1) {
			int consumed;
			consumed = wavLPC.WriteNotWait(pcm, pcm_size);
			pcm += consumed / 2;
			pcm_size -= consumed;
			if (!thiz->bRunLPC) {
				::SetEvent(thiz->ackLPC);
				break;
			}
			if (thiz->nResetLPC) {
				thiz->nResetLPC = FALSE;
				lpc_d6_synth_reset(synLPC);
				::SetEvent(thiz->ackLPC);
			}
			if (0 == consumed) Sleep(10);
		}
		if (eos) wavLPC.Drain();
	}
	lpc_d6_synth_delete(synLPC);
	wavLPC.Drain();
	wavLPC.Close();
	return 0;
}
void Mapper171::Init()
{
	DWORD dwThreadID;
	nSPReadPos = nSpWritePos = 0;
	bRunLPC = TRUE;
	reqLPC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ackLPC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	::ResetEvent(reqLPC);
	::ResetEvent(ackLPC);
	thdLPC = ::CreateThread(NULL, 0, LPCThreadProc, this, 0, &dwThreadID);
}
Mapper171::~Mapper171()
{
	bRunLPC = FALSE;
	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);
	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);
}

// Generic MM3 -----------------------------------------------------------------
void	Mapper171::MMC3_Reset()
{
	MMC3_cmd = 0;
	MMC3_prg0 = MMC3_fixPRAM(0);
	MMC3_prg1 = MMC3_fixPRAM(1);
	MMC3_chr01 = 0;
	MMC3_chr23 = 2;
	MMC3_chr4 = 4;
	MMC3_chr5 = 5;
	MMC3_chr6 = 6;
	MMC3_chr7 = 7;
	MMC3_irq_enable = 0;
	MMC3_irq_counter = 0;
	MMC3_irq_latch = 0xFF;
	MMC3_irq_request = 0;
	MMC3_irq_preset = 0;
	MMC3_irq_preset_vbl = 0;
	MMC3_SetBank_CPU();
	MMC3_SetBank_PPU();
}
void	Mapper171::MMC3_Write(WORD addr, BYTE data)
{
	switch (addr & 0xE001) {
	case	0x8000:
		MMC3_cmd = data;
		MMC3_SetBank_CPU();
		MMC3_SetBank_PPU();
		break;
	case	0x8001:
		switch (MMC3_cmd & 0x07) {
		case	0x00:
			MMC3_chr01 = data & 0xFE;
			MMC3_SetBank_PPU();
			break;
		case	0x01:
			MMC3_chr23 = data & 0xFE;
			MMC3_SetBank_PPU();
			break;
		case	0x02:
			MMC3_chr4 = data;
			MMC3_SetBank_PPU();
			break;
		case	0x03:
			MMC3_chr5 = data;
			MMC3_SetBank_PPU();
			break;
		case	0x04:
			MMC3_chr6 = data;
			MMC3_SetBank_PPU();
			break;
		case	0x05:
			MMC3_chr7 = data;
			MMC3_SetBank_PPU();
			break;
		case	0x06:
			MMC3_prg0 = MMC3_fixPRAM(data);
			MMC3_SetBank_CPU();
			break;
		case	0x07:
			MMC3_prg1 = MMC3_fixPRAM(data);
			MMC3_SetBank_CPU();
			break;
		}
		break;
	case	0xA000:
		SetVRAM_Mirror((data & 1) ? VRAM_HMIRROR : VRAM_VMIRROR);
		break;
	case	0xC000:
		MMC3_irq_latch = data;
		break;
	case	0xC001:
		if (nes->GetScanline() < 240) {
			MMC3_irq_counter |= 0x80;
			MMC3_irq_preset = 0xFF;
		}
		else {
			MMC3_irq_counter |= 0x80;
			MMC3_irq_preset_vbl = 0xFF;
			MMC3_irq_preset = 0;
		}
		break;
	case	0xE000:
		MMC3_irq_enable = 0;
		MMC3_irq_request = 0;
		nes->cpu->ClrIRQ(IRQ_MAPPER);
		break;
	case	0xE001:
		MMC3_irq_enable = 1;
		MMC3_irq_request = 0;
		break;
	}
}
void	Mapper171::MMC3_SetBank_CPU()
{
	BYTE pwrap = MMC3_cmd & 0x40;
	SetYPRAM_8K_Bank(pwrap ? 6 : 4, MMC3_prg0);
	SetYPRAM_8K_Bank(5, MMC3_prg1);
	SetYPRAM_8K_Bank(pwrap ? 4 : 6, PRAM_mask - 1);
	SetYPRAM_8K_Bank(7, PRAM_mask - 0);
}
void	Mapper171::MMC3_SetBank_PPU()
{
	BYTE cwrap = MMC3_cmd & 0x80;
	SetYCRAM_1K_Bank(cwrap ? 4 : 0, (MMC3_chr01 + 0) & YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 5 : 1, (MMC3_chr01 + 1) & YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 6 : 2, (MMC3_chr23 + 0) & YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 7 : 3, (MMC3_chr23 + 1) & YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 0 : 4, (MMC3_chr4)&YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 1 : 5, (MMC3_chr5)&YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 2 : 6, (MMC3_chr6)&YCRAM_mask);
	SetYCRAM_1K_Bank(cwrap ? 3 : 7, (MMC3_chr7)&YCRAM_mask);

}
BYTE	Mapper171::MMC3_fixPRAM(BYTE data)
{
	BYTE outdata = (data & YPRAM_mask) + (PRAM_mask - YPRAM_mask);
	if ((data & YPRAM_mask) == (YPRAM_mask - 3)) outdata = 0;
	if ((data & YPRAM_mask) == (YPRAM_mask - 2)) outdata = 1;
	return outdata;
}

void	Mapper171::MMC3_IRQSync()
{
	if (MMC3_irq_preset_vbl) {
		MMC3_irq_counter = MMC3_irq_latch;
		MMC3_irq_preset_vbl = 0;
	}
	if (MMC3_irq_preset) {
		MMC3_irq_counter = MMC3_irq_latch;
		MMC3_irq_preset = 0;
	}
	else if (MMC3_irq_counter > 0) MMC3_irq_counter--;
	if (MMC3_irq_counter == 0) {
		if (MMC3_irq_enable) {
			MMC3_irq_request = 0xFF;
			nes->cpu->SetIRQ(IRQ_MAPPER);
		}
		MMC3_irq_preset = 0xFF;
	}
}
//------------------------------------------------------------------------------

/*******************************************************************************
						   E N D  O F  F I L E
*******************************************************************************/
