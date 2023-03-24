#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include "processScheduling.h"
#include "processControlBlock.h"
#include "trapHandlers.h"
#include "pageTableManagement.h"

struct scheduleNode *head = NULL;

void
addToSchedule(struct processControlBlock *pcb){
    struct scheduleNode *newNode = malloc(sizeof(struct scheduleNode));
    newNode->next = head;
    newNode->pcb = pcb;
    head = newNode;
}

struct scheduleNode* getHead(){
    return head;
}

int getCurrentPid(){
  struct scheduleNode *node = getHead();
  struct processControlBlock *pcb = node->pcb;
  return pcb->pid;
}

void decreaseDelay(){
  TracePrintf(1, "decreasing delay for all processes");
  struct scheduleNode *curr = head;
  while(current != NULL){
    struct processControlBlock *pcb = curr->pcb;
    if (pcb->delay > 0){
      pcb->delay--;
    }
    curr = curr->next;
  }
}

void scheduleProcess(){
  TracePrintf(1, "processScheduling: Scheduling processes...\n");
  struct scheduleNode *currNode = head->next;
  struct processControlBlock *currPCB = head->pcb;

  // determine if there are other processes we can switch to
  int flag;
  while (currNode != NULL){
    currPCB = currNode->pcb;
    if (currPCB->delay == 0){
      flag = 1;
    }
    currNode = currNode->next;
  }
  if (flag == NULL){
    flag = 0;
  }
  currNode = head;

  if (flag || currPCB->pid != 1){
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
    struct processControlBlock *nextPCB = currNode->pcb;
    ContextSwitch(mySwitchFunc, &currPCB->savedContext, (void *)currPCB, (void *)nextPCB);
    resetSwitchTime();
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

