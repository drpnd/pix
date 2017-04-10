#
# Copyright (c) 2015-2017 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

ARCH=x86_64

## Global flags
ASFLAGS=-nostdlib -I./kernel/arch/$(ARCH)/
CFLAGS=-I./include -Wall -fleading-underscore -nostdinc -nostdlib -O3 -m64 \
	-DARCH_X86_64=1 -DPIX_VERSION='"$(VERSION)"'

## Override the flags
diskboot: ASFLAGS=-nostdlib -I./boot/arch/$(ARCH)/
bootmon: ASFLAGS=-nostdlib -I./boot/arch/$(ARCH)/
bootmon: CFLAGS=-I./include \
	-Wall -fleading-underscore -nostdlib -nodefaultlibs -fno-builtin -O3 -m64

pxeboot: ASFLAGS=-nostdlib -I./boot/arch/$(ARCH)/

kpack: ASFLAGS=-nostdlib -I./kernel/arch/$(ARCH)/
kpack: CFLAGS=-I./include \
	-Wall -fleading-underscore -mcmodel=large -nostdlib -nodefaultlibs \
	-fno-builtin -O3 -m64 -DARCH_X86_64=1

## IPL
diskboot: boot/arch/$(ARCH)/diskboot.o
	$(LD) -N -e start -Ttext=0x7c00 --oformat binary -o $@ $^

## Boot monitor
bootmon: boot/arch/$(ARCH)/bootmon.o \
	boot/arch/$(ARCH)/kernload.o \
	boot/arch/$(ARCH)/entry16.o \
	boot/arch/$(ARCH)/entry32.o \
	boot/arch/$(ARCH)/entry64.o \
	boot/arch/$(ARCH)/boot.o
	$(LD) -N -e bootmon -Ttext=0x9000 --oformat binary -o $@ $^

## PXE boot
pxeboot: boot/arch/$(ARCH)/pxeboot.o
	$(LD) -N -e pxeboot -Ttext=0x7c00 --oformat binary -o $@ $^

## Kernel
kpack:
	ARCH=${ARCH} make -C kernel kpack

## libc
LIBCOBJS=lib/arch/$(ARCH)/libc.o lib/arch/$(ARCH)/libcasm.o lib/stdio/fio.o \
	lib/stdio/print.o lib/string/str.o
LIBPIXOBJS=lib/pix/libpix.o
lib/arch/$(ARCH)/libc.o: lib/crt0.o

## init server
init: servers/init/init.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## process manager
pm: servers/pm/pm.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## forwarding engine
fe: ids/fe/fe.o ids/fe/i40e.o ids/fe/pci.o $(LIBCOBJS) $(LIBPIXOBJS) lib/driver.o
	$(LD) -T app.ld -o $@ $^
	$(LD) -T appdebug.ld -o $@.dbg $^

## file system
fs: servers/fs/fs.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## TCP server
tcp: servers/tcp/tcp_main.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## tty driver (including keyboard, video, and serial drivers)
tty: drivers/tty/tty.o drivers/tty/console.o drivers/tty/kbd.o \
	drivers/tty/serial.o drivers/tty/linebuf.o $(LIBCOBJS) lib/driver.o
	$(LD) -T app.ld -o $@ $^

## shell
pash: bin/pash/pash.o bin/pash/mod_cpu.o bin/pash/mod_clock.o \
	bin/pash/mod_system.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## PCI driver
pci: drivers/pci/pci.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## VMX driver
vmx: drivers/vmx/vmx.o $(LIBCOBJS)
	$(LD) -T app.ld -o $@ $^

## Clean
clean:
	find . -name "*.o" | xargs rm -f
	rm -f init
	rm -f pm
	rm -f kpack
	rm -f bootmon
	rm -f diskboot
