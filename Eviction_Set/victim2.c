#include "evset-timings.c"
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
    printf("debug size map_len %d\n", map_len);
	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
	if (mapping == MAP_FAILED){
		printf("[!] map: mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
}


int main(){
    // victim that loads data 
    void *my_victim = map("./build/victim", 0x1134);
    printf("my victim is located at %p\n", my_victim);
    probe_chase_loop(my_victim, 1);
    probe_chase_loop(my_victim, 1);
}