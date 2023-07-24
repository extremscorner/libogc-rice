#ifndef __OGC_N64_H__
#define __OGC_N64_H__

#include <gctypes.h>

#define N64_ERR_NONE			0
#define N64_ERR_NO_CONTROLLER	-1
#define N64_ERR_NOT_READY		-2
#define N64_ERR_TRANSFER		-3

#define N64_BUTTON_RIGHT		0x0001
#define N64_BUTTON_LEFT			0x0002
#define N64_BUTTON_DOWN			0x0004
#define N64_BUTTON_UP			0x0008
#define N64_BUTTON_START		0x0010
#define N64_BUTTON_Z			0x0020
#define N64_BUTTON_B			0x0040
#define N64_BUTTON_A			0x0080
#define N64_BUTTON_C_RIGHT		0x0100
#define N64_BUTTON_C_LEFT		0x0200
#define N64_BUTTON_C_DOWN		0x0400
#define N64_BUTTON_C_UP			0x0800
#define N64_BUTTON_R			0x1000
#define N64_BUTTON_L			0x2000

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef void (*N64SyncCallback)(s32 chan,s32 err);

typedef struct _n64status {
	u16 button;
	s8 stickX;
	s8 stickY;
	s8 err;
} N64Status;

void N64_Init();
s32 N64_GetStatusAsync(s32 chan,u8 *stat,N64SyncCallback cb);
s32 N64_GetStatus(s32 chan,u8 *stat);
s32 N64_ResetAsync(s32 chan,u8 *stat,N64SyncCallback cb);
s32 N64_Reset(s32 chan,u8 *stat);
s32 N64_ReadAsync(s32 chan,N64Status *status,N64SyncCallback cb);
s32 N64_Read(s32 chan,N64Status *status);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
