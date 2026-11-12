/*******************************************************************************
 *  Parallel port printer for SB2K
 *
 *  Author:  fanoble <87430545@qq.com>
 *
 *  Create:   June 27, 2024, by fanoble
 *******************************************************************************
 */

/*
 * signals:
 * reg 0, data port:
 *   data 0-7
 *
 * reg 1, status port:
 *   D7: #busy
 *   D6: ack
 *   D5: paper out
 *   D4: select
 *   D3: error
 *   D2: x
 *   D1: x
 *   D0: x
 *
 * reg 2, control port:
 *   D7: x
 *   D6: x
 *   D5: x
 *   D4: x
 *   D3: #select printer
 *   D2: reset
 *   D1: #line feed
 *   D0: #strobe
 */

class LPT {
public:
	LPT();
	~LPT();

	int HwReset();

// data
	int Write(unsigned char data);

// control
	int Reset(int s);
	int SelectPrinterN(int s);
	int LineFeedN(int s);
	int StrobeN(int s);

// status
	int BusyN();
	int Ack();
	int PaperOut();
	int Select();
	int Error();

// event handler
	int OnHSync();

protected:
	int m_bInitialized;

	int m_bReset;
	int m_bSelectPrinter;
	int m_bLineFeed;
	int m_bStrobe;

	int m_nSyncCount; // as watch dog

#define LPT_DS_IDLE  0
#define LPT_DS_ESC   1
#define LPT_DS_PARAM 2
#define LPT_DS_DATA  3
	int m_nDataState;
	int m_nEscCmd;
	int m_nDataLength;
	int m_nWidth;
	int m_nHeight;
	LPBYTE m_pbCmdParam;
	BYTE m_bCmdParams[32];
private:
};

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
