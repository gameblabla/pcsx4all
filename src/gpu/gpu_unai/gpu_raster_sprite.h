/***************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Unai                                               *
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

///////////////////////////////////////////////////////////////////////////////
//  GPU internal sprite drawing functions

void gpuDrawS(PtrUnion packet, const PS gpuSpriteSpanDriver)
{
  s32 x0, x1, y0, y1;
  u32 u0, v0;

  //NOTE: Must 11-bit sign-extend the whole sum here, not just packet X/Y,
  // or sprites in 1st level of SkullMonkeys disappear when walking right.
  // This now matches behavior of Mednafen and PCSX Rearmed's gpu_neon:
  x0 = GPU_EXPANDSIGN(packet.S2[2] + gpu_unai.DrawingOffset[0]);
  y0 = GPU_EXPANDSIGN(packet.S2[3] + gpu_unai.DrawingOffset[1]);

  u32 w = packet.U2[6] & 0x3ff; // Max width is 1023
  u32 h = packet.U2[7] & 0x1ff; // Max height is 511
  x1 = x0 + w;
  y1 = y0 + h;

  s32 xmin, xmax, ymin, ymax;
  xmin = gpu_unai.DrawingArea[0];
  xmax = gpu_unai.DrawingArea[2];
  ymin = gpu_unai.DrawingArea[1];
  ymax = gpu_unai.DrawingArea[3];

  u0 = packet.U1[8];
  v0 = packet.U1[9];

  s32 temp;
  temp = ymin - y0;
  if (temp > 0)
  {
    y0 = ymin;
    v0 += temp;
  }
  if (y1 > ymax) y1 = ymax;
  if (y1 <= y0) return;

  temp = xmin - x0;
  if (temp > 0)
  {
    x0 = xmin;
    u0 += temp;
  }
  if (x1 > xmax) x1 = xmax;
  x1 -= x0;
  if (x1 <= 0) return;

  gpu_unai.r5 = packet.U1[0] >> 3;
  gpu_unai.g5 = packet.U1[1] >> 3;
  gpu_unai.b5 = packet.U1[2] >> 3;

  u16 *Pixel = &((u16*)gpu_unai.vram)[FRAME_OFFSET(x0, y0)];
  const int li=gpu_unai.ilace_mask;
  const int pi=(ProgressiveInterlaceEnabled()?(gpu_unai.ilace_mask+1):0);
  const int pif=(ProgressiveInterlaceEnabled()?(gpu_unai.prog_ilace_flag?(gpu_unai.ilace_mask+1):0):1);
  unsigned int tmode = gpu_unai.TEXT_MODE >> 5;
  const u32 v0_mask = gpu_unai.TextureWindow[3];
  u8* pTxt_base = (u8*)gpu_unai.TBA;

  // Texture is accessed byte-wise, so adjust idx if 16bpp
  if (tmode == 3) u0 <<= 1;

  for (; y0<y1; ++y0)
  {
    u8* pTxt = pTxt_base + ((v0 & v0_mask) * 2048);
    if (!(y0&li) && (y0&pi)!=pif)
      gpuSpriteSpanDriver(Pixel, x1, pTxt, u0);
    Pixel += FRAME_WIDTH;
    v0++;
  }
}

#if defined(__arm__) || defined(RS97)
#if defined(__arm__)
  #include "gpu_arm.h"
#endif

/* Notaz 4bit sprites optimization */
void gpuDrawS16(PtrUnion packet)
{
  s32 x0, y0;
  s32 u0, v0;
  s32 xmin, xmax;
  s32 ymin, ymax;
  u32 h = 16;

  //NOTE: Must 11-bit sign-extend the whole sum here, not just packet X/Y,
  // or sprites in 1st level of SkullMonkeys disappear when walking right.
  // This now matches behavior of Mednafen and PCSX Rearmed's gpu_neon:
  x0 = GPU_EXPANDSIGN(packet.S2[2] + gpu_unai.DrawingOffset[0]);
  y0 = GPU_EXPANDSIGN(packet.S2[3] + gpu_unai.DrawingOffset[1]);

  xmin = gpu_unai.DrawingArea[0];
  xmax = gpu_unai.DrawingArea[2];
  ymin = gpu_unai.DrawingArea[1];
  ymax = gpu_unai.DrawingArea[3];
  u0 = packet.U1[8];
  v0 = packet.U1[9];

  if (x0 > xmax - 16 || x0 < xmin ||
      ((u0 | v0) & 15) || !(gpu_unai.TextureWindow[2] & gpu_unai.TextureWindow[3] & 8))
  {
    // send corner cases to general handler
    packet.U4[3] = 0x00100010;
    gpuDrawS(packet, gpuSpriteSpanFn<0x20>);
    return;
  }

  if (y0 >= ymax || y0 <= ymin - 16)
    return;
  if (y0 < ymin)
  {
    h -= ymin - y0;
    v0 += ymin - y0;
    y0 = ymin;
  }
  else if (ymax - y0 < 16)
    h = ymax - y0;

#if defined(RS97)
  __asm__ (
    ".set noreorder											\n"
    ".macro do_4_pixels rs ibase obase 	\n"
    ".if \\ibase - 1 < 0								\n"
    "   sll $t6, \\rs, 1								\n"
    "	  and $t2, $t7, $t6								\n"
    ".else															\n"
    "  srl $t6, \\rs, \\ibase-1					\n"
    "  and $t2, $t7, $t6								\n"
    ".endif															\n"

    "  srl $t6, \\rs, \\ibase+3					\n"
    "  and $t3, $t7, $t6								\n"
    "  srl $t6, \\rs, \\ibase+7					\n"
    "  and $t4, $t7, $t6								\n"
    "  srl $t6, \\rs, \\ibase+11				\n"
    "  and $t5, $t7, $t6								\n"

    "  add $t6, $a2, $t2								\n"
    "  lh $t2, 0($t6)										\n"
    "  add $t6, $a2, $t3								\n"
    "  lh $t3, 0($t6)										\n"
    "  add $t6, $a2, $t4								\n"
    "  lh $t4, 0($t6)										\n"
    "  add $t6, $a2, $t5								\n"
    "  lh $t5, 0($t6)										\n"

    "  beqz $t2, 1f											\n"
    "  nop															\n"
    "  sh $t2, \\obase+0($a0)						\n"
    "1:																	\n"
    "  beqz $t3, 2f											\n"
    "  nop															\n"
    "  sh $t3, \\obase+2($a0)						\n"
    "2:																	\n"
    "  beqz $t4, 3f											\n"
    "  nop															\n"
    "  sh $t4, \\obase+4($a0)						\n"
    "3:																	\n"
    "  beqz $t5, 4f											\n"
    "  nop															\n"
    "  sh $t5, \\obase+6($a0)						\n"
    "4:																	\n"
    ".endm															\n"

    "  move $a0, %[vram_off]						\n"
    "  move $a1, %[tba_off]							\n"
    "  move $a2, %[cba]									\n"
    "  move $a3, %[h]										\n"
    "  li $t7, 0x1e											\n"
    "0:																	\n"
    "  lw $t0, 0($a1)										\n"
    "  lw $t1, 4($a1)										\n"
    "  do_4_pixels $t0, 0,  0						\n"
    "  do_4_pixels $t0, 16, 8						\n"
    "  do_4_pixels $t1, 0,  16					\n"
    "  do_4_pixels $t1, 16, 24					\n"
    "  li $t6, 1												\n"
    "  subu $a3, $a3, $t6								\n"
    "  addiu $a0, $a0, 2048							\n"
    "  addiu $a1, $a1, 2048							\n"
    "  bnez $a3, 0b											\n"
    "  nop															\n"
    :
    :
    [vram_off] 	"r"	(&gpu_unai.vram[FRAME_OFFSET(x0, y0)]),
    [tba_off]		"r"	(&gpu_unai.TBA[FRAME_OFFSET(u0/4, v0)]),
    [cba]				"r" (gpu_unai.CBA),
    [h]					"r"	(h)
    :
    "$a0", "$a1", "$a2", "$a3"
  );
#else
  draw_spr16_full(&gpu_unai.vram[FRAME_OFFSET(x0, y0)], &gpu_unai.TBA[FRAME_OFFSET(u0/4, v0)], gpu_unai.CBA, h);
#endif
}
#endif // __arm__

void gpuDrawT(PtrUnion packet, const PT gpuTileSpanDriver)
{
  s32 x0, x1, y0, y1;

  // This now matches behavior of Mednafen and PCSX Rearmed's gpu_neon:
  x0 = GPU_EXPANDSIGN(packet.S2[2] + gpu_unai.DrawingOffset[0]);
  y0 = GPU_EXPANDSIGN(packet.S2[3] + gpu_unai.DrawingOffset[1]);

  u32 w = packet.U2[4] & 0x3ff; // Max width is 1023
  u32 h = packet.U2[5] & 0x1ff; // Max height is 511
  x1 = x0 + w;
  y1 = y0 + h;

  s32 xmin, xmax, ymin, ymax;
  xmin = gpu_unai.DrawingArea[0];
  xmax = gpu_unai.DrawingArea[2];
  ymin = gpu_unai.DrawingArea[1];
  ymax = gpu_unai.DrawingArea[3];

  if (y0 < ymin) y0 = ymin;
  if (y1 > ymax) y1 = ymax;
  if (y1 <= y0) return;

  if (x0 < xmin) x0 = xmin;
  if (x1 > xmax) x1 = xmax;
  x1 -= x0;
  if (x1 <= 0) return;

  const u16 Data = GPU_RGB16(packet.U4[0]);
  u16 *Pixel = &((u16*)gpu_unai.vram)[FRAME_OFFSET(x0, y0)];
  const int li=gpu_unai.ilace_mask;
  const int pi=(ProgressiveInterlaceEnabled()?(gpu_unai.ilace_mask+1):0);
  const int pif=(ProgressiveInterlaceEnabled()?(gpu_unai.prog_ilace_flag?(gpu_unai.ilace_mask+1):0):1);

  for (; y0<y1; ++y0)
  {
    if (!(y0&li) && (y0&pi)!=pif)
      gpuTileSpanDriver(Pixel,x1,Data);
    Pixel += FRAME_WIDTH;
  }
}
