#include "pp.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void test_pp_init(){
	printf("\n\nStart test_pp_init ... \n");
	
	void *return_value;
	
	// no conf
	CU_ASSERT_PTR_NULL(pp_init(NULL));
	
	Config *conf=initConfig(-1,-1,-1,-1,-1,-1, -1,0); // init conf with default values and buffer size of 0
	return_value=pp_init(conf);
	CU_ASSERT_PTR_NULL(return_value);
	
	// works
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,1);
	return_value=pp_init(conf);
	CU_ASSERT_PTR_NOT_NULL(return_value);
	munmap(return_value, 1);
	
	// limit testing
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,512*4096);
	return_value=pp_init(conf);
	CU_ASSERT_PTR_NOT_NULL(return_value);
	munmap(return_value, 512*4096);
	
	// limit testing
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,4096*4096);
	return_value=pp_init(conf);
	CU_ASSERT_PTR_NOT_NULL(return_value);
	munmap(return_value, 4096*4096);
	
	// limit testing
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,16000000);
	return_value=pp_init(conf);
	CU_ASSERT_PTR_NOT_NULL(return_value);
	munmap(return_value, 16000000);

	printf("End test_pp_init\n\n");
}

#define REPS 10
// SET CORRECT THRESHOLD!!
void test_pp_setup(){
	printf("\n\nStart test_pp_setup ... \n");
	wait(1E9);
	Config *conf = initConfig_D;
	Eviction_Set *evset =initEviction_Set(conf);
	u64 *target = malloc(100*sizeof(u64));
	i64 debug;
	// does it evict?
	pp_setup(conf, evset, target);
	load(target);
	// my_fence();
	// debug =pp_probe(evset);
	// printf("timing evset %li \n", debug);
	// my_fence();
	// debug =pp_probe(evset);
	// printf("timing evset %li \n", debug);
	// my_fence();
	// debug =pp_probe(evset);
	// printf("timing evset %li \n", debug);
	// my_fence();
	// debug =pp_probe(evset);
	// printf("timing evset %li \n", debug);
	// my_fence();
	// debug =pp_probe(evset);
	// printf("timing evset %li \n", debug);
	// my_fence();

	// for(int i=0;i<REPS;i++){
	// no evict
	flush(target);
	pp_probe(evset);
	// SET CORRECT THRESHOLD!!
	// case no target access
	debug =pp_probe(evset);
	CU_ASSERT_TRUE( debug< (i64) conf->threshold_L1);
	printf("evset timing w no access by target: %li\n", debug);
	
	// evict
	my_fence();
	load(target); // target access in between
	my_fence();
	// SET CORRECT THRESHOLD!!
	debug =pp_probe(evset);
	CU_ASSERT_TRUE(debug > (i64) conf->threshold_L1);
	printf("evset timing w loaded target: %li\n", debug);
	
	
	// no evict
	my_fence();
	// case no target access
	debug =pp_probe(evset);
	CU_ASSERT_TRUE( debug< (i64) conf->threshold_L1);
	printf("evset timing w no access by target: %li\n", debug);
	
	// evict
	my_fence();
	load(target); // target access in between
	my_fence();
	// SET CORRECT THRESHOLD!!
	debug =pp_probe(evset);
	CU_ASSERT_TRUE(debug > (i64) conf->threshold_L1);
	printf("evset timing w loaded target: %li\n", debug);

		// // no evict
		// flush(target);
		// // SET CORRECT THRESHOLD!!
		// debug =pp_probe(evset);
		// CU_ASSERT_TRUE( debug> (i64) conf->threshold_L1);
		// printf("flush: %li\n", debug);
		// // evict
		// load(target);
		// // SET CORRECT THRESHOLD!!
		// debug =pp_probe(evset);
		// CU_ASSERT_TRUE(debug < (i64) conf->threshold_L1);
		// printf("load: %li\n", debug);

	// }
	
	// testing target
	pp_probe(evset);
	load(target);
	my_fence();
	debug = probe(target);
	printf("cached target: %li\n", debug);
	flush(target);
	my_fence();
	debug = probe(target);
	printf("flushed target: %li\n", debug);
	my_fence();
	load(target);
	my_fence();
	debug = pp_probe(evset);
	my_fence();
	printf("timing evset: %li\n", debug);
	my_fence();
	debug = probe(target);
	printf("evset target: %li\n", debug);
	my_fence();
	pp_probe(evset);
	my_fence();
	debug = pp_probe(evset);
	my_fence();
	printf("evset cached: %li\n", debug);

	printf("End test_pp_setup\n\n");
}

void test_pp_probe(){
	
}

// void test_set_msr_bits(){
	// set_msr_bits(0);
	// set_msr_bits(0b1);
	// set_msr_bits(0b10);
	// set_msr_bits(0b100);
	// set_msr_bits(0b1000);
	// set_msr_bits(0b1010);
	
// }


int main(int ac, char **av) {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite pp", NULL, NULL);
    CU_add_test(suite, "test_pp_init", test_pp_init);
    CU_add_test(suite, "test_pp_setup", test_pp_setup);
    CU_basic_run_tests();
    CU_cleanup_registry();
	
    return 0;
}