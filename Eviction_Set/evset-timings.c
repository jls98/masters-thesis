#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>
#include <string.h>

// #include "evset.h"

// header
#define u64 uint64_t
#define PAGESIZE 2097152
#define NODESIZE 64
#define EVSET_SIZE 24 // hardcoded for alder lake gracemont
#define EVSET_STRIDE 2048 // hardcoded for alder lake gracemont: *2048* x 64 Bytes

typedef struct config {
	u64 evset_size; // cache ways 
    u64 sets;
	u64 cache_line_size; // cache line size (usually 64)
	u64 threshold; // threshold for cache (eg ~45 for L1 on i7)
	u64 cache_size; // cache size in bytes 
	u64 test_reps; // amount of repetitions in test function
	u64 hugepages;
} Config;

/* linked list containing an index and a pointer to     
 * prev and next element                    
 * if treated as (uint64_t *) or similar, it can be used as pointer chase when de-refed
 */
typedef struct node {
    struct node *next;
    struct node *prev;
    size_t delta;
    char pad[40];// offset to cache line size
} Node;

static Config *conf;
static Node **evsets = NULL;
static Node *buffer = NULL;
static u64 buffer_size = 0;
static Node **buffer_ptr;
static u64 lfsr;
static u64 msr_index=0;
static u64 *msrmts;



// Utils #################################################
/* wait for cycles cycles and activate cache            */
static void wait(u64 cycles);
static void flush(void *adrs);
static void my_access(void *adrs);
static uint64_t median_uint64(uint64_t *array, size_t size);
// static void fenced_access(void *adrs);
// static u64 rdtscpfence();


// Node functions ########################################
// TODO
static void list_init(Node *src, u64 size);
static void list_push(Node **head, Node *e);
static void list_append(Node **head, Node *e);
static Node *list_pop(Node **head);
static Node *list_get(Node **head, u64 *index);
static Node *list_take(Node **head, u64 *index);
static void list_shuffle(Node **head);
static void list_print(Node **head);

//Config functions #######################################
/* init Config */
static Config *config_init(u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages);

/* configure Config */
static void config_update(Config *con, u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages);


// PRG ###################################################
/* create random seed.                                  */
static u64 lfsr_create(void);
    
/* get pseudo randomly generated number.                */
static u64 lfsr_rand(u64* lfsr);

/* helper function for lfsr_rand to shift lfsr.         */
static u64 lfsr_step(u64 lfsr);


// algorithms ############################################

static void init_evset(Config *conf_ptr);
static Node **find_evset(Config *conf_ptr, void *target_adrs);
static Node **get_evset();
static void close_evsets();
// static void generate_conflict_set();
// static void traverse_list(Node *ptr);
static u64 test(Node *ptr, void *target);
static u64 test_intern(Node *ptr, void *target);
// static u64 probe_evset(Node *ptr);

// --- utils ---
// Comparison function for qsort
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

static void my_access(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
}

static void flush(void *adrs){
	__asm__ volatile("clflush [%0]" ::"r" (adrs));
}

static u64 probe_chase_loop(void *addr, u64 reps) {
	volatile u64 time;
	
	asm __volatile__ (
        "mov r8, %1;"
        "mov r9, %2;"
        // measure
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
		// BEGIN - probe address

        "loop:"
		"lfence;"
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop;"
		// END - probe address
		"lfence;"
		"rdtscp;"
		"sub rax, rsi;"
		: "=a" (time)
		: "b" (addr), "r" (reps)
		: "rcx", "rsi", "rdx", "r8", "r9"
	);
	return time;
}

static u64 probe_evset_chase(const void *addr) {
	volatile u64 time;
	asm __volatile__ (
        "mov r8, %1;"
        "mov r9, %2;"
        // measure
		"mfence;"
		"rdtscp;"
		"mov rsi, rax;"
		// BEGIN - probe address

        "loop2:"
        "lfence;" // when toggled I cannot see difference on L1 timings
		"mov r8, [r8];"
        "dec r9;"
        "lfence;"
        "jnz loop2;"
		// END - probe address
		"lfence;"
		"rdtscp;"
        // end - high precision
		"sub rax, rsi;"
		: "=a" (time)
		: "b" (addr), "r" (conf->evset_size+1)
		: "rcx", "rsi", "rdx", "r8", "r9"
	);
	return time;
}

// say hi to cache
static void wait(u64 cycles) {
	unsigned int ignore;
	u64 start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

#define FEEDBACK 0x80000000000019E2ULL
static u64 lfsr_create(void) {
  u64 lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  return lfsr;
}

static u64 lfsr_rand(u64* lfsr) {
    for (u64 i = 0; i < 8*sizeof(u64); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

static u64 lfsr_step(u64 lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

// I/O
// read file

// write file



// --- config ---
static Config *config_init(u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages){
	Config *con = malloc(sizeof(Config));
	config_update(con, evset_size, sets, cache_line_size, threshold, cache_size, test_reps, hugepages);
	return con;
}

static void config_update(Config *con, u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages){
	con->evset_size=evset_size;
	// con->evset_size=EVSET_SIZE; // TODO make nicer, hardcoded settings
	con->sets=sets;
	con->cache_line_size=cache_line_size;
	con->threshold=threshold;
	con->cache_size=cache_size;
	con->test_reps=test_reps;
	con->hugepages=hugepages;
}

// --- node ---
static void list_init(Node *src, u64 size) {
    
    srand(time(NULL)); // avoid pre fetchers by having different contents on the mem pages
    
    src[0].prev=NULL;
    src[0].next=NULL;
    src[0].delta=0;
    int *intArray = (int *)src[0].pad;
    for(int i=0;i<10;i++){
        intArray[i]=rand();
    }    
    for(u64 i=1;i<(size/sizeof(Node));i++){
        src[i].prev = &src[i-1];
        src[i].prev->next = &src[i];
        src[i].next=NULL;
        src[i].delta = 0; 
        intArray = (int *)src[i].pad;
        for(int i=0;i<10;i++){
            intArray[i]=rand();
        }
    }
}

// appears to be bugged!!!!
// add to beginning
static void list_push(Node **head, Node *e) {
    if (!e)  return;
    e->prev = NULL;
    e->next = *head;
    if(*head) (*head)->prev = e;
    *head=e;
}

// add to end
static void list_append(Node **head, Node *e){
    if(!e) return;
    if(!head) {
        e->next=NULL;
        e->prev=NULL;
        *head=e;        
        return;
    }
    if(!*head){
        e->next=NULL;
        e->prev=NULL;
        *head=e;        
        return;
    }    
    Node *tmp=*head;
    while(tmp->next){ // iterate to end
        tmp=tmp->next;    
    }
    tmp->next=e;
    e->prev=tmp;
    e->next=NULL;
}

// remove e and return first element of list
static Node *list_pop(Node **head) {
    Node *tmp = (head)? *head : NULL;
    if (!tmp) return NULL;  // rm nothing
    if (tmp->next) tmp->next->prev=NULL;
    *head = tmp->next;
    tmp->next=NULL;
    tmp->prev=NULL;
    return tmp;
}

static Node *list_get(Node **head, u64 *index) {
    Node *tmp = *head;
    u64 i=0;
    if(!tmp) return NULL;
    while (tmp && i<*index) {
        tmp=tmp->next;
        i++;
    }
    *index=i; // DEBUG purposes, toggle, to count list elements, use large index and retrieve value from pointer
    return tmp;
}

// remove when found
static Node *list_take(Node **head, u64 *index) {
    Node *tmp = *head;
    u64 i=0;
    if(!tmp) return NULL;
    while (tmp && i<*index) {
        tmp=tmp->next;
        i++;
    }
    if(!tmp) return NULL;
    if(tmp->next) tmp->next->prev=tmp->prev;
    if(tmp->prev) { // tmp->prev is not head
        tmp->prev->next=tmp->next;         
    }
    else{ // tmp is head -> tmp-> next is new head
        *head=tmp->next;
    }
    tmp->prev=NULL;
    tmp->next=NULL;
    return tmp;
}

static void list_shuffle(Node **head){
    if (!*head) return;
    Node **new_head = malloc(sizeof(Node*));
    *new_head = NULL;
    u64 size=UINT64_MAX;
    list_get(head, &size);
    // size has now the size of linked list 
    u64 index;
    if(!lfsr) lfsr=lfsr_create();    
    while(size>0){
        index = lfsr_rand(&lfsr)%size--;
        list_append(new_head, list_take(head, &index));
    }
    // connect tail to head
    Node *tmp;
    for(tmp=*new_head;tmp->next;tmp=tmp->next){
        my_access(tmp);
    }
    tmp->next=*new_head;
    (*new_head)->prev=tmp;
    *head = *new_head;  
}

static void list_print(Node **head){
    if(head==NULL) return;
    if(*head==NULL) return;
    printf("[+] printing adrs of list:\n");
    Node *tmp;
    u64 ctr=0;
    for(tmp=*head;tmp;tmp=tmp->next){
        printf("[%lu] %p %p\n", ++ctr, tmp, tmp->next);
        if(ctr>=conf->evset_size) break;
    }
}

// --- algorithms ---
#ifndef NOMAIN
int main(){

    return 0;
}
#endif

static void init_evset(Config *conf_ptr){
    wait(1E9);
    conf=conf_ptr;
    buffer_size = 2*PAGESIZE;
    buffer = (Node *) mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (madvise(buffer, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("[!] madvise failed!\n");
        return;
    }
    buffer_ptr=&buffer;
    list_init(buffer, buffer_size);    
    // init buffer for evset elements
    
    evsets = realloc(evsets, conf->evset_size*sizeof(Node *));
    *evsets = NULL;
    
    printf("[+] init_evset complete, evsets %p, buffer %p\n", evsets, buffer);
}    

static Node **find_evset(Config *conf_ptr, void *target_adrs){
    if (msrmts){
        free(msrmts);
        msrmts=NULL;
    }
    msrmts=realloc(msrmts, 1000*sizeof(u64));
    if (!buffer || !evsets){
        printf("find_evset: reset\n");
        close_evsets();
        init_evset(conf_ptr);
    }
    // find out which adrs offset works
    wait(1E9);
    
    // loop over each cache line
    // iterate over every cacheline
    u64 index;
    Node *tmp;
    // for(u64 offset=0;offset<(conf->cache_size/conf->cache_line_size);offset++){
    for(int offset=EVSET_STRIDE-1;offset>=0;offset--){        
        // create evset with offset as index of Node-array
        for(int i= (int) conf->evset_size-1;i>=0;i--){
            index=offset + ((u64)i)*EVSET_STRIDE-i*(EVSET_STRIDE-1-offset); // take elements from buffer from up to down for easier index calculation
            tmp = list_take(buffer_ptr, &index);
            list_append(evsets, tmp);
        }
        list_shuffle(evsets);
        if(msr_index>990) msr_index=0;
        // test if it is applicable, if yes yehaaw if not, proceed and reset evset pointer 
        
        
        if(test(*evsets, target_adrs)) if(test(*evsets, target_adrs)) break;        
        // remove elems from evsets and prepare next iteration
        while(*evsets) list_pop(evsets);       
    }   
    return evsets;
}

static void del_evset(){
    if(!evsets) return;
    while(*evsets) list_pop(evsets);  
}

static Node **get_evset(){
    if(!evsets){
        return NULL;
    }
    return evsets;
}

static void close_evsets(){
    if (evsets) {
        free(evsets);
        evsets=NULL;
    }
    if (buffer) munmap(buffer, buffer_size);
    if (conf){
        free(conf);
        conf=NULL;
    } 
    
}

static void traverse_list0(Node *ptr){
    u64 i=0;
    for(Node *tmp=ptr;i++<conf->evset_size;tmp=tmp->next){
        __asm__ volatile("lfence; ");
        my_access(tmp);
        
    }    
}

static u64 test_intern(Node *ptr, void *target){
    my_access(target);
    // __asm__ volatile ("lfence;");
    traverse_list0(ptr);
    // traverse_list0(ptr);
    
    // page walk
    my_access(target+222);
    // access(target-222);
    
    // measure
    msrmts[msr_index++]=probe_chase_loop(target, 1);
    // printf("%lu ", msrmts[msr_index-1]);
    return msrmts[msr_index-1];
}

static u64 test(Node *ptr, void *target){
    if(ptr ==NULL || target ==NULL){
        return 0;
    }
    return test_intern(ptr, target) > conf->threshold;
}

// static void calibrate(){
//     // TODO check if buffer size is large enough 
//     u64 buf_size = 4*PAGESIZE;
//     // map memory
//     Node *calibration_buffer = (Node *) mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
//     if (madvise(calibration_buffer, PAGESIZE, MADV_HUGEPAGE) == -1){
//         printf("madvise failed!\n");
//         return;
//     }
//     Node **cal_buf_ptr = &calibration_buffer;
//     list_init(calibration_buffer, buf_size);



//     // unmap and delete
// }
