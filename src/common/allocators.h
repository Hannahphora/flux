#ifndef INCLUDE_ALLOCATORS_H
#define INCLUDE_ALLOCATORS_H

// allocator backends
// - libc malloc
// - linux mmap
// - win32 virtualalloc

#define ALLOCATOR_BACKEND_LIBC_MALLOC               0
#define ALLOCATOR_BACKEND_LINUX_MMAP                1
#define ALLOCATOR_BACKEND_WIN32_VIRUALALLOC         2
#define ALLOCATOR_BACKEND_WASM_HEAPBASE             3

// default backend to libc malloc
#ifndef ALLOCATOR_BACKEND
#define ALLOCATOR_BACKEND ALLOCATOR_BACKEND_LIBC_MALLOC
#endif // ALLOCATOR_BACKEND

// alloc(allocator *alloc, ):
//      - reserves a subpartition of memory
// realloc():
//      - grows or shinks a previously reserved subpartition
//      - makes new alloc of requested size, memcpy from prev alloc to new alloc, free prev alloc 
//      - no-op on arena allocator
// free():
//      - frees an allocation
//      - no-op on arena allocator
// aquire():
//      - aquires memory from the backend
// release():
//      - frees memory aquired from the backend

// arena/linear/scratch/bump/monotonic allocator (theres alot of names)

// LIFO/stack allocator

// FIFO allocator

// block/pool allocator

#endif // INCLUDE_ALLOCATORS_H