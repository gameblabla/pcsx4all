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

#ifndef PLUGIN_LIB_H
#define PLUGIN_LIB_H

#include "plugins.h"
#include <sys/time.h>
#include <stdint.h>

struct pl_data_t
{
  bool fskip_advice, dynarec_compiled, is_pal;
  int8_t frameskip;
  int frame_interval, frame_interval1024;
  int vsync_usec_time;
  unsigned int dynarec_active_vsyncs;
  float fps_cur, cpu_cur;
  struct timeval tv_expect;

  struct timeval tv_last_clear;
  int clear_ctr;

  GPUScreenInfo_t sinfo, sinfo_last;
  char stats_msg[80]; // Short msg showing screen res, FPS, CPU usage, etc
};

extern struct pl_data_t pl_data;

void pl_frame_limit(void);
void pl_init(void);
void pl_reset(void);
void pl_pause(void);
void pl_resume(void);

void pl_clear_screen();
void pl_clear_borders();

static inline bool pl_frameskip_advice(void)
{
  return pl_data.fskip_advice;
}

// Dynamic recompilers call this to advise recompilation occurred
static inline void pl_dynarec_notify(void)
{
  pl_data.dynarec_compiled = true;
}

// In pl_sshot.cpp
void pl_screenshot_160x120_rgb565(u16 *dst);

#endif // PLUGIN_LIB_H
