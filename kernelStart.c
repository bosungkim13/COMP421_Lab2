#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>
#include "memoryManagement.h"
#include "trapHandlers.h"

void **interruptVectorTable;
int isInit = 1;

void KernelStart(ExceptionStackFrame *frame, unsigned int pnem_size, void *orig_brk, char **cmd_args){
    TracePrintf(1, "kernelStart - start of KernelStart to create %d physical pages.\n", pmem_size/PAGESIZE);

    // Initialize linked list to keep track of free pages
    initPhysicalPageArray(pmem_size);

    TracePrintf(2, "kernelStart - Free physical page structure initialized with %d free physical pages.\n", freePhysicalPageCount());

    // Mark the kernel stack
    markPagesInRange((void *)KERNEL_STACKBASE, (void *)KERNEL_STACK_LIMIT);

    // Create the interrupt vector table
    interrupVectorTable = malloc(sizeof(void *) * TRAP_VECTOR_SIZE);
    int i;
    for (i = 0; i < TRAP_VECTOR_SIZE; i++){
        switch(i){
            case TRAP_KERNEL:
                interruptVectorTable[i] = kernelTrapHandler;
                break;
            case TRAP_CLOCK:
                interruptVectorTable[i] = clockTrapHandler;
                break;
            case TRAP_ILLEGAL:
                interruptVectorTable[i] = illegalTrapHandler;
                break;
            case TRAP_MEMORY:
                interruptVectorTable[i] = memoryTrapHandler;
                break;
            case TRAP_MATH:
                interruptVectorTable[i] = mathTrapHandler;
                break;
            case TRAP_TTY_RECEIVE:
                interruptVectorTable[i] = ttyReceiveTrapHandler;
                break;
            case TRAP_TTY_TRANSMIT:
                interruptVectorTable[i] = ttyTransmitTrapHandler;
                break;
            default:
                interruptVectorTable[i] = NULL;
        }
    }
    // Point priveleged register to the trap interrupt table
    WriteRegister(REG_VECTOR_BASE, (RCS421RegValue) interruptVectorTable);
    TracePrintf(2, "kernelStart: REG_VECTOR_BASE has been set to interrupt vector table");

    // Initialize the kernel page table
    initKernelPT();




}
