//////////////////////////////////////////////////////////////////////////
// Mapper168                                                            //
//////////////////////////////////////////////////////////////////////////

#include "WaveOut.h"

class	Mapper168 : public Mapper
{
public:
	Mapper168( NES* parent ) : Mapper(parent) { Init(); }

	void	Init();
	void	Reset();
	~Mapper168();

	static int FeedLPC( void* host, unsigned char* food );
	static DWORD WINAPI LPCThreadProc( LPVOID lpParam );

	BYTE	ReadLow ( WORD addr );
	void	WriteLow(WORD addr, BYTE data);
	void	Write( WORD addr, BYTE data );
	void	PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr );
	void	Clock(INT cycles);
	BYTE	ExCmdRead ( EXCMDRD cmd );

protected:
	void	SetBank_CPU();
	BYTE	reg5000, reg5200, reg5300;
	BYTE	PPU_SW, NT_data;
	BYTE	Rom_Type;

private:
	BOOL	bDiskAccess;
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
