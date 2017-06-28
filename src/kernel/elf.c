/*_
 * Copyright (c) 2017 Hirochika Asai <asai@jar.jp>
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
#include <sys/syscall.h>
#include "kernel.h"


typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;
/* and unsigned char */

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;
/* and unsigned char */

typedef struct {
    unsigned char       e_ident[16]; /* Elf Identification */
    Elf64_Half          e_type;      /* Object file type */
    Elf64_Half          e_machine;   /* Machine type */
    Elf64_Word          e_version;   /* Object file version */
    Elf64_Addr          e_entry;     /* Entry point address */
    Elf64_Off           e_phoff;     /* Program header offset */
    Elf64_Off           e_shoff;     /* Section header offset */
    Elf64_Word          e_flags;     /* Processor-specific flags */
    Elf64_Half          e_ehsize;    /* ELF header size */
    Elf64_Half          e_phentsize; /* Size of program header entry */
    Elf64_Half          e_phnum;     /* Number of program header entries */
    Elf64_Half          e_shentsize; /* Size of section header entry */
    Elf64_Half          e_shnum;     /* Number of section header entries */
    Elf64_Half          e_shstrndx;  /* Section name string table index */
} __attribute__ ((packed)) Elf64_Ehdr;

enum {
    EI_MAG0 = 0,                /* File identification */
    EI_MAG1 = 1,
    EI_MAG2 = 2,
    EI_MAG3 = 3,
    EI_CLASS = 4,               /* File class */
    EI_DATA = 5,                /* Data encoding */
    EI_VERSION = 6,             /* File version */
    EI_OSABI = 7,               /* OS/ABI identification */
    EI_ABIVERSION = 8,          /* ABI version */
    EI_PAD = 9,                 /* Start of padding bytes */
    EI_NIDENT = 16,             /* Size of e_ident[] */
};

/* Object File Classes, e_ident[EI_CLASS] */
enum {
    ELFCLASS32 = 1,             /* 32-bit objects */
    ELFCLASS64 = 2,             /* 64-bit objects */
};

/* Data Encodings, e_ident[EI_DATA] */
enum {
    ELFDATA2LSB = 1,            /* Object file data structures are little-
                                   endian */
    ELFDATA2MSB = 2,            /* Object file data structures are big-endian */
};

#define EV_CURRENT      1

/* Operating System and ABI Identifiers, e_ident[EI_OSABI] */
enum {
    ELFOSABI_SYSV = 0,          /* System V ABI */
    ELFOSABI_HPUX = 1,          /* HP-UX operating system */
    ELFOSABI_STANDALONE = 255,  /* Standalone (embedded) application */
};

/* e_machine */
enum {
    EM_X86 = 0x03,
    EM_ARM = 0x28,
    EM_X86_64 = 0x3e,
    EM_AARCH64 = 0xb7,
};

/* Object File Types, e_type */
enum {
    ET_NONE = 0,                /* No file type */
    ET_REL = 1,                 /* Relocatable object file */
    ET_EXEC = 2,                /* Executable file */
    ET_DYN = 3,                 /* Shared object file */
    ET_CORE = 4,                /* Core file */
    ET_LOOS = 0xfe00,           /* Environment-specific use */
    ET_HIOS = 0xfeff,
    ET_LOPROC = 0xff00,         /* Processor-specific use */
    ET_HIPROC = 0xffff,
};

/* Special section indices */
enum {
    SHN_UNDEF = 0,              /* Used to mark an undefined or meaningless
                                   section reference */
    SHN_LOPROC = 0xff00,        /* Processor-specific use */
    SHN_HIPROC = 0xff1f,
    SHN_LOOS = 0xff20,          /* Environment-specific use */
    SHN_HIOS = 0xff3f,
    SHN_ABS = 0xfff1,           /* Indices that the corresponding reference is
                                   an absolute value */
    SHN_COMMON = 0xfff2,        /* Indicates a symbol that has been declared as
                                   a common block (Fortran COMMON or C tentative
                                   declaration) */
};

/* Section header entries */
typedef struct {
    Elf64_Word          sh_name;        /* Section name */
    Elf64_Word          sh_type;        /* Section type */
    Elf64_Xword         sh_flags;       /* Section attributes */
    Elf64_Addr          sh_addr;        /* Virtual address in memory */
    Elf64_Off           sh_offset;      /* Offset in file */
    Elf64_Xword         sh_size;        /* Size of section */
    Elf64_Word          sh_link;        /* Link to other section */
    Elf64_Word          sh_info;        /* Miscellaneous information */
    Elf64_Xword         sh_addralign;   /* Address alignment boundary */
    Elf64_Xword         sh_entsize;     /* Size of entries, if section has
                                           table */
} __attribute__ ((packed)) Elf64_Shdr;

/* Section Types, sh_type */
enum {
    SHT_NULL = 0,               /* Marks an unused section header */
    SHT_PROGBITS = 1,           /* Contains information defined by the
                                   program */
    SHT_SYMTAB = 2,             /* Contains a linker symbol table */
    SHT_STRTAB = 3,             /* Contains a string table */
    SHT_RELA = 4,               /* Contains "Rela" type relocation entries */
    SHT_HASH = 5,               /* Contains a symbol hash table */
    SHT_DYNAMIC = 6,            /* Contains dynamic linking tables */
    SHT_NOTE = 7,               /* Contains note information */
    SHT_NOBITS = 8,             /* Contains uninitialized space; does not occupy
                                   any space in the file */
    SHT_REL = 9,                /* Contains "Rel" type relocation entries */
    SHT_SHLIB = 10,             /* Reserved */
    SHT_DYNSYM = 11,            /* Contains a dynamic loader symbol table */
    SHT_LOOS = 0x60000000,      /* Environment-specific use */
    SHT_HIOS = 0x6fffffff,
    SHT_LOPROC = 0x70000000,    /* Processor-specific use */
    SHT_HIPROC = 0x7fffffff,
};

/* Section Attributes, sh_flags */
enum {
    SHF_WRITE = 0x1,            /* Section contains writable data */
    SHF_ALLOC = 0x2,            /* Section is allocated in memory image of
                                   program */
    SHF_EXECINSTR = 0x4,        /* Section contains executable instructions */
    SHF_MASKOS = 0x0f000000,    /* Environment-specific use */
    SHF_MASKPROC = 0xf0000000,  /* Processor-specific use */
};

/* Standard sections
   Section Name         Section Type    Flags
   .bss                 SHT_NOBITS      A, W    Uninitialized data
   .data                SHT_PROGBITS    A, W    Initialized data
   .interp              SHT_PROGBITS    [A]     Program interpreter path name
   .rodata              SHT_PROGBITS    A       Read-only data
   .text                SHT_PROGBITS    A, X    Executable code
   .comment             SHT_PROGBITS    none    Version control information
   .dynamic             SHT_DYNAMIC     A[, W]  Dynamic linking tables
   .dynstr              SHT_STRTAB      A       String table for .dynamic
   .dynsym              SHT_DYNSYM      A       Symbol table for dynamic linking
   etc...
 */

/* Symbol table */
typedef struct {
    Elf64_Word          st_name;        /* Symbol name */
    unsigned char       st_info;        /* Type and Binding attributes */
    unsigned char       st_other;       /* Reserved */
    Elf64_Half          st_shndx;       /* Section table index */
    Elf64_Addr          st_value;       /* Symbol value */
    Elf64_Xword         st_size;        /* Sizeof object (e.g., common) */
} __attribute__ ((packed)) Elf64_Sym;

/* Symbol Bindings */
enum {
    STB_LOCAL = 0,              /* Not visible outside the object file */
    STB_GLOBAL = 1,             /* Global symbol, visible to all object files */
    STB_WEAK = 2,               /* Global scope, but with lower precedence than
                                   global symbols */
    STB_LOOS = 10,              /* Environement-specific use */
    STB_HIOS = 12,
    STB_LOPROC = 13,            /* Processor-specific use */
    STB_HIPROC = 15,
};

/* Symbol Types */
enum {
    STT_NOTYPE = 0,             /* No type specified (e.g., an absolute
                                   symbol) */
    STT_OBJECT = 1,             /* Data object */
    STT_FUNC = 2,               /* Function entry point */
    STT_SECTION = 3,            /* Symbol is associated with a section */
    STT_FILE = 4,               /* Source file associated with the object
                                   file */
    STT_LOOS = 10,              /* Environement-specific use */
    STT_HIOS = 12,
    STT_LOPROC = 13,            /* Processor-specific use */
    STT_HIPROC = 15,
};

/* ELF-64 Relocation Entries */
typedef struct {
    Elf64_Addr          r_offset;       /* Address of reference */
    Elf64_Xword         r_info;         /* Symbol index and type of
                                           relocation */
} __attribute__ ((packed)) Elf64_Rel;
typedef struct {
    Elf64_Addr          r_offset;       /* Address of reference */
    Elf64_Xword         r_info;         /* Symbol index and type of
                                           relocation */
    Elf64_Sxword        r_addend;       /* Constant part of expression */
} __attribute__ ((packed)) Elf64_Rela;

/* Program header table entry */
typedef struct {
    Elf64_Word          p_type;         /* Type of segment */
    Elf64_Word          p_flags;        /* Segment attributes */
    Elf64_Off           p_offset;       /* Offset in file */
    Elf64_Addr          p_vaddr;        /* Virtual address in memory */
    Elf64_Addr          p_paddr;        /* Reserved */
    Elf64_Xword         p_filesz;       /* Size of segment in file */
    Elf64_Xword         p_memsz;        /* Size of segment in memory */
    Elf64_Xword         p_align;        /* Alignment of segment */
} __attribute__ ((packed)) Elf64_Phdr;

/* Segment Types, p_type */
enum {
    PT_NULL = 0,                /* Unused entry */
    PT_LOAD = 1,                /* Loadable segment */
    PT_DYNAMIC = 2,             /* Dynamic linking tables */
    PT_INTERP = 3,              /* Program interpreter path name */
    PT_NOTE = 4,                /* Note sections */
    PT_SHLIB = 5,               /* Reserved */
    PT_PHDR = 6,                /* Program header table */
    PT_GNU_STACK = 0x6474e551,  /* GNU Stack */
    PT_LOOS = 0x60000000,       /* Environement-specific use */
    PT_HIOS = 0x6fffffff,
    PT_LOPROC = 0x70000000,     /* Processor-specific use */
    PT_HIPROC = 0x7fffffff,
};

/* Segment Attributes, p_flags */
enum {
    PF_X = 0x1,                 /* Execute permission */
    PF_W = 0x2,                 /* Write permission */
    PF_R = 0x4,                 /* Read permission */
    PF_MASKOS = 0x00ff0000,     /* These flag bits are reserved for
                                   environment-specific use */
    PF_MASKPROC = 0xff000000,   /* These flag bits are reserved for
                                   processor-specific use */
};

/* Dynamic table */
typedef struct {
    Elf64_Sxword        d_tag;
    union {
        Elf64_Xword     d_val;
        Elf64_Addr      d_ptr;
    } d_un;
} Elf64_Dyn;

/* Dynamic table entries */
enum {
    DT_NULL = 0,                /* d_un = ignored; Marks the end of the dynamic
                                   array */
    DT_NEEDED = 1,              /* d_un = d_val; The string table offset of the
                                   name of a needed library */
    DT_PLTRELSZ = 2,            /* d_un = d_val; Total size, in bytes, of the
                                   relocation entries associated with the
                                   procedure linkage table */
    DT_PLTGOT = 3,
    DT_HASH = 4,
    DT_STRTAB = 5,
    DT_SYMTAB = 6,
    DT_RELA = 7,
    DT_RELASZ = 8,
    DT_RELAENT = 9,
    DT_STRSZ = 10,
    DT_SYMENT = 11,
    DT_INIT = 12,
    DT_FINI = 13,
    DT_SONAME = 14,
    DT_RPATH = 15,
    DT_SYMBOLIC = 16,
    DT_REL = 17,
    DT_RELSZ = 18,
    DT_RELENT = 19,
    DT_PLTREL = 20,
    DT_DEBUG = 21,
    DT_TEXTREL = 22,
    DT_JMPREL = 23,
    DT_BIND_NOW = 24,
    DT_INIT_ARRAY = 25,
    DT_FINI_ARRAY = 26,
    DT_INIT_ARRAYSZ = 27,
    DT_FINI_ARRAYSZ = 28,
    DT_LOOS = 0x60000000,
    DT_HIOS = 0x6fffffff,
    DT_LOPROC = 0x70000000,
    DT_HIPROC = 0x7fffffff,
};


/*
 * Compute hash value
 */
unsigned long
elf64_hash(const unsigned char *name)
{
    unsigned long h;
    unsigned long g;

    h = 0;
    while ( *name ) {
        h = (h << 4) + *name++;
        g = h & 0xf0000000;
        if ( g ) {
            h ^= g >> 24;
            h &= 0x0fffffff;
        }
    }

    return h;
}

/*
 * Load ELF64 program
 */
int
elf64_load2(void *data, size_t len)
{
    Elf64_Ehdr *hdr;

    /* Header */
    hdr = (Elf64_Ehdr *)data;

    /* Check the encoding */
    switch ( hdr->e_ident[EI_DATA] ) {
    case ELFDATA2LSB:
    case ELFDATA2MSB:
        break;
    default:
        return -1;
    }

    /* Check the ABI */
    switch ( hdr->e_ident[EI_OSABI] ) {
    case ELFOSABI_SYSV:
        break;
    case ELFOSABI_HPUX:
    case ELFOSABI_STANDALONE:
    default:
        return -1;
    }

    /* Validate the file version */
    switch ( hdr->e_ident[EI_VERSION] ) {
    case EV_CURRENT:
        break;
    default:
        return -1;
    }

    /* Check the type */
    switch ( hdr->e_type ) {
    default:
        return -1;
    }

}

/*
 * Load ELF program
 */
int
elf_load2(void *data, size_t len)
{
    Elf64_Ehdr *hdr;

    hdr = (Elf64_Ehdr *)data;

    /* Validate magic */
    if ( 0 != kmemcmp(hdr->e_ident, "\x7f\x45\x4c\x46", 4) ) {
        return -1;
    }
    switch ( hdr->e_ident[EI_CLASS]  ) {
    case ELFCLASS32:
        return -1;
    case ELFCLASS64:
        return elf64_load2(data, len);
    }

    return -1;
}

/*
 * Load ELF64 program
 */
int
elft64_load(uint8_t *data, size_t len)
{
    int i;
    Elf64_Ehdr *hdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shdr;
    char *shstrn;

    /* Check ELF header length */
    if ( len < sizeof(Elf64_Ehdr) ) {
        /* Invalid ELF file */
        return -1;
    }
    hdr = (Elf64_Ehdr *)data;

    /* Check the magic bytes */
    if ( 0 != kmemcmp(hdr->e_ident, "\x7f\x45\x4c\x46", 4) ) {
        /* Invalid magic */
        return -1;
    }

    /* Check class */
    if ( ELFCLASS64 != hdr->e_ident[EI_CLASS] ) {
        /* Only 64-bit objects are supported. */
        return -1;
    }

    /* Check the encoding */
    if ( ELFDATA2LSB != hdr->e_ident[EI_DATA] ) {
        /* 2's complement and little endian are supported. */
        return -1;
    }

    /* Check the ABI */
    if ( ELFOSABI_SYSV != hdr->e_ident[EI_OSABI] ) {
        /* Must be SYSV */
        return -1;
    }

    /* Validate the file version */
    if ( EV_CURRENT != hdr->e_ident[EI_VERSION] ) {
        /* Invalid verison */
        return -1;
    }

    /* Check the machine archtecture */
    if ( EM_X86_64 != hdr->e_machine ) {
        return -1;
    }

    /* Check the object file type */
    if ( ET_EXEC != hdr->e_type ) {
        return -1;
    }

    /* Size of this ELF header */
    if ( hdr->e_ehsize < sizeof(Elf64_Ehdr)) {
        return -1;
    }

    /* Parse program headers */
    if ( len < hdr->e_phoff + hdr->e_phentsize * hdr->e_phnum ) {
        return -1;
    }
    for ( i = 0; i < hdr->e_phnum; i++ ) {
        phdr = (Elf64_Phdr *)(data + hdr->e_phoff + hdr->e_phentsize * i);
        switch ( phdr->p_type ) {
        case PT_NULL:
            break;
        case PT_LOAD:
            /* Loadable segment */
            phdr->p_offset;
            //hdr->e_phentsize == sizeof(Elf64_Phdr)
            //phdr->p_filesz <= phdr->p_memsz;
            break;
        case PT_GNU_STACK:
            /* GNU Stack; p_flags sets the permission of the stack */
            break;
        default:
            /* Unknown type */
            break;
        }
    }

    /* Section name string table */
    shstrn = NULL;
    if ( SHN_UNDEF != hdr->e_shstrndx ) {
        shdr = (Elf64_Shdr *)(data + hdr->e_shoff + hdr->e_shentsize
                              * hdr->e_shstrndx);
        if ( SHT_STRTAB != shdr->sh_type ) {
            return -1;
        }
        shstrn = (char *)(data + shdr->sh_offset);
    }

    /* Section headers */
    for ( i = 0; i < hdr->e_shnum; i++ ) {
        shdr = (Elf64_Shdr *)(data + hdr->e_shoff + hdr->e_shentsize * i);
        shdr->sh_name;
        shdr->sh_type;
        shdr->sh_offset;
        shdr->sh_size;
        //printf(" %s\n", shstrn + shdr->sh_name);
    }

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
