#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>
#include "evset.h"



// --- utils ---
inline void access(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
}

inline void flush(void *adrs){
	__asm__ volatile("clflush [%0]" ::"r" (adrs));
}

inline u64 probe(void *adrs){
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
inline void wait(u64 cycles) {
	unsigned int ignore;
	u64 start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

// eax lsb, edx msb
inline u64 rdtscpfence(){
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
inline u64 lfsr_create(void) {
  u64 lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  return lfsr;
}

inline u64 lfsr_rand(u64* lfsr) {
    for (u64 i = 0; i < 8*sizeof(u64); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

inline u64 lfsr_step(u64 lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

// --- config ---
inline Config *initConfig(u64 ways,	u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages){
	Config *conf= (Config *) malloc(sizeof(Config));
	updateConfig(conf, ways, cache_line_size, threshold, cache_size, test_reps, hugepages);
	return conf;
}

inline void updateConfig(Config *conf, u64 ways, u64 cache_line_size, u64 threshold, u64 cache_size, u64 test_reps, u64 hugepages){
	conf->ways=ways;
	conf->cache_line_size=cache_line_size;
	conf->threshold=threshold;
	conf->cache_size=cache_size;
	conf->test_reps=test_reps;
	conf->hugepages=hugepages;
}

// --- node ---
// Function to initialize an empty linked list
inline void list_init() { // big fat TODO
    return;  // Return NULL to indicate an empty list
}

// add to beginning
inline void list_push(Node **head, Node *e) {
    if (!e)  return;
    e->prev = NULL;
    e->next = *head;
    if(*head) (*head)->prev = e;
    *head=e;
}

// add to end
inline void list_append(Node **head, Node *e){
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
inline Node *list_pop(Node **head) {
    Node *tmp = (head)? *head : NULL;
    if (!tmp) return NULL;  // rm nothing
    if (tmp->next) tmp->next->prev=NULL;
    *head = tmp->next;
    tmp->next=NULL;
    tmp->prev=NULL;
    return tmp;
}

// TODO
inline Node *list_union(Node* list1, Node* list2) {
    // Node* result = initLinkedList();
    // Node* current;
    // // Add elements from the first list
    // current = list1;
    // while (current != NULL) {
        // result = addElement(result, current->value);
        // current = current->next;
    // }
    // // Add elements from the second list (avoid duplicates)
    // current = list2;
    // while (current != NULL) {
        // if (!containsValue(result, current->value)) result = addElement(result, current->value);
        // current = current->next;
    // }
    // return result;
    return NULL;
}

inline Node *list_get(Node **head, u64 *index) {
    Node *tmp = *head;
    u64 i=0;
    if(!tmp) return NULL;
    while (tmp && i<=*index) {
        tmp=tmp->next;
        i++;
    }
    // *index=i; // DEBUG purposes, toggle, to count list elements, use large index and retrieve value from pointer
    return tmp;
}

// remove when found
inline Node *list_take(Node **head, u64 *index) {
    Node *tmp = *head;
    u64 i=0;
    if(!tmp) return NULL;
    while (tmp && i<=*index) {
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
    return 0;
}
#endif

// inline Node *create_minimal_eviction_set(void **candidate_set, u64 candidate_set_size, Node* evict_set, void *target_adrs, Config *conf){
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

// inline void traverse_list(u64 *addr, u64 size){
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




// inline void create_pointer_chase(void **candidate_set, u64 c_size, Node* set){
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

// inline int64_t pick(Node* candidate_set, u64 base_size, u64 *lfsr) {
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

// inline int64_t test_intern(void *addr, u64 size, void *target_adrs){
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


// inline int64_t test(void **candidate_set, u64 candidate_set_size, Node *test_index_set, void *target_adrs, Config *conf){
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