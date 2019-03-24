#include <stdint.h>
#include <string.h>
#include "port.h"
#include "r3000a.h"
#include "plugins.h"
#include "plugin_lib.h"
#include "perfmon.h"

#ifdef GPU_UNAI
  #include "gpu/gpu_unai/gpu.h"
#endif

const char CNTfix_table[25][10] =
{
	/* Vandal Hearts */
	{"SCPS45183"},
	{"SCPS45183"},
	{"SLES00204"},
	{"SLUS00447"},
	/* Vandal Hearts II */
	{"SLES02469"},
	{"SLES02497"},
	{"SLES02496"},
	{"SLUS00940"},
	{"SLPM86251"},
	{"SLPM86007"},
	/* Parasite Eve II */
	{"SLES02561"},
	{"SLES12562"},
	{"SLES02562"},
	{"SLES12560"},
	{"SLES02560"},
	{"SLES12559"},
	{"SLES02559"},
	{"SLES12558"},
	{"SLES02558"},
	{"SLUS01042"},
	{"SLUS01055"},
	{"SCPS45467"},
	{"SLPS02480"},
	{"SLPS91479"},
	{"SLPS02779"},
};

const char MemorycardHack[8][10] =
{
	/* Lifeforce Tenka, also known as Codename Tenka */
	{"SLES00613"},
	{"SLED00690"},
	{"SLES00614"},
	{"SLES00615"},
	{"SLES00616"},
	{"SLES00617"},
	{"SCUS94409"}
};

/* Function for automatic patching according to GameID.
 * It's possible that some of these games have no IDs, like some japanese games i encountered.
 * I need to check whenever this matters or not for our games.
 * (Plus it can still be activated in the menu)
 * Let's hope the IDs are also not shared with other games ! (Homebrew, don't screw it up)
 * */
void CheckforCDROMid_applyhacks()
{
	uint8_t i;
	
#ifdef GPU_UNAI
	/* Fixes Grandia JP. Need to check if the hack needs to be applied against PAL/US versions too. */
	extern uint8_t use_clip_368;
	use_clip_368 = gpu_unai_config_ext.clip_368;
	if (strncmp(CdromId, "SLPS02124", 9) == 0)
	{
		use_clip_368 = 1;
		return;
	}
#endif
	
	/* Apply hack battle fix for Inuyasha - Sengoku Otogi Kassen */
	if (strncmp(CdromId, "SLPS03503", 9) == 0)
	{
		Config.VSyncWA = 1;
		return;
	}
	
	/* Apply Memory card hack for Codename Tenka for going past the screen asking to remove MC */
	for(i=0;i<sizeof(MemorycardHack);i++)
	{
		if (strncmp(CdromId, MemorycardHack[i], 9) == 0)
		{
			Config.MemoryCardHack = 1;
		}
	}
	
	/* Apply hackfix for Parasite Eve 2, Vandal Hearts I/II */
	for(i=0;i<sizeof(CNTfix_table);i++)
	{
		if (strncmp(CdromId, CNTfix_table[i], 9) == 0)
		{
			Config.RCntFix = 1;
		}
	}
}
