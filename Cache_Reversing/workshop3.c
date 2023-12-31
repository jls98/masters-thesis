#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

// task 1 constants
#define FEEDBACK 0x80000000000019E2ULL
#define ACCELERATOR 1000000000

//#define ACCESS_AMOUNT 34359738368 // 2^35
//#define ACCESS_AMOUNT 33554432 // 2^25
#define ACCESS_AMOUNT 2048576 // 2^20

#define ADRS_AMOUNT18 262144 // 2^18
#define ADRS_AMOUNT17 131072 // 2^17
#define ADRS_AMOUNT16 65536 // 2^16
#define ADRS_AMOUNT15 32768 // 2^15
#define ADRS_AMOUNT14 16384 // 2^14 
#define ADRS_AMOUNT13 8192 // 2^13  
#define ADRS_AMOUNT12 4096 // 2^12 
#define ADRS_AMOUNT11 2048 // 2^11 
#define ADRS_AMOUNT10 1024 // 2^10 
#define ADRS_AMOUNT9 512 // 2^9 
#define ADRS_AMOUNT8 256 // 2^8 


uint64_t step(uint64_t lfsr) {
  lfsr = (lfsr & 1) ? (lfsr >> 1) ^ FEEDBACK : (lfsr >> 1);
  return lfsr;
}

uint64_t load(void *p, uint64_t *val)
{
	volatile unsigned long long time;  
	__asm__ volatile (
        " mfence            \n"
        " lfence            \n"
        " rdtsc             \n"
        " mov r8, rax 		\n"
        " mov %1, [%2]		\n"
        " lfence            \n"
        " rdtsc             \n"
        " sub rax, r8 		\n"
        : "=&a" (time), "=r" (*val)
        : "r" (p)
        : "rdx", "r8");
		
		return time;
}

uint64_t flush_load(void *p, void *p1, uint64_t *val)
{
	volatile unsigned long long time;  
	__asm__ volatile (
        " mfence            \n"
        " lfence            \n"
        " rdtsc             \n"
        " mov r8, rax 		\n"
        " mov %1, [%2]		\n"
        " lfence            \n"
        " rdtsc             \n"
        " sub rax, r8 		\n"
		" clflush [%3]		\n"
        : "=&a" (time), "=r" (*val)
        : "r" (p), "r" (p1)
        : "rdx", "r8");
		
		return time;
}

static __inline__ uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ volatile ("rdtscp" : "=a" (lo), "=d" (hi) : : "ecx");
    return ((uint64_t)hi<<32)|lo;
}

void *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    //struct stat st_buf;
    //if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    //size_t map_len = st_buf.st_size;
	//void *mapping = mmap(NULL, map_len, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
	//void *mapping = malloc(4096 * 4096*4);
	void *mapping = mmap(NULL, 4096 * 4096, PROT_READ, MAP_PRIVATE | MAP_ANON | MAP_HUGETLB, -1, 0); 
	if (mapping == MAP_FAILED){
		printf("mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
}

double detection_intern(uint64_t adrs_amount)
{
	// mmap zero file and cast to uint64_t array
	void *p = map("./zero_file", 0);
	
	// generate random access order
	uint64_t lfsr;
	uint64_t tmp_time=1;
	uint64_t *tmp_val = (uint64_t *)malloc(sizeof(uint64_t));
	__asm__ volatile("rdrand %0": "=r" (lfsr)::"flags");
	lfsr = step(lfsr);
	double avg_acs_time = 1.0;
	int misses = 0;
	
	for(uint64_t c=0; c<ACCESS_AMOUNT; c++)
	{
		tmp_time = load(p+(sizeof(uint64_t)*(lfsr%(adrs_amount))), tmp_val); // load p+8 byte * 
		lfsr ^= *tmp_val; // = lfsr since values are all 0
		lfsr = step(lfsr);
		avg_acs_time = (c==0) ? (double) tmp_time : (avg_acs_time*c + (double) tmp_time)/(c+1);	
		if (tmp_time > 40) misses++; 
		
	}
	//printf("bytes amount %lu, values tmp_time %lu, tmp_val %lu\n", adrs_amount*8, tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	//printf("average access time was %f\namount misses %i\n", avg_acs_time, misses);
	//printf("%7lu: avg access time %f\n", adrs_amount, avg_acs_time);
	free(tmp_val);
	return avg_acs_time;
}

// task 1
// To guarantee this we suggest incorporating dependency between each of these operations, e.g. each random generation takes a combination of the previous random value and the value accessed from the random address as input. 
void L1_detection(uint64_t adrs_amount){
	
	double avg_acs_time;
	do{
		// activate cache
		uint64_t c = rdtsc();
		while(rdtsc()-c < ACCELERATOR);
		
		// go
		avg_acs_time = detection_intern(adrs_amount);
	}while(avg_acs_time > 2000.0); // avoid odd measurements and repeat
	printf("%9lu: avg access time %f\n", adrs_amount*8, avg_acs_time);
}



// task 2
//#define DOUBLE_CACHESIZE 16384 // (in uint64_t)

// memory line can be stored in the cache lines in the way of a certain set 


/*Recall that the cache is organised into S cache sets that each contain W no. of ways. 
Memory lines each map to a certain cache set and can be stored in any of the ways within 
the set that they map to. Multiple lines map to the same set. Therefore the total cache 
size (in no. of lines) = S x W. From Task 1 we know the total cache size and now we want 
to find these other two parameters, when we find one of them we also find the other. 

 

We begin with a set of memory addresses whose size is double the cache size. From the 
last task we know that if we access every element, the average access time is high 
(roughly halfway between access times for a cache-hit vs a cache-miss). If we then 
limit the access to only every second element within the same overall set, then the 
same timing should be observed. If we continue doubling our guessed stride length (i.e. 
taking every 2n th element), when we reach a guess equal to double the cache stride, 
the entire access set will fit in the cache and we will only have cache hits.


Your task is to use access times observed in this manner to determine the cache 
stride and therefore also the no. of ways.*/

void set_n_ways_detection(uint64_t n, uint64_t double_cache_size)
{

	// start caching
	uint64_t c = rdtsc();
	while(rdtsc()-c < ACCELERATOR);
	
	// mmap zero file and cast to uint64_t array (file name is not used anymore)
	void *p = map("./zero_file", 0);
	
	// generate random access order
	uint64_t lfsr;
	uint64_t tmp_time=1;
	uint64_t *tmp_val = (uint64_t *)malloc(sizeof(uint64_t));
	__asm__ volatile("rdrand %0": "=r" (lfsr)::"flags");
	lfsr = step(lfsr);
	double avg_acs_time = 1.0;
	int misses = 0;
	
	for(c=0; c<ACCESS_AMOUNT; c++)
	{
		tmp_time = load(p+(sizeof(uint64_t)*(lfsr%(double_cache_size))), tmp_val);		
	}
	
	int counter=0;
	int sum=0;
	for(c=0; c<ACCESS_AMOUNT; c+=(1<<n))
	{
		
		tmp_time = load(p+(sizeof(uint64_t)*(lfsr%(double_cache_size))), tmp_val);
		sum+=tmp_time;
		lfsr ^= *tmp_val; // = lfsr since values are all 0
		lfsr = step(lfsr);
		avg_acs_time = (c==0) ? (double) tmp_time : (avg_acs_time*c + (double) tmp_time)/(c+1);
		counter++;
		if (tmp_time > 40) misses++; 
		
	}
	printf("values tmp_time (last measured timestamp) %lu, amount of sets %u, cache ways if pcore %i, cache ways if ecore %i\n", tmp_time, (1<<n)/64, 32768/(1<<n), 49152/(1<<n)); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\namount misses %i, counter %i, sum %i, real mean %f\n", avg_acs_time, misses, counter, sum, ((double) sum)/((double) counter) );
	
	free(tmp_val);
}

void set_n_ways_detection2(uint64_t n, uint64_t double_cache_size, int cache_size)
{

	// start caching
	uint64_t c = rdtsc();
	while(rdtsc()-c < ACCELERATOR);
	
	// mmap zero file and cast to uint64_t array (file name is not used anymore)
	void *p = map("./zero_file", 0);
	
	// generate random access order
	uint64_t lfsr;
	uint64_t tmp_time=1;
	uint64_t *tmp_val = (uint64_t *)malloc(sizeof(uint64_t));
	__asm__ volatile("rdrand %0": "=r" (lfsr)::"flags");
	lfsr = step(lfsr);
	double avg_acs_time = 1.0;
	int misses = 0;
	
	for(c=0; c<ACCESS_AMOUNT; c++)
	{
		tmp_time = load(p+(sizeof(uint64_t)*(lfsr%(double_cache_size))), tmp_val);		
	}
	int counter=0;
	for(c=0; c<ACCESS_AMOUNT; c+=n)
	{
		for(int i=0;i<cache_size;i++) load(p+(sizeof(uint64_t)*(lfsr%(cache_size))), tmp_val); // load something in L1 to possibly evict from L1
		tmp_time = load(p+(sizeof(uint64_t)*(lfsr%(double_cache_size))), tmp_val);
		lfsr ^= *tmp_val; // = lfsr since values are all 0
		lfsr = step(lfsr);
		avg_acs_time = (counter==0) ? (double) tmp_time : (avg_acs_time*c + (double) tmp_time)/(counter+1);	
		counter++;
		if (tmp_time > 40) misses++; 
		
	}
	printf("values tmp_time (last measured timestamp) %lu, amount of ways %u, cache ways if pcore %i, cache ways if ecore %i\n", tmp_time, 1<<n, 32768/(1<<n), 49152/(1<<n)); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\namount misses %i\n", avg_acs_time, misses);
	
	free(tmp_val);
}

// task 3
void L1_line_detection(){
	// pointer byte für byte erhöhen und access time messen

	// start caching
	uint64_t c = rdtsc();
	while(rdtsc()-c < ACCELERATOR);
	
	// mmap zero file and cast to uint64_t array
	char *p = (char *)map("./zero_file", 0);
	
	uint64_t *tmp_val = (uint64_t *)malloc(sizeof(uint64_t));
	for (int i=0;i<ACCESS_AMOUNT;i++) load(p, tmp_val);
	
	// prepare lfsr
	uint64_t lfsr;
	__asm__ volatile("rdrand %0": "=r" (lfsr)::"flags");
	lfsr = step(lfsr);
	
	// first char
	int offset=0;
	printf("offset %u\n", offset);
	uint64_t tmp_time=1;
	double avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
	
	// second char
	offset=1;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);

		// third char
	offset=2;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);

		// 4th char
	offset=3;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);

		// 5th char
	offset=4;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
		
		// 6th char
	offset=5;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
		
		
		// 7th char
	offset=6;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
		
		
		// 8th char
	offset=7;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
		
		// 9th char
	offset=8;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);		
		// 10th char
	offset=9;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);		
		// 11th char
	offset=10;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
	
			// 12th char
	offset=11;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);
	
			// 17th char
	offset=16;
	printf("offset %u\n", offset);
	tmp_time=1;
	avg_acs_time = 1.0;
	for (int i=0;i<ACCESS_AMOUNT;i++){
		load(p, tmp_val);
		tmp_time = flush_load(p+lfsr%(offset+1), p, tmp_val);
		lfsr ^= *tmp_val;
		lfsr = step(lfsr);
		avg_acs_time = (i==0) ? (double) tmp_time : (avg_acs_time*i + (double) tmp_time)/(i+1);			
	} 
	printf("values tmp_time %lu, tmp_val %lu\n", tmp_time, *tmp_val); // reading 8 '0's brings odd values (0x3030303030303030)
	printf("average access time was %f\n", avg_acs_time);

	free(tmp_val);
	
}


int main(){
	/*printf("check L1d and L2 sizes on the e core\n\n");
	// check for time dimensions
	for (int i=4;i<20;i++) L1_detection(1<<i); 
	
	printf("\nnow more fine tuned for L1d:\n\n");
	// theres a jump from 32kb to 64 kb for L1d on the e core
	for (int i=1;i<12;i++) L1_detection(ADRS_AMOUNT10*i);

	printf("\nnow more fine tuned for L2:\n\n");
	// theres a jump from 500kb to 1MB to 2MB for L2 on the e core
	for (int i=1;i<20;i++) L1_detection(ADRS_AMOUNT14*i);
	*/
	//L1_detection(ADRS_AMOUNT8);
	//L1_detection(ADRS_AMOUNT9);
	//L1_detection(ADRS_AMOUNT10);
	//L1_detection(ADRS_AMOUNT11*i);
	/*L1_detection(ADRS_AMOUNT12);	
	L1_detection(ADRS_AMOUNT13);
	L1_detection(ADRS_AMOUNT14); // 
	L1_detection(ADRS_AMOUNT15); // 
	L1_detection(ADRS_AMOUNT16); // 
	L1_detection(ADRS_AMOUNT17); // 
	L1_detection(ADRS_AMOUNT18); // 
	*/
	
	printf("detect ways on L1\n\n");
	//set_n_ways_detection(0);
	//set_n_ways_detection(1);
	//set_n_ways_detection(2);
	//set_n_ways_detection(3);
	//set_n_ways_detection(4);
	//set_n_ways_detection(5);
	//set_n_ways_detection(6);
	uint64_t double_cache_size = 131072;
	
    set_n_ways_detection(7, double_cache_size);
    set_n_ways_detection(8, double_cache_size);
    set_n_ways_detection(9, double_cache_size);
    set_n_ways_detection(10, double_cache_size);
    set_n_ways_detection(11, double_cache_size);
    set_n_ways_detection(12, double_cache_size);
    set_n_ways_detection(13, double_cache_size);
    set_n_ways_detection(14, double_cache_size);
    set_n_ways_detection(15, double_cache_size);

	
	
	
	L1_line_detection();
	return 0;
}

















