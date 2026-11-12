//
// パターンビュークラス
//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>
using namespace std;

#include "typedef.h"
#include "macro.h"

#include "VirtuaNESres.h"
#include "DebugOut.h"
#include "App.h"
#include "Pathlib.h"

#include "Wnd.h"
#include "MainFrame.h"
#include "PatternView.h"
#include "EmuThread.h"

#include "MMU.h"

#include "DirectDraw.h"

WND_MESSAGE_BEGIN(CPatternView)
WND_ON_MESSAGE( WM_CREATE,	OnCreate )
WND_ON_MESSAGE( WM_CLOSE,	OnClose )
WND_ON_MESSAGE( WM_DESTROY,	OnDestroy )
WND_ON_MESSAGE( WM_LBUTTONDOWN,	OnLButtonDown )
WND_ON_MESSAGE( WM_RBUTTONDOWN,	OnRButtonDown )
WND_ON_MESSAGE( WM_MOUSEMOVE,	OnMouseMove )
WND_ON_MESSAGE( WM_TIMER,	OnTimer )

WND_COMMAND_BEGIN()
WND_COMMAND_END()
WND_MESSAGE_END()

CPatternView::CPatternView()
{
	m_lpPattern = NULL;
	if( !(m_lpPattern = (LPBYTE)::malloc( 512 * 512 )) ) {
		throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
	}

	for( INT i = 0; i < 8; i++ ) {
		m_lpBank[i] = NULL;
	}
}

CPatternView::~CPatternView()
{
	FREE( m_lpPattern );
}

BOOL	CPatternView::Create( HWND hWndParent )
{
	HWND hWnd = CreateWindowEx(
			WS_EX_TOOLWINDOW,
			VIRTUANES_WNDCLASS,
			"PatternView 8x8@4c CHR",
			WS_OVERLAPPEDWINDOW,	// Window拡大縮小可能
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			hWndParent,
			NULL,
			CApp::GetInstance(),
			(LPVOID)this		// This を埋め込む為
		);
	if( !hWnd ) {
		DEBUGOUT( "CreateWindow faild.\n" );
		return	FALSE;
	}

	m_hWnd = hWnd;

	for( INT i = 0; i < 8; i++ ) {
		m_lpBank[i] = NULL;
	}

	//
	ZEROMEMORY( &m_BitmapHdr, sizeof(m_BitmapHdr) );

	m_BitmapHdr.bih.biSize        = sizeof(BITMAPINFOHEADER);
	m_BitmapHdr.bih.biWidth       = 512;
	m_BitmapHdr.bih.biHeight      = -512;
	m_BitmapHdr.bih.biPlanes      = 1;
	m_BitmapHdr.bih.biBitCount    = 8;
	m_BitmapHdr.bih.biCompression = BI_RGB;
	m_BitmapHdr.bih.biClrUsed     = 64;

	DirectDraw.GetPaletteData( m_Palette );

	m_nPage = 0;
	m_nDispMode = DISP_MODE_8x8_4c;

	m_SelectPal = 0;

	return	TRUE;
}

void	CPatternView::Destroy()
{
	if( m_hWnd && IsWindow(m_hWnd) ) {
		::DestroyWindow( m_hWnd );
		m_hWnd = NULL;
	}
}

WNDMSG	CPatternView::OnCreate( WNDMSGPARAM )
{
	DEBUGOUT( "CPatternView::OnCreate\n" );

	// 位置修正
	RECT	rw, rc;
	::GetWindowRect( m_hWnd, &rw );
	::GetClientRect( m_hWnd, &rc );
	INT	x = rw.right - rw.left - rc.right + 512;
	INT	y = rw.bottom - rw.top - rc.bottom + 512;
	::MoveWindow( m_hWnd, Config.general.rcPatternViewPos.left, Config.general.rcPatternViewPos.top, x, y, FALSE );

	::SetTimer( m_hWnd, 1, 50, NULL );

	::ShowWindow( m_hWnd, SW_SHOW );
	return	TRUE;
}

WNDMSG	CPatternView::OnClose( WNDMSGPARAM )
{
	DEBUGOUT( "CPatternView::OnClose\n" );
	::KillTimer( m_hWnd, 1 );
	::DestroyWindow( m_hWnd );
	return	TRUE;
}

WNDMSG	CPatternView::OnDestroy( WNDMSGPARAM )
{
	DEBUGOUT( "CPatternView::OnDestroy\n" );
	// 位置保存
	::GetWindowRect( m_hWnd, &Config.general.rcPatternViewPos );

	m_hWnd = NULL;
	return	TRUE;
}

void	CPatternView::ModeToString(CHAR* str)
{
	NES* pNes = Emu.GetNES();

	LPCSTR sMode = (m_nDispMode == DISP_MODE_8x8_16c) ? "8x8@16c" :
				   (m_nDispMode == DISP_MODE_8x16_4c) ? "8x16@4c" : "8x8@4c";

	CHAR szPage[16];
	if (m_nPage == 0)
		::wsprintf( szPage, "CHR");
	else
		::wsprintf( szPage, "MEM %04X", (m_nPage - 1) * 0x2000);

	if (NES::NESVARIANT_STD == pNes->GetVariant()) {
		if( m_SelectPal < 4 ) {
			::wsprintf( str, "BG%01d %s %s", m_SelectPal & 3, sMode, szPage );
		} else {
			::wsprintf( str, "SP%01d %s %s", m_SelectPal & 3, sMode, szPage );
		}
	} else {
		::wsprintf( str, "PAL%01d %s", m_SelectPal, sMode );
	}
}

WNDMSG	CPatternView::OnLButtonDown( WNDMSGPARAM )
{
	NES* pNes = Emu.GetNES();

	DEBUGOUT( "CPatternView::OnLButtonDown\n" );

	if ( wParam & MK_CONTROL ) {
		if (pNes) {
			m_nPage++;
			if (!pNes->mapper->GetExtVMEM((m_nPage - 1) * 0x2000)) {
				// page not exist
				m_nPage = 0;
			}
		} else {
			m_nPage = 0;
		}
	} else {
		m_SelectPal = (m_SelectPal + 1) & 15;
	}

	CHAR szMode[256];
	CHAR szTitle[256];

	ModeToString(szMode);

	::wsprintf( szTitle, "PatternView %s", szMode );

	::SetWindowText( m_hWnd, szTitle );

	return	TRUE;
}

// added by fanoble
WNDMSG	CPatternView::OnRButtonDown( WNDMSGPARAM )
{
	int max_modes;
	NES* pNes = Emu.GetNES();

	max_modes = (NES::NESVARIANT_STD == pNes->GetVariant()) ? DISP_MODE_8x8_16c : DISP_MODE_MAX;

	m_nDispMode++;
	if (m_nDispMode == max_modes)
		m_nDispMode = DISP_MODE_8x8_4c;

	if (m_nDispMode == DISP_MODE_8x8_16c) {
		// clear unsed part
		memset(m_lpPattern + 512 * 256, 0, 512 * 256);
	}

	CHAR szMode[256];
	CHAR szTitle[256];

	ModeToString(szMode);

	::wsprintf( szTitle, "PatternView %s", szMode );

	::SetWindowText( m_hWnd, szTitle );

	return	TRUE;
}

WNDMSG	CPatternView::OnMouseMove( WNDMSGPARAM )
{
	RECT rc;
	INT x = LOWORD(lParam);
	INT y = HIWORD(lParam);
	NES* pNes = Emu.GetNES();

	CHAR szMode[256];
	CHAR szTitle[256];

	::GetClientRect( m_hWnd, &rc );

	if (0 == rc.right || 0 == rc.bottom)
		return TRUE;

	ModeToString(szMode);

	// remap to 512x512
	x = x * 512 / rc.right;
	y = y * 512 / rc.bottom;

	if (NES::NESVARIANT_STD == pNes->GetVariant()) {
		// scaled 2x
		x /= 2;
		y /= 2;

		::wsprintf( szTitle, "PatternView %s x:%d y:%d", szMode, x, y );
	} else {
		// 6576
		if (m_nDispMode == DISP_MODE_8x8_16c) {
			int addr;
			int tile_x;
			int tile_y;

			tile_x = x / 8;
			tile_y = y / 8;

			addr = (tile_y * 64 + tile_x) * 32;

			if (y >= 256)
				::wsprintf( szTitle, "PatternView %s $----", szMode);
			else
				::wsprintf( szTitle, "PatternView %s $%04X", szMode, addr );
		} else if (m_nDispMode == DISP_MODE_8x8_4c) {
			int addr;
			int tile_x;
			int tile_y;

			tile_x = x / 8;
			tile_y = y / 8;

			addr = (tile_y * 64 + tile_x) * 16;

			::wsprintf( szTitle, "PatternView %s $%04X", szMode, addr );
		} else {
			// DISP_MODE_8x16_4c
			int addr;
			int tile_x;
			int tile_y;

			tile_x = x / 8;
			tile_y = y / 8;

			addr = ((tile_y / 2) * 64 + tile_x) * 32;

			if (tile_y & 1) addr += 16;

			::wsprintf( szTitle, "PatternView %s $%04X", szMode, addr );
		}
	}

	::SetWindowText( m_hWnd, szTitle );

	return TRUE;
}

WNDMSG	CPatternView::OnTimer( WNDMSGPARAM )
{
	NES* pNes;
	
	if( !Emu.IsRunning() )
		return	TRUE;

	pNes = Emu.GetNES();
	if (!pNes) return TRUE;

	if (NES::NESVARIANT_STD == pNes->GetVariant()) {
		// palette
		LPBYTE pPAL = (m_SelectPal < 4) ? &BGPAL[m_SelectPal * 4] : &SPPAL[(m_SelectPal & 3) * 4];

		m_BitmapHdr.rgb[0] = m_Palette[pPAL[0]];
		m_BitmapHdr.rgb[1] = m_Palette[pPAL[1]];
		m_BitmapHdr.rgb[2] = m_Palette[pPAL[2]];
		m_BitmapHdr.rgb[3] = m_Palette[pPAL[3]];

		// Character data update
		for( INT i = 0; i < 8; i++ ) {
			// Character update only for updated banks
			if( m_lpBank[i] != PPU_MEM_BANK[i] ||
//				PPU_MEM_TYPE[i] == BANKTYPE_CRAM ||
				m_nPage > 0) {

				m_lpBank[i] = PPU_MEM_BANK[i];

				LPBYTE	lpPtn = PPU_MEM_BANK[i];

				if (m_nPage > 0) {
					NES* pNes = Emu.GetNES();
					
					if (pNes) {
						LPBYTE pMem = pNes->mapper->GetExtVMEM((m_nPage - 1) * 0x2000);

						if (pMem)
							lpPtn = pMem + i * 0x400;
					}
				}

				for( INT p = 0; p < 64; p++ ) {
					BYTE tx, ty;
					LPBYTE lpScn;

					if (m_nDispMode == DISP_MODE_8x16_4c) { // 16 x 32 tiles
						tx = (p >> 1) & 15;
						ty = i * 4 + (p >> 5) * 2 + (p & 1);
					} else {
						tx = p & 15;
						ty = i * 4 + (p >> 4);
					}

					// scale 2x
					lpScn = m_lpPattern + ty * 512 * 8 * 2 + tx * 8 * 2;

					for( INT y = 0; y < 8; y++ ) {
						BYTE	chr_l = lpPtn[y];
						BYTE	chr_h = lpPtn[y + 8];

						lpScn[0 * 2 + 0] = ((chr_h >> 6) & 2) | ((chr_l >> 7) & 1);
						lpScn[0 * 2 + 1] = lpScn[0 * 2 + 0];
						lpScn[4 * 2 + 0] = ((chr_h >> 2) & 2) | ((chr_l >> 3) & 1);
						lpScn[4 * 2 + 1] = lpScn[4 * 2 + 0];
						lpScn[1 * 2 + 0] = ((chr_h >> 5) & 2) | ((chr_l >> 6) & 1);
						lpScn[1 * 2 + 1] = lpScn[1 * 2 + 0];
						lpScn[5 * 2 + 0] = ((chr_h >> 1) & 2) | ((chr_l >> 2) & 1);
						lpScn[5 * 2 + 1] = lpScn[5 * 2 + 0];
						lpScn[2 * 2 + 0] = ((chr_h >> 4) & 2) | ((chr_l >> 5) & 1);
						lpScn[2 * 2 + 1] = lpScn[2 * 2 + 0];
						lpScn[6 * 2 + 0] = ((chr_h >> 0) & 2) | ((chr_l >> 1) & 1);
						lpScn[6 * 2 + 1] = lpScn[6 * 2 + 0];
						lpScn[3 * 2 + 0] = ((chr_h >> 3) & 2) | ((chr_l >> 4) & 1);
						lpScn[3 * 2 + 1] = lpScn[3 * 2 + 0];
						lpScn[7 * 2 + 0] = ((chr_h << 1) & 2) | ((chr_l >> 0) & 1);
						lpScn[7 * 2 + 1] = lpScn[7 * 2 + 0];

						memcpy(lpScn + 512, lpScn, 8 * 2);

						// Next line
						lpScn += 512 * 2;
					}

					// Next pattern
					lpPtn += 16;
				}
			}
		}
	} else {
		// Palette update
		for (int i = 0; i < 64; i++ ) {
			m_BitmapHdr.rgb[i] = m_Palette[pNes->ppu->PaletteLookup(i, TRUE)];
		}

		// 6576
		// BG: 8x8 | 4/16 color | 16/4 bank pal
		// SP: 8x8/8x16 | 4 color | 4 bank pal

		// actual pattern start from $2000, with total size  $E000:
		//   8x8   4 color 16 3584 tiles
		//   8x16  4 color 32 1792 tiles
		//   8x8  16 color 32 1792 tiles

		// here we simply use full range of $0000-$FFFF
		//   8x8   4 color 16 4096 tiles
		//   8x16  4 color 32 2048 tiles
		//   8x8  16 color 32 2048 tiles
		if (m_nDispMode == DISP_MODE_8x8_16c) {
			for (int tile_y = 0; tile_y < 32; tile_y++) {
				for (int tile_x = 0; tile_x < 64; tile_x++) {
					int src;
					BYTE* dst;

					src = tile_y * 64 * 32 + tile_x * 32;
					dst = m_lpPattern + tile_y * 512 * 8 + tile_x * 8;

					for (int y = 0; y < 8; y++) {
						unsigned char plane[4];

						plane[0] = pNes->mapper->PPU_Peek(src +  0 + y, TRUE);
						plane[1] = pNes->mapper->PPU_Peek(src +  8 + y, TRUE);
						plane[2] = pNes->mapper->PPU_Peek(src + 16 + y, TRUE);
						plane[3] = pNes->mapper->PPU_Peek(src + 24 + y, TRUE);

						int mask = 0x80;
						for (int x = 0; x < 8; x++) {
							int index = 0;

							if (plane[0] & mask) index |= 1;
							if (plane[1] & mask) index |= 2;
							if (plane[2] & mask) index |= 4;
							if (plane[3] & mask) index |= 8;

							dst[y * 512 + x] = (m_SelectPal & 3) * 16 + index;

							mask >>= 1;
						}
					}
				}
			}
		} else {
			for (int tile_y = 0; tile_y < 64; tile_y++) {
				for (int tile_x = 0; tile_x < 64; tile_x++) {
					int src;
					BYTE* dst;

					src = tile_y * 64 * 16 + tile_x * 16;

					if (m_nDispMode == DISP_MODE_8x8_4c) {
						dst = m_lpPattern + tile_y * 512 * 8 + tile_x * 8;
					} else {
						// DISP_MODE_8x16_4c
						// 0 2 4 6 8...62 64..126
						// 1 3 5 7 9...63 65..127
						// 128...
						// 129...
						dst = m_lpPattern + (tile_y / 2) * 1024 * 8 + (tile_x / 2) * 8;
						if (tile_x & 1) dst += 512 * 8;
						if (tile_y & 1) dst += 32 * 8;
					}

					for (int y = 0; y < 8; y++) {
						unsigned char plane[2];

						plane[0] = pNes->mapper->PPU_Peek(src + 0 + y, TRUE);
						plane[1] = pNes->mapper->PPU_Peek(src + 8 + y, TRUE);

						int mask = 0x80;
						for (int x = 0; x < 8; x++) {
							int index = 0;

							if (plane[0] & mask) index |= 1;
							if (plane[1] & mask) index |= 2;

							dst[y * 512 + x] = m_SelectPal * 4 + index;

							mask >>= 1;
						}
					}
				}
			}
		}
	}

	RECT	rc;
	::GetClientRect( hWnd, &rc );
	HDC	hDC = ::GetDC( hWnd );
	::StretchDIBits( hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, 0, 0, 512, 512, m_lpPattern, (BITMAPINFO*)&m_BitmapHdr, DIB_RGB_COLORS, SRCCOPY );
	::ReleaseDC( hWnd, hDC );

	return	TRUE;
}

