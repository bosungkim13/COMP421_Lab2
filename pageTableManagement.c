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

struct pte*
createPageTable(){

    TracePrintf(3, "pageTableManagement: Started creating page table");
    struct pageTableRecord *current = getFirstPageTableRecord();
    while (current != NULL){
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
            current = current->next;
        }
    }
    TracePrintf(3, "pageTableManagement: No space in current page table records... creating new page table record");
    // creating new page table record entry

    // find the end
    while (current->next != NULL) {
        current = current->next;
    }

    struct pageTableRecord *newPTRecord = malloc(sizeof(struct pageTableRecord));

    void *pageBase = (void *)DOWN_TO_PAGE((long)current->pageBase - 1);

    newPTRecord->pageBase = pageBase;
    newPTRecord->isTopFull = 1;
    newPTRecord->isBottomFull = 0;
    newPTRecord->next = NULL;


    int vpn = (long)(pageBase - VMEM_1_BASE)/PAGESIZE;
    kernelPageTable[vpn].valid = 1;
    
    unsigned int pfn = getFreePhysicalPage();
    kernelPageTable[vpn].pfn = pfn;

    current->next = newPTRecord;

    struct pte *newPageTable = (struct pte *)((long)pageBase + PAGE_TABLE_SIZE);

    // return the top half of page table
    return newPageTable;
}

void prepPageTable(struct pte *pageTable){
    TracePrintf(2, "pageTableManagement: Prepping page table at address %p\n", pageTable);
    int i;
    for(i = 0; i < PAGE_TABLE_LEN; i++) {
        if (i < KERNEL_STACK_BASE / PAGESIZE) {
            // non kernel stack pages
            page_table[i].valid = 0;
            page_table[i].kprot = PROT_READ | PROT_WRITE;
            page_table[i].uprot = PROT_READ | PROT_WRITE | PROT_EXEC;
        } else {
            // kernel stack pages
            page_table[i].valid = 1;
            page_table[i].kprot = PROT_READ | PROT_WRITE;
            page_table[i].uprot = PROT_NONE;            
        }
    }
    TracePrintf(2, "pageTableManagement: Done filling out page table.\n");
}

