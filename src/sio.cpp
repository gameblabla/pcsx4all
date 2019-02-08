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

/*
* SIO functions (Pads / Memcards)
*/

#include "sio.h"
#include "psxevents.h"
#include "misc.h"
#include <sys/stat.h>
#include <unistd.h>

// Status Flags
#define TX_RDY		0x0001
#define RX_RDY		0x0002
#define TX_EMPTY	0x0004
#define PARITY_ERR	0x0008
#define RX_OVERRUN	0x0010
#define FRAMING_ERR	0x0020
#define SYNC_DETECT	0x0040
#define DSR			0x0080
#define CTS			0x0100
#define IRQ			0x0200

// Control Flags
#define TX_PERM		0x0001
#define DTR			0x0002
#define RX_PERM		0x0004
#define BREAK		0x0008
#define RESET_ERR	0x0010
#define RTS			0x0020
#define SIO_RESET	0x0040

struct SioStruct {
	unsigned char buf[256];
	unsigned char cardh1[4];
	unsigned char cardh2[4];

	unsigned short StatReg;
	unsigned short ModeReg;
	unsigned short CtrlReg;
	unsigned short BaudReg;

	unsigned int bufcount;
	unsigned int parp;
	unsigned int mcdst,rdwr;
	unsigned char adrH,adrL;
	unsigned int padst;

	u32 sio_cycle; /* for SIO_INT() */
};

static SioStruct psxSio;

void sioInit(void) {
	//senquack - added initialization of sio data:
	memset(&psxSio, 0, sizeof(psxSio));

	//senquack-Rearmed uses 535 in all cases, so we'll use that instead:
	//sio_cycle=200*BIAS; /* for SIO_INT() */

	// clk cycle byte
	// 4us * 8bits = (PSXCLK / 1000000) * 32; (linuzappz)
	psxSio.sio_cycle = 535;

	//senquack - updated to match PCSXR code:
	psxSio.cardh1[0] = psxSio.cardh2[0] = 0xff;
	psxSio.cardh1[1] = psxSio.cardh2[1] = 0x08;
	psxSio.cardh1[2] = psxSio.cardh2[2] = 0x5a;
	psxSio.cardh1[3] = psxSio.cardh2[3] = 0x5d;
	// Transfer Ready and the Buffer is Empty
	psxSio.StatReg = TX_RDY | TX_EMPTY;
}

//senquack - Updated to use PSXINT_* enum and intCycle struct (much cleaner than before)
//           Also, takes an argument eCycle whereas before it took none.
//           TODO: PCSXR code has SIO_INT take a parameter that always ends up being
//             535 (SIO_CYCLES) but I've left PCSX4ALL using this older SIO_INT(void)
//           TODO: Add support for newer PCSXR Config.Sio option
// clk cycle byte
static inline void SIO_INT(void) {
	psxEvqueueAdd(PSXINT_SIO, psxSio.sio_cycle);
}

void sioWrite8(unsigned char value) {
#ifdef PAD_LOG
	PAD_LOG("sio write8 %x\n", value);
#endif
	//senquack - all calls to SIO_INT() here now pass param SIO_CYCLES
	//           whereas before they passed nothing:
	switch (psxSio.padst) {
		case 1: SIO_INT();
			if ((value & 0x40) == 0x40) {
				psxSio.padst = 2; psxSio.parp = 1;

				switch (psxSio.CtrlReg & 0x2002) {
					case 0x0002:
						psxSio.buf[psxSio.parp] = PAD1_poll();
						break;
					case 0x2002:
						psxSio.buf[psxSio.parp] = PAD2_poll();
						break;
				}

				if (!(psxSio.buf[psxSio.parp] & 0x0f)) {
					psxSio.bufcount = 2 + 32;
				} else {
					psxSio.bufcount = 2 + (psxSio.buf[psxSio.parp] & 0x0f) * 2;
				}
				if (psxSio.buf[psxSio.parp] == 0x41) {
					switch (value) {
						case 0x43:
							psxSio.buf[1] = 0x43;
							break;
						case 0x45:
							psxSio.buf[1] = 0xf3;
							break;
					}
				}
			}
			else psxSio.padst = 0;
			return;
		case 2:
			psxSio.parp++;

			switch (psxSio.CtrlReg & 0x2002) {
				case 0x0002: psxSio.buf[psxSio.parp] = PAD1_poll(); break;
				case 0x2002: psxSio.buf[psxSio.parp] = PAD2_poll(); break;
			}

			if (psxSio.parp == psxSio.bufcount) { psxSio.padst = 0; return; }
			SIO_INT();
			return;
	}

	switch (psxSio.mcdst) {
		case 1:
			SIO_INT();
			if (psxSio.rdwr) { psxSio.parp++; return; }
			psxSio.parp = 1;
			switch (value) {
				case 0x52: psxSio.rdwr = 1; break;
				case 0x57: psxSio.rdwr = 2; break;
				default: psxSio.mcdst = 0;
			}
			return;
		case 2: // address H
			SIO_INT();
			psxSio.adrH = value;
			*psxSio.buf = 0;
			psxSio.parp = 0;
			psxSio.bufcount = 1;
			psxSio.mcdst = 3;
			return;
		case 3: // address L
			SIO_INT();
			psxSio.adrL = value;
			*psxSio.buf = psxSio.adrH;
			psxSio.parp = 0;
			psxSio.bufcount = 1;
			psxSio.mcdst = 4;
			return;
		case 4:
			SIO_INT();
			psxSio.parp = 0;
			switch (psxSio.rdwr) {
				case 1: // read
					psxSio.buf[0] = 0x5c;
					psxSio.buf[1] = 0x5d;
					psxSio.buf[2] = psxSio.adrH;
					psxSio.buf[3] = psxSio.adrL;
					switch (psxSio.CtrlReg & 0x2002) {
						case 0x0002:
							sioMcdRead(MCD1, (char*)&psxSio.buf[4], (psxSio.adrL | (psxSio.adrH << 8)) * 128, 128);
							break;
						case 0x2002:
							sioMcdRead(MCD2, (char*)&psxSio.buf[4], (psxSio.adrL | (psxSio.adrH << 8)) * 128, 128);
							break;
					}

					{
						char cxor = 0;
						int i;
						for (i=2;i<128+4;i++)
							cxor ^= psxSio.buf[i];
						psxSio.buf[132] = cxor;
					}
					psxSio.buf[133] = 0x47;
					psxSio.bufcount = 133;
					break;
				case 2: // write
					psxSio.buf[0] = psxSio.adrL;
					psxSio.buf[1] = value;
					psxSio.buf[129] = 0x5c;
					psxSio.buf[130] = 0x5d;
					psxSio.buf[131] = 0x47;
					psxSio.bufcount = 131;
					break;
			}
			psxSio.mcdst = 5;
			return;
		case 5:	
			psxSio.parp++;
			//senquack - updated to match PCSXR code:
			if ((psxSio.rdwr == 1 && psxSio.parp == 132) ||
			    (psxSio.rdwr == 2 && psxSio.parp == 129)) {
				// clear "new card" flags
				if (psxSio.CtrlReg & 0x2000)
					psxSio.cardh2[1] &= ~8;
				else
					psxSio.cardh1[1] &= ~8;
			}
			if (psxSio.rdwr == 2) {
				if (psxSio.parp < 128) psxSio.buf[psxSio.parp + 1] = value;
			}
			SIO_INT();
			return;
	}

	switch (value) {
		case 0x01: // start pad
			psxSio.StatReg |= RX_RDY;		// Transfer is Ready

			switch (psxSio.CtrlReg & 0x2002) {
				case 0x0002: psxSio.buf[0] = PAD1_startPoll(); break;
				case 0x2002: psxSio.buf[0] = PAD2_startPoll(); break;
			}

			psxSio.bufcount = 2;
			psxSio.parp = 0;
			psxSio.padst = 1;
			SIO_INT();
			return;
		case 0x81: // start memcard
			if (psxSio.CtrlReg & 0x2000) {
				if (!sioMcdInserted(MCD2))
					goto no_device;
				memcpy(psxSio.buf, psxSio.cardh2, 4);
			} else {
				if (!sioMcdInserted(MCD1))
					goto no_device;
				memcpy(psxSio.buf, psxSio.cardh1, 4);
			}
			psxSio.StatReg |= RX_RDY;
			psxSio.parp = 0;
			psxSio.bufcount = 3;
			psxSio.mcdst = 1;
			psxSio.rdwr = 0;
			SIO_INT();
			return;
		default:
		no_device:
			psxSio.StatReg |= RX_RDY;
			psxSio.buf[0] = 0xff;
			psxSio.parp = 0;
			psxSio.bufcount = 0;
			return;
	}
}

void sioWrite16(u16 value)
{
	sioWrite8(value & 0xff);
	sioWrite8((value>>8) & 0xff);
}

void sioWrite32(u32 value)
{
	sioWrite8( value      & 0xff);
	sioWrite8((value>>8)  & 0xff);
	sioWrite8((value>>16) & 0xff);
	sioWrite8((value>>24) & 0xff);
}

// Empty function, disabled for now -senquack
//void sioWriteStat16(unsigned short value) {
//}

void sioWriteMode16(unsigned short value) {
	psxSio.ModeReg = value;
}

void sioWriteCtrl16(unsigned short value) {
	psxSio.CtrlReg = value & ~RESET_ERR;
	if (value & RESET_ERR) psxSio.StatReg &= ~IRQ;
	//senquack - Updated to match PCSX Rearmed
	// 'no DTR resets device, tested on the real thing'
	//if ((psxSio.CtrlReg & SIO_RESET) || (!psxSio.CtrlReg)) {
	if ((psxSio.CtrlReg & SIO_RESET) || !(psxSio.CtrlReg & DTR)) {
		psxSio.padst = 0; psxSio.mcdst = 0; psxSio.parp = 0;
		psxSio.StatReg = TX_RDY | TX_EMPTY;
		psxEvqueueRemove(PSXINT_SIO);
	}
}

void sioWriteBaud16(unsigned short value) {
	psxSio.BaudReg = value;
}

unsigned char sioRead8() {
	unsigned char ret = 0;

	if ((psxSio.StatReg & RX_RDY)/* && (CtrlReg & RX_PERM)*/) {
		//psxSio.StatReg &= ~RX_OVERRUN;
		ret = psxSio.buf[psxSio.parp];
		if (psxSio.parp == psxSio.bufcount) {
			psxSio.StatReg &= ~RX_RDY;		// Receive is not Ready now
			if (psxSio.mcdst == 5) {
				psxSio.mcdst = 0;
				if (psxSio.rdwr == 2) {
					switch (psxSio.CtrlReg & 0x2002) {
						case 0x0002:
							sioMcdWrite(MCD1, (char*)&psxSio.buf[1], (psxSio.adrL | (psxSio.adrH << 8)) * 128, 128);
							break;
						case 0x2002:
							sioMcdWrite(MCD2, (char*)&psxSio.buf[1], (psxSio.adrL | (psxSio.adrH << 8)) * 128, 128);
							break;
					}
				}
			}
			if (psxSio.padst == 2) psxSio.padst = 0;
			if (psxSio.mcdst == 1) {
				psxSio.mcdst = 2;
				psxSio.StatReg |= RX_RDY;
			}
		}
	}

#ifdef PAD_LOG
	PAD_LOG("sio read8 ;ret = %x\n", ret);
#endif
	return ret;
}

u16 sioRead16()
{
	u16 retval = sioRead8();
	retval |= (sioRead8() << 8);
	return retval;
}

u32 sioRead32()
{
	u32 retval = sioRead8();
	retval |= (sioRead8() << 8);
	retval |= (sioRead8() << 16);
	retval |= (sioRead8() << 24);
	return retval;
}

unsigned short sioReadStat16() {
	return psxSio.StatReg;
}

unsigned short sioReadMode16() {
	return psxSio.ModeReg;
}

unsigned short sioReadCtrl16() {
	return psxSio.CtrlReg;
}

unsigned short sioReadBaud16() {
	return psxSio.BaudReg;
}

void sioInterrupt() {
#ifdef PAD_LOG
	PAD_LOG("Sio Interrupt (CP0.Status = %x)\n", psxRegs.CP0.n.Status);
#endif
	//printf("Sio Interrupt\n");
	//  pcsx_rearmed: only do IRQ if it's bit has been cleared
	if (!(psxSio.StatReg & IRQ)) {
		psxSio.StatReg |= IRQ;
		psxHu32ref(0x1070) |= SWAPu32(0x80);
		// Ensure psxBranchTest() is called soon when IRQ is pending:
		ResetIoCycle();
	}
}

int sioFreeze(void* f, FreezeMode mode)
{
	if (    freeze_rw(f, mode, psxSio.buf, sizeof(psxSio.buf))
	     || freeze_rw(f, mode, &psxSio.StatReg, sizeof(psxSio.StatReg))
	     || freeze_rw(f, mode, &psxSio.ModeReg, sizeof(psxSio.ModeReg))
	     || freeze_rw(f, mode, &psxSio.CtrlReg, sizeof(psxSio.CtrlReg))
	     || freeze_rw(f, mode, &psxSio.BaudReg, sizeof(psxSio.BaudReg))
	     || freeze_rw(f, mode, &psxSio.bufcount, sizeof(psxSio.bufcount))
	     || freeze_rw(f, mode, &psxSio.parp, sizeof(psxSio.parp))
	     || freeze_rw(f, mode, &psxSio.mcdst, sizeof(psxSio.mcdst))
	     || freeze_rw(f, mode, &psxSio.rdwr, sizeof(psxSio.rdwr))
	     || freeze_rw(f, mode, &psxSio.adrH, sizeof(psxSio.adrH))
	     || freeze_rw(f, mode, &psxSio.adrL, sizeof(psxSio.adrL))
	     || freeze_rw(f, mode, &psxSio.padst, sizeof(psxSio.padst)) )
		return -1;
	else
		return 0;
}

////////////////////////
// Memcard operations //
////////////////////////

//TODO: Provide callback for error reporting in frontend GUIs

struct Memcard {
	Memcard() :
		filename(NULL),
		file(NULL),
		cur_offset(0)
	{}

	~Memcard()
	{
		if (file) {
			const char *tmpstr = filename ? filename : "";
			printf("Warning: memcard file not closed, closing via dtor: %s\n", tmpstr);
			fclose(file);
			file = NULL;
		}
	}

	char* filename;       // Filename ptr, or NULL if card is disabled
	FILE* file;           // File ptr is non-NULL when card is being written to
	long  cur_offset;     // Current file offset (when file is open for writing)
	char  data[MCD_SIZE];
};

static Memcard memcards[2];

// Number of cycles after last memcard write to wait until
//  PSXINT_SIO_SYNC_MCD event closes the memcard file.
#define MEMCARD_SYNC_DELAY (PSXCLK / 4)

// Intended to be called from within the running emulator after a small
//  amount of time has passed after a memcard has been written to.
//  This is done by scheduling a PSXINT_SIO_SYNC_MCD event. This allows
//  memcard I/O to be done against a file kept open for a decent amount of
//  time, allowing buffering and fewer open/close system calls.
//  Will flush, sync, and close any memcard files opened for writing.
void sioSyncMcds()
{
	FlushMcd(MCD1, true);
	FlushMcd(MCD2, true);
#ifdef DEBUG_MEMCARDS
	printf("%s()\n", __func__);
#endif
}

// If 'src' pointer is non-NULL, memcard data array will be updated
//  with data at offset 'adr' starting from source 'src' of size 'size',
//  then, memcard file will be written with this new data. If the data
//  at 'src' pointer is identical to existing array data, it is a no-op.
//  This prevents unnecessary sector writes, particularly write-tests
//  to offset 0x1f80 (block 0, sector 63) at BIOS/game startup.
// If 'src' pointer is NULL, just the file write will occur.
int sioMcdWrite(enum MemcardNum mcd_num, const char *src, uint32_t adr, int size)
{
	if (adr >= MCD_SIZE) {
		printf("Error in %s(): memcard %d, adr %x is outside memcard bounds (128KB)\n",
				__func__, mcd_num+1, adr);
		return -1;
	}

	if ((adr + size) > MCD_SIZE) {
		printf("Error in %s(): memcard %d, adr %x + size %x is outside memcard bounds (128KB)\n",
				__func__, mcd_num+1, adr, size);
		size = MCD_SIZE - adr;
		printf("Adjusted size to within 128KB range: %x\n", size);
	}

	bool write_file = true;
	if (src) {
		char* dst = memcards[mcd_num].data + adr;
		if (memcmp(dst, src, size) != 0) {
			memcpy(dst, src, size);
		} else {
			// Source data is identical to existing memcard data
			write_file = false;
#ifdef DEBUG_MEMCARDS
			printf("Prevented redundant write of %u bytes to memcard %d at addr %x\n",
					size, mcd_num+1, adr);
#endif
		}
	}

	int retval = 0;
	if (write_file) {
#ifdef DEBUG_MEMCARDS
		printf("Writing %u bytes to memcard %d\n", size, mcd_num + 1);
#endif
		retval = SaveMcd(mcd_num, adr, size);
	}

	// If memcard file was already open for writing, or was just opened by
	//  call to SaveMcd(), (re)schedule a memcard file flush/sync/close
	if (memcards[mcd_num].file != NULL)
		psxEvqueueAdd(PSXINT_SIO_SYNC_MCD, MEMCARD_SYNC_DELAY);

	return retval;
}

int sioMcdRead(enum MemcardNum mcd_num, char *dst, uint32_t adr, int size)
{
	if (adr >= MCD_SIZE) {
		printf("Error in %s(): memcard %d, adr %x is outside memcard bounds (128KB)\n",
				__func__, mcd_num+1, adr);
		return -1;
	}

	if ((adr + size) > MCD_SIZE) {
		printf("Error in %s(): memcard %d, adr %x + size %x is outside memcard bounds (128KB)\n",
				__func__, mcd_num+1, adr, size);
		size = MCD_SIZE - adr;
		printf("Adjusted size to within 128KB range: %x\n", size);
	}

	const char* src = memcards[mcd_num].data + adr;
	memcpy(dst, src, size);
	return 0;
}

char* sioMcdDataPtr(enum MemcardNum mcd_num)
{
	return memcards[mcd_num].data;
}

bool sioMcdInserted(enum MemcardNum mcd_num)
{
	return memcards[mcd_num].filename != NULL;
}

int sioMcdFormat(enum MemcardNum mcd_num)
{
#ifdef DEBUG_MEMCARDS
	printf("%s() on memcard %d\n", __func__, mcd_num+1);
#endif

	InitMcdData(sioMcdDataPtr(mcd_num));
	return sioMcdWrite(mcd_num, NULL, 0, MCD_SIZE);
}

// FlushMcd() ensures a memcard file temporarily opened for writing gets
//  closed. If 'sync_file' is true, it will call fsync() before closing it.
int FlushMcd(enum MemcardNum mcd_num, bool sync_file)
{
	Memcard &mc = memcards[mcd_num];

	if (!mc.file)
		return 0;

	int retval = 0;
	if (sync_file) {
		if (fflush(mc.file)) retval = -1;
		if (fsync(fileno(mc.file))) retval = -1;
	}
	if (fclose(mc.file)) retval = -1;
	mc.file = NULL;
	mc.cur_offset = 0;
	if (retval < 0) {
		perror(__func__);
		const char* tmpstr = mc.filename ? mc.filename : "";
		printf("Error in %s() writing memcard file %s\n", __func__, tmpstr);
	}
	return retval;
}

int EjectMcd(enum MemcardNum mcd_num)
{
	int retval = FlushMcd(mcd_num, true);
	Memcard &mc = memcards[mcd_num];
	mc.filename = NULL;
	mc.file = NULL;
	mc.cur_offset = 0;
	memset(mc.data, 0, MCD_SIZE);
	return retval;
}

void InitMcdData(char *mcd_data)
{
	memset(mcd_data, 0, MCD_SIZE);
	unsigned off = 0;

	// Header
	mcd_data[off++] = 'M';
	mcd_data[off++] = 'C';
	off += 0x7d;
	mcd_data[off++] = 0x0e;

	// Block directory
	for (int i = 0; i < 15; i++) {
		mcd_data[off++] = 0xa0;
		off += 0x07;
		mcd_data[off++] = 0xff;
		mcd_data[off++] = 0xff;
		off += 0x75;
		mcd_data[off++] = 0xa0;
	}

	// Broken sectors list
	for (int i = 0; i < 20; i++) {
		mcd_data[off++] = 0xff;
		mcd_data[off++] = 0xff;
		mcd_data[off++] = 0xff;
		mcd_data[off++] = 0xff;
		off += 0x04;
		mcd_data[off++] = 0xff;
		mcd_data[off++] = 0xff;
		off += 0x76;
	}
}

int CreateMcd(char *filename, bool overwrite_file)
{
	if (filename == NULL || filename[0] == '\0') {
		printf("Error: NULL or empty filename parameter in %s\n", __func__);
		return -1;
	}

	if (!overwrite_file && FileExists(filename)) {
#ifdef DEBUG_MEMCARDS
		printf("%s(): File %s exists, will not overwrite.\n", __func__, filename);
#endif
		return 0;
	}

	FILE *f = NULL;
	char mcd_data[MCD_SIZE];
	InitMcdData(mcd_data);
	if ( (f = fopen(filename, "wb")) == NULL          ||
	     fwrite(mcd_data, 1, MCD_SIZE, f) != MCD_SIZE ||
	     fflush(f)                                    ||
	     fsync(fileno(f)) )
		goto error;

	if (fclose(f)) {
		f = NULL;
		goto error;
	}

	return 0;

error:
	printf("Error in %s() creating memcard file %s:\n", __func__, filename);
	perror(NULL);
	if (f) fclose(f);
	return -1;
}

// If filename ptr is NULL, memcard will be disabled
int LoadMcd(enum MemcardNum mcd_num, char* filename)
{
	FILE *f = NULL;
	size_t bytes_read = 0;
	char *data = NULL;
	struct stat stat_buf;
	bool convert_data = false;
	Memcard &mc = memcards[mcd_num];

	EjectMcd(mcd_num);

	if (filename == NULL) {
		printf("%s(): NULL filename param, memcard %d slot is now empty.\n", __func__, mcd_num+1);
		return 0;
	}

	if (mcd_num == MCD1) {
		psxSio.cardh1[1] |= 8; // mark as new
	} else {
		psxSio.cardh2[1] |= 8;
	}

	mc.filename = filename;
	if (*mc.filename == 0) {
		sprintf(mc.filename, "memcards/card%d.mcd", mcd_num+1);
		printf("No memory card value was specified - creating a default card %s\n", mc.filename);
	}

	if ((f = fopen(mc.filename, "rb")) == NULL) {
		printf("The memory card %s doesn't exist - creating it\n", mc.filename);
		if (CreateMcd(mc.filename, false)) {
			printf("Error in %s(): Creating memcard file failed.\n", __func__);
			printf("Maybe file already exists and file/folder lacks permissions?\n");
			printf("Memcard slot %d is now empty.\n", mcd_num+1);
			mc.filename = NULL;
			return -1;
		}
		if ((f = fopen(mc.filename, "rb")) == NULL)
			goto error;
	}

	printf("Loading memory card %s\n", mc.filename);

	if (fstat(fileno(f), &stat_buf) != -1) {
		if (stat_buf.st_size == MCD_SIZE + 64) {
			// Assume Connectix Virtual Gamestation .vgs/.mem format
			printf("Detected Connectix VGS memcard format.\n");
			convert_data = true;
			if (fseek(f, 64, SEEK_SET) == -1) {
				printf("Error seeking to position 64 (VGS data offset).\n");
				goto error;
			}
		} else if (stat_buf.st_size == MCD_SIZE + 3904) {
			// Assume DexDrive .gme format
			printf("Detected DexDrive memcard format.\n");
			convert_data = true;
			if (fseek(f, 3904, SEEK_SET) == -1) {
				printf("Error seeking to position 3904 (DexDrive data offset).\n");
				goto error;
			}
		} else if (stat_buf.st_size != MCD_SIZE) {
			// If there's a size mismatch, just issue a warning
			// TODO: add error-reporting callback for GUI frontend
			printf("Warning: unknown memcard format (not raw image, DexDrive, or Connectix VGS)\n"
			       "File size: %lu\n"
			       "Expected size: 131072 (raw), 134976 (DexDrive), 131136 (VGS)\n",
			       stat_buf.st_size);
		}
	} else {
		perror("Warning: fstat() file size check failed");
	}

	data = memcards[mcd_num].data;
	if ((bytes_read = fread(data, 1, MCD_SIZE, f)) != MCD_SIZE) {
		printf("Error reading data from memory card %s!\n", mc.filename);
		printf("Wanted %zu bytes and got %zu\n", (size_t)MCD_SIZE, bytes_read);
		goto error;
	}

	fclose(f);

	// Convert file to native raw memcard format, if not already
	if (convert_data) {
		// Truncate file to 0 bytes, then write just the memcard data back
		if ( (f = fopen(mc.filename, "wb")) == NULL ||
		     fwrite(data, 1, MCD_SIZE, f) != MCD_SIZE ) {
			perror("Error converting memcard file");
			printf("Maybe file/folder lacks write permissions?\n");
			printf("Memcard slot %d is now empty.\n", mcd_num+1);
			if (f) fclose(f);
			mc.filename = NULL;
			return -1;
		}
		printf("Converted memcard file to native raw format.\n");
		fclose(f);
	}

	return 0;

error:
	printf("Error in %s():\n", __func__);
	perror(NULL);
	if (f) {
		printf("Error reading memcard file %s\n", mc.filename);
		fclose(f);
	} else {
		printf("Error opening memcard file %s\n", mc.filename);
	}
	// If we couldn't read more than one 8kb block, disable the memcard
	if (bytes_read <= 0x2000) {
		EjectMcd(mcd_num);
		printf("Memcard slot %d is now empty.\n", mcd_num+1);
	}
	return -1;
}

int SaveMcd(enum MemcardNum mcd_num, uint32_t adr, int size)
{
	Memcard &mc = memcards[mcd_num];

	// File isn't currently open for writing
	if (mc.file == NULL) {
		if (mc.filename == NULL || *mc.filename == '\0')
			return 0;
		mc.cur_offset = 0;
		if ((mc.file = fopen(mc.filename, "r+b")) == NULL)
			goto error;
	}

	if (mc.cur_offset != adr) {
		if (fseek(mc.file, adr, SEEK_SET))
			goto error;
		mc.cur_offset = adr;
	}

	if (fwrite(mc.data + adr, 1, size, mc.file) != size)
		goto error;

	mc.cur_offset += size;

	// Leave file open for writing, it will be close automatically
	//  by PSXINT_SIO_SYNC_MCD event
	return 0;

error:
	printf("Error in %s() writing to memcard %d\n", __func__, mcd_num+1);
	perror(NULL);
	{
		const char* tmpstr = mc.filename ? mc.filename : "";
		printf("Error writing to memcard file %s\n", tmpstr);
	}
	return -1;
}

// remove the leading and trailing spaces in a string
static void trim(char *str) {
	int pos = 0;
	char *dest = str;

	// skip leading blanks
	while (str[pos] <= ' ' && str[pos] > 0)
		pos++;

	while (str[pos]) {
		*(dest++) = str[pos];
		pos++;
	}

	*(dest--) = '\0'; // store the null

	// remove trailing blanks
	while (dest >= str && *dest <= ' ' && *dest > 0)
		*(dest--) = '\0';
}

void GetMcdBlockInfo(enum MemcardNum mcd_num, int block, McdBlock *Info) {
	char *data = NULL, *ptr, *str, *sstr;
	unsigned short clut[16];
	unsigned short c;
	int i, x;

	memset(Info, 0, sizeof(McdBlock));

	if (!sioMcdInserted(mcd_num))
		return;

	data = memcards[mcd_num].data;
	ptr = data + block * 8192 + 2;
	Info->IconCount = *ptr & 0x3;
	ptr+= 2;
	x = 0;
	str = Info->Title;
	sstr = Info->sTitle;

	for (i=0; i < 48; i++) {
		c = *(ptr) << 8;
		c|= *(ptr+1);
		if (!c) break;

		if (c >= 0x8281 && c <= 0x829A)
			c = (c - 0x8281) + 'a';
		else if (c >= 0x824F && c <= 0x827A)
			c = (c - 0x824F) + '0';
		else if (c == 0x8140) c = ' ';
		else if (c == 0x8143) c = ',';
		else if (c == 0x8144) c = '.';
		else if (c == 0x8146) c = ':';
		else if (c == 0x8147) c = ';';
		else if (c == 0x8148) c = '?';
		else if (c == 0x8149) c = '!';
		else if (c == 0x815E) c = '/';
		else if (c == 0x8168) c = '"';
		else if (c == 0x8169) c = '(';
		else if (c == 0x816A) c = ')';
		else if (c == 0x816D) c = '[';
		else if (c == 0x816E) c = ']';
		else if (c == 0x817C) c = '-';
		else {
			str[i] = ' ';
			sstr[x++] = *ptr++; sstr[x++] = *ptr++;
			continue;
		}

		str[i] = sstr[x++] = c;
		ptr+=2;
	}

	trim(str);
	trim(sstr);

	ptr = data + block * 8192 + 0x60; // icon palete data

	for (i = 0; i < 16; i++) {
		clut[i] = *((unsigned short*)ptr);
		ptr += 2;
	}

	for (i = 0; i < Info->IconCount; i++) {
		short *icon = &Info->Icon[i*16*16];

		ptr = data + block * 8192 + 128 + 128 * i; // icon data

		for (x = 0; x < 16 * 16; x++) {
			icon[x++] = clut[*ptr & 0xf];
			icon[x] = clut[*ptr >> 4];
			ptr++;
		}
	}

	ptr = data + block * 128;

	Info->Flags = *ptr;

	ptr += 0xa;
	strncpy(Info->ID, ptr, 12);
	ptr += 12;
	strncpy(Info->Name, ptr, 16);
}
