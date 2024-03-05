#include "../Utils/utils.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define initConfig(D) initConfig(-1,-1,-1,-1,-1,-1,-1,-1)

void test_add_eviction_adrs(){
	printf("\n\nStart test_add_eviction_adrs ... \n");
	Config *conf = initConfig(D);
	Eviction_Set *evset; // evset 
	void *adrs=malloc(8); // just some pointer
	addEvictionAdrs(evset, adrs); // call with null evset
	
	CU_ASSERT_PTR_NULL(evset);
	
	// evset=initEviction_Set()


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