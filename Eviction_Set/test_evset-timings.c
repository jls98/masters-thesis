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
    Node *tmp=*buffer;
    Node **head=buffer;
    Node *next;
    u64 total_time=0;
    u64 *msrmt = malloc(reps*sizeof(u64));
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

    traverse_list0(tmp);
    traverse_list0(tmp);
    tmp=*head;
    for(int i=0;i<reps;i++){ // unstable TODO
        msrmt[i]=probe_chase_loop(tmp, total_size);        
    }   
    
    // return to old state, last element points to next, first is new head and reset prev
    tmp->prev->next=next;
    *buffer=*head;
    (*head)->prev=NULL;

    // printf("[!] msrmt\n");
    for(int i=0;i<reps;i++){
            total_time+=msrmt[i];
    //     printf("%lu; ", msrmt[i]);
    }    
    printf("\n[+] Results for buffer size %lu: total time %lu, avg %lu, median %lu, median avg %lu\n", total_size, total_time, total_time/total_size, median_uint64(msrmts, reps), median_uint64(msrmts, reps)/total_size);    
    free(msrmt);
    close_evsets();
    
    return total_time;
}

void test_cache_size(){
    wait(1E9);
    // allocate
    conf = config_init(16, 131072, 64, 70, 2097152, 1, 1);
    u64 buf_size = 8*PAGESIZE; 
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if(buf==MAP_FAILED){
        printf("mmap failed\n");
        return;
    }
    
    if (madvise(buf, buf_size, MADV_HUGEPAGE) ==-1){
        printf("advise failed!\n");
        return;
    }
    list_init(buf, buf_size);
    Node *tmp;
    u64 total_time=0;
    u64 tmp_buffer_size;
    Node **buffer=&buf;
    
    tmp_buffer_size = 16384;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);  

    tmp_buffer_size = 24384;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 26384;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    tmp_buffer_size = 28384;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 30384;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 32768;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    tmp_buffer_size = 34768;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    tmp_buffer_size = 36768;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    tmp_buffer_size = 38768;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);

    tmp_buffer_size = 65536;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 131072;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 262144;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 524288;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 1048576;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);

    tmp_buffer_size = 1848576;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 2097152;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);
    tmp_buffer_size = 2297152;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);

    tmp_buffer_size = 4194304;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    tmp_buffer_size = 8388608;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    tmp_buffer_size = 16777216;
    total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);

    munmap(buf, buf_size);
}

void test_cache_timings(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =105; // arbitrary offset
    // TODO init buffer
    u64 buf_size = 8*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    buffer_ptr=&buf;
    list_init(buf, buf_size);  
    evsets = realloc(evsets, 16*sizeof(Node *));
    

    // create evsets manually and test them with targets

    conf = config_init(8, 4096, 64, 85, 32768, 1, 1); // L1
    size_stride = 64;
    index = 120*size_stride + offset;

    Node *target = list_take(buffer_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset

    // add 1st elem
    index = 7*size_stride+offset;
    tmp=list_take(buffer_ptr, &index);
    list_append(head1, tmp);

    // add more elems
    for(int i=6; i >= 0; i--){
        index = i*size_stride + offset;
        tmp=list_take(buffer_ptr, &index);
        list_append(head1, tmp);
    }

    list_shuffle(head1);
    list_print(head1);

    u64 *msrmnt1=malloc(1000*sizeof(u64));

    msrmnt1[0] = probe_evset_chase(*head1); // miss miss
    msrmnt1[1] = probe_evset_chase(*head1); // hit
    // msrmnt1[2] = probe(target);
    // msrmnt1[3] = probe(target);
    access(target);
    msrmnt1[4] = probe_evset_chase(*head1); // miss
    access(target);
    access(target);
    msrmnt1[5] = probe_evset_chase(*head1); // miss
    msrmnt1[6] = probe_evset_chase(*head1); // hit
    msrmnt1[7] = probe_evset_chase(*head1); // hit

    msrmnt1[10] =probe_chase_loop(target, 1); // miss
    msrmnt1[11] =probe_chase_loop(target, 1); // hit
    msrmnt1[12] =probe_chase_loop(target, 1); // hit
    msrmnt1[13] = probe_evset_chase(*head1); // miss


    for(int i=0;i<14;i++) printf("[+] msrmnt %i %lu\n", i, msrmnt1[i]);
    

    free(conf);
    conf = config_init(16, 131072, 64, 70, 2097152, 1, 1); // L2



    close_evsets();
}


int main(int ac, char **av) {

	wait(1E9);
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);

    // CU_add_test(suite, "Test test_node", test_node);
    // CU_add_test(suite, "Test test_cache_size", test_cache_size);
    CU_add_test(suite, "Test test_cache_timings", test_cache_timings);

    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
