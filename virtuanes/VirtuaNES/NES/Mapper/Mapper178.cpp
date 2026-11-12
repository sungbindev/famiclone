//////////////////////////////////////////////////////////////////////////
// Mapper178  Education / WaiXing_FS305                                 //
//////////////////////////////////////////////////////////////////////////

#define		mirrorH !!(reg[0]&1)
#define		unrom   !!(reg[0]&2)
#define		nrom128 !!(reg[0]&4)
#define		prg     (OP_rom?reg[1]<<3|reg[2]<<1:reg[1]&7|reg[2]<<3)

void    Mapper178::Reset()
{
	for(int i=0;i<4;i++) reg[i]=0;
	SetBank_CPU();
	nes->SetVideoMode( 2 );
	OP_rom = 0;
	DWORD	crc = nes->rom->GetPROM_CRC();
	if( crc == 0x925926BC		//[CoolBoy] Kou Dai Zuan Shi Zhi Chong Wu Xiao Jing Ling 2 (C)
	|| crc == 0xB0B13DBD )		//[CoolBoy] Chong Wu Fei Cui Zhi Chong Wu Xiao Jing Ling IV (C)
		OP_rom = 1;
}

void	Mapper178::WriteLow( WORD addr, BYTE data )
{
	if( addr>=0x4800 && addr<=0x4803 ){
		reg[addr&3]=data;
		SetBank_CPU();
	}else if( addr>=0x6000 ) {
		CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
	}
}

void	Mapper178::SetBank_CPU()
{
//	SetYPRAM_8K_Bank(3, reg[3]);
	SetCRAM_8K_Bank(0);
	if(mirrorH)	SetVRAM_Mirror( VRAM_HMIRROR );
	else		SetVRAM_Mirror( VRAM_VMIRROR );
	SetPROM_16K_Bank(4, prg&~(!nrom128*!unrom  ));
	SetPROM_16K_Bank(6, prg| (!nrom128 |unrom*6));
}
