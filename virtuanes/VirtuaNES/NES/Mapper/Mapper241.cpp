//////////////////////////////////////////////////////////////////////////
// Mapper241  Education X-in-1                                          //
//////////////////////////////////////////////////////////////////////////

#include "LPC_D6_SYNTH.H"

//------------------------------------------------------------------------------
int Mapper241::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	Mapper241* thiz = (Mapper241*)host;

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

DWORD WINAPI Mapper241::LPCThreadProc(LPVOID lpParam)
{
	void*    synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024]; // should be enough

	Mapper241* thiz = (Mapper241*)lpParam;

	wavLPC.Open(10000, 1); // 10Khz Mono

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

			// command response
			if (!thiz->bRunLPC) {
				::SetEvent(thiz->ackLPC);
				break;
			}

			if (thiz->nResetLPC) {
				thiz->nResetLPC = FALSE;
				lpc_d6_synth_reset(synLPC);
				::SetEvent(thiz->ackLPC);
			}

			if (0 == consumed)
				Sleep(10);
		}

		if (eos)
			wavLPC.Drain();

	}

	lpc_d6_synth_delete(synLPC);

	wavLPC.Drain();
	wavLPC.Close();
	return 0;
}
//------------------------------------------------------------------------------

void Mapper241::Init()
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

Mapper241::~Mapper241()
{
	bRunLPC = FALSE;
	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);
	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);
}

//------------------------------------------------------------------------------

void    Mapper241::Reset()
{
	SetPROM_32K_Bank( 0 );
	if( VROM_1K_SIZE ) SetVROM_8K_Bank( 0 );

	Rom_type = 0;
	a3 = p_mode = s_mode = NT_data = 0;
	
	DWORD	crc = nes->rom->GetPROM_CRC();
	if(crc == 0x85068811) Rom_type = 1;	// [Subor] Subor V6.0 (C)
	if(crc == 0xE475D89A) Rom_type = 2;	// [Subor] Subor V9.0 (C)
	if(crc == 0x900D9E00) Rom_type = 2;	// [Subor] Subor V9.1 (C)
	if(crc == 0x51c41a5a				// [ChangDa] Duo Mei Ti Xue Sheng Dian Nao (C)
	|| crc == 0xd83d7f67){Rom_type = 3;	// 16-in-1 (ABM Study Card v5.0) (ChS) [by maxzhou88]
		nes->ppu->SetExtLatchMode( TRUE );}

	nResetLPC = FALSE;
	if(crc == 0x957ABE27){	// Sheng Ba Ka (C)
		nResetLPC = TRUE;
		::WaitForSingleObject(ackLPC, INFINITE);
		for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
	}

	stateLPC = 0;
	count = 0;
	temp_data = 0;

}

BYTE	Mapper241::ReadLow( WORD addr )
{
	if((addr>=0x5000)&&(addr<0x6000)){
		if (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
			stateLPC = 0x00;
		else
			stateLPC = 0x40;
		return stateLPC;
	}

	return	Mapper::ReadLow( addr );
}

void	Mapper241::WriteLow( WORD addr, BYTE data )
{
	if((addr>=0x5000)&&(addr<0x6000)){
		if((data&0xF0)==0x50){
//			nResetLPC = FALSE;
			count++;
			if(count==1) temp_data = data & 0x0F;
			if(count==2){
				count = 0;
				while (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
					Sleep(10); // should not be here
				nSPData[nSpWritePos] = ((data&0x0F)<<4) | temp_data;
				nSpWritePos = (nSpWritePos + 1) & (SP_BUF_SIZE - 1);
			}

		} else {
//			nSPReadPos = nSpWritePos = 0;
			nResetLPC = TRUE;
			::WaitForSingleObject(ackLPC, INFINITE);
			for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
			count = 0;
		}
	}

	if(addr>=0x6000){
		CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
	}
}

void	Mapper241::Write( WORD addr, BYTE data )
{

//	if(nes->GetScanline() == 283)
	DEBUGOUT( "MPRWR A=%04X D=%02X L=%3d CYC=%d\n", addr&0xFFFF, data&0xFF, nes->GetScanline(), nes->cpu->GetTotalCycles() );

//	if( addr == 0x8000 )	SetPROM_32K_Bank( data );

//	if( data&0x80 ) 
//		SetPROM_32K_Bank( ((data&7)>>1)+4 );
//	else
//		SetPROM_32K_Bank( (data&7)>>1 );

//	if( data&0x80 ) 
//		SetPROM_32K_Bank( data+8 );
//	else
		SetPROM_32K_Bank( data&0x3F );
		SetCRAM_8K_Bank(0);

	if(Rom_type==1)	//[Subor] Subor V6.0 (C)
		if((addr==0x8D56)&&(data==3)&&(nes->GetScanline()==283))
		{
			SetPROM_32K_Bank( 0 );
		}
	if(Rom_type==2)	//[Subor] Subor V9.0 (C)
	{
		if((addr==0x8D5C)&&(data==9)&&((nes->GetScanline()>282)&&(nes->GetScanline()<291)))
		{
			SetPROM_32K_Bank( 0 );
		}
		if((addr==0x8D56)&&(data==3)&&(nes->GetScanline()==283))
		{
			SetPROM_32K_Bank( 0 );
		}
	}
	if(Rom_type==3){
		s_mode = data&0x40;
		p_mode = data&0x80;
	}

}

void	Mapper241::PPU_Latch( WORD addr )
{
	if((addr&0xF000)==0x2000){
		NT_data=(addr>>8)&0x03;
		if(s_mode) SetCRAM_4K_Bank(4, NT_data<<1);
	}
}
void	Mapper241::PPU_ExtLatchX( INT x )
{
	a3 = (x&1)<<3;
}
void	Mapper241::PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr )
{
	INT loopy_v = nes->ppu->GetPPUADDR();
	INT loopy_y = nes->ppu->GetTILEY();
	INT	tileofs = (PPUREG[0]&PPU_BGTBL_BIT)<<8;
	INT	attradr = 0x23C0+(loopy_v&0x0C00)+((loopy_v&0x0380)>>4);
	INT	attrsft = (ntbladr&0x0040)>>4;
	LPBYTE	pNTBL = PPU_MEM_BANK[ntbladr>>10];
	INT	ntbl_x  = ntbladr&0x001F;
	INT	tileadr;

	attradr &= 0x3FF;
	attr = ((pNTBL[attradr+(ntbl_x>>2)]>>((ntbl_x&2)+attrsft))&3)<<2;
	tileadr = tileofs+pNTBL[ntbladr&0x03FF]*0x10+loopy_y;

	if(p_mode){
		if(NT_data&0x02)	tileadr |=  0x1000;
		else				tileadr &= ~0x1000;
		tileadr = (tileadr&0xfff7)|a3;
		chr_l = chr_h = PPU_MEM_BANK[tileadr>>10][ tileadr&0x03FF   ];
	}else{
		chr_l = PPU_MEM_BANK[tileadr>>10][ tileadr&0x03FF   ];
		chr_h = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+8];
	}
}
