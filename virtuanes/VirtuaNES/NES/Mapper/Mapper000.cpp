//////////////////////////////////////////////////////////////////////////
// Mapper000                                                            //
//////////////////////////////////////////////////////////////////////////
void	Mapper000::Reset()
{
	switch( PROM_16K_SIZE ) {
		default:
		case	1:	// 16K only
			SetPROM_16K_Bank( 4, 0 );
			SetPROM_16K_Bank( 6, 0 );
			break;
		case	2:	// 32K
			SetPROM_32K_Bank( 0 );
			break;
	}

	DWORD	crc = nes->rom->GetPROM_CRC();
	if( crc == 0x4e7db5af ) {	// Circus Charlie(J)
		nes->SetRenderMethod( NES::POST_RENDER );
	}
	if( crc == 0x57970078 ) {	// F-1 Race(J)
		nes->SetRenderMethod( NES::POST_RENDER );
	}
	if( crc == 0xaf2bbcbc		// Mach Rider(JU)
	 || crc == 0x3acd4bf1 ) {	// Mach Rider(Alt)(JU)
		nes->SetRenderMethod( NES::POST_RENDER );
	}

	SP_ROM = 0;
	if( crc == 0x366c20d7 ) {	// Subor LOGO V1.0 (C)
		SP_ROM = 1;
		regFFFF = NT_data = 0;
	}
}

void	Mapper000::Write( WORD addr, BYTE data )
{
	if ( (addr==0xFFFF) && (SP_ROM==1) ) regFFFF = data;

	if(nes->rom->GetPROM_CRC()==0x15612dc8){	// 迷魂车[黄信维]
		SetVROM_4K_Bank( 0, addr&1 );
		SetVROM_4K_Bank( 4, addr&1 );
		if(addr&1)	SetVRAM_Mirror( VRAM_MIRROR4H );
		else		SetVRAM_Mirror( VRAM_MIRROR4L );
	}
}

void	Mapper000::PPU_Latch( WORD addr )
{
	if( ((addr&0xF000)==0x2000) && (SP_ROM==1) ) {
		NT_data = (addr>>8) & 0x03;
		switch ( regFFFF&0x03 ) {
			case 0:	//Initialization
				SetCRAM_8K_Bank( regFFFF );
				break;
			case 1:	//Text Screen (全文字屏)
				SetCRAM_4K_Bank( 4, regFFFF );
				break;
			case 2:	//Full Screen (全图形屏)
				SetCRAM_4K_Bank( 4, NT_data+2 );
				break;
			case 3:	//Split Screen (图文混屏)
				SetCRAM_4K_Bank( 4, NT_data+2 );
				if(NT_data==3) SetCRAM_4K_Bank( 4, 1 );
				break;
		}
	}
}
