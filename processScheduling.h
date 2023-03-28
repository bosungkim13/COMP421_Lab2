#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdlib.h>

#define INIT_PID 0
#define IDLE_PID 1
#define IDLE_DELAY -1

struct scheduleNode {
  struct scheduleNode *next;
  struct processControlBlock *pcb;
};

void addToSchedule(struct processControlBlock *pcb);
void setIdlePCB(struct processControlBlock* pcb);
struct scheduleNode* getHead();
int getCurrentPid();

int setAndCheckClockTickPID();
void decreaseDelay();

void scheduleProcess(int isExit);
void chooseNextProcess();
int nextProcessToHead(int delayMatch);
void removeExitingProcess();
int updateAndGetNextPid();
void removeHead();
