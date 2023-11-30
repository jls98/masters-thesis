#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// constants
#define FEEDBACK 0x80000000000019E2ULL
#define ACCELERATOR 1000000000

//#define ACCESS_AMOUNT 34359738368 // 2^35
//#define ACCESS_AMOUNT 33554432 // 2^25
#define ACCESS_AMOUNT 2048576 // 2^20

uint64_t step(uint64_t lfsr) {
  lfsr = (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
  return lfsr;
}

uint64_t load_address1(void *p, uint64_t *val)
{
	volatile unsigned long long time;  
	__asm__ volatile (
        " mfence            \n"
        " lfence            \n"
        " rdtsc             \n"
        " mov r8, rax 		\n"
        " mov %1, [%2]		\n"
        " lfence            \n"
        " rdtsc             \n"
        " sub rax, r8 		\n"
        : "=&a" (time), "=r" (*val)
        : "r" (p)
        : "rdx", "r8");
		
		return time;
}

static __inline__ uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp" : "=a" (lo), "=d" (hi) : : "ecx");
    return ((uint64_t)hi<<32)|lo;
}

void *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    struct stat st_buf;
    if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    size_t map_len = st_buf.st_size;
	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
	if (mapping == MAP_FAILED){
		printf("mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
}

// determines L1 cache access time, determines L1 cache size, returns estimated L1 cache size
int L1_control()
{
	return 0;
}

int main()
{
	return 0;
}

// determine L1 cache hit access time 

// determine cache miss time but loaded from memory

// determine L1 cache size 

// determine L2 cache hit access time

// determine L2 cache size