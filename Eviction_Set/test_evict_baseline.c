#include "evict_baseline.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>


void test_create_pointer_chase(){

    
    uint64_t base_size = 512;
    uint64_t set_size = 0;
    void* *base = mmap(NULL, base_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    uint64_t *set = (uint64_t *) malloc(base_size * sizeof(uint64_t *));
    
    // set some values
    set[0] = 42;
    set[1] = 128;
    set[2] = 255;
    set[3] = 17;
    set[4] = 360;
    set[5] = 92;
    set[6] = 511;
    set[7] = 205;
    set[8] = 77;
    set[9] = 3;
    set[10] = 456;
    set[11] = 29;
    set[12] = 198;
    set[13] = 72;
    set[14] = 431;
    set[15] = 174;
    set[16] = 50;
    set[17] = 111;
    set[18] = 503;
    set[19] = 91;

    
    // case set size = 0
    create_pointer_chase(base, base_size, set, set_size);
    for (uint64_t i=0; i<base_size-1;i++){
        CU_ASSERT_EQUAL(base[i], &base[i]); // assure self assignment
    }
    
    // case element index out of range
    base_size=511;
    set_size=20;
    create_pointer_chase(base, base_size, set, set_size);
    
    // regular
    CU_ASSERT_EQUAL(base[42], &base[128]);
    CU_ASSERT_EQUAL(base[128], &base[255]);
    CU_ASSERT_EQUAL(base[255], &base[17]);
    CU_ASSERT_EQUAL(base[17], &base[360]);
    CU_ASSERT_EQUAL(base[360], &base[92]);
    CU_ASSERT_EQUAL(base[92], &base[92]); // remains the same! problem occured here
   
    // regular pointer chase
    base_size=512;
    set_size=5;
    create_pointer_chase(base, base_size, set, set_size);
    
    CU_ASSERT_EQUAL(base[42], &base[128]);
    CU_ASSERT_EQUAL(base[128], &base[255]);
    CU_ASSERT_EQUAL(base[255], &base[17]);
    CU_ASSERT_EQUAL(base[17], &base[360]);
    CU_ASSERT_EQUAL(base[360], &base[42]);
}

int main() {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    CU_add_test(suite, "Test create_pointer_chase", test_create_pointer_chase);

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}