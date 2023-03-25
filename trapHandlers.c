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
      delayHandler(frame);
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
  // create child process

  // call context switch that copies region 0

  // if the parent pcb is out of memory then pcb at the head is the child but hte page table and contezt are the parents
  // in this case, we need to remove the head from the schedule

  // otherwise, return childs pid or return 0 if you are the child


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

}

void memoryTrapHandler (ExceptionInfo *info) {

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
    // TODO need to do more bookkeeping to keep track of exiting processes
  }
  // TODO remove the current node from scheduling and perform scheduling to pick a new process

}

void resetSwitchTime(){
  timeToSwitch = SCHEDULE_DELAY;
}
