#include "../utils/utils.c"

static i64 probe_evset(Eviction_Set *evset){
	if (evset==NULL){
		printf("probe: evset is NULL!\n");
		return -1;
	}

	__asm__ volatile (
		" mov r9, %2\n" // 
		" mov rbx, [%1]		\n" // load target adrs
        " mfence            \n"
        " rdtscp             \n" // start time 
        " mov r8, rax 		\n" // move time to r8 
        " loop: lfence;"		
		" mov rbx, [rbx]\n" // pointer chase
		" dec r9\n"
		" jnz loop\n"
        " mfence            \n" 
        " rdtscp             \n" // end time 
        " sub rax, r8 		\n" // diff = end - start
        : "=&a" (evset->measurements[evset->cnt_measurement])
        : "r" (evset->adrs), "r" (evset->size) 
        : "ecx", "rbx", "rdx", "r8", "r9", "memory"
	);
	return evset->measurements[evset->cnt_measurement++];
}

void void_voidptr_fence(void (*foo)(void *), void *adrs){
	foo(adrs);
	my_fence();
}

u64 u64_voidptr_fence(u64 (*foo)(void *), void *adrs){
	u64 ret = (*foo)(adrs);
	my_fence();
	return ret;	
}

i64 i64_evset_fence(i64(*foo)(Eviction_Set *), Eviction_Set *evset){
	i64 ret = foo(evset);
	my_fence();
	return ret;
}

#define TEST_REPS 100
#define OUTSIDER_TRESHOLD 1000
void test_pointer_chase(Eviction_Set *evset, Eviction_Set *cleanup_evset, Eviction_Set *target_set){
	i64_evset_fence(probe_evset, target_set);
	i64_evset_fence(probe_evset, evset);
	i64_evset_fence(probe_evset, cleanup_evset);
	
	void_voidptr_fence(load, target_set->adrs[0]);
	u64 time = 	u64_voidptr_fence(probe, target_set->adrs[0]);
	printf("reload target %p with time %lu\n", target_set->adrs[0], time);
	// void_voidptr_fence(load, evset->adrs[0]);
	// void_voidptr_fence(load, evset->adrs[1]);
	// void_voidptr_fence(load, evset->adrs[2]);
	// void_voidptr_fence(load, evset->adrs[3]);
	// void_voidptr_fence(load, evset->adrs[4]);
	// void_voidptr_fence(load, evset->adrs[5]);
	// void_voidptr_fence(load, evset->adrs[6]);
	// void_voidptr_fence(load, evset->adrs[7]);
	i64_evset_fence(probe_evset, evset);
	time = 	u64_voidptr_fence(probe, target_set->adrs[0]);
	printf("load target %p after load evset with time %lu\n", target_set->adrs[0], time);
	
	void_voidptr_fence(load, evset->adrs[0]);
	time = 	u64_voidptr_fence(probe, evset->adrs[0]);
	printf("reload target %p with time %lu\n", evset->adrs[0], time);
	// void_voidptr_fence(load, cleanup_evset->adrs[0]);
	// void_voidptr_fence(load, cleanup_evset->adrs[1]);
	// void_voidptr_fence(load, cleanup_evset->adrs[2]);
	// void_voidptr_fence(load, cleanup_evset->adrs[3]);
	// void_voidptr_fence(load, cleanup_evset->adrs[4]);
	// void_voidptr_fence(load, cleanup_evset->adrs[5]);
	// void_voidptr_fence(load, cleanup_evset->adrs[6]);
	// void_voidptr_fence(load, cleanup_evset->adrs[7]);	
	i64_evset_fence(probe_evset, cleanup_evset);
	time = 	u64_voidptr_fence(probe, evset->adrs[0]);
	printf("load target %p after load evset with time %lu\n", evset->adrs[0], time);	
	
	i64_evset_fence(probe_evset, cleanup_evset);
	i64_evset_fence(probe_evset, cleanup_evset);
	time = 	u64_voidptr_fence(probe, evset->adrs[0]);
	printf("load target %p after load evset with time %lu\n", evset->adrs[0], time);	

}

void test_only_evset(Eviction_Set *evset, Eviction_Set *cleanup_evset){
	my_fence();
	i64_evset_fence(probe_evset, evset);
	i64_evset_fence(probe_evset, evset);

	i64_evset_fence(probe_evset, cleanup_evset);
	i64_evset_fence(probe_evset, cleanup_evset);
	i64_evset_fence(probe_evset, cleanup_evset);
	i64_evset_fence(probe_evset, evset);

	i64_evset_fence(probe_evset, cleanup_evset);
	i64_evset_fence(probe_evset, evset);
	printf("Loading evset 1st time takes %li\n", evset->measurements[evset->cnt_measurement-4]);
	printf("Loading evset second time takes %li\n", evset->measurements[evset->cnt_measurement-3]);
	printf("Loading evset after cleanup evset 3 times takes %li\n", evset->measurements[evset->cnt_measurement-2]);
	printf("Loading evset after cleanup evset once takes %li\n", evset->measurements[evset->cnt_measurement-1]);
	
	printf("Loading 1 cleanup_evset takes %li\n", evset->measurements[evset->cnt_measurement-4]);
	printf("Loading 1 cleanup_evset takes %li\n", evset->measurements[evset->cnt_measurement-3]);
	printf("Loading 2 cleanup_evset (directly after) takes %li\n", evset->measurements[evset->cnt_measurement-2]);
	printf("Loading 3 cleanup_evset takes %li\n", evset->measurements[evset->cnt_measurement-1]);
	

}

void test_L1_cache(){
	Config *conf=initConfig_D;
	void *buffer = mmap(NULL, 10*conf->buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	Eviction_Set *evset = initEviction_Set(conf);
	Eviction_Set *target_set = initEviction_Set(conf);
	Eviction_Set *cleanup_evset = initEviction_Set(conf);
	
	for(int i=0;i<8;i++){
		addEvictionAdrs(cleanup_evset, buffer+i*conf->pagesize); // clean up eviction set 
		addEvictionAdrs(evset, buffer+(16+i)*conf->pagesize); // eviction set 
		addEvictionAdrs(target_set, buffer+(32+i)*conf->pagesize); // target set 
	}
	createPointerChaseInEvictionSet(evset);
	createPointerChaseInEvictionSet(cleanup_evset);



	createPointerChaseInEvictionSet(target_set);

	wait(1e9);
	// test_only_evset(evset, cleanup_evset);
	test_pointer_chase(evset, cleanup_evset, target_set);
	// load_fence(target_set->adrs[0]);
	// printf("Time loading cached target at %p takes %lu\n", target_set->adrs[0], probe(target_set->adrs[0]));
	// my_fence();
	// load(evset->adrs[0]);
	// load(evset->adrs[1]);
	// load(evset->adrs[2]);
	// load(evset->adrs[3]);
	// load(evset->adrs[4]);
	// load(evset->adrs[5]);
	// load(evset->adrs[6]);
	// load_fence(evset->adrs[7]);
	// printf("Time loading target at %p after evset takes %lu\n", target_set->adrs[0], probe(target_set->adrs[0]));
	// // clean
	// for(int i=0;i<8;i++) {
		// flush(target_set->adrs[i]);
		// flush(evset->adrs[i]);
	// }
	
	// my_fence();
	
	// // load evset, load target, test each evset member
	// u64 time;
	
	// for(int i=0;i<8;i++){ // empty measurements
		// evset->measurements[i]=0;
	// }	

	
	// // printf("\nNo element loaded from target set with pp probe:\n");
	// // for (int j=0;j<TEST_REPS;j++){
		
		// // for(int i=0;i<8;i++) {
			// // flush(target_set->adrs[i]);
			// // flush(evset->adrs[i]);
		// // }
		
		// // my_fence();
	
		// // for(int i=0;i<8;i++){
			// // my_fence();
			// // pp_probe(evset); // load all evset elems
			// // my_fence();
			// // do{
				// // time = probe(evset->adrs[i]);
			// // } while(time > OUTSIDER_TRESHOLD);			
			// // my_fence();
			// // evset->measurements[i] += time;
			// // my_fence();
			// // // printf("Time of element %i is %lu\n", i, time);
		// // }
	// // }
	
	// // for(int i=0;i<8;i++){ // print and reset measurements
		// // printf("Time sum of element %i is %lu, avg per iteration is %lu\n", i, evset->measurements[i], evset->measurements[i]/TEST_REPS);
		// // evset->measurements[i]=0;
	// // }	
	
	// printf("\nNo element loaded from target set with load:\n");
	// for (int j=0;j<TEST_REPS;j++){
		
		// for(int i=0;i<8;i++) {
			// flush(target_set->adrs[i]);
			// flush(evset->adrs[i]);
		// }
		
		// my_fence();
	
		// for(int i=0;i<8;i++){
			// my_fence();
			// // load all evset elems
			// load_fence(evset->adrs[0]);
			// load_fence(evset->adrs[1]);
			// load_fence(evset->adrs[2]);
			// load_fence(evset->adrs[3]);
			// load_fence(evset->adrs[4]);
			// load_fence(evset->adrs[5]);
			// load_fence(evset->adrs[6]);
			// load_fence(evset->adrs[7]);
			// do{
				// time = probe_fence(evset->adrs[i]);
			// } while(time > OUTSIDER_TRESHOLD);			
			// my_fence();
			// evset->measurements[i] += time;
			// my_fence();
			// // printf("Time of element %i is %lu\n", i, time);
		// }
	// }


	// for(int i=0;i<8;i++){ // print and reset measurements
		// printf("Time sum of element %i at %p is %lu, avg per iteration is %lu\n", i, evset->adrs[i], evset->measurements[i], evset->measurements[i]/TEST_REPS);
		// evset->measurements[i]=0;
	// }	
		
	// printf("\n1 element loaded from target set:\n");
	// for(int j=0;j<TEST_REPS;j++){
		
		// for(int i=0;i<8;i++) {
			// flush(target_set->adrs[i]);
			// flush(evset->adrs[i]);
		// }
		// my_fence();
		// for(int i=0;i<8;i++){
			// my_fence();
			// pp_probe(evset); // load all evset elems
			// my_fence();
			// pp_probe(target_set); // load "1" target adrs 
			// my_fence();
			// do{
				// time = probe_fence(evset->adrs[i]);
			// } while(time > OUTSIDER_TRESHOLD);			
			// my_fence();
			// evset->measurements[i+j*8] = time;
			// my_fence();
			
			// // printf("Time of element %i is %lu\n", i, time);
		// }
	// }
	// for(int i=0;i<8;i++){ // print and reset measurements
		// printf("Element %i at %p:\n", i, evset->adrs[i]);
		// int sum=0;
		// for (int j=0;j<TEST_REPS;j++) {			
			// printf("%lu, ", evset->measurements[i+j*8]);
			// sum+= evset->measurements[i+j*8];
		// }
		// printf("%i\n", sum/TEST_REPS);
		
		// evset->measurements[i]=0;
	// }	
	
	// printf("\n1 element loaded from loaded target set:\n");
	// for(int j=0;j<TEST_REPS;j++){
		
		// for(int i=0;i<8;i++) {
			// flush(target_set->adrs[i]);
			// flush(evset->adrs[i]);
		// }
		// my_fence();
		// for(int i=0;i<8;i++){
			// my_fence();
			// pp_probe(evset); // load all evset elems
			// my_fence();
			// load_fence(target_set->adrs[0]); // load "1" target adrs 
			// do{
				// time = probe_fence(evset->adrs[i]);
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
	
	// printf("\n1 element loaded from target set:\n");
	// for(int j=0;j<TEST_REPS;j++){
		
		// for(int i=0;i<8;i++) {
			// flush(target_set->adrs[i]);
			// flush(evset->adrs[i]);
		// }
		// my_fence();
		// for(int i=0;i<8;i++){
			// my_fence();
			// pp_probe(evset); // load all evset elems
			// my_fence();
			// pp_probe(target_set); // load "1" target adrs 
			// my_fence();
			// do{
				// time = probe_fence(evset->adrs[i]);
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
	

	// repeat 
	
	// load evset, load target, load another target test each evset member
	
	// repeat 
	
	// load evset, load 3 targets, test each evset member
	munmap(buffer, 10*conf->buffer_size);
}


int main(){
	test_L1_cache();
}