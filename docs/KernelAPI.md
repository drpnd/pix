# Kernel API

## Memory Operations

### Memory zones
* `0x00000000--0x00ffffff` (0--16 MiB): DMA for traditional peripherals
* `0x01000000--0xffffffff` (16 MiB--4 GiB): LOWMEM for NUMA-unaware space
* `>=0x100000000`: UMA/NUMA(n) for NUMA-aware zones

### Physical page allocator
* pmem_prim_alloc_pages(): Allocate superpages from the specified zone
* pmem_alloc_pages(): Allocate pages

### Low-level
* kmem_prim_alloc_page(): Page allocator that is called from arch_vmem_map(),
  pmem_prim_alloc_pages(), and kmem_prim_alloc_pages()

* pgalloc(): Allocate pages (physically contiguous)

### High-level
* Kernel memory allocators
  * kmalloc(): Allocate kernel memory (physically and virtually contiguous)
  * vmalloc(): Allocate kernel memory (virtually contiguous)
  * kfree(): Release kernel memory
* Userland memory allocators (in kernel)
  * uvmalloc(): Allocate memory (virtually contiguous) in the virtual memory
    region of the current process

### System call
* mmap(): System call to allocate memory (high-level)

### Backend (not exposed to the kernel)
* Global
  * physcal_pgalloc(): Allocate physical memory pages
* Per page table
  * pg_map(): Page table mapping
  * pg_v2p(): Lookup the physical address from the virtual address

### Primitive functions (dependencies)

> Make sure not to create any circle dependencies!

* kmem_alloc_page()
* kmem_prim_alloc_superpages()
* pmem_prim_alloc_superpages()

* kmem_alloc_pages()
* pmem_alloc_pages()

* kmem_alloc_pages(): allocate contiguous 2^n superpages (wired) using
  pmem_alloc_pages(), i.e., from the LOWMEM zone
* pmem_prim_alloc_pages(): allocate contiguous 2^n superpages (physical) from
  the specified zone


#### Kernel memory allocator

    kmalloc()
      |
      +-> kmem_alloc_pages() -> arch_kmem_map()
      |
      |
      +-> pmem_alloc_pages()

    # 4K-page allocation
    kmem_prim_alloc_page()
      |
      +-> kmem_prim_alloc_superpage() -> arch_kmem_map()
      |
      |
      +-> pmem_prim_alloc_superpage()


    kmem_prim_alloc_pages()
      |
      +-> kmem_alloc_pages() -> arch_kmem_map()
      |     |                        Use 4K-pages
      |     v
      +-> pmem_prim_alloc_pages()

#### Virtual memory allocator

    uvmalloc()
      |
      +-> arch_vmem_map() -> kmalloc() (4K-pages)
      |
      |
      +-> pgalloc() -> kmalloc() (4K-pages)
           |
           +-> pmem_prim_alloc_pages()

### Memory allocation

    [S]mmap()        [K]kmalloc()/vmalloc()
        |                |
        v                |
    [K]pgalloc()<--------+
        |
        +-----------+
        |           |
        v           |
    [K]pg_map()-----+---->[K]primalloc()

* Page allocation
  1. Allocate virtual pages
  1. Allocate physical pages if necessary
  1. Set up page table (with right access right)
* On page fault
  1. Allocate physical memory
  1. Set up page table



### Options
* Wired / Swappable
* Contiguous / Non-contiguous
* Zone (DMA, LowMem, HighMem)
* NUMA domain

## Clock & Timer
### Clock
To get the current clock
* [MANDATORY] Jiffies: Using PIT or local APIC. Updated by BSP, read from BSP and APs.
* TSC: Per core

### Timer
To fire interrupts
* Local APIC timer

### Interfaces
* ksignal_
  * ksignal_clock(): Clock interrupt
  * ksignal_fpe(): Floating point exception
  * ksignal_segv(): Segmentation fault
  * ksignal_pf(): Page fault


## Inter-Process Communication
