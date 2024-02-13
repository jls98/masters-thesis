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
        " mov r8, rax 		\n"
        " mov rax, [%1]		\n"
        " lfence            \n"
        " rdtscp             \n"
        " sub rax, r8 		\n"
        : "=&a" (time)
        : "r" (adrs)
        : "ecx", "rdx", "r8", "memory"
	);
	return time;
}

void cpuid(){
    __asm__ volatile ("cpuid"::: "eax", "ebx","ecx", "edx");
}

int main()
{
	void *map_read = mmap(NULL, 64, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	_mm_clflush(map_read);
    _mm_mfence();
    uint64_t time = probe(map_read);
    _mm_mfence();
    printf("Hello World %lu\n", time);
    _mm_mfence();
    time = probe(map_read);
    _mm_mfence();
    printf("Hello World %lu\n", time);
    _mm_mfence();
    _mm_clflush(map_read);
    _mm_mfence();
    cpuid();
    _mm_mfence();
	_m_prefetchw(map_read);
    _mm_mfence();
    time = probe(map_read);
    _mm_mfence();
    printf("Hello World %lu\n", time);
    _mm_mfence();
    return 0;
}
