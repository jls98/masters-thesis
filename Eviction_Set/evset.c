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
} Node;

static Config *conf;
static Node **evsets = NULL;
static Node *pool = NULL;
static u64 pool_size = 0;
static Node *buffer = NULL;
static u64 buffer_size = 0;


// Utils #################################################
/* wait for cycles cycles and activate cache            */
static void wait(u64 cycles);

static void flush(void *adrs);

static void access(void *adrs);

static u64 rdtscpfence();


// Node functions ########################################
// TODO
static void list_init(Node *src, u64 size);
static void list_push(Node **head, Node *e);
static void list_append(Node **head, Node *e);
static Node *list_pop(Node **head);
static Node *list_get(Node **head, u64 *index);
static Node *list_take(Node **head, u64 *index);


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
static void traverse_list(u64 *addr, u64 size);

static Node *init_evset(Config *conf_ptr);

static Node *find_evset(/* TODO */);
static Node *get_evset(Config *conf_ptr);
static void close_evsets();
static int64_t test(void *addr, u64 size, void *target_adrs);


// --- utils ---
static void access(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
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
    for(u64 i=0;i<(size/sizeof(Node));i++){
        src[i].prev = NULL;
        src[i].next = NULL;
        src[i].delta = 0;
    }
}

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
    if(!*head){
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
    // *index=i; // DEBUG purposes, toggle, to count list elements, use large index and retrieve value from pointer
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
    if(tmp->prev) tmp->prev->next=tmp->next;
    else *head=tmp->next;
    tmp->prev=NULL;
    tmp->next=NULL;
    return tmp;
}


// --- algorithms ---
#ifndef NOMAIN
int main(int ac, char **av){
    wait(1E9);
    printf("size of Node %zu\n", sizeof(Node))
    Config con=config_init(8, 4096, 64, 47, 32768, 1, 1);
    printf("main: return init evsets %p\n", init_evset(con));
    return 0;
}
#endif


static Node *init_evset(Config *conf_ptr){
    memcpy(conf, conf_ptr, sizeof(Config));
    buffer_size = conf->ways*conf->sets*20*sizeof(Node);
    if(conf->hugepages){
        buffer = (Node *) mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 0, 0);
    }else{
        buffer = (Node *) mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);        
    }
    
    if(buffer==MAP_FAILED){
        printf("[!] init_evsets: mmap failed\n");
        return NULL;
    }
    list_init(buffer, buffer_size);
    return buffer;
}   

static Node *get_evset(Config *conf_ptr){
    if(!evsets){
        init_evset(conf_ptr);
        find_evset(/*TODO*/);
    }
    return *evsets;
}

static void close_evsets(){
    free(evsets);
    munmap(buffer, buffer_size);
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

// static void traverse_list(u64 *addr, u64 size){
    // u64 c=size;
    // while(c-2){
        // load(addr);
        // load(*addr);
        // load(*(*addr));
        // load(addr);
        // load(*addr);
        // load(*(*addr));
        // c--;
    // }
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

// static int64_t test_intern(void *addr, u64 size, void *target_adrs){
    // if(size==0 || addr ==NULL || target_adrs ==NULL || conf==NULL){
        // return -1;
    // } // TODO rm later // toggle if working
    // load(target_adrs);
    // load(target_adrs);
    // load(target_adrs);
    // load(target_adrs);
    // traverse_list(addr, size);
    
    // // victim + 222 access for page walk, maybe figure out later
    
    // u64 delta, time;
    // time=rdtscpfence();
    // load(target_adrs);
    // delta=rdtscpfence() - time;
    // return delta;
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