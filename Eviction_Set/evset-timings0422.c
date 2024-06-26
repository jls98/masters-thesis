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

typedef struct config {
	u64 ways; // cache ways 
    u64 sets;
	u64 cache_line_size; // cache line size (usually 64)
	u64 threshold; // threshold for cache (eg ~45 for L1 on i7)
	u64 cache_size; // cache size in bytes 
	u64 test_reps; // amount of repetitions in test function
	u64 hugepages;
} Config;

/* linked list containing an index and a pointer to     
 * prev and next element                                */
typedef struct node {
    struct node *next;
    struct node *prev;
    size_t delta;
    char pad[40];// offset to cache line size
} Node;

static Config *conf;
static Node **evsets = NULL;
static Node *pool = NULL;
static u64 pool_size = 0;
static Node *buffer = NULL;
static u64 buffer_size = 0;
static Node **buffer_ptr;
static u64 lfsr;
static u64 msr_index=0;
static u64 msrmts[1000];


// Utils #################################################
/* wait for cycles cycles and activate cache            */
static void wait(u64 cycles);

static void flush(void *adrs);

static void access(void *adrs);
static void fenced_access(void *adrs);
static u64 rdtscpfence();


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
static Config *config_init(u64 ways, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages);

/* configure Config */
static void config_update(Config *con, u64 ways, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages);


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
static Node **get_evset(Config *conf_ptr);
static void close_evsets();
static void generate_conflict_set();
static void traverse_list(Node *ptr);
static u64 test(Node *ptr, void *target);
static u64 test_intern(Node *ptr, void *target);
static u64 probe_evset(Node *ptr);
static void timings();

// --- utils ---
// Comparison function for qsort
int compare_uint64(const void *a, const void *b) {
    uint64_t ua = *(const uint64_t *)a;
    uint64_t ub = *(const uint64_t *)b;
    return (ua > ub) - (ua < ub);
}

// Function to compute the median of an array of uint64_t values
uint64_t median_uint64(uint64_t *array, size_t size) {
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

static void access(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
}
static void fenced_access(void *adrs){
	__asm__ volatile(
    "lfence; "
    "mov rax, [%0];"::"r" (adrs): "rax", "memory");
}

static void flush(void *adrs){
	__asm__ volatile("clflush [%0]" ::"r" (adrs));
}
 
static u64 probe(void *adrs){
	volatile u64 time;  
	__asm__ volatile (
        " mfence            \n"
        " rdtscp             \n"
        " mov r8, rax 		\n"
        " mov rax, [%1]		\n"
        " lfence            \n"
        " rdtscp             \n"
        " sub rax, r8 		\n"
        : "=&a" (time)
        : "r" (adrs)
        : "ecx", "rdx", "r8", "memory"
	);
	return time;
}

// say hi to cache
static void wait(u64 cycles) {
	unsigned int ignore;
	u64 start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

// eax lsb, edx msb
static u64 rdtscpfence(){
    unsigned a, d;
    __asm__ volatile(
    "lfence;"
    "rdtscp\n"
    "lfence;"
	: "=a" (a), "=d" (d)
	:: "rcx");
	return ((u64)d << 32) | a;
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
static Config *config_init(u64 ways, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages){
	Config *con = malloc(sizeof(Config));
	config_update(con, ways, sets, cache_line_size, threshold, cache_size, test_reps, hugepages);
	return con;
}

static void config_update(Config *con, u64 ways, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages){
	con->ways=ways;
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
        for(int i=0;i<i;i++){
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
    // printf("append\n");
    if(!e) return;
    if(!head) {
        e->next=NULL;
        e->prev=NULL;
        // printf("oh\n");
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
    // printf("list_append %p %p\n", tmp, tmp->next);
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
    *head = *new_head;  
}

static void list_print(Node **head){
    if(head==NULL) return;
    if(*head==NULL) return;
    printf("[+] printing adrs of list:\n");
    Node *tmp;
    int ctr=0;
    for(tmp=*head;tmp;tmp=tmp->next){
        printf("[%i] %p %p\n", ++ctr, tmp, tmp->next);
    }
}

// --- algorithms ---
#ifndef NOMAIN
int main(int ac, char **av){
    // wait(1E9);
    // printf("starting...\n");
    // // Config *con=config_init(8, 4096, 64, 47, 32768, 1, 1);
    // Config *con=config_init(16, 131072, 64, 70, 2097152, 1, 1);
    // u64 *target=malloc(sizeof(u64));
    // init_evset(con);
    // printf("init done\n");
    // Node **head=find_evset(con, target);
    // if(head){
        // printf("find done %p", head);
        // if(*head) printf(" %p %p %p", *head, (*head)->prev, (*head)->next);
    // }
    // printf("\n");
    // printf("taget %p \n", target);
    // list_print(head);
    // printf("m: %lu\n", test_intern(*head, (void *)target));
    
    timings();
    return 0;
}
#endif

#define PAGESIZE 2097152
#define NODESIZE 32

static void init_evset(Config *conf_ptr){
    wait(1E9);
    conf=conf_ptr;
    buffer = (Node *) mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (madvise(buffer, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("madvise failed!\n");
        return;
    }
    buffer_ptr=&buffer;
    list_init(buffer, PAGESIZE);    
    // init buffer for evset elements
    
    evsets = realloc(evsets, conf->ways*sizeof(Node *));
    
    printf("init_evset finished, evsets %p, buffer %p\n", evsets, buffer);
}    

static Node **find_evset(Config *conf_ptr, void *target_adrs){
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
    list_print(evsets);
    for(u64 offset=0;offset<(conf->cache_size/conf->cache_line_size);offset++){
        // create evset with offset as index of Node-array
        // printf("offset %lu\n", offset);
        for(u64 i=0;i<conf->ways;i++){
            index=offset/*(conf->cache_line_size/NODESIZE)*/ + i*(conf->sets/NODESIZE) -i-offset*i; //(compute size in NODE index)
            tmp = list_take(buffer_ptr, &index);
            // printf("offset %lu index %lu i*(conf->sets/NODESIZE) %lu:\n", offset, index, i*(conf->sets/NODESIZE));

            // printf("%p ", tmp);
            list_append(evsets, tmp);
        }
        // printf("\n");
        list_shuffle(evsets);
        if(msr_index>990) msr_index=0;
        // test if it is applicable, if yes yehaaw if not, proceed and reset evset pointer 
        
        if(test(*evsets, target_adrs)){
            return evsets;
        } 
        if(test(*evsets, target_adrs)) if(test(*evsets, target_adrs)) break;
        if(test(*evsets, target_adrs)) if(test(*evsets, target_adrs)) break;
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

static Node **get_evset(Config *conf_ptr){
    if(!evsets){
        return NULL;
    }
    return evsets;
}

static void close_evsets(){
    if (evsets) free(evsets);
    if (buffer) munmap(buffer, buffer_size);
}

static void generate_conflict_set(Config *conf_ptr, char *target){
    if(!evsets) return;
    if(!pool){
        printf("generate_conflict_set: pool is uninitialized! Trying to initialize...\n");
        init_evset(conf_ptr);
    }
    if(!pool){
        printf("generate_conflict_set: pool still NULL, requirements not met!\n");   
        return;        
    }
    
    u64 pool_elems = pool_size/sizeof(Node);
    u64 lfsr = lfsr_create();
    
    // evsets <- {} // otherweise returned
    // WIP
}

static void traverse_list(Node *ptr){
    access((void *) ptr);
    access((void *) ptr);
    access((void *) ptr->next);
    while(ptr && ptr->next && ptr->next->next){
        access((void *) ptr);
        access((void *) ptr->next);
        access((void *) ptr->next->next);
        access((void *) ptr);
        access((void *) ptr->next);
        access((void *) ptr->next->next);
        ptr=ptr->next;
    }
    access((void *) ptr);
    access((void *) ptr->next);   
    access((void *) ptr);
}

static void traverse_list0(Node *ptr){
    for(Node *tmp=ptr;tmp;tmp=tmp->next){
        access((void *) tmp);
    }    
}

static void traverse_list_fenced(Node *ptr){
    for(Node *tmp=ptr;tmp;tmp=tmp->next){
        fenced_access((void *) tmp);
    }    
}


static u64 test_intern(Node *ptr, void *target){
    access(target);
    __asm__ volatile ("lfence;");
    traverse_list0(ptr);
    traverse_list0(ptr);
    
    // page walk
    access(target+222);
    // access(target-222);
    
    // measure
    msrmts[msr_index++]=probe(target);
    // printf("%lu ", msrmts[msr_index-1]);
    return msrmts[msr_index-1];
}



static u64 test(Node *ptr, void *target){
    if(ptr ==NULL || target ==NULL){
        return 0;
    }
    return test_intern(ptr, target) > conf->threshold;
}

static u64 probe_evset(Node *ptr){
    if(!ptr) return 0;
    u64 start, delta;
    
    start = rdtscpfence();
    traverse_list_fenced(ptr);
    delta = rdtscpfence() - start;
    return delta;
    
}

#define TOTALACCESSES 100

static u64 static_accesses(Node **buffer, u64 total_size, u64 reps){
    Node *tmp=*buffer;
    Node *next;
    u64 total_time=0;
    u64 *msrmts = malloc(reps*sizeof(u64));
    for(int i=1;i*64<total_size;i++){
        access(tmp);
        tmp=tmp->next;
    }
    next=tmp->next;
    tmp->next=*buffer;
    tmp=*buffer;
    
    for(int i=0;i*64<total_size;i++){
        access(tmp);
        tmp=tmp->next;
    }
    tmp=*buffer;
    
    for(int i=0;i<reps;i++){
        msrmts[i]=probe(tmp);
        tmp=tmp->next;         
    }
    
    tmp=*buffer;
    for(int i=1;i*64<total_size;i++){
        tmp=tmp->next;
    }    
    tmp->next=next;
    
    // printf("msrmts\n");
    for(int i=0;i<reps;i++){
        total_time+=msrmts[i];
        // printf("%lu; ", msrmts[i]);
    }    
    return total_time;
}

static u64 static_accesses_random(Node **buffer, u64 total_size, u64 reps){
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
    printf("pre shuffle\n");
    list_shuffle(head);
    printf("post shuffle\n");
    tmp=*head;
    
    for(int i=1;i*64<total_size;i++){
        access(tmp);
        tmp=tmp->next;
    }
    tmp->next=*head;
    tmp=*head;
    for(int i=1;i*64<total_size;i++){
        access(tmp);
        access(tmp->next);
        access(tmp);
        access(tmp->next);
        tmp=tmp->next;
    }
    tmp=*head;
    
    for(int i=0;i<reps;i++){
        msrmts[i]=probe(tmp);
        tmp=tmp->next;         
    }
    
    
    tmp=*head;
    for(int i=1;i*64<total_size;i++){
        tmp=tmp->next;
    }    
    tmp->next=next;
    if (next) next->prev=tmp;
    *buffer=*head;
    printf("[!] msrmts\n");
    for(int i=0;i<reps;i++){
        total_time+=msrmts[i];
        printf("%lu; ", msrmts[i]);
    }    
    printf("\n[+] Results for buffer size %lu: total time %lu, avg %lu, median %lu\n", total_size, total_time, total_time/reps, median_uint64(msrmts, reps));    
    free(msrmts);
    return total_time;
}

static void timings(){
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
    u64 total_size;
    Node **buffer=&buf;

    // total_size = 16777216;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);

    // printf("total time %lu, avg %lu\n", total_time, total_time/TOTALACCESSES);
    
    // total_size = 8388608;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);

    // printf("total time %lu, avg %lu\n", total_time, total_time/TOTALACCESSES);
    
    
    
    // total_size = 4194304;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);

    
    
    
    // total_size = 2097152;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);

    
    // total_size = 1048576;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);



    // total_size = 524288;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);



    // total_size = 262144;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);



    // total_size = 131072;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);


    // total_size = 65536;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);

    // total_size = 32768;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);

    // total_size = 16384;
    // printf("\ntotal size %lu\n", total_size);
    // total_time=static_accesses(buffer, total_size, TOTALACCESSES);


    
    total_size = 16384;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 24384;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 26384;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 28384;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 30384;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES); 
    
    total_size = 32768;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 34768;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 36768;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 38768;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 65536;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 131072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 161072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 191072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 221072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 241072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 262144;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 281072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 301072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    total_size = 321072;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 524288;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 1048576;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 2097152;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);

    total_size = 4194304;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 8388608;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);
    
    total_size = 16777216;
    total_time=static_accesses_random(buffer, total_size, TOTALACCESSES);


    
    
}

// static Node *create_minimal_eviction_set(void **candidate_set, u64 candidate_set_size, Node* evict_set, void *target_adrs, Config *conf){
    // if (conf==NULL){
		// printf("create_minimal_eviction_set: Conf is NULL!\n");
		// return NULL;
	// }
	
	// // if candidate set is empty, no eviction set can be created
	// if (candidate_set==NULL){
		// printf("create_minimal_eviction_set: candidate_set is NULL!\n");
		// return NULL;
	// }
	
	// if (candidate_set_size==0){
		// printf("create_minimal_eviction_set: candidate_set_size is 0!\n");
		// return NULL;
	// }
	
	// clock_t track_start = clock();
    // // init lfsr, variable c stores currently picked candidate integer/index value
    // u64 lfsr = lfsr_create(), c, cnt_e=0; 
	// int64_t c_tmp;
	
	// // create current candidate set containing the indexes of unchecked candidates and initialize with all indexes
    // Node* cand_ind_set = initLinkedList();
    // for (u64 i=0; i<candidate_set_size-1;i+=8) cand_ind_set = addElement(cand_ind_set, i); 
    
    // // while |R| < a and cind still contains possible and unchecked candidates
    // while(cand_ind_set!=NULL /*&& test(candidate_set, candidate_set_size, evict_set, target_adrs, conf) !=1*/){        
        // // c <- pick(S) pick candidate index c from candidate set S/cand_ind_set
		// do{
			// c_tmp=pick(cand_ind_set, candidate_set_size, &lfsr);
		// }
        // while(c_tmp==-1 || containsValue(evict_set, (u64) c_tmp) || !containsValue(cand_ind_set, (u64) c_tmp)); // prevent picking duplicate candidates and continuing on error
				
		// c = (u64) c_tmp;
		// // remove c from S S <- S\{c}
		// cand_ind_set = deleteElement(cand_ind_set, c);         

        // // R union S\{c}
        // Node *combined_set = unionLists(cand_ind_set, evict_set);

        // // if not TEST(R union S\{c}), x)  
		// // if removing c results in not evicting x anymore, add c to current eviction set    	
		// if(test(candidate_set, candidate_set_size, combined_set, target_adrs, conf)==0 && test(candidate_set, candidate_set_size, evict_set, target_adrs, conf)==0){
            // evict_set = addElement(evict_set, c);
            // cnt_e++; // added elem to evict set -> if enough, evict_set complete   
        // }
    // }
    // printf("cind set contains still %i elements\n", count(cand_ind_set));
    // if (cand_ind_set==NULL && cnt_e < conf->ways) printf("create_minimal_eviction_set: not successful, eviction set contains less elements than cache ways!\n");
    
    // /* baseline algorithm */
	// printList(evict_set);
	
	// // measure time needed for this algorithm
	// double  cpu_time_used = ((double) (clock() - track_start)) / CLOCKS_PER_SEC;
    // // Print the measured time
    // printf("Time taken by myFunction: %.6f seconds\n", cpu_time_used);
	
	// printf("test of evict set %li\n", test(candidate_set, candidate_set_size, evict_set, target_adrs, conf));
	// return evict_set;
// }






// static void create_pointer_chase(void **candidate_set, u64 c_size, Node* set){
// #ifdef TEST_EVICT_BASELINE
	// clock_t start = clock();
// #endif
	// if (set == NULL) {
		// printf("create_pointer_chase: set is NULL!\n");
		// return; // no pointer chase 
	// }

    // // create pointer chase between set elements
    // Node* cur_no;  // current index (from set)
    // for (cur_no=set;cur_no->next !=NULL; cur_no=cur_no->next){
        // if (cur_no->next->value >= c_size){
            // printf("create_pointer_chase: current index from set greater than size! Element not contained in candidate_set!\n");
            // return;
        // } 
        // // set pointer to next element cur_no holds current index, cur_no->next holds next index, &addr[cur_no->next->value] is ptr to respective elem
        // candidate_set[cur_no->value] = &candidate_set[cur_no->next->value];    
    // }
    // candidate_set[cur_no->value] = &candidate_set[set->value]; // set pointer from last element to first element

// #ifdef TESTCASE
	// clock_t end = clock();
	// double  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	// printf("create_pointer_chase: took %.6f seconds to finish\n", cpu_time_used);
// #endif	

// }

// static int64_t pick(Node* candidate_set, u64 base_size, u64 *lfsr) {
    // // uninitialized parameters
    // if (lfsr==NULL){
		// printf("pick: lfsr is NULL!\n");
		// return -1;
	// } 
	// if (candidate_set==NULL){
		// printf("pick: candidate_set is NULL!\n");
		// return -1;
	// } 
	// if (base_size==0){
		// printf("pick: base_size is 0!\n");
		// return -1;
	// } 
	
    // u64 c=0, j, c_size; // c picked candidate, j index in candidate_set, c_size current candidate set size

    // // pick a random number, compute modulo amount of elements left in candidate set and take the value from the node element at the resulting position
    // c_size = (u64) count(candidate_set);
    // Node *cur_node = candidate_set;
    // j = lfsr_rand(lfsr) % c_size;
    // do{
        // if (c==j) break;
        // c++;
        // cur_node = cur_node->next;
    // }
    // while(cur_node->next != NULL);

    // return (int64_t) cur_node->value;
// }




// static int64_t test(void **candidate_set, u64 candidate_set_size, Node *test_index_set, void *target_adrs, Config *conf){
	// // // empty candidate set, no array to create pointer chase on    
    // // if (candidate_set==NULL){
		// // printf("test: candidate_set is NULL!\n");
		// // return -1;
	// // } 
	// // if (candidate_set_size==0){
		// // printf("test: candidate_set_size is 0!\n");
		// // return -1;
	// // } 
	// // if (test_index_set==NULL){
		// // return 0;
	// // } // toggle/use if debugging
		
	// // prepare pointer chase between elements from candidate_set indexed by test_index_set 
	// create_pointer_chase(candidate_set, candidate_set_size, test_index_set);
	
	// // test 
	// int64_t ret = test_intern(candidate_set[test_index_set->value], count(test_index_set), target_adrs);
	
	
	// return ret;
// }