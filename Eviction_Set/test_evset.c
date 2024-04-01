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

// 37 cached, 42 L2, 52 L3 
#define LALALALAL1 8
#define LALALALAL2 9

#define size_factor 99999999

#define REPS 1000
void testbench_skylake_evsets(){
    Node *buffer = (Node *) mmap(NULL, size_factor*sizeof(Node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 0, 0);    
    
    list_init(buffer, size_factor*sizeof(Node));
    uint64_t msrmts1=0;
    uint64_t msrmts2=0;
    uint64_t msrmts3=0;
    uint64_t offset = 3;
    probe((void *)buffer);
    __asm__ volatile("lfence;");
    for(int i=0;i<REPS;i++){
        msrmts1+=probe((void *)buffer);
    }
    for(int i=0;i<REPS;i++){
        for(int i=offset;i<LALALALAL1+offset;i++){
            access(&buffer[i*1024]);
        }
        probe(((void *)buffer)+222);
        __asm__ volatile("lfence;");
        msrmts2+=probe((void *)buffer);        
        __asm__ volatile("lfence;");
    }
    
    probe((void *)buffer);
    __asm__ volatile("lfence;");
    for(int i=0;i<REPS;i++){
        for(int i=offset;i<LALALALAL2+offset;i++){
            access(&buffer[i*2048]);
        }
        for(int i=offset;i<LALALALAL2+offset;i++){
            access(&buffer[i*2048]);
        }
        for(int i=offset;i<LALALALAL2+offset;i++){
            access(&buffer[i*2048]);
        }
        probe(((void *)buffer)+222);    
        __asm__ volatile("lfence;");
        msrmts3+=probe((void *)buffer);
        __asm__ volatile("lfence;");
    }
    
    printf("b: %lu %lu %lu\n", msrmts1, msrmts2, msrmts3);
    munmap(buffer, size_factor*sizeof(Node));    
}

#define INDEX_OFFSET 10
void test_test(){
    Node *buffer = (Node *) mmap(NULL, size_factor*sizeof(Node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 0, 0);
    list_init(buffer, size_factor*sizeof(Node));
    Node **buffer_ptr=&buffer;
    u64 index;
    printf("asd\n");
    index = 1024+INDEX_OFFSET;
    Node *tmp = list_get(buffer_ptr, &index);
    printf("asd %p %p\n", tmp, &tmp);
    Node **head1=&tmp;
    printf("asd\n");
    index =262144 + INDEX_OFFSET;
    tmp = list_get(buffer_ptr, &index);
    Node **head2=&tmp;
     printf("asd\n");
   
    for(int i=1;i<LALALALAL1;i++){
        index=i*1024+1024+ INDEX_OFFSET;
        tmp=list_get(buffer_ptr, &index);
        list_append(head1, tmp);
        printf("appended %p index %lu\n", tmp, *index);
    }    
     printf("asd\n");
   
    for(int i=1;i<LALALALAL2;i++){
        index = i*2048+262144+(u64)INDEX_OFFSET;
        tmp=list_get(buffer_ptr, &index);
        list_append(head2, tmp);        
        printf("appended %p index %lu\n", tmp, *index);
    }
    index =INDEX_OFFSET;
    void *target = (void *) list_get(buffer_ptr, &index); 
    printf("target %p\n", target);



    printf("asd\n");

    // L1
    init_evset(config_init(8, 4096, 64, 39, 32768, 1, 1));
    
    access(target);
    access(target);
    access(target);
    access(target);
    // cached 
    for(int i=0;i<REPS;i++){
        CU_ASSERT_TRUE(test(*head1, 7, target)== 0);
    }   
    for(int i=0;i<1000;i++) printf("%lu; ", msrmts_buf[i]);
    printf("\n");
    msrmts_ind=0;


    
    // not cached 
    access(target);
    access(target);
    access(target);
    access(target);
    // cached 
    for(int i=0;i<REPS;i++){
        CU_ASSERT_TRUE(test(*head1, LALALALAL1, target)== 1);
    }   
    for(int i=0;i<1000;i++) printf("%lu; ", msrmts_buf[i]);
    printf("\n");    
    munmap(buffer, size_factor*sizeof(Node));    
    
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
    CU_add_test(suite, "Test testbench_skylake_evsets", testbench_skylake_evsets);
    CU_add_test(suite, "Test test_test", test_test);

    CU_basic_run_tests();
    CU_cleanup_registry();
	// free(conf);
	
    return 0;
}
