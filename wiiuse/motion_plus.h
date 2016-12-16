/**
 *	@file
 *	@brief Motion plus extension
 */

#ifndef MOTION_PLUS_H_INCLUDED
#define MOTION_PLUS_H_INCLUDED

#include "wiiuse_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

void motion_plus_disconnected(struct wiimote_t* wm, struct motion_plus_t* mp);

int motion_plus_event(struct wiimote_t* wm, struct motion_plus_t* mp, ubyte* msg, ubyte len);

#ifdef __cplusplus
}
#endif

#endif
