#include "evict_baseline.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "evict1.c"

void test_test1(){
    printf("testing test1...\n");
    uint64_t size = CACHESIZE_DEFAULT*8, set_size=size; // set size should be large enough to evict everything from L1 lol
    void **base = mmap(NULL, size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    uint64_t *cand = (uint64_t *) malloc(sizeof(uint64_t *)); // just some random candidate :D
    uint64_t *set = (uint64_t *) malloc(set_size*sizeof(uint64_t *)); // indexes from 0-8 in eviction set
    
    wait(1E9);
    for (uint64_t i=0; i<set_size-1;i++){
        set[i]=i; // naive eviction set xd
    }
    
    // create pointer chase on base set
    create_pointer_chase(base, size, set, set_size);
    // uninitialized params/errors
    
    CU_ASSERT_EQUAL(TEST1(NULL, set_size, cand), -1); // assure self assignment
    CU_ASSERT_EQUAL(TEST1(base, 0, cand), -1); // assure self assignment
    CU_ASSERT_EQUAL(TEST1(base, set_size, NULL), -1); // assure self assignment

    
    // regular case (full huge page should evict (hopefully))
    CU_ASSERT_EQUAL(TEST1(base, set_size, cand), 1); // assure self assignment
    
    set[0]=5;
    set[1]=23;
    set[2]=65;
    create_pointer_chase(base, size, set, 3); // eviction set far too small -> no eviction of candidate
    CU_ASSERT_EQUAL(TEST1(base, 3, cand), 0); // assure self assignment
    printf("case 3 set %li\n", TEST1(base, 3, cand));
    
    printf("time %lu\n", probe(base, 3, cand));
    
}

void test_pick(){
    printf("testing pick...\n");
    uint64_t set_size = 5;      // size of current eviction set
    uint64_t base_size = 6;     // size of current base set 
    uint64_t size = 100;        // size of candidate set 
    uint64_t *set = (uint64_t *) malloc(set_size*sizeof(uint64_t *));
    uint64_t *base = (uint64_t *) malloc(base_size*sizeof(uint64_t *));
    uint64_t lfsr = lfsr_create();
    
    // test uninitialized params
    CU_ASSERT_EQUAL(pick(set, set_size, base, base_size, size, NULL), size+1);
    CU_ASSERT_EQUAL(pick(set, set_size, base, base_size, 0, &lfsr), 1);
    CU_ASSERT_EQUAL(pick(set, set_size, NULL, base_size, size, &lfsr), size+1);
    CU_ASSERT_EQUAL(pick(NULL, set_size, base, base_size, size, &lfsr), size+1);
   
    
    // set some arbitrary indexes
    set[0]=0;
    set[1]=5;
    set[2]=2;
    set[3]=7;
    set[4]=3;
    
    base[0]=5;
    base[1]=65;
    base[2]=3;
    base[3]=10;
    base[4]=8;
    base[5]=9;
    
    // test no candidate possible
    // no elements left in base
    CU_ASSERT_EQUAL(pick(set, set_size, base, 0, size, &lfsr), size+1); 

    // already in eviction set 
    CU_ASSERT_EQUAL(pick(set, set_size, base, 1, size, &lfsr), size+1); 

    // regular, 1 option left, base[0] should be always skipped
    CU_ASSERT_EQUAL(pick(set, set_size, base, 2, size, &lfsr), 65); 
}

void test_create_pointer_chase(){
    printf("testing create_pointer_chase...\n");
    uint64_t base_size = 512;
    uint64_t set_size = 0;
    void **base = mmap(NULL, base_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
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

    
    // case set size = 0
    create_pointer_chase(base, base_size, set, set_size);
    for (uint64_t i=0; i<base_size-1;i++){
        CU_ASSERT_EQUAL(base[i], &base[i]); // assure self assignment
    }
    
    // case element index out of range
    base_size=511;
    set_size=10;
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
    //CU_add_test(suite, "Test create_pointer_chase", test_create_pointer_chase);
    //CU_add_test(suite, "Test pick", test_pick);
    CU_add_test(suite, "Test test1", test_test1);

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}