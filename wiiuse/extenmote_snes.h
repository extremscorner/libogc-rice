/*
 *	wiiuse
 *
 *	Written By:
 *		Michael Laforest	< para >
 *		Email: < thepara (--AT--) g m a i l [--DOT--] com >
 *
 *	Copyright 2006-2007
 *
 *	This file is part of wiiuse.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	$Header: $
 *
 */

/**
 *	@file
 *	@brief Extenmote SNES expansion device.
 */

#ifndef EXTENMOTE_SNES_H_INCLUDED
#define EXTENMOTE_SNES_H_INCLUDED

#include "wiiuse_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int extenmote_snes_handshake(struct wiimote_t* wm, struct extenmote_snes_t* snes, ubyte* data, uword len);

void extenmote_snes_disconnected(struct wiimote_t* wm, struct extenmote_snes_t* snes);

int extenmote_snes_event(struct wiimote_t* wm, struct extenmote_snes_t* snes, ubyte* msg, ubyte len);

#ifdef __cplusplus
}
#endif

#endif // EXTENMOTE_SNES_H_INCLUDED
