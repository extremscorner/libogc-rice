/*-------------------------------------------------------------

audio.c -- Audio subsystem

Copyright (C) 2004 - 2025
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)
Extrems' Corner.org

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

#include <stdlib.h>
#include "asm.h"
#include "processor.h"
#include "irq.h"
#include "audio.h"
#include "timesupp.h"

// DSPCR bits
#define DSPCR_DSPRESET		0x0800        // Reset DSP
#define DSPCR_DSPDMA		0x0200        // ARAM dma in progress, if set
#define DSPCR_DSPINTMSK		0x0100        // * interrupt mask   (RW)
#define DSPCR_DSPINT		0x0080        // * interrupt active (RWC)
#define DSPCR_ARINTMSK		0x0040
#define DSPCR_ARINT			0x0020
#define DSPCR_AIINTMSK		0x0010
#define DSPCR_AIINT			0x0008
#define DSPCR_HALT			0x0004        // halt DSP
#define DSPCR_PIINT			0x0002        // assert DSP PI interrupt
#define DSPCR_RES			0x0001        // reset DSP

// Audio Interface Registers
#define AI_CONTROL			0
#define AI_STREAM_VOL		1
#define AI_SAMPLE_COUNT		2
#define AI_INT_TIMING		3

#define AI_PSTAT			0x01
#define AI_AISFR			0x02
#define AI_AIINTMSK			0x04
#define AI_AIINT			0x08
#define AI_AIINTVLD			0x10
#define AI_SCRESET			0x20
#define AI_DMAFR			0x40

#define _SHIFTL(v, s, w)	\
    ((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

#if defined(HW_DOL)
	static vu32* const _aiReg = (u32*)0xCC006C00;
#elif defined(HW_RVL)
	static vu32* const _aiReg = (u32*)0xCD006C00;
#else
	#error HW model not supported.
#endif

static vu16* const _dspReg = (u16*)0xCC005000;

static u32 __AIInitFlag = 0;
static u64 bound_32KHz,bound_48KHz,min_wait,max_wait,buffer;

#if defined(HW_DOL)
static AISCallback __AIS_Callback;
#endif
static AIDCallback __AID_Callback;

#if defined(HW_DOL)
static void __AISHandler(u32 nIrq,frame_context *pCtx)
{
	if(__AIS_Callback)
		__AIS_Callback(_aiReg[AI_SAMPLE_COUNT]);
	_aiReg[AI_CONTROL] |= AI_AIINT;
}
#endif

static void __AIDHandler(u32 nIrq,frame_context *pCtx)
{
	_dspReg[5] = (_dspReg[5]&~(DSPCR_DSPINT|DSPCR_ARINT))|DSPCR_AIINT;
	if(__AID_Callback)
		__AID_Callback();
}

static void __AISRCINIT()
{
	int done = 0;
	u32 sample_counter;
	u64 time1, time2, tdiff;
	u64 wait = 0;

	while (!done) {
		_aiReg[AI_CONTROL] |=  AI_SCRESET;
		_aiReg[AI_CONTROL] &= ~AI_AISFR;
		_aiReg[AI_CONTROL] |=  AI_PSTAT;

#ifdef HW_DOL
		sample_counter = _aiReg[AI_SAMPLE_COUNT];
		while (sample_counter == _aiReg[AI_SAMPLE_COUNT]) {}
#else
		sample_counter = _aiReg[AI_SAMPLE_COUNT] & 0x7fffffff;
		while (sample_counter == (_aiReg[AI_SAMPLE_COUNT] & 0x7fffffff)) {}
#endif

		time1 = gettime();

		_aiReg[AI_CONTROL] |= AI_AISFR;
		_aiReg[AI_CONTROL] |= AI_PSTAT;

#ifdef HW_DOL
		sample_counter = _aiReg[AI_SAMPLE_COUNT];
		while (sample_counter == _aiReg[AI_SAMPLE_COUNT]) {}
#else
		sample_counter = _aiReg[AI_SAMPLE_COUNT] & 0x7fffffff;
		while (sample_counter == (_aiReg[AI_SAMPLE_COUNT] & 0x7fffffff)) {}
#endif

		time2 = gettime();
		tdiff = time2 - time1;

		_aiReg[AI_CONTROL] &= ~AI_AISFR;
		_aiReg[AI_CONTROL] &= ~AI_PSTAT;

		if ((tdiff > (bound_32KHz - buffer)) &&
			(tdiff < (bound_32KHz + buffer))) {
			if (tdiff < (bound_48KHz - buffer)) {
				wait = max_wait;
				done = 1;
			}
		} else {
			wait = min_wait;
			done = 1;
		}
	}

	while (diff_ticks(time2, gettime()) < wait) {}
}

void AUDIO_Init()
{
	u32 rate,level;

	if(!__AIInitFlag) {
		bound_32KHz = nanosecs_to_ticks(31524);
		bound_48KHz = nanosecs_to_ticks(42024);
		min_wait = nanosecs_to_ticks(42000);
		max_wait = nanosecs_to_ticks(63000);
		buffer = nanosecs_to_ticks(3000);

		_aiReg[AI_CONTROL] &= ~(AI_AIINTVLD|AI_AIINTMSK|AI_PSTAT);
		_aiReg[AI_STREAM_VOL] = 0;
		_aiReg[AI_INT_TIMING] = 0;

		_aiReg[AI_CONTROL] = (_aiReg[AI_CONTROL]&~AI_SCRESET)|AI_SCRESET;

		rate = (_SHIFTR(_aiReg[AI_CONTROL],6,1))^1;
		if(rate==AI_SAMPLERATE_48KHZ) {
			_aiReg[AI_CONTROL] &= ~AI_DMAFR;
			_CPU_ISR_Disable(level);
			__AISRCINIT();
			_aiReg[AI_CONTROL] |= AI_DMAFR;
			_CPU_ISR_Restore(level);
		}

		__AID_Callback = NULL;

		IRQ_Request(IRQ_DSP_AI,__AIDHandler);
		__UnmaskIrq(IRQMASK(IRQ_DSP_AI));
#if defined(HW_DOL)
		__AIS_Callback = NULL;

		IRQ_Request(IRQ_AI,__AISHandler);
		__UnmaskIrq(IRQMASK(IRQ_AI));
#endif
		__AIInitFlag = 1;
	}
}

#if defined(HW_DOL)
void AUDIO_SetStreamVolLeft(u8 vol)
{
	_aiReg[AI_STREAM_VOL] = (_aiReg[AI_STREAM_VOL]&~0x000000ff)|(vol&0xff);
}

u8 AUDIO_GetStreamVolLeft()
{
	return (u8)(_aiReg[AI_STREAM_VOL]&0xff);
}

void AUDIO_SetStreamVolRight(u8 vol)
{
	_aiReg[AI_STREAM_VOL] = (_aiReg[AI_STREAM_VOL]&~0x0000ff00)|(_SHIFTL(vol,8,8));
}

u8 AUDIO_GetStreamVolRight()
{
	return (u8)(_SHIFTR(_aiReg[AI_STREAM_VOL],8,8));
}

void AUDIO_SetStreamSampleRate(u32 rate)
{
	_aiReg[AI_CONTROL] = (_aiReg[AI_CONTROL]&~AI_AISFR)|(_SHIFTL(rate,1,1));
}

u32 AUDIO_GetStreamSampleRate()
{
	return _SHIFTR(_aiReg[AI_CONTROL],1,1);
}

void AUDIO_SetStreamTrigger(u32 cnt)
{
	_aiReg[AI_INT_TIMING] = (_aiReg[AI_INT_TIMING]&~0x7fffffff)|(cnt&0x7fffffff);
}

void AUDIO_ResetStreamSampleCnt()
{
	_aiReg[AI_CONTROL] = (_aiReg[AI_CONTROL]&~AI_SCRESET)|AI_SCRESET;
}

void AUDIO_SetStreamPlayState(u32 state)
{
	u32 playstate,streamrate;
	u32 volright,volleft,level;

	playstate = AUDIO_GetStreamPlayState();
	streamrate = AUDIO_GetStreamSampleRate();
	if(playstate!=state && state==AI_STREAM_START && streamrate==AI_SAMPLERATE_32KHZ ) {
		volright = AUDIO_GetStreamVolRight();
		AUDIO_SetStreamVolRight(0);
		volleft = AUDIO_GetStreamVolLeft();
		AUDIO_SetStreamVolLeft(0);

		_CPU_ISR_Disable(level);
		__AISRCINIT();
		_aiReg[AI_CONTROL] = (_aiReg[AI_CONTROL]&~AI_SCRESET)|AI_SCRESET;
		_aiReg[AI_CONTROL] = (_aiReg[AI_CONTROL]&~0x01)|0x01;
		_CPU_ISR_Restore(level);
		AUDIO_SetStreamVolRight(volright);
		AUDIO_SetStreamVolLeft(volleft);
	} else {
		_aiReg[AI_CONTROL] = (_aiReg[AI_CONTROL]&~AI_PSTAT)|(state&AI_PSTAT);
	}
}

u32 AUDIO_GetStreamPlayState()
{
	return (_aiReg[AI_CONTROL]&AI_PSTAT);
}
#endif

AIDCallback AUDIO_RegisterDMACallback(AIDCallback callback)
{
	u32 level;
	AIDCallback old;

	_CPU_ISR_Disable(level);
	old = __AID_Callback;
	__AID_Callback = callback;
	_CPU_ISR_Restore(level);
	return old;
}

void AUDIO_InitDMA(u32 startaddr,u32 len)
{
	u32 level;

	_CPU_ISR_Disable(level);
	_dspReg[24] = (_dspReg[24]&~0x1fff)|(_SHIFTR(startaddr,16,13));
	_dspReg[25] = (_dspReg[25]&~0xffe0)|(startaddr&0xffff);
	_dspReg[27] = (_dspReg[27]&~0x7fff)|(_SHIFTR(len,5,15));
	_CPU_ISR_Restore(level);
}

u16 AUDIO_GetDMAEnableFlag()
{
	return (_SHIFTR(_dspReg[27],15,1));
}

void AUDIO_StartDMA()
{
	_dspReg[27] = (_dspReg[27]&~0x8000)|0x8000;
}

void AUDIO_StopDMA()
{
	_dspReg[27] = (_dspReg[27]&~0x8000);
}

u32 AUDIO_GetDMABytesLeft()
{
	return (_SHIFTL(_dspReg[29],5,15));
}

u32 AUDIO_GetDMAStartAddr()
{
	return (_SHIFTL((_dspReg[24]&0x1fff),16,13)|(_dspReg[25]&0xffe0));
}

u32 AUDIO_GetDMALength()
{
	return ((_dspReg[27]&0x7fff)<<5);
}

void AUDIO_SetDSPSampleRate(u32 rate)
{
	u32 level;

	if(AUDIO_GetDSPSampleRate()!=rate) {
		_aiReg[AI_CONTROL] &= ~AI_DMAFR;
		if(rate==AI_SAMPLERATE_32KHZ) {
			_CPU_ISR_Disable(level);
			__AISRCINIT();
			_aiReg[AI_CONTROL] |= AI_DMAFR;
			_CPU_ISR_Restore(level);
		}
	}
}

u32 AUDIO_GetDSPSampleRate()
{
	return (_SHIFTR(_aiReg[AI_CONTROL],6,1))^1;		//0^1(1) = 48Khz, 1^1(0) = 32Khz
}

#if defined(HW_DOL)
void AUDIO_SetHighResolution(u32 enable)
{
	_aiReg[AI_INT_TIMING] = (_aiReg[AI_INT_TIMING]&~0x80000000)|(_SHIFTL(enable,31,1));
}

u32 AUDIO_GetHighResolution()
{
	return (_SHIFTR(_aiReg[AI_INT_TIMING],31,1));
}
#endif
