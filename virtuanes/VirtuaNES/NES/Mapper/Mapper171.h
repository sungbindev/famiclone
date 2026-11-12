/*******************************************************************************
 * NES Mapper for BBK
 *
 *  Author:  <fanoble@yeah.net>
 *
 *  Create:   2014-06-24, by fanoble
 *******************************************************************************
 */

#include <queue>
#include "WaveOut.h"

class	Mapper171 : public Mapper
{
public:
	Mapper171(NES* parent) : Mapper(parent) { Init(); }
	void	Init();
	void	Reset();
	~Mapper171();

	static int FeedLPC(void* host, unsigned char* food);
	static DWORD WINAPI LPCThreadProc(LPVOID lpParam);

#define INNO_CS_NONE	0
#define INNO_CS_ROM		1
#define INNO_CS_DRAM	2
#define INNO_CS_HOLTEK	3
	DWORD	MapAddr(WORD addr, int* cs_type, BOOL is_write);

	// 8000 - FFFF
	BOOL	ReadHigh(WORD addr, LPBYTE pdata);
	void	Write(WORD addr, BYTE data);
	// 4100 - 7FFF
	BYTE	ReadLow(WORD addr);
	void	WriteLow(WORD addr, BYTE data);

	BYTE	ExCmdRead(EXCMDRD cmd);
	void	HSync(INT scanline);
	void	Clock(INT cycles);

protected:
	BYTE	YPRAM_mask;
	BYTE	YCRAM_mask;
	BOOL	MMC3_mode;
	BYTE	MMC3_cmd;
	BYTE	MMC3_prg0, MMC3_prg1;
	BYTE	MMC3_chr01, MMC3_chr23, MMC3_chr4, MMC3_chr5, MMC3_chr6, MMC3_chr7;
	BYTE	MMC3_irq_enable, MMC3_irq_counter, MMC3_irq_latch;
	BYTE	MMC3_irq_request, MMC3_irq_preset, MMC3_irq_preset_vbl;
	BYTE	MMC3_fixPRAM(BYTE data);
	void	MMC3_Reset();
	void	MMC3_Write(WORD addr, BYTE data);
	void	MMC3_SetBank_CPU();
	void	MMC3_SetBank_PPU();
	void	MMC3_IRQSync();
	void	SetBank();

private:
	BYTE	splitIndex;
	BYTE	scanlineCount;
	std::vector<BYTE> splitScanlines, tempSplitScanlines;
	std::vector<BYTE> splitBanks, tempSplitBanks;

	BYTE	reg[0x100];
	BYTE	PROM_mask;
	BYTE	PRAM_mask;
	BYTE	VRAM_mask;
	BOOL	bDiskAccess;

	BOOL	bios_type;
	BYTE	prev09;
	BOOL	bMapRam;
	BOOL	bSplitMode;
	BOOL	bEnableIRQ;
	BOOL	test_once;

	BOOL	BBK_928;

	BYTE	nRegSPInt;
	BYTE	nSPData[0x10];
	int		nSPReadPos;
	int		nSpWritePos;
	int		nResetLPC;
	BOOL    bRunLPC;
	HANDLE  reqLPC;
	HANDLE  ackLPC;
	HANDLE  thdLPC;
};

/*******************************************************************************
						   E N D  O F  F I L E
*******************************************************************************/
