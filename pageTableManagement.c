#include "memoryManagement.h"
#include "pageTableManagement.h"

// need to keep track of all the current page tables
struct pageTableRecord *firstPageTableRecord;

struct pte *kernelPageTable;

// getter for the firstPageTableRecord
struct pageTableRecord* 
getFirstPageTableRecord(){
    return firstPageTableRecord;
}

void initKernelPT(){
    TracePrintf(2, "pageTableManagement: Kernel page table initialization started.\n");

    kernelPageTable = malloc(PAGE_TABLE_SIZE);
    
    int endHeap = ((long)kernelBrk - (long)VMEM_1_BASE) / PAGESIZE;
    int endText = ((long)&_etext - (long)VMEM_1_BASE) / PAGESIZE;

    int i;
    for (i = 0; i < PAGE_TABLE_LEN; i++){
        // text has read and execute permissions
        if (i < endText){
            kernelPageTable[i].valid = 1;
            kernelPageTable[i].kprot = PROT_READ | PROT_EXEC;
        }else if (i <= endHeap){
            kernelPageTable[i].valid = 1;
            kernelPageTable[i].kprot = PROT_READ | PROT_WRITE;
        } else{
            kernelPageTable[i].valid = 0;
            kernelPageTable[i].kprot = PROT_READ | PROT_WRITE;
        }
        kernelPageTable[i].uprot = PROT_NONE;
        kernelPageTable[i].pfn = (long)VMEM_1_BASE / PAGESIZE + i;
    }
    TracePrintf(2, "pageTableManagement: Kernel page table initialized.\n");

}

// function to create the first page table record
void initFirstPTRecord(){
  struct pageTableRecord *pageTableRecord = malloc(sizeof(struct pageTableRecord));

  void *pageBase = (void *)DOWN_TO_PAGE(VMEM_1_LIMIT - 1);

  pageTableRecord->pageBase = pageBase;
  pageTableRecord->isTopFull = 0;
  pageTableRecord->isBottomFull = 0;
  pageTableRecord->next = NULL;

  unsigned int pfn = acquire_top_physical_page();

  int virtualPageNum = (long)(page_base - VMEM_1_BASE)/PAGESIZE;

  kernelPageTable[virtualPageNum].valid = 1;
  kernelPageTable[virtualPageNum].pfn = pfn;

  firstPageTableRecord = pageTableRecord;

}

