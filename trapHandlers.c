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

}

void forkTrapHandler(ExceptionInfo *info){


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

/*
 * Process:
 * 1. Set the delay inside the current process's pcb
 * 2. move current process to the end of the schedule
 * 3. move the next process to be run to the head
 * 4. context switch from currently running process to that next process
 */
void delayHandler(ExceptionInfo *info) {

}

void exitHandler(ExceptionInfo *info, int error) {

}

void resetSwitchTime(){
  timeToSwitch = SCHEDULE_DELAY;
}
