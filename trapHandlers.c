#include <stdio.h>
#include "processScheduling.h"
#include "memoryManagement.h"
#include "pageTableManagement.h"
#include "processControlBlock.h"
#include "loadProgram.h"
#include "contextSwitch.h"
#include "trapHandlers.h"

void kernelTrapHandler(ExceptionInfo *info) {
  TracePrintf(1, "trapHandlers: In TRAP_KERNEL interrupt handler...\n");

  int code = info->code;
  int currentPid = getCurrentPid();

  switch (code) {
    case YALNIX_FORK:
      TracePrintf(1, "trapHandlers: Fork requested by process %d.\n", currentPid);
      forkTrapHandler(info);
      break;
    case YALNIX_EXEC:
      TracePrintf(1, "trapHandlers: Exec requested by process %d.\n", currentPid);
      execTrapHandler(info);
      break;
    case YALNIX_EXIT:
      TracePrintf(1, "trapHandlers: Exit requested by process %d.\n", currentPid);
      exitHandler(info, 0);
      break;
    case YALNIX_WAIT:
      TracePrintf(1, "trapHandlers: Wait requested by process %d.\n", currentPid);
      waitTrapHandler(info);
      break;
    case YALNIX_GETPID:
      TracePrintf(1, "trapHandlers: GetPid requested by process %d.\n", currentPid);
      getPidHandler(info);
      break;
    case YALNIX_BRK:
      TracePrintf(1, "trapHandlers: Brk requested by process %d.\n", currentPid);
      brkHandler(info);
      break;
    case YALNIX_DELAY:
      TracePrintf(1, "trapHandlers: Delay requested by process %d.\n", currentPid);
      delayHandler(info);
      break;
    case YALNIX_TTY_READ:
      TracePrintf(1, "trapHandlers: Tty Read requested by process %d.\n", currentPid);
      ttyReadHandler(info);
      break;
    case YALNIX_TTY_WRITE:
      TracePrintf(1, "trapHandlers: Tty Write requested by process %d.\n", currentPid);
      ttyWriteHandler(info);
      break;
  }
}

void waitTrapHandler(ExceptionInfo *info){
	int* statusPtr = (int*)info->regs[1];
	struct processControlBlock* parentPCB = getRunningNode()->pcb;
	if(parentPCB->numChildren == 0){
		if(parentPCB->exitQ == NULL){
			// Never had children
			info->regs[0] = ERROR;
			return;
		}else{
			// Had children, they've all exited
		}
	}else{
		TracePrintf(1, "Parent process %d has %d children\n", parentPCB->pid, parentPCB->numChildren);
		if(parentPCB->exitQ == NULL){
			TracePrintf(1, "Parent process %d's children are all still running. Waiting...\n", parentPCB->pid);
			parentPCB->isWaiting = 1;
			scheduleProcess(0);
		}
		// Either we already had an exited child
		// or one of the children running exited and switched back to here
	}
	struct exitNode* exit = popChildExitNode(parentPCB);
	TracePrintf(1, "Parent process %d has an exited child: pid %d with status %d\n", parentPCB->pid, exit->pid, exit->exitType);
	*statusPtr = exit->exitType;
	info->regs[0] = exit->pid;
	free(exit);
}

void execTrapHandler(ExceptionInfo *info){
  /*char *filename = (char *)info->regs[1];
  char **argvec = (char **)info->regs[2];

  struct scheduleNode *node = getHead();

  int loadReturn = LoadProgram(filename, argvec, info, node->pcb);
  if (loadReturn == -1){
    info->regs[0] = ERROR;
  }*/
}

void forkTrapHandler(ExceptionInfo *info){
	struct scheduleNode* currentNode = getRunningNode();
	struct processControlBlock* parentPCB = currentNode->pcb;
	
	int childPid = updateAndGetNextPid();
	int parentPid = getCurrentPid();
	struct processControlBlock *childPCB = createNewProcess(childPid, parentPid);
	
	int physicalPagesNeeded = numPagesInUse(parentPCB->pageTable) + KERNEL_STACK_PAGES;
	int physicalPagesAvailable = freePhysicalPageCount();
	if (physicalPagesNeeded > physicalPagesAvailable){
		TracePrintf(1, "Trap Handlers - Fork: In fork handler but not enough free physical pages (%d needed, %d available) for copy\n", physicalPagesNeeded, physicalPagesAvailable);
		freePageTable(childPCB->pageTable);
		free(childPCB);
		info->regs[0] = ERROR;
		return;
	}
	
	parentPCB->numChildren++;
	TracePrintf(1, "Trap Handlers - Fork: Parent pcb %d now has %d running children\n", parentPCB->pid, parentPCB->pid);
	cloneAndSwitchToProcess(parentPCB, childPCB);
	// Returns as the child first, but we don't need to use that info
	if(getCurrentPid() == childPid){
		info->regs[0] = 0;
	} else {
		info->regs[0] = childPid;
	}
}

void clockTrapHandler (ExceptionInfo *info) {
	TracePrintf(1, "trapHandlers: begin clockTraphandler with PID: %d\n", getCurrentPid());
	int delayWasSetToZero = decreaseDelay();
	if(setAndCheckClockTickPID() || (delayWasSetToZero == 1 && getCurrentPid() == IDLE_PID)) {
		TracePrintf(1, "trapHandlers: time to switch... scheduling processes now\n");
		scheduleProcess(0);
	}
}

void illegalTrapHandler (ExceptionInfo *info) {
	TracePrintf(1, "trapHandlers: Illegal trap handler \n");
	
	if (info -> code == TRAP_ILLEGAL_ILLOPC) printf("Illegal Opcode\n");
	else if (info -> code == TRAP_ILLEGAL_ILLOPN) printf("Illegal Operand\n");
	else if (info -> code == TRAP_ILLEGAL_ILLADR) printf("Illegal address mode\n");
	else if (info -> code == TRAP_ILLEGAL_ILLTRP) printf("Illegal software trap\n");
	else if (info -> code == TRAP_ILLEGAL_PRVOPC) printf("Privileged opcode\n");
	else if (info -> code == TRAP_ILLEGAL_PRVREG) printf("Privileged register\n");
	else if (info -> code == TRAP_ILLEGAL_COPROC) printf("Coprocessor error\n");
	else if (info -> code == TRAP_ILLEGAL_BADSTK) printf("Bad stack\n");
	else if (info -> code == TRAP_ILLEGAL_KERNELI) printf("Linux kernel sent SIGILL\n");
	else if (info -> code == TRAP_ILLEGAL_USERIB) printf("Received SIGILL or SIGBUS from user\n");
	else if (info -> code == TRAP_ILLEGAL_ADRALN) printf("Invalid address alignment\n");
	else if (info -> code == TRAP_ILLEGAL_ADRERR) printf("Non-existent physical address\n");
	else if (info -> code == TRAP_ILLEGAL_OBJERR) printf("Object-specific HW error\n");
	else if (info -> code == TRAP_ILLEGAL_KERNELB) printf("Linux kernel sent SIGBUS\n");
	else printf("Some other illegal trap occured\n");
	
	exitHandler(info, 1);
}

void memoryTrapHandler (ExceptionInfo *info) {
  /*
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
    exitHandler(info, 1);
  }

  else if (info -> code == TRAP_MEMORY_ACCERR) {
    TracePrintf(1, "Protection violation at addr %p \n", info->addr);
    exitHandler(info, 1);
    return;
  }

  else if (info -> code == TRAP_MEMORY_KERNEL) {
    TracePrintf(1, "Linux kernel sent SIGSEGV at addr %p \n", info->addr);
    exitHandler(info, 1);
    return;
  }

  else if (info -> code == TRAP_MEMORY_USER) {
    TracePrintf(1, "Received SIGSEGV from user");
    exitHandler(info, 1);
    return;
  }
  else { 
    return; 
  }
  */

}

void mathTrapHandler (ExceptionInfo *info) {
	TracePrintf(1, "Exception: Math\n");
	
	if (info -> code == TRAP_MATH_INTDIV) printf("%s\n", "Integer divide by zero");
	else if (info -> code == TRAP_MATH_INTOVF) printf("%s\n", "Integer overflow");
	else if (info -> code == TRAP_MATH_FLTDIV) printf("%s\n", "Floating divide by zero");
	else if (info -> code == TRAP_MATH_FLTOVF) printf("%s\n", "Floating overflow");
	else if (info -> code == TRAP_MATH_FLTUND) printf("%s\n", "Floating underflow");
	else if (info -> code == TRAP_MATH_FLTRES) printf("%s\n", "Floating inexact result");
	else if (info -> code == TRAP_MATH_FLTINV) printf("%s\n", "Invalid floating operation");
	else if (info -> code == TRAP_MATH_FLTSUB) printf("%s\n", "FP subscript out of range");
	else if (info -> code == TRAP_MATH_KERNEL) printf("%s\n", "Linux kernel sent SIGFPE");
	else if (info -> code == TRAP_MATH_USER) printf("%s\n", "Received SIGFPE from user");
	else printf("%s\n", "Some other math error occured");
	
	exitHandler(info, 1);
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
	// Will not be correct if idle calls getCurrentPid() since idle isn't in the list, but idle won't call
	info->regs[0] = getCurrentPid();
}

void delayHandler(ExceptionInfo *info) {
	int ticksToGo = info->regs[1];
	struct processControlBlock *currPCB = getHead()->pcb;
	
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
		scheduleProcess(0);
	}
	return;
}

void exitHandler(ExceptionInfo *info, int calledDueToProgramError) {
	int currentPid = getCurrentPid();
	if(currentPid == IDLE_PID){
		printf("Idle process called Exit(). Halting...\n");
		Halt();
	}
	
	int parentPid = getRunningNode()->pcb->parentPid;
	int exitType = calledDueToProgramError ? ERROR : (int)(info->regs[1]);
	
	TracePrintf(1, "trapHandlers: Process with pid %d attempting to exit\n", currentPid);
	
	// Check that pcb is not an orphan process
	if (parentPid != ORPHAN_PARENT_PID) {
		struct processControlBlock* parentPCB = getPCB(parentPid);
		TracePrintf(3, "trapHandlers: parent: %d\n", parentPCB->pid);
		parentPCB->numChildren--;
		parentPCB->isWaiting = 0;
		// need to do more bookkeeping to keep track of exiting processes
		appendChildExitNode(parentPCB, getCurrentPid(), exitType);
	}
	// Orphan this process's children
	struct scheduleNode* checkingNode = getRunningNode();
	for(; checkingNode != NULL; checkingNode = checkingNode->next){
		if(checkingNode->pcb->parentPid == currentPid){
			checkingNode->pcb->parentPid = ORPHAN_PARENT_PID;
		}
	}
	// Remove the current node and perform scheduling to pick a new process
	removeExitingProcess();
}
