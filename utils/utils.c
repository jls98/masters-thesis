#define u64 uint64_t
#define i64 int64_t

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>
#include <string.h>

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
	void **adrs; 			// size equals Config->cache_ways
	u64 size;
	u64 max_size;
	i64 *measurements; 				// measured values
	u64 cnt_measurement; 				// amount of measured values
} Eviction_Set;

static u64 rng;
// lfsr
#define FEEDBACK 0x80000000000019E2ULL
static u64 lfsr_create(void) {
  u64 lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  return lfsr;
}

static u64 lfsr_step(u64 lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

static u64 lfsr_rand(u64* lfsr) {
    for (u64 i = 0; i < 8*sizeof(u64); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}




// struct Config 
// input 0 for params cache_ways, pagesize, threshold_L1, threshold_L2, threshold_L3 in that order to apply default values
static Config * initConfig(i64 cache_ways, i64 pagesize, i64 cache_sets, i64 cacheline_size, i64 threshold_L1, i64 threshold_L2, i64 threshold_L3, i64 buffer_size){
	Config *conf = malloc(sizeof(Config));
	// setting default values or apply input
	conf->cache_ways = (cache_ways != -1) ? cache_ways : 8;
	conf->pagesize = (pagesize != -1) ? pagesize : 4096;
	conf->cache_sets = (cache_sets != -1) ? cache_sets : 64; //  default, 32768 L1 cache
	conf->cacheline_size = (cacheline_size != -1) ? cacheline_size : 64;
	conf->threshold_L1 = (threshold_L1 != -1) ? threshold_L1 : 100;
	conf->threshold_L2 = (threshold_L2 != -1) ? threshold_L2 : 200;
	conf->threshold_L3 = (threshold_L3 != -1) ? threshold_L3 : 300;
	conf->buffer_size= (buffer_size != -1) ? buffer_size : 8*4096;;
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
	// targ->next=NULL;
	return targ;
}

// struct Eviction_Set
static Eviction_Set *initEviction_Set(Config *conf){
	Eviction_Set *evset = malloc(2*conf->cache_ways*sizeof(Eviction_Set));
	//evset->next=NULL;
	evset->adrs=malloc(conf->cache_ways*sizeof(void *));
	evset->measurements=malloc(100000*sizeof(u64 *)); // amount of max measurement (if too small increase)
	evset->cnt_measurement=0;
	evset->size =0;
	evset->max_size=conf->cache_ways;
	rng=lfsr_create();
	return evset;
}

static void addEvictionAdrs(Eviction_Set *evset, void *evset_adrs){
	if (evset == NULL){
		printf("addEvictionAdrs: evset NULL!\n");
		return;
	}
	if (evset->size < evset->max_size) evset->adrs[evset->size++] = evset_adrs;	
	else printf("addEvictionAdrs: evset exceeding evset_max_size %lu elements!\n", evset->max_size);
}

static void createPointerChaseInEvictionSet(Eviction_Set *evset){
	if (evset==NULL){ // no evset object
		printf("createPointerChaseInEvictionSet: evset is NULL!\n");
		return;
	}
	if(evset->size==0){ // no eviction set adresses
		return;
	}
	if (evset->adrs==NULL){ // no eviction set adresses
		printf("createPointerChaseInEvictionSet: evset->adrs is NULL but size is not 0!\n");
		return;
	}
	
	// marker array with all indexes
	u64 counter=1;
	int *marker=calloc(evset->size, sizeof(int));
	// memset(marker, 0, evset->size*sizeof(int));
	u64 index_next=rng%evset->size, index_current=0;
	u64 *pointer_current= (u64 *)evset->adrs[index_current], pointer_next= (u64 *)evset->adrs[index_next];
	marker[0]=1;
	while(counter!=evset->size){ // repeat until all evset adrs are set
		printf("picked next index %lu\n", index_next);
		// set adrs
		*pointer_current=(u64 *)pointer_next; // set current pointer value to next adrs
		
		// mark current index
		marker[index_next] =1;
		counter++;

		// all set, next block
		pointer_current=pointer_next;
		index_current=index_next;
		
		// pick next index at random w lfsr
		do{		
			index_next = lfsr_rand(&rng) % evset->size; // rng is set to next random number in lfsr, mod amount of adrs in evset 
		}while(counter!=evset->size && marker[index_next]); // do while not all adrs are picked yet and next index had not been picked yet
		pointer_next =(u64 *)evset->adrs[index_next];
	}
	// apply pointer from last index to 0 
	*pointer_current=(u64 *)evset->adrs[0];
	for(int i=0; i<evset->size;i++){
		printf("marker[%i]=%i\n", i, marker[i]);
	}
	printf("counter %lu\n", counter);
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

