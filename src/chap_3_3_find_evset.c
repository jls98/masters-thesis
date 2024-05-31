#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>
#include <string.h>

// #include "evset.h"

// TODOOOOO

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
} Config;

/* linked list containing an index and a pointer to     
 * prev and next element                    
 * if treated as (uint64_t *) or similar, it can be used as pointer chase when de-refed
 */
typedef struct node {
    struct node *next;
    char pad[56];// offset to cache line size
} Node;

static u64 msr_index=0;
static u64 *msrmts;
static Node *buffer;

// Utils #################################################
/* wait for cycles cycles and activate cache            */
static void wait(u64 cycles);
static void flush(void *adrs);
static void my_access(void *adrs);

// Node functions ########################################
// TODO
static void list_init(Node *src, u64 size);
// static void list_push(Node **head, Node *e);
static void list_append(Node **head, Node *e);
static Node *list_pop(Node **head);
static Node *list_get(Node **head, u64 *index);
static Node *list_take(Node **head, u64 *index);
static void list_shuffle(Node **head);
static void list_print(Node **head);

//Config functions #######################################
/* init Config */
static Config *config_init(u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size);

/* configure Config */
static void config_update(Config *con, u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size);


// PRG ###################################################
/* create random seed.                                  */
static u64 lfsr_create(void);
    
/* get pseudo randomly generated number.                */
static u64 lfsr_rand(u64* lfsr);

/* helper function for lfsr_rand to shift lfsr.         */
static u64 lfsr_step(u64 lfsr);

// algorithms ############################################

static Node **find_evset(Config *conf, void *target_adrs);
static void close_evsets(Config *conf, Node **evset);
static u64 test(Config *conf, Node *ptr, void *target);
static u64 test_intern(Config *conf, Node *ptr, void *target);
static void traverse_list0(Config *conf, Node *ptr);
// --- utils ---

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
        "lfence;"
        "loop:"		
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

static u64 probe_evset_chase(Config *conf, void *addr) {
	return probe_chase_loop(addr, conf->evset_size*2);
}

static u64 probe_evset_single(void *addr) {
	return probe_chase_loop(addr, 1);
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

// --- config ---
static Config *config_init(u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size){
	Config *con = malloc(sizeof(Config));
	config_update(con, evset_size, sets, cache_line_size, threshold, cache_size);
	return con;
}

static void config_update(Config *con, u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size){
	con->evset_size=evset_size;
	con->sets=sets;
	con->cache_line_size=cache_line_size;
	con->threshold=threshold;
	con->cache_size=cache_size;
}

// --- node ---
static void list_init(Node *src, u64 size) { 
    srand(time(NULL)); // avoid pre fetchers by having different contents on the mem pages
    src[0].next=NULL;
    int *intArray = (int *)src[0].pad;
    for(int i=0;i<14;i++){
        intArray[i]=rand();
    }    
    for(u64 i=1;i<(size/sizeof(Node));i++){
        src[i-1].next = &src[i];
        src[i].next=NULL;
        intArray = (int *)src[i].pad;
        for(int i=0;i<14;i++){
            intArray[i]=rand();
        }
    }
}

// add to end
static void list_append(Node **head, Node *e){
    if(!e) return;
    if(!head || !*head) {
        e->next=NULL;
        *head=e;        
        return;
    }
    Node *tmp=*head;
    while(tmp->next) tmp=tmp->next;    // iterate to end
         
    tmp->next=e;
    e->next=NULL;
}

// remove and return first element of list
static Node *list_pop(Node **head) {
    Node *tmp = (head)? *head : NULL;
    if (!tmp) return NULL;  // rm nothing
    *head = tmp->next;
    tmp->next=NULL;
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
    Node *tmp = *head, *prev=NULL;
    if(!tmp) return NULL;
    for(u64 i=0;tmp && i<*index; i++){
        prev=tmp;
        tmp=tmp->next; 
    } 
    if(!tmp) return NULL;
    if(prev) prev->next=tmp->next;  // tmp->prev is not head    
    else *head=tmp->next; // tmp is head -> tmp-> next is new head
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
    u64 lfsr=lfsr_create();    
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
    *head = *new_head;  
}

static void list_print(Node **head){
    if(head==NULL || *head==NULL) return;
    printf("[+] printing pointers of list elements:\n");
    {
        u64 ctr=0;
        for(Node *tmp=*head;tmp;tmp=tmp->next){
            printf("[%lu] %p %p\n", ++ctr, tmp, tmp->next);
            if(ctr > 1 && tmp==*head) break;
        }
    }
}

// --- algorithms ---
#ifdef EVSETMAIN
int main(){
    Config *conf=config_init(27, 2048, 64, 106, 2097152);
    void *target =malloc(8);
    Node **evset = find_evset(conf, target);
    if (evset == NULL) return 0;
    list_print(evset);
    flush(evset);
    probe_evset_chase(conf, *evset);
    close_evsets(conf, evset);
    return 0;
}
#endif

static void close_evsets(Config *conf, Node **evset){
    if(evset) {
        while(*evset) list_pop(evset);
        evset=NULL;
    }
    munmap(buffer, (PAGESIZE < conf->cache_size) ? 2*conf->cache_size : 2*PAGESIZE);
    buffer = NULL;
}

static Node **find_evset(Config *conf, void *target_adrs){
    u64 buf_size = PAGESIZE < conf->cache_size ? 2*conf->cache_size : 2*PAGESIZE;
    Node **evsets = malloc(conf->evset_size*sizeof(Node *));
    msrmts = realloc(msrmts, 1000000 * sizeof(u64));
    buffer = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (madvise(buffer, PAGESIZE, MADV_HUGEPAGE) == -1){
        printf("[!] madvise failed!\n");
        return NULL;
    }
    Node **buffer_ptr=&buffer;
    list_init(buffer, buf_size);    
    wait(1E9);
    
    // loop over each cache line
    // iterate over every cacheline

    // for(u64 offset=0;offset<(conf->cache_size/conf->cache_line_size);offset++){
    for(int offset=EVSET_STRIDE-1;offset>=0;offset--){      
        // create evset with offset as index of Node-array
        for(u64 i= conf->evset_size-1;i+1>0;i--){
            u64 index=offset + i*EVSET_STRIDE-i*(EVSET_STRIDE-1-offset); // take elements from buffer from up to down for easier index calculation
            
            Node *tmp = list_take(buffer_ptr, &index);
            list_append(evsets, tmp);
        }
        list_shuffle(evsets);
        // test if it is applicable, if yes yehaaw if not, proceed and reset evset pointer 
        
        if(test(conf, *evsets, target_adrs)) if(test(conf, *evsets, target_adrs)) break;        
        // remove elems from evsets and prepare next iteration
        while(*evsets) list_pop(evsets);       
    }   
    return evsets;
}

static void traverse_list0(Config *conf, Node *ptr){
    {
        u64 i=0;
        for(Node *tmp=ptr;i++<conf->evset_size;tmp=tmp->next){
            __asm__ volatile("lfence; ");
            my_access(tmp);        
        }
    }    
}

static u64 test_intern(Config *conf, Node *ptr, void *target){
    my_access(target);
    traverse_list0(conf, ptr);
    
    // page walk
    my_access(target+222);
    my_access(target-222);
    
    // measure
    msrmts[msr_index]=probe_evset_single(target);
    return msrmts[msr_index++];
}

static u64 test(Config *conf, Node *ptr, void *target){
    if(ptr ==NULL || target ==NULL) return 0; 
    if (!msrmts) msrmts = realloc(msrmts, 1000000 * sizeof(u64));
    if (msr_index >= 1000000) msr_index = 0;
    asm __inline__("mfence");
    return test_intern(conf, ptr, target) > conf->threshold;
}