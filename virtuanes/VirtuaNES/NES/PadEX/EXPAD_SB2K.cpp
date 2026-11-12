//////////////////////////////////////////////////////////////////////////
// SB2K Keyboard & Mouse                                             //
//////////////////////////////////////////////////////////////////////////

// Keyboard
// https://wiki.osdev.org/PS/2_Keyboard
// https://gist.github.com/tracend/912308

// Mouse @ MS+ Mode / 1200bps
// https://pdf.dzsc.com/88889/24770.pdf

#include <assert.h>

#include "DebugServer.h"

#define MCLK_1_MS 1663 // = mclk(1662607) / 1000
#define RESPONSE_DELAY (MCLK_1_MS * 2)

#define KBD_ACK 0xFA

#define KBD_REPORT_INTERVAL   (MCLK_1_MS * 20)
#define MOUSE_REPORT_INTERVAL (MCLK_1_MS * 150)

static const unsigned char mouse_pnp_id[] = {
	0x4D, 0x33,										// Old Mouse ID: Identifies a mouse for old microsoft mode drivers
	0x08,											// Begin Pnp:    "(" indicates PnP IDs will follow
	0x01, 0x24,										// Pnp Rev:      Identifies PnP version 1.0
	0x28, 0x34, 0x2B,								// EISA ID:      "HTK" ( A mouse company )
	0x10, 0x10, 0x10, 0x11,							// Product ID:   "0001" ( Unique product identifier )
	0x3C,											// Extended:     "\"
	0x3C, 0x2D, 0x2F, 0x35, 0x33, 0x25,				// Class Name:   "\MOUSE" fits a defined Windows 95 class name
	0x3C, 0x30, 0x2E, 0x30, 0x10, 0x26, 0x10, 0x23,	// Driver ID:    "\PNP0F0C" fits a defined Windows 95 microsoft mouse compatible ID
	0x19, 0x12,										// Check sum:    Check all characters from Begin PnP to End PnP, exclusive of the checksum characters themselves.
	0x09											// End PnP:      ")" indicates PnP IDs complete
};

void EXPAD_SB2K::Reset()
{
	kbd_tx_data    = 0xFF;
	kbd_rsp_delay  = 0;
	kbd_data_ready = 0;

	kbd_ret_delay  = 0;
	kbd_ret_delay_reload = 0;
	kbd_ret_index  = 0;
	kbd_ret_length = 0;

	kbd_cmd         = 0;
	kbd_rx_option   = 0;

	kbd_led_status    = 0;
	kbd_scan_code_set = 2;
	kbd_report_timer  = 0;

	kbd_initialized   = 0;

	memset(kbd_key_status, 0, 256);
	memset(kbd_key_changed, 0, 256);

	mouse_report_timer  = 0;
	mouse_initialized   = 0;

	mouse_tx_data    = 0;
	mouse_data_ready = 0;
	mouse_ret_delay  = 0;
	mouse_ret_delay_reload = 0;
	mouse_ret_index  = 0;
	mouse_ret_length = 0;
}

void EXPAD_SB2K::Strobe()
{
}

void EXPAD_SB2K::KeyboardProc(INT nCycles)
{
	if (kbd_initialized)
		kbd_report_timer += nCycles;

	// fifo full(1 entry only)
	if (kbd_data_ready)
		return;

	// delay to send response
	if (kbd_rsp_delay) {
		if (kbd_rsp_delay <= nCycles) {
			kbd_rsp_delay  = 0;
			kbd_data_ready = 1;
		} else {
			kbd_rsp_delay -= nCycles;
		}

		// send response first
		return;
	}

	// delay to send return values
	if (kbd_ret_delay) {
		if (kbd_ret_delay <= nCycles) {
			kbd_tx_data    = kbd_ret_data[kbd_ret_index];
			kbd_data_ready = 1;

			kbd_ret_index++;
			if (kbd_ret_index == kbd_ret_length)
				kbd_ret_delay = 0;
			else
				kbd_ret_delay = kbd_ret_delay_reload; // next byte
		} else {
			kbd_ret_delay -= nCycles;
		}

		return;
	}

	// idle mode, check input key
	if (kbd_initialized && kbd_report_timer >= KBD_REPORT_INTERVAL) {
		int i;
		int first_action_key = -1;
		BYTE cur_keys[256];

		// 真实键盘与调试服务注入的按键合并，统一用 0x80 表示按下
		for (i = 0; i < 256; i++)
			cur_keys[i] = (DirectInput.m_Sw[i] || g_dbgKeys[i]) ? 0x80 : 0;

		for (i = 0; i < 256; i++) {
			if (cur_keys[i] ^ kbd_key_status[i]) {

				if (first_action_key < 0)
					first_action_key = i;

				if (cur_keys[i]) {
					// pressed
					kbd_key_changed[i] |= 1;
				} else {
					// released
					kbd_key_changed[i] |= 2;
				}
			}
		}

		memcpy(kbd_key_status, cur_keys, 256);

		if (first_action_key >= 0) {
			if (first_action_key < 128) {
				if (kbd_key_changed[first_action_key] & 1) {
					kbd_ret_data[0] = first_action_key; // pressed
				} else {
					kbd_ret_data[0] = first_action_key | 0x80; // released
				}

				kbd_key_changed[first_action_key] = 0;

				kbd_ret_index   = 0;
				kbd_ret_length  = 1;
				kbd_ret_delay   = 1; // send immediately
			} else {
				// with extended byte (E0)
				kbd_ret_data[0] = 0xE0;

				if (kbd_key_changed[first_action_key] & 1) {
					kbd_ret_data[1] = first_action_key & 0x7F; // pressed
				} else {
					kbd_ret_data[1] = first_action_key; // released
				}

				kbd_key_changed[first_action_key] = 0;

				kbd_ret_index   = 0;
				kbd_ret_length  = 2;
				kbd_ret_delay   = 1; // send immediately
				kbd_ret_delay_reload = MCLK_1_MS;
			}
		}

		kbd_report_timer = 0;
	}
}

void EXPAD_SB2K::MouseProc(INT nCycles)
{
//	if (mouse_initialized)
//		mouse_report_timer += nCycles;

	if (mouse_data_ready) // wait to be read
		return;

	// delay to send return values
	if (mouse_ret_delay) {
		if (mouse_ret_delay <= nCycles) {
			mouse_tx_data    = mouse_ret_data[mouse_ret_index];
			mouse_data_ready = 1;

//			char s[128];
//			sprintf(s, "Mouse TX %02X\n", mouse_tx_data);
//			OutputDebugStringA(s);

			mouse_ret_index++;
			if (mouse_ret_index == mouse_ret_length)
				mouse_ret_delay = 0;
			else
				mouse_ret_delay = mouse_ret_delay_reload; // next byte
		} else {
			mouse_ret_delay -= nCycles;
		}

		return;
	}

	// idle mode, check mouse
	if (mouse_initialized && mouse_report_timer >= MOUSE_REPORT_INTERVAL) {

		int dx, dy, key;

		dx = nMouseDeltaX;
		dy = nMouseDeltaY;
		key = nMouseKey;

		nMouseDeltaX = 0;
		nMouseDeltaY = 0;

		mouse_ret_index   = 0;
		mouse_ret_length  = 3;
		mouse_ret_data[0] = 0x40 | ((key & 1) << 4) | ((key & 4) << 3) | ((dy & 0xC0) >> 4) | ((dx & 0xC0) >> 6);
		mouse_ret_data[1] = dx & 0x3F;
		mouse_ret_data[2] = dy & 0x3F;
		mouse_ret_data[3] = 0x00;
		mouse_ret_delay   = 1;
		mouse_ret_delay_reload = MCLK_1_MS * 5; // 8 ms per byte @ 1200bps

		mouse_report_timer = 0;
	}
}

void EXPAD_SB2K::Clock(INT nCycles)
{
	KeyboardProc(nCycles);
	MouseProc(nCycles);
}

void EXPAD_SB2K::ExWrite(INT nCode, INT nData)
{
	if (0 == nCode) {
		if (kbd_rx_option) {
			// option byte received
			switch (kbd_cmd) {
				case 0xED: // Set/Reset Status indicators
					kbd_led_status = nData;
					break;
				case 0xF0: // Get/set current scan code set
					if (0 == nData) {
						// to do: Get current scan code set
					} else {
						// set new scan code set
						kbd_scan_code_set = nData;
						kbd_initialized   = 1; // start working
					}
					break;
				case 0xF3: // Set typematic rate and delay
					break;
				case 0xFB: // Set specific key to typematic/autorepeat only (scancode set 3 only)
					break;
				case 0xFC: // Set specific key to make/release (scancode set 3 only)
					break;
				case 0xFD: // Set specific key to make only (scancode set 3 only)
					break;
			}

			kbd_rx_option = 0;

			kbd_tx_data   = KBD_ACK;
			kbd_rsp_delay = RESPONSE_DELAY;
		} else {
			// command received
			switch (nData) {
				case 0x01: //
					kbd_tx_data   = KBD_ACK;
					kbd_rsp_delay = RESPONSE_DELAY;
					break;

				case 0x02: //
					kbd_tx_data   = KBD_ACK;
					kbd_rsp_delay = RESPONSE_DELAY;
					break;

				case 0xED: // Set/Reset Status indicators
				case 0xF0: // Select Alternate Scan Codes
				case 0xF3: // Set typematic rate and delay
				case 0xFB: // Set specific key to typematic/autorepeat only (scancode set 3 only)
				case 0xFC: // Set specific key to make/release (scancode set 3 only)
				case 0xFD: // Set specific key to make only (scancode set 3 only)
					kbd_cmd       = nData;
					kbd_rx_option = 1;
					kbd_tx_data   = KBD_ACK;
					kbd_rsp_delay = RESPONSE_DELAY;
					break;

				case 0xF2: // read id
					kbd_tx_data   = KBD_ACK;
					kbd_rsp_delay = RESPONSE_DELAY;

					kbd_ret_index   = 0;
					kbd_ret_length  = 2;
					kbd_ret_data[0] = 0x12;
					kbd_ret_data[1] = 0x34;
					kbd_ret_delay_reload = MCLK_1_MS;
					kbd_ret_delay   = kbd_ret_delay_reload;
					break;

				case 0xFF: // reset
					kbd_tx_data   = KBD_ACK;
					kbd_rsp_delay = RESPONSE_DELAY;

					kbd_ret_index   = 0;
					kbd_ret_length  = 1;
					kbd_ret_data[0] = 0xAA; // self test ok
					kbd_ret_delay_reload = MCLK_1_MS * 100;
					kbd_ret_delay   = kbd_ret_delay_reload;
				break;

				default:
					kbd_tx_data   = KBD_ACK;
					kbd_rsp_delay = RESPONSE_DELAY;
					break;
			}
		}
	} else if (1 == nCode) {
		// Mouse
		if (0 == nData) {
//			char s[128];
//			sprintf(s, "Mouse RTS\n");
//			OutputDebugStringA(s);

			// RTS received
			mouse_ret_index   = 0;
			mouse_ret_length  = sizeof(mouse_pnp_id);
			memcpy(mouse_ret_data, mouse_pnp_id, sizeof(mouse_pnp_id));
			mouse_ret_delay   = MCLK_1_MS * 15; // 14ms - 15ms @ 1200bps
			mouse_ret_delay_reload = MCLK_1_MS * 5; // 8 ms per byte @ 1200bps

			mouse_initialized = 1;
		}
	}
}

INT EXPAD_SB2K::ExRead(INT nCode)
{
	switch (nCode) {
		case 0: // check kbd status
			return kbd_data_ready;

		case 1: // get kbd data
			kbd_data_ready = 0;
			return kbd_tx_data;

		case 2: // check mouse status
			return mouse_data_ready;

		case 3: // get mouse data
//			OutputDebugStringA("CPU RX\n");
			mouse_data_ready = 0;
			return ~mouse_tx_data; // /RXD bit reversed
	}

	return -1;
}

// nKeys:
// R: 01
// M: 02
// L: 04
void EXPAD_SB2K::SetMouseStatus( INT nX, INT nY, INT nKeys, INT nFlags )
{
	if (nFlags & 0xF0) {
		nMouseDeltaX = nX;
		nMouseDeltaY = nY;
	} else {
		nMouseKey = nKeys;
	}

	// report immediately
	mouse_report_timer = MOUSE_REPORT_INTERVAL;

//	char s[64];
//	sprintf(s, "X=%d Y=%d K=%02X\n", RAM[0x354], RAM[0x355], RAM[0x356]);
//	OutputDebugStringA(s);
}

// dummy functions
BYTE EXPAD_SB2K::Read4016()
{
	return 0;
}

BYTE EXPAD_SB2K::Read4017()
{
	int lpt_status = 0;

	if (nes->lpt->Error())
		lpt_status |= 4;

	if (nes->lpt->BusyN())
		lpt_status |= 8;

	return lpt_status;
}

void EXPAD_SB2K::Write4016( BYTE data )
{
	nes->lpt->SelectPrinterN(!!(data & 1));
	nes->lpt->LineFeedN(!!(data & 2));
	nes->lpt->Reset(!!(data & 4));
}
