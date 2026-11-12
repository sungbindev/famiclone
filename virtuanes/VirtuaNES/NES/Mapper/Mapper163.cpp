//////////////////////////////////////////////////////////////////////////
// Mapper163  NanJing Games                                             //
//////////////////////////////////////////////////////////////////////////

#define BANKs ((reg5000&0x0F)|(reg5200<<4))

void	Mapper163::Reset()
{
	reg5000 = 0xFF;
	reg5100 = reg5200 = reg5300 = 0;
	strobe = 1;
	trigger = 0;
	SetPROM_32K_Bank(0x0F);

	rom_type = 0;
	DWORD crc = nes->rom->GetPROM_CRC();
	if(crc == 0xb6a10d5d){	// Hu Lu Jin Gang (NJ039) (C)
		SetPROM_32K_Bank(0);
	}
	if(crc == 0xf52468e7	// San Guo Wu Shuang - Meng Jiang Zhuan (NJ047) (C)
	 ||crc == 0x696D98E3){	// San Guo Zhi Lv Bu Zhuan (NJ040) (C)
		rom_type = 1;
	}
	if(crc == 0xEFF96E8A){	// Mo Shou Shi Jie E Mo Lie Ren (NJ097) (C)
		rom_type = 2;
	}
	if(crc == 0x191f7d5e){	// Zhen Shi Hun Wu Shi Dao Lie Zhuan (NJ033) (C)
		rom_type = 3;
	}
}

BYTE	Mapper163::ReadLow(WORD addr)
{
	if((addr>=0x5000 && addr<0x6000)){
		switch(addr&0x7700){
			case 0x5100:
				return reg5300;
				break;
			case 0x5500:
				if(trigger) return reg5300;
				else		return 0;
				break;
		}
		return 4;
	}else if(addr>=0x6000){
		return CPU_MEM_BANK[addr>>13][addr&0x1FFF];
	}
	return Mapper::ReadLow( addr );
}

void	Mapper163::WriteLow(WORD addr, BYTE data)
{
	if((addr>=0x4020 && addr<0x6000)){
		if(addr==0x5101){
			if(strobe && !data){
				trigger ^= 1;
//				trigger ^= 0xFF;
			}
			strobe = data;
		}else if(addr==0x5100 && data==6){
			SetPROM_32K_Bank(3);
		}else{
			switch(addr&0x7300){
				case 0x5000:
					reg5000=data;
					SetPROM_32K_Bank(BANKs);
					if(!(reg5000&0x80)&&(nes->ppu->GetScanlineNo()<=127))
						SetCRAM_8K_Bank(0);
					if(rom_type==1) SetCRAM_8K_Bank(0);
					break;
				case 0x5100:	// PCM???
					reg5100=data;
					break;
				case 0x5200:
					reg5200=data;
					SetPROM_32K_Bank(BANKs);
					break;
				case 0x5300:
					reg5300=data;
					break;
			}
		}
	}else if(addr>=0x6000){
		CPU_MEM_BANK[addr>>13][addr&0x1FFF] = data;
		if(rom_type==3)
			if((addr>=0x7900 && addr<=0x79FF)){	// PCM data???
//				DEBUGOUT("Write %02X -> [%04X]\n", data, addr);
				;
			}
	}
}

void	Mapper163::HSync(int scanline)
{
	if((reg5000&0x80)&&nes->ppu->IsDispON()){
		if(scanline==127){
			SetCRAM_4K_Bank(0, 1);
			SetCRAM_4K_Bank(4, 1);
		}
		if(rom_type==1){
			if(scanline<127){
				SetCRAM_4K_Bank(0, 0);
				SetCRAM_4K_Bank(4, 0);
			}
		}else{
			if(scanline==239){
				SetCRAM_4K_Bank(0, 0);
				SetCRAM_4K_Bank(4, 0);
				if(rom_type==2) SetCRAM_4K_Bank(4, 1);
			}
		}
	}
}
