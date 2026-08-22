#include "mem/usage.h"

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <stdio.h>
#include <string.h>
#endif

void print_rss(const char *label) {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        fprintf(stderr, "[%s] VmRSS: %zu kB\n", label, (size_t)(pmc.WorkingSetSize / 1024));
    }
#elif __APPLE__
    // TODO
#else
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, "VmRSS:", 6) == 0) {
            fprintf(stderr, "[%s] %s", label, line);
            break;
        }
    }
    fclose(f);
#endif
}

