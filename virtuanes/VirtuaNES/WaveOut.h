#ifndef __WAVEOUT_H__
#define __WAVEOUT_H__

#include <windows.h>
#include <stdio.h>
#include <memory.h>

#define BUFFER_SIZE  1024
#define BUFFER_COUNT 16

class CWaveOut
{
public:
	CWaveOut();
	~CWaveOut();
	
	int Open(int sample_rate, int channels);
	int PushSample(short sample);
	int Write(const short* pcm, int size);
	int WriteNotWait(const short* pcm, int size);
	int Drain();
	int Close();

	void Pause() { waveOutPause(hWaveOut); }
	void Resume() { waveOutRestart(hWaveOut); }

	static void CALLBACK waveOutProc(
		HWAVEOUT	hWaveOut,
		UINT		uMsg,
		DWORD		dwInstance,
		DWORD		dwParam1,
		DWORD		dwParam2);

protected:
private:
	WAVEHDR*		waveBlocks;		// pointer to our ring buffer memory
	HWAVEOUT		hWaveOut;		// handle to the waveout device
	unsigned int	buf_write;
	unsigned int	buf_write_pos;
	int				full_buffers;
	int				buffered_bytes;
	unsigned int	buffer_size;
};

#endif // __WAVEOUT_H__
