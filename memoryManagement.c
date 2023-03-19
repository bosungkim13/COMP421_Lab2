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

void initPhysicalPageArray(unsigned int pmem_size){
    // keep track of page status (0 free, 1 used)
    numPhysicalPages = pmem_size/PAGESIZE;
    isPhysicalPageOccupied = malloc(numPhysicalPages * sizeof(int));
    memset(isPhysicalPageOccupied, 0, numPhysicalPages);
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
    for (i = 0; i < numPhysicalPages; i++){
        if (isPhysicalPageOccupied[i] == 0){
            isPhysicalPageOccupied[i] = 1;
            return i;
        }
    }
    Halt();
}


void brkHandler(ExceptionInfo *frame){
    // TODO
}

int SetKernelBrk(void *addr) {
    int i;
    if (isVMInitialized) {
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
        }
    } else {
        // SetKernelBrk should never be reallocate a page
        if ((long)addr <= (long)kernelBrk - PAGESIZE) {
            return -1;
        }
        markKernelPagesTo(addr);
    }

    return 0;
}

