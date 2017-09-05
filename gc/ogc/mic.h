#ifndef __MIC_H__
#define __MIC_H__

#include <gctypes.h>

// Size of buffer for device driver
//    12KB =  Approx. 139msec worth @ 44100Hz
//         =  Approx. 279msec worth @ 22050Hz
//         =  Approx. 557msec worth @ 11025Hz
#define MIC_RINGBUFF_SIZE         12*1024

// Returned values, tests, etc.
#define MIC_RESULT_UNLOCKED           1
#define MIC_RESULT_READY              0
#define MIC_RESULT_BUSY              -1
#define MIC_RESULT_WRONGDEVICE       -2
#define MIC_RESULT_NOCARD            -3
#define MIC_RESULT_INVALID_STATE     -4
#define MIC_RESULT_FATAL_ERROR     -128

// Button bits
#define MIC_BUTTON_0             0x0001
#define MIC_BUTTON_1             0x0002
#define MIC_BUTTON_2             0x0004
#define MIC_BUTTON_3             0x0008
#define MIC_BUTTON_4             0x0010
#define MIC_BUTTON_TALK    MIC_BUTTON_4

#define MIC_BMC              0x00000000

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef void (*MICCallback)(s32 chan, s32 result);

void MIC_Init(void);
s32 MIC_ProbeEx(s32 chan);
s32 MIC_GetResultCode(s32 chan);
u32 MIC_GetErrorCount(s32 chan);

s32 MIC_MountAsync(s32 chan, s16 *buffer, s32 size, MICCallback detachCallback, MICCallback attachCallback);
s32 MIC_Mount(s32 chan, s16 *buffer, s32 size, MICCallback detachCallback);
s32 MIC_Unmount(s32 chan);
s32 MIC_GetRingbuffsize(s32 chan, s32 *size);

s32 MIC_SetStatusAsync(s32 chan, u32 status, MICCallback setCallback);
s32 MIC_SetStatus(s32 chan, u32 status);
s32 MIC_GetStatus(s32 chan, u32 *status);

s32 MIC_SetParamsAsync(s32 chan, s32 size, s32 rate, s32 gain, MICCallback setCallback);
s32 MIC_SetParams(s32 chan, s32 size, s32 rate, s32 gain);
s32 MIC_GetParams(s32 chan, s32 *size, s32 *rate, s32 *gain);

s32 MIC_SetBuffsizeAsync(s32 chan, s32 size, MICCallback setCallback);
s32 MIC_SetBuffsize(s32 chan, s32 size);
s32 MIC_GetBuffsize(s32 chan, s32 *size);

s32 MIC_SetRateAsync(s32 chan, s32 rate, MICCallback setCallback);
s32 MIC_SetRate(s32 chan, s32 rate);
s32 MIC_GetRate(s32 chan, s32 *rate);

s32 MIC_SetGainAsync(s32 chan, s32 gain, MICCallback setCallback);
s32 MIC_SetGain(s32 chan, s32 gain);
s32 MIC_GetGain(s32 chan, s32 *gain);

s32 MIC_GetButton(s32 chan, u32 *button);
s32 MIC_GetDeviceID(s32 chan, u32 *id);

s32 MIC_SetOutAsync(s32 chan, u32 pattern, MICCallback setCallback);
s32 MIC_SetOut(s32 chan, u32 pattern);
s32 MIC_GetOut(s32 chan, u32 *pattern);

s32 MIC_StartAsync(s32 chan, MICCallback startCallback);
s32 MIC_Start(s32 chan);

s32 MIC_StopAsync(s32 chan, MICCallback stopCallback);
s32 MIC_Stop(s32 chan);

s32 MIC_ResetAsync(s32 chan, MICCallback stopCallback);
s32 MIC_Reset(s32 chan);

BOOL MIC_IsActive(s32 chan);
BOOL MIC_IsAttached(s32 chan);

s32 MIC_GetCurrentTop(s32 chan);
s32 MIC_UpdateIndex(s32 chan, s32 index, s32 samples);
s32 MIC_GetSamplesLeft(s32 chan, s32 index);
s32 MIC_GetSamples(s32 chan, s16 *buffer, s32 index, s32 samples);

MICCallback MIC_SetExiCallback(s32 chan, MICCallback exiCallback);
MICCallback MIC_SetTxCallback(s32 chan, MICCallback txCallback);

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
