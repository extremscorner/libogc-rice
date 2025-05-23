#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ogcsys.h>

#include "asm.h"
#include "processor.h"
#include "exi.h"
#include "lwp.h"
#include "system.h"
#include "semaphore.h"
#include "card_cmn.h"
//#include "card_fat.h"
#include "card_io.h"
#include "timesupp.h"

//#define _CARDIO_DEBUG
#ifdef _CARDIO_DEBUG
#include <stdio.h>
#endif
// SDHC support
// added by emu_kidid
#define TYPE_SD								0
#define TYPE_SDHC							1

#define MMC_ERROR_PARAM						0x0040
#define MMC_ERROR_ADDRESS					0x0020
#define MMC_ERROR_ERASE_SEQ					0x0010
#define MMC_ERROR_CRC						0x0008
#define MMC_ERROR_ILL						0x0004
#define MMC_ERROR_ERASE_RES					0x0002
#define MMC_ERROR_IDLE						0x0001

#define CARDIO_OP_INITFAILED				0x8000
#define CARDIO_OP_TIMEDOUT					0x4000
#define CARDIO_OP_IOERR_IDLE				0x2000
#define CARDIO_OP_IOERR_PARAM				0x1000
#define CARDIO_OP_IOERR_WRITE				0x0200
#define CARDIO_OP_IOERR_ADDR				0x0100
#define CARDIO_OP_IOERR_CRC					0x0002
#define CARDIO_OP_IOERR_ILL					0x0001
#define CARDIO_OP_IOERR_FATAL				(CARDIO_OP_IOERR_PARAM|CARDIO_OP_IOERR_WRITE|CARDIO_OP_IOERR_ADDR|CARDIO_OP_IOERR_CRC|CARDIO_OP_IOERR_ILL)

#define _SHIFTL(v, s, w)	\
    ((u32) (((u32)(v) & ((0x01 << (w)) - 1)) << (s)))
#define _SHIFTR(v, s, w)	\
    ((u32)(((u32)(v) >> (s)) & ((0x01 << (w)) - 1)))

typedef s32 (*cardiocallback)(s32 drv_no);

u8 g_CID[MAX_DRIVE][16];
u8 g_CSD[MAX_DRIVE][16];
u8 g_CardStatus[MAX_DRIVE][64];

u8 g_mCode[MAX_MI_NUM] = { 0x03 };

u16 g_dCode[MAX_MI_NUM][MAX_DI_NUM] = 
{
	{
		0x033f,			/* SD   8Mb */
		0x0383,			/* SD  16Mb */
		0x074b,			/* SD  32Mb */
		0x0edf,			/* SD  64Mb */
		0x0f03			/* SD 128Mb */
	}
};

static u32 _ioCardSelect[MAX_DRIVE];
static u32 _ioCardFreq[MAX_DRIVE];
static u32 _ioDefaultSpeed[MAX_DRIVE];
static u32 _ioHighSpeed[MAX_DRIVE];
static u32 _ioRetryCnt[MAX_DRIVE];
static cardiocallback _ioRetryCB = NULL;

static lwpq_t _ioEXILock[MAX_DRIVE];

static u32 _ioPageSize[MAX_DRIVE];
static u32 _ioReadSector[MAX_DRIVE];
static u32 _ioFlag[MAX_DRIVE];
static u32 _ioError[MAX_DRIVE];
static bool _ioCardInserted[MAX_DRIVE];

static u8 _ioResponse[MAX_DRIVE][128];
static u8 _ioCrc7Table[256];
static u16 _ioCrc16Table[2][256];

// SDHC support
static u32 _initType[MAX_DRIVE];
static u32 _ioAddressingType[MAX_DRIVE];
static u32 _ioTransferMode[MAX_DRIVE];

static __inline__ u32 __check_response(s32 drv_no,u8 res)
{
	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	_ioError[drv_no] = 0;
	if(_ioFlag[drv_no]==INITIALIZING && res&MMC_ERROR_IDLE) {
		_ioError[drv_no] |= CARDIO_OP_IOERR_IDLE;
		return CARDIO_ERROR_READY;
	} else {
		if(res&MMC_ERROR_PARAM) _ioError[drv_no] |= CARDIO_OP_IOERR_PARAM;
		if(res&MMC_ERROR_ADDRESS) _ioError[drv_no] |= CARDIO_OP_IOERR_ADDR;
		if(res&MMC_ERROR_CRC) _ioError[drv_no] |= CARDIO_OP_IOERR_CRC;
		if(res&MMC_ERROR_ILL) _ioError[drv_no] |= CARDIO_OP_IOERR_ILL;
	}
	return ((_ioError[drv_no]&CARDIO_OP_IOERR_FATAL)?CARDIO_ERROR_FATALERROR:CARDIO_ERROR_READY);
}

static void __init_crc7()
{
	s32 i,j;
	u8 crc7;

	for(i=0;i<256;i++) {
		crc7 = i;
		for(j=0;j<8;j++) {
			if(crc7&0x80) crc7 = (crc7<<1)^0x12;
			else crc7 <<= 1;
		}
		_ioCrc7Table[i] = crc7;
	}
}

static u8 __make_crc7(void *buffer,u32 len)
{
	s32 i;
	u8 crc7;
	u8 *ptr;

	crc7 = 0;
	ptr = buffer;
	for(i=0;i<len;i++) {
		crc7 ^= ptr[i];
		crc7 = _ioCrc7Table[crc7];
	}
	return crc7;
}

/* Old way, realtime
static u8 __make_crc7(void *buffer,u32 len)
{
	u32 mask,cnt,bcnt;
	u32 res,val;
	u8 *ptr = (u8*)buffer;

	cnt = 0;
	res = 0;
	while(cnt<len) {
		bcnt = 0;
		mask = 0x80;
		while(bcnt<8) {
			res <<= 1;
			val = *ptr^((res>>bcnt)&0xff);
			if(mask&val) {
				res |= 0x01;
				if(!(res&0x0008)) res |= 0x0008;
				else res &= ~0x0008;
				
			} else if(res&0x0008) res |= 0x0008;
			else res &= ~0x0008;
			
			mask >>= 1;
			bcnt++;
		}
		ptr++;
		cnt++;
	}
	return (res<<1)&0xff;
}
*/
static void __init_crc16()
{
	s32 i,j;
	u16 crc16;

	for(i=0;i<256;i++) {
		crc16 = ((u16)i)<<8;
		for(j=0;j<8;j++) {
			if(crc16&0x8000) crc16 = (crc16<<1)^0x1021;
			else crc16 <<= 1;
		}
		_ioCrc16Table[0][i] = crc16;
	}

	for(i=0;i<256;i++) {
		crc16 = _ioCrc16Table[0][i];
		crc16 = _ioCrc16Table[0][crc16>>8]^(crc16<<8);
		_ioCrc16Table[1][i] = crc16;
	}
}

static u16 __make_crc16(void *buffer,u32 len)
{
	s32 i;
	u16 crc16;
	u16 *ptr;

	crc16 = 0;
	len /= 2;
	ptr = buffer;
	for(i=0;i<len;i++) {
		crc16 ^= ptr[i];
		crc16 = _ioCrc16Table[1][crc16>>8]^_ioCrc16Table[0][crc16&0xff];
	}
	return crc16;
}

/* Old way, realtime
static u16 __make_crc16(void *buffer,u32 len)
{
	u32 mask,cnt,bcnt;
	u32 res,val,tmp;
	u8 *ptr = (u8*)buffer;

	cnt = 0;
	res = 0;
	while(cnt<len) {
		bcnt = 0;
		mask = 0x80;
		val = *ptr;
		while(bcnt<8) {
			tmp = val^((res>>(bcnt+8))&0xff);
			if(mask&tmp) {
				res = (res<<1)|0x0001;
				if(!(res&0x0020)) res |= 0x0020;
				else res &= ~0x0020;
				if(!(res&0x1000)) res |= 0x1000;
				else res &= ~0x1000;
			} else {
				res = (res<<1)&~0x0001;
				if(res&0x0020) res |= 0x0020;
				else res &= ~0x0020;
				if(res&0x1000) res |= 0x1000;
				else res &= ~0x1000;
			}
			mask >>= 1;
			bcnt++;
		}
		ptr++;
		cnt++;
	}
	
	return (res&0xffff);
}
*/

static u32 __card_checktimeout(s32 drv_no,u32 startT,u32 timeout)
{
	u32 endT,diff;
	u32 msec;

	endT = gettick();
	if(endT<startT) {
		diff = (endT+(-1-startT))+1;
	} else
		diff = (endT-startT);

	msec = (diff/TB_TIMER_CLOCK);
	if(msec<=timeout) return 0;

	_ioError[drv_no] |= CARDIO_OP_TIMEDOUT;
	return 1;
}

static s32 __exi_unlock(s32 chn,s32 dev)
{
	LWP_ThreadBroadcast(_ioEXILock[chn]);
	return 1;
}

static void __exi_wait(s32 drv_no)
{
	u32 ret;

	do {
		if((ret=EXI_Lock(drv_no,_ioCardSelect[drv_no],__exi_unlock))==1) break;
		LWP_ThreadSleep(_ioEXILock[drv_no]);
	} while(ret==0);
}

static s32 __card_exthandler(s32 chn,s32 dev)
{
	_ioFlag[chn] = NOT_INITIALIZED;
	_ioCardInserted[chn] = FALSE;
	sdgecko_ejectedCB(chn);
	return 1;
}

static s32 __card_writecmd0(s32 drv_no)
{
	u8 crc;
	u32 cnt;
	u8 dummy[128];
	u8 cmd[5] = {0,0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	cmd[0] = 0x40;
	crc = __make_crc7(cmd,5);

	__exi_wait(drv_no);

	if(EXI_SelectSD(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no])==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	cnt = 0;
	while(cnt<20) {
		if(EXI_ImmEx(drv_no,dummy,128,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
		cnt++;
	}
	EXI_Deselect(drv_no);
	
	if(EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no])==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	crc |= 0x01;
#ifdef _CARDIO_DEBUG
	printf("sd command0: %02x %02x %02x %02x %02x %02x\n",cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],crc);
#endif
	if(EXI_ImmEx(drv_no,cmd,5,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	if(EXI_ImmEx(drv_no,&crc,1,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);
	return CARDIO_ERROR_READY;
}

static s32 __card_writecmd(s32 drv_no,void *buf,s32 len)
{
	u8 crc,*ptr;
	u8 dummy[32];

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	ptr = buf;
	ptr[0] |= 0x40;
	crc = __make_crc7(buf,len);

	__exi_wait(drv_no);

	if(EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no])==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	if(EXI_ImmEx(drv_no,dummy,10,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	crc |= 0x01;
#ifdef _CARDIO_DEBUG
	printf("sd command: %02x %02x %02x %02x %02x %02x\n",((u8*)buf)[0],((u8*)buf)[1],((u8*)buf)[2],((u8*)buf)[3],((u8*)buf)[4],crc);
#endif
	if(EXI_ImmEx(drv_no,buf,len,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	if(EXI_ImmEx(drv_no,&crc,1,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);
	return CARDIO_ERROR_READY;
}

static s32 __card_readresponse(s32 drv_no,void *buf,s32 len)
{
	u8 *ptr;
	u32 cnt;
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	__exi_wait(drv_no);

	if((_ioTransferMode[drv_no]==CARDIO_TRANSFER_DMA?
		EXI_SelectSD(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]):
		EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]))==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	ret = CARDIO_ERROR_READY;
	ptr = buf;
	for(cnt=0;cnt<16;cnt++) {
		if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
#ifdef _CARDIO_DEBUG
		printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
		if(!(*ptr&0x80)) break;
	}
	if(cnt>=16) ret = CARDIO_ERROR_IOTIMEOUT;
	if(len>1 && ret==CARDIO_ERROR_READY) {
		if(EXI_ImmEx(drv_no,ptr+1,len-1,EXI_READ)==0) ret = CARDIO_ERROR_IOERROR;
	}
	
	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);
	return ret;
}

static s32 __card_stopreadresponse(s32 drv_no,void *buf,s32 len)
{
	u8 *ptr,tmp;
	s32 startT,ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	__exi_wait(drv_no);

	if((_ioTransferMode[drv_no]==CARDIO_TRANSFER_DMA?
		EXI_SelectSD(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]):
		EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]))==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	ret = CARDIO_ERROR_READY;
	ptr = buf;
	if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
#ifdef _CARDIO_DEBUG
	kprintf("sd response(%d): %02x\n",__LINE__,((u8*)buf)[0]);
#endif

	if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
#ifdef _CARDIO_DEBUG
	kprintf("sd response1: %02x\n",((u8*)buf)[0]);
#endif

	startT = gettick();
	while(*ptr&0x80) {
		if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
#ifdef _CARDIO_DEBUG
		kprintf("sd response2: %02x\n",((u8*)buf)[0]);
#endif
		if(!(*ptr&0x80)) break;
		if(__card_checktimeout(drv_no,startT,1500)!=0) {
			if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
				EXI_Deselect(drv_no);
				EXI_Unlock(drv_no);
				return CARDIO_ERROR_IOERROR;
			}
#ifdef _CARDIO_DEBUG
			kprintf("sd response3: %02x\n",((u8*)buf)[0]);
#endif
			if(*ptr&0x80) ret = CARDIO_ERROR_IOTIMEOUT;
			break;
		}
	}

	tmp = *ptr;
	while(*ptr!=0xff) {
		if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
#ifdef _CARDIO_DEBUG
		kprintf("sd response4: %02x\n",((u8*)buf)[0]);
#endif
		if(*ptr==0xff) break;
		if(__card_checktimeout(drv_no,startT,1500)!=0) {
			if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
				EXI_Deselect(drv_no);
				EXI_Unlock(drv_no);
				return CARDIO_ERROR_IOERROR;
			}
#ifdef _CARDIO_DEBUG
			kprintf("sd response5: %02x\n",((u8*)buf)[0]);
#endif
			if(*ptr!=0xff) ret = CARDIO_ERROR_IOTIMEOUT;
			break;
		}
	}
	*ptr = tmp;

	if(len>1 && ret==CARDIO_ERROR_READY) {
		if(EXI_ImmEx(drv_no,ptr+1,len-1,EXI_READ)==0) ret = CARDIO_ERROR_IOERROR;
	}
	
	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);
	return ret;
}

static s32 __card_datares(s32 drv_no,void *buf)
{
	u8 *ptr;
	s32 startT,ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	__exi_wait(drv_no);

	if((_ioTransferMode[drv_no]==CARDIO_TRANSFER_DMA?
		EXI_SelectSD(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]):
		EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]))==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	ret = CARDIO_ERROR_READY;
	ptr = buf;
	if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
#ifdef _CARDIO_DEBUG
	printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
	startT = gettick();
	while(*ptr&0x10) {
		if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
#ifdef _CARDIO_DEBUG
		printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
		if(!(*ptr&0x10)) break;
		if(__card_checktimeout(drv_no,startT,1500)!=0) {
			if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
				EXI_Deselect(drv_no);
				EXI_Unlock(drv_no);
				return CARDIO_ERROR_IOERROR;
			}
#ifdef _CARDIO_DEBUG
			printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
			if(*ptr&0x10) ret = CARDIO_ERROR_IOTIMEOUT;
			break;
		}
	}

	ptr++;
	if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	startT = gettick();
	while(!*ptr) {
		if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
#ifdef _CARDIO_DEBUG
		printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
		if(*ptr) break;
		if(__card_checktimeout(drv_no,startT,1500)!=0) {
			if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
				EXI_Deselect(drv_no);
				EXI_Unlock(drv_no);
				return CARDIO_ERROR_IOERROR;
			}
#ifdef _CARDIO_DEBUG
			printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
			if(!*ptr) ret = CARDIO_ERROR_IOTIMEOUT;
			break;
		}
	}
	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);

	return ret;
}

static s32 __card_stopresponse(s32 drv_no)
{
	s32 ret;

	if((ret=__card_stopreadresponse(drv_no,_ioResponse[drv_no],1))!=0) return ret;
	return __check_response(drv_no,_ioResponse[drv_no][0]);
}

static s32 __card_dataresponse(s32 drv_no)
{
	s32 ret;
	u8 res;

	if((ret=__card_datares(drv_no,_ioResponse[drv_no]))!=0) return ret;
	res = _SHIFTR(_ioResponse[drv_no][0],1,3);
	if(res==0x0005) ret = CARDIO_OP_IOERR_CRC;
	else if(res==0x0006) ret = CARDIO_OP_IOERR_WRITE;

	return ret;
}

static s32 __card_dataread(s32 drv_no,void *buf,u32 len)
{
	u8 *ptr;
	u16 crc,crc_org;
	s32 startT,ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	__exi_wait(drv_no);
	
	if((_ioTransferMode[drv_no]==CARDIO_TRANSFER_DMA?
		EXI_SelectSD(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]):
		EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no]))==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	ret = CARDIO_ERROR_READY;
	ptr = buf;
	if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
#ifdef _CARDIO_DEBUG
	printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
	
	startT = gettick();
	while(*ptr!=0xfe) {
		if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
#ifdef _CARDIO_DEBUG
		printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
		if(*ptr==0xfe) break;
		if(__card_checktimeout(drv_no,startT,1500)!=0) {
			if(EXI_ImmEx(drv_no,ptr,1,EXI_READ)==0) {
				EXI_Deselect(drv_no);
				EXI_Unlock(drv_no);
				return CARDIO_ERROR_IOERROR;
			}
#ifdef _CARDIO_DEBUG
			printf("sd response: %02x\n",((u8*)buf)[0]);
#endif
			if(*ptr!=0xfe) ret = CARDIO_ERROR_IOTIMEOUT;
			break;
		}
	}

	if(_ioTransferMode[drv_no]==CARDIO_TRANSFER_DMA) {
		if(EXI_DmaEx(drv_no,ptr,len,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
	} else {
		LWP_YieldThread();
		if(EXI_ImmEx(drv_no,ptr,len,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
	}

	if(EXI_ImmEx(drv_no,&crc_org,2,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
#ifdef _CARDIO_DEBUG
	printf("sd response: %04x\n",crc_org);
#endif

	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);

	crc = __make_crc16(buf,len);
	if(crc!=crc_org) ret = CARDIO_OP_IOERR_CRC;
#ifdef _CARDIO_DEBUG
	printf("crc ok: %04x : %04x\n",crc_org,crc);
#endif
	return ret;
}

static s32 __card_datawrite(s32 drv_no,void *buf,u32 len)
{
	u8 dummy[32];
	u16 crc;
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	crc = __make_crc16(buf,len);

	__exi_wait(drv_no);

	if(EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no])==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	dummy[0] = 0xfe;
	if(EXI_ImmEx(drv_no,dummy,1,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	if(EXI_DmaEx(drv_no,buf,len,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	ret = CARDIO_ERROR_READY;
	if(EXI_ImmEx(drv_no,&crc,2,EXI_WRITE)==0) ret = CARDIO_ERROR_IOERROR;

	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);

	return ret;
}

static s32 __card_multidatawrite(s32 drv_no,void *buf,u32 len)
{
	u8 dummy[32];
	u16 crc;
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	crc = __make_crc16(buf,len);
	
	__exi_wait(drv_no);

	if(EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no])==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}

	dummy[0] = 0xfc;
	if(EXI_ImmEx(drv_no,dummy,1,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	if(EXI_DmaEx(drv_no,buf,len,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	ret = CARDIO_ERROR_READY;
	if(EXI_ImmEx(drv_no,&crc,2,EXI_WRITE)==0) ret = CARDIO_ERROR_IOERROR;

	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);

	return ret;
}

static s32 __card_multiwritestop(s32 drv_no)
{
	s32 startT,ret;
	u8 dummy[32];

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	__exi_wait(drv_no);
	
	if(EXI_Select(drv_no,_ioCardSelect[drv_no],_ioCardFreq[drv_no])==0) {
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_NOCARD;
	}
	
	ret = CARDIO_ERROR_READY;
	dummy[0] = 0xfd;
	if(EXI_ImmEx(drv_no,dummy,1,EXI_WRITE)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}

	if(EXI_ImmEx(drv_no,dummy,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	if(EXI_ImmEx(drv_no,dummy,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	if(EXI_ImmEx(drv_no,dummy,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	if(EXI_ImmEx(drv_no,dummy,1,EXI_READ)==0) {
		EXI_Deselect(drv_no);
		EXI_Unlock(drv_no);
		return CARDIO_ERROR_IOERROR;
	}
	
	startT = gettick();
	ret = CARDIO_ERROR_READY;
	while(dummy[0]==0) {
		if(EXI_ImmEx(drv_no,dummy,1,EXI_READ)==0) {
			EXI_Deselect(drv_no);
			EXI_Unlock(drv_no);
			return CARDIO_ERROR_IOERROR;
		}
		if(dummy[0]) break;
		if(__card_checktimeout(drv_no,startT,1500)!=0) {
			if(EXI_ImmEx(drv_no,dummy,1,EXI_READ)==0) {
				EXI_Deselect(drv_no);
				EXI_Unlock(drv_no);
				return CARDIO_ERROR_IOERROR;
			}
			if(!dummy[0]) ret = CARDIO_ERROR_IOTIMEOUT;
			break;
		}
	}

	EXI_Deselect(drv_no);
	EXI_Unlock(drv_no);
	return ret;
}

static s32 __card_response1(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
	
	if((ret=__card_readresponse(drv_no,_ioResponse[drv_no],1))!=0) return ret;
	return __check_response(drv_no,_ioResponse[drv_no][0]);
}

static s32 __card_response2(s32 drv_no)
{
	u32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
	
	if((ret=__card_readresponse(drv_no,_ioResponse[drv_no],2))!=0) return ret;
	if((_ioResponse[drv_no][0]&0x7c) || (_ioResponse[drv_no][1]&0x9e)) return CARDIO_ERROR_FATALERROR;
	return CARDIO_ERROR_READY;
}

static s32 __card_response3(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
	
	if((ret=__card_readresponse(drv_no,_ioResponse[drv_no],5))!=0) return ret;
	return __check_response(drv_no,_ioResponse[drv_no][0]);
}

static s32 __card_sendcmd(s32 drv_no,u8 cmd,u8 *arg)
{
	s32 ret;
	u8 ccmd[5] = {0,0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	if(cmd==0x0C) _ioReadSector[drv_no] = ~0;
	else if(_ioReadSector[drv_no]!=~0) {
		if((ret=__card_sendcmd(drv_no,0x0C,NULL))!=0) return ret;
		if((ret=__card_stopresponse(drv_no))!=0) return ret;
	}

	ccmd[0] = cmd;
	if(arg) {
		ccmd[1] = arg[0];
		ccmd[2] = arg[1];
		ccmd[3] = arg[2];
		ccmd[4] = arg[3];
	}
	return __card_writecmd(drv_no,ccmd,5);
}

static s32 __card_sendappcmd(s32 drv_no,u8 cmd,u8 *arg)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	if((ret=__card_sendcmd(drv_no,0x37,NULL))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_sendappcmd(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	if((ret=__card_response1(drv_no))!=0) return ret;
	return __card_sendcmd(drv_no,cmd,arg);
}

static s32 __card_sendopcond(s32 drv_no)
{
	s32 startT,ret;
	u8 arg[4] = {0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("__card_sendopcond(%d)\n",drv_no);
#endif
	if(_initType[drv_no]==TYPE_SDHC) arg[0] = 0x40;

	ret = 0;
	startT = gettick();
	do {
		if((ret=__card_sendappcmd(drv_no,0x29,arg))!=0) {
#ifdef _CARDIO_DEBUG
			printf("__card_sendopcond(%d): sd write cmd failed.\n",ret);
#endif
			return ret;
		}
		if((ret=__card_response1(drv_no))!=0) return ret;
		if(!(_ioError[drv_no]&CARDIO_OP_IOERR_IDLE)) return CARDIO_ERROR_READY;

		ret = __card_checktimeout(drv_no,startT,1500);
	} while(ret==0);

	if((ret=__card_sendappcmd(drv_no,0x29,arg))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_sendopcond(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	if((ret=__card_response1(drv_no))!=0) return ret;
	if(_ioError[drv_no]&CARDIO_OP_IOERR_IDLE) return CARDIO_ERROR_IOTIMEOUT;

	return CARDIO_ERROR_READY;
}

static s32 __card_sendCMD6(s32 drv_no,u32 switch_func)
{
	s32 ret;
	u8 arg[4] = {0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	arg[0] = (switch_func>>24)&0xff;
	arg[1] = (switch_func>>16)&0xff;
	arg[2] = (switch_func>>8)&0xff;
	arg[3] = switch_func&0xff;
	if((ret=__card_sendcmd(drv_no,0x06,arg))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_sendCMD6(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	if((ret=__card_response1(drv_no))!=0) return ret;
	return __card_dataread(drv_no,_ioResponse[drv_no],64);
}

static s32 __card_sendCMD8(s32 drv_no)
{
	s32 ret;
	u8 arg[4] = {0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	arg[2] = 0x01;
	arg[3] = 0xAA;
	if((ret=__card_sendcmd(drv_no,0x08,arg))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_sendCMD8(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	return __card_response3(drv_no);
}

static s32 __card_sendCMD58(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	if((ret=__card_sendcmd(drv_no,0x3A,NULL))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_sendCMD58(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	return __card_response3(drv_no);
}

static s32 __card_sendCMD59(s32 drv_no,u32 crc_on_off)
{
	s32 ret;
	u8 arg[4] = {0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	arg[0] = (crc_on_off>>24)&0xff;
	arg[1] = (crc_on_off>>16)&0xff;
	arg[2] = (crc_on_off>>8)&0xff;
	arg[3] = crc_on_off&0xff;
	if((ret=__card_sendcmd(drv_no,0x3B,arg))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_sendCMD59(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	return __card_response1(drv_no);
}

static s32 __card_setblocklen(s32 drv_no,u32 block_len)
{
	s32 ret;
	u8 arg[4] = {0,0,0,0};

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("__card_setblocklen(%d,%d)\n",drv_no,block_len);
#endif
	if(block_len>PAGE_SIZE512) block_len = PAGE_SIZE512;

	arg[0] = (block_len>>24)&0xff;
	arg[1] = (block_len>>16)&0xff;
	arg[2] = (block_len>>8)&0xff;
	arg[3] = block_len&0xff;
	if((ret=__card_sendcmd(drv_no,0x10,arg))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_setblocklen(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	return __card_response1(drv_no);
}

static s32 __card_readcsd(s32 drv_no)
{
	s32 ret;
	u8 crc;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("__card_readcsd(%d)\n",drv_no);
#endif
	if((ret=__card_sendcmd(drv_no,0x09,NULL))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_readcsd(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	if((ret=__card_response1(drv_no))!=0) return ret;
	if((ret=__card_dataread(drv_no,g_CSD[drv_no],16))==CARDIO_OP_IOERR_CRC) {
		crc = __make_crc7(g_CSD[drv_no],15)|0x01;
		ret = (crc!=g_CSD[drv_no][15])?CARDIO_ERROR_FATALERROR:CARDIO_ERROR_READY;
	}
	return ret;
}

static s32 __card_readcid(s32 drv_no)
{
	s32 ret;
	u8 crc;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("__card_readcid(%d)\n",drv_no);
#endif
	if((ret=__card_sendcmd(drv_no,0x0A,NULL))!=0) {
#ifdef _CARDIO_DEBUG
		printf("__card_readcid(%d): sd write cmd failed.\n",ret);
#endif
		return ret;
	}
	if((ret=__card_response1(drv_no))!=0) return ret;
	if((ret=__card_dataread(drv_no,g_CID[drv_no],16))==CARDIO_OP_IOERR_CRC) {
		crc = __make_crc7(g_CID[drv_no],15)|0x01;
		ret = (crc!=g_CID[drv_no][15])?CARDIO_ERROR_FATALERROR:CARDIO_ERROR_READY;
	}
	return ret;
}

static s32 __card_sd_status(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("__card_sd_status(%d)\n",drv_no);
#endif
	if((ret=__card_sendappcmd(drv_no,0x0D,NULL))!=0) return ret;
	if((ret=__card_response2(drv_no))!=0) return ret;
	return __card_dataread(drv_no,g_CardStatus[drv_no],64);
}

static s32 __card_softreset(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("__card_softreset(%d)\n",drv_no);
#endif
	_ioCardFreq[drv_no] = _ioDefaultSpeed[drv_no];

	if(_ioFlag[drv_no]!=INITIALIZING) {
		_ioFlag[drv_no] = INITIALIZING;
		if((ret=__card_sendcmd(drv_no,0x00,NULL))!=0) {
#ifdef _CARDIO_DEBUG
			printf("__card_softreset(%d): sd write cmd failed.\n",ret);
#endif
			return ret;
		}
	} else {
		if((ret=__card_writecmd0(drv_no))!=0) {
#ifdef _CARDIO_DEBUG
			printf("__card_softreset(%d): sd write cmd0 failed.\n",ret);
#endif
			return ret;
		}
	}
	if((ret=__card_response1(drv_no))!=0) {
		if(_ioTransferMode[drv_no]!=CARDIO_TRANSFER_IMM && ret==CARDIO_ERROR_IOTIMEOUT) {
			_ioTransferMode[drv_no] = CARDIO_TRANSFER_IMM;
			if((ret=__card_response1(drv_no))!=0) {
				return __card_softreset(drv_no);
			}
		} else return ret;
	}
	if(!(_ioError[drv_no]&CARDIO_OP_IOERR_IDLE)) return CARDIO_ERROR_IOERROR;

	return __card_sendCMD59(drv_no,TRUE);
}

static bool __card_check(s32 drv_no)
{
	u32 id = 0;
	s32 ret;
	
	if(drv_no<0 || drv_no>=MAX_DRIVE) return FALSE;
#ifdef _CARDIO_DEBUG	
	printf("__card_check(%d)\n",drv_no);
#endif
	if(drv_no==0 && _ioCardSelect[drv_no]!=EXI_DEVICE_0) {
		if(EXI_GetID(drv_no,_ioCardSelect[drv_no],&id)==0) return FALSE;
		if(id!=0xffffffff) return FALSE;
		return TRUE;
	}
	while((ret=EXI_ProbeEx(drv_no))==0);
	if(ret!=1) return FALSE;

	if(EXI_GetID(drv_no,EXI_DEVICE_0,&id)==0) return FALSE;
	if(_ioCardSelect[drv_no]!=EXI_DEVICE_0) {
		if(id==0xffffffff) return FALSE;
		if(EXI_GetID(drv_no,_ioCardSelect[drv_no],&id)==0) return FALSE;
	}
	if(id!=0xffffffff) return FALSE;

	if(drv_no!=2 && _ioCardSelect[drv_no]==EXI_DEVICE_0) {
		if(!(EXI_GetState(drv_no)&EXI_FLAG_ATTACH)) {
			if(EXI_Attach(drv_no,__card_exthandler)==0) return FALSE;
#ifdef _CARDIO_DEBUG	
			printf("__card_check(%d, attached)\n",drv_no);
#endif
			sdgecko_insertedCB(drv_no);
		}
	}
	return TRUE;
}

static s32 __card_retrycb(s32 drv_no)
{
#ifdef _CARDIO_DEBUG	
	printf("__card_retrycb(%d)\n",drv_no);
#endif
	_ioRetryCB = NULL;
	_ioRetryCnt[drv_no]++;
	return sdgecko_initIO(drv_no);
}

static void __convert_sector(s32 drv_no,u32 sector_no,u8 *arg)
{
	if(_ioAddressingType[drv_no]==CARDIO_ADDRESSING_BYTE) {
		arg[0] = (sector_no>>15)&0xff;
		arg[1] = (sector_no>>7)&0xff;
		arg[2] = (sector_no<<1)&0xff;
		arg[3] = (sector_no<<9)&0xff;
	} else if(_ioAddressingType[drv_no]==CARDIO_ADDRESSING_BLOCK) {
		arg[0] = (sector_no>>24)&0xff;
		arg[1] = (sector_no>>16)&0xff;
		arg[2] = (sector_no>>8)&0xff;
		arg[3] = sector_no&0xff;
	}
}

void sdgecko_initIODefault()
{
	u32 i;
#ifdef _CARDIO_DEBUG	
	printf("card_initIODefault()\n");
#endif
	__init_crc7();
	__init_crc16();
	for(i=0;i<MAX_DRIVE;++i) {
		_ioRetryCnt[i] = 0;
		_ioError[i] = 0;
		_ioReadSector[i] = ~0;
		_ioCardInserted[i] = FALSE;
		_ioFlag[i] = NOT_INITIALIZED;
		_initType[i] = TYPE_SD;
		_ioAddressingType[i] = CARDIO_ADDRESSING_BYTE;
		_ioTransferMode[i] = CARDIO_TRANSFER_IMM;
		_ioCardSelect[i] = EXI_DEVICE_0;
		_ioCardFreq[i] = EXI_SPEED1MHZ;
		_ioDefaultSpeed[i] = EXI_SPEED32MHZ;
		_ioHighSpeed[i] = EXI_SPEEDMAX-1;
		LWP_InitQueue(&_ioEXILock[i]);
	}
}

s32 sdgecko_initIO(s32 drv_no)
{
	bool switch_func = FALSE;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	if((_ioRetryCnt[drv_no]%3)<2) switch_func = TRUE;
	if(_ioRetryCnt[drv_no]>=3) _ioDefaultSpeed[drv_no] = EXI_SPEED16MHZ;
	if(_ioRetryCnt[drv_no]>=6) {
		_ioRetryCnt[drv_no] = 0;
		_ioDefaultSpeed[drv_no] = EXI_SPEED32MHZ;
		return CARDIO_ERROR_IOERROR;
	}
	
	_ioCardInserted[drv_no] = __card_check(drv_no);

	if(_ioCardInserted[drv_no]==TRUE) {
		_ioFlag[drv_no] = INITIALIZING;
		_initType[drv_no] = TYPE_SD;
		_ioAddressingType[drv_no] = CARDIO_ADDRESSING_BYTE;
		_ioTransferMode[drv_no] = CARDIO_TRANSFER_IMM;

		if(drv_no!=0 && _ioCardSelect[drv_no]==EXI_DEVICE_0)
			_ioTransferMode[drv_no] = CARDIO_TRANSFER_DMA;
		if(__card_softreset(drv_no)!=0) goto exit;

		if(__card_sendCMD8(drv_no)!=0) goto exit;
#ifdef _CARDIO_DEBUG
		printf("Response %02X,%02X,%02X,%02X,%02X\n",_ioResponse[drv_no][0],_ioResponse[drv_no][1],_ioResponse[drv_no][2],_ioResponse[drv_no][3],_ioResponse[drv_no][4]);
#endif
		if(!(_ioResponse[drv_no][0]&MMC_ERROR_IDLE)) goto exit;
		else if(_ioResponse[drv_no][0]==MMC_ERROR_IDLE) {
			if((_ioResponse[drv_no][3]==1) && (_ioResponse[drv_no][4]==0xAA)) _initType[drv_no] = TYPE_SDHC;
			else goto exit;
		}

		if(__card_sendopcond(drv_no)!=0) goto exit;
		if(__card_sendCMD59(drv_no,TRUE)!=0) goto exit;

		if(_initType[drv_no]==TYPE_SDHC) {
			if(__card_sendCMD58(drv_no)!=0) goto exit;
#ifdef _CARDIO_DEBUG
			printf("Response %02X,%02X,%02X,%02X,%02X\n",_ioResponse[drv_no][0],_ioResponse[drv_no][1],_ioResponse[drv_no][2],_ioResponse[drv_no][3],_ioResponse[drv_no][4]);
#endif
			if(_ioResponse[drv_no][1]&0x40) _ioAddressingType[drv_no] = CARDIO_ADDRESSING_BLOCK;
		}

		__card_readcid(drv_no);
		if(__card_readcsd(drv_no)!=0) goto exit;

		_ioPageSize[drv_no] = PAGE_SIZE512;
		if(__card_setblocklen(drv_no,_ioPageSize[drv_no])!=0) goto exit;

		if(CCC(drv_no)&(1<<10) && _ioCardFreq[drv_no]<_ioHighSpeed[drv_no] && switch_func==TRUE) {
			if(__card_sendCMD6(drv_no,0x00fffff0)!=0) goto exit;
			if(((u16*)_ioResponse[drv_no])[6]&(1<<1)) {
				if(__card_sendCMD6(drv_no,0x80fffff1)!=0) goto exit;
				if((_ioResponse[drv_no][16]&0xf)==1) {
					_ioCardFreq[drv_no]++;
					if(__card_readcsd(drv_no)!=0) goto exit;
				}
			}
		}

		if(__card_sd_status(drv_no)!=0) goto exit;

		_ioRetryCnt[drv_no] = 0;
		_ioFlag[drv_no] = INITIALIZED;
		return CARDIO_ERROR_READY;
exit:
		_ioRetryCB = __card_retrycb;
		return sdgecko_doUnmount(drv_no);
	}
	return CARDIO_ERROR_NOCARD;
}

s32 sdgecko_preIO(s32 drv_no)
{
	s32 ret;

	if(_ioFlag[drv_no]==NOT_INITIALIZED) {
		ret = sdgecko_initIO(drv_no);
		if(ret!=CARDIO_ERROR_READY) {
#ifdef _CARDIO_DEBUG	
			printf("sdgecko_preIO(%d,ret = %d)\n",drv_no,ret);
#endif
			return ret;
		}
	}
	return CARDIO_ERROR_READY;
}

s32 sdgecko_enableCRC(s32 drv_no,bool enable)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("sdgecko_enableCRC(%d,%d)\n",drv_no,enable);
#endif
	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

	return __card_sendCMD59(drv_no,enable);
}

s32 sdgecko_readCID(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("sdgecko_readCID(%d)\n",drv_no);
#endif
	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;
	
	return __card_readcid(drv_no);
}

s32 sdgecko_readCSD(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("sdgecko_readCSD(%d)\n",drv_no);
#endif
	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

	return __card_readcsd(drv_no);
}

s32 sdgecko_readStatus(s32 drv_no)
{
	s32 ret;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;
#ifdef _CARDIO_DEBUG
	printf("sdgecko_readStatus(%d)\n",drv_no);
#endif
	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

	return __card_sd_status(drv_no);
}

s32 sdgecko_readSector(s32 drv_no,void *buf,u32 sector_no)
{
	s32 ret;
	u8 arg[4] = {0,0,0,0};
	char *ptr = (char*)buf;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

#ifdef _CARDIO_DEBUG
	printf("sdgecko_readSector(%d,%d,%d)\n",drv_no,sector_no,_ioPageSize[drv_no]);
#endif

	if(_ioPageSize[drv_no]!=PAGE_SIZE512) {
		_ioPageSize[drv_no] = PAGE_SIZE512;
		if((ret=__card_setblocklen(drv_no,_ioPageSize[drv_no]))!=0) return ret;
	}

	// SDHC support fix
	__convert_sector(drv_no,sector_no,arg);
	if((ret=__card_sendcmd(drv_no,0x11,arg))!=0) return ret;
	if((ret=__card_response1(drv_no))!=0) return ret;

	return __card_dataread(drv_no,ptr,_ioPageSize[drv_no]);
}

// Multiple sector read by emu_kidid
s32 sdgecko_readSectors(s32 drv_no,u32 sector_no,u32 num_sectors,void *buf)
{
	u32 i;
	s32 ret,ret2;
	u8 arg[4] = {0,0,0,0};
	char *ptr = (char*)buf;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

	if(num_sectors<1) return CARDIO_ERROR_INVALIDPARAM;

#ifdef _CARDIO_DEBUG
	kprintf("sdgecko_readSectors(%d,%d,%d,%d)\n",drv_no,sector_no,num_sectors,_ioPageSize[drv_no]);
#endif

	// Must be 512b, otherwise fail!
	if(_ioPageSize[drv_no]!=PAGE_SIZE512) {
		_ioPageSize[drv_no] = PAGE_SIZE512;
		if((ret=__card_setblocklen(drv_no,_ioPageSize[drv_no]))!=0) return ret;
	}

	if(_ioReadSector[drv_no]!=sector_no) {
		__convert_sector(drv_no,sector_no,arg);
		if((ret=__card_sendcmd(drv_no,0x12,arg))!=0) return ret;
		if((ret=__card_response1(drv_no))!=0) return ret;
	}

	for(i=0;i<num_sectors;i++) {
		if((ret=__card_dataread(drv_no,ptr,_ioPageSize[drv_no]))!=0) {
			if((ret2=__card_sendcmd(drv_no,0x0C,NULL))!=0) return ret2;
			if((ret2=__card_stopresponse(drv_no))!=0) return ret2;
			return ret;
		}
		ptr += _ioPageSize[drv_no];
		sector_no++;
	}
	_ioReadSector[drv_no] = sector_no;
	return ret;
}

s32 sdgecko_writeSector(s32 drv_no,const void *buf,u32 sector_no)
{
	s32 ret;
	u8 arg[4] = {0,0,0,0};
	char *ptr = (char*)buf;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

#ifdef _CARDIO_DEBUG
	printf("sdgecko_writeSector(%d,%d,%d)\n",drv_no,sector_no,_ioPageSize[drv_no]);
#endif

	if(_ioPageSize[drv_no]!=PAGE_SIZE512) {
		_ioPageSize[drv_no] = PAGE_SIZE512;
		if((ret=__card_setblocklen(drv_no,_ioPageSize[drv_no]))!=0) return ret;
	}

	// SDHC support fix
	__convert_sector(drv_no,sector_no,arg);
	if((ret=__card_sendcmd(drv_no,0x18,arg))!=0) return ret;
	if((ret=__card_response1(drv_no))!=0) return ret;

	if((ret=__card_datawrite(drv_no,ptr,_ioPageSize[drv_no]))!=0) return ret;
	if((ret=__card_dataresponse(drv_no))!=0) return ret;
	if((ret=__card_sendcmd(drv_no,0x0D,NULL))!=0) return ret;
	return __card_response2(drv_no);
}

s32 sdgecko_writeSectors(s32 drv_no,u32 sector_no,u32 num_sectors,const void *buf)
{
	u32 i;
	s32 ret,ret2;
	u8 arg[4] = {0,0,0,0};
	char *ptr = (char*)buf;

	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

	if(num_sectors<1) return CARDIO_ERROR_INVALIDPARAM;

#ifdef _CARDIO_DEBUG
	printf("sdgecko_writeSectors(%d,%d,%d,%d)\n",drv_no,sector_no,num_sectors,_ioPageSize[drv_no]);
#endif

	if(_ioPageSize[drv_no]!=PAGE_SIZE512) {
		_ioPageSize[drv_no] = PAGE_SIZE512;
		if((ret=__card_setblocklen(drv_no,_ioPageSize[drv_no]))!=0) return ret;
	}

	// send SET_WRITE_BLK_ERASE_CNT cmd
	arg[0] = (num_sectors>>24)&0xff;
	arg[1] = (num_sectors>>16)&0xff;
	arg[2] = (num_sectors>>8)&0xff;
	arg[3] = num_sectors&0xff;
	if((ret=__card_sendappcmd(drv_no,0x17,arg))!=0) return ret;
	if((ret=__card_response1(drv_no))!=0) return ret;

	// SDHC support fix
	__convert_sector(drv_no,sector_no,arg);
	if((ret=__card_sendcmd(drv_no,0x19,arg))!=0) return ret;
	if((ret=__card_response1(drv_no))!=0) return ret;

	for(i=0;i<num_sectors;i++) {
		if((ret=__card_multidatawrite(drv_no,ptr,_ioPageSize[drv_no]))!=0) return ret;
		if((ret=__card_dataresponse(drv_no))!=0) {
			if((ret2=__card_sendcmd(drv_no,0x0C,arg))!=0) return ret2;
			if((ret2=__card_stopresponse(drv_no))!=0) return ret2;
			return ret;
		}
		ptr += _ioPageSize[drv_no];
	}

	if((ret=__card_multiwritestop(drv_no))!=0) return ret;
	if((ret=__card_sendcmd(drv_no,0x0D,NULL))!=0) return ret;
	return __card_response2(drv_no);
}

s32 sdgecko_doUnmount(s32 drv_no)
{
	if(drv_no<0 || drv_no>=MAX_DRIVE) return CARDIO_ERROR_NOCARD;

	if(_ioCardInserted[drv_no]==TRUE) {
		if(_ioFlag[drv_no]==INITIALIZED)
			__card_softreset(drv_no);
		_ioFlag[drv_no] = NOT_INITIALIZED;
		_ioCardInserted[drv_no] = FALSE;
		if(drv_no!=2 && _ioCardSelect[drv_no]==EXI_DEVICE_0) {
			EXI_Detach(drv_no);
			sdgecko_ejectedCB(drv_no);
		}
	}
	if(_ioRetryCB)
		return _ioRetryCB(drv_no);

	return CARDIO_ERROR_READY;
}

static void (*pfCallbackIN[MAX_DRIVE])(s32) = {NULL, NULL};
static void (*pfCallbackOUT[MAX_DRIVE])(s32) = {NULL, NULL};

void sdgecko_insertedCB(s32 drv_no)
{
	if(pfCallbackIN[drv_no])
		pfCallbackIN[drv_no](drv_no);
}

void sdgecko_ejectedCB(s32 drv_no)
{
	if(pfCallbackOUT[drv_no])
		pfCallbackOUT[drv_no](drv_no);
}

u32 sdgecko_getDevice(s32 drv_no)
{
	return _ioCardSelect[drv_no];
}

void sdgecko_setDevice(s32 drv_no, u32 dev)
{
	if(_ioFlag[drv_no]==NOT_INITIALIZED)
		_ioCardSelect[drv_no] = dev;
}

u32 sdgecko_getSpeed(s32 drv_no)
{
	return _ioCardFreq[drv_no];
}

void sdgecko_setSpeed(s32 drv_no, u32 freq)
{
	_ioHighSpeed[drv_no] = freq;
	if(freq>EXI_SPEED32MHZ) freq = EXI_SPEED32MHZ;

	if(_ioFlag[drv_no]==NOT_INITIALIZED)
		_ioDefaultSpeed[drv_no] = freq;
}

u32 sdgecko_getPageSize(s32 drv_no)
{
	return _ioPageSize[drv_no];
}

s32 sdgecko_setPageSize(s32 drv_no, u32 size)
{
	s32 ret;

	ret = sdgecko_preIO(drv_no);
	if(ret!=0) return ret;

	if(_ioPageSize[drv_no]!=size) {
		_ioPageSize[drv_no] = size;
		ret = __card_setblocklen(drv_no, _ioPageSize[drv_no]);
	}
	return ret;
}

u32 sdgecko_getAddressingType(s32 drv_no)
{
	return _ioAddressingType[drv_no];
}

u32 sdgecko_getTransferMode(s32 drv_no)
{
	return _ioTransferMode[drv_no];
}

bool sdgecko_isInserted(s32 drv_no)
{
	return (_ioCardInserted[drv_no] = __card_check(drv_no));
}

bool sdgecko_isInitialized(s32 drv_no)
{
	return (_ioFlag[drv_no]==INITIALIZED);
}
