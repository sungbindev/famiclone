//////////////////////////////////////////////////////////////////////////
// BBK FD1 Keyboard & Mouse                                             //
//////////////////////////////////////////////////////////////////////////

#include <assert.h>

#define MOUSE_ENABLE
//#define BBK_MOUSE_HACK

#define BIOS_Mouse0X			0x5FF2
#define BIOS_Mouse0Y			0x5FF3
#define BIOS_MaximumX			0x5C47
#define BIOS_MaximumY			0x5C49
#define BIOS_MinimumX			0x5C6D
#define BIOS_MinimumY			0x5C6F
#define BIOS_MouseMovePlus		0x5C71

#define BIOS_CheckMouse			0xF50E

#define EM_CMD_SET_REMOTE_MODE	0xF0
#define EM_CMD_READ_DATA		0xEB
#define EM_CMD_RESPONSE			0xFA

enum EM_STATE
{
	EMS_IDLE = 0,
	EMS_RX_COMMAND,
	EMS_TX_DATA
};

void	EXPAD_BBK_FD1::Reset()
{
	bOut = FALSE;
	ScanNo = 0;

	bMouseActived = FALSE;

	nMouseKey = 0;
	nMouseDeltaX = 0;
	nMouseDeltaY = 0;

	nPrev$4016        = 0;
	nRead$4016Count   = 0;
	nRead$4017Count   = 0;

	nEM84502_State    = 0;
	nEM84502_Command  = 0;
	nEM84502_BitCount = 0;
	nEM84502_TxData   = 0;
	nEM84502_TxCount  = 0;
	nEM84502_Data[0]  = 0;
	nEM84502_Data[1]  = 0;
	nEM84502_Data[2]  = 0;

#ifdef BBK_MOUSE_HACK
	LPBYTE BIOS;

	BIOS = nes->rom->GetPROM();

	// hack CheckMouse
	BIOS[BIOS_CheckMouse + 0x10000] = 0x60; // RTS
#endif
}

void	EXPAD_BBK_FD1::Strobe()
{
}

BYTE	EXPAD_BBK_FD1::Read4016()
{
	BYTE	data = 0;

#ifdef MOUSE_ENABLE
	if (!bMouseActived) {
		nRead$4016Count++;
		if (9 == nRead$4016Count) {
			// start mouse reading
			bMouseActived = TRUE;
			nEM84502_State = EMS_IDLE;
		}
	}
#endif

//	return 0;

	// mouse: RLDUxxxx
//	data = (nMouseStatus & nMouseMask) ? 1 : 0;

//	nMouseMask <<= 1;

	return data;
}

// generate raw wave format
static int MouseGenTxData(BYTE data)
{
	int i;
	int txd;
	int mask;
	int parity;
	
	// start 4 bits
	txd = 0x0B; // 4'b1011

	// 3'b10d x 8
	mask = 0x10;
	parity = 0;
	for (i = 0; i < 8; i++) {
		if (0 == (data & 1)) {
			txd |= mask;
			parity ^= 1;
		}
		
		txd |= mask << 2;
		mask <<= 3;
		data >>= 1;
	}

	// 3'b10p
	if (parity) txd |= mask;
	mask <<= 2;
	txd |= mask;

	// 1'b0

// final format:
// 010p10d10d10d10d10d10d10d10d1011

	return txd;
}

BYTE	EXPAD_BBK_FD1::Read4017()
{
	BYTE	data = 0xFE;

	nRead$4016Count = 0;

#ifndef MOUSE_ENABLE
	data |= 1;
#endif

	if (bMouseActived) {
		// mouse
		switch (nEM84502_State)
		{
			case EMS_IDLE:
				data |= 0x01;
				break;
			case EMS_RX_COMMAND:
				if (0 == (nRead$4017Count & 1))
					data |= 0x01;
				nRead$4017Count++;
				break;
			case EMS_TX_DATA:
				if (nRead$4017Count < 32) {
					if (nEM84502_TxData & (1 << nRead$4017Count))
						data |= 0x01;
					nRead$4017Count++;
				} else {
					nRead$4017Count = 0;
					nEM84502_TxCount--;
					if (nEM84502_TxCount > 0) {
						// load next byte
						BYTE byte;

						// 3 2 1
						byte = nEM84502_Data[3 - nEM84502_TxCount];

						nEM84502_TxData = MouseGenTxData(byte);
					} else {
						bMouseActived = FALSE;
						nEM84502_State = EMS_IDLE;
					}
				}
				break;
		}
	}

	switch ( ScanNo ) {
		case	1:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_4       ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_G       ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_F       ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_C       ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F2       ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_E        ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_5        ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_V        ] ) data &= ~0x10;
			}
			break;
		case	2:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_2        ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_D        ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_S        ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_END      ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F1       ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_W         ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_3         ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_X         ] ) data &= ~0x10;
			}
			break;
		case	3:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_INSERT    ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_BACK      ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_NEXT      ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_RIGHT     ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F8    ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_PRIOR ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_DELETE] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_HOME  ] ) data &= ~0x10;
			}
			break;
		case	4:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_9     ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_I     ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_L     ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_COMMA ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F5     ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_O      ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_0      ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_PERIOD ] ) data &= ~0x10;
			}
			break;
		case	5:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_RBRACKET ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_RETURN   ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_UP       ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_LEFT     ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F7        ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_LBRACKET  ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_BACKSLASH ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_DOWN      ] ) data &= ~0x10;
			}
			break;
		case	6:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_Q       ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_CAPITAL ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_Z       ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_TAB     ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_ESCAPE   ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_A        ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_1        ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_LCONTROL ] ) data &= ~0x10;
			}
			break;
		case	7:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_7 ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_Y ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_K ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_M ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F4 ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_U  ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_8  ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_J  ] ) data &= ~0x10;
			}
			break;
		case	8:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_MINUS      ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_SEMICOLON  ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_APOSTROPHE ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_SLASH      ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F6     ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_P      ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_EQUALS ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_LSHIFT ] ||
					 DirectInput.m_Sw[DIK_RSHIFT ] ) data &= ~0x10;
			}
			break;
		case	9:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_T     ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_H     ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_N     ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_SPACE ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F3 ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_R  ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_6  ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_B  ] ) data &= ~0x10;
			}
			break;
		case	10:
			if ( bOut ) {
			} else {
				data &= ~0x02;
			}
			break;
		case	11:
			if ( bOut ) {
		//		if ( DirectInput.m_Sw[DIK_LMENU   ] ) data &= ~0x02; // null
		//		if ( DirectInput.m_Sw[DIK_NUMPAD4 ] ) data &= ~0x04;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD7 ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_F11     ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F12     ] ) data &= ~0x02;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD1 ] ) data &= ~0x04;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD2 ] ) data &= ~0x08;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD8 ] ) data &= ~0x10;
			}
			break;
		case	12:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_SUBTRACT ] ) data &= ~0x02;
				if ( DirectInput.m_Sw[DIK_ADD      ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_MULTIPLY ] ) data &= ~0x08;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD9  ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F10     ] ) data &= ~0x02;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD5 ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_DIVIDE  ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_NUMLOCK ] ) data &= ~0x10;
			}
			break;
		case	13:
			if ( bOut ) {
				if ( DirectInput.m_Sw[DIK_GRAVE   ] ) data &= ~0x02;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD6 ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_LALT    ] ||
					 DirectInput.m_Sw[DIK_RALT    ] ) data &= ~0x08;
				if ( DirectInput.m_Sw[DIK_TAB     ] ) data &= ~0x10;
			} else {
				if ( DirectInput.m_Sw[DIK_F9      ] ) data &= ~0x02;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD3 ] ) data &= ~0x04;
				if ( DirectInput.m_Sw[DIK_DECIMAL ] ) data &= ~0x08;
		//		if ( DirectInput.m_Sw[DIK_NUMPAD0 ] ) data &= ~0x10;
			}
			break;
	}

	return	data;
//	return	Subor_KeyMap(ScanNo, bOut, data);
}

void	EXPAD_BBK_FD1::SetMouseStatus( INT nX, INT nY, INT nKeys, INT nFlags )
{
	if (nFlags & 0xF0) {
		nMouseDeltaX = nX;
		nMouseDeltaY = nY;
	} else {
		nMouseKey = nKeys;
	}

#ifdef BBK_MOUSE_HACK
	if (nFlags & 0xF0) {
		// Speed up by write coordinate directly.
		INT nMaxX = nes->Read(BIOS_MaximumX);
		INT nMaxY = nes->Read(BIOS_MaximumY);
		INT nMinX = nes->Read(BIOS_MinimumX);
		INT nMinY = nes->Read(BIOS_MinimumY);

//		DEBUGOUT( "MOUSE: R=(%d,%d - %d,%d) X=%d Y=%d\n",
//			nMinX, nMinY, nMaxX, nMaxY,
//			nMouseX, nMouseY);

		// Check Mouse
		if (nMouseX >= nMaxX) nMouseX = nMaxX - 1;
		if (nMouseY >= nMaxY) nMouseY = nMaxY - 1;
		if (nMouseX <= nMinX) nMouseX = nMinX;
		if (nMouseY <= nMinY) nMouseY = nMinY;

		nes->Write(BIOS_Mouse0X, nMouseX);
		nes->Write(BIOS_Mouse0Y, nMouseY);
	}
#endif
}

#define BBK_MS_RIGHT 0x80
#define BBK_MS_LEFT  0x40
#define BBK_MS_DOWN  0x20
#define BBK_MS_UP    0x10
#define BBK_MS_MKEY  0x08
#define BBK_MS_RKEY  0x02
#define BBK_MS_LKEY  0x01

#define BBK_MOUSE_STEP 2

void	EXPAD_BBK_FD1::Write4016( BYTE data )
{
	nRead$4016Count = 0;

	// mouse
	if (bMouseActived) {
		switch (nEM84502_State)
		{
			case EMS_IDLE:
				if (data == 0x05) {
					nRead$4017Count = 0;
					nEM84502_Command = 0;
					nEM84502_BitCount = 0;
					nEM84502_State = EMS_RX_COMMAND;
				}
				break;
			case EMS_RX_COMMAND:
				if (data & 4) {
					nEM84502_Command |= (data & 1) ? 0 : (1 << nEM84502_BitCount);
					nEM84502_BitCount++;

					if (22 == nEM84502_BitCount) {
						// command end
						BYTE cmd;

						cmd = nEM84502_Command & 0xFF;

						nEM84502_TxData = MouseGenTxData(EM_CMD_RESPONSE);

						if (EM_CMD_SET_REMOTE_MODE == cmd) {
							nEM84502_TxCount = 1;
						} else if (EM_CMD_READ_DATA == cmd) {
							BYTE flag;
							int dx, dy;

							// rev X dir
							dx = -nMouseDeltaX;
							dy =  nMouseDeltaY;

							nMouseDeltaX = 0;
							nMouseDeltaY = 0;

							flag = 0;
							if (dx < 0) flag |= 0x20;
							if (dy < 0) flag |= 0x10;

							if (dx > 255 || dx < -256) {
								flag |= 0x80;
								dx = 0;
							}

							if (dy > 255 || dy < -256) {
								flag |= 0x40;
								dy = 0;
							}

							nEM84502_Data[0] = nMouseKey | flag;
							nEM84502_Data[1] = dy;
							nEM84502_Data[2] = dx;
							nEM84502_TxCount = 4;
						} else {
							assert(0);
						}

						nRead$4017Count = 0;
						nEM84502_BitCount = 0;
						nEM84502_State = EMS_TX_DATA;
					}
				}
				break;
			case EMS_TX_DATA:
				break;
		}
	}

	if (data & 4) {
		// keyboard pooling
		if ( data == 0x05 ) {
			bOut = FALSE;
			ScanNo = 0;
		} else if ( data == 0x04 ) {
			if ( ++ScanNo > 13 )
				ScanNo = 0;
			bOut = !bOut;
		} else if ( data == 0x06 ) {
			bOut = !bOut;
		}
	}
}
