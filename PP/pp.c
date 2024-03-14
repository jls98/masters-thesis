#include "../utils/utils.c"

// utils 
// measure time 

static i64 pp_probe(Eviction_Set *evset){
	if (evset==NULL){
		printf("probe: evset is NULL!\n");
		return -1;
	}

	__asm__ volatile (
		" mov r9, %2\n" // 
        " mfence            \n"
        " rdtscp             \n" // start time 
        " mov r8, rax 		\n" // move time to r8 
		" mov rax, [%1]		\n" // load target adrs
        " loop: lfence;"		
		" mov rax, [rax]\n" // pointer chase
		" dec r9\n"
		" lfence\n"
		" jnz loop\n"
        " lfence            \n" 
        " rdtscp             \n" // end time 
        " sub rax, r8 		\n" // diff = end - start
        : "=&a" (evset->measurements[evset->cnt_measurement])
        : "r" (evset->adrs), "r" (evset->size) 
        : "ecx", "rdx", "r8", "r9", "memory"
	);
	return evset->measurements[evset->cnt_measurement++];
}

static i64 pp_probe2(Eviction_Set *evset){
	if (evset==NULL){
		printf("probe: evset is NULL!\n");
		return -1;
	}

	__asm__ volatile (
		" mov r9, %3\n" // 
        " mfence            \n"
        " rdtscp             \n" // start time 
        " mov r8, rax 		\n" // move time to r8 
		" mov rax, [%2]		\n" // load target adrs
        " loop1: lfence;"		
		" mov rax, [rax]\n" // pointer chase
		" dec r9\n"
		" lfence\n"
		" jnz loop1\n"
        " lfence            \n" 
        " rdtscp             \n" // end time 
        " sub rax, r8 		\n" // diff = end - start
        " mov rbx, rax;"
        
        " mov r9, %3;" // load two more times
        " add r9, r9;"
        " mov rax, [%2];"
        " loop2: lfence;"
        " mov rax, [rax];"
        " dec r9;"
        " jne loop2;"
        " lfence;"
        " mov r9, %3;"
        " mfence;"
        " rdtscp;"
        " mov r8, rax;"
        " mov rax, [%2];"
        " loop3: lfence;"
        " mov rax, [rax];"
        " dec r9;"
        " lfence;"
        " jne loop3;"
        " lfence;"
        " rdtscp;"
        " sub rax, r8;"   
        
        : "=&b" (evset->measurements[evset->cnt_measurement++]), "=&a" (evset->measurements[evset->cnt_measurement])
        : "r" (evset->adrs), "r" (evset->size)
        : "ecx", "rdx", "r8", "r9", "memory"
	);
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
        printf("pp_init: mmap failed\n");
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
	
	if(target==NULL){
		printf("pp_setup: target is NULL!\n");
		return;
	}
	
	// create eviction set / TODO dont hardcode
	for(u64 i=0;i<conf->cache_ways;i++){
		// add multiple of pagesize to ensure to land in the same cache set
		addEvictionAdrs(evset, target+(i+1)*conf->pagesize); // void * -> exakt index value
		// printf("pp_setup: added addr %p at position %p\n", evset->evset_adrs[i], &evset->evset_adrs[i]); // works
	}
	createPointerChaseInEvictionSet(evset);
}

static void pp_monitor(Config *conf, Eviction_Set *evset, void *target) {
	
	printf("probe is %li\n", pp_probe(evset));
	printf("probe is %li\n", pp_probe(evset));
	printf("probe is %li\n", pp_probe(evset));
	load(target);
	printf("probe target is %li\n", probe(target));
	printf("probe is %li\n", pp_probe(evset));
	printf("probe target is %li\n", probe(target));
    
	printf("probe is %li\n", pp_probe(evset));
}

static void pp_run(Config *conf, void *target_adrs) { // atm support only 1 adrs, extend later (easy w linked list)
	if (target_adrs==NULL){
		printf("pp_run: target_adrs is NULL!\n");
		return;
	}
	void *cc_buffer=pp_init(conf);
	if (cc_buffer==NULL){
		printf("pp_run: buffer could not be initialized!\n");
		return;
	}
	// setup structs
	// Target *targ=initTarget(target_adrs);
	
	Eviction_Set *evset=initEviction_Set(conf);
	
	pp_setup(conf, evset, target_adrs);

	pp_monitor(conf, evset, target_adrs);
	
	munmap(cc_buffer, conf->buffer_size);
}

#ifdef PP
int main(){
	Config *conf=initConfig(-1,-1,-1,-1,-1,-1,-1, -1);
	void *target = malloc(sizeof(void));
    wait(1E9);
	pp_run(conf, target); // TODO change to real adrs and create a temp picker (file reader)
	
	
	
	
	free(conf);
}

#endif