#ifndef _INNER_BLIT_H_
#define _INNER_BLIT_H_

INLINE void GPU_BlitWW(const void* src, Uint16* dst16, Uint32 isRGB24)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 20;
    const Uint32* src32 = (const Uint32*) src; 
          Uint32* dst32 = (      Uint32*) dst16;
    do{
                            //B                                 //G                          //R
      dst32[0] = ((src32[0]&(0x1f001f<<10))>>10) | ((src32[0]&(0x1f001f<<5))<<1) | ((src32[0]&(0x1f001f<<0))<<11);
      dst32[1] = ((src32[1]&(0x1f001f<<10))>>10) | ((src32[1]&(0x1f001f<<5))<<1) | ((src32[1]&(0x1f001f<<0))<<11);
      dst32[2] = ((src32[2]&(0x1f001f<<10))>>10) | ((src32[2]&(0x1f001f<<5))<<1) | ((src32[2]&(0x1f001f<<0))<<11);
      dst32[3] = ((src32[3]&(0x1f001f<<10))>>10) | ((src32[3]&(0x1f001f<<5))<<1) | ((src32[3]&(0x1f001f<<0))<<11);
      dst32[4] = ((src32[4]&(0x1f001f<<10))>>10) | ((src32[4]&(0x1f001f<<5))<<1) | ((src32[4]&(0x1f001f<<0))<<11);
      dst32[5] = ((src32[5]&(0x1f001f<<10))>>10) | ((src32[5]&(0x1f001f<<5))<<1) | ((src32[5]&(0x1f001f<<0))<<11);
      dst32[6] = ((src32[6]&(0x1f001f<<10))>>10) | ((src32[6]&(0x1f001f<<5))<<1) | ((src32[6]&(0x1f001f<<0))<<11);
      dst32[7] = ((src32[7]&(0x1f001f<<10))>>10) | ((src32[7]&(0x1f001f<<5))<<1) | ((src32[7]&(0x1f001f<<0))<<11);
      dst32 += 8;
      src32 += 8;
    }while(--uCount);
  }
	else
  {
    uCount = 20;
    const Uint8* src8 = (const Uint8*)src;
    do{
      dst16[ 0] = video_RGB_color16(src8[ 0], src8[ 1], src8[ 2] );
      dst16[ 1] = video_RGB_color16(src8[ 3], src8[ 4], src8[ 5] );
      dst16[ 2] = video_RGB_color16(src8[ 6], src8[ 7], src8[ 8] );
      dst16[ 3] = video_RGB_color16(src8[ 9], src8[10], src8[11] );
      dst16[ 4] = video_RGB_color16(src8[12], src8[13], src8[14] );
      dst16[ 5] = video_RGB_color16(src8[15], src8[16], src8[17] );
      dst16[ 6] = video_RGB_color16(src8[18], src8[19], src8[20] );
      dst16[ 7] = video_RGB_color16(src8[21], src8[22], src8[23] );

      dst16[ 8] = video_RGB_color16(src8[24], src8[25], src8[26] );
      dst16[ 9] = video_RGB_color16(src8[27], src8[28], src8[29] );
      dst16[10] = video_RGB_color16(src8[30], src8[31], src8[32] );
      dst16[11] = video_RGB_color16(src8[33], src8[34], src8[35] );
      dst16[12] = video_RGB_color16(src8[36], src8[37], src8[38] );
      dst16[13] = video_RGB_color16(src8[39], src8[40], src8[41] );
      dst16[14] = video_RGB_color16(src8[42], src8[43], src8[44] );
      dst16[15] = video_RGB_color16(src8[45], src8[46], src8[47] );
      dst16 += 16;
      src8  += 48;
    }while(--uCount);
  }
}

INLINE void GPU_BlitWWSWWSWS(const void* src, Uint16* dst16, Uint32 isRGB24)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 32;
    const Uint16* src16 = (const Uint16*) src; 
    do{
                            //B                                 //G                          //R
      dst16[ 0] = ((src16[ 0]&(0x1f001f<<10))>>10) | ((src16[ 0]&(0x1f001f<<5))<<1) | ((src16[ 0]&(0x1f001f<<0))<<11);
      dst16[ 1] = ((src16[ 1]&(0x1f001f<<10))>>10) | ((src16[ 1]&(0x1f001f<<5))<<1) | ((src16[ 1]&(0x1f001f<<0))<<11);
      dst16[ 2] = ((src16[ 3]&(0x1f001f<<10))>>10) | ((src16[ 3]&(0x1f001f<<5))<<1) | ((src16[ 3]&(0x1f001f<<0))<<11);
      dst16[ 3] = ((src16[ 4]&(0x1f001f<<10))>>10) | ((src16[ 4]&(0x1f001f<<5))<<1) | ((src16[ 4]&(0x1f001f<<0))<<11);
      dst16[ 4] = ((src16[ 6]&(0x1f001f<<10))>>10) | ((src16[ 6]&(0x1f001f<<5))<<1) | ((src16[ 6]&(0x1f001f<<0))<<11);
      dst16[ 5] = ((src16[ 8]&(0x1f001f<<10))>>10) | ((src16[ 8]&(0x1f001f<<5))<<1) | ((src16[ 8]&(0x1f001f<<0))<<11);
      dst16[ 6] = ((src16[ 9]&(0x1f001f<<10))>>10) | ((src16[ 9]&(0x1f001f<<5))<<1) | ((src16[ 9]&(0x1f001f<<0))<<11);
      dst16[ 7] = ((src16[11]&(0x1f001f<<10))>>10) | ((src16[11]&(0x1f001f<<5))<<1) | ((src16[11]&(0x1f001f<<0))<<11);
      dst16[ 8] = ((src16[12]&(0x1f001f<<10))>>10) | ((src16[12]&(0x1f001f<<5))<<1) | ((src16[12]&(0x1f001f<<0))<<11);
	    dst16[ 9] = ((src16[14]&(0x1f001f<<10))>>10) | ((src16[14]&(0x1f001f<<5))<<1) | ((src16[14]&(0x1f001f<<0))<<11);
	    dst16 += 10;
      src16 += 16;
    }while(--uCount);
  }
else
  {
    uCount = 32;
    const Uint8* src8 = (const Uint8*)src;
    do{
      dst16[ 0] = video_RGB_color16(src8[ 0], src8[ 1], src8[ 2] );
      dst16[ 1] = video_RGB_color16(src8[ 3], src8[ 4], src8[ 5] );
      dst16[ 2] = video_RGB_color16(src8[ 9], src8[10], src8[11] );
      dst16[ 3] = video_RGB_color16(src8[12], src8[13], src8[14] );
      dst16[ 4] = video_RGB_color16(src8[18], src8[19], src8[20] );

      dst16[ 5] = video_RGB_color16(src8[24], src8[25], src8[26] );
      dst16[ 6] = video_RGB_color16(src8[27], src8[28], src8[29] );
      dst16[ 7] = video_RGB_color16(src8[33], src8[34], src8[35] );
      dst16[ 8] = video_RGB_color16(src8[36], src8[37], src8[38] );
      dst16[ 9] = video_RGB_color16(src8[42], src8[43], src8[44] );

      dst16 += 10;
      src8  += 48;
    }while(--uCount);
  }
}

INLINE void GPU_BlitWWWWWS(const void* src, Uint16* dst16, Uint32 isRGB24)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 32;
    const Uint16* src16 = (const Uint16*) src; 
    do{
							//B                                 //G                          //R
      dst16[ 0] = ((src16[0]&(0x1f001f<<10))>>10) | ((src16[0]&(0x1f001f<<5))<<1) | ((src16[0]&(0x1f001f<<0))<<11);
      dst16[ 1] = ((src16[1]&(0x1f001f<<10))>>10) | ((src16[1]&(0x1f001f<<5))<<1) | ((src16[1]&(0x1f001f<<0))<<11);
      dst16[ 2] = ((src16[2]&(0x1f001f<<10))>>10) | ((src16[2]&(0x1f001f<<5))<<1) | ((src16[2]&(0x1f001f<<0))<<11);
      dst16[ 3] = ((src16[3]&(0x1f001f<<10))>>10) | ((src16[3]&(0x1f001f<<5))<<1) | ((src16[3]&(0x1f001f<<0))<<11);
	    dst16[ 4] = ((src16[4]&(0x1f001f<<10))>>10) | ((src16[4]&(0x1f001f<<5))<<1) | ((src16[4]&(0x1f001f<<0))<<11);
      dst16[ 5] = ((src16[6]&(0x1f001f<<10))>>10) | ((src16[6]&(0x1f001f<<5))<<1) | ((src16[6]&(0x1f001f<<0))<<11);
      dst16[ 6] = ((src16[7]&(0x1f001f<<10))>>10) | ((src16[7]&(0x1f001f<<5))<<1) | ((src16[7]&(0x1f001f<<0))<<11);
      dst16[ 7] = ((src16[8]&(0x1f001f<<10))>>10) | ((src16[8]&(0x1f001f<<5))<<1) | ((src16[8]&(0x1f001f<<0))<<11);
      dst16[ 8] = ((src16[9]&(0x1f001f<<10))>>10) | ((src16[9]&(0x1f001f<<5))<<1) | ((src16[9]&(0x1f001f<<0))<<11);
	    dst16[ 9] = ((src16[10]&(0x1f001f<<10))>>10) | ((src16[10]&(0x1f001f<<5))<<1) | ((src16[10]&(0x1f001f<<0))<<11);
	    dst16 += 10;
      src16 += 12;
    }while(--uCount);
  }
  else
  {
    uCount = 32;
    const Uint8* src8 = (const Uint8*)src;
    do{
      dst16[0] = video_RGB_color16(src8[ 0], src8[ 1], src8[ 2] );
      dst16[1] = video_RGB_color16(src8[ 3], src8[ 4], src8[ 5] );
      dst16[2] = video_RGB_color16(src8[ 6], src8[ 7], src8[ 8] );
      dst16[3] = video_RGB_color16(src8[ 9], src8[10], src8[11] );
	    dst16[4] = video_RGB_color16(src8[12], src8[13], src8[14] );
      dst16[5] = video_RGB_color16(src8[18], src8[19], src8[20] );
      dst16[6] = video_RGB_color16(src8[21], src8[22], src8[23] );
      dst16[7] = video_RGB_color16(src8[24], src8[25], src8[26] );
      dst16[8] = video_RGB_color16(src8[27], src8[28], src8[29] );
	    dst16[9] = video_RGB_color16(src8[30], src8[31], src8[32] );
      dst16 += 10;
      src8  += 36;
    }while(--uCount);
  }
}

INLINE void GPU_BlitWWWWWWWWS(const void* src, Uint16* dst16, Uint32 isRGB24, Uint32 uClip_src)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 20;
    const Uint16* src16 = ((const Uint16*) src) + uClip_src; 
    do{                         //B                                 //G                          //R
      dst16[ 0] = ((src16[ 0]&(0x1f001f<<10))>>10) | ((src16[ 0]&(0x1f001f<<5))<<1) | ((src16[ 0]&(0x1f001f<<0))<<11);
      dst16[ 1] = ((src16[ 1]&(0x1f001f<<10))>>10) | ((src16[ 1]&(0x1f001f<<5))<<1) | ((src16[ 1]&(0x1f001f<<0))<<11);
      dst16[ 2] = ((src16[ 2]&(0x1f001f<<10))>>10) | ((src16[ 2]&(0x1f001f<<5))<<1) | ((src16[ 2]&(0x1f001f<<0))<<11);
      dst16[ 3] = ((src16[ 3]&(0x1f001f<<10))>>10) | ((src16[ 3]&(0x1f001f<<5))<<1) | ((src16[ 3]&(0x1f001f<<0))<<11);
	    dst16[ 4] = ((src16[ 4]&(0x1f001f<<10))>>10) | ((src16[ 4]&(0x1f001f<<5))<<1) | ((src16[ 4]&(0x1f001f<<0))<<11);
	    dst16[ 5] = ((src16[ 5]&(0x1f001f<<10))>>10) | ((src16[ 5]&(0x1f001f<<5))<<1) | ((src16[ 5]&(0x1f001f<<0))<<11);
	    dst16[ 6] = ((src16[ 6]&(0x1f001f<<10))>>10) | ((src16[ 6]&(0x1f001f<<5))<<1) | ((src16[ 6]&(0x1f001f<<0))<<11);
	    dst16[ 7] = ((src16[ 7]&(0x1f001f<<10))>>10) | ((src16[ 7]&(0x1f001f<<5))<<1) | ((src16[ 7]&(0x1f001f<<0))<<11);

      dst16[ 8] = ((src16[ 9]&(0x1f001f<<10))>>10) | ((src16[ 9]&(0x1f001f<<5))<<1) | ((src16[ 9]&(0x1f001f<<0))<<11);
      dst16[ 9] = ((src16[10]&(0x1f001f<<10))>>10) | ((src16[10]&(0x1f001f<<5))<<1) | ((src16[10]&(0x1f001f<<0))<<11);
      dst16[10] = ((src16[11]&(0x1f001f<<10))>>10) | ((src16[11]&(0x1f001f<<5))<<1) | ((src16[11]&(0x1f001f<<0))<<11);
      dst16[11] = ((src16[12]&(0x1f001f<<10))>>10) | ((src16[12]&(0x1f001f<<5))<<1) | ((src16[12]&(0x1f001f<<0))<<11);
	    dst16[12] = ((src16[13]&(0x1f001f<<10))>>10) | ((src16[13]&(0x1f001f<<5))<<1) | ((src16[13]&(0x1f001f<<0))<<11);
	    dst16[13] = ((src16[14]&(0x1f001f<<10))>>10) | ((src16[14]&(0x1f001f<<5))<<1) | ((src16[14]&(0x1f001f<<0))<<11);
	    dst16[14] = ((src16[15]&(0x1f001f<<10))>>10) | ((src16[15]&(0x1f001f<<5))<<1) | ((src16[15]&(0x1f001f<<0))<<11);
	    dst16[15] = ((src16[16]&(0x1f001f<<10))>>10) | ((src16[16]&(0x1f001f<<5))<<1) | ((src16[16]&(0x1f001f<<0))<<11);
	  	dst16 += 16;
      src16 += 18;
    }while(--uCount);
  }
  else
  {
    uCount = 20;
    const Uint8* src8 = (const Uint8*)src + (uClip_src<<1) + uClip_src;
    do{
      dst16[ 0] = video_RGB_color16(src8[ 0], src8[ 1], src8[ 2] );
      dst16[ 1] = video_RGB_color16(src8[ 3], src8[ 4], src8[ 5] );
      dst16[ 2] = video_RGB_color16(src8[ 6], src8[ 7], src8[ 8] );
      dst16[ 3] = video_RGB_color16(src8[ 9], src8[10], src8[11] );
      dst16[ 4] = video_RGB_color16(src8[12], src8[13], src8[14] );
      dst16[ 5] = video_RGB_color16(src8[15], src8[16], src8[17] );
      dst16[ 6] = video_RGB_color16(src8[18], src8[19], src8[20] );
      dst16[ 7] = video_RGB_color16(src8[21], src8[22], src8[23] );

      dst16[ 8] = video_RGB_color16(src8[27], src8[28], src8[29] );
      dst16[ 9] = video_RGB_color16(src8[30], src8[31], src8[32] );
      dst16[10] = video_RGB_color16(src8[33], src8[34], src8[35] );
      dst16[11] = video_RGB_color16(src8[36], src8[37], src8[38] );
      dst16[12] = video_RGB_color16(src8[39], src8[40], src8[41] );
      dst16[13] = video_RGB_color16(src8[42], src8[43], src8[44] );
      dst16[14] = video_RGB_color16(src8[45], src8[46], src8[47] );
      dst16[15] = video_RGB_color16(src8[48], src8[49], src8[50] );
      dst16 += 16;
      src8  += 54;
    }while(--uCount);
  }
}

INLINE void GPU_BlitWWDWW(const void* src, Uint16* dst16, Uint32 isRGB24)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 32;
    const Uint16* src16 = (const Uint16*) src; 
    do{                            //B                                 //G                          //R
      dst16[ 0] = ((src16[0]&(0x1f001f<<10))>>10) | ((src16[0]&(0x1f001f<<5))<<1) | ((src16[0]&(0x1f001f<<0))<<11);
      dst16[ 1] = ((src16[1]&(0x1f001f<<10))>>10) | ((src16[1]&(0x1f001f<<5))<<1) | ((src16[1]&(0x1f001f<<0))<<11);
      dst16[ 2] = dst16[1];
      dst16[ 3] = ((src16[2]&(0x1f001f<<10))>>10) | ((src16[2]&(0x1f001f<<5))<<1) | ((src16[2]&(0x1f001f<<0))<<11);
      dst16[ 4] = ((src16[3]&(0x1f001f<<10))>>10) | ((src16[3]&(0x1f001f<<5))<<1) | ((src16[3]&(0x1f001f<<0))<<11);
      dst16[ 5] = ((src16[4]&(0x1f001f<<10))>>10) | ((src16[4]&(0x1f001f<<5))<<1) | ((src16[4]&(0x1f001f<<0))<<11);
      dst16[ 6] = ((src16[5]&(0x1f001f<<10))>>10) | ((src16[5]&(0x1f001f<<5))<<1) | ((src16[5]&(0x1f001f<<0))<<11);
      dst16[ 7] = dst16[6];
      dst16[ 8] = ((src16[6]&(0x1f001f<<10))>>10) | ((src16[6]&(0x1f001f<<5))<<1) | ((src16[6]&(0x1f001f<<0))<<11);
      dst16[ 9] = ((src16[7]&(0x1f001f<<10))>>10) | ((src16[7]&(0x1f001f<<5))<<1) | ((src16[7]&(0x1f001f<<0))<<11);
      dst16 += 10;
      src16 +=  8;
    }while(--uCount);
  }
  else
  {
    uCount = 32;
    const Uint8* src8 = (const Uint8*)src;
    do{
      dst16[ 0] = video_RGB_color16(src8[0], src8[ 1], src8[ 2] );
      dst16[ 1] = video_RGB_color16(src8[3], src8[ 4], src8[ 5] );
	    dst16[ 2] = dst16[1];
      dst16[ 3] = video_RGB_color16(src8[6], src8[ 7], src8[ 8] );
      dst16[ 4] = video_RGB_color16(src8[9], src8[10], src8[11] );

      dst16[ 5] = video_RGB_color16(src8[12], src8[13], src8[14] );
      dst16[ 6] = video_RGB_color16(src8[15], src8[16], src8[17] );
	    dst16[ 7] = dst16[6];
      dst16[ 8] = video_RGB_color16(src8[18], src8[19], src8[20] );
      dst16[ 9] = video_RGB_color16(src8[21], src8[22], src8[23] );
      dst16 += 10;
      src8  += 24;
    }while(--uCount);
  }
}


INLINE void GPU_BlitWS(const void* src, Uint16* dst16, Uint32 isRGB24)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 20;
    const Uint16* src16 = (const Uint16*) src; 
    do{                            //B                                 //G                          //R
      dst16[ 0] = ((src16[ 0]&(0x1f001f<<10))>>10) | ((src16[ 0]&(0x1f001f<<5))<<1) | ((src16[ 0]&(0x1f001f<<0))<<11);
      dst16[ 1] = ((src16[ 2]&(0x1f001f<<10))>>10) | ((src16[ 2]&(0x1f001f<<5))<<1) | ((src16[ 2]&(0x1f001f<<0))<<11);
      dst16[ 2] = ((src16[ 4]&(0x1f001f<<10))>>10) | ((src16[ 4]&(0x1f001f<<5))<<1) | ((src16[ 4]&(0x1f001f<<0))<<11);
      dst16[ 3] = ((src16[ 6]&(0x1f001f<<10))>>10) | ((src16[ 6]&(0x1f001f<<5))<<1) | ((src16[ 6]&(0x1f001f<<0))<<11);

      dst16[ 4] = ((src16[ 8]&(0x1f001f<<10))>>10) | ((src16[ 8]&(0x1f001f<<5))<<1) | ((src16[ 8]&(0x1f001f<<0))<<11);
      dst16[ 5] = ((src16[10]&(0x1f001f<<10))>>10) | ((src16[10]&(0x1f001f<<5))<<1) | ((src16[10]&(0x1f001f<<0))<<11);
      dst16[ 6] = ((src16[12]&(0x1f001f<<10))>>10) | ((src16[12]&(0x1f001f<<5))<<1) | ((src16[12]&(0x1f001f<<0))<<11);
      dst16[ 7] = ((src16[14]&(0x1f001f<<10))>>10) | ((src16[14]&(0x1f001f<<5))<<1) | ((src16[14]&(0x1f001f<<0))<<11);

      dst16[ 8] = ((src16[16]&(0x1f001f<<10))>>10) | ((src16[16]&(0x1f001f<<5))<<1) | ((src16[16]&(0x1f001f<<0))<<11);
      dst16[ 9] = ((src16[18]&(0x1f001f<<10))>>10) | ((src16[18]&(0x1f001f<<5))<<1) | ((src16[18]&(0x1f001f<<0))<<11);
      dst16[10] = ((src16[20]&(0x1f001f<<10))>>10) | ((src16[20]&(0x1f001f<<5))<<1) | ((src16[20]&(0x1f001f<<0))<<11);
      dst16[11] = ((src16[22]&(0x1f001f<<10))>>10) | ((src16[22]&(0x1f001f<<5))<<1) | ((src16[22]&(0x1f001f<<0))<<11);

      dst16[12] = ((src16[24]&(0x1f001f<<10))>>10) | ((src16[24]&(0x1f001f<<5))<<1) | ((src16[24]&(0x1f001f<<0))<<11);
      dst16[13] = ((src16[26]&(0x1f001f<<10))>>10) | ((src16[26]&(0x1f001f<<5))<<1) | ((src16[26]&(0x1f001f<<0))<<11);
      dst16[14] = ((src16[28]&(0x1f001f<<10))>>10) | ((src16[28]&(0x1f001f<<5))<<1) | ((src16[28]&(0x1f001f<<0))<<11);
      dst16[15] = ((src16[30]&(0x1f001f<<10))>>10) | ((src16[30]&(0x1f001f<<5))<<1) | ((src16[30]&(0x1f001f<<0))<<11);

	    dst16 += 16;
      src16 += 32;
    }while(--uCount);
  }
  else
  {
    uCount = 20;
    const Uint8* src8 = (const Uint8*) src; 
    do{
      dst16[ 0] = video_RGB_color16(src8[ 0], src8[ 1], src8[ 2] );
      dst16[ 1] = video_RGB_color16(src8[ 6], src8[ 7], src8[ 8] );
      dst16[ 2] = video_RGB_color16(src8[12], src8[13], src8[14] );
      dst16[ 3] = video_RGB_color16(src8[18], src8[19], src8[20] );

      dst16[ 4] = video_RGB_color16(src8[24], src8[25], src8[26] );
      dst16[ 5] = video_RGB_color16(src8[30], src8[31], src8[32] );
      dst16[ 6] = video_RGB_color16(src8[36], src8[37], src8[38] );
      dst16[ 7] = video_RGB_color16(src8[42], src8[43], src8[44] );

      dst16[ 8] = video_RGB_color16(src8[48], src8[49], src8[50] );
      dst16[ 9] = video_RGB_color16(src8[54], src8[55], src8[56] );
      dst16[10] = video_RGB_color16(src8[60], src8[61], src8[62] );
      dst16[11] = video_RGB_color16(src8[66], src8[67], src8[68] );

      dst16[12] = video_RGB_color16(src8[72], src8[73], src8[74] );
      dst16[13] = video_RGB_color16(src8[78], src8[79], src8[80] );
      dst16[14] = video_RGB_color16(src8[84], src8[85], src8[86] );
      dst16[15] = video_RGB_color16(src8[90], src8[91], src8[92] );

      dst16 += 16;
      src8  += 96;
    }while(--uCount);
  }
}


INLINE void GPU_BlitWSSWSSWSSWSSWSSS(const void* src, Uint16* dst16, Uint32 isRGB24)
{
  Uint32 uCount;
  if(isRGB24 == 0)
  {
    uCount = 32;
    const Uint16* src16 = (const Uint16*) src; 
    do{                            //B                                 //G                          //R
      dst16[ 0] = ((src16[ 0]&(0x1f001f<<10))>>10) | ((src16[ 0]&(0x1f001f<<5))<<1) | ((src16[ 0]&(0x1f001f<<0))<<11);
      dst16[ 1] = ((src16[ 3]&(0x1f001f<<10))>>10) | ((src16[ 3]&(0x1f001f<<5))<<1) | ((src16[ 3]&(0x1f001f<<0))<<11);
      dst16[ 2] = ((src16[ 6]&(0x1f001f<<10))>>10) | ((src16[ 6]&(0x1f001f<<5))<<1) | ((src16[ 6]&(0x1f001f<<0))<<11);
      dst16[ 3] = ((src16[ 9]&(0x1f001f<<10))>>10) | ((src16[ 9]&(0x1f001f<<5))<<1) | ((src16[ 9]&(0x1f001f<<0))<<11);
      dst16[ 4] = ((src16[12]&(0x1f001f<<10))>>10) | ((src16[12]&(0x1f001f<<5))<<1) | ((src16[12]&(0x1f001f<<0))<<11);
                                                                                              
      dst16[ 5] = ((src16[16]&(0x1f001f<<10))>>10) | ((src16[16]&(0x1f001f<<5))<<1) | ((src16[16]&(0x1f001f<<0))<<11);
      dst16[ 6] = ((src16[19]&(0x1f001f<<10))>>10) | ((src16[19]&(0x1f001f<<5))<<1) | ((src16[19]&(0x1f001f<<0))<<11);
      dst16[ 7] = ((src16[22]&(0x1f001f<<10))>>10) | ((src16[22]&(0x1f001f<<5))<<1) | ((src16[22]&(0x1f001f<<0))<<11);
      dst16[ 8] = ((src16[25]&(0x1f001f<<10))>>10) | ((src16[25]&(0x1f001f<<5))<<1) | ((src16[25]&(0x1f001f<<0))<<11);
      dst16[ 9] = ((src16[28]&(0x1f001f<<10))>>10) | ((src16[28]&(0x1f001f<<5))<<1) | ((src16[28]&(0x1f001f<<0))<<11);

      dst16 += 10;
      src16 += 32;
    }while(--uCount);
  }
}

#endif //_INNER_BLIT_H_
