/***************************************************************************
 * (C) notaz, 2010-2011                                                    *
 * (C) PCSX4ALL team 2016                                                  *
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
 * Plugin library to assist with frameskip, controls, etc.
 * Largely taken/based on Notaz's PCSX Rearmed.
 *
 * Added Dec 2016 by senquack (Daniel Silsby)
 *
 */

#include <unistd.h>

#include "psxcommon.h"
#include "plugin_lib.h"
#include "perfmon.h"
#include "plugins.h"

#ifdef USE_GPULIB
  #include "gpu/gpulib/gpu.h"
#endif

static void pl_frameskip_prepare(s8 frameskip);
static void pl_stats_update(void);

#define MAX_LAG_FRAMES 3

#define tvdiff(tv, tv_old) \
  ((tv.tv_sec - tv_old.tv_sec) * 1000000 + tv.tv_usec - tv_old.tv_usec)

struct pl_data_t pl_data;

void pl_clear_screen()
{
  u16 *dst = SCREEN;
  memset((void*)dst, 0, 320*240*2);
}

void pl_clear_borders()
{
  gettimeofday(&pl_data.tv_last_clear, 0);
  pl_clear_screen();

  pl_data.clear_ctr = 4;
}

static void pl_frameskip_prepare(s8 frameskip)
{
  pl_data.fskip_advice = false;
  pl_data.frameskip = frameskip;
  pl_data.is_pal = (Config.PsxType == PSXTYPE_PAL);
  pl_data.frame_interval = pl_data.is_pal ? 20000 : 16667;
  pl_data.frame_interval1024 = pl_data.is_pal ? 20000*1024 : 17066667;

  struct timeval now;
  gettimeofday(&pl_data.tv_last_clear, 0);
  gettimeofday(&now, 0);
  pl_data.vsync_usec_time = now.tv_usec;
  while (pl_data.vsync_usec_time >= pl_data.frame_interval)
    pl_data.vsync_usec_time -= pl_data.frame_interval;

#ifdef USE_GPULIB
  gpulib_frameskip_prepare(frameskip);
#endif
}

/* called on every vsync */
void pl_frame_limit(void)
{
  struct timeval now;
  int diff, usadj;

  gettimeofday(&now, 0);

  GPU_getScreenInfo(&pl_data.sinfo);

  if (pl_data.clear_ctr > 0)
  {
    diff = tvdiff(now, pl_data.tv_last_clear);
    if (diff > 166667)
    {
      pl_data.clear_ctr--;
      pl_data.tv_last_clear = now;
      pl_clear_screen();
    }

  }

  // Update performance monitor
  bool new_stats = pmonUpdate(&now);
  if (new_stats)
  {
    pmonGetStats(&pl_data.fps_cur, &pl_data.cpu_cur);
    pl_stats_update();
  }
  extern boolean use_speedup;
  s8 framskip = use_speedup == false ? Config.FrameSkip : 3;
  // If cfg settings change, catch it here
  if (pl_data.frameskip != framskip ||
      pl_data.is_pal != (Config.PsxType == PSXTYPE_PAL))
  {
    pl_frameskip_prepare(framskip);
  }

  // tv_expect uses usec*1024 units instead of usecs for better accuracy
  pl_data.tv_expect.tv_usec += pl_data.frame_interval1024;
  if (pl_data.tv_expect.tv_usec >= (1000000 << 10))
  {
    pl_data.tv_expect.tv_usec -= (1000000 << 10);
    pl_data.tv_expect.tv_sec++;
  }
  diff = (pl_data.tv_expect.tv_sec - now.tv_sec) * 1000000 +
         (pl_data.tv_expect.tv_usec >> 10) - now.tv_usec;

  if (diff > MAX_LAG_FRAMES * pl_data.frame_interval ||
      diff < -MAX_LAG_FRAMES * pl_data.frame_interval)
  {
    //printf("pl_frame_limit reset, diff=%d, iv %d\n", diff, frame_interval);
    pl_data.tv_expect = now;
    diff = 0;
    // try to align with vsync
    usadj = pl_data.vsync_usec_time;
    while (usadj < pl_data.tv_expect.tv_usec - pl_data.frame_interval)
      usadj += pl_data.frame_interval;
    pl_data.tv_expect.tv_usec = usadj << 10;
  }

  if ((Config.FrameLimit && !use_speedup) && (diff > pl_data.frame_interval))
  {
    usleep(diff - pl_data.frame_interval);
  }

  if (diff < -pl_data.frame_interval)
  {
    pl_data.fskip_advice = true;
  }
  else if (diff >= 0)
  {
    pl_data.fskip_advice = false;
  }

  // recompilation is not that fast and may cause frame skip on
  // loading screens and such, resulting in flicker or glitches
  if (pl_data.dynarec_compiled)
  {
    if (pl_data.dynarec_active_vsyncs < 32)
      pl_data.fskip_advice = false;
    pl_data.dynarec_active_vsyncs++;
  }
  else
  {
    pl_data.dynarec_active_vsyncs = 0;
  }
  pl_data.dynarec_compiled = false;
}

void pl_init(void)
{
  pl_reset();

#ifdef USE_GPULIB
  gpulib_set_config(&gpulib_config);
#endif
}

void pl_reset(void)
{
  pl_data.clear_ctr = 0;
  pl_data.fps_cur = pl_data.cpu_cur = 0;
  pl_data.dynarec_compiled = false;
  pl_data.dynarec_active_vsyncs = 0;
  pl_frameskip_prepare(Config.FrameSkip);
  sprintf(pl_data.stats_msg, "000x000x00 CPU=000%% FPS=000/00");
  pmonReset(); // Reset performance monitor (FPS,CPU usage,etc)
}

// Called when emu paused and frontend is active
void pl_pause(void)
{
  pmonPause();
}

// Called when leaving frontend back to emu
void pl_resume(void)
{
  pmonResume();
  pl_frameskip_prepare(Config.FrameSkip);
  GPU_requestScreenRedraw(); // GPU plugin should redraw screen
}

static void pl_stats_update(void)
{
  // TODO: show skipped frames in stats message

  sprintf(pl_data.stats_msg, "%3ux%3ux%s CPU=%3u%% FPS=%3u/%u",
          pl_data.sinfo.hres,
          pl_data.sinfo.vres,
          pl_data.sinfo.depth24 ? "24" : "15",
          (unsigned int)(pl_data.cpu_cur + 0.5f),
          (unsigned int)(pl_data.fps_cur + 0.5f),
          pl_data.sinfo.pal ? 50 : 60);
}
