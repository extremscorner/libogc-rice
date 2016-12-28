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
 *	@brief Extenmote GC expansion device.
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
#include "extenmote_gc.h"
#include "io.h"

static ubyte msg_def[16] = { 0xFF, 0xFF, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00 };

static void extenmote_gc_pressed_buttons(struct extenmote_gc_t* gc, uword now);

/**
 *	@brief Handle the handshake data from the extenmote gc.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param gc		A pointer to a extenmote_gc_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */
int extenmote_gc_handshake(struct wiimote_t* wm, struct extenmote_gc_t* gc, ubyte* data, uword len)
{
	gc->btns = 0;
	gc->btns_held = 0;
	gc->btns_released = 0;

	/* joystick stuff */
	gc->ljs.max.x = 0xE8;
	gc->ljs.min.x = 0x18;
	gc->ljs.center.x = 0x80;
	gc->ljs.max.y = 0xE8;
	gc->ljs.min.y = 0x18;
	gc->ljs.center.y = 0x80;

	gc->rjs.max.x = 0xD8;
	gc->rjs.min.x = 0x28;
	gc->rjs.center.x = 0x80;
	gc->rjs.max.y = 0xD8;
	gc->rjs.min.y = 0x28;
	gc->rjs.center.y = 0x80;

	/* handshake done */
	wm->event = WIIUSE_EXTENMOTE_GC_INSERTED;
	wm->exp.type = EXP_EXTENMOTE_GC;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	/* reset extenmote data */
	wiiuse_write_data(wm, WM_EXP_EXTENMOTE_NATIVE, msg_def, sizeof(msg_def), NULL);
	return 1;
}


/**
 *	@brief The extenmote gc disconnected.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param gc		A pointer to a extenmote_gc_t structure.
 */
void extenmote_gc_disconnected(struct wiimote_t* wm, struct extenmote_gc_t* gc) 
{
	memset(gc, 0, sizeof(struct extenmote_gc_t));
}


/**
 *	@brief Handle extenmote gc event.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param gc		A pointer to a extenmote_gc_t structure.
 *	@param msg		The message specified in the event packet.
 *	@param len		The length of the message block, in bytes.
 *
 *	@return	Returns 1 if event was successful, 0 if not.
 */
int extenmote_gc_event(struct wiimote_t* wm, struct extenmote_gc_t* gc, ubyte* msg, ubyte len)
{
	if (msg[0] == 0xFF || msg[1] == 0xFF)
		return 0;

	extenmote_gc_pressed_buttons(gc, BIG_ENDIAN_SHORT(*(uword*)msg));

	/* left/right buttons */
	if (len >= 8) {
		gc->ls_raw = msg[6];
		gc->rs_raw = msg[7];
	} else {
		gc->ls_raw = 0;
		gc->rs_raw = 0;
	}

	/* calculate joystick orientation */
	gc->ljs.pos.x = msg[2];
	gc->ljs.pos.y = msg[3];
	gc->rjs.pos.x = msg[4];
	gc->rjs.pos.y = msg[5];

	if (wm->expansion_state == 5) {
		wm->expansion_state++;
		gc->ljs.center.x = gc->ljs.pos.x;
		gc->ljs.center.y = gc->ljs.pos.y;
		gc->rjs.center.x = gc->rjs.pos.x;
		gc->rjs.center.y = gc->rjs.pos.y;
	}

	if (gc->ljs.min.x > gc->ljs.pos.x)
		gc->ljs.min.x = gc->ljs.pos.x;
	if (gc->ljs.max.x < gc->ljs.pos.x)
		gc->ljs.max.x = gc->ljs.pos.x;
	if (gc->ljs.min.y > gc->ljs.pos.y)
		gc->ljs.min.y = gc->ljs.pos.y;
	if (gc->ljs.max.y < gc->ljs.pos.y)
		gc->ljs.max.y = gc->ljs.pos.y;

	if (gc->rjs.min.x > gc->rjs.pos.x)
		gc->rjs.min.x = gc->rjs.pos.x;
	if (gc->rjs.max.x < gc->rjs.pos.x)
		gc->rjs.max.x = gc->rjs.pos.x;
	if (gc->rjs.min.y > gc->rjs.pos.y)
		gc->rjs.min.y = gc->rjs.pos.y;
	if (gc->rjs.max.y < gc->rjs.pos.y)
		gc->rjs.max.y = gc->rjs.pos.y;

#ifndef GEKKO
	calc_joystick_state(&gc->ljs, gc->ljs.pos.x, gc->ljs.pos.y);
	calc_joystick_state(&gc->rjs, gc->rjs.pos.x, gc->rjs.pos.y);
#endif

	return 1;
}


/**
 *	@brief Find what buttons are pressed.
 *
 *	@param gc		A pointer to a extenmote_gc_t structure.
 *	@param msg		The message byte specified in the event packet.
 */
static void extenmote_gc_pressed_buttons(struct extenmote_gc_t* gc, uword now)
{
	/* message is normal (1 is active, 0 is inactive) */
	now = now & EXTENMOTE_GC_BUTTON_ALL;

	/* preserve old btns pressed */
	gc->btns_last = gc->btns;

	/* pressed now & were pressed, then held */
	gc->btns_held = (now & gc->btns);

	/* were pressed or were held & not pressed now, then released */
	gc->btns_released = ((gc->btns | gc->btns_held) & ~now);

	/* buttons pressed now */
	gc->btns = now;
}
