/*******************************************************************************
 * NES Mapper for PLQ AV Card
 *
 *  Author:  <87430545@qq.com>
 *
 *  2021-06-06 Created
 *  2024-05-25 QLS added
 *******************************************************************************
 */

//#define USE_KVBIN_FILE
//#define PLQ_BS_DEBUG
#define PLQ_USE_AUDIO_SYNC

#define PLQ_BUF_SIZE 16384
#define PLQ_BUF_MASK (PLQ_BUF_SIZE - 1)

class	MapperPLQ : public Mapper
{
public:
	MapperPLQ( NES* parent );
	~MapperPLQ();

	void	Reset();

	void	Clock( INT cycles );
	void	VSync();

	void	WritePAL( WORD addr, BYTE data );

	// $8000-$FFFF Memory write
	void	Write( WORD addr, BYTE data );

	// $8000-$FFFF Memory read
	BOOL	ReadHigh( WORD addr, LPBYTE pdata );

	BOOL	bStopReq;
	HANDLE	hAudioSync;
	INT		nAudioReadIndex;
	INT		nAudioWriteIndex;
	SHORT	sAudioBuf[PLQ_BUF_SIZE];

protected:
private:
	BOOL	bPaused;
	SHORT	sPrevSample;

#ifdef USE_KVBIN_FILE
	FILE*	fpKvBin;
#endif
#ifdef PLQ_BS_DEBUG
	FILE*	fpBs;
#endif

	UINT	nVersion;
	DWORD	nCurCycles;

	BYTE	DATA_IN;
	BYTE	PREV_DATA_IN;

	BYTE	bQTickCount;
	DWORD	nSampleCount;

	BYTE	REG_PAL;
	BYTE	EPRAM[8 * 1024];
	BYTE	EVRAM[8 * 1024];
};

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
