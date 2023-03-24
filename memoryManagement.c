// include headers as needed
#include "memoryManagement.h"
#include "pageTableManagement.h"
#include "processScheduling.h"
#include "processControlBlock.h"

// Initialize integer array to keep track of page status (0 free, 1 used)
int *isPhysicalPageOccupied = NULL;
int numPhysicalPages;
void *kernelBrk = (void *)VMEM_1_BASE;
int isVMInitialized = 0;
void* stackSwapSpace;

void initKernelBrk(void *origBrk){
	kernelBrk = origBrk;
}

void initPhysicalPageArray(unsigned int pmem_size){
    // keep track of page status (0 free, 1 used)
    numPhysicalPages = pmem_size/PAGESIZE;
    isPhysicalPageOccupied = malloc(numPhysicalPages * sizeof(int));
    memset(isPhysicalPageOccupied, 0, numPhysicalPages);
    markPagesInRange((void*)VMEM_1_BASE, kernelBrk);
}
// helper function to return number of free phyical pages
int freePhysicalPageCount(){
    int count = 0;
    int i;

    for (i = 0; i < numPhysicalPages; i++){
        if (isPhysicalPageOccupied[i] == 0){
            count = count + 1;
        }
    }
    return count;
}

// helper function to mark the pages in the given ranges as occupied
void markPagesInRange(void *start, void *end){
    int i;
    int begin = DOWN_TO_PAGE(start) / PAGESIZE;
    int limit = UP_TO_PAGE(end) / PAGESIZE;

    for (i = begin; i < limit; i++){
        isPhysicalPageOccupied[i] = 1;
    }
    TracePrintf(1, "memoryManagement - markPagesInRange - Marking pages [%d, %d)\n", begin, limit);
    TracePrintf(1, "memoryManagement - markPagesInRange - Current page marking:\n");
    for(i = 0; i < numPhysicalPages; i++){
    	TracePrintf(1, "Pfn %d has marking %d\n", i, isPhysicalPageOccupied[i]);
    }
}

// moves the kernel break and marks the physical pages as taken in the free physical page 
void markKernelPagesTo(void *end){
    if(isPhysicalPageOccupied != NULL){
        markPagesInRange(kernelBrk, end);
    }
    // move the kernelBrk up
    kernelBrk = (void *)UP_TO_PAGE(end);
}

unsigned int
getFreePhysicalPage(){
    int i;
    for (i = (isVMInitialized ? 0 : MEM_INVALID_PAGES); i < numPhysicalPages; i++){
        if (isPhysicalPageOccupied[i] == 0){
            isPhysicalPageOccupied[i] = 1;
            TracePrintf(1, "GetFreePhysicalPage - Providing physical page %d\n", i);
            return i;
        }
    }
    Halt();
}

unsigned int getTopFreePhysicalPage(){
    int pfn = DOWN_TO_PAGE(VMEM_1_LIMIT - 1) / PAGESIZE;
    isPhysicalPageOccupied[pfn] = 1;
    return pfn;
}

void freePhysicalPage(unsigned int pfn){
	isPhysicalPageOccupied[pfn] = 0;
}

void brkHandler(ExceptionInfo *frame){
    // TODO
}

int SetKernelBrk(void *addr) {
    int i;
    if (isVMInitialized) {
    	TracePrintf(2, "SetKernelBrk After VM: Requesting address %p, kernelBrk currently at %p\n", addr, kernelBrk);
        int numNeedPages = ((long)UP_TO_PAGE(addr) - (long)kernelBrk)/PAGESIZE;
        if(freePhysicalPageCount() < numNeedPages){
            return -1;
        } else {
            for (i = 0; i < numNeedPages; i++){
                unsigned int physicalPageNum = getFreePhysicalPage();
                int vpn = ((long)kernelBrk - VMEM_1_BASE)/PAGESIZE + i;

               // set the kernel pte as valid and update the corresponding ppn
                kernelPageTable[vpn].valid = 1;
                kernelPageTable[vpn].pfn = physicalPageNum;
            }
            markKernelPagesTo(addr);
        }
        TracePrintf(2, "SetKernelBrk After VM: Requested address %p, kernelBrk moved to %p\n", addr, kernelBrk);
    } else {
        // SetKernelBrk should never be reallocate a page
        if ((long)addr <= (long)kernelBrk - PAGESIZE) {
            return -1;
        }
        markKernelPagesTo(addr);
    }

    return 0;
}

void initVM(){
	isVMInitialized = 1;
	WriteRegister(REG_VM_ENABLE, 1);
}

void* virtualToPhysicalAddr(void *va){
    int pfn;
    void* vPageBase = (void*)DOWN_TO_PAGE(va);
    void* pPageBase;

    if (vPageBase < (void*)VMEM_1_BASE){
        struct scheduleNode *currentNode = getHead();
        pfn = currentNode->pcb->pageTable[((long)vPageBase) / PAGESIZE].pfn;
    }
    else{
        pfn = kernelPageTable[((long)vPageBase - VMEM_1_BASE) / PAGESIZE].pfn;
    }
    pPageBase = (void*) (long)(pfn*PAGESIZE);

    //offset is equivalent to va & PAGEOFFSET
    return (void*)((long)pPageBase + ((long)va & PAGEOFFSET));
}

void setupStackSwapSpace(){
	int numPages = KERNEL_STACK_PAGES;
	TracePrintf(2, "memoryManagement - setupSwapSpace - Beginning malloc for %d pages worth\n", numPages);
	stackSwapSpace = malloc(numPages * PAGESIZE);
	TracePrintf(2, "memoryManagement - setupSwapSpace - Completed malloc for %d pages worth\n", numPages);
}
void* getStackSwapSpace(){
	return stackSwapSpace;
}
