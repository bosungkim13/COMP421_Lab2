#include "processScheduling.h"
#include "processControlBlock.h"
#include "memoryManagement.h"
#include "pageTableManagement.h"

#include "contextSwitch.h"

SavedContext* switchProcessFunction(SavedContext* context, void* p1, void* p2){
	struct processControlBlock* sourcePCB = (struct processControlBlock*)p1;
	struct processControlBlock* destPCB = (struct processControlBlock*)p2;
	switchReg0To(destPCB->pageTable);
	TracePrintf(1, "ContextSwitch - SwitchToExistingProcess - Switched from process id %d to process id %d\n", sourcePCB->pid, destPCB->pid);
	return &destPCB->savedContext;
}

void switchToExistingProcess(struct processControlBlock* currentPCB,
				struct processControlBlock* targetPCB){
	ContextSwitch(switchProcessFunction, &currentPCB->savedContext,
	    (void*)currentPCB, (void*)targetPCB);
}

/*
SavedContext* mySwitchFunc(SavedContext *ctxp, void *p1, void *p2){
    struct processControlBlock *pcbTo = (struct processControlBlock *)p2;
    struct processControlBlock *pcbFrom = (struct processControlBlock *)p1;

    // set the special register to page table of process 2 and flush the TLB for region 0
    WriteRegister(REG_PTR0, (RCS421RegVal) virtualToPhysicalAddr(pcbTo->pageTable));
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)TLB_FLUSH_0);
    return &pcbTo->savedContext;
}*/

/**
 * First copy process 1's kernel stack over to process 2. Then, switches context from process 1 to process 2.
 */
SavedContext* idleInitFunc(SavedContext *ctxp, void* p1, void* p2){
    int i = 0;
    int j = 0;

    struct processControlBlock *pcbFrom = (struct processControlBlock *)p1;
    struct processControlBlock *pcbTo = (struct processControlBlock *)p2;

    // copy the kernel stack of pcb1->page_table to pcb2->page_table
    struct pte *pageTable1 = pcbFrom->pageTable;
    struct pte *pageTable2 = pcbTo->pageTable;

    TracePrintf(3, "context_switch: beginning kernel stack copy.\n");

    /*for (i = 0; i < KERNEL_STACK_PAGES; i++) {
        unsigned int process2PPN = getFreePhysicalPage();

        // CHANGE TO WHILE LOOP
        for (j = MEM_INVALID_PAGES; j < KERNEL_STACK_BASE/PAGESIZE; j++) {
        // find the first available page from the user's heap.
            if (pageTable1[j].valid == 0) {
                // temporarily map that page to a physical page.
                pageTable1[j].valid = 1;
                pageTable1[j].kprot = PROT_READ | PROT_WRITE;
                pageTable1[j].uprot = PROT_READ | PROT_EXEC;
                pageTable1[j].pfn = process2PPN;

                void *process1VirtualAddr = (void *)(long)(((KERNEL_STACK_BASE/PAGESIZE + i) * PAGESIZE) + VMEM_0_BASE);

                // temporary address for the kernel stack
                void *tempVirtualAddr = (void *)(long)((j * PAGESIZE) + VMEM_0_BASE);

                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) tempVirtualAddr);

                // copy kernel stack page into our new page of memory.
                memcpy(tempVirtualAddr, process1VirtualAddr, PAGESIZE);

                // pretend that the temp memory never existed.
                pageTable1[j].valid = 0;
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) tempVirtualAddr);

                // give the pfn from the temp memory to process 2's page table.
                pageTable2[i + KERNEL_STACK_BASE/PAGESIZE].pfn = process2PPN;
                break;
            }
        }
    }*/
    copyKernelStack(pcbFrom->pageTable, pcbTo->pageTable);

    /*WriteRegister(REG_PTR0, (RCS421RegVal)virtualToPhysicalAddr(pageTable2));
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);*/
    switchReg0To(pcbTo->pageTable);

    TracePrintf(1, "context_switch: idle_and_init_initialization completed.\n");

    return &pcbFrom->savedContext;  
}

// Assumes REG_PTR0 points to the physical address that is the base of virtPTFrom
// and returns with REG_PTR0 pointed to that as well
void copyKernelStack(struct pte* virtPTFrom, struct pte* virtPTTo){
	void* stackSwapSpace = getStackSwapSpace();
	
	TracePrintf(2, "contextSwitch - copyKernelStack - Copying kernelStack from source pt at %p to temp at %p\n", virtPTFrom, stackSwapSpace);
	memcpy(stackSwapSpace, (void*)KERNEL_STACK_BASE, KERNEL_STACK_SIZE);
	switchReg0To(virtPTTo);
	
	TracePrintf(2, "contextSwitch - copyKernelStack - Giving new process free physical pages\n");
	int i = 0;
	for(; i < KERNEL_STACK_PAGES; i++){
		virtPTTo[KERNEL_STACK_BASE/PAGESIZE + i].valid = 1;
		virtPTTo[KERNEL_STACK_BASE/PAGESIZE + i].pfn = getFreePhysicalPage();
	}
	
	TracePrintf(2, "contextSwitch - copyKernelStack - Copying kernelStack from temp at %p to dest pt at %p\n", stackSwapSpace, virtPTTo);
	memcpy((void*)KERNEL_STACK_BASE, stackSwapSpace, KERNEL_STACK_SIZE);
	switchReg0To(virtPTFrom);
	
	TracePrintf(2, "contextSwitch - copyKernelStack - Kernel stack successfully copied from pt %p to pt %p\n", virtPTFrom, virtPTTo);
}

void switchReg0To(void* destPTVirt){
	WriteRegister(REG_PTR0, (RCS421RegVal)virtualToPhysicalAddr(destPTVirt));
	WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
}

SavedContext* forkFunc(SavedContext *ctxp, void* p1, void* p2){
    TracePrintf(2, "contextSwitch: Starting forkFunc()\n");
    struct processControlBlock *parentPCB = (struct processControlBlock *)p1;
    struct processControlBlock *childPCB = (struct processControlBlock *)p2;

    int i = 0;
    int firstInvalidRegion0 = -1;

    // get page table entries
    struct pte *parentPT = parentPCB->pageTable;
    struct pte *childPT = childPCB->pageTable;

    // find the number of pages to copy
    int numPagesCopy = 0;
    for(i = 0; i < (VMEM_0_LIMIT - VMEM_0_BASE)/PAGESIZE; i++){
        if(parentPT[i].valid == 1){
            numPagesCopy++;
        }
    }

    TracePrintf(2, "contextSwitch: Copying %d pages\n", numPagesCopy);

    //Find first invalid parent region 0 page to use as our temp location for the page by page copy
    for (i = MEM_INVALID_PAGES; i < (parentPCB->userStackLimit - (void *)PAGESIZE)/PAGESIZE; i++) {
        if (parentPT[i].valid == 0) {
            firstInvalidRegion0 = i;
            TracePrintf(2, "contextSwitch: In forkFunc, first invalid region 0 page: %d\n", firstInvalidRegion0);
            break;
        }
    }

    TracePrintf(2, "contextSwitch: Starting region 0 copy.\n");

    //if we don't have enough physical memory to make the copy, return with parent saved context
    if(numPagesCopy > freePhysicalPageCount()){
        parentPCB->noMemory = 1;
        TracePrintf(2, "contextSwitch: # of user pages: %d , # of free physical pages: %d\n", (VMEM_0_LIMIT - VMEM_0_BASE)/PAGESIZE, freePhysicalPageCount());
        return &parentPCB->savedContext;
    }
    // space in region 0 to copy using temp address
    if (firstInvalidRegion0 != -1){
        TracePrintf(2, "contextSwitch: using the user stack for temp virtual page\n");

        for (i = MEM_INVALID_PAGES; i < (VMEM_0_LIMIT - VMEM_0_BASE)/PAGESIZE; i++) {
            if (parentPT[i].valid == 1) {
                TracePrintf(2, "contextSwitch: copying page: %d\n", i);

                unsigned int childPPN = getFreePhysicalPage();

                // temporarily map that page to a physical page
                parentPT[firstInvalidRegion0].valid = 1;
                parentPT[firstInvalidRegion0].kprot = PROT_READ | PROT_WRITE;
                parentPT[firstInvalidRegion0].uprot = PROT_READ | PROT_EXEC;
                parentPT[firstInvalidRegion0].pfn = childPPN;

                void *parentVirtual = (void *)(long)((i * PAGESIZE) + VMEM_0_BASE);
                void *tempVirtualRegion0 = (void *)(long)((firstInvalidRegion0 * PAGESIZE) + VMEM_0_BASE);

                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) tempVirtualRegion0);
                TracePrintf(2, "contextSwitch: Finished filling out pte in parent PT to be used for copy");
                TracePrintf(2, "contextSwitch: Memcopy, src: %p, dest: %p\n", parentVirtual, tempVirtualRegion0);

                // copy region 0 page into new page of memory.
                memcpy(
                    tempVirtualRegion0, // destination
                    parentVirtual, // source
                    PAGESIZE
                );

                TracePrintf(2, "contextSwitch: Setting parent PTE used to invalid...\n");
                // temp memory never existed.
                parentPT[firstInvalidRegion0].valid = 0;
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) tempVirtualRegion0);

                // set pfn in child's PT
                childPT[i].valid = 1;
                childPT[i].pfn = childPPN;
            }
        }
    } else {
        //We must use region 1 to copy
        int firstInvalidRegion1 = -1;

        for (i = 0; i < PAGE_TABLE_LEN; i++) {
            if (kernelPageTable[i].valid == 0) {
                firstInvalidRegion1 = i;
                parentPCB->noMemory = 0;
                break;
            }
        }

        if (firstInvalidRegion1 == -1) {
            parentPCB->noMemory = 1;
            return &parentPCB->savedContext;
        } else {
            // use the page to copy over the entire region_1.
            for (i = MEM_INVALID_PAGES; i < (VMEM_0_LIMIT - VMEM_0_BASE)/PAGESIZE; i++) {
                if (parentPT[i].valid == 1) {
                unsigned int childPPN = getFreePhysicalPage();

                // temporarily map to a physical page.
                kernelPageTable[firstInvalidRegion1].valid = 1;
                kernelPageTable[firstInvalidRegion1].pfn = childPPN;

                void *parentVirtual = (void *)(long)((i * PAGESIZE) + VMEM_0_BASE);
                void *tempVirtualRegion1 = (void *)(long)((firstInvalidRegion1 * PAGESIZE) + VMEM_1_BASE);
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) tempVirtualRegion1);

                // copy region 0 page into our new page.
                memcpy(
                    tempVirtualRegion1, // destination
                    parentVirtual, // source
                    PAGESIZE
                );

                // pretend temp memory doesn't exist
                parentPT[firstInvalidRegion1].valid = 0;
                WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) tempVirtualRegion1);

                // set child's page table.
                childPT[i].valid = 1;
                childPT[i].pfn = childPPN;
                }
            }
        }
    }

    TracePrintf(2, "contextSwitch: VALID PTEs:\n");
    for (i = 0; i < VMEM_0_LIMIT/PAGESIZE; i++) {
        if(childPT[i].valid == 1){
            TracePrintf(3, "Child: \n");
            TracePrintf(3, "contextSwitch: %d, %d \n", i, childPT[i].valid);
            TracePrintf(3, "Parent: \n");
            TracePrintf(3, "contextSwitch: %d, %d \n", i, parentPT[i].valid);
        }
    }
        
 
    WriteRegister(REG_PTR0, (RCS421RegVal)virtualToPhysicalAddr(childPT));
    TracePrintf(2, "Breaking on flush?\n");
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    // child pcb's saved_context should be the exact same as parent
    memcpy(
        &childPCB->savedContext, // destination
        &parentPCB->savedContext, // source
        sizeof(SavedContext)
    );

    TracePrintf(1, "contextSwitch: forkFunc() completed.\n");

    return &childPCB->savedContext;      
}

