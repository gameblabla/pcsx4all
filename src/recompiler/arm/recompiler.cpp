/**************************************************************************
*   Copyright (C) 2010 PCSX4ALL Team                                      *
*   Copyright (C) 2010 Franxis and Chui                                   *
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
* ARM assembly functions for R3000A core.
*/
#include "psxcommon.h"
#include "psxhw.h"
#include "r3000a.h"
#include "psxmem.h"
#include "disarm.h"
#include "gte.h"
#include "port.h"


#ifdef __SYMBIAN32__
void CLEAR_INSN_CACHE(u32* BEG, u32* END);
#define sys_cacheflush CLEAR_INSN_CACHE
#endif

static u32 psxRecLUT[0x010000];

#undef PC_REC
#undef PC_REC8
#undef PC_REC16
#undef PC_REC32
#define PC_REC(x)	(psxRecLUT[x >> 16] + (x & 0xffff))
#define PC_REC8(x)	(*(u8 *)PC_REC(x))
#define PC_REC16(x) (*(u16*)PC_REC(x))
#define PC_REC32(x) (*(u32*)PC_REC(x))

#define RECMEM_SIZE	(12 * 1024 * 1024)
// CHUI: El maximo del bloque recompilado
#define REC_MAX_OPCODES	80
// CHUI: Limite de opcodes para salir
#define REC_MAX_OPCODES_LIMIT (REC_MAX_OPCODES-(REC_MAX_OPCODES/4))
// CHUI: Maxima distancia entre saltos para saltar la comprobación de si es correcto
#if defined(DEBUG_CPU_OPCODES) || defined(DEBUG_CPU)
#define REC_MAX_TO_TEST 0
#else
#define REC_MAX_TO_TEST 64
#endif
// CHUI: El maximo de unroll-loops
#if defined(DEBUG_CPU_OPCODES) || defined(DEBUG_CPU)
#define REC_MAX_LOOPS 0
#else
#define REC_MAX_LOOPS (REC_MAX_OPCODES/REC_MAX_TO_TEST)
#endif
// CHUI: El maximo de saltos donde no se hace psxBranchTest
#if defined(DEBUG_CPU_OPCODES) || defined(DEBUG_CPU)
#define REC_MAX_SKIPS 0
#else
#define REC_MAX_SKIPS 5
#endif
// CHUI: El maximo de read/writes enlazados
#if defined(DEBUG_CPU_OPCODES) || defined(DEBUG_CPU)
#define REC_MAX_RWLOOP 0
#else
#define REC_MAX_RWLOOP 32
#endif
// CHUI: El maximo de offset entre read/writes enlazados
#define REC_MAX_RWLOOP_SIZE 0x10000
// CHUI: El maximo de opcodes sin actualizar los ciclos
#define REC_MAX_UPDATE_CYCLES 12
// CHUI: El maximo de registros a precachear en la fase previa
#define REC_MAX_2ND_REG_CACHE 6
// CHUI: El maximo de veces que un bloque no enlaza con el siguiente
#define REC_MAX_2ND_NOLINK 1024
// CHUI: Longitud maxima para forzar la tabla de immediatos
#define REC_MAX_IMMEDIATE_LONG 1024
// CHUI: Longitud de la tabla de UpdateRegs (consume mucha memoria)
#define REC_SIZE_TABLE_REGS_FLUSH (12*1024)
// CHUI: Usar el interpreter para el DelayTest
#if defined(DEBUG_CPU_OPCODES) || defined(DEBUG_CPU)
#define REC_FORCE_DELAY_READ
#endif

// CHUI: Definido si usamos R2 como cacheo de io_cycle
#define REC_USE_R2
// CHUI: Definido si usamos R3 como otro registro para mapear
#define REC_USE_R3
// CHUI: Definido si no se usa nunca psxMemRLUT/psxMemWLUT
#define REC_USE_DIRECT_MEM
// CHUI: Definido si metemos en linea las funciones GTE gtecalc
#define REC_USE_GTECALC_INLINE
// CHUI: Definido si usamos funciones para el retorno
#define REC_USE_RETURN_FUNCS
// CHUI: Definido si usamos funciones para lectura/escritura de memoria
#define REC_USE_MEMORY_FUNCS
// CHUI: Definido si usamos funciones para algunos opcodes GTE
#define REC_USE_GTE_FUNCS
// CHUI: Definido para usar calculo retardado en el GTE
#define REC_USE_GTE_DELAY_CALC
// CHUI: Definido para ejecuccion de modo bloque rapido
#define REC_USE_FAST_BLOCK
// CHUI: Definido para ejecuccion de modo secure rapido
#define REC_USE_FAST_SECURE
// CHUI: Definido para ejecuccion de modo BIOS rapido
#define REC_USE_FAST_BIOS
// CHUI: Definido para recompilación en dos fases mas lincando.
#define REC_USE_2ND_PHASE
// CHUI: Definido para preservar constantes y no hacer siempre el salto entre-bloque
#define REC_USE_2ND_PRESERVE_CONST
// CHUI: Definido para usar precarga de registros
#define REC_USE_2ND_REG_CACHE
// CHUI: Definido para usar tablas de UpdateRegs
//#define REC_USE_TABLE_REGS_FLUSH
// CHUI: Definido para detectar cuando escribe y lee lo mismo en dos opcodes consecutivos
//#define REC_USE_NO_REPEAT_READ_WRITE
// CHUI: Definido para usar funciones GTE en ASM que reemplazan a las de C++
#define REC_USE_GTE_ASM_FUNCS
// CHUI: Definido para mapear registros GTE sobre la CPU
#define REC_USE_GTE_MAP_REGS
// CHUI: Definido para poner en linea de 32 bytes
#define REC_USE_ALIGN

#ifdef gte_none
#ifndef REC_USE_GTE_ASM_FUNCS
#error NEED A GTE !
#else
#define gte_new
#endif
#endif

int rec_secure_writes=0;
#ifndef __SYMBIAN32__
static char recMem[RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000] __attribute__ ((__aligned__ (32)));	/* the recompiled blocks will be here */
#else
extern char *recMem;//[RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000] __attribute__ ((__aligned__ (32)));	/* the recompiled blocks will be here */
#endif
static char recRAM[0x200000] __attribute__ ((__aligned__ (32)));	/* and the ptr to the blocks here */
static char recROM[0x080000] __attribute__ ((__aligned__ (32)));	/* and here */

static u32 pc;			/* recompiler pc */
static u32 pcold;		/* recompiler oldpc */
// CHUI: Tenemos que guardar el pc inicial del bloque de compilacion para saber si es posible el loop
static u32 pcinit;		/* recompiler initial pc */
static int rec_inloop;	/* unroll-loops */
static int rec_skips;	/* skip psxBranchTest */
static int rec_opcodes=0;	/* opcodes compiled */
static int rec_total_opcodes=0;	/* total opcodes compiled */
static int branch;		/* set for branch */
static u32 target;		/* branch target */
static u32 block=0;

#if defined(interpreter_new) || defined(interpreter_none)
void _psxDelayTest(u32 *regs, u32 code, u32 bpc);
#endif

typedef struct {
	u32 state;
	u32 k;
	u32 reg;
	u32 regw;
} iRegisters;

static iRegisters iRegs[34] __attribute__ ((__aligned__ (32)));
static iRegisters iRegsS[34] __attribute__ ((__aligned__ (32)));
static iRegisters HWRegs[16] __attribute__ ((__aligned__ (32)));
static iRegisters HWRegsS[16] __attribute__ ((__aligned__ (32)));

#define ST_UNK    0
#define ST_CONST  1
#define ST_MAPPED 2
#define ST_HOLD   4

#define IsConst(reg)  (iRegs[reg].state == ST_CONST)
#define IsMapped(reg) (iRegs[reg].state == ST_MAPPED)

#ifdef REC_USE_2ND_PHASE
typedef struct {
	u32 pc;
	u32 *ptr;
	u32 prev;
	iRegisters regs[34];
} iPrevRecJump;

typedef struct {
	u32 pc;
	u32 *ptr;
	iPrevRecJump *jump;
	iRegisters regs[34];
	u8 jumped;
#ifndef REC_USE_2ND_PRESERVE_CONST
	u8 clear_const[34];
#else
	u64 consts;
#endif
#ifdef DEBUG_CPU
	u16 code;
#endif
} iPrevRec;

#ifdef REC_USE_2ND_PRESERVE_CONST
u64 actual_consts=1ULL;
#endif

static int recPrev_jumps=0;
static iPrevRec recPrev[REC_MAX_OPCODES+32] __attribute__ ((__aligned__ (32)));;
static iPrevRecJump recPrevJump[REC_MAX_OPCODES+32] __attribute__ ((__aligned__ (32)));;
static iRegisters recPrev_total[34] __attribute__ ((__aligned__ (32)));;
static iRegisters recPrev_totalS[34] __attribute__ ((__aligned__ (32)));;
static int rec_phase=1;
static int rec_total_opcodes_prev=0;	/* total opcodes compiled in previous phase*/
#else
#define rec_phase 1
#endif

extern void (*recBSC[64])();
extern void (*recSPC[64])();
extern void (*recREG[32])();
extern void (*recCP0[32])();
extern void (*recCP2[64])();
extern void (*recCP2BSC[32])();

#include "arm.h"

#ifdef REC_USE_2ND_PHASE
#ifdef DEBUG_CPU
#define recOpcodePrev() { \
	recPrev[rec_total_opcodes+rec_opcodes].pc=pc-4; \
	recPrev[rec_total_opcodes+rec_opcodes].code=psxRegs.code>>16; \
	dbgf("\trecPrev[%i].pc=%p\n",rec_total_opcodes+rec_opcodes,pc-4); \
}
#define recOpcodeNoPrev() { \
	if (recPrev[rec_total_opcodes+rec_opcodes].pc!=pc-4) { \
		printf("REC-2ND ERROR recPrev[%i].pc=%p != %p\n",rec_total_opcodes+rec_opcodes,recPrev[rec_total_opcodes+rec_opcodes].pc,pc-4); \
		pcsx4all_exit(); \
	} \
	if (recPrev[rec_total_opcodes+rec_opcodes].code!=psxRegs.code>>16) { \
		printf("REC-2ND ERROR recPrev[%i].code=%p != %p\n",rec_total_opcodes+rec_opcodes,recPrev[rec_total_opcodes+rec_opcodes].code,psxRegs.code>>16); \
		pcsx4all_exit(); \
	} \
}
#else
#define recOpcodePrev() { \
	recPrev[rec_total_opcodes+rec_opcodes].pc=pc-4; \
}
#define recOpcodeNoPrev() { \
/*	recPrev[rec_total_opcodes+rec_opcodes].ptr=armPtr; */ \
}
#endif
#else
#define recOpcodePrev()
#define recOpcodeNoPrev()
#endif


#if defined(DEBUG_CPU_OPCODES)
#ifdef DEBUG_CPU
static unsigned fallo_registro=0;
static unsigned ok_registro=0;
#define recOpcode() { \
	if (rec_phase) { \
		write32(0xe92d001f); /* stmdb	sp!, {r0-r4} */ \
		if (!r2_is_dirty) { \
			MOV32RtoR(HOST_r0,HOST_r2); \
			CALLFunc((u32)test_r2_iocycles); \
		} \
		MOV32ItoR(HOST_r3,block); \
		MOV32ItoR(HOST_r2,(((pc - pcold) / 4) * BIAS)); \
		MOV32ItoR(HOST_r1,psxRegs.code); \
		MOV32ItoR(HOST_r0,pc); \
		CALLFunc((u32)dbg_opcode); \
		unsigned _ptr_opcode=(unsigned)armPtr; \
		unsigned _old_pc=pc-4; \
		write32(0xe8bd001f); /* ldmia	sp!, {r0-r4} */ \
		recOpcodeNoPrev(); \
		rec_opcodes++; \
		recBSC[psxRegs.code >> 26](); \
		dbgf("\t\t%i bytes (PC=%p)\n",((unsigned)armPtr)-_ptr_opcode,_old_pc); \
	} else { \
		recOpcodePrev(); \
		rec_opcodes++; \
		recBSC[psxRegs.code >> 26](); \
	} \
}
#else
static psxRegisters merde;
static void copia(void) { memcpy((void *)&merde,(void *)&psxRegs,sizeof(psxRegisters)); }
static void restaura(void) { memcpy((void *)&psxRegs,(void *)&merde,sizeof(psxRegisters)); }
#define recOpcode() { \
	if (rec_phase) { \
		write32(0xe92d001f); /* stmdb	sp!, {r0-r4} */ \
		/* if (!r2_is_dirty) { */ \
			/*write32(0xe92d001f); */ \
			/*MOV32RtoR(HOST_r0,HOST_r2);*/ \
			/*CALLFunc((u32)test_r2_iocycles);*/  \
			/*write32(0xe8bd001f);*/ \
		/* } */ \
/*write32(0xe92d001f);MOV32ItoR(HOST_r0,(u32)"CHK");MOV32ItoR(HOST_r1,(u32)&psxM[0]);MOV32ItoR(HOST_r2,0x200000);CALLFunc((u32)dbgsum);write32(0xe8bd001f);*/ \
/*if (pc>=0x80083000 && pc<=0x80083100) { write32(0xe92d001f);CALLFunc((u32)copia);write32(0xe8bd001f); iUpdateRegs(0); CALLFunc((u32)dbgpsxregs); CALLFunc((u32)restaura); }*/ \
		MOV32ItoR(HOST_r3,block); \
		MOV32ItoR(HOST_r2,(((pc - pcold) / 4) * BIAS)); \
		MOV32ItoR(HOST_r1,psxRegs.code); \
		MOV32ItoR(HOST_r0,pc); \
		CALLFunc((u32)dbg_opcode); \
		write32(0xe8bd001f); /* ldmia	sp!, {r0-r4} */ \
		recOpcodeNoPrev(); \
	} else { \
		recOpcodePrev(); \
	} \
	rec_opcodes++; \
	recBSC[psxRegs.code >> 26](); \
}
#endif
#else
#ifdef DEBUG_CPU
unsigned fallo_registro=0;
unsigned ok_registro=0;
#define recOpcode() { \
	if (rec_phase) { \
		unsigned _ptr_opcode=(unsigned)armPtr; \
		unsigned _old_pc=pc-4; \
		recOpcodeNoPrev(); \
		rec_opcodes++; \
		recBSC[psxRegs.code >> 26](); \
		dbgf("\t\t%i bytes (PC=%p)\n",((unsigned)armPtr)-_ptr_opcode,_old_pc); \
	} else { \
		recOpcodePrev(); \
		rec_opcodes++; \
		recBSC[psxRegs.code >> 26](); \
	} \
}
#else
#define recOpcode() { \
	if (!rec_phase) { \
		recOpcodePrev(); \
	} else { \
		recOpcodeNoPrev(); \
	} \
	rec_opcodes++; \
	recBSC[psxRegs.code >> 26](); \
}
#endif
#endif


static int r2_is_dirty=1;
static int has_been_written=0;
static void recClear(u32 Addr, u32 Size);
static void recClearDouble(u32 Addr1, u32 Addr2);
static u32 cycles_pending=0;

static void iPutCyclesAdd(int forced) {
	if (!rec_phase)
		return;
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
	if (forced) {
		ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS)); // Actualizamos los ciclos
		MOV32ItoM_regs((u32)&psxRegs.cycle_add,0); // Establecemos los ciclos a añadir
		pcold=pc;
		cycles_pending=0;
	} else {
		MOV32ItoM_regs((u32)&psxRegs.cycle_add,(((pc - pcold) / 4) * BIAS)); // Establecemos los ciclos a añadir
	}
#else
	if (forced || (((pc-pcold)/4)>=REC_MAX_UPDATE_CYCLES)) {
		ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS)); // Actualizamos los ciclos
		pcold=pc;
		cycles_pending=0;
	}
#endif
}

#ifdef DEBUG_CPU_OPCODES
// CHUI: En modo depuración testeamos el cacheo de io_cycles en r2
static void test_r2_iocycles(unsigned r2) {
	if (r2!=psxRegs.io_cycle_counter) {
		dbgf("R2=%i != io_cycles=%i\n",r2,psxRegs.io_cycle_counter);
	}
}
#endif

static void UnmapGteReg(u32 cpu_reg);

static void MapConst(u32 reg, u32 _const) {
#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
		recPrev_total[reg].state = ST_CONST;
		iRegs[reg].state = ST_CONST;
		iRegs[reg].k = _const;
#ifdef REC_USE_2ND_PRESERVE_CONST
		actual_consts|=(1ULL<<reg);
#endif
		return;
	}
#endif
#ifdef DEBUG_CPU
			dbgf("\t\t\t\tMapConst r%i=%p",reg,_const);
#endif
	UnmapGteReg(reg);
	if (IsMapped(reg)) {
#ifdef DEBUG_CPU
		dbgf(" (!R%i)",iRegs[reg].reg);
#endif
		HWRegs[iRegs[reg].reg].state=ST_UNK;
	}
	iRegs[reg].k = _const;
	iRegs[reg].state = ST_CONST;
	iRegs[reg].regw = 1;
#ifdef DEBUG_CPU
	dbg("");
#endif
}

static void Unmap(u32 reg) {
	if (!rec_phase) {
		iRegs[reg].state = ST_UNK;
#ifdef REC_USE_2ND_PRESERVE_CONST
		actual_consts&=~(1ULL<<reg);
#endif
		return;
	}
#ifdef DEBUG_CPU
	dbgf("\t\t\t\tUnmap r%i",reg);
#endif
	if (IsMapped(reg)) {
#ifdef DEBUG_CPU
		dbgf(" (!R%i)",iRegs[reg].reg);
#endif
		HWRegs[iRegs[reg].reg].state=ST_UNK;
	}
	iRegs[reg].state = ST_UNK;
#ifdef DEBUG_CPU
	dbg("");
#endif
}

static void iFlushReg(u32 reg) {
	if (!rec_phase) {
		iRegs[reg].state = ST_UNK;
		return;
	}
	if ((IsConst(reg)) && iRegs[reg].regw) {
#ifdef DEBUG_CPU
		dbgf("\t\t\t\tr%i=%p\n",reg,iRegs[reg].k);
#endif
		MOV32ItoM_regs((u32)&psxRegs.GPR.r[reg], iRegs[reg].k);
	} else if (IsMapped(reg)) {
		if (iRegs[reg].regw)
		{
#ifdef DEBUG_CPU
			dbgf("\t\t\t\tr%i=R%i\n",reg,iRegs[reg].reg);
#endif
			MOV32RtoM_regs((u32)&psxRegs.GPR.r[reg], iRegs[reg].reg);
		}
		HWRegs[iRegs[reg].reg].state=ST_UNK;
	}
	iRegs[reg].state = ST_UNK;
}

static void ResetMapGteRegs(void);

#ifdef REC_USE_2ND_PHASE
static void iClearRegsPrev(void) {
	recPrev_jumps=0;
	memset((void *)iRegs, 0, sizeof(iRegs));
	memset((void *)recPrev,0,sizeof(recPrev));
	memset((void *)recPrev_total, 0, sizeof(recPrev_total));
	iRegs[0].state = recPrev_total[0].state = ST_CONST;
#ifdef REC_USE_2ND_PRESERVE_CONST
	actual_consts=1ULL;
#endif
}


static void iResetRegw(void) {
	int i;
	for(i=1;i<34;i++)
		iRegs[i].regw=1;
}
#endif

// CHUI: iClearRegs limpia iRegs/HWRegs
// CHUI: En la fase previa bloquea todo los registros
static void iClearRegs() {
#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
		int i;
		for(i=0;i<34;i++) {
			recPrev_total[i].state=ST_CONST;
			iRegs[i].state = ST_UNK;
		}
#ifdef REC_USE_2ND_PRESERVE_CONST
		actual_consts=1ULL;
#endif
		iRegs[0].state = ST_CONST;
		iRegs[0].k = 0;
		return;
	}
#endif
#ifdef DEBUG_CPU
	dbg("\t\t\tiClearRegs");
#endif
	memset(iRegs, 0, sizeof(iRegs));
	iRegs[0].state = ST_CONST;
	iRegs[0].k     = 0;
	HWRegs[0].state = ST_CONST; /* do not map r0 */
	HWRegs[1].state = ST_CONST; /* do not map r1 */
	HWRegs[2].state = ST_CONST; /* do not map r2 */
#ifndef REC_USE_R3
	HWRegs[3].state = ST_CONST; /* do not map r3 */
#else
	HWRegs[3].state = ST_UNK;
#endif
	HWRegs[4].state = ST_UNK;
	HWRegs[5].state = ST_UNK;
	HWRegs[6].state = ST_UNK;
	HWRegs[7].state = ST_UNK;
	HWRegs[8].state = ST_UNK;
	HWRegs[9].state = ST_UNK;
	HWRegs[10].state = ST_UNK;
	HWRegs[11].state = ST_CONST; /* do not map r11 (v8) - psxRegs pointer */
	HWRegs[12].state = ST_CONST; /* do not map r12 (ip) */
	HWRegs[13].state = ST_CONST; /* do not map r13 (sp) */
	HWRegs[14].state = ST_CONST; /* do not map r14 (lr) */
	HWRegs[15].state = ST_CONST; /* do not map r15 (pc) */
	ResetMapGteRegs();
}

static void iFlushRegs() {
	int i;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	unsigned _ptr_opcode_=(unsigned)armPtr;
	dbg("\t\t\tiFlushRegs");
#endif
	if (IsMapped(0)) Unmap(0);
	iRegs[0].k = 0; iRegs[0].state = ST_CONST;
	for (i=1; i<34; i++) {
		iFlushReg(i);
	}
#ifdef DEBUG_CPU
	dbgf("\t\t\t!iFlushRegs %i\n",((unsigned)armPtr)-_ptr_opcode_);
#endif
}

// CHUI: iUpdateRegs actualiza psxRegs con regs sin perder contexto
static void _iUpdateRegs(int clear) {
	int i;
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	unsigned _ptr_opcode_=(unsigned)armPtr;
	dbg("\t\t\tiUpdateRegs");
#endif
	for (i=1; i<34; i++) {
#ifdef DEBUG_CPU_OPCODES
		if (IsConst(i)) {
#else
		if ((IsConst(i)) && (iRegs[i].regw)) {
#endif
#ifdef DEBUG_CPU
			dbgf("\t\t\t\tr%i=%p\n",i,iRegs[i].k);
#endif
			MOV32ItoM_regs((u32)&psxRegs.GPR.r[i], iRegs[i].k);
			if (clear)
				iRegs[i].regw=0;
#ifdef DEBUG_CPU_OPCODES
		} else if (IsMapped(i)) {
#else
		} else if ((IsMapped(i)) && (iRegs[i].regw)) {
#endif
#ifdef DEBUG_CPU
			dbgf("\t\t\t\tr%i=R%i\n",i,iRegs[i].reg);
#endif
			MOV32RtoM_regs((u32)&psxRegs.GPR.r[i], iRegs[i].reg);
			if (clear)
				iRegs[i].regw=0;
		}
	}
#ifdef DEBUG_CPU
	dbgf("\t\t\t!iUpdateRegs %i\n",((unsigned)armPtr)-_ptr_opcode_);
#endif
}

#ifndef REC_USE_TABLE_REGS_FLUSH
#define iUpdateRegs(CLEAR) _iUpdateRegs(CLEAR)
#define iResetTableRegs()
#else
typedef struct {
	iRegisters regs[34];
	u32 ptr;
	u32 search_pattern;
	void *next;
} regs_flush_t;

static regs_flush_t table_regs_flush[REC_SIZE_TABLE_REGS_FLUSH];
static regs_flush_t *table_regs_flush_search[0x20000];
static u32 table_regs_flush_index=0;
static u32 table_regs_flush_last=0;

static void iResetTableRegs(void) {
	memset(table_regs_flush,0,sizeof(regs_flush_t));
	memset(table_regs_flush_search,0,sizeof(table_regs_flush_search));
	table_regs_flush_index=0;
}

static void iUpdateRegs(int clear) {
	int i;
	u32 pat=0;
	if (!rec_phase) {
		return;
	}
	for(i=1;i<34;i++) {
		if (iRegs[i].state==ST_MAPPED)
			pat|=(1<<(iRegs[i].reg-3));
		if (i<25 && (iRegs[i].state&3))
			pat|=(1<<(7+i));
	}
	regs_flush_t *entry=table_regs_flush_search[pat&0x1FFFF];
	regs_flush_t *back=NULL;
	while (entry) {
		if (entry->search_pattern==pat) {
			int encontrado=1;
			for(i=1;i<34;i++)
				if (((iRegs[i].state&3)!=(entry->regs[i].state&3))||(iRegs[i].state==ST_MAPPED && (iRegs[i].reg!=entry->regs[i].reg || iRegs[i].regw!=entry->regs[i].regw))||(iRegs[i].state==ST_CONST && (iRegs[i].k!=entry->regs[i].k || iRegs[i].regw!=entry->regs[i].regw))) {
						encontrado=0;
						break;
					}
			if (encontrado)
				break;
		}
		back=entry;
		entry=(regs_flush_t *)entry->next;
	}
	if (!entry) {
		if (table_regs_flush_index>=(sizeof(table_regs_flush)/sizeof(regs_flush_t))) {
			_iUpdateRegs(clear);
		} else {
			if (!back) {
				entry=table_regs_flush_search[pat&0x1FFFF]=&table_regs_flush[table_regs_flush_index++];
			} else {
				entry=&table_regs_flush[table_regs_flush_index++];
				back->next=(void *)entry;
			}
			u32 *ptr=armPtr;
			write32(ADD_IMM(HOST_lr,HOST_pc,0,0));
			_iUpdateRegs(clear);
			write32(BX_LR());
			*ptr|=((u32)armPtr)-((u32)ptr)-8;
			entry->ptr=table_regs_flush_last=((u32)ptr)+4;
			entry->search_pattern=pat;
			memcpy(entry->regs,iRegs,sizeof(iRegs));
		}
	} else {
		CALLFunc(entry->ptr);
		table_regs_flush_last=entry->ptr;
		if (clear){
			for(i=1;i<34;i++)
				iRegs[i].regw=0;
		}
	}
}
#endif

static u32 GetReg(u32 reg){
	int i,j=0;
	u32 k=0;
	for (i=15;i>=0;i--)
	{
		if (HWRegs[i].state==ST_UNK)
		{
			HWRegs[i].state=ST_MAPPED;
			HWRegs[i].k=0;
			HWRegs[i].reg=reg;
			j=i;
			for (i=0;i<16;i++) HWRegs[i].k++;
			return j;
		}
		if ((HWRegs[i].state==ST_MAPPED) && (HWRegs[i].k>k))
		{
			j=i;
			k=HWRegs[i].k;
		}
	}
	UnmapGteReg(HWRegs[j].reg);
	iFlushReg(HWRegs[j].reg);
	HWRegs[j].state=ST_MAPPED;
	HWRegs[j].k=0;
	HWRegs[j].reg=reg;
	for (i=0;i<16;i++) HWRegs[i].k++;
	return j;
}

static void iLockReg(u32 reg){
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\t\tiLockReg(%i)\n",reg);
	}
#endif
	if (HWRegs[reg].state==ST_MAPPED) {
		int i,j=0;
		u32 k=0;
		for (i=0;i<16;i++)
		{
			if (HWRegs[i].state==ST_UNK)
			{
#ifdef DEBUG_CPU
				if (rec_phase) {
					dbgf("\t\t\t\t-Use empty r%i\n",i);
				}
#endif
				MOV32RtoR(i,reg);
				iRegs[HWRegs[reg].reg].reg=i;
				HWRegs[i].k=HWRegs[reg].k;
				HWRegs[i].reg=HWRegs[reg].reg;
				HWRegs[i].state=ST_MAPPED;
				HWRegs[reg].state=ST_HOLD;
#ifdef DEBUG_CPU
				if (rec_phase) {
					dbg("\t\t\t!iLockReg");
				}
#endif
				return;
			}
			if ((HWRegs[i].state==ST_MAPPED) && (HWRegs[i].k>k))
			{
				j=i;
				k=HWRegs[i].k;
			}
		}
		if ((j==reg)||(HWRegs[reg].k>k)) {
			iFlushReg(HWRegs[reg].reg);
#ifdef DEBUG_CPU
			if (rec_phase) {
				dbg("\t\t\t\t-Discard");
			}
#endif
		} else {
			iFlushReg(HWRegs[j].reg);
#ifdef DEBUG_CPU
			if (rec_phase) {
				dbgf("\t\t\t\t-Flush %i\n",j);
			}
#endif
			MOV32RtoR(j,reg);
			iRegs[HWRegs[reg].reg].reg=j;
			HWRegs[j].k=HWRegs[reg].k;
			HWRegs[j].reg=HWRegs[reg].reg;
			HWRegs[j].state=ST_MAPPED;
		}
		HWRegs[reg].state=ST_HOLD;
	}
	else {
		if (HWRegs[reg].state!=ST_CONST)
			HWRegs[reg].state=ST_HOLD;
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t\t\t\t-Not mapped");
		}
#endif
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\t\t!iLockReg");
	}
#endif
}

static void iUnlockReg(u32 reg) {
	if (!rec_phase) {
		return;
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\t\tiUnlockReg(%i) %s\n",reg,HWRegs[reg].state==ST_HOLD?"HOLD":HWRegs[reg].state==ST_UNK?"EMPTY":"USED!");
	}
#endif
	if (HWRegs[reg].state==ST_HOLD) {
		HWRegs[reg].state=ST_UNK;
	}
}

static u32 TempReg(void) {
	if (!rec_phase) {
		return 0;
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t\t\tTempReg");
	}
#endif
	int i,j=0;
	u32 k=0;
	for (i=0;i<16;i++)
	{
		if (HWRegs[i].state==ST_UNK)
		{
			HWRegs[i].state=ST_CONST;
#ifdef DEBUG_CPU
			if (rec_phase) {
				dbgf("\t\t\t!TempReg R%i\n",i);
			}
#endif
			return i;
		}
		if ((HWRegs[i].state==ST_MAPPED) && (HWRegs[i].k>k))
		{
			j=i;
			k=HWRegs[i].k;
		}
	}
	iFlushReg(HWRegs[j].reg);
	HWRegs[j].state=ST_CONST;
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\t\t\t!TempReg R%i\n",j);
	}
#endif
	return j;
}

static void ReleaseTempReg(u32 reg) {
	if (!rec_phase) {
		return;
	}
	HWRegs[reg].state=ST_UNK;
}

static u32 ReadReg(u32 reg) {
#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
		if (recPrev_total[reg].state!=ST_CONST) {
			recPrev_total[reg].state=ST_MAPPED;
			recPrev_total[reg].k++;
		}
#ifdef REC_USE_2ND_PRESERVE_CONST
		actual_consts&=~(1ULL<<reg);
#endif
		iRegs[reg].state=ST_MAPPED;
		return 0;
	}
#endif
#ifdef DEBUG_CPU
	dbgf("\t\t\tReadReg r%i\n",reg);
#endif
	if (iRegs[reg].state!=ST_MAPPED)
	{
#ifdef DEBUG_CPU
		fallo_registro++;
#endif
		iRegs[reg].reg=GetReg(reg);
		iRegs[reg].state=ST_MAPPED;
		MOV32MtoR_regs(iRegs[reg].reg,&psxRegs.GPR.r[reg]);
		iRegs[reg].regw=0; /* not written */
	}
	else
	{
		int i;
#ifdef DEBUG_CPU
		ok_registro++;
#endif
		HWRegs[iRegs[reg].reg].k=0; /* no flush */
		for (i=0;i<16;i++) HWRegs[i].k++;
	}
#ifdef DEBUG_CPU
	dbgf("\t\t\t!ReadReg R%i\n",iRegs[reg].reg);
#endif
	return iRegs[reg].reg;
}

static u32 WriteReg(u32 reg) {
#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
		recPrev_total[reg].state=ST_CONST;
		iRegs[reg].state=ST_MAPPED;
#ifdef REC_USE_2ND_PRESERVE_CONST
		actual_consts&=~(1ULL<<reg);
#endif
		return 0;
	}
#endif
#ifdef DEBUG_CPU
	dbgf("\t\t\tWriteReg r%i\n",reg);
#endif
	UnmapGteReg(reg);
	if (iRegs[reg].state!=ST_MAPPED)
	{
#ifdef DEBUG_CPU
		fallo_registro++;
#endif
		iRegs[reg].reg=GetReg(reg);
		iRegs[reg].state=ST_MAPPED;
		iRegs[reg].regw=1; /* written */
	}
	else
	{
		int i;
#ifdef DEBUG_CPU
		ok_registro++;
#endif
		iRegs[reg].regw=1; /* written */
		HWRegs[iRegs[reg].reg].k=0; /* no flush */
		for (i=0;i<16;i++) HWRegs[i].k++;
	}
#ifdef DEBUG_CPU
	dbgf("\t\t\t!WriteReg R%i\n",iRegs[reg].reg);
#endif
	return iRegs[reg].reg;
}

#ifndef REC_USE_2ND_PRESERVE_CONST
static void MapToWrite(u32 reg) {
	if (iRegs[reg].state==ST_CONST) {
			u32 r, value=iRegs[reg].k;
			Unmap(reg);
			r=WriteReg(reg);
			if (rec_phase) {
				MOV32ItoR(r,value);
			}
	}
}
#endif

static u32 ReadWriteReg(u32 reg) {
#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
		if (recPrev_total[reg].state!=ST_CONST) {
			recPrev_total[reg].state=ST_CONST;
			recPrev_total[reg].k++;
		}
#ifdef REC_USE_2ND_PRESERVE_CONST
		actual_consts&=~(1ULL<<reg);
#endif
		iRegs[reg].state=ST_MAPPED;
		return 0;
	}
#endif
#ifdef DEBUG_CPU
	dbgf("\t\t\tReadWriteReg r%i\n",reg);
#endif
	UnmapGteReg(reg);
	if (iRegs[reg].state!=ST_MAPPED)
	{
#ifdef DEBUG_CPU
		fallo_registro++;
#endif
		iRegs[reg].reg=GetReg(reg);
		iRegs[reg].state=ST_MAPPED;
		MOV32MtoR_regs(iRegs[reg].reg,&psxRegs.GPR.r[reg]);
		iRegs[reg].regw=1; /* written */
	}
	else
	{
		int i;
#ifdef DEBUG_CPU
		ok_registro++;
#endif
		iRegs[reg].regw=1; /* written */
		HWRegs[iRegs[reg].reg].k=0; /* no flush */
		for (i=0;i<16;i++) HWRegs[i].k++;
	}
#ifdef DEBUG_CPU
	dbgf("\t\t\t!ReadWriteReg R%i\n",iRegs[reg].reg);
#endif
	return iRegs[reg].reg;
}

#if defined(REC_USE_2ND_PHASE) && defined(REC_USE_2ND_REG_CACHE)
static u32 FindCache(u32 rec_cache, int use) {
	u32 i, ret=0;
	for(i=0;i<32;i++) {
		if ((rec_cache>>i)&1) {
			if (!use)
				ret++;
			else 
				if (iRegs[i].state!=ST_UNK)
					ret++;
		}
	}
	return ret;
}

static void ConvertCache(u32 rec_cache) {
	u32 i, j, reg=10;
	iRegisters hw[16];
	iRegisters regs[34];
	memcpy(hw,HWRegs,sizeof(hw));
	memcpy(regs,iRegs,sizeof(regs));
#ifdef DEBUG_CPU
	dbgf("\t\tConvertCache ");
#endif
	for(i=0;i<32;i++) {
		if (!((rec_cache>>i)&1)) {
			if (regs[i].state==ST_MAPPED) {
				regs[i].state=ST_UNK;
				hw[regs[i].reg].state=ST_UNK;
			}
		}
#ifdef DEBUG_CPU
		else {
			dbgf("R%i ",i);
		}
#endif
	}
	if (regs[32].state==ST_MAPPED) {
		regs[32].state=ST_UNK;
		hw[regs[32].reg].state=ST_UNK;
	}
	if (regs[33].state==ST_MAPPED) {
		regs[33].state=ST_UNK;
		hw[regs[33].reg].state=ST_UNK;
	}
#ifdef DEBUG_CPU
	dbgf(". Free ");
	for(i=0;i<15;i++)
		if (hw[i].state==ST_UNK)
			dbgf("r%i ",i);
	dbg(".");
#endif
	for(i=0;i<32;i++) {
		if ((rec_cache>>i)&1) {
#ifdef DEBUG_CPU
			dbgf("\t\t\tR%i",i);
#endif
			switch(regs[i].state) {
				case ST_UNK:
					if (hw[reg].state==ST_MAPPED) {
#ifdef DEBUG_CPU
						dbgf(" UNK MAPPED: ");
#endif
						for(j=0;j<16;j++) {
							if (hw[j].state==ST_UNK) {
								u32 nreg=hw[reg].reg;
								regs[nreg].reg=j;
								hw[j].reg=nreg;
								hw[j].state=ST_MAPPED;
#ifdef DEBUG_CPU
								dbgf("MOV R%i, R%i and ",j,reg);
#endif
								MOV32RtoR(j,reg);
								break;
							}
						}
					}
#ifdef DEBUG_CPU
					else {
						dbgf(" UNK: ");
					}
					dbgf("MOV R%i, #R%i\n",reg,i);
#endif
					MOV32MtoR_regs(reg,&psxRegs.GPR.r[i]);
					break;
				case ST_CONST:
					if (hw[reg].state==ST_MAPPED) {
#ifdef DEBUG_CPU
						dbgf(" CONST MAPPED: ");
#endif
						for(j=0;j<16;j++) {
							if (hw[j].state==ST_UNK) {
								u32 nreg=hw[reg].reg;
								regs[nreg].reg=j;
								hw[j].reg=nreg;
								hw[j].state=ST_MAPPED;
#ifdef DEBUG_CPU
								dbgf("MOV R%i, R%i and ",j,reg);
#endif
								MOV32RtoR(j,reg);
								break;
							}
						}					
					}
#ifdef DEBUG_CPU
					else {
						dbgf(" CONST: ");
					}
					dbgf("MOV R%i, #%p\n",reg,regs[i].k);
#endif
					MOV32ItoR(reg,regs[i].k);
					break;
				case ST_MAPPED:
					if (regs[i].reg==reg) {
#ifdef DEBUG_CPU
						dbg(" MAPPED OK");
#endif
						break;
					}
					if (hw[reg].state==ST_MAPPED) {
#ifdef DEBUG_CPU
						dbgf(" MAPPED MAPPED: ");
#endif
						for(j=0;j<16;j++) {
							if (hw[j].state==ST_UNK) {
								u32 nreg=hw[reg].reg;
								regs[nreg].reg=j;
								hw[j].reg=nreg;
								hw[j].state=ST_MAPPED;
#ifdef DEBUG_CPU
								dbgf("MOV R%i, R%i and ",j,reg);
#endif
								MOV32RtoR(j,reg);
								break;
							}
						}
					}
#ifdef DEBUG_CPU
					else {
						dbgf(" MAPPED: ");
					}
					dbgf("MOV R%i, R%i\n",reg,regs[i].reg);
#endif
					MOV32RtoR(reg,regs[i].reg);
					regs[i].state=ST_UNK;
					hw[regs[i].reg].state=ST_UNK;
					break;
			}
			hw[reg].state=ST_CONST;
			reg--;
		}
	}
}
#endif

static int FindJump(iRegisters *src, iRegisters *dst) {
	int i;
	for(i=0;i<34;i++)
		if (dst[i].state==ST_CONST && (src[i].state!=ST_CONST || (src[i].state==ST_CONST && dst[i].k!=src[i].k)))
			return 0;
	return 1;
}

static void ConvertJump(iRegisters *src_, iRegisters *dst) {
	u32 i, j, k, l;
#ifdef DEBUG_CPU
	dbgf("\t\tConvertJump ");
#endif
	iRegisters hw[16];
	iRegisters src[34];
	memcpy(src,src_,sizeof(src));
	for(i=0;i<16;i++)
		hw[i].state=ST_UNK;
	hw[0].state=hw[1].state=hw[11].state=hw[12].state=hw[13].state=hw[14].state=hw[15].state=ST_CONST;
	for(i=0;i<34;i++)
		if (src[i].state==ST_MAPPED) {
			if (dst[i].state==ST_UNK || dst[i].state==ST_CONST) {
				if (src[i].regw) {
					MOV32RtoM_regs((u32)&psxRegs.GPR.r[i], src[i].reg);
				}
				src[i].state=ST_UNK;
				hw[src[i].reg].state=ST_UNK;
			} else {
				hw[src[i].reg].state=ST_MAPPED;
				hw[src[i].reg].reg=i;
			}
		} else if (src[i].state==ST_CONST && src[i].regw && dst[i].state==ST_UNK) {
			MOV32ItoM_regs((u32)&psxRegs.GPR.r[i], src[i].k);
		}
	for(i=0;i<34;i++) {
		if (dst[i].state==ST_MAPPED) {
			switch (src[i].state) {
				case ST_CONST:
				case ST_UNK:
					for(j=0;j<34;j++)
						if (src[j].state==ST_MAPPED && src[j].reg==dst[i].reg) {
							for(l=0;l<34;l++)
								if (src[l].state==ST_MAPPED && src[l].reg==dst[j].reg) {								
									for(k=0;k<16;k++)
										if (hw[k].state==ST_UNK) {
											MOV32RtoR(k,src[l].reg);
											src[l].reg=k;
											hw[k].reg=l;
											hw[k].state=ST_MAPPED;
											break;
										}
									break;
								}
							MOV32RtoR(dst[j].reg,src[j].reg);
							src[j].state=ST_MAPPED;
							src[j].reg=dst[j].reg;
							hw[src[j].reg].state=ST_MAPPED;
							hw[src[j].reg].reg=j;
							break;
						}
					if (src[i].state==ST_UNK) {
						MOV32MtoR_regs(dst[i].reg,&psxRegs.GPR.r[i]);
					} else {
						MOV32ItoR(dst[i].reg,src[i].k);
					}
					src[i].state=ST_MAPPED;
					src[i].reg=dst[i].reg;
					hw[src[i].reg].state=ST_MAPPED;
					hw[src[i].reg].reg=i;
					break;
				case ST_MAPPED:
					if (src[i].reg!=dst[i].reg) {
						for(j=0;j<34;j++) 
							if (src[j].state==ST_MAPPED && src[j].reg==dst[i].reg) {
								for(l=0;l<34;l++)
									if (src[l].state==ST_MAPPED && src[l].reg==dst[j].reg) {
										for(k=0;k<16;k++)
											if (hw[k].state==ST_UNK) {
												MOV32RtoR(k,src[l].reg);
												src[l].reg=k;
												hw[k].reg=l;
												hw[k].state=ST_MAPPED;
												break;
											}
										break;
									}
								MOV32RtoR(dst[j].reg,src[j].reg);
								src[j].state=ST_MAPPED;
								src[j].reg=dst[j].reg;
								hw[src[j].reg].state=ST_MAPPED;
								hw[src[j].reg].reg=j;
								break;
							}
						MOV32RtoR(dst[i].reg,src[i].reg);
						hw[src[i].reg].state=ST_UNK;
						src[i].reg=dst[i].reg;
						hw[dst[i].reg].reg=i;
						hw[dst[i].reg].state=ST_MAPPED;
					}
					break;
			}
		}
	}
	for(i=0;i<34;i++)
		if (dst[i].state==ST_MAPPED && !dst[i].regw && src[i].regw) {
			MOV32RtoM_regs((u32)&psxRegs.GPR.r[i], dst[i].reg);
		}
}

#if defined(REC_USE_2ND_PHASE) && defined(REC_USE_2ND_REG_CACHE)
static u32 rec2nd_func;

static void rec2nd(void) {
#ifdef DEBUG_CPU
	dbg("-- FUNCTION rec2nd --");
#endif
	write32(REC_MAX_2ND_NOLINK+1); // Maximo de veces que un bloque no puede enlazar con el siguiente
	rec2nd_func=(u32)armPtr;
	write32(LDR_IMM_NEG(HOST_r1,HOST_pc,12+(((u32)armPtr)-rec2nd_func)));
	write32(SUB_IMM(HOST_r1,HOST_r1,1,0));
	write32(STR_IMM_NEG(HOST_r1,HOST_pc,12+(((u32)armPtr)-rec2nd_func)));
	write32(CMP_IMM(HOST_r1,0,0));
	write32(BXNE_LR());
// CHUI: Descartar bloque actual para que vuelva a recompilar
	MOV32ItoR(HOST_r1,4);
	JUMPFunc((u32)recClear);
	UpdateImmediate(1);
}
#endif

static int iLoadTest() {
	u32 tmp;

	// check for load delay
	tmp = psxRegs.code >> 26;
	switch (tmp) {
		case 0x10: // COP0
			switch (_Rs_) {
				case 0x00: // MFC0
				case 0x02: // CFC0
					return 1;
			}
			break;
		case 0x12: // COP2
			switch (_Funct_) {
				case 0x00:
					switch (_Rs_) {
						case 0x00: // MFC2
						case 0x02: // CFC2
							return 1;
					}
					break;
			}
			break;
		case 0x32: // LWC2
			return 1;
		default:
			if (tmp >= 0x20 && tmp <= 0x26) { // LB/LH/LWL/LW/LBU/LHU/LWR
				return 1;
			}
			break;
	}
	return 0;
}

static unsigned func_TestBranchIfOK_ptr=0;

static void recTestBranchIfOK(int with_HOST_r0) {
	if (!with_HOST_r0)
		MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.reserved); 
	write32(BIC_IMM(HOST_lr,HOST_r0,0x7f,0x09)); // bic lr, r0, #0xff000000
	write32(BIC_IMM(HOST_lr,HOST_lr,0x0e,0x0c)); // bic lr, lr, #0xe00000
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM); // ip = psxM
	write32(LDR_REG(HOST_lr,HOST_lr,HOST_ip)); // lr = valor primer opcode
	GET_PTR();
	j32Ptr[4]=JEZ8(HOST_r1);
	write32(LDR_IMM_NEG(HOST_r0,HOST_r1,4)); // r0= primer opcode guardado
	write32(CMP_REGS(HOST_r0,HOST_lr));
	j32Ptr[6]=armPtr; write32(BNE_FWD(0));
	MOV32RtoR(HOST_pc,HOST_r1);
	armSetJ32(j32Ptr[6]);
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
	MOV32ItoR(HOST_r1,0x04);
	CALLFunc((u32)recClear);
	armSetJ32(j32Ptr[4]);
	RET_NC();
}

#ifdef REC_USE_FAST_BLOCK

static unsigned func_TestBranchBlock_ptr=0;

static void recTestBranchBlock(int with_HOST_r0) {
	if (!with_HOST_r0)
		MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
	write32(CMP_REGS(HOST_r1,HOST_r0));
	j32Ptr[1]=armPtr; write32(BEQ_FWD(0));
	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.reserved); 
// CHUI: Es realmente necesario?
#if 0
	write32(BIC_IMM(HOST_lr,HOST_r0,0x7f,0x09)); // bic lr, r0, #0xff000000
	write32(BIC_IMM(HOST_lr,HOST_lr,0x0e,0x0c)); // bic lr, lr, #0xe00000
	MOV32MtoR_regs(HOST_ip,&psxRegs.psxM); // ip = psxM
	write32(LDR_REG(HOST_lr,HOST_lr,HOST_ip)); // lr = valor primer opcode
#endif
	GET_PTR();
	j32Ptr[4]=JEZ8(HOST_r1);
// CHUI: Es realmente necesario?
#if 0
	write32(LDR_IMM_NEG(HOST_r0,HOST_r1,4)); // r0= primer opcode guardado
	write32(CMP_REGS(HOST_r0,HOST_lr));
	j32Ptr[6]=armPtr; write32(BNE_FWD(0));
#endif
	MOV32RtoR(HOST_pc,HOST_r1);
// CHUI: Es realmente necesario?
#if 0
	armSetJ32(j32Ptr[6]);
	MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
	MOV32ItoR(HOST_r1,0x04);
	CALLFunc((u32)recClear);
#endif
	armSetJ32(j32Ptr[4]);
	armSetJ32(j32Ptr[1]);
	RET_NC();
}
#endif

static unsigned func_Return_ptr=0;

static void recReturn(int with_HOST_r0) {
	if (!with_HOST_r0)
		MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
	MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.reserved); 
	GET_PTR();
	j32Ptr[4]=JEZ8(HOST_r1);
	MOV32RtoR(HOST_pc,HOST_r1);
	armSetJ32(j32Ptr[4]);
	RET_NC();
}

static unsigned pcabs(unsigned pca, unsigned pcb) {
	if (pca>pcb){
		return ((pca-pcb)/4);
	}
	return ((pcb-pca)/4);
}

static void UpdateGteDelay(int clear);
static unsigned func_GTE_delay_ptr=0;

/* set a pending branch */
// CHUI: Aproximadamente el 7% de los saltos son saltos a puntero variable.
// CHUI: Como en tiempo de recompilacion no sabemos cual es ese puntero no podemos alargar bloque
static void SetBranch() {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\tSetBranch");
	}
#endif
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc += 4;

#if defined(DEBUG_CPU) || defined(DEBUG_CPU_OPCODES) || defined(REC_FORCE_DELAY_READ)
	if (iLoadTest()) {
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t\t\t\tLoadTest");
		}
#endif	
		iUpdateRegs(1);
// CHUI: Actualiamos previamente PC, code y cycle
		MOV32ItoM_regs((u32)&psxRegs.pc,pc);
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;

#if defined(interpreter_new) || defined(interpreter_none)
		MOV32MtoR(HOST_r2,(u32)&target);
		MOV32ItoR(HOST_r1,(u32)psxRegs.code);
		MOV32ItoR(HOST_r0,(u32)&psxRegs);
		CALLFunc((u32)_psxDelayTest);
#else
		MOV32MtoR(HOST_r1,(u32)&target);
		MOV32ItoR(HOST_r0,_Rt_);
		CALLFunc((u32)psxDelayTest);
#endif

		RET()
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t!SetBranch");
		}
#endif
		return;
	}
#endif
#ifdef REC_USE_2ND_PRESERVE_CONST
	if (!rec_phase) {
		recPrev[rec_total_opcodes+rec_opcodes].consts = actual_consts;
	 }
#endif
	recOpcode()
	
	if (!rec_phase) {
		return;
	}
	
	UpdateGteDelay(1);
	iFlushRegs();
// CHUI: Actualiamos previamente PC
	MOV32MtoR(HOST_r0, (u32)&target);
	MOV32RtoM_regs((u32)&psxRegs.pc, HOST_r0);
#ifndef REC_USE_FAST_BLOCK
	if (!block)
#endif
	{
// CHUI: Actualizamos psxRegs.cycle mientras dejamos el valor actualizado en r1
		MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.cycle); // r1 = psxRegs.cycle
		ADD32ItoR(HOST_r1,cycles_pending+(((pc - pcold) / 4) * BIAS)); // r1 += ciclos
		MOV32RtoM_regs((u32)&psxRegs.cycle,HOST_r1); // psxRegs.cycle = r1
		cycles_pending=0;
// CHUI: Comparamos con io_cycle_counter para saber si hay que hacer psxBranchTest
		if (r2_is_dirty) {
			MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
		}
		write32(CMP_REGS(HOST_r1,HOST_r2)); // cmp r1, r2
// CHUI: Es necesario psxBranchTest
		CALLFuncCS((u32)psxBranchTestCalculate);
	}
#ifndef REC_USE_FAST_BLOCK
	else {
		ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;
		CALLFunc((u32)psxBranchTest);
	}
#endif
	
	if (Config.HLE) {
		if (block){
#ifndef REC_USE_FAST_BLOCK
			RET_NC();
#else
			MOV32ItoR(HOST_r1,block);
#ifdef REC_USE_RETURN_FUNCS
			JUMPFunc(func_TestBranchBlock_ptr);
			UpdateImmediate(0);
#else
			recTestBranchBlock(0);
#endif
#endif
		}else{
#ifdef REC_USE_RETURN_FUNCS
			JUMPFunc(func_TestBranchIfOK_ptr);
			UpdateImmediate(0);
#else
			recTestBranchIfOK(0);
#endif
		}
	} else {
		RET();
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t!SetBranch");
	}
#endif
}


// CHUI: Se puede alargar el bloque. Se debe comprobar que no ha cambiado la memoria a donde se salta desde que se decompila
static void iJump(u32 branchPC) {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\tiJump(%p)\n",branchPC);
	}
#endif
	u32 encontrado=0;
#ifndef REC_USE_TABLE_REGS_FLUSH
	u32 *ptr_update=NULL;
#else
	table_regs_flush_last=0;
#endif
#ifdef REC_USE_2ND_PHASE
	u32 actual_opcodes=rec_total_opcodes+rec_opcodes;
#ifdef REC_USE_2ND_PRESERVE_CONST
	if (!rec_phase) {
		recPrev[rec_total_opcodes+rec_opcodes].consts = actual_consts;
	}
#endif
#endif
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc+=4;

	if (iLoadTest()) {
#if defined(DEBUG_CPU) || defined(DEBUG_CPU_OPCODES) || defined(REC_FORCE_DELAY_READ)
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t\t\t\tLoadTest");
		}
#endif	
		iUpdateRegs(1);
// CHUI: Actualiamos previamente PC, code y cycle
		MOV32ItoM_regs((u32)&psxRegs.pc,pc);
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;

#if defined(interpreter_new) || defined(interpreter_none)
		MOV32ItoR(HOST_r2,branchPC);
		MOV32ItoR(HOST_r1,(u32)psxRegs.code);
		MOV32ItoR(HOST_r0,(u32)&psxRegs);
		CALLFunc((u32)_psxDelayTest);
#else
		MOV32ItoR(HOST_r1,branchPC);
		MOV32ItoR(HOST_r0,_Rt_);
		CALLFunc((u32)psxDelayTest);
#endif

		RET()
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t!iJump");
		}
#endif
		return;
#else
		switch (psxTestLoadDelay(_Rt_,PSXMu32(branchPC))) {
			case 1: // Skipped
				break;
			case 2: // No implementado. CUIDADO!!!!
			default:
				recOpcode();
		}
#endif
	} else
		recOpcode()

#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
#ifndef REC_USE_FAST_SECURE
		if (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)&&(!rec_secure_writes))) {
#else
#ifndef REC_USE_FAST_BIOS
		if (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE))) {
#else
		if (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000))) {
#endif
#endif
			if (!block) {
				recPrevJump[recPrev_jumps].pc=branchPC;
				memcpy(recPrevJump[recPrev_jumps].regs,iRegs,sizeof(iRegs));
				recPrev[rec_total_opcodes].jump=&recPrevJump[recPrev_jumps++];
			}
		} else {
			branch=0;
			pc=pcold=branchPC;
		}
		return;
	} else {
#ifndef REC_USE_FAST_SECURE
		if (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)&&(!rec_secure_writes))) {
#else
#ifndef REC_USE_FAST_BIOS
		if (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE))) {
#else
		if (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000))) {
#endif
#endif
			int i;
			for(i=rec_total_opcodes_prev-1;i>=0;i--)
				if (recPrev[i].pc==branchPC && recPrev[i].jumped) {
					if (!recPrev[i].ptr || FindJump((iRegisters *)&iRegs,(iRegisters *)&recPrev[i].regs)) {
						encontrado=i+1;
					}
					break;
				}
		}
	}
#endif

	iLockReg(3);
#ifndef REC_USE_FAST_SECURE
	if ((!encontrado || block) && (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)&&(!rec_secure_writes)))) {
#else
#ifndef REC_USE_FAST_BIOS
	if ((!encontrado || block) && (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)))) {
#else
	if ((!encontrado || block) && (!((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)))) {
#endif
#endif
		MOV32ItoM_regs((u32)&psxRegs.pc,branchPC);
#ifndef REC_USE_TABLE_REGS_FLUSH
		ptr_update=armPtr;
		write32(ADD_IMM(HOST_lr,HOST_pc,0,0));
		iUpdateRegs(1);
		write32(BX_LR());
		*ptr_update|=((u32)armPtr)-((u32)ptr_update)-8;
#else
		iUpdateRegs(1);
#endif
	}

#ifndef REC_USE_FAST_BLOCK
	if (!block)
#endif
	{
#ifndef REC_USE_FAST_BIOS
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
// CHUI: Se tiene que calcular previamente los ciclos para que funcione bien psxBranchTest
// CHUI: Actualizamos psxRegs.cycle mientras dejamos el valor actualizado en r0
			MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.cycle); // r1 = psxRegs.cycle
			ADD32ItoR(HOST_r1,cycles_pending+(((pc - pcold) / 4) * BIAS)); // r1 += ciclos
			MOV32RtoM_regs((u32)&psxRegs.cycle,HOST_r1); // psxRegs.cycle = r1
			cycles_pending=0;
// CHUI: Comparamos con io_cycle_counter para saber si hay que hacer psxBranchTest
			if (r2_is_dirty) {
				MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
#ifdef REC_USE_R2
				r2_is_dirty=0;
#endif
			}
			write32(CMP_REGS(HOST_r1,HOST_r2)); // cmp r1, r2
		}
#ifdef REC_USE_FAST_BLOCK
	}
	if (!block) {
#endif
#ifndef REC_USE_FAST_BIOS
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
			j32Ptr[7]=(u32*)(Bit32u)armPtr;
			write32(BCC_FWD(0));
// CHUI: Es necesario psxBranchTest
			if (encontrado) {
				MOV32ItoM_regs((u32)&psxRegs.pc,branchPC);
#ifndef REC_USE_TABLE_REGS_FLUSH
				ptr_update=armPtr;
				write32(ADD_IMM(HOST_lr,HOST_pc,0,0));
				iUpdateRegs(0);
				write32(BX_LR());
				*ptr_update|=((u32)armPtr)-((u32)ptr_update)-8;
#else
				iUpdateRegs(0);
#endif
			} else {
#ifndef REC_USE_FAST_SECURE
				if ((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
				if ((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)) {
#else
				if ((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)) {
#endif
#endif
					MOV32ItoM_regs((u32)&psxRegs.pc,branchPC);
#ifndef REC_USE_TABLE_REGS_FLUSH
					ptr_update=armPtr;
					write32(ADD_IMM(HOST_lr,HOST_pc,0,0));
					iUpdateRegs(0);
					write32(BX_LR());
					*ptr_update|=((u32)armPtr)-((u32)ptr_update)-8;
#else
					iUpdateRegs(0);
#endif
				}
			}
			CALLFunc((u32)psxBranchTestCalculate);
		}
#ifndef REC_USE_FAST_SECURE
		if ((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
		if ((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)&&(Config.HLE)) {
#else
		if ((!block)&&(rec_total_opcodes<REC_MAX_OPCODES_LIMIT)&&((branchPC&0x1fffffff)<0x800000)) {
#endif
#endif
//CHUI: Tenemos que mirar si ha cambiado el PC en el BranchTest
#ifndef REC_USE_FAST_BIOS
			if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
			if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
				MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
				MOV32ItoR(HOST_r1,branchPC);
				write32(CMP_REGS(HOST_r0,HOST_r1));
			}
			if (pcabs(pc,branchPC)>=REC_MAX_TO_TEST) {
				unsigned valor=*((u32 *)&psxM[branchPC&0x1fffff]);
#ifndef REC_USE_FAST_BIOS
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
					j32Ptr[1]=armPtr; write32(BNE_FWD(0));
#ifdef REC_USE_R2
					MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
					r2_is_dirty=0;
#endif
					armSetJ32(j32Ptr[7]);
				}
// CHUI: Se comprueba que el contenido a donde se salta es igual que en el momento de recompilacion
				MOV32ItoR(HOST_r0,valor); // r0=proximo opcode
				MOV32MtoR(HOST_r1,(u32)&psxM[branchPC&0x1fffff]);
				write32(CMP_REGS(HOST_r0,HOST_r1));
				j32Ptr[2]=armPtr; write32(BEQ_FWD(0));
// CHUI: No es posible seguir con el bloque porque el codigo a donde se salta ha cambiado
				UpdateGteDelay(0);
				MOV32ItoR(HOST_r0,pcinit);
				MOV32ItoR(HOST_r1,branchPC);
				CALLFunc((u32)recClearDouble);
#ifndef REC_USE_TABLE_REGS_FLUSH
				if (ptr_update) {
					CALLFunc(((u32)ptr_update)+4);
#else
				if (table_regs_flush_last) {
					CALLFunc(table_regs_flush_last);
#endif
				} else {
					iUpdateRegs(0);
				}
				MOV32ItoR(HOST_r0,branchPC);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
#ifndef REC_USE_FAST_BIOS
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
					armSetJ32(j32Ptr[1]);
				}
				RET_with_HOST_r0();
				armSetJ32(j32Ptr[2]);
			} else {
#ifndef REC_USE_FAST_BIOS
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
					j32Ptr[1]=armPtr; write32(BEQ_FWD(0));
#if defined(REC_USE_GTE_DELAY_CALC) && defined(gte_new) && defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTECALC_INLINE)
					if (func_GTE_delay_ptr) {
						UpdateGteDelay(0);
						RET();
					}
					else
#endif
					{
						RET_with_HOST_r0();
					}
					armSetJ32(j32Ptr[1]);
#ifdef REC_USE_R2
					MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
					r2_is_dirty=0;
#endif
					armSetJ32(j32Ptr[7]);
				}
			}
			branch=0;
			pc=pcold=branchPC;
		} else {
// Aqui siempre salimos por lo que podemos comprobar si tiene la cabecera de los opcodes precacheados
			UpdateGteDelay(0);
#if defined(REC_USE_2ND_PHASE) && defined(REC_USE_2ND_REG_CACHE)
			if (!encontrado && ((branchPC&0x1fffffff)<0x800000)) {
				u32 bpc=*((u32 *)PC_REC(branchPC));

				if (!bpc) {
//CHUI: Si no esta recompilado donde saltamos se anula el propio bloque para que la proxima vez este bien
#ifdef DEBUG_CPU
					dbg("\t\tNot recompiled");
#endif
#ifndef REC_USE_FAST_BIOS
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
						armSetJ32(j32Ptr[7]);
					} else {
						ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS));
					}
					MOV32ItoR(HOST_r0,pcinit);
					CALLFunc((u32)rec2nd_func);
					RET();
				} else {
//CHUI: Cargamos el rec_cache del siguiente bloque
					u32 rec_cache=*((u32 *)(bpc-8));
#ifdef DEBUG_CPU
					dbgf("\t\tRecompiled Cache=%.8X\n",rec_cache);
#endif
//CHUI: Tenemos que mirar previamente si ha cambiado el PC en el BranchTest
#ifndef REC_USE_FAST_BIOS
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
						MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
						MOV32ItoR(HOST_r1,branchPC);
						write32(CMP_REGS(HOST_r0,HOST_r1));
						j32Ptr[1]=armPtr; write32(BEQ_FWD(0));
						RET_with_HOST_r0();
						armSetJ32(j32Ptr[1]);
						armSetJ32(j32Ptr[7]);
					}
// CHUI: Es realmente necesario?
#if 0
// Se podria pasar todas a funcion?
					if (pcabs(pc,branchPC)>=REC_MAX_TO_TEST) {
						unsigned valor=*((u32 *)&psxM[branchPC&0x1fffff]);
// CHUI: Se comprueba que el contenido a donde se salta es igual que en el momento de recompilacion
						MOV32ItoR(HOST_r0,valor); // r0=proximo opcode
						MOV32MtoR(HOST_r1,(u32)&psxM[branchPC&0x1fffff]);
						write32(CMP_REGS(HOST_r0,HOST_r1));
						j32Ptr[2]=armPtr; write32(BEQ_FWD(0));
// CHUI: No es posible seguir con el bloque porque el codigo a donde se salta ha cambiado
						MOV32ItoR(HOST_r0,pcinit);
						MOV32ItoR(HOST_r1,branchPC);
						CALLFunc((u32)recClearDouble);
						RET_NC();
						armSetJ32(j32Ptr[2]);
					}
#endif
					if (!FindCache(rec_cache,1)) {
//CHUI: Si no tenemos nada igual se sale para llamar al siguiente bloque de forma normal
						MOV32ItoR(HOST_r0,bpc);
					} else {
//CHUI: Aqui sabemos que el salto a branchPC es correcto.
						ConvertCache(rec_cache);
						MOV32ItoR(HOST_r0,(bpc)+(4*FindCache(rec_cache,0)));
					}
					MOV32RtoR(HOST_pc,HOST_r0);
					UpdateImmediate(0);
				}
			} else 
#endif
			{
#ifdef REC_USE_2ND_PHASE
				if (encontrado) {
#ifndef REC_USE_FAST_BIOS
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
						RET();
						armSetJ32(j32Ptr[7]);
					} else {
						ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS));
					}
					if (!recPrev[encontrado-1].ptr) {
						recPrevJump[recPrev_jumps].ptr=armPtr;
						memcpy(recPrevJump[recPrev_jumps].regs,iRegs,sizeof(iRegs));
						recPrevJump[recPrev_jumps].pc=branchPC;
						recPrevJump[recPrev_jumps++].prev=encontrado-1;
						write32(NOP);
					} else {
						ConvertJump((iRegisters *)&iRegs,(iRegisters *)&recPrev[encontrado-1].regs);
						JUMPFunc((u32)recPrev[encontrado-1].ptr);
					}
				} else
#endif
				{
#ifndef REC_USE_FAST_BIOS
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
						armSetJ32(j32Ptr[7]);
					} else {
						ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS));
					}
					RET();
				}
			}
		}
	} else {
#ifndef REC_USE_FAST_BLOCK
		UpdateGteDelay(0);
		ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;
#ifndef REC_USE_FAST_BIOS
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
			CALLFunc((u32)psxBranchTest);
		}
		RET();
#else
#ifndef REC_USE_FAST_BIOS
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
			CALLFuncCS((u32)psxBranchTestCalculate);
		}
		UpdateGteDelay(0);
		MOV32ItoR(HOST_r1,block);
#ifdef REC_USE_RETURN_FUNCS
		JUMPFunc(func_TestBranchBlock_ptr);
		UpdateImmediate(0);
#else
		recTestBranchBlock(0);
#endif
#endif
	}
	iUnlockReg(3);
#ifndef REC_USE_FAST_BIOS
	if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
	if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
		rec_skips=rec_total_opcodes;
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t!iJump");
	}
#endif
}

static void iBranch(u32 branchPC, int savectx) {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbgf("\tiBranch(%p,%i)\n",branchPC,savectx);
	}
#endif
	u32 back_cycles_pending=cycles_pending;
	if (savectx==1) {
		memcpy(iRegsS, iRegs, sizeof(iRegs));
		memcpy(HWRegsS, HWRegs, sizeof(HWRegs));
#ifdef REC_USE_2ND_PHASE
		if (!rec_phase) {
			memcpy(recPrev_totalS, recPrev_total, sizeof(recPrev_total));
		}
#endif
	}
	u32 encontrado=0;
#ifdef REC_USE_2ND_PHASE
	u32 actual_opcodes=rec_total_opcodes+rec_opcodes;
#ifdef REC_USE_2ND_PRESERVE_CONST
	u64 back_actual_consts=actual_consts;
	if (!rec_phase) {
		recPrev[rec_total_opcodes+rec_opcodes].consts = actual_consts;
	}
#endif
#endif
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc+= 4;

	// the delay test is only made when the branch is taken
	if (iLoadTest()) {
#if defined(DEBUG_CPU) || defined(DEBUG_CPU_OPCODES) || defined(REC_FORCE_DELAY_READ)
#ifdef DEBUG_CPU
		if (rec_phase)
			dbg("\t\t\t\tLoadTest");
#endif	
		iUpdateRegs(1);
// CHUI: Actualiamos previamente PC, code y cycle
		MOV32ItoM_regs((u32)&psxRegs.pc,pc);
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;

#if defined(interpreter_new) || defined(interpreter_none)
		MOV32ItoR(HOST_r2,branchPC);
		MOV32ItoR(HOST_r1,(u32)psxRegs.code);
		MOV32ItoR(HOST_r0,(u32)&psxRegs);
		CALLFunc((u32)_psxDelayTest);
#else
		MOV32ItoR(HOST_r1,branchPC);
		MOV32ItoR(HOST_r0,_Rt_);
		CALLFunc((u32)psxDelayTest);
#endif

		RET()
#ifdef DEBUG_CPU
		if (rec_phase)
			dbg("\t!iBranch");
#endif
		if (savectx) {
			cycles_pending=back_cycles_pending;
			memcpy(iRegs, iRegsS, sizeof(iRegs));
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
#ifdef REC_USE_2ND_PRESERVE_CONST
			if (!rec_phase) {
				actual_consts=back_actual_consts;
			}
#endif
		}
		return;
#else
		switch (psxTestLoadDelay(_Rt_,PSXMu32(branchPC))) {
			case 1: // Skipped
				break;
			case 2: // No implementado. CUIDADO!!!!
			default:
				recOpcode();
		}
#endif
	} else
		recOpcode();

#ifdef REC_USE_2ND_PHASE
	if (!rec_phase) {
		if (!block && ((savectx)||(block)||(rec_total_opcodes>REC_MAX_OPCODES_LIMIT))) {
			recPrevJump[recPrev_jumps].pc=branchPC;
			memcpy(recPrevJump[recPrev_jumps].regs,iRegs,sizeof(iRegs));
			recPrev[rec_total_opcodes].jump=&recPrevJump[recPrev_jumps++];
		} else {
			if (!block) {
				branch=0;
				pc=pcold=branchPC;
				pc-=4;
			}
		}
		if (savectx) {
			cycles_pending=back_cycles_pending;
			memcpy(recPrev_total, recPrev_totalS, sizeof(recPrev_total));
			memcpy(iRegs, iRegsS, sizeof(iRegs));
			memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
#ifdef REC_USE_2ND_PRESERVE_CONST
			actual_consts=back_actual_consts;
#endif
		}
		return;
	} else {
		if (!block && ((savectx)||(rec_total_opcodes>REC_MAX_OPCODES_LIMIT))) {
			int i;
			for(i=rec_total_opcodes_prev-1;i>=0;i--)
				if (recPrev[i].pc==branchPC && recPrev[i].jumped) {
					if (!recPrev[i].ptr || FindJump((iRegisters *)&iRegs,(iRegisters *)&recPrev[i].regs)) {
						encontrado=i+1;
					}
					break;
				}
		}
	}
#endif

	iLockReg(3);
// CHUI: Si hay que salvar contexto o estamos en modo bloque se saldra de todos modos por lo que podemos actualizar los registros
	if ((!encontrado || block) && ((savectx)||(block)||(rec_total_opcodes>REC_MAX_OPCODES_LIMIT)))
		iUpdateRegs(1);
	if (!block) {
		if ((savectx)||(rec_total_opcodes>REC_MAX_OPCODES_LIMIT)) {
			UpdateGteDelay(0);
		}
#ifndef REC_USE_FAST_BIOS
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE))
#else
		if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS))
#endif
		{
			MOV32ItoM_regs((u32)&psxRegs.pc, branchPC);
// CHUI: Actualizamos psxRegs.cycle mientras dejamos el valor actualizado en r1
			MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.cycle); // r1 = psxRegs.cycle
			ADD32ItoR(HOST_r1,cycles_pending+(((pc - pcold) / 4) * BIAS)); // r1 += ciclos
			MOV32RtoM_regs((u32)&psxRegs.cycle,HOST_r1); // psxRegs.cycle = r1
			cycles_pending=0;
// CHUI: Comparamos con io_cycle_counter para saber si hay que hacer psxBranchTest
			if (r2_is_dirty) {
				MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
#ifdef REC_USE_R2
				if (!savectx)
					r2_is_dirty=0;
#endif
			}
			write32(CMP_REGS(HOST_r1,HOST_r2)); // cmp r1, r2
			j32Ptr[7]=(u32*)(Bit32u)armPtr;
			write32(BCC_FWD(0));
// CHUI: Es necesario psxBranchTest
// CHUI: Si no hay que salvar contexto y no estamos en modo bloque tenemso que actualizar los registros
			if (encontrado)
				iUpdateRegs(0);
			else
				if (!((savectx)||(rec_total_opcodes>REC_MAX_OPCODES_LIMIT)))
					iUpdateRegs(0);
			CALLFunc((u32)psxBranchTestCalculate);
		}
//CHUI: Salimos si estamos en modo block o hay que salvar contexto
		if ((savectx)||(rec_total_opcodes>REC_MAX_OPCODES_LIMIT)) {
#if defined(REC_USE_2ND_PHASE) && defined(REC_USE_2ND_REG_CACHE)
			if (((branchPC&0x1fffffff)<0x800000) && !encontrado) {
				u32 bpc=*((u32 *)PC_REC(branchPC));

				if (!bpc) {
//CHUI: Si no esta recompilado donde saltamos se anula el propio bloque para que la proxima vez este bien
#ifdef DEBUG_CPU
					dbg("\t\tNot recompiled");
#endif
#ifndef REC_USE_FAST_BIOS
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) { 
#else
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) { 
#endif
						MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
						j32Ptr[1]=armPtr; write32(B_FWD(0));
						armSetJ32(j32Ptr[7]);
						MOV32ItoR(HOST_r0,pcinit);
						CALLFunc((u32)rec2nd_func);
						MOV32ItoR(HOST_r0,branchPC);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
						armSetJ32(j32Ptr[1]);
					} else {
						MOV32ItoR(HOST_r0,pcinit);
						CALLFunc((u32)rec2nd_func);
						ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS)); // r0 += ciclos
						MOV32ItoR(HOST_r0,branchPC);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
					}
#ifndef REC_USE_FAST_SECURE
					if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
					if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#else
					if ((pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#endif
#endif
#ifdef REC_USE_RETURN_FUNCS
						JUMPFunc(func_TestBranchIfOK_ptr+4);
						UpdateImmediate(0);
#else
						recTestBranchIfOK(1);
#endif
					} else {
						RET_with_HOST_r0();
					}
				} else {
//CHUI: Cargamos el rec_cache del siguiente bloque
					u32 rec_cache=*((u32 *)(bpc-8));
#ifdef DEBUG_CPU
					dbgf("\t\tRecompiled Cache=%.8X\n",rec_cache);
#endif
#ifndef REC_USE_FAST_BIOS
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) { 
#else
					if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) { 
#endif
//CHUI: Tenemos que mirar previamente si ha cambiado el PC en el BranchTest
						MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
						MOV32ItoR(HOST_r1,branchPC);
						write32(CMP_REGS(HOST_r0,HOST_r1));
#ifndef REC_USE_FAST_SECURE
						if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
						if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#else
						if ((pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#endif
#endif
#ifdef REC_USE_RETURN_FUNCS
							JUMPFuncNE(func_TestBranchIfOK_ptr+4);
#else
							j32Ptr[1]=armPtr; write32(BEQ_FWD(0));
							recTestBranchIfOK(1);
							armSetJ32(j32Ptr[1]);
#endif
						} else {
							j32Ptr[1]=armPtr; write32(BEQ_FWD(0));
							RET_with_HOST_r0();
							armSetJ32(j32Ptr[1]);
						}
						armSetJ32(j32Ptr[7]);
					}
// CHUI: Es realmente necesario?
#if 0
// Se podria pasar todas a funcion?
					if (pcabs(pc,branchPC)>=REC_MAX_TO_TEST) {
						unsigned valor=*((u32 *)&psxM[branchPC&0x1fffff]);
// CHUI: Se comprueba que el contenido a donde se salta es igual que en el momento de recompilacion
						MOV32ItoR(HOST_r0,valor); // r0=proximo opcode
						MOV32MtoR(HOST_r1,(u32)&psxM[branchPC&0x1fffff]);
						write32(CMP_REGS(HOST_r0,HOST_r1));
						j32Ptr[2]=armPtr; write32(BEQ_FWD(0));
// CHUI: No es posible seguir con el bloque porque el codigo a donde se salta ha cambiado
						MOV32ItoR(HOST_r0,pcinit);
						MOV32ItoR(HOST_r1,branchPC);
						CALLFunc((u32)recClearDouble);
						RET_NC();
						armSetJ32(j32Ptr[2]);
					}
#endif
					if (!FindCache(rec_cache,1)) {
//CHUI: Si no tenemos nada igual se sale para llamar al siguiente bloque de forma normal
						MOV32ItoR(HOST_r0,bpc);
					} else {
//CHUI: Aqui sabemos que el salto a branchPC es correcto.
						ConvertCache(rec_cache);
						MOV32ItoR(HOST_r0,(bpc)+(4*FindCache(rec_cache,0)));
					}
					MOV32RtoR(HOST_pc,HOST_r0);
					UpdateImmediate(0);
				}
			}
			else 
#endif
			{
#ifndef REC_USE_FAST_BIOS
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) { 
#else
				if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) { 
#endif
					MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
					j32Ptr[1]=armPtr; write32(B_FWD(0)); // Apuntamos el salto
// CHUI: Aqui llegaremos si no es necesario psxBranchTest
					armSetJ32(j32Ptr[7]);
#ifdef REC_USE_2ND_PHASE
					if (encontrado) {
						if (!recPrev[encontrado-1].ptr) {
							recPrevJump[recPrev_jumps].ptr=armPtr;
							memcpy(recPrevJump[recPrev_jumps].regs,iRegs,sizeof(iRegs));
							recPrevJump[recPrev_jumps].pc=branchPC;
							recPrevJump[recPrev_jumps++].prev=encontrado-1;
							write32(NOP);
						} else {
							ConvertJump((iRegisters *)&iRegs,(iRegisters *)&recPrev[encontrado-1].regs);
							JUMPFunc((u32)recPrev[encontrado-1].ptr);
						}
					} else
#endif
						MOV32ItoR(HOST_r0,branchPC);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
					armSetJ32(j32Ptr[1]);
				} else {
					ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS)); // r0 += ciclos
#ifdef REC_USE_2ND_PHASE
					if (encontrado) {
						if (!recPrev[encontrado-1].ptr) {
							recPrevJump[recPrev_jumps].ptr=armPtr;
							memcpy(recPrevJump[recPrev_jumps].regs,iRegs,sizeof(iRegs));
							recPrevJump[recPrev_jumps].pc=branchPC;
							recPrevJump[recPrev_jumps++].prev=encontrado-1;
							write32(NOP);
						} else {
							ConvertJump((iRegisters *)&iRegs,(iRegisters *)&recPrev[encontrado-1].regs);
							JUMPFunc((u32)recPrev[encontrado-1].ptr);
						}
					} else
#endif
						MOV32ItoR(HOST_r0,branchPC);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
				}
#ifndef REC_USE_FAST_SECURE
				if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
				if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#else
				if ((pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#endif
#endif
#ifdef REC_USE_RETURN_FUNCS
					JUMPFunc(func_TestBranchIfOK_ptr+4);
					UpdateImmediate(0);
#else
					recTestBranchIfOK(1);
#endif
				} else {
					RET_with_HOST_r0();
				}
			}
		} else {
#ifndef REC_USE_FAST_BIOS
			if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
			if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
//CHUI: En caso contrario tenemos que mirar si ha cambiado el PC en el BranchTest
				MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
				MOV32ItoR(HOST_r1,branchPC);
				write32(CMP_REGS(HOST_r0,HOST_r1));
				j32Ptr[1]=armPtr; write32(BEQ_FWD(0));
#if defined(REC_USE_GTE_DELAY_CALC) && defined(gte_new) && defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTECALC_INLINE)
				if (func_GTE_delay_ptr) {
					UpdateGteDelay(0);
					MOV32MtoR_regs(HOST_r0,(u32)&psxRegs.pc);
				}
#endif
#ifndef REC_USE_FAST_SECURE
				if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
				if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#else
				if ((pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#endif
#endif
#ifdef REC_USE_RETURN_FUNCS
					JUMPFunc(func_TestBranchIfOK_ptr+4);
					UpdateImmediate(0);
#else
					recTestBranchIfOK(1);
#endif
				} else {
					RET_with_HOST_r0();
				}
				armSetJ32(j32Ptr[1]);
#ifdef REC_USE_R2
				MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
				r2_is_dirty=0;
#endif
// CHUI: Aqui llegaremos si no es necesario psxBranchTest
				armSetJ32(j32Ptr[7]);
			} else {
#if defined(USE_CYCLE_ADD) || defined(DEBUG_CPU_OPCODES)
				ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
#else
				cycles_pending+=(((pc - pcold) / 4) * BIAS); // r0 += ciclos
#endif
			}
#ifndef REC_USE_FAST_SECURE
			if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)&&(!rec_secure_writes)) {
#else
#ifndef REC_USE_FAST_BIOS
			if ((Config.HLE)&&(pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#else
			if ((pcabs(pc,branchPC)>=REC_MAX_TO_TEST)) {
#endif
#endif
// CHUI: Se comprueba que el contenido a donde se salta es igual que en el momento de recompilacion
				unsigned valor=*((u32 *)&psxM[branchPC&0x1fffff]);
				MOV32ItoR(HOST_r0,valor); // r0=proximo opcode
				MOV32MtoR(HOST_r1,(u32)&psxM[branchPC&0x1fffff]);
				write32(CMP_REGS(HOST_r0,HOST_r1));
				j32Ptr[2]=armPtr; write32(BEQ_FWD(0));
// CHUI: No es posible seguir con el bloque porque el codigo a donde se salta ha cambiado
				MOV32ItoR(HOST_r0,pcinit);
				MOV32ItoR(HOST_r1,branchPC);
				CALLFunc((u32)recClearDouble);
				iUpdateRegs(0);
				UpdateGteDelay(0);
#ifndef REC_USE_FAST_BIOS
				if (!(((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)))) {
#else
				if (!(((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)))) {
#endif
					ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending);
				}
				MOV32ItoR(HOST_r0,branchPC);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
				RET_with_HOST_r0();
				armSetJ32(j32Ptr[2]);
			}
			branch=0;
			pc=pcold=branchPC;
		}
	} else {
#ifndef REC_USE_FAST_BLOCK
		MOV32ItoM_regs((u32)&psxRegs.pc, branchPC);
		ADD32ItoM_regs((u32)&psxRegs.cycle,cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;
		CALLFunc((u32)psxBranchTest);
		UpdateGteDelay(0);
		RET();
#else
		MOV32ItoM_regs((u32)&psxRegs.pc, branchPC);
		MOV32MtoR_regs(HOST_r1,(u32)&psxRegs.cycle); // r1 = psxRegs.cycle
		ADD32ItoR(HOST_r1,cycles_pending+(((pc - pcold) / 4) * BIAS)); // r1 += ciclos
		MOV32RtoM_regs((u32)&psxRegs.cycle,HOST_r1); // psxRegs.cycle = r1
		cycles_pending=0;
		
		if (r2_is_dirty) {
			MOV32MtoR_regs(HOST_r2,(u32)&psxRegs.io_cycle_counter); // r2 = psxRegs.io_cycle_counter
		}
		write32(CMP_REGS(HOST_r1,HOST_r2)); // cmp r1, r2
// CHUI: Es necesario psxBranchTest si r1>r2
		CALLFuncCS((u32)psxBranchTestCalculate);
		UpdateGteDelay(0);
		MOV32ItoR(HOST_r1,block);
#ifdef REC_USE_RETURN_FUNCS
		JUMPFunc(func_TestBranchBlock_ptr);
		UpdateImmediate(0);
#else
		recTestBranchBlock(0);
#endif
#endif
	}
	pc-= 4;
	iUnlockReg(3);

	if (savectx) {
		cycles_pending=back_cycles_pending;
		memcpy(iRegs, iRegsS, sizeof(iRegs));
		memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
	}
#ifndef REC_USE_FAST_BIOS
	if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)||(!Config.HLE)) {
#else
	if ((rec_total_opcodes-rec_skips>=REC_MAX_SKIPS)) {
#endif
		rec_skips=rec_total_opcodes;
	}
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t!iBranch");
	}
#endif
}

// CHUI: No se puede activar roll-loops y segunda fase de recompilador por el momento.
#ifndef REC_USE_2ND_PHASE
static void iLoop(void) {
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\tiLoop");
	}
#endif
	u32 back_cycles_pending=cycles_pending;
	memcpy(iRegsS, iRegs, sizeof(iRegs));
	memcpy(HWRegsS, HWRegs, sizeof(HWRegs));

#ifdef REC_USE_2ND_PRESERVE_CONST
	u64 back_actual_consts=actual_consts;
	if (!rec_phase) {
		recPrev[rec_total_opcodes+rec_opcodes].consts = actual_consts;
	}
#endif
	branch = 1;
	psxRegs.code = PSXMu32(pc);
	pc+= 4;

	// the delay test is only made when the branch is taken
	if (iLoadTest()) {
#if defined(DEBUG_CPU) || defined(DEBUG_CPU_OPCODES) || defined(REC_FORCE_DELAY_READ)
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t\t\t\tLoadTest");
		}
#endif	
		iUpdateRegs(1);
// CHUI: Actualiamos previamente PC, code y cycle
		MOV32ItoM_regs((u32)&psxRegs.pc,pc);
		MOV32ItoM_regs((u32)&psxRegs.code, psxRegs.code);
		ADD32ItoM_regs((u32)&psxRegs.cycle, cycles_pending+(((pc - pcold) / 4) * BIAS));
		cycles_pending=0;

#if defined(interpreter_new) || defined(interpreter_none)
		MOV32ItoR(HOST_r2,pc);
		MOV32ItoR(HOST_r1,(u32)psxRegs.code);
		MOV32ItoR(HOST_r0,(u32)&psxRegs);
		CALLFunc((u32)_psxDelayTest);
#else
		MOV32ItoR(HOST_r1,pc);
		MOV32ItoR(HOST_r0,_Rt_);
		CALLFunc((u32)psxDelayTest);
#endif

		RET()
#ifdef DEBUG_CPU
		if (rec_phase) {
			dbg("\t!iLoop");
		}
#endif
		return;
#else
		switch (psxTestLoadDelay(_Rt_,PSXMu32(pc))) {
			case 1: // Skipped
				break;
			case 2: // No implementado. CUIDADO!!!!
			default:
				recOpcode();
		}
#endif
	} else
		recOpcode();

	iLockReg(3);
	UpdateGteDelay(0);
	iUpdateRegs(1);
#ifdef REC_USE_FAST_BLOCK
	if (!block) {
		MOV32ItoM_regs((u32)&psxRegs.pc, pc);
		RET_cycles();
	} else {
		ADD32ItoM_regs((u32)&psxRegs.cycle, (((pc - pcold) / 4) * BIAS));
		MOV32ItoR(HOST_r0, pc);
		MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
		MOV32ItoR(HOST_r1,block);
#ifdef REC_USE_RETURN_FUNCS
		JUMPFunc(func_TestBranchBlock_ptr+4);
		UpdateImmediate(0);
#else
		recTestBranchBlock(1);
#endif
	}
#else
	MOV32ItoM_regs((u32)&psxRegs.pc, pc);
	RET_cycles();
#endif
	pc-=4;
	iUnlockReg(3);
	cycles_pending=back_cycles_pending;
	memcpy(iRegs, iRegsS, sizeof(iRegs));
	memcpy(HWRegs, HWRegsS, sizeof(HWRegs));
#ifdef REC_USE_2ND_PRESERVE_CONST
	if (!rec_phase) {
		actual_consts=back_actual_consts;
	}
#endif
#ifdef DEBUG_CPU
	if (rec_phase) {
		dbg("\t!iLoop");
	}
#endif
}
#endif

static u32 iGetSaveMask(u32 rmin, u32 rmax) {
	u32 r, ret=0;
	for(r=rmin;r<=rmax;r++) {
		if (HWRegs[r].state==ST_MAPPED) {
			ret|=(1<<r);
		}
	}
	return ret;
}

static int recInit() {
	int i;

	memset(recMem,0,RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000);

	for (i=0; i<0x80; i++) psxRecLUT[i + 0x0000] = (u32)&recRAM[(i & 0x1f) << 16];
	memcpy(psxRecLUT + 0x8000, psxRecLUT, 0x80 * 4);
	memcpy(psxRecLUT + 0xa000, psxRecLUT, 0x80 * 4);

	for (i=0; i<0x08; i++) psxRecLUT[i + 0xbfc0] = (u32)&recROM[i << 16];

	iResetTableRegs();
	return 0;
}

static void recReset() {
	memset(recRAM, 0, 0x200000);
	memset(recROM, 0, 0x080000);

	armPtr=(u32*)recMem;

	branch = 0;
	memset(iRegs, 0, sizeof(iRegs));
	iRegs[0].state = ST_CONST;
	iRegs[0].k     = 0;
	
	memset(HWRegs, 0, sizeof(HWRegs));
	HWRegs[0].state = ST_CONST; /* do not map r0 */
	HWRegs[1].state = ST_CONST; /* do not map r1 */
	HWRegs[2].state = ST_CONST; /* do not map r2 */
#ifndef REC_USE_R3
	HWRegs[3].state = ST_CONST; /* do not map r3 */
#endif
	HWRegs[11].state = ST_CONST; /* do not map r11 (v8) - psxRegs pointer */
	HWRegs[12].state = ST_CONST; /* do not map r12 (ip) */
	HWRegs[13].state = ST_CONST; /* do not map r13 (sp) */
	HWRegs[14].state = ST_CONST; /* do not map r14 (lr) */
	HWRegs[15].state = ST_CONST; /* do not map r15 (pc) */	

	iResetTableRegs();
}

static void recShutdown() {
}

#include "opcodes.h"


static void recFunctions() {
	u32 *armPtr_old=armPtr;
#ifdef REC_USE_2ND_PHASE
	rec_phase=1;
#endif
	armPtr=(u32*)&recMem[RECMEM_SIZE + (REC_MAX_OPCODES*2)];
	func_TestBranchIfOK_ptr=(unsigned)armPtr;
#ifdef REC_USE_RETURN_FUNCS
#ifdef DEBUG_CPU
	dbg("-- FUNCTION TestBranchIfOK --");
#endif
	recTestBranchIfOK(0);
	func_Return_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION Return --");
#endif
	recReturn(0); gen_align4();
#ifdef REC_USE_FAST_BLOCK
	func_TestBranchBlock_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION TestBranchBlock --");
#endif
	recTestBranchBlock(0); gen_align4();
#endif
#endif
#ifdef REC_USE_MEMORY_FUNCS
	func_MemRead8_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION MemRead8 --");
#endif
	recMemRead8(); gen_align4();
	func_MemRead16_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbgf("-- FUNCTION MemRead16 --");
#endif
	recMemRead16(); gen_align4();
	func_MemRead32_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION MemRead32 --");
#endif
	recMemRead32(); gen_align4();
	func_HWRead8_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION HWRead8 --");
#endif
	recHWRead8(); gen_align4();
	func_HWRead16_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION HWRead16 --");
#endif
	recHWRead16(); gen_align4();
	func_HWRead32_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbgf("-- FUNCTION HWRead32 --");
#endif
	recHWRead32(); gen_align4();
#if !defined(USE_CYCLE_ADD) && !defined(DEBUG_CPU_OPCODES)
	func_MemWrite8_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION MemWrite8 --");
#endif
	recMemWrite8(); gen_align4();
	func_MemWrite16_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION MemWrite16 --");
#endif
	recMemWrite16(); gen_align4();
	func_MemWrite32_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION MemWrite32 --");
#endif
	recMemWrite32(); gen_align4();
#endif
#endif
#if defined(REC_USE_GTE_FUNCS) && defined(gte_new)
	func_GTE_MFC2_29_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_MFC2 --");
#endif
	recGTE_MFC2_29(HOST_r0);
	write32(BX_LR()); gen_align4();

	func_GTE_MTC2_15_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_MTC2_15 --");
#endif
	recGTE_MTC2_15(HOST_r1);
	write32(BX_LR()); gen_align4();

	func_GTE_MTC2_28_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_MTC2_28 --");
#endif
	recGTE_MTC2_28(HOST_r1);
	write32(BX_LR()); gen_align4();

	func_GTE_updateCODEs_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateCODEs --");
#endif
	recGTE_updateCODEs();
	write32(BX_LR()); gen_align4();
	func_GTE_updateMACs_lm0_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMACs0 --");
#endif
	recGTE_updateMACs(0,0);
	write32(BX_LR()); gen_align4();
	func_GTE_updateMACs_lm1_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMACs1 --");
#endif
	recGTE_updateMACs(1,0);
	write32(BX_LR()); gen_align4();
	func_GTE_updateMACs_lm0_shift12_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMACs0_12 --");
#endif
	recGTE_updateMACs(0,12);
	write32(BX_LR()); gen_align4();
	func_GTE_updateMACs_lm1_shift12_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMACs1_12 --");
#endif
	recGTE_updateMACs(1,12);
	write32(BX_LR()); gen_align4();
#ifndef USE_GTE_FLAG
	func_GTE_updateMACs_lm0_flag_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMACs0_flag --");
#endif
#if defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTE_DELAY_CALC)
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	recGTE_updateMACs_flag(0,0,1);
	write32(BX_LR()); gen_align4();
	func_GTE_updateMACs_lm1_flag_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMACs1_flag --");
#endif
#if defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTE_DELAY_CALC)
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	recGTE_updateMACs_flag(1,0,1);
	write32(BX_LR()); gen_align4();
#endif
	func_GTE_updateMAC3_lm0_flag_ptr=(unsigned)armPtr;
#ifdef DEBUG_CPU
	dbg("-- FUNCTION GTE_updateMAC3_flag --");
#endif
#if defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTE_DELAY_CALC)
	MOV32ItoM_regs((u32)&psxRegs.CP2C.r[31],0);
#endif
	recGTE_updateMAC3_flag(0,0,1);
	write32(BX_LR());
#endif

	if (armPtr>(u32 *)&recMem[RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000]){
		puts("recFunctions memory overflow");
		pcsx4all_exit();
	}
	UpdateImmediate(1);
	sys_cacheflush((u32*)&recMem[RECMEM_SIZE + (REC_MAX_OPCODES*2)],(u32*)&recMem[RECMEM_SIZE + (REC_MAX_OPCODES*2) + 0x4000]);
	armPtr=armPtr_old;
}

static void recRecompile() {
	u32 *armPtr_old;

	if (!func_TestBranchIfOK_ptr)
		recFunctions();
		
//	if (psxRegs.reserved!=(void *)&psxRecLUT[0])
		psxRegs.reserved=(void *)&psxRecLUT[0];

	/* if armPtr reached the mem limit reset whole mem */
	if (((u32)armPtr - (u32)recMem) >= (RECMEM_SIZE - 0x10000))
		recReset();
#if defined(REC_USE_GTE_DELAY_CALC) && defined(gte_new) && defined(REC_USE_GTE_FUNCS) && defined(REC_USE_GTECALC_INLINE)
	func_GTE_delay_ptr=0;
#endif

#ifdef DEBUG_CPU
	dbgf("->PC=%p%s\n",psxRegs.pc,block?" (block)":"");
#endif
	armPtr_old=armPtr;
#ifdef REC_USE_2ND_PHASE
	u32 cycles_old=psxRegs.cycle;
	u32 rec_cache=0;
	branch = 0;
	rec_phase=0;
	cycles_pending=0;
	pc = pcold = pcinit = psxRegs.pc;
	iClearRegsPrev();
	for (rec_total_opcodes = 0; !branch && rec_total_opcodes<REC_MAX_OPCODES; rec_total_opcodes+=rec_opcodes) {
#ifndef REC_USE_2ND_PRESERVE_CONST
		memcpy(recPrev[rec_total_opcodes].regs,iRegs,sizeof(iRegs));
#else
		recPrev[rec_total_opcodes].consts = actual_consts;
#endif
		//CHUI: Numero de opcodes en cada vuelta del bucle
		rec_opcodes=0;
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		recOpcode()
	}
#ifdef REC_USE_2ND_REG_CACHE
//	if (!block)
	{
		int i,j;
		for(j=0;j<REC_MAX_2ND_REG_CACHE;j++) {
			int candidato=0;
			for(i=1;i<32;i++) {
				if (recPrev_total[candidato].k<recPrev_total[i].k && (!((rec_cache>>i)&1))) {
					candidato=i;
				}
			}
			if (recPrev_total[candidato].k)
				rec_cache|=(1<<candidato);
		}
	}
#endif
	psxRegs.pc=pcinit;
#ifdef DEBUG_CPU
	if (armPtr_old!=armPtr) {
		puts("REC-2ND ERROR armPtr!");
		pcsx4all_exit();
	}
	if (cycles_old!=psxRegs.cycle) {
		puts("REC-2ND ERROR cycles!");
		pcsx4all_exit();
	}
#endif
	{
		int i,j,k;
		for(i=0;i<recPrev_jumps;i++)
			for(j=rec_total_opcodes-1;j>=0;j--)
				if (!recPrev[j].jumped && recPrev[j].pc==recPrevJump[i].pc) {
#ifndef REC_USE_2ND_PRESERVE_CONST
					for (k=0;k<34;k++)
						if (recPrev[j].regs[k].state==ST_CONST && (recPrevJump[i].regs[k].state!=ST_CONST || (recPrevJump[i].regs[k].state==ST_CONST && recPrev[j].regs[k].k!=recPrevJump[i].regs[k].k)))
							recPrev[j].clear_const[k]=1;
#else
					for (k=1;k<34;k++)
						if ((recPrev[j].consts&(1ULL<<k)) && recPrevJump[i].regs[k].state!=ST_CONST)
							break;
					if (k<34) break;
#endif
					recPrev[j].jumped=1;
					recPrev[j].ptr=NULL;
					break;
				}
		rec_total_opcodes_prev=rec_total_opcodes;
	}
	recPrev_jumps=0;
	rec_phase=1;
#ifdef REC_USE_2ND_REG_CACHE
// CHUI: Colocamos una pequeña funcion para el calculo de veces que un bloque no puede enlazar con el siguiente
	rec2nd();
// CHUI: Ponemos en la cabecera el resultado de los registros cacheados
	write32(rec_cache);
#endif
#endif

	// CHUI: Limpiamos registros porque no sabemos si ha salido limpio de un iUpdateRegs
	iClearRegs();

//	armPtr = (u32*)(((u32)armPtr + 32) & ~(31));
// CHUI: Ponemos como cabecera el primer opcode para verificar en SetBranch si realmente ha cambiado la memoria
	write32(*(u32 *)((char *)PSXM(psxRegs.pc)));
	immCount = 0;

// CHUI: Establecemos el principio del bloque recompilado	
	PC_REC32(psxRegs.pc) = (u32)armPtr;
// CHUI: Iniciamos tambien pcinit para saber cuando podemos hacer unroll-loops.
	pc = pcold = pcinit = psxRegs.pc;
	branch = 0;
	rec_inloop = 0;
	rec_skips = -REC_MAX_SKIPS;
	
// CHUI: Al comenzar el cacheo de io_cycles no esta en R2
	r2_is_dirty = 1;

#ifdef DEBUG_CPU
	static double media=0.0;
	static double media2=0.0;
	static double media3=0.0;
	static unsigned cuantos=0;
	immediates=immsize=fallo_registro=ok_registro=0;
#endif
#if defined(REC_USE_2ND_PHASE) && defined(REC_USE_2ND_REG_CACHE)
	for(rec_total_opcodes=0;rec_total_opcodes<32;rec_total_opcodes++) {
		if ((rec_cache>>rec_total_opcodes)&1) {
#ifdef DEBUG_CPU
			dbgf("\tCached R%i ",rec_total_opcodes);
#endif
			ReadReg(rec_total_opcodes);
		}
	}
#endif

	cycles_pending=0;
// CHUI: Fin del bloque de compilacion cuando ciclos superan REC_MAX_OPCODES_CYCLES.
	for (rec_total_opcodes = 0; (rec_total_opcodes<REC_MAX_OPCODES) && (!branch); rec_total_opcodes+=rec_opcodes) {
//CHUI: Numero de opcodes en cada vuelta del bucle
		rec_opcodes=0;
#ifdef REC_USE_2ND_PHASE
		if (recPrev[rec_total_opcodes].jumped) {
#ifndef REC_USE_2ND_PRESERVE_CONST
			int i;
			for (i=0;i<34;i++)
				if (recPrev[rec_total_opcodes].clear_const[i])
					MapToWrite(i);
#endif
			iPutCyclesAdd(1);
			UpdateGteDelay(1);
			ResetMapGteRegs();
//			iResetRegw();
//			gen_align4();
			rec_skips = rec_total_opcodes-REC_MAX_SKIPS;
			recPrev[rec_total_opcodes].ptr=armPtr;
			memcpy((void *)&recPrev[rec_total_opcodes].regs,iRegs,sizeof(iRegs));
			r2_is_dirty=1;
		}
#endif
		psxRegs.code = *(u32 *)((char *)PSXM(pc));
		pc += 4;
		recOpcode()
// CHUI: Tenemos que controlar que nos pasamos de los 4096 max para el offset de los inmediatos
		if (immCount && !branch)
			if (((unsigned)armPtr)-immPtr[0] > 3072)
			{
				u32 *p=armPtr;
				if (rec_total_opcodes>REC_MAX_OPCODES_LIMIT) // Si falta poco para terminar salimos
					break;
#ifdef DEBUG_CPU
				dbg("\t\tForzado UpdateImmediate");
#endif
				write32(0xe1a00000); // Aqui guardaremos el salto
				write32(0xe1a00000); // NOP
				UpdateImmediate(1);  // Creamos la tabla de inmediatos
				*p=B_FWD(((u32)armPtr)-((u32)p)-8); // Establecemos el salto
			}
	}
	UpdateGteDelay(1);

	if (!branch) {
		iFlushRegs();
		MOV32ItoM_regs((u32)&psxRegs.pc, pc);
		RET_cycles();
	}
	branch = 0;
#ifdef REC_USE_2ND_PHASE
	while (recPrev_jumps) {
		recPrev_jumps--;
		u32 offset=((((u32)(armPtr) - ((u32)(recPrevJump[recPrev_jumps].ptr) + (8))) >> 2) & 0xFFFFFF); *recPrevJump[recPrev_jumps].ptr = (B_FWD_(offset));
		if (recPrev[recPrevJump[recPrev_jumps].prev].ptr && FindJump((iRegisters *)&recPrevJump[recPrev_jumps].regs,(iRegisters *)&recPrev[recPrevJump[recPrev_jumps].prev].regs)) {
			ConvertJump((iRegisters *)&recPrevJump[recPrev_jumps].regs,(iRegisters *)&recPrev[recPrevJump[recPrev_jumps].prev].regs);
			JUMPFunc((u32)recPrev[recPrevJump[recPrev_jumps].prev].ptr);
		} else {
			memcpy(iRegs,recPrevJump[recPrev_jumps].regs,sizeof(iRegs));
			iUpdateRegs(1);
			MOV32ItoR(HOST_r0,recPrevJump[recPrev_jumps].pc);MOV32RtoM_regs((u32)&psxRegs.pc,HOST_r0);
#ifdef REC_USE_RETURN_FUNCS
			JUMPFunc(func_TestBranchIfOK_ptr+4);
			UpdateImmediate(0);
#else
			recTestBranchIfOK(1);
#endif
		}
	}
#endif
	UpdateImmediate(1);
	gen_align4();
	sys_cacheflush(armPtr_old,armPtr);
#ifdef DEBUG_CPU
	cuantos++;
	double numero=((double)(((unsigned)armPtr)-((unsigned)armPtr_old)))/((double)rec_total_opcodes);
	media=((media*((double)(cuantos-1)))+numero)/((double)cuantos);
	double numero2=immediates?((double)immsize)/((double)immediates):0.0;
	media2=((media2*(((double)cuantos-1)))+numero2)/((double)cuantos);
	double oks=(ok_registro||fallo_registro)?(((double)ok_registro)*100.0)/((double)(ok_registro+fallo_registro)):100.0;
	media3=((media3*(cuantos-1))+oks)/cuantos;
	dbgf("<-%i Opcodes, Tamaño bloque=%i bytes (%.2f por opcode y %.2f de media)\n",rec_total_opcodes,((unsigned)armPtr)-((unsigned)armPtr_old),numero,media);
	dbgf("  %i Inmediatos, Usado %i bytes (%.2f por inmediato y %.2f de media)\n",immediates,immsize,numero2,media2);
	dbgf("  %.2f%% registros usados (%.2f%% de media)\n",oks,media3);
#endif
}

static void recExecute() {
	for (;;)
	{
		u32 *p = (u32*)PC_REC(psxRegs.pc);
		if (*p == 0) recRecompile();
		recRun(*p,(u32)&psxRegs);
	}
}

static void recExecuteBlock(unsigned target_pc) {
	block=target_pc;
	do{
		u32 *p = (u32*)PC_REC(psxRegs.pc);
		if (*p == 0) recRecompile();
		recRun(*p,(u32)&psxRegs);
	}while(psxRegs.pc!=target_pc);
	block=0;
}

static void recClear(u32 Addr, u32 Size) {
	unsigned *ptr=(unsigned *)PC_REC(Addr);
	for(;Size;Size--)
		*ptr++=0;
}

static void recClearDouble(u32 Addr1, u32 Addr2) {
	*((unsigned *)PC_REC(Addr1)) = 0;
	*((unsigned *)PC_REC(Addr2)) = 0;
}

R3000Acpu psxRec = {
	recInit,
	recReset,
	recExecute,
	recExecuteBlock,
	recClear,
	recShutdown
};
