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
    pcb -> isReading = -1;
    pcb -> isWriting = -1;
    pcb -> isWaitReading = -1;
    pcb -> isWaitWriting = -1;
    
    addToSchedule(pcb);
    if (pid == IDLE_PID){
        fillInitialPageTable(pcb->pageTable);
    }else{
        fillPageTable(pcb->pageTable);
    }
    
    return pcb;
}

struct processControlBlock* getPCB(int pid){
    struct scheduleNode *currNode = getHead();

    // iterate and find node that matches target pid
    while (currNode != NULL){
        if (currNode->pcb->pid == pid){
            return currNode->pcb;
        }
        currNode = currNode->next;
    }
    return NULL;
}