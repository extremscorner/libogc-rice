#include <stdio.h>
#include <stdlib.h>

#include "definitions.h"
#include "wiiuse_internal.h"
#include "nunchuk.h"
#include "classic.h"
#include "guitar_hero_3.h"
#include "wiiupro.h"
#include "wiiboard.h"
#include "motion_plus.h"
#include "extenmote_nes.h"
#include "extenmote_snes.h"
#include "extenmote_n64.h"
#include "extenmote_gc.h"
#include "io.h"
#include "lwp_wkspace.h"

void wiiuse_handshake(struct wiimote_t *wm,ubyte *data,uword len)
{
	uint id;
	ubyte *buf = NULL;
	struct accel_t *accel = &wm->accel_calib;

	//printf("wiiuse_handshake(%d,%p,%d)\n",wm->handshake_state,data,len);

	switch(wm->handshake_state) {
		case 0:
			wm->handshake_state++;

			wiiuse_set_leds(wm,WIIMOTE_LED_NONE,NULL);

			buf = __lwp_wkspace_allocate(sizeof(ubyte)*6);
			wiiuse_read_data(wm,buf,WM_EXP_ID,6,wiiuse_handshake);
			break;
		case 1:
			wm->handshake_state++;

			id = BIG_ENDIAN_LONG(*(uint*)(&data[2]));
			__lwp_wkspace_free(data);

			switch(id) {
				case EXP_ID_CODE_WIIUPRO_CONTROLLER:
				case EXP_ID_CODE_WIIBOARD:
					WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_ACC);
					WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_IR|WIIMOTE_STATE_IR_INIT);
					WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_SPEAKER|WIIMOTE_STATE_SPEAKER_INIT);
					WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_HANDSHAKE);
					WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_HANDSHAKE_COMPLETE);
					WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_EXP_ONLY);

					wm->event = WIIUSE_CONNECT;
					wiiuse_status(wm,NULL);
					break;
				default:
					buf = __lwp_wkspace_allocate(sizeof(ubyte)*8);
					wiiuse_read_data(wm,buf,WM_MEM_OFFSET_CALIBRATION,8,wiiuse_handshake);
					break;
			}
			break;
		case 2:
			wm->handshake_state++;

			accel->cal_zero.x = ((data[0]<<2)|((data[3]>>4)&3));
			accel->cal_zero.y = ((data[1]<<2)|((data[3]>>2)&3));
			accel->cal_zero.z = ((data[2]<<2)|(data[3]&3));

			accel->cal_g.x = (((data[4]<<2)|((data[7]>>4)&3)) - accel->cal_zero.x);
			accel->cal_g.y = (((data[5]<<2)|((data[7]>>2)&3)) - accel->cal_zero.y);
			accel->cal_g.z = (((data[6]<<2)|(data[7]&3)) - accel->cal_zero.z);
			__lwp_wkspace_free(data);
			
			WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_HANDSHAKE);
			WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_HANDSHAKE_COMPLETE);

			wm->event = WIIUSE_CONNECT;
			wiiuse_status(wm,NULL);
			break;
		default:
			break;

	}
}

void wiiuse_handshake_expansion_start(struct wiimote_t *wm)
{
	if(WIIMOTE_IS_SET(wm,WIIMOTE_STATE_EXP) || WIIMOTE_IS_SET(wm,WIIMOTE_STATE_EXP_FAILED) || WIIMOTE_IS_SET(wm,WIIMOTE_STATE_EXP_HANDSHAKE))
		return;

	wm->expansion_state = WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP_ONLY) ? 4 : 0;
	WIIMOTE_ENABLE_STATE(wm, WIIMOTE_STATE_EXP_HANDSHAKE);
	wiiuse_handshake_expansion(wm, NULL, 0);
}

void wiiuse_handshake_expansion(struct wiimote_t *wm,ubyte *data,uword len)
{
	uint id;
	ubyte val;
	ubyte *buf = NULL;

	switch(wm->expansion_state) {
		/* These two initialization writes disable the encryption */
		case 0:
			wm->expansion_state++;
			val = 0x55;
			wiiuse_write_data(wm,WM_EXP_MEM_ENABLE1,&val,1,wiiuse_handshake_expansion);
			break;
		case 1:
			wm->expansion_state++;
			val = 0x00;
			wiiuse_write_data(wm,WM_EXP_MEM_ENABLE2,&val,1,wiiuse_handshake_expansion);
			break;
		case 2:
			wm->expansion_state++;
			if(WIIMOTE_IS_SET(wm,WIIMOTE_STATE_ACC) && WIIMOTE_IS_SET(wm,WIIMOTE_STATE_IR)) val = CLASSIC_CTRL_MODE_1;
			else val = CLASSIC_CTRL_MODE_3;
			wiiuse_write_data(wm,WM_EXP_CLASSIC_MODE,&val,1,wiiuse_handshake_expansion);
			break;
		case 3:
			wm->expansion_state++;
			val = 0x64;
			wiiuse_write_data(wm,WM_EXP_EXTENMOTE_NATIVE,&val,1,wiiuse_handshake_expansion);
			break;
		case 4:
			wm->expansion_state++;
			buf = __lwp_wkspace_allocate(sizeof(ubyte)*EXP_HANDSHAKE_LEN);
			wiiuse_read_data(wm,buf,WM_EXP_MEM_CALIBR,EXP_HANDSHAKE_LEN,wiiuse_handshake_expansion);
			break;
		case 5:
			if(!data || !len) return;
			id = BIG_ENDIAN_LONG(*(uint*)(&data[220]));

			switch(id) {
				case EXP_ID_CODE_NUNCHUK:
					if(!nunchuk_handshake(wm,&wm->exp.nunchuk,data,len)) return;
					break;
				case EXP_ID_CODE_CLASSIC_CONTROLLER:
					if(!classic_ctrl_handshake(wm,&wm->exp.classic,data,len)) return;
					break;
				case EXP_ID_CODE_GUITAR:
					if(!guitar_hero_3_handshake(wm,&wm->exp.gh3,data,len)) return;
					break;
				case EXP_ID_CODE_WIIUPRO_CONTROLLER:
					if(!wiiu_pro_ctrl_handshake(wm,&wm->exp.wup,data,len)) return;
					break;
 				case EXP_ID_CODE_WIIBOARD:
 					if(!wii_board_handshake(wm,&wm->exp.wb,data,len)) return;
 					break;
				case EXP_ID_CODE_EXTENMOTE_NES:
					if(!extenmote_nes_handshake(wm,&wm->exp.nes,data,len)) return;
					break;
				case EXP_ID_CODE_EXTENMOTE_SNES:
					if(!extenmote_snes_handshake(wm,&wm->exp.snes,data,len)) return;
					break;
				case EXP_ID_CODE_EXTENMOTE_N64:
					if(!extenmote_n64_handshake(wm,&wm->exp.n64,data,len)) return;
					break;
				case EXP_ID_CODE_EXTENMOTE_GC:
					if(!extenmote_gc_handshake(wm,&wm->exp.gc,data,len)) return;
					break;
				default:
					if(!classic_ctrl_handshake(wm,&wm->exp.classic,data,len)) return;
					break;
			}
			__lwp_wkspace_free(data);

			WIIMOTE_DISABLE_STATE(wm,WIIMOTE_STATE_EXP_HANDSHAKE);
			WIIMOTE_ENABLE_STATE(wm,WIIMOTE_STATE_EXP);
			wiiuse_set_ir_mode(wm);
			wiiuse_status(wm,NULL);
			break;
	}
}

void wiiuse_disable_expansion(struct wiimote_t *wm)
{
	if(!WIIMOTE_IS_SET(wm, WIIMOTE_STATE_EXP)) return;

	/* tell the associated module the expansion was removed */
	switch(wm->exp.type) {
		case EXP_NUNCHUK:
			nunchuk_disconnected(wm, &wm->exp.nunchuk);
			wm->event = WIIUSE_NUNCHUK_REMOVED;
			break;
		case EXP_CLASSIC:
			classic_ctrl_disconnected(wm, &wm->exp.classic);
			wm->event = WIIUSE_CLASSIC_CTRL_REMOVED;
			break;
		case EXP_GUITAR_HERO_3:
			guitar_hero_3_disconnected(wm, &wm->exp.gh3);
			wm->event = WIIUSE_GUITAR_HERO_3_CTRL_REMOVED;
			break;
		case EXP_WIIU_PRO:
			wiiu_pro_ctrl_disconnected(wm, &wm->exp.wup);
			wm->event = WIIUSE_WIIU_PRO_CTRL_REMOVED;
			break;
 		case EXP_WII_BOARD:
 			wii_board_disconnected(wm, &wm->exp.wb);
 			wm->event = WIIUSE_WII_BOARD_REMOVED;
 			break;
		case EXP_MOTION_PLUS:
 			motion_plus_disconnected(wm, &wm->exp.mp);
 			wm->event = WIIUSE_MOTION_PLUS_REMOVED;
 			break;
		case EXP_EXTENMOTE_NES:
			extenmote_nes_disconnected(wm, &wm->exp.nes);
			wm->event = WIIUSE_EXTENMOTE_NES_REMOVED;
			break;
		case EXP_EXTENMOTE_SNES:
			extenmote_snes_disconnected(wm, &wm->exp.snes);
			wm->event = WIIUSE_EXTENMOTE_SNES_REMOVED;
			break;
		case EXP_EXTENMOTE_N64:
			extenmote_n64_disconnected(wm, &wm->exp.n64);
			wm->event = WIIUSE_EXTENMOTE_N64_REMOVED;
			break;
		case EXP_EXTENMOTE_GC:
			extenmote_gc_disconnected(wm, &wm->exp.gc);
			wm->event = WIIUSE_EXTENMOTE_GC_REMOVED;
			break;
		default:
			break;
	}

	WIIMOTE_DISABLE_STATE(wm, WIIMOTE_STATE_EXP);
	wm->exp.type = EXP_NONE;

	wiiuse_set_ir_mode(wm);
	wiiuse_status(wm,NULL);
}
