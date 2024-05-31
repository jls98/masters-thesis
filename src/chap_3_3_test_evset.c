#include <stdint.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "chap_3_3_find_evset.c"

#define REPS 10000
void test_evset(Config *conf, void *target){
    wait(1E9);
    
    // setup and find evset
    Node **evset = NULL;
    while(evset==NULL || *evset==NULL) evset = find_evset(conf, target); // hopefully no while true

    // setup measurements
    msrmts=realloc(msrmts, REPS*sizeof(u64));
    msr_index=0;

    // multiple measurements
    for (int i=0;i<REPS;i++) test(conf, *evset, target);
    u64 above=0;
    u64 below=0;
    for (int i=0;i<REPS;i++) {
        // CU_ASSERT_TRUE(msrmts[i] > conf->threshold);
        if(msrmts[i] < conf->threshold)below++; 
        else above++;
    }
    printf("below %lu, above %lu\n", below, above);
    close_evsets(conf, evset);    
}

void test_no_evset(Config *conf, void *target){
    printf("hm\n");
    Node **no_evset = find_evset(conf, target);
    printf("ho\n");

    CU_ASSERT_PTR_NULL(*no_evset);
    close_evsets(conf, no_evset);    
}

void test_find_evset(){
    uint64_t *target = malloc(8);
    *target=0xffffffff;
    for (int i=0; i<100; i++) test_no_evset(config_init(15, 2048, 64, 106,2091752), target);
    printf("evset size 16\n");
    test_evset(config_init(16, 2048, 64, 106,2091752), target);
    printf("evset size 27\n");
    for(int i=0;i<5;i++){
        target = realloc(target, 8);
        *target=0xffffffff;
        test_evset(config_init(27, 2048, 64, 106,2091752), target);
    }     
    printf("evset size 28\n");
    for(int i=0;i<5;i++) test_evset(config_init(28, 2048, 64, 106,2091752), target);
    printf("evset size 29\n");
    for(int i=0;i<5;i++) test_evset(config_init(29, 2048, 64, 106,2091752), target);
    printf("evset size 30\n");
    for(int i=0;i<5;i++) test_evset(config_init(30, 2048, 64, 106,2091752), target);
    printf("evset size 31\n");
    for(int i=0;i<5;i++) test_evset(config_init(31, 2048, 64, 106,2091752), target);
    printf("evset size 32\n");
    for(int i=0;i<5;i++) test_evset(config_init(32, 2048, 64, 106,2091752), target);
    free(target);
}

int main(int ac, char** av) {
	wait(1E9);
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    CU_add_test(suite, "Test test_node", test_find_evset);
    CU_basic_run_tests();
    CU_cleanup_registry();
}