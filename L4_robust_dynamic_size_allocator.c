#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CANARYNUMBER 0xBAD
#define NUM_BINS 8
#define PAGE_SIZE 4096

typedef struct {
  size_t size;
  bool is_free;
} Footer;

typedef struct Block {
  size_t size;
  bool is_free;
  struct Block *next; // Next in BIN list
  struct Block *prev; // Prev in BIN list
  unsigned int canary;
} Block;

Block *bins[NUM_BINS] = {NULL};

int get_bin_index(size_t size) {
  if (size <= 32)
    return 0;
  if (size <= 64)
    return 1;
  if (size <= 128)
    return 2;
  if (size <= 256)
    return 3;
  if (size <= 512)
    return 4;
  if (size <= 1024)
    return 5;
  if (size <= 2048)
    return 6;
  return 7;
}

void update_footer(Block *block) {
  Footer *f = (Footer *)((char *)(block + 1) + block->size);
  f->size = block->size;
  f->is_free = block->is_free;
}

void check_corruption(Block *block) {
  if (block->canary != CANARYNUMBER) {
    printf("CRITICAL ERROR: Heap corruption detected at %p!\n", (void *)block);
    // In a production app, you would call abort() here.
  }
}

void add_to_bin(Block *block) {
  int idx = get_bin_index(block->size);
  block->next = bins[idx];
  block->prev = NULL;
  if (bins[idx])
    bins[idx]->prev = block;
  bins[idx] = block;
}

void remove_from_bin(Block *block) {
  int idx = get_bin_index(block->size);
  if (block->prev)
    block->prev->next = block->next;
  else
    bins[idx] = block->next;
  if (block->next)
    block->next->prev = block->prev;
}

void *myalloc(size_t size) {
  if (size <= 0)
    return NULL;
  size = (size + 15) & ~15; // 16-byte alignment

  int start_bin = get_bin_index(size);
  Block *found = NULL;
  for (int i = start_bin; i < NUM_BINS; i++) {
    Block *curr = bins[i];
    while (curr) {
      if (curr->size >= size) {
        found = curr;
        break;
      }
      curr = curr->next;
    }
    if (found)
      break;
  }

  if (found) {
    remove_from_bin(found);
    // Splitting Logic
    size_t needed = size + sizeof(Block) + sizeof(Footer);
    if (found->size >= (needed + 32)) { // Only split if remainder is useful
      Block *new_b = (Block *)((char *)(found + 1) + size + sizeof(Footer));
      new_b->size = found->size - size - sizeof(Block) - sizeof(Footer);
      new_b->is_free = true;
      new_b->canary = CANARYNUMBER;
      update_footer(new_b);
      add_to_bin(new_b);

      found->size = size;
    }
    found->is_free = false;
    update_footer(found);
    return (void *)(found + 1);
  }

  // 2. Heap Growth
  size_t total_request_size = sizeof(Block) + size + sizeof(Footer);
  size_t page_aligned_size =
      (total_request_size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
  Block *new_block = sbrk(page_aligned_size);
  if (new_block == (void *)-1)
    return NULL;

  new_block->size = size;
  new_block->is_free = false;
  new_block->canary = CANARYNUMBER;
  update_footer(new_block);

  return (void *)(new_block + 1);
}

void myfree(void *ptr) {
  if (!ptr)
    return;

  // 1. Recover the header
  Block *curr = (Block *)ptr - 1;

  // 2. SECURITY CHECK: Validate the canary BEFORE doing anything else
  check_corruption(curr);

  // 3. POISONING: Wipe the user's data so it can't be leaked or used after free
  // We use 0xAA to mark this as "Poisoned" memory
  memset(ptr, 0xAA, curr->size);

  // 4. Update status
  curr->is_free = true;

  // --- COALESCE FORWARD (Physics) ---
  Footer *curr_footer = (Footer *)((char *)(curr + 1) + curr->size);
  Block *next_phys = (Block *)((char *)curr_footer + sizeof(Footer));

  // Validate next_phys has a valid canary before trusting its is_free status
  if (next_phys->canary == CANARYNUMBER && next_phys->is_free) {
    remove_from_bin(next_phys);
    curr->size += sizeof(Block) + next_phys->size + sizeof(Footer);
  }

  // --- COALESCE BACKWARD (Rear-view Mirror) ---
  Footer *prev_footer = (Footer *)((char *)curr - sizeof(Footer));
  if (prev_footer->is_free && prev_footer->size > 0) {
    Block *prev_phys = (Block *)((char *)curr - sizeof(Footer) -
                                 prev_footer->size - sizeof(Block));

    // Final safety check on the previous block's integrity
    if (prev_phys->canary == CANARYNUMBER) {
      remove_from_bin(prev_phys);
      prev_phys->size += sizeof(Block) + curr->size + sizeof(Footer);
      curr = prev_phys;
    }
  }

  // 5. FINALIZATION: Update the new combined footer and sort it into a bin
  update_footer(curr);
  add_to_bin(curr);
}

int main() {
  printf("--- Level 5: Segregated Bins + Footers ---\n");
  void *p1 = myalloc(64);
  void *p2 = myalloc(64);

  printf("Freeing blocks to test physics-based coalescing...\n");
  myfree(p1);
  myfree(p2);

  void *p3 = myalloc(140); // Should be one giant coalesced block
  printf("p3 allocated at: %p (If match %p, Coalescing worked!)\n", p3, p1);

  return 0;
}
