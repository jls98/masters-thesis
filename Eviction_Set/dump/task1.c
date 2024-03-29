#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>

#define PROBE_REPS (1<<25)

static void wait(const uint64_t cycles);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);
static uint64_t probe_chase_loop(const void *addr, const uint64_t reps);
static void create_pointer_chase(void** addr, const uint64_t size);

int main(int ac, char **av) {
    wait(1E9);

    for (int k = 12; k < 26; k++) {
        int size = 1 << k;
        void* *buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        create_pointer_chase(buffer, size / sizeof(void*));
        uint64_t millicycles = probe_chase_loop(buffer, PROBE_REPS);
        printf("size: %2d bits; time: %7.3f cycles\n", k, (double)millicycles/(1<<10));

        munmap(buffer, size);
    }
    return 0;
}

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
    for (int i = 0; i < 8*sizeof(uint64_t); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

static uint64_t lfsr_step(uint64_t lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

static uint64_t probe_chase_loop(const void *addr, const uint64_t reps) {
	volatile uint64_t time;
	
	asm __volatile__ (
        // measure
		"mfence;"
		"lfence;"
		"rdtsc;"
		"lfence;"
		"mov rsi, rax;"
        // high precision
        "shl rdx, 32;"
		"or rsi, rdx;"
		// BEGIN - probe address
        "mov rax, %1;"
        "mov rdx, %2;"
        "loop:"
		"mov rax, [rax];"
        "dec rdx;"
        "jnz loop;"
		// END - probe address
		"lfence;"
		"rdtsc;"
        // start - high precision
        "shl rdx, 32;"
        "or rax, rdx;"
        // end - high precision
		"sub rax, rsi;"
		: "=a" (time)
		: "c" (addr), "r" (reps)
		: "esi", "edx"
	);
	return time / (uint64_t)(reps >> 10);
}

static void create_pointer_chase(void** addr, const uint64_t size) {
    for (int i = 0; i < size; i++) {
        addr[i] = NULL;
    }
    uint64_t lfsr = lfsr_create();
    uint64_t offset, curr = 0;
    for (int i = 0; i < size - 1; i++) {
        do {
            offset = lfsr_rand(&lfsr) % size;
        } while (offset == curr || addr[offset] != NULL);
        addr[curr] = &addr[offset];
        curr = offset;
    }
    addr[curr] = addr;
}
