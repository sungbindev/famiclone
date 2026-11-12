#include "WaveOut.h"

CWaveOut::CWaveOut()
{
	hWaveOut       = NULL;

	buf_write      = 0;
	buf_write_pos  = 0;
	full_buffers   = 0;
	buffered_bytes = 0;
	buffer_size    = BUFFER_SIZE;
}

CWaveOut::~CWaveOut()
{
	if (hWaveOut)
		Close();
}

void CALLBACK CWaveOut::waveOutProc(
			HWAVEOUT	hWaveOut,
			UINT		uMsg,
			DWORD		dwInstance,
			DWORD		dwParam1,
			DWORD		dwParam2)
{
	CWaveOut* thiz;
	LPWAVEHDR pHeader = (LPWAVEHDR)dwParam1;

	if (uMsg != WOM_DONE)
		return;

	thiz = (CWaveOut*)dwInstance;

	thiz->buffered_bytes -= pHeader->dwBufferLength;
}

int CWaveOut::Open(int sample_rate, int channels)
{
	WAVEFORMATEX	wf;
	MMRESULT		result;
	DWORD			totalBufferSize;
	int				i;
	char*			buffer;

	int sample_size = 16;

	switch (sample_rate)
	{
		case 8000:
		case 10000:
		case 11025:
		case 16000:
		case 22050:
		case 32000:
		case 44100:
		case 48000:
			buffer_size = BUFFER_SIZE;
			break;
		case 88200:
		case 96000:
			buffer_size = BUFFER_SIZE * 2;
			break;
		case 176400:
		case 192000:
			buffer_size = BUFFER_SIZE * 4;
			break;
		case 352800:
		case 384000:
			buffer_size = BUFFER_SIZE * 8;
			break;
		default:
			buffer_size = BUFFER_SIZE * 4;
			break;
	}

	// fill waveformatex
	wf.wFormatTag		= WAVE_FORMAT_PCM;
	wf.nBlockAlign		= sample_size / 8 * channels;
	wf.nAvgBytesPerSec	= sample_rate * wf.nBlockAlign;
	wf.nSamplesPerSec	= sample_rate;
	wf.nChannels		= channels;
	wf.wBitsPerSample	= sample_size;
	wf.cbSize			= sizeof(wf);

	// open sound device
	// WAVE_MAPPER always points to the default wave device on the system
	result = waveOutOpen(&hWaveOut, WAVE_MAPPER, (WAVEFORMATEX*)&wf,
		(DWORD)waveOutProc, (DWORD)this, CALLBACK_FUNCTION);

	if (result != MMSYSERR_NOERROR)
		return 0;

	totalBufferSize = (buffer_size + sizeof(WAVEHDR)) * BUFFER_COUNT;

	// allocate buffer memory as one big block
	buffer = new char [totalBufferSize];
	memset(buffer, 0, totalBufferSize);

	// and setup pointers to each buffer
	waveBlocks = (WAVEHDR*)buffer;
	buffer += sizeof(WAVEHDR) * BUFFER_COUNT;
	for (i = 0; i < BUFFER_COUNT; i++) {
		waveBlocks[i].dwFlags = WHDR_DONE;
		waveBlocks[i].lpData  = buffer;
		buffer += buffer_size;
	}

	buf_write      = 0;
	buf_write_pos  = 0;
	full_buffers   = 0;
	buffered_bytes = 0;

	return 0;
}

int CWaveOut::Write(const short* pcm, int len)
{
	int ret;

	ret = len;

	while (len > 1) {
		int consumed;

		consumed = WriteNotWait(pcm, len);

		if (0 == consumed)
			Sleep(10);

		pcm += consumed / 2;
		len -= consumed;
	}

	return ret;
}

int CWaveOut::WriteNotWait(const short* pcm, int len)
{
	WAVEHDR* current;
	int len2 = 0;
	int x;
	unsigned char* data = (unsigned char*)pcm;
	unsigned int buf_start_write = buf_write;

	if (buffered_bytes == BUFFER_COUNT * buffer_size) {
		return 0;
	}

	while (len > 0) {
		current = &waveBlocks[buf_write];
		if (0 == (current->dwFlags & WHDR_DONE)) {
			// block is full, find next!
			buf_write     = (buf_write + 1) % BUFFER_COUNT;
			buf_write_pos = 0;

			if (buf_start_write == buf_write)
				return len2;

			continue;
		}

		buf_start_write = buf_write;

		// unprepare the header if it is prepared
		if (current->dwFlags & WHDR_PREPARED)
			waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

		x = buffer_size - buf_write_pos;

		if (x > len) x = len;

		memcpy(current->lpData + buf_write_pos, data + len2, x);

		len2 += x;
		len  -= x;

		buffered_bytes += x;
		buf_write_pos  += x;

		if (buf_write_pos >= buffer_size) {
			// prepare header and write data to device
			current->dwBufferLength = buf_write_pos;

			waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
			waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));

			// move to the next block
			buf_write     = (buf_write + 1) % BUFFER_COUNT;
			buf_write_pos = 0;
		}
	}

	return len2;
}

int CWaveOut::PushSample(short sample)
{
	WAVEHDR* current;
	unsigned int buf_start_write = buf_write;

	if (buffered_bytes == BUFFER_COUNT * buffer_size)
		return 0;

	for (;;) {
		current = &waveBlocks[buf_write];
		if (current->dwFlags & WHDR_DONE) break;

		// block is full, find next!
		buf_write     = (buf_write + 1) % BUFFER_COUNT;
		buf_write_pos = 0;

		if (buf_start_write == buf_write)
			return 0;
	}

	// unprepare the header if it is prepared
	if (current->dwFlags & WHDR_PREPARED)
		waveOutUnprepareHeader(hWaveOut, current, sizeof(WAVEHDR));

	*(short*)(current->lpData + buf_write_pos) = sample;

	buffered_bytes += 2;
	buf_write_pos  += 2;

	// prepare header and write data to device
	current->dwBufferLength = buf_write_pos;

	if (buf_write_pos >= buffer_size)
	{
		waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));

		// move to the next block
		buf_write     = (buf_write + 1) % BUFFER_COUNT;
		buf_write_pos = 0;
	}

	return 1;
}

int CWaveOut::Drain()
{
	WAVEHDR* current;

	current = &waveBlocks[buf_write];
	if (0 == (current->dwFlags & WHDR_DONE)) {
		// no more data remained
		return 0;
	}

	// prepare header and write data to device
	current->dwBufferLength = buf_write_pos;

	waveOutPrepareHeader(hWaveOut, current, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, current, sizeof(WAVEHDR));

	buf_write_pos = 0;

	return 0;
}

int CWaveOut::Close()
{
	if (!hWaveOut)
		return 0;

//	Drain();

	while (buffered_bytes > 0)
		Sleep(50);

	waveOutPause(hWaveOut);
	waveOutClose(hWaveOut);

	delete (char*)waveBlocks;

	hWaveOut = NULL;

	return 0;
}
