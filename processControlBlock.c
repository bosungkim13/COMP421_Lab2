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
    pcb -> isWaiting = 0;
    pcb -> numChildren = 0;
    pcb -> isReading = 0;
    pcb -> isWriting = 0;
    pcb -> isWaitReading = 0;
    pcb -> isWaitWriting = 0;
    
    if (pid == IDLE_PID){
        fillInitialPageTable(pcb->pageTable);
    }else{
    	addToSchedule(pcb);
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

void appendChildExitNode(struct processControlBlock* parentPCB, int pid, int exitType){
    // malloc for new node and get the current head of exit queue
    struct exitNode *newExit = malloc(sizeof(struct exitNode));
    struct exitNode *currExit = parentPCB->exitQ;

    // set values in new exit node
    newExit->pid = pid;
    newExit->exitType = exitType;
    newExit->next = NULL;

    // add new exit node to the PCB starting a new list if needed
    if (currExit != NULL) {
	while (currExit->next != NULL) {
		currExit = currExit->next;
	}
        currExit->next = newExit;     
    } else {
        parentPCB->exitQ = newExit;
    }
}

struct exitNode *popChildExitNode(struct processControlBlock *pcb) {
    struct exitNode *head = pcb->exitQ;
    if (head == NULL) {
        return NULL;
    } else {
        pcb->exitQ = head->next;
        pcb->numChildren--;
        return head;
    }
}
