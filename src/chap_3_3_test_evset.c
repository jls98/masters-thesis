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
    Node **no_evset = find_evset(conf, target);
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

static int compare_uint64(const void *a, const void *b) {
    uint64_t ua = *(const uint64_t *)a;
    uint64_t ub = *(const uint64_t *)b;
    return (ua > ub) - (ua < ub);
}

// Function to compute the median of an array of uint64_t values
static uint64_t median_uint64(uint64_t *array, size_t size) {
    // Sort the array
    qsort(array, size, sizeof(uint64_t), compare_uint64);

    // Calculate the median
    if (size % 2 == 0) {
        // If the size is even, return the average of the middle two elements
        return (array[size / 2 - 1] + array[size / 2]) / 2;
    } else {
        // If the size is odd, return the middle element
        return array[size / 2];
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

#define MSRMNT_CNT 10
void intern_access2(Config *conf, Node **head1, Node **my_evset, u64 *msrmnt_, u64 index, int evset_size){
    for(int c=0;c<MSRMNT_CNT;c++){        
        // flush evset    
        for(int i=0;i<evset_size+1;i++) flush(my_evset[i]);

        // access whole evset+1
        traverse_list0(conf, *head1);

        // measure access time for one entry
        msrmnt_[c]=probe_evset_single(my_evset[index]);       
    }
}

u64 min_timing_first_element(int evset_size){
    Config *conf = config_init(evset_size+1, 4096, 64, 120, 2097152); // L1
    wait(1E9);
    u64 index=0, offset =32768; // arbitrary offset, 32768*64 = 2 MB
    // init buffer
    u64 buf_size = ((evset_size/16)+1)*PAGESIZE;     
    Node *tmp=NULL, *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0), **buf_ptr=&buf;
    Node **my_evset = malloc((evset_size+1)*sizeof(Node *)), **head1 = malloc(sizeof(Node *)); // refs to first element of evset;
    *head1 =NULL, *my_evset=NULL;

    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    list_init(buf, buf_size);  
    // create evsets manually and test them with targets
    index = 120*conf->sets + offset;
    
    // add 1st elem
    index = (evset_size-1+1)*conf->sets+offset; // -1
    tmp=list_take(buf_ptr, &index);
    list_append(head1, tmp);

    my_evset[evset_size-1+1] = tmp;
    // add remaining L2 elems
    for(int i=evset_size-2+1; i >=0; i--){ // > EVSET_L1 to add all L1 elems and last L2 elem
        index = i*conf->sets + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    // list_print(head1);
    list_shuffle(head1);

    // init buffer to store measurements
    u64 *msrmnt0=malloc(MSRMNT_CNT*(evset_size+1)*sizeof(u64));
    // preparation done

    // multiple measurements
    for (int i=0;i<evset_size+1;i++){
        intern_access2(conf, head1, my_evset, msrmnt0+MSRMNT_CNT*i, i, evset_size);
    }

    u64 aaaaa=0;
    for(tmp=*head1;aaaaa++<conf->evset_size;tmp=tmp->next){
        for(int bbbb=0;bbbb<evset_size+1;bbbb++){
            if(tmp==my_evset[bbbb]) printf("%2d %p %lu\n", bbbb, tmp, median_uint64(msrmnt0+bbbb*MSRMNT_CNT, MSRMNT_CNT)); // index, pointer, measured median
        }        
    }  
    close_evsets();
    for(int bbbb=0;bbbb<evset_size+1;bbbb++){
        if(*head1==my_evset[bbbb]){
            printf("head %p, bbbb %d, median %lu high %lu low %lu\n", *head1, bbbb, median_uint64(msrmnt0+bbbb*MSRMNT_CNT, MSRMNT_CNT), findMax(msrmnt0+bbbb*MSRMNT_CNT, MSRMNT_CNT), findMin(msrmnt0+bbbb*MSRMNT_CNT, MSRMNT_CNT));
            u64 retVal = findMin(msrmnt0+bbbb*MSRMNT_CNT, MSRMNT_CNT);
            free(my_evset);
            free(msrmnt0);
            free(head1);
            munmap(buf, buf_size);
            return retVal;
        } 
    }  
}

void test_evset_reliability(int evset_size){
    u64 ret =106;
    while(ret > 105){
        ret = min_timing_first_element(evset_size);
    }
    printf("Measurement below threshold detected %lu\n", ret);
}


int main(int ac, char** av) {
	wait(1E9);
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    CU_add_test(suite, "Test test_node", test_find_evset);
    CU_basic_run_tests();
    CU_cleanup_registry();

    test_evset_reliability(atoi(av[1]));
}