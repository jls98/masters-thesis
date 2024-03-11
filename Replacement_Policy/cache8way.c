#include "../utils/utils.c"

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

void test_L1_cache(){
	Config *conf=initConfig_D;
	void *buffer = mmap(NULL, 10*conf->buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	Eviction_Set *evset = initEviction_Set(conf);
	Eviction_Set *target_set = initEviction_Set(conf);
	
	for(int i=0;i<8;i++){
		addEvictionAdrs(evset, buffer+(8+i)*conf->pagesize); // eviction set 
		addEvictionAdrs(target_set, buffer+i*conf->pagesize); // target set 
	}
	createPointerChaseInEvictionSet(evset);
	target_set->size =1; // case 1 target
	createPointerChaseInEvictionSet(target_set);
	
	// clean
	for(int i=0;i<8;i++) {
		flush(target_set->adrs[i]);
		flush(evset->adrs[i]);
	}
	my_fence();
	
	// load evset, load target, test each evset member
	u64 time;
	
	printf("No element loaded from target set:\n");
	for(int i=0;i<8;i++){
		my_fence();
		pp_probe(evset); // load all evset elems
		my_fence();
		time = probe(evset->adrs[i]);
		my_fence();
		printf("Time of element %i is %lu\n", i, time);
	}
	
	
	printf("1 element loaded from target set:\n");
	for(int i=0;i<8;i++){
		my_fence();
		pp_probe(evset); // load all evset elems
		my_fence();
		pp_probe(target_set); // load "1" target adrs 
		my_fence();
		time = probe(evset->adrs[i]);
		my_fence();
		printf("Time of element %i is %lu\n", i, time);
	}
	
	// repeat 
	
	// load evset, load target, load another target test each evset member
	
	// repeat 
	
	// load evset, load 3 targets, test each evset member

}


int main(){
	test_L1_cache();
}