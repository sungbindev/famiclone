//////////////////////////////////////////////////////////////////////////
// 8088 XT Keyboard                                                     //
//                                                   written by XXXXXX  //
//                                                     last 2025/06/08  //
//////////////////////////////////////////////////////////////////////////

// 根据Rom运行代码推理出来的，不太正确，勉强能用；

void	EXPAD_XT_Keyboard::Reset()
{
	bOut = false;
	Enabled = false;
	r_LSB = false;
	r_MSB = false;
	ScanNo_pre = 0;
	ScanNo = 0;
	b1 = b2 = false;
}

BYTE	EXPAD_XT_Keyboard::Read4016()
{
	return	((b1&&b2)&1)<<1;
//	return	(bOut&1)<<1;
}

BYTE	EXPAD_XT_Keyboard::Read4017()
{
	unsigned char data = 0;
	if(Enabled){
		if(r_LSB){
			data = ((((ScanNo&0x0F)^0xFF)<<1)&0x0F)|(((ScanNo&0x0F)>7)?0x00:0x10);
		}
		if(r_MSB){
			data = (((ScanNo&0xF0)^0xFF)>>3)&(((ScanNo&0xF0)>0x70)?0x0F:0x1F);
		}
	}
	return data&0xFF;
}

void	EXPAD_XT_Keyboard::Write4016( BYTE data )
{
//	DEBUGOUT("Write4016 - [%02X]\n", data);
	switch( data ) {
		case	0:	//0 0 0
			;
			break;
		case	1:	//0 0 1 声像卡
			;
			break;
		case	2:	//0 1 0
			;
			break;
		case	3:	//0 1 1
			;
			break;
		case	4:	// 1 0 0 声像卡/打字专家/外部数据
			;
			break;
		case	5:	// 1 0 1
			bOut = !bOut;
			r_MSB = true;
			r_LSB = false;
			b1 = true;
			;
			break;
		case	6:	// 1 1 0
			bOut = false;
			r_MSB = false;
			r_LSB = false;
			b1 = false;
			b2 = false;
			;
			break;
		case	7:	// 1 1 1
			bOut = !bOut;
			r_MSB = false;
			r_LSB = true;
			b2 = true;
			;
			break;
	}

}

void	EXPAD_XT_Keyboard::Sync()
{
	Enabled = false;
	ScanNo_pre = ScanNo;
	for(int i=0;i<256;i++){
		if(DirectInput.m_Sw[i]){
			ScanNo = i&0x7F;
			Enabled = true;
			break;
		}
	}
	if(!Enabled){
		ScanNo = ScanNo_pre|0x80;
		Enabled = true;
	}
	if(Enabled&&(ScanNo_pre&0x80)){
//		ScanNo = 0;
		Enabled = false;
	}

}