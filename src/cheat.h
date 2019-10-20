/***************************************************************************
 *   Copyright (C) 2007 Ryan Schultz, PCSX-df Team, PCSX team              *
 *   schultz.ryan@gmail.com, http://rschultz.ath.cx/code.php               *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02111-1307 USA.            *
 ***************************************************************************/
 
/* Cheat code by Soar Qin, minor modifications by Gameblabla */

#ifndef __CHEAT_H__
#define __CHEAT_H__

#include "psxcommon.h"

typedef struct cheat_line_ {
  u32 code1;
  u16 code2;
} cheat_line_t;

typedef struct cheat_entry_ {
  int num_lines;
  int lines_cap;
  cheat_line_t *lines;
  char name[37];
  int user_enabled:2; // 0: disabled  1: enabled  2: M code
  int cont_enabled:1; // 0: disabled  1: enabled
} cheat_entry_t;

typedef struct cheat_ {
  int num_entries;
  int entries_cap;
  cheat_entry_t *entries;
} cheat_t;

void cheat_load(void);
void cheat_unload(void);
void cheat_set_run_per_sec(int r);
const cheat_t *cheat_get(void);
void cheat_apply(void);
void cheat_toggle(int idx);

#endif
