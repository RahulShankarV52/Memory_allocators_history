# Custom Memory Management Suite
> **A multi-stage implementation of dynamic memory allocators, ranging from primitive bump pointers to industrial-grade segregated-fit systems.**

This repository is a deep dive into the architecture of memory management. It documents the journey from interacting with raw Operating System primitives to building a production-grade allocator that manages fragmentation, alignment, and performance.

---
## The Evolution Roadmap

### Level 1 & 6: Linear "Bump" & Arena Allocators
* **The Concept:** The fastest possible allocation strategy. It moves a single "High Water Mark" pointer forward for every request.
* **Mechanism:** Uses `sbrk` (Global Heap) or `mmap` (Independent Slabs) to claim memory from the Kernel.
* **Complexity:** $O(1)$ Allocation, $O(1)$ Reset.
* **Best For:** Temporary, frame-based, or per-request data lifecycles where individual `free()` calls are unnecessary.

### Level 2 & 3: Fixed-Size Pool Allocator
* **The Concept:** High-speed allocation for uniform objects (e.g., game entities or network packets).
* **Mechanism:** Employs a **Linked Free-list** embedded within the unused objects themselves.
* **Complexity:** $O(1)$ Allocation and Deallocation.
* **Best For:** High-frequency objects where size is constant, ensuring zero fragmentation.

### Level 4: Explicit Free List (The Generalist)
* **The Concept:** A variable-sized allocator that handles any request size.
* **Mechanism:** Implements **Block Splitting** (to prevent internal waste) and **Bi-directional Coalescing** (to fuse adjacent holes into larger blocks).
* **Security:** Features **Canary Values** (`0xBAD`) to detect buffer overflows at the metadata level.

### Level 5: Segregated Bins & Boundary Tags (The Professional)
* **The Concept:** A production-grade system mirroring the behavior of `glibc`'s `malloc`.
* **Mechanism:** * **Segregated Fit:** Sorts free blocks into 8 size-classed bins for near-instant searches.
    * **Boundary Tag Footers:** A "rear-view mirror" for every block, allowing $O(1)$ coalescing with previous physical neighbors.
    * **Page Alignment:** Requests memory in 4096-byte chunks to minimize expensive Kernel context switches.
* **Security:** Implements **Memory Poisoning** (`0xAA`) to catch Use-After-Free bugs instantly.
---

## Core Technical Concepts

To understand the architecture of these allocators, one must grasp the "Memory Skyscraper":

1.  **System Calls vs. User-land:** `sbrk` and `mmap` are heavy Kernel requests; our allocators manage that memory in "User-land" to avoid the system call overhead.
2.  **Memory Alignment:** All allocations are rounded to **16-byte boundaries**. This prevents "Page Splitting" and ensures the CPU can fetch data in the fewest possible cycles.
3.  **The "Footer" Jump:** We use pointer arithmetic—`(Block *)((char *)curr - sizeof(Footer) - prev_f->size - sizeof(Block))`—to jump backward in physical memory without needing a global pointer.
4.  **Fragmentation:** We solve **External Fragmentation** through aggressive merging of free blocks, ensuring the heap doesn't become "Swiss Cheese."
---

## Performance Comparison

| Allocator | Search Time | Fragmentation | Standalone? | Best Use Case |
| :--- | :--- | :--- | :--- | :--- |
| **Arena** | $O(1)$ | High (Internal) | Yes (`mmap`) | Transient Task Data |
| **Pool** | $O(1)$ | Zero | No | Uniform Game Objects |
| **Explicit List**| $O(N)$ | Low | Yes (`sbrk`) | Basic General Purpose |
| **Binned Heap** | **$O(1)$** | **Low** | **Yes (`sbrk`)** | **Industrial Applications** |

---
## Resources:
1. **Dan Luu — _Malloc Tutorial_** https://gee.cs.oswego.edu/dl/html/malloc.html
2. **Jacob Sorber — _Making allocators and object pools faster using a free list_** https://www.youtube.com/watch?v=MxgnS9Lwv0k 
3. **Tsoding Dailt - Writing my own malloc in c** https://www.youtube.com/watch?v=sZ8GJ1TiMdk
4. **Daniel Hirsch - Coding malloc in C from Scratch** https://www.youtube.com/watch?v=_HLAWI84aFA
