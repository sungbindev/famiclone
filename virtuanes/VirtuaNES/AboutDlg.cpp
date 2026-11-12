//
// 僶乕僕儑儞僟僀傾儘僌僋儔僗
//
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
#include "AboutDlg.h"

DLG_MESSAGE_BEGIN(CAboutDlg)
DLG_ON_MESSAGE( WM_INITDIALOG,	OnInitDialog )

DLG_COMMAND_BEGIN()
DLG_ON_COMMAND( IDOK, OnOK )
DLG_ON_COMMAND( IDCANCEL, OnCancel )
DLG_COMMAND_END()
DLG_MESSAGE_END()

INT	CAboutDlg::DoModal( HWND hWndParent )
{
	return	::DialogBoxParam( CApp::GetPlugin(), MAKEINTRESOURCE(IDD_VERSION),
				hWndParent, g_DlgProc, (LPARAM)this );
}

// 对话框资源在插件里：“E-mail”标签改成“QQ”，原来的“网址”标签隐藏
static BOOL CALLBACK FixLabelProc( HWND hWnd, LPARAM )
{
	CHAR	szText[32];

	::GetWindowText( hWnd, szText, sizeof(szText) );
	if( ::lstrcmp( szText, "E-mail" ) == 0 )
		::SetWindowText( hWnd, "QQ" );
	else if( ::lstrcmp( szText, "\xCD\xF8\xD6\xB7" ) == 0 )
		::ShowWindow( hWnd, SW_HIDE );

	return	TRUE;
}

DLGMSG	CAboutDlg::OnInitDialog( DLGMSGPARAM )
{
//	DEBUGOUT( "CAboutDlg::OnInitDialog\n" );

	::SendDlgItemMessage( m_hWnd, IDC_VER_ICON, STM_SETICON,
			      (WPARAM)CApp::LoadIcon( IDI_ICON ), 0 );

	TCHAR	str[256];

	::wsprintf( str, " v%s", VIRTUANES_VERSION_TEXT );

	::SetDlgItemText( m_hWnd, IDC_VER_VERSION, str );

	// 原“网址”一行不再使用，QQ 号放在“E-mail”一行，并把它的标签改成“QQ”
	::ShowWindow( ::GetDlgItem( m_hWnd, IDC_VER_WEBSITE ), SW_HIDE );
	::EnumChildWindows( m_hWnd, FixLabelProc, 0 );

	::SetDlgItemText( m_hWnd, IDC_VER_EMAIL, VIRTUANES_QQ );

	return	TRUE;
}

DLGCMD	CAboutDlg::OnOK( DLGCMDPARAM )
{
//	DEBUGOUT( "CAboutDlg::OnOK\n" );

	::EndDialog( m_hWnd, IDOK );
}

DLGCMD	CAboutDlg::OnCancel( DLGCMDPARAM )
{
//	DEBUGOUT( "CAboutDlg::OnCancel\n" );

	::EndDialog( m_hWnd, IDCANCEL );
}
