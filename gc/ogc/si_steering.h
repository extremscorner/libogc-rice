#ifndef __SI_STEERING_H__
#define __SI_STEERING_H__

#include <gctypes.h>

#define SI_STEERING_ERR_NONE			0
#define SI_STEERING_ERR_NO_CONTROLLER	-1
#define SI_STEERING_ERR_NOT_READY		-2
#define SI_STEERING_ERR_TRANSFER		-3

#define SI_STEERING_FLAG_PEDALS			0x08

#define SI_STEERING_BUTTON_LEFT			0x0001
#define SI_STEERING_BUTTON_RIGHT		0x0002
#define SI_STEERING_BUTTON_DOWN			0x0004
#define SI_STEERING_BUTTON_UP			0x0008
#define SI_STEERING_BUTTON_Z			0x0010
#define SI_STEERING_BUTTON_R			0x0020
#define SI_STEERING_BUTTON_L			0x0040
#define SI_STEERING_BUTTON_A			0x0100
#define SI_STEERING_BUTTON_B			0x0200
#define SI_STEERING_BUTTON_X			0x0400
#define SI_STEERING_BUTTON_Y			0x0800
#define SI_STEERING_BUTTON_START		0x1000

#define SI_STEERING_FORCE_OFF			0x0400
#define SI_STEERING_FORCE_ON			0x0600

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef void (*SISteeringSyncCallback)(s32 chan,s32 err);
typedef void (*SISteeringSamplingCallback)(void);

typedef struct _sisteeringstatus {
	u16 button;
	u8 flag;
	s8 wheel;
	u8 pedalL;
	u8 pedalR;
	u8 paddleL;
	u8 paddleR;
	s8 err;
} SISteeringStatus;

void SI_InitSteering();
s32 SI_ResetSteeringAsync(s32 chan,SISteeringSyncCallback cb);
s32 SI_ResetSteering(s32 chan);
s32 SI_ReadSteering(s32 chan,SISteeringStatus *status);
SISteeringSamplingCallback SI_SetSteeringSamplingCallback(SISteeringSamplingCallback cb);
void SI_ControlSteering(s32 chan,u32 cmd,s32 force);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
