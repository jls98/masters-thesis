#include "pp.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>



void test_pp_init(){
	ptinf("\n\nStart test_pp_init ... \n")
	
	// no conf
	CU_ASSERT_PTR_NULL(pp_init(NULL));
	
	Config *conf=initConfig(-1,-1,-1,-1,-1,-1, -1,0); // init conf with default values and buffer size of 0
	CU_ASSERT_PTR_NULL(pp_init(conf));
	
	
	Config *conf=initConfig(-1,-1,-1,-1,-1,-1, -1,1);
	CU_ASSERT_PTR_NOT_NULL(pp_init(conf))
	printf("End test_pp_init\n\n")

}

int main(int ac, char **av) {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite pp", NULL, NULL);
    CU_add_test(suite, "test_pp_init", test_pp_init);

    CU_basic_run_tests();
    CU_cleanup_registry();
	
    return 0;
}