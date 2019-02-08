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
* Plugin library callback/access functions.
*/

#include "plugins.h"
#include "psxevents.h"
#include "plugin_lib.h"

#ifdef SPU_PCSXREARMED
#include "spu/spu_pcsxrearmed/spu_config.h"
#endif

int LoadPlugins(void) {
	int ret;
	const char *cdrfilename=NULL;

	ReleasePlugins();

	LoadMcd(MCD1, Config.Mcd1); //Memcard 1
	LoadMcd(MCD2, Config.Mcd2); //Memcard 2

	ret = CDR_init();
	if (ret < 0) { printf ("Error initializing CD-ROM plugin: %d\n", ret); return -1; }

	ret = GPU_init();
	if (ret < 0) { printf ("Error initializing GPU plugin: %d\n", ret); return -1; }

	ret = SPU_init();
	if (ret < 0) { printf ("Error initializing SPU plugin: %d\n", ret); return -1; }

	cdrfilename=GetIsoFile();
	if (cdrfilename[0] != '\0') {
		ret=CDR_open();
		if (ret < 0) { printf ("Error opening CD-ROM: %s\n", cdrfilename); return -1; }
	}
	
	printf("Plugins loaded.\n");
	return 0;
}

void ReleasePlugins(void) {
	CDR_shutdown();
	GPU_shutdown();
	SPU_shutdown();
}

//////////////////////////////////////////
// SPU functions, should have C linkage //
//////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

// A generic function SPU plugins can use to set hardware SPU IRQ
void CALLBACK Trigger_SPU_IRQ(void) {
	psxHu32ref(0x1070) |= SWAPu32(0x200);
	// Ensure psxBranchTest() is called soon when IRQ is pending:
	ResetIoCycle();
}

// A generic function SPU plugins can use to schedule an update
// that scans for upcoming SPU HW IRQs. If one is encountered, above
// function will be called.
// (This is used by games that use the actual hardware SPU IRQ like
//  Need for Speed 3, Metal Gear Solid, Chrono Cross, etc.)
void CALLBACK Schedule_SPU_IRQ(unsigned int cycles_after) {
	if (Config.SpuUpdateFreq > 0 &&
	    Config.SpuUpdateFreq <= SPU_UPDATE_FREQ_MAX)
		cycles_after >>= Config.SpuUpdateFreq;

	// If frameskip is advised, do SPU IRQ more frequently to avoid dropouts
	if (pl_frameskip_advice())
		cycles_after >>= 1;

	psxEvqueueAdd(PSXINT_SPUIRQ, cycles_after);
}

#ifdef SPU_PCSXREARMED
// We provide our own private SPU_init() in plugins.cpp that will call
// spu_pcsxrearmed plugin's SPUinit() and then set its settings.
long CALLBACK SPU_init(void)
{
	//senquack - added new SPUConfig member to indicate when no configuration has been set:
	if (spu_config.iHaveConfiguration == 0) {
		printf("ERROR: SPU plugin 'spu_pcsxrearmed' configuration settings not set, aborting.\n");
		return -1;
	}

	// Should be called before SPU_open()
	if (SPUinit() < 0) {
		printf("ERROR initializing SPU plugin 'spu_pcsxrearmed'");
		return -1;
	}

	// This inits the low-level audio backend driver, i.e. OSS,ALSA,SDL,PulseAudio etc
	if (SPUopen() < 0) {
		printf("ERROR opening audio backend for SPU 'spu_pcsxrearmed'");
		return -1;
	}

	printf("-> SPU plugin 'spu_pcsxrearmed' initialized successfully.\n");

	const char* interpol_str[4] = { "none", "simple", "gaussian", "cubic" };
	assert(spu_config.iUseInterpolation >= 0 && spu_config.iUseInterpolation <= 3);
	printf("-> SPU plugin using configuration settings:\n"
		   "    Volume:             %d\n"
		   "    Disabled (-silent): %d\n"
		   "    XAPitch:            %d\n"
		   "    UseReverb:          %d\n"
		   "    UseInterpolation:   %d (%s)\n"
		   "    Tempo:              %d\n"
		   "    UseThread:          %d\n"
		   "    UseFixedUpdates:    %d\n"
		   "    SyncAudio:          %d\n",
		   spu_config.iVolume, spu_config.iDisabled, spu_config.iXAPitch, spu_config.iUseReverb,
		   spu_config.iUseInterpolation, interpol_str[spu_config.iUseInterpolation],
		   spu_config.iTempo, spu_config.iUseThread, spu_config.iUseFixedUpdates,
		   Config.SyncAudio);

	//TODO: allow nullspu backend driver of spu_pcsxrearmed to provide
	//       simulated audio sync?
	if (Config.SyncAudio && spu_config.iDisabled)
		printf("-> WARNING: '-silent' option in effect; nullspu cannot sync emu to audio (yet).\n");

	SPU_registerCallback(Trigger_SPU_IRQ);
	SPU_registerScheduleCb(Schedule_SPU_IRQ);
	//NOTE: Even PCSX Rearmed emu never calls SPU_registerCDDAVolume()

	return 0;
}
#endif //SPU_PCSXREARMED

#ifdef __cplusplus
}
#endif
