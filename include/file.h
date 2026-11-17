#ifndef BORDEAUX_FILE_H
#define BORDEAUX_FILE_H

#include <stdio.h>

FILE *open_file(const char *path, const char *modes);

char *read_file(const char *path);

#endif // BORDEAUX_FILE_H
