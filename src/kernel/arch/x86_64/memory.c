/*_
 * Copyright (c) 2015-2016 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <aos/const.h>
#include "arch.h"
#include "memory.h"
#include "../../kernel.h"

/* FIXME: Rename the following */
#define KMEM_DIR_RW(a)          ((u64)(a) | 0x007ULL)
#define KMEM_PG_RW(a)           ((u64)(a) | 0x083ULL)
#define KMEM_PG_GRW(a)          ((u64)(a) | 0x183ULL)
#define VMEM_DIR_RW(a)          ((u64)(a) | 0x007ULL)
#define VMEM_PG_RW(a)           ((u64)(a) | 0x087ULL)
#define VMEM_PG_GRW(a)          ((u64)(a) | 0x187ULL)
#define VMEM_IS_PAGE(a)         ((u64)(a) & 0x080ULL)
#define VMEM_IS_PRESENT(a)      ((u64)(a) & 0x001ULL)
#define VMEM_PT(a)              (u64 *)((a) & 0x7ffffffffffff000ULL)
#define VMEM_PDPG(a)            (void *)((a) & 0x7fffffffffe00000ULL)

/* Type of memory area */
#define BSE_USABLE              1
#define BSE_RESERVED            2
#define BSE_ACPI_RECLAIMABLE    3
#define BSE_ACPI_NVS            4
#define BSE_BAD                 5

/*
 * Prototype declarations of static functions
 */
static struct kmem *
_kmem_init(struct kstring *, struct kstring *, struct kstring *);
static int _kmem_pgt_init(struct arch_kmem_space **, u64 *);
static void * _kmem_mm_page_alloc(struct kmem *);
static void _kmem_mm_page_free(struct kmem *, void *);
static int _kmem_space_init(struct kmem *, struct kstring *, u64 *);
static int _kmem_space_pgt_reflect(struct kmem *);
static int _kmem_map(struct kmem *, u64, u64, int);
static int _kmem_unmap(struct kmem *, u64);
static int
_pmem_init_stage1(struct bootinfo *, struct acpi *, struct kstring *,
                  struct kstring *, struct kstring *);
static int _pmem_init_stage2(struct kmem *);
static int _pmem_buddy_init(struct pmem *);
static int _pmem_buddy_order(struct pmem *, size_t);
static u64 _resolve_phys_mem_size(struct bootinfo *);
static void * _find_pmem_region(struct bootinfo *, u64 );
static __inline__ int _pmem_page_zone(void *, int);
static void _enable_page_global(void);
static void _disable_page_global(void);


/*
 * Initialize physical memory
 *
 * SYNOPSIS
 *      int
 *      arch_memory_init(struct bootinfo *bi, struct acpi *acpi);
 *
 * DESCRIPTION
 *      The arch_memory_init() function initializes the physical memory manager
 *      with the memory map information bi inherited from the boot monitor.
 *      The third argument acpi is used to determine the proximity domain of the
 *      memory spaces.
 *
 * RETURN VALUES
 *      If successful, the arch_memory_init() function returns the value of 0.
 *      It returns the value of -1 on failure.
 */
int
arch_memory_init(struct bootinfo *bi, struct acpi *acpi)
{
    struct kstring region;
    struct kstring pmem;
    struct kstring pmem_pages;
    int ret;

    /* Stage 1: Initialize the physical memory with the page table of linear
       addressing.  This allocates the data structure for the physical memory
       manager, and also stores the physical page zone in each page data
       structure. */

    /* Allocate physical memory management data structure */
    ret = _pmem_init_stage1(bi, acpi, &region, &pmem, &pmem_pages);
    if ( ret < 0 ) {
        return -1;
    }

    /* Stage 2: Setup the kernel page table, and initialize the kernel memory
       and physical memory with this page table. */

    /* Initialize the kernel memory management data structure */
    g_kmem = _kmem_init(&region, &pmem, &pmem_pages);
    if ( NULL == g_kmem ) {
        return -1;
    }

    /* Initialize the physical pages */
    ret = _pmem_init_stage2(g_kmem);
    if ( ret < 0 ) {
        return -1;
    }

    return 0;
}

/*
 * Allocate physical memory management data structure
 *
 * SYNOPSIS
 *      static int
 *      _pmem_init_stage1(struct bootinfo *bi, struct acpi *acpi,
 *                        struct kstring *region, struct kstring *pmem,
 *                        struct kstring *pmem_pages);
 *
 * DESCRIPTION
 *      The _pmem_init_stage1() function allocates a space for the physical
 *      memory manager from the memory map information bi inherited from the
 *      boot monitor.  The base address and the size of the allocated memory
 *      space are returned through the second and third arguments, base and
 *      pmsz.
 *
 * RETURN VALUES
 *      If successful, the _pmem_init_stage1() function returns the value of 0.
 *      It returns the value of -1 on failure.
 */
static int
_pmem_init_stage1(struct bootinfo *bi, struct acpi *acpi,
                  struct kstring *region, struct kstring *pmem,
                  struct kstring *pmem_pages)
{
    u64 sz;
    u64 npg;
    void *base;
    u64 pmsz;
    u64 i;
    u64 a;
    u64 b;
    u64 pg;
    struct pmem *pm;
    struct pmem_page *pgs;
    struct bootinfo_sysaddrmap_entry *bse;
    u64 pxbase;
    u64 pxlen;
    int prox;

    /* Check the number of address map entries */
    if ( bi->sysaddrmap.nr <= 0 ) {
        return -1;
    }

    /* Obtain memory (space) size from the system address map */
    sz = _resolve_phys_mem_size(bi);

    /* Calculate the number of pages from the upper-bound of the memory space */
    npg = DIV_CEIL(sz, SUPERPAGESIZE);

    /* Calculate the size required by the pmem and pmem_page structures */
    pmsz = sizeof(struct pmem) + npg * sizeof(struct pmem_page);

    /* Fine the available region for the pmem data structure */
    base = _find_pmem_region(bi, pmsz);
    if ( NULL == base ) {
        /* Could not find available pages for the management structure */
        return -1;
    }

    /* Return the memory space */
    region->base = base;
    region->sz = pmsz;
    /* Return the memory space used by the page management data structure */
    pmem_pages->base = base;
    pmem_pages->sz = npg * sizeof(struct pmem_page);
    pmem->base = base + pmem_pages->sz;
    pmem->sz = sizeof(struct pmem);

    /* Initialize the pmem data structure */
    pm = (struct pmem *)pmem->base;
    kmemset(pm, 0, sizeof(struct pmem));
    pm->nr = npg;
    pm->pages = NULL;

    /* Initialize the pages with the current boot strap linear addressing page
       table */
    pgs = (struct pmem_page *)pmem_pages->base;
    kmemset(pgs, 0, sizeof(struct pmem_page) * npg);
    for ( i = 0; i < npg; i++ ) {
        /* Initialize all pages as in the LOWMEM zone */
        pgs[i].zone = PMEM_ZONE_LOWMEM;
        pgs[i].flags = 0;
        pgs[i].order = PMEM_INVAL_BUDDY_ORDER;
        pgs[i].next = PMEM_INVAL_INDEX;
    }

    /* Mark as used for the reserved low memory (used by BIOS etc.) */
    for ( i = 0; i < DIV_CEIL(PMEM_LBOUND, SUPERPAGESIZE); i++ ) {
        pgs[i].flags |= PMEM_USED;
    }

    /* Mark as used for the pmem pages */
    for ( i = 0; i < DIV_CEIL(region->sz, SUPERPAGESIZE); i++ ) {
        pg = DIV_FLOOR((u64)region->base, SUPERPAGESIZE) + i;
        pgs[pg].flags |= PMEM_USED;
    }

    /* Mark the usable region */
    for ( i = 0; i < bi->sysaddrmap.nr; i++ ) {
        bse = &bi->sysaddrmap.entries[i];
        if ( BSE_USABLE == bse->type ) {
            a = DIV_CEIL(bse->base, SUPERPAGESIZE);
            b = DIV_FLOOR(bse->base + bse->len, SUPERPAGESIZE);
            for ( pg = a; pg < b; pg++ ) {
                pgs[pg].flags |= PMEM_USABLE;
            }
        }
    }

    /* Resolve the zone of each page */
    if ( acpi_is_numa(acpi) ) {
        /* NUMA */
        prox = -1;
        for ( i = 0; i < npg; i++ ) {
            if ( prox < 0
                 || !(SUPERPAGE_ADDR(i) >= pxbase
                      && SUPERPAGE_ADDR(i + 1) <= pxbase + pxlen) ) {
                /* Resolve the proximity domain of the page */
                prox = acpi_memory_prox_domain(acpi, SUPERPAGE_ADDR(i), &pxbase,
                                               &pxlen);
                if ( prox < 0
                     || !(SUPERPAGE_ADDR(i) >= pxbase
                          && SUPERPAGE_ADDR(i + 1) <= pxbase + pxlen) ) {
                    /* No proximity domain; meaning unusable page. */
                    continue;
                }
            }
            pgs[i].zone = _pmem_page_zone((void *)SUPERPAGE_ADDR(i), prox);
        }
    } else {
        /* UMA */
        for ( i = 0; i < npg; i++ ) {
            pgs[i].zone = _pmem_page_zone((void *)SUPERPAGE_ADDR(i), -1);
        }
    }

    return 0;
}

/*
 * Find the upper bound (highest address) of the memory region to determine the
 * number of physical pages
 */
static u64
_resolve_phys_mem_size(struct bootinfo *bi)
{
    struct bootinfo_sysaddrmap_entry *bse;
    u64 addr;
    u64 i;

    /* Obtain memory size */
    addr = 0;
    for ( i = 0; i < bi->sysaddrmap.nr; i++ ) {
        bse = &bi->sysaddrmap.entries[i];
        if ( bse->base + bse->len > addr ) {
            /* Get the highest address */
            addr = bse->base + bse->len;
        }
    }

    return addr;
}

/*
 * Find the memory region (physical-page-aligned) for the pmem data structure
 */
static void *
_find_pmem_region(struct bootinfo *bi, u64 sz)
{
    struct bootinfo_sysaddrmap_entry *bse;
    u64 addr;
    u64 i;
    u64 a;
    u64 b;

    /* Search free space system address map obitaned from BIOS for the memory
       allocator (calculated above) */
    addr = 0;
    for ( i = 0; i < bi->sysaddrmap.nr; i++ ) {
        bse = &bi->sysaddrmap.entries[i];
        if ( BSE_USABLE == bse->type ) {
            /* Available space from a to b */
            a = CEIL(bse->base, SUPERPAGESIZE);
            b = FLOOR(bse->base + bse->len, SUPERPAGESIZE);

            if ( b < PMEM_LBOUND ) {
                /* Skip below the lower bound */
                continue;
            } else if ( a < PMEM_LBOUND ) {
                /* Check the space from the lower bound to b */
                if ( b - PMEM_LBOUND >= sz ) {
                    /* Found */
                    addr = PMEM_LBOUND;
                    break;
                } else {
                    /* Not found, then search another space */
                    continue;
                }
            } else {
                if ( b - a >= sz ) {
                    /* Found */
                    addr = a;
                    break;
                } else {
                    /* Not found, then search another space */
                    continue;
                }
            }
        }
    }

    /* This address space must be within the first 3 GiB so that it is ensured
       to be mapped in the linear addressing space. */
    if ( addr + sz > PMEM_MAPPED_UBOUND ) {
        return (void *)0;
    }

    return (void *)addr;
}

/* Initialize the virtual memory space for kernel */
static struct kmem *
_kmem_init(struct kstring *region, struct kstring *pmem,
           struct kstring *pmem_pages)
{
    ssize_t i;
    u64 off;
    struct arch_kmem_space *akmem;
    struct kmem *kmem;
    struct kmem_mm_page *mmpg;
    int ret;

    /* Reset the offset to KMEM_BASE for the memory arrangement */
    off = 0;

    /* Prepare the minimum page table */
    ret = _kmem_pgt_init(&akmem, &off);
    if ( ret < 0 ) {
        return NULL;
    }
    /* Prepare the kernel memory space */
    kmem = (struct kmem *)KMEM_P2V(KMEM_BASE + off);
    off += sizeof(struct kmem);
    if ( off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(kmem, 0, sizeof(struct kmem));

    /* Initialize the kernel memory space */
    ret = _kmem_space_init(kmem, region, &off);
    if ( ret < 0 ) {
        return NULL;
    }
    /* Set the architecture-specific kernel memory management data structure */
    kmem->space->arch = akmem;

    /* Add the remaining pages to the free page list */
    kmem->pool.mm_pgs = NULL;
    for ( i = DIV_CEIL(off, PAGESIZE);
          i < (ssize_t)DIV_FLOOR(KMEM_MAX_SIZE, PAGESIZE); i++ ) {
        /* Prepend a page */
        mmpg = (struct kmem_mm_page *)KMEM_P2V(KMEM_BASE + PAGE_ADDR(i));
        mmpg->next = kmem->pool.mm_pgs;
        kmem->pool.mm_pgs = mmpg;
    }

    /* Initialize slab */
    kmemset(&kmem->slab, 0, sizeof(struct kmem_slab_root));

    /* Reflect the regions to the page table */
    ret = _kmem_space_pgt_reflect(kmem);
    if ( ret < 0 ) {
        return NULL;
    }

    /* Initialize the buddy system for the kernel region */
    ret = kmem_buddy_init(kmem);
    if ( ret < 0 ) {
        return NULL;
    }

    /* Setup physical memory */
    kmem->pmem
        = (struct pmem *)KMEM_P2V(PMEM_LBOUND + pmem->base - region->base);
    kmem->pmem->pages
        = (struct pmem_page *)KMEM_P2V(PMEM_LBOUND + pmem_pages->base
                                       - region->base);

    return kmem;
}

/*
 * Initialize virtual memory space for kernel (minimal initialization)
 *
 * SYNOPSIS
 *      static int
 *      _kmem_pgt_init(struct arch_vmem_space **avmem, u64 *off);
 *
 * DESCRIPTION
 *      The _kmem_pgt_init() function initializes the page table for the kernel
 *      memory.  It creates the mapping entries from 0 to 4 GiB memory space of
 *      virtual memory with 2 MiB paging, and enables the low address space
 *      (0-32 MiB).  (FIXME: Temporarily use 5 GiB space to support hundreds
 *      gigabytes memory)
 *
 * RETURN VALUES
 *      If successful, the _kmem_pgt_init() function returns the value of 0.  It
 *      returns the value of -1 on failure.
 */
static int
_kmem_pgt_init(struct arch_kmem_space **akmem, u64 *off)
{
    void *pgt;
    int i;
    int nspg;
    int pgtsz;

    /* Architecture-specific kernel memory management  */
    *akmem = (struct arch_kmem_space *)KMEM_P2V(KMEM_BASE + *off);
    *off += sizeof(struct arch_kmem_space);
    if ( *off > KMEM_MAX_SIZE ) {
        return -1;
    }

    /* Page-alignment */
    *off = PAGE_ALIGN(*off);

    /* Allocate a page table that consists of 6 blocks */
    pgt = (void *)(KMEM_BASE + *off);
    pgtsz = PAGESIZE * 3;
    *off += pgtsz;
    if ( *off > KMEM_MAX_SIZE ) {
        return -1;
    }
    kmemset(pgt, 0, pgtsz);

    (*akmem)->pml4 = (void *)KMEM_P2V(pgt);
    (*akmem)->pdpt = (void *)KMEM_P2V(pgt + PAGESIZE);
    (*akmem)->pd = (void *)KMEM_P2V(pgt + PAGESIZE * 2);
    (*akmem)->cr3 = pgt;

    /* Set physical addresses to page directories */
    (*akmem)->pml4->entries[0] = KMEM_DIR_RW(pgt + PAGESIZE);
    (*akmem)->pdpt->entries[3] = KMEM_DIR_RW(pgt + PAGESIZE * 2);

    /* Superpage for the region in the range of 0-32 MiB */
    nspg = DIV_CEIL(PMEM_LBOUND, SUPERPAGESIZE);
    if ( nspg > 512 ) {
        /* The low memory address space for kernel memory is too large. */
        return -1;
    }
    /* Page directories for 0-32 MiB; must be consistent with KMEM_LOW_P2V */
    for ( i = 0; i < nspg; i++ ) {
        (*akmem)->pd->entries[i] = KMEM_PG_GRW(SUPERPAGE_ADDR(i));
    }

    /* Disable the global page feature */
    _disable_page_global();

    /* Set the constructured page table */
    set_cr3(pgt);

    /* Enable the global page feature */
    _enable_page_global();

    return 0;
}

/*
 * Create virtual memory space for the kernel memory
 */
static int
_kmem_space_init(struct kmem *kmem, struct kstring *region, u64 *off)
{
    ssize_t pg;
    size_t npg;
    size_t kpg;
    ssize_t i;
    size_t n;
    struct kmem_space *space;
    struct kmem_page *pages;

    /* Kernel memory space */
    space = (struct kmem_space *)KMEM_P2V(KMEM_BASE + *off);
    *off += sizeof(struct kmem_space);
    if ( *off > KMEM_MAX_SIZE ) {
        return -1;
    }
    kmemset(space, 0, sizeof(struct kmem_space));

    /* Set the kernel region */
    space->start = (ptr_t)KMEM_P2V(0);
    space->len = KERNEL_SIZE + KERNEL_SPEC_SIZE;

    /* Page-alignment */
    *off = CEIL(*off, PAGESIZE);

    /* Initialize the pages in this region. */
    pages = (struct kmem_page *)KMEM_P2V(KMEM_BASE + *off);
    npg = DIV_CEIL(space->len, SUPERPAGESIZE);
    *off += sizeof(struct kmem_page) * npg;
    if ( *off > KMEM_MAX_SIZE ) {
        return -1;
    }
    kmemset(pages, 0, sizeof(struct kmem_page) * npg);

    /* Initialize the kernel pages that used by the low physical memory region.
       Note that the pages in this region have already been configured in the
       kernel's page table. */
    for ( pg = 0; pg < (ssize_t)DIV_CEIL(PMEM_LBOUND, SUPERPAGESIZE); pg++ ) {
        pages[pg].addr = (reg_t)space->start + SUPERPAGE_ADDR(pg);
        pages[pg].order = KMEM_INVAL_BUDDY_ORDER;
        pages[pg].flags = KMEM_USED;
        pages[pg].next = NULL;
    }

    /* Prepare page data structures for the physical memory region */
    n = DIV_CEIL(region->sz, SUPERPAGESIZE);
    if ( pg + n >= npg ) {
        /* Exceeded the kernel region */
        return -1;
    }
    for ( i = 0; i < (ssize_t)n; i++ ) {
        pages[pg].addr = (reg_t)region->base + SUPERPAGE_ADDR(i);
        pages[pg].order = KMEM_INVAL_BUDDY_ORDER;
        pages[pg].flags = KMEM_USED | KMEM_USABLE;
        pages[pg].next = NULL;
        pg++;
    }

    /* Prepare page data structures for the kernel memory region */
    kpg = DIV_CEIL(KERNEL_SIZE, SUPERPAGESIZE);
    for ( ; pg < (ssize_t)kpg; pg++ ) {
        pages[pg].addr = 0;
        pages[pg].order = KMEM_INVAL_BUDDY_ORDER;
        pages[pg].flags = KMEM_USABLE;
        pages[pg].next = NULL;
    }

    /* Prepare page data structures for the special use (e.g., APIC region) */
    for ( ; pg < (ssize_t)npg; pg++ ) {
        pages[pg].addr = (reg_t)space->start + SUPERPAGE_ADDR(pg);
        pages[pg].order = KMEM_INVAL_BUDDY_ORDER;
        pages[pg].flags = KMEM_USED | KMEM_USABLE;
        pages[pg].next = NULL;
    }

    /* Set the pages */
    space->pages = pages;

    /* Set the space */
    kmem->space = space;

    return 0;
}

#if 0
/*
 * Create virtual memory space for the kernel memory
 */
static struct vmem_space *
_kmem_vmem_space_create(void *pmbase, u64 pmsz, u64 *off)
{
    u64 i;
    size_t n;
    struct vmem_space *space;
    struct vmem_region *reg_low;
    struct vmem_region *reg_pmem;
    struct vmem_region *reg_kernel;
    struct vmem_region *reg_spec;
    struct vmem_superpage *spgs_low;
    struct vmem_superpage *spgs_pmem;
    struct vmem_superpage *spgs_kernel;
    struct vmem_superpage *spgs_spec;

    /* Virtual memory space */
    space = (struct vmem_space *)KMEM_LOW_P2V(KMEM_BASE + *off);
    *off += sizeof(struct vmem_space);
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(space, 0, sizeof(struct vmem_space));

    /* Low address space (below 32 MiB): Note that this operating system has
       ``two'' kernel regions shared unlike other UNIX-like systems, regions
       from 0 to 1 GiB and from 3 to 4 GiB.  The first region could be removed
       by relocating the kernel, but this version of our operating system does
       not do it. */
    reg_low = (struct vmem_region *)KMEM_LOW_P2V(KMEM_BASE + *off);
    *off += sizeof(struct vmem_region);
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(reg_low, 0, sizeof(struct vmem_region));
    reg_low->start = (void *)0;
    reg_low->len = CEIL(PMEM_LBOUND, SUPERPAGESIZE);

    /* Kernel address space (3-3.64 GiB) */
    reg_kernel = (struct vmem_region *)KMEM_LOW_P2V(KMEM_BASE + *off);
    *off += sizeof(struct vmem_region);
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(reg_kernel, 0, sizeof(struct vmem_region));
    reg_kernel->start = (void *)KMEM_REGION_KERNEL_BASE;
    reg_kernel->len = KMEM_REGION_KERNEL_SIZE;

    /* Kernel address space for special use (3.64-4 GiB) */
    reg_spec = (struct vmem_region *)KMEM_LOW_P2V(KMEM_BASE + *off);
    *off += sizeof(struct vmem_region);
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(reg_spec, 0, sizeof(struct vmem_region));
    reg_spec->start = (void *)KMEM_REGION_SPEC_BASE;
    reg_spec->len = KMEM_REGION_SPEC_SIZE;

    /* Physical pages: This region is not placed at the kernel region because
       this is not directly referred from user-land processes (e.g., through
       system calls).  FIXME: This temporarily uses 4-5 GiB region. */
    reg_pmem = (struct vmem_region *)KMEM_LOW_P2V(KMEM_BASE + *off);
    *off += sizeof(struct vmem_region);
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(reg_pmem, 0, sizeof(struct vmem_region));
    reg_pmem->start = (void *)KMEM_REGION_PMEM_BASE;
    reg_pmem->len = CEIL(pmsz, SUPERPAGESIZE);

    /* Page-alignment */
    *off = CEIL(*off, PAGESIZE);

    /* Initialize the pages in this region.  Note that the (super)pages in this
       region have already been configured in the kernel's page table. */
    spgs_low = (struct vmem_superpage *)KMEM_LOW_P2V(KMEM_BASE + *off);
    n = DIV_CEIL(reg_low->len, SUPERPAGESIZE);
    *off += sizeof(struct vmem_superpage) * n;
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(spgs_low, 0, sizeof(struct vmem_superpage) * n);
    for ( i = 0; i < n; i++ ) {
        spgs_low[i].u.superpage.addr
            = KMEM_LOW_P2V(SUPERPAGE_ADDR(i) + (reg_t)reg_low->start);
        spgs_low[i].order = VMEM_INVAL_BUDDY_ORDER;
        spgs_low[i].flags = VMEM_USED | VMEM_GLOBAL | VMEM_SUPERPAGE;
        spgs_low[i].region = reg_low;
        spgs_low[i].next = NULL;
    }

    /* Prepare page data structures for kernel memory region */
    spgs_kernel = (struct vmem_superpage *)KMEM_LOW_P2V(KMEM_BASE + *off);
    n = DIV_CEIL(reg_kernel->len, SUPERPAGESIZE);
    *off += sizeof(struct vmem_superpage) * n;
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(spgs_kernel, 0, sizeof(struct vmem_superpage) * n);
    for ( i = 0; i < n; i++ ) {
        spgs_kernel[i].u.superpage.addr
            = (reg_t)reg_kernel->start + SUPERPAGE_ADDR(i);
        spgs_kernel[i].order = VMEM_INVAL_BUDDY_ORDER;
        spgs_kernel[i].flags = VMEM_USABLE | VMEM_GLOBAL | VMEM_SUPERPAGE;
        spgs_kernel[i].region = reg_kernel;
        spgs_kernel[i].next = NULL;
    }
    spgs_kernel[0].u.superpage.addr = SUPERPAGE_ADDR(0);
    spgs_kernel[0].flags = VMEM_USED | VMEM_GLOBAL | VMEM_SUPERPAGE;

    /* Prepare page data structures for kernel memory region for special use */
    spgs_spec = (struct vmem_superpage *)KMEM_LOW_P2V(KMEM_BASE + *off);
    n = DIV_CEIL(reg_spec->len, SUPERPAGESIZE);
    *off += sizeof(struct vmem_superpage) * n;
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(spgs_spec, 0, sizeof(struct vmem_superpage) * n);
    for ( i = 0; i < n; i++ ) {
        /* Linear addressing */
        spgs_spec[i].u.superpage.addr
            = (reg_t)reg_spec->start + SUPERPAGE_ADDR(i);
        spgs_spec[i].order = VMEM_INVAL_BUDDY_ORDER;
        spgs_spec[i].flags = VMEM_USABLE | VMEM_USED | VMEM_GLOBAL
            | VMEM_SUPERPAGE;
        spgs_spec[i].region = reg_spec;
        spgs_spec[i].next = NULL;
    }

    /* Prepare page data structures for physical memory management region */
    spgs_pmem = (struct vmem_superpage *)KMEM_LOW_P2V(KMEM_BASE + *off);
    n = DIV_CEIL(reg_pmem->len, SUPERPAGESIZE);
    *off += sizeof(struct vmem_superpage) * n;
    if ( *off > KMEM_MAX_SIZE ) {
        return NULL;
    }
    kmemset(spgs_pmem, 0, sizeof(struct vmem_superpage) * n);
    for ( i = 0; i < n; i++ ) {
        spgs_pmem[i].u.superpage.addr = (reg_t)pmbase + SUPERPAGE_ADDR(i);
        spgs_pmem[i].order = VMEM_INVAL_BUDDY_ORDER;
        spgs_pmem[i].flags = VMEM_USABLE | VMEM_USED | VMEM_GLOBAL
            | VMEM_SUPERPAGE;
        spgs_pmem[i].region = reg_pmem;
        spgs_pmem[i].next = NULL;
    }

    /* Page-alignment */
    *off = CEIL(*off, PAGESIZE);

    /* Set the allocated pages to each region */
    reg_low->superpages = spgs_low;
    reg_kernel->superpages = spgs_kernel;
    reg_spec->superpages = spgs_spec;
    reg_pmem->superpages = spgs_pmem;

    /* Create the chain of regions */
    space->first_region = reg_low;
    reg_low->next = reg_kernel;
    reg_kernel->next = reg_spec;
    reg_spec->next = reg_pmem;
    reg_pmem->next = NULL;

    return space;
}
#endif

/*
 * Reflect the virtual memory regions to the page table
 */
static int
_kmem_space_pgt_reflect(struct kmem *kmem)
{
    ssize_t i;
    int ret;
    reg_t paddr;
    reg_t vaddr;
    int flags;

    for ( i = 0; i < (ssize_t)DIV_CEIL(kmem->space->len, SUPERPAGESIZE); i++ ) {
        if ( !(KMEM_USABLE & kmem->space->pages[i].flags)
             || !(KMEM_USED & kmem->space->pages[i].flags) ) {
            /* Not usable/used, then do nothing to this superpage */
            continue;
        }
        /* Register a superpage */
        vaddr = (reg_t)kmem->space->start + SUPERPAGE_ADDR(i);
        paddr = kmem->space->pages[i].addr;
        flags = kmem->space->pages[i].flags;

        /* Remap */
        ret = _kmem_map(kmem, vaddr, paddr, flags);
        if ( ret < 0 ) {
            return -1;
        }
    }

    return 0;
}

/*
 * Get a free page
 */
static void *
_kmem_mm_page_alloc(struct kmem *kmem)
{
    struct kmem_mm_page *mmpg;

    /* Get the head of the free page list */
    mmpg = kmem->pool.mm_pgs;
    if ( NULL == mmpg ) {
        /* No free page found */
        return NULL;
    }
    kmem->pool.mm_pgs = mmpg->next;

    if ( NULL == kmem->pool.mm_pgs ) {
        return NULL;
    }

    return (void *)mmpg;
}

/*
 * Release a page to the free list
 */
static void
_kmem_mm_page_free(struct kmem *kmem, void *vaddr)
{
    struct kmem_mm_page *mmpg;

    /* Resolve the virtual address */
    mmpg = (struct kmem_mm_page *)vaddr;
    /* Return to the list */
    mmpg->next = kmem->pool.mm_pgs;
    kmem->pool.mm_pgs = mmpg;
}

/*
 * Map a virtual page to a physical page (in a superpage granularity)
 */
static int
_kmem_map(struct kmem *kmem, u64 vaddr, u64 paddr, int flags)
{
    struct arch_kmem_space *akmem;
    int idxpd;
    int idxp;

    /* Check the flags to ensure the page is usable and marked as used */
    if ( !(KMEM_USABLE & flags) || !(KMEM_USED & flags) ) {
        /* This page is not usable nor used, then do nothing. */
        return -1;
    }

    /* Get the architecture-specific kernel memory manager */
    akmem = (struct arch_kmem_space *)kmem->space->arch;

    /* Index to page directory */
    idxpd = (vaddr >> 30);
    /* Index to page table */
    idxp = (vaddr >> 21) & 0x1ff;

    /* The virtual address must be in the range of kernel memory space */
    if ( 3 != idxpd ) {
        return -1;
    }

    /* Kernel page size must be 2 MiB for this architecture. */
#if (SUPERPAGESIZE) != 0x00200000ULL
#error "SUPERPAGESIZE must be 2 MiB."
#endif

    /* Check the physical and virtual addresses are aligned */
    if ( 0 != (paddr % SUPERPAGESIZE) || 0 != (vaddr % SUPERPAGESIZE) ) {
        /* Invalid physical or virtual address */
        return -1;
    }

    /* Check whether the page presented */
    if ( VMEM_IS_PRESENT(akmem->pd->entries[idxp])
         && !VMEM_IS_PAGE(akmem->pd->entries[idxp]) ) {
        /* This entry is already presented but not a page, then it is a
           directory to 4 KiB pages.  Return an error because 4 KiB paging is
           not (currently) supported in the kernel memory. */
        return -1;
    }

    /* Remapping the page in the page table */
    akmem->pd->entries[idxp] = KMEM_PG_GRW(paddr);

    /* Invalidate the page to activate it */
    invlpg((void *)vaddr);

    return 0;
}

/*
 * Unmap a virtual page (in a superpage granularity)
 */
static int
_kmem_unmap(struct kmem *kmem, u64 vaddr)
{
    struct arch_kmem_space *akmem;
    int idxpd;
    int idxp;

    /* Get the architecture-specific kernel memory manager */
    akmem = (struct arch_kmem_space *)kmem->space->arch;

    /* Index to page directory */
    idxpd = (vaddr >> 30);
    /* Index to page table */
    idxp = (vaddr >> 21) & 0x1ff;

    /* The virtual address must be in the range of kernel memory space */
    if ( 3 != idxpd ) {
        return -1;
    }

    /* Kernel page size must be 2 MiB for this architecture. */
#if (SUPERPAGESIZE) != 0x00200000ULL
#error "SUPERPAGESIZE must be 2 MiB."
#endif

    /* Check the virtual address is aligned */
    if ( 0 != (vaddr % SUPERPAGESIZE) ) {
        /* Invalid physical or virtual address */
        return -1;
    }

    /* Check whether the page presented */
    if ( !VMEM_IS_PRESENT(akmem->pd->entries[idxp])
         || !VMEM_IS_PAGE(akmem->pd->entries[idxp]) ) {
        /* Not a page, then return an error */
        return -1;
    }

    /* Remapping the page in the page table */
    akmem->pd->entries[idxp] = 0;

    /* Invalidate the page to activate it */
    invlpg((void *)vaddr);

    return 0;
}

/*
 * Initialize physical memory
 */
static int
_pmem_init_stage2(struct kmem *kmem)
{
    int ret;

    /* Initialize all usable pages with the buddy system */
    ret = _pmem_buddy_init(kmem->pmem);
    if ( ret < 0 ) {
        return -1;
    }

    return 0;
}

/*
 * Construct buddy system for physical memory
 */
static int
_pmem_buddy_init(struct pmem *pmem)
{
    u64 i;
    u64 j;
    int o;
    int z;

    /* Reset it */
    for ( i = 0; i < PMEM_NUM_ZONES; i++ ) {
        for ( j = 0; j <= PMEM_MAX_BUDDY_ORDER; j++ ) {
            pmem->zones[i].buddy.heads[j] = PMEM_INVAL_INDEX;
        }
    }

    for ( i = 0; i < pmem->nr; i += (1ULL << o) ) {
        /* Find the maximum contiguous usable pages fitting to the alignment of
           the buddy system */
        o = _pmem_buddy_order(pmem, i);
        if ( o < 0 ) {
            /* This page is not usable, then skip it. */
            o = 0;
        } else {
            /* This page is usable, then set the order to all the pages. */
            for ( j = 0; j < (1ULL << o); j++ ) {
                pmem->pages[i + j].order = o;
            }
            /* Get the zone */
            z = pmem->pages[i].zone;

            /* Add this to the buddy system at the order of o */
            pmem->pages[i].next = pmem->zones[z].buddy.heads[o];
            pmem->zones[z].buddy.heads[o] = i;
        }
    }

    return 0;
}

/*
 * Count the physical memory order for buddy system
 */
static int
_pmem_buddy_order(struct pmem *pmem, size_t pg)
{
    int o;
    size_t i;
    int zone;

    /* Check the pg argument within the range of the physical memory space */
    if ( pg >= pmem->nr ) {
        return -1;
    }

    /* Get the zone of the first page */
    zone = pmem->pages[pg].zone;

    /* Check the order for contiguous usable pages */
    for ( o = 0; o <= PMEM_MAX_BUDDY_ORDER; o++ ) {
        for ( i = pg; i < pg + (1ULL << o); i++ ) {
            if ( !PMEM_IS_FREE(&pmem->pages[i])
                 || zone != pmem->pages[i].zone ) {
                /* It contains an unusable page or a page of different zone,
                   then return the current order minus 1, immediately. */
                return o - 1;
            }
        }
        /* Test whether the next order is feasible; feasible if it is properly
           aligned and the pages are within the range of this zone. */
        if ( 0 != (pg & (1ULL << o)) || pg + (1ULL << (o + 1)) > pmem->nr ) {
            /* Infeasible, then return the current order immediately */
            return o;
        }
    }

    /* Return the maximum order */
    return PMEM_MAX_BUDDY_ORDER;
}

/*
 * Resolve the zone of the page (physical address)
 */
static __inline__ int
_pmem_page_zone(void *page, int prox)
{
    if ( (u64)page < 0x1000000ULL ) {
        /* DMA */
        return PMEM_ZONE_DMA;
    } else if ( (u64)page < 0x100000000ULL ) {
        /* Low address memory space */
        return PMEM_ZONE_LOWMEM;
    } else {
        /* High address memory space */
        if ( prox >= 0 ) {
            return PMEM_ZONE_NUMA(prox);
        } else {
            return PMEM_ZONE_UMA;
        }
    }
}

/*
 * Enable the global page feature
 */
static void
_enable_page_global(void)
{
    /* Enable the global page feature */
    set_cr4(get_cr4() | CR4_PGE);
}

/*
 * Disable the global page feature
 */
static void
_disable_page_global(void)
{
    /* Disable the global page feature */
    set_cr4(get_cr4() & ~CR4_PGE);
}

/*
 * Map a virtual page to a physical page
 */
int
arch_vmem_map(struct vmem_space *space, void *vaddr, void *paddr, int flags)
{
    struct arch_vmem_space *avmem;
    int idxpd;
    int idxp;
    int idx;
    u64 *pt;
    u64 *vpt;

    /* Check the flags */
    if ( !(VMEM_USABLE & flags) || !(VMEM_USED & flags) ) {
        /* This page is not usable nor used, then do nothing. */
        return -1;
    }

    /* Get the architecture-specific kernel memory manager */
    avmem = (struct arch_vmem_space *)space->arch;

    /* Index to page directory */
    idxpd = ((u64)vaddr >> 30);
    if ( idxpd >= 4 ) {
        return -1;
    }
    /* Index to page table */
    idxp = ((u64)vaddr >> 21) & 0x1ff;
    /* Index to page entry */
    idx = ((u64)vaddr >> 12) & 0x1ffULL;

    /* Superpage or page? */
    if ( VMEM_SUPERPAGE & flags ) {
        /* Superpage */
        /* Check the physical address argument */
        if ( 0 != ((u64)paddr % SUPERPAGESIZE)
             || 0 != ((u64)vaddr % SUPERPAGESIZE) ) {
            /* Invalid physical address */
            return -1;
        }

        /* Check whether the page presented */
        if ( VMEM_IS_PRESENT(VMEM_PD(avmem->array, idxpd)[idxp])
             && !VMEM_IS_PAGE(VMEM_PD(avmem->array, idxpd)[idxp]) ) {
            /* Present and 4 KiB paging, then remove the descendant table */
            pt = VMEM_PT(VMEM_PD(avmem->array, idxpd)[idxp]);
            vpt = VMEM_PT(avmem->vls[idxpd][idxp]);

            /* Delete descendant table */
            _kmem_mm_page_free(g_kmem, vpt);
        }

        /* Remapping */
        if ( flags & VMEM_GLOBAL ) {
            VMEM_PD(avmem->array, idxpd)[idxp] = VMEM_PG_GRW((u64)paddr);
            avmem->vls[idxpd][idxp] = VMEM_PG_GRW((u64)vaddr);
        } else {
            VMEM_PD(avmem->array, idxpd)[idxp] = VMEM_PG_RW((u64)paddr);
            avmem->vls[idxpd][idxp] = VMEM_PG_RW((u64)vaddr);
        }

        /* Invalidate the page */
        invlpg((void *)vaddr);
    } else {
        /* Page */
        /* Check the physical address argument */
        if ( 0 != ((u64)paddr % PAGESIZE) || 0 != ((u64)vaddr % PAGESIZE) ) {
            /* Invalid physical address */
            return -1;
        }

        /* Check whether the page presented */
        if ( !VMEM_IS_PRESENT(VMEM_PD(avmem->array, idxpd)[idxp])
             || VMEM_IS_PAGE(VMEM_PD(avmem->array, idxpd)[idxp]) ) {
            /* Not present or 2 MiB page, then create a new page table */
            vpt = _kmem_mm_page_alloc(g_kmem);
            if ( NULL == vpt ) {
                return -1;
            }
            /* Get the virtual address */
            pt = arch_kmem_addr_v2p(g_kmem, vpt);

            /* Update the entry */
            VMEM_PD(avmem->array, idxpd)[idxp] = VMEM_DIR_RW((u64)pt);
            avmem->vls[idxpd][idxp] = VMEM_DIR_RW((u64)vpt);
        } else {
            /* Directory */
            pt = VMEM_PT(VMEM_PD(avmem->array, idxpd)[idxp]);
            vpt = VMEM_PT(avmem->vls[idxpd][idxp]);
        }

        /* Remapping */
        if ( flags & VMEM_GLOBAL ) {
            vpt[idx] = VMEM_PG_GRW((u64)paddr);
        } else {
            vpt[idx] = VMEM_PG_RW((u64)paddr);
        }

        /* Invalidate the page */
        invlpg((void *)vaddr);
    }

    return 0;
}

/*
 * Map a virtual page to a physical page (in a superpage granularity)
 */
int
arch_kmem_map(struct kmem *kmem, void *vaddr, void *paddr, int flags)
{
    return _kmem_map(kmem, (reg_t)vaddr, (reg_t)paddr, flags);
}

/*
 * Unmap a virtual page (in a superpage granularity)
 */
int
arch_kmem_unmap(struct kmem *kmem, void *vaddr)
{
    return _kmem_unmap(kmem, (reg_t)vaddr);
}

void *
arch_kmem_addr_v2p(struct kmem *kmem, void *vaddr)
{
    struct arch_kmem_space *akmem;
    int idxpd;
    int idxp;
    //int idx;
    reg_t off;

    /* Get the architecture-specific kernel memory manager */
    akmem = (struct arch_kmem_space *)kmem->space->arch;

    /* Index to page directory */
    idxpd = ((reg_t)vaddr >> 30);
    /* Index to page table */
    idxp = ((reg_t)vaddr >> 21) & 0x1ff;
    /* Index to page entry */
    //idx = ((reg_t)vaddr >> 12) & 0x1ffULL;

    /* The virtual address must be in the range of kernel memory space */
    if ( 3 != idxpd ) {
        return NULL;
    }

    /* Kernel page size must be 2 MiB for this architecture. */
    if ( 0x00200000ULL != SUPERPAGESIZE ) {
        return NULL;
    }

    /* Check whether the page presented */
    if ( VMEM_IS_PRESENT(akmem->pd->entries[idxp])
         && !VMEM_IS_PAGE(akmem->pd->entries[idxp]) ) {
        /* 4 KiB paging is not supported in the kernel memory */
        return NULL;
    }

    /* Remapping */
    off = ((reg_t)vaddr) & 0x1fffffULL;
    return (void *)(VMEM_PDPG((reg_t)akmem->pd->entries[idxp]) + off);
}

/*
 * Get the address width
 */
int
arch_address_width(void)
{
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;

    /* Get the physical-address width */
    rax = cpuid(0x80000008, &rbx, &rcx, &rdx);

    return rax & 0xff;
}

/*
 * Resolve the physical address of the specified virtual address in a virtual
 * memory space
 */
void *
arch_vmem_addr_v2p(struct vmem_space *space, void *vaddr)
{
    struct arch_vmem_space *avmem;
    int idxpd;
    int idxp;
    int idx;
    u64 *pt;
    u64 off;

    /* Get the architecture-specific data structure */
    avmem = (struct arch_vmem_space *)space->arch;

    /* Index to page directory */
    idxpd = ((reg_t)vaddr >> 30);
    if ( idxpd >= avmem->nr ) {
        /* Oversized */
        return NULL;
    }
    /* Index to page table */
    idxp = ((reg_t)vaddr >> 21) & 0x1ff;
    /* Index to page entry */
    idx = ((reg_t)vaddr >> 12) & 0x1ffULL;

    if ( VMEM_IS_PAGE(avmem->vls[idxpd][idxp]) ) {
        /* 2-MiB paging */
        /* Get the offset */
        off = ((reg_t)vaddr) & 0x1fffffULL;
        return (void *)(VMEM_PDPG(VMEM_PD(avmem->array, idxpd)[idxp]) + off);
    } else {
        /* 4-KiB paging */
        pt = VMEM_PT(avmem->vls[idxpd][idxp]);
        /* Get the offset */
        off = ((reg_t)vaddr) & 0xfffULL;
        return (void *)(VMEM_PT(pt[idx]) + off);
    }
}

/*
 * Initialize the architecture-specific virtual memory
 */
#define VMEM_NPD    6
int
arch_vmem_init(struct vmem_space *space)
{
    struct arch_vmem_space *avmem;
    struct arch_kmem_space *tmp;
    u64 *vpg;
    u64 *vls;
    u64 *paddr;
    ssize_t i;

    avmem = kmalloc(sizeof(struct arch_vmem_space));
    if ( NULL == avmem ) {
        return -1;
    }
    avmem->array = kmalloc(sizeof(u64 *) * (VMEM_NENT(VMEM_NPD) + VMEM_NPD));
    if ( NULL == avmem->array ) {
        kfree(avmem);
        return -1;
    }
    avmem->vls = kmalloc(sizeof(u64 *) * VMEM_NPD);
    if ( NULL == avmem->vls ) {
        kfree(avmem->array);
        kfree(avmem);
        return -1;
    }
    avmem->nr = VMEM_NPD;

    kmemset(avmem->array, 0, sizeof(u64 *) * (VMEM_NENT(VMEM_NPD) + VMEM_NPD));
    kmemset(avmem->vls, 0, sizeof(u64 *) * VMEM_NPD);

    vpg = kmalloc(PAGESIZE * (VMEM_NENT(VMEM_NPD) + VMEM_NPD));
    if ( NULL == vpg ) {
        kfree(avmem->vls);
        kfree(avmem->array);
        kfree(avmem);
        return -1;
    }
    vls = kmalloc(PAGESIZE * VMEM_NPD);
    if ( NULL == vls ) {
        kfree(vpg);
        kfree(avmem->vls);
        kfree(avmem->array);
        kfree(avmem);
        return -1;
    }
    kmemset(vpg, 0, PAGESIZE * (VMEM_NENT(VMEM_NPD) + VMEM_NPD));
    kmemset(vls, 0, PAGESIZE * VMEM_NPD);

    /* Set physical addresses to page directories */
    avmem->pgt = arch_kmem_addr_v2p(g_kmem, vpg);

    VMEM_PML4(avmem->array) = vpg;
    VMEM_PDPT(avmem->array, 0) = vpg + 512;
    for ( i = 0; i < VMEM_NPD; i++ ) {
        VMEM_PD(avmem->array, i) = vpg + 1024 + 512 * i;
    }

    /* Page directories with virtual address */
    for ( i = 0; i < VMEM_NPD; i++ ) {
        avmem->vls[i] = vls + 512 * i;
    }

    /* Setup physical page table */
    paddr = arch_kmem_addr_v2p(g_kmem, VMEM_PDPT(avmem->array, 0));
    vpg[0] = VMEM_DIR_RW((u64)paddr);
    for ( i = 0; i < VMEM_NPD; i++ ) {
        paddr = arch_kmem_addr_v2p(g_kmem, VMEM_PD(avmem->array, i));
        vpg[512 + i] = VMEM_DIR_RW((u64)paddr);
    }

    /* Set the kernel region */
    //tmp = g_kmem->space->arch;
    //paddr = arch_kmem_addr_v2p(g_kmem, VMEM_PD(tmp->array, 0));
    //vpg[512] = KMEM_DIR_RW((u64)paddr);
    tmp = g_kmem->space->arch;
    vpg[512 + 3] = tmp->pdpt->entries[3];
    //paddr = arch_kmem_addr_v2p(g_kmem, VMEM_PD(tmp->array, 4));
    //vpg[512 + 4] = KMEM_DIR_RW((u64)paddr);
    //avmem->vls[0] = tmp->vls[0];
    avmem->vls[3] = tmp->pdpt->entries;
    //avmem->vls[4] = tmp->vls[4];

    /* Set the architecture-specific data structure to its parent */
    space->arch = avmem;

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
