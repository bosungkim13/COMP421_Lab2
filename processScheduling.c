#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "processScheduling.h"
#include "processControlBlock.h"
#include "trapHandlers.h"
#include "pageTableManagement.h"
#include "contextSwitch.h"
#include <stdio.h>

struct processControlBlock* idlePCB;
struct scheduleNode *idleNode = NULL;
struct scheduleNode *head = NULL;
int nextPid = 2; // first process (not init or idle) will have pid of 2
int lastClockTickPID = -1;
int isIdleRunning = 0; // 0 if not running now, 1 if running now

void addToSchedule(struct processControlBlock *pcb){
    struct scheduleNode *newNode = malloc(sizeof(struct scheduleNode));
    newNode->next = head;
    newNode->pcb = pcb;
    head = newNode;
}

void setIdlePCB(struct processControlBlock* pcb){
	idlePCB = pcb;
	idleNode = malloc(sizeof(struct scheduleNode));
	idleNode->next = NULL;
	idleNode->pcb = idlePCB;
}

struct scheduleNode* getHead(){
	return head;
}

struct scheduleNode* getRunningNode(){
	return isIdleRunning ? idleNode : head;
}

int getCurrentPid(){
	return isIdleRunning ? idlePCB->pid : head->pcb->pid;
}

int setAndCheckClockTickPID(){
	int pidRunningNow = isIdleRunning == 1 ? idlePCB->pid : head->pcb->pid;
	int result = (lastClockTickPID == pidRunningNow);
	TracePrintf(1, "SetAndCheckClockTickPID: Previous tick had pid %d, now setting to pid %d, was%s the same\n",
	    lastClockTickPID, pidRunningNow, result == 1 ? "" : " not");
	lastClockTickPID = pidRunningNow;
	return result;
}

// Returns 1 if some process had their delay decreased to 0 so could run now
int decreaseDelay(){
	int wasZeroed = 0;
	TracePrintf(1, "DecreasingDelay - Starting\n");
	struct scheduleNode* current = head;
	while(current != NULL){
		if (current->pcb->delay > 0){
			TracePrintf(1, "DecreaseDelay - Process id %d had delay %d, getting decreased to %d\n",
			    current->pcb->pid, current->pcb->delay, current->pcb->delay - 1);
			current->pcb->delay--;
			if(current->pcb->delay == 0) wasZeroed = 1;
		}
		current = current->next;
	}
	return wasZeroed;
}

int isThisProcessBlocked(struct processControlBlock* pcb){
	return pcb->delay > 0 || pcb->isWaiting || pcb->isReading || pcb->isWriting || pcb->isWaitReading || pcb->isWaitWriting;
}
void keepOrIdleProcess(struct processControlBlock* currentPCB){
	if(isIdleRunning == 0){
		// Idle is not running now. Switch to it if we need it.
		if(isThisProcessBlocked(currentPCB)){
			TracePrintf(1, "KeepOrIdleProcess: Switching to idle.\n");
			isIdleRunning = 1;
			switchToExistingProcess(currentPCB, idlePCB);
		}
	}else{
		// Idle is running now. Switch away if we can.
		if(currentPCB == idlePCB) return;
		if(!isThisProcessBlocked(currentPCB)){
			TracePrintf(1, "KeepOrIdleProcess: Switching away from idle.\n");
			isIdleRunning = 0;
			switchToExistingProcess(idlePCB, currentPCB);
		}
	}
}

void scheduleProcess(int isExit){
	TracePrintf(1, "processScheduling: Currently running as process %d, scheduling processes...\n", getCurrentPid());
	// if we are scheduling during a NON exit scenario
	if (isExit == 0){
		if(isIdleRunning == 1 && !isThisProcessBlocked(head->pcb)){
			// Happens when there are many processes to run, but they are all blocked,
			// and then the one at the head is unblocked and clock ticks.
			// Without this, the process at the head won't get switched to until another process gets
			// unblocked and switched to first.
			isIdleRunning = 0;
			switchToExistingProcess(idlePCB, head->pcb);
			return;
		}
		if (head->next == NULL) {
			// There are no other processes.
			// Keep the current process if it's not blocked, switch to idle if it is.
			TracePrintf(1, "ScheduleProcess - No other processes, switching to idle if current is blocked\n");
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
			TracePrintf(1, "ScheduleProcess - All other processes blocked, switching to idle if current is blocked\n");
			keepOrIdleProcess(getRunningNode()->pcb);
			return;
		}
		TracePrintf(1, "ScheduleProcess - First non-blocked process has id %d, switching to it\n", currentNode->pcb->pid);
		
		// Now currentNode->pcb points to a non-blocked process that is not the current process.
		// Keep moving the node at the head to the tail until currentNode == head
		struct scheduleNode* executingNode = getRunningNode();
		struct scheduleNode* tail = head;
		while(tail->next != NULL) tail = tail->next;
		// Rotate
		while(head != currentNode){
			TracePrintf(1, "ScheduleProcess - Rotating list: target pid %d, head pid %d, head->next pid %d, tail pid %d\n",
			    currentNode->pcb->pid, head->pcb->pid, head->next->pcb->pid, tail->pcb->pid);
			tail->next = head;
			tail = tail->next; // Now the process at 'head' is at 'tail'
			head = head->next;
			tail->next = NULL; // Now the 'head' and 'tail' pointers have each moved one
		}
		TracePrintf(1, "ScheduleProcess - After list rotating: target pid %d, head pid %d, tail pid %d\n",
		    currentNode->pcb->pid, head->pcb->pid, tail->pcb->pid);
		isIdleRunning = 0;
		switchToExistingProcess(executingNode->pcb, head->pcb);
		return;
	} else{
		struct scheduleNode* exitingNode = head;
		head = head->next;
		// Switch to idle, then switch away as normal
		// This prevents special case handling for switching away from the exiting process
		// which isn't in the list
		isIdleRunning = 1;
		switchToExistingProcess(exitingNode->pcb, idlePCB);
		scheduleProcess(0);
	}
}

void removeExitingProcess(){
	struct scheduleNode* currentNode = head; // The head is the node running now. Idle is not stored in
	// the list, but idle calling Exit() was handled already.
	if(currentNode->next == NULL){
		printf("The final user process is exiting. Halting...\n");
		Halt();
	}
	scheduleProcess(1);
	TracePrintf(1, "processScheduling: with scheduleNode: %p\n", currentNode);
	TracePrintf(1, "processScheduling: with PCB: %p\n", currentNode->pcb);
	TracePrintf(1, "processScheduling: with page table: %p\n", currentNode->pcb->pageTable);
	
	while(currentNode->pcb->exitQ != NULL){
		struct exitNode* nowOrphanedExitedChild = currentNode->pcb->exitQ;
		currentNode->pcb->exitQ = nowOrphanedExitedChild->next;
		free(nowOrphanedExitedChild);
	}
	freePageTable(currentNode->pcb->pageTable);
	free(currentNode->pcb);
	free(currentNode);
	TracePrintf(2, "processScheduling: Completed removal of exiting process.\n");
}

int updateAndGetNextPid(){
  return nextPid++;
}
/*
void removeHead(){
  TracePrintf(2, "processScheduling: Begin to remove head of scheduled processes.\n");
  struct scheduleNode *currHead = getHead();
  head = currHead->next;
  freePageTable(currHead->pcb->pageTable);
  free(currHead->pcb);
  free(currHead);
  TracePrintf(2, "processScheduling: End removal to head of scheduled processes.\n");
}
*/
