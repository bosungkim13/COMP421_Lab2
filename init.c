#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

void testDelay(int amount){
	printf("Init id %d: Calling Delay with parameter %d\n", GetPid(), amount);
	printf("Init id %d: Delay returned with value %d\n", GetPid(), Delay(amount));
}
void delayCountdownLoop(int start, int busyWorkInc){
	int i = start;
	for(; i >= 0; i--){
		printf("Init id %d: Delay loop i = %d\n", GetPid(), i);
		testDelay(i);
		
		printf("Init id %d: Doing busy work. Idle should not run until Delay!\n", GetPid());
		int j;
		for(j = 200; !(j >= 0 && j <= 100); j += busyWorkInc); // Will int overflow. Adjust inc for speed
	}
}

// Returns true if child
int testFork(){
	printf("Init id %d: Testing fork...\n", GetPid());

	int childPid = Fork();

	printf("Init id %d: Fork returned\n", GetPid());
	printf("Init id %d: childPid is %d\n", GetPid(), childPid);
	return childPid == 0;
}

void testExit(){
	printf("Init id %d: Testing exit\n", GetPid());
	Exit(0);
}

int main() {
	printf("Init id %d: Initialized and running.\n", GetPid());

	// Test invalid delay
	/*testDelay(-10);

	int i = 0;
	for(; i < 4; i++){
		printf("Init id %d: Main loop iteration %d\n", GetPid(), i);
		if(testFork()){
			// Child
			//testDelay(12);
			delayCountdownLoop(i+4, 8-i);
			break;
		}else{
			// Parent, or if fork failed
			//testDelay(2);
			delayCountdownLoop(i+5, 5);
		}
	}*/
	testFork();
	testFork();
	testFork();
	// There should be 8 processes
	testDelay(12-GetPid());
	
	// Test exit
	testExit();
	
	// TODO The true contents of init, once we're done testing.
	//while(1) Pause();
	
	return 0;
}
