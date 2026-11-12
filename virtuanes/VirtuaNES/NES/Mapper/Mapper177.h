//////////////////////////////////////////////////////////////////////////
// Mapper177  HengGe                                                    //
//////////////////////////////////////////////////////////////////////////
class	Mapper177 : public Mapper
{
public:
	Mapper177( NES* parent ) : Mapper(parent) { Init(); }
	void	Init();
	void	Reset();
	~Mapper177();

	static int FeedLPC( void* host, unsigned char* food );
	static DWORD WINAPI LPCThreadProc( LPVOID lpParam );

	BYTE	ReadLow ( WORD addr );
	void	Write(WORD addr, BYTE data);
	void	WriteLow(WORD addr, BYTE data);

protected:
private:
	BYTE SP_rom;

	int		stateLPC;
	BOOL	m_bSpeechEnd;
#define SP_BUF_SIZE 16
	BYTE	nRegSPInt;
	BYTE	nSPData[SP_BUF_SIZE];
	int		nSPReadPos;
	int		nSpWritePos;
	int		nResetLPC;
	BOOL    bRunLPC;
	HANDLE  reqLPC;
	HANDLE  ackLPC;
	HANDLE  thdLPC;
};
