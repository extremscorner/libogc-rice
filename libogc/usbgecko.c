/*
	USB Gecko SE API - Part of the Gecko Basic SDK
 
	gecko.c
 
	2010 www.usbgecko.com - code by ian@unrom.com

	All the functions below are used to interface with the USB Gecko SE device, the functions are highly optimized to use with the hardware
	and you should never have to modify them.

	Code is Public Domain.
*/


#include "usbgecko.h"

#if defined(HW_DOL)
	#define EXI_CHAN0SR		*(volatile unsigned long*) 0xCC006800 // Channel 0 Status Register
	#define EXI_CHAN1SR		*(volatile unsigned long*) 0xCC006814 // Channel 1 Status Register
	#define EXI_CHAN0CR		*(volatile unsigned long*) 0xCC00680c // Channel 0 Control Register
	#define EXI_CHAN1CR		*(volatile unsigned long*) 0xCC006820 // Channel 1 Control Register
	#define EXI_CHAN0DATA	*(volatile unsigned long*) 0xCC006810 // Channel 0 Immediate Data
	#define EXI_CHAN1DATA	*(volatile unsigned long*) 0xCC006824 // Channel 1 Immediate Data
#elif defined(HW_RVL)
	#define EXI_CHAN0SR		*(volatile unsigned long*) 0xCD006800 // Channel 0 Status Register
	#define EXI_CHAN1SR		*(volatile unsigned long*) 0xCD006814 // Channel 1 Status Register
	#define EXI_CHAN0CR		*(volatile unsigned long*) 0xCD00680c // Channel 0 Control Register
	#define EXI_CHAN1CR		*(volatile unsigned long*) 0xCD006820 // Channel 1 Control Register
	#define EXI_CHAN0DATA	*(volatile unsigned long*) 0xCD006810 // Channel 0 Immediate Data
	#define EXI_CHAN1DATA	*(volatile unsigned long*) 0xCD006824 // Channel 1 Immediate Data
#endif

#define EXI_TSTART			1


static unsigned int gecko_sendbyte(s32 chn, char data)
{
	unsigned int i = 0;

	if(chn == 0)
	{
		EXI_CHAN0SR = 0x000000D0;
		EXI_CHAN0DATA = 0xB0000000 | (data << 20);
		EXI_CHAN0CR = 0x19;

		while((EXI_CHAN0CR) & EXI_TSTART);
		i = EXI_CHAN0DATA;
		EXI_CHAN0SR = 0;
	}
	else if(chn == 1)
	{
		EXI_CHAN1SR = 0x000000D0;
		EXI_CHAN1DATA = 0xB0000000 | (data << 20);
		EXI_CHAN1CR = 0x19;

		while((EXI_CHAN1CR) & EXI_TSTART);
		i = EXI_CHAN1DATA;
		EXI_CHAN1SR = 0;
	}

	if (i&0x04000000)
	{
		return 1;
	}

	return 0;
}


static unsigned int gecko_receivebyte(s32 chn, char* data)
{
	unsigned int i = 0;

	if(chn == 0)
	{
		EXI_CHAN0SR = 0x000000D0;
		EXI_CHAN0DATA = 0xA0000000;	
		EXI_CHAN0CR = 0x19;

		while((EXI_CHAN0CR) & EXI_TSTART);
		i = EXI_CHAN0DATA;
		EXI_CHAN0SR = 0;
	}
	else if(chn == 1)
	{
		EXI_CHAN1SR = 0x000000D0;
		EXI_CHAN1DATA = 0xA0000000;
		EXI_CHAN1CR = 0x19;

		while((EXI_CHAN1CR) & EXI_TSTART);
		i = EXI_CHAN1DATA;
		EXI_CHAN1SR = 0;
	} 

	if (i&0x08000000)
	{
		*data = (i>>16)&0xff;
		return 1;
	} 
	
	return 0;
}


// return 1, it is ok to send data to PC
// return 0, FIFO full
static unsigned int gecko_checktx(s32 chn)
{
	unsigned int i  = 0;

	if(chn == 0)
	{
		EXI_CHAN0SR = 0x000000D0;
		EXI_CHAN0DATA = 0xC0000000;
		EXI_CHAN0CR = 0x09;

		while((EXI_CHAN0CR) & EXI_TSTART);
		i = EXI_CHAN0DATA;
		EXI_CHAN0SR = 0x0;
	}
	else if(chn == 1)
	{
		EXI_CHAN1SR = 0x000000D0;
		EXI_CHAN1DATA = 0xC0000000;
		EXI_CHAN1CR = 0x09;

		while((EXI_CHAN1CR) & EXI_TSTART);
		i = EXI_CHAN1DATA;
		EXI_CHAN1SR = 0x0;
	}

	if (i&0x04000000)
	{
		return 1;
	}

	return 0;
}


// return 1, there is data in the FIFO to recieve
// return 0, FIFO is empty
static unsigned int gecko_checkrx(s32 chn)
{
	unsigned int i = 0;

	if(chn == 0)
	{
		EXI_CHAN0SR = 0x000000D0;
		EXI_CHAN0DATA = 0xD0000000;
		EXI_CHAN0CR = 0x09;

		while((EXI_CHAN0CR) & EXI_TSTART);
		i = EXI_CHAN0DATA;
		EXI_CHAN0SR = 0x0;
	}
	else if(chn == 1)
	{
		EXI_CHAN1SR = 0x000000D0;
		EXI_CHAN1DATA = 0xD0000000;
		EXI_CHAN1CR = 0x09;

		while((EXI_CHAN1CR) & EXI_TSTART);
		i = EXI_CHAN1DATA;
		EXI_CHAN1SR = 0x0;
	}

	if (i&0x04000000)
	{
		return 1;
	}

	return 0;
}


void usb_flush(s32 chn)
{
	char tempbyte;
	int i;

	for(i = 0;i < 256;i++)
	{
		gecko_receivebyte(chn, &tempbyte);
	}
}

int usb_isgeckoalive(s32 chn)
{
	unsigned int ret = 0;

	if(chn == 0)
	{
		EXI_CHAN0SR = 0x000000D0;
		EXI_CHAN0DATA = 0x90000000;
		EXI_CHAN0CR = 0x19;

		while((EXI_CHAN0CR) & EXI_TSTART);
		ret = EXI_CHAN0DATA;
		EXI_CHAN0SR = 0x0;

		if(ret == 0x04700000)
		{
			usb_flush(0);
			return 1;
		}
	}
	else if(chn == 1)
	{
		EXI_CHAN1SR = 0x000000D0;
		EXI_CHAN1DATA = 0x90000000;
		EXI_CHAN1CR = 0x19;

		while((EXI_CHAN1CR) & EXI_TSTART);
		ret = EXI_CHAN1DATA;
		EXI_CHAN1SR = 0x0;

		if(ret == 0x04700000)
		{
			usb_flush(1);
			return 1;
		}
	}

	return 0;
}

int usb_recvbuffer(s32 chn,void *buffer,int size)
{
	char *receivebyte = (char*)buffer;
	unsigned int ret = 0;

	while (size > 0)
	{
		ret = gecko_receivebyte(chn, receivebyte);
		if(ret == 1)
		{
			receivebyte++;
			size--;
		}
	}

	return 0;
}

int usb_sendbuffer(s32 chn,const void *buffer,int size)
{
	char *sendbyte = (char*) buffer;
	unsigned int ret = 0;

	while (size  > 0)
	{
		ret = gecko_sendbyte(chn, *sendbyte);
		if(ret == 1)
		{
			sendbyte++;
			size--;
		}
	}

	return 0;
}

int usb_recvbuffer_safe(s32 chn,void *buffer,int size)
{
	char *receivebyte = (char*)buffer;
	unsigned int ret = 0;

	while (size > 0)
	{
		if(gecko_checkrx(chn))
		{
			ret = gecko_receivebyte(chn, receivebyte);
			if(ret == 1)
			{
				receivebyte++;
				size--;
			}
		}
	}

	return 0;
}

int usb_sendbuffer_safe(s32 chn,const void *buffer,int size)
{
	char *sendbyte = (char*) buffer;
	unsigned int ret = 0;

	while (size  > 0)
	{
		if(gecko_checktx(chn))
		{
			ret = gecko_sendbyte(chn, *sendbyte);
			if(ret == 1)
			{
				sendbyte++;
				size--;
			}
		}
	}

	return 0;
}
