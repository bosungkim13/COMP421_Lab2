#include "processControlBlock.h"
#include "memoryManagement.h"
#include "pageTableManagement.h"
#include "contextSwitch.h"

SavedContext* mySwitchFunc(SavedContext *ctxp, void *p1, void *p2){
    struct processControlBlock *pcbTo = (struct processControlBlock *)p2;
    struct processControlBlock *pcbFrom = (struct processControlBlock *)p1;

    // set the special register to page table of process 2 and flush the TLB for region 0
    WriteRegister(REG_PTR0, (RCS421RegVal) virtualToPhysicalAddr(pcbTo->pageTable));
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal)TLB_FLUSH_0);
    return &pcbTo->savedContext;
}

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

    for (i = 0; i < KERNEL_STACK_PAGES; i++) {
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
    }

    WriteRegister(REG_PTR0, (RCS421RegVal)virtualToPhysicalAddr(pageTable2));
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    TracePrintf(1, "context_switch: idle_and_init_initialization completed.\n");

    return &pcbFrom->savedContext;  
}
