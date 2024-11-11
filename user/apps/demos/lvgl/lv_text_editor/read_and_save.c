#include "read_and_save.h"
#include <stdio.h>

static FILE* file_ptr = NULL;
extern const char* filename;

int open_file() {
    file_ptr = fopen(filename, "r+");
    if (file_ptr == NULL) {
        printf("Failed to open file.\n");
        return -1;
    }
    return 0;
}

char* read_file() {
    if (file_ptr == NULL) {
        printf("File not opened.\n");
        return NULL;
    }
    
    fseek(file_ptr, 0, SEEK_END);
    long file_size = ftell(file_ptr);
    rewind(file_ptr);
    
    char* buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        printf("Failed to allocate memory.\n");
        return NULL;
    }
    
    fread(buffer, sizeof(char), file_size, file_ptr);
    buffer[file_size] = '\0';
    
    return buffer;
}

int save_file(char* buffer) {
    if (file_ptr == NULL) {
        printf("File not opened.\n");
        return -1;
    }
    
    fseek(file_ptr, 0, SEEK_SET);
    fwrite(buffer, sizeof(char), strlen(buffer), file_ptr);
    return 0;
}
