#
# Copyright (c) 2015-2017 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

## Define version
VERSION = nightly-build-$(shell date +%Y%m%d-%H%M%S)

## Phony target
.PHONY: all image bootloader initrd kernel

## "make all" is not supported.
all:
	@echo "make all is not currently supported."

## Compile initramfs (including kernel as well)
initrd:
#	Compile programs
	VERSION=${VERSION} $(MAKE) -C src init pm tty pash pci fe
#       Create initramfs image
	@./create_initrd.sh \
		init:/servers/init \
		pm:/servers/pm \
		tty:/drivers/tty \
		pash:/bin/pash \
		pci:/drivers/pci \
		fe:/ids/fe

## Compile boot loader
bootloader:
#	Compile the initial program loader in MBR
	$(MAKE) -C src mbr
#	Compile the PXE boot loader
	$(MAKE) -C src pxeboot
#	Compile the boot monitor called from mbr
	$(MAKE) -C src bootmon

## Compile kernel
kernel:
	$(MAKE) -C src kpack

## Create FAT12/16 image
image: pix.img
pix.img: bootloader kernel initrd
#	Create the boot image
	@./create_image.sh \
		src/boot/mbr \
		src/boot/bootmon \
		src/kernel/kpack \
		initramfs

## VMDK
vmdk: pix.vmdk
pix.vmdk: pix.img
	cp pix.img pix.raw.img
	@printf '\000' | dd of=pix.raw.img bs=1 seek=268435455 conv=notrunc > /dev/null 2>&1
	qemu-img convert -f raw -O vmdk pix.img pix.vmdk
	rm -f pix.raw.img

## Test
test:
	$(MAKE) -C src/tests test-all

## Clean
clean:
	$(MAKE) -C src clean
	rm -f initramfs
	rm -f pix.img
