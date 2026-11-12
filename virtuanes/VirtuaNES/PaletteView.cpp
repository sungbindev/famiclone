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
#include "PaletteView.h"
#include "EmuThread.h"

#include "MMU.h"

#include "DirectDraw.h"

WND_MESSAGE_BEGIN(CPaletteView)
WND_ON_MESSAGE( WM_CREATE,	OnCreate )
WND_ON_MESSAGE( WM_CLOSE,	OnClose )
WND_ON_MESSAGE( WM_DESTROY,	OnDestroy )
WND_ON_MESSAGE( WM_TIMER,	OnTimer )

WND_COMMAND_BEGIN()
WND_COMMAND_END()
WND_MESSAGE_END()

CPaletteView::CPaletteView()
{
	m_lpPattern = NULL;
	if( !(m_lpPattern = (LPBYTE)::malloc( 256*64 )) ) {
		throw	CApp::GetErrorString( IDS_ERROR_OUTOFMEMORY );
	}
}

CPaletteView::~CPaletteView()
{
	FREE( m_lpPattern );
}

BOOL	CPaletteView::Create( HWND hWndParent )
{
	HWND hWnd = CreateWindowEx(
			WS_EX_TOOLWINDOW,
			VIRTUANES_WNDCLASS,
			"PaletteView",
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

	//
	::ZeroMemory( &m_BitmapHdr, sizeof(m_BitmapHdr) );

	m_BitmapHdr.bih.biSize        = sizeof(BITMAPINFOHEADER);
	m_BitmapHdr.bih.biWidth       = 256;
	m_BitmapHdr.bih.biHeight      = -64;
	m_BitmapHdr.bih.biPlanes      = 1;
	m_BitmapHdr.bih.biBitCount    = 8;
	m_BitmapHdr.bih.biCompression = BI_RGB;
	m_BitmapHdr.bih.biClrUsed     = 64;

	DirectDraw.GetPaletteData( m_Palette );

	for( INT y = 0; y < 64; y++ ) {
		for( INT x = 0; x < 256; x++ ) {
			m_lpPattern[x+y*256] = (y>>4)*0x10+(x>>4);
		}
	}

	return	TRUE;
}

void	CPaletteView::Destroy()
{
	if( m_hWnd && IsWindow(m_hWnd) ) {
		::DestroyWindow( m_hWnd );
		m_hWnd = NULL;
	}
}

WNDMSG	CPaletteView::OnCreate( WNDMSGPARAM )
{
	DEBUGOUT( "CPaletteView::OnCreate\n" );

	// 位置修正
	RECT	rw, rc;
	::GetWindowRect( m_hWnd, &rw );
	::GetClientRect( m_hWnd, &rc );
	INT	x = rw.right - rw.left - rc.right + 256;
	INT	y = rw.bottom - rw.top - rc.bottom + 64;
	::MoveWindow( m_hWnd, Config.general.rcPaletteViewPos.left, Config.general.rcPaletteViewPos.top, x, y, FALSE );

	::SetTimer( m_hWnd, 1, 50, NULL );

	::ShowWindow( m_hWnd, SW_SHOW );
	return	TRUE;
}

WNDMSG	CPaletteView::OnClose( WNDMSGPARAM )
{
	DEBUGOUT( "CPaletteView::OnClose\n" );
	::KillTimer( m_hWnd, 1 );
	::DestroyWindow( m_hWnd );
	return	TRUE;
}

WNDMSG	CPaletteView::OnDestroy( WNDMSGPARAM )
{
	DEBUGOUT( "CPaletteView::OnDestroy\n" );
	// 位置保存
	::GetWindowRect( m_hWnd, &Config.general.rcPaletteViewPos );

	m_hWnd = NULL;
	return	TRUE;
}

WNDMSG	CPaletteView::OnTimer( WNDMSGPARAM )
{
	NES* pNes;

	if( !Emu.IsRunning() )
		return	TRUE;

	pNes = Emu.GetNES();
	if (!pNes) return TRUE;

	if (NES::NESVARIANT_STD == pNes->GetVariant()) {
		RGBQUAD crBlack;

		crBlack.rgbRed      = 0;
		crBlack.rgbGreen    = 0;
		crBlack.rgbBlue     = 0;
		crBlack.rgbReserved = 0;

		// パレット
		for( INT i = 0; i < 16; i++ ) {
			m_BitmapHdr.rgb[i]    = m_Palette[BGPAL[i]];
			m_BitmapHdr.rgb[i+16] = m_Palette[SPPAL[i]];
		}

		for(INT i = 32; i < 64; i++ ) {
			m_BitmapHdr.rgb[i] = crBlack;
		}
	} else {
		for( INT i = 0; i < 64; i++ ) {
			m_BitmapHdr.rgb[i] = m_Palette[pNes->ppu->PaletteLookup(i, TRUE)];
		}
	}

	RECT	rc;
	::GetClientRect( hWnd, &rc );
	HDC	hDC = ::GetDC( hWnd );
	::StretchDIBits( hDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, 0, 0, 256, 64, m_lpPattern, (BITMAPINFO*)&m_BitmapHdr, DIB_RGB_COLORS, SRCCOPY );
	::ReleaseDC( hWnd, hDC );

	return	TRUE;
}

