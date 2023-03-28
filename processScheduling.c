#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "processScheduling.h"
#include "processControlBlock.h"
#include "trapHandlers.h"
#include "pageTableManagement.h"
#include "contextSwitch.h"

struct processControlBlock* idlePCB;
struct scheduleNode *head = NULL;
int nextPid = 2; // first process (not init or idle) will have pid of 2
int lastClockTickPID = -1;

void addToSchedule(struct processControlBlock *pcb){
    struct scheduleNode *newNode = malloc(sizeof(struct scheduleNode));
    newNode->next = head;
    newNode->pcb = pcb;
    head = newNode;
}

struct scheduleNode* getHead(){
    return head;
}
void setIdlePCB(struct processControlBlock* pcb){
	idlePCB = pcb;
}

int getCurrentPid(){
  struct scheduleNode *node = getHead();
  struct processControlBlock *pcb = node->pcb;
  return pcb->pid;
}

int setAndCheckClockTickPID(){
	int result = (lastClockTickPID == head->pcb->pid);
	lastClockTickPID = head->pcb->pid;
	return result;
}

void decreaseDelay(){
	TracePrintf(1, "Decreasing delay for all processes\n");
	struct scheduleNode* current = head;
	while(current != NULL){
		if (current->pcb->delay > 0){
			current->pcb->delay--;
		}
		current = current->next;
	}
}

int isThisProcessBlocked(struct processControlBlock* pcb){
	return pcb->delay > 0 || pcb->isWaiting || pcb->isReading || pcb->isWriting || pcb->isWaitReading || pcb->isWaitWriting;
}
void keepOrIdleProcess(struct processControlBlock* currentPCB){
	// This may switch from idle to idle, which is pointless, if this gets called while the current process
	// is idle and all user processes are currently blocked. I don't care and don't feel like checking.
	if(isThisProcessBlocked(currentPCB)) switchToExistingProcess(currentPCB, idlePCB);
}

void scheduleProcess(int isExit){
	TracePrintf(1, "processScheduling: Scheduling processes...\n");	
	// if we are scheduling during a NON exit scenario
	if (isExit == 0){
		if (head->next == NULL) {
			// There are no other processes.
			// Keep the current process if it's not blocked, switch to idle if it is.
			keepOrIdleProcess(head->pcb);
			return;
		}
		
		// There are other user processes.
		struct scheduleNode* currentNode = head->next;
		// Determine the next non-blocked process
		while (currentNode != NULL){
			if (!isThisProcessBlocked(currentNode->pcb)) break;
			currentNode = currentNode->next;
		}
		if (currentNode == NULL){
			// All other processes are blocked.
			keepOrIdleProcess(head->pcb);
			return;
		}
		
		// Now currentNode->pcb points to a non-blocked process that is not the current process.
		// Keep moving the node at the head to the tail until currentNode == head
		struct scheduleNode* executingNode = head;
		struct scheduleNode* tail = head;
		while(tail->next != NULL) tail = tail->next;
		// Rotate
		while(head != currentNode){
			struct scheduleNode* temp = head;
			head = head->next;
			tail->next = temp;
			tail = tail->next;
			tail->next = NULL;
		}
		switchToExistingProcess(executingNode->pcb, head->pcb);
		return;
		
		/*
		if (currPCB->pid != 1){
			// move the head node to the tail end
			if (head->next != NULL && head != NULL){
				struct scheduleNode *newHead = head->next;
				while (currNode->next != NULL){
					currNode = currNode->next;
				}
				head->next = NULL;
				currNode->next = head;
				head = newHead;
			}
			chooseNextProcess();
			currNode = head;
			nextPCB = currNode->pcb;
			ContextSwitch(mySwitchFunc, &currPCB->savedContext, (void *)currPCB, (void *)nextPCB);
		}*/
	} else{
		struct scheduleNode* currentNode = getHead();
		struct processControlBlock* currPCB = currentNode->pcb;
		head = currentNode->next;
		
		chooseNextProcess();
		struct scheduleNode *nextHead = getHead();
		struct processControlBlock* nextPCB = nextHead->pcb;
		
		switchToExistingProcess(currPCB, nextPCB);
		//ContextSwitch(mySwitchFunc, &nextPCB->savedContext, (void *)currPCB, (void *)nextPCB);
	}
}


void chooseNextProcess(){
  TracePrintf(1, "processScheduling: Beginning select_next_process.\n");
  if (nextProcessToHead(0)) {
    return;
  } else if (nextProcessToHead(IDLE_DELAY)) {
    return;
  }
  // all processes have delays  
  Halt(); 
}

int nextProcessToHead(int delayMatch){
  struct scheduleNode *currNode = head;
  struct scheduleNode *prevNode = NULL;
  TracePrintf(2, "processScheduling: Moving next process to head.\n");
  while(currNode != NULL) {
    struct processControlBlock *pcb = currNode->pcb;
    if(pcb->delay == delayMatch && pcb->isWaiting == 0 && 
        pcb->isWaitReading == -1 && pcb->isWaitWriting == -1 && pcb->isWriting == -1){
      if(prevNode == NULL){
        return 1;
      } else {
        prevNode->next = currNode->next;
        currNode->next = head;
        head = currNode;
        return 1;
      }
    } else {
      prevNode = currNode;
      currNode = currNode->next;
    }
  }

  return 0;  
}

void removeExitingProcess(){
  struct scheduleNode *currNode = getHead();

  if (currNode == NULL) {
    TracePrintf(1, "processScheduling: Trying to remove an \"exiting\" process when there are no processes.\n");
    Halt();    
  }
  struct processControlBlock *currPCB = currNode->pcb;
  if (currPCB->pid == IDLE_PID) {
    TracePrintf(1, "processScheduling: Trying to exit with the idle process.\n");
    Halt();
  }
  scheduleProcess(1);
  TracePrintf(1, "processScheduling: with scheduleNode: %p\n", currNode);
  TracePrintf(1, "processScheduling: with PCB: %p\n", currPCB);
  TracePrintf(1, "processScheduling: with page table: %p\n", currPCB->pageTable);
  
  // remove the head off the list of processes
  currNode = getHead();
  head = currNode->next;
  freePageTable(currNode->pcb->pageTable);
  free(currNode->pcb);
  free(currNode);

  TracePrintf(2, "processScheduling: Completed removal of exiting process.\n");

}

int updateAndGetNextPid(){
  return nextPid++;
}

void removeHead(){
  TracePrintf(2, "processScheduling: Begin to remove head of scheduled processes.\n");
  struct scheduleNode *currHead = getHead();
  head = currHead->next;
  freePageTable(currHead->pcb->pageTable);
  free(currHead->pcb);
  free(currHead);
  TracePrintf(2, "processScheduling: End removal to head of scheduled processes.\n");
}

