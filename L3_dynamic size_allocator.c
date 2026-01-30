#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define CANARYNUMBER 0xBAD

typedef struct Block {
  size_t size;
  bool is_free;
  struct Block *next;
  struct Block *prev;
  unsigned int canary;
} Block;

Block *heap_start = NULL;

Block *find_free_block(size_t size) {
  Block *current = heap_start;
  while (current) {
    if (current->is_free && current->size >= size) {
      return current;
    }
    current = current->next;
  }
  // no blocks left
  return NULL;
}

void check_corruption(Block *block) {
  if (block->canary != CANARYNUMBER) {
    printf("Critical Error: Memory corrupted");
  }
}

void *myalloc(size_t size) {
  if (size <= 0)
    return NULL;

  // memory alignment to 16 bytes so that cpu can read in one cycle
  size = (size + 15) & ~15;

  Block *myblock = find_free_block(size);
  if (myblock) {

    // splitting logic
    // check if the exsiting block can hold the requested size+size of another
    // block and the smallest possible value that can be stored
    if (myblock->size >= (size + sizeof(Block) + sizeof(int))) {
      // starting address of new block will be current address+size of its
      // header block(read as character for byte control)+size allocated for
      // that chunk
      Block *new_block = (Block *)((char *)(myblock + 1) + size);
      new_block->size = myblock->size - (size + sizeof(Block));
      new_block->is_free = true;
      new_block->canary = CANARYNUMBER;
      new_block->next = myblock->next;
      if (new_block->next) {
        new_block->next->prev = new_block;
      }
      new_block->prev = myblock;
      myblock->next = new_block;
      myblock->size = size;
    }

    myblock->is_free = false;
    check_corruption(myblock);
    return (void *)(myblock + 1);
  }
  // no block available
  size_t totalsize = sizeof(Block) + size;
  myblock = sbrk(totalsize);
  if (myblock == (void *)-1)
    return NULL;

  myblock->size = size;
  myblock->is_free = false;
  myblock->canary = CANARYNUMBER;
  myblock->next = NULL;

  // Add to the heap
  if (heap_start == NULL) {
    heap_start = myblock;
  } else {
    Block *current = heap_start;
    while (current->next) {
      current = current->next;
    }
    current->next = myblock;
    myblock->prev = current;
  }
  return (void *)(myblock + 1);
}

void myfree(void *ptr) {
  if (!ptr)
    return;
  Block *myblock = (Block *)ptr - 1;
  check_corruption(myblock);
  myblock->is_free = true;

  // merging unused chunks
  // forward merging all that come after
  while (myblock->next && myblock->next->is_free) {
    myblock->size = myblock->size + sizeof(Block) + myblock->next->size;
    myblock->next = myblock->next->next;
    if (myblock->next)
      myblock->next->prev = myblock;
  }
  // backward merging all that come before
  if (myblock->prev && myblock->prev->is_free) {
    myblock->prev->size += sizeof(Block) + myblock->size;
    myblock->prev->next = myblock->next;
    if (myblock->next)
      myblock->next->prev = myblock->prev;
    myblock = myblock->prev;
  }
  // More robust implementation although in this particular scenarion not
  // necessary since it is checked every time free is called
  // while (myblock->prev && myblock->prev->is_free) {
  //  Block *p = myblock->prev;
  //  p->size += sizeof(Block) + myblock->size;
  //  p->next = myblock->next;
  //  if (myblock->next)
  //    myblock->next->prev = p;
  //  myblock = p; // Move the pointer back to continue the loop
  //}
}

int main() {
  printf("--- Level 4 Stress Test ---\n");

  // 1. Create 4 blocks
  void *p1 = myalloc(100);
  void *p2 = myalloc(100);
  void *p3 = myalloc(100);
  void *p4 = myalloc(100);

  // 2. Create fragmentation (Free alternate blocks)
  // [Free] [Busy] [Free] [Busy]
  printf("Freeing p1 and p3...\n");
  myfree(p1);
  myfree(p3);

  // 3. Close the gaps (Free the middle blocks in reverse order)
  // This triggers both forward and backward coalescing logic
  printf("Freeing p4 and p2...\n");
  myfree(p4);
  myfree(p2);

  // 4. The Grand Test
  // If coalescing worked, we should have one giant block of ~472 bytes
  // (100*4 + 24*3 for swallowed headers)
  void *big = myalloc(450);

  if (big == p1) {
    printf("SUCCESS: Coalescing fused 4 blocks into 1. Address match at %p\n",
           big);
  } else {
    printf(
        "FAILURE: Allocator requested new memory at %p instead of reusing %p\n",
        big, p1);
  }

  return 0;
}
