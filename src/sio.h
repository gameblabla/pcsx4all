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


#ifndef _SIO_H_
#define _SIO_H_

#include "psxcommon.h"
#include "r3000a.h"
#include "psxmem.h"
#include "plugins.h"
#include "psemu_plugin_defs.h"

void sioInit(void);

void sioWrite8(unsigned char value);
void sioWrite16(u16 value);
void sioWrite32(u32 value);

// Empty function, disabled for now -senquack
//void sioWriteStat16(unsigned short value);

void sioWriteMode16(unsigned short value);
void sioWriteCtrl16(unsigned short value);
void sioWriteBaud16(unsigned short value);

unsigned char sioRead8(void);
u16 sioRead16(void);
u32 sioRead32(void);

unsigned short sioReadStat16(void);
unsigned short sioReadMode16(void);
unsigned short sioReadCtrl16(void);
unsigned short sioReadBaud16(void);

void sioInterrupt(void);
int sioFreeze(void* f, FreezeMode mode);

////////////////////////
// Memcard operations //
////////////////////////

#define MCD_SIZE	(1024 * 8 * 16)

enum MemcardNum {
	MCD1 = 0,
	MCD2 = 1
};

/* High-level memcard operations
 * (Emulator code should use these when possible/appropriate)
 */
void sioSyncMcds(void);
int sioMcdWrite(enum MemcardNum mcd_num, const char *src, uint32_t adr, int size);
int sioMcdRead(enum MemcardNum mcd_num, char *dst, uint32_t adr, int size);
char* sioMcdDataPtr(enum MemcardNum mcd_num);
bool sioMcdInserted(enum MemcardNum mcd_num);
int sioMcdFormat(enum MemcardNum mcd_num);

/* Low-level memcard operations, used by above functions
 *  and any memcard management code.
 */
int FlushMcd(enum MemcardNum mcd_num, bool sync_file);
int EjectMcd(enum MemcardNum mcd_num);
void InitMcdData(char *mcd_data);
int CreateMcd(char *filename, bool overwrite_file);
int LoadMcd(enum MemcardNum mcd_num, char* filename);
int SaveMcd(enum MemcardNum mcd_num, uint32_t adr, int size);

typedef struct {
	char Title[48 + 1]; // Title in ASCII
	char sTitle[48 * 2 + 1]; // Title in Shift-JIS
	char ID[12 + 1];
	char Name[16 + 1];
	int IconCount;
	short Icon[16*16*3];
	unsigned char Flags;
} McdBlock;

void GetMcdBlockInfo(enum MemcardNum mcd_num, int block, McdBlock *Info);

#endif
