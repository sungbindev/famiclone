/*******************************************************************************
 * NES Mapper for SB2K
 *
 *  Author:  <87430545@qq.com>
 *
 *  Create:   2024-05-16, by fanoble
 *******************************************************************************
 */

#include <assert.h>
#include <stdio.h>

#include "LPC_D6_SYNTH.H"

#define DAC_CMD_NULL		0
#define DAC_CMD_OPEN		1
#define DAC_CMD_SAMPLE		2
#define DAC_CMD_CLOSE		3

//#define DUMP_DAC_DATA
//#define DUMP_LPC_DATA
//#define LPC_DRYRUN

//------------------------------------------------------------------------------

// TSP50C04
// https://www.ti.com/lit/ml/spss011d/spss011d.pdf

// Connection:
// /INIT: EIO6
// PB0: Mapper
// PB1: IOW (4080-43xx)

#ifdef DUMP_LPC_DATA
static FILE* fp_lpc = 0;
#endif

// 调试开关：非 0 时语音与 DAC 不按声卡的真实时间播放，解码出的采样直接丢弃，写入端与解码线程也不再睡眠等待；默认 0
volatile LONG MapperSB2K::s_nAudioFast = 0;

// 描述：设置声音最快播放开关，供调试服务在开机阶段跳过开机音乐与语音的播放时间。
// 参数：on 非 0 为最快播放，0 为真实时间播放。
void MapperSB2K::SetAudioFast(BOOL on)
{
	::InterlockedExchange(&s_nAudioFast, on ? 1 : 0);
}

int MapperSB2K::FeedLPC(void* host, unsigned char* food)
{
	int ret;
	MapperSB2K* thiz = (MapperSB2K*)host;

	ret = LPC_CMD_NONE;

//	::WaitForSingleObject(thiz->reqLPC, INFINITE);

	int spin = 0;

	while (thiz->nSPReadPos == thiz->nSpWritePos) {
		if (!thiz->bRunLPC) {
			ret = LPC_CMD_STOP;
			break;
		} else if (thiz->nResetLPC) {
			ret = LPC_CMD_RESET;
			thiz->nResetLPC = FALSE;
			break;
		}

		// 最快播放时先让出时间片自旋等数据，久等不到再每次睡 1ms，避免空闲时占满一个核
		if (s_nAudioFast)
			Sleep(++spin < 5000 ? 0 : 1);
		else
			Sleep(10);
	}

	if (LPC_CMD_NONE == ret) {
		*food = thiz->nSPData[thiz->nSPReadPos];
		thiz->nSPReadPos = (thiz->nSPReadPos + 1) & (SP_BUF_SIZE - 1);
		ret = LPC_CMD_PAYLOAD;
	} else {
		::SetEvent(thiz->ackLPC);
	}

	return ret;
}

DWORD WINAPI MapperSB2K::LPCThreadProc(LPVOID lpParam)
{
	void*    synLPC;
	CWaveOut wavLPC;
	short    pcmBuf[1024]; // should be enough

	MapperSB2K* thiz = (MapperSB2K*)lpParam;

	wavLPC.Open(10000, 1); // 10Khz Mono

	synLPC = lpc_d6_synth_new(FeedLPC, thiz, LPC_STD_VARIANT_Subor);

	while (thiz->bRunLPC) {
		int eos;
		int pcm_size;
		int restart = 0;

		short* pcm;

		eos = lpc_d6_synth_do(synLPC, pcmBuf, &pcm_size, &restart);

		if (eos)
			thiz->m_bSpeechEnd = TRUE;

		if (restart) {
			thiz->nSPReadPos = thiz->nSpWritePos; // clear ring buf
			thiz->m_bSpeechEnd = FALSE;
		}

		pcm = pcmBuf;
		pcm_size *= sizeof(short);

		while (pcm_size > 1) {
			int consumed;

			// 最快播放时不送声卡，视为全部播完
			consumed = s_nAudioFast ? pcm_size : wavLPC.WriteNotWait(pcm, pcm_size);
			pcm += consumed / 2;
			pcm_size -= consumed;

			// command response
			if (!thiz->bRunLPC) {
				::SetEvent(thiz->ackLPC);
				break;
			}

			if (thiz->nResetLPC) {
				thiz->nResetLPC = FALSE;
				lpc_d6_synth_reset(synLPC);
				::SetEvent(thiz->ackLPC);
			}

			if (0 == consumed)
				Sleep(10);
		}

		if (eos && !s_nAudioFast)
			wavLPC.Drain();
	}

	lpc_d6_synth_delete(synLPC);

	wavLPC.Drain();
	wavLPC.Close();

	return 0;
}

//------------------------------------------------------------------------------

static const int rate_table[] = {
	8000, 11025, 16000, 22050, 32000, 44100, 48000
};

#ifdef DUMP_DAC_DATA
static FILE* fp_pcm = 0;
#endif

// eliminate pop noise
// 00-7F x 8 : fadein
// 7F-01 x 8 : fadeout

#define DAC_PRE_BUF_SIZE    2048
#define DAC_FADE_THRESHOLD   512

typedef struct {
	int holds;

	int init;
	int repeat;
	int dir;
	int peak;
	int r_pos;
	unsigned char prev_data;

	unsigned char buf[DAC_PRE_BUF_SIZE];
} dac_pre_proc_t;

static int dpp_init(dac_pre_proc_t* dpp)
{
	dpp->holds = 0;

	dpp->init = 1;
	dpp->repeat = 0;
	dpp->prev_data = 0;

	dpp->dir = 0;
	dpp->peak = 0;
	dpp->r_pos = 0;

	return 0;
}

static int dpp_put_data(dac_pre_proc_t* dpp, unsigned char data)
{
	if (dpp->holds == DAC_PRE_BUF_SIZE)
		return -1;

	if (dpp->peak)
		return -2;

	dpp->buf[dpp->holds] = data;

	if (dpp->init) {
		dpp->init = 0;
		dpp->repeat = 0;
	} else {
		if (data == dpp->prev_data) {
			dpp->repeat++;
		} else {
			int new_dir;

			new_dir = (data > dpp->prev_data) ? 1 : -1;

			if (0 == dpp->dir) {
				// 1st peak, ignore it
				dpp->dir = new_dir;
			} else if (dpp->dir != new_dir) {
				dpp->dir = new_dir;
				dpp->peak = 1;
				dpp->r_pos = 0;
			}

			dpp->repeat = 0;
		}
	}

	dpp->prev_data = data;
	dpp->holds++;

	return 0;
}

static int dpp_get_data(dac_pre_proc_t* dpp, unsigned char* data)
{
	if (dpp->peak == 0)
		return -1;

	if (dpp->holds > DAC_FADE_THRESHOLD) {
		// fade? throw them
		dpp->peak = 0;
		dpp->holds = 0;
		dpp->r_pos = 0;
		return -2;
	}

	if (dpp->r_pos >= DAC_PRE_BUF_SIZE)
		dpp->r_pos = dpp->r_pos;

	*data = dpp->buf[dpp->r_pos++];

	if (1 == dpp->holds) {
		// do not touch while 1 data left
		dpp->peak = 0;
	} else if (dpp->r_pos == dpp->holds - 1) {
		dpp->buf[0] = dpp->buf[dpp->holds - 1]; // move to head
		dpp->peak = 0;
		dpp->holds = 1;
		dpp->r_pos = 0;
	}

	return 0;
}

static int dpp_darin(dac_pre_proc_t* dpp, unsigned char* data)
{
	if (dpp->holds == 0)
		return -1;

	if (dpp->holds > DAC_FADE_THRESHOLD) {
		// fade? throw them
		dpp->peak = 0;
		dpp->holds = 0;
		dpp->r_pos = 0;
		return -2;
	}

	*data = dpp->buf[dpp->r_pos++];

	if (dpp->r_pos == dpp->holds) {
		dpp->peak = 0;
		dpp->holds = 0;
		dpp->r_pos = 0;
	}

	return 0;
}

DWORD WINAPI MapperSB2K::DACThreadProc(LPVOID lpParam)
{
//	void*    synDAC;
	CWaveOut wavDAC;
	short    pcmBuf[1024]; // should be enough
	int      nr_samples = 0;
	int      playing = 0;

	dac_pre_proc_t dpp;

	MapperSB2K* thiz = (MapperSB2K*)lpParam;

	dpp_init(&dpp);

	for (;;) {
//		::WaitForSingleObject(thiz->reqDAC, INFINITE);

		if (!thiz->bRunDAC) {
			::SetEvent(thiz->ackDAC);
			break;
		}

		switch (thiz->m_nDACCmd) {
			case DAC_CMD_OPEN:
				// 11025 : 1662607 / 163
				// 22050 : 1662607 / 82
				// 44100 : 1662607 / 41

				// match rate
				{
					int min_diff = 1000000;
					int min_rate = 8000;
					for (int i = 0; i < sizeof(rate_table) / sizeof(rate_table[0]); i++) {
						int diff = thiz->m_nDACRate - rate_table[i];
						if (diff < 0) diff = -diff;
						if (diff < min_diff) {
							min_diff = diff;
							min_rate = rate_table[i];
						}
					}

					wavDAC.Open(min_rate, 1);
				}

				dpp_init(&dpp);
				nr_samples = 0;

				thiz->m_nDACCmd = DAC_CMD_NULL;
				::SetEvent(thiz->ackDAC);
				playing = 1;
				break;
			case DAC_CMD_SAMPLE:
				if (0 != dpp_put_data(&dpp, thiz->m_bDACData)) {
					//printf("put error.\n");
				}

				{
					BYTE data;
					while (0 == dpp_get_data(&dpp, &data)) {
						pcmBuf[nr_samples++] = (data ^ 0x80) << 8;
						if (nr_samples == 512) {
							if (!s_nAudioFast) wavDAC.Write(pcmBuf, 1024);
							nr_samples = 0;
						}
					}
				}

				// 只在仍是 SAMPLE 时清零：处理期间主线程可能已改发 CLOSE（复位），直接清零会丢掉它，主线程就永远等不到 ackDAC
				::InterlockedCompareExchange((volatile LONG*)&thiz->m_nDACCmd, DAC_CMD_NULL, DAC_CMD_SAMPLE);
//				::SetEvent(thiz->ackDAC);
				break;
			case DAC_CMD_CLOSE:
				if (playing) {
					BYTE data;
					while (0 == dpp_darin(&dpp, &data)) {
						pcmBuf[nr_samples++] = (data ^ 0x80) << 8;
						if (nr_samples == 512) {
							if (!s_nAudioFast) wavDAC.Write(pcmBuf, 1024);
							nr_samples = 0;
						}
					}

					if (!s_nAudioFast) wavDAC.Drain();
					wavDAC.Close();

					playing = 0;
				}
				thiz->m_nDACCmd = DAC_CMD_NULL;
				::SetEvent(thiz->ackDAC);
				break;
			default:
				if (!playing) Sleep(50);
				break;
		}
	}

	wavDAC.Close();

	return 0;
}

//------------------------------------------------------------------------------

void MapperSB2K::Init()
{
	int i;
	DWORD dwThreadID;

	nes->ppu->SetVariant(PPU::PPUVARIANT_UM6576);

	// default program bank
	for (i = 0; i < 8; i++)
		m_bPbank[i] = i;

	m_bVbank = 0;

	m_bRamBoot = FALSE;

	nSPReadPos = nSpWritePos = 0;

	m_bSpeechReady   = FALSE;
	m_bSpeechEnd     = FALSE;
	m_nSpeechPadding = 0;

	bRunLPC = TRUE;
	bRunDAC = TRUE;

	m_nDACCmd = DAC_CMD_NULL;

#ifdef DUMP_DAC_DATA
	fp_pcm = fopen("dump.pcm", "wb");
#endif

#ifdef DUMP_LPC_DATA
	fp_lpc = fopen("dump.lpc", "wb");
#endif

	reqLPC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ackLPC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	reqDAC = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	ackDAC = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	::ResetEvent(reqLPC);
	::ResetEvent(ackLPC);
	::ResetEvent(reqDAC);
	::ResetEvent(ackDAC);

	thdLPC = ::CreateThread(NULL, 0, LPCThreadProc, this, 0, &dwThreadID);
	thdDAC = ::CreateThread(NULL, 0, DACThreadProc, this, 0, &dwThreadID);
}

MapperSB2K::~MapperSB2K()
{
	bRunLPC = FALSE;

	bRunDAC = FALSE;
//	::SetEvent(reqDAC);

	::WaitForSingleObject(ackLPC, INFINITE);
	::WaitForSingleObject(thdLPC, INFINITE);

	::WaitForSingleObject(ackDAC, INFINITE);
	::WaitForSingleObject(thdDAC, INFINITE);

	CloseHandle(reqLPC);
	CloseHandle(ackLPC);
	CloseHandle(thdLPC);

	CloseHandle(reqDAC);
	CloseHandle(ackDAC);
	CloseHandle(thdDAC);
#ifdef DUMP_DAC_DATA
	if (fp_pcm) {
		fclose(fp_pcm);
		fp_pcm = 0;
	};
#endif
#ifdef DUMP_LPC_DATA
	if (fp_lpc) {
		fclose(fp_lpc);
		fp_lpc = 0;
	};
#endif
}

//------------------------------------------------------------------------------

//WRAM 2k/128K
//CRAM 10k/32K

// CPU MEMORY MAP
// 0000-0FFF WORKING RAM 4K
// 1000-1FFF WORKING RAM 4K
// 2000-203F GRAPHIC PORT
// 2040-3FFF NULL
// 4000-4FFF PSG & PERIPHERAL PORT (/IOW & /IOR signal : 4080-43xx)
// 5000-57FF WORKING RAM 2K
// 6000-6FFF EXTERNAL WORKING RAM 4K (selected by /EXRAM pin)
// 7000-7FFF EXTERNAL WORKING RAM 4K (selected by /EXRAM pin)
// 8000-8FFF PROGRAM BANK 0
// 9000-9FFF PROGRAM BANK 1
// A000-AFFF PROGRAM BANK 2
// B000-BFFF PROGRAM BANK 3
// C000-CFFF PROGRAM BANK 4
// D000-DFFF PROGRAM BANK 5
// E000-EFFF PROGRAM BANK 6
// F000-FFFF PROGRAM BANK 7

// GPU MEMORY MAP
// 0000-0FFF VIDEO RAM 4K
// 1000-1FFF VIDEO RAM 4K
// 2000-27FF VIDEO RAM 2K
// 2800-3FFF NULL
// 4000-4FFF NULL
// 5000-57FF NULL
// 6000-6FFF NULL
// 7000-7FFF NULL
// 8000-8FFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// 9000-9FFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// A000-AFFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// B000-BFFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// C000-CFFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// D000-DFFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// E000-EFFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
// F000-FFFF EXTENT PATTERN VROM/VRAM (/XVRAM_CS)
void MapperSB2K::Reset()
{
	BIOS = nes->rom->GetPROM();

	// external
	memset(EPRAM, 0, sizeof(EPRAM));
	memset(EVRAM, 0, sizeof(EVRAM));

	int i;

	// default program bank
	for (i = 0; i < 8; i++)
		m_bPbank[i] = i;

	m_bVbank   = 0;
	m_bRamBoot = FALSE;

	m_bC3_IRQEnable    = 0;
	m_bC3_IRQRequest   = 0;
	m_bC3_IRQLatch     = 0;
	m_bC3_IRQCounter   = 0;
	m_bC3_IRQPreset    = 0;
	m_bC3_IRQPresetVBL = 0;

	//nes->SetVariant(NES::NESVARIANT_UM6576);

	// 8xxx - Fxxx, default mapping, just for safety
	SetPROM_Bank(4, BIOS + 0x0000, BANKTYPE_ROM); // 8-9 8K
	SetPROM_Bank(5, BIOS + 0x2000, BANKTYPE_ROM); // A-B 8K
	SetPROM_Bank(6, BIOS + 0x4000, BANKTYPE_ROM); // C-D 8K
	SetPROM_Bank(7, BIOS + 0x6000, BANKTYPE_ROM); // E-F 8K

	// default mapping for MMC3
	SetVROM_Bank( 0, EVRAM + 0x0000, BANKTYPE_VROM); // 1K
	SetVROM_Bank( 1, EVRAM + 0x0400, BANKTYPE_VROM); // 1K
	SetVROM_Bank( 2, EVRAM + 0x0800, BANKTYPE_VROM); // 1K
	SetVROM_Bank( 3, EVRAM + 0x0C00, BANKTYPE_VROM); // 1K

	SetVROM_Bank( 4, EVRAM + 0x1000, BANKTYPE_VROM); // 1K
	SetVROM_Bank( 5, EVRAM + 0x1400, BANKTYPE_VROM); // 1K
	SetVROM_Bank( 6, EVRAM + 0x1800, BANKTYPE_VROM); // 1K
	SetVROM_Bank( 7, EVRAM + 0x1C00, BANKTYPE_VROM); // 1K

	SetVROM_Bank( 8, CRAM + 0x2000, BANKTYPE_CRAM); // 1K
	SetVROM_Bank( 9, CRAM + 0x2400, BANKTYPE_CRAM); // 1K

	SetVROM_Bank(10, CRAM + 0x2000, BANKTYPE_CRAM); // 1K
	SetVROM_Bank(11, CRAM + 0x2400, BANKTYPE_CRAM); // 1K

//------------------------------------------------------------------------------

	nMode6576 = 0;

	m_bRegKbdIn  = 0xFF;
	m_bRegKbdOut = 0x00;
//	m_bReg4022   = 0x80; // clk=1 dir=0(read kbd)
	m_bReg4022   = 0x00;
	m_bReg4023   = 0x00;
	m_bReg4024   = 0x00;
	m_bReg4026   = 0x00;
	m_bReg4032   = 0xFF; // disable all interrupts
	m_bReg4033   = 0x00;
	m_bReg4034   = 0x00;
	m_bReg4035   = 0x00;
	m_bReg4036   = 0x00;

	m_bTimerPrescaleCnt = 0;

	kbd_send_irq_delay = 0;

	m_bDACOn   = FALSE;
	m_nDACRate = 0;

	m_nDACCmd = DAC_CMD_CLOSE;
	::WaitForSingleObject(ackDAC, INFINITE);

	// 软驱：只有 0 号驱动器；DOR 清 0（电机停、控制器复位）
	nes->fdc->writeIO(2, 0);
	nes->fdc->setDriveMask(1);
	nes->fdc->reset();
	m_bFdcIrq = FALSE;

	bDiskAccess  = 0;

//------------------------------------------------------------------------------

	nResetLPC = 0;
	nSPReadPos = nSpWritePos = 0;

//------------------------------------------------------------------------------

	nes->SetVideoMode( 2 );

	// set ExPad
	nes->pad->SetExController(PAD::EXCONTROLLER_SB2K);
}

//------------------------------------------------------------------------------

BYTE MapperSB2K::Peek(INT addr, BOOL is_logical)
{
	BYTE data = 0xEE;

	if (addr < 0x2000) {
		data = RAM[addr]; // 8K
	} else if (addr >= 0x5000 && addr < 0x5800) {
		data = RAM[addr & 0x7FF]; // 2K
	} else if (addr >= 0x6000 && addr < 0x8000) {
		data = ERAM[addr & 0x1FFF]; // 8K
	} else if (addr >= 0x8000) {
		BYTE bank;

		bank = m_bPbank[(addr >> 12) & 7];

		if (bank < 0x80)
			data = BIOS[bank * 0x1000 | (addr & 0x0FFF)];
		else
			data = EPRAM[(bank & 0x7F) * 0x1000 | (addr & 0x0FFF)];
	}

	return data;
}

BYTE MapperSB2K::PPU_Peek(INT addr, BOOL is_logical)
{
	BYTE data = 0xEE;

	if (0 == nMode6576) {
		if (addr >= 0x3000) return data;
		return PPU_MEM_BANK[addr >> 10][addr & 0x3FF];
	}

//	if (addr < 0x2800) {
	if (addr < 0x8000) {
		data = CRAM[addr]; // 10K
	} else if (addr >= 0x8000) {
		data = EVRAM[m_bVbank * 0x8000 + (addr & 0x7FFF)];
	}

	return data;
}

BYTE MapperSB2K::ExCmdRead( EXCMDRD cmd )
{
	if (cmd == Mapper::EXCMDRD_DISKACCESS)
		return bDiskAccess;

	return 0;
}

// for debug
LPBYTE MapperSB2K::GetExtPMEM( int addr )
{
	return 0;
}

LPBYTE MapperSB2K::GetExtVMEM( int addr )
{
	if (addr >= 0x8000)
		return 0;

	return EVRAM + (addr & 0x7FFF);
}

LPBYTE MapperSB2K::GetPatternTable( int scanline, int addr )
{
	if (addr >= 0x2000 || scanline >= 240)
		return 0;

	// default
	return 0;
}

INT MapperSB2K::GetBankNo( int addr )
{
	if (addr < 0x8000)
		return 0;

	return m_bPbank[(addr >> 12) & 7];
}

BOOL MapperSB2K::IsThrottleRequired()
{
	// playing PCM raw data need more speed
	return m_bDACOn;
}

// 描述：把 CPU 地址解析为物理空间，供调试服务标记代码与数据。
// 参数：addr CPU 地址；off 返回物理空间内偏移。
// 返回：0 为 BIOS 只读存储器，1 为扩展程序 RAM，-1 为不在这两者内。
INT MapperSB2K::DbgResolve(WORD addr, INT& off)
{
	INT biosSize = nes->rom->GetPROM_SIZE() * 0x4000;

	if (addr < 0x8000)
		return -1;

	if (m_bRamBoot) {
		// 兼容 FC 卡带模式：读取走 MMC3 重映射后的 8K 页指针
		LPBYTE p = CPU_MEM_BANK[addr >> 13] + (addr & 0x1FFF);

		if (p >= EPRAM && p < EPRAM + sizeof(EPRAM)) {
			off = (INT)(p - EPRAM);
			return 1;
		}
		if (p >= BIOS && p < BIOS + biosSize) {
			off = (INT)(p - BIOS);
			return 0;
		}
		return -1;
	}

	BYTE bank = m_bPbank[(addr >> 12) & 7];

	if (bank < 0x80) {
		off = (bank * 0x1000) | (addr & 0x0FFF);
		return (off < biosSize) ? 0 : -1;
	}

	off = ((bank & 0x7F) * 0x1000) | (addr & 0x0FFF);
	return 1;
}

// 描述：按名字取物理空间。
// 参数：name 空间名；size 返回空间字节数。
// 返回：空间起始指针，名字未知时返回空指针。
LPBYTE MapperSB2K::DbgSpace(const char* name, INT& size)
{
	if (0 == strcmp(name, "rom")) {
		size = nes->rom->GetPROM_SIZE() * 0x4000;
		return BIOS;
	}
	if (0 == strcmp(name, "pram")) { size = sizeof(EPRAM); return EPRAM; }
	if (0 == strcmp(name, "vram")) { size = sizeof(EVRAM); return EVRAM; }
	if (0 == strcmp(name, "cram")) { size = sizeof(CRAM);  return CRAM; }
	if (0 == strcmp(name, "eram")) { size = 0x2000;        return ERAM; }
	if (0 == strcmp(name, "wram")) { size = 0x800;         return WRAM; }
	if (0 == strcmp(name, "ram"))  { size = 0x2000;        return RAM; }

	size = 0;
	return 0;
}

// 描述：把 bank 寄存器与扩展寄存器状态写成一行文本。
// 参数：buf 输出缓冲；cap 缓冲字节数。
// 返回：无。
void MapperSB2K::DbgInfo(char* buf, size_t cap)
{
	_snprintf(buf, cap,
		"mode6576=%d ramboot=%d pbank=%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X vbank=%02X ebank=%02X "
		"r4022=%02X r4023=%02X r4026=%02X r4032=%02X r4033=%02X r4034=%02X r4035=%02X r4036=%02X "
		"dma=%02X,%02X,%02X,%02X,%02X,%02X,%02X,%02X",
		nMode6576, m_bRamBoot,
		m_bPbank[0], m_bPbank[1], m_bPbank[2], m_bPbank[3], m_bPbank[4], m_bPbank[5], m_bPbank[6], m_bPbank[7],
		m_bVbank, m_bEBank,
		m_bReg4022, m_bReg4023, m_bReg4026, m_bReg4032, m_bReg4033, m_bReg4034, m_bReg4035, m_bReg4036,
		m_bDmaReg[0], m_bDmaReg[1], m_bDmaReg[2], m_bDmaReg[3], m_bDmaReg[4], m_bDmaReg[5], m_bDmaReg[6], m_bDmaReg[7]);
	buf[cap - 1] = 0;
}

//==============================================================================

// $4000-$40FF Extention register read
BYTE MapperSB2K::ExRead ( WORD addr )
{
	BYTE data = 0;

	switch (addr) {
		case 0x4020: // Keyboard input port
			data = m_bRegKbdIn;
			break;

		case 0x4022: // Keyboard data control
			data = m_bReg4022;
			break;

		case 0x4023: // 0:Joystick or 1:Mouse
			data = m_bReg4023 & 1;
			break;

		case 0x4024: // Mouse data
			data = m_bReg4024;
			break;

		case 0x4026: // External I/O
			data = m_bReg4026; // EIO01 : Mouse reset(HT6513B_RTS)

			if (!nes->lpt->Ack())
				data &= ~0x08;

			if (!nes->lpt->PaperOut())
				data &= ~0x10;

			if (!nes->lpt->Select())
				data &= ~0x20;

			break;

		case 0x4032: // IRQ mask & clean
			data = m_bReg4032;
			break;

		case 0x4033: // IRQ status
			data = m_bReg4033;
			break;

		case 0x4034: // Timer control
			data = m_bReg4034;
			break;

		case 0x4036: // Timer present value
			data = m_bReg4036;
			break;

		default:
			if (addr >= 0x4040 && addr < 0x4048) {
				// program bank select
				data = m_bPbank[addr & 7];
			} else if (addr >= 0x4048 && addr < 0x4050) {
				data = m_bDmaReg[addr & 7];
			}
			break;
	}

	return data;
}

//------------------------------------------------------------------------------

// from NES::Read 4100 - 7FFF
BYTE MapperSB2K::ReadLow(WORD addr)
{
	bDiskAccess = 0;

	if (addr < 0x4400) {
		BYTE data = 0;

		// super IO
		if (addr >= 0x4200 && addr < 0x4208) {
			// FDD：端口 0、1 是 DMA 数据（1 同时给出终止计数），2 的位 6 是 DRQ，3 的位 6 是 IRQ，4、5、7 同标准 PC 软驱控制器
			bDiskAccess = 1;
			switch (addr & 7) {
				case 0:
					return nes->fdc->receiveFromDMA();
				case 1:
					nes->fdc->setTerminalCount();
					return nes->fdc->receiveFromDMA();
				case 2:
					return nes->fdc->haveDMARequest() ? 0x40 : 0x00;
				case 6:
					return 0;
				default:
					return nes->fdc->readIO(addr & 7);
			}
		}

		switch (addr) {
			case 0x4302: // speech port
#ifdef LPC_DRYRUN
				data = 0x80; // ready
#else
				if (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
					data = 0x00; // buffer full, busy
				else
					data = 0x80; // ready
#endif

				data |= m_bSpeechEnd ? 0x0F : 0; // end
				break;
			default:
				break;
		}

		return data;
	} else if (addr < 0x5000)
		return (addr >> 8); // empty
	else if (addr < 0x6000)
		return WRAM[addr & 0x7FF]; // 2K
	else
		return ERAM[addr & 0x1FFF]; // 8K
}

static int dbg_dump = 0;

static void dbg_dump_mem(void* ivram, void* evram, void* ipram, void* epram)
{
	FILE* fp;

	fp = fopen("CHR_RAM.BIN", "wb");
	if (fp) {
		fwrite(ivram, 1024, 32, fp);
		fclose(fp);
	}

	fp = fopen("CHR_DRAM.BIN", "wb");
	if (fp) {
		fwrite(evram, 1024, 512, fp);
		fclose(fp);
	}

	fp = fopen("PRG_RAM.BIN", "wb");
	if (fp) {
		fwrite(ipram, 1024, 8, fp);
		fclose(fp);
	}

	fp = fopen("PRG_DRAM.BIN", "wb");
	if (fp) {
		fwrite(epram, 1024, 512, fp);
		fclose(fp);
	}

	dbg_dump = 0;
}

// from CPU::RD6502 8000 - FFFF
// Quick bank read after return false
BOOL MapperSB2K::ReadHigh(WORD addr, LPBYTE pdata)
{
	int index;
	BYTE bank;

	if (dbg_dump)
		dbg_dump_mem(CRAM, EVRAM, ERAM, EPRAM);

	index = addr >> 12;
	bank = m_bPbank[index & 7];

	if (m_bRamBoot) {
//		int base = m_bEBank * 0x20000;
//		*pdata = EPRAM[base + 0x18000 + (addr & 0x7FFF)];
		*pdata = CPU_MEM_BANK[addr >> 13][addr & 0x1FFF];
	} else if (bank < 0x80)
		*pdata = BIOS[bank * 0x1000 | (addr & 0x0FFF)];
	else
		*pdata = EPRAM[(bank & 0x7F) * 0x1000 | (addr & 0x0FFF)];

	return TRUE;
}

//==============================================================================

// from NES::Write
// 2000 - 3FFF
// 4100 - 7FFF
void MapperSB2K::WriteLow(WORD addr, BYTE data)
{
	// 2000-3FFF
	if (addr < 0x4000) {
		// GRAPHIC
	} else if (addr < 0x4400) {
		// 软驱的 $4100 写数据率选择、$4101 写 DOR（电机、复位、DMA 与中断允许），对应标准 PC 软驱控制器的 $3F7、$3F2
		if (addr == 0x4100 || addr == 0x4101) {
			bDiskAccess = 1;
			nes->fdc->writeIO(addr == 0x4100 ? 7 : 2, data);
			return;
		}

		// Super IO
		if (addr >= 0x4200 && addr < 0x4208) {
			// FDD：端口 0、1 写 DMA 数据（1 同时给出终止计数），4 的位 6 是控制器复位脚，2、5、7 同标准 PC 软驱控制器
			bDiskAccess = 1;
			switch (addr & 7) {
				case 0:
					nes->fdc->sendToDMA(data);
					break;
				case 1:
					nes->fdc->setTerminalCount();
					nes->fdc->sendToDMA(data);
					break;
				case 4:
					nes->fdc->setResetPin(!!(data & 0x40));
					break;
				case 2:
				case 5:
				case 7:
					nes->fdc->writeIO(addr & 7, data);
					break;
				default:
					break;
			}
			return;
		}

		switch (addr) {
			case 0x4300: // vram_bank
				m_bVbank = data & 15;
				break;
			case 0x4301:
				if (data == 0x55) {
					// hw reset???
					nMode6576 = 0;
					m_bRamBoot = TRUE;

					m_bC3_IRQEnable    = 0;
					m_bC3_IRQRequest   = 0;
					m_bC3_IRQLatch     = 0;
					m_bC3_IRQCounter   = 0;
					m_bC3_IRQPreset    = 0;
					m_bC3_IRQPresetVBL = 0;

					// reset 6576
					m_bRegKbdIn  = 0xFF;
					m_bRegKbdOut = 0x00;
					m_bReg4022   = 0x00;
					m_bReg4023   = 0x00;
					m_bReg4024   = 0x00;
					m_bReg4026   = 0x00;
					m_bReg4032   = 0xFF; // disable all interrupts
					m_bReg4033   = 0x00;
					m_bReg4034   = 0x00;
					m_bReg4035   = 0x00;
					m_bReg4036   = 0x00;

					m_bTimerPrescaleCnt = 0;

					kbd_send_irq_delay = 0;

					m_bDACOn   = FALSE;
					m_nDACRate = 0;

					nes->pad->SetExController(PAD::EXCONTROLLER_NONE);

					nes->SetVariant(NES::NESVARIANT_STD);
					nes->ppu->SetVariant(PPU::PPUVARIANT_UM6576);
				}
				break;
			case 0x4302: // speech port
//				if (m_bSpeechEnd && 0x0F == data) {
					// restart

//					nResetLPC = TRUE;
//					::WaitForSingleObject(ackLPC, INFINITE);

//					m_bSpeechReady   = TRUE;
//					m_bSpeechEnd     = FALSE;

//					break;
//				}
#ifdef LPC_DRYRUN
				if (0 == data) {
					m_nSpeechPadding++;
					if (m_nSpeechPadding > 32)
						m_bSpeechEnd = TRUE;
				} else {
					m_nSpeechPadding = 0;
					m_bSpeechEnd = FALSE;
				}

#else
				while (((nSpWritePos + 1) & (SP_BUF_SIZE - 1)) == nSPReadPos)
					Sleep(s_nAudioFast ? 0 : 10); // should not be here

				nSPData[nSpWritePos] = data;
				nSpWritePos = (nSpWritePos + 1) & (SP_BUF_SIZE - 1);
#endif

#ifdef DUMP_LPC_DATA
				if (fp_lpc) fwrite(&data, 1, 1, fp_lpc);
#endif

				if (0) {
					char s[64];
					sprintf(s, "Speech: %02X\n", data);
					OutputDebugStringA(s);
				}
				break;
			default:
				if (1) {
					char s[64];
					sprintf(s, "W %02X -> [%04X]\n", data, addr);
					//OutputDebugStringA(s);
				}
				break;
		}
	} else if (addr < 0x5000) {
		// PSG & PERIPHERAL
		if (1) {
			char s[64];
			sprintf(s, "W %02X -> [%04X]\n", data, addr);
			//OutputDebugStringA(s);
		}
	} else if (addr < 0x6000) {
		if (addr == 0x5006) {
			// 55 77 DD FF
			m_bEBank = ((data >> 2) & 2) | ((data >> 1) & 1);

			m_bC3_IRQEnable = 0;
			nes->cpu->ClrIRQ(IRQ_MAPPER);

			MMC3_Remap();
		}
		WRAM[addr & 0x7FF] = data; // 2K
	} else { // 6000-7FFF
		ERAM[addr & 0x1FFF] = data; // 8K
	}

	bDiskAccess = 0;
}

// $4000-$40FF Extention register write
void MapperSB2K::ExWrite( WORD addr, BYTE data )
{
	switch (addr) {
		case 0x4020:
			m_bTimingSettingControl = data;
			break;

		case 0x4021: // Keyboard output port
			m_bRegKbdOut = data;
			break;

		case 0x4022: // Keyboard data control
			if ((m_bReg4022 ^ data) & 0x80) { // clk changed
				if (m_bReg4022 & 0x80) {
					// falling edge
					; // to do: add counter
				} else {
					// rising edge
					if (data & 1) {// dir=1: wr kbd
						// start write
						nes->pad->ExWrite(0, m_bRegKbdOut);

						if (data & 4) {
							// gen irq after 1ms
							kbd_send_irq_delay = 1663; // = mclk(1662607) / 1000
						}
					}
				}
			}
			m_bReg4022 = data;
			break;

		case 0x4023: // 0:Joystick or 1:Mouse
			m_bReg4023 = data & 1;
			break;

/*
 * 
 * https://en.wikipedia.org/wiki/Parallel_port
 * 
 * input
 *  error:     $4017.D2
 * #busy:      $4017.D3
 *  ack:       EIO3
 *  paper out: EIO4
 *  select:    EIO5
 * 
 * output
 * #Strobe:          EIO2
 * #select printer:  OUT0
 * #Linefeed:        OUT1
 *  reset:           OUT2
 * 
 * 0: data port
 * 8 bits data
 * 
 * 1: status port
 * #busy, ack, paper out, select, error, 0, 0, 0
 * 
 * 2: control port
 * x, x, x, x, #select printer, reset, #Linefeed, #Strobe
 *
 */

		case 0x4025:
			nes->lpt->Write(data);
			break;

/*
 * EIO0 : NC
 * EIO1 : HT6513B_RTS Mouse reset
 * EIO2 : LPT.#Strobe:O
 * EIO3 : LPT.Ack:I
 * EIO4 : Printer.PaperOut:I
 * EIO5 : Printer.Select:I
 * EIO6 : TSP50C04_#INIT
 * EIO7 : KB.RST#
 */
		case 0x4026: // External I/O
			if ((m_bReg4026 ^ data) & 0x02) { // EIO01 changed
				// EIO01 : Mouse reset(HT6513B_RTS)
				if (data & 0x02) {
					// rising edge, send RTS
					nes->pad->ExWrite(1, 0);
				}
			}

			// speech
			if ((m_bReg4026 ^ data) & 0x40) { // EIO06 changed
				if (data & 0x40) {
					// rising edge, speech reset
					nSPReadPos = nSpWritePos = 0;

					nResetLPC = TRUE;
					::WaitForSingleObject(ackLPC, INFINITE);

					m_bSpeechReady   = TRUE;
					m_bSpeechEnd     = FALSE;
				}
			}

			if ((m_bReg4026 ^ data) & 0x80) { // EIO07 changed
				if (data & 0x80) {
					// rising edge, keyboard reset
				}
			}

			nes->lpt->StrobeN(!!(data & 4));

			m_bReg4026 = data & 0x7F; // do not set keyboard state 0x80
			break;

		case 0x4027: // DAC data
			if (!m_bDACOn) {
				int div;

				div = ((m_bReg4034 & 15) + 1) * (m_bReg4035 + 1);

				m_nDACRate = 1662607 / div;

				//m_wavDAC.Open(22050, 1);
				m_nDACCmd = DAC_CMD_OPEN;

//				::SetEvent(reqDAC);
				::WaitForSingleObject(ackDAC, INFINITE);

				m_bDACOn = TRUE;
			}

#ifdef DUMP_DAC_DATA
			if (fp_pcm) fwrite(&data, 1, 1, fp_pcm);
#endif

			// push sample
			while (m_nDACCmd == DAC_CMD_SAMPLE)
				Sleep(0);

			m_bDACData = data;
			m_nDACCmd = DAC_CMD_SAMPLE;

//			::SetEvent(reqDAC);
//			::WaitForSingleObject(ackDAC, INFINITE);

			break;

		case 0x4031:
			if (nMode6576 == 0) {
				if (data == 0x65) nMode6576 = 1;
			} else if (nMode6576 == 1) {
				if (data == 0x76) {
					nMode6576 = 2;
					nes->SetVariant(NES::NESVARIANT_UM6576);
				} else {
					nMode6576 = 0;
				}
			}
			break;

		case 0x4032: // IRQ mask & clean
			m_bReg4032  =  data;
			m_bReg4033 &= ~data;
			if (m_bReg4033 == 0)
				nes->cpu->ClrIRQ(IRQ_MAPPER);
			break;

		case 0x4034: // Timer control
			if ((m_bReg4034 ^ data) & 0x80) {
				if (data & 0x80) {
					// timer open
				} else {
					// timer close
					if (m_bDACOn) {
						//m_wavDAC.Close();
						m_nDACCmd = DAC_CMD_CLOSE;

//						::SetEvent(reqDAC);
						::WaitForSingleObject(ackDAC, INFINITE);

						m_bDACOn = FALSE;
					}
				}
			}
			m_bReg4034 = data;
			m_bTimerPrescaleCnt = (data & 15) + 1;
			break;

		case 0x4035: // Timer present value
			m_bReg4035 = data;
			m_bReg4036 = data; // Set Timer current value at the same time
			break;

		default:
			if (addr >= 0x4040 && addr < 0x4048) {
				// program bank select
				if ((addr == 0x4040) && (data & 0x80)){
//					char s[64];
//					sprintf(s, "BANK %02X -> %02X\n", (addr & 7) + 8, data);
//					OutputDebugStringA(s);
				}
				m_bPbank[addr & 7] = data;
			} else if (addr >= 0x4048 && addr < 0x4050) {
				// DMA
				m_bDmaReg[addr & 7] = data;

				if (addr == 0x4048 && (data & 0x80)) {
					// DMA start
					int src_addr;
					int dst_addr;
					int len;
					LPBYTE p_src;

					src_addr = ((m_bDmaReg[1] & 15) << 15) | ((m_bDmaReg[3] & 0x7F) << 8) | m_bDmaReg[2];
					p_src    = ((m_bDmaReg[1] & 0x10) ? EPRAM : BIOS) + src_addr;
					dst_addr = (m_bDmaReg[5] << 8) | m_bDmaReg[4];
					len      = ((m_bDmaReg[7] << 8) | m_bDmaReg[6]) + 0;

					if (len & 1) len++;

					if (m_bDmaReg[0] & 0x20) {
						// Work RAM
						dst_addr = dst_addr;
						while (len--)
							nes->Write(dst_addr++, *p_src++);
					} else {
						// VRAM
						LPBYTE p_dst;

						if (dst_addr & 0x8000) {
							p_dst = EVRAM + (dst_addr & 0x7FFF);
							p_dst += m_bVbank * 0x8000;
						} else {
							p_dst = CRAM + dst_addr;
						}

						memcpy(p_dst, p_src, len);
					}
				}
			} else {
				char s[64];
				sprintf(s, "W %02X -> [%04X]\n", data, addr);
				//OutputDebugStringA(s);
			}
			break;
	}
}

void MapperSB2K::MMC3_Remap()
{
	LPBYTE vbase = EVRAM + m_bEBank * 0x20000;
	LPBYTE pbase = EPRAM + m_bEBank * 0x20000;

	if (m_bC3_Sel & 0x80) {
		SetVROM_Bank( 0, vbase + (m_bC3_Reg[2] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 1, vbase + (m_bC3_Reg[3] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 2, vbase + (m_bC3_Reg[4] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 3, vbase + (m_bC3_Reg[5] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K

		SetVROM_Bank( 4, vbase + (m_bC3_Reg[0] & 0x7E) * 0x400 + 0x000, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 5, vbase + (m_bC3_Reg[0] & 0x7E) * 0x400 + 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 6, vbase + (m_bC3_Reg[1] & 0x7E) * 0x400 + 0x000, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 7, vbase + (m_bC3_Reg[1] & 0x7E) * 0x400 + 0x400, BANKTYPE_VROM); // 1K
	} else {
		SetVROM_Bank( 0, vbase + (m_bC3_Reg[0] & 0x7E) * 0x400 + 0x000, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 1, vbase + (m_bC3_Reg[0] & 0x7E) * 0x400 + 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 2, vbase + (m_bC3_Reg[1] & 0x7E) * 0x400 + 0x000, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 3, vbase + (m_bC3_Reg[1] & 0x7E) * 0x400 + 0x400, BANKTYPE_VROM); // 1K

		SetVROM_Bank( 4, vbase + (m_bC3_Reg[2] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 5, vbase + (m_bC3_Reg[3] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 6, vbase + (m_bC3_Reg[4] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
		SetVROM_Bank( 7, vbase + (m_bC3_Reg[5] & 0x7F) * 0x400, BANKTYPE_VROM); // 1K
	}

	if (m_bC3_Sel & 0x40) {
		SetPROM_Bank(4, pbase + 0x20000 - 0x4000,               BANKTYPE_ROM); // 8-9 8K
		SetPROM_Bank(5, pbase + (m_bC3_Reg[7] & 0x0F) * 0x2000, BANKTYPE_ROM); // A-B 8K
		SetPROM_Bank(6, pbase + (m_bC3_Reg[6] & 0x0F) * 0x2000, BANKTYPE_ROM); // C-D 8K
	} else {
		SetPROM_Bank(4, pbase + (m_bC3_Reg[6] & 0x0F) * 0x2000, BANKTYPE_ROM); // 8-9 8K
		SetPROM_Bank(5, pbase + (m_bC3_Reg[7] & 0x0F) * 0x2000, BANKTYPE_ROM); // A-B 8K
		SetPROM_Bank(6, pbase + 0x20000 - 0x4000,               BANKTYPE_ROM); // C-D 8K
	}

	SetPROM_Bank(7, pbase + 0x20000 - 0x2000, BANKTYPE_ROM); // E-F 8K
}

// from NES::Write 8000 - FFFF
void MapperSB2K::Write(WORD addr, BYTE data)
{
	BYTE bank;

	if (0 == nMode6576) {
		// MMC3
		switch ((addr >> 13) & 3) {
			case 0: // 8000-9FFF
				if (addr & 1) {
					m_bC3_Reg[m_bC3_Sel & 7] = data;

					MMC3_Remap();
				} else {
					// sel
					m_bC3_Sel = data;
				}
				break;
			case 1: // A000-BFFF
				if (addr & 1) {
					// PRG RAM protect
				} else {
					// Mirroring
					if (data & 1)
						SetVRAM_Mirror(VRAM_HMIRROR);
					else
						SetVRAM_Mirror(VRAM_VMIRROR);
				}
				break;
			case 2: // C000-DFFF
				if (addr & 1) {
					if (nes->GetScanline() < 240) {
						m_bC3_IRQCounter |= 0x80;
						m_bC3_IRQPreset = 0xFF;
					} else {
						m_bC3_IRQCounter |= 0x80;
						m_bC3_IRQPresetVBL = 0xFF;
						m_bC3_IRQPreset = 0;
					}
				} else {
					// latch
					m_bC3_IRQLatch = data;
				}
				break;
			case 3: // E000-FFFF
				if (addr & 1) {
					// enable IRQ
					m_bC3_IRQRequest = 0;
					m_bC3_IRQEnable  = 1;
				} else {
					// disable IRQ
					m_bC3_IRQEnable  = 0;
					m_bC3_IRQRequest = 0;
					nes->cpu->ClrIRQ(IRQ_MAPPER);
				}
				break;
		}

		return;
	}

	bank = m_bPbank[(addr >> 12) & 7];

	if (bank < 0x80) {
		// ROM
		char s[64];
		sprintf(s, "W %02X -> [%04X]\n", data, addr);
		OutputDebugStringA(s);
	} else {
		EPRAM[(bank & 0x7F) * 0x1000 + (addr & 0x0FFF)] = data;
	}

	return;
}

//------------------------------------------------------------------------------

BYTE MapperSB2K::PPU_Read ( WORD addr )
{
//	if (0 == nMode6576) {
//		return PPU_MEM_BANK[addr >> 10][addr & 0x3FF];
//	}

//	if (addr < 0x2800)
	if (addr < 0x8000)
		return CRAM[addr];
//	else if (addr < 0x3000)
//		return CRAM[0x2000 + (addr & 0x7FF)];
	else if (addr >= 0x8000)
		return EVRAM[m_bVbank * 0x8000 + (addr & 0x7FFF)];
	else
		return 0;
}

void MapperSB2K::PPU_Write( WORD addr, BYTE data )
{
//	if (0 == nMode6576) {
//		PPU_MEM_BANK[addr >> 10][addr & 0x3FF] = data;
//		return;
//	}

//	if (addr < 0x2800)
	if (addr < 0x8000)
		CRAM[addr] = data;
//	else if (addr < 0x3000)
//		CRAM[0x2000 + (addr & 0x7FF)] = data;
	else if (addr >= 0x8000)
		EVRAM[m_bVbank * 0x8000 + (addr & 0x7FFF)] = data;
}

//------------------------------------------------------------------------------

void MapperSB2K::PPU_Latch(WORD addr)
{
	;
}

BOOL MapperSB2K::CheckIRQ()
{
//	nes->cpu->SetIRQ(IRQ_MAPPER);
//	nes->cpu->ClrIRQ(IRQ_MAPPER);

	return FALSE;
}

void MapperSB2K::TimerTick(INT nTick)
{
	if (m_bDACOn) {
		// speed up to avoid audio buffer underflowing
		nTick = (int)(nTick * 1.5);
	}

	if (nTick >= m_bTimerPrescaleCnt) { // 1 based, with equal condition
		// Prescale underflow
		int cycles;
		int pre_period;

		nTick -= m_bTimerPrescaleCnt; // 1 based

		pre_period = (m_bReg4034 & 15) + 1;
		cycles     = (nTick / pre_period) + 1;

		// update new counter
		m_bTimerPrescaleCnt = pre_period - (nTick % pre_period);

		if (cycles > m_bReg4036) { // 0 based, no equal condition
			// Current underflow
			if (0 == (m_bReg4032 & 0x80)) {
				// gen timer irq
				m_bReg4033 |= 0x80;
				nes->cpu->SetIRQ(IRQ_MAPPER);
			}

			if (m_bReg4034 & 0x40)
				m_bReg4036 = m_bReg4035; // Auto load, 0 based
			else
				m_bReg4036 = 0;
		} else {
			m_bReg4036 -= cycles;
		}
	} else {
		m_bTimerPrescaleCnt -= nTick;
	}
}

void MapperSB2K::Clock(INT nCycles)
{
	// 软驱：推进控制器状态机；中断线出现上升沿时置 $4033 位 3（$4032 位 3 屏蔽，DOR 位 3 不允许时也不产生）
	nes->fdc->run();
	{
		BOOL irq = nes->fdc->irqRaised() && (nes->fdc->readIO(2) & 0x08);

		if (irq && !m_bFdcIrq && 0 == (m_bReg4032 & 0x08)) {
			m_bReg4033 |= 0x08;
			nes->cpu->SetIRQ(IRQ_MAPPER);
		}
		m_bFdcIrq = irq;
	}

	if ((m_bReg4034 & 0xA0) == 0x80) {
		// Timer use CPU clock
		TimerTick(nCycles);
	}

	if (kbd_send_irq_delay) {
		if (kbd_send_irq_delay <= nCycles) {
			// gen kbd_send_irq
			kbd_send_irq_delay = 0;
			if (0 == (m_bReg4032 & 0x20)) {
				m_bReg4033 |= 0x20;
				nes->cpu->SetIRQ(IRQ_MAPPER);
			}
		} else {
			kbd_send_irq_delay -= nCycles;
		}
	}

	// check keyboard
	if (1 == nes->pad->ExRead(0)) {
		// keyboard data ready
		m_bRegKbdIn = nes->pad->ExRead(1);

		// gen keyboard irq
		if (0 == (m_bReg4032 & 0x20)) {
			m_bReg4033 |= 0x20;
			nes->cpu->SetIRQ(IRQ_MAPPER);
		}
	}

	// check mouse
	if (1 == nes->pad->ExRead(2)) {
		BYTE data;

		// mouse data ready
		data = nes->pad->ExRead(3);

		if (m_bReg4023 & 1) { // 4017.D0 select mouse
			m_bReg4024 = data;

			// gen mouse irq
			if (0 == (m_bReg4032 & 0x40)) {
				m_bReg4033 |= 0x40;
				nes->cpu->SetIRQ(IRQ_MAPPER);
			}
		}
	}
}

void MapperSB2K::HSync(INT nScanline)
{
	if ((m_bReg4034 & 0xA0) == 0xA0) {
		// Timer use H-Blank
		TimerTick(1);
	}

	if (0 == nMode6576) {
		if ((nScanline >= 0 && nScanline < 240) && nes->ppu->IsDispON()) {
			if (m_bC3_IRQPresetVBL) {
				m_bC3_IRQCounter   = m_bC3_IRQLatch;
				m_bC3_IRQPresetVBL = 0;
			}

			if (m_bC3_IRQPreset) {
				m_bC3_IRQCounter = m_bC3_IRQLatch;
				m_bC3_IRQPreset  = 0;
			} else if (m_bC3_IRQCounter > 0) {
				m_bC3_IRQCounter--;
			}

			if (m_bC3_IRQCounter == 0) {
				if (m_bC3_IRQEnable) {
					m_bC3_IRQRequest = 0xFF;

					nes->cpu->SetIRQ( IRQ_MAPPER );
				}

				m_bC3_IRQPreset = 0xFF;
			}
		}
	}

	nes->lpt->OnHSync();
}

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
