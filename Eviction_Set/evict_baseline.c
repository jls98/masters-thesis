#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


/* threshold values for loading 1 adrs                  */
// IntelGen12 e core
#define THRESHOLD_SINGLE_L1D_E12 200      // ~2.2/ 61<60 on single measurement / new 200? 140-180
#define THRESHOLD_SINGLE_L2_E12 17      // <15
#define THRESHOLD_SINGLE_LLC_E12 70     // >40 (?)
#define THRESHOLD_SINGLE_DEFAULT_E12 THRESHOLD_SINGLE_L1D_E12

// IntelGen12 p core
#define THRESHOLD_SINGLE_L1D_P12 150      // 2.6/ 31<30 in single measurement / new 150? 60-90
#define THRESHOLD_SINGLE_L2_P12 9       // <8
#define THRESHOLD_SINGLE_LLC_P12 50     // ~30 (?)
#define THRESHOLD_SINGLE_DEFAULT_P12 THRESHOLD_SINGLE_L1D_P12

// IntelGen7 ? lab pc
#define THRESHOLD_SINGLE_L1D_LAB 5      // ~3.5
#define THRESHOLD_SINGLE_L2_LAB 12      // <10
#define THRESHOLD_SINGLE_LLC_LAB 45     // ~30
#define THRESHOLD_SINGLE_DEFAULT_LAB THRESHOLD_SINGLE_L1D_LAB

#define THRESHOLD THRESHOLD_SINGLE_DEFAULT_P12

#define CACHESIZE_DEFAULT 32768         // L1D e|lab 
//#define CACHESIZE_DEFAULT OTHERS TODO

// optional arguments for threshold in test functions, defaults to THRESHOLD
#define DEF_OR_ARG(value,...) value
#define TEST1(addr, size, cand, ...) test1(addr, size, cand, DEF_OR_ARG(__VA_ARGS__ __VA_OPT__(,) THRESHOLD))
#define TEST2(addr, size, ...) test2(addr, size, DEF_OR_ARG(__VA_ARGS__ __VA_OPT__(,) THRESHOLD))



/* #################################################### */
/* ####################### utils ###################### */
/* #################################################### */

/* wait for cycles cycles and activate cache            */
static void wait(uint64_t cycles);

/* access pointed adrs p                                */
static inline void* maccess(void *p);

/* lfenced time measurement; returns time stamp         */
static inline uint64_t rdtscpfence();

/* time measurement; returns time stamp                 */
static inline uint64_t rdtscp();

/* linked list containing an index and a pointer to     */
/* the next element                                     */
struct Node {
    uint64_t value;
    struct Node* next;
};

/* Function to add a new element to the linked list     */
struct Node* addElement(struct Node* head, uint64_t value);

/* Function to print the elements of the linked list    */
void printList(struct Node* head);

/* Function to free the memory allocated for the linked */
/* list                                                 */
void freeList(struct Node* head);

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

/* test2: eviction test for an arbitrary address.       */
/* Loads all elements from eviction set and then loads  */
/* elements again and measures time.                    */
/* addr: pointer to mapped adrs with size elements.     */
/* returns true if measurement is above a threshold.    */
static uint64_t test2(void *addr, uint64_t size);

/* pointer chase: creates pointer chase in subset of    */
/* by addr mapped set with size many elements.          */
/* set contains pointer to set of indexes of set of     */
/* interest with set_size many elements.                */
static void create_pointer_chase(void **addr, uint64_t size, uint64_t *set, uint64_t set_size);

/* pick lfsr pseudo-randomized next candidate (index)   */
/* new candidate should not be in eviction set, be part */
/* of base set and picked with lfsr state in possible   */
/* range. Furthermore, remove candidate index from      */
/* currently available indexes in base set.             */
/* returns index of candidate, if none found size+1     */
static uint64_t pick(uint64_t *set, uint64_t set_size, uint64_t *base, uint64_t base_size, uint64_t size, uint64_t *lfsr);

/* create minimal eviction set from base set for        */
/* victim_adrs in evict_set                             */
static void create_minimal_eviction_set(void *base_set, uint64_t base_size, uint64_t *evict_set, uint64_t *evict_size, uint64_t *victim_adrs);
/* #################################################### */
/* ################## implementation ################## */
/* #################################################### */

#ifndef TESTCASE
// optional argument 1 cache size
int main(int ac, char **av){
    /* preparation */
    wait(1E9); // boost cache 
    
    // if (cache) size set, take; divide by 4 since its cache size in bytes and we have 64 bit/8 byte pointer arrays but also take double size
    uint64_t base_size = ac == 3? atoi(av[2])/4 : CACHESIZE_DEFAULT/4;     
    uint64_t eviction_candidate_size = 0; // R <- {}

    // allocate space for eviction set
    uint64_t *evict_set = (uint64_t *) malloc(base_size/2 * sizeof(uint64_t));

    // map base set (using hugepages, twice the size of cache in bytes)
    void* *base_set = mmap(NULL, base_size * sizeof(void *), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

    // if adrs set, otherwise use some other uint64_t adrs
    uint64_t *victim_adrs = ac > 1? (uint64_t *)strtoull(av[1], NULL, 0) : &eviction_candidate_size;
    
    create_minimal_eviction_set(base_set, base_size, evict_set, &eviction_candidate_size, victim_adrs);
    
    return 0;
}
#endif

#define EVICT_SIZE_A 12 // p cores 12 ways
static void create_minimal_eviction_set(void *base_set, uint64_t base_size, uint64_t *evict_set, uint64_t *evict_size, uint64_t *victim_adrs){
    //uint64_t *current_base_set = (uint64_t *) malloc(base_size * sizeof(uint64_t));
    
    //uint64_t lfsr = lfsr_create(), c; // init lfsr, var c for picked candidate
    
    // create current candidate set and initialize with all indexes
    //uint64_t *current_base_set = (uint64_t *) malloc(base_size * sizeof(void *));
    
    
    // while |R| < a / while current eviction set is not big enough
    //while(*evict_size < EVICT_SIZE_A){        
        // c <- pick(S) pick candidate c from candidate set S
        // remove c from S
        //c=pick(evict_set, evict_size, uint64_t *base_set, uint64_t base_size, uint64_t size, uint64_t *lfsr)
    
    // if not TEST(R union S\{c}), x)  if removing c results in not evicting x anymore, add c to current eviction set    
      
    //}
 
    
    return; // TODO
    /* baseline algorithm */
    // TODO
}

static void wait(uint64_t cycles) {
	uint64_t start = rdtscp();
	while (rdtscp() - start < cycles);
}

// Function to initialize an empty linked list
struct Node* initLinkedList() {
    return NULL;  // Return NULL to indicate an empty list
}

struct Node* addElement(struct Node* head, uint64_t value) {
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

void printList(struct Node* head) {
    printf("Linked List: ");
    while (head != NULL) {
        printf("%lu ", head->value);
        head = head->next;
    }
    printf("\n");
}

void freeList(struct Node* head) {
    struct Node* current = head;
    struct Node* next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

static inline void* maccess(void *p){
    void *ret;
    __asm__ volatile("movq rax, %0;" : "=a"(ret) : "c" (p));
    return ret;
}

static inline uint64_t rdtscpfence(){
    uint64_t a, d;
    __asm__ volatile(
        "lfence;"
        "rdtscp;"
        "lfence;"
        : "=a" (a), "=d" (d) 
        :: "ecx");
    return ((d<<32) | a);
}

static inline uint64_t rdtscp()
{
	unsigned a, d;
	__asm__ volatile(
    "rdtscp;"
	"mov %0, edx;"
	"mov %1, eax;"
	: "=r" (a), "=r" (d)
	:: "rax", "rcx", "rdx");
	return ((uint64_t)a << 32) | d;
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
/* */
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

/*/ not sure what to use though -> Threshold values depend on implementation
static int64_t test1(void *addr, uint64_t size, void* cand, uint64_t threshold){    
    if (size==0 || addr==NULL || cand==NULL) return -1; // parameter check

	volatile uint64_t time;
	asm __volatile__ (
		// load candidate and set 
		// BEGIN - read every entry in addr
        "lfence;"
        "mov rax, %1;"
        "mov rdx, %2;"
		"mov rsi, [%3];" // load candidate 
        "loop:"
		"mov rax, [rax];"
        "dec rdx;"
        "jnz loop;"
		// END - reading set
        // measure start
		"mfence;"
        "lfence;"
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
		: "=a" (time)
		: "c" (addr), "r" (size), "r" (cand)
		: "rsi", "rdx"
	);
	return time > threshold? 1 : 0;
} /**/

static uint64_t test2(void *addr, uint64_t size){
    return 1; // TODO
}

static void create_pointer_chase(void **addr, uint64_t size, uint64_t *set, uint64_t set_size){
    
    for (uint64_t i=0; i< size-1;i++) addr[i]=&addr[i]; // every adrs points to itself/cleanup
       
    if (set_size == 0) return; // no pointer chase 
    
    // create pointer chase between set elements
    uint64_t cur_in=set[0]; // current index (from set)
    if (set[0] >= size){
        printf("create_pointer_chase: index set[0] > size! Element not contained in base set!\n");
        return;
    } 
    for (uint64_t i = 1; i<set_size; i++){
        if (set[i] >= size){
            printf("create_pointer_chase: current index > size! Element not contained in base set!\n");
            return;
        } 
        // set pointer to next element (set[i] contains index of next element)
        addr[cur_in] = &addr[set[i]]; 
        cur_in=set[i];   
    }
    addr[cur_in] = &addr[set[0]]; // set pointer from last element to first element

}

static uint64_t pick(uint64_t *set, uint64_t set_size, uint64_t *base, uint64_t base_size, uint64_t size, uint64_t *lfsr) {
    // uninitialized parameters
    if (lfsr==NULL || set==NULL || base==NULL || base_size ==0 || size==0){
        return size+1;
    }
    
    uint64_t candidate, j;
    
    // 99999 times set size + base size should suffice to find candidate in legitimate cases
    for(uint64_t i=0; i<99999*size;i++){         
        // pick pseudo-random candidate by index from base set
        candidate = lfsr_rand(lfsr) % base_size;
        
        // check if its in eviction set
        for (j=0;j<set_size-1;j++){
            if (set[j] == base[candidate]) break; // need new candidate
        }
        if(j==set_size-1) return base[candidate]; // no match in eviction set
    }
    
    // did not find candidate -> nothing to pick
    return size+1;
}