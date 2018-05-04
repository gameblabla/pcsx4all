/***************************************************************************
 * (C) PCSX4ALL team 2017                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02111-1307 USA.           *
 ***************************************************************************/

/*
 * Screenshot functionality for plugin_lib
 */

#include <stdint.h>
#include <string.h>

#include "plugin_lib.h"

#define RGB16(C)     ((((C)&(0x1f<<10))>>10) | (((C)&(0x1f<<5))<<1) | (((C)&(0x1f))<<11))
#define RGB24(R,G,B) ((((R)&0xF8)<<8)|(((G)&0xFC)<<3)|(((B)&0xF8)>>3))

// For embedded savestate screenshots
// (Simple pixel-dropping downscaling.. TODO: add interpolation)
void pl_screenshot_160x120_rgb565(uint16_t *dst)
{
  memset((void*)dst, 0, 160*120*2);
  int x = pl_data.sinfo.x;
  int y = pl_data.sinfo.y;
  int w = pl_data.sinfo.w;
  int h = pl_data.sinfo.h;
  int hres = pl_data.sinfo.hres;
  int vres = pl_data.sinfo.vres;
  bool depth24 = pl_data.sinfo.depth24;
  uint16_t *src = (uint16_t*)pl_data.sinfo.vram;
  const int vram_h = 512;

  if (w == 0 || h == 0)
    return;

  if (hres > 640) hres = 640;
  if (vres > 480) vres = 480;
  if (w > hres) w = hres;
  if (h > vres) h = vres;

  if (y + h > vram_h)
  {
    if (y + h - vram_h > h / 2)
    {
      // wrap
      h -= vram_h - y;
      y = 0;
    }
    else
    {
      // clip
      h = vram_h - y;
    }
  }

  // Only read even lines in case gpu plugin skipped rendering odd ones
  if (vres == 480)
    y &= ~1;

  src += y * 1024 + x;

  // Skip 1/2 of src lines for 240 vres, 3/4 of src lines for 480 vres
  const int src_stride = (vres == 480) ? 1024 * 4 : 1024 * 2;
  const int dst_stride = 160;

  int scale_numer, scale_denom;
  switch (hres)
  {
    case 256:
      scale_numer = 4;
      scale_denom = 5;
      break;
    case 320:
      scale_numer = 1;
      scale_denom = 1;
      break;
    case 368:
      scale_numer = 8;
      scale_denom = 7;
      break;
    case 384:
      scale_numer = 6;
      scale_denom = 5;
      break;
    case 512:
      scale_numer = 8;
      scale_denom = 5;
      break;
    case 640:
      scale_numer = 2;
      scale_denom = 1;
      break;
    default:
      printf("Warning: unrecognized hres %d in %s()\n", hres, __func__);
      return;
  }

  int start_i = 0, end_i = 160;
  int start_j = 0, end_j = 120;

  // Width centering
  if (w < hres)
  {
    int tmp = ((hres - w) / 2) * scale_denom / scale_numer;
    start_i += tmp / 2;
    end_i -= tmp / 2;
  }

  // Height centering
  if (h < vres)
  {
    int tmp = ((vres - h) / 2) / ((vres == 480) ? 2 : 1);
    start_j += tmp / 2;
    end_j -= tmp / 2;
  }

  dst += start_j * dst_stride + start_i;

  for (int j=start_j; j < end_j; ++j)
  {
    if (depth24)
    {
      for (int i=start_i; i < end_i; ++i)
      {
        int src_off = i * 2 * scale_numer / scale_denom;
        uint8_t *srcpix8 = (uint8_t*)src + src_off * 3;
        dst[i] = RGB24(srcpix8[0], srcpix8[1], srcpix8[2]);
      }
    }
    else
    {
      for (int i=start_i; i < end_i; ++i)
      {
        int src_off = i * 2 * scale_numer / scale_denom;
        dst[i] = RGB16(src[src_off]);
      }
    }
    dst += dst_stride;
    src += src_stride;
  }
}
