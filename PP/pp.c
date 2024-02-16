#include "../utils/utils.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

typedef struct {
	u64 cache_ways;
	u64 pagesize;
	u64 threshold_L1;
	u64 threshold_L2;
	u64 threshold_L3;
} Config;

typedef struct {
	Target *next;
	void *target_adrs;
} Target;

typedef struct {
	Eviction_Set *next;
	Target *target;				
	void **evset_adrs; 			// size equals Config->cache_ways
	u64 **measurements; 				// measured values
	u64 cnt_measurement; 				// amount of measured values
} Eviction_Set;


// struct Config 
// input 0 for params cache_ways, pagesize, threshold_L1, threshold_L2, threshold_L3 in that order to apply default values
Config *initConfig(u64 cache_ways, u64 pagesize, u64 threshold_L1, u64 threshold_L2, u64 threshold_L3){
	Config *conf = malloc(sizeof(Config));
	// setting default values or apply input
	conf->cache_ways = (cache_ways != 0) ? cache_ways : 8;
	conf->pagesize = (pagesize != 0) ? pagesize : 4096;
	conf->threshold_L1 = (threshold_L1 != 0) ? threshold_L1 : 100;
	conf->threshold_L2 = (threshold_L2 != 0) ? threshold_L2 : 200;
	conf->threshold_L3 = (threshold_L3 != 0) ? threshold_L3 : 300;
	return conf;
}

// struct Target 
Target *initTarget(void *target_adrs){
	if (target_adrs == NULL){
		printf("initTarget: target_adrs is NULL!\n");
		return NULL;
	}
	Target *targ = malloc(sizeof(Target));
	targ->target_adrs=target_adrs;
	targ->next=NULL;
	return targ;
}

void freeTarget (Target *targ){
    Target *current = targ;
    Target *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

// struct Eviction_Set
Eviction_Set *initEviction_Set(Target *target){
	if (target == NULL){
		printf("initEviction_Set: target is NULL!\n");
		return NULL;
	}
	Eviction_Set *evset = malloc(sizeof(Eviction_Set));
	evset->target=target;
	evset->next=NULL;
	evset->evset_adrs=NULL;
	evset->measurements=NULL;
	evset->cnt_measurement=0;
	return evset;
}

void freeEviction_Set (Eviction_Set *evset){
    Target *current = evset;
    Target *next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

void *pp_init() {
	// Implement
	// allocate 256 different cache lines on differenz mem pages
	size_t total_size = 256 * 4096; // 256 cache lines, 4096 bytes apart (mem pages)
	void *cc_buffer = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if (ptr == MAP_FAILED) {
        perror("cc_init: mmap failed");
        return;
	}
	return cc_buffer;
}

void pp_setup(Eviction_Set *evset, Config *conf) {
	if(evset==NULL){
		printf("pp_setup: evset is NULL!\n");
		return;
	}
	
	if(conf==NULL){
		printf("pp_setup: conf is NULL!\n");
		return;
	}
	
	// create eviction set
	for(u64 i=0;i<conf->cache_ways;i++){
		Eviction_Set->
	}
}

void pp_monitor(Eviction_Set *evset) {
	
}

void pp_run(void *target_adrs) { // atm support only 1 adrs, extend later (easy w linked list)
	if (target_adrs==NULL){
		printf("pp_run: target_adrs is NULL!\n");
		return;
	}
	void *cc_buffer=pp_init();
	
	// setup structs
	Target *targ=initTarget(target_adrs);
	
	Eviction_Set *evset=initEviction_Set(targ);
	
	pp_setup();
	pp_monitor();
}


int main(){
	Config *conf=initConfig(0,0,0,0,0);
	pp_run(conf); // TODO change to real adrs and create a temp picker (file reader)
}