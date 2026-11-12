//
// パターンビュークラス
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
using namespace std;

#include "VirtuaNESres.h"
#include "DebugOut.h"
#include "App.h"
#include "Pathlib.h"

#include "Wnd.h"
#include "NameTableView.h"
#include "EmuThread.h"

#include "MMU.h"

#include "DirectDraw.h"

WND_MESSAGE_BEGIN(CNameTableView)
WND_ON_MESSAGE( WM_CREATE,	OnCreate )
WND_ON_MESSAGE( WM_CLOSE,	OnClose )
WND_ON_MESSAGE( WM_DESTROY,	OnDestroy )
WND_ON_MESSAGE( WM_MOUSEMOVE, OnMouseMove )
WND_ON_MESSAGE( WM_TIMER,	OnTimer )

WND_COMMAND_BEGIN()
WND_COMMAND_END()
WND_MESSAGE_END()

CNameTableView::CNameTableView()
{
	m_lpPattern = NULL;
	if( !(m_lpPattern = (LPBYTE)::malloc( 512*512 )) ) {
		throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
	}
}

CNameTableView::~CNameTableView()
{
	FREE( m_lpPattern );
}

BOOL	CNameTableView::Create( HWND hWndParent )
{
	HWND hWnd = CreateWindowEx(
			WS_EX_TOOLWINDOW,
			VIRTUANES_WNDCLASS,
			"NameTableView",
			WS_OVERLAPPEDWINDOW,	// Window拡大縮小可能
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
//			hWndParent,
			HWND_DESKTOP,
			NULL,
			CApp::GetInstance(),
			(LPVOID)this		// This を埋め込む為
		);
	if( !hWnd ) {
		DEBUGOUT( "CreateWindow faild.\n" );
		return	FALSE;
	}

	m_hWnd = hWnd;

	ZEROMEMORY( &m_BitmapHdr, sizeof(m_BitmapHdr) );

	m_BitmapHdr.bih.biSize        = sizeof(BITMAPINFOHEADER);
	m_BitmapHdr.bih.biWidth       = 512;
	m_BitmapHdr.bih.biHeight      = -512;
	m_BitmapHdr.bih.biPlanes      = 1;
	m_BitmapHdr.bih.biBitCount    = 8;
	m_BitmapHdr.bih.biCompression = BI_RGB;
	m_BitmapHdr.bih.biClrUsed     = 64;

	DirectDraw.GetPaletteData( m_Palette );

//	::memcpy( &m_BitmapHdr.rgb[0], &m_BitmapHdr.rgb[64], 64*sizeof(RGBQUAD) );
//	m_BitmapHdr.rgb[1].rgbBlue  = 0x60;
//	m_BitmapHdr.rgb[1].rgbGreen = 0x60;
//	m_BitmapHdr.rgb[1].rgbRed   = 0x60;

	return	TRUE;
}

void	CNameTableView::Destroy()
{
	if( m_hWnd && IsWindow(m_hWnd) ) {
		::DestroyWindow( m_hWnd );
		m_hWnd = NULL;
	}
}

WNDMSG	CNameTableView::OnCreate( WNDMSGPARAM )
{
	DEBUGOUT( "CNameTableView::OnCreate\n" );

	RECT	rw, rc;
	::GetWindowRect( m_hWnd, &rw );
	::GetClientRect( m_hWnd, &rc );
	INT	x = rw.right - rw.left - rc.right + 512; 
	INT	y = rw.bottom - rw.top - rc.bottom + 512;
	::MoveWindow( m_hWnd, Config.general.rcNameTableViewPos.left, Config.general.rcNameTableViewPos.top, x, y, FALSE );

	::SetTimer( m_hWnd, 1, 50, NULL );

	::ShowWindow( m_hWnd, SW_SHOW );
	return	TRUE;
}

WNDMSG	CNameTableView::OnClose( WNDMSGPARAM )
{
	DEBUGOUT( "CNameTableView::OnClose\n" );
	::KillTimer( m_hWnd, 1 );
	::DestroyWindow( m_hWnd );
	return	TRUE;
}

WNDMSG	CNameTableView::OnDestroy( WNDMSGPARAM )
{
	DEBUGOUT( "CNameTableView::OnDestroy\n" );
	// 位置保存
	::GetWindowRect( m_hWnd, &Config.general.rcNameTableViewPos );

	m_hWnd = NULL;
	return	TRUE;
}

WNDMSG	CNameTableView::OnMouseMove( WNDMSGPARAM )
{
	RECT rc;
	INT x = LOWORD(lParam);
	INT y = HIWORD(lParam);
	NES* pNes = Emu.GetNES();

	CHAR szTitle[256];

	::GetClientRect( m_hWnd, &rc );

	if (0 == rc.right || 0 == rc.bottom)
		return TRUE;

	// remap to 512x512
	x = x * 512 / rc.right;
	y = y * 512 / rc.bottom;

	if (NES::NESVARIANT_STD == pNes->GetVariant()) {
		::wsprintf( szTitle, "NameTableView x:%d y:%d", x, y );
	} else {
		// 6576
		int page;
		int tile_addr;
		int tile_name;
		int tile_bank;
		int tile_offset;

		page = 0;
		if (x >= 256) page |= 1;
		if (y >= 256) page |= 2;

		x &= 255;
		y &= 255;

		tile_addr = page * 0x800 + (y / 8) * 64 + (x / 8) * 2;
		tile_name = pNes->mapper->PPU_Peek(tile_addr + 1, TRUE) * 256 +
					pNes->mapper->PPU_Peek(tile_addr, TRUE);

		if (pNes->ppu->GetColorMode() == 16) {
			tile_bank   = tile_name >> 14; // D7, D6 only
			tile_offset = (tile_name & 0xFFE) * 16;
		} else {
			// 4 color mode
			tile_bank   = tile_name >> 12;
			tile_offset = (tile_name & 0xFFF) * 16;
		}

		::wsprintf( szTitle, "NameTableView [$%04X]->%04X PAL=%d", tile_addr, tile_offset, tile_bank );
	}

	::SetWindowText( m_hWnd, szTitle );

	return TRUE;
}

WNDMSG CNameTableView::OnTimer( WNDMSGPARAM )
{
	NES* pNes;
	
	if( !Emu.IsRunning() )
		return TRUE;

	pNes = Emu.GetNES();
	if (!pNes) return TRUE;

	if (NES::NESVARIANT_STD == pNes->GetVariant()) {
		// Palette update
		for( INT i = 0; i < 16; i++ ) {
			m_BitmapHdr.rgb[i] = m_Palette[BGPAL[i]];
		}

		// Character data update
		for( INT n = 0; n < 4; n++ ) {
			LPBYTE lpVRAM = PPU_MEM_BANK[8 + n]; // name table
			LPBYTE lpScnv = &m_lpPattern[(n >> 1) * 512 * 240 + (n & 1) * 256];

			for( INT y = 0; y < 30; y++ ) {
				for( INT x = 0; x < 32; x++ ) {
					INT tile = lpVRAM[x + y * 32] * 16 + ((PPUREG[0] & PPU_BGTBL_BIT) << 8);
					BYTE attr = ((lpVRAM[0x03C0 + (x / 4) + (y >> 2) * 8] >> ((x & 2) + (y & 2) * 2)) & 3) << 2;
					LPBYTE lpScn = &lpScnv[x * 8 + y * 8 * 512];
					LPBYTE lpPtn;

					lpPtn = pNes->mapper->GetPatternTable(y * 8, tile);
					if (!lpPtn)
						lpPtn = &PPU_MEM_BANK[tile >> 10][tile & 0x03FF];

					for( INT p = 0; p < 8; p++ ) {
						BYTE chr_l = lpPtn[p];
						BYTE chr_h = lpPtn[p + 8];

						lpScn[0] = ((chr_h >> 6) & 2) | ((chr_l >> 7) & 1) | attr;
						lpScn[4] = ((chr_h >> 2) & 2) | ((chr_l >> 3) & 1) | attr;
						lpScn[1] = ((chr_h >> 5) & 2) | ((chr_l >> 6) & 1) | attr;
						lpScn[5] = ((chr_h >> 1) & 2) | ((chr_l >> 2) & 1) | attr;
						lpScn[2] = ((chr_h >> 4) & 2) | ((chr_l >> 5) & 1) | attr;
						lpScn[6] = ((chr_h >> 0) & 2) | ((chr_l >> 1) & 1) | attr;
						lpScn[3] = ((chr_h >> 3) & 2) | ((chr_l >> 4) & 1) | attr;
						lpScn[7] = ((chr_h << 1) & 2) | ((chr_l >> 0) & 1) | attr;

						// Next line
						lpScn += 512;
					}
				}
			}
		}
	} else {
		int i;
		int page;

		// Palette update
		for (i = 0; i < 64; i++ ) {
			m_BitmapHdr.rgb[i] = m_Palette[pNes->ppu->PaletteLookup(i, TRUE)];
		}

		for (page = 0; page < 4; page++) {
			int tile_base;
			int tile_addr;
			int tile_name;
			int tile_bank;
			int tile_offset;

			LPBYTE lpScnv = &m_lpPattern[(page >> 1) * 512 * 256 + (page & 1) * 256];

			tile_base = page * 0x800; // name table

			if (pNes->ppu->GetColorMode() == 16) {
				// 16 color mode
				for (INT tile_y = 0; tile_y < 32; tile_y++) {
					for (INT tile_x = 0; tile_x < 32; tile_x++) {
						tile_addr = tile_base + tile_y * 32 * 2 + tile_x * 2;
						tile_name = pNes->mapper->PPU_Peek(tile_addr + 1, TRUE) * 256 + pNes->mapper->PPU_Peek(tile_addr, TRUE);

						tile_bank   = (tile_name >> 12) & 0x0C; // D7, D6 only
						tile_offset = (tile_name & 0xFFE) * 16;

						LPBYTE lpScn = lpScnv + tile_y * 8 * 512 + tile_x * 8;

						for (int y = 0; y < 8; y++) {
							unsigned char plane[4];

							plane[0] = pNes->mapper->PPU_Peek(tile_offset +  0 + y, TRUE);
							plane[1] = pNes->mapper->PPU_Peek(tile_offset +  8 + y, TRUE);
							plane[2] = pNes->mapper->PPU_Peek(tile_offset + 16 + y, TRUE);
							plane[3] = pNes->mapper->PPU_Peek(tile_offset + 24 + y, TRUE);

							int mask = 0x80;
							for (int x = 0; x < 8; x++) {
								int index = 0;

								if (plane[0] & mask) index |= 1;
								if (plane[1] & mask) index |= 2;
								if (plane[2] & mask) index |= 4;
								if (plane[3] & mask) index |= 8;

								lpScn[y * 512 + x] = tile_bank * 4 + index;

								mask >>= 1;
							}
						}
					}
				}
			} else {
				// 4 color mode
				for (INT tile_y = 0; tile_y < 32; tile_y++) {
					for (INT tile_x = 0; tile_x < 32; tile_x++) {
						tile_addr = tile_base + tile_y * 32 * 2 + tile_x * 2;
						tile_name = pNes->mapper->PPU_Peek(tile_addr + 1, TRUE) * 256 + pNes->mapper->PPU_Peek(tile_addr, TRUE);

						tile_bank   = tile_name >> 12;
						tile_offset = (tile_name & 0xFFF) * 16;

						LPBYTE lpScn = lpScnv + tile_y * 8 * 512 + tile_x * 8;

						for (int y = 0; y < 8; y++) {
							unsigned char plane[2];

							plane[0] = pNes->mapper->PPU_Peek(tile_offset + 0 + y, TRUE);
							plane[1] = pNes->mapper->PPU_Peek(tile_offset + 8 + y, TRUE);

							int mask = 0x80;
							for (int x = 0; x < 8; x++) {
								int index = 0;

								if (plane[0] & mask) index |= 1;
								if (plane[1] & mask) index |= 2;

								lpScn[y * 512 + x] = tile_bank * 4 + index;

								mask >>= 1;
							}
						}
					}
				}
			}
		}
	}

	RECT rc;
	::GetClientRect( hWnd, &rc );
	HDC hDC = ::GetDC( hWnd );
	::StretchDIBits( hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, 0, 0, 512, 512,
		m_lpPattern, (BITMAPINFO*)&m_BitmapHdr, DIB_RGB_COLORS, SRCCOPY );
	::ReleaseDC( hWnd, hDC );

	return	TRUE;
}

