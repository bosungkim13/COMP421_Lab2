#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

//  each node of the linked list of free half physical pages to use when allocating memory for page tables.
//  In order to ensure that page tables were physically contiguous in memory, we took advantage
//  of the fact that a the size of a page table is PAGESIZE/2. These physical pages reside at
//  the top of REGION_1.
struct pageTableRecord{
    void *pageBase;
    int isTopFull;
    int isBottomFull;
    struct pageTableRecord *next;
};
extern struct pte *kernelPageTable;

void initKernelPT();