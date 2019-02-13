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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "port.h"
#include "gpu.h"

///////////////////////////////////////////////////////////////////////////////
// BLITTERS TAKEN FROM gpu_unai/gpu_blit.h
// GPU Blitting code with rescale and interlace support.
///////////////////////////////////////////////////////////////////////////////
#ifndef USE_BGR15
  #define RGB24(R,G,B)	(((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
  #define RGB16X2(C)      (((C)&(0x1f001f<<10))>>10) | (((C)&(0x1f001f<<5))<<1) | (((C)&(0x1f001f<<0))<<11)
  //#define RGB16(C)		(((C)&(0x1f<<10))>>10) | (((C)&(0x1f<<5))<<1) | (((C)&(0x1f<<0))<<11)
  //#define RGB16(C)		( (C)>>10 | ((C) & 0x03e0)<<1 | ((C) & 0x1f)<<11 )
  #define RGB16(C)		( ((C) & 0x7c00)>>10 | ((C) & 0x03e0)<<1 | ((C) & 0x1f)<<11 )
#else
  #define RGB24(R,G,B)  	((((R)&0xF8)>>3)|(((G)&0xF8)<<2)|(((B)&0xF8)<<7))
#endif

#define u8 uint8_t
#define s8 int8_t
#define u16 uint16_t
#define s16 int16_t
#define u32 uint32_t
#define s32 int32_t
#define s64 int64_t

uint8_t use_clip_368;

static inline u16 middle(u16 s1, u16 s2, u16 s3)
{
  u16 x, y, temp;
  u16 a[] = {s1, s2, s3};

  for(y=0; y<2; y++)
  {
    for(x=1; x<3; x++)
    {
      if(a[x] > a[y])
      {
        temp = a[x];
        a[x] = a[y];
        a[y] = temp;
      }
    }
  }
  return a[1];
}

static inline u16 mask_filter(u16 *s)
{
  /*
  u16 r0 = (s[0] & 0x7c00) >> 10;
  u16 r1 = (s[1] & 0x7c00) >> 10;

  u16 g0 = (s[0] & 0x03e0) >> 5;
  u16 g1 = (s[1] & 0x03e0) >> 5;

  u16 b0 = s[0] & 0x1f;
  u16 b1 = s[1] & 0x1f;

  u16 r = (r0 + r1) / 2;
  u16 g = (g0 + g1) / 2;
  u16 b = (b0 + b1) / 2;

  return (b<<11) | (g<<6) | r;*/

  u16 c1 = s[0];
  u16 c2 = s[1];
  u16 r = (u16)((u16)(c1 & 0x7c00) + (u16)(c2 & 0x7c00)) >> 11;
  u16 g = (u16)((u16)(c1 & 0x03e0) + (u16)(c2 & 0x03e0)); //>> 6
  u16 b = (u16)((u16)(c1 & 0x001f) + (u16)(c2 & 0x001f)) >> 1;

  return (b<<11) | (g) | r; //(g<<6)
}

int32_t Min2 (const int32_t _a, const int32_t _b)
{
	return (_a<_b)?_a:_b;
}

// GPU_BlitWW
static inline void GPU_Blit320(const void* src, u16* dst16, uint8_t isRGB24)
{
  u32 uCount;
  if (!isRGB24)
  {
#ifndef USE_BGR15
    uCount = 20;
    const u32* src32 = (const u32*) src;
    u32* dst32 = (u32*)(void*) dst16;
    do
    {
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
    }
    while(--uCount);
#else
    memcpy(dst16, src, 640);
#endif
  }
  else
  {
    uCount = 20;
    u8* src8 = (u8*)src;
    do
    {
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
    }
    while (--uCount);
  }
}

//  GPU_BlitWWSWWSWS
static inline void GPU_Blit512(const void* src, u16* dst16, uint8_t isRGB24)
{
  u32 uCount;
  if (!isRGB24)
  {
#ifndef USE_BGR15
    uCount = 32;
    u16* src16 = (u16*)src;
    do
    {
#if 0
      dst16[ 0] = RGB16(src16[0]);
      dst16[ 1] = RGB16(src16[1]);
      dst16[ 2] = RGB16(src16[3]);
      dst16[ 3] = RGB16(src16[4]);
      dst16[ 4] = RGB16(src16[6]);
      dst16[ 5] = RGB16(src16[8]);
      dst16[ 6] = RGB16(src16[9]);
      dst16[ 7] = RGB16(src16[11]);
      dst16[ 8] = RGB16(src16[12]);
      dst16[ 9] = RGB16(src16[14]);
#else
      dst16[ 0] = RGB16(src16[0]);
      dst16[ 1] = mask_filter(&src16[1]);
      dst16[ 2] = RGB16(src16[3]);
      dst16[ 3] = mask_filter(&src16[4]);
      dst16[ 4] = mask_filter(&src16[6]);
      dst16[ 5] = RGB16(src16[8]);
      dst16[ 6] = mask_filter(&src16[9]);
      dst16[ 7] = RGB16(src16[11]);
      dst16[ 8] = mask_filter(&src16[12]);
      dst16[ 9] = mask_filter(&src16[14]);
#endif
      dst16 += 10;
      src16 += 16;
    }
    while (--uCount);
#else
    uCount = 64;
    const u16* src16 = (const u16*) src;
    do
    {
      *dst16++ = *src16++;
      *dst16++ = *src16;
      src16 += 2;
      *dst16++ = *src16++;
      *dst16++ = *src16;
      src16 += 2;
      *dst16++ = *src16;
      src16 += 2;
    }
    while (--uCount);
#endif
  }
  else
  {
    uCount = 32;
    const u8* src8 = (const u8*)src;
    do
    {
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
    }
    while (--uCount);
  }
}

//  GPU_BlitWWWWWS
static inline void GPU_Blit384(const void* src, u16* dst16, uint8_t isRGB24)
{
  u32 uCount;
  if (!isRGB24)
  {
#ifndef USE_BGR15
    uCount = 32;
    u16* src16 = (u16*)src;
    do
    {
#if 0
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
#else
      dst16[ 0] = RGB16(src16[0]);
      dst16[ 1] = RGB16(src16[1]);
      dst16[ 2] = RGB16(src16[2]);
      dst16[ 3] = RGB16(src16[3]);
      dst16[ 4] = mask_filter(&src16[4]);
      dst16[ 5] = RGB16(src16[6]);
      dst16[ 6] = RGB16(src16[7]);
      dst16[ 7] = RGB16(src16[8]);
      dst16[ 8] = RGB16(src16[9]);
      dst16[ 9] = mask_filter(&src16[10]);
#endif
      dst16 += 10;
      src16 += 12;
    }
    while (--uCount);
#else
    uCount = 64;
    const u16* src16 = (const u16*) src;
    do
    {
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16;
      src16 += 2;
    }
    while (--uCount);
#endif
  }
  else
  {
    uCount = 32;
    const u8* src8 = (const u8*)src;
    do
    {
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
    }
    while (--uCount);
  }
}

//  GPU_Blit368: 320 -6 -6 = 308
static inline void GPU_Blit368_clip(const void* src, u16* dst16)
{
  /*
  u32 uCount;
  u16* src16 = ((u16*) src);
  uCount = 22;
  //
  do {
  	dst16[ 0] = RGB16(src16[0]);
  	dst16[ 1] = RGB16(src16[1]);
  	dst16[ 2] = RGB16(src16[2]);
  	dst16[ 3] = RGB16(src16[3]);
  	dst16[ 4] = RGB16(src16[4]);
  	dst16[ 5] = RGB16(src16[5]);
  	dst16[ 6] = mask_filter(&src16[6]);
  	dst16[ 7] = RGB16(src16[8]);
  	dst16[ 8] = RGB16(src16[9]);
  	dst16[ 9] = RGB16(src16[10]);
  	dst16[10] = RGB16(src16[11]);
  	dst16[11] = RGB16(src16[12]);
  	dst16[12] = RGB16(src16[13]);
  	dst16[13] = mask_filter(&src16[14]);
  	dst16 += 14;
  	src16 += 16;
  } while (--uCount);   //6 + 22 * 14 = 314, 320 - 314 = 6
  // 补齐末尾
  dst16[0] = dst16[1] = dst16[2] = dst16[3] = dst16[4] = dst16[5] = 0;*/
  u32 uCount;
  u16* src16 = ((u16*) src);
  uCount = 32;
  //
  do
  {
    dst16[ 0] = RGB16(src16[0]);
    dst16[ 1] = RGB16(src16[1]);
    dst16[ 2] = RGB16(src16[2]);
    dst16[ 3] = RGB16(src16[3]);
    dst16[ 4] = RGB16(src16[4]);
    dst16[ 5] = RGB16(src16[5]);
    dst16[ 6] = RGB16(src16[6]);
    dst16[ 7] = RGB16(src16[7]);
    dst16[ 8] = RGB16(src16[8]);
    dst16[ 9] = mask_filter(&src16[9]);
    dst16 += 10;
    src16 += 11;
  }
  while (--uCount);
}


//  GPU_BlitWWWWWWWWS
static inline void GPU_Blit368(const void* src, u16* dst16, uint8_t isRGB24/*, u32 uClip_src*/)
{
  u32 uCount;
  if (!isRGB24)
  {
#ifndef USE_BGR15
    u16* src16 = ((u16*) src)/* + uClip_src*/;
#if 0
    uCount = 20;
    do
    {
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
    }
    while (--uCount);
#else
    {
      /*
      u16 x, cnt=0, sx=0;
      for(x=0; x<320; x++){
      	sx++;
      	if(++cnt == 7){
      		cnt = 0;
      		dst16[x] = mask_filter(&src16[sx]);
      		sx++;
      	}
      	else{
      		dst16[x] = RGB16(src16[sx]);
      	}
      }*/
    }
    uCount = 22;
    dst16[0] = RGB16(src16[0]);
    dst16++;
    src16++;
    do
    {
      dst16[ 0] = RGB16(src16[0]);
      dst16[ 1] = RGB16(src16[1]);
      dst16[ 2] = RGB16(src16[2]);
      dst16[ 3] = RGB16(src16[3]);
      dst16[ 4] = RGB16(src16[4]);
      dst16[ 5] = RGB16(src16[5]);
      dst16[ 6] = mask_filter(&src16[6]);
      dst16[ 7] = RGB16(src16[8]);
      dst16[ 8] = RGB16(src16[9]);
      dst16[ 9] = RGB16(src16[10]);
      dst16[10] = RGB16(src16[11]);
      dst16[11] = RGB16(src16[12]);
      dst16[12] = RGB16(src16[13]);
      dst16[13] = mask_filter(&src16[14]);
      dst16 += 14;
      src16 += 16;
    }
    while (--uCount);     //1 + 22 * 14 = 309, 320 - 309 = 11
    {
      // 补齐末尾
      dst16[ 0] = RGB16(src16[0]);
      dst16[ 1] = RGB16(src16[1]);
      dst16[ 2] = RGB16(src16[2]);
      dst16[ 3] = RGB16(src16[3]);
      dst16[ 4] = RGB16(src16[4]);
      dst16[ 5] = RGB16(src16[5]);
      dst16[ 6] = mask_filter(&src16[6]);
      dst16[ 7] = RGB16(src16[8]);
      dst16[ 8] = RGB16(src16[9]);
      dst16[ 9] = RGB16(src16[10]);
      dst16[10] = RGB16(src16[11]);
    }
#endif
#else
    uCount = 40;
    const u16* src16 = ((const u16*) src)/* + uClip_src*/;
    do
    {
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16;
      src16 += 2;
    }
    while (--uCount);
#endif
  }
  else
  {
    uCount = 20;
    const u8* src8 = (const u8*)src/* + (uClip_src<<1) + uClip_src*/;
    do
    {
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
    }
    while (--uCount);
  }
}


// GPU_BlitWWDWW
static inline void GPU_Blit256(const void* src, u16* dst16, uint8_t isRGB24)
{
  u32 uCount;
  if (!isRGB24)
  {
#ifndef USE_BGR15
    uCount = 32;
    const u16* src16 = (const u16*) src;
    do
    {
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
    }
    while (--uCount);
#else
    uCount = 64;
    const u16* src16 = (const u16*) src;
    do
    {
      *dst16++ = *src16++;
      *dst16++ = *src16;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
      *dst16++ = *src16++;
    }
    while (--uCount);
#endif
  }
  else
  {
    uCount = 32;
    const u8* src8 = (const u8*)src;
    do
    {
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
    }
    while (--uCount);
  }
}


// GPU_BlitWS
static inline void GPU_Blit640(const void* src, u16* dst16, uint8_t isRGB24)
{
  u32 uCount;
  if (!isRGB24)
  {
#ifndef USE_BGR15
    uCount = 20;
    u16* src16 = (u16*)src;
    do
    {
#if 0
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
#else
      dst16[ 0] = mask_filter(&src16[0]);
      dst16[ 1] = mask_filter(&src16[2]);
      dst16[ 2] = mask_filter(&src16[4]);
      dst16[ 3] = mask_filter(&src16[6]);

      dst16[ 4] = mask_filter(&src16[8]);
      dst16[ 5] = mask_filter(&src16[10]);
      dst16[ 6] = mask_filter(&src16[12]);
      dst16[ 7] = mask_filter(&src16[14]);

      dst16[ 8] = mask_filter(&src16[16]);
      dst16[ 9] = mask_filter(&src16[18]);
      dst16[10] = mask_filter(&src16[20]);
      dst16[11] = mask_filter(&src16[22]);

      dst16[12] = mask_filter(&src16[24]);
      dst16[13] = mask_filter(&src16[26]);
      dst16[14] = mask_filter(&src16[28]);
      dst16[15] = mask_filter(&src16[30]);
#endif

      dst16 += 16;
      src16 += 32;
    }
    while (--uCount);
#else
    uCount = 320;
    const u16* src16 = (const u16*) src;
    do
    {
      *dst16++ = *src16;
      src16 += 2;
    }
    while (--uCount);
#endif
  }
  else
  {
    uCount = 20;
    const u8* src8 = (const u8*) src;
    do
    {
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
    }
    while(--uCount);
  }
}

extern volatile uint16_t *dma_buffer;

// Basically an adaption of old gpu_unai/gpu.cpp's gpuVideoOutput() that
// assumes 320x240 destination resolution (for now)
// TODO: clean up / improve / add HW scaling support
void vout_update(void)
{
  const int VIDEO_WIDTH = 320;
  
  //Debugging:
#if 0
  if (gpu.screen.w != gpu.screen.hres)
  {
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
  {
    return;
  }

  uint8_t isRGB24 = gpu.status.rgb24;
  u16* dst16 = (u16*)SCREEN;
  u16* src16 = (u16*)gpu.vram;

  // PS1 fb read wraps around (fixes black screen in 'Tobal no. 1')
  unsigned int src16_offs_msk = 1024*512-1;
  unsigned int src16_offs = (x0 + y0*1024) & src16_offs_msk;

  //  Height centering
  int sizeShift = 1;
  if (h0 == 256)
  {
    h0 = 240;
  }
  else if (h0 == 480)
  {
    sizeShift = 2;
  }

  if (h1 > h0)
  {
    src16_offs = (src16_offs + (((h1-h0) / 2) * 1024)) & src16_offs_msk;
    h1 = h0;
  }
  else if (h1 < h0)
  {
    dst16 += (((h0-h1) >> sizeShift) * VIDEO_WIDTH);
  }

  int incY = (h0 == 480) ? 2 : 1;
  h0 = ((h0 == 480) ? 2048 : 1024);
  switch ( w0 )
  {
    case 256:
    {
      for (int y1=y0+h1; y0<y1; y0+=incY)
      {
        GPU_Blit256(src16 + src16_offs, dst16, isRGB24);
        dst16 += VIDEO_WIDTH;
        src16_offs = (src16_offs + h0) & src16_offs_msk;
      }
    }
    break;

    case 368:
    {
      for (int y1=y0+h1; y0<y1; y0+=incY)   //SLPS02124
      {
        if (use_clip_368 == 0 || isRGB24)
        {
          GPU_Blit368(src16 + src16_offs, dst16, isRGB24);
        }
        else
        {
          GPU_Blit368_clip(src16 + src16_offs, dst16);
        }
        dst16 += VIDEO_WIDTH;
        src16_offs = (src16_offs + h0) & src16_offs_msk;
      }
    }
    break;

    case 320:
    {
      // Ensure 32-bit alignment for GPU_BlitWW() blitter:
      src16_offs &= ~1;
      for (int y1=y0+h1; y0<y1; y0+=incY)
      {
        GPU_Blit320(src16 + src16_offs, dst16, isRGB24);
        dst16 += VIDEO_WIDTH;
        src16_offs = (src16_offs + h0) & src16_offs_msk;
      }
    }
    break;

    case 384:
    {
      for (int y1=y0+h1; y0<y1; y0+=incY)
      {
        GPU_Blit384(src16 + src16_offs, dst16, isRGB24);
        dst16 += VIDEO_WIDTH;
        src16_offs = (src16_offs + h0) & src16_offs_msk;
      }
    }
    break;

    case 512:
    {
      for (int y1=y0+h1; y0<y1; y0+=incY)
      {
        GPU_Blit512(src16 + src16_offs, dst16, isRGB24);
        dst16 += VIDEO_WIDTH;
        src16_offs = (src16_offs + h0) & src16_offs_msk;
      }
    }
    break;

    case 640:
    {
      for (int y1=y0+h1; y0<y1; y0+=incY)
      {
        GPU_Blit640(src16 + src16_offs, dst16, isRGB24);
        dst16 += VIDEO_WIDTH;
        src16_offs = (src16_offs + h0) & src16_offs_msk;
      }
    }
    break;
  }

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
