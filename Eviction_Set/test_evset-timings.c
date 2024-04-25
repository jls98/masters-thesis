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

static u64 test_buffer(Node **buf, u64 total_size, u64 reps){
    Node *tmp=*buf;
    Node **head=buf;
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

    traverse_list0(tmp);
    traverse_list0(tmp);
    tmp=*head;
    for(int i=0;i<reps;i++){ // unstable TODO
        msrmt[i]=probe_chase_loop(tmp, total_size);        
    }   
    
    // return to old state, last element points to next, first is new head and reset prev
    tmp->prev->next=next;
    *buf=*head;
    (*head)->prev=NULL;

    for(int i=0;i<reps;i++){
            total_time+=msrmt[i];
    }    
    u64 median=median_uint64(msrmt, reps);
    printf("\n[+] Results for buffer size %lu: total time %lu, avg %lu, median %lu, median avg %lu\n", total_size, total_time, total_time/total_size, median, median/total_size);    
    free(msrmt);
    msrmt=NULL;    
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
    // tmp_buffer_size = 8388608;
    // total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);    
    // tmp_buffer_size = 16777216;
    // total_time=test_buffer(buffer, tmp_buffer_size, TOTALACCESSES);

    munmap(buf, buf_size);
}

void test_cache_timings1(){
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
    msrmnt1[2] =probe_chase_loop(target, 1);
    msrmnt1[4] = probe_evset_chase(*head1); // miss
    msrmnt1[5] =probe_chase_loop(target, 1);
    msrmnt1[6] =probe_chase_loop(target, 1);
    msrmnt1[7] = probe_evset_chase(*head1); // miss
    msrmnt1[8] = probe_evset_chase(*head1); // hit
    msrmnt1[9] = probe_evset_chase(*head1); // hit

    msrmnt1[10] =probe_chase_loop(target, 1); // miss (probe_chase_loop more realistic since usually no control over targets content)
    msrmnt1[11] =probe_chase_loop(target, 1); // hit
    msrmnt1[12] =probe_chase_loop(target, 1); // hit
    msrmnt1[13] = probe_evset_chase(*head1); // miss


    for(int i=0;i<14;i++) printf("[+] msrmnt %i %lu\n", i, msrmnt1[i]);
    close_evsets();
    free(msrmnt1);
}

void test_cache_timings2(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =105; // arbitrary offset
    // init buffer
    u64 buf_size = 8*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    buffer_ptr=&buf;
    list_init(buf, buf_size);  
    

    // create evsets manually and test them with targets

    conf = config_init(16, 131072, 64, 70, 2097152, 1, 1); // L2
    size_stride = 2048;
    index = 120*size_stride + offset;

    Node *target = list_take(buffer_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset

    // add 1st elem
    index = 15*size_stride+offset;
    tmp=list_take(buffer_ptr, &index);
    list_append(head1, tmp);

    // add more elems
    for(int i=14; i >= 0; i--){
        index = i*size_stride + offset;
        tmp=list_take(buffer_ptr, &index);
        list_append(head1, tmp);
    }

    list_shuffle(head1);
    list_print(head1);

    u64 *msrmnt1=malloc(1000*sizeof(u64));

    msrmnt1[0] = probe_evset_chase(*head1); // miss miss
    msrmnt1[1] = probe_evset_chase(*head1); // hit
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
    close_evsets();
    free(msrmnt1);
}

void test_L1_evset(){
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
    // access(target);
    msrmnt1[2] =probe_chase_loop(target, 1);
    msrmnt1[4] = probe_evset_chase(*head1); // miss
    msrmnt1[5] =probe_chase_loop(target, 1);
    msrmnt1[6] =probe_chase_loop(target, 1);
    // access(target);
    // access(target);
    msrmnt1[7] = probe_evset_chase(*head1); // miss
    msrmnt1[8] = probe_evset_chase(*head1); // hit
    msrmnt1[9] = probe_evset_chase(*head1); // hit

    msrmnt1[10] =probe_chase_loop(target, 1); // miss (probe_chase_loop more realistic since usually no control over targets content)
    msrmnt1[11] =probe_chase_loop(target, 1); // hit
    msrmnt1[12] =probe_chase_loop(target, 1); // hit
    // msrmnt1[10] =probe_evset_chase(target); // miss
    // msrmnt1[11] =probe_evset_chase(target); // hit
    // msrmnt1[12] =probe_evset_chase(target); // hit
    msrmnt1[13] = probe_evset_chase(*head1); // miss


    for(int i=0;i<14;i++) printf("[+] msrmnt %i %lu\n", i, msrmnt1[i]);
    close_evsets();
    free(msrmnt1);
}

void test_L2_evset(){

}

void test_evset_state(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =105; // arbitrary offset
    // TODO init buffer
    u64 buf_size = 20*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    buffer_ptr=&buf;
    list_init(buf, buf_size);  
    Node **my_evset = (Node *) malloc(8*sizeof(Node *));

    // create evsets manually and test them with targets

    conf = config_init(8, 4096, 64, 95, 32768, 1, 1); // L1
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
    my_evset[7] = tmp;
    // add more elems
    for(int i=6; i >= 0; i--){
        index = i*size_stride + offset;
        tmp=list_take(buffer_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    list_shuffle(head1);
    list_print(head1);

    u64 *msrmnt1=malloc(100*sizeof(u64));
    u64 *msrmnt2=malloc(100*sizeof(u64));

    msrmnt1[0] = probe_evset_chase(*head1); // miss miss
    msrmnt1[1] = probe_evset_chase(*head1); // hit
    access(target);
    msrmnt2[0] =probe_chase_loop(my_evset[0], 1);
    msrmnt2[1] =probe_chase_loop(my_evset[0], 1);
    msrmnt2[2] =probe_chase_loop(my_evset[0], 1);
    msrmnt1[2] = probe_evset_chase(*head1); // hit
    msrmnt1[3] = probe_evset_chase(*head1); // hit
    access(target);
    msrmnt2[3] =probe_chase_loop(my_evset[1], 1);
    msrmnt2[4] =probe_chase_loop(my_evset[1], 1);
    msrmnt2[5] =probe_chase_loop(my_evset[1], 1);
    msrmnt1[4] = probe_evset_chase(*head1); // hit
    msrmnt1[5] = probe_evset_chase(*head1); // hit
    access(target);
    msrmnt2[6] =probe_chase_loop(my_evset[2], 1);
    msrmnt2[7] =probe_chase_loop(my_evset[2], 1);
    msrmnt2[8] =probe_chase_loop(my_evset[2], 1);
    msrmnt1[6] = probe_evset_chase(*head1); // hit 
    msrmnt1[7] = probe_evset_chase(*head1); // miss
    access(target);
    msrmnt2[9] =probe_chase_loop(my_evset[3], 1);
    msrmnt2[10] =probe_chase_loop(my_evset[3], 1);
    msrmnt2[11] =probe_chase_loop(my_evset[3], 1);
    msrmnt1[8] = probe_evset_chase(*head1); // hit
    msrmnt1[9] = probe_evset_chase(*head1); // hit
    access(target);
    msrmnt2[12] =probe_chase_loop(my_evset[4], 1);
    msrmnt2[13] =probe_chase_loop(my_evset[4], 1);
    msrmnt2[14] =probe_chase_loop(my_evset[4], 1);
    msrmnt1[10] = probe_evset_chase(*head1); // miss
    msrmnt1[11] = probe_evset_chase(*head1); // miss
    access(target);
    msrmnt2[15] =probe_chase_loop(my_evset[5], 1);
    msrmnt2[16] =probe_chase_loop(my_evset[5], 1);
    msrmnt2[17] =probe_chase_loop(my_evset[5], 1);
    msrmnt1[12] = probe_evset_chase(*head1); // miss
    msrmnt1[13] = probe_evset_chase(*head1); // miss
    access(target);
    msrmnt2[18] =probe_chase_loop(my_evset[6], 1);
    msrmnt2[19] =probe_chase_loop(my_evset[6], 1);
    msrmnt2[20] =probe_chase_loop(my_evset[6], 1);
    msrmnt1[14] = probe_evset_chase(*head1); // miss
    msrmnt1[15] = probe_evset_chase(*head1); // miss
    access(target);
    msrmnt2[21] =probe_chase_loop(my_evset[7], 1);
    msrmnt2[22] =probe_chase_loop(my_evset[7], 1);
    msrmnt2[23] =probe_chase_loop(my_evset[7], 1);
    msrmnt1[16] = probe_evset_chase(*head1); // miss
    msrmnt1[17] = probe_chase_loop(target, 1); // miss
    msrmnt1[18] = probe_chase_loop(target, 1); // miss
    traverse_list0(*head1);
    traverse_list0(*head1);
    traverse_list0(*head1);
    traverse_list0(*head1);
    traverse_list0(*head1);
    traverse_list0(*head1);
    traverse_list0(*head1);
    msrmnt1[19] = probe_chase_loop(target, 1); // miss

    for(int i=0;i<20;i++) printf("[+] msrmnt1 %2d %3lu\n", i, msrmnt1[i]);
    for(int i=0;i<24;i++) printf("[+] msrmnt2 %2d %3lu\n", i, msrmnt2[i]);
    close_evsets();
    free(my_evset);
    free(msrmnt1);
    free(msrmnt2);
}

#define MSRMNT_CNT 100
#define EVSET_TARGETS 25
#define EVSET_L2 16
#define EVSET_L1 8

void intern_access(Node **head1, Node **my_evset, u64 *msrmnt_, u64 index){
    for(int c=0;c<MSRMNT_CNT;c++){
        
        // flush evset    
        for(int i=0;i<EVSET_TARGETS;i++) flush(my_evset[i]);

        // access whole evset +1 (9 elems)
        traverse_list0(*head1);
        // measure access time for one entry
        msrmnt_[c]=probe_chase_loop(my_evset[index], 1);       
    }
}

void intern_access_tar(Node **head1, Node **head2, Node **my_evset1, Node **my_evset2, u64 *msrmnt_, u64 index, void *target){
    for(int c=0;c<MSRMNT_CNT;c++){
        

        // access(target);
        // access(target);
        // access(target);
        // access(target);
        access(target);
        traverse_list0(*head2);
        traverse_list0(*head1);
        access(target+222);
        // measure access time for one entry
        msrmnt_[c]=probe_chase_loop(target, 1);       
    }
}

void intern_access_new(Node **head1, Node **my_evset, u64 *msrmnt_, void * target){
    for(int c=0;c<MSRMNT_CNT;c++){
        // access(target);
        // access(target);
        // access(target);
        // access(target);
        access(target);       
        // traverse_list0(*head1);
        traverse_list0(*head1);
        access(target+222);

        // measure access time for one entry
        msrmnt_[c]=probe_chase_loop(target, 1);       
    }
}

uint64_t findMin(const uint64_t *array, size_t n) {
    // Handle empty array case
    if (n == 0) return 0; // Or any other appropriate value

    uint64_t min = array[0];
    for (size_t i = 1; i < n; ++i) if (array[i] < min) min = array[i];
    return min;
}

uint64_t findMax(const uint64_t *array, size_t n) {
    // Handle empty array case
    if (n == 0) return 0; // Or any other appropriate value  
    
    uint64_t max = array[0];
    for (size_t i = 1; i < n; ++i) if (array[i] > max) max = array[i];
    return max;
}

void replacement_L1(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =105; // arbitrary offset
    // init buffer
    u64 buf_size = 20*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    Node **buf_ptr=&buf;
    list_init(buf, buf_size);  
    Node **my_evset = (Node *) malloc(9*sizeof(Node *));
    *my_evset=NULL;
    // create evsets manually and test them with targets

    conf = config_init(9, 4096, 64, 95, 32768, 1, 1); // L1
    size_stride = 64;
    index = 120*size_stride + offset;

    Node *target = list_take(buf_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset
    *head1 =NULL;
    // add 1st elem
    index = 8*size_stride+offset;
    tmp=list_take(buf_ptr, &index);
    
    list_append(head1, tmp);
    my_evset[8] = tmp;
    // add more elems
    for(int i=7; i >= 0; i--){
        index = i*size_stride + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    list_shuffle(head1);

    u64 *msrmnt0=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt1=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt2=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt3=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt4=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt5=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt6=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt7=malloc(MSRMNT_CNT*sizeof(u64));
    u64 *msrmnt8=malloc(MSRMNT_CNT*sizeof(u64));
    // preparation done

    u64 aaaaa=0;
    for(tmp=*head1;aaaaa++<conf->ways;tmp=tmp->next){
        for(int bbbb=0;bbbb<9;bbbb++){
            if(tmp==my_evset[bbbb]) printf("%i %p\n", bbbb, tmp);
        }        
    }   
    
    // multiple measurements
    intern_access(head1, my_evset, msrmnt0, 0);
    intern_access(head1, my_evset, msrmnt1, 1);
    intern_access(head1, my_evset, msrmnt2, 2);
    intern_access(head1, my_evset, msrmnt3, 3);
    intern_access(head1, my_evset, msrmnt4, 4);
    intern_access(head1, my_evset, msrmnt5, 5);
    intern_access(head1, my_evset, msrmnt6, 6);
    intern_access(head1, my_evset, msrmnt7, 7);
    intern_access(head1, my_evset, msrmnt8, 8);

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt0[i]);
    printf("\n\n");
    printf("0: median %lu high %lu low %lu\n", median_uint64(msrmnt0, MSRMNT_CNT), findMax(msrmnt0, MSRMNT_CNT), findMin(msrmnt0, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt1[i]);
    printf("\n\n");
    printf("1: median %lu high %lu low %lu\n", median_uint64(msrmnt1, MSRMNT_CNT), findMax(msrmnt1, MSRMNT_CNT), findMin(msrmnt1, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt2[i]);
    printf("\n\n");
    printf("2: median %lu high %lu low %lu\n", median_uint64(msrmnt2, MSRMNT_CNT), findMax(msrmnt2, MSRMNT_CNT), findMin(msrmnt2, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt3[i]);
    printf("\n\n");
    printf("3: median %lu high %lu low %lu\n", median_uint64(msrmnt3, MSRMNT_CNT), findMax(msrmnt3, MSRMNT_CNT), findMin(msrmnt3, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt4[i]);
    printf("\n\n");
    printf("4: median %lu high %lu low %lu\n", median_uint64(msrmnt4, MSRMNT_CNT), findMax(msrmnt4, MSRMNT_CNT), findMin(msrmnt4, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt5[i]);
    printf("\n\n");
    printf("5: median %lu high %lu low %lu\n", median_uint64(msrmnt5, MSRMNT_CNT), findMax(msrmnt5, MSRMNT_CNT), findMin(msrmnt5, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt6[i]);
    printf("\n\n");
    printf("6: median %lu high %lu low %lu\n", median_uint64(msrmnt6, MSRMNT_CNT), findMax(msrmnt6, MSRMNT_CNT), findMin(msrmnt6, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt7[i]);
    printf("\n\n");
    printf("7: median %lu high %lu low %lu\n", median_uint64(msrmnt7, MSRMNT_CNT), findMax(msrmnt7, MSRMNT_CNT), findMin(msrmnt7, MSRMNT_CNT));

    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt8[i]);
    printf("\n\n");
    printf("8: median %lu high %lu low %lu\n", median_uint64(msrmnt8, MSRMNT_CNT), findMax(msrmnt8, MSRMNT_CNT), findMin(msrmnt8, MSRMNT_CNT));
    printf("\n");    

    close_evsets();
    free(my_evset);
    free(msrmnt1);
    free(msrmnt2);
}



void replacement_L2(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride_L2=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 size_stride_L1=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =32768; // arbitrary offset, 32768*64 = 2 MB
    // init buffer
    u64 buf_size = 20*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    Node **buf_ptr=&buf;
    list_init(buf, buf_size);  
    Node **my_evset = (Node *) malloc(EVSET_TARGETS*sizeof(Node *));
    *my_evset=NULL;
    // create evsets manually and test them with targets

    conf = config_init(EVSET_TARGETS, 4096, 64, 105, 2097152, 1, 1); // L1
    size_stride_L2 = 2048;
    size_stride_L1 = 64;
    index = 120*size_stride_L2 + offset;

    Node *target = list_take(buf_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset
    *head1 =NULL;
    // add 1st elem
    index = (EVSET_L2-1+1)*size_stride_L2+offset; // -1 to be in 1 block, +1 for test 1 more
    tmp=list_take(buf_ptr, &index);
    printf("highest val %p\n", tmp);
    list_append(head1, tmp);
    my_evset[EVSET_TARGETS-1] = tmp;
    // add 14 L2 elems
    for(int i=EVSET_TARGETS-2; i > EVSET_L1; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = (i-8)*size_stride_L2 + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    // add 8 L1 elems
    for(int i=EVSET_L1; i >= 0; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = i*size_stride_L1 + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }    
    printf("lowest val %p\n", tmp);

    // add last L2 elem (0)
    list_print(head1);
    list_shuffle(head1);

    u64 *msrmnt0=malloc(MSRMNT_CNT*EVSET_TARGETS*sizeof(u64));
    // preparation done

    u64 aaaaa=0;
    for(tmp=*head1;aaaaa++<conf->ways;tmp=tmp->next){
        for(int bbbb=0;bbbb<EVSET_TARGETS;bbbb++){
            if(tmp==my_evset[bbbb]) printf("%2d %p\n", bbbb, tmp);
        }        
    }   
    
    // multiple measurements
    for (int i=0;i<EVSET_TARGETS;i++){
        intern_access(head1, my_evset, msrmnt0+MSRMNT_CNT*i, i);
    }

    for(int j=0;j<EVSET_TARGETS;j++){
        printf("%2d:\n", j);
        for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt0[i+j*MSRMNT_CNT]);
        printf("\n");
        printf("median %lu high %lu low %lu\n", median_uint64(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT), findMax(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT), findMin(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT));
        printf("\n\n");

    }

    close_evsets();
    free(my_evset);
    free(msrmnt0);
    free(head1);
    munmap(buf, buf_size);
}

void replacement_L2_2(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride_L2=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 size_stride_L1=0; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =32768; // arbitrary offset, 32768*64 = 2 MB
    // init buffer
    u64 buf_size = 20*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    Node **buf_ptr=&buf;
    list_init(buf, buf_size);  
    Node **my_evset1 = (Node *) malloc(EVSET_L1*sizeof(Node *));
    Node **my_evset2 = (Node *) malloc(EVSET_L2*sizeof(Node *));
    *my_evset1=NULL;
    *my_evset2=NULL;
    // create evsets manually and test them with targets

    conf = config_init(EVSET_TARGETS, 4096, 64, 105, 2097152, 1, 1); // L1
    size_stride_L2 = 2048;
    size_stride_L1 = 64;
    index = 120*size_stride_L2 + offset;

    Node *target = list_take(buf_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset
    Node **head2 = malloc(sizeof(Node *)); // refs to first element of evset
    *head1 =NULL;
    *head2 =NULL;

    // add 1st elem to L2 evset
    index = (EVSET_L2-1)*size_stride_L2+offset; // -1 to be in 1 block, +1 for test 1 more
    tmp=list_take(buf_ptr, &index);
    list_append(head2, tmp);
    my_evset2[EVSET_L2-1] = tmp;
    // add 14 L2 elems
    for(int i=EVSET_L2-2; i > 0; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = i*size_stride_L2 + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head2, tmp);
        my_evset2[i]=tmp;
    }

    // add 1st elem to L2 evset, but index 0 will go to L2 evset
    index = EVSET_L1*size_stride_L1+offset; // -1 to be in 1 block, +1 for test 1 more
    tmp=list_take(buf_ptr, &index);
    list_append(head1, tmp);
    my_evset2[EVSET_L1-1] = tmp;
    // add 7 L1 elems
    for(int i=EVSET_L1-1; i > 0; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = i*size_stride_L1 + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset1[i-1]=tmp;
    }    
    index = offset; 
    tmp=list_take(buf_ptr, &index);
    list_append(head2, tmp);
    my_evset2[0] = tmp;
   // add last L2 elem (0)


    list_print(head1);
    list_print(head2);
    list_shuffle(head1);
    list_shuffle(head2);

    u64 *msrmnt0=malloc(MSRMNT_CNT*sizeof(u64));

    intern_access_tar(head1, head2, my_evset1, my_evset2, msrmnt0, 0, target);

    printf("target:\n");
    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt0[i]);
    printf("\n");
    printf("median %lu high %lu low %lu\n", median_uint64(msrmnt0, MSRMNT_CNT), findMax(msrmnt0, MSRMNT_CNT), findMin(msrmnt0, MSRMNT_CNT));
    printf("\n\n");


    close_evsets();
    free(my_evset1);
    free(my_evset2);
    free(head1);
    free(head2);
    free(msrmnt0);
    munmap(buf, buf_size);
}

/* assumption all hugepages are allocated contiguously, so if I take elements for my evset from multiple hugepages, 
 * they will still have the same delta*x on their physical addresses, so taking 24 elements with offset 2048*64 Bytes 
 * over at least two hugepages will still evict a respective target ^.^
 */
#define EVSET_L2_NEW 24
void replacement_L2_only_L2(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride_L2=2048; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =32768; // arbitrary offset, 32768*64 = 2 MB
    // init buffer
    u64 buf_size = 20*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    Node **buf_ptr=&buf;
    list_init(buf, buf_size);  
    Node **my_evset = (Node *) malloc(EVSET_L2_NEW*sizeof(Node *));
    *my_evset=NULL;
    // create evsets manually and test them with targets

    conf = config_init(EVSET_L2_NEW, 4096, 64, 120, 2097152, 1, 1); // L1
    index = 120*size_stride_L2 + offset;

    Node *target = list_take(buf_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset
    *head1 =NULL;
    // add 1st elem
    index = (EVSET_L2_NEW-1)*size_stride_L2+offset; // -1
    tmp=list_take(buf_ptr, &index);
    list_append(head1, tmp);

    my_evset[EVSET_L2_NEW-1] = tmp;
    // add 14 L2 elems
    for(int i=EVSET_L2_NEW-2; i >=0; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = i*size_stride_L2 + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    list_print(head1);
    list_shuffle(head1);

    // init buffer to store measurements
    u64 *msrmnt0=malloc(MSRMNT_CNT*sizeof(u64));
    // preparation done

    // multiple measurements
    intern_access_new(head1, my_evset, msrmnt0, target);


    printf("target timings:\n");
    int j=0;
    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt0[i+j*MSRMNT_CNT]);
    printf("\n");
    printf("median %lu high %lu low %lu\n", median_uint64(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT), findMax(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT), findMin(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT));
    printf("\n\n");

    close_evsets();
    free(my_evset);
    free(msrmnt0);
    free(head1);
    munmap(buf, buf_size);
}


void replacement_L2_only_L2_mmap_file(){
    wait(1E9);
    Node *tmp=NULL;    // to hold tmp Nodes
    u64 index=0;    // holds index
    u64 size_stride_L2=2048; // holds current stride size in index value (size_stride*64 = X in Bytes)
    u64 offset =32768; // arbitrary offset, 32768*64 = 2 MB
    // init buffer
    u64 buf_size = 20*PAGESIZE;     
    Node *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    Node **buf_ptr=&buf;
    list_init(buf, buf_size);  
    Node **my_evset = (Node *) malloc(EVSET_L2_NEW*sizeof(Node *));
    *my_evset=NULL;
    // create evsets manually and test them with targets

    conf = config_init(EVSET_L2_NEW, 4096, 64, 120, 2097152, 1, 1); // L1
    index = 120*size_stride_L2 + offset;

    Node *target = list_take(buf_ptr, &index);
    target->next=target;
    target->prev=target;

    printf("[!] target %p\n", target);
    Node **head1 = malloc(sizeof(Node *)); // refs to first element of evset
    *head1 =NULL;
    // add 1st elem
    index = (EVSET_L2_NEW-1)*size_stride_L2+offset; // -1
    tmp=list_take(buf_ptr, &index);
    list_append(head1, tmp);

    my_evset[EVSET_L2_NEW-1] = tmp;
    // add 14 L2 elems
    for(int i=EVSET_L2_NEW-2; i >=0; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = i*size_stride_L2 + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    list_print(head1);
    list_shuffle(head1);

    // init buffer to store measurements
    u64 *msrmnt0=malloc(MSRMNT_CNT*sizeof(u64));
    // preparation done

    // u64 aaaaa=0;
    // for(tmp=*head1;aaaaa++<conf->ways;tmp=tmp->next){
    //     for(int bbbb=0;bbbb<EVSET_TARGETS;bbbb++){
    //         if(tmp==my_evset[bbbb]) printf("%2d %p\n", bbbb, tmp);
    //     }        
    // }   
    
    // multiple measurements
    intern_access_new(head1, my_evset, msrmnt0, target);


    printf("target timings:\n");
    int j=0;
    for(int i=0;i<MSRMNT_CNT;i++) printf("%lu; ", msrmnt0[i+j*MSRMNT_CNT]);
    printf("\n");
    printf("median %lu high %lu low %lu\n", median_uint64(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT), findMax(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT), findMin(msrmnt0+j*MSRMNT_CNT, MSRMNT_CNT));
    printf("\n\n");

    

    close_evsets();
    free(my_evset);
    free(msrmnt0);
    free(head1);
    munmap(buf, buf_size);
}

int main(int ac, char **av) {

	wait(1E9);
    CU_initialize_registry();

    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);

    // CU_add_test(suite, "Test test_node", test_node);
    // CU_add_test(suite, "Test test_cache_size", test_cache_size);
    // CU_add_test(suite, "Test test_cache_timings1", test_cache_timings1);
    // CU_add_test(suite, "Test test_cache_timings2", test_cache_timings2);
    // CU_add_test(suite, "Test test_L1_evset", test_L1_evset);
    // CU_add_test(suite, "Test test_L2_evset", test_L2_evset);
    // CU_add_test(suite, "Test test_evset_state", test_evset_state);
    // CU_add_test(suite, "Test replacement_L1", replacement_L1);

    CU_basic_run_tests();
    CU_cleanup_registry();
    replacement_L2();
    replacement_L2_2();
    replacement_L2_only_L2();
    return 0;
}
