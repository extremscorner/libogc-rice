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
 *	$Header: /lvm/shared/ds/ds/cvs/devkitpro-cvsbackup/libogc/wiiuse/classic.c,v 1.7 2008-11-14 13:34:57 shagkur Exp $
 *
 */

/**
 *	@file
 *	@brief Classic controller expansion device.
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
#include "classic.h"
#include "io.h"

static void classic_ctrl_pressed_buttons(struct classic_ctrl_t* cc, uword now);

/**
 *	@brief Handle the handshake data from the classic controller.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param cc		A pointer to a classic_ctrl_t structure.
 *	@param data		The data read in from the device.
 *	@param len		The length of the data block, in bytes.
 *
 *	@return	Returns 1 if handshake was successful, 0 if not.
 */
int classic_ctrl_handshake(struct wiimote_t* wm, struct classic_ctrl_t* cc, ubyte* data, uword len)
{
	cc->btns = 0;
	cc->btns_held = 0;
	cc->btns_released = 0;

	cc->type = data[218];
	cc->mode = data[222];

	switch (cc->mode) {
		default:
			cc->mode = CLASSIC_CTRL_MODE_COMPAT;

		case CLASSIC_CTRL_MODE_1:
			/* joystick stuff */
			cc->ljs.max.x = 0x3A;
			cc->ljs.min.x = 0x06;
			cc->ljs.center.x = 0x20;
			cc->ljs.max.y = 0x3A;
			cc->ljs.min.y = 0x06;
			cc->ljs.center.y = 0x20;

			cc->rjs.max.x = 0x1D;
			cc->rjs.min.x = 0x03;
			cc->rjs.center.x = 0x10;
			cc->rjs.max.y = 0x1D;
			cc->rjs.min.y = 0x03;
			cc->rjs.center.y = 0x10;
			break;
		case CLASSIC_CTRL_MODE_2:
			/* joystick stuff */
			cc->ljs.max.x = 0x3A0;
			cc->ljs.min.x = 0x060;
			cc->ljs.center.x = 0x200;
			cc->ljs.max.y = 0x3A0;
			cc->ljs.min.y = 0x060;
			cc->ljs.center.y = 0x200;

			cc->rjs.max.x = 0x3A0;
			cc->rjs.min.x = 0x060;
			cc->rjs.center.x = 0x200;
			cc->rjs.max.y = 0x3A0;
			cc->rjs.min.y = 0x060;
			cc->rjs.center.y = 0x200;
			break;
		case CLASSIC_CTRL_MODE_3:
			/* joystick stuff */
			cc->ljs.max.x = 0xE8;
			cc->ljs.min.x = 0x18;
			cc->ljs.center.x = 0x80;
			cc->ljs.max.y = 0xE8;
			cc->ljs.min.y = 0x18;
			cc->ljs.center.y = 0x80;

			cc->rjs.max.x = 0xE8;
			cc->rjs.min.x = 0x18;
			cc->rjs.center.x = 0x80;
			cc->rjs.max.y = 0xE8;
			cc->rjs.min.y = 0x18;
			cc->rjs.center.y = 0x80;
			break;
	}

	/* handshake done */
	wm->event = WIIUSE_CLASSIC_CTRL_INSERTED;
	wm->exp.type = EXP_CLASSIC;

	#ifdef WIN32
	wm->timeout = WIIMOTE_DEFAULT_TIMEOUT;
	#endif

	return 1;
}


/**
 *	@brief The classic controller disconnected.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param cc		A pointer to a classic_ctrl_t structure.
 */
void classic_ctrl_disconnected(struct wiimote_t* wm, struct classic_ctrl_t* cc) 
{
	memset(cc, 0, sizeof(struct classic_ctrl_t));
}


/**
 *	@brief Handle classic controller event.
 *
 *	@param wm		A pointer to a wiimote_t structure.
 *	@param cc		A pointer to a classic_ctrl_t structure.
 *	@param msg		The message specified in the event packet.
 *	@param len		The length of the message block, in bytes.
 *
 *	@return	Returns 1 if event was successful, 0 if not.
 */
int classic_ctrl_event(struct wiimote_t* wm, struct classic_ctrl_t* cc, ubyte* msg, ubyte len)
{
	switch (cc->mode) {
		case CLASSIC_CTRL_MODE_1:
			if (len >= 8) {
				wiiuse_disable_expansion(wm);
				return 0;
			}
		default:
			classic_ctrl_pressed_buttons(cc, LITTLE_ENDIAN_SHORT(*(uword*)(msg + 4)));

			/* left/right buttons */
			cc->ls_raw = ((msg[2] & 0x60) >> 2) | ((msg[3] & 0xE0) >> 5);
			cc->rs_raw = (msg[3] & 0x1F);

			/* calculate joystick orientation */
			cc->ljs.pos.x = (msg[0] & 0x3F);
			cc->ljs.pos.y = (msg[1] & 0x3F);
			cc->rjs.pos.x = ((msg[0] & 0xC0) >> 3) | ((msg[1] & 0xC0) >> 5) | ((msg[2] & 0x80) >> 7);
			cc->rjs.pos.y = (msg[2] & 0x1F);
			break;
		case CLASSIC_CTRL_MODE_2:
			if (len < 9) {
				wiiuse_disable_expansion(wm);
				return 0;
			}
			if (!(msg[7] & CLASSIC_CTRL_BUTTON_PRESENT)) {
				cc->mode = CLASSIC_CTRL_MODE_COMPAT;

				cc->ljs.max.x >>= 4;
				cc->ljs.min.x >>= 4;
				cc->ljs.center.x >>= 4;
				cc->ljs.max.y >>= 4;
				cc->ljs.min.y >>= 4;
				cc->ljs.center.y >>= 4;

				cc->rjs.max.x >>= 5;
				cc->rjs.min.x >>= 5;
				cc->rjs.center.x >>= 5;
				cc->rjs.max.y >>= 5;
				cc->rjs.min.y >>= 5;
				cc->rjs.center.y >>= 5;

				return classic_ctrl_event(wm, cc, msg, len);
			}

			classic_ctrl_pressed_buttons(cc, LITTLE_ENDIAN_SHORT(*(uword*)(msg + 7)));

			/* left/right buttons */
			cc->ls_raw = (msg[5] >> 3);
			cc->rs_raw = (msg[6] >> 3);

			/* calculate joystick orientation */
			cc->ljs.pos.x = (msg[0] << 2) | (msg[4] & 0x03);
			cc->ljs.pos.y = (msg[2] << 2) | ((msg[4] & 0x30) >> 4);
			cc->rjs.pos.x = (msg[1] << 2) | ((msg[4] & 0x0C) >> 2);
			cc->rjs.pos.y = (msg[3] << 2) | ((msg[4] & 0xC0) >> 6);
			break;
		case CLASSIC_CTRL_MODE_3:
			if (len < 8) {
				wiiuse_disable_expansion(wm);
				return 0;
			}
			if (!(msg[6] & CLASSIC_CTRL_BUTTON_PRESENT)) {
				cc->mode = CLASSIC_CTRL_MODE_COMPAT;

				cc->ljs.max.x >>= 2;
				cc->ljs.min.x >>= 2;
				cc->ljs.center.x >>= 2;
				cc->ljs.max.y >>= 2;
				cc->ljs.min.y >>= 2;
				cc->ljs.center.y >>= 2;

				cc->rjs.max.x >>= 3;
				cc->rjs.min.x >>= 3;
				cc->rjs.center.x >>= 3;
				cc->rjs.max.y >>= 3;
				cc->rjs.min.y >>= 3;
				cc->rjs.center.y >>= 3;

				return classic_ctrl_event(wm, cc, msg, len);
			}

			classic_ctrl_pressed_buttons(cc, LITTLE_ENDIAN_SHORT(*(uword*)(msg + 6)));

			/* left/right buttons */
			cc->ls_raw = (msg[4] >> 3);
			cc->rs_raw = (msg[5] >> 3);

			/* calculate joystick orientation */
			cc->ljs.pos.x = msg[0];
			cc->ljs.pos.y = msg[2];
			cc->rjs.pos.x = msg[1];
			cc->rjs.pos.y = msg[3];
			break;
	}

	if (wm->expansion_state == 4) {
		wm->expansion_state++;
		cc->ljs.center.x = cc->ljs.pos.x;
		cc->ljs.center.y = cc->ljs.pos.y;
		cc->rjs.center.x = cc->rjs.pos.x;
		cc->rjs.center.y = cc->rjs.pos.y;
	}

	if (cc->ljs.min.x > cc->ljs.pos.x)
		cc->ljs.min.x = cc->ljs.pos.x;
	if (cc->ljs.max.x < cc->ljs.pos.x)
		cc->ljs.max.x = cc->ljs.pos.x;
	if (cc->ljs.min.y > cc->ljs.pos.y)
		cc->ljs.min.y = cc->ljs.pos.y;
	if (cc->ljs.max.y < cc->ljs.pos.y)
		cc->ljs.max.y = cc->ljs.pos.y;

	if (cc->rjs.min.x > cc->rjs.pos.x)
		cc->rjs.min.x = cc->rjs.pos.x;
	if (cc->rjs.max.x < cc->rjs.pos.x)
		cc->rjs.max.x = cc->rjs.pos.x;
	if (cc->rjs.min.y > cc->rjs.pos.y)
		cc->rjs.min.y = cc->rjs.pos.y;
	if (cc->rjs.max.y < cc->rjs.pos.y)
		cc->rjs.max.y = cc->rjs.pos.y;

#ifndef GEKKO
	calc_joystick_state(&cc->ljs, cc->ljs.pos.x, cc->ljs.pos.y);
	calc_joystick_state(&cc->rjs, cc->rjs.pos.x, cc->rjs.pos.y);
#endif

	return 1;
}


/**
 *	@brief Find what buttons are pressed.
 *
 *	@param cc		A pointer to a classic_ctrl_t structure.
 *	@param msg		The message byte specified in the event packet.
 */
static void classic_ctrl_pressed_buttons(struct classic_ctrl_t* cc, uword now)
{
	/* message is inverted (0 is active, 1 is inactive) */
	now = ~now & CLASSIC_CTRL_BUTTON_ALL;

	/* preserve old btns pressed */
	cc->btns_last = cc->btns;

	/* pressed now & were pressed, then held */
	cc->btns_held = (now & cc->btns);

	/* were pressed or were held & not pressed now, then released */
	cc->btns_released = ((cc->btns | cc->btns_held) & ~now);

	/* buttons pressed now */
	cc->btns = now;
}
