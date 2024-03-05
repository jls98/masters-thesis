#include "../utils/utils.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>



// utils 
// measure time 

i64 probe(Eviction_Set *evset){
	if (evset==NULL){
		printf("probe: evset is NULL!\n");
		return -1;
	}
	printf("check\n");
	printf("check Target %p \n", evset->target);
	printf("check void adrs %p\n", evset->target->target_adrs);
	printf("check evset->cnt_measurement %lu\n", evset->cnt_measurement);
	printf("check evset->measurements[evset->cnt_measurement] %p\n", &evset->measurements[evset->cnt_measurement]);
	
	__asm__ volatile (
        " mfence            \n"
        " rdtscp             \n"
        " mov r8, rax 		\n"
        " mov rax, [%1]		\n"
        " lfence            \n"
        " rdtscp             \n"
        " sub rax, r8 		\n"
        : "=&a" (evset->measurements[evset->cnt_measurement])
        : "r" (evset->target->target_adrs)
        : "ecx", "rdx", "r8", "memory"
	);
	printf("probe donew %lu\n", evset->measurements[evset->cnt_measurement]);
	return evset->measurements[evset->cnt_measurement++];
}


void *pp_init(Config *conf) {
	// Implement
	// allocate 256 different cache lines on differenz mem pages
	conf->buffer_size = 256 * 4096; // 256 cache lines, 4096 bytes apart (mem pages)
	void *cc_buffer = mmap(NULL, conf->buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if (cc_buffer == MAP_FAILED) {
        perror("cc_init: mmap failed");
        return NULL;
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
		// add multiple of pagesize to ensure to land in the same cache set
		evset->evset_adrs[i]=evset->target->target_adrs+(i+1)*conf->pagesize; // void * -> exakt index value
		// printf("pp_setup: added addr %p at position %p\n", evset->evset_adrs[i], &evset->evset_adrs[i]); // works
	}
}

void pp_monitor(Eviction_Set *evset, Config *conf) {
	printf("p\n");	
	Target *targ = evset->target;
	printf("p %p\n", targ);
	void *t_adrs=targ->target_adrs;
	printf("p %p\n", t_adrs);

	printf("probe is %li\n", probe(evset));
	printf("probe is %li\n", probe(evset));
	printf("probe is %li\n", probe(evset));
}

void pp_run(void *target_adrs, Config *conf) { // atm support only 1 adrs, extend later (easy w linked list)
	if (target_adrs==NULL){
		printf("pp_run: target_adrs is NULL!\n");
		return;
	}
	void *cc_buffer=pp_init(conf);
	if (cc_buffer==NULL){
		printf("pp_run: buffer could nnot be initialized!\n");
		return;
	}
	// setup structs
	Target *targ=initTarget(target_adrs);
	
	Eviction_Set *evset=initEviction_Set(targ, conf);
	
	pp_setup(evset, conf);

	pp_monitor(evset, conf);
	
	munmap(cc_buffer, conf->buffer_size);
}

#ifdef PP
int main(){
	Config *conf=initConfig(0,0,0,0,0,0,0);
	void *target = malloc(sizeof(void));
	pp_run(target, conf); // TODO change to real adrs and create a temp picker (file reader)
	
	
	
	
	free(conf);
}

#endif