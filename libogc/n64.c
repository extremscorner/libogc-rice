#include "processor.h"
#include "lwp.h"
#include "system.h"
#include "si.h"
#include "n64.h"

typedef void (*N64TransferCallback)(s32 chan);

typedef struct _n64 {
	u8 out[35];
	u8 in[33];
	u32 out_len;
	u32 in_len;
	u8 *stat;
	N64Status *status;
	N64SyncCallback sync_cb;
	s32 err;
	lwpq_t sync_queue;
	N64TransferCallback xfer_cb;
} N64;

static N64 __N64[SI_MAX_CHAN];
static u32 __N64Reset;

static void __N64SyncCallback(s32 chan,s32 err);
static s32 __N64Sync(s32 chan);
static s32 __N64Transfer(s32 chan,u32 out_len,u32 in_len,N64TransferCallback cb);

static s32 __n64_onreset(s32 final);

static sys_resetinfo __n64_resetinfo = {
	{},
	__n64_onreset,
	127
};

void N64_Init()
{
	s32 chan;
	static u32 initialized;

	if(initialized) return;
	initialized = 1;

	chan = 0;
	while(chan<SI_MAX_CHAN) {
		LWP_InitQueue(&__N64[chan].sync_queue);
		chan++;
	}

	__N64Reset = 0;
	SYS_RegisterResetFunc(&__n64_resetinfo);
}

static void __N64DefaultCallback(s32 chan,s32 err)
{
}

static void __N64GetStatusCallback(s32 chan)
{
	if(__N64[chan].err==N64_ERR_NONE) {
		if(__N64[chan].in[0]!=0x05 || __N64[chan].in[1]!=0x00) {
			__N64[chan].err = N64_ERR_NO_CONTROLLER;
			return;
		}
		*__N64[chan].stat = __N64[chan].in[2];
	}
}

s32 N64_GetStatusAsync(s32 chan,u8 *stat,N64SyncCallback cb)
{
	s32 ret;
	u32 level;

	_CPU_ISR_Disable(level);

	if(!__N64[chan].sync_cb) {
		__N64[chan].sync_cb = (cb==NULL) ? __N64DefaultCallback : cb;
		ret = N64_ERR_NONE;
	} else {
		ret = N64_ERR_NOT_READY;
	}

	_CPU_ISR_Restore(level);

	if(ret==N64_ERR_NONE) {
		__N64[chan].out[0] = 0x00;
		__N64[chan].stat = stat;
		ret = __N64Transfer(chan,1,3,__N64GetStatusCallback);
	}
	return ret;
}

s32 N64_GetStatus(s32 chan,u8 *stat)
{
	s32 ret;

	ret = N64_GetStatusAsync(chan,stat,__N64SyncCallback);
	if(ret==N64_ERR_NONE) ret = __N64Sync(chan);
	return ret;
}

s32 N64_ResetAsync(s32 chan,u8 *stat,N64SyncCallback cb)
{
	s32 ret;
	u32 level;

	_CPU_ISR_Disable(level);

	if(!__N64[chan].sync_cb) {
		__N64[chan].sync_cb = (cb==NULL) ? __N64DefaultCallback : cb;
		ret = N64_ERR_NONE;
	} else {
		ret = N64_ERR_NOT_READY;
	}

	_CPU_ISR_Restore(level);

	if(ret==N64_ERR_NONE) {
		__N64[chan].out[0] = 0xff;
		__N64[chan].stat = stat;
		ret = __N64Transfer(chan,1,3,__N64GetStatusCallback);
	}
	return ret;
}

s32 N64_Reset(s32 chan,u8 *stat)
{
	s32 ret;

	ret = N64_ResetAsync(chan,stat,__N64SyncCallback);
	if(ret==N64_ERR_NONE) ret = __N64Sync(chan);
	return ret;
}

static s32 __n64_onreset(s32 final)
{
	__N64Reset = 1;
	return 1;
}

static void __N64ReadCallback(s32 chan)
{
	N64Status *status = __N64[chan].status;

	if(__N64[chan].err==N64_ERR_NONE) {
		status->button = (u16)((__N64[chan].in[1]<<8)|__N64[chan].in[0]);
		status->stickX = (s8)(__N64[chan].in[2]);
		status->stickY = (s8)(__N64[chan].in[3]);
	}
	status->err = __N64[chan].err;
}

s32 N64_ReadAsync(s32 chan,N64Status *status,N64SyncCallback cb)
{
	s32 ret;
	u32 level;

	_CPU_ISR_Disable(level);

	if(!__N64[chan].sync_cb) {
		__N64[chan].sync_cb = (cb==NULL) ? __N64DefaultCallback : cb;
		ret = N64_ERR_NONE;
	} else {
		ret = N64_ERR_NOT_READY;
	}

	_CPU_ISR_Restore(level);

	if(ret==N64_ERR_NONE) {
		__N64[chan].out[0] = 0x01;
		__N64[chan].status = status;
		ret = __N64Transfer(chan,1,4,__N64ReadCallback);
	}
	return ret;
}

s32 N64_Read(s32 chan,N64Status *status)
{
	s32 ret;

	ret = N64_ReadAsync(chan,status,__N64SyncCallback);
	if(ret==N64_ERR_NONE) ret = __N64Sync(chan);
	return ret;
}

static void __N64Callback(s32 chan,u32 type)
{
	N64TransferCallback xfer_cb;
	N64SyncCallback sync_cb;

	if(__N64Reset) return;

	if(type&SI_ERROR_NO_RESPONSE) __N64[chan].err = N64_ERR_NO_CONTROLLER;
	else if(type&0x0f) __N64[chan].err = N64_ERR_TRANSFER;
	else __N64[chan].err = N64_ERR_NONE;

	xfer_cb = __N64[chan].xfer_cb;
	if(xfer_cb) {
		__N64[chan].xfer_cb = NULL;
		xfer_cb(chan);
	}

	sync_cb = __N64[chan].sync_cb;
	if(sync_cb) {
		__N64[chan].sync_cb = NULL;
		sync_cb(chan,__N64[chan].err);
	}
}

static void __N64SyncCallback(s32 chan,s32 err)
{
	LWP_ThreadBroadcast(__N64[chan].sync_queue);
}

static s32 __N64Sync(s32 chan)
{
	s32 ret;
	u32 level;

	_CPU_ISR_Disable(level);

	while(__N64[chan].sync_cb)
		LWP_ThreadSleep(__N64[chan].sync_queue);
	ret = __N64[chan].err;

	_CPU_ISR_Restore(level);

	return ret;
}

static void __N64TypeCallback(s32 chan,u32 type)
{
	N64TransferCallback xfer_cb;
	N64SyncCallback sync_cb;

	if(__N64Reset) return;

	type &= ~0xff00;
	if(type==SI_N64_CONTROLLER) {
		if(SI_Transfer(chan,__N64[chan].out,__N64[chan].out_len,__N64[chan].in,__N64[chan].in_len,__N64Callback,0)) return;
		else __N64[chan].err = N64_ERR_NOT_READY;
	} else __N64[chan].err = N64_ERR_NO_CONTROLLER;

	xfer_cb = __N64[chan].xfer_cb;
	if(xfer_cb) {
		__N64[chan].xfer_cb = NULL;
		xfer_cb(chan);
	}

	sync_cb = __N64[chan].sync_cb;
	if(sync_cb) {
		__N64[chan].sync_cb = NULL;
		sync_cb(chan,__N64[chan].err);
	}
}

static s32 __N64Transfer(s32 chan,u32 out_len,u32 in_len,N64TransferCallback cb)
{
	u32 level;

	_CPU_ISR_Disable(level);

	__N64[chan].xfer_cb = cb;
	__N64[chan].out_len = out_len;
	__N64[chan].in_len = in_len;

	SI_GetTypeAsync(chan,__N64TypeCallback);

	_CPU_ISR_Restore(level);

	return N64_ERR_NONE;
}
