#include <stdio.h>
#include <comp421/hardware.h>

int main(){
	printf("Starting idle process initialization\n");
	int i = 0;
	while(1){
		printf("Running idle process: i = %d\n", i);
		i++;
		//Pause();
	}
}

