#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

#define PROBE_REPS (1<<20)
#define MEMSIZE_EXP_MIN 14
#define MEMSIZE_EXP_MAX 23
#define CACHE_SIZE_DEFAULT_L1 32768 // ecore
#define CACHE_SIZE_DEFAULT_L2 2097152 // ecore 2MB

static void wait(const uint64_t cycles);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);
static double probe_stride_loop(void *addr, uint64_t reps);
static double probe_stride_loop_c(void *addr, uint64_t reps);
static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride);
int get_ways_sqr(int cache_size);
int get_ways_lin(int cache_size);
int main(int ac, char **av){
    ac==2 ? get_ways_sqr(atoi(av[1])) : get_ways_sqr(CACHE_SIZE_DEFAULT_L1);
}

uint64_t log_2(uint64_t val) {
   uint64_t count = 0;
   while (val >>= 1) {
      ++count;
   }
   return count;
}

int get_ways_sqr(int cache_size) {
    wait(1E9);
    uint64_t double_cache_size = 2*cache_size;
    // check stride in power of two
	printf("probing with set of size %lu for cache size %i\n", double_cache_size, cache_size);
    for (uint64_t s = 1; s < log_2(double_cache_size)-1; s++) {
        void* buffer = mmap(NULL, double_cache_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
		uint64_t buffer_size = double_cache_size / sizeof(void*);
		uint64_t stride = 1<<s;
        create_pointer_stride_chase(buffer, buffer_size, stride);  
        //uint64_t reps = double_cache_size % stride == 0? buffer_size/stride : buffer_size/stride +1;
		//printf("s %lu, stride %lu, reps %lu, size of pointer chase %lu\n", s, stride, buffer_size, buffer_size/stride);
        probe_stride_loop(buffer, buffer_size);
        double millicycles = probe_stride_loop(buffer, PROBE_REPS);
        //printf("stride: %5d; time: %7.3f cycles\n", (1<<stride), millicycles);
        printf("%7ld %7.3f\n", 8*stride, (double) millicycles/(1<<5));
		
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

static uint64_t probe(void *adrs){
	volatile uint64_t time;  
	__asm__ volatile (
        " mfence            \n"
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

static double probe_stride_loop_c(void *addr, uint64_t reps) {
	if(reps==0) return 0.0f;
    uint64_t *tmp=addr;
	volatile uint64_t time=0;
    for(int i=0; i<reps;i++){
        time+=probe(tmp);
        tmp=(uint64_t *) *tmp;
    }
	return (double)time / (double)reps;
}



static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride) {
    for (uint64_t i = 0; i < size; i++) {
        addr[i] = NULL; // set all entries inn addr to NULL
    }
    uint64_t lfsr = lfsr_create(); // start random lfsr
    uint64_t offset, curr = 0; // offset = 0
    uint64_t stride_indexes = size % stride == 0? size/stride : size/stride +1; // calculate amount of entries when we jump through set with stride size stride
	uint64_t counter=0;
    // compute amount of entries with stride stride
    for (uint64_t i = 0; i < stride_indexes-1; i++) {
        do {
            offset = lfsr_rand(&lfsr) % size; // random number mod size 
        } while (offset == curr || addr[offset] != NULL || offset % stride != 0); // ensure that offset !=curr and addr[offset]==NULL and jumps only between entries of stride
        addr[curr] = &addr[offset]; // set the value of the curr index to the address at the offset index (linked list)
        curr = offset;
		counter+=curr;
    }
    addr[curr] = addr;

    //printf("create_pointer_stride_chase: set size stride_indexes %lu, counter %lu, stride %u entries size %lu\n", stride_indexes, counter, stride, size);
}
