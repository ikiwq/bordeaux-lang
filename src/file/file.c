#include "file/file.h"

#include <stdio.h>
#include <stdlib.h>

static void file_err(const char* path) {
    fprintf(stderr, "Could not open file %s\".\n", path);
    exit(74);
}

char* read_file(arena_t *arena, const char *path) {
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        file_err(path);
    }

    fseek(file, 0L, SEEK_END);
    const size_t file_size = ftell(file);
    rewind(file);

    char *buffer = arena_alloc(arena, file_size + 1);
    if(buffer == nullptr) {
        file_err(path);
    }

    const size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        file_err(path);
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}
