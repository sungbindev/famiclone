#include <stdio.h>

unsigned int rgb_pal[64] = {
/*
 * rgb
 * 333,014,006,326,403,503,510,420,320,120,031,040,022,000,000,000
 * 555,036,027,407,507,704,700,630,430,140,040,053,044,000,000,000
 * 777,357,447,637,707,737,740,750,660,360,070,276,077,000,000,000
 * 777,567,657,757,747,755,764,772,773,572,473,276,467,000,000,000
 *
 * 0 = 0x00
 * 1 = 0x25
 * 2 = 0x49
 * 3 = 0x6E
 * 4 = 0x92
 * 5 = 0xB7
 * 6 = 0xDB
 * 7 = 0xFF
 *
 */
	0x6E6E6E, 0x002592, 0x0000DB, 0x6E49DB, 0x92006E, 0xB7006E, 0xB72500, 0x924900, 0x6E4900, 0x254900, 0x006E25, 0x009200, 0x004949, 0x000000, 0x000000, 0x000000,
	0xB7B7B7, 0x6E00DB, 0x0049FF, 0x9200FF, 0xB700FF, 0xFF0092, 0xFF0000, 0xDB6E00, 0x926E00, 0x259200, 0x009200, 0x00B76E, 0x009292, 0x000000, 0x000000, 0x000000,
	0xFFFFFF, 0x6EB7FF, 0x9292FF, 0xDB6EFF, 0xFF00FF, 0xFF6EFF, 0xFF9200, 0xFFB700, 0xDBDB00, 0x6EDB00, 0x00FF00, 0x49FFDB, 0x00FFFF, 0x000000, 0x000000, 0x000000,
	0xFFFFFF, 0xB7DBFF, 0xDBB7FF, 0xFFB7FF, 0xFF92FF, 0xFFB7B7, 0xFFDB92, 0xFFFF49, 0xFFFF6E, 0xB7FF49, 0x92FF6E, 0x49FFDB, 0x92DBFF, 0x000000, 0x000000, 0x000000
};

unsigned char bmp4_header[] = {
	0x42, 0x4D, 0x76, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00, 0x00, 0x00, 0x28, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x10, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char basic_pal[4][4];
unsigned char img_pal[64];

unsigned char bitmap4[256 * 256 / 2];

int main(int argc, char* argv[])
{
	int i, x, y;
	FILE* fp_gra;
	FILE* fp_bmp;

	int width;
	int height;
	int pitch;

	if (argc < 3)
		return -1;

	fp_gra = fopen(argv[1], "rb");
	if (!fp_gra) return 0;

	width = 256;
	height = 240;
	pitch = (width * 4 + 31) / 32 * 4;

	fp_bmp = fopen(argv[2], "wb");
	if (!fp_bmp) {
		fclose(fp_gra);
		return -1;
	}

	// write header
	fwrite(bmp4_header, sizeof(bmp4_header), 1, fp_bmp);

	fseek(fp_gra, 16, SEEK_SET);
	fread(basic_pal, 4, 4, fp_gra);
	fread(img_pal, sizeof(img_pal), 1, fp_gra);
	
	// write pal
	for (i = 0; i < 16; i++) {
		int cr;

		cr = basic_pal[i >> 2][i & 3];

		cr = rgb_pal[cr];

		fwrite(&cr, 4, 1, fp_bmp);

//		printf("PAL[%X] = %06X\n", i, cr);
	}

	x = 0;
	y = 0;

	while (!feof(fp_gra)) {
		int sy;
		unsigned char pattern[16];

		int base_block_pal;
		int base_block_index;
		int base_block_offset;
		int base_block_offset_x;
		int base_block_offset_y;

		base_block_index = y / 32 * (width / 32) + x / 32;
		base_block_offset_x = (x >> 4) & 1;
		base_block_offset_y = (y >> 4) & 1;
		base_block_offset = (base_block_offset_y << 1) | base_block_offset_x;
		base_block_pal = (img_pal[base_block_index] >> (base_block_offset * 2)) & 3;

		fread(pattern, 16, 1, fp_gra);

		for (sy = 0; sy < 8; sy++) {
			int sx;
			int mask;

			mask = 0x80;

			for (sx = 0; sx < 8; sx++) {
				int cr_index;
				int pal_index;

				int pix_index;
				int pix_offset;

				cr_index = ((pattern[sy] & mask) ? 1 : 0) | ((pattern[sy + 8] & mask) ? 2 : 0);
				pal_index = (base_block_pal << 2) | cr_index;

				pix_index = (y + sy) * pitch + (x + sx) / 2;
				pix_offset = 1 - (sx & 1);

				bitmap4[pix_index] &= pix_offset ? 0x0F : 0xF0;
				bitmap4[pix_index] |= pal_index << (pix_offset * 4); 

				mask >>= 1;
			}
		}

		x += 8;
		if (x >= width) {
			y += 8;
			x = 0;
		}
	}

	fwrite(bitmap4, sizeof(bitmap4), 1, fp_bmp);

	fclose(fp_gra);
	fclose(fp_bmp);

	return 0;
}