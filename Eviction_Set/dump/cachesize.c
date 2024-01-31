#include <x86intrin.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MINSIZE 2
#define MAXSIZE 24



#define FEEDBACK 0x80000000000019E2ULL


uint64_t step(uint64_t lfsr) {
  lfsr = (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
  return lfsr;
}


char *map(uint64_t size) {
  char *rv = mmap(0, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
  if (rv == MAP_FAILED)
    return 0;
  for (int i=0; i < size; i+= 4096)
    rv[i] = 0;
  return rv;
}


int access(char *base, uint64_t count, uint64_t size) {
  uint64_t lfsr;
  asm volatile("rdrand %0": "=r" (lfsr)::"flags");
  int rv = 0;
  for (uint64_t i = 0; i < count; i++) {
    lfsr = step(lfsr + rv);
    uint64_t offset = lfsr & (size - 1);
    rv += base[offset];
  }
  return rv;
}

volatile int dummy;

int main(int ac, char **av) {
  char *base = map(1<<MAXSIZE);
  uint64_t count = 1ULL<<(MAXSIZE + 3);
  access(base, 1<<25, 1024);
  printf("Starting\n");
  uint64_t prevavg = 0;
  for (int lsize = MINSIZE; lsize <= MAXSIZE; lsize++) {
    uint64_t start = __rdtsc();
    dummy += access(base, count, 1<<lsize);
    uint64_t time = __rdtsc() - start;
    uint64_t avg = (time*1000 + count/2)/count;
    printf("%3d %lu (%lu.%03u)\n", lsize, time, avg/1000, avg % 1000);
  }
}



