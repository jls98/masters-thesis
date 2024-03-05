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
	
	for (int i=0;i<10;i++){
		u64 cur_size = evset->size;
		addEvictionAdrs(evset, adrs);
		CU_ASSERT_EQUAL(cur_size+1, evset->size); // +1 compared to before
		CU_ASSERT_EQUAL(evset->adrs[evset->size-1], adrs); // correct adrs set
	}
	
	printf("End test_add_eviction_adrs\n\n");
}

int main(int ac, char **av) {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite pp", NULL, NULL);
    CU_add_test(suite, "test_add_eviction_adrs", test_add_eviction_adrs);

    CU_basic_run_tests();
    CU_cleanup_registry();
	
    return 0;
}