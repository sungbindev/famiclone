//////////////////////////////////////////////////////////////////////////
// Mapper168          Subor (PPUExtLatch)                               //
//////////////////////////////////////////////////////////////////////////

#include "LPC_D6_SYNTH.H"

//------------------------------------------------------------------------------
int Mapper168::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	Mapper168* thiz = (Mapper168*)host;
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
	} else ::SetEvent(thiz->ackLPC);
	return ret;
}
DWORD WINAPI Mapper168::LPCThreadProc(LPVOID lpParam)
{
	void*    synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024]; // should be enough
	Mapper168* thiz = (Mapper168*)lpParam;
	wavLPC.Open(10000, 1); // 10Khz Mono
	synLPC = lpc_d6_synth_new(FeedLPC, thiz, LPC_STD_VARIANT_Subor);
	while (thiz->bRunLPC) {
		int eos;
		int pcm_size;
		int restart = 0;
		short* pcm;
		eos = lpc_d6_synth_do(synLPC, pcmBuf, &pcm_size, &restart);
		if (eos) thiz->m_bSpeechEnd = TRUE;
		if (restart) {
			thiz->nSPReadPos = thiz->nSpWritePos; // clear ring buf
			thiz->m_bSpeechEnd = FALSE;
		}
		pcm = pcmBuf;
		pcm_size *= sizeof(short);
		while (pcm_size > 1) {
			int consumed;
			consumed = wavLPC.WriteNotWait(pcm, pcm_size);
			pcm += consumed / 2;
			pcm_size -= consumed;
			if (!thiz->bRunLPC) {	// command response
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
//------------------------------------------------------------------------------
void Mapper168::Init()
{
	DWORD dwThreadID;
	nSPReadPos = nSpWritePos = 0;
	m_bSpeechEnd = FALSE;
	bRunLPC = TRUE;
	reqLPC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ackLPC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	::ResetEvent(reqLPC);
	::ResetEvent(ackLPC);
	thdLPC = ::CreateThread(NULL, 0, LPCThreadProc, this, 0, &dwThreadID);
}
Mapper168::~Mapper168()
{
	bRunLPC = FALSE;
	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);
	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);
}
//------------------------------------------------------------------------------

void	Mapper168::Reset()
{
	bDiskAccess  = 0;
	Rom_Type = 0;
	DWORD	crc = nes->rom->GetPROM_CRC();
	if( crc == 0x40A4C574 ){	//[Subor] Subor V5.0 (C) (SB97 Rom)
		Rom_Type = 2;
		nes->fdc->isSB97 = true;
		nes->fdc->reset();
	}
	nes->ppu->SetExtLatchMode( TRUE );
	reg5000 = 0;
	reg5200 = 0;
	reg5300 = 0;
	PPU_SW = 0;
	SetBank_CPU();
	nResetLPC = FALSE;
	nSPReadPos = nSpWritePos = 0;
	if( crc == 0x0A9808AE ){	//[Subor] Karaoke (C)
		Rom_Type = 1;
		nes->SetVideoMode( 2 );
		SetPROM_32K_Bank( 0 );
	}
}

BYTE Mapper168::ExCmdRead( EXCMDRD cmd )
{
	if (cmd == Mapper::EXCMDRD_DISKACCESS) return bDiskAccess;
	return 0;
}

BYTE Mapper168::ReadLow( WORD addr )
{
	bDiskAccess = 0;
	if(addr==0x5300) {	// LPC
		if (((nSpWritePos+1)&(SP_BUF_SIZE-1))==nSPReadPos)
			reg5300=0x00;
		else
			reg5300=m_bSpeechEnd?0x8F:0x80;
		return reg5300;
	}else if (addr>=0x5600 && addr<0x5608 && Rom_Type==2) {	// FDC
		bDiskAccess = 1;
		return nes->fdc->readIO(addr & 7);
	}else if(addr>=0x6000) return CPU_MEM_BANK[addr>>13][addr&0x1FFF];
	return Mapper::ReadLow( addr );
}

void Mapper168::WriteLow(WORD addr, BYTE data)
{
	bDiskAccess = 0;
	if(addr==0x5000){
		reg5000 = data;
		SetBank_CPU();
	}else if(addr==0x5200){
		reg5200 = data;
		SetBank_CPU();
	}else if(addr==0x5300){	// LPC
		while (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
			Sleep(10); // should not be here
		nSPData[nSpWritePos] = data;
		nSpWritePos = (nSpWritePos + 1) & (SP_BUF_SIZE - 1);
		if((reg5300==0x8F)&&m_bSpeechEnd) {	//LPC reset???
			nResetLPC = TRUE;
			::WaitForSingleObject(ackLPC, INFINITE);
			m_bSpeechEnd = FALSE;
		}
	}else if (addr>=0x5500 && addr<0x5508 && Rom_Type==2) {	// FDC
		if(addr==0x5501)		addr=0x5502;
		else if(addr==0x5500)	addr=0x5507;
		bDiskAccess = 1;
		nes->fdc->writeIO(addr & 7, data);
	}else if(addr>=0x6000) CPU_MEM_BANK[addr>>13][addr&0x1FFF]=data;
}

void Mapper168::Write(WORD addr, BYTE data)
{
	if(Rom_Type==1){	//[Subor] Karaoke (C)
		SetPROM_32K_Bank( data&0x1F );
		if(data&0x40)	SetVRAM_Mirror( VRAM_HMIRROR );
		else			SetVRAM_Mirror( VRAM_VMIRROR );
		if(data&0xC0)	PPU_SW = 1;
		else			PPU_SW = 0;
	} else if(Rom_Type==2){
		if (reg5000&0x80)	//write 128K PRAM
			CPU_MEM_BANK[addr>>13][addr&0x1FFF]=data;
		SetBank_CPU();
	}
}

void	Mapper168::SetBank_CPU()
{
	if (Rom_Type==2) SetYPRAM_8K_Bank( 3 , 0x10 );
	if (reg5000&0x80) {	//RAM mode
		if (reg5200&0x04)
			SetYPRAM_32K_Bank(reg5000&3);
		else {
			SetYPRAM_16K_Bank(4,reg5000&7);	// MMC2 mode
			SetPROM_16K_Bank(6,0);
		}
	} else {
		if (reg5200&0x04)
			SetPROM_32K_Bank(reg5000);
		else {
			SetPROM_16K_Bank(4,reg5000);
			SetPROM_16K_Bank(6,0);
		}
	}
	SetCRAM_8K_Bank(0);
	SetVRAM_Mirror((reg5200&1)?VRAM_HMIRROR:VRAM_VMIRROR);
	PPU_SW=((reg5200&3)==2)?1:0;
}

void	Mapper168::PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr )
{
	INT loopy_v = nes->ppu->GetPPUADDR();
	INT loopy_y = nes->ppu->GetTILEY();
	INT tileofs = (PPUREG[0]&PPU_BGTBL_BIT)<<8;
	INT attradr = 0x23C0+(loopy_v&0x0C00)+((loopy_v&0x0380)>>4);
	INT attrsft = (ntbladr&0x0040)>>4;
	LPBYTE pNTBL = PPU_MEM_BANK[ntbladr>>10];
	INT ntbl_x  = ntbladr&0x001F;
	INT tileadr, ntb;
	ntb = (ntbladr>>10)&3;
	if(ntb==2)
	tileofs |= 0x1000;
	else if(ntb && PPU_SW)
	tileofs |= 0x1000;
	else
	tileofs |= 0x0000;
	attradr &= 0x3FF;
	attr = ((pNTBL[attradr+(ntbl_x>>2)]>>((ntbl_x&2)+attrsft))&3)<<2;
	tileadr = tileofs+pNTBL[ntbladr&0x03FF]*0x10+loopy_y;
	chr_l = PPU_MEM_BANK[tileadr>>10][ tileadr&0x03FF   ];
	chr_h = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+8];
}

void Mapper168::Clock(INT nCycles)
{
	if (Rom_Type==2) {
		nes->fdc->run();
		if (nes->fdc->irqRaised())	nes->cpu->SetIRQ(IRQ_MAPPER);
		else						nes->cpu->ClrIRQ(IRQ_MAPPER);
	}
}
