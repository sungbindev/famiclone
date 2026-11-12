//////////////////////////////////////////////////////////////////////////
// Mapper241  Education X-in-1                                          //
//////////////////////////////////////////////////////////////////////////

class	Mapper241 : public Mapper
{
public:
	Mapper241( NES* parent ) : Mapper(parent) { Init(); }

	void	Init();
	~Mapper241();

	static int FeedLPC( void* host, unsigned char* food );
	static DWORD WINAPI LPCThreadProc( LPVOID lpParam );

	void	Reset();
	void	Write(WORD addr, BYTE data);
	void	WriteLow(WORD addr, BYTE data);
	BYTE	ReadLow( WORD addr );
	void	PPU_Latch( WORD addr );
	void	PPU_ExtLatchX( INT x );
	void	PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr );

protected:
	BYTE	Rom_type;
	BYTE	count, temp_data;
	BYTE	a3, p_mode, s_mode, NT_data;

private:

	int		stateLPC;
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
