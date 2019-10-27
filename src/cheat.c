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

#include "cheat.h"

#include "psxmem.h"
#include "misc.h"

#include "port.h"

#ifdef CHEAT_ZIP
#include "unzip.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

static cheat_t* ct = NULL;
static u32 run_interval = 200u;
static u32 next_ticks = 0u;

void cheat_load(void)
{
	char cheat_filename[PATH_MAX];
	FILE* f;
	char *filebuf;
	int i, disableall, buf_pos = 0;
	cheat_entry_t* entry = NULL;
	cheat_unload();
	snprintf(cheat_filename, sizeof(cheat_filename), "%s/%s.txt", cheatsdir, CdromId);
	f = fopen(cheat_filename, "r");
	if (f) 
	{
		fseeko(f, 0, SEEK_END);
		off_t len = ftello(f);
		filebuf = malloc(len + 1);
		filebuf[len] = 0;
		fseeko(f, 0, SEEK_SET);
		fread(filebuf, 1, len, f);
		fclose(f);
	}
	#ifdef CHEAT_ZIP
	else
	{
		unz_file_info finfo;
		snprintf(cheat_filename, sizeof(cheat_filename), "%s/cheats.zip", cheatsdir);
		unzFile uf = unzOpen(cheat_filename);
		if (uf == NULL) return;
		snprintf(cheat_filename, sizeof(cheat_filename), "%s.txt", CdromId);
		if (unzLocateFile(uf, cheat_filename, 2) != UNZ_OK || unzGetCurrentFileInfo(uf, &finfo, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK || unzOpenCurrentFile(uf) != UNZ_OK) {
			unzClose(uf);
			return;
		}
		filebuf = new char[finfo.uncompressed_size + 1];
		filebuf[finfo.uncompressed_size] = 0;
		unzReadCurrentFile(uf, filebuf, finfo.uncompressed_size);
		unzCloseCurrentFile(uf);
		unzClose(uf);
	}
	#else
	else
	{
		printf("No cheats found\n");
		return;
	}
	#endif
	ct = (cheat_t*) calloc(1, sizeof(cheat_t));
	for(;;) {
		char *buf = filebuf + buf_pos;
		if (*buf == 0) break;
		int token_pos = strcspn(buf, "\r\n");
		if (buf[token_pos] == 0)
			buf_pos += token_pos;
		else {
			buf[token_pos] = 0;
			buf_pos += token_pos + 1;
			while(filebuf[buf_pos] == '\r' || filebuf[buf_pos] == '\n') ++buf_pos;
		}
		while (*buf == ' ' || *buf == '\t') ++buf;
		if (*buf == '#') {
			int pos;
			// new entry
			if (ct->entries_cap <= ct->num_entries) {
				int new_cap = ct->num_entries + 4;
				ct->entries = (cheat_entry_t*) realloc(ct->entries, new_cap * sizeof(cheat_entry_t));
				memset(&ct->entries[ct->entries_cap], 0, (new_cap - ct->entries_cap) * sizeof(cheat_entry_t));
				ct->entries_cap = new_cap;
			}
			entry = &ct->entries[ct->num_entries++];
			entry->name[0] = entry->name[1] = ' ';
			snprintf(entry->name + 2, sizeof(entry->name) - 2, "%s", buf + 1);
			pos = strlen(entry->name);
			while (--pos >= 0 && (entry->name[pos] == '\r' || entry->name[pos] == '\n')) {
				entry->name[pos] = 0;
			}
		} else {
			if (!entry) continue;
			// new code llne
			// do simple check: 8 + 1 + 4
			if (strlen(buf) >= 13 && (buf[8] == ' ' || buf[8] == '\t')) {
				u32 code1;
				u16 code2;
				cheat_line_t* line;
				if ((*buf < '0' || *buf > '9') && (*buf < 'A' || *buf > 'F') && (*buf < 'a' || *buf > 'f')) {
					continue;
				}
				code1 = (u32) strtoul(buf, NULL, 16);
				buf += 9;
				while (*buf == ' ' || *buf == '\t') ++buf;
				if ((*buf < '0' || *buf > '9') && (*buf < 'A' || *buf > 'F') && (*buf < 'a' || *buf > 'f')) {
					continue;
				}
				code2 = (u16) strtoul(buf, NULL, 16);
				if (entry->lines_cap <= entry->num_lines) {
					entry->lines_cap = entry->num_lines + 4;
					entry->lines = (cheat_line_t*) realloc(entry->lines, entry->lines_cap * sizeof(cheat_line_t));
				}
				line = &entry->lines[entry->num_lines++];
				line->code1 = code1;
				line->code2 = code2;
			}
		}
	}
	if (filebuf) free(filebuf);
	disableall = 0;
	for (i = 0; i < ct->num_entries; ++i) {
		u32 code;
		cheat_entry_t* entry = &ct->entries[i];
		entry->cont_enabled = 1;
		if (entry->num_lines < 1) continue;
		// check M code
		code = entry->lines[0].code1 >> 24;
		if (code == 0xC0 || code == 0xC1) {
			entry->user_enabled = 2;
			entry->name[0] = 'M';
		} else entry->user_enabled = 0;
		// if we meet 0xC0 M code or 0xD5 pad enabler, disable all codes first
		if (code == 0xC0 || code == 0xD5) disableall = 1;
	}
	if (disableall) {
		for (i = 0; i < ct->num_entries; ++i) {
			ct->entries[i].cont_enabled = 0;
		}
	}
}

void cheat_unload(void)
{
	int i;
	if (!ct) return;
	if (ct->entries) {
		for (i = 0; i < ct->num_entries; ++i) {
			cheat_entry_t* e = &ct->entries[i];
			if (e->lines) {
				free(e->lines);
				e->lines = NULL;
			}
		}
		free(ct->entries);
		ct->entries = NULL;
	}
	free(ct);
	ct = NULL;
}

void cheat_set_run_per_sec(int r)
{
	run_interval = 999 / r + 1;
}

const cheat_t* cheat_get(void)
{
	return ct;
}

static int cheat_run_code(const cheat_line_t* lines, int total, int* mcode_run)
{
	u32 code1 = lines->code1;
	u16 code2 = lines->code2;
	switch (code1 >> 24) {
	case 0x10: {
		u32 addr = code1 & 0x00FFFFFFu;
		psxMemWrite16(addr, psxMemRead16(addr) + code2);
		return 1;
	}
	case 0x11: {
		u32 addr = code1 & 0x00FFFFFFu;
		psxMemWrite16(addr, psxMemRead16(addr) - code2);
		return 1;
	}
	case 0x20: {
		u32 addr = code1 & 0x00FFFFFFu;
		psxMemWrite8(addr, psxMemRead8(addr) + (code2 & 0xFFu));
		return 1;
	}
	case 0x21: {
		u32 addr = code1 & 0x00FFFFFFu;
		psxMemWrite8(addr, psxMemRead8(addr) - (code2 & 0xFFu));
		return 1;
	}
	case 0x1F: {
		if (code1 >= 0x1F800000u && code1 < 0x1F800400u) {
			psxMemWrite16(code1, code2);
		} else return 1;
	}
	case 0x30:psxMemWrite8(code1 & 0x00FFFFFFu, code2 & 0xFFu);
		return 1;
	case 0x80:psxMemWrite16(code1 & 0x00FFFFFFu, code2);
		return 1;
	case 0x50: {
		u32 cnt, addr, value, addrstep, addrend, bits;
		if (total < 2) return -1;
		cnt = (code1 >> 8) & 0xFFFFu;
		addrstep = code1 & 0xFFu;
		addr = (++lines)->code1;
		bits = (addr >> 28) != 3 ? 1 : 0; // 1=16bit 0=8bit
		addr &= 0x00FFFFFFu;
		addrend = addr + cnt * addrstep;
		value = lines->code2;
		while (addr < addrend) {
			addr += addrstep;
			value += code2;
			bits ? psxMemWrite16(addr, value) : psxMemWrite8(addr, value & 0xFFu);
		}
		return 2;
	}
	case 0xE0:if (psxMemRead8(code1 & 0x00FFFFFFu) != (code2 & 0xFFu)) return -1;
		return 1;
	case 0xE1:if (psxMemRead8(code1 & 0x00FFFFFFu) == (code2 & 0xFFu)) return -1;
		return 1;
	case 0xE2:if (psxMemRead8(code1 & 0x00FFFFFFu) <= (code2 & 0xFFu)) return -1;
		return 1;
	case 0xE3:if (psxMemRead8(code1 & 0x00FFFFFFu) >= (code2 & 0xFFu)) return -1;
		return 1;
	case 0xD0:if (psxMemRead16(code1 & 0x00FFFFFFu) != code2) return -1;
		return 1;
	case 0xD1:if (psxMemRead16(code1 & 0x00FFFFFFu) == code2) return -1;
		return 1;
	case 0xD2:if (psxMemRead16(code1 & 0x00FFFFFFu) <= code2) return -1;
		return 1;
	case 0xD3:if (psxMemRead16(code1 & 0x00FFFFFFu) >= code2) return -1;
		return 1;
	case 0xC0:if (psxMemRead16(code1 & 0x00FFFFFFu) != code2) return -2;
		*mcode_run = 1;
		return 1;
	case 0xC1:
		if (code2 > 0) {
			*mcode_run = 1;
#ifdef TIME_IN_MSEC
			next_ticks = get_ticks() + code2;
#else
			next_ticks = get_ticks() / 1000 + code2;
#endif
			return -2;
		}
		return 1;
	case 0xD4: {
		if ((pad_read(0) & code2) == code2) return 1;
		return -1;
	}
	case 0xD5: {
		if ((pad_read(0) & code2) == code2) {
			int i;
			for (i = 0; i < ct->num_entries; ++i) {
				if (ct->entries[i].cont_enabled == 0)
					ct->entries[i].cont_enabled = 1;
			}
		}
		return 1;
	}
	case 0xD6: {
		if ((pad_read(0) & code2) == code2) {
			int i;
			for (i = 0; i < ct->num_entries; ++i) {
				if (ct->entries[i].cont_enabled == 1)
					ct->entries[i].cont_enabled = 0;
			}
			return -2;
		}
		return 1;
	}
	case 0xC2: {
		u32 src, dst, srcend;
		if (total < 2) return -1;
		src = code1 & 0x00FFFFFFu;
		srcend = src + code2;
		dst = (lines + 1)->code1 & 0x00FFFFFFu;
		// TODO: optimize block copy algorithm
		while (src < srcend) {
			psxMemWrite8(dst++, psxMemRead8(src++));
		}
		return 2;
	}
	}
	return -1;
}

void cheat_apply(void)
{
	u32 curr_ticks;
	int i;
	if (!ct || !ct->num_entries) return;
#ifdef TIME_IN_MSEC
	curr_ticks = get_ticks();
#else
	curr_ticks = get_ticks() / 1000;
#endif
	if (curr_ticks < next_ticks) return;
	next_ticks = curr_ticks + run_interval;
	for (i = 0; i < ct->num_entries; ++i) {
		int j, num_lines;
		cheat_entry_t* entry = &ct->entries[i];
		if (entry->user_enabled == 0 || entry->cont_enabled == 0) continue;
		j = 0;
		num_lines = entry->num_lines;
		while (j < num_lines) {
			int mcode_run = 0;
			int skip_lines = cheat_run_code(&entry->lines[j], num_lines - j, &mcode_run);
			if (mcode_run) entry->cont_enabled = 0;
			if (skip_lines <= 0) {
				if (skip_lines == -2) return;
				break;
			}
			j += skip_lines;
		}
	}
}

void cheat_toggle(int idx)
{
	if (idx < 0 || idx >= ct->num_entries) return;
	if (ct->entries[idx].user_enabled == 0 || ct->entries[idx].user_enabled == 1) {
		ct->entries[idx].user_enabled ^= 1;
		ct->entries[idx].name[0] = ct->entries[idx].user_enabled ? '*' : ' ';
	}
}
