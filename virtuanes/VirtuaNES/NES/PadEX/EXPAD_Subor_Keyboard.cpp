//////////////////////////////////////////////////////////////////////////
// Subor Keyboard                                                       //
//////////////////////////////////////////////////////////////////////////
// code by TPU.

void	EXPAD_Subor_Keyboard::Reset()
{
	bOut = FALSE;
	ScanNo = 0;
}

BYTE	EXPAD_Subor_Keyboard::Read4016()
{
	BYTE	data = 0;
	return	data;
}

BYTE	EXPAD_Subor_Keyboard::Read4017()
{
	return	Subor_KeyMap(ScanNo, bOut, 0xFF);
}

void	EXPAD_Subor_Keyboard::Write4016( BYTE data )
{
	if( data == 0x05 ) {
		bOut = FALSE;
		ScanNo = 0;
	} else if( data == 0x04 ) {
		if( ++ScanNo > 13 )
			ScanNo = 0;
		bOut = !bOut;
	} else if( data == 0x06 ) {
		bOut = !bOut;
	}
}

