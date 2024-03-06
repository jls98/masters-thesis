#include "../utils/utils.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define initConfig(D) initConfig(-1,-1,-1,-1,-1,-1,-1,-1)

void test_add_eviction_adrs(){
	printf("\n\nStart test_add_eviction_adrs ... \n");
	Config *conf = initConfig(D);
	Eviction_Set *evset = NULL; // evset 
	void *adrs=malloc(8); // just some pointer
	addEvictionAdrs(evset, adrs); // call with null evset
	
	CU_ASSERT_PTR_NULL(evset);
	
	evset=initEviction_Set(conf);
	
	// regular fill of evset
	for (u64 i=0;i<conf->cache_ways;i++){
		u64 cur_size = evset->size;
		addEvictionAdrs(evset, adrs);
		CU_ASSERT_EQUAL(cur_size+1, evset->size); // +1 compared to before
		CU_ASSERT_EQUAL(evset->adrs[evset->size-1], adrs); // correct adrs set
	}
	
	// case evset full
	u64 cur_size = evset->size;
	void *adrs2=malloc(8);
	addEvictionAdrs(evset, adrs2);
	CU_ASSERT_EQUAL(cur_size, evset->size); // +1 compared to before
	CU_ASSERT_EQUAL(evset->adrs[evset->size-1], adrs); // correct adrs set
	CU_ASSERT_NOT_EQUAL(evset->adrs[evset->size-1], adrs2); // correct adrs set
	
	free(evset);
	free(conf);
	free(adrs);
	free(adrs2);
	
	printf("End test_add_eviction_adrs\n\n");
}

int contains(Eviction_Set *evset, void *candidate){
	u64 *current=(u64 *)evset->adrs;
	for(u64 i=0;i<evset->size;i++){
		if (candidate==current) return 1;
		printf("contains: cand %p cur %p\n", candidate, current);
		current=*current;
	}
	return 0;
}

void test_create_pointer_chase_in_eviction_set(){
	
	Eviction_Set *evset=NULL;
	createPointerChaseInEvictionSet(evset); // evset NULL
	CU_ASSERT_PTR_NULL(evset);	
	
	Config *conf = initConfig(D);
	evset = initEviction_Set(conf);
	void **adrs=malloc(conf->cache_ways*sizeof(void *)); // just some pointer
	
	createPointerChaseInEvictionSet(evset); // 
	u64 cur_rng=rng;
	CU_ASSERT_EQUAL(rng, cur_rng); // rng did not change -> instant return	
	
	// add cache_ways many different addresses to 
	for (u64 i=0;i<conf->cache_ways;i++){
		printf("adrs: %p\n", adrs+8*i);
		addEvictionAdrs(evset, adrs+8*i);
	}
	
	createPointerChaseInEvictionSet(evset);
	for (u64 i=0;i<conf->cache_ways;i++){
		printf("adrs: %p\n", adrs+i);
		CU_ASSERT_TRUE(contains(evset, adrs+i));
	}
	CU_ASSERT_FALSE(contains(evset, adrs+conf->cache_ways)); // adrs behind should not be contained

	
}

int main(int ac, char **av) {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite pp", NULL, NULL);
    CU_add_test(suite, "test_add_eviction_adrs", test_add_eviction_adrs);
    CU_add_test(suite, "test_create_pointer_chase_in_eviction_set", test_create_pointer_chase_in_eviction_set);

    CU_basic_run_tests();
    CU_cleanup_registry();
	
    return 0;
}