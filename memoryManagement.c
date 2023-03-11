// include headers as needed
#include "memoryManagement.h"
#include "pageTableManagement.h"
#include "processScheduling.h"
#include "processControlBlock.h"

// Initialize integer array to keep track of page status (0 free, 1 used)
int *isPhysicalPageOccupied = NULL;
int numPhysicalPages;
void *kernelBrk = (void *)VMEM_1_BASE;

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

