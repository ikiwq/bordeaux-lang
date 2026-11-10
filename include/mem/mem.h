#ifndef BORDEAUX_MEM_H
#define BORDEAUX_MEM_H

#define CAPACITY_GROW(capacity) \
    ((capacity) == 0 ? (8) : (capacity) * 2)

#endif // BORDEAUX_MEM_H
