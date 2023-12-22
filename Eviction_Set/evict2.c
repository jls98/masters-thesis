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

static uint64_t probe(const void *addr, const uint64_t reps, const void* cand);
static void wait(const uint64_t cycles);
static void control(uint64_t cache_size);
static void create_pointer_stride_chase(void** addr, const uint64_t size_addr, uint64_t *indexes, const uint64_t size_indexes);
static uint64_t lfsr_create(void);
static uint64_t lfsr_rand(uint64_t* lfsr);
static uint64_t lfsr_step(uint64_t lfsr);
static int contains(uint64_t * indexes, const uint64_t size_indexes, uint64_t offset);

int main(int ac, char **av){
    if (ac==2) control(atoi(av[1]));
	else control(200);
}

static void control(uint64_t cache_size){
	
	wait(1E9);
	int lines_in_bytes = cache_size;
	int lines_indexes = lines_in_bytes / sizeof(void*);
	
	void* *buffer=mmap(NULL, lines_in_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	
	uint64_t* conflict_set=(uint64_t*) malloc(sizeof(uint64_t) * lines_indexes); // empty conflict set, will not be completely filled
	uint64_t conflict_set_count=0;
	
	// foreach candidate from buffer do probe conflict set and candidate 
	
	printf("buffer %p\n", buffer);
	// steps:
	// create pointer chase between all entries in conflict set 
	// probe conflict_set and candidate 
	// maybe add candidate index to conflict set 
	for(uint64_t i=0; i < lines_indexes-1; i++){
		// create pointer chase between all entries in conflict set 
		printf("currently at i = %lu\n", i);
		create_pointer_stride_chase(buffer, lines_indexes, conflict_set, conflict_set_count);
		// buffer contains a pointer chase over the entries of the conflict set, other entries are empty and not pointed at
		printf("pointer chase created\n");
		
		for(uint64_t j=0;j<=i;j++){
			printf("addr %p, index %lu, j %lu\n", buffer[conflict_set[j]], conflict_set[j]);
		}
		if (probe(buffer, lines_indexes, buffer[i]) < THRESHOLD){ // probe if candidate is cached or evicted by conflict set
			// insert candidate to conflict set if conflict set cannot evict candidate
			conflict_set[conflict_set_count]=i;
			conflict_set_count+=1;
		}
		
	}
	printf("conflict set count %lu\n", conflict_set_count);
	
	/*
	create_pointer_stride_chase(buffer, lines_indexes, conflict_set, conflict_set_count);

	part = probe(buffer, lines_indexes, buffer);
	not_part = probe(buffer, lines_indexes, &candidate);
	
	printf("part %lu\nnot part %lu\n%p\n%p\n", part, not_part, &part, &not_part);	*/
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
static uint64_t probe(const void *addr, const uint64_t reps, const void* cand) {
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

static int contains(uint64_t * indexes, const uint64_t size_indexes, uint64_t offset){
	for (uint64_t i=0;i<size_indexes;i++){
		if (indexes[i] == offset) return 1; // contained in indexes
	
	}
	return 0; // no match found
}

// create pointer chase over the valid indexes from addr in indexes where the amount of valid entries is size. 
static void create_pointer_stride_chase(void** addr, const uint64_t size_addr, uint64_t *indexes, const uint64_t size_indexes) {
	uint64_t lfsr = lfsr_create(); // start random lfsr
    uint64_t offset, curr = 0; // offset = 0
	
	for (uint64_t i = 0; i < size_addr; i++) {
		addr[i] = NULL; // set all entries in addr to NULL
	}
	// compute entries, only fill size_indexes many entries with index included in indexes respectively
	for (uint64_t i = 0; i < size_indexes; i++) {
		do {
			offset = lfsr_rand(&lfsr) % size_addr; // random number mod size 
		} while (offset == curr || addr[offset] != NULL || contains(indexes, size_indexes, offset)); // ensure that offset !=curr and addr[offset]==NULL and included in indexes
		addr[curr] = &addr[offset]; // set the value of the curr index to the address at the offset index (linked list)
		
		printf("offset %lu, curr %lu, addr[curr] %p, &addr[offset] %p, addr %p, i %lu\n", offset, curr, addr[curr], &addr[offset], addr, i);
		curr = offset;
		
	}
	printf("curr %lu\n");
	addr[curr] = addr;
	
	
	
    
}
