#include <comp421/hardware.h>
#include <comp421/yalnix.h>


void getPidHandler(ExceptionStackFrame *frame);
void delayHandler(ExceptionStackFrame *frame);
void exitHandler(ExceptionStackFrame *frame, int error);
void forkTrapHandler(ExceptionStackFrame *frame);
void waitTrapHandler(ExceptionStackFrame *frame);
void execTrapHandler(ExceptionStackFrame *frame);
void ttyReadHandler(ExceptionStackFrame *frame);
void ttyWriteHandler(ExceptionStackFrame *frame);
void kernelTrapHandler(ExceptionStackFrame *frame);
void clockTrapHandler (ExceptionStackFrame *frame);
void illegalTrapHandler (ExceptionStackFrame *frame);
void memoryTrapHandler (ExceptionStackFrame *frame);
void mathTrapHandler (ExceptionStackFrame *frame);
void ttyRecieveTrapHandler (ExceptionStackFrame *frame);
void ttyTransmitTrapHandler (ExceptionStackFrame *frame);
void reset_time_till_switch();