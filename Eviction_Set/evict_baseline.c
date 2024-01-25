#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>

/* threshold values for loading 1 adrs                  */
// IntelGen12 e core
#define THRESHOLD_SINGLE_L1D_E12 60      // ~2.2/ 61<60 on single measurement / new 200? 140-180, 59 if L1, 60+ if L2 WTF?
#define THRESHOLD_SINGLE_L2_E12 70      // cached ~<18, 60-64, or 70?
#define THRESHOLD_SINGLE_LLC_E12 70     // ~52 (?)
#define THRESHOLD_SINGLE_DEFAULT_E12 THRESHOLD_SINGLE_L1D_E12

// IntelGen12 p core
#define THRESHOLD_SINGLE_L1D_P12 270      // 2.6/ 31<30 in single measurement / new 150? 60-90
#define THRESHOLD_SINGLE_L2_P12 9       // <8
#define THRESHOLD_SINGLE_LLC_P12 50     // ~30 (?)
#define THRESHOLD_SINGLE_DEFAULT_P12 THRESHOLD_SINGLE_L1D_P12

// IntelGen7 ? lab pc
#define THRESHOLD_SINGLE_L1D_LAB 44      // ~3.5 / 40-42 zu 46-50 38-42 for L1?
#define THRESHOLD_SINGLE_L2_LAB 70      // <10, 46, 47, 48 for L2?
#define THRESHOLD_SINGLE_LLC_LAB 200     // ~30
#define THRESHOLD_SINGLE_DEFAULT_LAB THRESHOLD_SINGLE_L1D_LAB

#define THRESHOLD THRESHOLD_SINGLE_L1D_LAB

#define CACHESIZE_DEFAULT 32768         // L1D e|lab 
//#define CACHESIZE_DEFAULT OTHERS TODO

// optional arguments for threshold in test functions, defaults to THRESHOLD
#define DEF_OR_ARG(value,...) value
#define TEST1(addr, size, cand, ...) test1(addr, size, cand, DEF_OR_ARG(__VA_ARGS__ __VA_OPT__(,) THRESHOLD))

#define EVICT_SIZE_A 8 // p cores 12/10 ways, else 8 ways

/* #################################################### */
/* ####################### utils ###################### */
/* #################################################### */

/* wait for cycles cycles and activate cache            */
static void wait(uint64_t cycles);

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

/* test1: eviction test for the specific address cand.  */
/* Loads cand and then all elements from eviction set   */
/* test. Finally, loads cand and measures time.         */
/* addr: pointer to first element from eviction set     */
/* cand: candidate adrs.                                */
/* returns true if measurement is above a threshold.    */
static int64_t test1(void *addr, uint64_t size, void* cand, uint64_t threshold);

/* pointer chase: creates pointer chase in subset of    */
/* by candidate_set mapped set with c_size elements.    */
/* set contains set of indexes for pointer chase        */
static void create_pointer_chase(void **candidate_set, uint64_t c_size, struct Node* set);

/* Pick lfsr pseudo-randomized next candidate (index).   
 * The new candidate should not be part of eviction set     
 * evict_set, be part of candidate set and picked with lfsr  
 * state in possible range.                              
 * returns candidate index, if none found base_size+1   */
static uint64_t pick(struct Node* evict_set, struct Node* candidate_set, uint64_t base_size, uint64_t *lfsr);

/* create minimal eviction set from candidate set for   */
/* victim_adrs in evict_set                             */
static struct Node * create_minimal_eviction_set(void **candidate_set, uint64_t base_size, struct Node* evict_set, void *victim_adrs);

/* #################################################### */
/* ################## implementation ################## */
/* #################################################### */
static uint64_t *times;
#ifndef TESTCASE
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
// optional argument 1 cache size
// evict_baseline VICTIM_ADRS BASE_SIZE


int main(int ac, char **av){
    /* preparation */
    wait(1E9); // boost cache 
	times = malloc(501*sizeof(uint64_t *));
	for(int i=0;i<501;i++) times[i]=0;
    // if (cache) size set, take; divide by 4 since its cache size in bytes and we have 64 bit/8 byte pointer arrays but also take double size
    uint64_t *base_size = malloc(sizeof(uint64_t *));
	*base_size = ac == 3? atoi(av[2])/4 : CACHESIZE_DEFAULT/4; // /8 *2 for twice the size of respective cache in bytes

    // R <- {}
    // allocate space for eviction set
    struct Node* evict_set = initLinkedList();

    // map candidate_set (using hugepages, twice the size of cache in bytes)
    void **candidate_set = mmap(NULL, *base_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
	for(uint64_t i=0;i<*base_size;i++) candidate_set[i] = &candidate_set[i]; // avoid null page by writing something on every entry (pointer to itself)
	
    // if adrs set, otherwise use some other uint64_t adrs
    uint64_t *victim_adrs = ac > 1? (uint64_t *)strtoull(av[1], NULL, 0) : base_size;
	
	// create evict set
    struct Node * tmp_evict_set = create_minimal_eviction_set(candidate_set, *base_size, evict_set, victim_adrs);
	
    printf("Eviction set for victim at %p %i\n", victim_adrs, tmp_evict_set==NULL);

	printf("Indexes in minimal eviction set:\n"); // print indexes of eviction set
	for(struct Node *it = tmp_evict_set;it!=NULL;it=it->next) printf("-%p, %lu\n", it, it->value);
	
	printf("\nTesting victim adrs %p now: \n", victim_adrs);

	// measure time when cached	
	load(victim_adrs);
	uint64_t time = probe(victim_adrs);
	
	printf("Time loading victim cached %lu\n", time);
	// measure time when uncached	
	flush(victim_adrs);
	time = probe(victim_adrs);

	printf("Time loading victim uncached %lu\n", time);

	create_pointer_chase(candidate_set, *base_size, tmp_evict_set);
		
	if(tmp_evict_set != NULL){
		void *cur = candidate_set[tmp_evict_set->value];
		load(victim_adrs);
		for(uint64_t counterj = 0;counterj<EVICT_SIZE_A+2;counterj++){
			cur=*((void **)cur);
			load(cur);
			//printf("%p, %lu\n", it, it->value);
		}
		
		time = probe(victim_adrs);
		printf("Time loading victim after evict set  %lu\n", time);
	}

    freeList(evict_set); // delete eviction set
	printf("times\n");
	for(int i=0;i<501;i++) printf("%4i: %6ld\n", i, times[i]);
	free(times);
    return 0;
}
#endif


static struct Node * create_minimal_eviction_set(void **candidate_set, uint64_t base_size, struct Node* evict_set, void *victim_adrs){
    clock_t track_start = clock();
    // init lfsr, variable c stores currently picked candidate integer/index value
    uint64_t lfsr = lfsr_create(), c, a_tmp=0, cnt, cnt_e; 
    
	// create current candidate set containing the indexes of unchecked candidates and initialize with all indexes
    struct Node* cind_set = initLinkedList();
    for (uint64_t i=0; i<base_size-1;i++) cind_set = addElement(cind_set, i); 
    
    // while |R| < a and cind still contains possible and unchecked candidates
    //while(a_tmp < EVICT_SIZE_A && cind_set!=NULL){        
    while(cind_set!=NULL){        
        // c <- pick(S) pick candidate index c from candidate set S/cind_set
		do{
			c=pick(evict_set, cind_set, base_size, &lfsr);
		}
        while(containsValue(evict_set, c) || !containsValue(cind_set, c)); // prevent picking duplicate candidates
		
		if (c==base_size+1) printf("pick returned invalid value!\n");
		
		// remove c from S S <- S\{c}
		cind_set = deleteElement(cind_set, c);         

        // R union S\{c}
        struct Node *combined_set = unionLists(cind_set, evict_set);
        
        // create pointer chase 
        create_pointer_chase(candidate_set, base_size, combined_set);
        
        // count amount of elements in combined_set
        cnt=0;
        for(struct Node* it=combined_set;it!=NULL; it=it->next) cnt++;
		cnt_e=0;
        for(struct Node* it=evict_set;it!=NULL; it=it->next) cnt_e++;
		if(cnt%1000==0) printf("cnt %lu, evict %lu\n", cnt, cnt_e);
        // if not TEST(R union S\{c}), x)  
		// if removing c results in not evicting x anymore, add c to current eviction set    
		if (cnt==cnt_e) break;
		if(combined_set != NULL && !TEST1(candidate_set[combined_set->value], cnt, victim_adrs)){
			//printf("%li\n", TEST1(candidate_set[combined_set->value], cnt, victim_adrs));
			if(!TEST1(candidate_set[combined_set->value], cnt, victim_adrs) && !TEST1(candidate_set[combined_set->value], cnt, victim_adrs)){
				evict_set = addElement(evict_set, c);
				//printf("head evict_set: %p\n", evict_set);			
				a_tmp++; // added elem to evict set -> if enough, evict_set complete
				
				// test if its an eviction set
				void *cur = candidate_set[evict_set->value];
				load(victim_adrs);
				for(uint64_t counterj = 0;counterj<100;counterj++){ // 100 is random
					cur=*((void **)cur);
					load(cur);
				}
				time = probe(victim_adrs);
				printf("Time loading victim after evict set  %lu\n", time);		
				if(time>threshold) break;
			}
		
				
        }
    }
    if (cind_set==NULL && a_tmp < EVICT_SIZE_A) printf("create_minimal_eviction_set: not successful!\n");
	//printf("a_tmp Elements in eviction set %lu, cind_set empty %i, evict_set empty %i\n", a_tmp, cind_set==NULL, evict_set==NULL);
    /* baseline algorithm */
	printList(evict_set);
	
	// measure time needed for this algorithm
	double  cpu_time_used = ((double) (clock() - track_start)) / CLOCKS_PER_SEC;
    // Print the measured time
    printf("Time taken by myFunction: %f seconds\n", cpu_time_used);
	return evict_set;
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
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
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
#define reps 200 // weird effects when increasing repetitions -> rapid increase of measured times per iteration
static int64_t test1(void *addr, uint64_t size, void* cand, uint64_t threshold){    
    if (size==0 || addr==NULL || cand==NULL) return -1; // parameter check
	
	wait(1E9);
	volatile uint64_t time, sum=0;
	for(uint64_t i=0;i<reps;i++){
		asm __volatile__ (
			"lfence;"
			"mfence;"
			// load candidate and set 
			"mov rax, %1;"
			"mov rdx, %2;"
			"mov rsi, [%3];" // load candidate
			"lfence;"
			// BEGIN - read every entry in addr
			"loop:"
			"mov rax, [rax];"
			"dec rdx;"
			"jnz loop;"
			// END - reading set
			// measure start
			"lfence;"
			"mfence;"
			"rdtsc;"		
			"lfence;"
			"mov rsi, rax;"
			// high precision
			"shl rdx, 32;"
			"or rsi, rdx;"
			"mov rax, [%3];" // load candidate 	
			"lfence;"
			"rdtsc;"
			// start - high precision
			"shl rdx, 32;"
			"or rax, rdx;"
			// end - high precision
			"sub rax, rsi;"
			"clflush [%3];" // flush data from candidate for repeated loading
			: "=a" (time)
			: "c" (addr), "r" (2*size), "r" (cand)
			: "rsi", "rdx"
		);
		sum +=time;
	}
    if(sum/reps<500) times[sum/reps]+=1;
	else times[500]+=1;
#ifndef TESTCASE
	
	if ( sum/reps <= threshold)printf("Sum %lu, sum/reps %lu for size %lu\n", sum, sum/reps, size);
#endif
#ifdef TESTCASE
	printf("Sum %lu, sum/reps %lu for size %lu\n", sum, sum/reps, size);
#endif

	return sum/reps > threshold? 1 : 0;
} /**/
 /**/

static void create_pointer_chase(void **candidate_set, uint64_t c_size, struct Node* set){
    if (set == NULL) return; // no pointer chase 

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
}

static uint64_t pick(struct Node* evict_set, struct Node* candidate_set, uint64_t base_size, uint64_t *lfsr) {
    // uninitialized parameters
    if (lfsr==NULL || candidate_set==NULL || base_size ==0) return base_size+1;
    uint64_t c, j, c_size; // c candidate, j index, c_size current candidate set size
    struct Node* cur_node;
    // get candidate set size  
    // iterate over all elements and count
    for(cur_node = candidate_set, c_size=0; cur_node != NULL; c_size++, cur_node = cur_node->next);
   
    // 99999 times base size should suffice to find candidate in legitimate cases
    for(uint64_t i=0; i<99999*base_size;i++){         
        // pick pseudo-random candidate by index from candidate_set
        // iterate over rand mod c_size elements in current candidate set list and get index value
        for(j=0, cur_node=candidate_set;cur_node != NULL;j++,cur_node=cur_node->next){
            if (j == lfsr_rand(lfsr) % c_size){
                c=cur_node->value;
                break;
            }
        }
        // check if its in eviction set
        for (cur_node=evict_set;cur_node != NULL;cur_node=cur_node->next){
            if (cur_node->value == c) break; // need new candidate
        }
        if(cur_node == NULL && c<base_size) return c; // no match in eviction set (no break before last), c<base_size since I got very odd return values...
    }
    // did not find candidate -> nothing to pick
	printf("pick: no candidate found for evict set %p and candidate set %p and base size %lu with lfsr %p!\n", evict_set, candidate_set, base_size, lfsr);
    return base_size+1;
}