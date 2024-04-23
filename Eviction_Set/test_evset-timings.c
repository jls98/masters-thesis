#include "evset-timings.c"
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#define PAGESIZE 2097152 // hugepage 2 MB
#define TOTALACCESSES 100

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

static u64 test_buffer(Node **buffer, u64 total_size, u64 reps){
    conf = config_init(16, 131072, 64, 70, 2097152, 1, 1);
    Node *tmp=*buffer;
    Node **head=buffer;
    Node *next;
    u64 total_time=0;
    u64 *msrmts = malloc(reps*sizeof(u64));
    for(int i=1;i*64<total_size;i++){
        access(tmp);
        tmp=tmp->next;
    }
    next=tmp->next;
    tmp->next=NULL;    
    printf("pre shuffle");
    list_shuffle(head);
    printf(" post shufflee\n");
    tmp=*head;
    
    // for(int i=1;i*64<2*total_size;i++){
    //     access(tmp);
    //     access(tmp->next);
    //     access(tmp);
    //     access(tmp->next);
    //     tmp=tmp->next;
    // }

    traverse_list0_limited(tmp);
    traverse_list0_limited(tmp);
    tmp=*head;
    for(int i=0;i<reps;i++){ // unstable TODO
        msrmts[i]=probe_chase_loop(tmp, total_size);        
    }   
    
    // return to old state, last element points to next, first is new head and reset prev
    tmp->prev->next=next;
    *buffer=*head;
    (*head)->prev=NULL;

    // printf("[!] msrmts\n");
    for(int i=0;i<reps;i++){
            total_time+=msrmts[i];
    //     printf("%lu; ", msrmts[i]);
    }    
    printf("\n[+] Results for buffer size %lu: total time %lu, avg %lu, median %lu, median avg %lu\n", total_size, total_time, total_time/total_size, median_uint64(msrmts, reps), median_uint64(msrmts, reps)/total_size);    
    free(msrmts);
    
    return total_time;
}

void test_cache_size(){
    wait(1E9);
    // allocate 
    Node *buf= (Node *) mmap(NULL, 8*PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if(buf==MAP_FAILED){
        printf("mmap failed\n");
        return;
    }
    
    if (madvise(buf, 8*PAGESIZE, MADV_HUGEPAGE) ==-1){
        printf("advise failed!\n");
        return;
    }
    list_init(buf, 8*PAGESIZE);
    Node *tmp;
    u64 total_time=0;
    u64 buffer_size;
    Node **buffer=&buf;
    
    buffer_size = 16384;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);  

    buffer_size = 24384;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 26384;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);    
    buffer_size = 28384;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 30384;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 32768;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);    
    buffer_size = 34768;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);    
    buffer_size = 36768;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);    
    buffer_size = 38768;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);

    buffer_size = 65536;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 131072;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 262144;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 524288;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 1048576;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);

    buffer_size = 1848576;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 2097152;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
    buffer_size = 2297152;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);

    buffer_size = 4194304;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);    
    buffer_size = 8388608;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);    
    buffer_size = 16777216;
    total_time=test_buffer(buffer, buffer_size, TOTALACCESSES);
}

int main(int ac, char **av) {

	wait(1E9);
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);

    CU_add_test(suite, "Test test_node", test_node);
    CU_add_test(suite, "Test test_cache_size", test_cache_size);

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
