#include "pp.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>



void test_pp_init(){
	printf("\n\nStart test_pp_init ... \n");
	
	void *return_value;
	
	// no conf
	CU_ASSERT_PTR_NULL(pp_init(NULL));
	
	Config *conf=initConfig(-1,-1,-1,-1,-1,-1, -1,0); // init conf with default values and buffer size of 0
	return_value=pp_init(conf)
	CU_ASSERT_PTR_NULL(return_value);
	
	// works
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,1);
	return_value=pp_init(conf)
	CU_ASSERT_PTR_NULL(return_value);
	munmap(return_value, 1);
	
	// limit testing
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,512*4096);
	return_value=pp_init(conf)
	CU_ASSERT_PTR_NULL(return_value);
	munmap(return_value, 512*4096);
	
	// limit testing
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,4096*4096);
	return_value=pp_init(conf)
	CU_ASSERT_PTR_NULL(return_value);
	munmap(return_value, 4096*4096);
	
	// limit testing
	conf=initConfig(-1,-1,-1,-1,-1,-1, -1,16000000);
	return_value=pp_init(conf)
	CU_ASSERT_PTR_NULL(return_value);
	munmap(return_value, 16000000);

	printf("End test_pp_init\n\n");

}

int main(int ac, char **av) {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite pp", NULL, NULL);
    CU_add_test(suite, "test_pp_init", test_pp_init);

    CU_basic_run_tests();
    CU_cleanup_registry();
	
    return 0;
}