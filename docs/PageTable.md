# Page Table

## x86-64 (proposed; not yet implemented)
     PML4              PDPT             PD (2 MiB paging)
    +--------------+     +--------------+     +--------------+
    | 0-512 GiB    |---->|              |---->|              |
    | 512-1024 GiB | -+  |              |     |              |
    +--------------+  |  +--------------+     +--------------+
                      |
                      |  +--------------+     +--------------+
                      +->| Linear       |---->|              |
                         | addressing   |     |              |
                         +--------------+     +--------------+

## Memory map (in x86-64)

### Physical memory statically used by kernel

| Start       | End         | Description                      |
| :---------- | :---------- | :------------------------------- |
| `0000 0000` | `0000 7fff` | Reserved by the boot loader      |
| `0000 8000` | `0000 80ff` | Information from the boot loader |
| `0000 8100` | `0000 8fff` | System memory map                |
| `0000 9000` | `0000 ffff` | Boot loader code (stage 2)       |
| `0001 0000` | `0002 ffff` | Kernel (128 KiB)                 |
| `0003 0000` | `0006 ffff` | Initramfs (256 KiB)              |
| `0007 0000` | `0007 8fff` | Used by kernel                   |
| `0007 9000` | `0007 ffff` | Bootstrap page table (2M paging) |
| `0008 0000` | `0008 ffff` | Low memory region (*DT, KVAR)    |
| `0009 0000` | `000f ffff` | Trampoline code                  |
| `0010 0000` | `00ff ffff` | Kernel memory (15 MiB)           |
| `0100 0000` | `01ff ffff` | Processor data (16 MiB)          |


### Bootstrap Page Table
* 2-MiB paging


### Kernel virtual memory
| Start       | End         | Description                       |
| :---------- | :---------- | :-------------------------------- |
| `c000 0000` | `c1ff ffff` | Map 0--32 MiB physical memory     |
| `c200 0000` | `---`       | Dynamically assigned by pmem/kmem |
