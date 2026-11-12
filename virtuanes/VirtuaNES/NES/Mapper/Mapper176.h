//////////////////////////////////////////////////////////////////////////
// Mapper176                                                            //
//////////////////////////////////////////////////////////////////////////
class	Mapper176 : public Mapper
{
public:
	Mapper176( NES* parent ) : Mapper(parent) { Init(); }
	void	Init();
	~Mapper176();
	static int FeedLPC( void* host, unsigned char* food );
	static DWORD WINAPI LPCThreadProc( LPVOID lpParam );

	void	Reset();
	BYTE	ReadLow ( WORD addr );
	void    WriteLow( WORD addr, BYTE data );
	void	Write(WORD addr, BYTE data);
	void	HSync(INT scanline);
	void	Clock(INT cycles);

protected:
	BYTE	reg5000;
	BYTE	reg5001;
	BYTE	reg5010;
	BYTE	reg5011;
	BYTE	reg5013;
	BYTE	reg5FF1;
	BYTE	reg5FF2;
	BYTE	we_sram;
	BYTE	SBW, sp_rom;

	BYTE	pointer;
	BYTE	mmc3reg[16];
	BYTE	fk23reg[8];
	BYTE	mirroring;
	BYTE	wram;
	BYTE	latch;
	BYTE	reg4800;

	BYTE	counter;
	BYTE	reloadValue;
	BYTE	pa12Filter;
	BOOL	enableIRQ;

private:
	int		getPRGBank(int bank);
	int		getCHRBank(int bank);
	void	sync();
	void	writeReg(int addr, int val);
	void	writeMMC3(int addr, int val);
	void	writeMirroringWRAM(int addr, int val);
	void	writeIRQConfig(int addr, int val);
	void	writeIRQEnable(int addr, int val);
	void	writeHST162(int addr, int val);
	int		readLPC(int addr);
	void	writeLPC(int addr, int val);

	BYTE	count, temp_data;
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
