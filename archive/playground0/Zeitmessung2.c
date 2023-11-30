#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sched.h>
static __inline__ uint32_t rdtsc(void)
{
    uint32_t lo;
    __asm__ volatile ("rdtscp" : "=a" (lo) : : "ecx", "edx");
    return lo;
}
#define MATRIX_SIZE 999
void do_something(){
	uint64_t matrix[MATRIX_SIZE][MATRIX_SIZE] = {{rand()}};
	for (int i=0;i<MATRIX_SIZE;i++) for (int j=0;j<MATRIX_SIZE;j++) matrix[i][j]=rand();
	for (int i=0;i<MATRIX_SIZE;i++) for (int j=0;j<MATRIX_SIZE;j++) matrix[i][j]= 1472 * matrix[i][j] * rand() + rand();
}

void get_core()
{
	// Get the CPU affinity mask
    cpu_set_t cpu_set;
    if (sched_getaffinity(0, sizeof(cpu_set), &cpu_set) == -1) {
        perror("sched_getaffinity");
        exit(EXIT_FAILURE);
    }

    // Find and print the core ID
    for (int i = 0; i < CPU_SETSIZE; ++i) {
        if (CPU_ISSET(i, &cpu_set)) {
            printf("The program is running on core %d\n", i);
            break;
        }
    }
}

int main(){
	get_core();
	srand(42);
	for (int j =0;j<10;j++){
		uint32_t start = rdtsc();
		while(rdtsc()-start < 1000000000);
		for (int i=0;i<10;i++) do_something();
		printf("%u\n", rdtsc()-start);
	}
	return 0;
}
