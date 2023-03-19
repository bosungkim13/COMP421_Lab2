#include "processControlBlock.h"
#include "pageTableManagement.h"
#include "processScheduling.h"


struct processControlBlock*
createNewProcess(int pid, int parentPid){
    struct processControlBlock *pcb = malloc(sizeof(struct processControlBlock));
    pcb -> pid = pid;
    pcb -> pageTable = createPageTable();
    pcb -> delay = 0;
    pcb -> parentPid = parentPid;
    pcb -> outOfMemory = 0;
    pcb -> isWaiting = 0;
    pcb -> numChildren = 0;
    
    addToSchedule(pcb);
    if (pid == IDLE_PID){
        fillInitialPageTable(pcb->pageTable);
    }else{
        fillPageTable(pcb->pageTable);
    }
    
    return pcb;
}