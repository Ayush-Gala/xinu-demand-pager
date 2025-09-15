# xinu-demand-pager

A hands-on implementation of demand paging, memory mapping, and page replacement policies in the Xinu toy operating system.

## Overview

This project explores **demand paging** which is the mechanism that allows an OS to implement virtual memory (aka. make a program think it has more memory than physically available) by transparently swapping pages in and out of a backing store.

I built this as part of **CSC 501: Operating Systems Fundamentals** at NC State University to gain a deeper understanding of:

* Virtual memory and address translation.
* On-demand page table creation and destruction.
* Page fault handling and Second-Chance (SC) page replacement policy.
* Private per-process heaps with dynamic memory allocation in user space.

This repo demonstrates how paging works under the hood in a real (albeit small) operating system.

---

## Goals & Learning Outcomes

The core objectives of this project were to:

✅ Implement **memory mapping syscalls**:

* `xmmap()` / `xmunmap()` – map/unmap backing stores to virtual pages.
* `vcreate()` – create processes with private virtual heaps.
* `vgetmem()` / `vfreemem()` – allocate/free memory from a process's private heap.

✅ Build **backing store management**:

* `get_bs()`, `release_bs()`, `read_bs()`, `write_bs()` – emulate a swap space in RAM.

✅ Implement **on-demand paging**:

* Handle page faults via an ISR.
* Dynamically allocate page tables and frames only when needed.
* Write dirty pages back to the backing store when evicted.

✅ Design and integrate **page replacement policy**:

* Implemented **Second-Chance (SC)** replacement algorithm using a circular queue.

By the end, I understood:

* How page directories and tables work on x86.
* How operating systems handle page faults.
* Why replacement policies matter for performance.
* How to manage memory isolation between processes.

---

## Memory Layout (Simplified)

```text
--------------------------------------------
Virtual Memory       (pages 4096 and beyond)
--------------------------------------------
8 Backing Stores     (pages 2048 - 4095)
--------------------------------------------
1024 Frames          (pages 1024 - 2047)
--------------------------------------------
Kernel + Global Memory (pages 0 - 1023)
--------------------------------------------
```

Each process gets:

* A **shared mapping** for the first 16 MB of memory.
* A **private page directory** and on-demand page tables for virtual memory.
* A **private heap** (if created with `vcreate()`).

---

## Project Organization & Design

This section explains how the code is structured and how different pieces work together to implement demand paging in Xinu. If you’re looking to explore the implementation or extend it, this will be your roadmap.

---

### Directory Layout

Most of the paging code lives in the `paging/` directory and the kernel headers under `h/`. Relevant files include:

* **`paging.h`** – Definitions for paging data structures, constants, and function prototypes.
* **`paging.c`** – Implementations of core paging syscalls (`xmmap`, `xmunmap`, `get_bs`, `release_bs`, etc.).
* **`frame.c`** – Frame allocator and free-list management.
* **`vcreate.c`**, **`vgetmem.c`**, **`vfreemem.c`** – Process creation with private heaps and heap memory management.
* **`pfint.c`** – Page fault ISR (Interrupt Service Routine).
* **`policy.c`** – Page replacement policy (Second-Chance queue implementation).
* **`initialize.c`** – Memory initialization and global page table setup.

---

### Memory & Backing Store emulation

#### Backing Store

Xinu has no disk or filesystem support, so swap space is emulated using physical memory.

* **Eight backing stores** (IDs `0–7`) are reserved in the top 8MB of physical memory (frames `2048–4095`).
* Each backing store can hold up to **256 pages** (1 MB).
* Functions like `get_bs()`, `release_bs()`, `read_bs()`, and `write_bs()` manage these stores.

Backing stores act like traditional swap space: you don’t normally interact with them directly except when mapping (`xmmap()`) or unmapping (`xmunmap()`) virtual memory regions, or when loading/evicting pages on a fault.

---

### Virtual Memory Layout

The memory is divided as follows (4KB page size):

```
--------------------------------------------
Virtual Memory       (pages 4096 and beyond)
--------------------------------------------
8 Backing Stores     (pages 2048 - 4095)
--------------------------------------------
1024 Frames          (pages 1024 - 2047)
--------------------------------------------
Kernel + Global Memory (pages 0 - 1023)
--------------------------------------------
```

* **Pages 0–1023:** Global, identity-mapped memory shared by all processes (kernel code, data, and kernel heap).
* **Pages 1024–2047:** Free frames for resident pages, page directories, and page tables.
* **Pages 2048–4095:** Reserved for backing stores (swap).
* **Pages ≥ 4096:** Each process’s private virtual memory space (heap, user stack, and demand-paged regions).

This design ensures that processes share the kernel’s lower memory region while isolating their own virtual address space.

---

### Supporting Data Structures

#### Backing Store Map

Maps a process’s virtual pages to a backing store. Each entry looks like:

```
{ pid, vpage, npages, store }
```

This allows the page fault handler to locate the correct backing store and offset for any faulted address.

#### Inverted Page Table

Tracks which process/page is currently stored in each physical frame. Each entry contains:

```
{ frame number, pid, virtual page number, ref_count, dirty_bit }
```

This table is crucial for page replacement and for writing dirty pages back to the correct backing store.

---

### Process-Specific Considerations

* **Process Creation (`vcreate`)**

  * Allocates a page directory for the new process.
  * Maps the first 16MB of physical memory (global space) into the process’s page directory.
  * Allocates a backing store for its private heap and sets up the backing store map.

* **Process Destruction**

  * Writes back dirty pages.
  * Frees all frames used by the process.
  * Releases its private backing store(s).
  * Cleans up its page directory.

* **Context Switching**

  * Updates the `PDBR` register to point to the next process’s page directory.
  * Ensures the MMU uses the correct address space after every switch.

* **System Initialization**

  * Creates global page tables for pages `0–4095`.
  * Sets up the NULL process’s page directory.
  * Installs the page fault ISR (interrupt 14).
  * Enables paging via the CR0 register.

---

### Page Fault Handling

When a page fault occurs:

1. The ISR extracts the faulting address and checks if it is mapped in the page directory.
2. If the relevant page table doesn’t exist, it is allocated on-demand.
3. A free frame is obtained (possibly evicting another page via the SC algorithm).
4. The page is read from its backing store into the frame.
5. The page table entry is updated, and the process resumes.

---

### Page Replacement (Second-Chance Policy)

Frames are maintained in a **circular queue**. When a replacement is needed:

* Check the reference bit of the current frame.
* If **clear**, evict it.
* If **set**, clear it and give the frame a “second chance” by moving on to the next frame.
* Continue until a victim frame is found.

This balances efficiency and fairness, approximating LRU while avoiding expensive time-stamp updates.

---

### Debugging & Output

When `srpolicy(SC)` is called, the system prints the frame numbers of replaced pages whenever replacement occurs. This was used for grading and validation of the SC algorithm.

---


## Testing

A test file `testmain.c` was used to verify:

* Correct memory mapping and data persistence across `xmmap()`/`xmunmap()`.
* Correct heap allocation and freeing with `vgetmem()`/`vfreemem()`.
* Proper frame allocation, replacement, and printing of replaced frame numbers.

Sample output snippet:

```
1: shared memory
0x40000000: A
0x40001000: B
...
0x40019000: Z

2: vgetmem/vfreemem
pid 47 has private heap
heap allocated at 1000000
heap variable: 100 200
```

---

## Setup Guide

> **Note:** This project is designed for the Xinu educational OS and runs in QEMU/Bochs with 16MB of simulated memory.

1. Clone the repo and build:

   ```bash
   git clone https://github.com/<your-username>/xinu-demand-pager.git
   cd compile
   make clean && make
   ```
2. Launch Xinu in QEMU:

   ```bash
   qemu-system-i386 -kernel xinu.elf
   ```
3. Run `testmain.c` to verify behavior.

---

## Next Steps / Ideas

* Add support for multiple replacement policies (e.g., LRU, FIFO) with runtime selection.
* Implement paging of page tables themselves (for a fully virtualized system).
* Measure page fault rate under different workloads and policies.
