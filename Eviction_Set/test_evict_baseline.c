#include "evict_baseline.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define reps_test1 100

void test_test1(){
	struct Config *conf = initConfig(8, 64, 54, 32768, 300);
    wait(1E9);
    printf("\nTesting test1...\n\n");
	// change to local system and cache in 8 bytes to check, a size of candidate set in index 

    uint64_t c_size = conf->cache_size/2; // two times cache size but as uint64_t
    void **cand_set = mmap(NULL, c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    uint64_t *cand = malloc(sizeof(uint64_t *)); // just some random candidate :D
	*cand = 1;
    uint64_t lfsr = lfsr_create();
    
    struct Node* cind_set = initLinkedList();   // current candidate set (indexes)
    struct Node* evict_set1 = initLinkedList();  // current eviction set
    struct Node* evict_set2 = initLinkedList();  // current eviction set

    for (uint64_t i=0; i<c_size-1;i+=8) cind_set = addElement(cind_set, i);  // init indexes for candidate set (add all indexes to candidate index set)
          
    // fill eviction set with all elements, maximal eviction set lol
    for (uint64_t i=0; i<c_size-1;i+=8){
        int64_t c = pick(evict_set1, cind_set, c_size, &lfsr);
        if(c == -1){
            printf("test_test1: filling eviction_set failed!\n");
            break;
        } 
        cind_set = deleteElement(cind_set, c);
        evict_set1 = addElement(evict_set1, c);
    }
    
    // create pointer chase on base set
    create_pointer_chase(cand_set, c_size, evict_set1);
    // uninitialized params/errors

    CU_ASSERT_EQUAL(test1(NULL, c_size, cand, conf), -1); // assure self assignment
    CU_ASSERT_EQUAL(test1(cand_set[evict_set1->value], 0, cand, conf), -1); // assure self assignment
    CU_ASSERT_EQUAL(test1(cand_set[evict_set1->value], c_size, NULL, conf), -1); 
    CU_ASSERT_EQUAL(test1(cand_set[evict_set1->value], c_size, cand, NULL), -1); 
    
    // regular case (full huge page should evict (hopefully))
    for (int i=0;i<reps_test1;i++) CU_ASSERT_EQUAL(test1(cand_set[evict_set1->value], c_size, cand, conf), 1); // assure self assignment
    
   
    evict_set2 = addElement(evict_set2, 64);
    evict_set2 = addElement(evict_set2, 24);
    evict_set2 = addElement(evict_set2, 8);
    create_pointer_chase(cand_set, c_size, evict_set2); // eviction set far too small -> no eviction of candidate
    for (int i=0;i<reps_test1;i++) {
        __asm__ volatile("lfence");
        CU_ASSERT_EQUAL(test1(cand_set[evict_set2->value], 3, cand, conf), 0); // assure self assignment
    }
    
    freeList(evict_set1);
    freeList(evict_set2);
    freeList(cind_set);
	free(conf);
	munmap(cand_set, c_size);
    printf("     ... finished!\n");
}

void test_pick(){
    printf("\nTesting pick...\n\n");
    
    // init (evict|cand) sets
    struct Node* evict_set = initLinkedList();
    struct Node* candidate_set1 = initLinkedList();
    struct Node* candidate_set2 = initLinkedList();
    struct Node* empty_set = initLinkedList();
    
    evict_set = addElement(evict_set, 0);
    evict_set = addElement(evict_set, 5);
    evict_set = addElement(evict_set, 2);
    evict_set = addElement(evict_set, 7);
    evict_set = addElement(evict_set, 3);
    
	// just add something
    candidate_set1 = addElement(candidate_set1, 1); 
    candidate_set1 = addElement(candidate_set1, 65);
    
    uint64_t base_size = 100;   // size of candidate set 
    uint64_t lfsr = lfsr_create();
    
    // test uninitialized params
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set1, base_size, NULL), -1); // lfsr
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set1, 0, &lfsr), -1);	// base candidate set empty -> size = 0
    CU_ASSERT_EQUAL(pick(evict_set, empty_set, base_size, &lfsr), -1); // empty candidate set
    
    // test no candidate possible
    // one duplicate element left in candidate set
    candidate_set2 = addElement(candidate_set2, 5);
    CU_ASSERT_EQUAL(pick(evict_set, candidate_set2, base_size, &lfsr), -1); 

    candidate_set2 = addElement(candidate_set2, 65);
    // regular, 1 valid candidate option left
    int64_t val_tmp = pick(evict_set, candidate_set2, base_size, &lfsr);
	uint64_t val = val_tmp != -1 ? (uint64_t) val_tmp : 0;
    CU_ASSERT_EQUAL(val, 65);  
    if (val != 65) printf("val65 %lx\n", val);
    
    candidate_set2 = addElement(candidate_set2, 67);
    candidate_set2 = addElement(candidate_set2, 69);
    candidate_set2 = addElement(candidate_set2, 66);
    val = pick(evict_set, candidate_set2, base_size, &lfsr);
    
    // TODO sometimes fail
    CU_ASSERT_TRUE(val > 64);
    if (val<65) printf("val<65 %lx\n", val);

    CU_ASSERT_TRUE(val < base_size); 
    if (val>= base_size) printf("val<bsize %lx\n", val);
    
    freeList(candidate_set1);
    freeList(candidate_set2);
    freeList(empty_set);
    freeList(evict_set);
    printf("     ... finished!\n");
}

void test_create_pointer_chase(){
    printf("\nTesting create_pointer_chase...\n\n");
    
	
	uint64_t c_size = 512;
    void **candidate_set = mmap(NULL, c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    struct Node* set = initLinkedList();

    // case set size = 0
    for (uint64_t i=0; i< c_size-1;i++) candidate_set[i]=&candidate_set[i]; // every adrs points to itself
    create_pointer_chase(candidate_set, c_size, set);
    for (uint64_t i=0; i<c_size-1;i++) CU_ASSERT_EQUAL(candidate_set[i], &candidate_set[i]); // self assignment remained -> no cahnges
    
    // set some values
    set = addElement(set, 3);    
    set = addElement(set, 77);
    set = addElement(set, 205);
    set = addElement(set, 511);
    set = addElement(set, 92);
    set = addElement(set, 360);
    set = addElement(set, 17);
    set = addElement(set, 255);
    set = addElement(set, 128);
    set = addElement(set, 42);

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
    
    /* // some debug prints
    printf("92: %p, &92 %p\n", candidate_set[92], &candidate_set[92]);
    printf("360: %p, &360 %p\n", candidate_set[360], &candidate_set[360]);
    printf("17: %p, &17 %p\n", candidate_set[17], &candidate_set[17]);
    printf("511: %p, &511 %p\n", candidate_set[511], &candidate_set[511]);*/

    // regular pointer chase
    c_size=512;
    create_pointer_chase(candidate_set, c_size, set);
    
    CU_ASSERT_EQUAL(candidate_set[42], &candidate_set[128]);
    CU_ASSERT_EQUAL(candidate_set[128], &candidate_set[255]);
    CU_ASSERT_EQUAL(candidate_set[255], &candidate_set[17]);
    CU_ASSERT_EQUAL(candidate_set[17], &candidate_set[360]);
    CU_ASSERT_EQUAL(candidate_set[360], &candidate_set[92]);
    CU_ASSERT_EQUAL(candidate_set[92], &candidate_set[511]);
    CU_ASSERT_EQUAL(candidate_set[511], &candidate_set[205]);
    CU_ASSERT_EQUAL(candidate_set[205], &candidate_set[77]);
    CU_ASSERT_EQUAL(candidate_set[77], &candidate_set[3]);
    CU_ASSERT_EQUAL(candidate_set[3], &candidate_set[42]);
    printf("     ... finished!\n");
}

int main() {
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    CU_add_test(suite, "Test create_pointer_chase", test_create_pointer_chase);
    CU_add_test(suite, "Test pick", test_pick);
    CU_add_test(suite, "Test test1", test_test1);

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}