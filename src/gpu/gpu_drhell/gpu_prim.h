/***********************************************************************
*
*	Dr.Hell's WinGDI GPU Plugin
*	Veorsion 0.8
*	Copyright (C)Dr.Hell, 2002-2004
*
*	Primitive Drawing
*
***********************************************************************/

/*----------------------------------------------------------------------
Constant
----------------------------------------------------------------------*/

#define	GPU_DIGITS	10

/*----------------------------------------------------------------------
Macro
----------------------------------------------------------------------*/

#define	GPU_SWAP(x,y,z)	{(z)=(x);(x)=(y);(y)=(z);}

#define	GPU_TESTRANGE(x) { if ((Uint32)((x) + 1024) > (Uint32)2047) return; }

INLINE Sint32 GPU_DIV(Sint32 ors, Sint32 ort)
{
	if(ort) return (ors / ort);
	else return (0);
}

/*----------------------------------------------------------------------
F3
----------------------------------------------------------------------*/

void gpuDrawF3(void)
{
	Sint32 temp, loop0, loop1, loop2;
	Sint32 xa, xb, xmin, xmax;
	Sint32 ya, yb, ymin, ymax;
	Sint32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	Sint32 y0, y1, y2;

	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	GPU_TESTRANGE(x0);
	x1 = PacketBuffer.S2[4];
	GPU_TESTRANGE(x1);
	x2 = PacketBuffer.S2[6];
	GPU_TESTRANGE(x2);
	y0 = PacketBuffer.S2[3];
	GPU_TESTRANGE(y0);
	y1 = PacketBuffer.S2[5];
	GPU_TESTRANGE(y1);
	y2 = PacketBuffer.S2[7];
	GPU_TESTRANGE(x2);
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	temp = PacketBuffer.U4[0];
	PixelData = GPU_RGB16(temp);
	temp = DrawingOffset[0];
	x0 += temp;
	x1 += temp;
	x2 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	y1 += temp;
	y2 += temp;
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
	}
	if (y1 > y2) {
		GPU_SWAP(x1, x2, temp);
		GPU_SWAP(y1, y2, temp);
	}
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
	}
	ya = y2 - y0;
	yb = y2 - y1;
	dx = (x2 - x1) * ya - (x2 - x0) * yb;
	for (loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;
			yb = y1;
			x3 = x4 = x0 << GPU_DIGITS;
			if (dx < 0) {
				temp = y2 - y0;
				dx3 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
				temp = y1 - y0;
				dx4 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
			} else {
				temp = y1 - y0;
				dx3 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
				temp = y2 - y0;
				dx4 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
			}
		} else {
			ya = y1;
			yb = y2;
			if (dx < 0) {
				temp = y1 - y0;
				x3 = (x0 << GPU_DIGITS) + (dx3 * temp);
				x4 = x1 << GPU_DIGITS;
				temp = y2 - y1;
				dx4 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
			} else {
				x3 = x1 << GPU_DIGITS;
				temp = y1 - y0;
				x4 = (x0 << GPU_DIGITS) + (dx4 * temp);
				temp = y2 - y1;
				dx3 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
			}
		}
		temp = ymin - ya;
		if (temp > 0) {
			ya = ymin;
			x3 += (dx3 * temp);
			x4 += (dx4 * temp);
		}
		if (yb > ymax)
			yb = ymax;
		loop1 = yb - ya;
		if (loop1 < 0)
			loop1 = 0;
		for (; loop1; loop1--) {
			xa = (x3 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			xb = (x4 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			temp = xmin - xa;
			if (temp > 0)
				xa = xmin;
			if (xb > xmax)
				xb = xmax;
			loop2 = xb - xa;
			if (loop2 < 0)
				loop2 = 0;
			else
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(xa, ya)];
			ya++;
			for (; loop2; loop2--) {
				gpuDriver();
				Pixel++;
			}
			x3 += dx3;
			x4 += dx4;
		}
	}
}

/*----------------------------------------------------------------------
FT3
----------------------------------------------------------------------*/

void gpuDrawFT3(void)
{
	Sint32 temp, loop0, loop1, loop2;
	Sint32 xa, xb, xmin, xmax;
	Sint32 ya, yb, ymin, ymax;
	Sint32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	Sint32 y0, y1, y2;
	Sint32 u0, u1, u2, u3, du3, u4, du4;
	Sint32 v0, v1, v2, v3, dv3, v4, dv4;
	
	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	GPU_TESTRANGE(x0);
	x1 = PacketBuffer.S2[6];
	GPU_TESTRANGE(x1);
	x2 = PacketBuffer.S2[10];
	GPU_TESTRANGE(x2);
	y0 = PacketBuffer.S2[3];
	GPU_TESTRANGE(y0);
	y1 = PacketBuffer.S2[7];
	GPU_TESTRANGE(y1);
	y2 = PacketBuffer.S2[11];
	GPU_TESTRANGE(y2);
	u0 = PacketBuffer.U1[8];
	u1 = PacketBuffer.U1[16];
	u2 = PacketBuffer.U1[24];
	v0 = PacketBuffer.U1[9];
	v1 = PacketBuffer.U1[17];
	v2 = PacketBuffer.U1[25];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	_LR = PacketBuffer.U1[0];
	_LG = PacketBuffer.U1[1];
	_LB = PacketBuffer.U1[2];
	temp = DrawingOffset[0];
	x0 += temp;
	x1 += temp;
	x2 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	y1 += temp;
	y2 += temp;
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
		GPU_SWAP(u0, u1, temp);
		GPU_SWAP(v0, v1, temp);
	}
	if (y1 > y2) {
		GPU_SWAP(x1, x2, temp);
		GPU_SWAP(y1, y2, temp);
		GPU_SWAP(u1, u2, temp);
		GPU_SWAP(v1, v2, temp);
	}
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
		GPU_SWAP(u0, u1, temp);
		GPU_SWAP(v0, v1, temp);
	}
	ya = y2 - y0;
	yb = y2 - y1;
	dx4 = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;
	dx = dx4;
	if (dx4 < 0) {
		dx4 = -dx4;
		du4 = -du4;
		dv4 = -dv4;
	}
	du4 = GPU_DIV(du4 << GPU_DIGITS, dx4);
	dv4 = GPU_DIV(dv4 << GPU_DIGITS, dx4);
	for (loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;
			yb = y1;
			x3 = x4 = x0 << GPU_DIGITS;
			u3 = u0 << GPU_DIGITS;
			v3 = v0 << GPU_DIGITS;
			if (dx < 0) {
				temp = y2 - y0;
				dx3 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
				du3 = GPU_DIV((u2 - u0) << GPU_DIGITS, temp);
				dv3 = GPU_DIV((v2 - v0) << GPU_DIGITS, temp);
				temp = y1 - y0;
				dx4 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
			} else {
				temp = y1 - y0;
				dx3 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
				du3 = GPU_DIV((u1 - u0) << GPU_DIGITS, temp);
				dv3 = GPU_DIV((v1 - v0) << GPU_DIGITS, temp);
				temp = y2 - y0;
				dx4 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
			}
		} else {
			ya = y1;
			yb = y2;
			if (dx < 0) {
				temp = y1 - y0;
				x3 = (x0 << GPU_DIGITS) + (dx3 * temp);
				u3 = (u0 << GPU_DIGITS) + (du3 * temp);
				v3 = (v0 << GPU_DIGITS) + (dv3 * temp);
				x4 = x1 << GPU_DIGITS;
				temp = y2 - y1;
				dx4 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
			} else {
				x3 = x1 << GPU_DIGITS;
				u3 = u1 << GPU_DIGITS;
				v3 = v1 << GPU_DIGITS;
				temp = y1 - y0;
				x4 = (x0 << GPU_DIGITS) + (dx4 * temp);
				temp = y2 - y1;
				dx3 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
				du3 = GPU_DIV((u2 - u1) << GPU_DIGITS, temp);
				dv3 = GPU_DIV((v2 - v1) << GPU_DIGITS, temp);
			}
		}
		temp = ymin - ya;
		if (temp > 0) {
			ya = ymin;
			x3 += (dx3 * temp);
			x4 += (dx4 * temp);
			u3 += (du3 * temp);
			v3 += (dv3 * temp);
		}
		if (yb > ymax)
			yb = ymax;
		loop1 = yb - ya;
		if (loop1 < 0)
			loop1 = 0;
		for (; loop1; loop1--) {
			u4 = u3;
			v4 = v3;
			xa = (x3 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			xb = (x4 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			temp = (xa << GPU_DIGITS) - x3;
			u4 += ((temp * du4) >> GPU_DIGITS);
			v4 += ((temp * dv4) >> GPU_DIGITS);
			u4 += (1 << (GPU_DIGITS - 1));
			v4 += (1 << (GPU_DIGITS - 1));
			temp = xmin - xa;
			if (temp > 0) {
				xa = xmin;
				u4 += (du4 * temp);
				v4 += (dv4 * temp);
			}
			if (xb > xmax)
				xb = xmax;
			loop2 = xb - xa;
			if (loop2 < 0)
				loop2 = 0;
			else
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(xa, ya)];
			ya++;
			for (; loop2; loop2--) {
				_TU = u4 >> GPU_DIGITS;
				_TV = v4 >> GPU_DIGITS;
				gpuDriver();
				Pixel++;
				u4 += du4;
				v4 += dv4;
			}
			x3 += dx3;
			x4 += dx4;
			u3 += du3;
			v3 += dv3;
		}
	}
}

/*----------------------------------------------------------------------
G3
----------------------------------------------------------------------*/

void gpuDrawG3(void)
{
	Sint32 temp, loop0, loop1, loop2;
	Sint32 xa, xb, xmin, xmax;
	Sint32 ya, yb, ymin, ymax;
	Sint32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	Sint32 y0, y1, y2;
	Sint32 r0, r1, r2, r3, dr3, r4, dr4;
	Sint32 g0, g1, g2, g3, dg3, g4, dg4;
	Sint32 b0, b1, b2, b3, db3, b4, db4;
	
	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	GPU_TESTRANGE(x0);
	x1 = PacketBuffer.S2[6];
	GPU_TESTRANGE(x1);
	x2 = PacketBuffer.S2[10];
	GPU_TESTRANGE(x2);
	y0 = PacketBuffer.S2[3];
	GPU_TESTRANGE(y0);
	y1 = PacketBuffer.S2[7];
	GPU_TESTRANGE(y1);
	y2 = PacketBuffer.S2[11];
	GPU_TESTRANGE(y2);
	r0 = PacketBuffer.U1[0];
	r1 = PacketBuffer.U1[8];
	r2 = PacketBuffer.U1[16];
	g0 = PacketBuffer.U1[1];
	g1 = PacketBuffer.U1[9];
	g2 = PacketBuffer.U1[17];
	b0 = PacketBuffer.U1[2];
	b1 = PacketBuffer.U1[10];
	b2 = PacketBuffer.U1[18];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	temp = DrawingOffset[0];
	x0 += temp;
	x1 += temp;
	x2 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	y1 += temp;
	y2 += temp;
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
		GPU_SWAP(r0, r1, temp);
		GPU_SWAP(g0, g1, temp);
		GPU_SWAP(b0, b1, temp);
	}
	if (y1 > y2) {
		GPU_SWAP(x1, x2, temp);
		GPU_SWAP(y1, y2, temp);
		GPU_SWAP(r1, r2, temp);
		GPU_SWAP(g1, g2, temp);
		GPU_SWAP(b1, b2, temp);
	}
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
		GPU_SWAP(r0, r1, temp);
		GPU_SWAP(g0, g1, temp);
		GPU_SWAP(b0, b1, temp);
	}
	ya = y2 - y0;
	yb = y2 - y1;
	dx4 = (x2 - x1) * ya - (x2 - x0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;
	dx = dx4;
	if (dx4 < 0) {
		dx4 = -dx4;
		dr4 = -dr4;
		dg4 = -dg4;
		db4 = -db4;
	}
	dr4 = GPU_DIV(dr4 << GPU_DIGITS, dx4);
	dg4 = GPU_DIV(dg4 << GPU_DIGITS, dx4);
	db4 = GPU_DIV(db4 << GPU_DIGITS, dx4);
	for (loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;
			yb = y1;
			x3 = x4 = x0 << GPU_DIGITS;
			r3 = r0 << GPU_DIGITS;
			g3 = g0 << GPU_DIGITS;
			b3 = b0 << GPU_DIGITS;
			if (dx < 0) {
				temp = y2 - y0;
				dx3 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
				dr3 = GPU_DIV((r2 - r0) << GPU_DIGITS, temp);
				dg3 = GPU_DIV((g2 - g0) << GPU_DIGITS, temp);
				db3 = GPU_DIV((b2 - b0) << GPU_DIGITS, temp);
				temp = y1 - y0;
				dx4 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
			} else {
				temp = y1 - y0;
				dx3 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
				dr3 = GPU_DIV((r1 - r0) << GPU_DIGITS, temp);
				dg3 = GPU_DIV((g1 - g0) << GPU_DIGITS, temp);
				db3 = GPU_DIV((b1 - b0) << GPU_DIGITS, temp);
				temp = y2 - y0;
				dx4 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
			}
		} else {
			ya = y1;
			yb = y2;
			if (dx < 0) {
				temp = y1 - y0;
				x3 = (x0 << GPU_DIGITS) + (dx3 * temp);
				r3 = (r0 << GPU_DIGITS) + (dr3 * temp);
				g3 = (g0 << GPU_DIGITS) + (dg3 * temp);
				b3 = (b0 << GPU_DIGITS) + (db3 * temp);
				x4 = x1 << GPU_DIGITS;
				temp = y2 - y1;
				dx4 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
			} else {
				x3 = x1 << GPU_DIGITS;
				r3 = r1 << GPU_DIGITS;
				g3 = g1 << GPU_DIGITS;
				b3 = b1 << GPU_DIGITS;
				temp = y1 - y0;
				x4 = (x0 << GPU_DIGITS) + (dx4 * temp);
				temp = y2 - y1;
				dx3 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
				dr3 = GPU_DIV((r2 - r1) << GPU_DIGITS, temp);
				dg3 = GPU_DIV((g2 - g1) << GPU_DIGITS, temp);
				db3 = GPU_DIV((b2 - b1) << GPU_DIGITS, temp);
			}
		}
		temp = ymin - ya;
		if (temp > 0) {
			ya = ymin;
			x3 += (dx3 * temp);
			x4 += (dx4 * temp);
			r3 += (dr3 * temp);
			g3 += (dg3 * temp);
			b3 += (db3 * temp);
		}
		if (yb > ymax)
			yb = ymax;
		loop1 = yb - ya;
		if (loop1 < 0)
			loop1 = 0;
		for (; loop1; loop1--) {
			r4 = r3;
			g4 = g3;
			b4 = b3;
			xa = (x3 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			xb = (x4 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			temp = (xa << GPU_DIGITS) - x3;
			r4 += ((temp * dr4) >> GPU_DIGITS);
			g4 += ((temp * dg4) >> GPU_DIGITS);
			b4 += ((temp * db4) >> GPU_DIGITS);
			r4 += (1 << (GPU_DIGITS - 1));
			g4 += (1 << (GPU_DIGITS - 1));
			b4 += (1 << (GPU_DIGITS - 1));
			temp = xmin - xa;
			if (temp > 0) {
				xa = xmin;
				r4 += (dr4 * temp);
				g4 += (dg4 * temp);
				b4 += (db4 * temp);
			}
			if (xb > xmax)
				xb = xmax;
			loop2 = xb - xa;
			if (loop2 < 0)
				loop2 = 0;
			else
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(xa, ya)];
			ya++;
			for (; loop2; loop2--) {
				PixelData = (0xF << 10) | (0xF << 5) | 0xF;
				_LR = r4 >> GPU_DIGITS;
				_LG = g4 >> GPU_DIGITS;
				_LB = b4 >> GPU_DIGITS;
				gpuDriver();
				Pixel++;
				r4 += dr4;
				g4 += dg4;
				b4 += db4;
			}
			x3 += dx3;
			x4 += dx4;
			r3 += dr3;
			g3 += dg3;
			b3 += db3;
		}
	}
}

/*----------------------------------------------------------------------
GT3
----------------------------------------------------------------------*/

void gpuDrawGT3(void)
{
	Sint32 temp, loop0, loop1, loop2;
	Sint32 xa, xb, xmin, xmax;
	Sint32 ya, yb, ymin, ymax;
	Sint32 x0, x1, x2, x3, dx3, x4, dx4, dx;
	Sint32 y0, y1, y2;
	Sint32 u0, u1, u2, u3, du3, u4, du4;
	Sint32 v0, v1, v2, v3, dv3, v4, dv4;
	Sint32 r0, r1, r2, r3, dr3, r4, dr4;
	Sint32 g0, g1, g2, g3, dg3, g4, dg4;
	Sint32 b0, b1, b2, b3, db3, b4, db4;
	
	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	GPU_TESTRANGE(x0);
	x1 = PacketBuffer.S2[8];
	GPU_TESTRANGE(x1);
	x2 = PacketBuffer.S2[14];
	GPU_TESTRANGE(x2);
	y0 = PacketBuffer.S2[3];
	GPU_TESTRANGE(y0);
	y1 = PacketBuffer.S2[9];
	GPU_TESTRANGE(y1);
	y2 = PacketBuffer.S2[15];
	GPU_TESTRANGE(y2);
	u0 = PacketBuffer.U1[8];
	u1 = PacketBuffer.U1[20];
	u2 = PacketBuffer.U1[32];
	v0 = PacketBuffer.U1[9];
	v1 = PacketBuffer.U1[21];
	v2 = PacketBuffer.U1[33];
	r0 = PacketBuffer.U1[0];
	r1 = PacketBuffer.U1[12];
	r2 = PacketBuffer.U1[24];
	g0 = PacketBuffer.U1[1];
	g1 = PacketBuffer.U1[13];
	g2 = PacketBuffer.U1[25];
	b0 = PacketBuffer.U1[2];
	b1 = PacketBuffer.U1[14];
	b2 = PacketBuffer.U1[26];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	temp = DrawingOffset[0];
	x0 += temp;
	x1 += temp;
	x2 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	y1 += temp;
	y2 += temp;
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
		GPU_SWAP(u0, u1, temp);
		GPU_SWAP(v0, v1, temp);
		GPU_SWAP(r0, r1, temp);
		GPU_SWAP(g0, g1, temp);
		GPU_SWAP(b0, b1, temp);
	}
	if (y1 > y2) {
		GPU_SWAP(x1, x2, temp);
		GPU_SWAP(y1, y2, temp);
		GPU_SWAP(u1, u2, temp);
		GPU_SWAP(v1, v2, temp);
		GPU_SWAP(r1, r2, temp);
		GPU_SWAP(g1, g2, temp);
		GPU_SWAP(b1, b2, temp);
	}
	if (y0 > y1) {
		GPU_SWAP(x0, x1, temp);
		GPU_SWAP(y0, y1, temp);
		GPU_SWAP(u0, u1, temp);
		GPU_SWAP(v0, v1, temp);
		GPU_SWAP(r0, r1, temp);
		GPU_SWAP(g0, g1, temp);
		GPU_SWAP(b0, b1, temp);
	}
	ya = y2 - y0;
	yb = y2 - y1;
	dx4 = (x2 - x1) * ya - (x2 - x0) * yb;
	du4 = (u2 - u1) * ya - (u2 - u0) * yb;
	dv4 = (v2 - v1) * ya - (v2 - v0) * yb;
	dr4 = (r2 - r1) * ya - (r2 - r0) * yb;
	dg4 = (g2 - g1) * ya - (g2 - g0) * yb;
	db4 = (b2 - b1) * ya - (b2 - b0) * yb;
	dx = dx4;
	if (dx4 < 0) {
		dx4 = -dx4;
		du4 = -du4;
		dv4 = -dv4;
		dr4 = -dr4;
		dg4 = -dg4;
		db4 = -db4;
	}

	du4 = GPU_DIV(du4 << GPU_DIGITS, dx4);
	dv4 = GPU_DIV(dv4 << GPU_DIGITS, dx4);
	dr4 = GPU_DIV(dr4 << GPU_DIGITS, dx4);
	dg4 = GPU_DIV(dg4 << GPU_DIGITS, dx4);
	db4 = GPU_DIV(db4 << GPU_DIGITS, dx4);
	for (loop0 = 2; loop0; loop0--) {
		if (loop0 == 2) {
			ya = y0;
			yb = y1;
			x3 = x4 = x0 << GPU_DIGITS;
			u3 = u0 << GPU_DIGITS;
			v3 = v0 << GPU_DIGITS;
			r3 = r0 << GPU_DIGITS;
			g3 = g0 << GPU_DIGITS;
			b3 = b0 << GPU_DIGITS;
			if (dx < 0) {
				temp = y2 - y0;
				dx3 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
				du3 = GPU_DIV((u2 - u0) << GPU_DIGITS, temp);
				dv3 = GPU_DIV((v2 - v0) << GPU_DIGITS, temp);
				dr3 = GPU_DIV((r2 - r0) << GPU_DIGITS, temp);
				dg3 = GPU_DIV((g2 - g0) << GPU_DIGITS, temp);
				db3 = GPU_DIV((b2 - b0) << GPU_DIGITS, temp);
				temp = y1 - y0;
				dx4 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
			} else {
				temp = y1 - y0;
				dx3 = GPU_DIV((x1 - x0) << GPU_DIGITS, temp);
				du3 = GPU_DIV((u1 - u0) << GPU_DIGITS, temp);
				dv3 = GPU_DIV((v1 - v0) << GPU_DIGITS, temp);
				dr3 = GPU_DIV((r1 - r0) << GPU_DIGITS, temp);
				dg3 = GPU_DIV((g1 - g0) << GPU_DIGITS, temp);
				db3 = GPU_DIV((b1 - b0) << GPU_DIGITS, temp);
				temp = y2 - y0;
				dx4 = GPU_DIV((x2 - x0) << GPU_DIGITS, temp);
			}
		} else {
			ya = y1;
			yb = y2;
			if (dx < 0) {
				temp = y1 - y0;
				x3 = (x0 << GPU_DIGITS) + (dx3 * temp);
				u3 = (u0 << GPU_DIGITS) + (du3 * temp);
				v3 = (v0 << GPU_DIGITS) + (dv3 * temp);
				r3 = (r0 << GPU_DIGITS) + (dr3 * temp);
				g3 = (g0 << GPU_DIGITS) + (dg3 * temp);
				b3 = (b0 << GPU_DIGITS) + (db3 * temp);
				x4 = x1 << GPU_DIGITS;
				temp = y2 - y1;
				dx4 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
			} else {
				x3 = x1 << GPU_DIGITS;
				u3 = u1 << GPU_DIGITS;
				v3 = v1 << GPU_DIGITS;
				r3 = r1 << GPU_DIGITS;
				g3 = g1 << GPU_DIGITS;
				b3 = b1 << GPU_DIGITS;
				temp = y1 - y0;
				x4 = (x0 << GPU_DIGITS) + (dx4 * temp);
				temp = y2 - y1;
				dx3 = GPU_DIV((x2 - x1) << GPU_DIGITS, temp);
				du3 = GPU_DIV((u2 - u1) << GPU_DIGITS, temp);
				dv3 = GPU_DIV((v2 - v1) << GPU_DIGITS, temp);
				dr3 = GPU_DIV((r2 - r1) << GPU_DIGITS, temp);
				dg3 = GPU_DIV((g2 - g1) << GPU_DIGITS, temp);
				db3 = GPU_DIV((b2 - b1) << GPU_DIGITS, temp);
			}
		}
		temp = ymin - ya;
		if (temp > 0) {
			ya = ymin;
			x3 += (dx3 * temp);
			x4 += (dx4 * temp);
			u3 += (du3 * temp);
			v3 += (dv3 * temp);
			r3 += (dr3 * temp);
			g3 += (dg3 * temp);
			b3 += (db3 * temp);
		}
		if (yb > ymax)
			yb = ymax;
		loop1 = yb - ya;
		if (loop1 < 0)
			loop1 = 0;
		for (; loop1; loop1--) {
			u4 = u3;
			v4 = v3;
			r4 = r3;
			g4 = g3;
			b4 = b3;
			xa = (x3 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			xb = (x4 + ((1 << GPU_DIGITS) - 1)) >> GPU_DIGITS;
			temp = (xa << GPU_DIGITS) - x3;
			u4 += ((temp * du4) >> GPU_DIGITS);
			v4 += ((temp * dv4) >> GPU_DIGITS);
			r4 += ((temp * dr4) >> GPU_DIGITS);
			g4 += ((temp * dg4) >> GPU_DIGITS);
			b4 += ((temp * db4) >> GPU_DIGITS);
			u4 += (1 << (GPU_DIGITS - 1));
			v4 += (1 << (GPU_DIGITS - 1));
			r4 += (1 << (GPU_DIGITS - 1));
			g4 += (1 << (GPU_DIGITS - 1));
			b4 += (1 << (GPU_DIGITS - 1));
			temp = xmin - xa;
			if (temp > 0) {
				xa = xmin;
				u4 += (du4 * temp);
				v4 += (dv4 * temp);
				r4 += (dr4 * temp);
				g4 += (dg4 * temp);
				b4 += (db4 * temp);
			}
			if (xb > xmax)
				xb = xmax;
			loop2 = xb - xa;
			if (loop2 < 0)
				loop2 = 0;
			else
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(xa, ya)];
			ya++;
			for (; loop2; loop2--) {
				_TU = u4 >> GPU_DIGITS;
				_TV = v4 >> GPU_DIGITS;
				_LR = r4 >> GPU_DIGITS;
				_LG = g4 >> GPU_DIGITS;
				_LB = b4 >> GPU_DIGITS;
				gpuDriver();
				Pixel++;
				u4 += du4;
				v4 += dv4;
				r4 += dr4;
				g4 += dg4;
				b4 += db4;
			}
			x3 += dx3;
			x4 += dx4;
			u3 += du3;
			v3 += dv3;
			r3 += dr3;
			g3 += dg3;
			b3 += db3;
		}
	}
}

/*----------------------------------------------------------------------
LF
----------------------------------------------------------------------*/

void gpuDrawLF(void)
{
	Sint32 temp;
	Sint32 xmin, xmax;
	Sint32 ymin, ymax;
	Sint32 x0, x1, dx;
	Sint32 y0, y1, dy;
	
	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	GPU_TESTRANGE(x0);
	x1 = PacketBuffer.S2[4];
	GPU_TESTRANGE(x1);
	y0 = PacketBuffer.S2[3];
	GPU_TESTRANGE(y0);
	y1 = PacketBuffer.S2[5];
	GPU_TESTRANGE(y1);
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	temp = PacketBuffer.U4[0];
	PixelData = GPU_RGB16(temp);
	temp = DrawingOffset[0];
	x0 += temp;
	x1 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	y1 += temp;
	dy = (y1 - y0);
	if (dy < 0)
		dy = -dy;
	dx = (x1 - x0);
	if (dx < 0)
		dx = -dx;
	if (dx > dy) {
		if (x0 > x1) {
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
		}
		y1 = GPU_DIV((y1 - y0) << GPU_DIGITS, dx);
		y0 <<= GPU_DIGITS;
		temp = xmin - x0;
		if (temp > 0) {
			x0 = xmin;
			y0 += (y1 * temp);
		}
		if (x1 > xmax)
			x1 = xmax;
		x1 -= x0;
		if (x1 < 0)
			x1 = 0;
		for (; x1; x1--) {
			temp = y0 >> GPU_DIGITS;
			if ((Uint32) (temp - ymin) < (Uint32) (ymax - ymin)) {
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0, temp)];
				gpuDriver();
			}
			x0++;
			y0 += y1;
		}
	} else if (dy) {
		if (y0 > y1) {
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
		}
		x1 = GPU_DIV((x1 - x0) << GPU_DIGITS, dy);
		x0 <<= GPU_DIGITS;
		temp = ymin - y0;
		if (temp > 0) {
			y0 = ymin;
			x0 += (x1 * temp);
		}
		if (y1 > ymax)
			y1 = ymax;
		y1 -= y0;
		if (y1 < 0)
			y1 = 0;
		for (; y1; y1--) {
			temp = x0 >> GPU_DIGITS;
			if ((Uint32) (temp - xmin) < (Uint32) (xmax - xmin)) {
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(temp, y0)];
				gpuDriver();
			}
			y0++;
			x0 += x1;
		}
	} else {
		if ((Uint32) (x0 - xmin) < (Uint32) (xmax - xmin)) {
			if ((Uint32) (y0 - ymin) < (Uint32) (ymax - ymin)) {
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0, y0)];
				gpuDriver();
			}
		}
	}
}

/*----------------------------------------------------------------------
GF
----------------------------------------------------------------------*/

void gpuDrawGF(void)
{
	long temp;
	long xmin, xmax;
	long ymin, ymax;
	long x0, x1, dx;
	long y0, y1, dy;
	long r0, r1;
	long g0, g1;
	long b0, b1;
	
	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	GPU_TESTRANGE(x0);
	x1 = PacketBuffer.S2[6];
	GPU_TESTRANGE(x1);
	y0 = PacketBuffer.S2[3];
	GPU_TESTRANGE(y0);
	y1 = PacketBuffer.S2[7];
	GPU_TESTRANGE(y1);
	r0 = PacketBuffer.U1[0];
	r1 = PacketBuffer.U1[8];
	g0 = PacketBuffer.U1[1];
	g1 = PacketBuffer.U1[9];
	b0 = PacketBuffer.U1[2];
	b1 = PacketBuffer.U1[10];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	temp = DrawingOffset[0];
	x0 += temp;
	x1 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	y1 += temp;
	dy = (y1 - y0);
	if (dy < 0)
		dy = -dy;
	dx = (x1 - x0);
	if (dx < 0)
		dx = -dx;
	if (dx > dy) {
		if (x0 > x1) {
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
			GPU_SWAP(r0, r1, temp);
			GPU_SWAP(g0, g1, temp);
			GPU_SWAP(b0, b1, temp);
		}
		y1 = GPU_DIV((y1 - y0) << GPU_DIGITS, dx);
		r1 = GPU_DIV((r1 - r0) << GPU_DIGITS, dx);
		g1 = GPU_DIV((g1 - g0) << GPU_DIGITS, dx);
		b1 = GPU_DIV((b1 - b0) << GPU_DIGITS, dx);
		y0 <<= GPU_DIGITS;
		r0 <<= GPU_DIGITS;
		g0 <<= GPU_DIGITS;
		b0 <<= GPU_DIGITS;
		temp = xmin - x0;
		if (temp > 0) {
			x0 = xmin;
			y0 += (y1 * temp);
			r0 += (r1 * temp);
			g0 += (g1 * temp);
			b0 += (b1 * temp);
		}
		if (x1 > xmax)
			x1 = xmax;
		x1 -= x0;
		if (x1 < 0)
			x1 = 0;
		for (; x1; x1--) {
			temp = y0 >> GPU_DIGITS;
			if ((Uint32) (temp - ymin) < (Uint32) (ymax - ymin)) {
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0, temp)];
				PixelData = (0xF << 10) | (0xF << 5) | 0xF;
				_LR = r0 >> GPU_DIGITS;
				_LG = g0 >> GPU_DIGITS;
				_LB = b0 >> GPU_DIGITS;
				gpuDriver();
			}
			x0++;
			y0 += y1;
			r0 += r1;
			g0 += g1;
			b0 += b1;
		}
	} else if (dy) {
		if (y0 > y1) {
			GPU_SWAP(x0, x1, temp);
			GPU_SWAP(y0, y1, temp);
			GPU_SWAP(r0, r1, temp);
			GPU_SWAP(g0, g1, temp);
			GPU_SWAP(b0, b1, temp);
		}
		x1 = GPU_DIV((x1 - x0) << GPU_DIGITS, dy);
		r1 = GPU_DIV((r1 - r0) << GPU_DIGITS, dy);
		g1 = GPU_DIV((g1 - g0) << GPU_DIGITS, dy);
		b1 = GPU_DIV((b1 - b0) << GPU_DIGITS, dy);
		x0 <<= GPU_DIGITS;
		r0 <<= GPU_DIGITS;
		g0 <<= GPU_DIGITS;
		b0 <<= GPU_DIGITS;
		temp = ymin - y0;
		if (temp > 0) {
			y0 = ymin;
			x0 += (x1 * temp);
			r0 += (r1 * temp);
			g0 += (g1 * temp);
			b0 += (b1 * temp);
		}
		if (y1 > ymax)
			y1 = ymax;
		y1 -= y0;
		if (y1 < 0)
			y1 = 0;
		for (; y1; y1--) {
			temp = x0 >> GPU_DIGITS;
			if ((Uint32) (temp - xmin) < (Uint32) (xmax - xmin)) {
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(temp, y0)];
				PixelData = 0x8000 | (0xF << 10) | (0xF << 5) | 0xF;
				_LR = r0 >> 12;
				_LG = g0 >> 12;
				_LB = b0 >> 12;
				gpuDriver();
			}
			y0++;
			x0 += x1;
			r0 += r1;
			g0 += g1;
			b0 += b1;
		}
	} else {
		if ((Uint32) (x0 - xmin) < (Uint32) (xmax - xmin)) {
			if ((Uint32) (y0 - ymin) < (Uint32) (ymax - ymin)) {
				Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0, y0)];
				PixelData = 0x8000 | (0xF << 10) | (0xF << 5) | 0xF;
				_LR = r0;
				_LG = g0;
				_LB = b0;
				gpuDriver();
			}
		}
	}
}

/*----------------------------------------------------------------------
T
----------------------------------------------------------------------*/

void gpuDrawT(void)
{
	Sint32 temp;
	Sint32 xmin, xmax;
	Sint32 ymin, ymax;
	Sint32 x0, w0;
	Sint32 y0, h0;

	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	w0 = PacketBuffer.S2[4];
	y0 = PacketBuffer.S2[3];
	h0 = PacketBuffer.S2[5];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	temp = PacketBuffer.U4[0];
	PixelData = GPU_RGB16(temp);
	temp = DrawingOffset[0];
	x0 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	h0 += y0;
	if (y0 < ymin)
		y0 = ymin;
	if (h0 > ymax)
		h0 = ymax;
	h0 -= y0;
	if (h0 < 0)
		h0 = 0;
	w0 += x0;
	if (x0 < xmin)
		x0 = xmin;
	if (w0 > xmax)
		w0 = xmax;
	w0 -= x0;
	if (w0 < 0)
		w0 = 0;
	Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0, y0)];
	temp = FRAME_WIDTH - w0;
	for (; h0; h0--) {
		for (x0 = w0; x0; x0--) {
			gpuDriver();
			Pixel++;
		}
		Pixel += temp;
	}
}

/*----------------------------------------------------------------------
S
----------------------------------------------------------------------*/

void gpuDrawS(void)
{
	Sint32 temp;
	Sint32 xmin, xmax;
	Sint32 ymin, ymax;
	Sint32 x0, x1;
	Sint32 y0, y1;
	Sint32 u0;
	Sint32 v0;

	if ( Skip )
		return;

	updateLace = 1;

	x0 = PacketBuffer.S2[2];
	x1 = PacketBuffer.S2[6];
	y0 = PacketBuffer.S2[3];
	y1 = PacketBuffer.S2[7];
	u0 = PacketBuffer.U1[8];
	v0 = PacketBuffer.U1[9];
	xmin = DrawingArea[0];
	xmax = DrawingArea[2];
	ymin = DrawingArea[1];
	ymax = DrawingArea[3];
	_LR = PacketBuffer.U1[0];
	_LG = PacketBuffer.U1[1];
	_LB = PacketBuffer.U1[2];
	temp = DrawingOffset[0];
	x0 += temp;
	temp = DrawingOffset[1];
	y0 += temp;
	x1 += x0;
	y1 += y0;
	temp = ymin - y0;
	if (temp > 0) {
		y0 = ymin;
		v0 += temp;
	}
	if (y1 > ymax)
		y1 = ymax;
	y1 -= y0;
	if (y1 < 0)
		y1 = 0;
	temp = xmin - x0;
	if (temp > 0) {
		x0 = xmin;
		u0 += temp;
	}
	if (x1 > xmax)
		x1 = xmax;
	x1 -= x0;
	if (x1 < 0)
		x1 = 0;
	Pixel = &((Uint16*)FrameBuffer)[FRAME_OFFSET(x0, y0)];
	temp = FRAME_WIDTH - x1;
	_TV = v0;
	for (y0 = y1; y0; y0--) {
		_TU = u0;
		for (x0 = x1; x0; x0--) {
			gpuDriver();
			Pixel++;
			_TU++;
		}
		Pixel += temp;
		_TV++;
	}
}
