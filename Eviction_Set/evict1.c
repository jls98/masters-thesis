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

#define DEF_OR_ARG(value,...) value
#define CREATE_POINTER_STRIDE_CHASE(addr, size, stride, ...) create_pointer_stride_chase(addr, size, stride, DEF_OR_ARG(__VA_ARGS__ __VA_OPT__(,) 0))

static uint64_t probe(const void *addr, const uint64_t reps, const uint64_t* cand);
static void wait(const uint64_t cycles);
static void control(uint64_t cache_size);
static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride, uint64_t ** indexes);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);


int main(int ac, char **av){
    control(atoi(av[1]));
}



static void control(uint64_t cache_size){
	
	wait(1E9);
	uint64_t part, not_part;
	int ar_size = cache_size;
	uint64_t candidate = 64;
	void* *buffer = mmap(NULL, ar_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	CREATE_POINTER_STRIDE_CHASE(buffer, ar_size, 1);
	part = probe(buffer, ar_size, buffer);
	not_part = probe(buffer, ar_size, &candidate);
	
	printf("part %lu\nnot part %lu\n%p\n%p\n", part, not_part, &part, &not_part);	
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

// create pointer chase over the valid indexes from addr in indexes where the amount of valid entries is size. indexes is optional
static void create_pointer_stride_chase(void** addr, const uint64_t size, const uint32_t stride, uint64_t ** indexes) {
    uint64_t lfsr = lfsr_create(); // start random lfsr
    uint64_t offset, curr = 0; // offset = 0
    uint64_t stride_indexes = size % stride == 0? size/stride : size/stride +1;
    uint64_t a=1;
	
	for (uint64_t i = 0; i < size; i++) {
        addr[i] = NULL; // set all entries inn addr to NULL
    }
    // compute amount of entries with stride stride
    for (uint64_t i = 0; i < stride_indexes-1; i++) {
        do {
            offset = lfsr_rand(&lfsr) % size; // random number mod size 
        } while (offset == curr || addr[offset] != NULL /*|| offset % stride != 0*/); // ensure that offset !=curr and addr[offset]==NULL and jumps only between entries of stride, entries NULL initialized
        a+=1;
		addr[curr] = &addr[offset]; // set the value of the curr index to the address at the offset index (linked list)
        curr = offset;
    }
	addr[curr] = &a;
    addr[curr] = addr;
}
