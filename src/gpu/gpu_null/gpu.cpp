#include <stdio.h>
#include "psxcommon.h"
#include "port.h"
#include "plugins.h"

#define MAXSKIP			6
#define FRAME_HEIGHT            512
#define FRAME_OFFSET(x,y)       (((y)<<10)+(x))

//CHUI: No se usan pero por compatibilidad:
bool frameLimit = false; /* frames to wait */
int skipCount = 2; /* frame skip (0,1,2,3...) */
int enableAbbeyHack = 0; /* Abe's Odyssey hack */
int linesInterlace      = 0;  /* Internal lines interlace */
int linesInterlace_user = 0; /* Lines interlace 0,1,3,5,7 */
bool progressInterlace = false;
int alt_fps = 0; /* Alternative FPS algorithm */

typedef union {
	s8 S1[64];
	s16 S2[32];
	s32 S4[16];
	u8 U1[64];
	u16 U2[32];
	u32 U4[16];
} PacketBuffer_t;

u32 frameRate=60;
double frameRateAvg=0.0;
s32 framesToSkip=0;
s32 framesSkipped=0;
u32 displayFrameInfo=1; //0;
s32 frameRateCounter=0;
u32 framesTotal=0;
u32 autoFrameSkip = 0;
unsigned systicks=0;

int GPU_framesInterlace=0;
int GPU_framesProgresiveInt=1;

s32 GPU_gp0=0;
s32 GPU_gp1=0;
s32 FrameToRead=0;
s32 FrameToWrite=0;
s32 FrameWidth;
s32 FrameCount;
s32 FrameIndex;
PacketBuffer_t PacketBuffer;
s32 PacketCount;
s32 PacketIndex;
s32 isPAL = 0;
s32 TextureWindow[4];
s32 DrawingArea[4];
s32 DrawingOffset[2];
s32 DisplayArea[8];
s32 OtherEnv[16];
#if 0
s32 skip_this_frame=0;
#else
#define skip_this_frame 0
#endif
static s32 lastframerate = 0;
static s32 frameskipChange = 2;	// must be an even number so both frames in the buffer are cleared


u8 PacketSize[256] = {
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		0-15
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		16-31
	3, 3, 3, 3, 6, 6, 6, 6, 4, 4, 4, 4, 8, 8, 8, 8,	//		32-47
	5, 5, 5, 5, 8, 8, 8, 8, 7, 7, 7, 7, 11, 11, 11, 11,	//	48-63
	2, 2, 2, 2, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3,	//		64-79
	3, 3, 3, 3, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 4,	//		80-95
	2, 2, 2, 2, 3, 3, 3, 3, 1, 1, 1, 1, 2, 2, 2, 2,	//		96-111
	1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2,	//		112-127
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		128-
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		144
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//		160
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	//
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0	//
};

u16 HorizontalResolution[8] = {
	256, 368, 320, 384, 512, 512, 640, 640
};

u16 VerticalResolution[4] = {
	240, 480, 256, 480
};


static u16 _dummy_gpu_frame_buffer[(0x100000)/2];
u32 gpu_writeDmaWidth=0, gpu_writeDmaHeight=0;
u16 *gpu_frame_buffer=&_dummy_gpu_frame_buffer[0];
u16 *gpu_pvram=&_dummy_gpu_frame_buffer[0];
s32 gpu_x_start=0, gpu_y_start=0, gpu_x_end=0, gpu_y_end=0;
s32 gpu_px=0, gpu_py=0;
s32 gpu_updateLace = 1;

static u8 TextureMask[32] = {
	255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,
	127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7
};


void gpu_videoOutput(void)
{
//	gpu_updateLace = 1;
}

void gpu_reset(void)
{
	memset((void *)&_dummy_gpu_frame_buffer[0],0,0x100000);
	gpu_frame_buffer=&_dummy_gpu_frame_buffer[0];
	gpu_pvram=&_dummy_gpu_frame_buffer[0];
	gpu_px=gpu_py=0;
	gpu_x_start=gpu_y_start=0;
	gpu_x_end=gpu_y_end=0;
	gpu_writeDmaWidth=0;
	gpu_writeDmaHeight=0;
	gpu_updateLace = 1;
}

void gpu_openVideo(long p1)
{
	gpu_reset();
}

void gpu_closeVideo(void)
{
}

/* LoadImage */
void gpu_loadImage(void)
{
	u16 x0, y0, w0, h0;
	x0 = PacketBuffer.U2[2] & 1023;
	y0 = PacketBuffer.U2[3] & 511;
	w0 = PacketBuffer.U2[4];
	h0 = PacketBuffer.U2[5];
	FrameIndex = FRAME_OFFSET(x0, y0);
	if ((y0 + h0) > FRAME_HEIGHT) {
		h0 = FRAME_HEIGHT - y0;
	}
	FrameToWrite = w0 * h0;
	FrameCount = FrameWidth = w0;

	gpu_writeDmaWidth = w0;
	gpu_writeDmaHeight = h0;
	
	gpu_px = gpu_x_start = x0;
	gpu_py = gpu_y_start = y0;
	gpu_x_end = gpu_x_start + w0;
	gpu_y_end = gpu_y_start + h0;
	gpu_pvram = &gpu_frame_buffer[gpu_py*1024];

	gpu_updateLace = 1;
}

/* StoreImage */
void gpu_storeImage(void)
{
	u16 x0, y0, w0, h0;
	x0 = PacketBuffer.U2[2] & 1023;
	y0 = PacketBuffer.U2[3] & 511;
	w0 = PacketBuffer.U2[4];
	h0 = PacketBuffer.U2[5];
	FrameIndex = FRAME_OFFSET(x0, y0);
	if ((y0 + h0) > FRAME_HEIGHT) {
		h0 = FRAME_HEIGHT - y0;
	}
	FrameToRead = w0 * h0;
	FrameCount = FrameWidth = w0;

	gpu_px = gpu_x_start = x0;
	gpu_py = gpu_y_start = y0;
	gpu_x_end = gpu_x_start + w0;
	gpu_y_end = gpu_y_start + h0;
	gpu_pvram = &gpu_frame_buffer[gpu_py*1024];
	
	GPU_gp1 |= 0x08000000;
}

static __inline__ void gpuSetTexture(u16 tpage)
{
//GPU_gp1 = (GPU_gp1 & ~0x1FF) | (tpage & 0x1FF);
	GPU_gp1 = (GPU_gp1 & ~0x7FF) | (tpage & 0x7FF);
}

/*----------------------------------------------------------------------
gpuSendPacket
----------------------------------------------------------------------*/

void gpu_sendPacket(void)
{
	u32 temp;

	temp = PacketBuffer.U4[0];
	switch (temp >> 24) {
		case 0x00:
		case 0x01:
			return;
		case 0x02:
//			gpuClearImage();
			gpu_updateLace = 0;
			return;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawF3();
			}
			return;
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[4] >> 16);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawFT3();
			}
			return;
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawF3();
			}
			PacketBuffer.U4[1] = PacketBuffer.U4[4];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawF3();
			}
			return;
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[4] >> 16);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawFT3();
			}
			PacketBuffer.U4[1] = PacketBuffer.U4[7];
			PacketBuffer.U4[2] = PacketBuffer.U4[8];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawFT3();
			}
			return;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawG3();
			}
			return;
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[5] >> 16);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawGT3();
			}
			return;
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawG3();
			}
			PacketBuffer.U4[0] = PacketBuffer.U4[6];
			PacketBuffer.U4[1] = PacketBuffer.U4[7];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawG3();
			}
			return;
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[5] >> 16);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawGT3();
			}
			PacketBuffer.U4[0] = PacketBuffer.U4[9];
			PacketBuffer.U4[1] = PacketBuffer.U4[10];
			PacketBuffer.U4[2] = PacketBuffer.U4[11];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawGT3();
			}
			return;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawLF();
			}
			return;
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawLF();
			}
			if ((PacketBuffer.U4[3] & 0xF000F000) != 0x50005000) {
				PacketBuffer.U4[1] = PacketBuffer.U4[2];
				PacketBuffer.U4[2] = PacketBuffer.U4[3];
				PacketCount = 1;
				PacketIndex = 3;
			}
			return;
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawGF();
			}
			return;
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawGF();
			}
			if ((PacketBuffer.U4[4] & 0xF000F000) != 0x50005000) {
				PacketBuffer.U1[3 + (2 * 4)] =
					PacketBuffer.U1[3 + (0 * 4)];
				PacketBuffer.U4[0] = PacketBuffer.U4[2];
				PacketBuffer.U4[1] = PacketBuffer.U4[3];
				PacketBuffer.U4[2] = PacketBuffer.U4[4];
				PacketCount = 2;
				PacketIndex = 3;
			}

			return;
		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawT();
			}
			return;
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawS();
			}
			return;
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
			PacketBuffer.U4[2] = 0x00010001;
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawT();
			}
			return;
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x6F:
			PacketBuffer.U4[3] = 0x00010001;
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawS();
			}
			return;
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
			PacketBuffer.U4[2] = 0x00080008;
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawT();
			}
			return;
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
			PacketBuffer.U4[3] = 0x00080008;
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawS();
			}
			return;
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
			PacketBuffer.U4[2] = 0x00100010;
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawT();
			}
			return;
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F:
			PacketBuffer.U4[3] = 0x00100010;
//			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
//			gpuDriver = (void (*)())gpuDrivers[Masking | ((temp >> 24) & 7)];
			if (!skip_this_frame)
			{
				gpu_updateLace = 1;
//				gpuDrawS();
			}
			return;
		case 0x80:
//			gpuMoveImage();
			gpu_updateLace = 1;
			return;
		case 0xA0:
			gpu_loadImage();
			return;
		case 0xC0:
			gpu_storeImage();
			return;
		case 0xE1:
			GPU_gp1 = (GPU_gp1 & ~0x000007FF) | (temp & 0x000007FF);
			gpuSetTexture(temp);
			return;
		case 0xE2:
			TextureWindow[0] = ((temp >> 10) & 0x1F) << 3;
			TextureWindow[1] = ((temp >> 15) & 0x1F) << 3;
			TextureWindow[2] = TextureMask[(temp >> 0) & 0x1F];
			TextureWindow[3] = TextureMask[(temp >> 5) & 0x1F];
			TextureWindow[0] &= ~TextureWindow[2];
			TextureWindow[1] &= ~TextureWindow[3];
			gpuSetTexture(GPU_gp1);
			return;
		case 0xE3:
//SysMessage("E3 %x", temp); // EDIT TEMP
			DrawingArea[0] = temp & 0x3FF;
			DrawingArea[1] = (temp >> 10) & 0x3FF;
			return;
		case 0xE4:
//SysMessage("E4 %x", temp); // EDIT TEMP
			DrawingArea[2] = (temp & 0x3FF) + 1;
			DrawingArea[3] = ((temp >> 10) & 0x3FF) + 1;
			return;
		case 0xE5:
		
			// DrawingOffset[0] = temp & 0x7FF;
			// DrawingOffset[1] = (temp >> 11) & 0x7FF;
		
//SysMessage("E5 %x", temp); // EDIT TEMP
			DrawingOffset[0] = ((s32)temp<<(32-11))>>(32-11);
			DrawingOffset[1] = ((s32)temp<<(32-22))>>(32-11);
			return;
		case 0xE6:
			temp &= 3;
			GPU_gp1 = (GPU_gp1 & ~0x00001800) | (temp << 11);
//			Masking = (temp << 2) & 0x8;
//			PixelMSB = temp << 15;
			return;
	}
}

/* GPU_reset */
void GPU_reset(void)
{
	//ZeroMem(&gpuFreezeBegin,
	//		(long) &gpuFreezeEnd - (long) &gpuFreezeBegin);
	GPU_gp1 = 0x14802000;
	TextureWindow[2] = 255;
	TextureWindow[3] = 255;
	DrawingArea[2] = 256;
	DrawingArea[3] = 240;
	DisplayArea[2] = 256;
	DisplayArea[3] = 240;
	DisplayArea[6] = 256;
	DisplayArea[7] = 240;
	gpu_reset();
}


long int GPU_init(void) {
	gpu_openVideo(0);
	GPU_reset();
	return 0;
}

long int GPU_shutdown(void) {
	gpu_closeVideo();
	return 0;
}

int GPU_Open(u32 *gpu) {
	systicks=get_ticks()/1000;
	return GPU_init(); 
}

void GPU_Close(void) {
}

s32 GPU_configure(void) {
	return 0;
}

s32 GPU_test(void) {
	return 0;
}

void GPU_about(void) {
}

void GPU_makeSnapshot(void) {
}

void GPU_keypressed(s32) {
}

void GPU_displayText(s8 *) {
}

long  GPU_freeze(unsigned int bWrite, GPUFreeze_t* p2) {
	return 0;
}

void GPU_getScreenPic(u8 *) {
}

/* GPUgetMode */
s32  GPU_getMode(void)
{
	/* Support Discontinued */
	return (0);
}

/* GPUsetMode */
void  GPU_setMode(u32 p1)
{
	/* Support Discontinued */
}


static void FrameSkip(void)
{
	static u32 firstTime = 1;
	static s32 palhz = 50;
	static s32 ntschz = 60;

	static s32 pollcount = 0;
	s32 hz = (isPAL ? palhz : ntschz);
	s32 previousframeskip = framesToSkip;

	if( firstTime )
	{
		palhz = 50;
		ntschz = 60;
		framesToSkip = 0;
		frameskipChange = 2;
		lastframerate = 0;
		firstTime = 0;
		return;
	}

	if( frameRate * framesToSkip < hz )			// are we below 50/60 FPS?
	{
		pollcount++;
		if( pollcount > MAXSKIP )				// if we tried fixing the frameskip over MAXSKIP times
		{
			pollcount = 0;
			framesToSkip = 2;					// reset/reduce the frameskip
			if( isPAL ) palhz -= 5;				// reduce target framerate by half
			else ntschz -= 5;
			
			if( palhz < 10 ) palhz = 10;
			if( ntschz < 10 ) ntschz = 10;
		}
		else if( lastframerate > frameRate * framesToSkip )			// check if the previous change was effective
		{										// it wasn't effective
			frameskipChange = -frameskipChange; // reverse the direction of frameskipping
			framesToSkip += frameskipChange;	// add the framerate change
		}
		else
		{
			framesToSkip += frameskipChange;	// everything going good? keep going
			pollcount = 0;						// and reset pollcount
		}
	}
	else										// were above or equal to the correct framerate
	{
		if( palhz < 50 || ntschz < 60 )			// did we reduce the target rate before?
		{
			if( isPAL ) palhz += 5;				// then up the target framerate
			else		ntschz += 5;

			if( palhz > 50 ) palhz = 50;
			if( ntschz > 60 ) ntschz = 60;
		}
/*
		else
		{											// we already are at the target framerate
			framesToSkip -= 2;						// try going down a frame
		}
*/
		pollcount = 0;								// we made our target framerate, so resset pollcount
	}

	if( framesToSkip < 0 ) framesToSkip = 0;
	if( framesToSkip > MAXSKIP ) framesToSkip = MAXSKIP;

	lastframerate = frameRate * previousframeskip;
}





/* GPUupdateLace */
void  GPU_updateLace(void)
{
	unsigned newticks;
	unsigned diffticks = 0;	

	GPU_gp1 ^= 0x80000000;

	frameRateCounter++;
	framesTotal++;
	newticks=get_ticks()/1000;

	if( (diffticks = (newticks-systicks)) >= 1000 ) // poll every second
	{
		static unsigned cuantos=0;
		double rate=(((double)frameRateCounter) / (((double)diffticks)/1000.0));
		cuantos++;
		frameRate = (u32)rate;
		systicks += 1000;
		frameRateCounter = 0;
		frameRateAvg=((frameRateAvg*((double)(cuantos-1)))+rate)/((double)cuantos);
		// when the framerate is updated, update the autoframeskip as well
		// CHUI: Bad method, its necesary update frameskip per frame.
		if( autoFrameSkip )
		{
			FrameSkip(); // auto frameskip for FramesToSkip setting	
		}

		if( displayFrameInfo )
			printf("FrameRate: %d, Avg=%.2f\n", frameRate, frameRateAvg);
	}
}


/* GPUwriteStatus */
void GPU_writeStatus(u32 data)
{
	switch (data >> 24) {
		case 0x00:
			GPU_reset();
			break;
		case 0x01:
			GPU_gp1 &= ~0x08000000;
			PacketCount = FrameToRead = FrameToWrite = 0;
			break;
		case 0x02:
			GPU_gp1 &= ~0x08000000;
			PacketCount = FrameToRead = FrameToWrite = 0;
			break;
		case 0x03:
			GPU_gp1 = (GPU_gp1 & ~0x00800000) | ((data & 1) << 23);
			break;
		case 0x04:
			if (data == 0x04000000)
				PacketCount = 0;
			GPU_gp1 = (GPU_gp1 & ~0x60000000) | ((data & 3) << 29);
			break;
		case 0x05:
			DisplayArea[0] = data & 0x000003FF; //(short)(data & 0x3ff);
			DisplayArea[1] = (data & 0x000FFC00) >> 10; //(short)((data>>10)&0x1ff);
			break;
		case 0x06:
			DisplayArea[4] = data & 0x00000FFF; //(short)(data & 0x7ff);
			DisplayArea[6] = (data & 0x00FFF000) >> 12; //(short)((data>>12) & 0xfff);
			break;
		case 0x07:
			{		
				//int iT;
				DisplayArea[5] = data & 0x000003FF; //(short)(data & 0x3ff);
				DisplayArea[7] = (data & 0x000FFC00) >> 10; //(short)((data>>10) & 0x3ff);
				
				/*
				drawHeight = DisplayArea[7] - DisplayArea[5];

				if(isPAL) iT=48; else iT=28;

				if(DisplayArea[5]>=iT)
				{
					DisplayArea[5] = (short)(DisplayArea[5]-iT-4);
					if(DisplayArea[5]<0)
					{
						DisplayArea[5]=0;
					}
					drawHeight += DisplayArea[5];
				}
				else 
				{
					DisplayArea[5] = 0;
				}
				*/
			}
			break;
		case 0x08:
			OtherEnv[0x08] = (data >> 7) & 1;	//	reverse(?)
			GPU_gp1 =
				(GPU_gp1 & ~0x007F0000) | ((data & 0x3F) << 17) |
				((data & 0x40) << 10);

			{
				long oldResX = DisplayArea[2];
				long oldResY = DisplayArea[3];

				DisplayArea[2] = HorizontalResolution[(GPU_gp1 >> 16) & 7];
				DisplayArea[3] = VerticalResolution[(GPU_gp1 >> 19) & 3];
			}

			isPAL = (data & 0x08) ? 1 : 0; // if 1 - PAL mode, else NTSC
			break;
		case 0x09:
			OtherEnv[0x09] = data & 1;			//	gpub(?)
			break;
		case 0x10:
			switch (data & 0xffff) {
				case 0:
				case 1:
				case 3:
					GPU_gp0 = (DrawingArea[1] << 10) | DrawingArea[0];
					break;
				case 4:
					GPU_gp0 =
						((DrawingArea[3] - 1) << 10) | (DrawingArea[2] -
														1);
					break;
				case 6:
				case 5:
					GPU_gp0 = (DrawingOffset[1] << 11) | DrawingOffset[0];
					break;
				case 7:
					GPU_gp0 = 2;
					break;
				default:
					GPU_gp0 = 0;
			}
			break;
	}
}

/* GPUreadStatus */
u32  GPU_readStatus(void)
{
//	u32 ret=(GPU_gp1 | 0x1c000000) & ~0x00480000;
	u32 ret=GPU_gp1;

	return ret;
}


/* GPUwriteData */
void GPU_writeData(u32 data)
{
	GPU_gp1 &= ~0x14000000;
	if (FrameToWrite > 0) {
          gpu_pvram[gpu_px]=(u16)data;
          if (++gpu_px>=gpu_x_end) {
               gpu_px = gpu_x_start;
               gpu_pvram += 1024;
               if (++gpu_py>=gpu_y_end) {
		       FrameToWrite=0;
		       GPU_gp1 &= ~0x08000000;
	       }
          }
          if (FrameToWrite > 0) {
               gpu_pvram[gpu_px]=data>>16;
               if (++gpu_px>=gpu_x_end) {
                    gpu_px = gpu_x_start;
                    gpu_pvram += 1024;
                    if (++gpu_py>=gpu_y_end){
			    FrameToWrite=0;
			    GPU_gp1 &= ~0x08000000;
		    }
               }
          }
	} else {
		if (PacketCount) {
			PacketCount--;
			PacketBuffer.U4[PacketIndex++] = data;
		} else {
			PacketBuffer.U4[0] = data;
			PacketCount = PacketSize[data >> 24];
			PacketIndex = 1;
		}
		if (!PacketCount)
		{
			gpu_sendPacket();
		}
	}
	GPU_gp1 |= 0x14000000;
}

void GPU_writeDataMem(u32 * dmaAddress, s32 dmaCount)
{
	u32 temp, temp2;

	GPU_gp1 &= ~0x14000000;

	while (dmaCount) {
		if (FrameToWrite > 0) {
			while (dmaCount--) 
			{
				u32 data = *dmaAddress++;

				if (gpu_px<1024 && gpu_py<512)
					gpu_pvram[gpu_px] = data;
				if (++gpu_px>=gpu_x_end) 
				{
					gpu_px = gpu_x_start;
					gpu_pvram += 1024;
					if (++gpu_py>=gpu_y_end) 
					{
						FrameToWrite = 0;
						GPU_gp1 &= ~0x08000000;
						break;
					}
				}
				if (gpu_px<1024 && gpu_py<512)
					gpu_pvram[gpu_px] = data>>16;
				if (++gpu_px>=gpu_x_end) 
				{
					gpu_px = gpu_x_start;
					gpu_pvram += 1024;
					if (++gpu_py>=gpu_y_end) 
					{
						FrameToWrite = 0;
						GPU_gp1 &= ~0x08000000;
						break;
					}
				}
			}
		}
		else
		{
			temp = *dmaAddress++;
			dmaCount--;
			if (PacketCount) {
				PacketBuffer.U4[PacketIndex++] = temp;
				PacketCount--;
			} else {
				PacketBuffer.U4[0] = temp;
				PacketCount = PacketSize[temp >> 24];
				PacketIndex = 1;
			}
			if (!PacketCount)
			{
				gpu_sendPacket();
			}
		}
	}
	GPU_gp1 = (GPU_gp1 | 0x14000000) & ~0x60000000;
}

/* GPUreadData */
u32  GPU_readData(void)
{
	GPU_gp1 &= ~0x14000000;
	if (FrameToRead)
	{
		GPU_gp0 = gpu_pvram[gpu_px];
		if (++gpu_px>=gpu_x_end) {
		   gpu_px = gpu_x_start;
		   gpu_pvram += 1024;
		   if (++gpu_py>=gpu_y_end) FrameToRead=0;
		}
		GPU_gp0 |= gpu_pvram[gpu_px]<<16;
		if (++gpu_px>=gpu_x_end) {
		   gpu_px = gpu_x_start;
		   gpu_pvram +=1024;
		   if (++gpu_py>=gpu_y_end) FrameToRead=0;
		}

		if( FrameToRead == 0 ) GPU_gp1 &= ~0x08000000;
	}
	GPU_gp1 |= 0x14000000;

	return (GPU_gp0);
}

void  GPU_readDataMem(u32 * dmaAddress, s32 dmaCount)
{
	if( FrameToRead == 0 ) return;

	GPU_gp1 &= ~0x14000000;

	do 
	{
		// lower 16 bit
		u32 data = (unsigned long)gpu_pvram[gpu_px];

		if (++gpu_px>=gpu_x_end) 
		{
			gpu_px = gpu_x_start;
			gpu_pvram += 1024;
		}

		// higher 16 bit (always, even if it's an odd width)
		data |= (unsigned long)(gpu_pvram[gpu_px])<<16;
    
		*dmaAddress++ = data;

		if (++gpu_px>=gpu_x_end) 
		{
			gpu_px = gpu_x_start;
			gpu_pvram += 1024;
			if (++gpu_py>=gpu_y_end) 
			{
				FrameToRead = 0;
				GPU_gp1 &= ~0x08000000;
				break;
			}
		}
	} while (--dmaCount);

	GPU_gp1 = (GPU_gp1 | 0x14000000) & ~0x60000000;
}

long int GPU_dmaChain(u32 * baseAddr, u32 dmaVAddr)
{
	u32 temp, data, *address, count, offset;
	GPU_gp1 &= ~0x14000000;
	dmaVAddr &= 0x00FFFFFF;
	while (dmaVAddr != 0xFFFFFF) {
		address = (baseAddr + (dmaVAddr >> 2));
		data = *address++;
		count = (data >> 24);
		offset = data & 0x00FFFFFF;
		if (dmaVAddr != offset)
			dmaVAddr = offset;
		else
			dmaVAddr = 0xFFFFFF;
		while (count) {
			data = *address++;
			count--;
			//temp = PacketCount;
			if (PacketCount) {
				PacketCount--;
				PacketBuffer.U4[PacketIndex++] = data;
			} else {
				PacketBuffer.U4[0] = data;
				PacketCount = PacketSize[data >> 24];
				PacketIndex = 1;
			}
			//PacketCount = temp;
			if (!PacketCount)
			{
				gpu_sendPacket();
			}
		}
	}
	GPU_gp1 = (GPU_gp1 | 0x14000000) & ~0x60000000;

	return 0;
}
