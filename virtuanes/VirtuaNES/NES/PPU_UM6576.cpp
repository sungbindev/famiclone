/*******************************************************************************
 * PPU for UM6576
 *
 *  Author:  <87430545@qq.com>
 *
 *  Create:   2024-05-17, by fanoble
 *******************************************************************************
 */

#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "VirtuaNESres.h"

#include "typedef.h"
#include "macro.h"

#include "DebugOut.h"
#include "App.h"

#include "nes.h"
#include "mmu.h"
#include "cpu.h"
#include "ppu.h"
#include "ppu_um6576.h"
#include "rom.h"
#include "mapper.h"

PPU_UM6576::PPU_UM6576( NES* parent ) : nes(parent), PPU(parent)
{
	lpScreen    = 0;
	lpScanline  = 0;
	nScanlineNo = 0;
	lpColormode = 0;

	GA = 0;
	m_nVramReg = 0;
	m_bW2005Toggle = FALSE;
	m_bW2006Toggle = FALSE;

	m_bRegPosX = 0;
	m_bRegPosY = 0;

	m_bReg2000 = 0;
	m_bReg2001 = 0;
	m_bReg2002 = 0;
	m_bReg2003 = 0;
	m_bReg2008 = 0;

	// mirror table
	for (int i = 0; i < 256; i++) {
		BYTE	m = 0x80;
		BYTE	c = 0;

		for (int j = 0; j < 8; j++) {
			if (i & (1 << j))
				c |= m;

			m >>= 1;
		}

		Bit2Rev[i] = c;
	}
}

PPU_UM6576::~PPU_UM6576()
{
}

void PPU_UM6576::Reset()
{
	GA = 0;
	m_bW2005Toggle = FALSE;
	m_bW2006Toggle = FALSE;

	m_bRegPosX = 0;
	m_bRegPosY = 0;

	m_bReg2000 = 0;
	m_bReg2001 = 0;
	m_bReg2002 = 0;
	m_bReg2003 = 0;
	m_bReg2008 = 0;

	if (lpScreen)
		::memset(lpScreen, 0x3F, SCREEN_WIDTH * SCREEN_HEIGHT);
}

//==============================================================================

BYTE PPU_UM6576::Read ( WORD addr )
{
	BYTE ret = 0;

	switch (addr) {
		case 0x2002: // Sprite flag
			ret = m_bReg2002;
			m_bReg2002 &= 0x7F; // remove V-Blank flag
			m_bW2005Toggle = 0;
			m_bW2006Toggle = 0;
			break;
		case 0x2004: // R Sprite RAM data port
			ret = SPRAM[m_bReg2003++];
			break;
		case 0x2007: // R Vram data port
			ret = m_nVramReg; // previous loaded

			// load new data to reg
			m_nVramReg = nes->mapper->PPU_Read(GA);

			if (m_bReg2000 & 4)
				GA = GA + 32;
			else
				GA = GA + 1;

			break;
		case 0x2008: // R Color select & pattern address
			ret = m_bReg2008;
			break;

		default:
			if (addr >= 0x2040 && addr < 0x2080) {
				// Color palette table
				ret = m_bPalette[addr & 0x3F];
			}
			break;
	}

	return ret;
}

//==============================================================================

void PPU_UM6576::Write( WORD addr, BYTE data )
{
	switch (addr) {
		case 0x2000: // System control
			m_bReg2000 = data;
			break;
		case 0x2001: // Color & display
			m_bReg2001 = data;
			break;
		case 0x2003: // Setting Sprite RAM address
			m_bReg2003 = data;
			break;
		case 0x2004: // W Sprite data port
			SPRAM[m_bReg2003++] = data;
			break;
		case 0x2005: // Display origin point
			if (m_bW2005Toggle) {
				// Y second
				m_bRegPosY = data;
			} else {
				// X first
				m_bRegPosX = data;
			}
			m_bW2005Toggle = !m_bW2005Toggle;
			break;
		case 0x2006: // Vram address
			if (m_bW2006Toggle) {
				WORD new_ga = GA;
				new_ga &= 0xFF00;
				new_ga |= data;
				GA = new_ga;
			} else {
				// high byte first
				WORD new_ga = GA;
				new_ga &= 0x00FF;
				new_ga |= data << 8;
				GA = new_ga;
			}
			m_bW2006Toggle = !m_bW2006Toggle;
			break;
		case 0x2007: // W Vram data port
			// vram write
			nes->mapper->PPU_Write(GA, data);

			if (m_bReg2000 & 4)
				GA = GA + 32;
			else
				GA = GA + 1;
			break;
		case 0x2008: // W Color select & pattern address
			m_bReg2008 = data;
			break;

		//case 0x4014
		default:
			if (addr >= 0x2040 && addr < 0x2080) {
				// Color palette table
				m_bPalette[addr & 0x3F] = data;
			}
			break;
	}
}

//==============================================================================

void PPU_UM6576::DMA( BYTE data )
{
	WORD addr = data << 8;

	for (INT i = 0; i < 256; i++)
		SPRAM[i] = nes->Read(addr + i);
}

//==============================================================================

void PPU_UM6576::SetScreenPtr(LPBYTE lpScn, LPBYTE lpMode)
{
	lpScreen = lpScn;
	lpColormode = lpMode;
}

LPBYTE PPU_UM6576::GetScreenPtr()
{
	return lpScreen;
}

void PPU_UM6576::SetRenderScanline(INT scanline)
{
	nScanlineNo = scanline;

	if (scanline < 240)
		lpScanline = lpScreen + SCREEN_WIDTH * scanline;
}

INT PPU_UM6576::GetScanlineNo()
{
	return nScanlineNo;
}

//==============================================================================

void PPU_UM6576::VBlankStart()
{
	m_bReg2002 |= 0x80; // PPU_VBLANK_FLAG
}

void PPU_UM6576::VBlankEnd()
{
	m_bReg2002 &= 0x7F; // PPU_VBLANK_FLAG
}

void PPU_UM6576::FrameStart() {}
void PPU_UM6576::FrameEnd() {}

void PPU_UM6576::ScanlineStart() {}
void PPU_UM6576::ScanlineNext() {}

//==============================================================================

// Scanline base render
void PPU_UM6576::Scanline(INT scanline, BOOL bMax, BOOL bLeftClip)
{
	int i;

	// dest is lpScanline, with SCREEN_WIDTH

	// to do:
	// 1. page mode while reg2008.0 = 1
	// 2. sprite mirroring

	// BG
	if (m_bReg2001 & 0x08) { // BG enabled
		int dst_x;
		int tile_base;
		int tile_y = (scanline + m_bRegPosY) / 8;

		tile_base = (m_bReg2000 & 3) * 2048;
		if (tile_y >= 32) { // next page
			tile_y -= 32;

			if (m_bReg2000 & 2)
				tile_base -= 4096; // 2,3 -> 0,1
			else
				tile_base += 4096; // 0,1 -> 2,3
		}

		dst_x = 8;
		for (i = 0; i < 33; i++) { // 32/33 tiles per line
			int y;
			int tile_addr;
			int tile_name;
			int tile_bank;
			int tile_offset;
			int tile_width;

			int tile_x = (m_bRegPosX / 8) + i;

			int x;
			int mask;
			unsigned char plane[4];

			if (tile_x & 0x20) // the next screen
				tile_addr = tile_base + 2048 + tile_y * 32 * 2 + (tile_x & 31) * 2;
			else
				tile_addr = tile_base + tile_y * 32 * 2 + tile_x * 2;

			tile_name = nes->mapper->PPU_Read(tile_addr + 1) * 256 + nes->mapper->PPU_Read(tile_addr);

			mask = i ? 0x80 : (0x80 >> (m_bRegPosX & 7)); // the 1st tile

			if (i == 0)
				tile_width = 8 - (m_bRegPosX & 7); // the 1st tile
			else if (i == 32)
				tile_width = m_bRegPosX & 7; // the last tile
			else
				tile_width = 8;

			y = (scanline + m_bRegPosY) & 7;

			if (m_bReg2008 & 0x80) {
				// 16 color mode
				tile_bank   = (tile_name >> 12) & 0x0C; // D7, D6 only
				tile_offset = (tile_name & 0xFFE) * 16;

				// 8x8 tile
				plane[0] = nes->mapper->PPU_Read(tile_offset +  0 + y);
				plane[1] = nes->mapper->PPU_Read(tile_offset +  8 + y);
				plane[2] = nes->mapper->PPU_Read(tile_offset + 16 + y);
				plane[3] = nes->mapper->PPU_Read(tile_offset + 24 + y);

				for (x = 0; x < tile_width; x++) {
					int index = 0;

					if (plane[0] & mask) index |= 1;
					if (plane[1] & mask) index |= 2;
					if (plane[2] & mask) index |= 4;
					if (plane[3] & mask) index |= 8;

					lpScanline[dst_x++] = m_bPalette[tile_bank * 4 + index];

					mask >>= 1;
				}
			} else {
				// 4 color mode
				tile_bank   = tile_name >> 12;
				tile_offset = (tile_name & 0xFFF) * 16;

				// 8x8 tile
				plane[0] = nes->mapper->PPU_Read(tile_offset + 0 + y);
				plane[1] = nes->mapper->PPU_Read(tile_offset + 8 + y);

				for (x = 0; x < tile_width; x++) {
					int index = 0;

					if (plane[0] & mask) index |= 1;
					if (plane[1] & mask) index |= 2;

					lpScanline[dst_x++] = m_bPalette[tile_bank * 4 + index];

					mask >>= 1;
				}
			}
		}
	} else {
		// BG disabled
		memset(lpScanline, 0, SCREEN_WIDTH);
	}

	m_bReg2002 &= ~0x20;//PPU_SPMAX_FLAG;

	if (scanline > 239)
		return;

	if (0 == (m_bReg2001 & 0x10)) {
		// Sprite disabled
		return;
	}

	// Sprite
	LPSPRITE sp = (LPSPRITE)SPRAM;
	sp += 63; // the last one

	int sp_h = (m_bReg2000 & 0x20) ? 16 : 8;

	for (i = 0; i < 64; i++) {
		// Check if SPRITE is in the scanline
		int sp_y = sp->y + 1;

		if (scanline < sp_y || scanline >= sp_y + sp_h) {
			sp--;
			continue;
		}

		if (sp->attr & 0x20) {
			// BG priority set
			sp--;
			continue;
		}

		// sprite use 4 color only
		int x, y;
		int tile_x;
		int tile_name;
		int tile_base;
		int tile_offset;

		int mask;
		unsigned char plane[2];

		tile_x    = sp->x;
		tile_name = sp->tile;

		// 8x8 tile
		y = scanline - sp_y;

		tile_base = ((m_bReg2008 >> 2) & 3) << 12;
		tile_offset = tile_base + tile_name * 16;
		if (m_bReg2000 & 0x20) {
			// 8x16
			if (y < 8)
				tile_offset -= 16; // previous one
			else
				y -= 8;
		}

		plane[0] = nes->mapper->PPU_Read(tile_offset + 0 + y);
		plane[1] = nes->mapper->PPU_Read(tile_offset + 8 + y);

		mask = 0x80;
		for (x = 0; x < 8; x++) {
			int cr;
			int index = 0;

			if (plane[0] & mask) index |= 1;
			if (plane[1] & mask) index |= 2;

			if (index) {
				index = (sp->attr & 3) * 4 + index;

				cr = m_bPalette[16 + index];
				lpScanline[tile_x + 8 + x] = cr;
			}

			mask >>= 1;
		}

		sp--;
	}
}

void PPU_UM6576::DummyScanline(INT scanline)
{
}

//==============================================================================

WORD PPU_UM6576::GetPPUADDR()	{ return 0; }
WORD PPU_UM6576::GetTILEY()	{ return 0; }

// For mapper
void PPU_UM6576::SetExtLatchMode( BOOL bMode )	{}
void PPU_UM6576::SetChrLatchMode( BOOL bMode )	{}
void PPU_UM6576::SetExtNameTableMode( BOOL bMode ) {}
void PPU_UM6576::SetExtMonoMode( BOOL bMode )	{}
BOOL PPU_UM6576::GetExtMonoMode() { return FALSE; }

void PPU_UM6576::SetVromWrite(BOOL bMode) {}
BOOL PPU_UM6576::IsVromWrite() { return FALSE; }

BOOL PPU_UM6576::IsNMIEnabled()
{
	return m_bReg2000 & 0x80;
}

BOOL PPU_UM6576::IsDispON()	{ return FALSE; }
BOOL PPU_UM6576::IsBGON()	{ return FALSE; }
BOOL PPU_UM6576::IsSPON()	{ return FALSE; }
BYTE PPU_UM6576::GetBGCOLOR()	{ return 0; }
BOOL PPU_UM6576::IsBGCLIP()	{ return FALSE; }
BOOL PPU_UM6576::IsSPCLIP()	{ return FALSE; }
BOOL PPU_UM6576::IsMONOCROME()	{ return FALSE; }
BOOL PPU_UM6576::IsVBLANK()	{ return FALSE; }
BOOL PPU_UM6576::IsSPHIT()	{ return FALSE; }
BOOL PPU_UM6576::IsSPMAX()	{ return FALSE; }
BOOL PPU_UM6576::IsPPUWE()	{ return FALSE; }
BOOL PPU_UM6576::IsSprite0( INT scanline ) { return FALSE; }

// For VS-Unisystem
void PPU_UM6576::SetVSMode( BOOL bMode ) {}
void PPU_UM6576::SetVSSecurity( BYTE data ) {}
void PPU_UM6576::SetVSColorMap( INT nColorMap ) {}
