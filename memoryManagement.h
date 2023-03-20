#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include <string.h>

extern void *kernelBrk;

void initKernelBrk(void *orig_brk);
void initPhysicalPageArray(unsigned int pmem_size);
int freePhysicalPageCount();
void markPagesInRange(void *start, void *end);
unsigned int getFreePhysicalPage();
void freePhysicalPage(unsigned int pfn);
void brkHandler(ExceptionInfo *frame);
void markKernelPagesTo(void *end);
