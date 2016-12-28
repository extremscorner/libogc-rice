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
 *	@brief Extenmote N64 expansion device.
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
#include "extenmote_n64.h"
#include "io.h"

static ubyte msg_def[16] = { 0xFF, 0xFF, 0x00, 0x00 };

static void extenmote_n64_pressed_buttons(struct extenmote_n64_t* n64, uword now);

/**
 *	@brief Handle the handshake data from the extenmote n64.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param n64		A pointer to a extenmote_n64_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */
int extenmote_n64_handshake(struct wiimote_t* wm, struct extenmote_n64_t* n64, ubyte* data, uword len)
{
	n64->btns = 0;
	n64->btns_held = 0;
	n64->btns_released = 0;

	/* joystick stuff */
	n64->js.max.x = 0xD0;
	n64->js.min.x = 0x30;
	n64->js.center.x = 0x80;
	n64->js.max.y = 0xD0;
	n64->js.min.y = 0x30;
	n64->js.center.y = 0x80;

	/* handshake done */
	wm->event = WIIUSE_EXTENMOTE_N64_INSERTED;
	wm->exp.type = EXP_EXTENMOTE_N64;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	/* reset extenmote data */
	wiiuse_write_data(wm, WM_EXP_EXTENMOTE_NATIVE, msg_def, sizeof(msg_def), NULL);
	return 1;
}


/**
 *	@brief The extenmote n64 disconnected.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param n64		A pointer to a extenmote_n64_t structure.
 */
void extenmote_n64_disconnected(struct wiimote_t* wm, struct extenmote_n64_t* n64) 
{
	memset(n64, 0, sizeof(struct extenmote_n64_t));
}


/**
 *	@brief Handle extenmote n64 event.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param n64		A pointer to a extenmote_n64_t structure.
 *	@param msg		The message specified in the event packet.
 *	@param len		The length of the message block, in bytes.
 *
 *	@return	Returns 1 if event was successful, 0 if not.
 */
int extenmote_n64_event(struct wiimote_t* wm, struct extenmote_n64_t* n64, ubyte* msg, ubyte len)
{
	if (msg[0] == 0xFF || msg[1] == 0xFF)
		return 0;

	extenmote_n64_pressed_buttons(n64, LITTLE_ENDIAN_SHORT(*(uword*)msg));

	/* calculate joystick orientation */
	n64->js.pos.x = (msg[2] ^ 0x80);
	n64->js.pos.y = (msg[3] ^ 0x80);

	if (n64->js.min.x > n64->js.pos.x)
		n64->js.min.x = n64->js.pos.x;
	if (n64->js.max.x < n64->js.pos.x)
		n64->js.max.x = n64->js.pos.x;
	if (n64->js.min.y > n64->js.pos.y)
		n64->js.min.y = n64->js.pos.y;
	if (n64->js.max.y < n64->js.pos.y)
		n64->js.max.y = n64->js.pos.y;

#ifndef GEKKO
	calc_joystick_state(&n64->js, n64->js.pos.x, n64->js.pos.y);
#endif

	return 1;
}


/**
 *	@brief Find what buttons are pressed.
 *
 *	@param n64		A pointer to a extenmote_n64_t structure.
 *	@param msg		The message byte specified in the event packet.
 */
static void extenmote_n64_pressed_buttons(struct extenmote_n64_t* n64, uword now)
{
	/* message is normal (1 is active, 0 is inactive) */
	now = now & EXTENMOTE_N64_BUTTON_ALL;

	/* preserve old btns pressed */
	n64->btns_last = n64->btns;

	/* pressed now & were pressed, then held */
	n64->btns_held = (now & n64->btns);

	/* were pressed or were held & not pressed now, then released */
	n64->btns_released = ((n64->btns | n64->btns_held) & ~now);

	/* buttons pressed now */
	n64->btns = now;
}
