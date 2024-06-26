#ifndef __OGC_CONTEXT_H__
#define __OGC_CONTEXT_H__

#include <gctypes.h>

#define NUM_EXCEPTIONS		15

#define EX_SYS_RESET		 0
#define EX_MACH_CHECK		 1
#define EX_DSI				 2
#define EX_ISI				 3
#define EX_INT				 4
#define EX_ALIGN			 5
#define EX_PRG				 6
#define EX_FP				 7
#define EX_DEC				 8
#define EX_SYS_CALL			 9
#define EX_TRACE			10
#define EX_PERF				11
#define EX_IABR				12
#define EX_RESV				13
#define EX_THERM			14

#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

typedef struct _excption_frame {
	u32 SP, LRSAVE;
	u32 EXCPT_Number;
	u32 SRR0, SRR1;
	u32 GPR[32];
	u32 CR, LR, CTR, XER, MSR;

	f64 FPR[32];
	u64	FPSCR;
	f64 PSFPR[32];
} frame_context;

#ifdef __cplusplus
   }
#endif /* __cplusplus */

#endif
