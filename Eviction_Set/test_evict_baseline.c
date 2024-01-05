#include "evict_baseline.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

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
    CU_ASSERT_EQUAL(TEST1(&base[set[0]], 0, cand), -1); // assure self assignment
    CU_ASSERT_EQUAL(TEST1(&base[set[0]], set_size, NULL), -1); // assure self assignment

    
    // regular case (full huge page should evict (hopefully))
    CU_ASSERT_EQUAL(TEST1(&base[set[0]], set_size, cand), 1); // assure self assignment
    
    set[0]=5;
    set[1]=23;
    set[2]=65;
    create_pointer_chase(base, size, set, 3); // eviction set far too small -> no eviction of candidate
    CU_ASSERT_EQUAL(TEST1(&base[set[0]], 3, cand), 0); // assure self assignment
    //printf("case 3 set %li\n", TEST1(base, 3, cand)); 
}

void test_pick(){
    printf("testing pick...\n");
    
    // init (evict|cand) sets
    struct Node* evict_set = initLinkedList();
    struct Node* candidate_set = initLinkedList();
    
    evict_set = addElement(evict_set, 0);
    evict_set = addElement(evict_set, 5);
    evict_set = addElement(evict_set, 2);
    evict_set = addElement(evict_set, 7);
    evict_set = addElement(evict_set, 3);
    
    candidate_set = addElement(candidate_set, 5); // should not happen
    candidate_set = addElement(candidate_set, 65);
    
    uint64_t base_size = 100;   // size of candidate set 
    uint64_t lfsr = lfsr_create();
    
    // test uninitialized params
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set, base_size, NULL), base_size+1);
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set, 0, &lfsr), 1);
    CU_ASSERT_EQUAL(pick(evict_set, initLinkedList(), base_size, &lfsr), base_size+1);
    CU_ASSERT_EQUAL(pick(initLinkedList(), candidate_set, base_size, &lfsr), base_size+1); 
    
    // test no candidate possible
    // one duplicate element left in candidate set
    freeList(candidate_set);
    candidate_set = initLinkedList();
    candidate_set = addElement(candidate_set, 5);
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set, base_size, &lfsr), base_size+1); 

    candidate_set = addElement(candidate_set, 65);
    // regular, 1 valid candidate option left
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set, base_size, &lfsr), 65);  

    candidate_set = addElement(candidate_set, 67);
    candidate_set = addElement(candidate_set, 69);
    candidate_set = addElement(candidate_set, 66);
    uint64_t val = pick(evict_set, candidate_set, base_size, &lfsr);
    CU_ASSERT_TRUE(val > 64 && val < base_size); 

    freeList(candidate_set);
    freeList(evict_set);
}

void test_create_pointer_chase(){
    printf("testing create_pointer_chase...\n");
    uint64_t c_size = 512;
    void **candidate_set = mmap(NULL, c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    struct Node* set = initLinkedList();

    // case set size = 0
    for (uint64_t i=0; i< c_size-1;i++) candidate_set[i]=&candidate_set[i]; // every adrs points to itself
    create_pointer_chase(candidate_set, c_size, set);
    for (uint64_t i=0; i<c_size-1;i++) CU_ASSERT_EQUAL(candidate_set[i], &candidate_set[i]); // self assignment remained -> no cahnges
    
    // set some values
    set = addElement(set, 42);
    set = addElement(set, 128);
    set = addElement(set, 255);
    set = addElement(set, 17);
    set = addElement(set, 360);
    set = addElement(set, 92);
    set = addElement(set, 511);
    set = addElement(set, 205);
    set = addElement(set, 77);
    set = addElement(set, 3);
    
    // case element index out of range
    c_size=511;
    create_pointer_chase(candidate_set, c_size, set);
    
    // regular
    CU_ASSERT_EQUAL(candidate_set[42], &candidate_set[128]);
    CU_ASSERT_EQUAL(candidate_set[128], &candidate_set[255]);
    CU_ASSERT_EQUAL(candidate_set[255], &candidate_set[17]);
    CU_ASSERT_EQUAL(candidate_set[17], &candidate_set[360]);
    CU_ASSERT_EQUAL(candidate_set[360], &candidate_set[92]);
    CU_ASSERT_EQUAL(candidate_set[92], &candidate_set[92]); // remains the same! problem occured here
   

    // regular pointer chase
    c_size=512;
    create_pointer_chase(candidate_set, c_size, set);
    
    CU_ASSERT_EQUAL(base[42], &base[128]);
    CU_ASSERT_EQUAL(base[128], &base[255]);
    CU_ASSERT_EQUAL(base[255], &base[17]);
    CU_ASSERT_EQUAL(base[17], &base[360]);
    CU_ASSERT_EQUAL(base[360], &base[92]);
    CU_ASSERT_EQUAL(base[92], &base[511]);
    CU_ASSERT_EQUAL(base[511], &base[205]);
    CU_ASSERT_EQUAL(base[205], &base[77]);
    CU_ASSERT_EQUAL(base[77], &base[3]);
    CU_ASSERT_EQUAL(base[3], &base[42]);
}

int main() {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    //CU_add_test(suite, "Test create_pointer_chase", test_create_pointer_chase);
    CU_add_test(suite, "Test pick", test_pick);
    //CU_add_test(suite, "Test test1", test_test1);

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}