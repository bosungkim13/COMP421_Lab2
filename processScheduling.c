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