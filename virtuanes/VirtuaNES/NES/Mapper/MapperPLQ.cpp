/*******************************************************************************
 * NES Mapper for PLQ AV Card
 *
 *  Author:  <87430545@qq.com>
 *
 *  2021-06-06 Created
 *  2024-05-25 QLS added
 *******************************************************************************
 */

#include "..\..\s_lib\s_intf.h"
#pragma comment(lib, "s_lib\\s_lib.lib")

#ifdef USE_KVBIN_FILE
#define AUDIO_FS 100000
#else
#define AUDIO_FS 96000
#endif

MapperPLQ::MapperPLQ( NES* parent ) : Mapper(parent)
{
	bStopReq = FALSE;

	hAudioSync = CreateEvent(0, 0, 1, 0);

#ifdef USE_KVBIN_FILE
	fpKvBin = NULL;
#else
	player_init(AUDIO_FS);
#endif
#ifdef PLQ_BS_DEBUG
	fpBs = NULL;
#endif

	nVersion = 1;
}

MapperPLQ::~MapperPLQ()
{
	bStopReq = TRUE;
	player_stop();
	WaitForSingleObject(hAudioSync, -1);
	SetEvent(hAudioSync);

	player_done();
	CloseHandle(hAudioSync);
#ifdef USE_KVBIN_FILE
	if (fpKvBin) fclose(fpKvBin);
#endif
#ifdef PLQ_BS_DEBUG
	if (fpBs) fclose(fpBs);
#endif
}

void MapperPLQ::Reset()
{
	int i;
	LPBYTE pROM;
	BOOL bChanged;
	const char* szUrl;

	DATA_IN = 0;
	REG_PAL = 0xFF;
	PREV_DATA_IN = 0;
	bQTickCount = 0;
	nSampleCount = 0;

	sPrevSample = 0;

	memset(EPRAM, 0, sizeof(EPRAM)); // 8K
	memset(EVRAM, 0, sizeof(EVRAM)); // 8K

	pROM = nes->rom->GetPROM();
	nVersion = nes->rom->GetPROM_SIZE(); // size represent version: 1 or 2

	if (1 == nVersion) {
		// ∆’¿Ô∆Ê Cxxx - Dxxx: Fixed Map
		SetPROM_Bank(4, pROM,           BANKTYPE_ROM); // Dummy 8K
		SetPROM_Bank(5, pROM,           BANKTYPE_ROM); // Dummy 8K
		SetPROM_Bank(6, EPRAM,          BANKTYPE_RAM); // 8K
		SetPROM_Bank(7, pROM + 0x2000,  BANKTYPE_ROM); // 8K
	} else {
		// ∆Ê¿œ ¶ ROM 18K
		SetPROM_Bank(4, EPRAM,          BANKTYPE_RAM);
		SetPROM_Bank(5, 0,              BANKTYPE_MAPPER);
		SetPROM_Bank(6, pROM + 0x4000,  BANKTYPE_ROM);
		SetPROM_Bank(7, pROM + 0x6000,  BANKTYPE_ROM);
	}

	// 2xxx - 3xxx: Fixed Map
	for (i = 0; i < 8; i++)
		SetVROM_Bank(i + 8, EVRAM + 0x0200 * i, BANKTYPE_VRAM); // 1K

	// CI_VRAM_A10 <-> PA11
	SetVRAM_Mirror(VRAM_HMIRROR);

	nCurCycles = 0;

	bStopReq = TRUE;
	player_stop();
	WaitForSingleObject(hAudioSync, -1);
	bStopReq = FALSE;
	SetEvent(hAudioSync);

	szUrl = nes->rom->GetAudioSource(bChanged);
	if (szUrl[0])
	{
#ifdef USE_KVBIN_FILE
		if (fpKvBin) fclose(fpKvBin);
		fpKvBin = fopen(szUrl, "rb");
#else
		player_play(szUrl, this);
#endif
	}

#ifdef PLQ_BS_DEBUG
	fpBs = fopen("PLQ_BS.BIN", "wb");
#endif

	bPaused = FALSE;

	DATA_IN = 0;

	nAudioReadIndex = 0;
	nAudioWriteIndex = 0;
}

void MapperPLQ::VSync()
{
	if (GetAsyncKeyState(VK_HOME) & 1)
	{
		bPaused = !bPaused;
		player_pause(bPaused);
	}

	if (GetAsyncKeyState(VK_PRIOR) & 1)
		player_seek(player_get_pos() - 10000);

	if (GetAsyncKeyState(VK_NEXT) & 1)
		player_seek(player_get_pos() + 10000);
}

extern "C" int player_on_data(short* pcm, int samples, void* cookie)
{
	MapperPLQ* thiz;

	thiz = (MapperPLQ*)cookie;

#ifdef PLQ_USE_AUDIO_SYNC
	if (WAIT_TIMEOUT == WaitForSingleObject(thiz->hAudioSync, 10000))
		return -1;
#endif

	while (samples--)
	{
		short sl = pcm[0];
		short sr = pcm[1];

		pcm[1] = sl; // copy L to R

#ifdef PLQ_USE_AUDIO_SYNC
		while (((thiz->nAudioWriteIndex + 1) & PLQ_BUF_MASK) == thiz->nAudioReadIndex)
		{
			// buffer full
			if (thiz->bStopReq) // user break
			{
				SetEvent(thiz->hAudioSync);
				return -1;
			}

			Sleep(1);
		}

		thiz->sAudioBuf[thiz->nAudioWriteIndex] = sr;
		thiz->nAudioWriteIndex = (thiz->nAudioWriteIndex + 1) & PLQ_BUF_MASK;
#else
		thiz->sAudioBuf[thiz->nAudioWriteIndex] = sr;

		if (((thiz->nAudioWriteIndex + 1) & PLQ_BUF_MASK) == thiz->nAudioReadIndex)
		{
			// drop old one
		}
		else
		{
			thiz->nAudioWriteIndex = (thiz->nAudioWriteIndex + 1) & PLQ_BUF_MASK;
		}
#endif

		pcm += 2;
	}

#ifdef PLQ_USE_AUDIO_SYNC
	SetEvent(thiz->hAudioSync);
#endif

	return 0;
}

// CPU Clock = 26.601712MHz °¬ 16 = 1.662607 MHz
// T = 1 / 1.662607 = 16 / 26601712 = 0.6uS
// 176.4Khz sample rate = 377 / 40T
// 100KHz sample rate = 16.625 T = 133 / 8 T
// 44.1Khz sample rate = 37.698 T = 377 / 10 T = 2375 / 63 T
// 96KHz sample rate = 16626 / 960

#if (AUDIO_FS == 100000)
// 100KHz
#define CYC_SCALE 8
#define FS_IN_T 133
#elif (AUDIO_FS == 192000)
// 192KHz
#define CYC_SCALE 1920
#define FS_IN_T 16626
#elif (AUDIO_FS == 96000)
// 96KHz
#define CYC_SCALE 960
#define FS_IN_T 16626
#elif (AUDIO_FS == 88200)
// 88.2KHz
#define CYC_SCALE 20
#define FS_IN_T 377
#elif (AUDIO_FS == 44100)
// 44.1KHz
#define CYC_SCALE 10
#define FS_IN_T 377
//#define CYC_SCALE 63
//#define FS_IN_T 2375
#else
#error sample rate not supported.
#endif

void MapperPLQ::Clock( INT cycles )
{
	nCurCycles += cycles * CYC_SCALE;

	if (nCurCycles >= FS_IN_T)
	{
		INT nTimeout;
		SHORT sSample = 0;

		nCurCycles -= FS_IN_T;

#ifdef USE_KVBIN_FILE
		if (fpKvBin)
		{
			if (feof(fpKvBin)) rewind(fpKvBin);
			fread(&sSample, 2, 1, fpKvBin);
		}

		DATA_IN = sSample & 1;
#else
		// fetch one sample
#ifdef PLQ_USE_AUDIO_SYNC
		nTimeout = 0;

		while (nAudioWriteIndex == nAudioReadIndex)
		{
			int status;

			status = player_get_status();
			if (status != 0) break; // not playing

			Sleep(1);
			nTimeout++;
			if (nTimeout > 2000) break;
		}
#endif

		if (nAudioWriteIndex == nAudioReadIndex)
		{
			sSample = sPrevSample;
		}
		else
		{
			sSample = sAudioBuf[nAudioReadIndex];
			nAudioReadIndex = (nAudioReadIndex + 1) & PLQ_BUF_MASK;
		}

		DATA_IN = (sSample > -1000) ? 0 : 1;
#endif // USE_KVBIN

		if (2 == nVersion) { // QLS only
			if (PREV_DATA_IN ^ DATA_IN) {
				if (0 == DATA_IN) {
//					char buf[128];

					// falling edge
					BYTE tick;

					tick = (BYTE)(nSampleCount * 1764 / (AUDIO_FS / 100));
//					tick = (BYTE)(nSampleCount * 4);

					bQTickCount += tick;
					nSampleCount = 0;

//					sprintf(buf, "tick = %d\n", tick);
//					OutputDebugStringA(buf);

					nes->cpu->SetIRQ(IRQ_MAPPER);
				} else {
				}
			}

			nSampleCount++;
			PREV_DATA_IN = DATA_IN;
		}
//#endif

#ifdef PLQ_BS_DEBUG
		//if (fpBs) fputc(DATA_IN, fpBs);
		if (fpBs) fwrite(&sSample, 2, 1, fpBs);
#endif

		sPrevSample = sSample;
	}
}

void MapperPLQ::WritePAL( WORD addr, BYTE data )
{
	BYTE D0, D1, D2, D3, D4, D5, D6;
	BYTE Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7;
	BYTE q0, q1, q2, q3, q4, q5, q6, q7;

	D0 = data & 1;
	D1 = (data & 2) ? 1 : 0;
	D2 = (data & 4) ? 1 : 0;
	D3 = (data & 8) ? 1 : 0;
	D4 = (addr & 4) ? 1 : 0;
	D5 = (addr & 2) ? 1 : 0;
	D6 = DATA_IN;

	Q0 = REG_PAL & 1;
	Q1 = (REG_PAL & 2) ? 1 : 0;
	Q2 = (REG_PAL & 4) ? 1 : 0;
	Q3 = (REG_PAL & 8) ? 1 : 0;
	Q4 = (REG_PAL & 0x10) ? 1 : 0;
	Q5 = (REG_PAL & 0x20) ? 1 : 0;
	Q6 = (REG_PAL & 0x40) ? 1 : 0;
	Q7 = (REG_PAL & 0x80) ? 1 : 0;

	q0 =
		(D0 & !D4 & !D5) |
		(D0 & !Q0 & !D4 & D5) |
		(!D0 & Q0 & !D4 & D5) |
		(D0 & D4 & !D5) |
		(Q0 & D4 & !D5) |
		(D0 & Q0 & D4 & D5);
	
	q1 =
		(D1 & !D4 & !D5) |
		(D1 & !Q1 & !D4 & D5) |
		(!D1 & Q1 & !D4 & D5) |
		(D1 & D4 & !D5) |
		(Q1 & D4 & !D5) |
		(!D0 & D1 & Q1 & D4 & D5) |
		(D0 & Q1 & D4 & D5);
	
	q2 =
		(D2 & !D4 & !D5) |
		(D2 & !Q2 & !D4 & D5) |
		(!D2 & Q2 & !D4 & D5) |
		(D2 & D4 & !D5) |
		(Q2 & D4 & !D5) |
		(!D0 & D2 & Q2 & D4 & D5) |
		(D0 & Q2 & D4 & D5);
	
	q3 =
		(D3 & !D4 & !D5) |
		(D3 & !Q3 & !D4 & D5) |
		(!D3 & Q3 & !D4 & D5) |
		(D3 & D4 & !D5) |
		(Q3 & D4 & !D5) |
		(!D0 & D3 & Q3 & D4 & D5) |
		(D0 & Q3 & D4 & D5);
	
	q4 =
		(!D4 & Q4 & !D5) |
		(D3 & !Q3 & !D4 & D5) |
		(!D3 & Q3 & !D4 & D5) |
		(D4 & Q4 & !D5) |
		(D3 & D4 & !D5) |
		(D0 & Q3 & D4 & !Q4 & D5) |
		(D0 & !Q3 & D4 & Q4 & D5) |
		(!D0 & D4 & D5 & D6);

	q5 =
		(!D4 & !D5 & Q5) |
		(D2 & !Q2 & !D4 & D5) |
		(!D2 & Q2 & !D4 & D5) |
		(D4 & !D5 & Q5) |
		(D2 & D4 & !D5) |
		(D0 & Q2 & D4 & D5 & !Q5) |
		(D0 & !Q2 & D4 & D5 & Q5);
	
	q6 =
		(!D4 & !D5 & Q6) |
		(D1 & !Q1 & !D4 & D5) |
		(!D1 & Q1 & !D4 & D5) |
		(D4 & !D5 & Q6) |
		(D1 & D4 & !D5) |
		(D0 & Q1 & D4 & D5 & !Q6) |
		(D0 & !Q1 & D4 & D5 & Q6);

	q7 =
		(!D4 & !D5 & Q7) |
		(D0 & !Q0 & !D4 & D5) |
		(!D0 & Q0 & !D4 & D5) |
		(D4 & !D5 & Q7) |
		(D1 & D4 & !D5) |
		(D0 & Q0 & D4 & D5 & !Q7) |
		(D0 & !Q0 & D4 & D5 & Q7);

	REG_PAL =	q0 |
				(q1 << 1) |
				(q2 << 2) |
				(q3 << 3) |
				(q7 << 4) |
				(q6 << 5) |
				(q5 << 6) |
				(q4 << 7);
}

// $8000-$FFFF Memory write
void MapperPLQ::Write( WORD addr, BYTE data )
{
	if (1 == nVersion) {
		if (addr >= 0xE000)
		{
			// ROM
		}
		else if (addr >= 0xC000)
		{
			// EPRAM
			EPRAM[addr & 0x1FFF] = data;
		}
		else if (addr >= 0xA000)
		{
			// PAL_REG
			// should never be here
		}
		else // 0x8000
		{
			// LATCH
			WritePAL(addr, data);
		}
	} else {
		// 1K ram
		if (addr < 0x9000) {
			EPRAM[addr & 0x03FF] = data;
		}
	}
}

BOOL MapperPLQ::ReadHigh( WORD addr, LPBYTE pdata )
{
	if (1 == nVersion) {
		if (addr >= 0xE000)
		{
			// ROM
			return FALSE;
		}
		else if (addr >= 0xC000)
		{
			// EPRAM
			*pdata = EPRAM[addr & 0x1FFF];
			return TRUE;
		}
		else if (addr >= 0xA000)
		{
			// PAL_REG
			*pdata = REG_PAL;
			return TRUE;
		}
		else // 0x8000
		{
			// LATCH
			// should never be here
			*pdata = 0x00;
			return TRUE;
		}
	} else {
		if (addr < 0x9000) {
			// 1K ram
			*pdata = EPRAM[addr & 0x03FF];
			return TRUE;
		} else if (addr < 0xA000) {
			// tick counter
			nes->cpu->ClrIRQ(IRQ_MAPPER);
			*pdata = bQTickCount;
			return TRUE;
		} else if (addr < 0xB800) {
			// empty
			*pdata = 0;
			return TRUE;
		} else {
			// ROM, using default read
			return FALSE;
		}
	}

	return FALSE;
}
