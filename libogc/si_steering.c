#include "processor.h"
#include "lwp.h"
#include "system.h"
#include "video.h"
#include "si.h"
#include "si_steering.h"

typedef void (*SISteeringTransferCallback)(s32 chan);

typedef struct _sisteering {
	u8 out[3];
	u8 in[8];
	u32 out_len;
	u32 in_len;
	SISteeringSyncCallback sync_cb;
	s32 err;
	lwpq_t sync_queue;
	SISteeringTransferCallback xfer_cb;
} SISteering;

static SISteering __SISteering[SI_MAX_CHAN];
static u32 __SIResetSteering;
static u32 __SISteeringEnableBits;
static SISteeringSamplingCallback __SISteeringSamplingCallback;

static void __SISteeringSyncCallback(s32 chan,s32 err);
static s32 __SISteeringSync(s32 chan);
static s32 __SISteeringTransfer(s32 chan,u32 out_len,u32 in_len,SISteeringTransferCallback cb);
static void __SISteeringEnable(s32 chan);
static void __SISteeringDisable(s32 chan);

static s32 __steering_onreset(s32 final);

static sys_resetinfo __steering_resetinfo = {
	{},
	__steering_onreset,
	127
};

void SI_InitSteering()
{
	s32 chan;
	static u32 initialized;

	if(initialized) return;
	initialized = 1;

	chan = 0;
	while(chan<SI_MAX_CHAN) {
		__SISteering[chan].err = SI_STEERING_ERR_NONE;
		LWP_InitQueue(&__SISteering[chan].sync_queue);
		chan++;
	}

	SI_RefreshSamplingRate();
	__SIResetSteering = 0;
	SYS_RegisterResetFunc(&__steering_resetinfo);
}

static void __SISteeringDefaultCallback(s32 chan,s32 err)
{
}

static void __SISteeringResetCallback(s32 chan)
{
	if(__SISteering[chan].err==SI_STEERING_ERR_NONE) {
		if(!__SIResetSteering)
			__SISteeringEnable(chan);
	}
}

s32 SI_ResetSteeringAsync(s32 chan,SISteeringSyncCallback cb)
{
	s32 ret;
	u32 level;

	_CPU_ISR_Disable(level);

	if(!__SISteering[chan].sync_cb) {
		__SISteering[chan].sync_cb = (cb==NULL) ? __SISteeringDefaultCallback : cb;
		ret = SI_STEERING_ERR_NONE;
	} else {
		ret = SI_STEERING_ERR_NOT_READY;
	}

	_CPU_ISR_Restore(level);

	if(ret==SI_STEERING_ERR_NONE) {
		__SISteering[chan].out[0] = 0xff;
		ret = __SISteeringTransfer(chan,1,3,__SISteeringResetCallback);
	}
	return ret;
}

s32 SI_ResetSteering(s32 chan)
{
	s32 ret;

	ret = SI_ResetSteeringAsync(chan,__SISteeringSyncCallback);
	if(ret==SI_STEERING_ERR_NONE) ret = __SISteeringSync(chan);
	return ret;
}

static s32 __steering_onreset(s32 final)
{
	s32 chan;
	static u32 count;

	if(!__SIResetSteering) {
		__SIResetSteering = 1;

		chan = 0;
		while(chan<SI_MAX_CHAN) {
			SI_ControlSteering(chan,SI_STEERING_FORCE_OFF,0);
			chan++;
		}
		count = VIDEO_GetRetraceCount();
	}

	if(VIDEO_GetRetraceCount()-count>2) {
		while(__SISteeringEnableBits) {
			chan = cntlzw(__SISteeringEnableBits);
			__SISteeringDisable(chan);
		}
		return 1;
	}
	return 0;
}

static void __SISteeringCallback(s32 chan,u32 type)
{
	if(__SIResetSteering) return;

	if(type&SI_ERROR_NO_RESPONSE) __SISteering[chan].err = SI_STEERING_ERR_NO_CONTROLLER;
	else if(type&0x0f) __SISteering[chan].err = SI_STEERING_ERR_TRANSFER;
	else __SISteering[chan].err = SI_STEERING_ERR_NONE;

	if(__SISteering[chan].xfer_cb) {
		__SISteering[chan].xfer_cb(chan);
		__SISteering[chan].xfer_cb = NULL;
	}

	if(__SISteering[chan].sync_cb) {
		__SISteering[chan].sync_cb(chan,__SISteering[chan].err);
		__SISteering[chan].sync_cb = NULL;
	}
}

static void __SISteeringSyncCallback(s32 chan,s32 err)
{
	LWP_ThreadBroadcast(__SISteering[chan].sync_queue);
}

static s32 __SISteeringSync(s32 chan)
{
	s32 ret;
	u32 level;

	_CPU_ISR_Disable(level);

	while(__SISteering[chan].sync_cb)
		LWP_ThreadSleep(__SISteering[chan].sync_queue);
	ret = __SISteering[chan].err;

	_CPU_ISR_Restore(level);

	return ret;
}

static void __SISteeringTypeCallback(s32 chan,u32 type)
{
	if(__SIResetSteering) return;

	type &= ~0xffff;
	if(type==SI_GC_STEERING) {
		if(SI_Transfer(chan,__SISteering[chan].out,__SISteering[chan].out_len,__SISteering[chan].in,__SISteering[chan].in_len,__SISteeringCallback,0)) return;
		else __SISteering[chan].err = SI_STEERING_ERR_NOT_READY;
	} else __SISteering[chan].err = SI_STEERING_ERR_NO_CONTROLLER;

	if(__SISteering[chan].xfer_cb) {
		__SISteering[chan].xfer_cb(chan);
		__SISteering[chan].xfer_cb = NULL;
	}

	if(__SISteering[chan].sync_cb) {
		__SISteering[chan].sync_cb(chan,__SISteering[chan].err);
		__SISteering[chan].sync_cb = NULL;
	}
}

static s32 __SISteeringTransfer(s32 chan,u32 out_len,u32 in_len,SISteeringTransferCallback cb)
{
	u32 level;

	_CPU_ISR_Disable(level);

	__SISteering[chan].xfer_cb = cb;
	__SISteering[chan].out_len = out_len;
	__SISteering[chan].in_len = in_len;

	SI_GetTypeAsync(chan,__SISteeringTypeCallback);

	_CPU_ISR_Restore(level);

	return SI_STEERING_ERR_NONE;
}

static void __SISteeringEnable(s32 chan)
{
	u32 mask;
	u32 buf[2];

	mask = SI_CHAN_BIT(chan);
	if(!(__SISteeringEnableBits&mask)) {
		__SISteeringEnableBits |= mask;
		SI_GetResponse(chan,buf);
		SI_SetCommand(chan,0x00300680);
		SI_EnablePolling(__SISteeringEnableBits);
	}
}

static void __SISteeringDisable(s32 chan)
{
	u32 mask;

	mask = SI_CHAN_BIT(chan);
	SI_DisablePolling(mask);
	__SISteeringEnableBits &= ~mask;
}

s32 SI_ReadSteering(s32 chan,SISteeringStatus *status)
{
	s32 ret;
	u32 level;
	u32 buf[2];

	_CPU_ISR_Disable(level);

	if(SI_IsChanBusy(chan)) {
		__SISteering[chan].err = SI_STEERING_ERR_NOT_READY;
	} else if(!(__SISteeringEnableBits&SI_CHAN_BIT(chan))) {
		__SISteering[chan].err = SI_STEERING_ERR_NO_CONTROLLER;
	} else if(SI_GetStatus(chan)&SI_ERROR_NO_RESPONSE) {
		SI_GetResponse(chan,buf);
		__SISteeringDisable(chan);
		__SISteering[chan].err = SI_STEERING_ERR_NO_CONTROLLER;
	} else if(!SI_GetResponse(chan,buf) || buf[0]&0x80000000) {
		__SISteering[chan].err = SI_STEERING_ERR_TRANSFER;
	} else {
		__SISteering[chan].err = SI_STEERING_ERR_NONE;
		if(status) {
			status->button = (buf[0]>>16)&0xffff;
			status->flag = (buf[0]>>8)&0xff;
			status->wheel = (buf[0]&0xff)-128;
			status->pedalL = (buf[1]>>24)&0xff;
			status->pedalR = (buf[1]>>16)&0xff;
			status->paddleL = (buf[1]>>8)&0xff;
			status->paddleR = (buf[1]&0xff);
		}
	}

	if(status) status->err = __SISteering[chan].err;
	ret = __SISteering[chan].err;

	_CPU_ISR_Restore(level);

	return ret;
}

static void __SISteeringSamplingHandler(u32 irq,void *ctx)
{
	if(__SISteeringSamplingCallback)
		__SISteeringSamplingCallback();
}

SISteeringSamplingCallback SI_SetSteeringSamplingCallback(SISteeringSamplingCallback cb)
{
	SISteeringSamplingCallback ret;

	ret = __SISteeringSamplingCallback;
	__SISteeringSamplingCallback = cb;

	if(cb) SI_RegisterPollingHandler(__SISteeringSamplingHandler);
	else SI_UnregisterPollingHandler(__SISteeringSamplingHandler);

	return ret;
}

void SI_ControlSteering(s32 chan,u32 cmd,s32 force)
{
	u32 level;

	if(force<-128) force = 0;
	else if(force>128) force = 256;
	else force += 128;

	_CPU_ISR_Disable(level);

	if(__SISteeringEnableBits&SI_CHAN_BIT(chan)) {
		cmd = 0x00300000|(cmd&0x0600)|(force&0x01ff);
		SI_SetCommand(chan,cmd);
		SI_TransferCommands();
	}

	_CPU_ISR_Restore(level);
}
