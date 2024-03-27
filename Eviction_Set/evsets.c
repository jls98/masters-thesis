#ifndef evsets_c
#define evsets_c

#define u64 uint64_t

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <time.h>

/* #################################################### */
/* ####################### utils ###################### */
/* #################################################### */


/* #################################################### */
/* ###################### structs ##################### */

typedef struct node
{
	struct node *next;
	struct node *prev;
	int set;
	size_t delta;
	char pad[32]; // up to 64B
} Node;


/* #################################################### */
/* ##################### functions #################### */

inline void maccess (void *p){
    __asm__ volatile ("mov rax, [%0]\n" : : "r" (p) : "rax");
}

inline u64 rdtscpfence()
{
	u64 a, d;
	__asm__ volatile 
        ("lfence;"
         "rdtscp;"
         "lfence;"               
        : "=a" (a), "=d" (d) : : "ecx");
	return ((d<<32) | a);
}




inline void traverse_

#endif /* evsets_c */