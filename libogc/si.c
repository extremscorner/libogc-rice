#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "asm.h"
#include "processor.h"
#include "system.h"
#include "ogcsys.h"
#include "video.h"
#include "irq.h"
#include "si.h"
#include "timesupp.h"

//#define _SI_DEBUG

#define _SHIFTL(v, s, w)	\
    ((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

#define SISR_ERRORMASK(chn)			(0x0f000000>>((chn)<<3))
#define SIPOLL_ENABLE(chn)			(0x80000000>>((chn)+24))

#define SICOMCSR_TCINT				(1<<31)
#define SICOMCSR_TCINT_ENABLE		(1<<30)
#define SICOMCSR_COMERR				(1<<29)
#define SICOMCSR_RDSTINT			(1<<28)
#define SICOMCSR_RDSTINT_ENABLE		(1<<27)
#define SICOMCSR_TSTART				(1<<0)

#define SISR_UNDERRUN				0x0001
#define SISR_OVERRUN				0x0002
#define SISR_COLLISION				0x0004
#define SISR_NORESPONSE				0x0008
#define SISR_WRST					0x0010
#define SISR_RDST					0x0020

typedef union _sicomcsr {
	u32 val;
	struct {
		u32 tcint		: 1;
		u32 tcintmsk	: 1;
		u32 comerr		: 1;
		u32 rdstint		: 1;
		u32 rdstintmsk	: 1;
		u32 pad2		: 4;
		u32 outlen		: 7;
		u32 pad1		: 1;
		u32 inlen		: 7;
		u32 pad0		: 5;
		u32 channel		: 2;
		u32 tstart		: 1;
	} csrmap;
} sicomcsr;

static struct _sipacket {
	s32 chan;
	void *out;
	u32 out_bytes;
	void *in;
	u32 in_bytes;
	SICallback callback;
	u64 fire;
} sipacket[4];

static struct _sicntrl {
	s32 chan;
	u32 poll;
	u32 in_bytes;
	void *in;
	SICallback callback;
} sicntrl = {
	-1,
	0,
	0,
	NULL,
	NULL
};

static struct _xy {
	u16 line;
	u8 cnt;
} xy[4][12] = {
	{
		{0x00F6,0x02},{0x000F,0x12},{0x001E,0x09},{0x002C,0x06},
		{0x0034,0x05},{0x0041,0x04},{0x0057,0x03},{0x0057,0x03},
		{0x0057,0x03},{0x0083,0x02},{0x0083,0x02},{0x0083,0x02}
	},
	{
		{0x0128,0x02},{0x000F,0x15},{0x001D,0x0B},{0x002D,0x07},
		{0x0034,0x06},{0x003F,0x05},{0x004E,0x04},{0x0068,0x03},
		{0x0068,0x03},{0x0068,0x03},{0x0068,0x03},{0x009C,0x02}
	},
	{
		{0x015E,0x02},{0x0015,0x12},{0x002A,0x09},{0x003E,0x06},
		{0x004B,0x05},{0x005D,0x04},{0x007D,0x03},{0x007D,0x03},
		{0x007D,0x03},{0x00BB,0x02},{0x00BB,0x02},{0x00BB,0x02}
	},
	{
		{0x0163,0x02},{0x0012,0x15},{0x0022,0x0B},{0x0035,0x07},
		{0x003E,0x06},{0x004B,0x05},{0x005D,0x04},{0x007D,0x03},
		{0x007D,0x03},{0x007D,0x03},{0x007D,0x03},{0x00BB,0x02}
	}
};

u32 __PADFixBits = 0;

static u32 sampling_rate = 0;
static u32 cmdtypeandstatus$47 = 0;
static u32 cmdtypeandstatus$223 = 0;
static u32 cmdfixdevice[4] = {0,0,0,0};
static u32 si_type[4] = {8,8,8,8};
static u32 inputBufferVCount[4] = {0,0,0,0};
static u32 inputBufferValid[4] = {0,0,0,0};
static u32 inputBuffer[4][2] = {{0,0},{0,0},{0,0},{0,0}};
static RDSTHandler rdstHandlers[4] = {NULL,NULL,NULL,NULL};
static u64 typeTime[4] = {0,0,0,0};
static u64 xferTime[4] = {0,0,0,0};
static SICallback typeCallback[4][4] = {{NULL,NULL,NULL,NULL},
										{NULL,NULL,NULL,NULL},
										{NULL,NULL,NULL,NULL},
										{NULL,NULL,NULL,NULL}};
static syswd_t si_alarm[4];

#if defined(HW_DOL)
	static vu32* const _siReg = (u32*)0xCC006400;
#elif defined(HW_RVL)
	static vu32* const _siReg = (u32*)0xCD006400;
#else
	#error HW model unknown.
#endif

static u32 __si_transfer(s32 chan,void *out,u32 out_len,void *in,u32 in_len,SICallback cb);

static __inline__ struct _xy* __si_getxy()
{
	switch(VIDEO_GetCurrentTvMode()) {
		case VI_PAL:
		case VI_DEBUG_PAL:
			return xy[1];
		case VI_HD60:
			return xy[2];
		case VI_HD50:
		case VI_HD48:
			return xy[3];
		default:
			return xy[0];
	}
}

static __inline__ void __si_cleartcinterrupt()
{
	_siReg[13] = (_siReg[13]|SICOMCSR_TCINT)&~SICOMCSR_TSTART;
}

static void __si_alarmhandler(syswd_t thealarm,void *cbarg)
{
	u32 chn;
#ifdef _SI_DEBUG
	printf("__si_alarmhandler(%08x)\n",thealarm);
#endif
	chn = 0;
	while(chn<4) {
		if(si_alarm[chn]==thealarm) break;
		chn++;
	}
	if(chn==4) return;

	if(sipacket[chn].chan!=-1) {
		if(__si_transfer(sipacket[chn].chan,sipacket[chn].out,sipacket[chn].out_bytes,sipacket[chn].in,sipacket[chn].in_bytes,sipacket[chn].callback)) sipacket[chn].chan = -1;
	}
}

static u32 __si_completetransfer()
{
	u32 sisr,val,cnt,i;
	u32 *in;

#ifdef _SI_DEBUG
	printf("__si_completetransfer(csr = %08x,sr = %08x,chan = %d)\n",_siReg[13],_siReg[14],sicntrl.chan);
#endif
	sisr = _siReg[14];
	__si_cleartcinterrupt();

	if(sicntrl.chan==-1) return sisr;

	xferTime[sicntrl.chan] = __SYS_GetSystemTime();

	in = (u32*)sicntrl.in;
	cnt = (sicntrl.in_bytes/4);
	for(i=0;i<cnt;i++) in[i] = _siReg[32+i];
	if(sicntrl.in_bytes&0x03) {
		val = _siReg[32+cnt];
		__stswx(in+cnt,(sicntrl.in_bytes&0x03),val);
	}
#ifdef _SI_DEBUG
	printf("__si_completetransfer(csr = %08x)\n",_siReg[13]);
#endif
	if(_siReg[13]&SICOMCSR_COMERR) {
		sisr = (sisr>>((3-sicntrl.chan)*8))&0x0f;
		if(sisr&SISR_NORESPONSE && !(si_type[sicntrl.chan]&SI_ERROR_BUSY)) si_type[sicntrl.chan] = SI_ERROR_NO_RESPONSE;
		if(!sisr) sisr = SISR_COLLISION;
	} else {
		typeTime[sicntrl.chan] = __SYS_GetSystemTime();
		sisr = 0;
	}

	sicntrl.chan = -1;
	return sisr;
}

static u32 __si_transfer(s32 chan,void *out,u32 out_len,void *in,u32 in_len,SICallback cb)
{
	u32 level,val,cnt,i;
	sicomcsr csr;
#ifdef _SI_DEBUG
	printf("__si_transfer(%d,%p,%d,%p,%d,%p)\n",chan,out,out_len,in,in_len,cb);
#endif
	_CPU_ISR_Disable(level);
	if(sicntrl.chan!=-1) {
		_CPU_ISR_Restore(level);
		return 0;
	}
#ifdef _SI_DEBUG
	printf("__si_transfer(out = %08x,csr = %08x,sr = %08x)\n",*(u32*)out,_siReg[13],_siReg[14]);
#endif
	_siReg[14] &= SISR_ERRORMASK(chan);

	sicntrl.chan = chan;
	sicntrl.callback = cb;
	sicntrl.in_bytes = in_len;
	sicntrl.in = in;
#ifdef _SI_DEBUG
	printf("__si_transfer(csr = %08x,sr = %08x)\n",_siReg[13],_siReg[14]);
#endif
	cnt = (out_len/4);
	for(i=0;i<cnt;i++) _siReg[32+i] = ((u32*)out)[i];
	if(out_len&0x03) {
		val = __lswx((u32*)out+cnt,(out_len&0x03));
		_siReg[32+cnt] = val;
	}

	csr.val = _siReg[13];
	csr.csrmap.tcint = 1;
	csr.csrmap.tcintmsk = 0;
	if(cb) csr.csrmap.tcintmsk = 1;

	if(out_len==128) out_len = 0;
	csr.csrmap.outlen = out_len&0x7f;

	if(in_len==128) in_len = 0;
	csr.csrmap.inlen = in_len&0x7f;

	csr.csrmap.channel = chan&0x03;
	csr.csrmap.tstart = 1;
#ifdef _SI_DEBUG
	printf("__si_transfer(csr = %08x)\n",csr.val);
#endif
	_siReg[13] = csr.val;
	_CPU_ISR_Restore(level);

	return 1;
}

static void __si_calltypandstatuscallback(s32 chan,u32 type)
{
	u32 typ;
	SICallback cb = NULL;
#ifdef _SI_DEBUG
	printf("__si_calltypandstatuscallback(%d,%08x)\n",chan,type);
#endif
	typ = 0;
	while(typ<4) {
		cb = typeCallback[chan][typ];
		if(cb) {
			typeCallback[chan][typ] = NULL;
			cb(chan,type);
		}
		typ++;
	}
}

static void __si_gettypecallback(s32 chan,u32 type)
{
	u32 sipad_en,id;

	si_type[chan] = (si_type[chan]&~SI_ERROR_BUSY)|type;
	typeTime[chan] = __SYS_GetSystemTime();
#ifdef _SI_DEBUG
	printf("__si_gettypecallback(%d,%08x,%08x)\n",chan,type,si_type[chan]);
#endif
	sipad_en = __PADFixBits&SI_CHAN_BIT(chan);
	__PADFixBits &= ~SI_CHAN_BIT(chan);

	if(type&0x0f || ((si_type[chan]&SI_TYPE_MASK)-SI_TYPE_GC)
		|| !(si_type[chan]&SI_GC_WIRELESS) || si_type[chan]&SI_WIRELESS_IR) {
		SYS_SetWirelessID(chan,0);
		__si_calltypandstatuscallback(chan,si_type[chan]);
		return;
	}

	id = _SHIFTL(SYS_GetWirelessID(chan),8,16);
#ifdef _SI_DEBUG
	printf("__si_gettypecallback(id = %08x)\n",id);
#endif
	if(sipad_en && id&SI_WIRELESS_FIX_ID) {
		cmdfixdevice[chan] = 0x4e100000|(id&0x00CFFF00);
		si_type[chan] = SI_ERROR_BUSY;
		SI_Transfer(chan,&cmdfixdevice[chan],3,&si_type[chan],3,__si_gettypecallback,0);
		return;
	}

	if(si_type[chan]&SI_WIRELESS_FIX_ID) {
		if((id&0x00CFFF00)==(si_type[chan]&0x00CFFF00)) goto exit;
		if(!(id&SI_WIRELESS_FIX_ID)) {
			id = SI_WIRELESS_FIX_ID|(si_type[chan]&0x00CFFF00);
			SYS_SetWirelessID(chan,_SHIFTR(id,8,16));
		}
		cmdfixdevice[chan] = 0x4e000000|id;
		si_type[chan] = SI_ERROR_BUSY;
		SI_Transfer(chan,&cmdfixdevice[chan],3,&si_type[chan],3,__si_gettypecallback,0);
		return;
	}

	if(si_type[chan]&SI_WIRELESS_RECEIVED) {
		id = SI_WIRELESS_FIX_ID|(si_type[chan]&0x00CFFF00);
		SYS_SetWirelessID(chan,_SHIFTR(id,8,16));

		cmdfixdevice[chan] = 0x4e000000|id;
		si_type[chan] = SI_ERROR_BUSY;
		SI_Transfer(chan,&cmdfixdevice[chan],3,&si_type[chan],3,__si_gettypecallback,0);
		return;
	}
	SYS_SetWirelessID(chan,0);

exit:
	__si_calltypandstatuscallback(chan,si_type[chan]);
}

static void __si_transfernext(u32 chan)
{
	u32 cnt;
	u64 now;
	s64 diff;
#ifdef _SI_DEBUG
	printf("__si_transfernext(%d)\n",chan);
#endif
	cnt = 0;
	while(cnt<4) {
		chan++;
		chan %= 4;
#ifdef _SI_DEBUG
		printf("__si_transfernext(chan = %d,sipacket.chan = %d)\n",chan,sipacket[chan].chan);
#endif
		if(sipacket[chan].chan!=-1) {
			now = __SYS_GetSystemTime();
			diff = (now - sipacket[chan].fire);
			if(diff>=0) {
				if(!__si_transfer(sipacket[chan].chan,sipacket[chan].out,sipacket[chan].out_bytes,sipacket[chan].in,sipacket[chan].in_bytes,sipacket[chan].callback)) break;
				SYS_CancelAlarm(si_alarm[chan]);
				sipacket[chan].chan = -1;
			}
		}
		cnt++;
	}
}

static void __si_interrupthandler(u32 irq,frame_context *ctx)
{
	SICallback cb;
	u32 chn,curr_line,line,ret;
	sicomcsr csr;

	csr.val = _siReg[13];
#ifdef _SI_DEBUG
	printf("__si_interrupthandler(csr = %08x)\n",csr.val);
#endif
	if(csr.csrmap.tcintmsk && csr.csrmap.tcint) {
		chn = sicntrl.chan;
		cb = sicntrl.callback;
		sicntrl.callback = NULL;

		ret = __si_completetransfer();
		__si_transfernext(chn);

		if(cb) cb(chn,ret);

		_siReg[14] &= SISR_ERRORMASK(chn);

		if(si_type[chn]==SI_ERROR_BUSY && !SI_IsChanBusy(chn)) SI_Transfer(chn,&cmdtypeandstatus$47,1,&si_type[chn],3,__si_gettypecallback,65);
	}

	if(csr.csrmap.rdstintmsk && csr.csrmap.rdstint) {
		curr_line = VIDEO_GetCurrentLine();
		curr_line++;
		line = _SHIFTR(sicntrl.poll,16,10);

		chn = 0;
		while(chn<4) {
			if(SI_GetResponseRaw(chn)) inputBufferVCount[chn] = curr_line;
			chn++;
		}

		chn = 0;
		while(chn<4) {
			if(sicntrl.poll&SIPOLL_ENABLE(chn)) {
				if(!inputBufferVCount[chn] || ((line>>1)+inputBufferVCount[chn])<curr_line) return;
			}
			chn++;
		}

		chn = 0;
		while(chn<4) inputBufferVCount[chn++] = 0;

		chn = 0;
		while(chn<4) {
			if(rdstHandlers[chn]) rdstHandlers[chn](irq,ctx);
			chn++;
		}
	}
}

u32 SI_Sync()
{
	u32 level,ret;

	while(_siReg[13]&SICOMCSR_TSTART);

	_CPU_ISR_Disable(level);
	ret = __si_completetransfer();
	__si_transfernext(4);
	_CPU_ISR_Restore(level);

	return ret;
}

u32 SI_Busy()
{
	return (sicntrl.chan==-1)?0:1;
}

u32 SI_IsChanBusy(s32 chan)
{
	u32 ret = 0;

	if(sipacket[chan].chan!=-1 || sicntrl.chan==chan) ret = 1;

	return ret;
}

void SI_SetXY(u16 line,u8 cnt)
{
	u32 level;
#ifdef _SI_DEBUG
	printf("SI_SetXY(%d,%d)\n",line,cnt);
#endif
	_CPU_ISR_Disable(level);
	sicntrl.poll = (sicntrl.poll&~0x3ffff00)|_SHIFTL(line,16,10)|_SHIFTL(cnt,8,8);
	_siReg[12] = sicntrl.poll;
	_CPU_ISR_Restore(level);
}

void SI_EnablePolling(u32 poll)
{
	u32 level,mask;
#ifdef _SI_DEBUG
	printf("SI_EnablePolling(%08x)\n",poll);
#endif
	_CPU_ISR_Disable(level);
	poll >>= 24;
	mask = (poll>>4)&0x0f;
	sicntrl.poll &= ~mask;

	poll &= (0x03fffff0|mask);

	sicntrl.poll |= (poll&~0x03ffff00);
	SI_TransferCommands();
#ifdef _SI_DEBUG
	printf("SI_EnablePolling(%08x)\n",sicntrl.poll);
#endif
	_siReg[12] = sicntrl.poll;
	_CPU_ISR_Restore(level);
}

void SI_DisablePolling(u32 poll)
{
	u32 level,mask;
#ifdef _SI_DEBUG
	printf("SI_DisablePolling(%08x)\n",poll);
#endif
	_CPU_ISR_Disable(level);
	mask = (poll>>24)&0xf0;
	sicntrl.poll &= ~mask;
	_siReg[12] = sicntrl.poll;
	_CPU_ISR_Restore(level);
}

void SI_SetSamplingRate(u32 samplingrate)
{
	u32 div,level;
	struct _xy *xy = NULL;

	if(samplingrate>11) samplingrate = 11;

	_CPU_ISR_Disable(level);
	sampling_rate = samplingrate;
	xy = __si_getxy();

	div = 1;
	if(VIDEO_GetCurrentViMode()&VI_ENHANCED) div = 2;

	SI_SetXY(div*xy[samplingrate].line,xy[samplingrate].cnt);
	_CPU_ISR_Restore(level);
}

void SI_RefreshSamplingRate()
{
	SI_SetSamplingRate(sampling_rate);
}

u32 SI_GetStatus(s32 chan)
{
	u32 level,sisr;

	_CPU_ISR_Disable(level);
	sisr = (_siReg[14]>>((3-chan)<<3));
	if(sisr&SISR_NORESPONSE && !(si_type[chan]&SI_ERROR_BUSY)) si_type[chan] = SI_ERROR_NO_RESPONSE;
	_CPU_ISR_Restore(level);
	return sisr;
}

u32 SI_GetResponseRaw(s32 chan)
{
	u32 status,ret;
#ifdef _SI_DEBUG
	printf("SI_GetResponseRaw(%d)\n",chan);
#endif
	ret = 0;
	status = SI_GetStatus(chan);
	if(status&SISR_RDST) {
		inputBuffer[chan][0] = _siReg[(chan*3)+1];
		inputBuffer[chan][1] = _siReg[(chan*3)+2];
		inputBufferValid[chan] = 1;
		ret = 1;
	}
	return ret;
}

u32 SI_GetResponse(s32 chan,void *buf)
{
	u32 level,valid;
	_CPU_ISR_Disable(level);
	SI_GetResponseRaw(chan);
	valid = inputBufferValid[chan];
	inputBufferValid[chan] = 0;
#ifdef _SI_DEBUG
	printf("SI_GetResponse(%d,%p,%d)\n",chan,buf,valid);
#endif
	if(valid) {
		((u32*)buf)[0] = inputBuffer[chan][0];
		((u32*)buf)[1] = inputBuffer[chan][1];
	}
	_CPU_ISR_Restore(level);
	return valid;
}

void SI_SetCommand(s32 chan,u32 cmd)
{
	_siReg[chan*3] = cmd;
}

u32 SI_GetCommand(s32 chan)
{
	return (_siReg[chan*3]);
}

u32 SI_Transfer(s32 chan,void *out,u32 out_len,void *in,u32 in_len,SICallback cb,u32 us_delay)
{
	u32 ret = 0;
	u32 level;
	s64 diff;
	u64 now,fire;
	struct timespec tb;
#ifdef _SI_DEBUG
	printf("SI_Transfer(%d,%p,%d,%p,%d,%p,%d)\n",chan,out,out_len,in,in_len,cb,us_delay);
#endif
	_CPU_ISR_Disable(level);
	if(sipacket[chan].chan==-1 && sicntrl.chan!=chan) {
		ret = 1;
		fire = now = __SYS_GetSystemTime();
		if(us_delay) fire = xferTime[chan]+microsecs_to_ticks(us_delay);
		diff = (now - fire);
		if(diff<0) {
			tb.tv_sec = 0;
			tb.tv_nsec = ticks_to_nanosecs((fire - now));
			SYS_SetAlarm(si_alarm[chan],&tb,__si_alarmhandler,NULL);
		} else if(__si_transfer(chan,out,out_len,in,in_len,cb)) {
			_CPU_ISR_Restore(level);
			return ret;
		}
		sipacket[chan].chan = chan;
		sipacket[chan].out = out;
		sipacket[chan].out_bytes = out_len;
		sipacket[chan].in = in;
		sipacket[chan].in_bytes = in_len;
		sipacket[chan].callback = cb;
		sipacket[chan].fire = fire;
#ifdef _SI_DEBUG
		printf("SI_Transfer(%d,%p,%d,%p,%d,%p,%08x%08x)\n",sipacket[chan].chan,sipacket[chan].out,sipacket[chan].out_bytes,sipacket[chan].in,sipacket[chan].in_bytes,sipacket[chan].callback,(u32)(sipacket[chan].fire>>32),(u32)sipacket[chan].fire);
#endif
	}
	_CPU_ISR_Restore(level);
	return ret;
}

u32 SI_GetType(s32 chan)
{
	u32 level,type;
	u64 now;
	s64 diff;
#ifdef _SI_DEBUG
	printf("SI_GetType(%d)\n",chan);
#endif
	_CPU_ISR_Disable(level);
	now = __SYS_GetSystemTime();
	type = si_type[chan];
	diff = (now - typeTime[chan]);
	if(sicntrl.poll&(0x80>>chan)) {
		if(type!=SI_ERROR_NO_RESPONSE) {
			typeTime[chan] = __SYS_GetSystemTime();
			_CPU_ISR_Restore(level);
			return type;
		}
		si_type[chan] = type = SI_ERROR_BUSY;
	} else if(diff<=millisecs_to_ticks(50) && type!=SI_ERROR_NO_RESPONSE) {
		_CPU_ISR_Restore(level);
		return type;
	} else if(diff<=millisecs_to_ticks(75)) si_type[chan] = SI_ERROR_BUSY;
	else si_type[chan] = type = SI_ERROR_BUSY;

	typeTime[chan] = __SYS_GetSystemTime();

	SI_Transfer(chan,&cmdtypeandstatus$223,1,&si_type[chan],3,__si_gettypecallback,65);
	_CPU_ISR_Restore(level);

	return type;
}

u32 SI_GetTypeAsync(s32 chan,SICallback cb)
{
	u32 level;
	u32 type,i;
#ifdef _SI_DEBUG
	printf("SI_GetTypeAsync(%d)\n",chan);
#endif
	_CPU_ISR_Disable(level);
	type = SI_GetType(chan);
	if(si_type[chan]&SI_ERROR_BUSY) {
		i=0;
		for(i=0;i<4;i++) {
			if(!typeCallback[chan][i] && typeCallback[chan][i]!=cb) {
				typeCallback[chan][i] = cb;
				break;
			}
		}
		_CPU_ISR_Restore(level);
		return type;
	}

	cb(chan,type);
	_CPU_ISR_Restore(level);
	return type;
}

u32 SI_DecodeType(u32 type)
{
	if(type&SI_ERROR_NO_RESPONSE) return SI_ERROR_NO_RESPONSE;
	if(type&0x0f) return SI_ERROR_UNKNOWN;
	if(type&0xff) return SI_ERROR_BUSY;

	type &= ~0xffff;
	switch(type&SI_TYPE_MASK) {
		case SI_TYPE_N64:
			switch(type) {
				case SI_N64_CONTROLLER:
				case SI_N64_MIC:
				case SI_N64_KEYBOARD:
				case SI_N64_MOUSE:
				case SI_GBA:
					return type;
			}
			break;
		case SI_TYPE_GC:
			if(type==SI_GC_STEERING) return SI_GC_STEERING;
			if((type&~0x001f0000)==SI_GC_KEYBOARD) return SI_GC_KEYBOARD;
			if((type&SI_GC_WIRELESS) && !(type&SI_WIRELESS_IR)) {
				if((type&SI_GC_WAVEBIRD)==SI_GC_WAVEBIRD) return SI_GC_WAVEBIRD;
				else if(!(type&SI_WIRELESS_STATE)) return SI_GC_RECEIVER;
			}
			if((type&SI_GC_CONTROLLER)==SI_GC_CONTROLLER) return SI_GC_CONTROLLER;
			break;
	}
	return SI_ERROR_UNKNOWN;
}

u32 SI_Probe(s32 chan)
{
	u32 type;
	type = SI_GetType(chan);
	type = SI_DecodeType(type);
	return type;
}

char *SI_GetTypeString(u32 type)
{
	switch(SI_DecodeType(type)) {
		case SI_ERROR_BUSY:
			return "Busy";
		case SI_ERROR_UNKNOWN:
			return "Unknown";
		case SI_ERROR_NO_RESPONSE:
			return "No response";
		case SI_N64_CONTROLLER:
			return "N64 controller";
		case SI_N64_MIC:
			return "N64 microphone";
		case SI_N64_KEYBOARD:
			return "N64 keyboard";
		case SI_N64_MOUSE:
			return "N64 mouse";
		case SI_GBA:
			return "GameBoy Advance";
		case SI_GC_CONTROLLER:
			return "Standard controller";
		case SI_GC_RECEIVER:
			return "Wireless receiver";
		case SI_GC_WAVEBIRD:
			return "WaveBird controller";
		case SI_GC_KEYBOARD:
			return "Keyboard";
		case SI_GC_STEERING:
			return "Steering";
		default:
			return "Unknown";
	}
}

void SI_TransferCommands()
{
	_siReg[14] = 0x80000000;
}

u32 SI_RegisterPollingHandler(RDSTHandler handler)
{
	u32 level,i;

	_CPU_ISR_Disable(level);

	i = 0;
	for(i=0;i<4;i++) {
		if(rdstHandlers[i]==handler) {
			_CPU_ISR_Restore(level);
			return 1;
		}
	}

	for(i=0;i<4;i++) {
		if(rdstHandlers[i]==NULL) {
			rdstHandlers[i] = handler;
			SI_EnablePollingInterrupt(TRUE);
			_CPU_ISR_Restore(level);
			return 1;
		}
	}

	_CPU_ISR_Restore(level);
	return 0;
}

u32 SI_UnregisterPollingHandler(RDSTHandler handler)
{
	u32 level,i;

	_CPU_ISR_Disable(level);
	for(i=0;i<4;i++) {
		if(rdstHandlers[i]==handler) {
			rdstHandlers[i] = NULL;
			for(i=0;i<4;i++) {
				if(rdstHandlers[i]!=NULL) break;
			}
			if(i>=4) SI_EnablePollingInterrupt(FALSE);

			_CPU_ISR_Restore(level);
			return 1;
		}
	}
	_CPU_ISR_Restore(level);
	return 0;
}

u32 SI_EnablePollingInterrupt(s32 enable)
{
	sicomcsr csr;
	u32 level,ret,i;

	_CPU_ISR_Disable(level);

	ret = 0;
	csr.val = _siReg[13];
	if(csr.csrmap.rdstintmsk) ret = 1;

	if(enable) {
		csr.csrmap.rdstintmsk = 1;
		for(i=0;i<4;i++) inputBufferVCount[i] = 0;
	} else
		csr.csrmap.rdstintmsk = 0;

	csr.val &= 0x7ffffffe;
	_siReg[13] = csr.val;

	_CPU_ISR_Restore(level);
	return ret;
}

void __si_init()
{
	u32 i;
#ifdef _SI_DEBUG
	printf("__si_init()\n");
#endif
	for(i=0;i<4;i++) {
		sipacket[i].chan = -1;
		SYS_CreateAlarm(&si_alarm[i]);
	}
	sicntrl.poll = 0;

	SI_SetSamplingRate(0);
	while(_siReg[13]&0x0001);
	_siReg[13] = 0x80000000;

	_siReg[15] &= ~0x80000000;		// permit exi clock to be set to 32MHz

	IRQ_Request(IRQ_PI_SI,__si_interrupthandler);
	__UnmaskIrq(IRQMASK(IRQ_PI_SI));

	SI_GetType(0);
	SI_GetType(1);
	SI_GetType(2);
	SI_GetType(3);
}
