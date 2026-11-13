/*******************************************************************************
 * NES Mapper for BBK
 *
 *  Author:  <fanoble@yeah.net>
 *
 *  Create:   2014-06-24, by fanoble
 *******************************************************************************
 */

#include "WaveOut.h"

class	MapperSB2K : public Mapper
{
public:
	MapperSB2K( NES* parent ) : Mapper(parent) { Init(); }

	void	Init();
	void	Reset();

	~MapperSB2K();

//------------------------------------------------------------------------------

	static int FeedLPC( void* host, unsigned char* food );

	static DWORD WINAPI LPCThreadProc( LPVOID lpParam );
	static DWORD WINAPI DACThreadProc( LPVOID lpParam );

//------------------------------------------------------------------------------

	// 8000 - FFFF
	BOOL	ReadHigh( WORD addr, LPBYTE pdata );
	void	Write( WORD addr, BYTE data );

	// 4100 - 7FFF
	BYTE	ReadLow ( WORD addr );
	void	WriteLow( WORD addr, BYTE data );

	// $4018-$40FF Extention register read/write
	BYTE	ExRead ( WORD addr );
	void	ExWrite( WORD addr, BYTE data );

	BYTE	PPU_Read ( WORD addr );
	void	PPU_Write( WORD addr, BYTE data );

	// for debug view
	BYTE	Peek ( INT addr, BOOL is_logical );
	BYTE	PPU_Peek ( INT addr, BOOL is_logical );

//------------------------------------------------------------------------------
	void	PPU_Latch( WORD addr );
	LPBYTE  GetExtPMEM( int addr );
	LPBYTE  GetExtVMEM( int addr );
	LPBYTE	GetPatternTable( int scanline, int addr );
	INT     GetBankNo( int addr );

	BYTE	ExCmdRead ( EXCMDRD cmd );
	BOOL	IsThrottleRequired();

	// 调试：打开后语音与 DAC 线程不再等声卡按真实时间播放（开机音乐瞬间播完），关闭为真实时间；静态变量，复位后保持
	static void	SetAudioFast(BOOL on);

	// 调试：把 CPU 地址解析为物理空间。返回 0 为 BIOS 只读存储器，1 为扩展程序 RAM，-1 为不在两者内；off 返回空间内偏移
	INT		DbgResolve(WORD addr, INT& off);
	// 调试：按名字取物理空间（rom、pram、vram、cram、eram、wram、ram），返回起始指针并经 size 返回大小
	LPBYTE	DbgSpace(const char* name, INT& size);
	// 调试：把 bank 寄存器与扩展寄存器状态写成一行文本
	void	DbgInfo(char* buf, size_t cap);
	// 已经完成 $4031 初始设置，处于 6576 模式
	BOOL	IsMode6576() const { return 2 == nMode6576; }

	BOOL	CheckIRQ();
//------------------------------------------------------------------------------

	void	Clock( INT nCycles );
	void	HSync( INT scanline );

//------------------------------------------------------------------------------

	volatile int m_nDACCmd;

	BYTE	m_bDACData;
	int		m_nDACRate;

protected:

	void	TimerTick( INT nTick );

	void	MMC3_Remap();

private:
	static volatile LONG s_nAudioFast;

	// 软驱中断线上一次的状态，用来只在上升沿置 $4033 位 3
	BOOL	m_bFdcIrq;

	INT		nMode6576;

	BYTE	m_bEBank; // 128K x 4

	BYTE	m_bC3_Sel;
	BYTE	m_bC3_Reg[8];

	BYTE	m_bC3_IRQEnable;
	BYTE	m_bC3_IRQRequest;
	BYTE	m_bC3_IRQLatch;
	BYTE	m_bC3_IRQCounter;
	BYTE	m_bC3_IRQPreset;
	BYTE	m_bC3_IRQPresetVBL;

	BOOL	m_bRamBoot;

	BYTE	m_bVbank; // 32K * 16 = 512K

	BYTE	m_bPbank[8];
	BYTE	m_bDmaReg[8];
	BYTE	m_bTimingSettingControl;

	BYTE	m_bRegKbdIn;
	BYTE	m_bRegKbdOut;
	BYTE	m_bReg4022;
	BYTE	m_bReg4023;
	BYTE	m_bReg4024;
	BYTE	m_bReg4026;
	BYTE	m_bReg4032;
	BYTE	m_bReg4033;
	BYTE	m_bReg4034;
	BYTE	m_bReg4035;
	BYTE	m_bReg4036;

	BYTE	m_bTimerPrescaleCnt;
	BOOL	m_bTimerOneShotDone; // 一次计时（$4034 位 6 为 0）已经下溢，当前值保持预置值、不再计时

	BOOL	m_bDACOn;

	int		kbd_send_irq_delay;

	BOOL	m_bSpeechReady;
	BOOL	m_bSpeechEnd;
	int		m_nSpeechPadding;

	BOOL	bDiskAccess;

//------------------------------------------------------------------------------

	// for ROM and RAM
	LPBYTE	BIOS;
	LPBYTE	DISK;
	BYTE	EPRAM[512 * 1024];
	BYTE	EVRAM[512 * 1024];

//------------------------------------------------------------------------------

#define SP_BUF_SIZE 16
	BYTE	nSPData[SP_BUF_SIZE];
	int		nSPReadPos;
	int		nSpWritePos;

	int		nResetLPC;

	BOOL    bRunLPC;
	HANDLE  reqLPC;
	HANDLE  ackLPC;
	HANDLE  thdLPC;

	BOOL    bRunDAC;
	HANDLE  reqDAC;
	HANDLE  ackDAC;
	HANDLE  thdDAC;
};

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
