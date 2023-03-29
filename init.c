#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>


void testDelay(int amount){
	printf("Init: Calling Delay with parameter %d\n", amount);
	printf("Init: Delay returned with value %d\n", Delay(amount));
}

void testFork(){
	printf("Test fork Process Initialized.\n");

	int childPid = Fork();

	printf("My Pid is: %d\n", GetPid());
	printf("child_pid is: %d\n", childPid);
}

int main() {
	printf("Init: Initialized and running.\n");

	// test fork
	testFork();
	/*
	// Test delay
	testDelay(-10);	
	int i;
	for(i = 10; i >= 0; i--){
		printf("Init (id = %d): i = %d\n", GetPid(), i);
		testDelay(i);
		
		// Just spend some time here, for clock cycles to check that it doesn't switch init out if no other
		printf("Init: Doing busy work. Idle should not run until Delay!\n");
		int j;
		for(j = 200; !(j >= 0 && j <= 100); j += 7); // Will int overflow. Adjust inc for speed
	}
	*/
	
	// TODO The true contents of init, once we're done testing.
	
	while(1) Pause();
	
	return 0;
}
