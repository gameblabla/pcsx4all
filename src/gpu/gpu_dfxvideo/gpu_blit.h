/***************************************************************************
                          draw.c  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

// misc globals
long           lLowerpart;
BOOL           bCheckMask = FALSE;
unsigned short sSetMask = 0;
unsigned long  lSetMask = 0;

unsigned char *pVideoBuffer=NULL;

/*
static void BlitScreen32(unsigned char *surf, int32_t x, int32_t y)
{
 unsigned char *pD;
 unsigned int startxy;
 uint32_t lu;
 unsigned short s;
 unsigned short row, column;
 unsigned short dx = PreviousPSXDisplay.Range.x1;
 unsigned short dy = PreviousPSXDisplay.DisplayMode.y;

 int32_t lPitch = PSXDisplay.DisplayMode.x << 2;

 uint32_t *destpix;

 if (PreviousPSXDisplay.Range.y0) // centering needed?
  {
   memset(surf, 0, (PreviousPSXDisplay.Range.y0 >> 1) * lPitch);

   dy -= PreviousPSXDisplay.Range.y0;
   surf += (PreviousPSXDisplay.Range.y0 >> 1) * lPitch;

   memset(surf + dy * lPitch,
          0, ((PreviousPSXDisplay.Range.y0 + 1) >> 1) * lPitch);
  }

 if (PreviousPSXDisplay.Range.x0)
  {
   for (column = 0; column < dy; column++)
    {
     destpix = (uint32_t *)(surf + (column * lPitch));
     memset(destpix, 0, PreviousPSXDisplay.Range.x0 << 2);
    }
   surf += PreviousPSXDisplay.Range.x0 << 2;
  }

 if (PSXDisplay.RGB24)
  {
   for (column = 0; column < dy; column++)
    {
     startxy = ((1024) * (column + y)) + x;
     pD = (unsigned char *)&psxVuw[startxy];
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       lu = *((uint32_t *)pD);
       destpix[row] = 
          0xff000000 | (RED(lu) << 16) | (GREEN(lu) << 8) | (BLUE(lu));
       pD += 3;
      }
    }
  }
 else
  {
   for (column = 0;column<dy;column++)
    {
     startxy = (1024 * (column + y)) + x;
     destpix = (uint32_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       s = GETLE16(&psxVuw[startxy++]);
       destpix[row] = 
          (((s << 19) & 0xf80000) | ((s << 6) & 0xf800) | ((s >> 7) & 0xf8)) | 0xff000000;
      }
    }
  }
}
*/

static void BlitScreen16(unsigned char *surf, int32_t x, int32_t y)
{
 unsigned char *pD;
 unsigned int startxy;
 uint32_t lu;
 unsigned short s;
 unsigned short row, column;
 unsigned short dx = PreviousPSXDisplay.Range.x1;
 unsigned short dy = PreviousPSXDisplay.DisplayMode.y;

 int32_t lPitch = PSXDisplay.DisplayMode.x << 1;

 uint16_t *destpix;

 if (PreviousPSXDisplay.Range.y0) // centering needed?
  {
   memset(surf, 0, (PreviousPSXDisplay.Range.y0 >> 1) * lPitch);

   dy -= PreviousPSXDisplay.Range.y0;
   surf += (PreviousPSXDisplay.Range.y0 >> 1) * lPitch;

   memset(surf + dy * lPitch,
          0, ((PreviousPSXDisplay.Range.y0 + 1) >> 1) * lPitch);
  }

 if (PreviousPSXDisplay.Range.x0)
  {
   for (column = 0; column < dy; column++)
    {
     destpix = (uint16_t *)(surf + (column * lPitch));
     memset(destpix, 0, PreviousPSXDisplay.Range.x0 << 1);
    }
   surf += PreviousPSXDisplay.Range.x0 << 1;
  }

 if (PSXDisplay.RGB24)
  {
   for (column = 0; column < dy; column++)
    {
     startxy = ((1024) * (column + y)) + x;
     pD = (unsigned char *)&psxVuw[startxy];
     destpix = (uint16_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       lu = *((uint32_t *)pD);
       destpix[row] = 
          ((RED(lu)<<8)&0xf800)|((GREEN(lu)<<3)&0x7e0)|(BLUE(lu)>>3);
       pD += 3;
      }
    }
  }
 else
  {
   for (column = 0;column<dy;column++)
    {
     startxy = (1024 * (column + y)) + x;
     destpix = (uint16_t *)(surf + (column * lPitch));
     for (row = 0; row < dx; row++)
      {
       s = GETLE16(&psxVuw[startxy++]);
       destpix[row] = 
          ((s<<11)&0xf800f800)|((s<<1)&0x7c007c0)|((s>>10)&0x1f001f);
      }
    }
  }
}

static void DoBufferSwap(void)
{
	if ((PSXDisplay.DisplayMode.x == 0) || (PSXDisplay.DisplayMode.y == 0)) return;
	BlitScreen16(pVideoBuffer, PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y);
	video_set((unsigned short*)pVideoBuffer,PSXDisplay.DisplayMode.x,PSXDisplay.DisplayMode.y);
}

static void DoClearScreenBuffer(void)
{
	video_clear();
}

static void ulInitDisplay(void)
{
	/*
	Allocate max that could be needed:
	Big(est?) PSX res: 640x512
	16bpp (times 2)
	*/
	pVideoBuffer=(unsigned char *)malloc(640*512*2);    // alloc video buffer
	memset(pVideoBuffer,0,640*512*2);                   // init video buffer

	bUsingTWin=FALSE;
	return;
}

static void CloseDisplay(void)
{
	free(pVideoBuffer);                                 // free video buffer
	pVideoBuffer = NULL;
}
