#include <stdio.h>
#include <stdlib.h>

void generateZeroFile(const char *file_name, size_t file_size) {
    FILE *file = fopen(file_name, "wb");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Fill the file with '0' bytes
    for (size_t i = 0; i < file_size; i++) {
        fputc('0', file);
    }

    fclose(file);
}

int main() {
    const char *file_name = "zero_file";
    size_t file_size = 1074000000;  // Size in bytes

    generateZeroFile(file_name, file_size);

    printf("File '%s' with %zu '0' bytes has been generated.\n", file_name, file_size);

    return 0;
}
