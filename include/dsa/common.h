#ifndef DSA_COMMON_H
#define DSA_COMMON_H

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define ARRAY_SIZE(arr) \
    (sizeof(arr) / sizeof((arr)[0]))

#endif // DSA_COMMON_H
