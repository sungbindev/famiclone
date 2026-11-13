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
	m_bBus = 0;

	::memset(m_bBgOpaque, 0, sizeof(m_bBgOpaque));
	::memset(m_bSpOccupied, 0, sizeof(m_bSpOccupied));

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
	m_bBus = 0;

	if (lpScreen)
		::memset(lpScreen, 0x3F, SCREEN_WIDTH * SCREEN_HEIGHT);
}

//==============================================================================

BYTE PPU_UM6576::Read ( WORD addr )
{
	BYTE ret = 0;

	switch (addr) {
		case 0x2002: // Sprite flag
			// 真机实测：位 7-5 是标志，低 5 位是最后一次写 PPU 寄存器的值
			ret = (m_bReg2002 & 0xE0) | (m_bBus & 0x1F);
			m_bReg2002 &= 0x7F; // remove V-Blank flag
			m_bW2005Toggle = 0;
			m_bW2006Toggle = 0;
			break;
		case 0x2004: // R Sprite RAM data port
			ret = SPRAM[m_bReg2003];
			// 真机实测：精灵属性字节的位 4-2 读回不固定（见过 E3、F3、FF），这里读回写入的值
			m_bReg2003++;
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
				// Color palette table：真机实测，只有低 6 位有效，位 6 读出为 1
				ret = m_bPalette[addr & 0x3F] | 0x40;
			}
			break;
	}

	return ret;
}

//==============================================================================

void PPU_UM6576::Write( WORD addr, BYTE data )
{
	if (addr <= 0x2007)
		m_bBus = data;

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
				m_bPalette[addr & 0x3F] = data & 0x3F;
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
	// 垂直消隐结束（预渲染行）：清除位 7、6（精灵 0 碰撞）、5（一行超过 8 个精灵）
	m_bReg2002 &= 0x1F;
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
	// 1. sprite limit: draw only the first 8 sprites per line (the overflow flag is already set)

	// 真机实测：$2001 位 5 增强绿、位 6 增强红、位 7 增强蓝（PAL 的顺序），位 0 为黑白；
	// 渲染层的色表按位 0 红、位 1 绿、位 2 蓝排列，位 7 表示黑白
	if (lpColormode && scanline >= 0 && scanline < 240) {
		lpColormode[scanline] = ((m_bReg2001 & 0x40) >> 6) | ((m_bReg2001 & 0x20) >> 4)
			| ((m_bReg2001 & 0x80) >> 5) | ((m_bReg2001 & 0x01) << 7);
	}

	::memset(m_bBgOpaque, 0, sizeof(m_bBgOpaque));

	// BG
	if (m_bReg2001 & 0x08) { // BG enabled
		int dst_x;
		int tile_base;
		int tile_y = (scanline + m_bRegPosY) / 8;

		// 真机实测：$2008 位 0 为 1 时名称表在显存 $2000 起，只有一页（不随 $2000 位 1-0 变化）
		if (m_bReg2008 & 1) {
			tile_base = 0x2000;
			if (tile_y >= 32)
				tile_y -= 32;
		} else {
			tile_base = (m_bReg2000 & 3) * 2048;
			if (tile_y >= 32) { // next page
				tile_y -= 32;

				if (m_bReg2000 & 2)
					tile_base -= 4096; // 2,3 -> 0,1
				else
					tile_base += 4096; // 0,1 -> 2,3
			}
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

			if ((m_bReg2008 & 1) || !(tile_x & 0x20)) // 单页模式或第一屏
				tile_addr = tile_base + tile_y * 32 * 2 + (tile_x & 31) * 2;
			else // the next screen
				tile_addr = tile_base + 2048 + tile_y * 32 * 2 + (tile_x & 31) * 2;

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

					m_bBgOpaque[dst_x] = (index != 0);
					// 真机实测：颜色号为 0 的像素是透明的，显示 $2040 的第 0 项（背景色），不用调色板组自己的第 0 项
					lpScanline[dst_x++] = index ? m_bPalette[tile_bank * 4 + index] : m_bPalette[0];

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

					m_bBgOpaque[dst_x] = (index != 0);
					lpScanline[dst_x++] = index ? m_bPalette[tile_bank * 4 + index] : m_bPalette[0];

					mask >>= 1;
				}
			}
		}
		// 真机实测：$2001 位 1 为 0 时背景最左 8 个像素不显示（画面左边有 8 像素的边）
		if (0 == (m_bReg2001 & 0x02)) {
			for (i = 8; i < 16; i++) {
				lpScanline[i] = m_bPalette[0];
				m_bBgOpaque[i] = 0;
			}
		}
	} else {
		// BG disabled
		memset(lpScanline, 0, SCREEN_WIDTH);
	}

	if (scanline > 239)
		return;

	if (0 == (m_bReg2001 & 0x10)) {
		// Sprite disabled
		return;
	}

	// Sprite：按序号从小到大处理，序号小的优先（同一个像素只由最前面的不透明精灵决定）
	memset(m_bSpOccupied, 0, sizeof(m_bSpOccupied));

	LPSPRITE sp = (LPSPRITE)SPRAM;

	int sp_h = (m_bReg2000 & 0x20) ? 16 : 8;
	int sp_in_line = 0;

	for (i = 0; i < 64; i++, sp++) {
		// Check if SPRITE is in the scanline
		int sp_y = sp->y + 1;

		if (scanline < sp_y || scanline >= sp_y + sp_h)
			continue;

		// 真机实测：同一行超过 8 个精灵时 $2002 位 5 置 1（这里仍然全部绘制）
		if (++sp_in_line > 8)
			m_bReg2002 |= 0x20;

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

		y = scanline - sp_y;

		// 真机实测：属性位 7 为垂直翻转，位 6 为水平镜像，位 5 为 1 时在背景之后
		if (sp->attr & 0x80)
			y = sp_h - 1 - y;

		tile_base = ((m_bReg2008 >> 2) & 3) << 12;
		if (sp_h == 16) {
			// 8x16：图块号的位 0 不用，上半是偶数号，下半是它的下一号（真机实测）
			tile_name &= 0xFE;
			if (y >= 8) {
				tile_name++;
				y -= 8;
			}
		}
		tile_offset = tile_base + tile_name * 16;

		plane[0] = nes->mapper->PPU_Read(tile_offset + 0 + y);
		plane[1] = nes->mapper->PPU_Read(tile_offset + 8 + y);

		for (x = 0; x < 8; x++) {
			int index = 0;
			int sx = tile_x + 8 + x;

			mask = (sp->attr & 0x40) ? (0x01 << x) : (0x80 >> x);

			if (plane[0] & mask) index |= 1;
			if (plane[1] & mask) index |= 2;

			if (0 == index)
				continue;

			// 真机实测：$2001 位 2 为 0 时精灵最左 8 个像素不显示
			if (tile_x + x < 8 && 0 == (m_bReg2001 & 0x04))
				continue;

			if (m_bSpOccupied[sx])
				continue;
			m_bSpOccupied[sx] = 1;

			// 精灵 0 与不透明的背景像素重合：$2002 位 6（最右一列不算）
			if (0 == i && m_bBgOpaque[sx] && tile_x + x != 255)
				m_bReg2002 |= 0x40;

			// 在背景之后的精灵：背景不透明处被盖住（仍然挡住序号更大的精灵）
			if ((sp->attr & 0x20) && m_bBgOpaque[sx])
				continue;

			lpScanline[sx] = m_bPalette[16 + (sp->attr & 3) * 4 + index];
		}
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
