#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    asm volatile ("rdtscp" : "=A" (x));
    return x;
}

int probe(char *adrs)
{
    volatile unsigned long time;    
	uint64_t start = rdtsc();
	int a=0;
	while(rdtsc()-start < 1000000000){a++;}
    asm volatile (
        " lfence            \n"
        " rdtsc             \n"
        " mov r8, rax 		\n"
        " mov rax, [%1]		\n"
        " lfence            \n"
        " rdtsc             \n"
        " sub rax, r8 		\n"
        : "=&a" (time)
        : "r" (adrs)
        : "rdx", "r8");
    return time;
}

int probe_t2(char *adrs)
{
    volatile unsigned long time;    
	uint64_t start = rdtsc();
	int a=0;
	while(rdtsc()-start < 1000000000){a++;}
    asm volatile (
	    " mfence            \n"
        " lfence            \n"
        " rdtsc             \n"
        " mov r8, rax 		\n"
        " mov rax, [%1]		\n"
        " lfence            \n"
        " rdtsc             \n"
        " sub rax, r8 		\n"
        " clflush [%1]     \n"
        : "=&a" (time)
        : "r" (adrs)
        : "rdx", "r8");
    return time;
}

int measure(){	
	FILE *file_pointer = fopen("./test", "r"); // hard coded path to open the executable used by the victim 
    int file_descriptor = fileno(file_pointer);     
    struct stat st_buf;
    fstat(file_descriptor, &st_buf);
    int map_len = st_buf.st_size;
	
	void *base = mmap(NULL, map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
	
	
	// task 1
	int uncached = probe(base);
	int result = probe(base);
	printf("result for task 1 is %i, uncached is %i\n", result, uncached);


	// task 2 
	int cached2 = probe_t2(base);
	int uncached2 = probe_t2(base);
	printf("result for task 2 is %i uncached, cached is is %i, adress is %p\n", uncached2, cached2, base);
	
	// task 3
	char* char_ptr = (char*) base;
	for (int i = 0; i< 20; i++)	printf("%c", char_ptr[i]);
	printf("\n ---- \ncontrol sequence of mapped file\n ---- \n\n");
	
	// task 4
	void *base_t4 = mmap(NULL, map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
	
	// flush base adrs and load in mem
	probe_t2(base);
	probe_t2(base_t4);
	
	int uncached_t4 = probe(base);
	int cached_shared_t4 = probe(base_t4);
	printf("loading base address took %i and loading (same) base address from second mapping later took %i\n", uncached_t4, cached_shared_t4);
	
	
	// task 5
	void *base_anon = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	for (int i = 0; i< 20; i++)	printf("%02x", *((char*)base_anon+i));
	printf("\n ---- \ncontrol sequence of anon mapping\n ---- \n\n");
	
	munmap(base, map_len);
	munmap(base_anon, 4096);
	munmap(base_t4, map_len);
	return result;
}

void task6(){
	void *base_anon1 = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	void *base_anon2 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	void *base_anon3 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	void *base_anon4 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	//void *base_anon5 = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	//void *base_anon6 = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	//void *base_anon7 = mmap(NULL, 4096, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	printf("load anon pages\n");
	int first_load_first_ptr = probe(base_anon1);
	int second_load_first_ptr = probe(base_anon1);
	int first_load_second_ptr = probe(base_anon2);
	int second_load_second_ptr = probe(base_anon2);
	printf("measurements 1st load, 1st adrs - %i\n2nd load, 1st adrs - %i\n1st load, 2nd adrs %i\n2nd load, 2nd ptr %i\n", first_load_first_ptr, second_load_first_ptr, first_load_second_ptr, second_load_second_ptr);
	
	// flush cache
	probe_t2(base_anon1);
	probe_t2(base_anon2); 
	printf("load anon pages from memory/flush cache before\n");

	first_load_first_ptr = probe(base_anon1);
	second_load_first_ptr = probe(base_anon1);
	first_load_second_ptr = probe(base_anon2);
	second_load_second_ptr = probe(base_anon2);
	printf("measurements 1st load, 1st adrs - %i\n2nd load, 1st adrs - %i\n1st load, 2nd adrs %i\n2nd load, 2nd ptr %i\n", first_load_first_ptr, second_load_first_ptr, first_load_second_ptr, second_load_second_ptr);
	
	// modify the second anon mapping, write 0 to the 2nd position
	size_t offset = 2; 
    *((char *)base_anon2 + offset) = 0;
	
	// flush cache
	probe_t2(base_anon1);
	probe_t2(base_anon2); 
	
	printf("load anon pages with one modified (write 0) from memory/flush cache before\n");
	first_load_first_ptr = probe(base_anon1);
	second_load_first_ptr = probe(base_anon1);
	first_load_second_ptr = probe(base_anon2);
	second_load_second_ptr = probe(base_anon2);
	printf("measurements 1st load, 1st adrs - %i\n2nd load, 1st adrs - %i\n1st load, 2nd adrs %i\n2nd load, 2nd ptr %i\n", first_load_first_ptr, second_load_first_ptr, first_load_second_ptr, second_load_second_ptr);
	
	// flush cache
	probe_t2(base_anon3);
	probe_t2(base_anon4);
	*((char *)base_anon3 + offset) = 0;	
	*((char *)base_anon4 + offset) = 0;	
	
	printf("load 2 anon pages modified (write 0 written on same position) from memory/flush cache before modification\n");

	first_load_first_ptr = probe(base_anon3);
	second_load_first_ptr = probe(base_anon3);
	first_load_second_ptr = probe(base_anon4);
	second_load_second_ptr = probe(base_anon4);
	printf("measurements 1st load, 3rd adrs - %i\n2nd load, 3rd adrs - %i\n1st load, 4th adrs %i\n2nd load, 4th adrs %i\n", first_load_first_ptr, second_load_first_ptr, first_load_second_ptr, second_load_second_ptr);

	printf("load 2 anon pages modified (write 0 written on same position) from memory/flush cache before\n");
	probe_t2(base_anon3);
	probe_t2(base_anon4); 
	first_load_first_ptr = probe(base_anon3);
	second_load_first_ptr = probe(base_anon3);
	first_load_second_ptr = probe(base_anon4);
	second_load_second_ptr = probe(base_anon4);
	printf("measurements 1st load, 3rd adrs - %i\n2nd load, 3rd adrs - %i\n1st load, 4th adrs %i\n2nd load, 4th adrs %i\n", first_load_first_ptr, second_load_first_ptr, first_load_second_ptr, second_load_second_ptr);

	
	munmap(base_anon1, 4096);
	munmap(base_anon2, 4096);
	munmap(base_anon3, 4096);
	munmap(base_anon4, 4096);

}

void main(){
	//int res = measure();
	task6();
}
