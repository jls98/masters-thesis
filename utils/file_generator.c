#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void generateZeroFile(const char *file_name, size_t file_size) {
    FILE *file = fopen(file_name, "wb");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    // Fill the file with '0' bytes
    char *buf=malloc(sizeof(char));
    int randint;
    for (size_t i = 0; i < file_size; i++) {
        randint = rand() % 256;
        sprintf(buf, "%c", randint);
        fputc(*buf, file);
    }
    free(buf);
    fclose(file);
}

int main(int ac, char**av) {
	
    const char *file_name = "zero_file";
    size_t file_size = ac==2 ? atoi(av[1]) : 1074000000;  // Size in bytes

    generateZeroFile(file_name, file_size);

    printf("File '%s' with %zu rand bytes has been generated.\n", file_name, file_size);

    return 0;
}
