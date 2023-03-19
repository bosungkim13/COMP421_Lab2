#include <comp421/hardware.h>
#include <comp421/yalnix.h>

#define ORPHAN_PARENT_PID -1

struct processControlBlock
{
  int pid;
  struct pte *pageTable;
  SavedContext savedContext;
  int delay;
  void *brk;
  void *userStackLimit;
  int isWaiting; // 1 if blocked due to a Wait call. 0 otherwise.
  int numChildren;
  int parentPid;
  int outOfMemory;

  // need to add stuff for exit status as well

  // probably need to add some more stuff for the terminal blocking for reading and writing
};

struct processControlBlock* createNewProcess(int pid, int parentPid);