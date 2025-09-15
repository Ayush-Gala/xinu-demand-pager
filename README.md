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

## 🔮 Next Steps / Ideas

* Add support for multiple replacement policies (e.g., LRU, FIFO) with runtime selection.
* Implement paging of page tables themselves (for a fully virtualized system).
* Measure page fault rate under different workloads and policies.
