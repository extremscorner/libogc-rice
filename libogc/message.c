/*-------------------------------------------------------------

message.c -- Thread subsystem II

Copyright (C) 2004
Michael Wiedenbauer (shagkur)
Dave Murphy (WinterMute)

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


#include <asm.h>
#include <stdlib.h>
#include <lwp_messages.h>
#include <lwp_objmgr.h>
#include <lwp_config.h>
#include "message.h"

#define LWP_OBJTYPE_MBOX			6

#define LWP_CHECK_MBOX(hndl)		\
{									\
	if(((hndl)==MQ_BOX_NULL) || (LWP_OBJTYPE(hndl)!=LWP_OBJTYPE_MBOX))	\
		return NULL;				\
}

typedef struct _mqbox_st
{
	lwp_obj object;
	mq_cntrl mqueue;
} mqbox_st;

lwp_objinfo _lwp_mqbox_objects;

void __lwp_mqbox_init()
{
	__lwp_objmgr_initinfo(&_lwp_mqbox_objects,LWP_MAX_MQUEUES,sizeof(mqbox_st));
}

static __inline__ mqbox_st* __lwp_mqbox_open(mqbox_t mbox)
{
	LWP_CHECK_MBOX(mbox);
	return (mqbox_st*)__lwp_objmgr_get(&_lwp_mqbox_objects,LWP_OBJMASKID(mbox));
}

static __inline__ void __lwp_mqbox_free(mqbox_st *mqbox)
{
	__lwp_objmgr_close(&_lwp_mqbox_objects,&mqbox->object);
	__lwp_objmgr_free(&_lwp_mqbox_objects,&mqbox->object);
}

static mqbox_st* __lwp_mqbox_allocate()
{
	mqbox_st *mqbox;

	__lwp_thread_dispatchdisable();
	mqbox = (mqbox_st*)__lwp_objmgr_allocate(&_lwp_mqbox_objects);
	if(mqbox) {
		__lwp_objmgr_open(&_lwp_mqbox_objects,&mqbox->object);
		return mqbox;
	}
	__lwp_thread_dispatchenable();
	return NULL;
}

static BOOL __lwp_mqbox_sendsupp(mqbox_t mqbox,mqmsg_t msg,u32 type,u32 wait,u64 timeout)
{
	u32 status;
	mqbox_st *mbox;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	status = __lwpmq_submit(&mbox->mqueue,mbox->object.id,(void*)&msg,sizeof(mqmsg_t),type,wait,timeout);
	__lwp_thread_dispatchenable();

	if(status==LWP_MQ_STATUS_UNSATISFIED_WAIT)
		status = _thr_executing->wait.ret_code;
	return (status==LWP_MQ_STATUS_SUCCESSFUL);
}

static BOOL __lwp_mqbox_receivesupp(mqbox_t mqbox,mqmsg_t *msg,u32 wait,u64 timeout)
{
	u32 status,tmp;
	mqbox_st *mbox;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return FALSE;

	__lwpmq_seize(&mbox->mqueue,mbox->object.id,(void*)msg,&tmp,wait,timeout);
	__lwp_thread_dispatchenable();

	status = _thr_executing->wait.ret_code;
	return (status==LWP_MQ_STATUS_SUCCESSFUL);
}

s32 MQ_Init(mqbox_t *mqbox,u32 count)
{
	mq_attr attr;
	mqbox_st *ret = NULL;

	if(!mqbox) return -1;

	ret = __lwp_mqbox_allocate();
	if(!ret) return MQ_ERROR_TOOMANY;

	attr.mode = LWP_MQ_FIFO;
	if(!__lwpmq_initialize(&ret->mqueue,&attr,count,sizeof(mqmsg_t))) {
		__lwp_mqbox_free(ret);
		__lwp_thread_dispatchenable();
		return MQ_ERROR_TOOMANY;
	}

	*mqbox = (mqbox_t)(LWP_OBJMASKTYPE(LWP_OBJTYPE_MBOX)|LWP_OBJMASKID(ret->object.id));
	__lwp_thread_dispatchenable();
	return MQ_ERROR_SUCCESSFUL;
}

void MQ_Close(mqbox_t mqbox)
{
	mqbox_st *mbox;

	mbox = __lwp_mqbox_open(mqbox);
	if(!mbox) return;

	__lwpmq_close(&mbox->mqueue,0);
	__lwp_thread_dispatchenable();

	__lwp_mqbox_free(mbox);
}

BOOL MQ_Send(mqbox_t mqbox,mqmsg_t msg,u32 flags)
{
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	return __lwp_mqbox_sendsupp(mqbox,msg,LWP_MQ_SEND_REQUEST,wait,LWP_THREADQ_NOTIMEOUT);
}

BOOL MQ_TimedSend(mqbox_t mqbox,mqmsg_t msg,const struct timespec *abstime)
{
	u64 timeout = LWP_THREADQ_NOTIMEOUT;

	if(abstime) timeout = __lwp_wd_calc_ticks(abstime);
	return __lwp_mqbox_sendsupp(mqbox,msg,LWP_MQ_SEND_REQUEST,TRUE,timeout);
}

BOOL MQ_Jam(mqbox_t mqbox,mqmsg_t msg,u32 flags)
{
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	return __lwp_mqbox_sendsupp(mqbox,msg,LWP_MQ_SEND_URGENT,wait,LWP_THREADQ_NOTIMEOUT);
}

BOOL MQ_TimedJam(mqbox_t mqbox,mqmsg_t msg,const struct timespec *abstime)
{
	u64 timeout = LWP_THREADQ_NOTIMEOUT;

	if(abstime) timeout = __lwp_wd_calc_ticks(abstime);
	return __lwp_mqbox_sendsupp(mqbox,msg,LWP_MQ_SEND_URGENT,TRUE,timeout);
}

BOOL MQ_Receive(mqbox_t mqbox,mqmsg_t *msg,u32 flags)
{
	u32 wait = (flags==MQ_MSG_BLOCK)?TRUE:FALSE;

	return __lwp_mqbox_receivesupp(mqbox,msg,wait,LWP_THREADQ_NOTIMEOUT);
}

BOOL MQ_TimedReceive(mqbox_t mqbox,mqmsg_t *msg,const struct timespec *abstime)
{
	u64 timeout = LWP_THREADQ_NOTIMEOUT;

	if(abstime) timeout = __lwp_wd_calc_ticks(abstime);
	return __lwp_mqbox_receivesupp(mqbox,msg,TRUE,timeout);
}
