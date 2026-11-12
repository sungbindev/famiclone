/*******************************************************************************
 *  Parallel port printer for SB2K
 *
 *  Author:  fanoble <87430545@qq.com>
 *
 *  Create:   June 27, 2024, by fanoble
 *******************************************************************************
 */

// https://en.wikipedia.org/wiki/Parallel_port

// ESC/POS
// https://download4.epson.biz/sec_pubs/pos/reference_en/escpos/commands.html
// https://files.support.epson.com/pdf/general/escp2ref.pdf

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <vector>
#include <winspool.h>
#include <WinUser.h>
#include <string>

#include "lpt.h"
#include "pngwrite.h"

// Common DPI: 72 96 144 200 300 600

// pixels @ 200dpi
// A4:     1660 x 2340
// Letter: 1700 x 2200

// Format   Width x Height (in mm)  Width x Height (in inches)
//   A4       210 x 297               8.3 x 11.7
// Letter   215.9 x 279.4             8.5 x 11

//#define DUMP_SCP_DATA

// 24P printer use 180dpi x 180dpi, round to 200dpi then scaled to 600 DPI
//  9P printer use  60dpi x  72dpi, treat as 200dpi also, then scaled to 600 DPI
#define DEFAULT_LEFT_MARGIN   60
#define DEFAULT_TOP_MARGIN    96
#define DEFAULT_X_SCALE       3
#define DEFAULT_Y_SCALE       3

#define PAPER_A4_200DPI_WIDTH  1660
#define PAPER_A4_200DPI_HEIGHT 2340

#define LPT_TIMEOUT_SYNC       (312 * 50 * 6) // about 6 seconds

//==============================================================================

typedef struct {
	HDC  dcPrinter;
	BOOL bOutputPNGImage;
	CHAR szPrinterName[MAX_PATH];

	int x;
	int y;
	int t;
	int scale_x;
	int scale_y;
	int left_margin;
	int top_margin;
	int mode_24p;

	int graphics_mode;
	int graphics_dots;

	int dpi_x;
	int dpi_y;
	int line_space;

	int page_width;
	int page_height;

	BYTE* bits; // for png
} PRINTER_CONFIG;

static int PrinterStart(PRINTER_CONFIG* pc);
static int PrinterNextPage(PRINTER_CONFIG* pc);
static int PrinterStop(PRINTER_CONFIG* pc);

static int PrinterDrawBits(PRINTER_CONFIG* pc, unsigned char data);

static PRINTER_CONFIG g_PrintConfig;

//==============================================================================

#ifdef DUMP_SCP_DATA
static FILE* fp_scp = 0;
#endif

LPT::LPT()
{
	m_bReset         = 0;
	m_bStrobe        = 0;
	m_bLineFeed      = 0;
	m_bSelectPrinter = 0;
	m_bInitialized   = 0;
	m_nSyncCount     = 0;

	m_nWidth      = 0;
	m_nHeight     = 0;
	m_nEscCmd     = 0;
	m_nDataLength = 0;
	m_nDataState  = LPT_DS_IDLE;

#ifdef DUMP_SCP_DATA
	fp_scp = fopen("DUMP.SCP", "wb");
#endif

	memset(&g_PrintConfig, 0, sizeof(PRINTER_CONFIG));
}

LPT::~LPT()
{
#ifdef DUMP_SCP_DATA
	if (fp_scp) {
		fclose(fp_scp);
		fp_scp = 0;
	}
#endif

	if (m_bInitialized) {
		PrinterStop(&g_PrintConfig);
		m_bInitialized = 0;
	}
}

int LPT::HwReset()
{
	m_nSyncCount = 0;
	if (m_bInitialized) {
		PrinterStop(&g_PrintConfig);
		m_bInitialized = 0;
	}

	return 0;
}

//==============================================================================

typedef struct {
	int m;
	int dpi_x;
	int dpi_y;
	int dots;
} DOT_DENSITY_ENTRY;

static const DOT_DENSITY_ENTRY dot_density_table[] = {
	{ 0,  60,  72,  8},
	{ 1, 120,  72,  8},
	{ 2, 120,  72,  8},
	{ 3, 240,  72,  8},
	{ 4,  80,  72,  8},
	{ 5,  72,  72,  8},
	{ 6,  90,  72,  8},
	{ 7, 144,  72,  8},
	{32,  60, 180, 24},
	{33, 120, 180, 24},
	{38,  90, 180, 24},
	{39, 180, 180, 24},
	{40, 360, 180, 24},
	{ 0,   0,   0,  0}
};

// data
int LPT::Write(unsigned char data)
{
#ifdef DUMP_SCP_DATA
	if (fp_scp)
		fwrite(&data, 1, 1, fp_scp);
#endif

	m_nSyncCount = 0;

	switch (m_nDataState) {
		case LPT_DS_DATA:
			if (m_nDataLength > 0) {
				m_nDataLength--;
				PrinterDrawBits(&g_PrintConfig, data);
				if (0 == m_nDataLength) {
					// line data end
					m_nDataState = LPT_DS_IDLE;
					if (g_PrintConfig.graphics_mode) {
						g_PrintConfig.graphics_mode = 0;
						g_PrintConfig.scale_y = g_PrintConfig.bOutputPNGImage ? 1 : DEFAULT_Y_SCALE;
					} else {
						g_PrintConfig.x = 0;
					}
				}
			}
			break;

		case LPT_DS_IDLE:
			switch (data) {
				case 0x0A:
					if (g_PrintConfig.mode_24p) {
						// n / 180 inch
						g_PrintConfig.y += g_PrintConfig.line_space * g_PrintConfig.dpi_y / 180;
					} else {
						// n / 216 inch
						g_PrintConfig.y += g_PrintConfig.line_space * g_PrintConfig.dpi_y / 216;
					}
					break;
				case 0x0C:
					// FF: "Printing is completed" in Standard mode.
					m_bInitialized = 0;
					PrinterStop(&g_PrintConfig);
					break;
				case 0x0D:
					g_PrintConfig.x = 0;
					break;
				case 0x1B:
					m_nDataState = LPT_DS_ESC;
					break;

				default:
					data = data;
					break;
			}
			break;

		case LPT_DS_ESC:
			m_nEscCmd = data;
			switch (data) {
				case 0x3C:
					// Move the printer head to the extreme left position so the next line will print left to right.
					// This is a nonrecommended command; use the ESC U command instead.
					g_PrintConfig.x = 0;
					m_nDataState = LPT_DS_IDLE;
					break;
				case 0x40:
					if (!m_bInitialized) {
						m_bInitialized = 1;
						m_nDataLength  = 0;
						m_nDataState   = LPT_DS_IDLE;

						memset(&g_PrintConfig, 0, sizeof(PRINTER_CONFIG));

						g_PrintConfig.y = 0;
						g_PrintConfig.line_space = 30; // 1/6 inch

						// start print
						PrinterStart(&g_PrintConfig);
					}

					g_PrintConfig.x = 0;
					break;
				case 0x33:
				case 0x4A:
				case 0x4E:
					m_nDataLength = 1;
					m_nDataState  = LPT_DS_PARAM;
					break;
				case 0x43:
				case 0x4C:
					m_nDataLength = 2;
					m_nDataState  = LPT_DS_PARAM;
					break;
				case 0x2A:
					m_nDataLength = 3;
					m_nDataState  = LPT_DS_PARAM;
					break;

				default:
					data = data;
					break;
			}

			m_pbCmdParam = m_bCmdParams;
			break;

		case LPT_DS_PARAM:
			m_nDataLength--;
			*m_pbCmdParam++ = data;
			if (m_nDataLength <= 0) {
				int i;

				// execute command
				m_nDataState = LPT_DS_IDLE;
				switch (m_nEscCmd) {
					case 0x2A:
						i = 0;
						while (dot_density_table[i].dots) {
							if (dot_density_table[i].m == m_bCmdParams[0])
								break;
							i++;
						}

						if (0 == dot_density_table[i].dots) {
							// default settings
							g_PrintConfig.dpi_x = 180;
							g_PrintConfig.dpi_y = 180;
							g_PrintConfig.mode_24p = 1;
						} else {
							g_PrintConfig.dpi_x = dot_density_table[i].dpi_x;
							g_PrintConfig.dpi_y = dot_density_table[i].dpi_y;
							g_PrintConfig.mode_24p = (dot_density_table[i].dots == 24);
						}

						m_nDataLength  = m_bCmdParams[2] * 256 + m_bCmdParams[1];
						if (g_PrintConfig.mode_24p)
							m_nDataLength *= 3;

						g_PrintConfig.t = 0;
						m_nDataState = LPT_DS_DATA;
						break;
					case 0x33:
						// Sets the line spacing to n/180 or n/216 inch, default is 1/6 inch
						g_PrintConfig.line_space = m_bCmdParams[0];
						break;
					case 0x43:
						// Set the page length to n lines in the current line spacing
						break;
					case 0x4A:
						// advance the vertical print position n/180 inch or n/216 inch
						if (g_PrintConfig.mode_24p)
							g_PrintConfig.y += m_bCmdParams[0] * g_PrintConfig.dpi_y / 180;
						else
							g_PrintConfig.y += m_bCmdParams[0] * g_PrintConfig.dpi_y / 216;
						break;
					case 0x4C:
						// Select 120 dpi graphics mode
						if (g_PrintConfig.mode_24p) {
							// 120 x 60 dpi
							g_PrintConfig.dpi_x = 120;
							g_PrintConfig.dpi_y = 60;
						} else {
							// 120 x 72 dpi
							g_PrintConfig.dpi_x = 120;
							g_PrintConfig.dpi_y = 72;
						}

						g_PrintConfig.scale_y = g_PrintConfig.bOutputPNGImage ? 2 : DEFAULT_Y_SCALE * 2;

						m_nDataLength = m_bCmdParams[1] * 256 + m_bCmdParams[0];

						g_PrintConfig.t = 0;
						g_PrintConfig.graphics_mode = 1;
						g_PrintConfig.graphics_dots = 8;
						m_nDataState = LPT_DS_DATA;
						break;
					case 0x4E:
						// Set bottom margin to n lines
						break;
					default:
						break;
				}
			}
			break;

		default:
			break;
	}

	return 0;
}

//==============================================================================

// control
int LPT::Reset(int s)
{
	if (m_bReset != s) {
		if (0 == m_bReset) {
			OutputDebugString("Reset printer!\n");

			m_nSyncCount = 0;

			if (m_bInitialized) {
				PrinterStop(&g_PrintConfig);
				m_bInitialized = 0;
				m_nDataState = LPT_DS_IDLE;
			}
		}

		m_bReset = s;
	}

	return 0;
}

int LPT::SelectPrinterN(int s) // negative
{
	s = !s;

	if (m_bSelectPrinter != s) {
		m_nSyncCount = 0;

		if (0 == m_bSelectPrinter) {
			OutputDebugString("Select printer.");
		} else {
			OutputDebugString("Deselect printer.");
		}

		m_bSelectPrinter = s;
	}

	return 0;
}

int LPT::LineFeedN(int s) // negative
{
	s = !s;

	if (m_bLineFeed != s) {
		m_nSyncCount = 0;

		m_bLineFeed = s;
	}

	return 0;
}

int LPT::StrobeN(int s) // negative
{
	s = !s;

	if (m_bStrobe != s) {
		m_nSyncCount = 0;

		m_bStrobe = s;
	}

	return 0;
}

//==============================================================================

// status
int LPT::BusyN() // negative
{
	return 1;
}

int LPT::Ack()
{
	return 1;
}

int LPT::PaperOut()
{
	return 0;
}

int LPT::Select()
{
	return 1;
}

int LPT::Error()
{
	return 0;
}

//==============================================================================

int LPT::OnHSync()
{
	m_nSyncCount++;
	if (m_nSyncCount > LPT_TIMEOUT_SYNC) {
		// timeout
		if (m_bInitialized) {
			PrinterStop(&g_PrintConfig);
			m_bInitialized = 0;
		}

		m_nSyncCount = 0;
	}

	return 0;
}

//==============================================================================
//                             Printer API
//==============================================================================

#define TEXT_DLGTITLE		L"Print to..."
#define TEXT_DLGFONT		L"MS Sans Serif"
#define TEXT_DLGDEVICE		L"Device"
#define TEXT_DLGPRINT		L"Print"
#define TEXT_DLGCANCEL		L"Cancel"
#define NUMCHARS(aa)		(sizeof(aa) / sizeof((aa)[0]))

#define IDC_COMBOBOX		1001
#define IDC_STATIC			1002

#pragma pack(push, 4)
typedef struct {
	DWORD  style;
	DWORD  dwExtendedStyle;
	WORD   ccontrols;
	short  x;
	short  y;
	short  cx;
	short  cy;
	WORD   menu;         // name or ordinal of a menu resource
	WORD   windowClass;  // name or ordinal of a window class
	WCHAR  wszTitle[NUMCHARS(TEXT_DLGTITLE)]; // title string of the dialog box
	short  pointsize;       // only if DS_SETFONT flag is set
	WCHAR  wszFont[NUMCHARS(TEXT_DLGFONT)];   // typeface name, if DS_SETFONT is set

	// control info
	//
	struct {
		DWORD  style;
		DWORD  exStyle;
		short  x;
		short  y;
		short  cx;
		short  cy;
		WORD   id;
		WORD   sysClass;       // 0xFFFF identifies a system window class
		WORD   idClass;        // ordinal of a system window class
		WCHAR  wszTitle[NUMCHARS(TEXT_DLGDEVICE)];
		WORD   cbCreationData; // bytes of following creation data
	} list_text;

	struct {
		DWORD  style;
		DWORD  exStyle;
		short  x;
		short  y;
		short  cx;
		short  cy;
		WORD   id;
		WORD   sysClass;       // 0xFFFF identifies a system window class
		WORD   idClass;        // ordinal of a system window class
		WCHAR  wszTitle[1];
		WORD   cbCreationData; // bytes of following creation data
	} list;

	struct {
		DWORD  style;
		DWORD  exStyle;
		short  x;
		short  y;
		short  cx;
		short  cy;
		WORD   id;
		WORD   sysClass;       // 0xFFFF identifies a system window class
		WORD   idClass;        // ordinal of a system window class
		WCHAR  wszTitle[NUMCHARS(TEXT_DLGPRINT)];
		WORD   cbCreationData; // bytes of following creation data
	} print;

	struct {
		DWORD  style;
		DWORD  exStyle;
		short  x;
		short  y;
		short  cx;
		short  cy;
		WORD   id;
		WORD   sysClass;       // 0xFFFF identifies a system window class
		WORD   idClass;        // ordinal of a system window class
		WCHAR  wszTitle[NUMCHARS(TEXT_DLGCANCEL)];
		WORD   cbCreationData; // bytes of following creation data
	} cancel;
} PRINT_DT;

#define SITEM_BUTTON		0xFFFF, 0x0080
#define SITEM_EDIT			0xFFFF, 0x0081
#define SITEM_STATIC		0xFFFF, 0x0082
#define SITEM_LISTBOX		0xFFFF, 0x0083
#define SITEM_SCROLLBAR		0xFFFF, 0x0084
#define SITEM_COMBOBOX		0xFFFF, 0x0085

static PRINT_DT g_PrintDlgTemplate = {
	WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU  // style  0x94c800c4
	| DS_MODALFRAME | DS_3DLOOK
	| DS_SETFONT,
	0x0,                     // exStyle
	4,                       // ccontrols
	500, 300, 300, 120,
	0,                       // menu: none
	0,                       // window class: none
	TEXT_DLGTITLE,           // Window caption
	8,                       // font pointsize
	TEXT_DLGFONT,

	{
		WS_CHILD | WS_VISIBLE,
		0,
		20,22,50,40,
		IDC_STATIC,
		SITEM_STATIC, // Static
		TEXT_DLGDEVICE, 0,
	},

	{
		CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP,
		0,
		60,20,200,144,
		IDC_COMBOBOX,
		SITEM_COMBOBOX, // COMBOBOX
		L"", 0,
	},

	{
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_DEFPUSHBUTTON,
		WS_EX_NOPARENTNOTIFY, // 0x4
		50,80,50,14,
		IDOK,
		SITEM_BUTTON, // button
		TEXT_DLGPRINT, 0,
	},

	{
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		WS_EX_NOPARENTNOTIFY, // 0x4
		200,80,50,14,
		IDCANCEL,
		SITEM_BUTTON, // button
		TEXT_DLGCANCEL, 0,
	}
};

static int InitPrinterList(HWND hwnd, LPARAM lParam)
{
	HWND hCBox;

	hCBox = GetDlgItem(hwnd, IDC_COMBOBOX);

	SetWindowLongA(hwnd, GWL_USERDATA, lParam);

// https://learn.microsoft.com/en-us/windows/win32/controls/combo-boxes

	ComboBox_AddString(hCBox, "PNG image");
	ComboBox_SetCurSel(hCBox, 0);

	DWORD needed, returned;
	EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 2, NULL, 0, &needed, &returned);

	std::vector<BYTE> buffer(needed);
	if (EnumPrinters(PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, NULL, 2, NULL, needed, &needed, &returned)) {
		PRINTER_INFO_2* printerInfo = reinterpret_cast<PRINTER_INFO_2*>(NULL);
		for (DWORD i = 0; i < returned; ++i) {
			ComboBox_AddString(hCBox, printerInfo[i].pPrinterName);
		}
	}

	return 0;
}

static int GetPrinterConfig(HWND hwnd)
{
	HWND hCBox;
	PRINTER_CONFIG* cfg;

	hCBox = GetDlgItem(hwnd, IDC_COMBOBOX);

	cfg = (PRINTER_CONFIG*)GetWindowLongA(hwnd, GWL_USERDATA);

	int nSel = ComboBox_GetCurSel(hCBox);

	if (0 == nSel) {
		cfg->bOutputPNGImage = TRUE;
		cfg->szPrinterName[0] = 0;
	} else {
		cfg->bOutputPNGImage = FALSE;
		ComboBox_GetLBText(hCBox, nSel, cfg->szPrinterName);
	}

	return 0;
}

static INT_PTR CALLBACK PrintDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UINT wId = LOWORD(wParam);

	switch (uMsg) {
		case WM_INITDIALOG:
			InitPrinterList(hwnd, lParam);
			return TRUE;
			break;

		case WM_COMMAND:
			if (wId == IDOK || wId == IDCANCEL) {
				GetPrinterConfig(hwnd);
				EndDialog(hwnd, wId);
			}
			break;

		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;

		default:
			break;
	}

	return FALSE;
}

static int DoPrintDialog(PRINTER_CONFIG* pc)
{
	LRESULT r;
	HINSTANCE hinst = GetModuleHandle(NULL);

	r = DialogBoxIndirectParamW(hinst, (LPCDLGTEMPLATEW)&g_PrintDlgTemplate, 0,
								PrintDlgProc, (LPARAM)pc);

	if (IDOK != r)
		return -1;

	return 0;
}

static int SaveFileDlg(CHAR* szFile, int nMax, int isPDF)
{
	OPENFILENAME ofn;

	memset(&ofn, 0, sizeof(ofn));

	ofn.lpstrTitle      = "Save to...";
	ofn.lStructSize     = sizeof(ofn);
	ofn.hwndOwner       = 0;
	ofn.lpstrFile       = szFile;
	ofn.nMaxFile        = nMax;
	if (isPDF) {
		ofn.lpstrDefExt     = "pdf";
		ofn.lpstrFilter     = "PDF Files(*.pdf)\0*.pdf\0All Files(*.*)\0*.*\0";
	} else {
		// PNG
		ofn.lpstrDefExt     = "png";
		ofn.lpstrFilter     = "PNG Files(*.png)\0*.png\0All Files(*.*)\0*.*\0";
	}
	ofn.nFilterIndex    = 1;
	ofn.Flags           = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir = ".";

	if (!GetSaveFileName(&ofn))
		return -1;

	szFile = szFile;

	return 0;
}

static void SetPaperSize(char* szPrinter, int PaperSize, int dpi)
{
	// 打开打印机
	HANDLE hPrinter;
	PRINTER_DEFAULTS pd = {NULL, NULL, PRINTER_ACCESS_USE};
//	PRINTER_DEFAULTS pd = {NULL, NULL, PRINTER_ALL_ACCESS};
	if (!OpenPrinter(szPrinter, &hPrinter, &pd)) {
		return;
	}

	// 获取打印机信息
	DWORD dwNeeded;
	GetPrinter(hPrinter, 2, NULL, 0, &dwNeeded);
	PRINTER_INFO_2* pPrinterInfo = (PRINTER_INFO_2*)malloc(dwNeeded);
	if (!GetPrinter(hPrinter, 2, (LPBYTE)pPrinterInfo, dwNeeded, &dwNeeded)) {
		ClosePrinter(hPrinter);
		free(pPrinterInfo);
		return;
	}

	// 设置默认纸张大小为A4
	DEVMODE* pDevMode = pPrinterInfo->pDevMode;
	if (pDevMode) {
		pDevMode->dmFields |= DM_PAPERSIZE;
		pDevMode->dmPaperSize = PaperSize;

		pDevMode->dmFields |= DM_PRINTQUALITY;
		pDevMode->dmPrintQuality = dpi;

		SetPrinter(hPrinter, 2, (LPBYTE)pPrinterInfo, 0);
	}

	// 清理
	ClosePrinter(hPrinter);
	free(pPrinterInfo);
}

#if 0
static int GetDPI(char* szPrinter)
{
	// 打开打印机
	HANDLE hPrinter;
	PRINTER_DEFAULTS pd = {NULL, NULL, PRINTER_ACCESS_USE};
	if (!OpenPrinter(szPrinter, &hPrinter, &pd)) {
		return 0;
	}

	// 获取打印机信息
	DWORD dwNeeded;
	GetPrinter(hPrinter, 2, NULL, 0, &dwNeeded);
	PRINTER_INFO_2* pPrinterInfo = (PRINTER_INFO_2*)malloc(dwNeeded);
	if (!GetPrinter(hPrinter, 2, (LPBYTE)pPrinterInfo, dwNeeded, &dwNeeded)) {
		ClosePrinter(hPrinter);
		free(pPrinterInfo);
		return 0;
	}

	int dpi = 0;
	DEVMODE* pDevMode = pPrinterInfo->pDevMode;
	if (pDevMode) {
		dpi = pDevMode->dmPrintQuality;
	}

	// 清理
	ClosePrinter(hPrinter);
	free(pPrinterInfo);

	return dpi;
}
#endif

static int PrinterStartDoc(PRINTER_CONFIG* pc)
{
	CHAR szFile[MAX_PATH * 2];

	pc->dcPrinter = 0;

	szFile[0] = 0;
	// 直接发送到打印机经常卡死
	if (0 == strcmp(pc->szPrinterName, "Microsoft Print to PDF")) {
		int r;
		r = SaveFileDlg(szFile, sizeof(szFile), 1);
		if (r) return r;
	}

	// M$'s driver support 600 dpi only.
	SetPaperSize(pc->szPrinterName, DMPAPER_A4, 600);

#if 0
	int new_dpi = GetDPI(pc->szPrinterName);

	if (new_dpi && new_dpi != 200) {
		pc->scale_x     = DEFAULT_X_SCALE     * new_dpi / 200;
		pc->scale_y     = DEFAULT_Y_SCALE     * new_dpi / 200;
		pc->left_margin = DEFAULT_LEFT_MARGIN * new_dpi / 200;
		pc->top_margin  = DEFAULT_TOP_MARGIN  * new_dpi / 200;
	}
#endif

	pc->page_width  = PAPER_A4_200DPI_WIDTH;
	pc->page_height = PAPER_A4_200DPI_HEIGHT;

	// 创建打印设备上下文
	HDC hdc = CreateDC("WINSPOOL", pc->szPrinterName, NULL, NULL);
	if (!hdc)
		return -1;

#if 0
	int paperWidthMM = GetDeviceCaps(hdc, HORZSIZE);
	int paperHeightMM = GetDeviceCaps(hdc, VERTSIZE);

	// 将毫米转换为英寸
	double paperWidthInches = paperWidthMM / 25.4;
	double paperHeightInches = paperHeightMM / 25.4;

	// 计算像素大小
	width = static_cast<int>(paperWidthInches * dpi);
	height = static_cast<int>(paperHeightInches * dpi);
#endif

	// 设置文档信息
	DOCINFO di;
	memset(&di, 0, sizeof(DOCINFO));
	di.cbSize = sizeof(DOCINFO);
	di.lpszDocName = "Output"; // 文档名称
	di.lpszOutput = szFile[0] ? szFile : NULL;// "printer.out.pdf"; // 输出文件路径，NULL 表示直接发送到打印机
	di.lpszDatatype = NULL; // 数据类型，NULL 表示默认类型

	// 开始文档
	if (StartDoc(hdc, &di) <= 0) {
		CHAR szErr[32];
		DWORD dwErr = GetLastError();
		sprintf(szErr, "StartDoc failed: %d", dwErr);
		MessageBox(0, szErr, "TIPS", MB_OK);
		DeleteDC(hdc);
		return -2;
	}

	// 开始页面
	if (StartPage(hdc) <= 0) {
		EndDoc(hdc);
		DeleteDC(hdc);
		return -3;
	}

	pc->dcPrinter = hdc;

	return 0;
}

static int PrinterStart(PRINTER_CONFIG* pc)
{
	int r;

	pc->left_margin = DEFAULT_LEFT_MARGIN;
	pc->top_margin  = DEFAULT_TOP_MARGIN;

	r = DoPrintDialog(pc);
	if (r) return r;

	if (pc->bOutputPNGImage) {
		pc->scale_x = 1;
		pc->scale_y = 1;

		pc->page_width  = PAPER_A4_200DPI_WIDTH;
		pc->page_height = PAPER_A4_200DPI_HEIGHT;

		// aligned to 4 bytes
		pc->page_width = (pc->page_width + 3) & ~3;

		pc->bits = new BYTE [pc->page_width * pc->page_height];

		// clear to white
		if (pc->bits) memset(pc->bits, 0xFF, pc->page_width * pc->page_height);
	} else {
		pc->scale_x = DEFAULT_X_SCALE;
		pc->scale_y = DEFAULT_Y_SCALE;

		r = PrinterStartDoc(pc);
		if (r) return r;
	}

	return 0;
}

static int PrinterNextPage(PRINTER_CONFIG* pc)
{
	if (pc->bOutputPNGImage) {
	} else {
		HDC hdc = pc->dcPrinter;
		if (!hdc) return -1;

		// 结束页面
		if (EndPage(hdc) <= 0) {
			EndDoc(hdc);
			DeleteDC(hdc);
			pc->dcPrinter = 0;
			return -2;
		}

		// 开始页面
		if (StartPage(hdc) <= 0) {
			EndDoc(hdc);
			DeleteDC(hdc);
			pc->dcPrinter = 0;
			return -3;
		}
	}

	pc->x = 0;
	pc->y = 0;
	pc->t = 0;

	return 0;
}

static int PrinterStop(PRINTER_CONFIG* pc)
{
	if (pc->bOutputPNGImage) {
		if (pc->bits) {
			PNGWRITE png;
			RGBQUAD rgb[256];
			CHAR szFile[MAX_PATH * 2];

			memset(&rgb, 0, sizeof(rgb));

			rgb[255].rgbBlue  = 255;
			rgb[255].rgbGreen = 255;
			rgb[255].rgbRed   = 255;

			int r;

			szFile[0] = 0;
			r = SaveFileDlg(szFile, sizeof(szFile), 0);
			if (0 == r) {
				png.Write(szFile, pc->page_width, pc->page_height,
					rgb, pc->bits, pc->page_width);
			}

			delete pc->bits;
			pc->bits = 0;
		}
	} else {
		HDC hdc = pc->dcPrinter;
		if (!hdc) return -1;

		// 结束文档
		if (EndDoc(hdc) <= 0) {
			DeleteDC(hdc);
			return -2;
		}

		// 删除设备上下文
		DeleteDC(hdc);
	}

	return 0;
}

static int PrinterDrawBits(PRINTER_CONFIG* pc, unsigned char data)
{
	if (pc->bOutputPNGImage) {
		if (!pc->bits)
			return -1;
	} else {
		if (!pc->dcPrinter)
			return -1;
	}

	int mask = 0x80;
	for (int i = 0; i < 8; i++) {
		if (data & mask) {
			int x = pc->x;
			int y = pc->y + + pc->t * 8 + i;

			if (pc->scale_x == 1 && pc->scale_y == 1) {
				if (pc->bOutputPNGImage) {
					int px = pc->left_margin + x;
					int py = pc->top_margin  + y;

					if (px < pc->page_width && py < pc->page_height)
						pc->bits[py * pc->page_width + px] = 0;
				} else {
					SetPixel(pc->dcPrinter, pc->left_margin + x, pc->top_margin + y, 0);
				}
			} else {
				RECT rt;

				rt.left   = (x + pc->left_margin) * pc->scale_x;
				rt.top    = (y + pc->top_margin)  * pc->scale_y;
				rt.right  = rt.left + pc->scale_x;
				rt.bottom = rt.top  + pc->scale_y;

				if (pc->bOutputPNGImage) {
					int h;
					BYTE* pbits;

					pbits = pc->bits + rt.top * pc->page_width + rt.left;

					if (rt.right >= pc->page_width)
						rt.right = pc->page_width;

					if (rt.bottom >= pc->page_height)
						rt.bottom = pc->page_height;

					h = rt.bottom - rt.top;

					while (h-- > 0) {
						int w = rt.right - rt.left;

						while (w-- > 0)
							*pbits++ = 0;

						// next line
						pbits += pc->page_width - pc->scale_x;
					}
				} else {
					HBRUSH br;
					br = (HBRUSH)GetStockObject(BLACK_BRUSH);
					FillRect(pc->dcPrinter, &rt, br);
				}
			}
		}
		mask >>= 1;
	}

	pc->t++;
	if (g_PrintConfig.graphics_mode || !pc->mode_24p) {
		pc->t = 0;
		pc->x++;
	} else if (pc->t == 3) {
		pc->t = 0;
		pc->x++;
	}

	return 0;
}

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
