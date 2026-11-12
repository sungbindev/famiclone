//////////////////////////////////////////////////////////////////////////
// YuXing_Mouse                                                         //
//////////////////////////////////////////////////////////////////////////
// code form NintendulatorNRS by NewRisingSun.

void	EXPAD_YuXing_Mouse::Reset()
{
	mouse_button = 0;
	mouse_x = mouse_y = 0;
	mouse_x_move = mouse_y_move = 0;
	mouse_clock = false;
	bits = std::queue<bool>();

	bOut = FALSE;
	ScanNo = 0;
}

BYTE	EXPAD_YuXing_Mouse::Read4016()
{
	BYTE outbit =0x40;
	if (mouse_clock && !bits.empty()) outbit|=(bits.front()*1)^1;
	return outbit;
}

BYTE	EXPAD_YuXing_Mouse::Read4017()
{
	BYTE outbit =0x40;
	if (mouse_clock && !bits.empty()) outbit|=(bits.front()*1)^1;
	if (nes->pad->Subor_KB) outbit=Subor_KeyMap(ScanNo, bOut, 0xFF);
	return outbit;
}

void	EXPAD_YuXing_Mouse::Write4016( BYTE data )
{
	if (~data&4 && !mouse_clock && data&1){
		if (bits.empty()){
			AddByte(bits,(mouse_button&1?0x60:0x40)|(mouse_button&2?0x50:0x40)|mouse_x_move>>4&0x0C|mouse_y_move>>6&0x03);
			AddByte(bits,mouse_y_move&0x3F);
			AddByte(bits,mouse_x_move&0x3F);
		}else bits.pop();
	}
	mouse_clock =!!(data&1);

	if(nes->pad->Subor_KB){
		if (data & 4) {
			if ( data == 0x05 ) {
				bOut = FALSE;
				ScanNo = 0;
			} else if ( data == 0x04 ) {
				if ( ++ScanNo > 9 )
					ScanNo = 0;
				bOut = !bOut;
			} else if ( data == 0x06 ) {
				bOut = !bOut;
			}
		}
	}
}

void	EXPAD_YuXing_Mouse::Sync()
{
	mouse_button = 0;
	if( ::GetAsyncKeyState(VK_LBUTTON)&0x8000 )
		mouse_button |= 0x01;
	if( ::GetAsyncKeyState(VK_RBUTTON)&0x8000 )
		mouse_button |= 0x02;
	mouse_x_move=mouse_x;
	mouse_y_move=mouse_y;
	mouse_x = mouse_y = 0;
}

void EXPAD_YuXing_Mouse::AddByte(std::queue<bool>& to, BYTE data )
{
	to.push(false);
	for (int i =0; i <7; i++) {
		to.push(!!(data &0x40));
		data <<=1;
	}
	to.push(true);
}

void EXPAD_YuXing_Mouse::SetMouseStatus( INT nX, INT nY, INT nKeys, INT nFlags )
{
	if (nFlags & 0xF0) {
		mouse_x = nX;
		mouse_y = nY;
	} else {
		;
	}
}
