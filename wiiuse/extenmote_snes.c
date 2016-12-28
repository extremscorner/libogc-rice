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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef WIN32
	#include <Winsock2.h>
#endif

#include "definitions.h"
#include "wiiuse_internal.h"
#include "dynamics.h"
#include "events.h"
#include "extenmote_snes.h"
#include "io.h"

static ubyte msg_def[16] = { 0xFF, 0xFF };

static void extenmote_snes_pressed_buttons(struct extenmote_snes_t* snes, uword now);

/**
 *	@brief Handle the handshake data from the extenmote snes.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param snes		A pointer to a extenmote_snes_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */
int extenmote_snes_handshake(struct wiimote_t* wm, struct extenmote_snes_t* snes, ubyte* data, uword len)
{
	snes->btns = 0;
	snes->btns_held = 0;
	snes->btns_released = 0;

	/* handshake done */
	wm->event = WIIUSE_EXTENMOTE_SNES_INSERTED;
	wm->exp.type = EXP_EXTENMOTE_SNES;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	/* reset extenmote data */
	wiiuse_write_data(wm, WM_EXP_EXTENMOTE_NATIVE, msg_def, sizeof(msg_def), NULL);
	return 1;
}


/**
 *	@brief The extenmote snes disconnected.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param snes		A pointer to a extenmote_snes_t structure.
 */
void extenmote_snes_disconnected(struct wiimote_t* wm, struct extenmote_snes_t* snes) 
{
	memset(snes, 0, sizeof(struct extenmote_snes_t));
}


/**
 *	@brief Handle extenmote snes event.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param snes		A pointer to a extenmote_snes_t structure.
 *	@param msg		The message specified in the event packet.
 *	@param len		The length of the message block, in bytes.
 *
 *	@return	Returns 1 if event was successful, 0 if not.
 */
int extenmote_snes_event(struct wiimote_t* wm, struct extenmote_snes_t* snes, ubyte* msg, ubyte len)
{
	if (msg[0] == 0xFF || msg[1] == 0xFF)
		return 0;

	extenmote_snes_pressed_buttons(snes, LITTLE_ENDIAN_SHORT(*(uword*)msg));
	return 1;
}


/**
 *	@brief Find what buttons are pressed.
 *
 *	@param snes		A pointer to a extenmote_snes_t structure.
 *	@param msg		The message byte specified in the event packet.
 */
static void extenmote_snes_pressed_buttons(struct extenmote_snes_t* snes, uword now)
{
	/* message is normal (1 is active, 0 is inactive) */
	now = now & EXTENMOTE_SNES_BUTTON_ALL;

	/* preserve old btns pressed */
	snes->btns_last = snes->btns;

	/* pressed now & were pressed, then held */
	snes->btns_held = (now & snes->btns);

	/* were pressed or were held & not pressed now, then released */
	snes->btns_released = ((snes->btns | snes->btns_held) & ~now);

	/* buttons pressed now */
	snes->btns = now;
}
