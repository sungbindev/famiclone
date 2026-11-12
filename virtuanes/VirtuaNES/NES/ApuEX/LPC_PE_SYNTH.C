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

#include <string.h>
#include "LPC_PE_SYNTH.H"

#define LPC_ORDER 10
#define LPC_SAMPLES_PER_FRAME 200 // speed

#define LPC_FRAC_BITS 15 // using Q15

typedef struct tag_lpc_frame {
	short energy;
	short pitch;
	short k[LPC_ORDER]; // K1-K10
} LPC_FRAME;

typedef struct tag_lpc_synth {
	LPC_FRAME frame_prev; // 0
	LPC_FRAME frame_curr; // 2
	LPC_FRAME frame_next; // 1

	short need_interp;
	short random_seed;
	short curr_pitch;
	short sample_index; // from 0 to LPC_SAMPLES_PER_FRAME - 1

	short x[LPC_ORDER + 1]; // filter buf
	short synth_out; // output sample

	int external;

	// for bit reader
	int bits_left;
	unsigned short data_cache;
	int bitstream_length;
	const unsigned char* in_stream;
} LPC_SYNTH;

//------------------------------------------------------------------------------

#pragma warning (disable:4305)

static const short lpc_gain_tab[] =
{
	0x0000, 0x000C, 0x0104, 0x010C, 0x0208, 0x0308, 0x0500, 0x0704,
    0x0A00, 0x0E04, 0x1404, 0x1C08, 0x2804, 0x380C, 0x5004, 0x0000
};

static const short lpc_pitch_tab[] =
{
	0x0000, 0x0100, 0x0110, 0x0120, 0x0130, 0x0140, 0x0150, 0x0160,
	0x0170, 0x0180, 0x0190, 0x01A0, 0x01B0, 0x01C0, 0x01D0, 0x01E0,
	0x01F0, 0x0200, 0x0210, 0x0220, 0x0230, 0x0240, 0x0250, 0x0260,
	0x0270, 0x0280, 0x0290, 0x02A0, 0x02B0, 0x02D0, 0x02F0, 0x0310,
	0x0330, 0x0350, 0x0360, 0x0390, 0x03B0, 0x03D0, 0x03F0, 0x0420,
	0x0450, 0x0470, 0x0490, 0x04D0, 0x04F0, 0x0510, 0x0550, 0x0570,
	0x05C0, 0x05F0, 0x0630, 0x0660, 0x06A0, 0x06E0, 0x0730, 0x0770,
	0x07B0, 0x0800, 0x0850, 0x08A0, 0x08F0, 0x0950, 0x09A0, 0x0A00
};

static const short lpc_k1_tab[] =
{
	0x820C, 0x8308, 0x830C, 0x8404, 0x840C, 0x8504, 0x8600, 0x8708,
	0x8808, 0x8908, 0x8A0C, 0x8C00, 0x8D04, 0x8F00, 0x900C, 0x920C,
	0x9900, 0xA104, 0xAB08, 0xB804, 0xC704, 0xD80C, 0xEB0C, 0x0000,
	0x1404, 0x2704, 0x380C, 0x470C, 0x5408, 0x5E0C, 0x6700, 0x6D04
};

static const short lpc_k2_tab[] =
{
	0xAE00, 0xB408, 0xBB08, 0xC304, 0xCB08, 0xD404, 0xDD0C, 0xE708,
	0xF108, 0xFB0C, 0x0600, 0x1004, 0x1A04, 0x2400, 0x2D04, 0x3600,
	0x3E04, 0x450C, 0x4C0C, 0x5300, 0x5808, 0x5D0C, 0x6204, 0x6604,
	0x690C, 0x6C0C, 0x6F08, 0x710C, 0x730C, 0x7508, 0x7700, 0x7E08
};

static const short lpc_k3_tab[] =
{
	0x9200, 0x9F00, 0xAD00, 0xBA00, 0xC800, 0xD500, 0xE300, 0xF000,
	0xFE00, 0x0B00, 0x1900, 0x2600, 0x3400, 0x4100, 0x4F00, 0x5C00
};

static const short lpc_k4_tab[] =
{
	0xAE00, 0xBC00, 0xCA00, 0xD800, 0xE600, 0xF400, 0x0100, 0x0F00,
	0x1D00, 0x2B00, 0x3900, 0x4700, 0x5500, 0x6300, 0x7100, 0x7E00
};

static const short lpc_k5_tab[] =
{
	0xAE00, 0xBA00, 0xC500, 0xD100, 0xDD00, 0xE800, 0xF400, 0xFF00,
	0x0B00, 0x1700, 0x2200, 0x2E00, 0x3900, 0x4500, 0x5100, 0x5C00
};

static const short lpc_k6_tab[] =
{
	0xC000, 0xCB00, 0xD600, 0xE100, 0xEC00, 0xF700, 0x0300, 0x0E00,
	0x1300, 0x2400, 0x2F00, 0x3A00, 0x4500, 0x5000, 0x5B00, 0x6600
};

static const short lpc_k7_tab[] =
{
	0xB300, 0xBF00, 0xCB00, 0xD700, 0xE300, 0xEF00, 0xFB00, 0x0700,
	0x1300, 0x1F00, 0x2B00, 0x3700, 0x4300, 0x4F00, 0x5A00, 0x6600
};

static const short lpc_k8_tab[] =
{
	0xC000, 0xD800, 0xF000, 0x0700, 0x1F00, 0x3700, 0x4F00, 0x6600
};

static const short lpc_k9_tab[] =
{
	0xC000, 0xD400, 0xE800, 0xFC00, 0x1000, 0x2500, 0x3900, 0x4D00
};

static const short lpc_k10_tab[] =
{
	0xCD00, 0xDF00, 0xF100, 0x0400, 0x1600, 0x2800, 0x3B00, 0x4D00
};

static const short lpc_excit[] =
{
	0x0580, 0x0580, 0x0580, 0x05C0, 0x05C0, 0x05C0, 0x05C0, 0x05C0,
	0x05C0, 0x0580, 0x0580, 0x0580, 0x0580, 0x0580, 0x0580, 0x0580,
	0x3A80, 0x3A80, 0x3A80, 0x3A80, 0x3A80, 0x3AC0, 0x3AC0, 0x3AC0,
	0x3A80, 0x3AC0, 0x3AC0, 0x3A80, 0x3A80, 0x3AC0, 0x3A80, 0x3AC0,
	0x00E2, 0x00AF, 0x00BA, 0x00C2, 0x00C7, 0x0089, 0x008A, 0x0086,
	0x0082, 0x00FC, 0x00F5, 0x00ED, 0x00E5, 0x009E, 0x009A, 0x0095,
	0x0095, 0x00D8, 0x00DF, 0x00E8, 0x00F8, 0x008A, 0x00E3, 0x00BE,
	0x015F, 0x0101, 0x0129, 0x0191, 0x01BD, 0x01A8, 0x0216, 0x0200,
	0x026C, 0x0292, 0x02F9, 0x02D9, 0x02B8, 0x034F, 0x0365, 0x0332,
	0x033F, 0x0343, 0x0307, 0x0305, 0x0345, 0x033F, 0x037D, 0x037A,
	0x037D, 0x0341, 0x030E, 0x035F, 0x033B, 0x03A0, 0x03D2, 0x040D,
	0x0417, 0x04ED, 0x0551, 0x0582, 0x0640, 0x06CA, 0x071F, 0x07FD,
	0x0824, 0x0911, 0x0981, 0x0A34, 0x0B26, 0x0B95, 0x0C3F, 0x0D60,
	0x0DB7, 0x0E40, 0x0EBB, 0x0F64, 0x0F7A, 0x0FFC, 0x0FE9, 0x0FBF,
	0x0FBF, 0x0FA9, 0x0FBC, 0x0F7A, 0x0F64, 0x0EFB, 0x0E40, 0x0DB7,
	0x0D20, 0x0C3F, 0x0B95, 0x0B66, 0x0A74, 0x0981, 0x0911, 0x0864,
	0x07FD, 0x071F, 0x06CA, 0x0600, 0x0582, 0x0551, 0x04ED, 0x0417,
	0x044D, 0x03D2, 0x03E0, 0x033B, 0x031F, 0x030E, 0x0301, 0x033D,
	0x033A, 0x033D, 0x033F, 0x0305, 0x0305, 0x0307, 0x0303, 0x037F,
	0x0372, 0x0365, 0x034F, 0x02F8, 0x02D9, 0x02B9, 0x0292, 0x022C,
	0x0240, 0x0256, 0x01E8, 0x01FD, 0x01D1, 0x0169, 0x0141, 0x015F,
	0x00FE, 0x00E3, 0x00CA, 0x00F8, 0x00E8, 0x00DF, 0x00D8, 0x00D5,
	0x00D5, 0x00DA, 0x00DE, 0x00E5, 0x00ED, 0x00F5, 0x00FC, 0x00C2,
	0x00C6, 0x00CA, 0x00C9, 0x00C7, 0x00C2, 0x00FA, 0x00EF, 0x00E2
};

//------------------------------------------------------------------------------

void get_bits_init(LPC_SYNTH* s, const unsigned char* bs, int len)
{
	s->in_stream = bs;
	s->bitstream_length = len;

	s->data_cache = 0;
	s->bits_left = 0;
}

static unsigned char byte_load(LPC_SYNTH* s, unsigned char a)
{
	if (s->external)
		a ^= 0x41;

	// 76543210
	a = (a >> 4) | (a << 4); // Swap in groups of 4

	// 32107654
	a = ((a & 0xCC) >> 2) | ((a & 0x33) << 2); // Swap in groups of 2

	// 10325476
	a = ((a & 0xAA) >> 1) | ((a & 0x55) << 1); // Swap bit pairs

	// 01234567
	return a;
}

// bits <= 8
static short get_nbits(LPC_SYNTH* s, short bits)
{
	unsigned short data;

	if (s->bits_left < bits)
	{
		s->data_cache <<= 8;

		if (s->bitstream_length < 1)
			return -1;

		s->data_cache |= byte_load(s, *s->in_stream);
		s->in_stream++;
		s->bitstream_length--;
		s->bits_left += 8;
	}

	data = s->data_cache >> (s->bits_left - bits);
	data &= (1 << bits) - 1;
	s->bits_left -= bits;

	return (short)data;
}

//------------------------------------------------------------------------------

static int lpc_get_frame(LPC_SYNTH* s, LPC_FRAME* dst, LPC_FRAME* ref)
{
	int pitch_i;
	int energy_i;
	int repeat;

	energy_i = get_nbits(s, 4);
	energy_i &= 15;
	if (0 == energy_i) {
		// silent
		dst->energy = 0;
		dst->pitch = 0;
		memset(dst->k, 0, sizeof(ref->k));
		return 0;
	}

	if (15 == energy_i) // end of stream
		return -1;

	repeat = get_nbits(s, 1); // repeat
	pitch_i = get_nbits(s, 6); // pitch

	// coded -> uncoded
	dst->energy = lpc_gain_tab[energy_i];
	dst->pitch  = lpc_pitch_tab[pitch_i];

	if (repeat) {
		dst->k[0] = ref->k[0];
		dst->k[1] = ref->k[1];
		dst->k[2] = ref->k[2];
		dst->k[3] = ref->k[3];
	} else {
		dst->k[0] = lpc_k1_tab[get_nbits(s, 5)]; // K1
		dst->k[1] = lpc_k2_tab[get_nbits(s, 5)]; // K2
		dst->k[2] = lpc_k3_tab[get_nbits(s, 4)]; // K3
		dst->k[3] = lpc_k4_tab[get_nbits(s, 4)]; // K4
	}

	if (0 == pitch_i) {
		// lpc unvoiced frame
		dst->k[4] = 0;
		dst->k[5] = 0;
		dst->k[6] = 0;
		dst->k[7] = 0;
		dst->k[8] = 0;
		dst->k[9] = 0;
	} else {
		// voiced
		if (repeat) {
			memcpy(dst->k, ref->k, sizeof(ref->k));
		} else {
			dst->k[4] = lpc_k5_tab[get_nbits(s, 4)]; // K5
			dst->k[5] = lpc_k6_tab[get_nbits(s, 4)]; // K6
			dst->k[6] = lpc_k7_tab[get_nbits(s, 4)]; // K7
			dst->k[7] = lpc_k8_tab[get_nbits(s, 3)]; // K8
			dst->k[8] = lpc_k9_tab[get_nbits(s, 3)]; // K9
			dst->k[9] = lpc_k10_tab[get_nbits(s, 3)]; // K10
		}
	}

	return 0;
}

static void lpc_set_interp_flag(LPC_SYNTH* s)
{
	s->need_interp = 1;

	if (s->frame_prev.energy == 0) {
		// silent
		if (s->frame_next.pitch == 0) {
			if (s->frame_next.energy)
				s->need_interp = 0;
		}
	} else {
		if (s->frame_prev.pitch) {
			if (s->frame_next.pitch == 0) {
				if (s->frame_next.energy)
					s->need_interp = 0;
			}
		} else {
			if (s->frame_next.pitch)
				s->need_interp = 0;
		}
	}
}

static void lpc_synth_init(LPC_SYNTH* s)
{
	memset(s, 0, sizeof(LPC_SYNTH));

	s->random_seed = 0xFFFF;

	lpc_get_frame(s, &s->frame_prev, &s->frame_prev);
	lpc_get_frame(s, &s->frame_next, &s->frame_prev);

	s->frame_curr = s->frame_prev;

	lpc_set_interp_flag(s);
}

static short lpc_reload_pitch(LPC_SYNTH* s)
{
	short curr_pitch;

	curr_pitch = s->curr_pitch;

	if (s->need_interp) {
		int i;
		int ratio;
		short* vector0;
		short* vector1;
		short* vector2;

		ratio = (s->sample_index << LPC_FRAC_BITS) / LPC_SAMPLES_PER_FRAME;

		vector0 = (short*)&s->frame_prev;
		vector1 = (short*)&s->frame_next;
		vector2 = (short*)&s->frame_curr;

		// do interp
		for (i = 0; i < sizeof(LPC_FRAME) / sizeof(short); i++)
		{
			short v;

			v = vector1[i] - vector0[i];
			v = (short)((v * ratio) >> LPC_FRAC_BITS);
			vector2[i] = v + vector0[i];
		}

		curr_pitch += s->frame_curr.pitch;
		if (curr_pitch > 0)
			return curr_pitch;
	}

	if (s->frame_curr.pitch != 0) {
		curr_pitch += s->frame_curr.pitch;
		return curr_pitch;
	}

	return 0x80;
}

#define LPC_CLAMP_P 27500 // to be fixed
#define LPC_CLAMP_N -(LPC_CLAMP_P + 1)

static void lpc_synth_do_filter(LPC_SYNTH* s)
{
	int i;
	int sample;
	short* x;
	short* k;

	x = s->x;
	k = s->frame_curr.k;
	sample = s->synth_out;

	for (i = 0; i < LPC_ORDER; i++) {
		sample -= (k[LPC_ORDER - 1 - i] * x[LPC_ORDER - 1 - i]) >> LPC_FRAC_BITS;

		if (sample > LPC_CLAMP_P)
			sample = LPC_CLAMP_P;
		else if (sample < LPC_CLAMP_N)
			sample = LPC_CLAMP_N;

		x[LPC_ORDER - i] = x[LPC_ORDER - 1 - i] +
			((sample * k[LPC_ORDER - 1 - i]) >> LPC_FRAC_BITS);
	}

	x[0] = s->synth_out = (short)sample;
}

static short random_gen(LPC_SYNTH* s)
{
	short r;
	short seed;

	r = 0;

	seed = s->random_seed << 1;

	r = ((seed >> 12) ^ (seed >> 13)) & 1;

	s->random_seed = seed | r;

	return r;
}

static short* lpc_synth_run(LPC_SYNTH* s, short* pcm_out, int* eos)
{
	int i;
	int excit;

	excit = 0;
	if (eos) *eos = 0;

	for (i = 0; i < LPC_SAMPLES_PER_FRAME; i++) {
		s->sample_index = i;
		s->curr_pitch -= 16;

		if (s->curr_pitch < 0)
			s->curr_pitch = lpc_reload_pitch(s);

		if (0 == s->frame_curr.pitch) {
			// unvoiced
			if (s->frame_curr.energy) {
				excit = random_gen(s) ? 1408 : -1408;
				excit = (excit * s->frame_curr.energy) >> LPC_FRAC_BITS;
			} else {
				// silent
				excit = 0;
			}
		} else if (s->curr_pitch >= sizeof(lpc_excit) / sizeof(lpc_excit[0])) {
			excit = 0;
		} else {
			// lpc voiced excit
			excit = lpc_excit[s->curr_pitch];
			excit = (excit * s->frame_curr.energy) >> LPC_FRAC_BITS;
		}

		excit *= 8;

		// setup lpc
		s->synth_out = (short)excit;

		lpc_synth_do_filter(s);

		*pcm_out++ = s->synth_out;
	}

	// prepare for the next frame
	s->frame_prev = s->frame_next;
	s->frame_curr = s->frame_next;

	if (lpc_get_frame(s, &s->frame_next, &s->frame_prev)) {
		// end of stream
		if (eos) *eos = 1;
	}

	lpc_set_interp_flag(s);

	return pcm_out;
}

int lpc_pe_synth_do(short* pcm, int* pcm_size,
					const unsigned char* bs, int len, int external)
{
	int i;
	LPC_SYNTH lpc;
	short* pcm_base;

	lpc_synth_init(&lpc);

	lpc.external = external;

	get_bits_init(&lpc, bs, len);

	if (external) {
		// skip junk byte
		get_nbits(&lpc, 8);
	}

	pcm_base = pcm;

	for (i = 0; i < 1000; i++) {
		int eos;

		pcm = lpc_synth_run(&lpc, pcm, &eos);
		if (eos) break;
	}

	*pcm_size = pcm - pcm_base;

	return 0;
}

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
