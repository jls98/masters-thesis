#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>

uint64_t probe(void *adrs){
	volatile uint64_t time;  
	__asm__ volatile (
        " mfence            \n"
        " lfence            \n"
        " rdtscp             \n"
        " mov %rax, %r8 		\n"
        " mov (%%1), %rax		\n"
        " lfence            \n"
        " rdtscp             \n"
        " sub %r8, %rax 		\n"
        : "=&a" (time)
        : "r" (adrs)
        : "%ecx", "%rdx", "%r8", "memory"
	);
	return time;
}

void flush(void *adrs){
	__asm__ volatile("clflush [%0];lfence" ::"r" (adrs));
}

int main()
{
	void *map_read = mmap(NULL, 64, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	flush(map_read);
    printf("Hello World %lu\n", probe(map_read));
    printf("Hello World %lu\n", probe(map_read));
	flush(map_read);
	_m_prefetchw(map_read);
    printf("Hello World %lu\n", probe(map_read));
    return 0;
}
