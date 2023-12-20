#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

static void wait(const uint64_t cycles);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);
static uint64_t probe(const void *addr, const uint64_t reps, const uint64_t* cand);


static void wait(const uint64_t cycles) {
	unsigned int ignore;
	uint64_t start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

#define FEEDBACK 0x80000000000019E2ULL
static uint64_t lfsr_create(void) {
  uint64_t lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  return lfsr;
}

static uint64_t lfsr_rand(uint64_t* lfsr) {
    for (uint64_t i = 0; i < 8*sizeof(uint64_t); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

static uint64_t lfsr_step(uint64_t lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

// addr contains first addr to array of pointerchase
// reps pointer chase array size 
// cand is candidate to probe
static uint64_t probe(const void *addr, const uint64_t reps, const uint64_t* cand) {
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
		: "c" (addr), "r" (reps), "r" (cand)
		: "esi", "edx"
	);
	return time;
}