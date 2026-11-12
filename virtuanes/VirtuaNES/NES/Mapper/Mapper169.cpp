//////////////////////////////////////////////////////////////////////////
// Mapper169          YuXing                                            //
//////////////////////////////////////////////////////////////////////////

#define Split_2Scr !!(reg4800&0x80 && !(reg5500&0x80))	//2 Split screen
#define Split_4Scr !!(reg4800&0x80 && reg5500&0x80)		//4 Split screen
#define MMC3_CRAMbank (MMC3_CRAMfix&0x10|MMC3_CRAMfix<<2&0x0C|MMC3_CRAMfix>>2&3)	//fix CRAM bank for MMC3

void	Mapper169::Reset()
{
	key_map_row = 0;

	reg5002 = 2;
	reg4800 = 0;
	reg5500 = 0;
	reg5501 = 0;
	reg8000 = 0;

	a3 = 0;
	NT_data = 0;

	MMC3_mode = 0;
	MMC3_Reset();

	SetPROM_32K_Bank( 0 );
	SetYCRAM_8K_Bank( 0 );

	bDiskAccess  = 0;
	R4034count = 0;
	nes->fdc->isYuXing = true;
	nes->fdc->reset();	//normal speed; the same as D and 98/F.

	nResetLPC = TRUE;
	::WaitForSingleObject(ackLPC, INFINITE);
	for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
	readLPC = FALSE;
	stateLPC = 0;
	count = 0;
	dataLSB = dataMSB = 0;
	maskMSB = 0xC0;
	datamask = 0x41;

	PRAM_mask = 0x3F;	//1024K PRAM
	CRAM_mask = 0x0F;	//128K VRAM

	YX_type = 0;
	DWORD	crc = nes->rom->GetPROM_CRC();
	if(crc == 0xCEAC04C7	//YX_V4.0
	 ||crc == 0xFFFFFFFF	//
	 ||crc == 0xFFFFFFFF)	//
	{
		YX_type = 7;
		PRAM_mask = 0x01;	//32K PRAM
		CRAM_mask = 0x03;	//32K VRAM
		nes->pad->SetExController(PAD::EXCONTROLLER_KEYBOARD); //Family Basic Keyboard
	}
	if(crc == 0x3B02AF09	//YX_V5.0
	 ||crc == 0x871254E8	//YX_V5.0 + WuBi
	 ||crc == 0xFFFFFFFF	//YX_V5.0 + 
	 ||crc == 0xFFFFFFFF)	//
	{
		YX_type = 1;
		PRAM_mask = 0x01;	//32K PRAM
		CRAM_mask = 0x03;	//32K VRAM
		nes->pad->SetExController(PAD::EXCONTROLLER_XT_KEYBOARD);
	}
	if(crc == 0x3DF65263)	//YX_V7.0
	{
		YX_type = 2;
		PRAM_mask = 0x07;	//128K PRAM
		CRAM_mask = 0x03;
	}
	if(crc == 0x6BA6BD80)	//YX_V8.0-C
	{
		YX_type = 3;
		PRAM_mask = 0x07;
		CRAM_mask = 0x03;
	}
	if(crc == 0x2B1B969E	//YX_V8.2-D
	|| crc == 0xA70FD0F3)	//YX_V8.3-D
	{
		YX_type = 4;
		PRAM_mask = 0x07;
		CRAM_mask = 0x03;
	}
	if(crc == 0x6085FEE8)	//YX_V9.0-98
	{
		YX_type = 5;
		PRAM_mask = 0x3F;
		CRAM_mask = 0x0F;
	}
	if(crc == 0x2A1E4D89	//YX_V9.2-98
	|| crc == 0x5C6CE13E)	//YX_V9.2-F
	{
		YX_type = 6;
		PRAM_mask = 0x3F;
		CRAM_mask = 0x1F;
	}

	nes->SetVideoMode( 2 );
	nes->ppu->SetExtLatchMode( TRUE );
}

BYTE	Mapper169::ReadLow( WORD addr )
{
	int key_map[14][8] = {	//YuXing_Keyboard data by tpu
		{DIK_ESCAPE, DIK_F9,         DIK_7,      DIK_R,       DIK_A,      0x00,          0x00,         DIK_LSHIFT},
		{0x00,       DIK_NUMPADENTER,0x00,       DIK_MULTIPLY,DIK_DIVIDE, DIK_UP,        DIK_BACKSPACE,DIK_F12   },
		{0x00,       0x00,           0x00,       DIK_ADD,     DIK_NUMLOCK,DIK_LEFT,      DIK_DOWN,     DIK_RIGHT },
		{0x00,       DIK_NUMPAD7,    DIK_NUMPAD8,DIK_SUBTRACT,DIK_NUMPAD9,DIK_F11,       DIK_END,      DIK_PGDN  },
		{DIK_F8,     DIK_6,          DIK_E,      DIK_RBRACKET,DIK_L,      DIK_Z,         DIK_N,        DIK_SPACE },
		{DIK_F7,     DIK_5,          DIK_W,      DIK_LBRACKET,DIK_K,      DIK_DELETE,    DIK_B,        DIK_SLASH },
		{DIK_F6,     DIK_4,          DIK_Q,      DIK_P,       DIK_J,      DIK_BACKSLASH, DIK_V,        DIK_PERIOD},
		{DIK_F5,     DIK_3,          DIK_EQUALS, DIK_O,       DIK_H,      DIK_APOSTROPHE,DIK_C,        DIK_COMMA },
		{DIK_F4,     DIK_2,          DIK_MINUS,  DIK_I,       DIK_G,      DIK_SEMICOLON, DIK_X,        DIK_M     },
		{DIK_F3,     DIK_1,          DIK_0,      DIK_U,       DIK_F,      DIK_CAPITAL,   DIK_LWIN,     0x00      },
		{DIK_F2,     DIK_GRAVE,      DIK_9,      DIK_Y,       DIK_D,      DIK_LCONTROL,  DIK_LMENU,    DIK_SCROLL},
		{DIK_F1,     DIK_RETURN,     DIK_8,      DIK_T,       DIK_S,      0x00,          DIK_RWIN,     DIK_APPS  },
		{DIK_DECIMAL,DIK_F10,        DIK_NUMPAD4,DIK_NUMPAD6, DIK_NUMPAD5,DIK_INSERT,    DIK_HOME,     DIK_PGUP  },
		{0x00,       DIK_NUMPAD0,    DIK_NUMPAD1,DIK_NUMPAD3, DIK_NUMPAD2,DIK_SYSRQ,     DIK_TAB,      DIK_PAUSE },
	};
	if( YX_type == 4 )	//for D (without new keys)
	{
		key_map[9][6] = 0x00;
		key_map[11][6] = 0x00;
		key_map[11][7] = 0x00;
	}

	bDiskAccess = 0;

	switch( addr ) {
		case	0x4207:	//YuXing_Keyboard code by tpu
			int r, i, data;
			data = 0;
			for(r=0; r<14; r++){
				if(key_map_row&(1<<r)){
					for(i=0; i<8; i++){
						if(key_map[r][i])
							if(DirectInput.m_Sw[key_map[r][i]]) data |= 1<<i;
						//special cases
						if((r== 0)&&(i==7))
							if(DirectInput.m_Sw[DIK_RSHIFT   ]) data |= 1<<i;
						if((r==10)&&(i==5))
							if(DirectInput.m_Sw[DIK_RCONTROL ]) data |= 1<<i;
						if((r==10)&&(i==6))
							if(DirectInput.m_Sw[DIK_RMENU    ]) data |= 1<<i;
					}
				}
			}
			return data;
			break;

		case	0x5002:	// data&2=0: VCD screen, data&2=2: No VCD Screen
			return reg5002;
			break;

		case	0x4304:	//FDCResetPortO/FDCStatPortI
		case	0x4305:	//FDCDataPortIO
			bDiskAccess = 1;
			return nes->fdc->readIO(addr & 7);
			break;

		case	0x4701:	//LPC
				if (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
					stateLPC = 0x80;
				else
					stateLPC = 0x00;
				return stateLPC;
			break;

		default:
//			DEBUGOUT("Read %02X\n", addr);
			break;

	}

	if(addr>=0x6000) return CPU_MEM_BANK[addr>>13][addr&0x1FFF];

	return Mapper::ReadLow( addr );
}

void	Mapper169::WriteLow( WORD addr, BYTE data )
{
	bDiskAccess = 0;

	switch( addr ) {
		case	0x4200://FDCChangePortI/FDCSpeedPortO
			bDiskAccess = 1;
			nes->fdc->writeIO(7, data);
			break;
		case	0x4201://FDCDRQPortI/FDCCtrlPortO
			bDiskAccess = 1;
			nes->fdc->writeIO(2, data);
			break;
		case	0x4205:	//FDCDataPortIO
			bDiskAccess = 1;
			nes->fdc->writeIO(5, data);
			break;

		case	0x4202:	//YuXing_Keyboard
		case	0x4302:
		case	0x5004:	//裕兴超级学习机1号_Keyboard
			key_map_row &= 0xff00;
			key_map_row |= data;
			break;
		case	0x4203:	//YuXing_Keyboard
		case	0x4303:
		case	0x5005:	//裕兴超级学习机1号_Keyboard
			key_map_row &= 0x00ff;
			key_map_row |= (data & 0x3f)<<8;
			break;

		case	0x4700:	//LPC (PE tables)
			if(data==0x00){	//LPC reset
				readLPC=FALSE;
				count = 0;
				nResetLPC=TRUE;
//				::WaitForSingleObject(ackLPC, INFINITE);
				for(int i=0;i<SP_BUF_SIZE;i++) nSPData[i]=0;
			}else if(data==0xFF){	//LPC start read data
				readLPC=TRUE;
//			}else if((data&0x0F)==0 && !readLPC){	//LPC MSB mask ???
//				maskMSB = data;
			}else if((data&0xF0)==maskMSB && readLPC){	//LPC data
				nResetLPC=FALSE;
				count++;
				if(count==1) dataLSB = data&0x0F;
				if(count==2){
					count = 0;
					dataMSB = (data&0x0F)<<4;
					while (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
						Sleep(10); // should not be here
					nSPData[nSpWritePos] = (dataMSB | dataLSB) ^ datamask;
					nSpWritePos = (nSpWritePos + 1) & (SP_BUF_SIZE - 1);
				}
			}
			break;

		case	0x4800:
			reg4800 = data;
			SetBank_CPU();
			break;
		case	0x5500:
			reg5500 = data;
			SetBank_CPU();
			break;
		case	0x5501:
			reg5501 = data;
			SetYCRAM_8K_Bank(reg5501&CRAM_mask);
			if(reg5501&0x80){
				MMC3_mode = 1;
//				if(YX_type==6) CRAM_mask = 0x1F;	//256K VRAM for MMC3 games
//				SetYPRAM_16K_Bank(6,0x1F);
				MMC3_SetBank_CPU();
				MMC3_SetBank_PPU();
			}else{
				MMC3_mode = 0;
//				if(YX_type==6) CRAM_mask = 0x0F;
				SetBank_CPU();
			}
			break;
		default:
//			DEBUGOUT("Write %02X -> [%04X]\n", data, addr);
			break;
	}

	if(addr>=0x6000) CPU_MEM_BANK[addr>>13][addr&0x1FFF]=data;
}

void	Mapper169::Write( WORD addr, BYTE data )
{
	if(MMC3_mode){
		MMC3_Write( addr, data );
	}else{
		if(reg5500&0x04){
			if (YX_type==1) {
				reg8000 = data&7;
			}else{
				if(reg4800&0x20){
					reg8000 = data&PRAM_mask;
				}else{ //write PRAM
					CPU_MEM_BANK[addr>>13][addr&0x1FFF]=data;
				}
			}
		}
		SetBank_CPU();
	}
}

void	Mapper169::SetBank_CPU()
{
	if(reg5500&0x04 && MMC3_mode) return;

	if(reg5500&0x04){
		if (YX_type==1) {
			SetPROM_16K_Bank( 4, 0x20|reg8000 );
			SetPROM_16K_Bank( 6, 0x27 );
//		if (YX_type==7) {
//			SetPROM_16K_Bank( 4, 0x4|(reg4800&3) );
//			SetPROM_16K_Bank( 6, 0x7 );
		}else{
			if (reg5500&0x40) {
				SetYPRAM_32K_Bank( reg8000>>1 );
			} else {
				SetYPRAM_16K_Bank( 4, reg8000 );
				SetYPRAM_16K_Bank( 6, PRAM_mask );
			}
		}
	}else{
		SetPROM_32K_Bank(reg4800&0x1F);

//		if(YX_type==7){
//			SetPROM_32K_Bank((reg4800&0x40)>>6);
//		}

	}

//	if(reg5500&0x3)	SetYPRAM_8K_Bank( 3 , reg5500&0x3 );	//???
//	else			SetYPRAM_8K_Bank( 3 , 0x3C&PRAM_mask );	//???

	if(reg5500&0x3)		SetPROM_Bank( 3, &YSRAM[0x2000*(reg5500&0x3)], BANKTYPE_RAM );	//???
	else if(YX_type!=1)	SetYPRAM_8K_Bank( 3 , 0x3C&PRAM_mask );	//???

	SetVRAM_Mirror((reg5500&8)?VRAM_HMIRROR:VRAM_VMIRROR);
}

void	Mapper169::PPU_Latch( WORD addr )
{
	switch (addr>>12) {
		case 0:	// PPUMEM 0x0000 - 0x0FFF
		case 1:	// PPUMEM 0x1000 - 0x1FFF
			if (Split_4Scr)
				SetYCRAM_8K_Bank(reg5501&CRAM_mask);
			break;
		case 2:	// PPUMEM 0x2000 - 0x2FFF
			NT_data=(addr>>8)&0x03;
			if (Split_4Scr)
				SetYCRAM_4K_Bank(4,NT_data<<3|(reg5501&0x03)<<1|1);
			break;
	}
}
void	Mapper169::PPU_ExtLatchX( INT x )
{
	a3 = (x&1)<<3;
}
void	Mapper169::PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr )
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
	if(Split_2Scr){
		if(NT_data&0x02)	tileadr |=  0x1000;
		else				tileadr &= ~0x1000;
		tileadr = (tileadr&0xFFF7)|a3;
		chr_l = chr_h = PPU_MEM_BANK[tileadr>>10][tileadr&0x03FF];
	}else{
		chr_l = PPU_MEM_BANK[tileadr>>10][ tileadr&0x03FF   ];
		chr_h = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+8];
	}
}
BYTE	Mapper169::PPU_ExtLatchSP()
{
	if(Split_2Scr) return 2;
	return 0;
}

void Mapper169::Clock(INT nCycles)
{
	nes->fdc->run();

	if(DirectInput.m_Sw[DIK_PAUSE]){
//		nes->Dump_YPRAM();
//		nes->Dump_YSRAM();
	}

}

void	Mapper169::HSync(INT scanline)
{
	if( (scanline >= 0 && scanline <= 239) && nes->ppu->IsDispON() ) {
		if( MMC3_mode ) MMC3_IRQSync();
	}
}

BYTE Mapper169::ExCmdRead( EXCMDRD cmd )
{
	if (cmd == Mapper::EXCMDRD_DISKACCESS) return bDiskAccess;
	return 0;
}

#include "LPC_D6_SYNTH.H"
int Mapper169::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	Mapper169* thiz = (Mapper169*)host;
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
DWORD WINAPI Mapper169::LPCThreadProc(LPVOID lpParam)
{
	void*    synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024]; // should be enough
	Mapper169* thiz = (Mapper169*)lpParam;
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
void Mapper169::Init()
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
Mapper169::~Mapper169()
{
	bRunLPC = FALSE;
	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);
	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);
}

// Generic MM3 -----------------------------------------------------------------
void	Mapper169::MMC3_Reset()
{
	MMC3_cmd = 0;
	MMC3_prg0 = 0;
	MMC3_prg1 = 1;
	MMC3_chr01 = 0;
	MMC3_chr23 = 2;
	MMC3_chr4  = 4;
	MMC3_chr5  = 5;
	MMC3_chr6  = 6;
	MMC3_chr7  = 7;
	MMC3_irq_enable = 0;
	MMC3_irq_counter = 0;
	MMC3_irq_latch = 0xFF;
	MMC3_irq_request = 0;
	MMC3_irq_preset = 0;
	MMC3_irq_preset_vbl = 0;
	MMC3_CRAMfix = 0;
}
void	Mapper169::MMC3_Write( WORD addr, BYTE data )
{
	switch( addr & 0xE001 ) {
		case	0x8000:
			MMC3_cmd = data;
			MMC3_SetBank_CPU();
			MMC3_SetBank_PPU();
			break;
		case	0x8001:
			switch( MMC3_cmd & 0x07 ) {
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
					MMC3_prg0 = data;
					MMC3_SetBank_CPU();
					break;
				case	0x07:
					MMC3_prg1 = data;
					MMC3_SetBank_CPU();
					break;
			}
			break;
		case	0xA000:
			SetVRAM_Mirror((data&1)?VRAM_HMIRROR:VRAM_VMIRROR);
			break;
		case	0xC000:
			MMC3_irq_latch = data;
			break;
		case	0xC001:
			if( nes->GetScanline() < 240 ) {
				MMC3_irq_counter |= 0x80;
				MMC3_irq_preset = 0xFF;
			} else {
				MMC3_irq_counter |= 0x80;
				MMC3_irq_preset_vbl = 0xFF;
				MMC3_irq_preset = 0;
			}
			break;
		case	0xE000:
			MMC3_irq_enable = 0;
			MMC3_irq_request = 0;
			nes->cpu->ClrIRQ( IRQ_MAPPER );
			break;
		case	0xE001:
			MMC3_irq_enable = 1;
			MMC3_irq_request = 0;
			break;
	}
}
void	Mapper169::MMC3_SetBank_CPU()
{
	BYTE pwrap = MMC3_cmd&0x40;
	SetYPRAM_8K_Bank( pwrap?6:4, MMC3_prg0 );
	SetYPRAM_8K_Bank(         5, MMC3_prg1 );
	SetYPRAM_8K_Bank( pwrap?4:6,      0x3E );
	SetYPRAM_8K_Bank(         7,      0x3F );
}
void	Mapper169::MMC3_SetBank_PPU()
{
	BYTE cwrap = MMC3_cmd&0x80;
	SetYCRAM_1K_Bank( cwrap?4:0, MMC3_fixCHR(MMC3_chr01+0) );
	SetYCRAM_1K_Bank( cwrap?5:1, MMC3_fixCHR(MMC3_chr01+1) );
	SetYCRAM_1K_Bank( cwrap?6:2, MMC3_fixCHR(MMC3_chr23+0) );
	SetYCRAM_1K_Bank( cwrap?7:3, MMC3_fixCHR(MMC3_chr23+1) );
	SetYCRAM_1K_Bank( cwrap?0:4, MMC3_fixCHR(MMC3_chr4) );
	SetYCRAM_1K_Bank( cwrap?1:5, MMC3_fixCHR(MMC3_chr5) );
	SetYCRAM_1K_Bank( cwrap?2:6, MMC3_fixCHR(MMC3_chr6) );
	SetYCRAM_1K_Bank( cwrap?3:7, MMC3_fixCHR(MMC3_chr7) );

}
BYTE	Mapper169::MMC3_fixCHR(BYTE chr)
{
	MMC3_CRAMfix = (chr>>3)&CRAM_mask;
	chr = MMC3_CRAMbank<<3|chr&7;
	return chr;
}
void	Mapper169::MMC3_IRQSync()
{
	if( MMC3_irq_preset_vbl ) {
		MMC3_irq_counter = MMC3_irq_latch;
		MMC3_irq_preset_vbl = 0;
	}
	if( MMC3_irq_preset ) {
		MMC3_irq_counter = MMC3_irq_latch;
		MMC3_irq_preset = 0;
	} else if( MMC3_irq_counter > 0 ) MMC3_irq_counter--;
	if( MMC3_irq_counter == 0 ) {
		if( MMC3_irq_enable ) {
			MMC3_irq_request = 0xFF;
			nes->cpu->SetIRQ( IRQ_MAPPER );
		}
		MMC3_irq_preset = 0xFF;
	}
}
//------------------------------------------------------------------------------
