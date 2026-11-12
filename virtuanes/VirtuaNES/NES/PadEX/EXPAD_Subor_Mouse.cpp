//////////////////////////////////////////////////////////////////////////
//      Subor Mouse                                                     //
//////////////////////////////////////////////////////////////////////////
// code form NintendulatorNRS by NewRisingSun.

struct SuborMouse_State {
	unsigned long Bits;
	unsigned char BitPtr;
	unsigned char Strobe;
	int Xmov, Ymov;
	int Xdelta, Ydelta;
	unsigned char Buttons;
	unsigned char ByteNum;
} *State;

void	EXPAD_Subor_Mouse::Reset()
{
	bOut = FALSE;
	ScanNo = 0;
	mouse_x = mouse_y = 0;
	State =new SuborMouse_State;
	State->Bits = 0;
	State->BitPtr = 0;
	State->Strobe = 0;
	State->Xmov = 0;
	State->Ymov = 0;
	State->Xdelta = 0;
	State->Ydelta = 0;
	State->Buttons = 0;
	State->ByteNum = 0;
}

BYTE	EXPAD_Subor_Mouse::Read4016()
{
	unsigned char result = 0x40;
	if (nes->pad->Subor_is1P)
		if (State->BitPtr <24)
			return (unsigned char)(((State->Bits << State->BitPtr++) & 0x800000) >> 23);

	return result;
}

BYTE	EXPAD_Subor_Mouse::Read4017()
{
	unsigned char result = 0x40;
	if (!nes->pad->Subor_is1P)
		if (State->BitPtr <24)
			return (unsigned char)(((State->Bits << State->BitPtr++) & 0x800000) >> 23);

	if (nes->pad->Subor_KB) result=Subor_KeyMap(ScanNo, bOut, 0xFF);
	return result;
}

void	EXPAD_Subor_Mouse::Write4016( BYTE data )
{
	if ((data &7) ==1 && (nes->pad->Mouse_type ==1)) {
		State->Bits =0;
		int amountY = State->Ymov;
		int amountX = State->Xmov;
		if (amountY <0) amountY =-amountY;
		if (amountX <0) amountX =-amountX;
		if (amountY >127) amountY =127;
		if (amountX >127) amountX =127;
		if (State->Ymov <0) {
			State->Bits |=0x080000 | amountY;
			State->Ymov +=amountY;
		} else
		if (State->Ymov >0) {
			State->Bits |=0x040000 | amountY;
			State->Ymov -=amountY;
		}
		if (State->Xmov <0) {
			State->Bits |=0x020000 | (amountX <<8);
			State->Xmov +=amountX;
		} else
		if (State->Xmov >0) {
			State->Bits |=0x010000 | (amountX <<8);
			State->Xmov -=amountX;
		}
		if (amountY >1 || amountX >1) State->Bits |=0x200000;
		State->Bits |=State->Buttons <<22;
		State->BitPtr = 0;
	} else
	if (State->Strobe && (~data &1) && (nes->pad->Mouse_type ==2)) {
		int Ymov =State->Ymov;
		int Xmov =State->Xmov;
		if (Ymov <-31) Ymov =-31;
		if (Xmov <-31) Xmov =-31;
		if (Ymov > 31) Ymov = 31;
		if (Xmov > 31) Xmov = 31;
		if (Xmov || Ymov) {
			if (++State->ByteNum >2) {
				State->ByteNum =0;
			}
			switch (State->ByteNum) {
				case 0:	State->Bits  =0x01;
					State->Bits |=State->Buttons <<6;
					State->Bits |=(Ymov <0)? 0x08: 0x00;
					State->Bits |=(((Ymov <0)? -Ymov: Ymov) >>4) <<2;
					State->Bits |=(Xmov <0)? 0x20: 0x00;
					State->Bits |=(((Xmov <0)? -Xmov: Xmov) >>4) <<4;
					State->Bits |=(State->Buttons &3) <<6;
					break;
				case 1: State->Bits  =0x02;
					State->Bits |=(((Xmov <0)? -Xmov: Xmov) &0xF) <<2;
					break;
				case 2: State->Bits  =0x03;
					State->Bits |=(((Ymov <0)? -Ymov: Ymov) &0xF) <<2;
					State->Xmov -=Xmov;
					State->Ymov -=Ymov;
					break;
			}
		} else {
			State->Bits  =0x00;
			State->Bits |=(State->Buttons <<6);
		}	
		State->Bits <<=16;
		State->BitPtr = 0;
	}
	State->Strobe =data &1;

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

void	EXPAD_Subor_Mouse::Sync()
{

	State->Buttons = 0;
	if( ::GetAsyncKeyState(VK_LBUTTON)&0x8000 )
		State->Buttons |= 0x02;
	if( ::GetAsyncKeyState(VK_RBUTTON)&0x8000 )
		State->Buttons |= 0x01;
	if( ::GetAsyncKeyState(VK_MBUTTON)&0x8000 )
		State->Buttons |= 0x04;

	int Xdelta = mouse_x;
	int Ydelta = mouse_y;
	mouse_x = mouse_y = 0;
	State->Xdelta =(Xdelta >>1) | (Xdelta &1);
	State->Ydelta =(Ydelta >>1) | (Ydelta &1);
	State->Xmov +=State->Xdelta;
	State->Ymov +=State->Ydelta;

}

void	EXPAD_Subor_Mouse::SetMouseStatus( INT nX, INT nY, INT nKeys, INT nFlags )
{
	if (nFlags & 0xF0) {
		mouse_x = nX;
		mouse_y = nY;
	} else {
		;
	}
}
