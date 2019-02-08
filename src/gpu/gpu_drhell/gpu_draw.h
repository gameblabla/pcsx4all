/***********************************************************************
*
*	Dr.Hell's WinGDI GPU Plugin
*	Version 0.8
*	Copyright (C)Dr.Hell, 2002-2004
*
*	Drawing
*
***********************************************************************/

/*----------------------------------------------------------------------
Global
----------------------------------------------------------------------*/

Uint16*  Pixel;
Uint16	  PixelData;
Uint16*  TextureBaseAddress;
Uint16*  ClutBaseAddress;
Uint8	  TextureLocation[2];
Uint8	  LightColour[3];

/* Short Name of Global */
#define	_TBA	TextureBaseAddress
#define	_CBA	ClutBaseAddress
#define	_TU		TextureLocation[0]
#define	_TV		TextureLocation[1]
#define	_TUM	TextureWindow[2]
#define	_TVM	TextureWindow[3]
#define	_LR		LightColour[0]
#define	_LG		LightColour[1]
#define	_LB		LightColour[2]

/*----------------------------------------------------------------------
Table
----------------------------------------------------------------------*/

Uint8 TextureMask[32] = {
	255, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7,	//
	127, 7, 15, 7, 31, 7, 15, 7, 63, 7, 15, 7, 31, 7, 15, 7	//
};

/*----------------------------------------------------------------------
Lighting
----------------------------------------------------------------------*/

void gpuLighting(void)
{
	Sint32 rr, gg, bb;
	Uint16 rgb;
	rgb=PixelData;
	bb = ((rgb & 0x7C00) * _LB) >> (7 + 10);
	gg = ((rgb & 0x03E0) * _LG) >> (7 + 5);
	rr = ((rgb & 0x001F) * _LR) >> (7 + 0);
	bb -= 31;
	gg -= 31;
	rr -= 31;
	bb &= (bb >> 31);
	gg &= (gg >> 31);
	rr &= (rr >> 31);
	bb += 31;
	gg += 31;
	rr += 31;
	PixelData = (rgb & 0x8000) | (bb << 10) | (gg << 5) | (rr);
}

/*----------------------------------------------------------------------
Blending
----------------------------------------------------------------------*/

/*	0.5 x Back + 0.5 x Forward	*/
void gpuBlending00(void)
{
	*Pixel = PixelMSB | (((*Pixel & 0x7BDE) + (PixelData & 0x7BDE)) >> 1);
}

/*	1.0 x Back + 1.0 x Forward	*/
void gpuBlending01(void)
{
  Sint32 rr, gg, bb;
  Uint16 bk, fr;
	bk = *Pixel;
	fr = PixelData;
	bb = (bk & 0x7C00) + (fr & 0x7C00);
	gg = (bk & 0x03E0) + (fr & 0x03E0);
	rr = (bk & 0x001F) + (fr & 0x001F);
	bb -= 0x7C00;
	gg -= 0x03E0;
	rr -= 0x001F;
	bb &= (bb >> 31);
	gg &= (gg >> 31);
	rr &= (rr >> 31);
	bb += 0x7C00;
	gg += 0x03E0;
	rr += 0x001F;
	*Pixel = PixelMSB | bb | gg | rr;
}

/*	1.0 x Back - 1.0 x Forward	*/
void gpuBlending02(void)
{
  Sint32 rr, gg, bb;
  Uint16 bk, fr;
	bk = *Pixel;
	fr = PixelData;
	bb = (bk & 0x7C00) - (fr & 0x7C00);
	gg = (bk & 0x03E0) - (fr & 0x03E0);
	rr = (bk & 0x001F) - (fr & 0x001F);
	bb &= ~(bb >> 31);
	gg &= ~(gg >> 31);
	rr &= ~(rr >> 31);
	*Pixel = PixelMSB | bb | gg | rr;
}

/*	1.0 x Back + 0.25 x Forward	*/
void gpuBlending03(void)
{
  Sint32 rr, gg, bb;
  Uint16 bk, fr;
	bk = *Pixel;
	fr = PixelData >> 2;
	bb = (bk & 0x7C00) + (fr & 0x1C00);
	gg = (bk & 0x03E0) + (fr & 0x00E0);
	rr = (bk & 0x001F) + (fr & 0x0007);
	bb -= 0x7C00;
	gg -= 0x03E0;
	rr -= 0x001F;
	bb &= (bb >> 31);
	gg &= (gg >> 31);
	rr &= (rr >> 31);
	bb += 0x7C00;
	gg += 0x03E0;
	rr += 0x001F;
	*Pixel = PixelMSB | bb | gg | rr;
}

/*	Function Pointer	*/
typedef void (*PF)();
PF gpuBlending;
PF gpuBlendings[4] =  {
	gpuBlending00, gpuBlending01, gpuBlending02, gpuBlending03
};

/*----------------------------------------------------------------------
Texture Mapping
----------------------------------------------------------------------*/

/*	4bitCLUT	*/
void gpuTextureMapping00(void)
{
	Uint8 tu, tv;
	Uint16 rgb;
	tu = _TU & _TUM;
	tv = _TV & _TVM;
	rgb = _TBA[FRAME_OFFSET(tu >> 2, tv)];
	tu = (tu & 3) << 2;
	PixelData = _CBA[(rgb >> tu) & 15];
}

/*	8bitCLUT	*/
void gpuTextureMapping01(void)
{
	Uint8 tu, tv;
	Uint16 rgb;
	tu = _TU & _TUM;
	tv = _TV & _TVM;
	rgb = _TBA[FRAME_OFFSET(tu >> 1, tv)];
	tu = (tu & 1) << 3;
	PixelData = _CBA[(rgb >> tu) & 255];
}

/*	15bitDirect	*/
void gpuTextureMapping02(void)
{
	Uint8 tu, tv;
	tu = _TU & _TUM;
	tv = _TV & _TVM;
	PixelData = _TBA[FRAME_OFFSET(tu, tv)];
}

/*	4bitCLUT(2)	*/
void gpuTextureMapping04(void)
{
	Uint8 tu, tv;
	Uint16 rgb;

	tu = _TU;
	tv = _TV;
	rgb = _TBA[FRAME_OFFSET(tu >> 2, tv)];
	tu = (tu & 3) << 2;
	PixelData = _CBA[(rgb >> tu) & 15 ];
}

/*	8bitCLUT(2)	*/
void gpuTextureMapping05(void)
{
	Uint8 tu, tv;
	Uint16 rgb;
	tu = _TU;
	tv = _TV;
	rgb = _TBA[FRAME_OFFSET(tu >> 1, tv)];
	tu = (tu & 1) << 3;
	PixelData = _CBA[(rgb >> tu) & 255];
}

/*	15bitDirect(2)	*/
void gpuTextureMapping06(void)
{
	PixelData = _TBA[FRAME_OFFSET(_TU, _TV)];
}

/*	Function Pointer	*/
PF gpuTextureMapping;
PF gpuTextureMappings[8] =  {
	gpuTextureMapping00,
	gpuTextureMapping01,
	gpuTextureMapping02,
	gpuTextureMapping00,
	gpuTextureMapping04,
	gpuTextureMapping05,
	gpuTextureMapping06,
	gpuTextureMapping00
};

/*----------------------------------------------------------------------
Driver
----------------------------------------------------------------------*/

/* Function Pointer */
PF gpuDriver;

#define	GPU_MASKING() { if (*Pixel & 0x8000) return; }

#define	GPU_TEXTUREMAPPING() { gpuTextureMapping(); if (!PixelData) return; }

#define	GPU_LIGHTING() { gpuLighting(); }

#define	GPU_BLENDING() { gpuBlending(); }

#define	GPU_BLENDING_STP() { if (PixelData & 0x8000) { gpuBlending(); } else { *Pixel = PixelMSB | (PixelData & 0x7FFF); } }

#define	GPU_NOBLENDING() { *Pixel = PixelMSB | (PixelData & 0x7FFF); }

void gpuDriver00(void)
{
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver01(void)
{
	GPU_NOBLENDING();
}
void gpuDriver02(void)
{
	GPU_LIGHTING();
	GPU_BLENDING();
}
void gpuDriver03(void)
{
	GPU_BLENDING();
}
void gpuDriver04(void)
{
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver05(void)
{
	GPU_TEXTUREMAPPING();
	GPU_NOBLENDING();
}
void gpuDriver06(void)
{
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_BLENDING_STP();
}
void gpuDriver07(void)
{
	GPU_TEXTUREMAPPING();
	GPU_BLENDING_STP();
}
void gpuDriver08(void)
{
	GPU_MASKING();
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver09(void)
{
	GPU_MASKING();
	GPU_NOBLENDING();
}
void gpuDriver0A(void)
{
	GPU_MASKING();
	GPU_LIGHTING();
	GPU_BLENDING();
}
void gpuDriver0B(void)
{
	GPU_MASKING();
	GPU_BLENDING();
}
void gpuDriver0C(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_NOBLENDING();
}
void gpuDriver0D(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_NOBLENDING();
}
void gpuDriver0E(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_LIGHTING();
	GPU_BLENDING_STP();
}
void gpuDriver0F(void)
{
	GPU_MASKING();
	GPU_TEXTUREMAPPING();
	GPU_BLENDING_STP();
}

PF gpuDrivers[16] = {
	gpuDriver00,
	gpuDriver01,
	gpuDriver02,
	gpuDriver03,
	gpuDriver04,
	gpuDriver05,
	gpuDriver06,
	gpuDriver07,
	gpuDriver08,
	gpuDriver09,
	gpuDriver0A,
	gpuDriver0B,
	gpuDriver0C,
	gpuDriver0D,
	gpuDriver0E,
	gpuDriver0F
};

/*----------------------------------------------------------------------
Texture Setting
----------------------------------------------------------------------*/

void gpuSetTexture(Uint16 tpage)
{
	Sint32 tp;
	Sint32 tx, ty;
	GPU_gp1 = (GPU_gp1 & ~0x1FF) | (tpage & 0x1FF);
	gpuBlending = gpuBlendings[(tpage >> 5) & 3];
	tp = (tpage >> 7) & 3;
	tx = (tpage & 0x0F) << 6;
	ty = (tpage & 0x10) << 4;
	tx += (TextureWindow[0] >> (2 - tp));
	ty += TextureWindow[1];
	_TBA = &((Uint16*)FrameBuffer)[FRAME_OFFSET(tx, ty)];
	tp |= (((_TUM & _TVM) >> 5) & 4);
	gpuTextureMapping = gpuTextureMappings[tp];
}

/*----------------------------------------------------------------------
CLUT Setting
----------------------------------------------------------------------*/

void gpuSetCLUT(Uint16 clut)
{
	_CBA = &((Uint16*)FrameBuffer)[(clut & 0x7FFF) << 4];
}

/*----------------------------------------------------------------------
Primitive Drawing
----------------------------------------------------------------------*/

#include "gpu_prim.h"

/*----------------------------------------------------------------------
Image Transmission
----------------------------------------------------------------------*/

/* LoadImage */
void gpuLoadImage(void)
{
	Uint16 x0, y0, w0, h0;
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

	writeDmaWidth = w0;
	writeDmaHeight = h0;
	
	px = x_start = x0;
	py = y_start = y0;
	x_end = x_start + w0;
	y_end = y_start + h0;
	pvram = &((Uint16*)FrameBuffer)[py*1024];

	updateLace = 1;
}

/* StoreImage */
void gpuStoreImage(void)
{
	Uint16 x0, y0, w0, h0;
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

	px = x_start = x0;
	py = y_start = y0;
	x_end = x_start + w0;
	y_end = y_start + h0;
	pvram = &((Uint16*)FrameBuffer)[py*1024];
	
	GPU_gp1 |= 0x08000000;
}

/* MoveImage */
void gpuMoveImage(void)
{
	long x0, y0, x1, y1, w0, h0;
	unsigned short *lpDst, *lpSrc;
	x0 = PacketBuffer.U2[2] & 1023;
	y0 = PacketBuffer.U2[3] & 511;
	x1 = PacketBuffer.U2[4] & 1023;
	y1 = PacketBuffer.U2[5] & 511;
	w0 = PacketBuffer.U2[6];
	h0 = PacketBuffer.U2[7];
	if ((y0 + h0) > FRAME_HEIGHT)
		return;
	if ((y1 + h0) > FRAME_HEIGHT)
		return;
	lpDst = lpSrc = (Uint16*)FrameBuffer;
	lpSrc += FRAME_OFFSET(x0, y0);
	lpDst += FRAME_OFFSET(x1, y1);
	x1 = FRAME_WIDTH - w0;
	for (; h0; h0--) {
		for (x0 = w0; x0; x0--)
			*lpDst++ = *lpSrc++;
		lpDst += x1;
		lpSrc += x1;
	}
	updateLace = 1;
}

void gpuClearImage(void)
{
	long x0, y0, w0, h0;
	unsigned short *pixel, rgb;

	updateLace = 0; /* reset updateLace when an image is cleared! */

	/* for new frameskip */
	if( Skip ) return;

	x0 = PacketBuffer.S4[0];
	rgb = GPU_RGB16(x0);
	x0 = PacketBuffer.S2[2];
	y0 = PacketBuffer.S2[3];
	w0 = PacketBuffer.S2[4];
	h0 = PacketBuffer.S2[5];

	w0 += x0;
	if (x0 < 0)
		x0 = 0;
	if (w0 > FRAME_WIDTH)
		w0 = FRAME_WIDTH;
	w0 -= x0;
	if (w0 < 0)
		w0 = 0;
	h0 += y0;
	if (y0 < 0)
		y0 = 0;
	if (h0 > FRAME_HEIGHT)
		h0 = FRAME_HEIGHT;
	h0 -= y0;
	if (h0 < 0)
		h0 = 0;
	pixel = (Uint16*)FrameBuffer + FRAME_OFFSET(x0, y0);
	y0 = FRAME_WIDTH - w0;
	for (; h0; h0--) {
		for (x0 = w0; x0; x0--)
			*pixel++ = rgb;
		pixel += y0;
	}
}

/*----------------------------------------------------------------------
gpuSendPacket
----------------------------------------------------------------------*/

void gpuSendPacket(void)
{
	Uint32 temp;

	temp = PacketBuffer.U4[0];
	switch (temp >> 24) {
		case 0x00:
		case 0x01:
			return;
		case 0x02:
			gpuClearImage();
			return;
		case 0x20:
		case 0x21:
		case 0x22:
		case 0x23:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawF3();
			return;
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[4] >> 16);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawFT3();
			return;
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawF3();
			PacketBuffer.U4[1] = PacketBuffer.U4[4];
			gpuDrawF3();
			return;
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[4] >> 16);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawFT3();
			PacketBuffer.U4[1] = PacketBuffer.U4[7];
			PacketBuffer.U4[2] = PacketBuffer.U4[8];
			gpuDrawFT3();
			return;
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2)];
			gpuDrawG3();
			return;
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[5] >> 16);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawGT3();
			return;
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2)];
			gpuDrawG3();
			PacketBuffer.U4[0] = PacketBuffer.U4[6];
			PacketBuffer.U4[1] = PacketBuffer.U4[7];
			gpuDrawG3();
			return;
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(PacketBuffer.U4[5] >> 16);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawGT3();
			PacketBuffer.U4[0] = PacketBuffer.U4[9];
			PacketBuffer.U4[1] = PacketBuffer.U4[10];
			PacketBuffer.U4[2] = PacketBuffer.U4[11];
			gpuDrawGT3();
			return;
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawLF();
			return;
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
		case 0x4F:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawLF();
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
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2)];
			gpuDrawGF();
			return;
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
		case 0x5C:
		case 0x5D:
		case 0x5E:
		case 0x5F:
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2)];
			gpuDrawGF();
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
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawT();
			return;
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawS();
			return;
		case 0x68:
		case 0x69:
		case 0x6A:
		case 0x6B:
			PacketBuffer.U4[2] = 0x00010001;
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawT();
			return;
		case 0x6C:
		case 0x6D:
		case 0x6E:
		case 0x6F:
			PacketBuffer.U4[3] = 0x00010001;
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawS();
			return;
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
			PacketBuffer.U4[2] = 0x00080008;
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawT();
			return;
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
			PacketBuffer.U4[3] = 0x00080008;
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawS();
			return;
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
			PacketBuffer.U4[2] = 0x00100010;
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 2) | 1];
			gpuDrawT();
			return;
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F:
			PacketBuffer.U4[3] = 0x00100010;
			gpuSetCLUT(PacketBuffer.U4[2] >> 16);
			gpuSetTexture(GPU_gp1);
			gpuDriver = gpuDrivers[Masking | ((temp >> 24) & 7)];
			gpuDrawS();
			return;
		case 0x80:
			gpuMoveImage();
			return;
		case 0xA0:
			gpuLoadImage();
			return;
		case 0xC0:
			gpuStoreImage();
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
			DrawingArea[0] = temp & 0x3FF;
			DrawingArea[1] = (temp >> 10) & 0x3FF;
			return;
		case 0xE4:
			DrawingArea[2] = (temp & 0x3FF) + 1;
			DrawingArea[3] = ((temp >> 10) & 0x3FF) + 1;
			return;
		case 0xE5:
			DrawingOffset[0] = ((Sint32)temp<<(32-11))>>(32-11);
			DrawingOffset[1] = ((Sint32)temp<<(32-22))>>(32-11);
			return;
		case 0xE6:
			temp &= 3;
			GPU_gp1 = (GPU_gp1 & ~0x00001800) | (temp << 11);
			Masking = (temp << 2) & 0x8;
			PixelMSB = temp << 15;
			return;
	}
}
