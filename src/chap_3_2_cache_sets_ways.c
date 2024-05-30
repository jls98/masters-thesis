#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

#define PROBE_REPS (1<<25)

// Wait cycles amount of time.
static void wait(const uint64_t cycles);

// lfsr
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);

// Probe a pointer chase starting from addr and access reps many elements of the pointer chase.
static double probe_stride_loop(void *addr, uint64_t reps);

// Creates pointer chase between elements of addr with size many elements with elements of stride stride.
static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride);
int get_ways_sqr(int cache_size);

// Input cache size in bytes. 32768, 49152, 262144, 1310720, 2097152.
int main(int ac, char **av){
    ac==2 ? get_ways_sqr(atoi(av[1])) : get_ways_sqr(2097152);
}

// Computes log with base 2 of input val.
uint64_t log_2(uint64_t val) {
   uint64_t count = 0;
   while (val >>= 1) ++count;
   return count;
}

static void wait(const uint64_t cycles) {
	unsigned int ignore;
	uint64_t start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

int get_ways_sqr(int cache_size) {
    wait(1E9);
    uint64_t size_buf = 2*cache_size, len_buf = size_buf / sizeof(void*);
    
    // Stride as multiple of two.
    for (uint64_t s = 1; s < log_2(size_buf)-1; s++) {
        void* buffer = mmap(NULL, size_buf, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
		uint64_t stride = 1<<s;
        create_pointer_stride_chase(buffer, len_buf, stride);
        probe_stride_loop(buffer, len_buf);
        double millicycles = probe_stride_loop(buffer, PROBE_REPS);
        printf("%7ld %7.3f\n", 8*stride, (double) millicycles/(1<<5));
        munmap(buffer, size_buf);
    }
    return 0;
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

static double probe_stride_loop(void *addr, uint64_t reps) {
	if(reps==0) return 0.0f;
	volatile uint64_t time;
	asm __volatile__ (
		// load all entries into cache
		"lfence;"
		"mov rax, %1;" 
		"mov rdx, %2;"
		"loop0:"
		"mov rax, [rax];"
        "dec rdx;"
        "jnz loop0;"
        // START
		"lfence;"
		"rdtsc;"
		"lfence;"
		"mov rsi, rax;"
        // high precision
        "shl rdx, 32;"
		"or rsi, rdx;"
		// BEGIN - probe addresses
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
		// END
		"sub rax, rsi;"
		: "=a" (time)
		: "c" (addr), "r" (reps)
		: "esi", "edx"
	);
	return (double)time / (uint64_t)(reps >> 5);
}

static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride) {
    for (uint64_t i = 0; i < size; i++) addr[i] = NULL; // set all entries inn addr to NULL    
    uint64_t lfsr = lfsr_create(), offset, curr = 0; 

    // Calculate number of elements that remain when reducing the set with size size to only the elements of stride stride.
    uint64_t stride_indexes = size % stride == 0? size/stride : size/stride +1; 
    
    // Find elements in addr that are not assigned yet and in stride stride.
    for (uint64_t i = 0; i < stride_indexes-1; i++) {
        do {
            offset = lfsr_rand(&lfsr) % size; // random number mod size 
        } while (offset == curr || addr[offset] != NULL || offset % stride != 0); // ensure that offset !=curr and addr[offset]==NULL and jumps only between entries of stride
        addr[curr] = &addr[offset]; // set the value of the curr index to the address at the offset index (linked list)
        curr = offset;
    }
    addr[curr] = addr;
}
