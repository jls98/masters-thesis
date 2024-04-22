#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <math.h>

#define u64 uint64_t


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