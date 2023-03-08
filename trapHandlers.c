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
      TracePrintf(1, "trap_handlers: Fork requested.\n");
      fork_trap_handler(frame);
      break;
    case YALNIX_EXEC:
      TracePrintf(1, "trap_handlers: Exec requested.\n");
      exec_trap_handler(frame);
      break;
    case YALNIX_EXIT:
      TracePrintf(1, "trap_handlers: Exit requested.\n");
      exit_handler(frame, 0);
      break;
    case YALNIX_WAIT:
      TracePrintf(1, "trap_handlers: Wait requested.\n");
      wait_trap_handler(frame);
      break;
    case YALNIX_GETPID:
      TracePrintf(1, "trap_handlers: GetPid requested.\n");
      getpid_handler(frame);
      break;
    case YALNIX_BRK:
      TracePrintf(1, "trap_handlers: Brk requested.\n");
      brk_handler(frame);
      break;
    case YALNIX_DELAY:
      TracePrintf(1, "trap_handlers: Delay requested.\n");
      delay_handler(frame);
      break;
    case YALNIX_TTY_READ:
      TracePrintf(1, "trap_handlers: Tty Read requested.\n");
      tty_read_handler(frame);
      break;
    case YALNIX_TTY_WRITE:
      TracePrintf(1, "trap_handlers: Tty Write requested.\n");
      tty_write_handler(frame);
      break;
  }

}

void wait_trap_handler(ExceptionStackFrame *frame){

}

void exec_trap_handler(ExceptionStackFrame *frame){

}

void fork_trap_handler(ExceptionStackFrame *frame){


}

void clock_trap_handler (ExceptionStackFrame *frame) {

}

void illegal_trap_handler (ExceptionStackFrame *frame) {

}

void memory_trap_handler (ExceptionStackFrame *frame) {

}

void math_trap_handler (ExceptionStackFrame *frame) {

}

void tty_recieve_trap_handler (ExceptionStackFrame *frame) {

}

void
tty_transmit_trap_handler (ExceptionStackFrame *frame) {

}

void
tty_read_handler(ExceptionStackFrame *frame) {

}

void
tty_write_handler(ExceptionStackFrame *frame) {

}

void getpid_handler(ExceptionStackFrame *frame) {

}

/*
 * Process:
 * 1. Set the delay inside the current process's pcb
 * 2. call move_head_to_tail() to move current process to the end of the schedule
 * 3. call select_next_process() to move the next process to be run to the head
 * 4. context switch from currently running process to that next process
 */
void delay_handler(ExceptionStackFrame *frame) {

}

void exit_handler(ExceptionStackFrame *frame, int error) {

}

void
reset_time_till_switch() {
  time_till_switch = SCHEDULE_DELAY;
}