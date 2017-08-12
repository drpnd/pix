#
# Copyright (c) 2017 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

CFLAGS=-I../include \
	-Wall -fleading-underscore -mcmodel=large -nostdlib -nodefaultlibs \
	-fno-builtin -O3 -m64 -DARCH_X86_64=1

.PHONY: arch/$(ARCH)/archpack.a

## Override the flags
kpack: ASFLAGS=-nostdlib -I./arch/$(ARCH)/
kpack: CFLAGS=-I../include \
	-Wall -fleading-underscore -mcmodel=large -nostdlib -nodefaultlibs \
	-fno-builtin -O3 -m64 -DARCH_X86_64=1

%.o: ../include/aos/const.h kernel.h clock.h elf.h ktimer.h rbtree.h

arch/$(ARCH)/archpack.a:
	$(MAKE) -C arch/$(ARCH) archpack.a

kpack: arch/$(ARCH)/archpack.a \
	kernel.o ktimer.o clock.o memory.o pmem.o kmem.o vmem.o \
	strfmt.o sched.o exec.o elf.o rbtree.o mpq.o ramfs.o devfs.o \
	syscall.o syspix.o sysdriver.o
	$(LD) -N -T kernel.ld -o $@ $^
	$(LD) -N -T kerneldebug.ld -o kpack.dbg $^

