#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>

/* #################################################### */
/* ####################### utils ###################### */
/* #################################################### */

static uint64_t time_buf;

/* wait for cycles cycles and activate cache            */
static void wait(uint64_t cycles);


static struct Config {
	uint64_t ways; // cache ways 
	uint64_t cache_line_size; // cache line size (usually 64)
	uint64_t threshold; // threshold for cache (eg ~45 for L1 on i7)
	uint64_t cache_size; // cache size in bytes 
	uint64_t test_reps; // amount of repetitions in test function
} Config;

/* linked list containing an index and a pointer to     */
/* the next element                                     */
static struct Node {
    uint64_t value;
    struct Node* next;
} Node;

/* Function to initialize an empty linked list          */
static struct Node* initLinkedList();

/* Function to add a new element to the linked list     */
static struct Node* addElement(struct Node* head, uint64_t value);

/* Function to delete an element with a specific value  */
/* from the linked list                                 */
static struct Node* deleteElement(struct Node* head, uint64_t value);

/* Function to clone a list                             */
static struct Node* cloneList(struct Node* original);

/* Function to union two lists                          */
static struct Node* unionLists(struct Node* list1, struct Node* list2);

/*  helper func for unionLists                          */
static int containsValue(struct Node* head, uint64_t value);

/* helper function to count elements in Node linked list */
static int count(struct Node *head);

/* Function to print the elements of the linked list    */
static void printList(struct Node* head);

/* Function to free the memory allocated for the linked */
/* list                                                 */
static void freeList(struct Node* head);

/* #################################################### */
/* ############## pseudo random generator ############# */
/* #################################################### */

/* create random seed.                                  */
static uint64_t lfsr_create(void);
    
/* get pseudo randomly generated number.                */
static uint64_t lfsr_rand(uint64_t* lfsr);

/* helper function for lfsr_rand to shift lfsr.         */
static uint64_t lfsr_step(uint64_t lfsr);


/* #################################################### */
/* #################### algorithms #################### */
/* #################################################### */

// candidate_set pointer to the first location of the candidate_set addresses
// candidate_set_size size of the candidate_set 
// test_index_set set containing all indexes from a set under test, e.g. the eviction set
// target_adrs the address to be evicted 
// threshold the threshold in cycles required to determine if something is cached. Depends on machine and cache level.
// returns 1 if target_adrs is being evicted (1 on miss) and measurement takes longer than threshold time, 0 if time measurement is lower than threshold
// returns -1 if there is an error
static int64_t test(void **candidate_set, uint64_t candidate_set_size, struct Node *test_index_set, void *target_adrs, struct Config *conf);

/* test1: eviction test for the specific address target_adrs.  */
/* Loads target_adrs and then all elements from eviction set   */
/* test. Finally, loads target_adrs and measures time.         */
/* addr: pointer to first element from eviction set     */
/* target_adrs: target adrs.                                */
/* returns true/1 if measurement is above a threshold.    */
static int64_t test1(void *addr, uint64_t size, void* target_adrs, struct Config *conf);

/* pointer chase: creates pointer chase in subset of    */
/* by candidate_set mapped set with c_size elements.    */
/* set contains set of indexes for pointer chase        */
static void create_pointer_chase(void **candidate_set, uint64_t c_size, struct Node* set);

/* Pick lfsr pseudo-randomized next candidate (index).   
 * The new candidate should not be part of eviction set     
 * evict_set, be part of candidate set and picked with lfsr  
 * state in possible range.                              
 * returns candidate index, if none found base_size+1   */
static int64_t pick(struct Node* candidate_set, uint64_t base_size, uint64_t *lfsr);

/* create minimal eviction set from candidate set for   */
/* target_adrs in evict_set                             */
static struct Node *create_minimal_eviction_set(void **candidate_set, uint64_t candidate_set_size, struct Node* evict_set, void *target_adrs, struct Config *conf);

/* init Config */
static struct Config *initConfig(uint64_t ways,	uint64_t cache_line_size, uint64_t threshold, uint64_t cache_size, uint64_t test_reps);

/* configure Config */
static void updateConfig(struct Config *conf, uint64_t ways, uint64_t cache_line_size, uint64_t threshold, uint64_t cache_size, uint64_t test_reps);

/* #################################################### */
/* ################## implementation ################## */
/* #################################################### */

static void load(void *adrs){
	__asm__ volatile("mov rax, [%0];"::"r" (adrs): "rax", "memory");
}

static void flush(void *adrs){
	__asm__ volatile("clflush [%0];lfence" ::"r" (adrs));
}

static uint64_t probe(void *adrs){
	volatile uint64_t time;  
	__asm__ volatile (
        " mfence            \n"
        " lfence            \n"
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

#define target_index 51200
#ifdef EVICT_BASELINE
int main(int ac, char **av){
    /* preparation */
    
	struct Config *conf = initConfig(8, 64, 53, 32768, 1000); 	// L1 i7
	// struct Config *conf = initConfig(8, 64, 58, 262144, 1000); 	// L2 i7
    
	// struct Config *conf = initConfig(8, 64, 75, 32768, 1000); 	// L1 i12 // remember taskset -c 8!!
    
    
    wait(1E9); // boost cache 
	uint64_t c_size = conf->cache_size/2; // uint64_t = 4 Bytes -> 16384 indexes address 65536 Bytes
    // R <- {}
    // allocate space for eviction set
    struct Node* evict_set = initLinkedList();

    // map candidate_set (using hugepages, twice the size of cache in bytes (4 times to have space for target))
    void **candidate_set = mmap(NULL, 10* c_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	
    candidate_set[target_index] = &candidate_set[target_index];
    candidate_set[target_index-1] = &candidate_set[target_index-1];
    candidate_set[target_index+1] = &candidate_set[target_index+1];
    void *target_adrs = &candidate_set[target_index]; // take target somewhere in the middle of allocated memory
    
	printf("main: c[] %p and &c[] %p \n", candidate_set[target_index], &candidate_set[target_index]);
	// create handmade eviction set: M memory addresses = 32768/8=4096, S sets = 64 -> stride of 64 in indexes
	for(int i=512;i<4137;i+=512){ // 512, 1024, 1536, 2048, 2560, 3112, 3624, 4136  index +1 == 8 bytes
		evict_set = addElement(evict_set, i); 
	}
	
	printf("main: test %li\n", test(candidate_set, c_size, evict_set, target_adrs, conf)); // works
	
	for(struct Node *it = evict_set;it!=NULL;it=it->next) printf("-%p, %p, %lu\n", candidate_set[it->value], &candidate_set[it->value], it->value);
	
	freeList(evict_set);
	evict_set = initLinkedList();
	
	// --------------------------------
	// create evict set
    struct Node *tmp_evict_set = create_minimal_eviction_set(candidate_set, c_size, evict_set, target_adrs, conf);
	
    printf("main: Eviction set for target at %p %i\n", target_adrs, tmp_evict_set==NULL);

	printf("main: Indexes in minimal eviction set:\n"); // print indexes of eviction set
	for(struct Node *it = tmp_evict_set;it!=NULL;it=it->next) printf("-%p, %p, %lu\n", candidate_set[it->value], &candidate_set[it->value], it->value);
	
	printf("\nmain: Testing target adrs %p now: \n", target_adrs);

	// measure time when cached	
	load(target_adrs);
	uint64_t time = probe(target_adrs);
	
	printf("main: Time loading victim cached %lu\n", time);
	// measure time when uncached	
	flush(target_adrs);
	time = probe(target_adrs);

	printf("main: Time loading victim uncached %lu\n", time);
    
    flush(&candidate_set[target_index+8]);
    flush(&candidate_set[target_index+7]);
    flush(&candidate_set[target_index]);
	printf("main: count %i\n", count(tmp_evict_set));
	printf("main: test %i %li %p\n", target_index, test(candidate_set, c_size, tmp_evict_set, &candidate_set[target_index], conf), &candidate_set[target_index]);
	printf("main: test %i %li %p\n", target_index+7, test(candidate_set, c_size, tmp_evict_set, &candidate_set[target_index+7], conf), &candidate_set[target_index+7]);
	printf("main: test %i %li %p\n", target_index+8, test(candidate_set, c_size, tmp_evict_set, &candidate_set[target_index+8], conf), &candidate_set[target_index+8]);

    freeList(evict_set); // delete eviction set	
    freeList(tmp_evict_set); // delete eviction set	
    return 0;
}
#endif


static struct Node *create_minimal_eviction_set(void **candidate_set, uint64_t candidate_set_size, struct Node* evict_set, void *target_adrs, struct Config *conf){
    if (conf==NULL){
		printf("create_minimal_eviction_set: Conf is NULL!\n");
		return NULL;
	}
	
	// if candidate set is empty, no eviction set can be created
	if (candidate_set==NULL){
		printf("create_minimal_eviction_set: candidate_set is NULL!\n");
		return NULL;
	}
	
	if (candidate_set_size==0){
		printf("create_minimal_eviction_set: candidate_set_size is 0!\n");
		return NULL;
	}
	
	clock_t track_start = clock();
    // init lfsr, variable c stores currently picked candidate integer/index value
    uint64_t lfsr = lfsr_create(), c, cnt_e=0; 
	int64_t c_tmp;
	
	// create current candidate set containing the indexes of unchecked candidates and initialize with all indexes
    struct Node* cind_set = initLinkedList();
    for (uint64_t i=0; i<candidate_set_size-1;i+=8) cind_set = addElement(cind_set, i); 
    
    // while |R| < a and cind still contains possible and unchecked candidates
    //while(a_tmp < EVICT_SIZE_A && cind_set!=NULL){   // TODO change back     
    while(cind_set!=NULL /*&& cnt_e < conf->ways*/){        
        // c <- pick(S) pick candidate index c from candidate set S/cind_set
		do{
			c_tmp=pick(cind_set, candidate_set_size, &lfsr);
		}
        while(c_tmp==-1 || containsValue(evict_set, (uint64_t) c_tmp) || !containsValue(cind_set, (uint64_t) c_tmp)); // prevent picking duplicate candidates and continuing on error
				
		c = (uint64_t) c_tmp;
		// remove c from S S <- S\{c}
		cind_set = deleteElement(cind_set, c);         

        // R union S\{c}
        struct Node *combined_set = unionLists(cind_set, evict_set);

        // if not TEST(R union S\{c}), x)  
		// if removing c results in not evicting x anymore, add c to current eviction set    
        // majority voting for test, if 2 out of 3 times evicted -> >1, if only 1 time or less, <=1
		if(test(candidate_set, candidate_set_size, combined_set, target_adrs, conf)==0){
            evict_set = addElement(evict_set, c);
            cnt_e++; // added elem to evict set -> if enough, evict_set complete          
        }
    }
    printf("cind set count %i\n", count(cind_set));
    if (cind_set==NULL && cnt_e < conf->ways) printf("create_minimal_eviction_set: not successful, eviction set contains less elements than cache ways!\n");
    
    /* baseline algorithm */
	printList(evict_set);
	
	// measure time needed for this algorithm
	double  cpu_time_used = ((double) (clock() - track_start)) / CLOCKS_PER_SEC;
    // Print the measured time
    printf("Time taken by myFunction: %.6f seconds\n", cpu_time_used);
	
	printf("test of evict set %li\n", test(candidate_set, candidate_set_size, evict_set, target_adrs, conf));
	return evict_set;
}

static struct Config *initConfig(uint64_t ways,	uint64_t cache_line_size, uint64_t threshold, uint64_t cache_size, uint64_t test_reps){
	struct Config *conf= (struct Config *) malloc(sizeof(struct Config));
	updateConfig(conf, ways, cache_line_size, threshold, cache_size, test_reps);
	return conf;
}

static void updateConfig(struct Config *conf, uint64_t ways, uint64_t cache_line_size, uint64_t threshold, uint64_t cache_size, uint64_t test_reps){
	conf->ways=ways;
	conf->cache_line_size=cache_line_size;
	conf->threshold=threshold;
	conf->cache_size=cache_size;
	conf->test_reps=test_reps;
}


// say hi to cache
static void wait(uint64_t cycles) {
	unsigned int ignore;
	uint64_t start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

// Function to initialize an empty linked list
static struct Node* initLinkedList() {
    return NULL;  // Return NULL to indicate an empty list
}

static struct Node* addElement(struct Node* head, uint64_t value) {
    // Allocate memory for a new node
    struct Node* newNode = (struct Node*) malloc(sizeof(struct Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    // Set the value and next pointer for the new node
    newNode->value = value;
    newNode->next = head;
    // Update the head to point to the new node
    head = newNode;
    return head;
}

static struct Node* deleteElement(struct Node* head, uint64_t value) {
    struct Node* current = head;
    struct Node* previous = NULL;

    // Traverse the list to find the node with the specified value
    while (current != NULL && current->value != value) {
        previous = current;
        current = current->next;
    }

    // If the value is not found, return the original head
	if (current == NULL){
		printf("deleteElement: value %lu not found in Node %p\n", value, head);
		return head;
	}

    // Update the next pointer of the previous node or the head if the first node is deleted
    if (previous != NULL) previous->next = current->next;
    else head = current->next;
    
    free(current);
    return head;
}

static struct Node* cloneList(struct Node* original) {
    struct Node* cloned = initLinkedList();
    struct Node* current = original;
    while (current != NULL) {
        cloned = addElement(cloned, current->value);
        current = current->next;
    }
    return cloned;
}

static struct Node* unionLists(struct Node* list1, struct Node* list2) {
    struct Node* result = initLinkedList();
    struct Node* current;
    // Add elements from the first list
    current = list1;
    while (current != NULL) {
        result = addElement(result, current->value);
        current = current->next;
    }
    // Add elements from the second list (avoid duplicates)
    current = list2;
    while (current != NULL) {
        if (!containsValue(result, current->value)) result = addElement(result, current->value);
        current = current->next;
    }
    return result;
}

static int containsValue(struct Node* head, uint64_t value) {
    struct Node* current = head;
    while (current != NULL) {
        if (current->value == value) return 1;  // Value found
        current = current->next;
    }
    return 0;  // Value not found
}

static int count(struct Node *head){
	int ctr = 0;
	for(struct Node *it=head;it!=NULL;it=it->next) ctr+=1;
	return ctr;
}

static void printList(struct Node* head) {
    printf("Linked List: ");
    while (head != NULL) {
        printf("%lu ", head->value);
        head = head->next;
    }
    printf("\n");
}

static void freeList(struct Node* head) {
    struct Node* current = head;
    struct Node* next;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

#define FEEDBACK 0x80000000000019E2ULL
static uint64_t lfsr_create(void) {
  uint64_t lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  return lfsr;
}

static uint64_t lfsr_rand(uint64_t* lfsr) {
    for (uint64_t i = 0; i < 8*sizeof(uint64_t); i++) {
        *lfsr = lfsr_step(*lfsr);
    }
    return *lfsr;
}

static uint64_t lfsr_step(uint64_t lfsr) {
  return (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
}

// delete */ to toggle test1 implementation -> keep in mind that change of threshold values is required
/*
static int64_t test1(void *addr, uint64_t size, void* cand, uint64_t threshold){
    if (size==0 || addr==NULL || cand==NULL) return -1; // parameter check
	uint64_t count = size;
    void *cur_adrs = addr;
    wait(1E9);
    // load candidate
    maccess(cand);
    maccess(cand);
    maccess(cand);

    __asm__ volatile ("mfence");
    // load eviction set
    while (count-->0) cur_adrs = maccess(cur_adrs);
    
    // measure time to access candidate
    uint64_t time = rdtscpfence();
    maccess(cand);
    uint64_t delta = rdtscpfence() - time;
    printf("delta %lu\n", delta);
    return delta > threshold;
}




/*/ // not sure what to use though -> Threshold values depend on implementation
static int64_t test1(void *addr, uint64_t size, void* target_adrs, struct Config *conf){    
    // parameter check
    if (size==0){
		printf("test1: size is 0!\n");
		return -1;
	} 	
	 
	if (addr==NULL){
		printf("test1: addr is NULL!\n");
		return -1;
	} 	
	
	if (target_adrs==NULL){
		printf("test1: target_adrs is NULL!\n");
		return -1;
	} 	
	
	if (conf==NULL){
		printf("test1: conf is NULL!\n");
		return -1;
	} 	
	
#ifdef TESTCASE
	clock_t start_clk = clock();
#endif	
	volatile uint64_t time, sum=0;
	for(uint64_t i=0;i<conf->test_reps;i++){
		asm __volatile__ (
			"lfence;"
			"mfence;"
			// load target and set 
			"mov rdx, %2;"
			"mov rsi, [%3];" // load target
			"lfence;"
			// BEGIN - read every entry in addr
			"loop:"
			"mov %1, [%1];"
			"dec rdx;"
			"jnz loop;"
			// END - reading set
			// measure start
			"lfence;"
			"mfence;"
			"rdtscp;"		
			"lfence;"
			"mov rsi, rax;"
			// high precision
			"shl rdx, 32;"
			"or rsi, rdx;"
			"mov rax, [%3];" // load target 	
			"lfence;"
			"rdtscp;"
			// start - high precision
			"shl rdx, 32;"
			"or rax, rdx;"
			// end - high precision
			"sub rax, rsi;"
			"clflush [%3];" // flush data from candidate for repeated loading
			: "=a" (time)
			: "r" (addr), "r" (size), "r" (target_adrs)
			: "rsi", "rdx", "rcx"
		);
		sum +=time;
	}
#ifdef TESTCASE
	//printf("test1: measurement %lu\n", sum/conf->test_reps);
	printf("test1: took %.6f seconds to finish, measurement %lu\n", ((double) (clock() - start_clk)) / CLOCKS_PER_SEC, sum/conf->test_reps);
#endif	
    time_buf = sum/conf->test_reps;
	return sum/conf->test_reps > conf->threshold? 1 : 0;
} 

static void create_pointer_chase(void **candidate_set, uint64_t c_size, struct Node* set){
#ifdef TESTCASE
	clock_t start = clock();
#endif
	if (set == NULL) {
		printf("create_pointer_chase: set is NULL!\n");
		return; // no pointer chase 
	}

    // create pointer chase between set elements
    struct Node* cur_no;  // current index (from set)
    for (cur_no=set;cur_no->next !=NULL; cur_no=cur_no->next){
        if (cur_no->next->value >= c_size){
            printf("create_pointer_chase: current index from set greater than size! Element not contained in candidate_set!\n");
            return;
        } 
        // set pointer to next element cur_no holds current index, cur_no->next holds next index, &addr[cur_no->next->value] is ptr to respective elem
        candidate_set[cur_no->value] = &candidate_set[cur_no->next->value];    
    }
    candidate_set[cur_no->value] = &candidate_set[set->value]; // set pointer from last element to first element

#ifdef TESTCASE
	clock_t end = clock();
	double  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

	printf("create_pointer_chase: took %.6f seconds to finish\n", cpu_time_used);
#endif	

}

static int64_t pick(struct Node* candidate_set, uint64_t base_size, uint64_t *lfsr) {
    // uninitialized parameters
    if (lfsr==NULL){
		printf("pick: lfsr is NULL!\n");
		return -1;
	} 
	if (candidate_set==NULL){
		printf("pick: candidate_set is NULL!\n");
		return -1;
	} 
	if (base_size==0){
		printf("pick: base_size is 0!\n");
		return -1;
	} 
	
    uint64_t c=0, j, c_size; // c picked candidate, j index in candidate_set, c_size current candidate set size

    // pick a random number, compute modulo amount of elements left in candidate set and take the value from the node element at the resulting position
    c_size = (uint64_t) count(candidate_set);
    struct Node *cur_node = candidate_set;
    j = lfsr_rand(lfsr) % c_size;
    do{
        if (c==j) break;
        c++;
        cur_node = cur_node->next;
    }
    while(cur_node->next != NULL);

    int64_t ret =(int64_t) cur_node->value;   
#ifdef TESTCASE
    printf("return value %li\n", ret);
#endif
    return ret;
}


static int64_t test(void **candidate_set, uint64_t candidate_set_size, struct Node *test_index_set, void *target_adrs, struct Config *conf){
	// empty candidate set, no array to create pointer chase on    
    if (candidate_set==NULL){
		printf("test: candidate_set is NULL!\n");
		return -1;
	} 
	if (candidate_set_size==0){
		printf("test: candidate_set_size is 0!\n");
		return -1;
	} 
	if (test_index_set==NULL){
		printf("test: test_index_set is NULL!\n");
		return -1;
	} 
	
	uint64_t test_index_set_size=count(test_index_set);
	
	// prepare pointer chase between elements from candidate_set indexed by test_index_set 
	create_pointer_chase(candidate_set, candidate_set_size, test_index_set);
	
	// test 
	return test1(candidate_set[test_index_set->value], count(test_index_set), target_adrs, conf);
}