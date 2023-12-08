#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

#define PROBE_REPS (1<<25)
#define MEMSIZE_EXP_MIN 14
#define MEMSIZE_EXP_MAX 23
#define CACHE_SIZE_DEFAULT 32768


static void wait(const uint64_t cycles);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);
static uint64_t probe_stride_loop(const void *addr, const uint64_t reps);
static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride);
int get_ways(int cache_size);

int main(int ac, char **av){
    return ac==2 ? get_ways(atoi(av[1])) : get_ways(CACHE_SIZE_DEFAULT);
}

uint64_t log_2(uint64_t val) {
   uint64_t count = 0;
   while (val >>= 1) {
      ++count;
   }
   return count;
}

int get_ways(int cache_size) {
    wait(1E9);
    uint64_t double_cache_size = 4*cache_size;
    // check stride in power of two
    for (uint32_t stride = 1; stride < log_2(double_cache_size)-1; stride++) {
        void* buffer = mmap(NULL, double_cache_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        create_pointer_stride_chase(buffer, double_cache_size / sizeof(void*), 1<<stride);        
        uint64_t reps = double_cache_size % (1<<stride) == 0? double_cache_size/(1<<stride) : double_cache_size/(1<<stride) +1;
        printf("reps: %5d\n", reps);
        uint64_t millicycles = probe_stride_loop(buffer, reps);
        printf("stride: %5d; time: %7.3f cycles\n", stride, ((double)millicycles)/(1<<10));

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

static uint64_t probe_stride_loop(const void *addr, const uint64_t reps) {
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

static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride) {
    for (uint64_t i = 0; i < size; i++) {
        addr[i] = NULL; // set all entries inn addr to NULL
    }
    uint64_t lfsr = lfsr_create(); // start random lfsr
    uint64_t offset, curr = 0; // offset = 0
    uint64_t stride_indexes = size % stride == 0? size/stride : size/stride +1;
    for (uint64_t i = 0; i < stride_indexes-1; i++) {
        do {
            offset = lfsr_rand(&lfsr) % size; // random number mod size 
        } while (offset == curr || addr[offset] != NULL || offset % stride != 0); // ensure that offset !=curr and addr[offset]==NULL and jumps only between entries of stride
        addr[curr] = &addr[offset]; // set the value of the curr index to the address at the offset index (linked list)
        curr = offset;
    }
    addr[curr] = addr;
}
