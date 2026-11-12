//////////////////////////////////////////////////////////////////////////
// Mapper177  HengGe                                                    //
//////////////////////////////////////////////////////////////////////////

#include "LPC_D6_SYNTH.H"
//------------------------------------------------------------------------------
int Mapper177::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	Mapper177* thiz = (Mapper177*)host;
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
DWORD WINAPI Mapper177::LPCThreadProc(LPVOID lpParam)
{
	void*    synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024]; // should be enough
	Mapper177* thiz = (Mapper177*)lpParam;
	wavLPC.Open(10000, 1); // 10Khz Mono
	synLPC = lpc_d6_synth_new(FeedLPC, thiz, LPC_STD_VARIANT_ABC);
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
void Mapper177::Init()
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
Mapper177::~Mapper177()
{
	bRunLPC = FALSE;
	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);
	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);
}
//------------------------------------------------------------------------------

void    Mapper177::Reset()
{
	SetPROM_32K_Bank( 0 );
	SP_rom = 0;
	DWORD	crc = nes->rom->GetPROM_CRC();
	if(crc==0xB5E83C9A) SP_rom=1;	//HengGe - Xing Ji Zheng Ba (C)

//	nResetLPC = FALSE;
	nResetLPC = TRUE;
	::WaitForSingleObject(ackLPC, INFINITE);
//	for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
//	nSPReadPos = nSpWritePos = 0;
	stateLPC = 0;

}

BYTE	Mapper177::ReadLow( WORD addr )
{
	if(addr==0x5002) {	// LPC
//		DEBUGOUT("Read %02X\n", addr);
//		if (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
//			stateLPC = 0x00;
//		else
//			stateLPC = 0x8F;

		if (((nSpWritePos+1)&(SP_BUF_SIZE-1))==nSPReadPos)
			stateLPC=0x00;
		else
			stateLPC=m_bSpeechEnd?0x8F:0x80;

		return stateLPC;
	}else if(addr>=0x6000) return CPU_MEM_BANK[addr>>13][addr&0x1FFF];
	return Mapper::ReadLow( addr );
}

void	Mapper177::WriteLow( WORD addr, BYTE data )
{
	if((addr==0x4800)&&(SP_rom)){
		if(data& 0x01)	SetVRAM_Mirror( VRAM_HMIRROR );
		else			SetVRAM_Mirror( VRAM_VMIRROR );
	}
	if(addr==0x5002) {
		DEBUGOUT("Write %02X -> [%04X]\n", data, addr);
		while (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
			Sleep(10); // should not be here
		nSPData[nSpWritePos] = data;
		nSpWritePos = (nSpWritePos + 1) & (SP_BUF_SIZE - 1);
		if((stateLPC==0x8F)&&m_bSpeechEnd) {	//LPC reset???
			nResetLPC = TRUE;
//			nSPReadPos = nSpWritePos = 0;
			::WaitForSingleObject(ackLPC, INFINITE);
//			for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
			m_bSpeechEnd = FALSE;
		}
	}
	if( addr>=0x6000 ) {
		CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
	}
}

void	Mapper177::Write( WORD addr, BYTE data )
{
	SetPROM_32K_Bank( data );
	if(SP_rom==0){
		if(data&0x20)	SetVRAM_Mirror( VRAM_HMIRROR );
		else			SetVRAM_Mirror( VRAM_VMIRROR );
	}
}

