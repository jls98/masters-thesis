#include "pp.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>





int main(int ac, char **av) {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite pp", NULL, NULL);
    // CU_add_test(suite, "Test create_pointer_chase", test_create_pointer_chase);

    CU_basic_run_tests();
    CU_cleanup_registry();
	
    return 0;
}