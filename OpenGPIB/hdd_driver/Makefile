#
# Makefile for the Linux fat filesystem support.
#
KERNEL_SOURCE = /lib/modules/`uname -r`/build
CONFIG_FAT_FS=m
CONFIG_VFAT_FS=m
CONFIG_MSDOS_FS=m

obj-$(CONFIG_FAT_FS) += fat.o
obj-$(CONFIG_VFAT_FS) += vfat.o
obj-$(CONFIG_MSDOS_FS) += msdos.o

fat-y := cache.o dir.o fatent.o file.o inode.o misc.o
vfat-y := namei_vfat.o
msdos-y := namei_msdos.o

default:
	make -C  ${KERNEL_SOURCE} SUBDIRS=`pwd` modules

clean:
	-rm *.o *.ko *.mod.* modules.order Module.symvers
	
