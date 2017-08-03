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

## Boot loader
mbr:
	ARCH=${ARCH} $(MAKE) -C boot mbr
bootmon:
	ARCH=${ARCH} $(MAKE) -C boot bootmon
pxeboot:
	ARCH=${ARCH} $(MAKE) -C boot pxeboot

## Kernel
kpack:
	ARCH=${ARCH} $(MAKE) -C kernel kpack

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
	rm -f mbr
