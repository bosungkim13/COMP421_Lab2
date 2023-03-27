#include "trapHandlers.h"
#include <stdio.h>
#include "memoryManagement.h"

#define SCHEDULE_DELAY  2
int timeToSwitch = SCHEDULE_DELAY;

void kernelTrapHandler(ExceptionInfo *info) {
  TracePrintf(1, "trapHandlers: In TRAP_KERNEL interrupt handler...\n");

  int code = info->code;

  switch (code) {
    case YALNIX_FORK:
      TracePrintf(1, "trapHandlers: Fork requested.\n");
      forkTrapHandler(info);
      break;
    case YALNIX_EXEC:
      TracePrintf(1, "trapHandlers: Exec requested.\n");
      execTrapHandler(info);
      break;
    case YALNIX_EXIT:
      TracePrintf(1, "trapHandlers: Exit requested.\n");
      exitHandler(info, 0);
      break;
    case YALNIX_WAIT:
      TracePrintf(1, "trapHandlers: Wait requested.\n");
      waitTrapHandler(info);
      break;
    case YALNIX_GETPID:
      TracePrintf(1, "trapHandlers: GetPid requested.\n");
      getPidHandler(info);
      break;
    case YALNIX_BRK:
      TracePrintf(1, "trapHandlers: Brk requested.\n");
      brkHandler(info);
      break;
    case YALNIX_DELAY:
      TracePrintf(1, "trapHandlers: Delay requested.\n");
      delayHandler(info);
      break;
    case YALNIX_TTY_READ:
      TracePrintf(1, "trapHandlers: Tty Read requested.\n");
      ttyReadHandler(info);
      break;
    case YALNIX_TTY_WRITE:
      TracePrintf(1, "trapHandlers: Tty Write requested.\n");
      ttyWriteHandler(info);
      break;
  }

}

void waitTrapHandler(ExceptionInfo *info){

}

void execTrapHandler(ExceptionInfo *info){
  char *filename = (char *)info->regs[1];
  char **argvec = (char **)info->regs[2];

  struct scheduleNode *node = getHead();

  int loadReturn = LoadProgram(filename, argvec, info, node->pcb);
  if (loadReturn == -1){
    info->regs[0] = ERROR;
  }
}

void forkTrapHandler(ExceptionInfo *info){
  struct scheduleNode *currNode = getHead();
  struct processControlBlock *parentPCB = currNode->pcb;

  //create child process
  int childPid = nextPid();
  int parentPid = getCurrentPid();
  struct processControlBlock *childPCB = createNewProcess(childPid, parentPid);

  // call contezt switch that copies region 0
  ContextSwitch(forkFunc, &parentPCB->savedContext, (void *)parentPCB, (void *)childPCB);

  if (parentPCB->noMemory) {
  // if the parent pcb is out of memory then pcb at the head is the child but hte page table and contezt are the parents
  // in this case, we need to remove the head from the schedule
    TracePrintf(1, "trapHandlers: in fork handler but not enough memory for region 1 copy.\n");
    struct scheduleNode *currNode = getHead();
    // TODO remove the head of the schedule but we should create a function since we cannot modify from this file.
    removeHead();
    info->regs[0] = ERROR;
  } else {
    // otherwise, return childs pid or return 0 if you are the child
    if(getCurrentPid() == childPid){
      info->regs[0] = 0;
    } else {
      info->regs[0] = child_pid;
      parentPCB->numChildren++;
    }
  }
}

void clockTrapHandler (ExceptionInfo *info) {
  TracePrintf(1, "trapHandlers: begin clockTraphandler with PID: %d\n", getCurrentPid());
  //TESTING
  timeToSwitch--;
  decrementDelay();
  if(timeToSwitch == 0) {
    TracePrintf(1, "trapHandlers: time to switch... scheduling processes now\n");
    // reset the time until we switch next
    timeToSwitch = SCHEDULE_DELAY;
    scheduleProcess();
  }
}

void illegalTrapHandler (ExceptionInfo *info) {

  TracePrintf(1, "trapHandlers: Illegal trap handler \n");
  if (info -> code == TRAP_ILLEGAL_ILLOPC) {
      TracePrintf(1, "Illegal Opcode \n");
  }

  else if (info -> code == TRAP_ILLEGAL_ILLOPN) {
      TracePrintf(1, "Illegal Operand \n");
  }

  else if (info -> code == TRAP_ILLEGAL_ILLADR) {
      TracePrintf(1, "Illegal address mode \n");
  }

  else if (info -> code == TRAP_ILLEGAL_ILLTRP) {
      TracePrintf(1, "Illegal software trap \n");
  }

  else if (info -> code == TRAP_ILLEGAL_PRVOPC) {
      TracePrintf(1, "Privileged opcode \n");
  }

  else if (info -> code == TRAP_ILLEGAL_PRVREG) {
      TracePrintf(1, "Privileged register \n");
  }

  else if (info -> code == TRAP_ILLEGAL_COPROC) {
      TracePrintf(1, "Coprocessor error \n");
  }

  else if (info -> code == TRAP_ILLEGAL_BADSTK) {
      TracePrintf(1, "Bad stack \n");
  }

  else if (info -> code == TRAP_ILLEGAL_KERNELI) {
      TracePrintf(1, "Linux kernel sent SIGILL \n");
  }

  else if (info -> code == TRAP_ILLEGAL_USERIB) {
      TracePrintf(1, "Received SIGILL or SIGBUS from user \n");
  }

  else if (info -> code == TRAP_ILLEGAL_ADRALN) {
      TracePrintf(1, "Invalid address alignment \n");
  }

  else if (info -> code == TRAP_ILLEGAL_ADRERR) {
      TracePrintf(1, "Non-existent physical address \n");
  }

  else if (info -> code == TRAP_ILLEGAL_OBJERR) {
      TracePrintf(1, "Object-specific HW error \n");
  }

  else if (info -> code == TRAP_ILLEGAL_KERNELB) {
      TracePrintf("Linux kernel sent SIGBUS \n");
  }
  else { 
    return; 
  }
  exitHandler(info, info->code);
  return;
}

void memoryTrapHandler (ExceptionInfo *info) {
  TracePrintf(1, "trapHandler: Memory handler \n");
  if (info -> code == TRAP_MEMORY_MAPERR) {
    TracePrintf(1, "No mapping at address... trying to grow user stack...");
    struct scheduleNode *head = getHead();
    void *addr = info->addr;
    // changed to
    if (growUserStack(info, head)){
      return;
    }
    // TODO should I add an exit handler here
    exitHandler(info, info->code);
  }

  else if (info -> code == TRAP_MEMORY_ACCERR) {
    TracePrintf(1, "Protection violation at addr %p \n", info->addr);
    exitHandler(info, info->code);
    return;
  }

  else if (info -> code == TRAP_MEMORY_KERNEL) {
    TracePrintf("Linux kernel sent SIGSEGV at addr %p \n", info->addr);
    exitHandler(info, info->code);
    return;
  }

  else if (info -> code == TRAP_MEMORY_USER) {
    TracePrintf(1, "Received SIGSEGV from user");
    exitHandler(info, info->code);
    return;
  }
  else { 
    return; 
  }

}

void mathTrapHandler (ExceptionInfo *info) {

}

void ttyRecieveTrapHandler (ExceptionInfo *info) {

}

void
ttyTransmitTrapHandler (ExceptionInfo *info) {

}

void
ttyReadHandler(ExceptionInfo *info) {

}

void
ttyWriteHandler(ExceptionInfo *info) {

}

void getPidHandler(ExceptionInfo *info) {
  info->regs[0] = getCurrentPid();
}

void delayHandler(ExceptionInfo *info) {
  int ticksToGo = info->regs[1];
  struct scheduleNode *currNode = getHead();
  struct processControlBlock *currPCB = currNode->pcb;
  
  // check that we have a valid delay
  if (ticksToGo < 0){
    info->regs[0] = ERROR;
    return;
  }
  // set the delay in current process
  currPCB->delay = ticksToGo;
  info->regs[0] = 0;

  if(ticksToGo > 0){
    TracePrintf(1, "trapHandlers: In delayHandler... initiating a context switch.\n");
    scheduleProcess();
  }
  return;
}

void resetSwitchTime(){
  timeToSwitch = SCHEDULE_DELAY;
}


void exitHandler(ExceptionInfo *info, int error) {
  struct scheduleNode *currNode = getHead();
  int exitType;
  if (error) {
    exitType = ERROR;
  } else {
    exitType= info->regs[1];
  }

  TracePrintf(1, "trapHandlers: Process with pid %d attempting to exit\n", currNode->pcb->pid);

  // check that pcb is not an orphan process
  if (currNode->pcb->parentPid != ORPHAN_PARENT_PID) {
    struct processControlBlock *parentPCB = getPCB(currNode->pcb->parentPid);
    TracePrintf(3, "trap_handlers: parent: %d\n", parentPCB->pid);
    parentPCB->isWaiting = 0;
    // need to do more bookkeeping to keep track of exiting processes
    appendChildExitNode(parentPCB, getCurrentPid(), exitType);
  }
  // remove the current node from scheduling and perform scheduling to pick a new process
  removeExitingProcess();
}
