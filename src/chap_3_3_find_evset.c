#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>
#include <string.h>

// header
#define u64 uint64_t
#define PAGESIZE 2097152

typedef struct config {
	u64 evset_size; 
    u64 sets;
	u64 cache_line_size; 
	u64 threshold; // threshold, 95 for L1d, 110 for L2 Alder Lake
	u64 cache_size; // 32768, 49152, 262144, 1310720, 2097152
} Config;

// Node of size 64 bytes (like cache line)
typedef struct node {
    struct node *next;
    char pad[56]; // offset to cache line size
} Node;

// contains measurements
static u64 msr_index=0;
static u64 *msrmts;

static Node *buffer;

// Utils #################################################
static void wait(u64 cycles);
static void my_access(void *adrs);

// lfsr
static u64 lfsr_create(void);
static u64 lfsr_rand(u64* lfsr);
static u64 lfsr_step(u64 lfsr);

// Node functions ########################################
// Initialize linked list.
static void list_init(Node *src, u64 size);

// Append e to end of list head.
static void list_append(Node **head, Node *e);

// Remove and return first element from list head.
static Node *list_pop(Node **head);

// Return element at index index from list head. If index is larger than size of head, it returns the size of list head.
static Node *list_get(Node **head, u64 *index);

// Remove element at index index from list head and return it.
static Node *list_take(Node **head, u64 *index);

// Shuffle elements in list head.
static void list_shuffle(Node **head);

// Print list head.
static void list_print(Node **head);

//Config functions #######################################
static Config *config_init(u64 evset_size, u64 sets, u64 cache_line_size, u64 threshold, u64 cache_size);

// evset ############################################

// Find and return an eviction set for target_adrs.
static Node **find_evset(Config *conf, void *target_adrs);
static void close_evsets(Config *conf, Node **evset);

// Test if list ptr is an eviction set for target. Return 1 if target is evicted.
static u64 test(Config *conf, Node *ptr, void *target);

// Measure timing of target after accessing ptr.
static u64 test_intern(Config *conf, Node *ptr, void *target);
static void traverse_list0(Config *conf, Node *ptr);

// --- utils ---
static void my_access(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
}

// Probe pointer chase starting at addr and access reps many elements. 
// Return execution time for pointer chase. 
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

// Fixed pointer chase probe for eviction sets (access all elements twice).
static u64 probe_evset_chase(Config *conf, void *addr) {
	return probe_chase_loop(addr, conf->evset_size*2);
}

// Probe a single address1
static u64 probe_evset_single(void *addr) {
	return probe_chase_loop(addr, 1);
}

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
	con->evset_size=evset_size;
	con->sets=sets;
	con->cache_line_size=cache_line_size;
	con->threshold=threshold;
	con->cache_size=cache_size;
	return con;
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
    while(tmp->next) tmp=tmp->next;   
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
    if(!tmp) return NULL;
    u64 i=0;
    while (tmp && i<*index) {
        tmp=tmp->next;
        i++;
    }
    *index=i;
    return tmp;
}

// remove element at index and return when found
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
    if (!head || !*head) return;
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
    if (new_head==NULL) return;
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
    u64 buf_size = 4*PAGESIZE;
    Node **evsets = malloc(conf->evset_size*sizeof(Node *));
    *evsets =NULL;
    u64 *new_msrmts = realloc(msrmts, 1000000 * sizeof(u64));
    if(new_msrmts==NULL){
        printf("[!] find_evset realloc failed!\n");
        free(msrmts);
        free(evsets);
        return NULL;
    }
    else msrmts=new_msrmts;
    
    buffer = mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (madvise(buffer, buf_size, MADV_HUGEPAGE) == -1){
        printf("[!] madvise failed!\n");
        free(msrmts);
        free(evsets);
        munmap(buffer, buf_size);
        return NULL;
    }
    Node **buffer_ptr=&buffer;
    list_init(buffer, buf_size);    
    wait(1E9);
    
    // Test every cache set. Iterate over the locations in the "first" stride.
    for(int offset=conf->sets-1;offset>=0;offset--){     

        // Create evset based on current offset/cache set.
        for(u64 i= conf->evset_size-1;i+1>0;i--){
            // Take elements from buffer from high to low for easier index calculation, since we are removing elements from the buffer linked list.
            u64 index=offset + i*conf->sets-i*(conf->sets-1-offset); 
            Node *tmp = list_take(buffer_ptr, &index);
            if(tmp==NULL) {
                printf("[!] list take failed! \n");
                return NULL;
            }
            list_append(evsets, tmp);
        }
        list_shuffle(evsets);
        
        // Test twice to avoid noise.
        if(test(conf, *evsets, target_adrs)) if(test(conf, *evsets, target_adrs)) break;        
        
        // No evset! Remove elements from evset candidate.
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

    // Make sure measurements can be stored properly and we do not run into null pointers.
    if (!msrmts) msrmts = realloc(msrmts, 1000000 * sizeof(u64));
    if (msr_index >= 1000000) msr_index = 0;
    asm __inline__("mfence");
    return test_intern(conf, ptr, target) > conf->threshold;
}