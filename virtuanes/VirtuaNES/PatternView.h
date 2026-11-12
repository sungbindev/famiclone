//
// パターンビュークラス
//
#ifndef	__CPATTERNVIEW_INCLUDED__
#define	__CPATTERNVIEW_INCLUDED__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
using namespace std;

#include "Wnd.h"

class	CPatternView : public CWnd
{
public:
	// Constructor/Destructor
	CPatternView();
	~CPatternView();

	// Override from CWnd
	BOOL	Create( HWND hWndParent );
	void	Destroy();

protected:
	// Message map
	WND_MESSAGE_MAP()
	WNDMSG		OnCreate( WNDMSGPARAM );
	WNDMSG		OnClose( WNDMSGPARAM );
	WNDMSG		OnDestroy( WNDMSGPARAM );
	WNDMSG		OnLButtonDown( WNDMSGPARAM );
	WNDMSG		OnRButtonDown( WNDMSGPARAM );
	WNDMSG		OnMouseMove( WNDMSGPARAM );
	WNDMSG		OnTimer( WNDMSGPARAM );
	//

	void	ModeToString(CHAR* str);

	struct BITMAPHDR {
		BITMAPINFOHEADER bih;
		RGBQUAD		rgb[64];
	} m_BitmapHdr;

	// added by fanoble
	INT m_nPage; // access external video memory, 0 for internal

#define DISP_MODE_8x8_4c   0
#define DISP_MODE_8x16_4c  1
#define DISP_MODE_8x8_16c  2
#define DISP_MODE_MAX      3
	INT m_nDispMode;

	CWnd* m_wndMain;

	INT	m_SelectPal;
	RGBQUAD	m_Palette[256];
	LPBYTE	m_lpBank[8];
	LPBYTE	m_lpPattern;
private:
};

#endif	// !__CPATTERNVIEW_INCLUDED__

