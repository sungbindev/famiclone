/*******************************************************************************
 *
 *  LPC-10 PE Synthesizer
 *
 *  Mark: QDS9501 (YuXing)
 *
 *  Author:  <87430545@qq.com>
 *
 *  Create:  Aug/25/2021 by fanoble
 *
 *******************************************************************************
 */

#ifndef __LPC_PE_SYNTH_H__
#define __LPC_PE_SYNTH_H__

#ifdef __cplusplus
extern "C" {
#endif

int lpc_pe_synth_do(short* pcm, int* pcm_size, 
					const unsigned char* bs, int len, int external);

#ifdef __cplusplus
}
#endif

#endif
