/*
 * ------------------------------------------------------------------------
 * 名称：DebugServer.cpp
 * 说明：调试服务。经本机 TCP（仅回环地址）接收文本命令，驱动模拟器运行、
 *       设置断点、读写内存、注入键盘鼠标、截图、记录指令跟踪与代码数据标记，
 *       供外部反汇编与分析工具自动化使用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-09
 * 备注：协议为一行一个命令，应答若干行文本，以单独的“.”行结束；错误应答以“ERR ”开头。
 *       只有设置环境变量 FCDEV_DEBUG_PORT 才启用；启用后模拟线程只在命令授权时推进帧。
 *       命令只在模拟线程空闲（没有正在执行的帧）时执行，因此访问模拟器状态无需加锁。
 * ------------------------------------------------------------------------
 */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>
using namespace std;

#pragma comment(lib, "ws2_32.lib")

#include "VirtuaNESres.h"
#include "typedef.h"
#include "macro.h"

#include "App.h"
#include "Config.h"
#include "EmuThread.h"
#include "DirectDraw.h"
#include "Pngwrite.h"
#include "mmu.h"
#include "fdc.h"
#include "MapperSB2K.h"

#include "DebugServer.h"

volatile LONG g_dbgMode = 0;
volatile LONG g_dbgActive = 0;
BYTE g_dbgKeys[256];
BYTE g_dbgPad[4];

namespace {

// 代码数据标记的位含义
const BYTE CDL_OPCODE = 0x01;  // 作为指令第一个字节被执行
const BYTE CDL_OPERAND = 0x02; // 作为指令操作数被执行
const BYTE CDL_READ = 0x04;    // 被当作数据读取
const BYTE CDL_WRITE = 0x08;   // 被写入
// 高 4 位：bit4-6 为最近一次访问时所在的 4K 窗口号（0 对应 $8000），bit7 表示曾在不同窗口访问过
const BYTE CDL_WINDOW_MASK = 0x70;
const BYTE CDL_CONFLICT = 0x80;

const int CDL_SPACE_SIZE = 512 * 1024;

// 断点：type 为 'x' 执行、'r' 读、'w' 写
struct Breakpoint {
	int id;
	char type;
	WORD lo;
	WORD hi;
};

// ---- 帧门控 ----
volatile LONG s_pending = 0;   // 授权运行的帧数
volatile LONG s_busy = 0;      // 模拟线程正在处理一帧
volatile LONG s_frameNo = 0;   // 已完成的总帧数
HANDLE s_evFrame = NULL;       // 每帧结束时置位

// ---- 停止与步进 ----
volatile LONG s_stop = 0;      // 非 0 表示已请求停止
char s_stopReason[96] = "";
WORD s_stopPc = 0;
bool s_skipFirst = false;      // 恢复运行后第一条指令不检查执行断点
bool s_stepMode = false;
LONG s_stepLeft = 0;

// ---- 当前指令信息 ----
WORD s_curPc = 0;
int s_curLen = 1;
unsigned __int64 s_instrCount = 0;

// 暂存一次读取：下一条指令的取指也经过读取钩子，要等 OnInstr 才能区分
struct PendingRead {
	bool valid;
	WORD addr;
	BYTE data;
	WORD pc;
	int len;
};
PendingRead s_pend = { false, 0, 0, 0, 0 };

vector<Breakpoint> s_bps;
int s_nextBpId = 1;

// ---- 跟踪与日志 ----
FILE* s_trace = NULL;
unsigned __int64 s_traceLeft = 0;
bool s_traceUnique = false; // 只记每个指令地址、每个中断入口的第一次
BYTE s_traceSeen[65536];
vector<unsigned> s_traceSeenInt;

// unique 模式下判断这个指令地址是否已经记录过，没记录过就记下
bool TraceAlreadySeen(WORD pc)
{
	if (!s_traceUnique)
		return false;
	if (s_traceSeen[pc])
		return true;
	s_traceSeen[pc] = 1;
	return false;
}

// unique 模式下判断这个中断入口是否已经记录过，没记录过就记下
bool TraceInterruptSeen(int type, WORD target)
{
	unsigned key = ((unsigned)type << 16) | target;

	if (!s_traceUnique)
		return false;
	for (size_t i = 0; i < s_traceSeenInt.size(); i++)
		if (s_traceSeenInt[i] == key)
			return true;
	s_traceSeenInt.push_back(key);
	return false;
}

FILE* s_io = NULL;
unsigned __int64 s_ioLeft = 0;
bool s_ioWritesOnly = false; // 轮询状态寄存器的读取很多，只看写入时不记录读取

// ---- 代码数据标记 ----
bool s_cdlOn = false;
BYTE* s_cdl[2] = { NULL, NULL };

// ---- 按 PC 记录调用：每次执行到指定地址就记录寄存器与若干段内存 ----
struct MemRange {
	WORD addr;
	WORD len;
};
struct CallLog {
	FILE* fp;
	WORD pc;
	unsigned __int64 left;
	vector<MemRange> ranges;
	vector<MemRange> skip; // addr 处的字节等于 len（此处借用为值）时不记录
};
vector<CallLog> s_calls;

// ---- 软驱扇区访问日志 ----
FILE* s_disk = NULL;
unsigned __int64 s_diskLeft = 0;

SOCKET s_listen = INVALID_SOCKET;

// 刷新总开关：任一功能有效时 CPU 核心才会调用钩子
void UpdateActive()
{
	g_dbgActive = (g_dbgMode && (!s_bps.empty() || s_trace || s_io || s_cdlOn || s_stepMode || !s_calls.empty() || s_disk)) ? 1 : 0;
}

NES* GetNes()
{
	return Emu.IsRunning() ? Emu.GetNES() : NULL;
}

MapperSB2K* GetSb2k(NES* nes)
{
	return nes ? dynamic_cast<MapperSB2K*>(nes->mapper) : NULL;
}

// 指令总字节数，由寻址方式推出，覆盖官方与常见非官方指令
int OpLen(BYTE op)
{
	int cc = op & 3;
	int bbb = (op >> 2) & 7;

	switch (op) {
	case 0x00: return 2; // BRK 带一个填充字节
	case 0x20: return 3; // JSR
	case 0x40: case 0x60: return 1;
	case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
	case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
		return 1; // JAM
	}

	if (cc == 1 || cc == 3) {
		static const int len01[8] = { 2, 2, 2, 3, 2, 2, 3, 3 };
		return len01[bbb];
	}
	if (cc == 2) {
		static const int len10[8] = { 2, 2, 1, 3, 1, 2, 1, 3 };
		return len10[bbb];
	}

	static const int len00[8] = { 2, 2, 1, 3, 2, 2, 1, 3 };
	return len00[bbb];
}

// 读取 CPU 可见字节，不触发外设副作用；外设区返回 0xFF
BYTE PeekCpu(NES* nes, WORD a)
{
	if (a < 0x2000)
		return RAM[a & 0x07FF];
	if (a >= 0x8000) {
		BYTE d = 0xFF;
		nes->mapper->ReadHigh(a, &d);
		return d;
	}
	if (a >= 0x6000)
		return ERAM[a & 0x1FFF];
	// 6576 模式下 $4400-$5FFF 不是 RAM，真机读出都是 $68
	if (a >= 0x4400 && a < 0x6000) {
		MapperSB2K* m = GetSb2k(nes);

		if (m && m->IsMode6576())
			return 0x68;
	}
	if (a >= 0x5000)
		return WRAM[a & 0x07FF];
	return 0xFF;
}

void MarkCdl(NES* nes, WORD addr, BYTE flag)
{
	MapperSB2K* m = GetSb2k(nes);
	INT off;
	INT space;

	if (!m || !s_cdl[0])
		return;

	space = m->DbgResolve(addr, off);
	if (space >= 0 && off < CDL_SPACE_SIZE) {
		BYTE cur = s_cdl[space][off];
		BYTE window = (BYTE)(((addr >> 12) & 7) << 4);

		if ((cur & 0x0F) == 0)
			cur = (BYTE)((cur & ~CDL_WINDOW_MASK) | window);
		else if ((cur & CDL_WINDOW_MASK) != window)
			cur |= CDL_CONFLICT;

		s_cdl[space][off] = (BYTE)(cur | flag);
	}
}

void RequestStop(const char* reason)
{
	if (!s_stop) {
		_snprintf(s_stopReason, sizeof(s_stopReason), "%s", reason);
		s_stopReason[sizeof(s_stopReason) - 1] = 0;
		s_stopPc = s_curPc;
		s_stop = 1;
	}
}

bool InRange(const Breakpoint& b, WORD addr)
{
	return addr >= b.lo && addr <= b.hi;
}

// 处理一次数据读取：断点、日志与标记
void ProcessRead(WORD addr, BYTE data, WORD pc, int len)
{
	NES* nes = Emu.GetNES();
	char buf[96];

	// 属于当前指令自身字节的读取是取操作数，不算数据读取
	if ((WORD)(addr - pc) < (WORD)len)
		return;

	for (size_t i = 0; i < s_bps.size(); i++) {
		if (s_bps[i].type == 'r' && InRange(s_bps[i], addr)) {
			_snprintf(buf, sizeof(buf), "read bp #%d addr=%04X", s_bps[i].id, addr);
			RequestStop(buf);
		}
	}

	if (s_io && s_ioLeft && !s_ioWritesOnly && addr >= 0x2000 && addr < 0x5000) {
		fprintf(s_io, "R %04X %02X pc=%04X\n", addr, data, pc);
		if (--s_ioLeft == 0) { fclose(s_io); s_io = NULL; }
	}

	if (s_cdlOn && addr >= 0x8000)
		MarkCdl(nes, addr, CDL_READ);
}

void FlushPending()
{
	if (s_pend.valid) {
		s_pend.valid = false;
		ProcessRead(s_pend.addr, s_pend.data, s_pend.pc, s_pend.len);
	}
}

// ---- 数字与文本解析 ----
unsigned long ParseHex(const char* s)
{
	if (s[0] == '$')
		s++;
	return strtoul(s, NULL, (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) ? 0 : 16);
}

unsigned long ParseDec(const char* s)
{
	return strtoul(s, NULL, 0);
}

vector<string> Split(const string& line)
{
	vector<string> t;
	size_t i = 0;

	while (i < line.size()) {
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
		size_t j = i;
		while (j < line.size() && line[j] != ' ' && line[j] != '\t') j++;
		if (j > i)
			t.push_back(line.substr(i, j - i));
		i = j;
	}
	return t;
}

// 在命令里路径可能含空格，取第 n 个词之后的整段文本
string RestOf(const string& line, size_t n)
{
	size_t i = 0;

	for (size_t k = 0; k < n; k++) {
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
		while (i < line.size() && line[i] != ' ' && line[i] != '\t') i++;
	}
	while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) i++;
	return line.substr(i);
}

string Fmt(const char* fmt, ...)
{
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf) - 1] = 0;
	return buf;
}

string RegsLine(NES* nes)
{
	R6502 r;

	nes->cpu->GetContext(r);
	return Fmt("pc=%04X a=%02X x=%02X y=%02X s=%02X p=%02X instr=%I64u frame=%ld",
		r.PC, r.A, r.X, r.Y, r.S, r.P, s_instrCount, (long)s_frameNo);
}

// ---- 运行控制 ----

// 授权运行 n 帧，遇到停止请求提前结束；返回实际完成的帧数
long RunFrames(long n, DWORD timeoutMs)
{
	long startNo = s_frameNo;
	DWORD t0 = GetTickCount();

	s_skipFirst = true;
	s_stop = 0;
	s_stopReason[0] = 0;
	UpdateActive();

	s_pending = n;
	MemoryBarrier();

	while (s_frameNo - startNo < n && !s_stop) {
		WaitForSingleObject(s_evFrame, 20);
		if (GetTickCount() - t0 > timeoutMs)
			break;
	}

	// 取消剩余授权并等待正在执行的帧结束
	s_pending = 0;
	MemoryBarrier();
	while (s_busy)
		WaitForSingleObject(s_evFrame, 5);

	return s_frameNo - startNo;
}

// ---- 对话框自动关闭 ----
// 调试时模拟器弹出的消息框、文件框会让线程卡在模态循环里，监视线程发现后自动按下按钮并记录文字，用 dialogs 命令查看。

CRITICAL_SECTION s_dlgLock;
vector<string> s_dlgLog;

// 系统代码页文本转 UTF-8，调试客户端按 UTF-8 解码应答
string AcpToUtf8(const string& acp)
{
	int wn = MultiByteToWideChar(CP_ACP, 0, acp.c_str(), -1, NULL, 0);
	if (wn <= 0)
		return acp;

	vector<wchar_t> wide(wn);
	MultiByteToWideChar(CP_ACP, 0, acp.c_str(), -1, &wide[0], wn);

	int un = WideCharToMultiByte(CP_UTF8, 0, &wide[0], -1, NULL, 0, NULL, NULL);
	if (un <= 0)
		return acp;

	vector<char> utf8(un);
	WideCharToMultiByte(CP_UTF8, 0, &wide[0], -1, &utf8[0], un, NULL, NULL);
	return string(&utf8[0]);
}

struct DialogScan {
	DWORD pid;
	vector<HWND> found;
};

BOOL CALLBACK EnumDialogProc(HWND hwnd, LPARAM lp)
{
	DialogScan* scan = (DialogScan*)lp;
	DWORD pid = 0;
	char cls[32];

	GetWindowThreadProcessId(hwnd, &pid);
	if (pid != scan->pid || !IsWindowVisible(hwnd))
		return TRUE;

	GetClassNameA(hwnd, cls, sizeof(cls));
	if (strcmp(cls, "#32770") == 0)
		scan->found.push_back(hwnd);
	return TRUE;
}

// 读取窗口文字，对方线程无响应时最多等 200 毫秒
string WindowTextOf(HWND hwnd)
{
	char buf[512];
	DWORD_PTR len = 0;

	buf[0] = 0;
	if (!SendMessageTimeoutA(hwnd, WM_GETTEXT, sizeof(buf), (LPARAM)buf, SMTO_ABORTIFHUNG, 200, &len))
		return "";
	buf[sizeof(buf) - 1] = 0;
	return buf;
}

BOOL CALLBACK EnumDialogChildProc(HWND hwnd, LPARAM lp)
{
	string* text = (string*)lp;
	char cls[32];

	GetClassNameA(hwnd, cls, sizeof(cls));
	if (_stricmp(cls, "Static") == 0) {
		string t = WindowTextOf(hwnd);
		if (!t.empty()) {
			if (!text->empty())
				*text += " / ";
			*text += t;
		}
	}
	return TRUE;
}

// 文件对话框（打开、另存为）里有 DirectUI 或外壳视图控件
BOOL CALLBACK EnumFileDialogChildProc(HWND hwnd, LPARAM lp)
{
	char cls[64];

	GetClassNameA(hwnd, cls, sizeof(cls));
	if (strcmp(cls, "DUIViewWndClassName") == 0 || strcmp(cls, "SHELLDLL_DefView") == 0) {
		*(bool*)lp = true;
		return FALSE;
	}
	return TRUE;
}

// 按下对话框里最“保守”的按钮：确定、取消、否、忽略、关闭，都没有就发关闭消息；返回按下的按钮编号，0 为关闭消息
// 文件对话框的“确定”在没有文件名时不起作用，会一直卡住，所以直接按取消
int PressDialogButton(HWND dlg)
{
	static const int ids[] = { IDOK, IDCANCEL, IDNO, IDIGNORE, IDCLOSE };
	bool isFileDialog = false;

	EnumChildWindows(dlg, EnumFileDialogChildProc, (LPARAM)&isFileDialog);
	if (isFileDialog) {
		HWND cancel = GetDlgItem(dlg, IDCANCEL);
		if (cancel) {
			PostMessage(dlg, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), (LPARAM)cancel);
			return IDCANCEL;
		}
	}

	for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
		HWND btn = GetDlgItem(dlg, ids[i]);
		if (btn) {
			PostMessage(dlg, WM_COMMAND, MAKEWPARAM(ids[i], BN_CLICKED), (LPARAM)btn);
			return ids[i];
		}
	}
	PostMessage(dlg, WM_CLOSE, 0, 0);
	return 0;
}

DWORD WINAPI DialogWatchThread(LPVOID)
{
	// 刚处理过的对话框 100 毫秒后可能还没销毁，记下句柄与时刻避免重复处理
	vector<pair<HWND, DWORD> > handled;

	for (;;) {
		Sleep(100);

		DialogScan scan;
		scan.pid = GetCurrentProcessId();
		EnumWindows(EnumDialogProc, (LPARAM)&scan);

		DWORD now = GetTickCount();
		for (size_t i = 0; i < handled.size();) {
			if (now - handled[i].second > 2000)
				handled.erase(handled.begin() + i);
			else
				i++;
		}

		for (size_t i = 0; i < scan.found.size(); i++) {
			HWND dlg = scan.found[i];
			bool seen = false;

			for (size_t k = 0; k < handled.size(); k++)
				if (handled[k].first == dlg)
					seen = true;
			if (seen)
				continue;

			string text;
			EnumChildWindows(dlg, EnumDialogChildProc, (LPARAM)&text);
			string title = WindowTextOf(dlg);
			int pressed = PressDialogButton(dlg);

			EnterCriticalSection(&s_dlgLock);
			if (s_dlgLog.size() < 200)
				s_dlgLog.push_back(AcpToUtf8(Fmt("title=%s | text=%s | button=%d", title.c_str(), text.c_str(), pressed)));
			LeaveCriticalSection(&s_dlgLock);

			handled.push_back(make_pair(dlg, now));
		}
	}
	return 0;
}

// dialogs [clear]：列出自动关闭过的对话框，clear 时同时清空记录
string CmdDialogs(const vector<string>& t)
{
	string out;

	EnterCriticalSection(&s_dlgLock);
	out = Fmt("OK count=%u\n", (unsigned)s_dlgLog.size());
	for (size_t i = 0; i < s_dlgLog.size(); i++)
		out += s_dlgLog[i] + "\n";
	if (t.size() >= 2 && t[1] == "clear")
		s_dlgLog.clear();
	LeaveCriticalSection(&s_dlgLock);
	return out;
}

// ---- 命令处理 ----

string CmdStatus()
{
	NES* nes = GetNes();

	if (!nes)
		return "OK running=0\n";

	char info[512] = "";
	MapperSB2K* m = GetSb2k(nes);
	if (m)
		m->DbgInfo(info, sizeof(info));

	return Fmt("OK running=1 rom=%s variant=%d mapper=%d %s\n%s\n",
		nes->rom->GetRomName(), (int)nes->GetVariant(), nes->rom->GetMapperNo(),
		RegsLine(nes).c_str(), info);
}

string CmdRun(NES* nes, const vector<string>& t)
{
	long n = t.size() > 1 ? (long)ParseDec(t[1].c_str()) : 1;
	long done;

	s_stepMode = false;
	done = RunFrames(n, 600000);

	return Fmt("OK frames=%ld stop=%s stoppc=%04X %s\n", done,
		s_stop ? s_stopReason : "none", s_stopPc, RegsLine(nes).c_str());
}

string CmdStep(NES* nes, const vector<string>& t)
{
	long n = t.size() > 1 ? (long)ParseDec(t[1].c_str()) : 1;
	long done;

	// 步进计数在指令钩子里递减，跨帧继续
	s_stepMode = true;
	s_stepLeft = n;
	done = RunFrames(100000, 600000);
	s_stepMode = false;
	UpdateActive();

	return Fmt("OK frames=%ld stop=%s stoppc=%04X %s\n", done,
		s_stop ? s_stopReason : "none", s_stopPc, RegsLine(nes).c_str());
}

string CmdBp(const vector<string>& t)
{
	if (t.size() >= 2 && t[1] == "list") {
		string r = "OK\n";
		for (size_t i = 0; i < s_bps.size(); i++)
			r += Fmt("bp %d %c %04X %04X\n", s_bps[i].id, s_bps[i].type, s_bps[i].lo, s_bps[i].hi);
		return r;
	}
	if (t.size() >= 2 && t[1] == "del") {
		if (t.size() >= 3 && t[2] != "all") {
			int id = (int)ParseDec(t[2].c_str());
			for (size_t i = 0; i < s_bps.size(); i++)
				if (s_bps[i].id == id) { s_bps.erase(s_bps.begin() + i); break; }
		} else {
			s_bps.clear();
		}
		UpdateActive();
		return "OK\n";
	}
	if (t.size() >= 3 && (t[1] == "x" || t[1] == "r" || t[1] == "w")) {
		Breakpoint b;
		b.id = s_nextBpId++;
		b.type = t[1][0];
		b.lo = (WORD)ParseHex(t[2].c_str());
		b.hi = t.size() >= 4 ? (WORD)ParseHex(t[3].c_str()) : b.lo;
		s_bps.push_back(b);
		UpdateActive();
		return Fmt("OK id=%d\n", b.id);
	}
	return "ERR usage: bp x|r|w <addr> [end] | bp del <id|all> | bp list\n";
}

string CmdRegs(NES* nes, const vector<string>& t)
{
	R6502 r;

	nes->cpu->GetContext(r);
	for (size_t i = 1; i < t.size(); i++) {
		size_t eq = t[i].find('=');
		if (eq == string::npos)
			return "ERR usage: regs [pc=XXXX a=XX x=XX y=XX s=XX p=XX]\n";
		string k = t[i].substr(0, eq);
		unsigned long v = ParseHex(t[i].c_str() + eq + 1);
		if (k == "pc") r.PC = (WORD)v;
		else if (k == "a") r.A = (BYTE)v;
		else if (k == "x") r.X = (BYTE)v;
		else if (k == "y") r.Y = (BYTE)v;
		else if (k == "s") r.S = (BYTE)v;
		else if (k == "p") r.P = (BYTE)v;
		else return "ERR unknown register\n";
	}
	nes->cpu->SetContext(r);
	return "OK " + RegsLine(nes) + "\n";
}

// 取空间指针：cpu 空间返回 NULL 并由调用方逐字节 Peek
LPBYTE SpacePtr(NES* nes, const string& name, INT& size)
{
	MapperSB2K* m = GetSb2k(nes);

	size = 0;
	if (!m)
		return NULL;
	return m->DbgSpace(name.c_str(), size);
}

string CmdRead(NES* nes, const vector<string>& t)
{
	if (t.size() < 4)
		return "ERR usage: read <space> <addr> <len>\n";

	unsigned long addr = ParseHex(t[2].c_str());
	unsigned long len = ParseDec(t[3].c_str());
	string out = "OK\n";
	INT size;
	LPBYTE base = NULL;

	if (len > 1024 * 1024)
		return "ERR len too large\n";

	if (t[1] != "cpu") {
		base = SpacePtr(nes, t[1], size);
		if (!base)
			return "ERR unknown space\n";
		if (addr + len > (unsigned long)size)
			return "ERR out of range\n";
	}

	for (unsigned long i = 0; i < len; i += 64) {
		string line = "HEX ";
		for (unsigned long j = i; j < i + 64 && j < len; j++) {
			BYTE b = base ? base[addr + j] : PeekCpu(nes, (WORD)(addr + j));
			char h[4];
			_snprintf(h, sizeof(h), "%02X", b);
			line += h;
		}
		out += line + "\n";
	}
	return out;
}

string CmdWrite(NES* nes, const vector<string>& t)
{
	if (t.size() < 4)
		return "ERR usage: write <space> <addr> <hexbytes>\n";

	unsigned long addr = ParseHex(t[2].c_str());
	string hex = t[3];
	INT size;
	LPBYTE base = NULL;

	if (hex.size() % 2)
		return "ERR odd hex length\n";

	if (t[1] != "cpu") {
		base = SpacePtr(nes, t[1], size);
		if (!base)
			return "ERR unknown space\n";
		if (addr + hex.size() / 2 > (unsigned long)size)
			return "ERR out of range\n";
	}

	for (size_t i = 0; i < hex.size() / 2; i++) {
		char h[3] = { hex[i * 2], hex[i * 2 + 1], 0 };
		BYTE b = (BYTE)strtoul(h, NULL, 16);
		if (base)
			base[addr + i] = b;
		else
			nes->Write((WORD)(addr + i), b);
	}
	return "OK\n";
}

string CmdResolve(NES* nes, const vector<string>& t)
{
	MapperSB2K* m = GetSb2k(nes);
	INT off = 0;
	INT space;

	if (t.size() < 2 || !m)
		return "ERR usage: resolve <addr>\n";

	space = m->DbgResolve((WORD)ParseHex(t[1].c_str()), off);
	return Fmt("OK space=%s offset=%06X\n", space == 0 ? "rom" : (space == 1 ? "pram" : "none"), off);
}

string CmdKey(const vector<string>& t)
{
	if (t.size() >= 2 && t[1] == "clear") {
		memset(g_dbgKeys, 0, sizeof(g_dbgKeys));
		return "OK\n";
	}
	if (t.size() < 3)
		return "ERR usage: key <dik> <0|1> | key clear\n";

	unsigned long code = ParseHex(t[1].c_str());
	if (code > 255)
		return "ERR bad scancode\n";
	g_dbgKeys[code] = (BYTE)(ParseDec(t[2].c_str()) ? 0x80 : 0);
	return "OK\n";
}

// pad <手柄号 0-3> <十六进制掩码>：注入手柄按键，掩码位顺序 A、B、Select、Start、上、下、左、右；pad clear 清空
string CmdPad(const vector<string>& t)
{
	if (t.size() >= 2 && t[1] == "clear") {
		memset(g_dbgPad, 0, sizeof(g_dbgPad));
		return "OK\n";
	}
	if (t.size() < 3)
		return "ERR usage: pad <0-3> <hexmask> | pad clear\n";

	unsigned long no = ParseDec(t[1].c_str());
	if (no > 3)
		return "ERR bad pad number\n";
	g_dbgPad[no] = (BYTE)ParseHex(t[2].c_str());
	return "OK\n";
}

string CmdMouse(NES* nes, const vector<string>& t)
{
	if (t.size() >= 4 && t[1] == "move") {
		nes->pad->SetMouseStatus((INT)ParseDec(t[2].c_str()), (INT)ParseDec(t[3].c_str()), 0, 0x10);
		return "OK\n";
	}
	if (t.size() >= 3 && t[1] == "btn") {
		nes->pad->SetMouseStatus(0, 0, (INT)ParseDec(t[2].c_str()), 0);
		return "OK\n";
	}
	return "ERR usage: mouse move <dx> <dy> | mouse btn <mask>\n";
}

string CmdShot(NES* nes, const string& line)
{
	string path = RestOf(line, 1);
	PNGWRITE png;

	if (path.empty())
		return "ERR usage: shot <file.png>\n";

	if (NES::NESVARIANT_UM6576 == nes->GetVariant()) {
		// SB2000：每一行有自己的颜色模式（$2001 位 0 黑白、位 5-7 增强色），最多取 4 种，每种占 64 项调色板
		static RGBQUAD all[8 * 256];
		static RGBQUAD rgb[256];
		static BYTE img[256 * 240];
		LPBYTE mode = DirectDraw.GetLineColormode();
		LPBYTE scr = nes->ppu->GetScreenPtr() + 8;
		BYTE slotMode[4] = { 0, 0, 0, 0 };
		int slots = 0;

		memset(all, 0, sizeof(all));
		memset(rgb, 0, sizeof(rgb));
		DirectDraw.GetPaletteData2(all);

		for (int y = 0; y < 240; y++) {
			int slot = 0;

			while (slot < slots && slotMode[slot] != mode[y])
				slot++;
			if (slot == slots) {
				if (slots < 4) {
					slotMode[slots++] = mode[y];
				} else {
					slot = 3; // 超过 4 种时并入最后一种
				}
			}

			for (int x = 0; x < 256; x++)
				img[y * 256 + x] = (BYTE)((scr[y * CDirectDraw::RENDER_WIDTH + x] & 0x3F) + slot * 64);
		}

		for (int s = 0; s < slots; s++) {
			for (int i = 0; i < 64; i++) {
				int m = slotMode[s] & 7;

				rgb[s * 64 + i] = (slotMode[s] & 0x80) ? all[m * 256 + 0x40 + i] : all[m * 256 + i];
			}
		}

		if (!png.Write(path.c_str(), 256, 240, rgb, img, 256))
			return "ERR write failed\n";
		return "OK\n";
	}

	RGBQUAD rgb[256];

	memset(rgb, 0, sizeof(rgb));
	DirectDraw.GetPaletteData(rgb);
	if (!png.Write(path.c_str(), 256, 240, rgb, nes->ppu->GetScreenPtr() + 8, CDirectDraw::RENDER_WIDTH))
		return "ERR write failed\n";
	return "OK\n";
}

// trace <文件> [最大行数] [unique]、trace off；unique 时每个 CPU 地址的指令、每个中断入口只记第一次，最大行数按实际写出的行数算
string CmdTrace(const vector<string>& t)
{
	if (s_trace) { fclose(s_trace); s_trace = NULL; }
	if (t.size() >= 2 && t[1] != "off") {
		s_traceLeft = 1000000;
		s_traceUnique = false;
		for (size_t i = 2; i < t.size(); i++) {
			if (t[i] == "unique")
				s_traceUnique = true;
			else
				s_traceLeft = ParseDec(t[i].c_str());
		}
		memset(s_traceSeen, 0, sizeof(s_traceSeen));
		s_traceSeenInt.clear();
		s_trace = fopen(t[1].c_str(), "w");
		if (!s_trace)
			return "ERR cannot open file\n";
	}
	UpdateActive();
	return "OK\n";
}

string CmdIo(const vector<string>& t)
{
	if (s_io) { fclose(s_io); s_io = NULL; }
	if (t.size() >= 2 && t[1] != "off") {
		s_ioLeft = t.size() >= 3 ? ParseDec(t[2].c_str()) : 1000000;
		s_ioWritesOnly = t.size() >= 4 && t[3] == "w";
		s_io = fopen(t[1].c_str(), "w");
		if (!s_io)
			return "ERR cannot open file\n";
	}
	UpdateActive();
	return "OK\n";
}

// calllog <file> <pc> <addr:len>[,<addr:len>...] [max] [skip=addr=hex[,...]]：每次执行到 pc 就记录寄存器、栈顶与所给内存段；可添加多条，calllog off 全部关闭
string CmdCallLog(const vector<string>& t)
{
	if (t.size() >= 2 && t[1] == "off") {
		for (size_t i = 0; i < s_calls.size(); i++)
			fclose(s_calls[i].fp);
		s_calls.clear();
		UpdateActive();
		return "OK\n";
	}
	if (t.size() < 4)
		return "ERR usage: calllog <file> <pc> <addr:len>[,<addr:len>...] [max] | calllog off\n";
	if (s_calls.size() >= 8)
		return "ERR too many call logs\n";

	CallLog log;
	size_t start = 0;
	while (start < t[3].size()) {
		size_t comma = t[3].find(',', start);
		string part = t[3].substr(start, comma == string::npos ? string::npos : comma - start);
		size_t colon = part.find(':');
		if (colon == string::npos)
			return "ERR bad range, use addr:len\n";
		MemRange r;
		r.addr = (WORD)ParseHex(part.c_str());
		r.len = (WORD)ParseDec(part.c_str() + colon + 1);
		log.ranges.push_back(r);
		if (comma == string::npos)
			break;
		start = comma + 1;
	}

	log.pc = (WORD)ParseHex(t[2].c_str());
	log.left = 100000;

	// 可选参数：纯数字是最大次数，skip=地址=值[,地址=值...] 表示该地址字节等于值时不记录
	for (size_t i = 4; i < t.size(); i++) {
		if (t[i].compare(0, 5, "skip=") != 0) {
			log.left = ParseDec(t[i].c_str());
			continue;
		}

		size_t from = 5;
		while (from < t[i].size()) {
			size_t comma = t[i].find(',', from);
			string part = t[i].substr(from, comma == string::npos ? string::npos : comma - from);
			size_t eq = part.find('=');
			if (eq == string::npos)
				return "ERR bad skip, use addr=value\n";
			MemRange r;
			r.addr = (WORD)ParseHex(part.c_str());
			r.len = (WORD)ParseHex(part.c_str() + eq + 1);
			log.skip.push_back(r);
			if (comma == string::npos)
				break;
			from = comma + 1;
		}
	}
	log.fp = fopen(t[1].c_str(), "w");
	if (!log.fp)
		return "ERR cannot open file\n";
	s_calls.push_back(log);
	UpdateActive();
	return "OK\n";
}

string CmdDiskLog(const vector<string>& t)
{
	if (s_disk) { fclose(s_disk); s_disk = NULL; }
	if (t.size() >= 2 && t[1] != "off") {
		s_diskLeft = t.size() >= 3 ? ParseDec(t[2].c_str()) : 1000000;
		s_disk = fopen(t[1].c_str(), "w");
		if (!s_disk)
			return "ERR cannot open file\n";
	}
	UpdateActive();
	return "OK\n";
}

// 软驱故障注入：fdc eject|insert|wp <0|1>|noindex <0|1>|seekfail <0|1>|dead <0|1>|bad <lba...>|clear|status
string CmdFdc(NES* nes, const vector<string>& t)
{
	const char* usage = "ERR usage: fdc eject|insert|wp <0|1>|noindex <0|1>|seekfail <0|1>|dead <0|1>|bad <lba...>|clear|status\n";
	FDC* f = nes->fdc;

	if (t.size() < 2)
		return usage;

	const string& s = t[1];
	if (s == "eject") {
		f->DbgEject();
	} else if (s == "insert") {
		if (!f->DbgInsert())
			return "ERR no disk image loaded\n";
	} else if (s == "wp" && t.size() >= 3) {
		f->DbgSetWriteProtect(t[2] != "0");
	} else if (s == "noindex" && t.size() >= 3) {
		f->dbgNoIndex = (t[2] != "0");
	} else if (s == "seekfail" && t.size() >= 3) {
		f->dbgSeekFail = (t[2] != "0");
	} else if (s == "dead" && t.size() >= 3) {
		f->dbgDead = (t[2] != "0");
	} else if (s == "bad" && t.size() >= 3) {
		for (size_t i = 2; i < t.size(); i++)
			f->dbgBadLba.push_back((int)ParseDec(t[i].c_str()));
	} else if (s == "clear") {
		f->DbgClearFaults();
	} else if (s == "status") {
		return "OK " + f->DbgStatus() + "\n";
	} else {
		return usage;
	}
	return "OK\n";
}

string CmdCdl(const vector<string>& t)
{
	if (t.size() < 2)
		return "ERR usage: cdl on|off|clear|save <prefix>\n";

	if (t[1] == "on") {
		for (int i = 0; i < 2; i++)
			if (!s_cdl[i])
				s_cdl[i] = (BYTE*)calloc(CDL_SPACE_SIZE, 1);
		s_cdlOn = true;
	} else if (t[1] == "off") {
		s_cdlOn = false;
	} else if (t[1] == "clear") {
		for (int i = 0; i < 2; i++)
			if (s_cdl[i]) memset(s_cdl[i], 0, CDL_SPACE_SIZE);
	} else if (t[1] == "save" && t.size() >= 3) {
		static const char* names[2] = { "rom", "pram" };
		for (int i = 0; i < 2; i++) {
			if (!s_cdl[i])
				return "ERR cdl is not enabled\n";
			string p = t[2] + "-" + names[i] + ".cdl";
			FILE* fp = fopen(p.c_str(), "wb");
			if (!fp)
				return "ERR cannot open file\n";
			fwrite(s_cdl[i], 1, CDL_SPACE_SIZE, fp);
			fclose(fp);
		}
	} else {
		return "ERR usage: cdl on|off|clear|save <prefix>\n";
	}
	UpdateActive();
	return "OK\n";
}

// tape play|rec <文件>、tape stop、tape status：数据录音机（.vtp），Family BASIC 的 SAVE、LOAD 经它存取；rec 的文件在 stop 后才完整
string CmdTape(NES* nes, const string& line, const vector<string>& t)
{
	const char* usage = "ERR usage: tape play <file> | tape rec <file> | tape stop | tape status\n";

	if (t.size() < 2)
		return usage;

	if (t[1] == "stop") {
		if (nes->IsTapePlay() || nes->IsTapeRec())
			nes->TapeStop();
		return "OK\n";
	}
	if (t[1] == "status")
		return Fmt("OK play=%d rec=%d\n", nes->IsTapePlay() ? 1 : 0, nes->IsTapeRec() ? 1 : 0);
	if ((t[1] == "play" || t[1] == "rec") && t.size() >= 3) {
		string path = RestOf(line, 2);
		BOOL ok = (t[1] == "play") ? nes->TapePlay(path.c_str()) : nes->TapeRec(path.c_str());
		return ok ? "OK\n" : "ERR cannot open tape file\n";
	}
	return usage;
}

// 执行一行命令并返回应答文本（不含结束行）
string Execute(const string& line)
{
	vector<string> t = Split(line);
	NES* nes = GetNes();

	if (t.empty())
		return "ERR empty command\n";

	const string& c = t[0];

	if (c == "ping")
		return "OK pong\n";
	if (c == "status")
		return CmdStatus();
	if (c == "quit") {
		PostMessage(CApp::GetHWnd(), WM_CLOSE, 0, 0);
		return "OK\n";
	}
	if (c == "key")
		return CmdKey(t);
	if (c == "pad")
		return CmdPad(t);
	if (c == "bp")
		return CmdBp(t);
	if (c == "trace")
		return CmdTrace(t);
	if (c == "io")
		return CmdIo(t);
	if (c == "cdl")
		return CmdCdl(t);
	if (c == "dialogs")
		return CmdDialogs(t);
	if (c == "calllog")
		return CmdCallLog(t);
	if (c == "disklog")
		return CmdDiskLog(t);
	// 声音播放速度开关是全局静态状态，不依赖模拟器是否正在运行
	if (c == "audio") {
		if (t.size() == 2 && (t[1] == "fast" || t[1] == "real")) {
			MapperSB2K::SetAudioFast(t[1] == "fast");
			return "OK\n";
		}
		return "ERR usage: audio fast|real\n";
	}

	if (!nes)
		return "ERR emulator is not running\n";

	// 切换为自由运行：不再由 run 授权帧数，模拟器按正常速度连续运行（fcdev run 用它把窗口交给用户操作）
	if (c == "live") {
		InterlockedExchange(&g_dbgMode, 0);
		UpdateActive();
		return "OK\n";
	}

	if (c == "tape")
		return CmdTape(nes, line, t);
	if (c == "run")
		return CmdRun(nes, t);
	if (c == "step")
		return CmdStep(nes, t);
	if (c == "regs")
		return CmdRegs(nes, t);
	if (c == "read")
		return CmdRead(nes, t);
	if (c == "write")
		return CmdWrite(nes, t);
	if (c == "resolve")
		return CmdResolve(nes, t);
	if (c == "mouse")
		return CmdMouse(nes, t);
	if (c == "shot")
		return CmdShot(nes, line);
	if (c == "disk") {
		string p = RestOf(line, 1);
		return nes->fdc->LoadDiskImage(p.c_str()) ? "OK\n" : "ERR cannot load disk image\n";
	}
	if (c == "fdc")
		return CmdFdc(nes, t);
	if (c == "savedisk")
		return nes->fdc->SaveDiskImage() ? "OK\n" : "ERR cannot save disk image\n";
	if (c == "reset") {
		nes->Command(NES::NESCMD_HWRESET);
		return "OK\n";
	}
	// 软复位只复位 CPU、手柄与 APU，映射器状态（FC 兼容模式、bank）保持不变
	if (c == "softreset") {
		nes->Command(NES::NESCMD_SWRESET);
		return "OK\n";
	}
	if (c == "savestate")
		return nes->SaveState(RestOf(line, 1).c_str()) ? "OK\n" : "ERR cannot save state\n";
	if (c == "loadstate")
		return nes->LoadState(RestOf(line, 1).c_str()) ? "OK\n" : "ERR cannot load state\n";

	return "ERR unknown command\n";
}

// 客户端发来的是 UTF-8，文件函数使用系统代码页，这里转换后路径里的中文才能打开
string Utf8ToAcp(const string& utf8)
{
	int wn = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
	if (wn <= 0)
		return utf8;

	vector<wchar_t> wide(wn);
	MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], wn);

	int an = WideCharToMultiByte(CP_ACP, 0, &wide[0], -1, NULL, 0, NULL, NULL);
	if (an <= 0)
		return utf8;

	vector<char> acp(an);
	WideCharToMultiByte(CP_ACP, 0, &wide[0], -1, &acp[0], an, NULL, NULL);
	return string(&acp[0]);
}

bool SendAll(SOCKET s, const string& data)
{
	size_t off = 0;

	while (off < data.size()) {
		int n = send(s, data.data() + off, (int)(data.size() - off), 0);
		if (n <= 0)
			return false;
		off += n;
	}
	return true;
}

void ServeClient(SOCKET c)
{
	string buf;
	char tmp[4096];

	for (;;) {
		int n = recv(c, tmp, sizeof(tmp), 0);
		if (n <= 0)
			break;
		buf.append(tmp, n);

		size_t pos;
		while ((pos = buf.find('\n')) != string::npos) {
			string line = buf.substr(0, pos);
			buf.erase(0, pos + 1);
			while (!line.empty() && (line[line.size() - 1] == '\r'))
				line.erase(line.size() - 1);

			string reply = Execute(Utf8ToAcp(line));
			reply += ".\n";
			if (!SendAll(c, reply))
				return;
		}
	}
}

DWORD WINAPI ServerThread(LPVOID)
{
	for (;;) {
		SOCKET c = accept(s_listen, NULL, NULL);
		if (c == INVALID_SOCKET)
			break;
		ServeClient(c);
		closesocket(c);
	}
	return 0;
}

} // namespace

// ---- 对外接口 ----

void DebugServer_Start()
{
	const char* portText = getenv("FCDEV_DEBUG_PORT");
	WSADATA wsa;
	sockaddr_in addr;
	int port;

	if (!portText || !*portText)
		return;

	port = atoi(portText);
	if (port <= 0 || port > 65535)
		return;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;

	s_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s_listen == INVALID_SOCKET)
		return;

	// 只监听本机回环地址，避免被局域网访问
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (bind(s_listen, (sockaddr*)&addr, sizeof(addr)) != 0 || listen(s_listen, 1) != 0) {
		closesocket(s_listen);
		s_listen = INVALID_SOCKET;
		return;
	}

	s_evFrame = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 非法指令不弹错误框，由指令钩子按 JAM 停止
	Config.emulator.bIllegalOp = TRUE;

	g_dbgMode = 1;
	UpdateActive();

	InitializeCriticalSection(&s_dlgLock);
	CreateThread(NULL, 0, DialogWatchThread, NULL, 0, NULL);
	CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
}

BOOL DebugServer_BeginFrame()
{
	// 先置忙再检查授权，使“取消授权后等待空闲”的一方不会漏掉刚开始的帧
	s_busy = 1;
	MemoryBarrier();

	LONG p = s_pending;
	if (p <= 0 || InterlockedCompareExchange(&s_pending, p - 1, p) != p) {
		s_busy = 0;
		return FALSE;
	}
	return TRUE;
}

void DebugServer_EndFrame()
{
	InterlockedIncrement(&s_frameNo);
	s_busy = 0;
	if (s_evFrame)
		SetEvent(s_evFrame);
}

BOOL DebugServer_WantBlt()
{
	return s_pending <= 0;
}

BOOL DebugHook_OnInstr(WORD pc, BYTE opcode, const R6502& r)
{
	NES* nes = Emu.GetNES();
	char buf[96];

	// 刚才暂存的读取如果就是这条指令的取指，丢弃它
	if (s_pend.valid && s_pend.addr == pc)
		s_pend.valid = false;
	FlushPending();

	if (s_stop)
		return TRUE;

	s_curPc = pc;
	s_curLen = OpLen(opcode);

	if (opcode == 0x02 || opcode == 0x12 || opcode == 0x22 || opcode == 0x32 || opcode == 0x42 ||
		opcode == 0x52 || opcode == 0x62 || opcode == 0x72 || opcode == 0x92 || opcode == 0xB2 ||
		opcode == 0xD2 || opcode == 0xF2) {
		_snprintf(buf, sizeof(buf), "jam opcode=%02X", opcode);
		RequestStop(buf);
		return TRUE;
	}

	if (s_stepMode) {
		if (s_stepLeft <= 0) {
			RequestStop("step");
			return TRUE;
		}
		s_stepLeft--;
	}

	if (!s_skipFirst) {
		for (size_t i = 0; i < s_bps.size(); i++) {
			if (s_bps[i].type == 'x' && InRange(s_bps[i], pc)) {
				_snprintf(buf, sizeof(buf), "exec bp #%d", s_bps[i].id);
				RequestStop(buf);
				return TRUE;
			}
		}
	}
	s_skipFirst = false;

	s_instrCount++;

	for (size_t k = 0; k < s_calls.size(); k++) {
		CallLog& log = s_calls[k];

		if (pc != log.pc || !log.left)
			continue;

		bool skipped = false;
		for (size_t i = 0; i < log.skip.size(); i++)
			if (PeekCpu(nes, log.skip[i].addr) == (BYTE)log.skip[i].len)
				skipped = true;
		if (skipped)
			continue;

		fprintf(log.fp, "C pc=%04X a=%02X x=%02X y=%02X s=%02X p=%02X F=%ld\n", pc, r.A, r.X, r.Y, r.S, r.P, (long)s_frameNo);
		fprintf(log.fp, "STK");
		for (int i = 1; i <= 8; i++)
			fprintf(log.fp, " %02X", RAM[0x100 | ((r.S + i) & 0xFF)]);
		fprintf(log.fp, "\n");
		for (size_t i = 0; i < log.ranges.size(); i++) {
			fprintf(log.fp, "M %04X", log.ranges[i].addr);
			for (int j = 0; j < log.ranges[i].len; j++)
				fprintf(log.fp, " %02X", PeekCpu(nes, (WORD)(log.ranges[i].addr + j)));
			fprintf(log.fp, "\n");
		}
		if (--log.left == 0) {
			fclose(log.fp);
			s_calls.erase(s_calls.begin() + k);
			k--;
		}
	}
	if (s_cdlOn) {
		MarkCdl(nes, pc, CDL_OPCODE);
		for (int i = 1; i < s_curLen; i++)
			MarkCdl(nes, (WORD)(pc + i), CDL_OPERAND);
	}

	if (s_trace && s_traceLeft && !TraceAlreadySeen(pc)) {
		MapperSB2K* m = GetSb2k(nes);
		INT off = 0;
		INT space = m ? m->DbgResolve(pc, off) : -1;

		fprintf(s_trace, "T %04X %c%06X %02X %02X %02X A=%02X X=%02X Y=%02X S=%02X P=%02X F=%ld\n",
			pc, space == 0 ? 'R' : (space == 1 ? 'E' : '-'), space >= 0 ? off : 0,
			opcode,
			s_curLen > 1 ? PeekCpu(nes, (WORD)(pc + 1)) : 0,
			s_curLen > 2 ? PeekCpu(nes, (WORD)(pc + 2)) : 0,
			r.A, r.X, r.Y, r.S, r.P, (long)s_frameNo);
		if (--s_traceLeft == 0) { fclose(s_trace); s_trace = NULL; }
	}

	return FALSE;
}

void DebugHook_OnDisk(int lba, int write)
{
	if (s_disk && s_diskLeft) {
		fprintf(s_disk, "D %c %d pc=%04X F=%ld\n", write ? 'W' : 'R', lba, s_curPc, (long)s_frameNo);
		if (--s_diskLeft == 0) { fclose(s_disk); s_disk = NULL; }
	}
}

void DebugHook_OnRead(WORD addr, BYTE data)
{
	FlushPending();
	s_pend.valid = true;
	s_pend.addr = addr;
	s_pend.data = data;
	s_pend.pc = s_curPc;
	s_pend.len = s_curLen;
}

void DebugHook_OnWrite(WORD addr, BYTE data)
{
	NES* nes = Emu.GetNES();
	char buf[96];

	FlushPending();

	for (size_t i = 0; i < s_bps.size(); i++) {
		if (s_bps[i].type == 'w' && InRange(s_bps[i], addr)) {
			_snprintf(buf, sizeof(buf), "write bp #%d addr=%04X data=%02X", s_bps[i].id, addr, data);
			RequestStop(buf);
		}
	}

	if (s_io && s_ioLeft && addr >= 0x2000 && addr < 0x5000) {
		fprintf(s_io, "W %04X %02X pc=%04X\n", addr, data, s_curPc);
		if (--s_ioLeft == 0) { fclose(s_io); s_io = NULL; }
	}

	if (s_cdlOn && addr >= 0x8000)
		MarkCdl(nes, addr, CDL_WRITE);
}

void DebugHook_OnInterrupt(int type, WORD vectorTarget)
{
	if (s_trace && s_traceLeft && !TraceInterruptSeen(type, vectorTarget)) {
		fprintf(s_trace, "I %s %04X F=%ld\n", type == 0 ? "NMI" : "IRQ", vectorTarget, (long)s_frameNo);
		if (--s_traceLeft == 0) { fclose(s_trace); s_trace = NULL; }
	}
}
