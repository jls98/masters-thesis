#include "evset.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

void test_node(){
    Node *nodes= (Node *) mmap(NULL, 5000*sizeof(Node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0); 

    // test init 
    list_init(nodes, 5000*sizeof(Node));
    for(int i=1;i<4999;i++){  
        CU_ASSERT_EQUAL(nodes[i].prev, &nodes[i-1]);
        CU_ASSERT_EQUAL(nodes[i].next, &nodes[i+1]);
        CU_ASSERT_EQUAL(nodes[i].delta, 0);
    }
       
    // test get, take
    uint64_t zero=0;
    for(int i=0;i<5000;i++){
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
#define LALALALAL1 8 // TODO figure out why 9 is needed! why do brackets solve it??????
#define LALALALAL2 9

#define size_factor 99999999

#define REPS 100
void testbench_skylake_evsets(){
    Node *buffer = (Node *) mmap(NULL, size_factor*sizeof(Node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);    
    
    list_init(buffer, size_factor*sizeof(Node));
    uint64_t msrmts1=0;
    uint64_t msrmts2=0;
    uint64_t msrmts3=0;
    uint64_t offset = 3;
    probe((void *)buffer);
    __asm__ volatile("lfence;");
    for(int i=0;i<REPS;i++){
        
        msrmts[msr_index]=probe((void *)buffer);
        msrmts1+=msrmts[msr_index++];
    }
    for(int i=0;i<REPS;i++){
        printf("cached: %lu\n", msrmts[i]);
    }
    msr_index=0;
    
    for(int j=0;j<REPS;j++){
        for(u64 i=offset;i<(LALALALAL1+offset);i++){
            access(&buffer[i*1024]);
        }
        probe(((void *)buffer)+222);
        __asm__ volatile("lfence;");
        msrmts[msr_index]=probe((void *)buffer);    
        
        
        __asm__ volatile("lfence;");
        msrmts2+=msrmts[msr_index++];
    }
    for(int i=0;i<REPS;i++){
        printf("L1: %lu\n", msrmts[i]);
    }
    msr_index=0;

    
    probe((void *)buffer);
    __asm__ volatile("lfence;");
    for(int j=0;j<REPS;j++){
        for(u64 i=offset;i<(LALALALAL2+offset);i++){
            access(&buffer[i*2048]);
        }
        for(u64 i=offset;i<(LALALALAL2+offset);i++){
            access(&buffer[i*2048]);
        }
        for(u64 i=offset;i<(LALALALAL2+offset);i++){
            access(&buffer[i*2048]);
        }
        probe(((void *)buffer)+222);    
        __asm__ volatile("lfence;");
        msrmts[msr_index]=probe((void *)buffer);
        __asm__ volatile("lfence;");
        msrmts3+=msrmts[msr_index++];
    }
    for(int i=0;i<REPS;i++){
        printf("L3: %lu\n", msrmts[i]);
    }
    msr_index=0;
    
    printf("d: %lu %lu %lu\n", msrmts1, msrmts2, msrmts3);
    munmap(buffer, size_factor*sizeof(Node));    
}

#define REPS1 5
#define INDEX_OFFSET 10
#define PAGESIZE 2097152

#define EVSET1_SIZE_SKYLAKE 8
#define EVSET1_SIZE_SKYLAKE2 4
#define EVSET1_SIZE_ALDERLAKE2 16

#define STRIDE_SKYLAKE 2048 //(2048 weil 8 statt 4 Elemente?)
#define STRIDE_ALDERLAKE2 4096

#define STRIDE_SIZE STRIDE_ALDERLAKE2
#define EVSET1_SIZE EVSET1_SIZE_ALDERLAKE2
void test_test(){
    u64 bufsize = 2*PAGESIZE;
    Node *buffer = (Node *) mmap(NULL, bufsize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (madvise(buffer, bufsize, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    list_init(buffer, bufsize);
    Node **buffer_ptr=&buffer;
    u64 index;
    
    // init evset1
    index = STRIDE_SIZE+INDEX_OFFSET;
    Node *tmp = list_take(buffer_ptr, &index);
    Node **head1=malloc(sizeof(Node *));
    list_append(head1, tmp);
    
    for(int i=1;i<EVSET1_SIZE;i++){
        index=i*STRIDE_SIZE+STRIDE_SIZE+ INDEX_OFFSET-i;
        tmp=list_take(buffer_ptr, &index);
        list_append(head1, tmp);
    }    
    
    index =INDEX_OFFSET;
    void *target = (void *) list_take(buffer_ptr, &index); 
    
    list_shuffle(head1);
    
    // L1
    init_evset(config_init(16, 131072, 64, 70, 2097152, 1, 1));
    

    // evict
    for(int i=0;i<REPS1;i++){
        CU_ASSERT_TRUE(test(*head1, target)== 1);
    }   
    
    for(int i=0;i<REPS1;i++){
        printf("b: %lu\n", msrmts[i]);
    }
    printf("target %p \n", target);
    printf("print list\n");
    list_print(head1);
    munmap(buffer, bufsize);    
    
}
#define AMOUNT_HISTO 9999999
void test_get_histogram_data(){
    init_evset(config_init(8, 4096, 64, 39, 32768, 1, 1));  

    for(int size=5;size<27;size++){
        Node *buffer = (Node *) mmap(NULL, AMOUNT_HISTO*sizeof(Node), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if(buffer==MAP_FAILED){
            printf("mmap failed\n");
            return;
        }
        list_init(buffer, AMOUNT_HISTO*sizeof(Node));
        Node **buffer_ptr=&buffer;
        u64 index;
        
        // init evset1
        index = 2048+INDEX_OFFSET;
        Node *tmp = list_take(buffer_ptr, &index);
        Node **head1=malloc(sizeof(Node *));
        list_append(head1, tmp);

        for(int i=1;i<size;i++){
            index=i*2048+2048+ INDEX_OFFSET-i;
            tmp=list_take(buffer_ptr, &index);
            list_append(head1, tmp);
        } 
        index =INDEX_OFFSET;
        void *target = (void *) list_take(buffer_ptr, &index);  
        access(target);
        list_shuffle(head1);  
        for(int i=0;i<1000;i++){
            test(*head1, target);
        }        
        for(int i=0;i<1000;i++){
            printf("%i %lu\n", size, msrmts[i]);
        }      
        msr_index=0;
        munmap(buffer, AMOUNT_HISTO*sizeof(Node)); 
    }
}
#define STRIDE_REPS 5
#define ALDER_L2 2097152
#define SKYLAKE_L2 262144
#define CACHE_L1 32768
#define SIZE_VALUE ALDER_L2

void test_strides(){
    // stride * 2^5 since sizeof(Node) = 32
    int size = 2*SIZE_VALUE;
    Node **head;
    head=malloc(sizeof(Node *));
    *head=NULL;
    printf("stride (Bytes)| evset size | msrmts sum | msrmts per msr\n");

    for(int stride = 4; stride< 1<<17;stride=stride<<1){
        
        if(stride==SIZE_VALUE) break;
        Node *buffer = (Node *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
        if(buffer==MAP_FAILED){
            printf("mmap failed\n");
            return;
        }
        if (madvise(buffer, size, MADV_HUGEPAGE) == -1){
            printf("madvise failed!\n");
            return;
        }   
        Node **buffer_ptr=&buffer;
        list_init(buffer, size);
        
        Node *tmp;
        u64 index;
        for(u64 i=0;i<size/(stride*sizeof(Node));i++){
            index=i*stride-i;
            tmp=list_take(buffer_ptr, &index);
            
            list_append(head, tmp);
        }
        list_shuffle(head);
        // list_print(head);
        // if(!*head) break;
        msrmts[0]=0;
        int j=0;
        for(tmp=*head;tmp;tmp=tmp->next){
            probe(tmp);
            // test_intern(*head, *head);
        }
        for(tmp=*head;tmp;tmp=tmp->next){
            msrmts[0]+=probe(tmp);
            j++;
            // test_intern(*head, *head);
        }
        if(j==0) break;
        printf("%7i %5li %7lu %4li\n", stride<<5, size/(stride*sizeof(Node)), msrmts[0], msrmts[0]/j);
        // for(int i=0;i<STRIDE_REPS;i++){
            // printf("%i %lu\n", stride<<5, msrmts[i]);
        // }
        // msr_index=0;
        while(*head){
            list_pop(head);
        }
        // free(head);
        munmap(buffer, size);
    }
    free(head);
    
}

static void cache_line(){
    wait(1E9);
    void *ptr=  mmap(NULL, 2097152, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if(buffer==MAP_FAILED){
        printf("mmap failed\n");
        return;
    }
    if (madvise(buffer, 2097152, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }   
    u64 time_buf[1000];
    flush(ptr+4066);
    flush(ptr+4096);
    flush(ptr+4120);
    access(ptr+4096);
    access(ptr+4096);
    access(ptr+4096);
    __asm__ volatile ("lfence;");
    
    time_buf[1]=probe(ptr+4096);
    __asm__ volatile ("lfence;");
    flush(ptr+4096);
    __asm__ volatile ("lfence;");
    access(ptr+4096);
    access(ptr+4096);
    access(ptr+4096);
    __asm__ volatile ("lfence;");
    time_buf[0]=probe(ptr+4066);
    __asm__ volatile ("lfence;");
    flush(ptr+4066);
    __asm__ volatile ("lfence;");
    access(ptr+4096);
    access(ptr+4096);
    access(ptr+4096);
    __asm__ volatile ("lfence;");
    time_buf[2]=probe(ptr+4120);

    __asm__ volatile ("lfence;");
    
    printf("%lu %lu %lu\n", time_buf[0], time_buf[1], time_buf[2]);
    munmap(ptr, 2097152);

}

static void test_find_evset(){
    Config *con = config_init(16, 131072, 64, 70, 2097152, 1, 1); // test L2
    
    u64 *target=realloc(target, (i+1)*sizeof(u64));  
    Node **evset_ptr= find_evset(con, target);
    u64 test_result= test_intern(*evset_ptr, target);
    
    init_evset(con);
        
    CU_ASSERT_TRUE(test_result > con->threshold);
    del_evset();
    close_evsets();
    
    free(con);
    
}

static void test_probe_evset(){
    Config *con = config_init(16, 131072, 64, 70, 2097152, 1, 1); // test L2
    init_evset(con);
    u64 *target= malloc(sizeof(u64));
    Node **evset_ptr= find_evset(con, target);
    
    printf("test evset: b4 testing\n");
    for(int i=0;i<10;i++){
        if(!test(*evset_ptr, target)) {
            printf("evset unreliable, i is %i\n");
            return;
        }
    }
    printf("test evset: b4 trav\n");
    // load evset 
    traverse_list0(*evset_ptr);
    traverse_list0(*evset_ptr);
    
    // test just evset
    u64 *result = malloc(10*sizeof(u64));
    result[0]= probe_evset(*evset_ptr);
    result[1]= probe_evset(*evset_ptr);
    result[2]= probe_evset(*evset_ptr);
    result[3]= probe_evset(*evset_ptr);
    result[4]= probe_evset(*evset_ptr);
    
    // test target access    
    access(target);
    result[5]= probe_evset(*evset_ptr);
    access(target);
    result[6]= probe_evset(*evset_ptr);
    access(target);
    result[7]= probe_evset(*evset_ptr);
    access(target);
    result[8]= probe_evset(*evset_ptr);
    access(target);
    result[9]= probe_evset(*evset_ptr);   

    printf("result\n");
    for(int i=0;i<10;i++){
        printf("%lu\n", result[i]);
    }
    
    free(con);
    free(result);
    close_evsets();
    
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

    // CU_add_test(suite, "Test test_node", test_node);
    // CU_add_test(suite, "Test testbench_skylake_evsets", testbench_skylake_evsets);
    CU_add_test(suite, "Test test_test", test_test);
    CU_add_test(suite, "Test test_find_evset", test_find_evset);
    // CU_add_test(suite, "Test test_get_histogram_data", test_get_histogram_data);

    CU_basic_run_tests();
    CU_cleanup_registry();
	// test_get_histogram_data();
    // test_strides();
    // cache_line(); // madvise tend to fail??
    test_probe_evset();
    return 0;
}
