#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

static __inline__ uint32_t rdtsc(void)
{
    uint32_t lo;
    __asm__ volatile ("rdtscp" : "=a" (lo) : : "ecx", "edx");
    return lo;
}

void *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    struct stat st_buf;
    if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    size_t map_len = st_buf.st_size - offset;
	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_PRIVATE, file_descriptor, offset);
	if (mapping == MAP_FAILED){
		printf("mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return mapping;  // mapping will be implicitly unmapped when calling function will be exited
}

uint32_t reload(void *p)
{
	volatile unsigned long time;    
    __asm__ volatile (
        " mfence            \n"
        " lfence            \n"
        " rdtsc             \n"
        " mov r8, rax 		\n"
        " mov rax, [%1]		\n"
        " lfence            \n"
        " rdtsc             \n"
        " sub rax, r8 		\n"
        : "=&a" (time)
        : "r" (p)
        : "rdx", "r8");
    return time;
}


#define REPEATS 99999
void do_something(void *p){
	for(int i=0;i<REPEATS;i++) reload(p);
	// load REPEATS times
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
	void *file_ptr = map("zero_file", 0);
	uint32_t start = rdtsc();
	while(rdtsc()-start < 1000000000);
	while (1){
		start=rdtsc();
		do_something(file_ptr);
		printf("%u\n", rdtsc()-start);
	}
	return 0;
}
