#include <stdio.h>
#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <strings.h>
#include <stdlib.h>

int
main() {
    printf("Init Process Initialized.\n");
	int i = 100;
	while(1){
		printf("Running init process: i = %d\n", i);
		i--;
		//Pause();
	}
}