/*
 * (C) Gražvydas "notaz" Ignotas, 2011
 *
 * This work is licensed under the terms of any of these licenses
 * (at your option):
 *  - GNU GPL, version 2 or later.
 *  - GNU LGPL, version 2.1 or later.
 * See the COPYING file in the top-level directory.
 */

#ifndef GPULIB_GPU_H
#define GPULIB_GPU_H

#include <stdint.h>

#ifdef GPU_UNAI
  #include "gpu/gpu_unai/gpu.h"  // To get config struct definition
#endif

#define CMD_BUFFER_LEN          1024

struct psx_gpu
{
  uint32_t cmd_buffer[CMD_BUFFER_LEN];
  uint32_t regs[16];
  uint16_t *vram;
  union
  {
    uint32_t reg;
    struct
    {
      uint32_t tx:4;        //  0 texture page
      uint32_t ty:1;
      uint32_t abr:2;
      uint32_t tp:2;        //  7 t.p. mode (4,8,15bpp)
      uint32_t dtd:1;       //  9 dither
      uint32_t dfe:1;
      uint32_t md:1;        // 11 set mask bit when drawing
      uint32_t me:1;        // 12 no draw on mask
      uint32_t unkn:3;
      uint32_t width1:1;    // 16
      uint32_t width0:2;
      uint32_t dheight:1;   // 19 double height
      uint32_t video:1;     // 20 NTSC,PAL
      uint32_t rgb24:1;
      uint32_t interlace:1; // 22 interlace on
      uint32_t blanking:1;  // 23 display not enabled
      uint32_t unkn2:2;
      uint32_t busy:1;      // 26 !busy drawing
      uint32_t img:1;       // 27 ready to DMA image data
      uint32_t com:1;       // 28 ready for commands
      uint32_t dma:2;       // 29 off, ?, to vram, from vram
      uint32_t lcf:1;       // 31
    };
  } status;
  uint32_t gp0;
  uint32_t ex_regs[8];
  struct
  {
    int hres, vres;
    int x, y, w, h;
    int x1, x2;
    int y1, y2;
  } screen;
  struct
  {
    int x, y, w, h;
    short int offset, is_read;
  } dma, dma_start;
  int cmd_len;
  uint32_t zero;
  struct
  {
    uint32_t fb_dirty:1;
    uint32_t old_interlace:1;
    uint32_t allow_interlace:2;
    uint32_t blanked:1;
    uint32_t enhancement_enable:1;
    uint32_t enhancement_active:1;
    uint32_t *frame_count;
    uint32_t *hcnt; /* hsync count */
    struct
    {
      uint32_t addr;
      uint32_t cycles;
      uint32_t frame;
      uint32_t hcnt;
    } last_list;
    uint32_t last_vram_read_frame;
  } state;
  struct
  {
    int32_t set:3; /* -1 auto, 0 off, 1-3 fixed */
    int32_t cnt:3; /* amount skipped in a row */
    uint32_t active:1;
    uint32_t allow:1;
    uint32_t frame_ready:1;
    uint32_t last_flip_frame;
    uint32_t pending_fill[3];
  } frameskip;
#ifdef GPULIB_USE_MMAP
  void *(*mmap)(unsigned int size);
  void  (*munmap)(void *ptr, unsigned int size);
#endif
};

extern struct psx_gpu gpu;

struct gpulib_config_t
{
#ifdef GPULIB_USE_MMAP
  void *(*mmap)(unsigned int size);
  void  (*munmap)(void *ptr, unsigned int size);
#endif

  struct
  {
    int   iUseDither;
    int   dwActFixes;
    float fFrameRateHz;
    int   dwFrameRateTicks;
  } gpu_peops_config;
  struct
  {
    //senquack - disabled, not sure this is needed and would require modifying
    // sprite-span functions, perhaps unnecessarily. No Abe Oddysey hack was
    // present in latest PCSX4ALL sources we were using.
    //bool abe_hack;
    uint8_t no_light, no_blend;
    int  lineskip;
  } gpu_unai_config;
};

extern struct gpulib_config_t gpulib_config;

void gpulib_frameskip_prepare(signed char frameskip);
void gpulib_set_config(const struct gpulib_config_t *config);

int  vout_init(void);
int  vout_finish(void);
void vout_update(void);
void vout_blank(void);
void vout_set_config(const struct gpulib_config_t *config);
#endif // GPULIB_GPU_H
