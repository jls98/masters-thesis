#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

#define PROBE_REPS (1<<25)
#define MEMSIZE_EXP_MIN 14
#define MEMSIZE_EXP_MAX 23

#define E_L1_CACHE_SIZE 32768 	// ecore L1D 32kb
#define E_L2_CACHE_SIZE 2097152 // ecore L2 2MB
#define E_L1_STRIDE 512 		// 8 ways
#define E_L2_STRIDE 32768  		// 16 ways
  
#define P_CACHE_SIZE_DEFAULT_L1 49152 	// pcore L1D 48 kb
#define P_CACHE_SIZE_DEFAULT_L2 1310720 // ecore  L2 1.2MB
#define P_L1_STRIDE 512 				// 12 ways
#define P_L2_STRIDE 16384  				// 10 ways

#define THRESHOLD 100 // TO Determine



int main(int ac, char **av){
	control(2* E_L2_CACHE_SIZE);
}

static void control(uint64_t size){
	// create array with lines (twice cache size)
	void* buffer=mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	
	// conflict set 
	void* conflict_set = (void*)malloc(size);
	uint64_t conflict_set_len = size/sizeof(void*);
	
}

static uint64_t probe(const void *set, const uint64_t set_len, const void* cand) {
	if (reps==0) return 0; // if set is empty, it'll result in cache hit
	
	volatile uint64_t time;
	asm __volatile__ (
		// load candidate and set 
		"mov rax, [%3];" // load candidate 
		// BEGIN - read every entry in addr
        "mov rax, %1;"
        "mov rdx, %2;"
        "loop:"
		"mov rax, [rax];"
        "dec rdx;"
        "jnz loop;"
		// END - reading set
        // measure start
		"mfence;"
		"lfence;"
		"rdtsc;"		
		"lfence;"
		"mov rsi, rax;"
        // high precision
        "shl rdx, 32;"
		"or rsi, rdx;"
		"mov rax, [%3];" // load candidate 	
		"lfence;"
		"rdtsc;"
        // start - high precision
        "shl rdx, 32;"
        "or rax, rdx;"
        // end - high precision
		"sub rax, rsi;"
		: "=a" (time)
		: "c" (set), "r" (set_len), "r" (cand)
		: "esi", "edx"
	);
	return time;
}