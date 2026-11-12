//////////////////////////////////////////////////////////////////////////
// Mapper560          C/E basic                                         //
//////////////////////////////////////////////////////////////////////////

void	Mapper560::Reset()
{
	mode = 1;
	OPT1 = OPT2 = 0;
	Rom_Type = 0;	// C/E basic type1
	memcpy(YCRAM+0x20000, VROM, 0x20000);
	if(nes->rom->GetSubMapperNo()==1)	// Subor_V1.0(SB-218)
	{
		Rom_Type = 1;
		VROM_SB2CE();
	}
	if(nes->rom->GetSubMapperNo()==2)	// C/E basic type2
	{
		Rom_Type = 2;
	}
	CRAM_CE2SB();
	SetPROM_32K_Bank( mode&1 );
	SetYCRAM_8K_Bank( 0 );
	nes->ppu->SetExtLatchMode( TRUE );
//	nes->SetVideoMode( 2 );
//	nes->pad->SetExController(PAD::EXCONTROLLER_KEYBOARD);
}

void	Mapper560::Write( WORD addr, BYTE data )
{
	mode = !mode;
	SetPROM_32K_Bank( mode&1 );
}

void	Mapper560::PPU_getNTdata( INT bank, WORD addr, BYTE data )
{
	if(mode){
		if(bank>=8)
			YSRAM[addr|(bank&3)<<10] = data;

		//Special Cases
		if(bank==7&&addr==0x3E0)
			PPU_MEM_BANK[7][0x3E0]=PPU_MEM_BANK[7][0x3E1];
		if(bank==8&&addr==0x000)
			PPU_MEM_BANK[8][0x000]=PPU_MEM_BANK[8][0x001];
		OPT1 = 0;
		if(YSRAM[0x089]==0x8D && YSRAM[0x096]==0xD0
		&& YSRAM[0x0E3]==0xDE && YSRAM[0x0FC]==0xDF
		&& YSRAM[0x2A3]==0xE0 && YSRAM[0x2BC]==0xE1)
			if(Rom_Type==0) OPT1 = 1;	//option 1;
		OPT2 = 0;
		if(YSRAM[0x047]==0xEF && YSRAM[0x098]==0xF2
		&& YSRAM[0x102]==0xDE && YSRAM[0x11E]==0xDF
		&& YSRAM[0x2A2]==0xE0 && YSRAM[0x2BE]==0xE1)
			if(Rom_Type==0) OPT2 = 1;	//option 2;
	}

}

void	Mapper560::PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr )
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

	if(mode){
		SetYCRAM_4K_Bank( 0, 1 );
		if(Rom_Type==1)
			SetYCRAM_4K_Bank( 0, 0 );
		if(YSRAM[(ntbladr&0x3FF)|0x400]!=1){
			chr_l = chr_h =
				YCRAM[(YSRAM[(ntbladr&0x3FF)|0x400]&0x3F)<<11|(tileadr&0x0FFF)>>1&~7|(tileadr&0x0FFF)&7|0x20000];
			//Special Cases
			if(OPT2==1 || Rom_Type==1)
				chr_h = 0;
		}else{
			chr_l = chr_h =	PPU_MEM_BANK[tileadr>>10][tileadr&0x03FF];
			//Special Cases
			if(OPT1==1 && (tileadr>>10)==3)
				chr_l = chr_h = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+8];
			if(OPT2==1 || Rom_Type==1)
				chr_h = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+8];
		}
		if(Rom_Type==2) chr_h = 0;

	}else{
		SetYCRAM_8K_Bank( 0 );
		chr_l = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+0];
		chr_h = PPU_MEM_BANK[tileadr>>10][(tileadr&0x03FF)+8];
	}
}

void	Mapper560::CRAM_CE2SB()
{
	long i,j;
	j=0;
	for(i=0;i<0x2000;i++){
		memcpy(YCRAM+((j+0)<<3), YCRAM+0x20000+(i<<3), 8);
		memcpy(YCRAM+((j+1)<<3), YCRAM+0x30000+(i<<3), 8);
		j+=2;
	}
}

void	Mapper560::VROM_SB2CE()
{
	long i,j;
	j=0;
	for(i=0;i<0x2000;i++){
		memcpy(YCRAM+0x20000+(i<<3), VROM+((j+0)<<3), 8);
		memcpy(YCRAM+0x30000+(i<<3), VROM+((j+1)<<3), 8);
		j+=2;
	}
}
