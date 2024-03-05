#include "../utils/utils.c"

// utils 
// measure time 

static i64 pp_probe(Eviction_Set *evset){
	if (evset==NULL){
		printf("probe: evset is NULL!\n");
		return -1;
	}
	printf("check\n");
	printf("check evset->cnt_measurement %lu\n", evset->cnt_measurement);
	printf("check evset->measurements[evset->cnt_measurement] %p\n", &evset->measurements[evset->cnt_measurement]);
	
	__asm__ volatile (
        " mfence            \n"
        " rdtscp             \n" // start time 
        " mov r8, rax 		\n" // move time to r8 
        " mov rax, [%1]		\n" // load target adrs 
        " lfence            \n" 
        " rdtscp             \n" // end time 
        " sub rax, r8 		\n" // diff = end - start
        : "=&a" (evset->measurements[evset->cnt_measurement])
        : "r" (evset) // TODO change
        : "ecx", "rdx", "r8", "memory"
	);
	printf("probe donew %lu\n", evset->measurements[evset->cnt_measurement]);
	return evset->measurements[evset->cnt_measurement++];
}


static void *pp_init(Config *conf) {
	// Implement
	// allocate 256 different cache lines on differenz mem pages
	if (conf == NULL){
		printf("pp_init: conf NULL!\n");
		return NULL;
	}
	//conf->buffer_size = 258 * 4096; // 256 cache lines, 4096 bytes apart (mem pages)
	void *cc_buffer = mmap(NULL, conf->buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	if (cc_buffer == MAP_FAILED) {
        printf("cc_init: mmap failed");
        return NULL;
	}
	return cc_buffer;
}

static void pp_setup(Config *conf, Eviction_Set *evset, void *target) {
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
		addEvictionAdrs(evset, target+(i+1)*conf->pagesize); // void * -> exakt index value
		// printf("pp_setup: added addr %p at position %p\n", evset->evset_adrs[i], &evset->evset_adrs[i]); // works
	}
}

static void pp_monitor(Config *conf, Eviction_Set *evset, void *target) {
	
	load(target);
	printf("probe is %li\n", pp_probe(evset));
	printf("probe is %li\n", pp_probe(evset));
	printf("probe is %li\n", pp_probe(evset));
}

static void pp_run(void *target_adrs, Config *conf, void *target) { // atm support only 1 adrs, extend later (easy w linked list)
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
	
	Eviction_Set *evset=initEviction_Set(conf);
	
	pp_setup(evset, conf, target);

	pp_monitor(evset, conf, target);
	
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