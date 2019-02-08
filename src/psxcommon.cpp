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

#include "psxcommon.h"
#include "plugin_lib/plugin_lib.h"

void EmuUpdate()
{
	pl_frame_limit();

	// Update controls
	// NOTE: This is point of control transfer to frontend menu..
	//  Only allow re-entry to frontend when PS1 cache status is normal.
	//  We don't want to allow creation of savestates when cache is isolated.
	//  See cache control port comments in psxmem.cpp psxMemWrite32().
	if (psxRegs.writeok) {
		pad_update();
	}
}
