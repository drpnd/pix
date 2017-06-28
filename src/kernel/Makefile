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

## Override the flags
kpack: ASFLAGS=-nostdlib -I./arch/$(ARCH)/
kpack: CFLAGS=-I../include \
	-Wall -fleading-underscore -mcmodel=large -nostdlib -nodefaultlibs \
	-fno-builtin -O3 -m64 -DARCH_X86_64=1

arch/$(ARCH)/arch.o: ../include/aos/const.h kernel.h \
	arch/$(ARCH)/arch.h \
	../include/sys/syscall.h
arch/$(ARCH)/desc.o: ../include/aos/const.h kernel.h \
	arch/$(ARCH)/arch.h \
	arch/$(ARCH)/desc.h
arch/$(ARCH)/acpi.o: ../include/aos/const.h kernel.h \
	arch/$(ARCH)/arch.h \
	arch/$(ARCH)/acpi.h
arch/$(ARCH)/i8254.o: ../include/aos/const.h kernel.h \
	arch/$(ARCH)/arch.h \
	arch/$(ARCH)/i8254.h
kernel.o: ../include/aos/const.h kernel.h
ramfs.o: ../include/aos/const.h kernel.h
syscall.o: ../include/aos/const.h kernel.h
kpack: arch/$(ARCH)/asm.o \
	arch/$(ARCH)/arch.o \
	arch/$(ARCH)/desc.o \
	arch/$(ARCH)/acpi.o \
	arch/$(ARCH)/apic.o \
	arch/$(ARCH)/i8254.o \
	arch/$(ARCH)/memory.o \
	arch/$(ARCH)/trampoline.o \
	arch/$(ARCH)/ap_entry32.o \
	arch/$(ARCH)/ap_entry64.o \
	arch/$(ARCH)/cpuid.o \
	arch/$(ARCH)/vmx.o \
	arch/$(ARCH)/task.o \
	arch/$(ARCH)/cmos.o \
	arch/$(ARCH)/ktimer_tsc.o \
	kernel.o \
	ktimer.o \
	memory.o \
	pmem.o \
	kmem.o \
	vmem.o \
	strfmt.o \
	sched.o \
	elf.o \
	rbtree.o \
	mpq.o \
	ramfs.o \
	devfs.o \
	syscall.o \
	syspix.o \
	sysdriver.o
	$(LD) -N -T kernel.ld -o $@ $^
	$(LD) -N -T kerneldebug.ld -o kpack.dbg $^

