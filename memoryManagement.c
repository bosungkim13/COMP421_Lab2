#include <stdio.h>
#include "pageTableManagement.h"
#include "processControlBlock.h"
#include "processScheduling.h"

#include "memoryManagement.h"

// Initialize integer array to keep track of page status (0 free, 1 used)
int *isPhysicalPageOccupied = NULL;
int numPhysicalPages;
void *kernelBrk = (void *)VMEM_1_BASE;
int isVMInitialized = 0;
void* pageSwapSpace;

void initKernelBrk(void *origBrk){
	kernelBrk = origBrk;
}
void* getKernelBrk(){
	return kernelBrk;
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
    printf("GetFreePhysicalPage - Request for physical page failed because no pages are free. Halting...\n");
    Halt();
}

unsigned int getTopFreePhysicalPage(){
    int pfn = DOWN_TO_PAGE(VMEM_1_LIMIT - 1) / PAGESIZE;
    if(isPhysicalPageOccupied[pfn] == 1){
    	printf("GetTopFreePhysicalPage - Request for top physical page while top page is already in use! Halting...\n");
    	Halt();
    }
    isPhysicalPageOccupied[pfn] = 1;
    return pfn;
}

void freePhysicalPage(unsigned int pfn){
	isPhysicalPageOccupied[pfn] = 0;
}

int SetKernelBrk(void *addr) {
    int i;
    if (isVMInitialized) {
    	// TODO - Calling should make kernelBrk = addr, not UP_TO_PAGE(addr). Note kernelBrk is not necessarily page boundary aligned
    	TracePrintf(2, "SetKernelBrk After VM: Requesting address %p, kernelBrk currently at %p\n", addr, kernelBrk);
        int numNeedPages = ((long)UP_TO_PAGE(addr) - (long)kernelBrk)/PAGESIZE;
        if(freePhysicalPageCount() < numNeedPages){
            return -1;
        } else {
            for (i = 0; i < numNeedPages; i++){
                unsigned int physicalPageNum = getFreePhysicalPage();
                int vpn = ((long)kernelBrk - VMEM_1_BASE)/PAGESIZE + i;
                if(kernelPageTable[vpn].valid == 1){
                	printf("Kernel tried malloc-ing memory but ran into the descending list of page tables. Halting...\n");
                	Halt();
                }

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

void setupPageSwapSpace(){
	pageSwapSpace = malloc(PAGESIZE);
}
void* getPageSwapSpace(){
	return pageSwapSpace;
}

// return 1 on success 0 on failure
int growUserStack(ExceptionInfo *info, struct scheduleNode *head){
    void *addr = info->addr;
    if((DOWN_TO_PAGE(addr) < DOWN_TO_PAGE(head->pcb->userStackLimit)) && (UP_TO_PAGE(addr) > (UP_TO_PAGE(head->pcb->brk) + PAGESIZE))){
        struct processControlBlock *pcb = head->pcb;
        int neededPages = (DOWN_TO_PAGE(pcb->userStackLimit) - DOWN_TO_PAGE(addr)) / PAGESIZE;
        TracePrintf(2, "memoryManagement: Entering grow_user_stack with process %d, for addr %p and need %d pages \n", pcb->pid, addr, neededPages);
        int i;
        for(i = 0; i < neededPages; i++) {
            unsigned int ppn = getFreePhysicalPage();
            int vpn = (long)DOWN_TO_PAGE(pcb->userStackLimit) / PAGESIZE - i - 1;
            pcb->pageTable[vpn].valid = 1;
            pcb->pageTable[vpn].pfn = ppn;
        }
        pcb->userStackLimit = (void *)DOWN_TO_PAGE(addr);
        TracePrintf(2, "memoryManagement: Grew user stack limiit to %p \n", pcb->userStackLimit);
        return 1;
    } 
    else{
        return 0;
    }
}

void brkHandler(ExceptionInfo *info){ 
    void *addr = (void *)info->regs[1];
    int i;

    // invalid addr
    if(UP_TO_PAGE(addr) <= MEM_INVALID_SIZE){
        info->regs[0] = ERROR;
        return;
    }

    struct scheduleNode *item = getRunningNode();
    struct processControlBlock *pcb = item->pcb;
    void *brk = pcb->brk;
    void *userStackLimit = pcb->userStackLimit;
    struct pte *userPT = pcb->pageTable;

    // invalid addr
    if(UP_TO_PAGE(addr) >= DOWN_TO_PAGE(userStackLimit) -1){
        info->regs[0] = ERROR;
        return;
    }

    if(UP_TO_PAGE(addr) > UP_TO_PAGE(brk)){

        int numNeededPages = ((long)UP_TO_PAGE(addr) - (long)UP_TO_PAGE(brk))/PAGESIZE;
        // not enough physical memory
        if(freePhysicalPageCount() < numNeededPages){
            info->regs[0] = ERROR;
            return;
        } else {
        TracePrintf(3, "memoryManagement: Getting %d physical pages...\n", numNeededPages);
        for(i = 0; i < numNeededPages; i++) {
            unsigned int ppn = getFreePhysicalPage();
            int vpn = (long)UP_TO_PAGE(brk)/PAGESIZE + i;
            userPT[vpn].valid = 1;
            userPT[vpn].pfn = ppn;
        }
        }
    // we can free pages that we don't need anymore
    } else if(UP_TO_PAGE(addr) < UP_TO_PAGE(brk)){
        int numPagesFree = ((long)UP_TO_PAGE(brk) - (long)UP_TO_PAGE(addr))/PAGESIZE;
        TracePrintf(3, "memoryManagement: Freeing %d physical pages...\n", numPagesFree);
        for(i = 0; i < numPagesFree; i++){
            userPT[(long)UP_TO_PAGE(brk)/PAGESIZE - i].valid = 0;
            int ppn = userPT[(long)UP_TO_PAGE(brk)/PAGESIZE - i].pfn;
            freePhysicalPage(ppn);
        }
    }

    info->regs[0] = 0;
    pcb->brk = (void *)UP_TO_PAGE(addr);

    TracePrintf(3, "memoryManagement: brk_handler execution completed\n");
    }
