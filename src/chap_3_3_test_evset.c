#include <stdint.h>
#include <stdlib.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include "chap_3_3_find_evset.c"

#define REPS 10000
void test_evset(Config *conf, void *target){
    wait(1E9);
    u64 *my_msrmts = malloc(REPS*sizeof(u64));
    Node **evset = NULL;
    evset = find_evset(conf, target); 
    if (evset==NULL){
        printf("[!] Failed to find evset!\n");
        return;
    }

    // multiple measurements
    for (int i=0;i<REPS;i++) my_msrmts[i] =test(conf, *evset, target);
    {
        u64 number=0;
        for (int i=0;i<REPS;i++) number+=my_msrmts[i];        
        printf("below %lu, above %lu\n", REPS-number, number);
    }
    close_evsets(conf, evset);    
    free(my_msrmts);
}

void test_no_evset(Config *conf, void *target){
    Node **no_evset = find_evset(conf, target);
    CU_ASSERT_PTR_NULL(*no_evset);
    close_evsets(conf, no_evset);    
}

void test_find_evset(){
    uint64_t *target = malloc(8);
    *target=0xffffffff;
    for (int i=0; i<5; i++) test_no_evset(config_init(15, 2048, 64, 106,2091752), target);
    printf("evset size 17\n");
    test_evset(config_init(17, 2048, 64, 106,2091752), target);
    printf("evset size 27\n");
    for(int i=0;i<5;i++) test_evset(config_init(27, 2048, 64, 106,2091752), target); 
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

// Function to compute the median of an array of uint64_t values.
static uint64_t median_uint64(uint64_t *array, size_t size) {
    qsort(array, size, sizeof(uint64_t), compare_uint64);

    // If the size is even, return the average of the middle two elements
    if (size % 2 == 0) return (array[size / 2 - 1] + array[size / 2]) / 2;
    else return array[size / 2];
    
}

uint64_t findMin(const uint64_t *array, size_t n) {
    if (n == 0) return 0;
    uint64_t min = array[0];
    for (size_t i = 1; i < n; ++i) if (array[i] < min) min = array[i];
    return min;
}

uint64_t findMax(const uint64_t *array, size_t n) {
    if (n == 0) return 0;    
    uint64_t max = array[0];
    for (size_t i = 1; i < n; ++i) if (array[i] > max) max = array[i];
    return max;
}

#define MSRMNT_CNT 10 // amount of measurements
void measure_element_timing(Config *conf, Node **head1, Node **my_evset, u64 *msrmnt_, u64 index, int evset_size){
    for(int c=0;c<MSRMNT_CNT;c++){        
        // flush evset    
        for(int i=0;i<evset_size;i++) __asm__ volatile("clflush [%0]" ::"r" (my_evset[i]));
        // Access whole set.
        traverse_list0(conf, *head1); // TODO maybe replace with asm implementation
        // Measure access time for element at index.
        msrmnt_[c]=probe_evset_single(my_evset[index]);       
    }
}

// Function to measure the access timings of all set elements for a certain set size. Return the minimum timing of the measurements of the first accessed element.
u64 min_timing_first_element(Config *conf){
    wait(1E9);
    u64 index=0, offset =32768; // arbitrary offset, 32768*64 = 2 MB
    // init buffer, size depending on evset size
    u64 buf_size = ((conf->evset_size/16)+3)*PAGESIZE;     
    Node *tmp=NULL, *buf= (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0), **buf_ptr=&buf;
    // setup and init
    if (madvise(buf, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        munmap(tmp, buf_size);
        return 0;
    }
    Node **my_evset = malloc(conf->evset_size*sizeof(Node *)), **head1 = malloc(sizeof(Node *)); // refs to first element of evset;
    *head1 =NULL, *my_evset=NULL;
    list_init(buf, buf_size);  
    
    // create evsets manually and test them with targets
    index = 120*conf->sets + offset;
    
    // add 1st elem
    index = (conf->evset_size-1)*conf->sets+offset;
    tmp=list_take(buf_ptr, &index);
    list_append(head1, tmp);
    my_evset[conf->evset_size-1] = tmp;

    // add remaining elems, stride equals no of sets
    for(int i=conf->evset_size-2; i >=0; i--){
        index = i*conf->sets + offset;
        tmp=list_take(buf_ptr, &index);
        list_append(head1, tmp);
        my_evset[i]=tmp;
    }

    // list_print(head1);
    u64 *ind=malloc(sizeof(u64));
    *ind=0;
    list_shuffle(head1);

    // init buffer to store measurements
    u64 *msrmnt0=malloc(MSRMNT_CNT*(conf->evset_size)*sizeof(u64));
    // preparation done

    // Measure timings for every element after completing pointer chase.
    for (u64 i=0;i<conf->evset_size;i++) measure_element_timing(conf, head1, my_evset, msrmnt0+MSRMNT_CNT*i, i, conf->evset_size);
    
    // Find first index and return respective minimum measurement
    for(u64 index_tmp=0;index_tmp<conf->evset_size;index_tmp++){
        if(*head1==my_evset[index_tmp]){
            u64 retVal = findMin(msrmnt0+index_tmp*MSRMNT_CNT, MSRMNT_CNT);
            while(*my_evset) list_pop(my_evset);    
            my_evset = NULL;
            free(msrmnt0);
            free(head1);
            munmap(buf, buf_size);
            buf = NULL;            
            return retVal;
        } 
    }  
    return 0; // something went wrong, return 0 anyways.
}

void test_evset_reliability(Config *conf){
    u64 ret =111;    
    while(ret > 110) ret = min_timing_first_element(conf);  
    printf("[!] Measurement below threshold detected: %lu!\n", ret);
}


int main(int ac, char** av) {
	wait(1E9);
    CU_initialize_registry();
    CU_pSuite suite = CU_add_suite("Test Suite evict_baseline", NULL, NULL);
    CU_add_test(suite, "Test test_node", test_find_evset);
    CU_basic_run_tests();
    CU_cleanup_registry();
    
    Config *conf;
    conf = ac==2 ? config_init(atoi(av[1])+1, 4096, 64, 120, 2097152) : config_init(17, 4096, 64, 120, 2097152); // L2, Gen12
    test_evset_reliability(conf);
}