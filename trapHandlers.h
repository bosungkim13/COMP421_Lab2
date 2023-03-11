#include <comp421/hardware.h>
#include <comp421/yalnix.h>


void getPidHandler(ExceptionInfo *frame);
void delayHandler(ExceptionInfo *frame);
void exitHandler(ExceptionInfo *frame, int error);
void forkTrapHandler(ExceptionInfo *frame);
void waitTrapHandler(ExceptionInfo *frame);
void execTrapHandler(ExceptionInfo *frame);
void ttyReadHandler(ExceptionInfo *frame);
void ttyWriteHandler(ExceptionInfo *frame);
void kernelTrapHandler(ExceptionInfo *frame);
void clockTrapHandler (ExceptionInfo *frame);
void illegalTrapHandler (ExceptionInfo *frame);
void memoryTrapHandler (ExceptionInfo *frame);
void mathTrapHandler (ExceptionInfo *frame);
void ttyRecieveTrapHandler (ExceptionInfo *frame);
void ttyTransmitTrapHandler (ExceptionInfo *frame);
void reset_time_till_switch();