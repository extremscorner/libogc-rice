#include <stdlib.h>
#include <system.h>
#include <string.h>
#include <asm.h>
#include <processor.h>
#include "system.h"
#include "lwp_wkspace.h"

heap_cntrl __wkspace_heap;
static heap_iblock __wkspace_iblock;
static u32 __wkspace_heap_size = 0;

u32 __lwp_wkspace_heapsize()
{
	return __wkspace_heap_size;
}

u32 __lwp_wkspace_heapfree()
{
	__lwp_heap_getinfo(&__wkspace_heap,&__wkspace_iblock);
	return __wkspace_iblock.free_size;
}

u32 __lwp_wkspace_heapused()
{
	__lwp_heap_getinfo(&__wkspace_heap,&__wkspace_iblock);
	return __wkspace_iblock.used_size;
}

void __lwp_wkspace_init(u32 size)
{
	void *arLo = SYS_AllocArenaMemLo(size,PPC_CACHE_ALIGNMENT);
	__wkspace_heap_size = __lwp_heap_init(&__wkspace_heap,arLo,size,PPC_ALIGNMENT);
}
