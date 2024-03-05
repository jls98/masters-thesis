#define u64 uint64_t
#define i64 int64_t

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

typedef struct {
	u64 cache_ways;
	u64 cache_sets;
	u64 cacheline_size;
	u64 pagesize;
	u64 threshold_L1;
	u64 threshold_L2;
	u64 threshold_L3;
	size_t buffer_size;
} Config;

typedef struct {
	//Target *next;
	void *target_adrs;
} Target;

typedef struct {
	//Eviction_Set *next;
	Target *target;				
	void **evset_adrs; 			// size equals Config->cache_ways
	i64 *measurements; 				// measured values
	u64 cnt_measurement; 				// amount of measured values
} Eviction_Set;

// struct Config 
// input 0 for params cache_ways, pagesize, threshold_L1, threshold_L2, threshold_L3 in that order to apply default values
static Config * initConfig(u64 cache_ways, u64 pagesize, u64 cache_sets, u64 cacheline_size, u64 threshold_L1, u64 threshold_L2, u64 threshold_L3){
	Config *conf = malloc(sizeof(Config));
	// setting default values or apply input
	conf->cache_ways = (cache_ways != 0) ? cache_ways : 8;
	conf->pagesize = (pagesize != 0) ? pagesize : 4096;
	conf->cache_sets = (cache_sets != 0) ? cache_sets : 64; //  default, 32768 L1 cache
	conf->cacheline_size = (cacheline_size != 0) ? cacheline_size : 64;
	conf->threshold_L1 = (threshold_L1 != 0) ? threshold_L1 : 100;
	conf->threshold_L2 = (threshold_L2 != 0) ? threshold_L2 : 200;
	conf->threshold_L3 = (threshold_L3 != 0) ? threshold_L3 : 300;
	conf->buffer_size=0;
	return conf;
}

// struct Target 
static Target *initTarget(void *target_adrs){
	if (target_adrs == NULL){
		printf("initTarget: target_adrs is NULL!\n");
		return NULL; // avoid null pointers
	}
	Target *targ = malloc(sizeof(Target));
	targ->target_adrs=target_adrs;
	targ->next=NULL;
	return targ;
}

// struct Eviction_Set
static Eviction_Set * initEviction_Set(Target *target, Config *conf){
	if (target == NULL){
		printf("initEviction_Set: target is NULL!\n");
		return NULL;
	}
	Eviction_Set *evset = malloc(sizeof(Eviction_Set));
	evset->target=target;
	//evset->next=NULL;
	evset->evset_adrs=malloc(conf->cache_ways*sizeof(void *));
	evset->measurements=malloc(100000*sizeof(u64 *)); // amount of max measurement (if too small increase)
	evset->cnt_measurement=0;
	return evset;
}

/*void freeTarget (Target *targ){
    Target *current = targ;
    Target *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}*/


static void freeEviction_Set (Eviction_Set *evset){
    /*Target *current = evset;
    Target *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
	*/
	free(evset->measurements);
	free(evset);
}

// say hi to cache
static void wait(uint64_t cycles) {
	unsigned int ignore;
	uint64_t start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

static void load(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
}

static void flush(void *adrs){
	__asm__ volatile("clflush [%0];lfence" ::"r" (adrs));
}

// lfsr
#define FEEDBACK 0x80000000000019E2ULL
static uint64_t lfsr_create(void) {
  uint64_t lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  return lfsr;
}

static uint64_t lfsr_step(uint64_t lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

static uint64_t lfsr_rand(uint64_t* lfsr) {
    for (uint64_t i = 0; i < 8*sizeof(uint64_t); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

