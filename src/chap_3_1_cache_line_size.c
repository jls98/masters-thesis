#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <x86intrin.h>

// Tests function for the cache line size. 
void test_cache_line_size();

int main(){
    test_cache_line_size();
    return 0;
}

static void wait(const uint64_t cycles) {
	unsigned int ignore;
	uint64_t start = __rdtscp(&ignore);
	while (__rdtscp(&ignore) - start < cycles);
}

void test_cache_line_size(){
    wait(1E9);
    // Initialize array to access, variables
    int len_buf = 16777216*2; // Set buffer size here.
    uint8_t *buf = malloc(len_buf*sizeof(uint8_t));
    uint64_t *diffs = malloc(12*sizeof(uint64_t)), start, end; // Variables for measurements.

    // Access whole buffer so that the first iteration has cached elements, too.
    for(int i=0;i<len_buf;i++) buf[i] = 0xff; 
    
    // Increase exponent K, for stride 1<<K.
    for(int K=1;K<13;K++){
        // Start timer.
        __asm__ __volatile__ ("mfence; rdtsc" : "=A" (start));
        // Access buffer once.
        for (int i = 0; i < len_buf; i += 1<<K) buf[i] *= 3;
        // Stop timer.
        __asm__ __volatile__ ("mfence; rdtsc" : "=A" (end));
        diffs[K] = end-start;
    }
    printf("stride|  total | avg\n");
    for(int i=0;i<12;i++) printf(" %4d %9lu %3.3f \n", 1<<i, diffs[i], (double)diffs[i]/(len_buf/(1<<i)));
}