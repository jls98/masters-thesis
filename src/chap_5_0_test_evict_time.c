#include "chap_3_3_find_evset.c"
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <x86intrin.h>
#include <errno.h>


void *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    struct stat st_buf;
    if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    size_t map_len = st_buf.st_size;

	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
	if (mapping == MAP_FAILED){
		printf("[!] map: mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
}

#define TEST_REPS 1000
#define TEST_CYCLES 777777

// Works very unreliable with loop (1 detection on 1000 reps).
void test_victim(){
    void *my_victim = map("./build/victim", 0x1136);
    printf("my victim is located at %p, hold value %lx\n", my_victim, *((uint64_t *) my_victim));
    unsigned long long old_tsc, tsc;
    __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
    for(int i=0;i<TEST_REPS;i++){
        old_tsc=tsc;
        __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
        while (tsc - old_tsc < TEST_CYCLES) __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
        probe_evset_single(my_victim);  
        probe_evset_single(my_victim);  
        probe_evset_single(my_victim);  
    }
}

//
void test_victim_syscall(){
    unsigned long long old_tsc, tsc;
    __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
    for(int i=0;i<TEST_REPS;i++){
        old_tsc=tsc;
        __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
        while (tsc - old_tsc < TEST_CYCLES) __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
        system("./build/victim");
    }
}


int main(){
    // victim that loads data 
    test_victim_syscall();    
}

// #include "chap_3_3_find_evset.c"
// #include <sys/stat.h>
// #include <stdlib.h>
// #include <sys/mman.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <stdint.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <x86intrin.h>
// #include <errno.h>


// void *map(char *file_name, uint64_t offset)
// {
//     int file_descriptor = open(file_name, O_RDONLY); 
//     if (file_descriptor == -1) return NULL;
//     struct stat st_buf;
//     if (fstat(file_descriptor, &st_buf) == -1) return NULL;
//     size_t map_len = st_buf.st_size;

// 	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
// 	if (mapping == MAP_FAILED){
// 		printf("[!] map: mmap fail with errno %d\n", errno); // fix problems with mmap
// 		return NULL;
// 	}
// 	close(file_descriptor);
// 	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
// }


// int main(){
//     // victim that loads data 
//     void *my_victim = map("./build/victim", 0x1136);
//     printf("my victim is located at %p\n", my_victim);
//     probe_evset_single(my_victim);
// }