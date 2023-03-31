#include "processScheduling.h"
#include "memoryManagement.h"
#include <stdio.h>
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
    
    int endHeap = UP_TO_PAGE((long)getKernelBrk() - (long)VMEM_1_BASE) / PAGESIZE;
    int endText = ((long)&_etext - (long)VMEM_1_BASE) / PAGESIZE;
    TracePrintf(2, "pageTableManagement: kernel heap ends at vpn: %d kernel text ends at vpn: %d.\n", endHeap, endText);

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

    TracePrintf(3, "pageTableManagement: Started creating page table\n");
    struct pageTableRecord *current = getFirstPageTableRecord();
    while (1){
        if (current->isTopFull == 0){
            struct pte *newPT = (struct pte*) ((long)current->pageBase + PAGE_TABLE_SIZE);
            current -> isTopFull = 1;
            TracePrintf(3, "pageTableManagement: Used top of page to create Page Table at %p\n", newPT);
            return newPT;
        } else if (current ->isBottomFull == 0){
            struct pte *newPT = (struct pte*) ((long) current->pageBase);
            current -> isBottomFull = 1;
            TracePrintf(3, "pageTableManagement: Used bottom of page to create Page Table at %p\n", newPT);
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
    TracePrintf(2, "pageTableManagement: No space in current page table records... creating new page table record\n");
    // creating new page table record entry, then giving the top half as the new page table

    struct pageTableRecord *newPTRecord = malloc(sizeof(struct pageTableRecord));
    if(newPTRecord == NULL){
    	printf("Kernel failed malloc for new pageTableRecord, halting ...\n");
    	Halt();
    }

    long pfn = getFreePhysicalPage();
    void *pageBase = (void*)DOWN_TO_PAGE((long)current->pageBase - 1);

    newPTRecord->pageBase = pageBase;
    newPTRecord->isTopFull = 1;
    newPTRecord->isBottomFull = 0;
    newPTRecord->next = NULL;

    int vpn = (long)(pageBase - VMEM_1_BASE) / PAGESIZE;
    if(kernelPageTable[vpn].valid == 1){
    	printf("Kernel tried creating new page table virtually contiguous to previous page table, but that vpn was already grown to by kernel malloc. Halting...\n");
    	Halt();
    }
    kernelPageTable[vpn].valid = 1;
    kernelPageTable[vpn].pfn = pfn;
    current->next = newPTRecord;

    struct pte *newPageTable = (struct pte *)((long)pageBase + PAGE_TABLE_SIZE);
    TracePrintf(3, "pageTableManagement: Created new PT record and used top of page to create Page Table at %p\n", newPageTable);
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

void freePageTable(struct pte* pageTable){
	TracePrintf(1, "pageTableManagement: Freeing page table: %p\n", pageTable);
	
	int vpn = 0;
	for(; vpn < VMEM_0_LIMIT/PAGESIZE; vpn++){
		if(pageTable[vpn].valid == 1){
			TracePrintf(1, "FreePageTable - Freeing physical page %d found in vpn %d\n", pageTable[vpn].pfn, vpn);
			freePhysicalPage(pageTable[vpn].pfn);
		}
	}
	
	// Find the page base and which half is used
	void* pageBase = (void*)DOWN_TO_PAGE(pageTable);
	int isBottomOfPage = pageTable == pageBase;
	
	// Find entry corresponding to page base
	struct pageTableRecord* current = getFirstPageTableRecord();
	struct pageTableRecord* previous = NULL;
	while (current != NULL) {
		if (current->pageBase == pageBase) {
			if (isBottomOfPage) current->isBottomFull = 0;
			else current->isTopFull = 0;
			// If the page is now completely empty we can free the entire page
			if (current->isBottomFull == 0 && current->isTopFull == 0 && current->next == NULL) {
				freePhysicalPage((long)pageBase / PAGESIZE);
				free(current);
				previous->next = NULL; // The first page table record is the only case where
				// previous == NULL, but the first page table record has the idle page table,
				// which is never freed.
			}
			return;
		}
		previous = current;
		current = current->next;
	}
	printf("Page table %p trying to be freed is not in the page record list. Halting...\n", pageTable);
	Halt();   
}
