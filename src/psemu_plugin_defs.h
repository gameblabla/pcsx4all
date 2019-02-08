#ifndef _PSEMU_PLUGIN_DEFS_H
#define _PSEMU_PLUGIN_DEFS_H

// header version
#define _PPDK_HEADER_VERSION		3

#define PLUGIN_VERSION				1

// DLL function return codes
#define PSE_ERR_SUCCESS				0	// every function in DLL if completed sucessfully should return this value
#define PSE_ERR_FATAL				-1	// undefined error but fatal one, that kills all functionality

// XXX_Init return values
// Those return values apply to all libraries
// currently obsolete - preserved for compatibilty

#define PSE_INIT_ERR_SUCCESS		0	// initialization went OK
#define PSE_INIT_ERR_NOTCONFIGURED	-2	// this driver is not configured
#define PSE_INIT_ERR_NOHARDWARE		-3	// this driver can not operate properly on this hardware or hardware is not detected

/*         GPU PlugIn          */

//  GPU_Test return values

// sucess, everything configured, and went OK.
#define PSE_GPU_ERR_SUCCESS			0

// ERRORS
// this error might be returned as critical error but none of below
#define PSE_GPU_ERR					-20


// this driver is not configured
#define PSE_GPU_ERR_NOTCONFIGURED	PSE_GPU_ERR - 1
// this driver failed Init
#define PSE_GPU_ERR_INIT			PSE_GPU_ERR - 2

// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_GPU_WARN				20

//  GPU_Query		- will be implemented soon

typedef struct
{
	uint32_t	flags;
	uint32_t	status;
 	void*	window;
	unsigned char reserved[100];
} gpuQueryS;

// gpuQueryS.flags
// if driver can operate in both modes it must support GPU_changeMode();
#define PSE_GPU_FLAGS_FULLSCREEN		1	// this driver can operate in fullscreen mode
#define PSE_GPU_FLAGS_WINDOWED			2	// this driver can operate in windowed mode

// gpuQueryS.status
#define PSE_GPU_STATUS_WINDOWWRONG	1	// this driver cannot operate in this windowed mode

//  GPU_Query	End	- will be implemented in v2


/*         CDR PlugIn          */

//	CDR_Test return values

// sucess, everything configured, and went OK.
#define PSE_CDR_ERR_SUCCESS			0

// general failure (error undefined)
#define PSE_CDR_ERR_FAILURE			-1

// ERRORS
#define PSE_CDR_ERR -40
// this driver is not configured
#define PSE_CDR_ERR_NOTCONFIGURED	PSE_CDR_ERR - 0
// if this driver is unable to read data from medium
#define PSE_CDR_ERR_NOREAD			PSE_CDR_ERR - 1

// WARNINGS
#define PSE_CDR_WARN 40
// if this driver emulates lame mode ie. can read only 2048 tracks and sector header is emulated
// this might happen to CDROMS that do not support RAW mode reading - surelly it will kill many games
#define PSE_CDR_WARN_LAMECD			PSE_CDR_WARN + 0




/*         SPU PlugIn          */

// some info retricted (now!)

// sucess, everything configured, and went OK.
#define PSE_SPU_ERR_SUCCESS 0

// ERRORS
// this error might be returned as critical error but none of below
#define PSE_SPU_ERR					-60

// this driver is not configured
#define PSE_SPU_ERR_NOTCONFIGURED	PSE_SPU_ERR - 1
// this driver failed Init
#define PSE_SPU_ERR_INIT			PSE_SPU_ERR - 2


// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_SPU_WARN				60

#endif // _PSEMU_PLUGIN_DEFS_H
