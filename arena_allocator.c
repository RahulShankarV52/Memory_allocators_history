#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

typedef struct {
  uint8_t *buffer;
  size_t capacity; // Total bytes mapped from OS
  size_t offset;   // Current "Bump" pointer
} Arena;

// Define a Marker for Stack-style rollback
typedef size_t Marker;

// 1. Standalone Initialization (Talking to Kernel)
Arena arena_create(size_t size) {
  Arena a = {0};

  // Page-align the capacity (usually 4096 bytes)
  size_t page_size = sysconf(_SC_PAGESIZE);
  a.capacity = (size + page_size - 1) & ~(page_size - 1);

  // mmap: The "Pro" way to get memory without malloc
  // MAP_ANONYMOUS means it's not backed by a file, just RAM.
  a.buffer = mmap(NULL, a.capacity, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (a.buffer == MAP_FAILED) {
    perror("mmap failed");
    a.buffer = NULL;
  }
  return a;
}

// 2. High-Speed Allocation (O(1))
void *arena_alloc(Arena *a, size_t size) {
  // Maintain 16-byte alignment for modern CPUs
  size = (size + 15) & ~15;

  if (a->offset + size > a->capacity) {
    return NULL; // Out of memory
  }

  void *ptr = &a->buffer[a->offset];
  a->offset += size;
  return ptr;
}

// 3. Stack Functionality: Marker Logic
Marker arena_get_marker(Arena *a) { return a->offset; }

void arena_pop_to_marker(Arena *a, Marker m) {
  if (m <= a->offset) {
    a->offset = m;
  }
}

// 4. Arena Functionality: Reset
void arena_reset(Arena *a) { a->offset = 0; }

// 5. Cleanup (Returning memory to the Kernel)
void arena_destroy(Arena *a) {
  if (a->buffer) {
    munmap(a->buffer, a->capacity);
  }
}

int main() {
  Arena my_arena = arena_create(64 * 1024);

  Marker checkpoint = arena_get_marker(&my_arena);

  int *nums = arena_alloc(&my_arena, sizeof(int) * 5);
  printf("Allocated 5 ints. Offset now at: %zu\n", my_arena.offset);

  arena_pop_to_marker(&my_arena, checkpoint);
  printf("Popped to marker. Offset now at: %zu\n", my_arena.offset);

  char *str = arena_alloc(&my_arena, 100);
  arena_reset(&my_arena); // Wipe everything
  printf("Arena reset. Offset now at: %zu\n", my_arena.offset);

  arena_destroy(&my_arena);
  return 0;
}
