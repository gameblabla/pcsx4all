/***************************************************************************
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
 * FPS & CPU-usage monitoring
 *
 * Added November 2016 by senquack (Daniel Silsby)
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// We only support CPU stats on UNIX platforms
#ifndef _WIN32
  #include <sys/resource.h>
  #define PERFMON_CPU_STATS
#endif

#ifdef _WIN32
  #ifndef suseconds_t
    #define suseconds_t long
  #endif
#endif //_WIN32

#include "perfmon.h"
#include "psxcommon.h"

static struct
{
  struct timeval tv_last;
  unsigned frame_ctr;

  struct
  {
    int num_entries;
    float fps[5];
#ifdef PERFMON_CPU_STATS
    float cpu[5];
#endif
  } buf;

  float fps_cur, fps_avg, fps_min, fps_max;
#ifdef PERFMON_CPU_STATS
  float cpu_cur, cpu_avg, cpu_min, cpu_max;
  struct timeval tv_last_ru_utime, tv_last_ru_stime;
#endif
} pmon;

// Returns # of microseconds spanning interval between tv and tv_old
static inline suseconds_t tvdiff_usec(const timeval &tv, const timeval &tv_old)
{
  return (tv.tv_sec - tv_old.tv_sec) * 1000000 + tv.tv_usec - tv_old.tv_usec;
}

// Returns # of microseconds total in tv1 and tv2
static inline suseconds_t tvsum_usec(const timeval &tv1, const timeval &tv2)
{
  return (tv1.tv_sec + tv2.tv_sec) * 1000000 + tv1.tv_usec + tv2.tv_usec;
}

#ifdef PERFMON_CPU_STATS
static void pmonInitCpuUsage()
{
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) < 0)
    return;

  pmon.tv_last_ru_utime = ru.ru_utime;
  pmon.tv_last_ru_stime = ru.ru_stime;
}

static float pmonGetCpuUsage(suseconds_t usecs_span)
{
  struct rusage ru;
  if (usecs_span == 0 || getrusage(RUSAGE_SELF, &ru) < 0)
    return 0;
  suseconds_t usecs_used = tvdiff_usec(ru.ru_utime, pmon.tv_last_ru_utime) +
                           tvdiff_usec(ru.ru_stime, pmon.tv_last_ru_stime);
  pmon.tv_last_ru_utime = ru.ru_utime;
  pmon.tv_last_ru_stime = ru.ru_stime;
  return (float)(100 * usecs_used) / (float)usecs_span;
}
#endif //PERFMON_CPU_STATS

void pmonReset()
{
  pmon.frame_ctr = 0;
  pmon.fps_cur = 0;
  memset(&pmon.buf, 0, sizeof(pmon.buf));

#ifdef PERFMON_CPU_STATS
  pmon.cpu_cur = 0;
  pmonInitCpuUsage();
#endif
  gettimeofday(&pmon.tv_last, 0);
}

bool pmonUpdate(struct timeval *tv_now)
{
  bool ret = false;
  pmon.frame_ctr++;
  suseconds_t diff = tvdiff_usec(*tv_now, pmon.tv_last);

  if (diff >= 1000000)
  {
    ret = true;
    pmon.fps_cur = 1000000.0f * (float)pmon.frame_ctr / (float)diff;
#ifdef PERFMON_CPU_STATS
    pmon.cpu_cur = pmonGetCpuUsage(diff);
#endif
    pmon.tv_last = *tv_now;
    pmon.frame_ctr = 0;

    bool new_detailed_stats = false;
    if (Config.PerfmonDetailedStats)
    {
      // Move old buffer entries to top, insert new entry at bottom
      pmon.buf.fps[pmon.buf.num_entries] = pmon.fps_cur;
#ifdef PERFMON_CPU_STATS
      pmon.buf.cpu[pmon.buf.num_entries] = pmon.cpu_cur;
#endif
      pmon.buf.num_entries++;

      if (pmon.buf.num_entries >= 5)
      {
        new_detailed_stats = true;
        pmon.buf.num_entries = 0;

        // Some very large pos/neg values to establish max/mins
        pmon.fps_min = (float)+1000000;
        pmon.fps_max = (float)-1000000;
        int fps_min_idx = 0;

        // Determine min/max value of all FPS entries
        for (int i=0; i < 5; ++i)
        {
          if (pmon.buf.fps[i] > pmon.fps_max)
          {
            pmon.fps_max = pmon.buf.fps[i];
          }
          if (pmon.buf.fps[i] < pmon.fps_min)
          {
            pmon.fps_min = pmon.buf.fps[i];
            fps_min_idx = i;
          }
        }

        // Compute FPS average. We don't include the minimum FPS
        //  value so external events don't skew the results too much.
        pmon.fps_avg = 0;
        for (int i=0; i < 5; ++i)
        {
          if (i != fps_min_idx) pmon.fps_avg += pmon.buf.fps[i];
        }
        pmon.fps_avg *= 0.25f;

#ifdef PERFMON_CPU_STATS
        // Some very large pos/neg values to establish max/mins
        pmon.cpu_min = (float)+1000000;
        pmon.cpu_max = (float)-1000000;
        int cpu_max_idx = 0;

        // Determine min/max value of all CPU entries
        for (int i=0; i < 5; ++i)
        {
          if (pmon.buf.cpu[i] > pmon.cpu_max)
          {
            pmon.cpu_max = pmon.buf.cpu[i];
            cpu_max_idx = i;
          }
          if (pmon.buf.cpu[i] < pmon.cpu_min)
          {
            pmon.cpu_min = pmon.buf.cpu[i];
          }
        }

        // Compute average values. We don't include the maximum CPU
        //  value so external events don't skew the results too much.
        pmon.cpu_avg = 0;
        for (int i=0; i < 5; ++i)
        {
          if (i != cpu_max_idx) pmon.cpu_avg += pmon.buf.cpu[i];
        }
        pmon.cpu_avg *= 0.25f;
#endif
      }
    }

    if (Config.PerfmonConsoleOutput)
      pmonPrintStats(new_detailed_stats);
  }
  return ret;
}

void pmonPause()
{
}

void pmonResume()
{
  pmon.frame_ctr = 0;
  gettimeofday(&pmon.tv_last, 0);
#ifdef PERFMON_CPU_STATS
  pmonInitCpuUsage();
#endif
}

void pmonGetStats(float *fps_cur, float *cpu_cur)
{
  *fps_cur = pmon.fps_cur;
#ifdef PERFMON_CPU_STATS
  *cpu_cur = pmon.cpu_cur;
#else
  *cpu_cur = 0;
#endif
}

void pmonPrintStats(bool print_detailed_stats)
{
#ifdef PERFMON_CPU_STATS
  printf("FPS: %6.1f  CPU: %6.1f%%\n", pmon.fps_cur, pmon.cpu_cur);
  if (print_detailed_stats)
  {
    printf("FPS min: %6.1f  max: %6.1f  avg: %6.1f\n", pmon.fps_min, pmon.fps_max, pmon.fps_avg);
    printf("CPU min: %6.1f%% max: %6.1f%% avg: %6.1f%%\n", pmon.cpu_min, pmon.cpu_max, pmon.cpu_avg);
    printf("\n");
  }
#else
  printf("FPS: %6.1f\n", pmon.fps_cur);
  if (print_detailed_stats)
  {
    printf("FPS min: %6.1f  max: %6.1f  avg: %6.1f\n", pmon.fps_min, pmon.fps_max, pmon.fps_avg);
    printf("\n");
  }
#endif
}
