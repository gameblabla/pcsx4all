 /***********************************************************************
*
*	Dr.Hell's WinGDI GPU Plugin
*	Version 0.8
*	Copyright (C)Dr.Hell, 2002-2004
*
*	Drawing
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "time.h"
#include "port.h"

#define VIDEO_WIDTH 320
#define VIDEO_PITCH 640

typedef unsigned int Uint32;
typedef signed int Sint32;
typedef unsigned short Uint16;
typedef signed short Sint16;
typedef unsigned char Uint8;
typedef signed char Sint8;

typedef struct {
	Uint32 Version;
	Uint32 GPU_gp1;
	Uint32 Control[256];
	Uint8 FrameBuffer[0x100000];
} GPUFreeze_t;

#define	FRAME_BUFFER_SIZE (1024*512*2)
#define	FRAME_WIDTH	1024
#define	FRAME_HEIGHT 512

#define video_RGB_color16(R,G,B)	(((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3)))
#define	FRAME_OFFSET(x,y)	(((y)<<10)+(x))
#define	GPU_RGB16(rgb) ((((rgb)&0xF80000)>>9)|(((rgb)&0xF800)>>6)|(((rgb)&0xF8)>>3))

// My Frameskip Globals
Uint32 firstTime = 1;
Uint32 autoFrameSkip = 1;
Sint32 framesToSkip = 0;
Sint32	frameRateCounter;
Sint32 frameRate = 60;
Uint32 systicks;
Sint32 Skip = 0;
Sint32	updateLace = 0;

Sint32	isPAL = 0;

Uint32 writeDmaWidth, writeDmaHeight;

Sint32		px,py;
Sint32		x_start,y_start,x_end,y_end;
Uint16	*pvram;

Sint32 GPU_gp0;
Sint32 GPU_gp1;
Sint32 FrameToRead;
Sint32 FrameToWrite;
Sint32 FrameWidth;
Sint32 FrameCount;
Sint32 FrameIndex;
union {
	Sint8 S1[64];
	Sint16 S2[32];
	Sint32 S4[16];
	Uint8 U1[64];
	Uint16 U2[32];
	Uint32 U4[16];
} PacketBuffer;
Sint32 PacketCount;
Sint32 PacketIndex;
Sint32 TextureWindow[4];
Sint32 DrawingArea[4];
Sint32 DrawingOffset[2];
Sint32 DisplayArea[8];
Uint32 Masking;
Uint32 PixelMSB;
Sint32 OtherEnv[16];
Sint32 DrawingCount[4];
Sint32 DisplayCount[4];
Uint16*  FrameBuffer;

/*----------------------------------------------------------------------
Table
----------------------------------------------------------------------*/

Uint8 PacketSize[256] = {
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		0-15
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		16-31
	3, 3, 3, 3, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 8, 8,	//		32-47
	5, 5, 5, 5, 8, 8, 8, 8, 7, 7, 7, 7, 11, 11, 11, 11,	//	48-63
	2, 2, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,	//		64-79
	3, 3, 3, 3, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4,	//		80-95
	2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2,	//		96-111
	1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,	//		112-127
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		128-
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		144
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	//
};

Uint16 HorizontalResolution[8] = {
	256, 368, 320, 384, 512, 512, 640, 640
};

Uint16 VerticalResolution[4] = {
	240, 480, 256, 480
};

/*----------------------------------------------------------------------
Device Depended
----------------------------------------------------------------------*/

#define	GetMem(p1)	malloc((p1))
#define	FreeMem(p1)	free((p1))
#define CopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define ZeroMemory(Destination,Length) memset((Destination),0,(Length))
#define	CopyMem(p1,p2,p3)	CopyMemory((p1),(p2),(p3))
#define	ZeroMem(p1,p2)	ZeroMemory((p1),(p2))

INLINE Uint32 gpuGetTime(void)
{
	return clock()/1000;
}

INLINE void gpuOpenVideo(long p1)
{
	FrameBuffer = (Uint16*)GetMem(FRAME_BUFFER_SIZE);
	systicks = gpuGetTime();
}

INLINE void gpuCloseVideo(void)
{
	FreeMem(FrameBuffer);
}

template<typename T>
INLINE T Min2 (const T _a, const T _b) { return (_a<_b)?_a:_b; }

#include "gpu_blit.h"

INLINE void gpuVideoOutput(void)
{
	static Sint16 old_res_horz, old_res_vert, old_rgb24;
	Sint16 h0, x0, y0, w0, h1;

	x0 = DisplayArea[0];
	y0 = DisplayArea[1];
	
	w0 = DisplayArea[2];
	h0 = DisplayArea[3];

	h1 = DisplayArea[7] - DisplayArea[5];
	if (h0 == 480)
		h1 = Min2(h1*2,480);

	Uint16* dest_screen16 = SCREEN;
	Uint16* src_screen16  = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0,y0)];
	Uint32 isRGB24 = (GPU_gp1 & 0x00200000 ? 32 : 0);

	/* Clear the screen if resolution changed to prevent interlacing and clipping to clash */
	if( (w0 != old_res_horz || h1 != old_res_vert || (Sint16)isRGB24 != old_rgb24) )
	{
		// Update old resolution
		old_res_horz = w0;
		old_res_vert = h1;
		old_rgb24 = (Sint16)isRGB24;
		// Finally, clear the screen for this special case
		video_clear();
	}

	//  Height centering
	int sizeShift = 1;
	if(h0==256) h0 = 240;
	else if(h0==480) sizeShift = 2;

	if(h1>h0)
	{
		src_screen16  += ((h1-h0)>>sizeShift)*FRAME_WIDTH;
		h1 = h0;
	}
	else if(h1<h0) dest_screen16 += ((h0-h1)>>sizeShift)*VIDEO_WIDTH;

	int incY = (h0==480) ? 2 : 1;
	h0=(h0==480 ? (FRAME_WIDTH*2) : FRAME_WIDTH);
	
	for(int y1=y0+h1; y0<y1; y0+=incY)
	{
		/* Blit span */
		switch( w0 )
		{
		case 256:
				GPU_BlitWWDWW(	src_screen16,	dest_screen16, isRGB24);
			break;
		case 368:
				GPU_BlitWWWWWWWWS(	src_screen16,	dest_screen16, isRGB24, 4);
			break;
		case 320:
				GPU_BlitWW(	src_screen16,	dest_screen16, isRGB24);
			break;
		case 384:
				GPU_BlitWWWWWS(	src_screen16,	dest_screen16, isRGB24);
			break;
		case 512:
				GPU_BlitWWSWWSWS(	src_screen16, dest_screen16, isRGB24);
			break;
		case 640:
				GPU_BlitWS(	src_screen16, dest_screen16, isRGB24);
			break;
		}

		dest_screen16 += VIDEO_WIDTH;
		src_screen16  += h0;
	}
	
	video_flip();
}

/*----------------------------------------------------------------------
Drawing
----------------------------------------------------------------------*/

#include "gpu_draw.h"

/*----------------------------------------------------------------------
Other
----------------------------------------------------------------------*/

/* gpuReset */
INLINE void gpuReset(void)
{
	GPU_gp1 = 0x14802000;
	TextureWindow[2] = 255;
	TextureWindow[3] = 255;
	DrawingArea[2] = 256;
	DrawingArea[3] = 240;
	DisplayArea[2] = 256;
	DisplayArea[3] = 240;
	DisplayArea[6] = 256;
	DisplayArea[7] = 240;
}

/*----------------------------------------------------------------------
Plugin Functions
----------------------------------------------------------------------*/

/* GPUinit */
Sint32  GPU_init(void)
{
	gpuOpenVideo(0);
	gpuReset();
	return (0);
}

/* GPUshutdown */
Sint32  GPU_shutdown(void)
{
	gpuCloseVideo();
	return (0);
}

/* GPUwriteData */
void GPU_writeData(Uint32 data)
{
	GPU_gp1 &= ~0x14000000;

	if (FrameToWrite > 0) {
          pvram[px]=(Uint16)data;
          if (++px>=x_end) {
               px = x_start;
               pvram += FRAME_WIDTH;
               if (++py>=y_end) FrameToWrite=0;
          }
          if (FrameToWrite > 0) {
               pvram[px]=data>>16;
               if (++px>=x_end) {
                    px = x_start;
                    pvram += FRAME_WIDTH;
                    if (++py>=y_end) FrameToWrite=0;
               }
          }
	} else {
		if (PacketCount) {
			PacketCount--;
			PacketBuffer.U4[PacketIndex++] = data;
		} else {
			PacketBuffer.U4[0] = data;
			PacketCount = PacketSize[data >> 24];
			PacketIndex = 1;
		}
		if (!PacketCount)
		{
			gpuSendPacket();
		}
	}
	GPU_gp1 |= 0x14000000;
}

/* GPUwriteStatus */
void GPU_writeStatus(Uint32 data)
{
	switch (data >> 24) {
		case 0x00:
			gpuReset();
			break;
		case 0x01:
			GPU_gp1 &= ~0x08000000;
			PacketCount = FrameToRead = FrameToWrite = 0;
			break;
		case 0x02:
			GPU_gp1 &= ~0x08000000;
			PacketCount = FrameToRead = FrameToWrite = 0;
			break;
		case 0x03:
			GPU_gp1 = (GPU_gp1 & ~0x00800000) | ((data & 1) << 23);
			break;
		case 0x04:
			if (data == 0x04000000)
				PacketCount = 0;
			GPU_gp1 = (GPU_gp1 & ~0x60000000) | ((data & 3) << 29);
			break;
		case 0x05:
			DisplayArea[0] = data & 0x000003FF; //(short)(data & 0x3ff);
			DisplayArea[1] = (data & 0x000FFC00) >> 10; //(short)((data>>10)&0x1ff);
			break;
		case 0x06:
			DisplayArea[4] = data & 0x00000FFF; //(short)(data & 0x7ff);
			DisplayArea[6] = (data & 0x00FFF000) >> 12; //(short)((data>>12) & 0xfff);
			break;
		case 0x07:
			{		
				//int iT;
				DisplayArea[5] = data & 0x000003FF; //(short)(data & 0x3ff);
				DisplayArea[7] = (data & 0x000FFC00) >> 10; //(short)((data>>10) & 0x3ff);
			}
			break;
		case 0x08:
			OtherEnv[0x08] = (data >> 7) & 1;	//	reverse(?)
			GPU_gp1 =
				(GPU_gp1 & ~0x007F0000) | ((data & 0x3F) << 17) |
				((data & 0x40) << 10);

			{
				DisplayArea[2] = HorizontalResolution[(GPU_gp1 >> 16) & 7];
				DisplayArea[3] = VerticalResolution[(GPU_gp1 >> 19) & 3];
			}

			isPAL = (data & 0x08) ? 1 : 0; // if 1 - PAL mode, else NTSC
			break;
		case 0x09:
			OtherEnv[0x09] = data & 1;			//	gpub(?)
			break;
		case 0x10:
			switch (data & 0xffff) {
				case 0:
				case 1:
				case 3:
					GPU_gp0 = (DrawingArea[1] << 10) | DrawingArea[0];
					break;
				case 4:
					GPU_gp0 =
						((DrawingArea[3] - 1) << 10) | (DrawingArea[2] -
														1);
					break;
				case 6:
				case 5:
					GPU_gp0 = (DrawingOffset[1] << 11) | DrawingOffset[0];
					break;
				case 7:
					GPU_gp0 = 2;
					break;
				default:
					GPU_gp0 = 0;
			}
			break;
	}
}

/* GPUreadData */
Uint32 GPU_readData(void)
{
	GPU_gp1 &= ~0x14000000;
	if (FrameToRead)
	{
		GPU_gp0 = pvram[px];
		if (++px>=x_end) {
		   px = x_start;
		   pvram += FRAME_WIDTH;
		   if (++py>=y_end) FrameToRead=0;
		}
		GPU_gp0 |= pvram[px]<<16;
		if (++px>=x_end) {
		   px = x_start;
		   pvram +=FRAME_WIDTH;
		   if (++py>=y_end) FrameToRead=0;
		}

		if( FrameToRead == 0 ) GPU_gp1 &= ~0x08000000;
	}
	GPU_gp1 |= 0x14000000;
	return (GPU_gp0);
}

/* GPUreadStatus */
Uint32 GPU_readStatus(void)
{
	Uint32 ret=(GPU_gp1 | 0x1c000000) & ~0x00480000;
	return ret;
}

Uint32 *lUsedAddr[3];
INLINE int CheckForEndlessLoop(Uint32 *laddr)
{
 if(laddr==lUsedAddr[1]) return 1;
 if(laddr==lUsedAddr[2]) return 1;

 if(laddr<lUsedAddr[0]) lUsedAddr[1]=laddr;
 else                   lUsedAddr[2]=laddr;
 lUsedAddr[0]=laddr;
 return 0;
}

/* GPUdmaChain */
Sint32  GPU_dmaChain(Uint32 * baseAddr, Uint32 dmaVAddr)
{
	Uint32 data, *address, count, offset;
    unsigned int DMACommandCounter = 0;
	//Uint32 temp;
	GPU_gp1 &= ~0x14000000;

    lUsedAddr[0]=lUsedAddr[1]=lUsedAddr[2]=(Uint32*)0xffffff;
	dmaVAddr &= 0x00FFFFFF;

	while (dmaVAddr != 0xFFFFFF) {
		address = (baseAddr + (dmaVAddr >> 2));

        if(DMACommandCounter++ > 2000000) break;
		if(CheckForEndlessLoop(address)) break;
		
		data = *address++;
		count = (data >> 24);
		offset = data & 0x00FFFFFF;
		if (dmaVAddr != offset)
			dmaVAddr = offset;
		else
			dmaVAddr = 0xFFFFFF;
		while (count) {
			data = *address++;
			count--;
			//temp = PacketCount;
			if (PacketCount) {
				PacketCount--;
				PacketBuffer.U4[PacketIndex++] = data;
			} else {
				PacketBuffer.U4[0] = data;
				PacketCount = PacketSize[data >> 24];
				PacketIndex = 1;
			}
			//PacketCount = temp;
			if (!PacketCount)
			{
				gpuSendPacket();
			}
		}
	}
	
	GPU_gp1 |= 0x14000000;
	return (0);
}

#define MAXSKIP		6

static Sint32 lastframerate = 0;
static Sint32 frameskipChange = 2;	// must be an even number so both frames in the buffer are cleared

static void FrameSkip(void)
{
	static Sint32 palhz = 50;
	static Sint32 ntschz = 60;

	static Sint32 pollcount = 0;
	Sint32 hz = (isPAL ? palhz : ntschz);
	Sint32 previousframeskip = framesToSkip;

	if( firstTime )
	{
		palhz = 50;
		ntschz = 60;
		framesToSkip = 0;
		frameskipChange = 2;
		lastframerate = 0;
		firstTime = 0;
		return;
	}

	if( frameRate * framesToSkip < hz )			// are we below 50/60 FPS?
	{
		pollcount++;
		if( pollcount > MAXSKIP )				// if we tried fixing the frameskip over MAXSKIP times
		{
			pollcount = 0;
			framesToSkip = 2;					// reset/reduce the frameskip
			if( isPAL ) palhz -= 5;				// reduce target framerate by half
			else ntschz -= 5;
			
			if( palhz < 10 ) palhz = 10;
			if( ntschz < 10 ) ntschz = 10;
		}
		else if( lastframerate > frameRate * framesToSkip )			// check if the previous change was effective
		{										// it wasn't effective
			frameskipChange = -frameskipChange; // reverse the direction of frameskipping
			framesToSkip += frameskipChange;	// add the framerate change
		}
		else
		{
			framesToSkip += frameskipChange;	// everything going good? keep going
			pollcount = 0;						// and reset pollcount
		}
	}
	else										// were above or equal to the correct framerate
	{
		if( palhz < 50 || ntschz < 60 )			// did we reduce the target rate before?
		{
			if( isPAL ) palhz += 5;				// then up the target framerate
			else		ntschz += 5;

			if( palhz > 50 ) palhz = 50;
			if( ntschz > 60 ) ntschz = 60;
		}
		pollcount = 0;								// we made our target framerate, so resset pollcount
	}

	if( framesToSkip < 0 ) framesToSkip = 0;
	if( framesToSkip > MAXSKIP ) framesToSkip = MAXSKIP;

	lastframerate = frameRate * previousframeskip;
}

/* GPUupdateLace */
void  GPU_updateLace(void)
{	
	Uint32 newticks;
	Uint32 diffticks = 0;	
	
	/*	NOTE: GPU_gp1 must have the interlace bit toggle here, 
		since it shouldn't be in readStatus as it was previously */
	GPU_gp1 ^= 0x80000000;

    if(Skip)
    {
      --Skip;
      if(!Skip)
        updateLace = 0;
      return;
    }

	Skip = framesToSkip;

	frameRateCounter++;
	newticks=gpuGetTime();

	if( (diffticks = (newticks-systicks)) >= 1000 ) // poll every second
	{
		systicks = newticks;
		frameRate = (Sint32)(((double)frameRateCounter) / (((double)diffticks)/1000.0));
		frameRateCounter = 0;

		// when the framerate is updated, update the autoframeskip as well
		if( autoFrameSkip )
		{
			FrameSkip(); // auto frameskip for FramesToSkip setting	
		}
	}

	if (updateLace)
	{
		gpuVideoOutput();
		updateLace=0;
	}

}

/* GPUwriteDataMem */
void GPU_writeDMA( unsigned short* src, unsigned short* dst, unsigned long src_advance, unsigned long w0, unsigned long h1, unsigned long dmaCount );
void GPU_writeDMA_Fast( unsigned char* src, unsigned char* dst, unsigned long src_advance, unsigned long w0, unsigned long h1, unsigned long dmaCount );

void GPU_writeDataMem(Uint32 * dmaAddress, Sint32 dmaCount)
{
	Uint32 temp;

	GPU_gp1 &= ~0x14000000;

	while (dmaCount) {
		if (FrameToWrite > 0) {
			{
				while (dmaCount--) 
				{
					Uint32 data = *dmaAddress++;

					if (px<FRAME_WIDTH && py<512)
						pvram[px] = data;
					if (++px>=x_end) 
					{
						px = x_start;
						pvram += FRAME_WIDTH;
						if (++py>=y_end) 
						{
							FrameToWrite = 0;
							GPU_gp1 &= ~0x08000000;
							break;
						}
					}
					if (px<FRAME_WIDTH && py<512)
						pvram[px] = data>>16;
					if (++px>=x_end) 
					{
						px = x_start;
						pvram += FRAME_WIDTH;
						if (++py>=y_end) 
						{
							FrameToWrite = 0;
							GPU_gp1 &= ~0x08000000;
							break;
						}
					}
				}
			}
		} else {
			temp = *dmaAddress++;
			dmaCount--;
			if (PacketCount) {
				PacketBuffer.U4[PacketIndex++] = temp;
				PacketCount--;
			} else {
				PacketBuffer.U4[0] = temp;
				PacketCount = PacketSize[temp >> 24];
				PacketIndex = 1;
			}
			if (!PacketCount)
			{
				gpuSendPacket();
			}
		}
	}
	GPU_gp1 |= 0x14000000;
}

/* GPUreadDataMem */
void  GPU_readDataMem(Uint32 * dmaAddress, Sint32 dmaCount)
{
	if( FrameToRead == 0 ) return;

	GPU_gp1 &= ~0x14000000;

	do 
	{
		// lower 16 bit
		Uint32 data = (unsigned long)pvram[px];

		if (++px>=x_end) 
		{
			px = x_start;
			pvram += FRAME_WIDTH;
		}

		// higher 16 bit (always, even if it's an odd width)
		data |= (unsigned long)(pvram[px])<<16;
    
		*dmaAddress++ = data;

		if (++px>=x_end) 
		{
			px = x_start;
			pvram += FRAME_WIDTH;
			if (++py>=y_end) 
			{
				FrameToRead = 0;
				GPU_gp1 &= ~0x08000000;
				break;
			}
		}
	} while (--dmaCount);

	GPU_gp1 |= 0x14000000;
}

/* GPUfreeze */
long  GPU_freeze(Uint32 p1, GPUFreeze_t * p2)
{
	Uint32 temp;
	Sint32 ret=0;

	if (p1 == 2) {
		temp = *(Uint32 *) p2;
		if ((temp < 0) || (temp > 8))
			ret=0;
		else
			ret=1;
	}
	else
	if (p2 == NULL)
		ret=0;
	else
	if (p2->Version != 1)
		ret=0;
	else
	if (p1 == 1) {
		p2->GPU_gp1 = GPU_gp1;
		CopyMem(p2->FrameBuffer, (Uint16*)FrameBuffer, FRAME_BUFFER_SIZE);
		ret=1;
	}
	else
	if (p1 == 0) {
		GPU_gp1 = p2->GPU_gp1;
		CopyMem((Uint16*)FrameBuffer, p2->FrameBuffer, FRAME_BUFFER_SIZE);
		ret=1;
	}
	return ret;
}
