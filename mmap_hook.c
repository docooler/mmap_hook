#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

// Define pointers to the original mmap and munmap functions
static void* (*real_mmap)(void*, size_t, int, int, int, off_t) = NULL;
static int (*real_munmap)(void*, size_t) = NULL;

// A simple structure to keep track of allocated memory regions
typedef struct MallocRegion {
    void* addr;
    size_t length;
    struct MallocRegion* next;
} MallocRegion;

// Head of the linked list to store malloc'd regions
static MallocRegion* malloc_regions = NULL;
static pthread_mutex_t malloc_regions_mutex = PTHREAD_MUTEX_INITIALIZER;

// Add a malloc region to the list
void add_malloc_region(void* addr, size_t length) {
    pthread_mutex_lock(&malloc_regions_mutex);
    MallocRegion* region = malloc(sizeof(MallocRegion));
    region->addr = addr;
    region->length = length;
    region->next = malloc_regions;
    malloc_regions = region;
    pthread_mutex_unlock(&malloc_regions_mutex);
}

// Remove a malloc region from the list
int remove_malloc_region(void* addr) {
    pthread_mutex_lock(&malloc_regions_mutex);
    MallocRegion** current = &malloc_regions;
    while (*current) {
        if ((*current)->addr == addr) {
            MallocRegion* to_free = *current;
            *current = (*current)->next;
            free(to_free);
            pthread_mutex_unlock(&malloc_regions_mutex);
            return 1;
        }
        current = &(*current)->next;
    }
    pthread_mutex_unlock(&malloc_regions_mutex);
    return 0;
}

// Our custom mmap function
void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
    // Initialize the original mmap function pointer
    if (!real_mmap) {
        real_mmap = dlsym(RTLD_NEXT, "mmap");
        if (!real_mmap) {
            fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }
    }

    // Check the offset
    if (offset == 0xf0000) {
        printf("Intercepted mmap call with specific offset, using malloc instead.\n");

        // Use malloc to allocate memory
        void* result = malloc(length);
        if (!result) {
            fprintf(stderr, "Error in `malloc`: unable to allocate memory\n");
            exit(EXIT_FAILURE);
        }

        // Zero out the allocated memory
        memset(result, 0, length);

        // Add to our list of malloc'd regions
        add_malloc_region(result, length);

        return result;
    }

    // Otherwise, call the original mmap function
    return real_mmap(addr, length, prot, flags, fd, offset);
}

// Our custom munmap function
int munmap(void* addr, size_t length) {
    // Initialize the original munmap function pointer
    if (!real_munmap) {
        real_munmap = dlsym(RTLD_NEXT, "munmap");
        if (!real_munmap) {
            fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }
    }

    // Check if the address is in our malloc'd regions
    if (remove_malloc_region(addr)) {
        printf("Intercepted munmap call for malloc'd region, using free instead.\n");
        free(addr);
        return 0;
    }

    // Otherwise, call the original munmap function
    return real_munmap(addr, length);
}