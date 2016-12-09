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
 *	@brief Wii U Pro Controller expansion device.
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
#include "wiiupro.h"
#include "io.h"

static void wiiu_pro_ctrl_pressed_buttons(struct wiiu_pro_ctrl_t* wup, uint now);

/**
 *	@brief Handle the handshake data from the wiiu pro controller.
 *
 *	@param wup		A pointer to a wiiu_pro_ctrl_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */
int wiiu_pro_ctrl_handshake(struct wiimote_t* wm, struct wiiu_pro_ctrl_t* wup, ubyte* data, uword len)
{
	wup->btns = 0;
	wup->btns_held = 0;
	wup->btns_released = 0;

	/* joystick stuff */
	wup->ljs.max.x = 0xC00;
	wup->ljs.min.x = 0x400;
	wup->ljs.center.x = 0x800;
	wup->ljs.max.y = 0xC00;
	wup->ljs.min.y = 0x400;
	wup->ljs.center.y = 0x800;

	wup->rjs.max.x = 0xC00;
	wup->rjs.min.x = 0x400;
	wup->rjs.center.x = 0x800;
	wup->rjs.max.y = 0xC00;
	wup->rjs.min.y = 0x400;
	wup->rjs.center.y = 0x800;

	/* handshake done */
	wm->event = WIIUSE_WIIU_PRO_CTRL_INSERTED;
	wm->exp.type = EXP_WIIU_PRO;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	return 1;
}


/**
 *	@brief The wiiu pro controller disconnected.
 *
 *	@param wup		A pointer to a wiiu_pro_ctrl_t structure.
 */
void wiiu_pro_ctrl_disconnected(struct wiimote_t* wm, struct wiiu_pro_ctrl_t* wup) 
{
	memset(wup, 0, sizeof(struct wiiu_pro_ctrl_t));
}


/**
 *	@brief Handle wiiu pro controller event.
 *
 *	@param wup		A pointer to a wiiu_pro_ctrl_t structure.
 *	@param msg		The message specified in the event packet.
 */
void wiiu_pro_ctrl_event(struct wiimote_t* wm, struct wiiu_pro_ctrl_t* wup, ubyte* msg)
{
	wiiu_pro_ctrl_pressed_buttons(wup, LITTLE_ENDIAN_LONG(*(uint*)(msg + 8)));

	wup->battery_level = ((msg[10] & 0x70) >> 4);

	/* calculate joystick orientation */
	wup->ljs.pos.x = LITTLE_ENDIAN_SHORT(*(uword*)(msg + 0));
	wup->ljs.pos.y = LITTLE_ENDIAN_SHORT(*(uword*)(msg + 4));
	wup->rjs.pos.x = LITTLE_ENDIAN_SHORT(*(uword*)(msg + 2));
	wup->rjs.pos.y = LITTLE_ENDIAN_SHORT(*(uword*)(msg + 6));

	if (wm->expansion_state == 3) {
		wm->expansion_state++;
		wup->ljs.center.x = wup->ljs.pos.x;
		wup->ljs.center.y = wup->ljs.pos.y;
		wup->rjs.center.x = wup->rjs.pos.x;
		wup->rjs.center.y = wup->rjs.pos.y;
	}

	if (wup->ljs.min.x > wup->ljs.pos.x)
		wup->ljs.min.x = wup->ljs.pos.x;
	if (wup->ljs.max.x < wup->ljs.pos.x)
		wup->ljs.max.x = wup->ljs.pos.x;
	if (wup->ljs.min.y > wup->ljs.pos.y)
		wup->ljs.min.y = wup->ljs.pos.y;
	if (wup->ljs.max.y < wup->ljs.pos.y)
		wup->ljs.max.y = wup->ljs.pos.y;
	if (wup->rjs.min.x > wup->rjs.pos.x)
		wup->rjs.min.x = wup->rjs.pos.x;
	if (wup->rjs.max.x < wup->rjs.pos.x)
		wup->rjs.max.x = wup->rjs.pos.x;
	if (wup->rjs.min.y > wup->rjs.pos.y)
		wup->rjs.min.y = wup->rjs.pos.y;
	if (wup->rjs.max.y < wup->rjs.pos.y)
		wup->rjs.max.y = wup->rjs.pos.y;

#ifndef GEKKO
	calc_joystick_state(&wup->ljs, wup->ljs.pos.x, wup->ljs.pos.y);
	calc_joystick_state(&wup->rjs, wup->rjs.pos.x, wup->rjs.pos.y);
#endif
}


/**
 *	@brief Find what buttons are pressed.
 *
 *	@param wup		A pointer to a wiiu_pro_ctrl_t structure.
 *	@param msg		The message byte specified in the event packet.
 */
static void wiiu_pro_ctrl_pressed_buttons(struct wiiu_pro_ctrl_t* wup, uint now)
{
	/* message is inverted (0 is active, 1 is inactive) */
	now = ~now & WIIU_PRO_CTRL_BUTTON_ALL;

	/* preserve old btns pressed */
	wup->btns_last = wup->btns;

	/* pressed now & were pressed, then held */
	wup->btns_held = (now & wup->btns);

	/* were pressed or were held & not pressed now, then released */
	wup->btns_released = ((wup->btns | wup->btns_held) & ~now);

	/* buttons pressed now */
	wup->btns = now;
}
