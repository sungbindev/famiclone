//////////////////////////////////////////////////////////////////////////
// Mapper176         ShuQiYu / HengGe / WaiXing                         //
//////////////////////////////////////////////////////////////////////////

/*	Mappers:
	176 - Standard
	523 - Jncota KT-xxx (1 KiB->2 KiB, 2 KiB->4 KiB CHR, hard-wired nametable mirroring)	

	Submappers:	
	0 - Standard
	1 - FK-xxx
	2 - 外星 FS005/FS006	// Waixing FS005/FS006 has extended WRAM behavior 
	3 - JX9003B
	4 - GameStar Smart Genius Deluxe. Has LPC10 chip at 4800.
	5 - HST-162 (500-in-1)
	6 - FZZC3Y
	
	Verified on real hardware:
	"Legend of Kage" sets CNROM latch 1 and switches between CHR bank 0 and 1 using 5FF2,
	 causing the wrong bank (1 instead of 0) during gameplay."
	
	Heuristics for NES 1.0:
	- 1 MiB PRG+1 MiB CHR     => Submapper 1
	- 256 KiB PRG+128 KiB CHR => Submapper 1
	- 128 KiB PRG+64 KiB CHR  => Submapper 1
	- A001.5 ever set         => Submapper 2
	- 5FF5/5FF6 written-to    => Submapper 3
*/
// Mapper176 new code form NintendulatorNRS by NewRisingSun.

/*
	crc == 0x416C07A1		//[ES-1006] Meng Huan Zhi Xing IV (C) & WXN-梦幻之星	√
	crc == 0x94782FBD		//[ES-1057] San Guo Zhi - Xiong Ba Tian Xia (C)
	crc == 0xF9863ADF		//[ES-1066] Xi Chu Ba Wang (C)
	crc == 0xB511C04B		//[ES-1071] San Xia Wu Yi (C)
	crc == 0x1923A8C5		//[ES-1087] Shui Hu Shen Shou (C) & WXN-水浒神兽(fix)	√
	crc == 0x095D8678		//[ES-0122] Shuang Yue Zhuan (C)
	crc == 0xD5F7AAEF		//[ES-XXXX] Shen Feng Jian (C)
	crc == 0x8f6ab5ac		//WXN-三国忠烈传		√
	crc == 0x99051cb5		//WXN-雄霸天下			√
	crc == 0xf29c8186		//WXN-大富翁2-上海大亨	√
	crc == 0xc768098b		//WXN-三侠五义			√
	crc == 0x49f22159		//WXN-超级大富翁		√
	crc == 0xf354d847		//WXN-格兰帝亚			√
	crc == 0x5ee2ef97		//WXN-帝国时代(fix)		√
	crc == 0x977d22c3		//WXN-破釜沉舟(fix)		√
	crc == 0xf1d803f3		//WXN-西楚霸王(fix)		√
	crc == 0x85dd49b6		//WXN-口袋金(fix)		√
	crc == 0x97b82f53		//WXN-爆笑三国(fix)		√
	crc == 0xce2ea530		//WXN-宠物翡翠(fix)		√
*/

#define	prgInvert		!!(pointer&0x40)
#define	chrInvert		!!(pointer&0x80)
#define	prgMode			(fk23reg[0]&7)
#define	chrMixed		nes->rom->GetSubMapperNo()==2 && !!(wram&0x04)
#define	chrNROM			(!!(fk23reg[0]&0x40) || nes->rom->GetVROM_SIZE()==0)
#define	chrCNROM		(~fk23reg[0]&0x20 && (nes->rom->GetSubMapperNo()==1 || nes->rom->GetSubMapperNo()==3) || nes->rom->GetSubMapperNo()==5)
#define	chrSmall		!!(fk23reg[0]&0x10)
#define	chrRAM			(!!(fk23reg[0]&0x20) && !(fk23reg[0]&0x40) || nes->rom->GetVROM_SIZE()==0)
#define	wramEnabled		!!(wram&0x80)
#define	enableRegs		(nes->rom->GetSubMapperNo()!=2 || !!(wram&0x40))
#define	mmc3Extended	(!!(fk23reg[3]&0x02) && (nes->rom->GetSubMapperNo()==1 || nes->rom->GetSubMapperNo()==2 || nes->rom->GetMapperNo()==523))

int		Mapper176::getPRGBank (int bank) {
	const static int prgModeAND[8] ={ 0x3F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x7F, 0xFF};
	int prgAND =prgModeAND[prgMode];
	int prgOR  =fk23reg[1] &0x07F;
	switch (nes->rom->GetSubMapperNo()) {
		case 1: // FK-xxx
			if (prgMode ==0 && mmc3Extended) prgAND =0xFF; // 2 MiB MMC3
			break;
		case 2: // Waixing FS005
			prgOR |=fk23reg[0] <<4 &0x080 |
			        fk23reg[0] <<1 &0x100 |
			        fk23reg[2] <<3 &0x600 |
			        fk23reg[2] <<6 &0x800;
			break;
		case 3: // Super Mario 160 Funny Time: Enhanced variant with extra MSB register
			prgOR |=fk23reg[5] <<7 &0x80 | fk23reg[2] <<2 &0x100 | fk23reg[2] <<4 &0x200;
			if (prgMode ==0) prgAND =0xFF; // 2 MiB MMC3
			break;
		case 4: // GameStar Smart Genius Deluxe
			prgOR |=fk23reg[2] &0x080;
			break;
		case 5:	// HST-162
			prgOR =prgOR &0x1F | reg4800 <<5;
			break;
	}
	switch(prgMode) {
		case 3:	// NROM-128
			return prgOR <<1     | bank &1;
		case 4:	// NROM-256
			return prgOR <<1 &~3 | bank;
		case 5: // UNROM
			return prgOR <<1 &~15| latch <<1 &14 | bank &1 | (bank &2? 14: 0);
		case 6: // Invalid, verified to force bank 0
		case 7:	return 0;
		default: // MMC3 with 512/256/128 KiB PRG
			prgOR <<=1;
			if (~bank &1 && prgInvert) bank ^=2;
			return (!mmc3Extended && bank &2? (0xFE | bank &1): mmc3reg[6 +bank]) &prgAND | prgOR &~prgAND;
	}
}

int		Mapper176::getCHRBank (int bank) {
	int chrOR  =fk23reg[2] <<3;
	int chrAND =chrSmall? 0x7F: 0xFF;
	if (nes->rom->GetSubMapperNo()==3) chrOR |=fk23reg[6] <<11; // Super Mario 160 Funny Time: Enhanced variant with extra MSB register
	if (chrNROM) {
		int mask =chrCNROM? (chrSmall? 0x08: 0x18): 0x00;
		return (nes->rom->GetSubMapperNo()==5? chrOR: (chrOR &~mask)) | (latch <<3) &mask | bank;
	} else {
		const static int bank2ext[8] ={  0, 10,  1, 11,  2,  3,  4,  5 };
		const static int bank2reg[8] ={  0,  0,  1,  1,  2,  3,  4,  5 };
		const static int bankAND [8] ={ ~1, ~1, ~1, ~1, ~0, ~0, ~0, ~0 };
		const static int bankOR  [8] ={  0,  1,  0,  1,  0,  0,  0,  0 };
		if (chrInvert) bank ^=4;
		if (mmc3Extended)
			return mmc3reg[bank2ext[bank]] &chrAND                              |chrOR &~chrAND;
		else
			return mmc3reg[bank2reg[bank]] &chrAND &bankAND[bank] |bankOR[bank] |chrOR &~chrAND;
	}
}

void	Mapper176::sync() {
	for (int bank =0; bank <4; bank ++) SetPROM_8K_Bank(0x4+bank, getPRGBank(bank));
	
	if (nes->rom->GetMapperNo()==523) 
		for (int bank =0; bank <8; bank +=2) SetVROM_2K_Bank(bank, getCHRBank(bank));
	else
		for (int bank =0; bank <8; bank++) {
			int val =getCHRBank(bank);
			if (chrRAM || chrMixed && val <0x08)
				SetYCRAM_1K_Bank(bank, val);
			else
				SetVROM_1K_Bank(bank, val);
		}

	switch (mirroring &(nes->rom->GetSubMapperNo()==2? 3: 1)) {
		case 0:	SetVRAM_Mirror( VRAM_VMIRROR  ); break;
		case 1: SetVRAM_Mirror( VRAM_HMIRROR  ); break;
		case 2:	SetVRAM_Mirror( VRAM_MIRROR4L ); break;
		case 3: SetVRAM_Mirror( VRAM_MIRROR4H ); break;
	}
}

void	Mapper176::writeReg (int addr, int val) {
	if (nes->rom->GetSubMapperNo()==4 && addr &0x800) {
		writeLPC(addr, val);
	} else
	if (enableRegs) {
		int index =addr &(nes->rom->GetSubMapperNo()==3? 7: 3);
		fk23reg[index] =val;
		sync();
	}
}

void	Mapper176::writeMMC3 (int addr, int val) {
	if (prgMode <5) switch (addr &3) {
		case 0:
			pointer =val;		
			if (nes->rom->GetSubMapperNo()==2 && prgInvert && (val &0x07) >=6) pointer ^=1; // FS005 swaps registers $46 and $47
			break;
		case 1:
			mmc3reg[pointer &(mmc3Extended? 0xF: 0x7)] =val;
			break;
		case 2: // Confirmed on real hardware: MMC3 address mask is $E003 instead of the correct $E001
		case 3:
			break;
	}
	latch =val;
	sync();
}

void	Mapper176::writeMirroringWRAM (int addr, int val) {
	if (prgMode <5) {
		if (addr &1)
			wram =val;
		else
			mirroring =val;
	}
	latch =val;
	sync();
}

void	Mapper176::writeIRQConfig (int addr, int val) {
	if (prgMode <5) {
		if (addr &1)
			counter =0;
		else
			reloadValue =val;
	}
	latch =val;
	sync();
}

void	Mapper176::writeIRQEnable (int addr, int val) {
	if (prgMode <5) {
		enableIRQ =!!(addr &1);
		if (!enableIRQ) nes->cpu->ClrIRQ(IRQ_MAPPER);
	}
	latch =val;
	sync();
}

void	Mapper176::writeHST162 (int addr, int val) {
	if (addr &0x800) {
		reg4800 =val;
		sync();
	}
}

int		Mapper176::readLPC(int addr) {
	if (addr &0x800)
		return (((nSpWritePos+1)&(SP_BUF_SIZE-1))==nSPReadPos)?0x00:0x40;
	return	Mapper::ReadLow( addr );
}

void	Mapper176::writeLPC (int addr, int val) {
	if (addr &0x800) {
		if((val&0xF0)==0x50){
//			nResetLPC = FALSE;
			count++;
			if(count==1) temp_data=val&0x0F;
			if(count==2){
				count = 0;
				while (((nSpWritePos+1)&(SP_BUF_SIZE-1))==nSPReadPos)
					Sleep(10); // should not be here
				nSPData[nSpWritePos]=((val&0x0F)<<4)|temp_data;
				nSpWritePos=(nSpWritePos+1)&(SP_BUF_SIZE-1);
			}

		} else {
			nResetLPC = TRUE;
			::WaitForSingleObject(ackLPC, INFINITE);
			for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
			count = 0;
		}
	}
}

void	Mapper176::Reset()
{
//Old code------------------------------------------------------------------------------
	if(PROM_16K_SIZE>32){
		SetPROM_32K_Bank( 0, 1, (PROM_8K_SIZE/2)-2, (PROM_8K_SIZE/2)-1 );	//For 1M byte Cartridge
	}else{
		SetPROM_32K_Bank( 0, 1, PROM_8K_SIZE-2, PROM_8K_SIZE-1 );
	}
	if(VROM_1K_SIZE) SetVROM_8K_Bank( 0 );

	reg5000 = 0;
	reg5001 = 0;
	reg5010 = 0;
	reg5011 = 0;
	reg5013 = 0;
	reg5FF1 = 0;
	reg5FF2 = 0;
	we_sram  = 0;
	SBW = 0;
	sp_rom = 0;
	DWORD	crc = nes->rom->GetPROM_CRC();
	if(crc == 0x095D8678	//[ES-0122] Shuang Yue Zhuan (C)
	|| crc == 0xD5F7AAEF){	//[ES-XXXX] Shen Feng Jian (C)
		sp_rom = 1;
		nes->SetSAVERAM_SIZE( 32*1024 );
	}
//Old code------------------------------------------------------------------------------

	if(!nes->rom->IsNES20()) return;

	nResetLPC = FALSE;
	count = 0;
	temp_data = 0;

	static const BYTE initialReg[12] = { 0x00, 0x02, 0x04, 0x05, 0x06, 0x07, 0x00, 0x01, 0xFE, 0xFF, 0x01, 0x03 };
	for (int i =0; i <12; i++) mmc3reg[i] = initialReg[i];
	for (int j =0; j < 8; j++) fk23reg[j] = 0x00;
	if(nes->rom->GetSubMapperNo()==1) fk23reg[1] =0xFF;
	if(nes->rom->GetMapperNo()==523) 
		mirroring =1;
	else
		mirroring =0;
	wram =nes->rom->GetSubMapperNo()==2? 0xC0: 0x80;
	pointer =0;
	counter =0;
	reloadValue =0;
	pa12Filter =0;
	latch =0;
	reg4800 =0;
	enableIRQ =FALSE;
	nes->cpu->ClrIRQ(IRQ_MAPPER);
	sync();

}

BYTE	Mapper176::ReadLow( WORD addr )
{
//Old code------------------------------------------------------------------------------
	if(sp_rom==1 && addr>=0x6000){
		switch( we_sram ) {
			case 0xE4:
			case 0xEC: return WRAM[(addr&0x1FFF)+0x0000];
			case 0xE5:
			case 0xED: return WRAM[(addr&0x1FFF)+0x2000];
			case 0xE6:
			case 0xEE: return WRAM[(addr&0x1FFF)+0x4000];
			case 0xE7:
			case 0xEF: return WRAM[(addr&0x1FFF)+0x6000];
			default:   return CPU_MEM_BANK[addr>>13][addr&0x1FFF];
		}
	}
//Old code------------------------------------------------------------------------------

	if(!nes->rom->IsNES20()) return	Mapper::ReadLow( addr );

	if(addr>=0x4100 && addr<=0x5FFF){
		if(nes->rom->GetSubMapperNo()==4){
			readLPC(addr);
		}
	}

	return	Mapper::ReadLow( addr );
}

void	Mapper176::WriteLow( WORD addr, BYTE data )
{
//	DEBUGOUT("Address=%04X Data=%02X\n", addr&0xFFFF, data&0xFF );

//Old code------------------------------------------------------------------------------
	switch( addr ) {
		case	0x5000:
			reg5000 = data;
			break;
		case	0x5001:			//[ES-1006] Meng Huan Zhi Xing IV (C)
			reg5001 = data;
			if(SBW) SetPROM_32K_Bank(reg5001);
			break;
		case	0x5010:
			reg5010 = data;
			if(reg5010==0x24) SBW=1;
			break;
		case	0x5011:
			reg5011 = data>>1;
			if(SBW)	SetPROM_32K_Bank(reg5011);
			break;
		case	0x5013:
			reg5013 = data;
			break;
		case	0x5ff1:
			reg5FF1 = data>>1;
			SetPROM_32K_Bank(reg5FF1);
			break;
		case	0x5ff2:
			reg5FF2 = data;
			SetVROM_8K_Bank(reg5FF2);
			break;
	}
	if(addr>=0x6000) {
		if(sp_rom==1) {
			switch( we_sram ) {
				case 0xE4:	//CPU_MEM_BANK
				case 0xEC:	//CPU_MEM_BANK
					WRAM[(addr&0x1FFF)+0x0000] = data;
					CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
					break;
				case 0xE5:	//SRAM
				case 0xED:	//SRAM
					WRAM[(addr&0x1FFF)+0x2000] = data;
					break;
				case 0xE6:
				case 0xEE:
					WRAM[(addr&0x1FFF)+0x4000] = data;
					break;
				case 0xE7:
				case 0xEF:
					WRAM[(addr&0x1FFF)+0x6000] = data;
					break;
				default:
					CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
					break;
			}
		}else CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
	}
//Old code------------------------------------------------------------------------------

	if(!nes->rom->IsNES20()) return;

	if(addr>=0x4100 && addr<=0x4FFF){
		if(nes->rom->GetSubMapperNo()==4){
			writeLPC(addr, data);
		}
		if(nes->rom->GetSubMapperNo()==5){
			writeHST162(addr, data);
		}
	}
	if(addr>=0x5000 && addr<=0x5FFF){
		writeReg(addr, data);
	}

}

void	Mapper176::Write( WORD addr, BYTE data )
{
//	DEBUGOUT("Address=%04X Data=%02X\n", addr&0xFFFF, data&0xFF );

//Old code------------------------------------------------------------------------------
	if(addr==0xa000){
		data &= 0x03;
		if		( data == 0 )	SetVRAM_Mirror( VRAM_VMIRROR );
		else if	( data == 1 )	SetVRAM_Mirror( VRAM_HMIRROR );
		else if	( data == 2 )	SetVRAM_Mirror( VRAM_MIRROR4L );
		else					SetVRAM_Mirror( VRAM_MIRROR4H );
	}
	if(addr==0xa001) we_sram = data;
//Old code------------------------------------------------------------------------------

	if(!nes->rom->IsNES20()) return;

	if(addr>=0x8000&&addr<=0x9FFF) writeMMC3(addr, data);
	if(addr>=0xA000&&addr<=0xBFFF) writeMirroringWRAM(addr, data);
	if(addr>=0xC000&&addr<=0xDFFF) writeIRQConfig(addr, data);
	if(addr>=0xE000&&addr<=0xFFFF) writeIRQEnable(addr, data);
}

void	Mapper176::Clock(INT nCycles)
{
	if (pa12Filter) pa12Filter--;
}

void	Mapper176::HSync(INT nScanline)
{
//	if (addr &0x1000) {
		if (!pa12Filter) {
			counter =!counter? reloadValue: --counter;
			if (!counter && enableIRQ) nes->cpu->SetIRQ(IRQ_MAPPER);
		}
		pa12Filter =5;
//	}
}

//------------------------------------------------------------------------------
#include "LPC_D6_SYNTH.H"
int Mapper176::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	Mapper176* thiz = (Mapper176*)host;
	ret = LPC_CMD_NONE;
	while (thiz->nSPReadPos == thiz->nSpWritePos) {
		if (!thiz->bRunLPC) {
			ret = LPC_CMD_STOP;
			break;
		} else if (thiz->nResetLPC) {
			ret = LPC_CMD_RESET;
			thiz->nResetLPC = FALSE;
			break;
		}
		Sleep(10);
	}
	if (LPC_CMD_NONE == ret) {
		*food = thiz->nSPData[thiz->nSPReadPos];
		thiz->nSPReadPos = (thiz->nSPReadPos + 1) & (SP_BUF_SIZE - 1);
		ret = LPC_CMD_PAYLOAD;
	} else {
		::SetEvent(thiz->ackLPC);
	}
	return ret;
}
DWORD WINAPI Mapper176::LPCThreadProc(LPVOID lpParam)
{
	void*    synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024];
	Mapper176* thiz = (Mapper176*)lpParam;
	wavLPC.Open(10000, 1);
	synLPC = lpc_d6_synth_new(FeedLPC, thiz, LPC_STD_VARIANT_Generic);
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
void Mapper176::Init()
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
Mapper176::~Mapper176()
{
	bRunLPC = FALSE;
	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);
	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);
}
//------------------------------------------------------------------------------
