#include "evict_baseline.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define reps_test1 64 // at 64 segmentation fault
static struct Config *conf;

void test_test1(){
    wait(1E9);
    printf("\nTesting test1...\n\n");

    uint64_t c_size = conf->cache_size/2; // two times cache size but as uint64_t

	void **cand_set;
    void *target_adrs;
	
    uint64_t lfsr = lfsr_create();
    
    struct Node* evict_set = initLinkedList(); 
    struct Node* evict_set_minimal = initLinkedList();
    
    // 512, 1024, 1536, 2048, 2560, 3112, 3624, 4136  index +1 == 8 bytes on L1
    for(int i=1;i<9;i+=1) evict_set_minimal = addElement(evict_set_minimal, i*(conf->cache_size/conf->cache_line_size)); 
    
	// test with a minimal eviction set (4096 bytes apart on L1)
	for (int i=0;i<reps_test1;i++){
		cand_set = conf->hugepages==1 ? mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0) : mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		target_adrs = &cand_set[c_size+4096*2]; // just some random candidate :D
		create_pointer_chase(cand_set, c_size, evict_set_minimal);
        __asm__ volatile("lfence");
		CU_ASSERT_EQUAL(test1(cand_set[evict_set_minimal->value], c_size, target_adrs, conf), 1); 
		munmap(cand_set, 10* c_size * sizeof(void *));
    }
	// works on L1, modification for other setups might be a TODO

	for(int i=1;i<64;i++){
		evict_set = addElement(evict_set, i*8);
		evict_set = addElement(evict_set, (64+i)*8);
	} 
		
	// eviction set on wrong offsets -> should not evict!
    for (int i=0;i<reps_test1;i++) {
		cand_set = conf->hugepages==1 ? mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0) : mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		target_adrs = &cand_set[c_size+4096*2]; // just some random candidate :D
		create_pointer_chase(cand_set, c_size, evict_set); 
        __asm__ volatile("lfence");
		// test no eviction
        CU_ASSERT_EQUAL(test1(cand_set[evict_set->value], 126, target_adrs, conf), 0); // assure self assignment
		munmap(cand_set, 10* c_size * sizeof(void *));
    }
	
	
	cand_set = conf->hugepages==1 ? mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0) : mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	target_adrs = &cand_set[c_size+4096*2]; // just some random candidate :D
	
    // create pointer chase on base set
    create_pointer_chase(cand_set, c_size, evict_set);
    // uninitialized params/errors

	// test with invalid candidate set
    CU_ASSERT_EQUAL(test1(NULL, c_size, target_adrs, conf), -1); 
	
	// test with empty candidate set
    CU_ASSERT_EQUAL(test1(cand_set[evict_set->value], 0, target_adrs, conf), -1); 
    
	// test with invalid target address
	CU_ASSERT_EQUAL(test1(cand_set[evict_set->value], c_size, NULL, conf), -1); 
	
	// test with uninitialized config
    CU_ASSERT_EQUAL(test1(cand_set[evict_set->value], c_size, target_adrs, NULL), -1); 
    
    printf("get adrs:\nevictset %p\ncandset %p\ntarget adrs %p\n", &evict_set, &cand_set, target_adrs);
    
    freeList(evict_set);
	munmap(cand_set, 10* c_size * sizeof(void *));
    printf("     ... finished!\n");
}

void test_pick(){
    printf("\nTesting pick...\n\n");
    
    // init (evict|cand) sets
    struct Node* candidate_set1 = initLinkedList();
    struct Node* candidate_set2 = initLinkedList();
    struct Node* empty_set = initLinkedList();
    
	// just add something
    candidate_set1 = addElement(candidate_set1, 1); 
    candidate_set1 = addElement(candidate_set1, 65);
    
    uint64_t base_size = 100;   // size of candidate set 
    uint64_t lfsr = lfsr_create();
    
    // test uninitialized params
    CU_ASSERT_EQUAL(pick(candidate_set1, base_size, NULL), -1); // lfsr
    CU_ASSERT_EQUAL(pick(candidate_set1, 0, &lfsr), -1);	// base candidate set empty -> size = 0
    CU_ASSERT_EQUAL(pick(empty_set, base_size, &lfsr), -1); // empty candidate set

    candidate_set2 = addElement(candidate_set2, 65);
    // regular, 1 valid candidate option left
    int64_t val_tmp = pick(candidate_set2, base_size, &lfsr);
	uint64_t val = val_tmp != -1 ? (uint64_t) val_tmp : 0;
    CU_ASSERT_EQUAL(val, 65);  
    if (val != 65) printf("val65 %lu\n", val); // debug for failed test
    
    candidate_set2 = addElement(candidate_set2, 67);
    candidate_set2 = addElement(candidate_set2, 69);
    candidate_set2 = addElement(candidate_set2, 66);
    val = pick(candidate_set2, base_size, &lfsr);
    
    CU_ASSERT_TRUE(val > 64);
    if (val<65) printf("val<65 %lu\n", val); // debug for failed test

    CU_ASSERT_TRUE(val < base_size); 
    if (val>= base_size) printf("val<bsize %lu\n", val);
    
    freeList(candidate_set1);
    freeList(candidate_set2);
    freeList(empty_set);
    
    for(int i=0;i<m_ind;i++){
        printf("%lu\n" , msrmts[i]);
    }
    
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

int main(int ac, char **av) {
	if (ac==1) conf = initConfig(8, 64, 54, 32768, 1, 1); // default L1 lab machine, no inputs
	if (ac==2){
		int conf_choice = strtol(av[1], NULL, 10);
		if (conf_choice==11) conf = initConfig(8, 64, 54, 32768, 1, 1); 	// L1 i7
		if (conf_choice==12) conf = initConfig(8, 64, 58, 262144, 1, 1); 	// L2 i7 what value?
		if (conf_choice==13) conf = initConfig(8, 64, 58, 262144, 1, 1); 	// L3 i7 TODO or unneeded
		
		if (conf_choice==21) conf = initConfig(8, 64, 75, 32768, 1, 1); 	// L1e i12
		if (conf_choice==22) conf = initConfig(8, 64, 85, 2097152, 1, 1); 	// L2e i12 TODO
		if (conf_choice==23) conf = initConfig(8, 64, 58, 262144, 1, 1); 	// L3e i12 TODO or unneeded
		if (conf==NULL){
			printf("Error, no valid choice, XY, whereby X is the CPU (1:i7, 2:i12) and Y the cache Level (1-3)");
			return 0;
		}
	}
	if (ac==7){ // self, ways, cache line size, threshold, cache size, test reps
		conf = initConfig(strtol(av[1], NULL, 10), strtol(av[2], NULL, 10), strtol(av[3], NULL, 10), strtol(av[4], NULL, 10), strtol(av[5], NULL, 10), strtol(av[6], NULL, 10));
	}
	
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    CU_add_test(suite, "Test create_pointer_chase", test_create_pointer_chase);
    CU_add_test(suite, "Test pick", test_pick);
    CU_add_test(suite, "Test test1", test_test1);

    CU_basic_run_tests();
    CU_cleanup_registry();
	free(conf);
	
    return 0;
}