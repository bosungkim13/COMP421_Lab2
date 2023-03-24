#include <comp421/hardware.h>
#include <comp421/yalnix.h>

SavedContext* mySwitchFunc(SavedContext *ctxp, void* p1, void* p2);
SavedContext* idleInitFunc(SavedContext *ctxp, void* p1, void* p2);
SavedContext* forkFunc(SavedContext *ctxp, void* p1, void* p2);
void copyKernelStack(struct pte* virtPTFrom, struct pte* virtPTTo);
void switchReg0To(void* destPTVirt);
