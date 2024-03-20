#include "../utils/utils.c"

// utils 
// measure time 

static void pp_load(Eviction_Set *evset){
    if (evset==NULL){
		printf("probe: evset is NULL!\n");
		return -1;
	}

	__asm__ volatile (
		" mov rbx, %1\n" // 
        " mfence            \n"
		" mov rax, [%0]		\n" // load target adrs
        " loop0: lfence;"		
		" mov rax, [rax]\n" // pointer chase
		" dec rbx\n"
		" lfence\n"
		" jnz loop0\n"
        " lfence            \n" 
        : 
        : "r" (evset->adrs), "r" (evset->size) 
        : "rax", "rbx", "memory"
	);
}

static i64 pp_probe1(Eviction_Set *evset){
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

static i64 pp_probe_simple(Eviction_Set *evset){
	unsigned int ignore;
    
    u64 *adrs=(u64 *)evset->adrs[0];
    my_fence();
    u64 start = __rdtscp(&ignore);
    // for (int i=0;i<evset->size;i++){
        // adrs=*adrs;
        // my_fence();
    // }
    for (int i=0;i<evset->size;i++){
        load(evset->adrs[i]);
        my_fence();
    }
    u64 end = __rdtscp(&ignore);
    u64 diff = end-start;
    evset->measurements[evset->cnt_measurement++]=diff;
    return diff;
}

static i64 pp_probe(Eviction_Set *evset){
    i64 ret = pp_probe_simple(evset);
    my_fence();
    return ret;
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

static void pp_setup(Config *conf, Eviction_Set *evset, void *target, void *cc_buffer) {
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
    intptr_t buf = (intptr_t) cc_buffer;
    intptr_t tar = (intptr_t) target;    
    i64 offset = 4096 - (abs(((i64) cc_buffer) - ((i64) target)) % 4096) + 10*4096;
    // printf("computed offset %li from buf %p and tar %p\n", offset, buf, tar);
	for(u64 i=0;i<conf->cache_ways;i++){
		// add multiple of pagesize to ensure to land in the same cache set
		addEvictionAdrs(evset, cc_buffer+offset+(i+1)*conf->pagesize); // void * -> exakt index value
		// printf("pp_setup: added addr %p at position %p\n", evset->evset_adrs[i], &evset->evset_adrs[i]); // works
	}
	createPointerChaseInEvictionSet(evset);
}

static void pp_monitor(Config *conf, Eviction_Set *evset, void *target) {
    i64 tar_buf[100];
    // // load evset (memory or hard drive)
    // pp_load(evset);
    // probe evset (cached)
    pp_probe(evset); // evset 1
    // probe evset (cached)
    pp_probe(evset); // evset 2
    // load target (memory or hard drive)
    tar_buf[0] = load(target);
    // probe evset (cached)
    pp_probe(evset); // evset 3
    // probe evset (cached)
    pp_probe(evset); // evset 4
    // probe target (L2/L3)
    tar_buf[1] = probe(target);
    // probe target (cached)
    tar_buf[2] = probe(target);
    // probe evset (cached)
    pp_probe(evset); // evset 5
    // probe evset (cached)
    pp_probe(evset); // evset 6
    
    my_fence();
    for(int i=0;i<3;i++){
        printf("Target %i: %li\n", i, tar_buf[i]);
    }
    for (int i=0;i<6;i++){
        printf("Evset %i: %li\n", i, evset->measurements[i]);
    }
    
    printf("Target adrs %p\n", target);
    printf("Evset adrs:\n");
    for(int i=0;i<evset->size;i++){
        printf("%p\n", evset->adrs[i]);
    }


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
	
	pp_setup(conf, evset, cc_buffer, cc_buffer+4096);

	pp_monitor(conf, evset, cc_buffer);
	
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