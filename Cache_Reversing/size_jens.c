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

static void access(int size){
        void* *buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        create_pointer_chase(buffer, size / sizeof(void*));
        uint64_t millicycles = probe_chase_loop(buffer, PROBE_REPS);
        printf("%2d %7.3f\n", size, (double)millicycles/(1<<10));

        munmap(buffer, size);
}

int main(int ac, char **av) {
    wait(1E9);

    access(16384+12*1024);
    access(16384+13*1024);
    access(16384+14*1024);
    access(16384+15*1024);
    access(16384+16*1024);
    access(16384+17*1024);
    access(16384+18*1024);
    access(16384+19*1024);
    access(16384+20*1024);
    access(16384+21*1024);
    access(16384+22*1024);
    access(16384+23*1024);
    access(16384+24*1024);
    access(16384+25*1024);
    access(16384+26*1024);
    access(16384+27*1024);
    access(16384+28*1024);
    access(16384+29*1024);
    access(16384+30*1024);
    access(16384+31*1024);
    access(16384+32*1024);
    access(16384+33*1024);
    access(16384+34*1024);
    access(16384+35*1024);
    access(16384+36*1024);
    access(16384+37*1024);

    access(65536);
    access(65536*2);

    access(262144-8*1024);
    access(262144-4*1024);
    access(262144-2*1024);
    access(262144-1*1024);
    access(262144);
    access(262144+1*1024);
    access(262144+2*1024);
    access(262144+4*1024);
    access(262144+8*1024);
    access(262144+16*1024);
    access(262144+32*1024);

    access(65536*8);
    access(65536*14);
    access(65536*15);
    access(65536*16);
    access(65536*17);
    access(65536*18);
    access(65536*19);
    access(65536*20);
    access(65536*21);

    access(2097152-4*65536);
    access(2097152-2*65536);
    access(2097152-1*65536);
    access(2097152);
    access(2097152+1*65536);
    access(2097152+2*65536);
    access(2097152+3*65536);
    access(2097152+4*65536);
    access(2097152+8*65536);
    access(2097152+12*65536);
    access(2097152+16*65536);

    access(4194304);
    access(4194304*2);
    access(4194304*4);
    access(4194304*5);
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
