//////////////////////////////////////////////////////////////////////////
// Mapper169                                                            //
//////////////////////////////////////////////////////////////////////////

#include "VirtuaNESres.h"
#include "DirectInput.h"
#include "App.h"
#include "WaveOut.h"

class	Mapper169 : public Mapper
{
public:
	Mapper169( NES* parent ) : Mapper(parent) { Init(); }

	void	Init();
	~Mapper169();

	static int FeedLPC( void* host, unsigned char* food );
	static DWORD WINAPI LPCThreadProc( LPVOID lpParam );

	void	Reset();
	BYTE	ReadLow ( WORD addr );
	void	WriteLow( WORD addr, BYTE data );
	void	Write( WORD addr, BYTE data );
	void	PPU_Latch( WORD addr );
	void	PPU_ExtLatchX( INT x );
	void	PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr );
	BYTE	PPU_ExtLatchSP();
	void	HSync(INT scanline);
	void	Clock(INT cycles);

	BYTE	ExCmdRead ( EXCMDRD cmd );

protected:
	BYTE	YX_type;
	BYTE	PRAM_mask;
	BYTE	CRAM_mask;

	INT		key_map_row;

	BYTE	reg5002;
	BYTE	reg4800,reg5500,reg5501;
	BYTE	reg8000;

	BYTE	a3,NT_data;

	BYTE	MMC3_mode;
	BYTE	MMC3_cmd;
	BYTE	MMC3_prg0,MMC3_prg1;
	BYTE	MMC3_chr01,MMC3_chr23,MMC3_chr4,MMC3_chr5,MMC3_chr6,MMC3_chr7;
	BYTE	MMC3_irq_enable,MMC3_irq_counter,MMC3_irq_latch;
	BYTE	MMC3_irq_request,MMC3_irq_preset,MMC3_irq_preset_vbl;
	BYTE	MMC3_CRAMfix;

	INT		R4034count;

private:
	void	SetBank_CPU();
	void	MMC3_Reset();
	void	MMC3_Write( WORD addr, BYTE data );
	void	MMC3_SetBank_CPU();
	void	MMC3_SetBank_PPU();
	void	MMC3_IRQSync();
	BYTE	MMC3_fixCHR( BYTE chr );

	BOOL	bDiskAccess;
	INT		count;
	BYTE	datamask;
	BYTE	dataLSB,dataMSB;
	BYTE	maskMSB;
	BOOL	readLPC;
	int		stateLPC;
	BOOL	m_bSpeechEnd;
#define SP_BUF_SIZE 16
	BYTE	nSPData[SP_BUF_SIZE];
	int		nSPReadPos;
	int		nSpWritePos;
	BOOL	nResetLPC;
	BOOL    bRunLPC;
	HANDLE  reqLPC;
	HANDLE  ackLPC;
	HANDLE  thdLPC;
};
