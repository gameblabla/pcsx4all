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

#ifndef PERFMON_H
#define PERFMON_H

#include <sys/time.h>

// Called when (re)starting a game, before first call to pmonUpdate()
void pmonReset();

// Called once per frame, updating the stats.
// Returns true if new stats were generated.
bool pmonUpdate(struct timeval *tv_now);

// Return current FPS, CPU%
void pmonGetStats(float *fps_cur, float *cpu_cur);

// Output stats to console
void pmonPrintStats(bool print_detailed_stats);

// Called when pausing emu and entering frontend, or vice versa
void pmonPause();
void pmonResume();

#endif //PERFMON_H
