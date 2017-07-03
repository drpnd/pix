# Kernel API

## Memory Operations

### Memory zones
* `0x00000000--0x00ffffff` (0--16 MiB): DMA for traditional peripherals
* `0x01000000--0xffffffff` (16 MiB--4 GiB): LOWMEM for NUMA-unaware space
* `>=0x100000000`: UMA/NUMA(n) for NUMA-aware zones

### Low-level
* pgalloc(): Allocate physical pages (physically contiguous)

### High-level
* kmalloc(): Allocate kernel memory (physically and virtually contiguous)
* vmalloc(): Allocate kernel memory (virtually contiguous)

### Userland memory
* mmap(): System call to allocate memory (high-level)

### Backend (not exposed to the kernel)
* Global
  * physcal_pgalloc(): Allocate physical memory pages
* Per page table
  * pg_map(): Page table mapping
  * pg_v2p(): Lookup the physical address from the virtual address

### Primitive functions (dependencies)

> Make sure not to create any circle dependencies!

* kmem_alloc_pages(): allocate contiguous 2^n superpages (wired) using
  pmem_alloc_pages(), i.e., from the LOWMEM zone
* pmem_alloc_pages(): allocate contiguous 2^n superpages (physical) from the
  LOWMEM zone


    kmem_cache_create()
      |
      +-> kmalloc()
           |
           +-> kmem_alloc_pages() -> arch_kmem_map()
           |      |
           |      v
           +----> pmem_alloc_pages()

--

#### Kernel memory allocator
    kmalloc()
      |
      +-> arch_kmem_map() -> kmem_mm_page_alloc()/kmem_mm_page_free()
      |
      |
      +-> pmem_alloc_pages()

#### Virtual memory allocator
    vmalloc()
      |
      +-> arch_vmem_map() -> kmalloc()
      |
      |
      +-> pmem_alloc_pages()


### Options
* Wired / Swappable
* Contiguous / Non-contiguous
* Zone (DMA, LowMem, HighMem)
* NUMA domain

## Inter-Process Communication
