// TEMP
#define ARENA_ALLOCATOR_IMPLEMENTATION
#define LIFO_ALLOCATOR_IMPLEMENTATION
#define FIFO_ALLOCATOR_IMPLEMENTATION
#define POOL_ALLOCATOR_IMPLEMENTATION
#define FLUX_ALLOCATOR_BACKEND FLUX_ALLOCATOR_BACKEND_WIN32_VIRUALALLOC
// TEMP END

/*
    NOTES:
        - all allocator backings must be at least a page size big (page size assumed to be 4kb)

    void *flux_alloc(void *allocator, size_t size_bytes):
        - reserves a subpartition of memory of size_bytes
        - returns NULL ptr if failed, otherwise returns pointer to reserved region
        - returned region is always initialised to zero
    void *flux_realloc(void *allocator, void *ptr, size_t size_bytes):
        - grows or shinks a previously reserved subpartition
        - reserves new region of size, memcpys from ptr to new region, frees ptr 
        - additional space at end of new region is always initialised to zero
        - no-op for arena allocator
    bool flux_free(void *allocator, void *ptr):
        - frees the region pointed to by ptr
        - no-op for arena allocator

    bool flux_allocator_backing_alloc(Flux_Allocator_Backing *backing, size_t size_bytes):
        - aquires memory from the backend to serve as backing
        - usually called on system initialisation
    bool flux_allocator_backing_realloc(Flux_Allocator_Backing *backing, size_t size_bytes):
        - reallocs an existing backing to new size
        - usually called if allocator runs out of backing memory at runtime
        - try to minimise calling this as much as possible, rather do all
          backing allocations during initialisation
    bool flux_allocator_backing_free(Flux_Allocator_Backing *backing):
        - frees backing memory aquired from the backend
        - usually called on system deinitialisation/cleanup
*/

#ifndef INCLUDE_ALLOCATORS_H
#define INCLUDE_ALLOCATORS_H

#define FLUX_ALLOCATOR_BACKEND_LIBC_MALLOC                  0
#define FLUX_ALLOCATOR_BACKEND_LINUX_MMAP                   1
#define FLUX_ALLOCATOR_BACKEND_WIN32_VIRUALALLOC            2

#ifndef FLUX_ALLOCATOR_BACKEND
    // default backend to libc malloc
    #define FLUX_ALLOCATOR_BACKEND FLUX_ALLOCATOR_BACKEND_LIBC_MALLOC
#endif // FLUX_ALLOCATOR_BACKEND

typedef struct {
    void *ptr;
    // size in bytes
    size_t size;
} Flux_Allocator_Backing;

#if FLUX_ALLOCATOR_BACKEND == FLUX_ALLOCATOR_BACKEND_LIBC_MALLOC
    #include <stdlib.h>

    bool flux_allocator_backing_alloc(Flux_Allocator_Backing *backing, size_t size_bytes)
    {
        backing->ptr = malloc(size_bytes);
        if (backing->ptr == NULL) {
            // TODO: log malloc() error
            return false;
        }
        backing->size = size_bytes;
        return true;
    }

    bool flux_allocator_backing_realloc(Flux_Allocator_Backing *backing, size_t size_bytes)
    {
        void *tmp = realloc(backing->ptr, size_bytes);
        if (tmp == NULL) {
            // TODO: log realloc() error
            return false;
        }
        backing->ptr = tmp;
        backing->size = size_bytes;
        return true;
    }

    bool flux_allocator_backing_free(Flux_Allocator_Backing *backing)
    {
        free(backing->ptr);
        backing->ptr = NULL;
        backing->size = 0;
        return true;
    }

#elif FLUX_ALLOCATOR_BACKEND == FLUX_ALLOCATOR_BACKEND_LINUX_MMAP
    #include <unistd.h>
    #include <sys/mman.h>

    bool flux_allocator_backing_alloc(Flux_Allocator_Backing *backing, size_t size_bytes)
    {
        backing->ptr = mmap(
            NULL,                           // let kernel choose page aligned location
            size_bytes,                     // size of the allocation in bytes
            PROT_READ | PROT_WRITE,         // memory protection level
            MAP_ANONYMOUS | MAP_PRIVATE,    // flags: not backed by file and process local
            -1,                             // file descriptor: N/A
            0                               // file offset: N/A
        );
        if (backing->ptr == MAP_FAILED) {
            // TODO: log mmap() error and errno
            return false;
        }
        backing->size = size_bytes;
        return true;
    }

    bool flux_allocator_backing_realloc(Flux_Allocator_Backing *backing, size_t size_bytes)
    {
        void *tmp = mremap(
            backing->ptr,       // ptr to old mapping
            backing->size,      // size of old mapping
            size_bytes          // size of new mapping
            MREMAP_MAYMOVE,     // flags: allow original mapping to be moved
        );
        if (tmp == MAP_FAILED) {
            // TODO: log mremap() error and errno
            return false;
        }
        backing->ptr = tmp;
        backing->size = size_bytes;
        return true;
    }

    bool flux_allocator_backing_free(Flux_Allocator_Backing *backing)
    {
        if (!munmap(backing->ptr, backing->size)) {
            // TODO: log munmap() error and errno
            return false;
        }
        backing->ptr = NULL;
        backing->size = 0;
        return true;
    }

#elif FLUX_ALLOCATOR_BACKEND == FLUX_ALLOCATOR_BACKEND_WIN32_VIRUALALLOC
    #ifndef _WIN32
        #error "Current platform is not Windows"
    #endif // _WIN32

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #define INV_HANDLE(x) (((x) == NULL) || ((x) == INVALID_HANDLE_VALUE))

    bool flux_allocator_backing_alloc(Flux_Allocator_Backing *backing, size_t size_bytes)
    {
        backing->ptr = VirtualAllocEx(
            GetCurrentProcess(),            // allocate in current process address space
            NULL,                           // unknown position
            size_bytes,                     // bytes to allocate
            MEM_COMMIT | MEM_RESERVE,       // reserve and commit allocated page 
            PAGE_READWRITE                  // permissions: read/write
        );
        if (INV_HANDLE(backing->ptr)) {
            // TODO: log VirtualAllocEx() error
            return false;
        }
        backing->size = size_bytes;
        return true;
    }

    bool flux_allocator_backing_realloc(Flux_Allocator_Backing *backing, size_t size_bytes)
    {
        void *tmp = VirtualAllocEx(
            GetCurrentProcess(),            // allocate in current process address space
            NULL,                           // unknown position
            size_bytes,                     // bytes to allocate
            MEM_COMMIT | MEM_RESERVE,       // reserve and commit allocated page 
            PAGE_READWRITE                  // permissions: read/write
        );
        if (INV_HANDLE(tmp)) {
            // TODO: log VirtualAllocEx() error
            return false;
        }

        // TODO: copy memory from old backing to tmp

        if (!flux_allocator_backing_free(backing))
            return false;

        backing->ptr = tmp;
        backing->size = size_bytes;
        return true;
    }

    bool flux_allocator_backing_free(Flux_Allocator_Backing *backing)
    {
        BOOL free_result = VirtualFreeEx(
            GetCurrentProcess(),            // deallocate from current process address space
            (LPVOID)backing->ptr,           // address to deallocate
            0,                              // bytes to deallocate (unknown, deallocate entire page)
            MEM_RELEASE                     // release the page (and implicitly decommit it)
        );
        if (free_result == FALSE) {
            // TODO: log VirtualFreeEx() error
            return false;
        }
        backing->ptr = NULL;
        backing->size = 0;
        return true;
    }

#else
    #error "Unknown allocator backend"
#endif // FLUX_ALLOCATOR_BACKEND

#ifdef FLUX_ARENA_ALLOCATOR_IMPLEMENTATION

    // arena/linear/scratch/bump/monotonic allocator (theres alot of names)

    typedef struct {
        Flux_Allocator_Backing backing;

    } Flux_Arena_Allocator;

    void *flux_alloc(void *allocator, size_t size_bytes)
    {

    }

#endif // FLUX_ARENA_ALLOCATOR_IMPLEMENTATION

#ifdef FLUX_LIFO_ALLOCATOR_IMPLEMENTATION

    // LIFO/stack allocator

    typedef struct {
        Flux_Allocator_Backing backing;

    } Flux_Lifo_Allocator;




#endif // FLUX_LIFO_ALLOCATOR_IMPLEMENTATION

#ifdef FLUX_FIFO_ALLOCATOR_IMPLEMENTATION

    // FIFO allocator

    typedef struct {
        Flux_Allocator_Backing backing;

    } Flux_Fifo_Allocator;



#endif // FLUX_FIFO_ALLOCATOR_IMPLEMENTATION

#ifdef FLUX_POOL_ALLOCATOR_IMPLEMENTATION

    // block/pool allocator

    typedef struct {
        Flux_Allocator_Backing backing;
        
    } Flux_Pool_Allocator;





#endif // FLUX_POOL_ALLOCATOR_IMPLEMENTATION

#endif // INCLUDE_ALLOCATORS_H

#ifndef ALLOCATORS_H_USE_FLUX_PREFIX_GUARD
#define ALLOCATORS_H_USE_FLUX_PREFIX_GUARD
    #ifndef ALLOCATORS_H_USE_FLUX_PREFIX



    #endif // ALLOCATORS_H_USE_FLUX_PREFIX
#endif // ALLOCATORS_H_USE_FLUX_PREFIX_GUARD