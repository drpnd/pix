#
# Copyright (c) 2015-2016 Hirochika Asai
# All rights reserved.
#
# Authors:
#      Hirochika Asai  <asai@jar.jp>
#

## Define version
VERSION = nightly-build-$(shell date +%Y%m%d-%H%M%S)

## "make all" is not supported.
all:
	@echo "make all is not currently supported."

## Compile initramfs (including kernel as well)
initramfs:
#	Compile programs
	VERSION=${VERSION} make -C src init pm tty pash pci fe
#       Create an image
	@./create_initrd.sh \
		init:/servers/init \
		pm:/servers/pm \
		tty:/drivers/tty \
		pash:/bin/pash \
		pci:/drivers/pci \
		fe:/ids/fe

## Compile boot loader
bootloader: src/diskboot src/pxeboot src/bootmon
src/diskboot:
#	Compile the initial program loader in MBR
	make -C src diskboot
src/pxeboot:
#	Compile the PXE boot loader
	make -C src pxeboot
src/bootmon:
#	Compile the boot monitor called from diskboot
	make -C src bootmon

## Compile kernel
src/kernel/kpack:
	make -C src kpack

## Create FAT12/16 image
image: pix.img
pix.img: bootloader src/kernel/kpack initramfs
#	Create the boot image
	@./create_image.sh \
		src/boot/diskboot \
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
	make -C src/tests test-all

## Clean
clean:
	make -C src clean
	rm -f initramfs
	rm -f pix.img
