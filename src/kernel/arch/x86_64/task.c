/*_
 * Copyright (c) 2015-2017 Hirochika Asai <asai@jar.jp>
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
#include "../../kernel.h"
#include "arch.h"
#include "memory.h"

#define XSAVE_SIZE  4096

/* Kernel memory */
//extern struct kmem *g_kmem;

/*
 * Create a new task
 */
struct ktask *
task_new(void)
{
    struct arch_task *t;

    /* Allocate the architecture-specific task structure of a new task */
    t = kmalloc(sizeof(struct arch_task));
    if ( NULL == t ) {
        return NULL;
    }
    /* Create a space for FPU/SSE registers */
    t->xregs = kmalloc(XSAVE_SIZE);
    if ( NULL == t->xregs ) {
        kfree(t);
        return NULL;
    }
    kmemset(t->xregs, 0, XSAVE_SIZE);
    //xsave(t->xregs, 5, 0);
    /* Allocate the kernel task structure of a new task */
    t->kstack = kmalloc(KSTACK_SIZE);
    if ( NULL == t->kstack ) {
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    /* Allocate the user stack of a new task */
    t->ustack = kmalloc(USTACK_SIZE);
    if ( NULL == t->ustack ) {
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    /* Allocate the kernel stack of a new task */
    t->ktask = kmalloc(sizeof(struct ktask));
    if ( NULL == t->ktask ) {
        kfree(t->ustack);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    t->ktask->arch = t;
    t->ktask->proc_task_next = NULL;

    t->sp0 = (u64)t->kstack + KSTACK_SIZE - 16;

    return t->ktask;
}

/*
 * Create a task
 */
struct ktask *
task_create(struct proc *proc, void *(*restart_point)(void *), void *args)
{
    struct arch_task *t;
    void *paddr1;
    ssize_t i;
    int ret;
    struct ktask *kt;
    int tid;

    /* Search an available task ID */
    kt = proc->tasks;
    tid = -1;
    while ( NULL != kt ) {
        if ( kt->id > tid ) {
            tid = kt->id;
        }
        kt = kt->proc_task_next;
    }
    tid++;

    /* Allocate the architecture-specific task structure of a new task */
    t = kmalloc(sizeof(struct arch_task));
    if ( NULL == t ) {
        return NULL;
    }
    /* Create a space for FPU/SSE registers */
    t->xregs = kmalloc(XSAVE_SIZE);
    if ( NULL == t->xregs ) {
        kfree(t);
        return NULL;
    }
    kmemset(t->xregs, 0, XSAVE_SIZE);
    /* Allocate the kernel task structure of a new task */
    t->kstack = kmalloc(KSTACK_SIZE);
    if ( NULL == t->kstack ) {
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    /* Allocate the kernel stack of a new task */
    t->ktask = kmalloc(sizeof(struct ktask));
    if ( NULL == t->ktask ) {
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    kmemset(t->ktask, 0, sizeof(struct ktask));
    t->ktask->arch = t;
    t->ktask->proc = proc;
    t->ktask->state = KTASK_STATE_READY;
    t->ktask->signaled = 0;
    t->ktask->id = tid;
    t->ktask->proc_task_next = proc->tasks;
    t->ktask->proc->tasks = t->ktask;
    t->ktask->next = NULL;
    /* Allocate the user stack of a new task */
    paddr1 = pmem_prim_alloc_superpages(PMEM_ZONE_LOWMEM,
                                        bitwidth(USTACK_SIZE / SUPERPAGESIZE));
    if ( NULL == paddr1 ) {
        kfree(t->ktask);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }

    /* Virtual address of the user stack of this task */
    t->ustack = (void *)(USTACK_INIT - USTACK_SIZE * tid);

    /* Initialize the kernel stack */
    kmemset(t->kstack, 0, KSTACK_SIZE);

    /* FIXME: Tempoary... */
    for ( i = 0; i < (ssize_t)(USTACK_SIZE / SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(proc->vmem, t->ustack + SUPERPAGE_ADDR(i),
                            paddr1 + SUPERPAGE_ADDR(i),
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            panic("FIXME: task_create()");
        }
    }

    /* Reset the user stack */
    kmemset(t->ustack, 0, USTACK_SIZE);

    /* Setup the restart point */
    t->rp = t->kstack + KSTACK_SIZE - 16 - sizeof(struct stackframe64);
    kmemset(t->rp, 0, sizeof(struct stackframe64));
    t->rp->di = (u64)args;
    t->rp->cs = GDT_RING3_CODE64_SEL + 3;
    t->rp->ss = GDT_RING3_DATA64_SEL + 3;
    t->rp->gs = t->rp->ss;
    t->rp->fs = t->rp->ss;
    t->rp->sp = (u64)t->ustack + USTACK_SIZE - 16 - 8;
    t->rp->ip = (u64)restart_point;
    t->rp->flags = 0x3202;

    t->cr3 = ((struct arch_vmem_space *)proc->vmem->arch)->pgt;
    t->sp0 = (u64)t->kstack + KSTACK_SIZE - 16;

    /* Return */
    return t->ktask;
}

/*
 * Clone the task
 */
struct proc *
proc_fork(struct proc *op, struct ktask *ot, struct ktask **ntp)
{
    struct arch_task *t;
    struct proc *np;
    void *paddr1;
    void *paddr2;
    void *exec;
    void *saved_cr3;
    ssize_t i;
    int ret;
    size_t size;

    /* Create a new process */
    np = kmalloc(sizeof(struct proc));
    if ( NULL == np ) {
        return NULL;
    }
    kmemset(np, 0, sizeof(struct proc));
    kmemcpy(np->name, op->name, PATH_MAX);
    np->code_size = op->code_size;

    /* Copy all file descriptors and increment the reference counter */
    for ( i = 0; i < FD_MAX; i++ ) {
        if ( NULL != op->fds[i] ) {
            np->fds[i] = op->fds[i];
            np->fds[i]->refs++;
            /* FIXME: Add new process to the process list associated with the
               file descriptor */
        }
    }

    /* Allocate the architecture-specific task structure of a new task */
    t = kmalloc(sizeof(struct arch_task));
    if ( NULL == t ) {
        kfree(np);
        return NULL;
    }
    /* Create a space for FPU/SSE registers */
    t->xregs = kmalloc(XSAVE_SIZE);
    if ( NULL == t->xregs ) {
        kfree(t);
        kfree(np);
        return NULL;
    }
    kmemset(t->xregs, 0, XSAVE_SIZE);
    kmemcpy(t->xregs, ((struct arch_task *)ot->arch)->xregs, XSAVE_SIZE);
    /* Allocate the kernel task structure of a new task */
    t->kstack = kmalloc(KSTACK_SIZE);
    if ( NULL == t->kstack ) {
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }
    /* Allocate the kernel stack of a new task */
    t->ktask = kmalloc(sizeof(struct ktask));
    if ( NULL == t->ktask ) {
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }
    kmemset(t->ktask, 0, sizeof(struct ktask));
    t->ktask->arch = t;
    t->ktask->proc = np;
    t->ktask->state = KTASK_STATE_READY;
    t->ktask->signaled = 0;
    t->ktask->id = 0;
    t->ktask->proc_task_next = NULL;
    t->ktask->proc->tasks = t->ktask;
    t->ktask->next = NULL;
    /* Allocate the user stack of a new task */
    paddr1 = pmem_prim_alloc_superpages(PMEM_ZONE_LOWMEM,
                                        bitwidth(USTACK_SIZE / SUPERPAGESIZE));
    if ( NULL == paddr1 ) {
        kfree(t->ktask);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }
    /* For exec */
    size = t->ktask->proc->code_size;
    if ( size <= 0 ) {
        /* Invald code */
        pmem_prim_free_pages(paddr1);
        kfree(t->ktask);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }
    paddr2 = pmem_prim_alloc_superpages(PMEM_ZONE_LOWMEM,
                                        bitwidth(DIV_CEIL(size, SUPERPAGESIZE)));
    if ( NULL == paddr2 ) {
        pmem_prim_free_pages(paddr1);
        kfree(t->ktask);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }
    np->code_paddr = paddr2;

    t->ustack = ((struct arch_task *)ot->arch)->ustack;
    exec = kmalloc(CEIL(size, SUPERPAGESIZE));
    if ( NULL == exec ) {
        pmem_prim_free_pages(paddr2);
        pmem_prim_free_pages(paddr1);
        kfree(t->ktask);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }

    /* Copy the user stack to the temporary buffer */
    kmemcpy(exec, (void *)CODE_INIT, size);

    /* Copy the kernel stack */
    kmemcpy(t->kstack, ((struct arch_task *)ot->arch)->kstack, KSTACK_SIZE);

    /* Create a virtual memory space */
    np->vmem = vmem_space_create();
    if ( NULL == np->vmem ) {
        kfree(exec);
        pmem_prim_free_pages(paddr2);
        pmem_prim_free_pages(paddr1);
        kfree(t->ktask);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        kfree(np);
        return NULL;
    }

    /* FIXME: Tempoary... */
    for ( i = 0; i < (ssize_t)(USTACK_SIZE / SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(np->vmem, t->ustack + SUPERPAGE_ADDR(i),
                            paddr1 + SUPERPAGE_ADDR(i),
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            panic("FIXME a");
        }
    }
    for ( i = 0; i < (ssize_t)DIV_CEIL(size, SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(np->vmem, (void *)(CODE_INIT + SUPERPAGESIZE * i),
                            paddr2 + SUPERPAGESIZE * i,
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            panic("FIXME b");
        }
    }

    /* Save the current cr3: This must be done before copying the user stack to
       copy this variable to the new stack. */
    saved_cr3 = get_cr3();

    /* This function uses "user"-stack, not kernel stack because syscall does
       not switch the stack pointer.  Therefore, the user stack must be copied
       before swapping the page table.  The following function maps the physical
       pages of the user stack of new process to a certain virtual memory space,
       and copies the stack there. */
    void *ustack2copy = (void *)0x90000000ULL;
    for ( i = 0; i < (ssize_t)(USTACK_SIZE / SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(op->vmem, ustack2copy + SUPERPAGE_ADDR(i),
                            paddr1 + SUPERPAGE_ADDR(i),
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            panic("FIXME c");
        }
    }
    kmemcpy(ustack2copy, ((struct arch_task *)ot->arch)->ustack, USTACK_SIZE);

    set_cr3(((struct arch_vmem_space *)np->vmem->arch)->pgt);

    /* Copy the program memory */
    kmemcpy((void *)CODE_INIT, exec, size);

    /* Restore cr3 */
    set_cr3(saved_cr3);

    /* Setup the restart point */
    t->rp = (struct stackframe64 *)
        ((u64)((struct arch_task *)ot->arch)->rp + (u64)t->kstack
         - (u64)((struct arch_task *)ot->arch)->kstack);

    /* Free the temporary buffers */
    kfree(exec);

    t->cr3 = ((struct arch_vmem_space *)np->vmem->arch)->pgt;
    t->sp0 = (u64)t->kstack + KSTACK_SIZE - 16;

    /* Return */
    *ntp = t->ktask;
    return np;
}

/*
 * Create an idle task
 */
struct arch_task *
task_create_idle(void)
{
    struct arch_task *t;

    /* Allocate and initialize the architecture-specific kernel task */
    t = kmalloc(sizeof(struct arch_task));
    if ( NULL == t ) {
        return NULL;
    }
    kmemset(t, 0, sizeof(struct arch_task));

    /* Page table for the kernel */
    t->cr3 = ((struct arch_kmem_space *)g_kmem->space->arch)->cr3;

    /* Create a space for FPU/SSE registers */
    t->xregs = kmalloc(XSAVE_SIZE);
    if ( NULL == t->xregs ) {
        kfree(t);
        return NULL;
    }
    kmemset(t->xregs, 0, XSAVE_SIZE);
    //xsave(t->xregs, 5, 0);

    /* Kernel stack */
    t->kstack = kmalloc(KSTACK_SIZE);
    if ( NULL == t->kstack ) {
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    kmemset(t->kstack, 0, KSTACK_SIZE);

    /* User stack (in the kernel space) */
    t->ustack = kmalloc(USTACK_SIZE);
    if ( NULL == t->ustack ) {
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    kmemset(t->ustack, 0, USTACK_SIZE);

    /* Kernel task */
    t->ktask = kmalloc(sizeof(struct ktask));
    if ( NULL == t->ktask ) {
        kfree(t->ustack);
        kfree(t->kstack);
        kfree(t->xregs);
        kfree(t);
        return NULL;
    }
    kmemset(t->ktask, 0, sizeof(struct ktask));

    /* Create a bidirectional link */
    t->ktask->arch = t;

    /* No process associated with the idle task */
    t->ktask->proc = NULL;

    /* Set the task state to ready */
    t->ktask->state = KTASK_STATE_READY;
    t->ktask->signaled = 0;
    t->ktask->id = 0;

    /* Setup the restart point */
    t->rp = t->kstack + KSTACK_SIZE - 16 - sizeof(struct stackframe64);

    /* The idle task runs at ring 0. */
    t->rp->cs = GDT_RING0_CODE_SEL;
    t->rp->ss = GDT_RING0_DATA_SEL;

    /* Entry point, user/kernel stack, and flags of the idle task */
    t->rp->ip = (u64)arch_idle;
    t->rp->sp = (u64)t->ustack + USTACK_SIZE - 16;
    t->rp->flags = 0x0202;
    t->sp0 = (u64)t->kstack + KSTACK_SIZE - 16;

    return t;
}

/*
 * Create a process
 */
int
proc_create(const char *path, const char *name, pid_t pid)
{
    u64 *initramfs = (u64 *)KMEM_P2V(INITRAMFS_BASE);
    u64 offset = 0;
    u64 size;
    struct arch_task *t;
    struct ktask_list *l;
    struct proc *proc;
    void *ppage1;
    void *ppage2;
    u64 cs;
    u64 ss;
    u64 flags;
    int policy = KTASK_POLICY_USER;
    void *exec;
    void *saved_cr3;
    int ret;
    ssize_t i;

    /* Check the process table first */
    if ( NULL != g_proc_table->procs[pid] ) {
        /* The process is already exists */
        return -1;
    }

    /* Find the file pointed by path from the initramfs */
    while ( 0 != *initramfs ) {
        if ( 0 == kstrcmp((char *)initramfs, path) ) {
            offset = *(initramfs + 2);
            size = *(initramfs + 3);
            break;
        }
        initramfs += 4;
    }
    if ( 0 == offset ) {
        /* Could not find the process */
        return -1;
    }

    /* New process */
    proc = kmalloc(sizeof(struct proc));
    if ( NULL == proc ) {
        goto error_proc;
    }
    kmemset(proc, 0, sizeof(struct proc));

    /* Set the process name */
    kstrlcpy(proc->name, name, PATH_MAX);

    /* Set the policy */
    proc->policy = KTASK_POLICY_USER;

    /* Create a virtual memory space */
    proc->vmem = vmem_space_create();
    if ( NULL == proc->vmem ) {
        goto error_vmem;
    }
    proc->code_size = size;

    /* Process table */
    g_proc_table->procs[pid] = proc;
    g_proc_table->lastpid = pid;

    /* Create an architecture-specific task data structure */
    t = kmalloc(sizeof(struct arch_task));
    if ( NULL == t ) {
        goto error_arch_task;
    }
    kmemset(t, 0, sizeof(struct arch_task));

    /* Create a space for FPU/SSE registers */
    t->xregs = kmalloc(XSAVE_SIZE);
    if ( NULL == t->xregs ) {
        goto error_task;
    }
    kmemset(t->xregs, 0, XSAVE_SIZE);
    //xsave(t->xregs, 5, 0);

    /* Create a task */
    t->ktask = kmalloc(sizeof(struct ktask));
    if ( NULL == t->ktask ) {
        goto error_xregs;
    }
    kmemset(t->ktask, 0, sizeof(struct ktask));
    t->ktask->arch = t;
    t->ktask->proc_task_next = NULL;

    /* Associate the task with a process */
    t->ktask->proc = proc;
    t->ktask->proc->tasks = t->ktask;

    /* Prepare the kernel stack */
    t->kstack = kmalloc(KSTACK_SIZE);
    if ( NULL == t->kstack ) {
        goto error_kstack;
    }

    /* Prepare the user stack */
    ppage1 = pmem_prim_alloc_superpages(PMEM_ZONE_LOWMEM,
                                        bitwidth(USTACK_SIZE / SUPERPAGESIZE));
    if ( NULL == ppage1 ) {
        goto error_ustack;
    }

    /* Prepare exec */
    ppage2 = pmem_prim_alloc_superpages(PMEM_ZONE_LOWMEM,
                                        bitwidth(DIV_CEIL(size,
                                                          SUPERPAGESIZE)));
    if ( NULL == ppage2 ) {
        goto error_exec;
        return -1;
    }
    proc->code_paddr = ppage2;

    /* Set user stack */
    t->ustack = (void *)USTACK_INIT;
    for ( i = 0; i < (ssize_t)(USTACK_SIZE / SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(proc->vmem, t->ustack + SUPERPAGE_ADDR(i),
                            ppage1 + SUPERPAGE_ADDR(i),
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            panic("FIXME 1");
        }
    }
    exec = (void *)CODE_INIT;
    for ( i = 0; i < (ssize_t)DIV_CEIL(size, SUPERPAGESIZE); i++ ) {
        ret = arch_vmem_map(proc->vmem, exec + SUPERPAGESIZE * i,
                            ppage2 + SUPERPAGESIZE * i,
                            VMEM_USABLE | VMEM_USED | VMEM_SUPERPAGE);
        if ( ret < 0 ) {
            /* FIXME: Handle this error */
            panic("FIXME 2");
        }
    }

    /* Temporary set the page table to the user's one to copy the exec file from
       kernel to the user space */
    saved_cr3 = get_cr3();
    set_cr3(((struct arch_vmem_space *)proc->vmem->arch)->pgt);

    /* Copy the program from the initramfs to user space */
    (void)kmemcpy(exec, (void *)(KMEM_P2V(INITRAMFS_BASE) + offset), size);

    /* Restore CR3 */
    set_cr3(saved_cr3);

    /* Set the restart pointer and the task state */
    t->rp = t->kstack + KSTACK_SIZE - 16 - sizeof(struct stackframe64);
    kmemset(t->rp, 0, sizeof(struct stackframe64));
    t->ktask->state = KTASK_STATE_READY;
    t->ktask->signaled = 0;
    t->ktask->id = 0;

    /* Kernel task */
    l = kmalloc(sizeof(struct ktask_list));
    if ( NULL == l ) {
        goto error_tl;
    }
    l->ktask = t->ktask;
    l->next = NULL;
    /* Push */
    if ( NULL == g_ktask_root->r.head ) {
        g_ktask_root->r.head = l;
        g_ktask_root->r.tail = l;
    } else {
        g_ktask_root->r.tail->next = l;
        g_ktask_root->r.tail = l;
    }

    /* Configure the ring protection by the policy */
    switch ( policy ) {
    case KTASK_POLICY_KERNEL:
        cs = GDT_RING0_CODE_SEL;
        ss = GDT_RING0_DATA_SEL;
        flags = 0x0202;
        break;
    case KTASK_POLICY_DRIVER:
    case KTASK_POLICY_SERVER:
    case KTASK_POLICY_USER:
    default:
        cs = GDT_RING3_CODE64_SEL + 3;
        ss = GDT_RING3_DATA64_SEL + 3;
        flags = 0x3202;
        break;
    }

    t->sp0 = (u64)t->kstack + KSTACK_SIZE - 16;
    t->rp->gs = ss;
    t->rp->fs = ss;
    t->rp->sp = USTACK_INIT + USTACK_SIZE - 16 - 8;
    t->rp->ss = ss;
    t->rp->cs = cs;
    t->rp->ip = CODE_INIT;
    t->rp->flags = flags;
    t->cr3 = ((struct arch_vmem_space *)proc->vmem->arch)->pgt;

    return 0;

error_tl:
    pmem_prim_free_pages(ppage2);
error_exec:
    pmem_prim_free_pages(ppage1);
error_ustack:
    kfree(t->kstack);
error_kstack:
    kfree(t->ktask);
error_xregs:
    kfree(t->xregs);
error_task:
    kfree(t);
error_arch_task:
    vmem_space_delete(proc->vmem);
error_vmem:
    kfree(proc);
error_proc:
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
