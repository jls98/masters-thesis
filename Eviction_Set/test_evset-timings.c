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
    // msrmnt1[10] = probe_evset_chase(target); // miss
    // msrmnt1[11] = probe_evset_chase(target); // hit
    // msrmnt1[12] = probe_evset_chase(target); // hit
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

void intern_access(Node **head1, Node **my_evset, u64 *msrmnt_){
    for(int c=0;c<100;c++){
        
        // flush evset    
        for(int i=0;i<9;i++) flush(my_evset[i]);

        // access whole evset +1 (9 elems)
        // probe_evset_chase(*head1);
        traverse_list0(*head1);
        // measure access time for one entry
        msrmnt_[c]=probe_chase_loop(my_evset[0], 1);       
    }
}

uint64_t findMin(const uint64_t *array, size_t n) {
    if (n == 0) {
        // Handle empty array case
        return 0; // Or any other appropriate value
    }
    
    uint64_t min = array[0];
    
    for (size_t i = 1; i < n; ++i) {
        if (array[i] < min) {
            min = array[i];
        }
    }
    
    return min;
}

uint64_t findMax(const uint64_t *array, size_t n) {
    if (n == 0) {
        // Handle empty array case
        return 0; // Or any other appropriate value
    }
    
    uint64_t max = array[0];
    
    for (size_t i = 1; i < n; ++i) {
        if (array[i] > max) {
            max = array[i];
        }
    }
    
    return max;
}
void replacement_L1(){
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
    list_print(head1);

    u64 *msrmnt0=malloc(100*sizeof(u64));
    u64 *msrmnt1=malloc(100*sizeof(u64));
    u64 *msrmnt2=malloc(100*sizeof(u64));
    u64 *msrmnt3=malloc(100*sizeof(u64));
    u64 *msrmnt4=malloc(100*sizeof(u64));
    u64 *msrmnt5=malloc(100*sizeof(u64));
    u64 *msrmnt6=malloc(100*sizeof(u64));
    u64 *msrmnt7=malloc(100*sizeof(u64));
    u64 *msrmnt8=malloc(100*sizeof(u64));
    // preparation done

    u64 aaaaa=0;
    for(tmp=*head1;aaaaa++<conf->ways;tmp=tmp->next){
        for(int bbbb=0;bbbb<9;bbbb++){
            if(tmp==my_evset[bbbb]) printf("%i %p\n", bbbb, tmp);
        }        
    }    
    
    
    
    
    // multiple measurements
    intern_access(head1, my_evset, msrmnt0);
    intern_access(head1, my_evset, msrmnt1);
    intern_access(head1, my_evset, msrmnt2);
    intern_access(head1, my_evset, msrmnt3);
    intern_access(head1, my_evset, msrmnt4);
    intern_access(head1, my_evset, msrmnt5);
    intern_access(head1, my_evset, msrmnt6);
    intern_access(head1, my_evset, msrmnt7);
    intern_access(head1, my_evset, msrmnt8);

    printf("0: median %lu high %lu low %lu\n", median_uint64(msrmnt0, 100), findMax(msrmnt0, 100), findMin(msrmnt0, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt0[i]);
    printf("\n\n");

    printf("1: median %lu high %lu low %lu\n", median_uint64(msrmnt1, 100), findMax(msrmnt1, 100), findMin(msrmnt1, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt1[i]);
    printf("\n\n");

    printf("2: median %lu high %lu low %lu\n", median_uint64(msrmnt2, 100), findMax(msrmnt2, 100), findMin(msrmnt2, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt2[i]);
    printf("\n\n");

    printf("3: median %lu high %lu low %lu\n", median_uint64(msrmnt3, 100), findMax(msrmnt3, 100), findMin(msrmnt3, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt3[i]);
    printf("\n\n");

    printf("4: median %lu high %lu low %lu\n", median_uint64(msrmnt4, 100), findMax(msrmnt4, 100), findMin(msrmnt4, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt4[i]);
    printf("\n\n");

    printf("5: median %lu high %lu low %lu\n", median_uint64(msrmnt5, 100), findMax(msrmnt5, 100), findMin(msrmnt5, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt5[i]);
    printf("\n\n");

    printf("6: median %lu high %lu low %lu\n", median_uint64(msrmnt6, 100), findMax(msrmnt6, 100), findMin(msrmnt6, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt6[i]);
    printf("\n\n");

    printf("7: median %lu high %lu low %lu\n", median_uint64(msrmnt7, 100), findMax(msrmnt7, 100), findMin(msrmnt7, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt7[i]);
    printf("\n\n");

    printf("8: median %lu high %lu low %lu\n", median_uint64(msrmnt8, 100), findMax(msrmnt8, 100), findMin(msrmnt8, 100));
    for(int i=0;i<100;i++) printf("%lu; ", msrmnt8[i]);
    printf("\n\n");
    
    

     



    // msrmnt1[0] = probe_evset_chase(*head1); // miss miss
    // for(int i=0;i<8;i++) flush(my_evset[i]);
    // msrmnt1[1] = probe_evset_chase(*head1); // hit
    // access(target);
    // msrmnt2[0] =probe_chase_loop(my_evset[0], 1);
    // msrmnt2[1] =probe_chase_loop(my_evset[0], 1);
    // msrmnt2[2] =probe_chase_loop(my_evset[0], 1);


    // for(int i=0;i<5;i++) printf("[+] msrmnt1 %2d %3lu\n", i, msrmnt1[i]);
    // for(int i=0;i<5;i++) printf("[+] msrmnt2 %2d %3lu\n", i, msrmnt2[i]);
    close_evsets();
    free(my_evset);
    free(msrmnt1);
    free(msrmnt2);
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
    // test_evset_state();
    replacement_L1();
    return 0;
}
