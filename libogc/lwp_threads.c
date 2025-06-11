/*
 * Copyright (C) 1989-1999 On-Line Applications Research Corporation (OAR).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-------------------------------------------------------------

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
#include <stdio.h>
#include <string.h>
#include "asm.h"
#include "processor.h"
#include "sys_state.h"
#include "lwp_stack.h"
#include "lwp_threads.h"
#include "lwp_threadq.h"
#include "lwp_watchdog.h"

#define LWP_MAXPRIORITIES		256

/* new one */
frame_context core_context;

lwp_cntrl *_thr_main = NULL;
lwp_cntrl *_thr_idle = NULL;

lwp_cntrl *_thr_executing = NULL;
lwp_cntrl *_thr_heir = NULL;

vu32 _context_switch_want;
vu32 _thread_dispatch_disable_level;

wd_cntrl _lwp_wd_timeslice;
u32 _lwp_ticks_per_timeslice = 0;
void **__lwp_thr_libc_reent = NULL;
lwp_queue _lwp_thr_ready[LWP_MAXPRIORITIES];

static void (*_lwp_exitfunc)(void);

extern void _cpu_context_switch(void *,void *);
extern void _cpu_context_switch_ex(void *,void *);
extern void _cpu_context_save_fp(void *);
extern void _cpu_context_restore_fp(void *);

extern int __libc_create_hook(lwp_cntrl *,lwp_cntrl *);
extern int __libc_start_hook(lwp_cntrl *,lwp_cntrl *);
extern int __libc_delete_hook(lwp_cntrl *, lwp_cntrl *);

extern void kprintf(const char *fmt,...);

#ifdef _LWPTHREADS_DEBUG
static void __lwp_dumpcontext(frame_context *ctx)
{
	kprintf("GPR00 %08x GPR08 %08x GPR16 %08x GPR24 %08x\n",ctx->GPR[0], ctx->GPR[8], ctx->GPR[16], ctx->GPR[24]);
	kprintf("GPR01 %08x GPR09 %08x GPR17 %08x GPR25 %08x\n",ctx->GPR[1], ctx->GPR[9], ctx->GPR[17], ctx->GPR[25]);
	kprintf("GPR02 %08x GPR10 %08x GPR18 %08x GPR26 %08x\n",ctx->GPR[2], ctx->GPR[10], ctx->GPR[18], ctx->GPR[26]);
	kprintf("GPR03 %08x GPR11 %08x GPR19 %08x GPR27 %08x\n",ctx->GPR[3], ctx->GPR[11], ctx->GPR[19], ctx->GPR[27]);
	kprintf("GPR04 %08x GPR12 %08x GPR20 %08x GPR28 %08x\n",ctx->GPR[4], ctx->GPR[12], ctx->GPR[20], ctx->GPR[28]);
	kprintf("GPR05 %08x GPR13 %08x GPR21 %08x GPR29 %08x\n",ctx->GPR[5], ctx->GPR[13], ctx->GPR[21], ctx->GPR[29]);
	kprintf("GPR06 %08x GPR14 %08x GPR22 %08x GPR30 %08x\n",ctx->GPR[6], ctx->GPR[14], ctx->GPR[22], ctx->GPR[30]);
	kprintf("GPR07 %08x GPR15 %08x GPR23 %08x GPR31 %08x\n",ctx->GPR[7], ctx->GPR[15], ctx->GPR[23], ctx->GPR[31]);
	kprintf("LR %08x SRR0 %08x SRR1 %08x MSR %08x\n\n", ctx->LR, ctx->SRR0, ctx->SRR1,ctx->MSR);
}

void __lwp_showmsr()
{
	register u32 msr;
	_CPU_MSR_GET(msr);
	kprintf("msr: %08x\n",msr);
}

void __lwp_dumpcontext_fp(lwp_cntrl *thrA,lwp_cntrl *thrB)
{
	kprintf("_cpu_contextfp_dump(%p,%p)\n",thrA,thrB);
}
#endif

void __lwp_thread_delayended(void *arg)
{
	lwp_cntrl *thethread = (lwp_cntrl*)arg;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_delayended(%p)\n",thethread);
#endif
	if(!thethread) return;
	
	__lwp_thread_dispatchdisable();
	__lwp_thread_unblock(thethread);
	__lwp_thread_dispatchunnest();
}

void __lwp_thread_tickle_timeslice(void *arg)
{
	s64 ticks;
	lwp_cntrl *exec;

	exec = _thr_executing;
	ticks = millisecs_to_ticks(1);
	
	__lwp_thread_dispatchdisable();

	if(!__lwp_stateready(exec->cur_state)) {
		__lwp_wd_insert_ticks(&_lwp_wd_timeslice,ticks);
		__lwp_thread_dispatchunnest();
		return;
	}

	switch(exec->budget_algo) {
		case LWP_CPU_BUDGET_ALGO_NONE:
			break;
		case LWP_CPU_BUDGET_ALGO_TIMESLICE:
			if((--exec->cpu_time_budget)==0) {
				__lwp_thread_resettimeslice();
				exec->cpu_time_budget = _lwp_ticks_per_timeslice;
			}
			break;
	}

	__lwp_wd_insert_ticks(&_lwp_wd_timeslice,ticks);
	__lwp_thread_dispatchunnest();
}

void __thread_dispatch()
{
	u32 level;
	lwp_cntrl *exec,*heir;

	_CPU_ISR_Disable(level);
	exec = _thr_executing;
	while(_context_switch_want==TRUE) {
		heir = _thr_heir;
		_thread_dispatch_disable_level = 1;
		_context_switch_want = FALSE;
		_thr_executing = heir;
		_CPU_ISR_Restore(level);

		if(__lwp_thr_libc_reent) {
			exec->libc_reent = *__lwp_thr_libc_reent;
			*__lwp_thr_libc_reent = heir->libc_reent;
		}
#ifdef _DEBUG
		_cpu_context_switch_ex((void*)&exec->context,(void*)&heir->context);
#else
		_cpu_context_switch((void*)&exec->context,(void*)&heir->context);
#endif
		exec = _thr_executing;
		_CPU_ISR_Disable(level);
	}
	_thread_dispatch_disable_level = 0;
	_CPU_ISR_Restore(level);
}

static void __lwp_thread_handler()
{
	lwp_cntrl *exec;

	exec = _thr_executing;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_handler(%p,%d)\n",exec,_thread_dispatch_disable_level);
#endif
	__lwp_thread_dispatchenable();
	exec->wait.ret_arg = exec->entry(exec->arg);

	__lwp_thread_exit(exec,exec->wait.ret_arg);
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_handler(%p): thread returned(%p)\n",exec,exec->wait.ret_arg);
#endif
}

void __lwp_rotate_readyqueue(u32 prio)
{
	u32 level;
	lwp_cntrl *exec;
	lwp_queue *ready;
	lwp_node *node;

	ready = &_lwp_thr_ready[prio];
	exec = _thr_executing;
	
	if(ready==exec->ready) {
		__lwp_thread_yield();
		return;
	}

	_CPU_ISR_Disable(level);
	if(!__lwp_queue_isempty(ready) && !__lwp_queue_onenode(ready)) {
		node = __lwp_queue_firstnodeI(ready);
		__lwp_queue_appendI(ready,node);
	}
	_CPU_ISR_Flash(level);
	
	if(_thr_heir->ready==ready)
		_thr_heir = (lwp_cntrl*)ready->first;

	if(exec!=_thr_heir)
		_context_switch_want = TRUE;

#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_rotate_readyqueue(%d,%p,%p)\n",prio,exec,_thr_heir);
#endif
	_CPU_ISR_Restore(level);
}

void __lwp_thread_yield()
{
	u32 level;
	lwp_cntrl *exec;
	lwp_queue *ready;
	
	exec = _thr_executing;
	ready = exec->ready;
	
	_CPU_ISR_Disable(level);
	if(!__lwp_queue_onenode(ready)) {
		__lwp_queue_extractI(&exec->object.node);
		__lwp_queue_appendI(ready,&exec->object.node);
		_CPU_ISR_Flash(level);
		if(__lwp_thread_isheir(exec))
			_thr_heir = (lwp_cntrl*)ready->first;
		_context_switch_want = TRUE;
	} else if(!__lwp_thread_isheir(exec))
		_context_switch_want = TRUE;
	_CPU_ISR_Restore(level);
}

void __lwp_thread_resettimeslice()
{
	u32 level;
	lwp_cntrl *exec;
	lwp_queue *ready;

	exec = _thr_executing;
	ready = exec->ready;

	_CPU_ISR_Disable(level);
	if(__lwp_queue_onenode(ready)) {
		_CPU_ISR_Restore(level);
		return;
	}

	__lwp_queue_extractI(&exec->object.node);
	__lwp_queue_appendI(ready,&exec->object.node);

	_CPU_ISR_Flash(level);

	if(__lwp_thread_isheir(exec))
		_thr_heir = (lwp_cntrl*)ready->first;

	_context_switch_want = TRUE;
	_CPU_ISR_Restore(level);
}

void __lwp_thread_setstate(lwp_cntrl *thethread,u32 state)
{
	u32 level;
	lwp_queue *ready;

	ready = thethread->ready;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_setstate(%d,%p,%p,%08x)\n",_context_switch_want,_thr_heir,thethread,thethread->cur_state);
#endif
	_CPU_ISR_Disable(level);
	if(!__lwp_stateready(thethread->cur_state)) {
		thethread->cur_state = __lwp_clearstate(thethread->cur_state,state);
		_CPU_ISR_Restore(level);
		return;
	}

	thethread->cur_state = state;
	if(__lwp_queue_onenode(ready)) {
		__lwp_queue_init_empty(ready);
		__lwp_priomap_removefrom(&thethread->priomap);
	} else 
		__lwp_queue_extractI(&thethread->object.node);
	_CPU_ISR_Flash(level);

	if(__lwp_thread_isheir(thethread))
		__lwp_thread_calcheir();
	if(__lwp_thread_isexec(thethread))
		_context_switch_want = TRUE;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_setstate(%d,%p,%p,%08x)\n",_context_switch_want,_thr_heir,thethread,thethread->cur_state);
#endif
	_CPU_ISR_Restore(level);
}

void __lwp_thread_clearstate(lwp_cntrl *thethread,u32 state)
{
	u32 level,cur_state;
	
	_CPU_ISR_Disable(level);

	cur_state = thethread->cur_state;
	if(__lwp_statesset(cur_state,state)) {
		cur_state = thethread->cur_state = __lwp_clearstate(cur_state,state);
		if(__lwp_stateready(cur_state)) {
			__lwp_priomap_addto(&thethread->priomap);
			__lwp_queue_appendI(thethread->ready,&thethread->object.node);
			_CPU_ISR_Flash(level);
			
			if(thethread->cur_prio<_thr_heir->cur_prio) {
				_thr_heir = thethread;
				_context_switch_want = TRUE;
			}
		}
	}

	_CPU_ISR_Restore(level);
}

void __lwp_thread_changepriority(lwp_cntrl *thethread,u32 prio,u32 prependit)
{
	u32 level;
	
	__lwp_thread_settransient(thethread);
	
	if(thethread->cur_prio!=prio) 
		__lwp_thread_setpriority(thethread,prio);

	_CPU_ISR_Disable(level);

	thethread->cur_state = __lwp_clearstate(thethread->cur_state,LWP_STATES_TRANSIENT);
	if(!__lwp_stateready(thethread->cur_state)) {
		_CPU_ISR_Restore(level);
		return;
	}

	__lwp_priomap_addto(&thethread->priomap);
	if(prependit)
		__lwp_queue_prependI(thethread->ready,&thethread->object.node);
	else
		__lwp_queue_appendI(thethread->ready,&thethread->object.node);

	_CPU_ISR_Flash(level);

	__lwp_thread_calcheir();
	
	if(!__lwp_thread_isheir(_thr_executing))
		_context_switch_want = TRUE;

	_CPU_ISR_Restore(level);
}

void __lwp_thread_setpriority(lwp_cntrl *thethread,u32 prio)
{
	thethread->cur_prio = prio;
	thethread->ready = &_lwp_thr_ready[prio];
	__lwp_priomap_init(&thethread->priomap,prio);
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_setpriority(%p,%d,%p)\n",thethread,prio,thethread->ready);
#endif
}

void __lwp_thread_suspend(lwp_cntrl *thethread)
{
	u32 level;
	lwp_queue *ready;

	ready = thethread->ready;
	
	_CPU_ISR_Disable(level);
	thethread->suspendcnt++;
	if(!__lwp_stateready(thethread->cur_state)) {
		thethread->cur_state = __lwp_setstate(thethread->cur_state,LWP_STATES_SUSPENDED);
		_CPU_ISR_Restore(level);
		return;
	}
	
	thethread->cur_state = LWP_STATES_SUSPENDED;
	if(__lwp_queue_onenode(ready)) {
		__lwp_queue_init_empty(ready);
		__lwp_priomap_removefrom(&thethread->priomap);
	} else {
		__lwp_queue_extractI(&thethread->object.node);
	}
	_CPU_ISR_Flash(level);
	
	if(__lwp_thread_isheir(thethread))
		__lwp_thread_calcheir();
	
	if(__lwp_thread_isexec(thethread))
		_context_switch_want = TRUE;

	_CPU_ISR_Restore(level);
}

void __lwp_thread_settransient(lwp_cntrl *thethread)
{
	u32 level,oldstates;
	lwp_queue *ready;

	ready = thethread->ready;

	_CPU_ISR_Disable(level);
	
	oldstates = thethread->cur_state;
	thethread->cur_state = __lwp_setstate(oldstates,LWP_STATES_TRANSIENT);

	if(__lwp_stateready(oldstates)) {
		if(__lwp_queue_onenode(ready)) {
			__lwp_queue_init_empty(ready);
			__lwp_priomap_removefrom(&thethread->priomap);
		} else {
			__lwp_queue_extractI(&thethread->object.node);
		}
	}

	_CPU_ISR_Restore(level);
}

void __lwp_thread_resume(lwp_cntrl *thethread,u32 force)
{
	u32 level,state;
	
	_CPU_ISR_Disable(level);

	if(force==TRUE)
		thethread->suspendcnt = 0;
	else
		thethread->suspendcnt--;
	
	if(thethread->suspendcnt>0) {
		_CPU_ISR_Restore(level);
		return;
	}

	state = thethread->cur_state;
	if(__lwp_statesuspended(state)) {
		state = thethread->cur_state = __lwp_clearstate(thethread->cur_state,LWP_STATES_SUSPENDED);
		if(__lwp_stateready(state)) {
			__lwp_priomap_addto(&thethread->priomap);
			__lwp_queue_appendI(thethread->ready,&thethread->object.node);
			_CPU_ISR_Flash(level);
			if(thethread->cur_prio<_thr_heir->cur_prio) {
				_thr_heir = thethread;
				_context_switch_want = TRUE;
			}
		}
	}
	_CPU_ISR_Restore(level);
}

void __lwp_thread_loadenv(lwp_cntrl *thethread)
{
	u32 stackbase,sp,size;

	stackbase = (u32)thethread->stack;
	size = thethread->stack_size;

	// tag both bottom & head of stack
	*((u32*)stackbase) = 0xDEADBABE;
	sp = stackbase+size-CPU_MINIMUM_STACK_FRAME_SIZE;
	*((u32*)sp) = 0;
	
	thethread->context.GPR[1] = sp;
	
	thethread->context.LR = (u32)__lwp_thread_handler;
	thethread->context.MSR = MSR_EE|MSR_FP|MSR_ME|MSR_IR|MSR_DR|MSR_RI;

#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_loadenv(%p,%p,%d,%p)\n",thethread,(void*)stackbase,size,(void*)sp);
#endif
}

void __lwp_thread_ready(lwp_cntrl *thethread)
{
	u32 level;
	lwp_cntrl *heir;

	_CPU_ISR_Disable(level);
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_ready(%p)\n",thethread);
#endif
	thethread->cur_state = LWP_STATES_READY;
	__lwp_priomap_addto(&thethread->priomap);
	__lwp_queue_appendI(thethread->ready,&thethread->object.node);
	_CPU_ISR_Flash(level);

	__lwp_thread_calcheir();
	heir = _thr_heir;
	if(!__lwp_thread_isexec(heir))
		_context_switch_want = TRUE;
	
	_CPU_ISR_Restore(level);
}

u32 __lwp_thread_init(lwp_cntrl *thethread,void *stack_area,u32 stack_size,u32 prio)
{
	u32 act_stack_size = 0;

#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_init(%p,%p,%d,%d)\n",thethread,stack_area,stack_size,prio);
#endif

	if(!stack_area) {
		act_stack_size = __lwp_stack_allocate(thethread,stack_size);
		if(!act_stack_size) return 0;
		thethread->stack_allocated = TRUE;
	} else {
		thethread->stack = stack_area;
		act_stack_size = stack_size;
		thethread->stack_allocated = FALSE;
	}
	thethread->stack_size = act_stack_size;

	__lwp_threadqueue_init(&thethread->join_list,LWP_THREADQ_MODEFIFO,LWP_STATES_WAITING_FOR_JOIN,0);

	memset(&thethread->context,0,sizeof(thethread->context));
	memset(&thethread->wait,0,sizeof(thethread->wait));

	thethread->budget_algo = (prio<128 ? LWP_CPU_BUDGET_ALGO_NONE : LWP_CPU_BUDGET_ALGO_TIMESLICE);
	thethread->real_prio = prio;
	thethread->cur_state = LWP_STATES_DORMANT;
	thethread->cpu_time_budget = _lwp_ticks_per_timeslice;
	thethread->suspendcnt = 0;
	thethread->res_cnt = 0;
	__lwp_thread_setpriority(thethread,prio);

	__libc_create_hook(_thr_executing,thethread);

	return 1;
}

void __lwp_thread_close(lwp_cntrl *thethread)
{
	__lwp_thread_setstate(thethread,LWP_STATES_TRANSIENT);
	
	if(!__lwp_threadqueue_extractproxy(thethread)) {
		if(__lwp_wd_isactive(&thethread->timer)) {
			__lwp_wd_deactivate(&thethread->timer);
			__lwp_wd_remove_ticks(&thethread->timer);
		}
	}

	__libc_delete_hook(_thr_executing,thethread);

	__lwp_stack_free(thethread);

	__lwp_objmgr_close(thethread->object.information,&thethread->object);
	__lwp_objmgr_free(thethread->object.information,&thethread->object);
}

void __lwp_thread_closeall()
{
	u32 i,level;
	lwp_queue *header;
	lwp_cntrl *ptr,*next;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_closeall(enter)\n");
#endif
	_CPU_ISR_Disable(level);
	for(i=0;i<LWP_MAXPRIORITIES;i++) {
		header = &_lwp_thr_ready[i];
		ptr = (lwp_cntrl*)header->first;
		while(ptr!=(lwp_cntrl*)__lwp_queue_tail(&_lwp_thr_ready[i])) {
			next = (lwp_cntrl*)ptr->object.node.next;
			if(ptr!=_thr_executing)
				__lwp_thread_close(ptr);

			ptr = next;
		}
	}
	_CPU_ISR_Restore(level);
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_closeall(leave)\n");
#endif
}

void __lwp_thread_exit(lwp_cntrl *thethread,void *value_ptr)
{
	lwp_cntrl *p;

	__lwp_thread_dispatchdisable();
	thethread->wait.ret_arg = value_ptr;
	if((p=__lwp_threadqueue_dequeue(&thethread->join_list))) {
		do {
			*(void**)p->wait.ret_arg = value_ptr;
		} while((p=__lwp_threadqueue_dequeue(&thethread->join_list)));
	} else {
		__lwp_thread_setstate(thethread,LWP_STATES_WAITING_FOR_JOINATEXIT);
		__lwp_thread_dispatchenable();
		__lwp_thread_dispatchdisable();
	}
	__lwp_thread_close(thethread);
	__lwp_thread_dispatchenable();
}

u32 __lwp_thread_start(lwp_cntrl *thethread,void* (*entry)(void*),void *arg)
{
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_thread_start(%p,%p,%p,%d)\n",thethread,entry,arg,thethread->cur_state);
#endif
	if(__lwp_statedormant(thethread->cur_state)) {
		thethread->entry = entry;
		thethread->arg = arg;
		__lwp_thread_loadenv(thethread);
		__lwp_thread_ready(thethread);
		__libc_start_hook(_thr_executing,thethread);
		return 1;
	}
	return 0;
}

void __lwp_thread_startmultitasking()
{
	_lwp_exitfunc = NULL;

	__sys_state_set(SYS_STATE_BEGIN_MT);
	__sys_state_set(SYS_STATE_UP);

	_context_switch_want = FALSE;
	_thr_executing = _thr_heir;
#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_start_multitasking(%p,%p)\n",_thr_executing,_thr_heir);
#endif
	__lwp_thread_starttimeslice();
	_cpu_context_switch((void*)&core_context,(void*)&_thr_heir->context);

	if(_lwp_exitfunc) _lwp_exitfunc();
}

void __lwp_thread_stopmultitasking(void (*exitfunc)())
{
	_lwp_exitfunc = exitfunc;
	if(__sys_state_get()!=SYS_STATE_SHUTDOWN) {
		__lwp_thread_stoptimeslice();
		__sys_state_set(SYS_STATE_SHUTDOWN);
		_cpu_context_switch((void*)&_thr_executing->context,(void*)&core_context);
	}
}

void __lwp_thread_coreinit()
{
	u32 index;

#ifdef _LWPTHREADS_DEBUG
	kprintf("__lwp_sys_init()\n\n");
#endif
	__lwp_thread_dispatchinitialize();
	__lwp_thread_inittimeslice();
	
	_context_switch_want = FALSE;
	_thr_executing = NULL;
	_thr_heir = NULL;
	_lwp_ticks_per_timeslice = 10;

	memset(&core_context,0,sizeof(core_context));

	for(index=0;index<=LWP_PRIO_MAX;index++)
		__lwp_queue_init_empty(&_lwp_thr_ready[index]);
	
	__sys_state_set(SYS_STATE_BEFORE_MT);
}
