#ifndef __OGC_MACHINE_PROCESSOR_H__
#define __OGC_MACHINE_PROCESSOR_H__

#include <gctypes.h>
#include "asm.h"

#define __stringify(rn)								#rn
#define ATTRIBUTE_ALIGN(v)							__attribute__((aligned(v)))
// courtesy of Marcan
#define STACK_ALIGN(type, name, cnt, alignment)		u8 _al__##name[((sizeof(type)*(cnt)) + (alignment) + (((sizeof(type)*(cnt))%(alignment)) > 0 ? ((alignment) - ((sizeof(type)*(cnt))%(alignment))) : 0))]; \
													type *name = (type*)(((u32)(_al__##name)) + ((alignment) - (((u32)(_al__##name))&((alignment)-1))))

#define _sync() asm volatile("sync")
#define _isync() asm volatile("isync")
#define _nop() asm volatile("nop")
#define ppcsync() _isync(); _sync()
#define ppchalt() ({					\
	asm volatile("sync");				\
	while(1) {							\
		asm volatile("nop");			\
		asm volatile("li 3,0");			\
		asm volatile("nop");			\
	}									\
})

#define mfpvr() ({register u32 _rval; \
		asm volatile("mfpvr %0" : "=r"(_rval)); _rval;})

#define mfdcr(_rn) ({register u32 _rval; \
		asm volatile("mfdcr %0," __stringify(_rn) \
             : "=r" (_rval)); _rval;})
#define mtdcr(rn, val)  asm volatile("mtdcr " __stringify(rn) ",%0" : : "r" (val))

#define mfmsr()   ({register u32 _rval; \
		asm volatile("mfmsr %0" : "=r" (_rval)); _rval;})
#define mtmsr(val)  asm volatile("mtmsr %0" : : "r" (val))

#define mfdec()   ({register u32 _rval; \
		asm volatile("mfdec %0" : "=r" (_rval)); _rval;})
#define mtdec(_val)  asm volatile("mtdec %0" : : "r" (_val))

#define mfspr(_rn) \
({	register u32 _rval = 0; \
	asm volatile("mfspr %0," __stringify(_rn) \
	: "=r" (_rval));\
	_rval; \
})

#define mtspr(_rn, _val) asm volatile("mtspr " __stringify(_rn) ",%0" : : "r" (_val))

#define mfwpar()		mfspr(WPAR)
#define mtwpar(_val)	mtspr(WPAR,_val)

#define mfmmcr0()		mfspr(MMCR0)
#define mtmmcr0(_val)	mtspr(MMCR0,_val)
#define mfmmcr1()		mfspr(MMCR1)
#define mtmmcr1(_val)	mtspr(MMCR1,_val)

#define mfpmc1()		mfspr(PMC1)
#define mtpmc1(_val)	mtspr(PMC1,_val)
#define mfpmc2()		mfspr(PMC2)
#define mtpmc2(_val)	mtspr(PMC2,_val)
#define mfpmc3()		mfspr(PMC3)
#define mtpmc3(_val)	mtspr(PMC3,_val)
#define mfpmc4()		mfspr(PMC4)
#define mtpmc4(_val)	mtspr(PMC4,_val)

#define mfhid0()		mfspr(HID0)
#define mthid0(_val)	mtspr(HID0,_val)
#define mfhid1()		mfspr(HID1)
#define mthid1(_val)	mtspr(HID1,_val)
#define mfhid2()		mfspr(HID2)
#define mthid2(_val)	mtspr(HID2,_val)
#define mfhid4()		mfspr(HID4)
#define mthid4(_val)	mtspr(HID4,_val)

#define mfthrm1()		mfspr(THRM1)
#define mtthrm1(_val)	mtspr(THRM1,_val)
#define mfthrm2()		mfspr(THRM2)
#define mtthrm2(_val)	mtspr(THRM2,_val)
#define mfthrm3()		mfspr(THRM3)
#define mtthrm3(_val)	mtspr(THRM3,_val)

#define __lhbrx(base,index)			\
({	register u16 res;				\
	__asm__ volatile ("lhbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

#define __lwbrx(base,index)			\
({	register u32 res;				\
	__asm__ volatile ("lwbrx	%0,%1,%2" : "=r"(res) : "b%"(index), "r"(base) : "memory"); \
	res; })

#define __lswx(base,bytes)			\
({	register u32 res;				\
	__asm__ volatile ("mtxer %2; lswx %0,%y1" : "=&b"(res) : "Z"(*(u32*)(base)), "r"(bytes) : "xer"); \
	res; })

#define __sthbrx(base,index,value)	\
	__asm__ volatile ("sthbrx	%0,%1,%2" : : "r"(value), "b%"(index), "r"(base) : "memory")

#define __stwbrx(base,index,value)	\
	__asm__ volatile ("stwbrx	%0,%1,%2" : : "r"(value), "b%"(index), "r"(base) : "memory")

#define __stswx(base,bytes,value)	\
	__asm__ volatile ("mtxer %2; stswx %1,%y0" : "=Z"(*(u32*)(base)) : "r"(value), "r"(bytes) : "xer");

#ifndef bswap16
#define bswap16(_val)	__builtin_bswap16(_val)
#endif
#ifndef bswap32
#define bswap32(_val)	__builtin_bswap32(_val)
#endif
#ifndef bswap64
#define bswap64(_val)	__builtin_bswap64(_val)
#endif

#define cntlzw(_val)	__builtin_clz(_val)
#define cntlzd(_val)	__builtin_clzll(_val)

#define _CPU_MSR_GET( _msr_value ) \
  do { \
    _msr_value = 0; \
    asm volatile ("mfmsr %0" : "=&r" ((_msr_value)) : "0" ((_msr_value))); \
  } while (0)

#define _CPU_MSR_SET( _msr_value ) \
{ asm volatile ("mtmsr %0" : "=&r" ((_msr_value)) : "0" ((_msr_value))); }

#define _CPU_ISR_Enable() \
	do { \
		register u32 _val = 0; \
		__asm__ __volatile__ ( \
			"mfmsr %0\n" \
			"ori %0,%0,0x8000\n" \
			"mtmsr %0" \
			: "=&r" (_val) : : "memory" \
		); \
	} while (0)

#define _CPU_ISR_Disable( _isr_cookie ) \
	do { \
		register u32 _disable_mask = 0; \
		__asm__ __volatile__ ( \
			"mfmsr %1\n" \
			"rlwinm %0,%1,0,17,15\n" \
			"mtmsr %0\n" \
			"extrwi %1,%1,1,16" \
			: "=&r" (_disable_mask), "=&r" (_isr_cookie) : : "memory" \
		); \
	} while (0)

#define _CPU_ISR_Restore( _isr_cookie )  \
	do { \
		register u32 _enable_mask = 0; \
		__asm__ __volatile__ ( \
			"mfmsr %0\n" \
			"insrwi %0,%1,1,16\n" \
			"mtmsr %0\n" \
			: "=&r" (_enable_mask) : "r" (_isr_cookie) : "memory" \
		); \
	} while (0)

#define _CPU_ISR_Flash( _isr_cookie ) \
	do { \
		register u32 _flash_mask = 0; \
		__asm__ __volatile__ ( \
			"mfmsr %0\n" \
			"insrwi %0,%1,1,16\n" \
			"mtmsr %0\n" \
			"rlwinm %0,%0,0,17,15\n" \
			"mtmsr %0\n" \
			: "=&r" (_flash_mask) : "r" (_isr_cookie) : "memory" \
		); \
	} while (0)

#define _CPU_FPR_Enable() \
{ register u32 _val = 0; \
	  asm volatile ("mfmsr %0; ori %0,%0,0x2000; mtmsr %0" : \
					"=&r" (_val) : "0" (_val));\
}

#define _CPU_FPR_Disable() \
{ register u32 _val = 0; \
	  asm volatile ("mfmsr %0; rlwinm %0,%0,0,19,17; mtmsr %0" : \
					"=&r" (_val) : "0" (_val));\
}

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

// Basic I/O

static inline u32 read32(u32 addr)
{
	u32 x;
	asm volatile("lwz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

static inline void write32(u32 addr, u32 x)
{
	asm("stw %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

static inline void mask32(u32 addr, u32 clear, u32 set)
{
	write32(addr, (read32(addr)&(~clear)) | set);
}

static inline u16 read16(u32 addr)
{
	u16 x;
	asm volatile("lhz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

static inline void write16(u32 addr, u16 x)
{
	asm("sth %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

static inline u8 read8(u32 addr)
{
	u8 x;
	asm volatile("lbz %0,0(%1) ; sync" : "=r"(x) : "b"(0xc0000000 | addr));
	return x;
}

static inline void write8(u32 addr, u8 x)
{
	asm("stb %0,0(%1) ; eieio" : : "r"(x), "b"(0xc0000000 | addr));
}

static inline void writef32(u32 addr, f32 x)
{
	asm("stfs %0,0(%1) ; eieio" : : "f"(x), "b"(0xc0000000 | addr));
}

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
