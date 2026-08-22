#ifndef NUM_H
#define NUM_H

#include <math.h>
#include <stdlib.h>

// Expects an unsigned integer
#define uint_len(n) \
    ((n) == 0 ? 1 : floor(log10(n)) + 1)

#endif // NUM_H
