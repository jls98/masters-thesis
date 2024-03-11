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

#define TEST_REPS 100
#define OUTSIDER_TRESHOLD 1000
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

	wait(1e9);
	load(target_set->adrs[0]);
	my_fence();
	printf("Time loading cached target at %p takes %lu\n", target_set->adrs[0], probe(target_set->adrs[0]));
	my_fence();
	load(evset->adrs[0]);
	load(evset->adrs[1]);
	load(evset->adrs[2]);
	load(evset->adrs[3]);
	load(evset->adrs[4]);
	load(evset->adrs[5]);
	load(evset->adrs[6]);
	load(evset->adrs[7]);
	my_fence();
	printf("Time loading target at %p after evset takes %lu\n", target_set->adrs[0], probe(target_set->adrs[0]));
	// clean
	for(int i=0;i<8;i++) {
		flush(target_set->adrs[i]);
		flush(evset->adrs[i]);
	}
	
	my_fence();
	
	// load evset, load target, test each evset member
	u64 time;
	
	for(int i=0;i<8;i++){ // empty measurements
		evset->measurements[i]=0;
	}	

	
	// printf("\nNo element loaded from target set with pp probe:\n");
	// for (int j=0;j<TEST_REPS;j++){
		
		// for(int i=0;i<8;i++) {
			// flush(target_set->adrs[i]);
			// flush(evset->adrs[i]);
		// }
		
		// my_fence();
	
		// for(int i=0;i<8;i++){
			// my_fence();
			// pp_probe(evset); // load all evset elems
			// my_fence();
			// do{
				// time = probe(evset->adrs[i]);
			// } while(time > OUTSIDER_TRESHOLD);			
			// my_fence();
			// evset->measurements[i] += time;
			// my_fence();
			// // printf("Time of element %i is %lu\n", i, time);
		// }
	// }
	
	// for(int i=0;i<8;i++){ // print and reset measurements
		// printf("Time sum of element %i is %lu, avg per iteration is %lu\n", i, evset->measurements[i], evset->measurements[i]/TEST_REPS);
		// evset->measurements[i]=0;
	// }	
	
	printf("\nNo element loaded from target set with load:\n");
	for (int j=0;j<TEST_REPS;j++){
		
		for(int i=0;i<8;i++) {
			flush(target_set->adrs[i]);
			flush(evset->adrs[i]);
		}
		
		my_fence();
	
		for(int i=0;i<8;i++){
			my_fence();
			// load all evset elems
			load(evset->adrs[0]);
			my_fence();
			load(evset->adrs[1]);
			my_fence();
			load(evset->adrs[2]);
			my_fence();
			load(evset->adrs[3]);
			my_fence();
			load(evset->adrs[4]);
			my_fence();
			load(evset->adrs[5]);
			my_fence();
			load(evset->adrs[6]);
			my_fence();
			load(evset->adrs[7]);
			my_fence();
			do{
				time = probe(evset->adrs[i]);
			} while(time > OUTSIDER_TRESHOLD);			
			my_fence();
			evset->measurements[i] += time;
			my_fence();
			// printf("Time of element %i is %lu\n", i, time);
		}
	}
	
	for(int i=0;i<8;i++){ // print and reset measurements
		printf("Time sum of element %i is %lu, avg per iteration is %lu\n", i, evset->measurements[i], evset->measurements[i]/TEST_REPS);
		evset->measurements[i]=0;
	}	
	
	printf("\n1 element loaded from target set:\n");
	for(int j=0;j<TEST_REPS;j++){
		
		for(int i=0;i<8;i++) {
			flush(target_set->adrs[i]);
			flush(evset->adrs[i]);
		}
		my_fence();
		for(int i=0;i<8;i++){
			my_fence();
			pp_probe(evset); // load all evset elems
			my_fence();
			pp_probe(target_set); // load "1" target adrs 
			my_fence();
			do{
				time = probe(evset->adrs[i]);
			} while(time > OUTSIDER_TRESHOLD);			
			my_fence();
			evset->measurements[i] += time;
			my_fence();
			
			// printf("Time of element %i is %lu\n", i, time);
		}
	}
	for(int i=0;i<8;i++){ // print and reset measurements
		printf("Time sum of element %i is %lu, avg per iteration is %lu\n", i, evset->measurements[i], evset->measurements[i]/TEST_REPS);
		evset->measurements[i]=0;
	}	
	
	printf("\n1 element loaded from loaded target set:\n");
	for(int j=0;j<TEST_REPS;j++){
		
		for(int i=0;i<8;i++) {
			flush(target_set->adrs[i]);
			flush(evset->adrs[i]);
		}
		my_fence();
		for(int i=0;i<8;i++){
			my_fence();
			pp_probe(evset); // load all evset elems
			my_fence();
			load(target_set->adrs[0]); // load "1" target adrs 
			my_fence();
			do{
				time = probe(evset->adrs[i]);
			} while(time > OUTSIDER_TRESHOLD);			
			my_fence();
			evset->measurements[i] += time;
			my_fence();
			
			// printf("Time of element %i is %lu\n", i, time);
		}
	}
	for(int i=0;i<8;i++){ // print and reset measurements
		printf("Time sum of element %i is %lu, avg per iteration is %lu\n", i, evset->measurements[i], evset->measurements[i]/TEST_REPS);
		evset->measurements[i]=0;
	}	
	// repeat 
	
	// load evset, load target, load another target test each evset member
	
	// repeat 
	
	// load evset, load 3 targets, test each evset member

}


int main(){
	test_L1_cache();
}