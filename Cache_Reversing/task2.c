#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

#define PROBE_REPS (1<<25)
#define MEMSIZE_EXP_MIN 14
#define MEMSIZE_EXP_MAX 23
#define CACHE_SIZE_DEFAULT 32770


static void wait(const uint64_t cycles);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);
static uint64_t probe_stride_loop(const void *addr, const uint64_t addr_len, const uint64_t reps, const uint64_t stride);
static unsigned int log2_floor(unsigned int val);
static void create_pointer_chase(void** addr, const uint64_t size);
int get_ways(int cache_size);

int main(int ac, char **av){
    return ac==2 ? get_ways(atoi(av[1])) : get_ways(CACHE_SIZE_DEFAULT);
}

// compute log of 2 floored to get possible amount of divisions by 2
static unsigned int log2_floor(unsigned int val) {
    unsigned int count = 0;
    while (val >>= 1) ++count;
    return count;
}

int get_ways(int cache_size) {
    wait(1E9);
    int double_cache_size = 2*cache_size;
    void* buffer = mmap(NULL, double_cache_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    create_pointer_chase(buffer, double_cache_size / sizeof(void*));
    // check stride in power of two
    for (uint32_t stride = 0; stride < log2_floor(double_cache_size); stride++) {
        uint64_t millicycles = probe_stride_loop(buffer, double_cache_size, PROBE_REPS, stride);
        printf("stride: %5d; time: %7.3f cycles\n", stride, (double)millicycles/(1<<10));

        munmap(buffer, double_cache_size);
        
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
    for (uint64_t i = 0; i < 8*sizeof(uint64_t); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

static uint64_t lfsr_step(uint64_t lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

static uint64_t probe_stride_loop(const void *addr, const uint64_t addr_len, const uint64_t reps, const uint64_t stride) {
	volatile uint64_t time;
	uint64_t stride_size = stride*sizeof(void*);
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
        "xor r9, r9;" // zero, r8 index counter
        "mov rdx, %1;"
        "loop:"
		"mov r8, [r9];" // load
        // calculate new index
        "add r9, %4;" // index_offset + stride      
        "mov rdx, r9;"   // compute new index (old_index+stride)
        "add rdx, %1;"
        "div rbx;" // eax contains quotient, edx contains remainder 
        "dec %k3;" // decrement counter reps
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
		: "r" (addr), "b" (addr_len), "r" (reps), "r" (stride_size)
		: "rsi", "rdx", "r8", "r9" // rsi and rdx used by rdtsc, r8 holds loaded value, r9 holds current adrs (base+index), rdx holds (current) index
	);
	return time / (uint64_t)(reps >> 10);
}

static void create_pointer_chase(void** addr, const uint64_t size) {
    for (uint64_t i = 0; i < size; i++) {
        addr[i] = NULL; // set all entries inn addr to NULL
    }
    uint64_t lfsr = lfsr_create(); // start random lfsr
    uint64_t offset, curr = 0; // offset = 0
    for (uint64_t i = 0; i < size - 1; i++) {
        do {
            offset = lfsr_rand(&lfsr) % size; // random number mod size 
        } while (offset == curr || addr[offset] != NULL); // ensure that offset !=curr and addr[offset]==NULL
        addr[curr] = &addr[offset]; // set the value of the curr index to the address at the offset index (linked list)
        curr = offset;
    }
    addr[curr] = addr;
}
