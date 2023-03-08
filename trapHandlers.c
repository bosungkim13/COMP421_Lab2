#include "trapHandlers.h"
#include <stdio.h>
#include "memoryManagement.h"

void getpid_handler(ExceptionStackFrame *frame);
void delay_handler(ExceptionStackFrame *frame);
void exit_handler(ExceptionStackFrame *frame, int error);
void fork_trap_handler(ExceptionStackFrame *frame);
void wait_trap_handler(ExceptionStackFrame *frame);
void exec_trap_handler(ExceptionStackFrame *frame);
void tty_read_handler(ExceptionStackFrame *frame);
void tty_write_handler(ExceptionStackFrame *frame);

#define SCHEDULE_DELAY  2
int time_till_switch = SCHEDULE_DELAY;

void kernelTrapHandler(ExceptionStackFrame *frame) {
  TracePrintf(1, "trapHandlers: In TRAP_KERNEL interrupt handler...\n");

  int code = frame->code;

  switch (code) {
    case YALNIX_FORK:
      TracePrintf(1, "trapHandlers: Fork requested.\n");
      forkTrapHandler(frame);
      break;
    case YALNIX_EXEC:
      TracePrintf(1, "trapHandlers: Exec requested.\n");
      execTrapHandler(frame);
      break;
    case YALNIX_EXIT:
      TracePrintf(1, "trapHandlers: Exit requested.\n");
      exitHandler(frame, 0);
      break;
    case YALNIX_WAIT:
      TracePrintf(1, "trapHandlers: Wait requested.\n");
      waitTrapHandler(frame);
      break;
    case YALNIX_GETPID:
      TracePrintf(1, "trapHandlers: GetPid requested.\n");
      getPidHandler(frame);
      break;
    case YALNIX_BRK:
      TracePrintf(1, "trapHandlers: Brk requested.\n");
      brkHandler(frame);
      break;
    case YALNIX_DELAY:
      TracePrintf(1, "trapHandlers: Delay requested.\n");
      delayHandler(frame);
      break;
    case YALNIX_TTY_READ:
      TracePrintf(1, "trapHandlers: Tty Read requested.\n");
      ttyReadHandler(frame);
      break;
    case YALNIX_TTY_WRITE:
      TracePrintf(1, "trapHandlers: Tty Write requested.\n");
      ttyWriteHandler(frame);
      break;
  }

}

void waitTrapHandler(ExceptionStackFrame *frame){

}

void execTrapHandler(ExceptionStackFrame *frame){

}

void forkTrapHandler(ExceptionStackFrame *frame){


}

void clockTrapHandler (ExceptionStackFrame *frame) {

}

void illegalTrapHandler (ExceptionStackFrame *frame) {

}

void memoryTrapHandler (ExceptionStackFrame *frame) {

}

void mathTrapHandler (ExceptionStackFrame *frame) {

}

void ttyRecieveTrapHandler (ExceptionStackFrame *frame) {

}

void
ttyTransmitTrapHandler (ExceptionStackFrame *frame) {

}

void
ttyReadHandler(ExceptionStackFrame *frame) {

}

void
ttyWriteHandler(ExceptionStackFrame *frame) {

}

void getPidHandler(ExceptionStackFrame *frame) {

}

/*
 * Process:
 * 1. Set the delay inside the current process's pcb
 * 2. call move_head_to_tail() to move current process to the end of the schedule
 * 3. call select_next_process() to move the next process to be run to the head
 * 4. context switch from currently running process to that next process
 */
void delayHandler(ExceptionStackFrame *frame) {

}

void exitHandler(ExceptionStackFrame *frame, int error) {

}

void
resetTimeTillSwitch() {
  time_till_switch = SCHEDULE_DELAY;
}