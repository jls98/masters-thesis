#include "evset.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void test_node(){
    Node *nodes= malloc(100*sizeof(Node));
    
    // test init 
    list_init(nodes, 100*sizeof(Node));
    for(int i=1;i<99;i++){  
        CU_ASSERT_EQUAL(nodes[i].prev, &nodes[i-1]);
        CU_ASSERT_EQUAL(nodes[i].next, &nodes[i+1]);
        CU_ASSERT_EQUAL(nodes[i].delta, 0);
    }
    
    
    // test get, take
    uint64_t zero=0;
    for(int i=0;i<100;i++){
        Node *ptr_node = &nodes[i];
        Node **ptr_ptr_node = &ptr_node;
        CU_ASSERT_EQUAL(&nodes[i], list_get(ptr_ptr_node, &zero));
        CU_ASSERT_EQUAL(&nodes[i], list_take(ptr_ptr_node, &zero));
    }
    
    // test append
    uint64_t index;
    for(int i=0;i<10;i++){
        index=i;
        list_append(&nodes, &nodes[i]);
        CU_ASSERT_EQUAL(&nodes[i], list_get(&nodes, &index));
    }
    
    // test pop
    Node *head=nodes;
    Node *tmp;
    for(int i=0;i<10;i++){
        tmp = list_pop(&head);
        CU_ASSERT_EQUAL(&nodes[i], tmp);
    }
    
    // test push
    head=nodes;
    for(int i=0;i<10;i++){
        index=i;
        list_push(&head, &nodes[i]);
        CU_ASSERT_EQUAL(nodes, list_get(&head, &index));
        CU_ASSERT_EQUAL(head, list_get(&head, &zero));
    }
    // later TODO init 
}

#define LALALALAL1 11
#define LALALALAL2 35
#define LALALALAL3 1026
#define LALALALAL4 1027
void test_test(){
    // TODO
    Node *test = (Node *) mmap(NULL, 99999999*sizeof(Node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 0, 0);    
    
    // malloc(1025*sizeof(Node)); // TODO change to mmap
    list_init(test, 99999999*sizeof(Node));
    uint64_t msrmts[2000];
    flush(test);
    for(int i=0;i<5;i++){
        msrmts[i]=probe((void *)test);
    }
    
    for(int i=3;i<LALALALAL1;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL1;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL1;i++){
        access(&test[i*128]);
    }
    probe(((void *)test)+222);
    
    for(int i=5;i<7;i++){
        msrmts[i]=probe((void *)test);
    }
    
    for(int i=3;i<LALALALAL2;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL2;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL2;i++){
        access(&test[i*128]);
    }
    probe(((void *)test)+222);
    
     for(int i=7;i<9;i++){
        msrmts[i]=probe((void *)test);
    }
    
    for(int i=3;i<LALALALAL3;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL3;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL3;i++){
        access(&test[i*128]);
    }
    probe(((void *)test)+222);
    
     for(int i=9;i<11;i++){
        msrmts[i]=probe((void *)test);
    }
    
    for(int i=3;i<LALALALAL4;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL4;i++){
        access(&test[i*128]);
    }
    for(int i=3;i<LALALALAL4;i++){
        access(&test[i*128]);
    }
    probe(((void *)test)+222);
    
     for(int i=11;i<13;i++){
        msrmts[i]=probe((void *)test);
    }
       
    
    for(int i=0;i<13;i++){
        printf("b: %lu\n", msrmts[i]);
    }  
    for(int i=1;i<9;i++){
        printf("%p\n", &test[i*128]);
    }
    munmap(test, 1025*sizeof(Node));    
}

int main(int ac, char **av) {
	// if (ac==1) conf = initConfig(8, 64, 41, 32768, 1, 1); // default L1 lab machine, no inputs
	// if (ac==2){
		// int conf_choice = strtol(av[1], NULL, 10);
		// if (conf_choice==11) conf = initConfig(8, 64, 54, 32768, 1, 1); 	// L1 i7
		// if (conf_choice==12) conf = initConfig(8, 64, 58, 262144, 1, 1); 	// L2 i7 what value?
		// if (conf_choice==13) conf = initConfig(8, 64, 58, 262144, 1, 1); 	// L3 i7 TODO or unneeded
		
		// if (conf_choice==21) conf = initConfig(8, 64, 75, 32768, 1, 1); 	// L1e i12
		// if (conf_choice==22) conf = initConfig(8, 64, 85, 2097152, 1, 1); 	// L2e i12 TODO
		// if (conf_choice==23) conf = initConfig(8, 64, 58, 262144, 1, 1); 	// L3e i12 TODO or unneeded
		// if (conf==NULL){
			// printf("Error, no valid choice, XY, whereby X is the CPU (1:i7, 2:i12) and Y the cache Level (1-3)");
			// return 0;
		// }
	// }
	// if (ac==7){ // self, ways, cache line size, threshold, cache size, test reps, hugepages
		// conf = initConfig(strtol(av[1], NULL, 10), strtol(av[2], NULL, 10), strtol(av[3], NULL, 10), strtol(av[4], NULL, 10), strtol(av[5], NULL, 10), strtol(av[6], NULL, 10));
	// }
	wait(1E9);
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);

    CU_add_test(suite, "Test test_node", test_node);
    CU_add_test(suite, "Test test_test", test_test);

    CU_basic_run_tests();
    CU_cleanup_registry();
	// free(conf);
	
    return 0;
}
