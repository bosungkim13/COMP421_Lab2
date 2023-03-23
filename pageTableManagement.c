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

struct pte* initKernelPT(){
    TracePrintf(2, "pageTableManagement: Kernel page table initialization started.\n");

    kernelPageTable = malloc(PAGE_TABLE_SIZE);
    
    int endHeap = UP_TO_PAGE((long)kernelBrk - (long)VMEM_1_BASE) / PAGESIZE;
    int endText = ((long)&_etext - (long)VMEM_1_BASE) / PAGESIZE;
    TracePrintf(2, "pageTableManagement: kernel heap ends at vpn: %d kernel text ends at vpn: %d.\n");

    int i;
    for (i = 0; i < PAGE_TABLE_LEN; i++){
        // text has read and execute permissions
        if (i < endText){
            kernelPageTable[i].valid = 1;
            kernelPageTable[i].kprot = PROT_READ | PROT_EXEC;
        }else if (i < endHeap){
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
    return kernelPageTable;

}

// function to create the first page table record
void initFirstPTRecord(){
  struct pageTableRecord *pageTableRecord = malloc(sizeof(struct pageTableRecord));

  void *pageBase = (void *)DOWN_TO_PAGE(VMEM_1_LIMIT - 1);
  //markPagesInRange((void*)(VMEM_1_LIMIT - 1), (void*)(VMEM_1_LIMIT - 1));

  pageTableRecord->pageBase = pageBase;
  pageTableRecord->isTopFull = 0;
  pageTableRecord->isBottomFull = 0;
  pageTableRecord->next = NULL;

  unsigned int pfn = getTopFreePhysicalPage();

  int virtualPageNum = (long)(pageBase - VMEM_1_BASE)/PAGESIZE;

  kernelPageTable[virtualPageNum].valid = 1;
  kernelPageTable[virtualPageNum].pfn = pfn;

  firstPageTableRecord = pageTableRecord;

}

struct pte*
createPageTable(){

    TracePrintf(3, "pageTableManagement: Started creating page table");
    struct pageTableRecord *current = getFirstPageTableRecord();
    while (1){
        if (current->isTopFull == 0){
            struct pte *newPT = (struct pte*) ((long)current->pageBase + PAGE_TABLE_SIZE);
            current -> isTopFull = 1;
            TracePrintf(3, "pageTableManagement: Used top of page to create Page Table");
            return newPT;
        } else if (current ->isBottomFull == 0){
            struct pte *newPT = (struct pte*) ((long) current->pageBase);
            current -> isBottomFull = 1;
            TracePrintf(3, "pageTableManagement: Used bottom of page to create Page Table");
            return newPT;
        }else{
            if(current->next != NULL){
                current = current->next;
            }else{
                // current is not null, current->next is null
                // we need to keep current so that when we malloc a new pageTableRecord
                // we can set the next pointer in current to the malloced struct
                break;
            }
        }
    }
    TracePrintf(3, "pageTableManagement: No space in current page table records... creating new page table record");
    // creating new page table record entry, then giving the top half as the new page table

    struct pageTableRecord *newPTRecord = malloc(sizeof(struct pageTableRecord));
    if(newPTRecord == NULL){
    	TracePrintf(2, "Kernel failed malloc for new pageTableRecord, halting ...");
    	Halt();
    }

    long pfn = getFreePhysicalPage();
    void *pageBase = (void*)(pfn * PAGESIZE);

    newPTRecord->pageBase = pageBase;
    newPTRecord->isTopFull = 1;
    newPTRecord->isBottomFull = 0;
    newPTRecord->next = NULL;

    int vpn = 1023;
    for(; vpn >= VMEM_1_BASE/PAGESIZE && kernelPageTable[vpn].valid == 1; vpn--){}
    if(vpn == (VMEM_0_LIMIT-1)/PAGESIZE){
    	TracePrintf(2, "Kernel failed finding an invalid vpn in the kernel pt to use for accessing the new pageTableRecord, halting ...");
    	Halt();
    }
    kernelPageTable[vpn].valid = 1;
    kernelPageTable[vpn].pfn = pfn;
    current->next = newPTRecord;

    struct pte *newPageTable = (struct pte *)((long)pageBase + PAGE_TABLE_SIZE);
    return newPageTable;
}

void fillPageTable(struct pte *pageTable){
    TracePrintf(2, "pageTableManagement: Filling page table at address %p\n", pageTable);
    int i;
    for (i = 0; i < PAGE_TABLE_LEN; i++) {
        if (i < KERNEL_STACK_BASE / PAGESIZE) {
            // non kernel stack pages
            pageTable[i].valid = 0;
            pageTable[i].kprot = PROT_READ | PROT_WRITE;
            pageTable[i].uprot = PROT_READ | PROT_WRITE | PROT_EXEC;
        } else {
            // kernel stack pages
            pageTable[i].valid = 1;
            pageTable[i].kprot = PROT_READ | PROT_WRITE;
            pageTable[i].uprot = PROT_NONE;
            pageTable[i].pfn = i;
        }
    }
    TracePrintf(2, "pageTableManagement: Done filling out page table.\n");
}

void fillInitialPageTable(struct pte *pageTable){
    int i;

    TracePrintf(2, "pageTableManagement: Begin filling initial page table at address %p.\n", pageTable);
    
    for (i = 0; i < PAGE_TABLE_LEN; i++) {
        if (i >= KERNEL_STACK_BASE / PAGESIZE) {
            // kernel stack pages
            pageTable[i].valid = 1;
            pageTable[i].kprot = PROT_READ | PROT_WRITE;
            pageTable[i].uprot = PROT_NONE;
        } else {
            // non kernel stack pages
            pageTable[i].valid = 0;
            pageTable[i].kprot = PROT_NONE;
            pageTable[i].uprot = PROT_READ | PROT_WRITE | PROT_EXEC;
        }
        // set the pfn
        pageTable[i].pfn = i;
    }
    TracePrintf(2, "pageTableManagement: Initial page table initialized.\n");

}

int numPagesInUse(struct pte *pageTable){
    int i;
    int count = 0;
    for(i = 0; i < PAGE_TABLE_LEN - KERNEL_STACK_PAGES; i++){
        if(pageTable[i].valid == 1){
        count++;
        }
    }
    return count;
}


