#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

struct pageTableRecord{
    void *pageBase;
    int isTopFull;
    int isBottomFull;
    struct pageTableRecord *next;
};
extern struct pte *kernelPageTable;

void initKernelPT();