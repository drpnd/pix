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
#include "kernel.h"
#include "elf.h"

int execve_mem(const char *, void *, size_t, char *const [], char *const []);
int execve_elf(const char *, void *, size_t, char *const [], char *const []);
int execve_raw(const char *, void *, size_t, char *const [], char *const []);

/*
 * Execute a file
 */
int
kexecve(const char *path, char *const argv[], char *const envp[])
{
    u64 *initramfs = (u64 *)(INITRAMFS_BASE + 0xc0000000);
    u64 offset = 0;
    u64 size;
    struct ktask *t;
    const char *name;
    ssize_t i;
    struct proc *proc;
    void *data;

    /* Find the file pointed by path from the initramfs to load the program
       file */
    while ( 0 != *initramfs ) {
        if ( 0 == kstrcmp((char *)initramfs, path) ) {
            offset = *(initramfs + 2);
            size = *(initramfs + 3);
            break;
        }
        initramfs += 4;
    }
    if ( 0 == offset ) {
        /* Could not find the file */
        return -1;
    }

    /* Program file */
    data = (void *)(INITRAMFS_BASE + 0xc0000000 + offset);

    /* Get the name of the new process */
    name = path;
    for ( i = kstrlen(path) - 1; i >= 0; i-- ) {
        if ( '/' == path[i] ) {
            name = path + i + 1;
            break;
        }
    }

    return execve_mem(name, data, size, argv, envp);

    /* Get the task and the corresponding process currently running on this
       processor */
    t = this_ktask();
    proc = t->proc;

    /* Get the name of the new process */
    name = path;
    for ( i = kstrlen(path) - 1; i >= 0; i-- ) {
        if ( '/' == path[i] ) {
            name = path + i + 1;
            break;
        }
    }
    /* Replace the process name with the new process */
    kstrlcpy(proc->name, name, kstrlen(name) + 1);

    /* Execute the process */
    //arch_exec(t->arch, (void *)(INITRAMFS_BASE + 0xc0000000 + offset), size,
    //          KTASK_POLICY_USER, argv, envp);

    /* On failure */
    return -1;
}

int
execve_mem(const char *name, void *data, size_t size, char *const argv[],
           char *const envp[])
{
    if ( 0 == kmemcmp(data, "\x7f\x45\x4c\x46", 4) ) {
        /* Presumed to be ELF */
        return execve_elf(name, data, size, argv, envp);
    } else {
        /* Not ELF */
        return execve_raw(name, data, size, argv, envp);
    }

    return -1;
}

int
execve_elf(const char *name, void *data, size_t size, char *const argv[],
           char *const envp[])
{
    return -1;
}

int arch_exec(void *, void (*)(void), size_t, int, char *const [],
              char *const []);
int
execve_raw(const char *name, void *data, size_t size, char *const argv[],
           char *const envp[])
{
    struct ktask *t;
    struct proc *proc;
    void *code;

    /* Allocate memory for the code */
    code = kmalloc(size);
    kfree(code);

    /* Get the task and the corresponding process currently running on this
       processor */
    t = this_ktask();
    proc = t->proc;

    /* Replace the process name with the new process */
    kstrlcpy(proc->name, name, kstrlen(name) + 1);

    /* Execute the process */
    arch_exec(t->arch, data, size, KTASK_POLICY_USER, argv, envp);

    /* On failure */
    return -1;
}



/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
