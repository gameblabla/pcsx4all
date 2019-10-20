/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *   Copyright (C) 2016 PCSX4ALL Team
 *   Copyright (C) 2016 Senquack (dansilsby <AT> gmail <DOT> com)
 *   Copyright (C) 2010 Unai
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <SDL/SDL.h>
#include "port.h"
#include "gpu.h"

///////////////////////////////////////////////////////////////////////////////
// BLITTERS TAKEN FROM gpu_unai/gpu_blit.h
// GPU Blitting code with rescale and interlace support.
///////////////////////////////////////////////////////////////////////////////
#ifndef USE_BGR15
#define RGB24(R,G,B)	(((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
#define RGB16X2(C)      (((C)&(0x1f001f<<10))>>10) | (((C)&(0x1f001f<<5))<<1) | (((C)&(0x1f001f<<0))<<11)
#define RGB16(C)		(((C)&(0x1f<<10))>>10) | (((C)&(0x1f<<5))<<1) | (((C)&(0x1f<<0))<<11)
#else
#define RGB24(R,G,B)  	((((R)&0xF8)>>3)|(((G)&0xF8)<<2)|(((B)&0xF8)<<7))
#endif

#ifndef HW_SCALE

static inline void GPU_BlitWW(const void* src, uint16_t* dst16, uint_fast8_t isRGB24)
{
	uint32_t uCount;
	if (!isRGB24)
	{
#ifndef USE_BGR15
		uCount = 20;
		const uint32_t* src32 = (const uint32_t*) src;
		uint32_t* dst32 = (uint32_t*)(void*) dst16;
		do {
			dst32[0] = RGB16X2(src32[0]);
			dst32[1] = RGB16X2(src32[1]);
			dst32[2] = RGB16X2(src32[2]);
			dst32[3] = RGB16X2(src32[3]);
			dst32[4] = RGB16X2(src32[4]);
			dst32[5] = RGB16X2(src32[5]);
			dst32[6] = RGB16X2(src32[6]);
			dst32[7] = RGB16X2(src32[7]);
			dst32 += 8;
			src32 += 8;
		} while(--uCount);
#else
		memcpy(dst16, src, 640);
#endif
	} else
	{
		uCount = 20;
		const uint8_t* src8 = (const uint8_t*)src;
		do{
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 5] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 6] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 7] = RGB24(src8[21], src8[22], src8[23] );

			dst16[ 8] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 9] = RGB24(src8[27], src8[28], src8[29] );
			dst16[10] = RGB24(src8[30], src8[31], src8[32] );
			dst16[11] = RGB24(src8[33], src8[34], src8[35] );
			dst16[12] = RGB24(src8[36], src8[37], src8[38] );
			dst16[13] = RGB24(src8[39], src8[40], src8[41] );
			dst16[14] = RGB24(src8[42], src8[43], src8[44] );
			dst16[15] = RGB24(src8[45], src8[46], src8[47] );
			dst16 += 16;
			src8  += 48;
		} while (--uCount);
	}
}

static inline void GPU_BlitWWSWWSWS(const void* src, uint16_t* dst16, uint_fast8_t isRGB24)
{
	uint32_t uCount;
	if (!isRGB24)
	{
#ifndef USE_BGR15
		uCount = 64;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			dst16[0] = RGB16(src16[0]);
			dst16[1] = RGB16(src16[1]);
			dst16[2] = RGB16(src16[3]);
			dst16[3] = RGB16(src16[4]);
			dst16[4] = RGB16(src16[6]);
			dst16 += 5;
			src16 += 8;
		} while (--uCount);
#else
		uCount = 64;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			dst16[0] = src16[0];
			dst16[1] = src16[1];
			dst16[2] = src16[3];
			dst16[3] = src16[4];
			dst16[4] = src16[6];
			dst16 += 5;
			src16 += 8;
		} while (--uCount);
#endif
	} else
	{
		uCount = 32;
		const uint8_t* src8 = (const uint8_t*)src;
		do {
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 3] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 4] = RGB24(src8[18], src8[19], src8[20] );

			dst16[ 5] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 6] = RGB24(src8[27], src8[28], src8[29] );
			dst16[ 7] = RGB24(src8[33], src8[34], src8[35] );
			dst16[ 8] = RGB24(src8[36], src8[37], src8[38] );
			dst16[ 9] = RGB24(src8[42], src8[43], src8[44] );

			dst16 += 10;
			src8  += 48;
		} while (--uCount);
	}
}

static inline void GPU_BlitWWWWWS(const void* src, uint16_t* dst16, uint_fast8_t isRGB24)
{
	uint32_t uCount;
	if (!isRGB24)
	{
#ifndef USE_BGR15
		uCount = 32;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			dst16[ 0] = RGB16(src16[0]);
			dst16[ 1] = RGB16(src16[1]);
			dst16[ 2] = RGB16(src16[2]);
			dst16[ 3] = RGB16(src16[3]);
			dst16[ 4] = RGB16(src16[4]);
			dst16[ 5] = RGB16(src16[6]);
			dst16[ 6] = RGB16(src16[7]);
			dst16[ 7] = RGB16(src16[8]);
			dst16[ 8] = RGB16(src16[9]);
			dst16[ 9] = RGB16(src16[10]);
			dst16 += 10;
			src16 += 12;
		} while (--uCount);
#else
		uCount = 64;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16;
			src16 += 2;
		} while (--uCount);
#endif
	} else
	{
		uCount = 32;
		const uint8_t* src8 = (const uint8_t*)src;
		do {
			dst16[0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[5] = RGB24(src8[18], src8[19], src8[20] );
			dst16[6] = RGB24(src8[21], src8[22], src8[23] );
			dst16[7] = RGB24(src8[24], src8[25], src8[26] );
			dst16[8] = RGB24(src8[27], src8[28], src8[29] );
			dst16[9] = RGB24(src8[30], src8[31], src8[32] );
			dst16 += 10;
			src8  += 36;
		} while (--uCount);
	}
}

static inline void GPU_BlitWWWWWWWWS(const void* src, uint16_t* dst16, uint_fast8_t isRGB24, uint32_t uClip_src)
{
	uint32_t uCount;
	if (!isRGB24)
	{
#ifndef USE_BGR15
		uCount = 20;
		const uint16_t* src16 = ((const uint16_t*) src) + uClip_src;
		do {
			dst16[ 0] = RGB16(src16[0]);
			dst16[ 1] = RGB16(src16[1]);
			dst16[ 2] = RGB16(src16[2]);
			dst16[ 3] = RGB16(src16[3]);
			dst16[ 4] = RGB16(src16[4]);
			dst16[ 5] = RGB16(src16[5]);
			dst16[ 6] = RGB16(src16[6]);
			dst16[ 7] = RGB16(src16[7]);

			dst16[ 8] = RGB16(src16[9]);
			dst16[ 9] = RGB16(src16[10]);
			dst16[10] = RGB16(src16[11]);
			dst16[11] = RGB16(src16[12]);
			dst16[12] = RGB16(src16[13]);
			dst16[13] = RGB16(src16[14]);
			dst16[14] = RGB16(src16[15]);
			dst16[15] = RGB16(src16[16]);
			dst16 += 16;
			src16 += 18;
		} while (--uCount);
#else
		uCount = 40;
		const uint16_t* src16 = ((const uint16_t*) src) + uClip_src;
		do {
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16;
			src16 += 2;
		} while (--uCount);
#endif
	} else
	{
		uCount = 20;
		const uint8_t* src8 = (const uint8_t*)src + (uClip_src<<1) + uClip_src;
		do {
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 5] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 6] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 7] = RGB24(src8[21], src8[22], src8[23] );

			dst16[ 8] = RGB24(src8[27], src8[28], src8[29] );
			dst16[ 9] = RGB24(src8[30], src8[31], src8[32] );
			dst16[10] = RGB24(src8[33], src8[34], src8[35] );
			dst16[11] = RGB24(src8[36], src8[37], src8[38] );
			dst16[12] = RGB24(src8[39], src8[40], src8[41] );
			dst16[13] = RGB24(src8[42], src8[43], src8[44] );
			dst16[14] = RGB24(src8[45], src8[46], src8[47] );
			dst16[15] = RGB24(src8[48], src8[49], src8[50] );
			dst16 += 16;
			src8  += 54;
		} while (--uCount);
	}
}

static inline void GPU_BlitWWDWW(const void* src, uint16_t* dst16, uint_fast8_t isRGB24)
{
	uint32_t uCount;
	if (!isRGB24)
	{
#ifndef USE_BGR15
		uCount = 32;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			dst16[ 0] = RGB16(src16[0]);
			dst16[ 1] = RGB16(src16[1]);
			dst16[ 2] = dst16[1];
			dst16[ 3] = RGB16(src16[2]);
			dst16[ 4] = RGB16(src16[3]);
			dst16[ 5] = RGB16(src16[4]);
			dst16[ 6] = RGB16(src16[5]);
			dst16[ 7] = dst16[6];
			dst16[ 8] = RGB16(src16[6]);
			dst16[ 9] = RGB16(src16[7]);
			dst16 += 10;
			src16 +=  8;
		} while (--uCount);
#else
		uCount = 64;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			*dst16++ = *src16++;
			*dst16++ = *src16;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
			*dst16++ = *src16++;
		} while (--uCount);
#endif
	} else
	{
		uCount = 32;
		const uint8_t* src8 = (const uint8_t*)src;
		do {
			dst16[ 0] = RGB24(src8[0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[3], src8[ 4], src8[ 5] );
			dst16[ 2] = dst16[1];
			dst16[ 3] = RGB24(src8[6], src8[ 7], src8[ 8] );
			dst16[ 4] = RGB24(src8[9], src8[10], src8[11] );

			dst16[ 5] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 6] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 7] = dst16[6];
			dst16[ 8] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 9] = RGB24(src8[21], src8[22], src8[23] );
			dst16 += 10;
			src8  += 24;
		} while (--uCount);
	}
}


static inline void GPU_BlitWS(const void* src, uint16_t* dst16, uint_fast8_t isRGB24)
{
	uint32_t uCount;
	if (!isRGB24) {
#ifndef USE_BGR15
		uCount = 20;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			dst16[ 0] = RGB16(src16[0]);
			dst16[ 1] = RGB16(src16[2]);
			dst16[ 2] = RGB16(src16[4]);
			dst16[ 3] = RGB16(src16[6]);

			dst16[ 4] = RGB16(src16[8]);
			dst16[ 5] = RGB16(src16[10]);
			dst16[ 6] = RGB16(src16[12]);
			dst16[ 7] = RGB16(src16[14]);

			dst16[ 8] = RGB16(src16[16]);
			dst16[ 9] = RGB16(src16[18]);
			dst16[10] = RGB16(src16[20]);
			dst16[11] = RGB16(src16[22]);

			dst16[12] = RGB16(src16[24]);
			dst16[13] = RGB16(src16[26]);
			dst16[14] = RGB16(src16[28]);
			dst16[15] = RGB16(src16[30]);

			dst16 += 16;
			src16 += 32;
		} while (--uCount);
#else
		uCount = 320;
		const uint16_t* src16 = (const uint16_t*) src;
		do {
			*dst16++ = *src16;
			src16 += 2;
		} while (--uCount);
#endif
	} else
	{
		uCount = 20;
		const uint8_t* src8 = (const uint8_t*) src;
		do {
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 2] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 3] = RGB24(src8[18], src8[19], src8[20] );

			dst16[ 4] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 5] = RGB24(src8[30], src8[31], src8[32] );
			dst16[ 6] = RGB24(src8[36], src8[37], src8[38] );
			dst16[ 7] = RGB24(src8[42], src8[43], src8[44] );

			dst16[ 8] = RGB24(src8[48], src8[49], src8[50] );
			dst16[ 9] = RGB24(src8[54], src8[55], src8[56] );
			dst16[10] = RGB24(src8[60], src8[61], src8[62] );
			dst16[11] = RGB24(src8[66], src8[67], src8[68] );

			dst16[12] = RGB24(src8[72], src8[73], src8[74] );
			dst16[13] = RGB24(src8[78], src8[79], src8[80] );
			dst16[14] = RGB24(src8[84], src8[85], src8[86] );
			dst16[15] = RGB24(src8[90], src8[91], src8[92] );

			dst16 += 16;
			src8  += 96;
		} while(--uCount);
	}
}

#else

#ifdef BGR_PCSX
#undef RGB24
#define RGB24(R,G,B)	(((((R)&0xF8)>>3)|(((G)&0xF8)<<2)|(((B)&0xF8)<<7)))
#endif

static inline void GPU_BlitCopy(const void* src, uint16_t* dst16, uint_fast8_t isRGB24)
{
	uint32_t uCount;
	if (!isRGB24)
	{
#ifndef USE_BGR15
		uCount = SCREEN_WIDTH / 16;
		const uint32_t* src32 = (const uint32_t*) src;
		uint32_t* dst32 = (uint32_t*)(void*) dst16;
		do {
			dst32[0] = RGB16X2(src32[0]);
			dst32[1] = RGB16X2(src32[1]);
			dst32[2] = RGB16X2(src32[2]);
			dst32[3] = RGB16X2(src32[3]);
			dst32[4] = RGB16X2(src32[4]);
			dst32[5] = RGB16X2(src32[5]);
			dst32[6] = RGB16X2(src32[6]);
			dst32[7] = RGB16X2(src32[7]);
			dst32 += 8;
			src32 += 8;
		} while(--uCount);
#else
		memcpy(dst16, src, SCREEN_WIDTH * 2);
#endif
	} else
	{
		uCount = SCREEN_WIDTH / 16;
		const uint8_t* src8 = (const uint8_t*)src;
		do{
			dst16[ 0] = RGB24(src8[ 0], src8[ 1], src8[ 2] );
			dst16[ 1] = RGB24(src8[ 3], src8[ 4], src8[ 5] );
			dst16[ 2] = RGB24(src8[ 6], src8[ 7], src8[ 8] );
			dst16[ 3] = RGB24(src8[ 9], src8[10], src8[11] );
			dst16[ 4] = RGB24(src8[12], src8[13], src8[14] );
			dst16[ 5] = RGB24(src8[15], src8[16], src8[17] );
			dst16[ 6] = RGB24(src8[18], src8[19], src8[20] );
			dst16[ 7] = RGB24(src8[21], src8[22], src8[23] );

			dst16[ 8] = RGB24(src8[24], src8[25], src8[26] );
			dst16[ 9] = RGB24(src8[27], src8[28], src8[29] );
			dst16[10] = RGB24(src8[30], src8[31], src8[32] );
			dst16[11] = RGB24(src8[33], src8[34], src8[35] );
			dst16[12] = RGB24(src8[36], src8[37], src8[38] );
			dst16[13] = RGB24(src8[39], src8[40], src8[41] );
			dst16[14] = RGB24(src8[42], src8[43], src8[44] );
			dst16[15] = RGB24(src8[45], src8[46], src8[47] );
			dst16 += 16;
			src8  += 48;
		} while (--uCount);
	}
}

#endif

// Basically an adaption of old gpu_unai/gpu.cpp's gpuVideoOutput() that
//  assumes 320x240 destination resolution (for now)
// TODO: clean up / improve
void vout_update(void)
{
	//Debugging:
#if 0
	if (gpu.screen.w != gpu.screen.hres) {
		int start_x = (gpu.screen.x1 - 0x260) * gpu.screen.hres / 2560;
		int end_x = (gpu.screen.x2 - 0x260) * gpu.screen.hres / 2560;
		int rounded_w= (((gpu.screen.x2 - gpu.screen.x1) / 2560) + 2) & (~3);
		printf("screen.w: %d  screen.hres: %d  rounded_w:%d\n", gpu.screen.w, gpu.screen.hres, rounded_w);
		printf("start_x: %d  end_x: %d  x1: %d  x2: %d\n", start_x, end_x, gpu.screen.x1, gpu.screen.x2);
	}
#endif

	int x0 = gpu.screen.x;
	int y0 = gpu.screen.y;
	int w0 = gpu.screen.hres;
	int h0 = gpu.screen.vres;
	int h1 = gpu.screen.h;     // height of image displayed on screen

	if (w0 == 0 || h0 == 0)
		return;

	uint_fast8_t isRGB24 = gpu.status.rgb24;
	uint16_t* dst16 = SCREEN;
	uint16_t* src16 = (uint16_t*)gpu.vram;

	// PS1 fb read wraps around (fixes black screen in 'Tobal no. 1')
	unsigned int src16_offs_msk = 1024*512-1;
	unsigned int src16_offs = (x0 + y0*1024u) & src16_offs_msk;

#ifndef HW_SCALE
	//  Height centering
	int sizeShift = 1;
	if (h0 == 256) {
		h0 = 240;
	} else if (h0 == 480) {
		sizeShift = 2;
	}
	if (h1 > h0) {
		src16_offs = (src16_offs + (((h1-h0) / 2) * 1024)) & src16_offs_msk;
		h1 = h0;
	} else if (h1 < h0) {
		dst16 += ((h0-h1) >> sizeShift) * SCREEN_WIDTH;
	}

	int incY = (h0 == 480) ? 2 : 1;
	h0 = ((h0 == 480) ? 2048 : 1024);

	switch ( w0 )
	{
		case 256: {
			for (int y1=y0+h1; y0<y1; y0+=incY) {
				GPU_BlitWWDWW(src16 + src16_offs, dst16, isRGB24);
				dst16 += SCREEN_WIDTH;
				src16_offs = (src16_offs + h0) & src16_offs_msk;
			}
		} break;

		case 368: {
			for (int y1=y0+h1; y0<y1; y0+=incY) {
				GPU_BlitWWWWWWWWS(src16 + src16_offs, dst16, isRGB24, 4);
				dst16 += SCREEN_WIDTH;
				src16_offs = (src16_offs + h0) & src16_offs_msk;
			}
		} break;

		case 320: {
			// Ensure 32-bit alignment for GPU_BlitWW() blitter:
			src16_offs &= ~1;
			for (int y1=y0+h1; y0<y1; y0+=incY) {
				GPU_BlitWW(src16 + src16_offs, dst16, isRGB24);
				dst16 += SCREEN_WIDTH;
				src16_offs = (src16_offs + h0) & src16_offs_msk;
			}
		} break;

		case 384: {
			for (int y1=y0+h1; y0<y1; y0+=incY) {
				GPU_BlitWWWWWS(src16 + src16_offs, dst16, isRGB24);
				dst16 += SCREEN_WIDTH;
				src16_offs = (src16_offs + h0) & src16_offs_msk;
			}
		} break;

		case 512: {
			for (int y1=y0+h1; y0<y1; y0+=incY) {
				GPU_BlitWWSWWSWS(src16 + src16_offs, dst16, isRGB24);
				dst16 += SCREEN_WIDTH;
				src16_offs = (src16_offs + h0) & src16_offs_msk;
			}
		} break;

		case 640: {
			for (int y1=y0+h1; y0<y1; y0+=incY) {
				GPU_BlitWS(src16 + src16_offs, dst16, isRGB24);
				dst16 += SCREEN_WIDTH;
				src16_offs = (src16_offs + h0) & src16_offs_msk;
			}
		} break;
	}
#else
	if (h1 > h0) {
		src16_offs = (src16_offs + (((h1-h0) >> 1) * 1024)) & src16_offs_msk;
		h1 = h0;
	} else if (h1 < h0) {
		dst16 += ((h0-h1) >> 1) * SCREEN_WIDTH;
	}

	src16_offs &= ~1u;
#ifdef BGR_PCSX
	if (isRGB24) {
		for (int y1 = y0+h1; y0<y1; y0++) {
			GPU_BlitCopy(src16+src16_offs, dst16, isRGB24);
			dst16 += SCREEN_WIDTH;
			src16_offs = (src16_offs+1024) & src16_offs_msk;
		}
	} else {
		for (int y1 = y0+h1; y0<y1; y0++) {
			memcpy(dst16, src16+src16_offs, 2 * SCREEN_WIDTH);
			dst16 += SCREEN_WIDTH;
			src16_offs = (src16_offs+1024) & src16_offs_msk;
		}
	}
#else
	for (int y1 = y0+h1; y0<y1; y0++) {
		GPU_BlitCopy(src16+src16_offs, dst16, isRGB24);
		dst16 += SCREEN_WIDTH;
		src16_offs = (src16_offs+1024) & src16_offs_msk;
	}
#endif
#endif
	video_flip();
}

int vout_init(void)
{
	return 0;
}

int vout_finish(void)
{
	return 0;
}

//senquack - Handles PSX display disabling (TODO: implement?)
void vout_blank(void)
{
}

void vout_set_config(const struct gpulib_config_t *config)
{
}
