#
# Copyright (c) 2017 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

CFLAGS=-I../../../include \
	-Wall -fleading-underscore -mcmodel=large -nostdlib -nodefaultlibs \
	-fno-builtin -O3 -m64 -DARCH_X86_64=1

%.o: ../../../include/aos/const.h ../../kernel.h ../../../include/sys/syscall.h \
	arch.h acpi.h apic.h cmos.h const.h desc.h i8254.h memory.h vmx.h

archpack.a: asm.o arch.o desc.o acpi.o apic.o i8254.o memory.o trampoline.o \
	ap_entry32.o ap_entry64.o cpuid.o vmx.o task.o cmos.o clock_tsc.o
	$(AR) r $@ $^
