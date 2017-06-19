#
# Copyright (c) 2017 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

## Override the flags
diskboot: ASFLAGS=-nostdlib -I./arch/$(ARCH)/
bootmon: ASFLAGS=-nostdlib -I./arch/$(ARCH)/
bootmon: CFLAGS=-I../include \
	-Wall -fleading-underscore -nostdlib -nodefaultlibs -fno-builtin -O3 -m64

pxeboot: ASFLAGS=-nostdlib -I./arch/$(ARCH)/

## IPL
diskboot: arch/$(ARCH)/diskboot.o
	$(LD) -N -T mbr.ld -o $@ $^

## Boot monitor
bootmon: arch/$(ARCH)/bootmon.o \
	arch/$(ARCH)/kernload.o \
	arch/$(ARCH)/entry16.o \
	arch/$(ARCH)/entry32.o \
	arch/$(ARCH)/entry64.o \
	arch/$(ARCH)/boot.o
	$(LD) -N -T bootmon.ld -o $@ $^

## PXE boot
pxeboot: arch/$(ARCH)/pxeboot.o
	$(LD) -N -T pxe.ld -o $@ $^

