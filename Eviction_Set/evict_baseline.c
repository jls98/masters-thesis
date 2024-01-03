#include <stdint.h>
#include <stdio.h>

/* threshold values for loading 1 adrs                  */
// IntelGen12 e core
#define THRESHOLD_SINGLE_L1D_E12 4      // ~2.2
#define THRESHOLD_SINGLE_L2_E12 17      // <15
#define THRESHOLD_SINGLE_LLC_E12 70     // >40 (?)
#define THRESHOLD_SINGLE_DEFAULT_E12 THRESHOLD_SINGLE_L1D_E12

// IntelGen12 p core
#define THRESHOLD_SINGLE_L1D_P12 4      // 2.6
#define THRESHOLD_SINGLE_L2_P12 9       // <8
#define THRESHOLD_SINGLE_LLC_P12 50     // ~30 (?)
#define THRESHOLD_SINGLE_DEFAULT_P12 THRESHOLD_SINGLE_L1D_P12

// IntelGen7 ? lab pc
#define THRESHOLD_SINGLE_L1D_LAB 5      // ~3.5
#define THRESHOLD_SINGLE_L2_LAB 12      // <10
#define THRESHOLD_SINGLE_LLC_LAB 45     // ~30
#define THRESHOLD_SINGLE_DEFAULT_LAB THRESHOLD_SINGLE_L1D_LAB


/* #################################################### */
/* ####################### utils ###################### */
/* #################################################### */

/* wait for cycles cycles and activate cache            */
static void wait(const uint64_t cycles);


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
/* addr: pointer to mapped adrs with size elements.     */
/* cand: candidate adrs.                                */
/* returns true if measurement is above a threshold.    */
static uint64_t test1(const void *addr, const uint64_t size, const void* cand)

/* test2: eviction test for an arbitrary address.       */
/* Loads all elements from eviction set and then loads  */
/* elements again and measures time.                    */
/* addr: pointer to mapped adrs with size elements.     */
/* returns true if measurement is above a threshold.    */
static uint64_t test2(const void *addr, const uint64_t size)

/* pointer chase: creates pointer chase in subset of    */
/* by addr mapped set with size many elements.          */
/* set contains pointer to set of indexes of set of     */
/* interest with set_size many elements.                */
static void create_pointer_chase(const void *addr, const uint64_t size, const uint64_t *set, const uint64_t set_size);

/* pick lfsr pseudo-randomized next candidate (index)   */
/* new candidate should not be in eviction set, be part */
/* of base set and picked with lfsr state in possible   */
/* range. Furthermore, remove candidate index from      */
/* currently available indexes in base set.             */
/* returns index of candidate                           */
static uint64_t pick(const uint64_t *set_addr, const uint64_t set_size, const uint64_t *base_addr, const uint64_t size, uint64_t lfsr);


int main(int ac, char **av){
    return 0;
}

static void wait(const uint64_t cycles) {
	unsigned int ignore;
	uint64_t start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
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

static uint64_t test1(const void *addr, const uint64_t size, const void* cand){
    return 1; // TODO, implement first!
}

static uint64_t test2(const void *addr, const uint64_t size){
    return 1; // TODO
}

static void create_pointer_chase(const void *addr, const uint64_t size, const uint64_t *set, const uint64_t set_size){
    // TODO unrandomized iteriert durch indexes
}

static uint64_t pick(const uint64_t *set_addr, const uint64_t set_size, const uint64_t *base_addr, const uint64_t size, uint64_t lfsr) {
    
}
